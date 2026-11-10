#include "defines.h"
#include "../include/bg.h"
#include "../include/berry.h"
#include "../include/field_weather.h"
#include "../include/scanline_effect.h"
#include "../include/text_window.h"
#include "../include/gpu_regs.h"
#include "../include/item_menu.h"
#include "../include/list_menu.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/menu.h"
#include "../include/menu_helpers.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/decompress.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/field_screen_effect.h"
#include "../include/constants/songs.h"
#include "../include/constants/items.h"
#include "../include/new/Vanilla_functions.h"
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"
#include "hack_blender_calc.h"
#include "hack_pokeblock_use.h"

extern bool8 GivePokeblockToContestLady(struct Pokeblock* pokeblock);   //src/hack_lilycove_lady.c（コンテストのおねえさん）
#include "hack_safari_pokeblock.h"

/*
 * ポロックケースの画面（2026-11-09 FEAT-pokeblock 段 2・patches/0978）。
 * 原文: 参考/pokeemerald/src/pokeblock.c（OpenPokeblockCase 以下の画面）。gSaveBlock1Ptr->pokeblocks[i] は段 1 の PokeblockImage
 * （この画面を開いている間 AllocZeroed して Pokeblock_Load・変えたら Pokeblock_Commit）に読み替えた。追加・外す・捨てるは Pokeblock_Add／Remove。
 * 原文との差（docs: work/tmp/pb/stage2_plan.md の突き合わせ表）:
 *   - 前回の行・スクロール（sSavedPokeblockData）は static が書けない（6.13）ので SaveBlock1 の 4 バイト＝台帳の「ポロックケースの前回の位置」（Pokeblock_Get/SetCasePos・直には触らない）。
 *   - FONT_NARROW は素の FR に無い → FONT_NORMAL（2）。窓の枠・メッセージ枠・アクション窓・はい／いいえは素の FR の同等品。
 *   - 通信中の待ち（MenuHelpers_ShouldWaitForLinkRecv）は省く（入口がバッグ・登録のフィールドだけ・段 7 で再考）。
 *   - 「つかう」→ポケモンを選ぶ画面（ChooseMonToGivePokeblock）は段 3（src/hack_pokeblock_use.c）。戦闘・フィーダー・あげる は段 4 以降＝今は何もせずケースへ戻る。
 * 状態は Alloc した入れ物（EWRAM）。ポインタの語は 0x203F044（段 6 で 0x203E038 から変えた＝戦闘中の gNewBS と重ならない・RAM 台帳 docs/research/flag-ledger.md）。
 */

#define PBC_STATE_PTR 0x203F044   //★ 段 6: 戦闘中はこの語を使う（0x203E038 は戦闘中 gNewBS＝上書きすると戦闘が壊れる）。全部の場面でこの語に統一
#define sPbc (*(struct PokeblockMenu**) PBC_STATE_PTR)

#define MAX_MENU_ITEMS 9
#define MENU_MIDPOINT (MAX_MENU_ITEMS / 2)

#define TILE_HIGHLIGHT_NONE 0x0005
#define TILE_HIGHLIGHT_BLUE 0x1005
#define TILE_HIGHLIGHT_RED  0x2005

#define TAG_POKEBLOCK_CASE 14800
#define TAG_SCROLL_ARROW   1110
#define TAG_SWAP_LINE      0x2722

#define POKEBLOCK_MAX_FEEL 99
#define NUM_SWAP_LINE_SPRITES 7
#define PBC_FONT 2                       //素の FR の FONT_NORMAL
#define PBC_ALL_PALETTES 0xFFFFFFFFu

//素の FR（JP）の番地（BPRJ.ld に無いもの・tools/rom/verify_pokeblock_case.py が逆アセンブルで確かめる）
#define Pbc_DisplayMessageAndContinueTask ((void (*)(u8, u8, u16, u8, u8, u8, const u8*, void*))(0x080C0644 + 1))
#define Pbc_CreateYesNoMenuWithCallbacks ((void (*)(u8, const struct WindowTemplate*, u8, u8, u8, u16, u8, const struct YesNoFuncTable*))(0x080C07A0 + 1))
#define Pbc_SetFieldCallback2ForItemUse ((void (*)(void))(0x080A2458 + 1))
#define Pbc_SetVBlankHBlankCallbacksToNull ((void (*)(void))(0x080C0938 + 1))
#define Pbc_DisplayCannotUse ((void (*)(u8, u8, u8, const u8*))(0x080A2398 + 1))   //素の PrintNotTheTimeToUseThat（バッグなら DisplayItemMessageInBag・登録からなら DisplayItemMessageOnField で文を出して元の場所へ戻る）
#define Pbc_RemoveScrollIndicatorArrowPair ((void (*)(u8))(0x0813441C + 1))

enum
{
	WIN_TITLE, WIN_LIST, WIN_SPICY, WIN_DRY, WIN_SWEET, WIN_BITTER, WIN_SOUR, WIN_FEEL,
	WIN_ACTIONS_TALL, WIN_ACTIONS, WIN_TOSS_MSG,
};

enum { PKBL_USE_ON_FIELD, PKBL_TOSS, PKBL_CANCEL, PKBL_USE_IN_BATTLE, PKBL_USE_ON_FEEDER, PKBL_GIVE_TO_LADY };

struct PokeblockMenu
{
	u8 tilemap[0x800];
	MainCallback callbackOnUse;
	MainCallback returnCallback;
	const u8* pokeblockActionIds;
	struct PokeblockImage* image;
	u8 numActions;
	u8 caseId;
	u8 returnKind;
	u8 itemsNo;
	u8 maxShowed;
	u8 pokeblockCaseSpriteId;
	u8 swapLineSpriteIds[NUM_SWAP_LINE_SPRITES];
	u8 arrowTaskId;
	u8 isSwapping;
	u8 loadState;
	u8 usable;
	u16 selectedRow;
	u16 scrollOffset;
	struct ListMenuItem items[POKEBLOCK_CASE_SLOTS + 1];
	u8 menuItemsStrings[POKEBLOCK_CASE_SLOTS + 1][32];
};

extern const u8 gPokeblockMenuTiles[], gPokeblockMenuMap[], gPokeblockMenuPal[], gPokeblockDeviceTiles[], gPokeblockDevicePal[];
extern const u8 gPokeblockSwapLineTiles[], gPokeblockSwapLinePal[];
extern const u8 gText_Pb_Red[], gText_Pb_Blue[], gText_Pb_Pink[], gText_Pb_Green[], gText_Pb_Yellow[], gText_Pb_Purple[], gText_Pb_Indigo[];
extern const u8 gText_Pb_Brown[], gText_Pb_LiteBlue[], gText_Pb_Olive[], gText_Pb_Gray[], gText_Pb_Black[], gText_Pb_White[], gText_Pb_Gold[];
extern const u8 gText_Pb_Spicy[], gText_Pb_Dry[], gText_Pb_Sweet[], gText_Pb_Bitter[], gText_Pb_Sour[], gText_Pb_StowCase[];
extern const u8 gText_Pb_Use[], gText_Pb_Toss[], gText_Pb_Cancel[], gText_Pb_Give[], gText_Pb_ThrowAway[], gText_Pb_ThrownAway[];
extern const u8 gText_Pb_NoExt[], gText_Pb_NeedReport[], gText_Pb_BadData[];

const s8 gPokeblockFlavorCompatibilityTable[NUM_NATURES * FLAVOR_COUNT] =
{
	//からい,しぶい,あまい,にがい,すっぱい
	0, 0, 0, 0, 0,		//がんばりや
	1, 0, 0, 0, -1,		//さみしがり
	1, 0, -1, 0, 0,		//ゆうかん
	1, -1, 0, 0, 0,		//いじっぱり
	1, 0, 0, -1, 0,		//やんちゃ
	-1, 0, 0, 0, 1,		//ずぶとい
	0, 0, 0, 0, 0,		//すなお
	0, 0, -1, 0, 1,		//のんき
	0, -1, 0, 0, 1,		//わんぱく
	0, 0, 0, -1, 1,		//のうてんき
	-1, 0, 1, 0, 0,		//おくびょう
	0, 0, 1, 0, -1,		//せっかち
	0, 0, 0, 0, 0,		//まじめ
	0, -1, 1, 0, 0,		//ようき
	0, 0, 1, -1, 0,		//むじゃき
	-1, 1, 0, 0, 0,		//ひかえめ
	0, 1, 0, 0, -1,		//おっとり
	0, 1, -1, 0, 0,		//れいせい
	0, 0, 0, 0, 0,		//てれや
	0, 1, 0, -1, 0,		//うっかりや
	-1, 0, 0, 1, 0,		//おだやか
	0, 0, 0, 1, -1,		//おとなしい
	0, 0, -1, 1, 0,		//なまいき
	0, -1, 0, 1, 0,		//しんちょう
	0, 0, 0, 0, 0,		//きまぐれ
};

static const struct BgTemplate sBgTemplates[] =
{
	{.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
	{.bg = 1, .charBaseIndex = 0, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
	{.bg = 2, .charBaseIndex = 3, .mapBaseIndex = 29, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
};

const u8* const gPokeblockNames[] =
{
	[PBLOCK_CLR_NONE] = NULL,
	[PBLOCK_CLR_RED] = gText_Pb_Red,
	[PBLOCK_CLR_BLUE] = gText_Pb_Blue,
	[PBLOCK_CLR_PINK] = gText_Pb_Pink,
	[PBLOCK_CLR_GREEN] = gText_Pb_Green,
	[PBLOCK_CLR_YELLOW] = gText_Pb_Yellow,
	[PBLOCK_CLR_PURPLE] = gText_Pb_Purple,
	[PBLOCK_CLR_INDIGO] = gText_Pb_Indigo,
	[PBLOCK_CLR_BROWN] = gText_Pb_Brown,
	[PBLOCK_CLR_LITE_BLUE] = gText_Pb_LiteBlue,
	[PBLOCK_CLR_OLIVE] = gText_Pb_Olive,
	[PBLOCK_CLR_GRAY] = gText_Pb_Gray,
	[PBLOCK_CLR_BLACK] = gText_Pb_Black,
	[PBLOCK_CLR_WHITE] = gText_Pb_White,
	[PBLOCK_CLR_GOLD] = gText_Pb_Gold,
};

static void PokeblockAction_UseOnField(u8 taskId);
static void PokeblockAction_Toss(u8 taskId);
static void PokeblockAction_Cancel(u8 taskId);
static void PokeblockAction_UseInBattle(u8 taskId);
static void PokeblockAction_UseOnFeeder(u8 taskId);
static void PokeblockAction_GiveToContestLady(u8 taskId);
static void TossedPokeblockMessage(u8 taskId);
static void CloseTossPokeblockWindow(u8 taskId);
static void Task_FreeDataAndExitPokeblockCase(u8 taskId);
static void Task_HandlePokeblockActionsInput(u8 taskId);
static void Task_HandlePokeblocksSwapInput(u8 taskId);
static void Task_HandlePokeblockMenuInput(u8 taskId);
static void SpriteCB_ShakePokeblockCase(struct Sprite* sprite);
static void MovePokeblockMenuCursor(s32 pkblId, bool8 onInit, struct ListMenu* list);
static void CB2_InitPokeblockMenu(void);
static void UsePokeblockOnField(void);
static void ReturnToPokeblockCaseOnField(void);
static void CreateTossPokeblockYesNoMenu(u8 taskId);
static void ShowPokeblockActionsWindow(u8 taskId);
static void TossPokeblock(u8 taskId);

static const struct MenuAction sPokeblockMenuActions[] =
{
	[PKBL_USE_ON_FIELD] = {gText_Pb_Use, {PokeblockAction_UseOnField}},
	[PKBL_TOSS] = {gText_Pb_Toss, {PokeblockAction_Toss}},
	[PKBL_CANCEL] = {gText_Pb_Cancel, {PokeblockAction_Cancel}},
	[PKBL_USE_IN_BATTLE] = {gText_Pb_Use, {PokeblockAction_UseInBattle}},
	[PKBL_USE_ON_FEEDER] = {gText_Pb_Use, {PokeblockAction_UseOnFeeder}},
	[PKBL_GIVE_TO_LADY] = {gText_Pb_Give, {PokeblockAction_GiveToContestLady}},
};

static const u8 sActionsOnField[] = {PKBL_USE_ON_FIELD, PKBL_TOSS, PKBL_CANCEL};
static const u8 sActionsInBattle[] = {PKBL_USE_IN_BATTLE, PKBL_CANCEL};
static const u8 sActionsOnPokeblockFeeder[] = {PKBL_USE_ON_FEEDER, PKBL_CANCEL};
static const u8 sActionsWhenGivingToLady[] = {PKBL_GIVE_TO_LADY, PKBL_CANCEL};

static const struct YesNoFuncTable sTossYesNoFuncTable = {TossedPokeblockMessage, CloseTossPokeblockWindow};

static const struct OamData sOamData_PokeblockCase =
{
	.affineMode = ST_OAM_AFFINE_OFF,
	.objMode = ST_OAM_OBJ_NORMAL,
	.shape = SPRITE_SHAPE(64x64),
	.size = SPRITE_SIZE(64x64),
	.priority = 2,
};

static const union AnimCmd sSpriteAnim_PokeblockCase[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd* const sSpriteAnimTable_PokeblockCase[] = {sSpriteAnim_PokeblockCase};

static const union AffineAnimCmd sAffineAnim_PokeblockCaseShake[] =
{
	AFFINEANIMCMD_FRAME(0, 0, -2, 2),
	AFFINEANIMCMD_FRAME(0, 0, 2, 4),
	AFFINEANIMCMD_FRAME(0, 0, -2, 4),
	AFFINEANIMCMD_FRAME(0, 0, 2, 2),
	AFFINEANIMCMD_END
};
static const union AffineAnimCmd* const sAffineAnims_PokeblockCaseShake[] = {sAffineAnim_PokeblockCaseShake};

static const struct CompressedSpriteSheet sPokeblockCase_SpriteSheet = {gPokeblockDeviceTiles, 0x800, TAG_POKEBLOCK_CASE};
static const struct SpritePalette sPokeblockCase_SpritePal = {(const u16*) gPokeblockDevicePal, TAG_POKEBLOCK_CASE};

static const struct SpriteTemplate sSpriteTemplate_PokeblockCase =
{
	.tileTag = TAG_POKEBLOCK_CASE,
	.paletteTag = TAG_POKEBLOCK_CASE,
	.oam = &sOamData_PokeblockCase,
	.anims = sSpriteAnimTable_PokeblockCase,
	.images = NULL,
	.affineAnims = gDummySpriteAffineAnimTable,
	.callback = SpriteCallbackDummy,
};

//食べさせる演出（src/hack_pokeblock_feed.c・原文 pokeblock_feed.c が使う CreatePokeblockCaseSprite と gPokeblockCase_SpriteSheet／SpritePal）に貸す
u8 CreatePokeblockCaseSprite(s16 x, s16 y, u8 subpriority)
{
	return CreateSprite(&sSpriteTemplate_PokeblockCase, x, y, subpriority);
}

void PokeblockCase_LoadSpriteGfx(void)
{
	LoadCompressedSpriteSheet(&sPokeblockCase_SpriteSheet);
	LoadSpritePalette(&sPokeblockCase_SpritePal);
}

//入れ替えの線（原文 menu_helpers.c の SwapLine・素の FR に無い）
static const struct OamData sOamData_SwapLine =
{
	.affineMode = ST_OAM_AFFINE_OFF,
	.objMode = ST_OAM_OBJ_NORMAL,
	.shape = SPRITE_SHAPE(16x16),
	.size = SPRITE_SIZE(16x16),
	.priority = 0,
};
static const union AnimCmd sAnim_SwapLine_RightArrow[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd sAnim_SwapLine_Line[] = {ANIMCMD_FRAME(4, 0), ANIMCMD_END};
static const union AnimCmd sAnim_SwapLine_LeftArrow[] = {ANIMCMD_FRAME(0, 0, .hFlip = TRUE), ANIMCMD_END};
static const union AnimCmd* const sAnims_SwapLine[] = {sAnim_SwapLine_RightArrow, sAnim_SwapLine_Line, sAnim_SwapLine_LeftArrow};
static const struct CompressedSpriteSheet sSpriteSheet_SwapLine = {gPokeblockSwapLineTiles, 0x100, TAG_SWAP_LINE};
static const struct SpritePalette sSpritePalette_SwapLine = {(const u16*) gPokeblockSwapLinePal, TAG_SWAP_LINE};
static const struct SpriteTemplate sSpriteTemplate_SwapLine =
{
	.tileTag = TAG_SWAP_LINE,
	.paletteTag = TAG_SWAP_LINE,
	.oam = &sOamData_SwapLine,
	.anims = sAnims_SwapLine,
	.images = NULL,
	.affineAnims = gDummySpriteAffineAnimTable,
	.callback = SpriteCallbackDummy,
};

static const u8 sTextColor[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GREY, TEXT_COLOR_LIGHT_GREY};

static const struct Pokeblock sFavoritePokeblocksTable[FLAVOR_COUNT] =
{
	[FLAVOR_SPICY] = {PBLOCK_CLR_RED, 20, 0, 0, 0, 0, 20},
	[FLAVOR_DRY] = {PBLOCK_CLR_BLUE, 0, 20, 0, 0, 0, 20},
	[FLAVOR_SWEET] = {PBLOCK_CLR_PINK, 0, 0, 20, 0, 0, 20},
	[FLAVOR_BITTER] = {PBLOCK_CLR_GREEN, 0, 0, 0, 20, 0, 20},
	[FLAVOR_SOUR] = {PBLOCK_CLR_YELLOW, 0, 0, 0, 0, 20, 20},
};

static const struct WindowTemplate sWindowTemplates[] =
{
	[WIN_TITLE] = {.bg = 0, .tilemapLeft = 2, .tilemapTop = 1, .width = 9, .height = 2, .paletteNum = 15, .baseBlock = 0x1E},
	[WIN_LIST] = {.bg = 0, .tilemapLeft = 15, .tilemapTop = 1, .width = 14, .height = 18, .paletteNum = 15, .baseBlock = 0x30},
	[WIN_SPICY] = {.bg = 0, .tilemapLeft = 2, .tilemapTop = 13, .width = 5, .height = 2, .paletteNum = 15, .baseBlock = 0x12C},
	[WIN_DRY] = {.bg = 0, .tilemapLeft = 2, .tilemapTop = 15, .width = 5, .height = 2, .paletteNum = 15, .baseBlock = 0x136},
	[WIN_SWEET] = {.bg = 0, .tilemapLeft = 2, .tilemapTop = 17, .width = 5, .height = 2, .paletteNum = 15, .baseBlock = 0x140},
	[WIN_BITTER] = {.bg = 0, .tilemapLeft = 8, .tilemapTop = 13, .width = 5, .height = 2, .paletteNum = 15, .baseBlock = 0x14A},
	[WIN_SOUR] = {.bg = 0, .tilemapLeft = 8, .tilemapTop = 15, .width = 5, .height = 2, .paletteNum = 15, .baseBlock = 0x154},
	[WIN_FEEL] = {.bg = 0, .tilemapLeft = 11, .tilemapTop = 17, .width = 2, .height = 2, .paletteNum = 15, .baseBlock = 0x15E},
	[WIN_ACTIONS_TALL] = {.bg = 1, .tilemapLeft = 7, .tilemapTop = 5, .width = 6, .height = 6, .paletteNum = 15, .baseBlock = 0x162},
	[WIN_ACTIONS] = {.bg = 1, .tilemapLeft = 7, .tilemapTop = 7, .width = 6, .height = 4, .paletteNum = 15, .baseBlock = 0x186},
	[WIN_TOSS_MSG] = {.bg = 1, .tilemapLeft = 2, .tilemapTop = 15, .width = 27, .height = 4, .paletteNum = 15, .baseBlock = 0x19E},
	DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sTossPkblockWindowTemplate =
	{.bg = 1, .tilemapLeft = 21, .tilemapTop = 9, .width = 5, .height = 4, .paletteNum = 15, .baseBlock = 0x20A};

static const struct ListMenuTemplate sPokeblockListMenuTemplate =
{
	.items = NULL,
	.moveCursorFunc = MovePokeblockMenuCursor,
	.itemPrintFunc = NULL,
	.totalItems = 0,
	.maxShowed = 0,
	.windowId = WIN_LIST,
	.header_X = 0,
	.item_X = 1,
	.cursor_X = 0,
	.upText_Y = 1,
	.cursorPal = 2,
	.fillValue = 0,
	.cursorShadowPal = 3,
	.lettersSpacing = 0,
	.itemVerticalPadding = 0,
	.scrollMultiple = LIST_MULTIPLE_SCROLL_DPAD,
	.fontId = PBC_FONT,
	.cursorKind = 1,   //CURSOR_INVISIBLE
};

//---------------------------------------------------------------- 数字・表（原文の Get 系）

s16 GetPokeblockData(const struct Pokeblock* pokeblock, u8 field)
{
	if (field == PBLOCK_COLOR)
		return pokeblock->color;
	if (field == PBLOCK_SPICY)
		return pokeblock->spicy;
	if (field == PBLOCK_DRY)
		return pokeblock->dry;
	if (field == PBLOCK_SWEET)
		return pokeblock->sweet;
	if (field == PBLOCK_BITTER)
		return pokeblock->bitter;
	if (field == PBLOCK_SOUR)
		return pokeblock->sour;
	if (field == PBLOCK_FEEL)
		return pokeblock->feel;
	return 0;
}

u8 GetHighestPokeblocksFlavorLevel(const struct Pokeblock* pokeblock)
{
	u8 i;
	u8 maxFlavor = GetPokeblockData(pokeblock, PBLOCK_SPICY);

	for (i = PBLOCK_SPICY; i < FLAVOR_COUNT; i++)
	{
		u8 currFlavor = GetPokeblockData(pokeblock, PBLOCK_SPICY + i);

		if (maxFlavor < currFlavor)
			maxFlavor = currFlavor;
	}
	return maxFlavor;
}

u8 GetPokeblocksFeel(const struct Pokeblock* pokeblock)
{
	u8 feel = GetPokeblockData(pokeblock, PBLOCK_FEEL);

	if (feel > POKEBLOCK_MAX_FEEL)
		feel = POKEBLOCK_MAX_FEEL;
	return feel;
}

s16 PokeblockGetGain(u8 nature, const struct Pokeblock* pokeblock)
{
	u8 flavor;
	s16 curGain, totalGain = 0;

	for (flavor = 0; flavor < FLAVOR_COUNT; flavor++)
	{
		curGain = GetPokeblockData(pokeblock, flavor + PBLOCK_SPICY);
		if (curGain > 0)
			totalGain += curGain * gPokeblockFlavorCompatibilityTable[FLAVOR_COUNT * nature + flavor];
	}
	return totalGain;
}

void PokeblockCopyName(const struct Pokeblock* pokeblock, u8* dest)
{
	u8 color = GetPokeblockData(pokeblock, PBLOCK_COLOR);

	StringCopy(dest, gPokeblockNames[color]);
}

bool8 CopyMonFavoritePokeblockName(u8 nature, u8* dest)
{
	u8 i;

	for (i = 0; i < FLAVOR_COUNT; i++)
	{
		if (PokeblockGetGain(nature, &sFavoritePokeblocksTable[i]) > 0)
		{
			StringCopy(dest, gPokeblockNames[i + 1]);
			return TRUE;
		}
	}
	return FALSE;
}

u8 GetPokeblocksFlavor(const struct Pokeblock* pokeblock)
{
	s16 bestFlavor = 0;
	s16 i;

	for (i = 0; i < FLAVOR_COUNT; i++)
	{
		if (GetPokeblockData(pokeblock, bestFlavor + 1) < GetPokeblockData(pokeblock, i + 1))
			bestFlavor = i;
	}
	return bestFlavor;
}

//---------------------------------------------------------------- 入れ物（SaveBlock1 の前回の位置・台帳の「ポロックケースの前回の位置」）

static MainCallback CallbackForKind(u8 kind)
{
	switch (kind)
	{
	case PBC_RET_FIELD: return CB2_ReturnToField;
	case PBC_RET_SCRIPT: return CB2_ReturnToFieldContinueScript;    //台本の special から（おき場・waitstate で待つ台本が続く）
	case PBC_RET_BATTLE: return (MainCallback) (0x08076EFC + 1);    //素の ReshowBattleScreenAfterMenu（戦闘の画面を作り直して BattleMainCB2 へ）
	default: return CB2_BagMenuFromStartMenu;
	}
}

void ResetPokeblockScrollPositions(void)
{
	Pokeblock_SetCasePos(0, 0);
}

void CompactPokeblockSlots(struct PokeblockImage* image)
{
	u16 i, j;

	for (i = 0; i < POKEBLOCK_CASE_SLOTS - 1; i++)
	{
		for (j = i + 1; j < POKEBLOCK_CASE_SLOTS; j++)
		{
			if (image->blocks[i].color == PBLOCK_CLR_NONE)
			{
				struct Pokeblock temp = image->blocks[i];
				image->blocks[i] = image->blocks[j];
				image->blocks[j] = temp;
			}
		}
	}
}

static void SwapPokeblockMenuItems(struct PokeblockImage* image, u32 id1, u32 id2)
{
	s16 i, count;
	struct Pokeblock* pokeblocks = image->blocks;
	struct Pokeblock copy1;

	if (id1 == id2)
		return;
	copy1 = pokeblocks[id1];
	if (id2 > id1)
	{
		id2--;
		for (count = id2, i = id1; i < count; i++)
			pokeblocks[i] = pokeblocks[i + 1];
	}
	else
	{
		for (count = id2, i = id1; i > count; i--)
			pokeblocks[i] = pokeblocks[i - 1];
	}
	pokeblocks[id2] = copy1;
}

//---------------------------------------------------------------- 画面の組み立て

void TransferPlttBuffer(void);                 //src/dns.c

static void CB2_PokeblockMenu(void)
{
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	DoScheduledBgTilemapCopiesToVram();
	UpdatePaletteFade();
}

static void VBlankCB_PokeblockMenu(void)
{
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

static void PrintOnPokeblockWindow(u8 windowId, const u8* string, s32 x)
{
	AddTextPrinterParameterized4(windowId, PBC_FONT, x, 1, 0, 0, sTextColor, 0, string);
}

static void DrawPokeblockMenuHighlight(u16 cursorPos, u16 tileNum)
{
	FillBgTilemapBufferRect_Palette0(2, tileNum, 0xF, (cursorPos * 2) + 1, 0xE, 2);
	ScheduleBgCopyTilemapToVram(2);
}

static void SetTilemapRect(u16 x, u16 y, u16 w, u16 h, const u16* src)   //原文 CopyToBgTilemapBufferRect（BG2 の配置の入れ物に直接・32 マス幅）
{
	u16* map = (u16*) sPbc->tilemap;
	u16 i, j;

	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			map[(y + j) * 32 + x + i] = src[j * w + i];
}

static void DrawPokeblockInfo(s32 pkblId)
{
	u8 i;
	struct Pokeblock* pokeblock;
	u16 rectTilemapSrc[2];

	FillWindowPixelBuffer(WIN_FEEL, 0);
	if (pkblId != LIST_CANCEL)
	{
		pokeblock = &sPbc->image->blocks[pkblId];
		rectTilemapSrc[0] = 0x17;
		rectTilemapSrc[1] = 0x18;
		for (i = 0; i < FLAVOR_COUNT; i++)
		{
			if (GetPokeblockData(pokeblock, PBLOCK_SPICY + i) > 0)
			{
				rectTilemapSrc[0] = (i << 12) + 0x17;
				rectTilemapSrc[1] = (i << 12) + 0x18;
			}
			else
			{
				rectTilemapSrc[0] = 0xF;
				rectTilemapSrc[1] = 0xF;
			}
			SetTilemapRect((i / 3 * 6) + 1, (i % 3 * 2) + 13, 1, 2, rectTilemapSrc);
		}
		ConvertIntToDecimalStringN(gStringVar1, GetPokeblocksFeel(pokeblock), STR_CONV_MODE_RIGHT_ALIGN, 2);
		PrintOnPokeblockWindow(WIN_FEEL, gStringVar1, 0);      //原文は x=4（エメラルドの数字は 6px 幅）。FR の数字は 8px 幅で 2 桁だと窓（16px）から右が欠けた（2026-11-09 mGBA）ので 0
	}
	else
	{
		rectTilemapSrc[0] = 0xF;
		rectTilemapSrc[1] = 0xF;
		for (i = 0; i < FLAVOR_COUNT; i++)
			SetTilemapRect((i / 3 * 6) + 1, (i % 3 * 2) + 13, 1, 2, rectTilemapSrc);
		CopyWindowToVram(WIN_FEEL, 2);
	}
	ScheduleBgCopyTilemapToVram(0);
	ScheduleBgCopyTilemapToVram(2);
}

static void MovePokeblockMenuCursor(s32 pkblId, bool8 onInit, struct ListMenu* list)
{
	(void) list;
	if (onInit != TRUE)
	{
		PlaySE(SE_SELECT);
		gSprites[sPbc->pokeblockCaseSpriteId].callback = SpriteCB_ShakePokeblockCase;
	}
	if (!sPbc->isSwapping)
		DrawPokeblockInfo(pkblId);
}

static void PutPokeblockListMenuString(u8* dst, u16 pkblId)
{
	struct Pokeblock* pkblock = &sPbc->image->blocks[pkblId];
	u8* txtPtr = StringCopy(dst, gPokeblockNames[pkblock->color]);

	*(txtPtr++) = EXT_CTRL_CODE_BEGIN;
	*(txtPtr++) = 0x12;
	*(txtPtr++) = 0x57;                   //原文 CHAR_BLOCK_1
	*(txtPtr++) = 0xF9;                   //{LV_2}（エメラルド日本語版 0x5C9864 の「Ｌｖ」）
	*(txtPtr++) = 0x05;
	ConvertIntToDecimalStringN(txtPtr, GetHighestPokeblocksFlavorLevel(pkblock), STR_CONV_MODE_LEFT_ALIGN, 3);
}

static void UpdatePokeblockList(struct ListMenuTemplate* tmpl)
{
	u16 i;

	for (i = 0; i < sPbc->itemsNo - 1; i++)
	{
		PutPokeblockListMenuString(sPbc->menuItemsStrings[i], i);
		sPbc->items[i].name = sPbc->menuItemsStrings[i];
		sPbc->items[i].id = i;
	}
	StringCopy(sPbc->menuItemsStrings[i], gText_Pb_StowCase);
	sPbc->items[i].name = sPbc->menuItemsStrings[i];
	sPbc->items[i].id = LIST_CANCEL;

	*tmpl = sPokeblockListMenuTemplate;
	tmpl->totalItems = sPbc->itemsNo;
	tmpl->items = sPbc->items;
	tmpl->maxShowed = sPbc->maxShowed;
}

static void SetMenuItemsCountAndMaxShowed(void)
{
	u16 i;

	CompactPokeblockSlots(sPbc->image);
	for (sPbc->itemsNo = 0, i = 0; i < POKEBLOCK_CASE_SLOTS; i++)
	{
		if (sPbc->image->blocks[i].color != PBLOCK_CLR_NONE)
			sPbc->itemsNo++;
	}
	sPbc->itemsNo++;   //ケースをしまう
	sPbc->maxShowed = (sPbc->itemsNo > MAX_MENU_ITEMS) ? MAX_MENU_ITEMS : sPbc->itemsNo;
}

static void LimitMenuScrollAndRow(void)
{
	if (sPbc->scrollOffset != 0)
	{
		if (sPbc->scrollOffset + sPbc->maxShowed > sPbc->itemsNo)
			sPbc->scrollOffset = sPbc->itemsNo - sPbc->maxShowed;
	}
	if (sPbc->scrollOffset + sPbc->selectedRow >= sPbc->itemsNo)
	{
		if (sPbc->itemsNo == 0)
			sPbc->selectedRow = 0;
		else
			sPbc->selectedRow = sPbc->itemsNo - 1;
	}
}

static void SetInitialScroll(void)
{
	if (sPbc->selectedRow > MENU_MIDPOINT)
	{
		u8 i;

		for (i = 0; (i < sPbc->selectedRow - MENU_MIDPOINT) && (sPbc->scrollOffset + sPbc->maxShowed != sPbc->itemsNo);
			 sPbc->selectedRow--, sPbc->scrollOffset++, i++);
	}
}

static void CreateScrollArrows(void)
{
	if (sPbc->arrowTaskId == 0xFF)
	{
		sPbc->arrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 0xB0, 8, 0x98, sPbc->itemsNo - sPbc->maxShowed,
																	 TAG_SCROLL_ARROW, TAG_SCROLL_ARROW, &sPbc->scrollOffset);
	}
}

static void DestroyScrollArrows(void)
{
	if (sPbc->arrowTaskId != 0xFF)
	{
		Pbc_RemoveScrollIndicatorArrowPair(sPbc->arrowTaskId);
		sPbc->arrowTaskId = 0xFF;
	}
}

static void CreateSwapLineSprites(u8* spriteIds, u8 count)
{
	u8 i;

	for (i = 0; i < count; i++)
	{
		spriteIds[i] = CreateSprite(&sSpriteTemplate_SwapLine, i * 16, 0, 0);
		if (i != 0)
			StartSpriteAnim(&gSprites[spriteIds[i]], 1);
		gSprites[spriteIds[i]].invisible = TRUE;
	}
}

static void SetSwapLineSpritesInvisibility(u8* spriteIds, u8 count, bool8 invisible)
{
	u8 i;

	for (i = 0; i < count; i++)
		gSprites[spriteIds[i]].invisible = invisible;
}

static void UpdateSwapLineSpritesPos(u8* spriteIds, u8 count, s16 x, u16 y)
{
	u8 i;

	for (i = 0; i < count; i++)
	{
		gSprites[spriteIds[i]].pos2.x = x;
		gSprites[spriteIds[i]].pos1.y = 1 + y;
	}
}

static void HandleInitBackgrounds(void)
{
	ResetVramOamAndBgCntRegs();
	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sBgTemplates, 3);
	SetBgTilemapBuffer(2, sPbc->tilemap);
	ResetBgPositions();
	ScheduleBgCopyTilemapToVram(2);
	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
	ShowBg(0);
	ShowBg(1);
	ShowBg(2);
	SetGpuReg(REG_OFFSET_BLDCNT, 0);
}

static bool8 LoadPokeblockMenuGfx(void)
{
	switch (sPbc->loadState)
	{
	case 0:
		ResetTempTileDataBuffers();
		decompress_and_copy_tile_data_to_vram(2, gPokeblockMenuTiles, 0, 0, 0);
		sPbc->loadState++;
		break;
	case 1:
		if (free_temp_tile_data_buffers_if_possible() != TRUE)
		{
			LZDecompressWram(gPokeblockMenuMap, sPbc->tilemap);
			sPbc->loadState++;
		}
		break;
	case 2:
		LoadPalette(gPokeblockMenuPal, 0, 6 * 32);
		sPbc->loadState++;
		break;
	case 3:
		LoadCompressedSpriteSheet(&sPokeblockCase_SpriteSheet);
		sPbc->loadState++;
		break;
	case 4:
		LoadSpritePalette(&sPokeblockCase_SpritePal);
		sPbc->loadState++;
		break;
	case 5:
		LoadCompressedSpriteSheet(&sSpriteSheet_SwapLine);
		LoadSpritePalette(&sSpritePalette_SwapLine);
		sPbc->loadState = 0;
		return TRUE;
	}
	return FALSE;
}

static void HandleInitWindows(void)
{
	u8 i;

	InitWindows(sWindowTemplates);
	DeactivateAllTextPrinters();
	TextWindow_SetStdFrame0_WithPal(0, 1, 14 * 16);              //原文 LoadUserWindowBorderGfx(0, 1, 14)
	TextWindow_SetUserSelectedFrame(0, 0xA, 13 * 16);            //原文 LoadMessageBoxGfx(0, 0xA, 13)
	Menu_LoadStdPalAt(15 * 16);                                  //原文 LoadPalette(gStandardMenuPalette, 15)
	for (i = 0; i < ARRAY_COUNT(sWindowTemplates) - 1; i++)
		FillWindowPixelBuffer(i, 0);
	ScheduleBgCopyTilemapToVram(0);
	ScheduleBgCopyTilemapToVram(1);
}

static void DrawPokeblockMenuTitleText(void)
{
	u8 i;
	u8 itemName[24];

	CopyItemName(ITEM_POKEBLOCK_CASE, itemName);
	PrintOnPokeblockWindow(WIN_TITLE, itemName, (0x48 - GetStringWidth(PBC_FONT, itemName, 0)) / 2);   //原文 GetStringCenterAlignXOffset(font, str, 0x48)
	PrintOnPokeblockWindow(WIN_SPICY, gText_Pb_Spicy, 0);
	PrintOnPokeblockWindow(WIN_DRY, gText_Pb_Dry, 0);
	PrintOnPokeblockWindow(WIN_SWEET, gText_Pb_Sweet, 0);
	PrintOnPokeblockWindow(WIN_BITTER, gText_Pb_Bitter, 0);
	PrintOnPokeblockWindow(WIN_SOUR, gText_Pb_Sour, 0);
	for (i = 0; i < WIN_ACTIONS_TALL; i++)
		PutWindowTilemap(i);
}

#define tListTaskId data[0]
#define tWindowId data[1]
#define tToSwapId data[2]

static bool8 InitPokeblockMenu(void)
{
	u8 taskId;
	struct ListMenuTemplate tmpl;

	switch (gMain.state)
	{
	case 0:
		Pbc_SetVBlankHBlankCallbacksToNull();
		clear_scheduled_bg_copies_to_vram();
		gMain.state++;
		break;
	case 1:
		ScanlineEffect_Stop();
		gMain.state++;
		break;
	case 2:
		FreeAllSpritePalettes();
		gMain.state++;
		break;
	case 3:
		ResetPaletteFade();
		gPaletteFade->bufferTransferDisabled = TRUE;
		gMain.state++;
		break;
	case 4:
		ResetSpriteData();
		gMain.state++;
		break;
	case 5:
		ResetTasks();
		gMain.state++;
		break;
	case 6:
		HandleInitBackgrounds();
		sPbc->loadState = 0;
		gMain.state++;
		break;
	case 7:
		if (!LoadPokeblockMenuGfx())
			return FALSE;
		gMain.state++;
		break;
	case 8:
		SetMenuItemsCountAndMaxShowed();
		LimitMenuScrollAndRow();
		SetInitialScroll();
		gMain.state++;
		break;
	case 9:
		sPbc->pokeblockCaseSpriteId = CreateSprite(&sSpriteTemplate_PokeblockCase, 56, 64, 0);
		gMain.state++;
		break;
	case 10:
		CreateSwapLineSprites(sPbc->swapLineSpriteIds, NUM_SWAP_LINE_SPRITES);
		gMain.state++;
		break;
	case 11:
		DrawPokeblockMenuHighlight(sPbc->selectedRow, TILE_HIGHLIGHT_BLUE);
		gMain.state++;
		break;
	case 12:
		HandleInitWindows();
		gMain.state++;
		break;
	case 13:
		UpdatePokeblockList(&tmpl);
		gMain.state++;
		break;
	case 14:
		CreateScrollArrows();
		gMain.state++;
		break;
	case 15:
		taskId = CreateTask(Task_HandlePokeblockMenuInput, 0);
		UpdatePokeblockList(&tmpl);
		gTasks[taskId].tListTaskId = ListMenuInit(&tmpl, sPbc->scrollOffset, sPbc->selectedRow);
		gMain.state++;
		break;
	case 16:
		DrawPokeblockMenuTitleText();
		gMain.state++;
		break;
	case 17:
		BlendPalettes(PBC_ALL_PALETTES, 16, 0);
		gMain.state++;
		break;
	case 18:
		BeginNormalPaletteFade(PBC_ALL_PALETTES, 0, 16, 0, 0);
		gPaletteFade->bufferTransferDisabled = FALSE;
		gMain.state++;
		break;
	default:
		SetVBlankCallback(VBlankCB_PokeblockMenu);
		SetMainCallback2(CB2_PokeblockMenu);
		return TRUE;
	}
	return FALSE;
}

static void CB2_InitPokeblockMenu(void)
{
	while (1)
	{
		if (InitPokeblockMenu() == TRUE)
			break;
	}
}

#ifdef HACK_DEBUG_MENU
//デバッグ版だけ（配る版には入らない・verify_release_no_debug）: ケースを開くとき R ボタンを押していると 5 個（L も押していると 40 個）足す。mGBA で画面を見るための道
//  Pokeblock_Add を通すので区画・状態の整合まで同じ道を通る
static void DebugFillCase(struct PokeblockImage* image, u8 n)
{
	u8 i;
	struct Pokeblock pb;

	for (i = 0; i < n; i++)
	{
		pb.color = 1 + (i % 14);
		pb.spicy = (i % 5 == 0 || i % 7 == 1) ? 10 + (i * 3) % 60 : 0;
		pb.dry = (i % 5 == 1 || i % 7 == 2) ? 12 + (i * 5) % 50 : 0;
		pb.sweet = (i % 5 == 2 || i % 7 == 3) ? 14 + (i * 7) % 40 : 0;
		pb.bitter = (i % 5 == 3 || i % 7 == 4) ? 16 + (i * 11) % 30 : 0;
		pb.sour = (i % 5 == 4 || i % 7 == 5) ? 18 + (i * 13) % 20 : 0;
		pb.feel = 20 + (i * 9) % 90;
		if (!Pokeblock_Add(image, &pb, NULL))
			break;
	}
}
//SELECT を押しながら開くと、スキップ（腕前ふつう）の「ブレンド」を 2〜4 人・名人ありで何通りか作ってケースに足す（FEAT-pokeblock 段 4a・mGBA で計算の結果を見る道・きのみは減らさない）
static void DebugBlendSkip(struct PokeblockImage* image)
{
	static const u16 sPlayerBerries[5] = {ITEM_CHERI_BERRY, ITEM_LEPPA_BERRY, ITEM_SPELON_BERRY, ITEM_LIECHI_BERRY, ITEM_ENIGMA_BERRY};
	u8 k, players, master, f[BLENDER_FLAVORS];
	struct BlenderBerry b[BLENDER_MAX_PLAYERS];
	struct Pokeblock pb;

	for (k = 0; k < 5; k++)
	{
		for (players = 2; players <= 4; players++)
		{
			for (master = 0; master <= (players == 2); master++)
			{
				HackBlender_ConvertItem(&b[0], sPlayerBerries[k]);
				HackBlender_SetOpponentBerries(sPlayerBerries[k], players, master, b);
				HackBlender_Calculate(b, &pb, players, f, HackBlender_SkipMaxRPM(players, master));
				if (!Pokeblock_Add(image, &pb, NULL))
					return;
			}
		}
	}
}
#endif

//---------------------------------------------------------------- 開く

void OpenPokeblockCase(u8 caseId, u8 returnKind)
{
	u8 row, scroll;

	sPbc = AllocZeroed(sizeof(struct PokeblockMenu));
	sPbc->caseId = caseId;
	sPbc->returnKind = returnKind;
	sPbc->returnCallback = CallbackForKind(returnKind);
	sPbc->callbackOnUse = NULL;
	sPbc->arrowTaskId = 0xFF;
	sPbc->isSwapping = FALSE;
	sPbc->image = AllocZeroed(sizeof(struct PokeblockImage));
	sPbc->usable = Pokeblock_Usable();
	if (!Pokeblock_Load(sPbc->image))
		sPbc->usable = POKEBLOCK_BAD_DATA;
#ifdef HACK_DEBUG_MENU
	if (gMain.heldKeys & R_BUTTON)
		DebugFillCase(sPbc->image, (gMain.heldKeys & L_BUTTON) ? 40 : 5);
	if (gMain.heldKeys & SELECT_BUTTON)
		DebugBlendSkip(sPbc->image);
#endif
	Pokeblock_GetCasePos(&row, &scroll);
	sPbc->selectedRow = row;
	sPbc->scrollOffset = scroll;

	switch (caseId)
	{
	case PBLOCK_CASE_BATTLE:
		sPbc->pokeblockActionIds = sActionsInBattle;
		sPbc->numActions = ARRAY_COUNT(sActionsInBattle);
		break;
	case PBLOCK_CASE_FEEDER:
		sPbc->pokeblockActionIds = sActionsOnPokeblockFeeder;
		sPbc->numActions = ARRAY_COUNT(sActionsOnPokeblockFeeder);
		break;
	case PBLOCK_CASE_GIVE:
		sPbc->pokeblockActionIds = sActionsWhenGivingToLady;
		sPbc->numActions = ARRAY_COUNT(sActionsWhenGivingToLady);
		break;
	default:
		sPbc->pokeblockActionIds = sActionsOnField;
		sPbc->numActions = ARRAY_COUNT(sActionsOnField);
		break;
	}
	gMain.state = 0;
	SetMainCallback2(CB2_InitPokeblockMenu);
}

//バッグ・登録（Select）から: 素の FieldUseFunc_TownMap（pokefirered item_use.c:649・JP 080A2F28）と同じ形
static void OpenPokeblockCaseFromBag(void)
{
	OpenPokeblockCase(PBLOCK_CASE_FIELD, PBC_RET_BAG);
}

static void Task_UsePokeblockCaseFromField(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		CleanupOverworldWindowsAndTilemaps();
		Pbc_SetFieldCallback2ForItemUse();
		OpenPokeblockCase(PBLOCK_CASE_FIELD, PBC_RET_FIELD);
		DestroyTask(taskId);
	}
}

//開いてよいか: 使えない版・レポート前・壊れ は文を出して開かない（ユーザー決定 2026-11-09・仮の文は strings/hack_pokeblock.string）
static const u8* CaseRefusalText(void)
{
	u8 st = Pokeblock_Usable();

	if (st == POKEBLOCK_OK)
	{
		struct PokeblockImage* probe = AllocZeroed(sizeof(struct PokeblockImage));
		bool8 ok = Pokeblock_Load(probe);

		Free(probe);
		if (!ok)
			st = POKEBLOCK_BAD_DATA;
	}
	switch (st)
	{
	case POKEBLOCK_OK: return NULL;
	case POKEBLOCK_NO_EXT: return gText_Pb_NoExt;
	case POKEBLOCK_NEED_REPORT: return gText_Pb_NeedReport;
	default: return gText_Pb_BadData;
	}
}

//ミナモのロビーの台（src/hack_berry_blender.c の HoennNative_PokeblockFreeSlot／PokeblockRefuse）が同じ判定を使う（FEAT-pokeblock 段 5）
const u8* Pbc_GetRefusalText(void)
{
	return CaseRefusalText();
}

void FieldUseFunc_PokeblockCase(u8 taskId)
{
	const u8* refusal = CaseRefusalText();

	if (refusal != NULL)
	{
		Pbc_DisplayCannotUse(taskId, gTasks[taskId].data[3], 4, refusal);   //バッグから＝バッグへ・登録から＝フィールドへ
		return;
	}
	if (gTasks[taskId].data[3] == 0)
	{
		ItemMenu_SetExitCallback(OpenPokeblockCaseFromBag);
		ItemMenu_StartFadeToExitCallback(taskId);
	}
	else
	{
		FadeScreen(1, 0);
		gTasks[taskId].func = Task_UsePokeblockCaseFromField;
	}
}

//---------------------------------------------------------------- 動き

static void SpriteCB_ShakePokeblockCase(struct Sprite* sprite)
{
#define sState data[0]
#define sTimer data[1]
	if (sprite->sState > 1)
		sprite->sState = 0;
	switch (sprite->sState)
	{
	case 0:
		sprite->oam.affineMode = ST_OAM_AFFINE_NORMAL;
		sprite->affineAnims = sAffineAnims_PokeblockCaseShake;
		InitSpriteAffineAnim(sprite);
		sprite->sState = 1;
		sprite->sTimer = 0;
		break;
	case 1:
		if (++sprite->sTimer > 11)
		{
			sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
			sprite->sState = 0;
			sprite->sTimer = 0;
			FreeOamMatrix(sprite->oam.matrixNum);
			sprite->callback = SpriteCallbackDummy;
		}
		break;
	}
#undef sState
#undef sTimer
}

static void FadePaletteAndSetTaskToClosePokeblockCase(u8 taskId)
{
	BeginNormalPaletteFade(PBC_ALL_PALETTES, 0, 0, 16, 0);
	gTasks[taskId].func = Task_FreeDataAndExitPokeblockCase;
}

static void Task_FreeDataAndExitPokeblockCase(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	if (!gPaletteFade->active)
	{
		MainCallback next;

		DestroyListMenuTask(tListTaskId, &sPbc->scrollOffset, &sPbc->selectedRow);
		DestroyScrollArrows();
		ResetSpriteData();
		FreeAllSpritePalettes();
		Pokeblock_SetCasePos(sPbc->selectedRow, sPbc->scrollOffset);
		next = (sPbc->callbackOnUse != NULL) ? sPbc->callbackOnUse : sPbc->returnCallback;
		FreeAllWindowBuffers();
		Free(sPbc->image);
		Free(sPbc);
		sPbc = NULL;
		SetMainCallback2(next);
		DestroyTask(taskId);
	}
}

static void Task_HandlePokeblockMenuInput(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	if (!gPaletteFade->active)
	{
		if (JOY_NEW(SELECT_BUTTON))
		{
			ListMenuGetScrollAndRow(tListTaskId, &sPbc->scrollOffset, &sPbc->selectedRow);
			if (sPbc->scrollOffset + sPbc->selectedRow != sPbc->itemsNo - 1)
			{
				PlaySE(SE_SELECT);
				DrawPokeblockMenuHighlight(sPbc->selectedRow, TILE_HIGHLIGHT_RED);
				tToSwapId = sPbc->scrollOffset + sPbc->selectedRow;
				sPbc->isSwapping = TRUE;
				gTasks[taskId].func = Task_HandlePokeblocksSwapInput;
			}
		}
		else
		{
			u16 oldPosition = sPbc->selectedRow;
			s32 input = ListMenu_ProcessInput(tListTaskId);

			ListMenuGetScrollAndRow(tListTaskId, &sPbc->scrollOffset, &sPbc->selectedRow);
			if (oldPosition != sPbc->selectedRow)
			{
				DrawPokeblockMenuHighlight(oldPosition, TILE_HIGHLIGHT_NONE);
				DrawPokeblockMenuHighlight(sPbc->selectedRow, TILE_HIGHLIGHT_BLUE);
			}
			switch (input)
			{
			case LIST_NOTHING_CHOSEN:
				break;
			case LIST_CANCEL:
				PlaySE(SE_SELECT);
				Var800D = 0xFFFF;
				gSpecialVar_ItemId = 0;
				FadePaletteAndSetTaskToClosePokeblockCase(taskId);
				break;
			default:
				PlaySE(SE_SELECT);
				gSpecialVar_ItemId = input;
				ShowPokeblockActionsWindow(taskId);
				break;
			}
		}
	}
}

static void UpdatePokeblockSwapMenu(u8 taskId, bool8 noSwap)
{
	u8 i;
	s16* data = gTasks[taskId].data;
	u16 swappedFromId = sPbc->scrollOffset + sPbc->selectedRow;
	struct ListMenuTemplate tmpl;
	bool8 moved = TRUE;

	sPbc->isSwapping = FALSE;
	DestroyListMenuTask(tListTaskId, &sPbc->scrollOffset, &sPbc->selectedRow);
	if (!noSwap && tToSwapId != swappedFromId && tToSwapId != swappedFromId - 1)
	{
		struct Pokeblock backup[POKEBLOCK_CASE_SLOTS];
		u8 k;

		for (k = 0; k < POKEBLOCK_CASE_SLOTS; k++)
			backup[k] = sPbc->image->blocks[k];
		SwapPokeblockMenuItems(sPbc->image, tToSwapId, swappedFromId);
		if (!Pokeblock_Commit(sPbc->image))     //段 1 の設計: 変えたら書く（レポートまでは同じ区画の上書き）。書けなかったら画面の中の並びも元に戻して SE（保存と画面を食い違わせない）
		{
			for (k = 0; k < POKEBLOCK_CASE_SLOTS; k++)
				sPbc->image->blocks[k] = backup[k];
			PlaySE(SE_ERROR);
			moved = FALSE;
		}
		UpdatePokeblockList(&tmpl);
	}
	else
		UpdatePokeblockList(&tmpl);
	if (moved && tToSwapId < swappedFromId)
		sPbc->selectedRow--;
	tListTaskId = ListMenuInit(&tmpl, sPbc->scrollOffset, sPbc->selectedRow);
	ScheduleBgCopyTilemapToVram(0);
	SetSwapLineSpritesInvisibility(sPbc->swapLineSpriteIds, NUM_SWAP_LINE_SPRITES, TRUE);
	for (i = 0; i < MAX_MENU_ITEMS; i++)
		DrawPokeblockMenuHighlight(i, TILE_HIGHLIGHT_NONE);
	DrawPokeblockMenuHighlight(sPbc->selectedRow, TILE_HIGHLIGHT_BLUE);
	gTasks[taskId].func = Task_HandlePokeblockMenuInput;
}

static void Task_HandlePokeblocksSwapInput(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	if (JOY_NEW(SELECT_BUTTON))
	{
		PlaySE(SE_SELECT);
		ListMenuGetScrollAndRow(tListTaskId, &sPbc->scrollOffset, &sPbc->selectedRow);
		UpdatePokeblockSwapMenu(taskId, FALSE);
	}
	else
	{
		u16 i = sPbc->scrollOffset;
		u16 row = sPbc->selectedRow;
		s32 input = ListMenu_ProcessInput(tListTaskId);

		ListMenuGetScrollAndRow(tListTaskId, &sPbc->scrollOffset, &sPbc->selectedRow);
		if (i != sPbc->scrollOffset || row != sPbc->selectedRow)
		{
			for (i = 0; i < MAX_MENU_ITEMS; i++)
			{
				row = i + sPbc->scrollOffset;
				if (row == tToSwapId)
					DrawPokeblockMenuHighlight(i, TILE_HIGHLIGHT_RED);
				else
					DrawPokeblockMenuHighlight(i, TILE_HIGHLIGHT_NONE);
			}
		}
		SetSwapLineSpritesInvisibility(sPbc->swapLineSpriteIds, NUM_SWAP_LINE_SPRITES, FALSE);
		UpdateSwapLineSpritesPos(sPbc->swapLineSpriteIds, NUM_SWAP_LINE_SPRITES, 128, (sPbc->selectedRow * 16) + 8);
		switch (input)
		{
		case LIST_NOTHING_CHOSEN:
			break;
		case LIST_CANCEL:
			PlaySE(SE_SELECT);
			if (JOY_NEW(A_BUTTON))
				UpdatePokeblockSwapMenu(taskId, FALSE);
			else
				UpdatePokeblockSwapMenu(taskId, TRUE);
			break;
		default:
			PlaySE(SE_SELECT);
			UpdatePokeblockSwapMenu(taskId, FALSE);
			break;
		}
	}
}

//---------------------------------------------------------------- アクション窓（原文 PrintMenuActionTextsInUpperLeftCorner／InitMenuInUpperLeftCornerNormal／Menu_ProcessInputNoWrap の同等）

static void ShowPokeblockActionsWindow(u8 taskId)
{
	s16* data = gTasks[taskId].data;
	u8 i;

	tWindowId = (sPbc->numActions == 3) ? WIN_ACTIONS_TALL : WIN_ACTIONS;
	data[3] = 0;                                   //アクション窓の今のカーソル
	DestroyScrollArrows();
	DrawStdFrameWithCustomTileAndPalette(tWindowId, FALSE, 1, 0xE);
	for (i = 0; i < sPbc->numActions; i++)
		AddTextPrinterParameterized4(tWindowId, PBC_FONT, 8, i * 16 + 1, 0, 0, sTextColor, 0,
									 sPokeblockMenuActions[sPbc->pokeblockActionIds[i]].text);
	Menu_InitCursor(tWindowId, PBC_FONT, 0, 0, 16, sPbc->numActions, 0);
	PutWindowTilemap(tWindowId);
	ScheduleBgCopyTilemapToVram(1);
	gTasks[taskId].func = Task_HandlePokeblockActionsInput;
}

static void Task_HandlePokeblockActionsInput(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	if (JOY_NEW(DPAD_UP) && data[3] > 0)
	{
		PlaySE(SE_SELECT);
		data[3] = Menu_MoveCursorNoWrapAround(-1);
	}
	else if (JOY_NEW(DPAD_DOWN) && data[3] < sPbc->numActions - 1)
	{
		PlaySE(SE_SELECT);
		data[3] = Menu_MoveCursorNoWrapAround(1);
	}
	else if (JOY_NEW(A_BUTTON))
	{
		PlaySE(SE_SELECT);
		sPokeblockMenuActions[sPbc->pokeblockActionIds[data[3]]].func.void_u8(taskId);
	}
	else if (JOY_NEW(B_BUTTON))
	{
		PlaySE(SE_SELECT);
		PokeblockAction_Cancel(taskId);
	}
}

static void PokeblockAction_UseOnField(u8 taskId)
{
	sPbc->callbackOnUse = UsePokeblockOnField;
	Pokeblock_SetCaseReturn(sPbc->returnKind);   //閉じたあと sPbc は無い＝戻り先の種類を SaveBlock1 に置く
	FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

//ポケモンを選ぶ画面（原文 UsePokeblockOnField→ChooseMonToGivePokeblock・src/hack_pokeblock_use.c）。選んだポロックは gSpecialVar_ItemId（並べ直しのあとの番号）
static void UsePokeblockOnField(void)
{
	ChooseMonToGivePokeblock(ReturnToPokeblockCaseOnField);
}

//食べさせたあと（やめたあと）はケースを同じ場所に開き直す（原文 ReturnToPokeblockCaseOnField）。戻り先の種類は閉じるとき SaveBlock1 に置いた（PokeblockAction_UseOnField）
static void ReturnToPokeblockCaseOnField(void)
{
	OpenPokeblockCase(PBLOCK_CASE_FIELD, Pokeblock_GetCaseReturn());
}

//戦闘で「つかう」（原文 PokeblockAction_UseInBattle・FEAT-pokeblock 段 6）。相手の先頭の性格との相性で 気になる／夢中／無視 の 1・2・3 を色と一緒に gSpecialVar_ItemId へ（色 << 8 | 反応）。
//  ★ 原文は TryClearPokeblock のあとで色を読む（次のポロックの色になる）が、ここは使うポロックの色（使う側 HackSafari_HandleActionBait は色の名前にだけ使う）。書けなかったら使わない
static void PokeblockAction_UseInBattle(u8 taskId)
{
	u8 nature = GetNature(&gEnemyParty[0]);
	u8 slot = gSpecialVar_ItemId;
	u8 color = sPbc->image->blocks[slot].color;
	u8 react = HackSafari_BattleReaction(nature, &sPbc->image->blocks[slot]);

	if (!Pokeblock_Remove(sPbc->image, slot))
	{
		PlaySE(SE_ERROR);
		PokeblockAction_Cancel(taskId);
		return;
	}
	gSpecialVar_ItemId = (color << 8) | react;
	FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

//フィーダー（おき場）に置く（原文 PokeblockAction_UseOnPokeblockFeeder）。台本へ: LASTRESULT = 元の番号・gStringVar1 = 色の名前
static void PokeblockAction_UseOnFeeder(u8 taskId)
{
	u8 slot = gSpecialVar_ItemId;
	struct Pokeblock pb = sPbc->image->blocks[slot];

	if (!Pokeblock_Remove(sPbc->image, slot))
	{
		PlaySE(SE_ERROR);
		PokeblockAction_Cancel(taskId);
		return;
	}
	HackSafari_ActivateFeeder(&pb);
	StringCopy(gStringVar1, gPokeblockNames[pb.color]);
	Var800D = slot;
	gSpecialVar_ItemId = 0;
	FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

//ミナモのポケモンセンターのコンテストのおねえさんに あげる（原文 PokeblockAction_GiveToContestLady・まとめ 0.66.0: コンテスト枝の HackLady の口 GivePokeblockToContestLady につないだ）。
//  台本へ: Var8004 = よいポロックなら 1・LASTRESULT（Var800D）= 元の番号。書けなかったら あげない
static void PokeblockAction_GiveToContestLady(u8 taskId)
{
	u8 slot = gSpecialVar_ItemId;
	struct Pokeblock pb = sPbc->image->blocks[slot];

	if (!Pokeblock_Remove(sPbc->image, slot))
	{
		PlaySE(SE_ERROR);
		PokeblockAction_Cancel(taskId);
		return;
	}
	Var8004 = GivePokeblockToContestLady(&pb) ? 1 : 0;
	Var800D = slot;
	gSpecialVar_ItemId = 0;
	FadePaletteAndSetTaskToClosePokeblockCase(taskId);
}

static void PokeblockAction_Toss(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
	PokeblockCopyName(&sPbc->image->blocks[gSpecialVar_ItemId], gStringVar1);
	StringExpandPlaceholders(gStringVar4, gText_Pb_ThrowAway);
	Pbc_DisplayMessageAndContinueTask(taskId, WIN_TOSS_MSG, 0xA, 13, PBC_FONT, GetPlayerTextSpeedDelay(), gStringVar4, CreateTossPokeblockYesNoMenu);
}

static void CreateTossPokeblockYesNoMenu(u8 taskId)
{
	Pbc_CreateYesNoMenuWithCallbacks(taskId, &sTossPkblockWindowTemplate, PBC_FONT, 0, 2, 1, 0xE, &sTossYesNoFuncTable);
}

static void TossedPokeblockMessage(u8 taskId)
{
	StringExpandPlaceholders(gStringVar4, gText_Pb_ThrownAway);
	Pbc_DisplayMessageAndContinueTask(taskId, WIN_TOSS_MSG, 0xA, 13, PBC_FONT, GetPlayerTextSpeedDelay(), gStringVar4, TossPokeblock);
}

static void TossPokeblock(u8 taskId)
{
	if (JOY_NEW(A_BUTTON | B_BUTTON))
	{
		s16* data;
		u16* scrollOffset;
		u16* selectedRow;
		struct ListMenuTemplate tmpl;

		if (!Pokeblock_Remove(sPbc->image, gSpecialVar_ItemId))
			PlaySE(SE_ERROR);                   //書けなかった: Remove が入れ物を元に戻す（すてていない）
		else
			PlaySE(SE_SELECT);
		scrollOffset = &sPbc->scrollOffset;
		selectedRow = &sPbc->selectedRow;
		data = gTasks[taskId].data;
		DestroyListMenuTask(tListTaskId, scrollOffset, selectedRow);
		DrawPokeblockMenuHighlight(*selectedRow, TILE_HIGHLIGHT_NONE);
		SetMenuItemsCountAndMaxShowed();
		LimitMenuScrollAndRow();
		UpdatePokeblockList(&tmpl);
		tListTaskId = ListMenuInit(&tmpl, *scrollOffset, *selectedRow);
		DrawPokeblockMenuHighlight(*selectedRow, TILE_HIGHLIGHT_BLUE);
		ScheduleBgCopyTilemapToVram(0);
		ScheduleBgCopyTilemapToVram(1);
		CloseTossPokeblockWindow(taskId);
	}
}

static void CloseTossPokeblockWindow(u8 taskId)
{
	ClearDialogWindowAndFrame(WIN_TOSS_MSG, FALSE);
	ScheduleBgCopyTilemapToVram(1);
	CreateScrollArrows();
	gTasks[taskId].func = Task_HandlePokeblockMenuInput;
}

static void PokeblockAction_Cancel(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	ClearStdWindowAndFrameToTransparent(tWindowId, FALSE);
	ScheduleBgCopyTilemapToVram(1);
	CreateScrollArrows();
	gTasks[taskId].func = Task_HandlePokeblockMenuInput;
}
