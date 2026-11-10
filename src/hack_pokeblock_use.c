#include "defines.h"
#include "../include/bg.h"
#include "../include/berry.h"
#include "../include/scanline_effect.h"
#include "../include/text_window.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/menu.h"
#include "../include/menu_helpers.h"
#include "../include/palette.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/decompress.h"
#include "../include/string_util.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/trig.h"
#include "../include/event_data.h"
#include "../include/constants/songs.h"
#include "../include/new/Vanilla_functions.h"
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"
#include "hack_condition.h"
#include "hack_condition_graph.h"
#include "hack_pokeblock_use.h"
#include "hack_pokeblock_feed.h"
#include "../include/pokemon_storage_system.h"
#define gNatureNamePointers ((const u8* const*) 0x0842D1EC)   //素の FR（JP）のせいかく名の表 25 個（検算が 25 個の文字列を日本語版の名前と突き合わせる）

void TransferPlttBuffer(void);
void __attribute__((long_call)) CopyToBgTilemapBufferRect_ChangePalette(u8 bg, const void* src, u8 destX, u8 destY, u8 rectWidth, u8 rectHeight, u8 palette);
void __attribute__((long_call)) CreateYesNoMenu(const struct WindowTemplate* window, u8 fontId, u8 left, u8 top, u16 baseTileNum, u8 paletteNum, u8 initialCursorPos);   //素の FR（08110A94・原文 Emerald は引数が少ない形）
extern void CompactPokeblockSlots(struct PokeblockImage* image);   //hack_pokeblock_case.c

/*
 * ポロックをポケモンにあげる画面（2026-11-09 FEAT-pokeblock 段 3a・patches/0978）。
 * 原文: 参考/pokeemerald/src/use_pokeblock.c（L36-L1677）。ゲージ・星・絵の出入り・選択の玉は src/hack_condition_graph.c（原文 menu_specialized.c）。
 * 原文との差:
 *   - コンディション 5 値とけづやは MON_DATA_COOL〜SMART／SHEEN ではなく、ポケモン自身の ID を鍵にした表 HackCondition_*（src/hack_condition.c）で読み書きする
 *     （手持ちの +0x3E〜+0x41 は個体の ID＝書くと色違い・Moémon が変わる）。
 *   - ポロックは SaveBlock の配列ではなく追加の区画（段 1 の PokeblockImage）。画面は選んだポロックの写しを持ち、食べさせるときに Pokeblock_Remove で 1 個外す。
 *     原文は gSpecialVar_ItemId に載せた番号で外す（TryClearPokeblock）が、ここは写しと同じ中身の最初の 1 個を外す（同じ中身は区別が無い）。
 *   - 食べさせる順（原文は 1 回の書き込みだが、ここは保存が 2 回になるので決めた）: ①コンディションを保存（失敗なら食べない・SE）②ポロックを外す（失敗なら①を戻す・SE）
 *     ③なつき度（好きな味のとき・追加の仕様・HackPokeblock_AddFriendship）。
 *   - 食べさせる演出画面（pokeblock_feed.c）は 3b。3a は「はい」のあとそのまま結果（ゲージ）へ進む。
 *   - 状態は Alloc した入れ物。ポインタの語は ポロックケースと同じ 0x203E038（大きな画面は同時に開かない）。書き換え可能な static は ROM に置かれる（6.13）ので原文の static ポインタは入れ物のメンバー。
 *   - FONT_NARROW・FONT_NORMAL→素の FR の 2。フレーバーの好き嫌いは素の FR の GetMonFlavorRelation が static で呼べないので同じ表（gPokeblockFlavorCompatibilityTable）で引く。
 */

#define PBU_STATE_PTR 0x203E038
#define PBU_FONT 2
#define PBU_ALL_PALETTES 0xFFFFFFFFu
#define BG_SCREEN_SIZE 0x800
#define PBU_GRAPH_GFX_SIZE 4000   //ゲージの BG タイル（日本語版エメラルドの gPbuGraph_Gfx＝125 タイル。英語版は 6656）
#define COPYWIN_MAP 1
#define COPYWIN_GFX 2
#define COPYWIN_FULL 3
#define BG_COORD_SET 0
#define MENU_B_PRESSED -1
#define SPRITE_NONE 0xFF

enum {
	WIN_NAME,
	WIN_NATURE,
	WIN_TEXT,
	WIN_COUNT
};

#define TAG_UP_DOWN     0
#define TAG_CONDITION   1

// 一度に読み込んでおく数（今の子とその前後 2 匹）
#define NUM_SELECTIONS_LOADED 3

struct UsePokeblockSession
{
	void (*callback)(void);
	MainCallback exitCallback;
	struct Pokeblock* pokeblock;
	struct Pokemon* mon;
	u8 stringBuffer[64];
	u8 mainState;
	u8 unused1;
	u8 timer;
	u8 condition;
	u8 numEnhancements;
	u8 unused2;
	bool8 monInTopHalf;
	u8 conditionsBeforeBlock[CONDITION_COUNT];
	u8 conditionsAfterBlock[CONDITION_COUNT];
	u8 enhancements[CONDITION_COUNT];
	s16 pokeblockStatBoosts[CONDITION_COUNT];
	u8 numSelections; // 手持ちの数 + 1（やめる）
	u8 curSelection;
	bool8 (*loadNewSelection)(void);
	u8 helperState;
	u8 unused3;
	u8 natureText[34];
	s16 gain;                      //原文は gPokeblockGain（グローバル）。ポロックの味とせいかくの相性の合計
	//ここから先は食べさせる演出（3b）をまたぐための追加（原文は gPokeblockMonId 等のグローバル）
	u8 sheenBefore;                //食べる前のけづや（食べたあとはコンディションの表が変わっているので画面を作り直すとき星の数を前の値に置き直す）
	bool8 resume;                  //演出のあとの作り直し（LoadMonInfo が食べた子だけ前の値に置き直す）
	u8 resumeMonId;                //食べた子の gPlayerParty の番号
	u8 resumeSelection;            //食べた子の選択の番号（たまご抜き）
};

struct UsePokeblockMenuPokemon
{
	u8 boxId; // 手持ちだけなので TOTAL_BOXES_COUNT 固定
	u8 monId;
	u16 data;
};

struct UsePokeblockMenu
{
	u32 unused;
	u16 partyPalettes[PARTY_SIZE][0x40];
	u8 partySheets[NUM_SELECTIONS_LOADED][MON_PIC_SIZE];
	u8 tilemapBuffer[BG_SCREEN_SIZE + 2];
	u8 selectionIconSpriteIds[PARTY_SIZE + 1];
	s16 curMonXOffset;
	u8 curMonSpriteId;
	u16 curMonPalette;
	u16 curMonSheet;
	u8* curMonTileStart;
	struct Sprite* sparkles[MAX_CONDITION_SPARKLES];
	struct Sprite* condition[2];
	u8 toLoadSelection;
	u8 monNameStrings[NUM_SELECTIONS_LOADED][64];
	struct ConditionGraph graph;
	u8 numSparkles[NUM_SELECTIONS_LOADED];
	s8 curLoadId;
	s8 nextLoadId;
	s8 prevLoadId;
	s8 toLoadId;
	struct UsePokeblockMenuPokemon party[PARTY_SIZE];
	struct UsePokeblockSession info;
	//ここから先は原文に無い（入れ物の中に持つ・6.13）
	struct Pokeblock pokeblockCopy;    //選んだポロックの写し（ケースの入れ物は画面を閉じるとき解放される）
	u8* graphTilemap;                  //原文の sGraph_Tilemap
	u8* graphGfx;                      //原文の sGraph_Gfx
	u8* monFrameTilemap;               //原文の sMonFrame_TilemapPtr
};

#define sMenu (*(struct UsePokeblockMenu**) PBU_STATE_PTR)
#define sCarryWord (*(struct PokeblockCarry**) PBU_STATE_PTR)   //演出の間だけ語は carry を指す（hack_pokeblock_feed.h）
#define sInfo (&sMenu->info)
#define sGraph_Tilemap (sMenu->graphTilemap)
#define sGraph_Gfx (sMenu->graphGfx)
#define sMonFrame_TilemapPtr (sMenu->monFrameTilemap)
#define gPokeblockGain (sInfo->gain)

extern const u8 gPbuCondition_Gfx[], gPbuUpDown_Gfx[], gPbuGraph_Gfx[], gPbuGraph_Tilemap[], gPbuGraphData_Tilemap[];
extern const u8 gPbuMonFrame_Gfx[], gPbuMonFrame_Tilemap[], gPbuMonFrame_Pal[];
extern const u16 gPbuCondition_Pal[], gPbuUpDown_Pal[], gPbuGraph_Pal[], gPbuNatureWin[], gPbuCondGraphData_Pal[], gPbuCondText_Pal[];
extern const u8 gText_Pbu_Coolness[], gText_Pbu_Beauty[], gText_Pbu_Cuteness[], gText_Pbu_Smartness[], gText_Pbu_Toughness[];
extern const u8 gText_Pbu_GetsAPokeBlockQuestion[], gText_Pbu_WasEnhanced[], gText_Pbu_NothingChanged[], gText_Pbu_WontEatAnymore[], gText_Pbu_NatureSlash[];
extern const u8 gText_Pbu_Beauty[];

static void SetUsePokeblockCallback(void (*func)(void));
static void LoadUsePokeblockMenu(void);
static void CB2_UsePokeblockMenu(void);
static void ShowUsePokeblockMenu(void);
static void LoadPartyInfo(void);
static void LoadAndCreateSelectionIcons(void);
static bool8 LoadConditionTitle(void);
static bool8 LoadUsePokeblockMenuGfx(void);
static void UpdateMonPic(u8);
static void UpdateMonInfoText(u16, bool8);
static void UsePokeblockMenu(void);
static void UpdateSelection(bool8);
static void CloseUsePokeblockMenu(void);
static void AskUsePokeblock(void);
static s8 HandleAskUsePokeblockInput(void);
static bool8 IsSheenMaxed(void);
static void PrintWontEatAnymore(void);
static void FeedPokeblockToMon(void);
static void EraseMenuWindow(void);
static u8 GetPartyIdFromSelectionId(u8);
static void ShowPokeblockResults(void);
static bool8 CalculateConditionEnhancements(void);
static void LoadAndCreateUpDownSprites(void);
static void CalculateNumAdditionalSparkles(u8);
static void PrintFirstEnhancement(void);
static bool8 TryPrintNextEnhancement(void);
static void BufferEnhancedText(u8*, u8, s16);
static void PrintMenuWindowText(const u8*);
static void CalculatePokeblockEffectiveness(struct Pokeblock*, struct Pokemon*);
static void SpriteCB_UpDown(struct Sprite*);
static void LoadInitialMonInfo(void);
static void LoadMonInfo(s16, u8);
static bool8 LoadNewSelection_CancelToMon(void);
static bool8 LoadNewSelection_MonToCancel(void);
static bool8 LoadNewSelection_MonToMon(void);
static void SpriteCB_SelectionIconPokeball(struct Sprite*);
static void SpriteCB_SelectionIconCancel(struct Sprite*);
static void SpriteCB_MonPic(struct Sprite*);
static void SpriteCB_Condition(struct Sprite*);

//素の FR の GetMonFlavorRelation は static。同じ表（原文の sPokeblockFlavorCompatibilityTable）で引く
static s8 GetMonFlavorRelation(struct Pokemon* mon, u8 flavor)
{
	return gPokeblockFlavorCompatibilityTable[GetNature(mon) * FLAVOR_COUNT + flavor];
}

// The condition/flavors aren't listed in their normal order in this file, they're listed as shown on the graph going counter-clockwise
// Normally they would go Cool/Spicy, Beauty/Dry, Cute/Sweet, Smart/Bitter, Tough/Sour (also graph order, but clockwise)
// ゲージの並び（COOL・TOUGH・SMART・CUTE・BEAUTY）→ 表（hack_condition.h）の添字。原文は MON_DATA_*（この改造では手持ちの ID の 4 バイトを含むので使わない）
static const u8 sConditionToCond[CONDITION_COUNT] =
{
    [CONDITION_COOL]   = HCOND_COOL,
    [CONDITION_TOUGH]  = HCOND_TOUGH,
    [CONDITION_SMART]  = HCOND_SMART,
    [CONDITION_CUTE]   = HCOND_CUTE,
    [CONDITION_BEAUTY] = HCOND_BEAUTY
};

static const u8 sConditionToFlavor[CONDITION_COUNT] =
{
    [CONDITION_COOL]   = FLAVOR_SPICY,
    [CONDITION_TOUGH]  = FLAVOR_SOUR,
    [CONDITION_SMART]  = FLAVOR_BITTER,
    [CONDITION_CUTE]   = FLAVOR_SWEET,
    [CONDITION_BEAUTY] = FLAVOR_DRY
};

static const u8 sNatureTextColors[] =
{
    TEXT_COLOR_TRANSPARENT,
    TEXT_COLOR_BLUE,
    TEXT_COLOR_WHITE
};

static const struct BgTemplate sBgTemplates[4] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 0x1F,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 0x1E,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 0x1D,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0x100
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 0x17,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const struct WindowTemplate sWindowTemplates[WIN_COUNT + 1] =
{
    [WIN_NAME] = {
        .bg = 0,
        .tilemapLeft = 13,
        .tilemapTop = 1,
        .width = 13,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WIN_NATURE] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 14,
        .width = 11,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x35
    },
    [WIN_TEXT] = {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 17,
        .width = 28,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x4B
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sUsePokeblockYesNoWinTemplate =
{
    .bg = 0,
    .tilemapLeft = 24,
    .tilemapTop = 11,
    .width = 5,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x83
};

static const u8 *const sConditionNames[CONDITION_COUNT] =
{
    [CONDITION_COOL]   = gText_Pbu_Coolness,
    [CONDITION_TOUGH]  = gText_Pbu_Toughness,
    [CONDITION_SMART]  = gText_Pbu_Smartness,
    [CONDITION_CUTE]   = gText_Pbu_Cuteness,
    [CONDITION_BEAUTY] = gText_Pbu_Beauty
};

static const struct SpriteSheet sSpriteSheet_UpDown =
{
    gPbuUpDown_Gfx, 0x200, TAG_UP_DOWN
};

static const struct SpritePalette sSpritePalette_UpDown =
{
    gPbuUpDown_Pal, TAG_UP_DOWN
};

static const s16 sUpDownCoordsOnGraph[CONDITION_COUNT][2] =
{
    [CONDITION_COOL]   = {156,  36},
    [CONDITION_TOUGH]  = {117,  59},
    [CONDITION_SMART]  = {117, 118},
    [CONDITION_CUTE]   = {197, 118},
    [CONDITION_BEAUTY] = {197,  59}
};

static const struct OamData sOam_UpDown =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_Up[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Down[] =
{
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_UpDown[] =
{
    sAnim_Up,
    sAnim_Down
};

static const struct SpriteTemplate sSpriteTemplate_UpDown =
{
    .tileTag = TAG_UP_DOWN,
    .paletteTag = TAG_UP_DOWN,
    .oam = &sOam_UpDown,
    .anims = sAnims_UpDown,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOam_Condition =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_Condition_0[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Condition_1[] =
{
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_END
};

static const union AnimCmd sAnim_Condition_2[] =
{
    ANIMCMD_FRAME(64, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_Condition[] =
{
    sAnim_Condition_0,
    sAnim_Condition_1,
    sAnim_Condition_2
};

static const struct SpriteTemplate sSpriteTemplate_Condition =
{
    .tileTag = TAG_CONDITION,
    .paletteTag = TAG_CONDITION,
    .oam = &sOam_Condition,
    .anims = sAnims_Condition,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Condition,
};

static const struct SpritePalette sSpritePalette_Condition =
{
    gPbuCondition_Pal, TAG_CONDITION
};

// When first opening the selection screen

// When returning to the selection screen after feeding a pokeblock to a mon



static void CB2_UsePokeblockMenu(void)
{
    sInfo->callback();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void VBlankCB_UsePokeblockMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ConditionGraph_Draw(&sMenu->graph);
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void SetUsePokeblockCallback(void (*func)(void))
{
    sInfo->callback = func;
    sInfo->mainState = 0;
}

static void LoadUsePokeblockMenu(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        sMenu->curMonSpriteId = SPRITE_NONE;
        ConditionGraph_Init(&sMenu->graph);
        sInfo->mainState++;
        break;
    case 1:
        ResetSpriteData();
        FreeAllSpritePalettes();
        sInfo->mainState++;
        break;
    case 2:
        SetVBlankCallback(NULL);
        CpuFill32(0, (void *)(VRAM), VRAM_SIZE);
        sInfo->mainState++;
        break;
    case 3:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        InitWindows(sWindowTemplates);
        DeactivateAllTextPrinters();
        TextWindow_SetStdFrame0_WithPal(0, 0x97, 14 * 16);   //原文 LoadUserWindowBorderGfx(0, 0x97, BG_PLTT_ID(14))
        sInfo->mainState++;
        break;
    case 4:
        sInfo->mainState++;
        break;
    case 5:
        if (!LoadConditionTitle())
            sInfo->mainState++;
        break;
    case 6:
        LoadPartyInfo();
        sInfo->mainState++;
        break;
    case 7:
        if (!LoadUsePokeblockMenuGfx())
            sInfo->mainState++;
        break;
    case 8:
        UpdateMonPic(0);
        LoadAndCreateSelectionIcons();
        sInfo->mainState++;
        break;
    case 9:
        if (!MoveConditionMonOnscreen(&sMenu->curMonXOffset))
            sInfo->mainState++;
        break;
    case 10:
        sInfo->mainState++;
        break;
    case 11:
        ConditionGraph_CalcPositions(sMenu->graph.conditions[0], sMenu->graph.savedPositions[0]);
        ConditionGraph_InitResetScanline(&sMenu->graph);
        sInfo->mainState++;
        break;
    case 12:
        if (!ConditionGraph_ResetScanline(&sMenu->graph))
        {
            ConditionGraph_SetNewPositions(&sMenu->graph, sMenu->graph.savedPositions[0], sMenu->graph.savedPositions[0]);
            sInfo->mainState++;
        }
        break;
    case 13:
        ConditionGraph_Update(&sMenu->graph);
        sInfo->mainState++;
        break;
    case 14:
        PutWindowTilemap(WIN_NAME);
        PutWindowTilemap(WIN_NATURE);
        UpdateMonInfoText(0, TRUE);
        sInfo->mainState++;
        break;
    case 15:
        SetUsePokeblockCallback(ShowUsePokeblockMenu);
        break;
    }
}

static void ShowUsePokeblockMenu(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        BeginNormalPaletteFade(PBU_ALL_PALETTES, 0, 16, 0, 0);
        SetVBlankCallback(VBlankCB_UsePokeblockMenu);
        ShowBg(0);
        ShowBg(1);
        ShowBg(3);
        ShowBg(2);
        sInfo->mainState++;
        break;
    case 1:
        if (!gPaletteFade->active)
        {
            ResetConditionSparkleSprites(sMenu->sparkles);
            if (sMenu->info.curSelection != sMenu->info.numSelections - 1)
            {
                u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
                CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
            }

            SetUsePokeblockCallback(UsePokeblockMenu);
        }
        break;
    }
}

enum {
    STATE_HANDLE_INPUT,
    STATE_UPDATE_SELECTION,
    STATE_2, // unused state
    STATE_CLOSE,
    STATE_4, // unused state
    STATE_CONFIRM_SELECTION,
    STATE_HANDLE_CONFIRMATION,
    STATE_WAIT_MSG,
};

static void UsePokeblockMenu(void)
{
    bool8 loading;

    switch (sInfo->mainState)
    {
    case STATE_HANDLE_INPUT:
        if (JOY_HELD(DPAD_UP))
        {
            PlaySE(SE_SELECT);
            UpdateSelection(TRUE);
            DestroyConditionSparkleSprites(sMenu->sparkles);
            sInfo->mainState = STATE_UPDATE_SELECTION;
        }
        else if (JOY_HELD(DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            UpdateSelection(FALSE);
            DestroyConditionSparkleSprites(sMenu->sparkles);
            sInfo->mainState = STATE_UPDATE_SELECTION;
        }
        else if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sInfo->mainState = STATE_CLOSE;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);

            // If last item, selected Cancel. Otherwise selected mon
            if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
                sInfo->mainState = STATE_CLOSE;
            else
                sInfo->mainState = STATE_CONFIRM_SELECTION;
        }
        break;
    case STATE_UPDATE_SELECTION:
        loading = sMenu->info.loadNewSelection();
        if (!loading)
            sInfo->mainState = STATE_HANDLE_INPUT;
        break;
    case STATE_2:
        break;
    case STATE_CLOSE:
        SetUsePokeblockCallback(CloseUsePokeblockMenu);
        break;
    case STATE_4:
        break;
    case STATE_CONFIRM_SELECTION:
        AskUsePokeblock();
        sInfo->mainState++;
        break;
    case STATE_HANDLE_CONFIRMATION:
        switch (HandleAskUsePokeblockInput())
        {
        case 1: // NO
        case MENU_B_PRESSED:
            sInfo->mainState = STATE_HANDLE_INPUT;
            break;
        case 0: // YES
            if (IsSheenMaxed())
            {
                PrintWontEatAnymore();
                sInfo->mainState = STATE_WAIT_MSG;
            }
            else
            {
                SetUsePokeblockCallback(FeedPokeblockToMon);
            }
            break;
        }
        break;
    case STATE_WAIT_MSG:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            EraseMenuWindow();
            sInfo->mainState = STATE_HANDLE_INPUT;
        }
        break;
    }
}



static void ShowPokeblockResults(void)
{
    switch (sInfo->mainState)
    {
    case 0:
        sInfo->mon = gPlayerParty;
        sInfo->mon += sMenu->party[sMenu->info.curSelection].monId;
        DestroyConditionSparkleSprites(sMenu->sparkles);
        sInfo->mainState++;
        break;
    case 1:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            sInfo->mainState++;
        break;
    case 2:
        // 計算と保存は「はい」のとき済み（FeedPokeblockToMon）
        ConditionGraph_CalcPositions(sInfo->conditionsAfterBlock, sMenu->graph.savedPositions[CONDITION_GRAPH_LOAD_MAX - 1]);
        ConditionGraph_SetNewPositions(&sMenu->graph, sMenu->graph.savedPositions[sMenu->curLoadId], sMenu->graph.savedPositions[CONDITION_GRAPH_LOAD_MAX - 1]);
        LoadAndCreateUpDownSprites();
        sInfo->mainState++;
        break;
    case 3:
        if (!ConditionGraph_TryUpdate(&sMenu->graph))
        {
            CalculateNumAdditionalSparkles(GetPartyIdFromSelectionId(sMenu->info.curSelection));
            if (sMenu->info.curSelection != sMenu->info.numSelections - 1)
            {
                u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
                CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
            }

            sInfo->timer = 0;
            sInfo->mainState++;
        }
        break;
    case 4:
        if (++sInfo->timer > 16)
        {
            PrintFirstEnhancement();
            sInfo->mainState++;
        }
        break;
    case 5:
        if (JOY_NEW(A_BUTTON | B_BUTTON) && !TryPrintNextEnhancement())
        {
            SetUsePokeblockCallback(CloseUsePokeblockMenu);
        }
        break;
    }
}

static void CloseUsePokeblockMenu(void)
{
    u8 i;

    switch (sInfo->mainState)
    {
    case 0:
        BeginNormalPaletteFade(PBU_ALL_PALETTES, 0, 0, 16, 0);
        sInfo->mainState++;
        break;
    case 1:
        if (!gPaletteFade->active)
            sInfo->mainState = 2;
        break;
    case 2:
        gScanlineEffect.state = 3;
        ScanlineEffect_InitHBlankDmaTransfer();
        sInfo->mainState++;
        break;
    case 3:
        SetMainCallback2(sInfo->exitCallback);
        FreeConditionSparkles(sMenu->sparkles);
        for (i = 0; i < ARRAY_COUNT(sMenu->selectionIconSpriteIds); i++)
            DestroySprite(&gSprites[sMenu->selectionIconSpriteIds[i]]);

        FreeSpriteTilesByTag(TAG_UP_DOWN);
        FreeSpriteTilesByTag(TAG_CONDITION);
        FreeSpritePaletteByTag(TAG_UP_DOWN);
        FreeSpritePaletteByTag(TAG_CONDITION);

        for (i = 0; i < ARRAY_COUNT(sMenu->condition); i++)
            DestroySprite(sMenu->condition[i]);

        if (sMenu->curMonSpriteId != SPRITE_NONE)
            DestroySprite(&gSprites[sMenu->curMonSpriteId]);

        SetVBlankCallback(NULL);
        FREE_AND_SET_NULL(sGraph_Tilemap);
        FREE_AND_SET_NULL(sGraph_Gfx);
        FREE_AND_SET_NULL(sMonFrame_TilemapPtr);
        FREE_AND_SET_NULL(sMenu);
        FreeAllWindowBuffers();
        break;
    }
}

static void AskUsePokeblock(void)
{
    u8 stringBuffer[0x40];

    GetMonData(&gPlayerParty[GetPartyIdFromSelectionId(sMenu->info.curSelection)], MON_DATA_NICKNAME, stringBuffer);
    StringAppend(stringBuffer, gText_Pbu_GetsAPokeBlockQuestion);
    StringCopy(gStringVar4, stringBuffer);
    FillWindowPixelBuffer(WIN_TEXT, 17);
    DrawTextBorderOuter(WIN_TEXT, 151, 14);
    AddTextPrinterParameterized(WIN_TEXT, PBU_FONT, gStringVar4, 0, 1, 0, NULL);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
    CreateYesNoMenu(&sUsePokeblockYesNoWinTemplate, PBU_FONT, 0, 2, 151, 14, 0);
}

static s8 HandleAskUsePokeblockInput(void)
{
    s8 menuItem = Menu_ProcessInputNoWrapClearOnChoose();

    switch (menuItem)
    {
    case 0: // YES
        break;
    case MENU_B_PRESSED:
    case 1: // NO
        PlaySE(SE_SELECT);
        ClearStdWindowAndFrameToTransparent(2, FALSE);   //原文 rbox_fill_rectangle
        ClearWindowTilemap(2);
        break;
    }

    return menuItem;
}

static void PrintFirstEnhancement(void)
{
    DrawTextBorderOuter(WIN_TEXT, 151, 14);
    FillWindowPixelBuffer(WIN_TEXT, 17);

    for (sInfo->condition = 0; sInfo->condition < CONDITION_COUNT; sInfo->condition++)
    {
        if (sInfo->enhancements[sInfo->condition] != 0)
            break;
    }

    if (sInfo->condition < CONDITION_COUNT)
        BufferEnhancedText(gStringVar4, sInfo->condition, sInfo->enhancements[sInfo->condition]);
    else
        BufferEnhancedText(gStringVar4, sInfo->condition, 0);

    PrintMenuWindowText(gStringVar4);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
}

static bool8 TryPrintNextEnhancement(void)
{
    FillWindowPixelBuffer(WIN_TEXT, 17);

    while (1)
    {
        sInfo->condition++;
        if (sInfo->condition < CONDITION_COUNT)
        {
            if (sInfo->enhancements[sInfo->condition] != 0)
                break;
        }
        else
        {
            sInfo->condition = CONDITION_COUNT;
            return FALSE;
        }
    }

    BufferEnhancedText(gStringVar4, sInfo->condition, sInfo->enhancements[sInfo->condition]);
    PrintMenuWindowText(gStringVar4);
    CopyWindowToVram(WIN_TEXT, COPYWIN_GFX);

    return TRUE;
}

static void PrintWontEatAnymore(void)
{
    FillWindowPixelBuffer(WIN_TEXT, 17);
    DrawTextBorderOuter(WIN_TEXT, 151, 14);
    AddTextPrinterParameterized(WIN_TEXT, PBU_FONT, gText_Pbu_WontEatAnymore, 0, 1, 0, NULL);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
}

static void EraseMenuWindow(void)
{
    ClearStdWindowAndFrameToTransparent(WIN_TEXT, FALSE);   //原文 rbox_fill_rectangle
    ClearWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
}

static void PrintMenuWindowText(const u8 *message)
{
    AddTextPrinterParameterized(WIN_TEXT, PBU_FONT, gStringVar4, 0, 1, 0, NULL);
}

static void BufferEnhancedText(u8 *dest, u8 condition, s16 enhancement)
{
    switch (enhancement)
    {
    case 1 ... 32767: // if > 0
        enhancement = 0;
        // fallthrough
    case -32768 ... -1: // if < 0
        if (enhancement)
            dest[(u16)enhancement] += 0; // something you can't imagine
        StringCopy(dest, sConditionNames[condition]);
        StringAppend(dest, gText_Pbu_WasEnhanced);
        break;
    case 0:
        StringCopy(dest, gText_Pbu_NothingChanged);
        break;
    }
}

static void GetMonConditions(struct Pokemon *mon, u8 *data)
{
    u16 i;
    u8 v[CONDITION_COUNT_VALUES];

    HackCondition_Get((struct BoxPokemon *) mon, v);
    for (i = 0; i < CONDITION_COUNT; i++)
        data[i] = v[sConditionToCond[i]];
}

// 原文は SetMonData で 6 回書くが、ここは表（hack_condition.c）へ 1 回で書く。書けなかったら FALSE（表は変わらない）
static bool8 AddPokeblockToConditions(struct Pokeblock *pokeblock, struct Pokemon *mon)
{
    u16 i;
    s16 stat;
    u8 v[CONDITION_COUNT_VALUES];

    HackCondition_Get((struct BoxPokemon *) mon, v);
    if (v[HCOND_SHEEN] != MAX_SHEEN)
    {
        CalculatePokeblockEffectiveness(pokeblock, mon);
        for (i = 0; i < CONDITION_COUNT; i++)
        {
            stat = v[sConditionToCond[i]] + sInfo->pokeblockStatBoosts[i];
            if (stat < 0)
                stat = 0;
            if (stat > MAX_CONDITION)
                stat = MAX_CONDITION;
            v[sConditionToCond[i]] = stat;
        }

        stat = v[HCOND_SHEEN] + pokeblock->feel;
        if (stat > MAX_SHEEN)
            stat = MAX_SHEEN;
        v[HCOND_SHEEN] = stat;
    }
    return HackCondition_Set((struct BoxPokemon *) mon, v);
}

// 食べさせるとき 1 回だけ（原文は結果の画面の中で計算するが、保存の順を決めるため「はい」のときに先に行う）。FALSE = 保存できなかった（何も変えていない）
static bool8 CalculateConditionEnhancements(void)
{
    u16 i;
    struct Pokemon *mon = gPlayerParty;
    mon += sMenu->party[sMenu->info.curSelection].monId;

    GetMonConditions(mon, sInfo->conditionsBeforeBlock);
    if (!AddPokeblockToConditions(sInfo->pokeblock, mon))
        return FALSE;
    GetMonConditions(mon, sInfo->conditionsAfterBlock);
    for (i = 0; i < CONDITION_COUNT; i++)
        sInfo->enhancements[i] = sInfo->conditionsAfterBlock[i] - sInfo->conditionsBeforeBlock[i];
    return TRUE;
}

static void CalculatePokeblockEffectiveness(struct Pokeblock *pokeblock, struct Pokemon *mon)
{
    s8 i, direction, flavor;

    sInfo->pokeblockStatBoosts[CONDITION_COOL] = pokeblock->spicy;
    sInfo->pokeblockStatBoosts[CONDITION_TOUGH] = pokeblock->sour;
    sInfo->pokeblockStatBoosts[CONDITION_SMART] = pokeblock->bitter;
    sInfo->pokeblockStatBoosts[CONDITION_CUTE] = pokeblock->sweet;
    sInfo->pokeblockStatBoosts[CONDITION_BEAUTY] = pokeblock->dry;

    if (gPokeblockGain > 0)
        direction = 1;
    else if (gPokeblockGain < 0)
        direction = -1;
    else
        return;

    for (i = 0; i < CONDITION_COUNT; i++)
    {
        s16 amount = sInfo->pokeblockStatBoosts[i];
        s8 boost = amount / 10;

        if (amount % 10 >= 5) // round to the nearest
            boost++;

        flavor = GetMonFlavorRelation(mon, sConditionToFlavor[i]);
        if (flavor == direction)
            sInfo->pokeblockStatBoosts[i] += boost * flavor;
    }
}

static bool8 IsSheenMaxed(void)
{
    struct BoxPokemon *mon = (struct BoxPokemon *) &gPlayerParty[sMenu->party[sMenu->info.curSelection].monId];

    return HackCondition_GetOne(mon, HCOND_SHEEN) == MAX_SHEEN;
}

static u8 GetPartyIdFromSelectionId(u8 selectionId)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (!GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
        {
            if (selectionId == 0)
                return i;
            selectionId--;
        }
    }

    return 0;
}

// Eggs are not viewable on the condition screen, so count how many are skipped over to reach the party id


static void LoadAndCreateUpDownSprites(void)
{
    u16 i;

    LoadSpriteSheet(&sSpriteSheet_UpDown);
    LoadSpritePalette(&sSpritePalette_UpDown);
    sInfo->numEnhancements = 0;

    for (i = 0; i < CONDITION_COUNT; i++)
    {
        if (sInfo->enhancements[i] != 0)
        {
            u16 spriteId = CreateSprite(&sSpriteTemplate_UpDown, sUpDownCoordsOnGraph[i][0], sUpDownCoordsOnGraph[i][1], 0);
            if (spriteId != MAX_SPRITES)
            {
                if (sInfo->enhancements[i] != 0) // Always true here
                    gSprites[spriteId].callback = SpriteCB_UpDown;
                sInfo->numEnhancements++;
            }
        }
    }
}

#define tTimer data[0]

static void SpriteCB_UpDown(struct Sprite *sprite)
{
    if (sprite->tTimer < 6)
        sprite->pos2.y -= 2;
    else if (sprite->tTimer < 12)
        sprite->pos2.y += 2;

    if (++sprite->tTimer > 60)
    {
        DestroySprite(sprite);
        sInfo->numEnhancements--;
    }
}

#undef tTimer

static void LoadPartyInfo(void)
{
    u16 i;
    u16 numMons;

    for (i = 0, numMons = 0; i < CalculatePlayerPartyCount(); i++)
    {
        if (!GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
        {
            sMenu->party[numMons].boxId = TOTAL_BOXES_COUNT;
            sMenu->party[numMons].monId = i;
            sMenu->party[numMons].data = 0;
            numMons++;
        }
    }

    sMenu->info.curSelection = 0;
    sMenu->info.numSelections = numMons + 1;
    LoadInitialMonInfo();
}

static void LoadInitialMonInfo(void)
{
    s16 nextSelection, prevSelection;

    LoadMonInfo(sMenu->info.curSelection, 0);
    sMenu->curLoadId = 0;
    sMenu->nextLoadId = 1;
    sMenu->prevLoadId = 2;

    nextSelection = sMenu->info.curSelection + 1;
    if (nextSelection >= sMenu->info.numSelections)
        nextSelection = 0;

    prevSelection = sMenu->info.curSelection - 1;
    if (prevSelection < 0)
        prevSelection = sMenu->info.numSelections - 1;

    LoadMonInfo(nextSelection, 1);
    LoadMonInfo(prevSelection, 2);
}

static void LoadMonInfo(s16 partyId, u8 loadId)
{
    u8 monId = sMenu->party[partyId].monId;
    u8 numSelections = sMenu->info.numSelections;

    GetConditionMenuMonName(sMenu->monNameStrings[loadId], monId, partyId, numSelections);
    GetConditionMenuMonConditions(&sMenu->graph, sMenu->numSparkles, monId, partyId, loadId, numSelections);
    GetConditionMenuMonGfx(sMenu->partySheets[loadId], sMenu->partyPalettes[loadId], monId, partyId, numSelections);

    // 演出（3b）のあとの作り直し: 食べた子は表がもう新しい値なので、ゲージが動く前の絵は前の値に置き直す（原文は食べたあとに表を書くので不要）
    if (sInfo->resume && partyId != numSelections - 1 && monId == sInfo->resumeMonId)
    {
        u8 i;

        for (i = 0; i < CONDITION_COUNT; i++)
            sMenu->graph.conditions[loadId][i] = sInfo->conditionsBeforeBlock[i];
        sMenu->numSparkles[loadId] = GET_NUM_CONDITION_SPARKLES(sInfo->sheenBefore);
        ConditionGraph_CalcPositions(sMenu->graph.conditions[loadId], sMenu->graph.savedPositions[loadId]);
    }
}

static void UpdateMonPic(u8 loadId)
{
    u8 spriteId;
    struct SpriteTemplate spriteTemplate;
    struct SpriteSheet spriteSheet;
    struct SpritePalette spritePal;

    if (sMenu->curMonSpriteId == SPRITE_NONE)
    {
        LoadConditionMonPicTemplate(&spriteSheet, &spriteTemplate, &spritePal);
        spriteSheet.data = sMenu->partySheets[loadId];
        spritePal.data = sMenu->partyPalettes[loadId];
        sMenu->curMonPalette = LoadSpritePalette(&spritePal);
        sMenu->curMonSheet = LoadSpriteSheet(&spriteSheet);
        spriteId = CreateSprite(&spriteTemplate, 38, 104, 0);
        sMenu->curMonSpriteId = spriteId;
        if (spriteId == MAX_SPRITES)
        {
            FreeSpriteTilesByTag(TAG_CONDITION_MON);
            FreeSpritePaletteByTag(TAG_CONDITION_MON);
            sMenu->curMonSpriteId = SPRITE_NONE;
        }
        else
        {
            sMenu->curMonSpriteId = spriteId;
            gSprites[sMenu->curMonSpriteId].callback = SpriteCB_MonPic;
            gSprites[sMenu->curMonSpriteId].pos2.y -= 34;
            sMenu->curMonTileStart = (void *)(OBJ_VRAM0 + (sMenu->curMonSheet * 32));
            sMenu->curMonPalette = (0x100 + sMenu->curMonPalette * 16);
        }
    }
    else
    {
        CpuSet(sMenu->partySheets[loadId], sMenu->curMonTileStart, MON_PIC_SIZE / 2);   //原文 Dma3CopyLarge16_
        LoadPalette(sMenu->partyPalettes[loadId], sMenu->curMonPalette, 32);
    }
}

static void LoadAndCreateSelectionIcons(void)
{
    u16 i, spriteId;
    struct SpriteSheet spriteSheets[4];
    struct SpriteTemplate spriteTemplate;
    struct SpritePalette spritePals[3];
    struct SpriteSheet spriteSheet2;
    struct SpritePalette spritePal2;

    LoadConditionSelectionIcons(spriteSheets, &spriteTemplate, spritePals);
    LoadSpriteSheets(spriteSheets);
    LoadSpritePalettes(spritePals);

    // Fill Poké Ball selection icons up to number in party
    for (i = 0; i < sMenu->info.numSelections - 1; i++)
    {
        spriteId = CreateSprite(&spriteTemplate, 226, (i * 20) + 8, 0);
        if (spriteId != MAX_SPRITES)
        {
            sMenu->selectionIconSpriteIds[i] = spriteId;
            gSprites[spriteId].data[0] = i;
            gSprites[spriteId].callback = SpriteCB_SelectionIconPokeball;
        }
        else
        {
            sMenu->selectionIconSpriteIds[i] = -1;
        }
    }

    // Fill placeholder icons for remaining (empty) party slots
    spriteTemplate.tileTag = TAG_CONDITION_BALL_PLACEHOLDER;
    for (; i < PARTY_SIZE; i++)
    {
        spriteId = CreateSprite(&spriteTemplate, 230, (i * 20) + 8, 0);
        if (spriteId != MAX_SPRITES)
        {
            sMenu->selectionIconSpriteIds[i] = spriteId;
            gSprites[spriteId].oam.size = 0;
        }
        else
        {
            sMenu->selectionIconSpriteIds[i] = -1;
        }
    }

    // Add cancel selection icon at bottom
    spriteTemplate.tileTag = TAG_CONDITION_CANCEL;
    spriteTemplate.callback = SpriteCB_SelectionIconCancel;
    spriteId = CreateSprite(&spriteTemplate, 222, (i * 20) + 8, 0);
    if (spriteId != MAX_SPRITES)
    {
        sMenu->selectionIconSpriteIds[i] = spriteId;
        gSprites[spriteId].oam.shape = SPRITE_SHAPE(32x16);
        gSprites[spriteId].oam.size = SPRITE_SIZE(32x16);
    }
    else
    {
        sMenu->selectionIconSpriteIds[i] = -1;
    }

    LoadConditionSparkle(&spriteSheet2, &spritePal2);
    LoadSpriteSheet(&spriteSheet2);
    LoadSpritePalette(&spritePal2);
}

static bool8 LoadUsePokeblockMenuGfx(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 136 << 6, BG_COORD_SET);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(11, 4));
        break;
    case 1:
        sGraph_Gfx = AllocZeroed(PBU_GRAPH_GFX_SIZE);
        sGraph_Tilemap = AllocZeroed(1280);
        sMonFrame_TilemapPtr = AllocZeroed(1280);
        break;
    case 2:
        LZ77UnCompVram(gPbuMonFrame_Tilemap, sMonFrame_TilemapPtr);
        break;
    case 3:
        LoadBgTiles(3, gPbuMonFrame_Gfx, 224, 0);
        break;
    case 4:
         LoadBgTilemap(3, sMonFrame_TilemapPtr, 1280, 0);
        break;
    case 5:
        LoadPalette(gPbuMonFrame_Pal, (13 * 16), 32);
        sMenu->curMonXOffset = -80;
        break;
    case 6:
        LZ77UnCompVram(gPbuGraph_Gfx, sGraph_Gfx);
        break;
    case 7:
        LZ77UnCompVram(gPbuGraph_Tilemap, sGraph_Tilemap);
        LoadPalette(gPbuGraph_Pal, (2 * 16), 32);
        break;
    case 8:
        LoadBgTiles(1, sGraph_Gfx, PBU_GRAPH_GFX_SIZE, 160 << 2);
        break;
    case 9:
        SetBgTilemapBuffer(1, sGraph_Tilemap);
        CopyToBgTilemapBufferRect_ChangePalette(1, gPbuNatureWin, 0, 13, 12, 4, 17);   //原文 CopyToBgTilemapBufferRect（パレットは変えない＝17）
        CopyBgTilemapBufferToVram(1);
        break;
    case 10:
        LZ77UnCompVram(gPbuGraphData_Tilemap, sMenu->tilemapBuffer);
        break;
    case 11:
        LoadBgTilemap(2, sMenu->tilemapBuffer, 1280, 0);
        LoadPalette(gPbuCondGraphData_Pal, (3 * 16), 32);
        LoadPalette(gPbuCondText_Pal, (15 * 16), 32);
        ConditionGraph_InitWindow(2);
        break;
    default:
        sMenu->info.helperState = 0;
        return FALSE;
    }

    sMenu->info.helperState++;
    return TRUE;
}

static void UpdateMonInfoText(u16 loadId, bool8 firstPrint)
{
    u8 partyIndex;
    u8 nature;
    u8 *str;

    FillWindowPixelBuffer(WIN_NAME, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_NATURE, PIXEL_FILL(0));
    if (sMenu->info.curSelection != sMenu->info.numSelections - 1)
    {
        AddTextPrinterParameterized(WIN_NAME, PBU_FONT, sMenu->monNameStrings[loadId], 0, 1, 0, NULL);
        partyIndex = GetPartyIdFromSelectionId(sMenu->info.curSelection);
        nature = GetNature(&gPlayerParty[partyIndex]);
        str = StringCopy(sMenu->info.natureText, gText_Pbu_NatureSlash);
        str = StringCopy(str, gNatureNamePointers[nature]);
        AddTextPrinterParameterized3(WIN_NATURE, PBU_FONT, 2, 1, sNatureTextColors, 0, sMenu->info.natureText);
    }

    if (firstPrint)
    {
        CopyWindowToVram(WIN_NAME, COPYWIN_FULL);
        CopyWindowToVram(WIN_NATURE, COPYWIN_FULL);
    }
    else
    {
        CopyWindowToVram(WIN_NAME, COPYWIN_GFX);
        CopyWindowToVram(WIN_NATURE, COPYWIN_GFX);
    }
}

static void UpdateSelection(bool8 up)
{
    u16 newLoadId;
    bool32 startedOnMon, endedOnMon;

    if (up)
        newLoadId = sMenu->prevLoadId;
    else
        newLoadId = sMenu->nextLoadId;

    ConditionGraph_SetNewPositions(&sMenu->graph, sMenu->graph.savedPositions[sMenu->curLoadId], sMenu->graph.savedPositions[newLoadId]);

    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        startedOnMon = FALSE; // moving off of Cancel
    else
        startedOnMon = TRUE;

    if (up)
    {
        sMenu->prevLoadId = sMenu->nextLoadId; // temporarily store nextLoadId, prevLoadId no longer needed
        sMenu->nextLoadId = sMenu->curLoadId;
        sMenu->curLoadId = newLoadId;
        sMenu->toLoadId = sMenu->prevLoadId; // next load will be the mon that's one up from new selection

        // Check for wrap to bottom of list
        sMenu->info.curSelection = (sMenu->info.curSelection == 0)
            ? sMenu->info.numSelections - 1
            : sMenu->info.curSelection - 1;

        sMenu->toLoadSelection = (sMenu->info.curSelection == 0)
            ? sMenu->info.numSelections - 1
            : sMenu->info.curSelection - 1;
    }
    else
    {
        sMenu->nextLoadId = sMenu->prevLoadId; // temporarily store prevLoadId, nextLoadId no longer needed
        sMenu->prevLoadId = sMenu->curLoadId;
        sMenu->curLoadId = newLoadId;
        sMenu->toLoadId = sMenu->nextLoadId; // next load will be the mon that's one down from new selection

        // Check for wrap to top of list
        sMenu->info.curSelection = (sMenu->info.curSelection < sMenu->info.numSelections - 1)
            ? sMenu->info.curSelection + 1
            : 0;

        sMenu->toLoadSelection = (sMenu->info.curSelection < sMenu->info.numSelections - 1)
            ? sMenu->info.curSelection + 1
            : 0;
    }

    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        endedOnMon = FALSE; // moving onto Cancel
    else
        endedOnMon = TRUE;

    DestroyConditionSparkleSprites(sMenu->sparkles);

    if (!startedOnMon)
        sMenu->info.loadNewSelection = LoadNewSelection_CancelToMon;
    else if (!endedOnMon)
        sMenu->info.loadNewSelection = LoadNewSelection_MonToCancel;
    else
        sMenu->info.loadNewSelection = LoadNewSelection_MonToMon;
}

static bool8 LoadNewSelection_CancelToMon(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        UpdateMonPic(sMenu->curLoadId);
        sMenu->info.helperState++;
        break;
    case 1:
        UpdateMonInfoText(sMenu->curLoadId, FALSE);
        sMenu->info.helperState++;
        break;
    case 2:
        if (!ConditionMenu_UpdateMonEnter(&sMenu->graph, &sMenu->curMonXOffset))
        {
            // Load the new adjacent Pokémon (not the one being shown)
            LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
            sMenu->info.helperState++;
        }
        break;
    case 3:
        ResetConditionSparkleSprites(sMenu->sparkles);
        if (sMenu->info.curSelection != sMenu->info.numSelections - 1)
        {
            u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
            CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
        }

        sMenu->info.helperState = 0;
        return FALSE;
    }

    return TRUE;
}

static bool8 LoadNewSelection_MonToCancel(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        if (!ConditionMenu_UpdateMonExit(&sMenu->graph, &sMenu->curMonXOffset))
            sMenu->info.helperState++;
        break;
    case 1:
        UpdateMonInfoText(sMenu->curLoadId, FALSE);
        sMenu->info.helperState++;
        break;
    case 2:
        LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
        sMenu->info.helperState++;
        break;
    case 3:
        sMenu->info.helperState = 0;
        return FALSE;
    }

    return TRUE;
}

static bool8 LoadNewSelection_MonToMon(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        ConditionGraph_TryUpdate(&sMenu->graph);
        if (!MoveConditionMonOffscreen(&sMenu->curMonXOffset))
        {
            UpdateMonPic(sMenu->curLoadId);
            sMenu->info.helperState++;
        }
        break;
    case 1:
        UpdateMonInfoText(sMenu->curLoadId, FALSE);
        sMenu->info.helperState++;
        break;
    case 2:
        if (!ConditionMenu_UpdateMonEnter(&sMenu->graph, &sMenu->curMonXOffset))
        {
            // Load the new adjacent Pokémon (not the one being shown)
            LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
            sMenu->info.helperState++;
        }
        break;
    case 3:
        ResetConditionSparkleSprites(sMenu->sparkles);
        if (sMenu->info.curSelection != sMenu->info.numSelections - 1)
        {
            u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
            CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
        }

        sMenu->info.helperState = 0;
        return FALSE;
    }

    return TRUE;
}

static void SpriteCB_MonPic(struct Sprite *sprite)
{
    sprite->pos1.x = sMenu->curMonXOffset + 38;
}

static void SpriteCB_SelectionIconPokeball(struct Sprite *sprite)
{
    if (sprite->data[0] == sMenu->info.curSelection)
        StartSpriteAnim(sprite, CONDITION_ICON_SELECTED);
    else
        StartSpriteAnim(sprite, CONDITION_ICON_UNSELECTED);
}

static void SpriteCB_SelectionIconCancel(struct Sprite *sprite)
{
    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        sprite->oam.paletteNum = IndexOfSpritePaletteTag(TAG_CONDITION_BALL);
    else
        sprite->oam.paletteNum = IndexOfSpritePaletteTag(TAG_CONDITION_CANCEL);
}

// Calculate the max id for sparkles/stars that appear around the Pokémon on the condition screen
// All Pokémon start with 1 sparkle (added by CreateConditionSparkleSprites), so the number here +1
// is the total number of sparkles that appear
static void CalculateNumAdditionalSparkles(u8 monIndex)
{
    u8 sheen = HackCondition_GetOne((struct BoxPokemon *) &gPlayerParty[monIndex], HCOND_SHEEN);
    sMenu->numSparkles[sMenu->curLoadId] = GET_NUM_CONDITION_SPARKLES(sheen);
}

static void LoadConditionGfx(void)
{
    struct CompressedSpriteSheet spriteSheet;
    struct SpritePalette spritePalette;

    spritePalette = sSpritePalette_Condition;
    spriteSheet.data = gPbuCondition_Gfx;
    spriteSheet.size = 0x800;
    spriteSheet.tag = TAG_CONDITION;
    LoadCompressedSpriteSheet(&spriteSheet);
    LoadSpritePalette(&spritePalette);
}

#define sSpeed   data[0]
#define sTargetX data[1]

static void CreateConditionSprite(void)
{
    u16 i;
    s16 xDiff, xStart;
    int yStart = 17;
    int speed = 8;
    struct Sprite **sprites = sMenu->condition;
    const struct SpriteTemplate *template = &sSpriteTemplate_Condition;

    for (i = 0, xDiff = 64, xStart = -96; i < 2; i++)
    {
        u8 spriteId = CreateSprite(template, i * xDiff + xStart, yStart, 0);
        if (spriteId != MAX_SPRITES)
        {
            gSprites[spriteId].sSpeed = speed;
            gSprites[spriteId].sTargetX = (i * xDiff) | 0x20;
            gSprites[spriteId].data[2] = i; // Unused
            StartSpriteAnim(&gSprites[spriteId], i);
            sprites[i] = &gSprites[spriteId];
        }
    }
}

static bool8 LoadConditionTitle(void)
{
    switch (sMenu->info.helperState)
    {
    case 0:
        LoadConditionGfx();
        sMenu->info.helperState++;
        return TRUE;
    case 1:
        CreateConditionSprite();
        sMenu->info.helperState = 0;
        return FALSE;
    }

    return FALSE;
}

// Literally the word "Condition", the title block that appears over the mon icon
static void SpriteCB_Condition(struct Sprite *sprite)
{
    s16 prevX = sprite->pos1.x;

    // Slide onscreen
    sprite->pos1.x += sprite->sSpeed;

    // Check if target position has been reached/surpassed
    if ((prevX <= sprite->sTargetX && sprite->pos1.x >= sprite->sTargetX)
     || (prevX >= sprite->sTargetX && sprite->pos1.x <= sprite->sTargetX))
    {
        // End slide onscreen, become static sprite.
        sprite->pos1.x = sprite->sTargetX;
        sprite->callback = SpriteCallbackDummy;
    }
}

//---------------------------------------------------------------- この改造の追加

// ポケモンのなつき度（追加の仕様・ユーザー決定 2026-11-09）: 好きな味（ポロックの味とせいかくの相性の合計 > 0）のポロックを食べると +3（上限 255）。
//   ふつう（= 0）・きらい（< 0）は増減なし。数を変えるときはこの 1 つの定数だけ。戦闘の効果（hack_affection.c の 220／255）・なつき進化・おんがえしがこの値を読む
#define PBU_FRIENDSHIP_GAIN 3
#define PBU_FRIENDSHIP_MAX 255

void HackPokeblock_AddFriendship(struct Pokemon* mon, s16 gain)
{
	u16 f;

	if (gain <= 0 || mon->species == SPECIES_NONE || mon->isEgg)
		return;
	f = mon->friendship + PBU_FRIENDSHIP_GAIN;
	mon->friendship = (f > PBU_FRIENDSHIP_MAX) ? PBU_FRIENDSHIP_MAX : f;
}

static bool8 SameBlock(const struct Pokeblock* a, const struct Pokeblock* b)
{
	return a->color == b->color && a->spicy == b->spicy && a->dry == b->dry && a->sweet == b->sweet && a->bitter == b->bitter && a->sour == b->sour && a->feel == b->feel;
}

// 写しと同じ中身の最初の 1 個を追加の区画のケースから外す。FALSE = 無い・書けなかった（ケースは変わらない）
static bool8 RemoveSelectedPokeblock(void)
{
	struct PokeblockImage* image = AllocZeroed(sizeof(struct PokeblockImage));
	bool8 ok = FALSE;
	u8 i;

	if (Pokeblock_Load(image))
	{
		for (i = 0; i < POKEBLOCK_CASE_SLOTS; i++)
		{
			if (image->blocks[i].color != PBLOCK_CLR_NONE && SameBlock(&image->blocks[i], sInfo->pokeblock))
			{
				ok = Pokeblock_Remove(image, i);
				break;
			}
		}
	}
	Free(image);
	return ok;
}

// ケースの「つかう」から。選んだポロックはケースの画面の入れ物（gSpecialVar_ItemId の番号・並べ直したあとの番号）にある＝追加の区画から読み直して写しを持つ
void ChooseMonToGivePokeblock(MainCallback callback)
{
	struct PokeblockImage* image = AllocZeroed(sizeof(struct PokeblockImage));
	bool8 ok = Pokeblock_Load(image);

	if (ok)
		CompactPokeblockSlots(image);
	if (!ok || gSpecialVar_ItemId >= POKEBLOCK_CASE_SLOTS || image->blocks[gSpecialVar_ItemId].color == PBLOCK_CLR_NONE)
	{
		Free(image);
		SetMainCallback2(callback);                       //読めない・番号がおかしい: 何もせずケースへ戻る
		return;
	}
	sMenu = AllocZeroed(sizeof(struct UsePokeblockMenu));
	sMenu->pokeblockCopy = image->blocks[gSpecialVar_ItemId];
	Free(image);
	sInfo->pokeblock = &sMenu->pokeblockCopy;
	sInfo->exitCallback = callback;
	SetUsePokeblockCallback(LoadUsePokeblockMenu);
	SetMainCallback2(CB2_UsePokeblockMenu);
}

// 「はい」のあと。①コンディションの保存（失敗: 食べない）→②ポロックを外す（失敗: ①を戻す）→③なつき度 → 暗転 → 演出画面（hack_pokeblock_feed.c）。
//  原文 FeedPokeblockToMon は暗転して PreparePokeblockFeedScene へ行くだけ（食べる処理は結果の画面）。ここは保存の順を決めた都合で先に済ませる（3a と同じ）。
static void FeedPokeblockToMon(void)
{
	switch (sInfo->mainState)
	{
	case 0:
	{
		struct Pokemon* mon = &gPlayerParty[GetPartyIdFromSelectionId(sMenu->info.curSelection)];
		u8 before[CONDITION_COUNT_VALUES];

		HackCondition_Get((struct BoxPokemon*) mon, before);
		gPokeblockGain = PokeblockGetGain(GetNature(mon), sInfo->pokeblock);   //原文は演出画面（Task_PrintAtePokeblockMessage）で設定
		if (!CalculateConditionEnhancements())
		{
			PlaySE(SE_ERROR);
			EraseMenuWindow();
			SetUsePokeblockCallback(UsePokeblockMenu);
			return;
		}
		if (!RemoveSelectedPokeblock())
		{
			HackCondition_Set((struct BoxPokemon*) mon, before);              //戻す（書けなければ値だけ残る＝食べた扱いにならないのでポロックも残る）
			PlaySE(SE_ERROR);
			EraseMenuWindow();
			SetUsePokeblockCallback(UsePokeblockMenu);
			return;
		}
		HackPokeblock_AddFriendship(mon, gPokeblockGain);
		sInfo->sheenBefore = before[HCOND_SHEEN];
		EraseMenuWindow();                                                    //質問の窓を消す
		DestroyConditionSparkleSprites(sMenu->sparkles);
		BeginNormalPaletteFade(PBU_ALL_PALETTES, 0, 0, 16, 0);
		sInfo->mainState++;
		break;
	}
	case 1:
		if (!gPaletteFade->active)
		{
			struct PokeblockCarry* carry = AllocZeroed(sizeof(struct PokeblockCarry));
			u8 i;

			carry->pokeblock = sMenu->pokeblockCopy;
			carry->exitCallback = sInfo->exitCallback;
			carry->gain = gPokeblockGain;
			carry->partyId = GetPartyIdFromSelectionId(sMenu->info.curSelection);
			carry->selection = sMenu->info.curSelection;
			carry->sheenBefore = sInfo->sheenBefore;
			for (i = 0; i < CONDITION_COUNT; i++)
			{
				carry->before[i] = sInfo->conditionsBeforeBlock[i];
				carry->after[i] = sInfo->conditionsAfterBlock[i];
				carry->enhancements[i] = sInfo->enhancements[i];
			}
			gScanlineEffect.state = 3;                                        //ゲージの HBlank 転送を止める（閉じるときと同じ）
			ScanlineEffect_InitHBlankDmaTransfer();
			SetVBlankCallback(NULL);
			FREE_AND_SET_NULL(sGraph_Tilemap);
			FREE_AND_SET_NULL(sGraph_Gfx);
			FREE_AND_SET_NULL(sMonFrame_TilemapPtr);
			FREE_AND_SET_NULL(sMenu);
			FreeAllWindowBuffers();
			sCarryWord = carry;
			PokeblockFeed_Start();
		}
		break;
	}
}

//---------------------------------------------------------------- 演出のあと（原文 CB2_ReturnAndChooseMonToGivePokeblock 以下）

static void CB2_ReturnToUsePokeblockMenu(void);
static void CB2_ShowUsePokeblockMenuForResults(void);

// 演出画面のあと: あげる画面を作り直す（carry の中身を入れ物へ移す）。食べた子の表はもう新しい値なので LoadMonInfo が前の値に置き直す
void CB2_PokeblockUseReturn(void)
{
	struct PokeblockCarry* carry = sCarryWord;
	u8 i;

	sMenu = AllocZeroed(sizeof(struct UsePokeblockMenu));
	sMenu->pokeblockCopy = carry->pokeblock;
	sInfo->pokeblock = &sMenu->pokeblockCopy;
	sInfo->exitCallback = carry->exitCallback;
	gPokeblockGain = carry->gain;
	for (i = 0; i < CONDITION_COUNT; i++)
	{
		sInfo->conditionsBeforeBlock[i] = carry->before[i];
		sInfo->conditionsAfterBlock[i] = carry->after[i];
		sInfo->enhancements[i] = carry->enhancements[i];
	}
	sInfo->sheenBefore = carry->sheenBefore;
	sInfo->resume = TRUE;
	sInfo->resumeMonId = carry->partyId;
	sInfo->resumeSelection = carry->selection;
	sInfo->monInTopHalf = (carry->selection <= PARTY_SIZE / 2) ? FALSE : TRUE;
	Free(carry);
	SetUsePokeblockCallback(LoadUsePokeblockMenu);
	SetMainCallback2(CB2_ReturnToUsePokeblockMenu);
}

static void CB2_ReturnToUsePokeblockMenu(void)
{
	sInfo->callback();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
	if (sInfo->callback == ShowUsePokeblockMenu)
	{
		sInfo->mainState = 0;
		SetMainCallback2(CB2_ShowUsePokeblockMenuForResults);
	}
}

static void ShowUsePokeblockMenuForResults(void)
{
	bool8 loading;

	switch (sInfo->mainState)
	{
	case 0:
		if (sMenu->info.curSelection != sInfo->resumeSelection)
		{
			UpdateSelection(sInfo->monInTopHalf);
			sInfo->mainState++;
		}
		else
		{
			sInfo->mainState = 3;
		}
		break;
	case 1:
		loading = sMenu->info.loadNewSelection();
		if (!loading)
			sInfo->mainState = 0;
		break;
	case 2:
		break;
	case 3:
		BlendPalettes(PBU_ALL_PALETTES, 16, 0);
		sInfo->mainState++;
		break;
	case 4:
		ShowBg(0);
		ShowBg(1);
		ShowBg(3);
		ShowBg(2);
		sInfo->mainState++;
		break;
	case 5:
		SetVBlankCallback(VBlankCB_UsePokeblockMenu);
		BeginNormalPaletteFade(PBU_ALL_PALETTES, 0, 16, 0, 0);
		sInfo->mainState++;
		break;
	case 6:
		if (!gPaletteFade->active)
		{
			ResetConditionSparkleSprites(sMenu->sparkles);
			SetUsePokeblockCallback(ShowPokeblockResults);
			SetMainCallback2(CB2_UsePokeblockMenu);
		}
		break;
	}
}

static void CB2_ShowUsePokeblockMenuForResults(void)
{
	ShowUsePokeblockMenuForResults();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
}
