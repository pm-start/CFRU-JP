#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/bg.h"
#include "../include/window.h"
#include "../include/text.h"
#include "../include/palette.h"
#include "../include/decompress.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/event_data.h"
#include "../include/malloc.h"
#include "../include/overworld.h"
#include "../include/scanline_effect.h"
#include "../include/sprite.h"
#include "../include/sound.h"
#include "../include/m4a.h"
#include "../include/random.h"
#include "../include/menu.h"
#include "../include/data2.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/new/Vanilla_functions.h"
#include "../include/new/Vanilla_functions_battle.h"
#include "../include/battle_anim.h"
#include "../include/new/battle_anims.h"
#include "hack_contest.h"
#include "hack_contest_addr.h"
#include "hack_contest_text.h"

/*
 * コンテスト移植 S2a（FEAT-contest・patches/0985）: 会場の画面（原文 contest.c の CB2_StartContest〜SetupContestGraphics〜幕）。
 * 原文 参考/pokeemerald/src/contest.c 1005-1470 を、日本語版エメラルドの contest.s（参考/pokeemerald-jp/asm/contest.s）と突き合わせて移した。
 * 日本語版が英語版と違う所: 窓の表（15 個・番号と並びが違う）・文字描画（font 1・y 2）・色コード（FC 02 00 FC 01 <色> の形）。
 * 状態は全部 HACK_CONTEST_PTR の先（6.13・static の書き換えは使わない）。この段では「窓・BG・幕・審査員・観客」まで。
 * 技を選ぶ画面・一次審査・ハートのメーター・拍手のメーター・順番の札・審査員の目・出場者の枠の点滅は S2b 以降。
 */

//FR の技のアニメの RAM（BPRJ.ld に無い物。素の FR の EWRAM の並び pret battle_anim.c:21-43 を BPRJ.ld の既知の 7 本（gAnimMovePower 0x2037E20・gBattleAnimArgs 0x2037E36・sAnimMoveIndex 0x2037E4C・gBattleAnimAttacker／Target 0x2037E4E／F）から数えて換算。
//  gAnimMoveTurn は S0 の試験（hack_contest_proto.c PROTO_MOVETURN）と同じ番地。実機で値が動くか確認する）
#define gAnimMoveTurn (*(volatile u8*) 0x02037E4A)
#define gAnimFriendship (*(volatile u8*) 0x02037E32)
#define gAnimBattlerSpecies ((volatile u16*) 0x02037E50)

extern u8 gMoveNames[][MOVE_NAME_LENGTH + 1];
u16 __attribute__((long_call)) LoadCompressedSpritePalette(const struct CompressedSpritePalette* src); //BPRJ.ld にある（ヘッダに宣言が無い）

// タイルマップ・パレットの原文の定数
#define HC_TAG_JUDGE 0x4E21
#define HC_TAG_JUDGE_SYMBOLS 0xABE0
#define HC_TEXT_COLOR_START 10

// 窓の番号（日本語版の窓の表 0x08560EA8・原文と違う）
#define HC_WIN_NICKNAME0 0   /* 0..3 ポケモン名 */
#define HC_WIN_TRAINER0 4    /* 4..7 トレーナー名 */
#define HC_WIN_GENERAL_TEXT 8

static const struct BgTemplate sContestBgTemplates[] =
{
	{ .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 0x18, .screenSize = 2, .paletteMode = 0, .priority = 0, .baseTile = 0 },
	{ .bg = 1, .charBaseIndex = 2, .mapBaseIndex = 0x1E, .screenSize = 2, .paletteMode = 0, .priority = 1, .baseTile = 0 },
	{ .bg = 2, .charBaseIndex = 0, .mapBaseIndex = 0x1C, .screenSize = 2, .paletteMode = 0, .priority = 0, .baseTile = 0 },
	{ .bg = 3, .charBaseIndex = 0, .mapBaseIndex = 0x1A, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0 },
};

//日本語版の窓の表（0x08560EA8）の 15 個＋終端
static const struct WindowTemplate sContestWindowTemplates[] =
{
	{ 0, 19, 0, 5, 2, 15, 0x200 }, { 0, 19, 5, 5, 2, 15, 0x214 }, { 0, 19, 10, 5, 2, 15, 0x228 }, { 0, 19, 15, 5, 2, 15, 0x23C },
	{ 0, 24, 0, 6, 2, 15, 0x250 }, { 0, 24, 5, 6, 2, 15, 0x266 }, { 0, 24, 10, 6, 2, 15, 0x27C }, { 0, 24, 15, 6, 2, 15, 0x292 },
	{ 0, 1, 15, 17, 4, 15, 0x2A8 },
	{ 0, 2, 31, 7, 2, 15, 0x2EC }, { 0, 2, 33, 7, 2, 15, 0x2FA }, { 0, 2, 35, 7, 2, 15, 0x308 }, { 0, 2, 37, 7, 2, 15, 0x316 },
	{ 0, 16, 31, 1, 2, 15, 0x324 },
	{ 0, 11, 35, 18, 4, 15, 0x326 },
	DUMMY_WIN_TEMPLATE
};

#define HC_WINDOW_COUNT 15

// 審査員（原文 sOam_Judge・sSpriteTemplate_Judge）
static const struct OamData sOam_Judge =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = 0, .bpp = 0,
	.shape = 0, .x = 0, .matrixNum = 0, .size = 3, .tileNum = 0, .priority = 3, .paletteNum = 2,
};

static const struct SpriteTemplate sSpriteTemplate_Judge =
{
	.tileTag = HC_TAG_JUDGE, .paletteTag = HC_TAG_JUDGE, .oam = &sOam_Judge,
	.anims = (const union AnimCmd* const*) 0x081F11E4 /* gDummySpriteAnimTable */, .images = NULL,
	.affineAnims = (const union AffineAnimCmd* const*) 0x081F11F0 /* gDummySpriteAffineAnimTable */,
	.callback = SpriteCallbackDummy,
};

static const struct CompressedSpriteSheet sSpriteSheet_Judge = { gHackContestJudgeGfx, 0x800, HC_TAG_JUDGE };
static const struct CompressedSpriteSheet sSpriteSheet_JudgeSymbols = { gHackContestJudgeSymbolsGfx, 0x380, HC_TAG_JUDGE_SYMBOLS };
static const struct CompressedSpritePalette sSpritePalette_JudgeSymbols = { gHackContestJudgeSymbolsPal, HC_TAG_JUDGE_SYMBOLS };

// 吹き出し（原文 gOamData_AffineOff_ObjNormal_16x16）
static const struct OamData sOam_16x16 =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = 0, .bpp = 0,
	.shape = 0, .x = 0, .matrixNum = 0, .size = 1, .tileNum = 0, .priority = 0, .paletteNum = 0,
};

static const struct SpriteTemplate sSpriteTemplate_JudgeSpeechBubble =
{
	.tileTag = HC_TAG_JUDGE_SYMBOLS, .paletteTag = HC_TAG_JUDGE_SYMBOLS, .oam = &sOam_16x16,
	.anims = (const union AnimCmd* const*) 0x081F11E4, .images = NULL,
	.affineAnims = (const union AffineAnimCmd* const*) 0x081F11F0,
	.callback = SpriteCallbackDummy,
};

// 出場ポケモンの絵の向き（原文 gAffineAnims_BattleSpriteContest[BATTLER_AFFINE_NORMAL]＝左右反転・Unown 以外）
static const union AffineAnimCmd sAffineAnim_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_Normal[] =
{
	AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
	AFFINEANIMCMD_END,
};
static const union AffineAnimCmd* const sAffineAnimsContest[] = { sAffineAnim_Flipped };
static const union AffineAnimCmd* const sAffineAnimsContestUnown[] = { sAffineAnim_Normal };

#define gMonBackPicTable ((const struct CompressedSpriteSheet*) *((u32*) 0x0800012C))
#define gMonBackPicCoords ((const struct MonCoords*) *((u32*) 0x08073DD8))

static void SetBgCopyFlag(struct HackContestState* s, u32 bg)
{
	s->bgCopyFlags |= 1 << bg;
}

static void HC_SetNextTurnSpritePriority(struct HackContestState* s, u8 priority);
static void HC_InitTurnOrder(struct HackContestState* s);

//原文 Contest_PrintTextToBg0WindowStd（日本語版: font 1・x 0・y 2・fg15 bg0 shadow8・speed 0）
static void HC_PrintTextToBg0WindowStd(struct HackContestState* s, u32 windowId, const u8* str)
{
	struct TextPrinterTemplate t;

	t.currentChar = str;
	t.windowId = windowId;
	t.fontId = 1;
	t.x = 0;
	t.y = 2;
	t.currentX = 0;
	t.currentY = 2;
	t.letterSpacing = 0;
	t.lineSpacing = 0;
	t.unk = 0;
	t.fgColor = 15;
	t.bgColor = 0;
	t.shadowColor = 8;
	ContestFr_AddTextPrinter(&t, 0, NULL);
	PutWindowTilemap(windowId);
	SetBgCopyFlag(s, 0);
}

//日本語版 Contest_CopyStringWithColor: StringCopy(gDisplayedStringBattle, ColorTransparent)→戻り値（終端 FF の所）に色→ +1 から本文
static u8* HC_CopyStringWithColor(const u8* string, u8 color)
{
	u8* ptr = StringCopy(gDisplayedStringBattle, gHackContest_ColorTransparent);

	*ptr = color;
	return StringCopy(ptr + 1, string);
}

static void HC_PrintContestantTrainerName(struct HackContestState* s, u8 contestant)
{
	u8 buffer[PLAYER_NAME_LENGTH + 4];

	StringCopy(buffer, gHackContest_Slash);
	StringAppend(buffer, s->mons[contestant].trainerName);
	HC_CopyStringWithColor(buffer, contestant + HC_TEXT_COLOR_START);
	HC_PrintTextToBg0WindowStd(s, s->turnOrder[contestant] + HC_WIN_TRAINER0, gDisplayedStringBattle);
}

static void HC_PrintContestantMonName(struct HackContestState* s, u8 contestant)
{
	HC_CopyStringWithColor(s->mons[contestant].nickname, contestant + HC_TEXT_COLOR_START);
	HC_PrintTextToBg0WindowStd(s, s->turnOrder[contestant] + HC_WIN_NICKNAME0, gDisplayedStringBattle);
}

static void HC_DrawContestantWindowText(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		FillWindowPixelBuffer(s->turnOrder[i], PIXEL_FILL(0));
		FillWindowPixelBuffer(s->turnOrder[i] + HC_WIN_TRAINER0, PIXEL_FILL(0));
		HC_PrintContestantMonName(s, i);
		HC_PrintContestantTrainerName(s, i);
	}
}

static void HC_DrawContestantWindows(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		LoadPalette(s->cachedWindowPalettes[5 + i], 16 * (5 + s->turnOrder[i]), 32);
	HC_DrawContestantWindowText(s);
}

static void HC_BgFillBoxWithTile(struct HackContestState* s, u8 bg, u16 firstTile, u8 x, u8 y, u8 w, u8 h, u8 palSlot)
{
	ContestFr_WriteSequenceToBgTilemapBuffer(bg, firstTile, x, y, w, h, palSlot, 0);
	SetBgCopyFlag(s, bg);
}

static void HC_FillContestantWindowBgs(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		HC_BgFillBoxWithTile(s, 0, 0, 0x16, 2 + i * 5, 8, 2, 0x11);
}

static void HC_SwapMoveDescAndContestTilemaps(struct HackContestState* s)
{
	CpuCopy16(s->bgTilemap[0], (u8*) s->bgTilemap[0] + 0x500, 32 * 20);
	CpuCopy16(s->bgTilemap[2], (u8*) s->bgTilemap[2] + 0x500, 32 * 20);
}

static void HC_LoadContestPalettes(void)
{
	s32 i;

	LoadPalette(gHackContestTextPal, 16 * 15, 32);
	FillPalette(RGB_BLACK, 0, 2);
	for (i = 10; i < 14; i++)
		LoadPalette(&gPlttBufferUnfaded[16 * 15 + 1], 16 * 15 + i, 2);
	FillPalette(RGB(31, 17, 31), 16 * 15 + 3, 2);
}

static void HC_SetupGpuRegs(void)
{
	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP);
	SetGpuReg(REG_OFFSET_BLDCNT, 0);
	SetGpuReg(REG_OFFSET_BLDALPHA, 0);
	SetGpuReg(REG_OFFSET_BLDY, 0);
	SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
		| WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR);
	SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR
		| WINOUT_WINOBJ_BG0 | WINOUT_WINOBJ_BG1 | WINOUT_WINOBJ_BG2 | WINOUT_WINOBJ_BG3 | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
	SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG0_ON | DISPCNT_BG1_ON | DISPCNT_BG2_ON | DISPCNT_BG3_ON | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);

	gBattle_BG0_X = 0;
	gBattle_BG0_Y = 0;
	gBattle_BG1_X = 0;
	gBattle_BG1_Y = 0;
	gBattle_BG2_X = 0;
	gBattle_BG2_Y = 0;
	gBattle_BG3_X = 0;
	gBattle_BG3_Y = 0;
	gBattle_WIN0H = 0;
	gBattle_WIN0V = 0;
	gBattle_WIN1H = 0;
	gBattle_WIN1V = 0;
}

static void HC_InitBgs(struct HackContestState* s)
{
	s32 i;

	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sContestBgTemplates, 4);
	SetBgAttribute(3, BG_ATTR_WRAPAROUND, 1);
	for (i = 0; i < 4; i++)
		SetBgTilemapBuffer(i, s->bgTilemap[i]);
}

static void HC_InitWindows(struct HackContestState* s)
{
	InitWindows(sContestWindowTemplates);
	DeactivateAllTextPrinters();
	gTextFlags.canABSpeedUpPrint = (s->linkFlags & 1) ? FALSE : TRUE;
}

//原文 SetBgForCurtainDrop
static void HC_SetBgForCurtainDrop(struct HackContestState* s)
{
	u16 bg0Cnt, bg1Cnt, bg2Cnt;

	bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
	bg1Cnt &= ~0x3;                 //priority = 0
	bg1Cnt = (bg1Cnt & ~(3 << 14)) | (2 << 14); //screenSize = 2
	bg1Cnt &= ~(1 << 13);           //areaOverflowMode = 0
	bg1Cnt &= ~(3 << 2);            //charBaseBlock = 0
	SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);

	bg0Cnt = GetGpuReg(REG_OFFSET_BG0CNT);
	bg2Cnt = GetGpuReg(REG_OFFSET_BG2CNT);
	bg0Cnt = (bg0Cnt & ~0x3) | 1;
	bg2Cnt = (bg2Cnt & ~0x3) | 1;
	SetGpuReg(REG_OFFSET_BG0CNT, bg0Cnt);
	SetGpuReg(REG_OFFSET_BG2CNT, bg2Cnt);

	gBattle_BG1_X = DISPLAY_WIDTH;
	gBattle_BG1_Y = DISPLAY_HEIGHT;
	SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
	SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);

	CpuFill32(0, s->bgTilemap[1], 0x1000);
	CopyToBgTilemapBuffer(1, gHackContestCurtainTilemap, 0, 0);
	SetBgCopyFlag(s, 1);
	HC_SetNextTurnSpritePriority(s, 1); //（スライダーのハートの priority は次の担当）
}

static u8 HC_CreateJudgeSprite(void)
{
	u8 spriteId;

	LoadCompressedSpriteSheet(&sSpriteSheet_Judge);
	LoadCompressedPalette((const u32*) gHackContestJudgePal, 16 * 17, 32);
	spriteId = CreateSprite(&sSpriteTemplate_Judge, 112, 36, 30);
	gSprites[spriteId].oam.paletteNum = 1;
	gSprites[spriteId].callback = SpriteCallbackDummy;
	return spriteId;
}

static u8 HC_CreateJudgeSpeechBubbleSprite(void)
{
	u8 spriteId;

	LoadCompressedSpriteSheet(&sSpriteSheet_JudgeSymbols);
	LoadCompressedSpritePalette(&sSpritePalette_JudgeSymbols);
	spriteId = CreateSprite(&sSpriteTemplate_JudgeSpeechBubble, 96, 10, 29);
	gSprites[spriteId].invisible = TRUE;
	gSprites[spriteId].data[0] = gSprites[spriteId].oam.tileNum;
	return spriteId;
}

//原文 CreateContestantSprite（出場ポケモンの絵）。GetBattlerSpriteFinal_Y は FR に無いので、コンテスト（単戦・位置 3）の分だけ自作:
//  y = gMonBackPicCoords[species].y_offset + sBattlerCoords[単戦][3].y(=80)（原文 battle_anim_mons.c:269-283・Unown/Castform の特例は S2b）
//原文 SanitizeSpecies
static u16 HC_SanitizeSpecies(u16 species)
{
	return species >= NUM_SPECIES ? 0 : species;
}

//原文 GetBattlerYDelta（コンテスト＝後ろ向きの表・Unown は文字ごとの行・Castform は形 0 の 0）
#define HC_UNOWN_LETTER(p) ((((p) & 0x03000000) >> 18 | ((p) & 0x00030000) >> 12 | ((p) & 0x00000300) >> 6 | ((p) & 0x00000003)) % 28)
static u8 HC_GetBattlerYDelta(u16 species, u32 personality)
{
	if (species == SPECIES_UNOWN)
	{
		u32 letter = HC_UNOWN_LETTER(personality);

		return gMonBackPicCoords[letter == 0 ? species : letter + SPECIES_UNOWN_B - 1].y_offset;
	}
	if (species == SPECIES_CASTFORM)
		return 0;
	if (species > NUM_SPECIES)
		return gMonBackPicCoords[0].y_offset;
	return gMonBackPicCoords[species].y_offset;
}

static u8 __attribute__((unused)) HC_CreateContestantSprite(u16 species, u32 otId, u32 personality)
{
	u8 spriteId;
	u8 y;

	species = HC_SanitizeSpecies(species);
	HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonBackPicTable[species], gMonSpritesGfxPtr->sprites[B_POSITION_PLAYER_LEFT], species, personality);
	LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, otId, personality), 16 * 18, 32);
	ContestFr_SetMultiuseSpriteTemplateToPokemon(species, B_POSITION_PLAYER_LEFT);
	y = HC_GetBattlerYDelta(species, personality) + 80;
	spriteId = CreateSprite((const struct SpriteTemplate*) 0x0202443C /* gMultiuseSpriteTemplate */, 0x70, y, 30);
	gSprites[spriteId].oam.paletteNum = 2;
	gSprites[spriteId].oam.priority = 2;
	gSprites[spriteId].subpriority = GetBattlerSpriteSubpriority(2);
	gSprites[spriteId].callback = SpriteCallbackDummy;
	gSprites[spriteId].data[0] = gSprites[spriteId].oam.paletteNum;
	gSprites[spriteId].data[2] = species;
	gSprites[spriteId].affineAnims = (species == SPECIES_UNOWN) ? sAffineAnimsContestUnown : sAffineAnimsContest;
	StartSpriteAffineAnim(&gSprites[spriteId], 0);
	return spriteId;
}

//---- S2b ここから --------------------------------------------------------------------------------------------

#define HC_WIN_MOVE0 9      /* 9..12 技名 */
#define HC_WIN_SLASH 13
#define HC_WIN_MOVE_DESC 14
#define HC_TAG_NEXT_TURN_PAL 0x4E22
#define HC_CONTESTANT_NONE 0xFF
#define HC_STRING_NONE 0xFF
#define HC_PLAYER s->playerIndex
#define HC_TILE_EMPTY_APPEAL_HEART 0x5035
#define HC_TILE_FILLED_APPEAL_HEART 0x5012
#define HC_TILE_EMPTY_JAM_HEART 0x5036
#define HC_TILE_FILLED_JAM_HEART 0x5014
#define HC_MAX_HEARTS 8

static const u8 sNextTurnSpriteYPositions[HACK_CONTEST_CONTESTANTS] = { 36, 76, 116, 156 };

//次の番の札（日本語版: 0x08560A4C 以降。シートは 0xA0・数字は tile+2 へ 32 バイト・subsprite は x=-20（32x8）/+12（8x8）・原文と違う）
static const struct CompressedSpriteSheet sSpriteSheet_NextTurn[HACK_CONTEST_CONTESTANTS] =
{
	{ gHackContestNextTurnGfx, 0xA0, 0x4E22 }, { gHackContestNextTurnGfx, 0xA0, 0x4E23 },
	{ gHackContestNextTurnGfx, 0xA0, 0x4E24 }, { gHackContestNextTurnGfx, 0xA0, 0x4E25 },
};
static const struct SpritePalette sSpritePalette_NextTurn = { (const u16*) gHackContestPal, HC_TAG_NEXT_TURN_PAL };
static const struct OamData sOam_NextTurn =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = 0, .bpp = 0,
	.shape = 1, .x = 0, .matrixNum = 0, .size = 1, .tileNum = 0, .priority = 0, .paletteNum = 0,
};
#define HC_NEXT_TURN_TEMPLATE(tag) \
	{ .tileTag = tag, .paletteTag = HC_TAG_NEXT_TURN_PAL, .oam = &sOam_NextTurn, \
	  .anims = (const union AnimCmd* const*) 0x081F11E4, .images = NULL, \
	  .affineAnims = (const union AffineAnimCmd* const*) 0x081F11F0, .callback = SpriteCallbackDummy }
static const struct SpriteTemplate sSpriteTemplates_NextTurn[HACK_CONTEST_CONTESTANTS] =
{
	HC_NEXT_TURN_TEMPLATE(0x4E22), HC_NEXT_TURN_TEMPLATE(0x4E23), HC_NEXT_TURN_TEMPLATE(0x4E24), HC_NEXT_TURN_TEMPLATE(0x4E25),
};
static const struct Subsprite sSubsprites_NextTurn[] =
{
	{ .x = -20, .y = -4, .shape = 1, .size = 1, .tileOffset = 0, .priority = 0 },
	{ .x = 12, .y = -4, .shape = 0, .size = 0, .tileOffset = 4, .priority = 0 }, /* 日本語版 0x08560ADC: 2 つ目は 8x8 */
};
static const struct SubspriteTable sSubspriteTable_NextTurn[] = { { 2, sSubsprites_NextTurn } };

static void HC_CreateNextTurnSprites(struct HackContestState* s)
{
	s32 i;

	LoadSpritePalette(&sSpritePalette_NextTurn);
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		LoadCompressedSpriteSheet(&sSpriteSheet_NextTurn[i]);
		s->nextTurnSpriteId[i] = CreateSprite(&sSpriteTemplates_NextTurn[i], 204, sNextTurnSpriteYPositions[s->turnOrder[i]], 0);
		SetSubspriteTables(&gSprites[s->nextTurnSpriteId[i]], sSubspriteTable_NextTurn);
		gSprites[s->nextTurnSpriteId[i]].invisible = TRUE;
	}
}

static const u8* HC_GetTurnOrderNumberGfx(struct HackContestState* s, u8 contestant)
{
	if (s->status[contestant].turnOrderMod != 1)
		return gHackContestNextTurnRandomGfx;
	return gHackContestNextTurnNumbersGfx + s->status[contestant].nextTurnOrder * 32;
}

//原文 ShowHideNextTurnGfx
static void __attribute__((unused)) HC_ShowHideNextTurnGfx(struct HackContestState* s, bool8 show)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct Sprite* sprite = &gSprites[s->nextTurnSpriteId[i]];

		if (s->status[i].turnOrderMod != 0 && show)
		{
			CpuCopy32(HC_GetTurnOrderNumberGfx(s, i), (void*) (OBJ_VRAM0 + (sprite->oam.tileNum + 2) * 32), 32);
			sprite->pos1.y = sNextTurnSpriteYPositions[s->turnOrder[i]];
			sprite->invisible = FALSE;
		}
		else
			sprite->invisible = TRUE;
	}
}

static void HC_SetNextTurnSpritePriority(struct HackContestState* s, u8 priority)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		gSprites[s->nextTurnSpriteId[i]].oam.priority = priority;
		gSprites[s->sliderHeartSpriteId[i]].oam.priority = priority; //原文 SetBgForCurtainDrop・UpdateContestantBoxOrder は両方同時
	}
}

//原文 UpdateContestantBoxOrder（日本語版も同じ並び）。スライダーハートの priority は次の担当
static void HC_UpdateContestantBoxOrder(struct HackContestState* s)
{
	u16 bg1Cnt;

	ContestFr_RequestDma3Fill(0, (void*) BG_CHAR_ADDR(2), 0x2000, 1);
	CpuFill32(0, s->bgTilemap[1], 0x1000);
	SetBgCopyFlag(s, 1);
	bg1Cnt = GetGpuReg(REG_OFFSET_BG1CNT);
	bg1Cnt = (bg1Cnt & ~0x3) | 1;                 //priority = 1
	bg1Cnt &= ~(3 << 14);                         //screenSize = 0
	bg1Cnt &= ~(1 << 13);                         //areaOverflowMode = 0
	bg1Cnt = (bg1Cnt & ~(3 << 2)) | (2 << 2);     //charBaseBlock = 2
	SetGpuReg(REG_OFFSET_BG1CNT, bg1Cnt);
	gBattle_BG1_X = 0;
	gBattle_BG1_Y = 0;
	HC_SetNextTurnSpritePriority(s, 0);
}

//原文 Contest_IsMonsTurnDisabled
static bool8 HC_IsMonsTurnDisabled(struct HackContestState* s, u8 contestant)
{
	return s->status[contestant].numTurnsSkipped != 0 || s->status[contestant].noMoreTurns;
}

//原文 CalculateRound1Points（コンディション 6 値から。値の読み口は仮＝呼び元が s->mons[].stats を埋める）
static void HC_CalculateRound1Points(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		const u8* st = s->mons[i].stats; //0 cool 1 beauty 2 cute 3 smart 4 tough
		u8 mainStat, sub1, sub2;

		switch (s->category)
		{
		case 0: mainStat = st[0]; sub1 = st[4]; sub2 = st[1]; break;
		case 1: mainStat = st[1]; sub1 = st[0]; sub2 = st[2]; break;
		case 2: mainStat = st[2]; sub1 = st[1]; sub2 = st[3]; break;
		case 3: mainStat = st[3]; sub1 = st[2]; sub2 = st[4]; break;
		default: mainStat = st[4]; sub1 = st[3]; sub2 = st[0]; break;
		}
		s->round1Points[i] = mainStat + (sub1 + sub2 + s->mons[i].sheen) / 2;
	}
}

//原文 SortContestants(FALSE)（一次審査の点の高い順に並べ、同点は乱数。最後に順番表へ反転）
static void HC_SortContestants(struct HackContestState* s)
{
	u8 scratch[HACK_CONTEST_CONTESTANTS];
	u16 randomOrdering[HACK_CONTEST_CONTESTANTS] = { 0 };
	s32 i, v3;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s32 j;

		randomOrdering[i] = Random();
		for (j = 0; j < i; j++)
		{
			if (randomOrdering[i] == randomOrdering[j])
			{
				i--;
				break;
			}
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->turnOrder[i] = i;
		for (v3 = 0; v3 < i; v3++)
		{
			if (s->round1Points[s->turnOrder[v3]] < s->round1Points[i]
			 || (s->round1Points[s->turnOrder[v3]] == s->round1Points[i] && randomOrdering[s->turnOrder[v3]] < randomOrdering[i]))
			{
				s32 j;

				for (j = i; j > v3; j--)
					s->turnOrder[j] = s->turnOrder[j - 1];
				s->turnOrder[v3] = i;
				break;
			}
		}
		if (v3 == i)
			s->turnOrder[i] = i;
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		scratch[i] = s->turnOrder[i];
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		s->turnOrder[scratch[i]] = i;
}

//原文 ApplyNextTurnOrder
static void HC_ApplyNextTurnOrder(struct HackContestState* s)
{
	u8 nextContestant = 0;
	s32 i, j;
	u8 newTurnOrder[HACK_CONTEST_CONTESTANTS];
	bool8 isContestantOrdered[HACK_CONTEST_CONTESTANTS];

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		newTurnOrder[i] = s->turnOrder[i];
		isContestantOrdered[i] = FALSE;
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		for (j = 0; j < HACK_CONTEST_CONTESTANTS; j++)
		{
			if (s->status[j].nextTurnOrder == i)
			{
				newTurnOrder[j] = i;
				isContestantOrdered[j] = TRUE;
				break;
			}
		}
		if (j == HACK_CONTEST_CONTESTANTS)
		{
			for (j = 0; j < HACK_CONTEST_CONTESTANTS; j++)
			{
				if (!isContestantOrdered[j] && s->status[j].nextTurnOrder == HC_CONTESTANT_NONE)
				{
					nextContestant = j;
					j++;
					break;
				}
			}
			for (; j < HACK_CONTEST_CONTESTANTS; j++)
			{
				if (!isContestantOrdered[j] && s->status[j].nextTurnOrder == HC_CONTESTANT_NONE
				 && s->turnOrder[nextContestant] > s->turnOrder[j])
					nextContestant = j;
			}
			newTurnOrder[nextContestant] = i;
			isContestantOrdered[nextContestant] = TRUE;
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->status[i].nextTurnOrder = HC_CONTESTANT_NONE;
		s->status[i].turnOrderMod = 0;
		s->turnOrder[i] = newTurnOrder[i];
	}
}

//原文 InitContestResources の順番の所: 一次審査の点→SortContestants→ApplyNextTurnOrder（原文のとおりの順。debug の実測は点の高い者が 1 番目）
static void HC_InitTurnOrder(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->status[i].nextTurnOrder = HC_CONTESTANT_NONE;
		s->status[i].effectStringId = HC_STRING_NONE;
		s->status[i].effectStringId2 = HC_STRING_NONE;
	}
	HC_CalculateRound1Points(s);
	HC_SortContestants(s);
	HC_ApplyNextTurnOrder(s);
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		s->prevTurnOrder[i] = s->turnOrder[i];
}

//窓 8（一般テキスト）: 原文 Contest_StartTextPrinter（日本語版: font1・x0 y2・fg1 bg0 shadow8・b なら文字送り）
static void HC_StartTextPrinter(struct HackContestState* s, const u8* str, bool32 b)
{
	struct TextPrinterTemplate t;
	u8 speed = 0;

	t.currentChar = str;
	t.windowId = HC_WIN_GENERAL_TEXT;
	t.fontId = 1;
	t.x = 0;
	t.y = 2;
	t.currentX = 0;
	t.currentY = 2;
	t.letterSpacing = 0;
	t.lineSpacing = 0;
	t.unk = 0;
	t.fgColor = 1;
	t.bgColor = 0;
	t.shadowColor = 8;
	if (b)
		speed = (s->linkFlags & 1) ? 4 : GetPlayerTextSpeedDelay();
	ContestFr_AddTextPrinter(&t, speed, NULL);
	PutWindowTilemap(HC_WIN_GENERAL_TEXT);
	SetBgCopyFlag(s, 0);
}

static bool32 HC_RunTextPrinters(void)
{
	RunTextPrinters();
	return IsTextPrinterActive(HC_WIN_GENERAL_TEXT);
}

//原文 ContestClearGeneralTextWindow
static void HC_ClearGeneralTextWindow(struct HackContestState* s)
{
	FillWindowPixelBuffer(HC_WIN_GENERAL_TEXT, PIXEL_FILL(0));
	CopyWindowToVram(HC_WIN_GENERAL_TEXT, COPYWIN_GFX);
	SetBgCopyFlag(s, 0);
}

static void HC_BgFillBoxIncr(struct HackContestState* s, u8 bg, u16 firstTile, u8 x, u8 y, u8 w, u8 h, u8 palSlot, s16 step)
{
	ContestFr_WriteSequenceToBgTilemapBuffer(bg, firstTile, x, y, w, h, palSlot, step);
	SetBgCopyFlag(s, bg);
}

//原文 PrintContestMoveDescription（日本語版 contest.s sub_080DAAD4 の並び: 部門の絵 5x2・アピール/ジャマーのハート 8 個・説明は窓 14・斜線は窓 13）
static void HC_PrintContestMoveDescription(struct HackContestState* s, u16 move)
{
	struct HackContestMoveInfo info;
	u16 categoryTile;
	u8 numHearts;

	HackContest_GetMove(move, &info);
	switch (info.category)
	{
	case 0: categoryTile = 0x4040; break;
	case 1: categoryTile = 0x4045; break;
	case 2: categoryTile = 0x404A; break;
	case 3: categoryTile = 0x406A; break;
	default: categoryTile = 0x408A; break;
	}
	HC_BgFillBoxIncr(s, 0, categoryTile, 0x0B, 0x1F, 5, 1, 0x11, 1);
	HC_BgFillBoxIncr(s, 0, categoryTile + 0x10, 0x0B, 0x20, 5, 1, 0x11, 1);

	numHearts = info.appeal == 0xFF ? 0 : info.appeal / 10;
	if (numHearts > HC_MAX_HEARTS)
		numHearts = HC_MAX_HEARTS;
	HC_BgFillBoxIncr(s, 0, HC_TILE_EMPTY_APPEAL_HEART, 0x15, 0x1F, HC_MAX_HEARTS, 1, 0x11, 0);
	HC_BgFillBoxIncr(s, 0, HC_TILE_FILLED_APPEAL_HEART, 0x15, 0x1F, numHearts, 1, 0x11, 0);

	numHearts = info.jam == 0xFF ? 0 : info.jam / 10;
	if (numHearts > HC_MAX_HEARTS)
		numHearts = HC_MAX_HEARTS;
	HC_BgFillBoxIncr(s, 0, HC_TILE_EMPTY_JAM_HEART, 0x15, 0x20, HC_MAX_HEARTS, 1, 0x11, 0);
	HC_BgFillBoxIncr(s, 0, HC_TILE_FILLED_JAM_HEART, 0x15, 0x20, numHearts, 1, 0x11, 0);

	FillWindowPixelBuffer(HC_WIN_MOVE_DESC, PIXEL_FILL(0));
	HC_PrintTextToBg0WindowStd(s, HC_WIN_MOVE_DESC, gHackContestEffectDescriptions[info.effect]);
	HC_PrintTextToBg0WindowStd(s, HC_WIN_SLASH, gHackContest_Slash);
}

//矢印（日本語版: bg2・tile 1・x 1・幅 1 高 2・増分 1／消すのは tile 0xC。原文と違う）
static void HC_DrawMoveSelectArrow(struct HackContestState* s, s8 moveIndex)
{
	HC_BgFillBoxIncr(s, 2, 1, 1, 31 + moveIndex * 2, 1, 2, 0x11, 1);
}

static void HC_EraseMoveSelectArrow(struct HackContestState* s, s8 moveIndex)
{
	HC_BgFillBoxIncr(s, 2, 0xC, 1, 31 + moveIndex * 2, 1, 2, 0x11, 0);
}

static void HC_PrintAppealNumberText(struct HackContestState* s, bool32 slow)
{
	ConvertIntToDecimalStringN(gStringVar1, s->appealNumber + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
	StringCopy(gDisplayedStringBattle, HC_IsMonsTurnDisabled(s, HC_PLAYER) ? gHackContest_AppealNumButItCantParticipate : gHackContest_AppealNumWhichMoveWillBePlayed);
	HC_ClearGeneralTextWindow(s);
	StringExpandPlaceholders(gStringVar4, gDisplayedStringBattle);
	HC_StartTextPrinter(s, gStringVar4, slow);
}

static void Task_HC_Idle(u8 taskId) __attribute__((unused));
static void Task_HC_DisplayAppealNumber(u8 taskId);
static void Task_HC_TryShowMoveSelect(u8 taskId);
static void Task_HC_HandleMoveSelectInput(u8 taskId);
static void Task_HC_SelectedMove(u8 taskId);
static void Task_HC_HideApplauseMeterForAppealStart(u8 taskId);
static void HC_SetBottomSliderHeartsInvisibility(struct HackContestState* s, bool8 invisible);

//原文 Task_ShowMoveSelectScreen（日本語版 sub_080D7CBC）
static void Task_HC_ShowMoveSelect(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	struct HackContestStatus* st = &s->status[HC_PLAYER];
	s32 i;
	u8 buf[32];

	gBattle_BG0_Y = DISPLAY_HEIGHT;
	gBattle_BG2_Y = DISPLAY_HEIGHT;
	for (i = 0; i < 4; i++)
	{
		u16 move = s->mons[HC_PLAYER].moves[i];
		struct HackContestMoveInfo info;
		u8* p = buf;

		HackContest_GetMove(move, &info);
		if (st->prevMove != 0 && !(st->repeatedMove || st->nervous)
		 && HackContest_AreMovesCombo(st->prevMove, move, s->mons[HC_PLAYER].moves) && st->hasJudgesAttention)
			p = StringCopy(buf, gHackContest_ColorLightShadowDarkGray);
		else if (move != 0 && st->prevMove == move && info.effect != HACK_CONTEST_EFFECT_REPETITION_NOT_BORING)
			p = StringCopy(buf, gHackContest_ColorBlue);
		StringCopy(p, gMoveNames[move]);
		FillWindowPixelBuffer(i + HC_WIN_MOVE0, PIXEL_FILL(0));
		HC_PrintTextToBg0WindowStd(s, i + HC_WIN_MOVE0, buf);
	}
	HC_DrawMoveSelectArrow(s, s->playerMoveChoice);
	HC_PrintContestMoveDescription(s, s->mons[HC_PLAYER].moves[s->playerMoveChoice]);
	gTasks[taskId].func = Task_HC_HandleMoveSelectInput;
}

static void Task_HC_HandleMoveSelectInput(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u8 numMoves = 0;
	s32 i;

	for (i = 0; i < 4; i++)
	{
		if (s->mons[HC_PLAYER].moves[i] != 0)
			numMoves++;
	}
	if (gMain.newKeys & A_BUTTON)
	{
		PlaySE(5);
		gTasks[taskId].func = Task_HC_SelectedMove;
		return;
	}
	switch (gMain.newAndRepeatedKeys)
	{
	case B_BUTTON:
		PlaySE(5);
		HC_SetBottomSliderHeartsInvisibility(s, FALSE);
		HC_PrintAppealNumberText(s, FALSE);
		gBattle_BG0_Y = 0;
		gBattle_BG2_Y = 0;
		gTasks[taskId].func = Task_HC_TryShowMoveSelect;
		break;
	case DPAD_UP:
		HC_EraseMoveSelectArrow(s, s->playerMoveChoice);
		s->playerMoveChoice = s->playerMoveChoice == 0 ? numMoves - 1 : s->playerMoveChoice - 1;
		HC_DrawMoveSelectArrow(s, s->playerMoveChoice);
		HC_PrintContestMoveDescription(s, s->mons[HC_PLAYER].moves[s->playerMoveChoice]);
		if (numMoves > 1)
			PlaySE(5);
		break;
	case DPAD_DOWN:
		HC_EraseMoveSelectArrow(s, s->playerMoveChoice);
		s->playerMoveChoice = s->playerMoveChoice == numMoves - 1 ? 0 : s->playerMoveChoice + 1;
		HC_DrawMoveSelectArrow(s, s->playerMoveChoice);
		HC_PrintContestMoveDescription(s, s->mons[HC_PLAYER].moves[s->playerMoveChoice]);
		if (numMoves > 1)
			PlaySE(5);
		break;
	}
}

//原文 Task_HideMoveSelectScreen（日本語版 sub_080D8158）。拍手メーターの退場・スライダーハートは次の担当
static void Task_HC_HideMoveSelect(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	s32 i;

	HC_ClearGeneralTextWindow(s);
	gBattle_BG0_Y = 0;
	gBattle_BG2_Y = 0;
	HC_SetBottomSliderHeartsInvisibility(s, FALSE);
	for (i = 0; i < 4; i++)
	{
		FillWindowPixelBuffer(HC_WIN_MOVE0 + i, PIXEL_FILL(0));
		PutWindowTilemap(HC_WIN_MOVE0 + i);
		CopyWindowToVram(HC_WIN_MOVE0 + i, COPYWIN_GFX);
	}
	SetBgCopyFlag(s, 0);
	CpuCopy32(gPlttBufferFaded, s->cachedPlttFaded, 0x400);
	LoadPalette(s->cachedPlttUnfaded, 0, 0x400);
	gTasks[taskId].data[0] = 0;
	gTasks[taskId].data[1] = 0;
	gTasks[taskId].func = Task_HC_HideApplauseMeterForAppealStart; //S2c: →Task_AppealSetup→Task_DoAppeals
}

static void Task_HC_SelectedMove(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	s->status[HC_PLAYER].currMove = HC_IsMonsTurnDisabled(s, HC_PLAYER) ? 0 : s->mons[HC_PLAYER].moves[s->playerMoveChoice];
	//原文 GetAllChosenMoves／GetChosenMove（contest.c:3388-3411）: 出られない者は技なし・NPC は AI（S3・contest_ai.c）
	{
		s32 i;

		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			if (i == HC_PLAYER)
				continue;
			if (HC_IsMonsTurnDisabled(s, i))
			{
				s->status[i].currMove = 0;
				continue;
			}
			HackContest_AIReset(s, i);
			s->status[i].currMove = s->mons[i].moves[HackContest_AIGetActionToUse(s)];
		}
	}
	gTasks[taskId].func = Task_HC_HideMoveSelect;
}

//原文 Task_DisplayAppealNumberText（日本語版 sub_080D7B68）
static void Task_HC_DisplayAppealNumber(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (gTasks[taskId].data[0] == 0)
	{
		gBattle_BG0_Y = 0;
		gBattle_BG2_Y = 0;
		CpuCopy32(gPlttBufferUnfaded, s->cachedPlttUnfaded, 0x400);
		HC_PrintAppealNumberText(s, TRUE);
		gTasks[taskId].data[0]++;
	}
	else if (!HC_RunTextPrinters())
	{
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].func = Task_HC_TryShowMoveSelect;
	}
}

//原文 Task_TryShowMoveSelectScreen（日本語版 sub_080D7C50）。debug は SELECT で出口へ
static void Task_HC_TryShowMoveSelect(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

#ifdef HACK_DEBUG_MENU
	if (gMain.newKeys & SELECT_BUTTON)
	{
		BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
		gTasks[taskId].data[0] = 1;
		gTasks[taskId].func = Task_HC_Idle;
		return;
	}
#endif
	if ((gMain.newKeys & A_BUTTON) || gMain.newKeys == B_BUTTON)
	{
		PlaySE(5);
		if (!HC_IsMonsTurnDisabled(s, HC_PLAYER))
		{
			HC_SetBottomSliderHeartsInvisibility(s, TRUE);
			gTasks[taskId].func = Task_HC_ShowMoveSelect;
		}
		else
			gTasks[taskId].func = Task_HC_SelectedMove;
	}
}
//---- S2b ここまで --------------------------------------------------------------------------------------------

//---- S2c ここから（二次審査のターン・ハート・拍手メーター・審査員の目・技のアニメ）------------------------------------------
//原文 参考/pokeemerald/src/contest.c の 1683-2760（Task_AppealSetup〜Task_ContestReturnToField）・3737-3945（ハート）・3962-4040（目）・
//  4728-5380（拍手・観客・幕・スライダー・技のアニメ）。座標・タイルの表は原文のまま（日本語版との突き合わせは verify_contest_screen の H 節）。
//効果 48 の中身（gContestEffectFuncs）は S3＝ここは HC_CalculateAppealMoveImpact の「仮」（技の効果表のアピール値だけ・ジャマーなし・コンボ・連続・盛り上がりは原文どおり）

//原文 LoadContestBgAfterMoveAnim（技のアニメが書き換えた BG・パレットを会場に戻す。FR の技のアニメは IsContest で呼ばないので自分で呼ぶ）
static void HC_LoadContestBgAfterMoveAnim(struct HackContestState* s)
{
	s32 i;

	LZDecompressVram((const u32*) gHackContestInterfaceGfx, (void*) VRAM);
	LZDecompressVram((const u32*) gHackContestAudienceGfx, (void*) BG_SCREEN_ADDR(4));
	CopyToBgTilemapBuffer(3, gHackContestAudienceTilemap, 0, 0);
	CopyBgTilemapBufferToVram(3);
	LoadCompressedPalette((const u32*) gHackContestInterfaceAudiencePal, 0, 0x200);
	HC_LoadContestPalettes();
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		LoadPalette(s->cachedWindowPalettes[5 + i], 16 * (5 + s->turnOrder[i]), 32);
}

//S2d: 素の LoadDefaultBg の置き換え（hack_contest.c）から呼ぶ入口（restorebg が技のアニメの途中で会場の BG を戻す）
void HackContest_LoadContestBgAfterMoveAnim(void)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s != NULL)
		HC_LoadContestBgAfterMoveAnim(s);
}

#define HC_TAG_SLIDER_HEART 0x4E20
#define HC_TAG_APPLAUSE_METER 0xABE2
#define HC_TAG_BLINK0 0x80E8
#define HC_NUM_APPEALS 5
#define HC_STRING_NONE 0xFF

//音（FR の番号。エメラルドの SE と FR の SE は同じ音（トラックが同一）だが番号が違う＝S9 で 6 本を FR の本物の番号に直した。曲の番号は tools/rom/verify_contest_music.py が EM と突き合わせる）
#define HC_SE_SELECT 5
#define HC_SE_PC_LOGIN 2
#define HC_SE_BOO 22
#define HC_SE_SUCCESS 25
#define HC_SE_FAILURE 26
#define HC_SE_WARP_IN 39
#define HC_SE_EXP_MAX 84
#define HC_SE_HEAL_BELL 188
#define HC_SE_ENCORE2 216
#define HC_SE_HEART 89                     /* 原文 SE_CONTEST_HEART（EM 96） */
#define HC_SE_ICON_CHANGE 92               /* 原文 SE_CONTEST_ICON_CHANGE（EM 99） */
#define HC_SE_ICON_CLEAR 93                /* 原文 SE_CONTEST_ICON_CLEAR（EM 100） */
#define HC_SE_MONS_TURN 94                 /* 原文 SE_CONTEST_MONS_TURN（EM 101） */
#define HC_SE_CONDITION_LOSE 32            /* 原文 SE_CONTEST_CONDITION_LOSE（EM 38） */
#define HC_SE_CURTAIN_FALL 91              /* 原文 SE_CONTEST_CURTAIN_FALL（EM 98） */
#define HC_SE_CURTAIN_RISE 90              /* 原文 SE_CONTEST_CURTAIN_RISE（EM 97） */
#define HC_MUS_TOO_BAD 271

enum { HC_JUDGE_SWIRL, HC_JUDGE_ONE_EXCL = 2, HC_JUDGE_TWO_EXCL, HC_JUDGE_NUMBER_ONE = 5, HC_JUDGE_NUMBER_FOUR, HC_JUDGE_QUESTION, HC_JUDGE_STAR };
enum { HC_SYM_CIRCLE, HC_SYM_WAVE, HC_SYM_X, HC_SYM_SWIRL, HC_SYM_SQUARE };

static const u8 sSliderHeartYPositions[HACK_CONTEST_CONTESTANTS] = { 36, 76, 116, 156 };

//スライダーのハート
static const struct SpriteSheet sSpriteSheet_SliderHeart = { gHackContestSliderHeartGfx, 0x20, HC_TAG_SLIDER_HEART };
static const struct OamData sOam_SliderHeart =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = 0, .bpp = 0,
	.shape = 0, .x = 0, .matrixNum = 0, .size = 0, .tileNum = 0, .priority = 0, .paletteNum = 0,
};
static const union AffineAnimCmd sAffineAnim_SliderHeart_Normal[] = { AFFINEANIMCMD_FRAME(256, 256, 0, 0), AFFINEANIMCMD_END };
static const union AffineAnimCmd sAffineAnim_SliderHeart_SpinDisappear[] = { AFFINEANIMCMD_FRAME(256, 256, 0, 0), AFFINEANIMCMD_FRAME(-10, -10, -20, 20), AFFINEANIMCMD_END };
static const union AffineAnimCmd sAffineAnim_SliderHeart_SpinAppear[] = { AFFINEANIMCMD_FRAME(56, 56, 0, 0), AFFINEANIMCMD_FRAME(10, 10, 20, 20), AFFINEANIMCMD_END };
static const union AffineAnimCmd* const sAffineAnims_SliderHeart[] = { sAffineAnim_SliderHeart_Normal, sAffineAnim_SliderHeart_SpinDisappear, sAffineAnim_SliderHeart_SpinAppear };
static const struct SpriteTemplate sSpriteTemplate_SliderHeart =
{
	.tileTag = HC_TAG_SLIDER_HEART, .paletteTag = HC_TAG_JUDGE_SYMBOLS, .oam = &sOam_SliderHeart,
	.anims = (const union AnimCmd* const*) 0x081F11E4, .images = NULL, .affineAnims = sAffineAnims_SliderHeart, .callback = SpriteCallbackDummy,
};

//拍手メーター（64x32）
static const struct CompressedSpriteSheet sSpriteSheet_ApplauseMeter = { gHackContestApplauseGfx, 0x400, HC_TAG_APPLAUSE_METER };
static const struct SpritePalette sSpritePalette_ApplauseMeter = { (const u16*) gHackContestPal, HC_TAG_APPLAUSE_METER };
static const struct OamData sOam_ApplauseMeter =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = 0, .bpp = 0,
	.shape = 1, .x = 0, .matrixNum = 0, .size = 3, .tileNum = 0, .priority = 0, .paletteNum = 0,
};
static const struct SpriteTemplate sSpriteTemplate_ApplauseMeter =
{
	.tileTag = HC_TAG_APPLAUSE_METER, .paletteTag = HC_TAG_APPLAUSE_METER, .oam = &sOam_ApplauseMeter,
	.anims = (const union AnimCmd* const*) 0x081F11E4, .images = NULL,
	.affineAnims = (const union AffineAnimCmd* const*) 0x081F11F0, .callback = SpriteCallbackDummy,
};

//出場者の枠の点滅（64x64・ダブルサイズのアフィン・ブレンド）
static const struct CompressedSpriteSheet sSpriteSheets_Blink[HACK_CONTEST_CONTESTANTS] =
{
	{ gHackContestBlankGfx, 0x1000, 0x80E8 }, { gHackContestBlankGfx, 0x1000, 0x80E9 },
	{ gHackContestBlankGfx, 0x1000, 0x80EA }, { gHackContestBlankGfx, 0x1000, 0x80EB },
};
static const struct OamData sOam_Blink =
{
	.y = 0, .affineMode = 3 /* ST_OAM_AFFINE_DOUBLE */, .objMode = 1 /* ST_OAM_OBJ_BLEND */, .mosaic = 0, .bpp = 0,
	.shape = 0, .x = 0, .matrixNum = 0, .size = 3, .tileNum = 0, .priority = 0, .paletteNum = 0,
};
static const union AffineAnimCmd sAffineAnim_Blink_0[] = { AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0), AFFINEANIMCMD_END };
static const union AffineAnimCmd sAffineAnim_Blink_1[] =
{
	AFFINEANIMCMD_FRAME(3, 3, 0, 15), AFFINEANIMCMD_FRAME(-3, -3, 0, 15), AFFINEANIMCMD_FRAME(3, 3, 0, 15), AFFINEANIMCMD_FRAME(-3, -3, 0, 15),
	AFFINEANIMCMD_END
};
static const union AffineAnimCmd* const sAffineAnims_Blink[] = { sAffineAnim_Blink_0, sAffineAnim_Blink_1 };
#define HC_BLINK_TEMPLATE(tag) \
	{ .tileTag = tag, .paletteTag = tag, .oam = &sOam_Blink, .anims = (const union AnimCmd* const*) 0x081F11E4, .images = NULL, \
	  .affineAnims = sAffineAnims_Blink, .callback = SpriteCallbackDummy }
static const struct SpriteTemplate sSpriteTemplates_Blink[HACK_CONTEST_CONTESTANTS] =
{
	HC_BLINK_TEMPLATE(0x80E8), HC_BLINK_TEMPLATE(0x80E9), HC_BLINK_TEMPLATE(0x80EA), HC_BLINK_TEMPLATE(0x80EB),
};

//盛り上がりの表（原文 sContestExcitementTable[部門][技の部門]）
static const s8 sContestExcitementTable[5][5] =
{
	{ +1, 0, -1, -1, 0 }, { 0, +1, 0, -1, -1 }, { -1, 0, +1, 0, -1 }, { -1, -1, 0, +1, 0 }, { 0, -1, -1, 0, +1 },
};

//文の表（原文 contest_text_tables.h の sAppealResultTexts／sRoundResultTexts／sContestConditions）
static const u8* const sAppealResultTexts[62] =
{
	gHackContest_BecameMoreConsciousOfOtherMons, gHackContest_MonCantMakeAnAppealAfterThis, gHackContest_SettledDownJustLittleBit, gHackContest_BecameObliviousToOtherMons,
	gHackContest_BecameLessAwareOfOtherMons, gHackContest_StoppedCaringAboutOtherMons, gHackContest_TriedToStartleOtherMons, gHackContest_TriedToDazzleOthers,
	gHackContest_JudgeLookedAwayFromMon, gHackContest_TriedToUnnerveNextMon, gHackContest_MonBecameNervous, gHackContest_AppealTriedToUnnerveWaitingMons,
	gHackContest_TauntedMonsDoingWell, gHackContest_MonRegainedItsForm, gHackContest_TriedToJamMonDoingWell, gHackContest_StandoutMonHustledEvenMore,
	gHackContest_LargelyUnnoticedMonWorkedHard, gHackContest_WorkedAsMuchAsMonBefore, gHackContest_MonsAppealDidNotGoWell, gHackContest_WorkedAsMuchAsPrecedingMon,
	gHackContest_MonsAppealDidNotGoWell2, gHackContest_MonsAppealDidNotGoWell3, gHackContest_MonsAppealWentSlightlyWell, gHackContest_MonsAppealWentPrettyWell,
	gHackContest_MonsAppealWentExcellently, gHackContest_MonsAppealWasDud, gHackContest_MonsAppealDidNotWorkVeryWell, gHackContest_MonsAppealWentSlightlyWell2,
	gHackContest_MonsAppealWentPrettyWell2, gHackContest_MonsAppealWentVeryWell, gHackContest_MonsAppealWentExcellently2, gHackContest_SameTypeAsOneBeforeGood,
	gHackContest_NotSameTypeAsOneBeforeGood, gHackContest_StoodOutMuchMoreThanMonBefore, gHackContest_DidntDoAsWellAsMonBefore, gHackContest_MonsConditionRoseAboveUsual,
	gHackContest_MonsHotStatusMadeGreatAppeal, gHackContest_MovedUpInLineForNextAppeal, gHackContest_MovedBackInLineForNextAppeal, gHackContest_ScrambledUpOrderForNextTurn,
	gHackContest_JudgeLookedAtMonExpectantly, gHackContest_AppealComboWentOverWell, gHackContest_AppealComboWentOverVeryWell, gHackContest_AppealComboWentOverExcellently,
	gHackContest_MonManagedToAvertGaze, gHackContest_MonManagedToAvoidSeeingIt, gHackContest_MonIsntFazedByThatSortOfThing, gHackContest_MonBecameALittleDistracted,
	gHackContest_TriedToStartleOtherPokemon, gHackContest_MonLookedDownOutOfDistraction, gHackContest_MonTurnedBackOutOfDistraction, gHackContest_MonCouldntHelpUtteringCry,
	gHackContest_MonCouldntHelpLeapingUp, gHackContest_MonTrippedOutOfDistraction, gHackContest_ButItMessedUp2, gHackContest_ButItFailedToMakeTargetNervous,
	gHackContest_ButItFailedToMakeAnyoneNervous, gHackContest_ButItWasIgnored, gHackContest_CouldntImproveItsCondition, gHackContest_BadConditionResultedInWeakAppeal,
	gHackContest_MonWasUnaffected, gHackContest_AttractedCrowdsAttention,
};
static const u8* const sRoundResultTexts[7] =
{
	gHackContest_MonFailedToStandOutAtAll, gHackContest_MonDidntStandOutVeryMuch, gHackContest_MonCaughtALittleAttention, gHackContest_MonAttractedALotOfAttention,
	gHackContest_MonCommandedTotalAttention, gHackContest_MonHasntMadeItsAppeal, gHackContest_JudgesViewsOnMonHeldFirm,
};
static const u8* const sContestConditions[5] =
{
	gHackContest_Contest_Coolness, gHackContest_Contest_Beauty, gHackContest_Contest_Cuteness, gHackContest_Contest_Smartness, gHackContest_Contest_Toughness,
};

static void Task_HC_HideApplauseMeterForAppealStart(u8 taskId);
static void Task_HC_DoAppeals(u8 taskId);
static void Task_HC_FinishRoundOfAppeals(u8 taskId);
static void HC_SlideApplauseMeterIn(struct HackContestState* s);
static void HC_SlideApplauseMeterOut(struct HackContestState* s);

//---- 小さい道具 ---------------------------------------------------------------------------------------
static void HC_SetBottomSliderHeartsInvisibility(struct HackContestState* s, bool8 invisible)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		if (s->turnOrder[i] > 1)
			gSprites[s->sliderHeartSpriteId[i]].pos1.x = invisible ? 256 : 180;
	}
}

static void HC_CreateSliderHeartSprites(struct HackContestState* s)
{
	s32 i;

	LoadSpriteSheet(&sSpriteSheet_SliderHeart);
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		s->sliderHeartSpriteId[i] = CreateSprite(&sSpriteTemplate_SliderHeart, 180, sSliderHeartYPositions[s->turnOrder[i]], 1);
}

static void HC_CreateApplauseMeterSprite(struct HackContestState* s)
{
	u8 spriteId;

	LoadCompressedSpriteSheet(&sSpriteSheet_ApplauseMeter);
	LoadSpritePalette(&sSpritePalette_ApplauseMeter);
	spriteId = CreateSprite(&sSpriteTemplate_ApplauseMeter, 30, 44, 1);
	gSprites[spriteId].invisible = TRUE;
	s->applauseMeterSpriteId = spriteId;
}

static u16 HC_GetAppealHeartTileOffset(u8 contestant)
{
	u16 offset;

	if (contestant == 0) offset = 0x5011;
	else if (contestant == 1) offset = 0x6011;
	else if (contestant == 2) offset = 0x7011;
	else offset = 0x8011;
	return offset + 1;
}

static s8 HC_GetNumHeartsFromAppealPoints(s16 appeal)
{
	s8 hearts = appeal / 10;

	if (hearts > 16) hearts = 16;
	else if (hearts < -16) hearts = -16;
	return hearts;
}

//ハートの増減（原文 UpdateAppealHearts／Task_UpdateAppealHearts）
static void Task_HC_UpdateAppealHearts(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u8 contestant = gTasks[taskId].data[3];
	s16 startHearts = gTasks[taskId].data[0];
	s16 heartsDelta = gTasks[taskId].data[1];

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	if (++gTasks[taskId].data[10] > 14)
	{
		u16 heartOffset;
		u8 newNumHearts;
		u8 pitchMod;
		bool8 onSecondLine;

		gTasks[taskId].data[10] = 0;
		if (gTasks[taskId].data[1] == 0)
		{
			DestroyTask(taskId);
			s->updatingAppealHearts[contestant] = FALSE;
			return;
		}
		else if (startHearts == 0)
		{
			if (heartsDelta < 0)
			{
				heartOffset = HC_GetAppealHeartTileOffset(contestant) + 2;
				gTasks[taskId].data[1]++;
			}
			else
			{
				heartOffset = HC_GetAppealHeartTileOffset(contestant);
				gTasks[taskId].data[1]--;
			}
			newNumHearts = gTasks[taskId].data[0]++;
		}
		else
		{
			if (gTasks[taskId].data[2] < 0)
			{
				if (heartsDelta < 0)
				{
					newNumHearts = gTasks[taskId].data[0]++;
					gTasks[taskId].data[1]++;
					heartOffset = HC_GetAppealHeartTileOffset(contestant) + 2;
				}
				else
				{
					newNumHearts = --gTasks[taskId].data[0];
					heartOffset = 0;
					gTasks[taskId].data[1]--;
				}
			}
			else
			{
				if (heartsDelta < 0)
				{
					newNumHearts = --gTasks[taskId].data[0];
					heartOffset = 0;
					gTasks[taskId].data[1]++;
				}
				else
				{
					newNumHearts = gTasks[taskId].data[0]++;
					gTasks[taskId].data[1]--;
					heartOffset = HC_GetAppealHeartTileOffset(contestant);
				}
			}
		}
		pitchMod = newNumHearts;
		onSecondLine = FALSE;
		if (newNumHearts > 7)
		{
			onSecondLine = TRUE;
			newNumHearts -= 8;
		}
		HC_BgFillBoxWithTile(s, 0, heartOffset, newNumHearts + 22, s->turnOrder[contestant] * 5 + 2 + onSecondLine, 1, 1, 17);
		if (heartsDelta > 0)
		{
			PlaySE(HC_SE_HEART);
			m4aMPlayImmInit(&gMPlayInfo_SE1);
			ContestFr_m4aMPlayPitchControl(&gMPlayInfo_SE1, 0xFFFF, pitchMod * 256); //原文 contest.c:3825（TRACKS_ALL・ハートが増えるごとに音程が上がる）
		}
		else
			PlaySE(HC_SE_BOO);
		if (!onSecondLine && newNumHearts == 0 && heartOffset == 0)
			gTasks[taskId].data[2] = -gTasks[taskId].data[2];
	}
}

static u8 HC_UpdateAppealHearts(struct HackContestState* s, s16 startAppeal, s16 appealDelta, u8 contestant)
{
	u8 taskId;
	s8 startHearts;
	s8 heartsDelta;

	s->updatingAppealHearts[contestant] = TRUE;
	taskId = CreateTask(Task_HC_UpdateAppealHearts, 20);
	startHearts = HC_GetNumHeartsFromAppealPoints(startAppeal);
	heartsDelta = HC_GetNumHeartsFromAppealPoints(startAppeal + appealDelta) - startHearts;
	gTasks[taskId].data[0] = startHearts < 0 ? -startHearts : startHearts;
	gTasks[taskId].data[1] = heartsDelta;
	if (startHearts > 0 || (startHearts == 0 && heartsDelta > 0))
		gTasks[taskId].data[2] = 1;
	else
		gTasks[taskId].data[2] = -1;
	gTasks[taskId].data[3] = contestant;
	return taskId;
}

//スライダー（原文 UpdateHeartSlider）
static void SpriteCB_HC_UpdateHeartSlider(struct Sprite* sprite)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
		return;
	if (sprite->pos2.x == sprite->data[1])
	{
		s->sliderUpdating[sprite->data[0]] = FALSE;
		sprite->callback = SpriteCallbackDummy;
	}
	else
		sprite->pos2.x += sprite->data[2];
}

static void HC_UpdateHeartSliders(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct Sprite* sprite = &gSprites[s->sliderHeartSpriteId[i]];
		s16 slideTarget = s->status[i].pointTotal / 10 * 2;

		s->sliderUpdating[i] = TRUE;
		if (slideTarget > 56) slideTarget = 56;
		else if (slideTarget < 0) slideTarget = 0;
		sprite->invisible = FALSE;
		sprite->data[0] = i;
		sprite->data[1] = slideTarget;
		sprite->data[2] = slideTarget > sprite->pos2.x ? 1 : -1;
		sprite->callback = SpriteCB_HC_UpdateHeartSlider;
	}
}

static bool8 HC_SlidersDoneUpdating(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		if (s->sliderUpdating[i])
			return FALSE;
	}
	return TRUE;
}

static void HC_UpdateSliderHeartSpriteYPositions(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		gSprites[s->sliderHeartSpriteId[i]].pos1.y = sSliderHeartYPositions[s->turnOrder[i]];
}

//スライダーのハートの出入り（原文 AnimateSliderHearts）
static void Task_HC_WaitForSliderHeartAnim(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	s32 i;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	if (gSprites[s->sliderHeartSpriteId[0]].affineAnimEnded)
	{
		if ((u8) gTasks[taskId].data[0] == 1 /* DISAPPEAR */)
		{
			for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
				gSprites[s->sliderHeartSpriteId[i]].invisible = TRUE;
		}
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
			FreeSpriteOamMatrix(&gSprites[s->sliderHeartSpriteId[i]]);
		s->sliderHeartsAnimating = FALSE;
		DestroyTask(taskId);
	}
}

static void HC_AnimateSliderHearts(struct HackContestState* s, u8 animId)
{
	s32 i;
	u8 taskId;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct Sprite* sprite = &gSprites[s->sliderHeartSpriteId[i]];

		sprite->oam.matrixNum = AllocOamMatrix();
		sprite->oam.affineMode = 1; //ST_OAM_AFFINE_NORMAL
		StartSpriteAffineAnim(sprite, animId);
		if (animId == 2 /* APPEAR */)
		{
			AnimateSprite(sprite);
			sprite->invisible = FALSE;
		}
	}
	taskId = CreateTask(Task_HC_WaitForSliderHeartAnim, 5);
	gTasks[taskId].data[0] = animId;
	s->sliderHeartsAnimating = TRUE;
}

//審査員の目（原文 CreateJudgeAttentionEyeTask 以下）
static void Task_HC_FlashJudgeAttentionEye(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u8 i;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		u8 offset = i * 4;

		if (gTasks[taskId].data[offset + 0] != 0xFF)
		{
			if (gTasks[taskId].data[offset + 1] == 0)
				gTasks[taskId].data[offset + 0]++;
			else
				gTasks[taskId].data[offset + 0]--;
			if (gTasks[taskId].data[offset + 0] == 16 || gTasks[taskId].data[offset + 0] == 0)
				gTasks[taskId].data[offset + 1] ^= 1;
			BlendPalette(16 * (5 + s->prevTurnOrder[i]) + 6, 2, gTasks[taskId].data[offset + 0], RGB(31, 31, 18));
		}
	}
}

static void HC_CreateJudgeAttentionEyeTask(struct HackContestState* s)
{
	u8 i;
	u8 taskId = CreateTask(Task_HC_FlashJudgeAttentionEye, 30);

	s->judgeAttentionTaskId = taskId;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		gTasks[taskId].data[i * 4] = 0xFF;
}

static void HC_StartFlashJudgeAttentionEye(struct HackContestState* s, u8 contestant)
{
	gTasks[s->judgeAttentionTaskId].data[contestant * 4 + 0] = 0;
	gTasks[s->judgeAttentionTaskId].data[contestant * 4 + 1] = 0;
}

static void Task_HC_StopFlashJudgeAttentionEye(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u8 contestant = gTasks[taskId].data[0];

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	if (gTasks[s->judgeAttentionTaskId].data[contestant * 4 + 0] == 0 || gTasks[s->judgeAttentionTaskId].data[contestant * 4 + 0] == 0xFF)
	{
		gTasks[s->judgeAttentionTaskId].data[contestant * 4 + 0] = 0xFF;
		gTasks[s->judgeAttentionTaskId].data[contestant * 4 + 1] = 0;
		BlendPalette(16 * (5 + s->prevTurnOrder[contestant]) + 6, 2, 0, RGB(31, 31, 18));
		DestroyTask(taskId);
	}
}

static void HC_StopFlashJudgeAttentionEye(struct HackContestState* s, u8 contestant)
{
	u8 taskId = CreateTask(Task_HC_StopFlashJudgeAttentionEye, 31);

	gTasks[taskId].data[0] = contestant;
}

static void HC_StartStopFlashJudgeAttentionEye(struct HackContestState* s, u8 contestant)
{
	if (s->status[contestant].hasJudgesAttention)
		HC_StartFlashJudgeAttentionEye(s, contestant);
	else
		HC_StopFlashJudgeAttentionEye(s, contestant);
}

//審査員の吹き出し（原文 DoJudgeSpeechBubble）
static void SpriteCB_HC_JudgeSpeechBubble(struct Sprite* sprite)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (sprite->data[1]++ > 84)
	{
		sprite->data[1] = 0;
		sprite->invisible = TRUE;
		sprite->callback = SpriteCallbackDummy;
		if (s != NULL)
			s->waitForJudgeSpeechBubble = FALSE;
	}
}

static void HC_DoJudgeSpeechBubble(struct HackContestState* s, u8 symbolId)
{
	struct Sprite* sprite = &gSprites[s->judgeSpeechBubbleSpriteId];

	switch (symbolId)
	{
	case HC_JUDGE_SWIRL:
		sprite->oam.tileNum = sprite->data[0];
		PlaySE(HC_SE_FAILURE);
		break;
	case HC_JUDGE_ONE_EXCL:
		sprite->oam.tileNum = sprite->data[0] + 4;
		PlaySE(HC_SE_SUCCESS);
		break;
	case HC_JUDGE_TWO_EXCL:
		sprite->oam.tileNum = sprite->data[0] + 8;
		PlaySE(HC_SE_SUCCESS);
		break;
	case HC_JUDGE_NUMBER_ONE:
		sprite->oam.tileNum = sprite->data[0] + 12;
		PlaySE(HC_SE_WARP_IN);
		break;
	case HC_JUDGE_NUMBER_FOUR:
		sprite->oam.tileNum = sprite->data[0] + 16;
		PlaySE(HC_SE_WARP_IN);
		break;
	case HC_JUDGE_STAR:
		sprite->oam.tileNum = sprite->data[0] + 24;
		PlaySE(HC_SE_HEAL_BELL);
		break;
	case HC_JUDGE_QUESTION:
	default:
		sprite->oam.tileNum = sprite->data[0] + 20;
		PlaySE(HC_SE_WARP_IN);
		break;
	}
	sprite->data[1] = 0;
	sprite->invisible = FALSE;
	sprite->callback = SpriteCB_HC_JudgeSpeechBubble;
	s->waitForJudgeSpeechBubble = TRUE;
}

//拍手メーター（原文 UpdateApplauseMeter 以下）
static void Task_HC_ApplauseOverflowAnimation(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	if (++gTasks[taskId].data[0] == 1)
	{
		gTasks[taskId].data[0] = 0;
		if (gTasks[taskId].data[3] == 0)
			gTasks[taskId].data[4]++;
		else
			gTasks[taskId].data[4]--;
		BlendPalette(16 * (16 + gTasks[taskId].data[2]) + 8, 1, gTasks[taskId].data[4], 0x7FFF);
		if (gTasks[taskId].data[4] == 0 || gTasks[taskId].data[4] == 16)
		{
			gTasks[taskId].data[3] ^= 1;
			if (s->applauseLevel < 5)
			{
				BlendPalette(16 * (16 + gTasks[taskId].data[2]) + 8, 1, 0, 0x001F);
				DestroyTask(taskId);
			}
		}
	}
}

static void HC_UpdateApplauseMeter(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < 5; i++)
	{
		const u8* src;
		u16 tileNum = gSprites[s->applauseMeterSpriteId].oam.tileNum;

		src = i < s->applauseLevel ? &gHackContestApplauseMeterGfx[64] : gHackContestApplauseMeterGfx;
		CpuCopy32(src, (void*) (OBJ_VRAM0 + (tileNum + 17 + i) * 32), 32);
		CpuCopy32(src + 32, (void*) (OBJ_VRAM0 + (tileNum + 25 + i) * 32), 32);
		if (s->applauseLevel > 4)
		{
			u8 taskId = CreateTask(Task_HC_ApplauseOverflowAnimation, 10);

			gTasks[taskId].data[1] = 1;
			gTasks[taskId].data[2] = IndexOfSpritePaletteTag(HC_TAG_APPLAUSE_METER);
		}
	}
}

static void Task_HC_SlideApplauseMeterIn(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	struct Sprite* sprite;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	sprite = &gSprites[s->applauseMeterSpriteId];
	gTasks[taskId].data[10] += 1664;
	sprite->pos2.x += gTasks[taskId].data[10] >> 8;
	gTasks[taskId].data[10] = gTasks[taskId].data[10] & 0xFF;
	if (sprite->pos2.x > 0)
		sprite->pos2.x = 0;
	if (sprite->pos2.x == 0)
	{
		s->applauseMeterIsMoving = FALSE;
		DestroyTask(taskId);
	}
}

static void HC_SlideApplauseMeterIn(struct HackContestState* s)
{
	CreateTask(Task_HC_SlideApplauseMeterIn, 10);
	gSprites[s->applauseMeterSpriteId].pos2.x = -70;
	gSprites[s->applauseMeterSpriteId].invisible = FALSE;
	s->applauseMeterIsMoving = TRUE;
}

static void Task_HC_SlideApplauseMeterOut(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	struct Sprite* sprite;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	sprite = &gSprites[s->applauseMeterSpriteId];
	gTasks[taskId].data[10] += 1664;
	sprite->pos2.x -= gTasks[taskId].data[10] >> 8;
	gTasks[taskId].data[10] = gTasks[taskId].data[10] & 0xFF;
	if (sprite->pos2.x < -70)
		sprite->pos2.x = -70;
	if (sprite->pos2.x == -70)
	{
		sprite->invisible = TRUE;
		s->applauseMeterIsMoving = FALSE;
		DestroyTask(taskId);
	}
}

static void HC_SlideApplauseMeterOut(struct HackContestState* s)
{
	if (gSprites[s->applauseMeterSpriteId].invisible == TRUE)
		s->applauseMeterIsMoving = FALSE;
	else
	{
		CreateTask(Task_HC_SlideApplauseMeterOut, 10);
		gSprites[s->applauseMeterSpriteId].pos2.x = 0;
		s->applauseMeterIsMoving = TRUE;
	}
}

static void Task_HC_ShowAndUpdateApplauseMeter(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	switch (gTasks[taskId].data[10])
	{
	case 0:
		HC_SlideApplauseMeterIn(s);
		gTasks[taskId].data[10]++;
		break;
	case 1:
		if (!s->applauseMeterIsMoving)
			gTasks[taskId].data[10]++;
		break;
	case 2:
		if (gTasks[taskId].data[11]++ > 20)
		{
			gTasks[taskId].data[11] = 0;
			HC_UpdateApplauseMeter(s);
			s->isShowingApplauseMeter = FALSE;
			DestroyTask(taskId);
		}
		break;
	}
}

static void HC_ShowAndUpdateApplauseMeter(struct HackContestState* s, s8 unused)
{
	u8 taskId = CreateTask(Task_HC_ShowAndUpdateApplauseMeter, 5);

	gTasks[taskId].data[0] = unused;
	s->isShowingApplauseMeter = TRUE;
}

//観客（原文 AnimateAudience・BlendAudienceBackground）
static void Task_HC_AnimateAudience(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	if (gTasks[taskId].data[10]++ > 6)
	{
		gTasks[taskId].data[10] = 0;
		if (gTasks[taskId].data[11] == 0)
			ContestFr_RequestDma3Copy(s->audienceGfx + 0x1000, (void*) BG_SCREEN_ADDR(4), 0x1000, 1);
		else
		{
			ContestFr_RequestDma3Copy(s->audienceGfx, (void*) BG_SCREEN_ADDR(4), 0x1000, 1);
			gTasks[taskId].data[12]++;
		}
		gTasks[taskId].data[11] ^= 1;
		if (gTasks[taskId].data[12] == 9)
		{
			s->animatingAudience = FALSE;
			DestroyTask(taskId);
		}
	}
}

static void HC_AnimateAudience(struct HackContestState* s)
{
	CreateTask(Task_HC_AnimateAudience, 15);
	s->animatingAudience = TRUE;
}

static void Task_HC_BlendAudienceBackground(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	if (gTasks[taskId].data[10]++ >= 0)
	{
		gTasks[taskId].data[10] = 0;
		if (gTasks[taskId].data[2] > 0)
			gTasks[taskId].data[1]++;
		else
			gTasks[taskId].data[1]--;
		BlendPalette(16 * 1 + 1, 1, gTasks[taskId].data[1], gTasks[taskId].data[0]);
		BlendPalette(16 * 1 + 10, 1, gTasks[taskId].data[1], gTasks[taskId].data[0]);
		if (gTasks[taskId].data[1] == gTasks[taskId].data[3])
		{
			DestroyTask(taskId);
			s->waitForAudienceBlend = FALSE;
		}
	}
}

static void HC_BlendAudienceBackground(struct HackContestState* s, s8 excitementDir, s8 blendDir)
{
	u8 taskId = CreateTask(Task_HC_BlendAudienceBackground, 10);
	u16 blendColor;
	u8 blendCoeff;
	u8 targetBlendCoeff;

	if (excitementDir > 0)
	{
		blendColor = RGB(30, 27, 8);
		if (blendDir > 0)
		{
			blendCoeff = 0;
			targetBlendCoeff = s->applauseLevel * 3;
		}
		else
		{
			blendCoeff = s->applauseLevel * 3;
			targetBlendCoeff = 0;
		}
	}
	else
	{
		blendColor = RGB_BLACK;
		if (blendDir > 0)
		{
			blendCoeff = 0;
			targetBlendCoeff = 12;
		}
		else
		{
			blendCoeff = 12;
			targetBlendCoeff = 0;
		}
	}
	gTasks[taskId].data[0] = blendColor;
	gTasks[taskId].data[1] = blendCoeff;
	gTasks[taskId].data[2] = blendDir;
	gTasks[taskId].data[3] = targetBlendCoeff;
	s->waitForAudienceBlend = FALSE;
}

//出場者の枠の点滅（原文 CreateContestantBoxBlinkSprites・BlinkContestantBox）
static void SpriteCB_HC_EndBlinkContestantBox(struct Sprite* sprite)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u8 spriteId2 = sprite->data[0];

	if (s != NULL)
		s->boxBlinking[sprite->data[1]] = FALSE;
	FreeSpriteOamMatrix(&gSprites[spriteId2]);
	DestroySprite(&gSprites[spriteId2]);
	DestroySpriteAndFreeResources(sprite);
	SetGpuReg(REG_OFFSET_BLDCNT, 0);
	SetGpuReg(REG_OFFSET_BLDALPHA, 0);
}

static void SpriteCB_HC_BlinkContestantBox(struct Sprite* sprite)
{
	if (sprite->affineAnimEnded)
	{
		u8 spriteId2 = sprite->data[0];

		if (gSprites[spriteId2].affineAnimEnded)
		{
			sprite->invisible = TRUE;
			gSprites[spriteId2].invisible = TRUE;
			sprite->callback = SpriteCB_HC_EndBlinkContestantBox;
		}
	}
}

static u8 HC_CreateContestantBoxBlinkSprites(struct HackContestState* s, u8 contestant)
{
	u8 spriteId1, spriteId2;
	u8 x = s->turnOrder[contestant] * 40 + 32;
	struct SpritePalette pal;

	pal.data = s->cachedWindowPalettes[5 + contestant];
	pal.tag = HC_TAG_BLINK0 + contestant;
	LoadCompressedSpriteSheet(&sSpriteSheets_Blink[contestant]);
	LoadSpritePalette(&pal);
	spriteId1 = CreateSprite(&sSpriteTemplates_Blink[contestant], 184, x, 29);
	spriteId2 = CreateSprite(&sSpriteTemplates_Blink[contestant], 248, x, 29);
	gSprites[spriteId2].oam.tileNum += 64;
	ContestFr_CopySpriteTiles(0, 3, (u8*) VRAM, (u16*) (BG_SCREEN_ADDR(28) + s->turnOrder[contestant] * 5 * 64 + 0x26), s->boxBlink1);
	ContestFr_CopySpriteTiles(0, 3, (u8*) VRAM, (u16*) (BG_SCREEN_ADDR(28) + s->turnOrder[contestant] * 5 * 64 + 0x36), s->boxBlink2);
	CpuFill32(0, s->boxBlink1 + 0x500, 0x300);
	CpuFill32(0, s->boxBlink2 + 0x500, 0x300);
	ContestFr_RequestDma3Copy(s->boxBlink1, (u8*) (OBJ_VRAM0 + gSprites[spriteId1].oam.tileNum * 32), 0x800, 1);
	ContestFr_RequestDma3Copy(s->boxBlink2, (u8*) (OBJ_VRAM0 + gSprites[spriteId2].oam.tileNum * 32), 0x800, 1);
	gSprites[spriteId1].data[0] = spriteId2;
	gSprites[spriteId2].data[0] = spriteId1;
	gSprites[spriteId1].data[1] = contestant;
	gSprites[spriteId2].data[1] = contestant;
	return spriteId1;
}

static void HC_BlinkContestantBox(struct HackContestState* s, u8 spriteId, bool8 b)
{
	u8 spriteId2;

	SetGpuReg(REG_OFFSET_BLDCNT, 0x3F00 | 0x0040); //BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND
	SetGpuReg(REG_OFFSET_BLDALPHA, 7 | (9 << 8));
	s->boxBlinking[gSprites[spriteId].data[1]] = TRUE;
	spriteId2 = gSprites[spriteId].data[0];
	StartSpriteAffineAnim(&gSprites[spriteId], 1);
	StartSpriteAffineAnim(&gSprites[spriteId2], 1);
	gSprites[spriteId].callback = SpriteCB_HC_BlinkContestantBox;
	gSprites[spriteId2].callback = SpriteCallbackDummy;
	PlaySE(b == FALSE ? HC_SE_MONS_TURN : HC_SE_PC_LOGIN);
}

//条件の星・状態の印（原文 UpdateConditionStars・DrawStatusSymbol 以下）
static bool8 HC_UpdateConditionStars(struct HackContestState* s, u8 contestantIdx, bool8 resetMod)
{
	u8 contestantOffset;
	s32 numStars;
	struct HackContestStatus* st = &s->status[contestantIdx];

	if (st->conditionMod == 0)
		return FALSE;
	contestantOffset = s->turnOrder[contestantIdx] * 5 + 2;
	numStars = st->condition / 10;
	if (st->conditionMod == 1)
	{
		HC_BgFillBoxWithTile(s, 0, 0x2034, 19, contestantOffset, 1, numStars, 17);
		if (resetMod)
		{
			PlaySE(HC_SE_EXP_MAX);
			st->conditionMod = 0;
		}
	}
	else
	{
		HC_BgFillBoxWithTile(s, 0, 0, 19, contestantOffset + numStars, 1, 3 - numStars, 17);
		if (resetMod)
		{
			PlaySE(HC_SE_CONDITION_LOSE);
			st->conditionMod = 0;
		}
	}
	return TRUE;
}

static void HC_DrawConditionStars(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		u8 contestantOffset = s->turnOrder[i] * 5 + 2;
		s32 numStars = s->status[i].condition / 10;

		HC_BgFillBoxWithTile(s, 0, 0x2034, 19, contestantOffset, 1, numStars, 17);
		HC_BgFillBoxWithTile(s, 0, 0, 19, contestantOffset + numStars, 1, 3 - numStars, 17);
	}
}

static u16 HC_GetStatusSymbolTileOffset(u8 status)
{
	u16 offset = 0;

	switch (status)
	{
	case HC_SYM_CIRCLE: offset = 0x80; break;
	case HC_SYM_WAVE: offset = 0x84; break;
	case HC_SYM_X: offset = 0x86; break;
	case HC_SYM_SWIRL: offset = 0x88; break;
	case HC_SYM_SQUARE: offset = 0x82; break;
	}
	return offset + 0x9000;
}

static bool8 HC_DrawStatusSymbol(struct HackContestState* s, u8 contestant)
{
	struct HackContestStatus* st = &s->status[contestant];
	bool8 statused = TRUE;
	u16 symbolOffset = 0;
	u8 contestantOffset = s->turnOrder[contestant] * 5 + 2;

	if (st->resistant || st->immune || st->jamSafetyCount != 0 || st->jamReduction != 0)
		symbolOffset = HC_GetStatusSymbolTileOffset(HC_SYM_CIRCLE);
	else if (st->nervous)
		symbolOffset = HC_GetStatusSymbolTileOffset(HC_SYM_WAVE);
	else if (st->numTurnsSkipped != 0 || st->noMoreTurns)
		symbolOffset = HC_GetStatusSymbolTileOffset(HC_SYM_X);
	else
		statused = FALSE;
	if (statused)
	{
		HC_BgFillBoxIncr(s, 0, symbolOffset, 20, contestantOffset, 2, 1, 17, 1);
		HC_BgFillBoxIncr(s, 0, symbolOffset + 16, 20, contestantOffset + 1, 2, 1, 17, 1);
	}
	else
		HC_BgFillBoxWithTile(s, 0, 0, 20, contestantOffset, 2, 2, 17);
	return statused;
}

static void HC_DrawStatusSymbols(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		HC_DrawStatusSymbol(s, i);
}

static void HC_DrawUnnervedSymbols(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		if (s->status[i].unnervedPokes != 0 && !HC_IsMonsTurnDisabled(s, i))
		{
			u32 contestantOffset = s->turnOrder[i] * 5 + 2;
			u16 symbolOffset = HC_GetStatusSymbolTileOffset(HC_SYM_SWIRL);

			HC_BgFillBoxIncr(s, 0, symbolOffset, 20, contestantOffset, 2, 1, 17, 1);
			symbolOffset += 16;
			HC_BgFillBoxIncr(s, 0, symbolOffset, 20, contestantOffset + 1, 2, 1, 17, 1);
			PlaySE(HC_SE_ICON_CHANGE);
		}
	}
}

//技のアニメ（案 b: FR の技のアニメが読む場所へ種族と性格値を書いて DoMoveAnim）
static u16 HC_SanitizeMove(u16 move)
{
	return move >= MOVES_COUNT ? MOVE_POUND : move;
}

static void HC_SetBattleTargetSpritePosition(void)
{
	struct Sprite* sprite = &gSprites[gBattlerSpriteIds[3]];

	sprite->pos2.x = 0;
	sprite->pos2.y = 0;
	sprite->pos1.x = GetBattlerSpriteCoord(3, 0 /* BATTLER_COORD_X */);
	sprite->pos1.y = GetBattlerSpriteCoord(3, 1 /* BATTLER_COORD_Y */);
	sprite->invisible = TRUE;
}

static void HC_CreateInvisibleBattleTargetSprite(void)
{
	gBattlerSpriteIds[3] = ContestFr_CreateInvisibleSpriteWithCallback(SpriteCallbackDummy);
	InitSpriteAffineAnim(&gSprites[gBattlerSpriteIds[gBankTarget]]);
	HC_SetBattleTargetSpritePosition();
}

static void HC_SetMoveAnimAttackerData(struct HackContestState* s, u8 contestant)
{
	s->moveAnim.contestant = contestant;
	s->moveAnim.species = HC_SanitizeSpecies(s->mons[contestant].species);
	s->moveAnim.personality = s->mons[contestant].personality;
	s->moveAnim.otId = s->mons[contestant].otId;
}

static void HC_SetMoveSpecificAnimData(struct HackContestState* s, u8 contestant)
{
	s32 i;
	u16 move = HC_SanitizeMove(s->status[contestant].currMove);
	u16 species = HC_SanitizeSpecies(s->mons[contestant].species);
	u8 targetContestant;

	s->moveAnim.targetSpecies = 0;
	s->moveAnim.targetPersonality = 0;
	s->moveAnim.hasTargetAnim = FALSE;
	ContestFr_ClearBattleAnimationVars();
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		gBattleMonForms[i] = 0;
	switch (move)
	{
	case MOVE_CURSE:
		gAnimMoveTurn = (gBaseStats[species].type1 == TYPE_GHOST || gBaseStats[species].type2 == TYPE_GHOST) ? 0 : 1;
		break;
	case MOVE_TRANSFORM:
	case MOVE_ROLEPLAY:
		targetContestant = s->status[contestant].contestantAnimTarget;
		s->moveAnim.targetSpecies = HC_SanitizeSpecies(s->mons[targetContestant].species);
		s->moveAnim.targetPersonality = s->mons[targetContestant].personality;
		s->moveAnim.hasTargetAnim = TRUE;
		break;
	case MOVE_RETURN:
		gAnimFriendship = 255;
		break;
	case MOVE_FRUSTRATION:
		gAnimFriendship = 0;
		break;
	case MOVE_SOLARBEAM:
	case MOVE_RAZORWIND:
	case MOVE_SKULLBASH:
	case MOVE_SKYATTACK:
		if (s->moveAnimTurnCount == 0)
		{
			s->moveAnimTurnCount = 2;
			gAnimMoveTurn = 0;
		}
		else
			gAnimMoveTurn = 1;
		break;
	}
	HC_SetBattleTargetSpritePosition();
}

static void HC_ClearMoveAnimData(struct HackContestState* s)
{
	s->moveAnim.species = 0;
	s->moveAnim.targetSpecies = 0;
	s->moveAnim.personality = 0;
	s->moveAnim.targetPersonality = 0;
	s->moveAnim.otId = 0;
	s->moveAnim.hasTargetAnim = FALSE;
	if (s->moveAnimTurnCount != 0)
		s->moveAnimTurnCount--;
}

static void HC_SetMoveTargetPosition(u16 move)
{
	switch (gBattleMoves[move].target)
	{
	case MOVE_TARGET_USER_OR_PARTNER: //（FR/CFRU の名前: 原文の USER_OR_SELECTED）
	case MOVE_TARGET_USER:
		gBankTarget = 2;
		break;
	default:
		gBankTarget = 3;
		break;
	}
}

//FR の技のアニメが種族・性格値を読む場所（battle_anim_mons.c の GetBattlerSpriteCoord・LaunchBattleAnimation の gAnimBattlerSpecies・gTransformedPersonalities）に書く
static void HC_WriteMoveAnimSpecies(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		u16 species = s->moveAnim.species;
		u32 personality = s->moveAnim.personality;

		if (s->moveAnim.hasTargetAnim && i == gBankTarget)
		{
			species = s->moveAnim.targetSpecies;
			personality = s->moveAnim.targetPersonality;
		}
		gBattleMons[i].species = species;
		gBattleMons[i].personality = personality;
		gTransformedPersonalities[i] = personality;
		gBattleSpritesDataPtr->bankData[i].transformSpecies = species;
	}
}

static void HC_WriteAnimBattlerSpecies(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		gAnimBattlerSpecies[i] = gBattleMons[i].species; //DoMoveAnim 内で party の種族に上書きされるので、あとから書き直す
}

//---- 出場者の絵の出入り（原文 SpriteCB_MonSlideIn／Out）----------------------------------------------------
static void SpriteCB_HC_MonSlideIn(struct Sprite* sprite)
{
	if (sprite->pos2.x != 0)
		sprite->pos2.x -= 2;
	else if (++sprite->data[0] == 31)
	{
		sprite->data[0] = 0;
		sprite->callback = SpriteCallbackDummy;
	}
}

static void SpriteCB_HC_MonSlideOut(struct Sprite* sprite)
{
	sprite->pos2.x -= 6;
	if (sprite->pos1.x + sprite->pos2.x < -32)
	{
		sprite->callback = SpriteCallbackDummy;
		sprite->invisible = TRUE;
	}
}

//---- 技の効果（仮・S3 で効果 48 の中身に差し替える）---------------------------------------------------------
static s8 HC_GetMoveExcitement(struct HackContestState* s, u16 move)
{
	struct HackContestMoveInfo info;

	HackContest_GetMove(move, &info);
	return sContestExcitementTable[s->category][info.category];
}

s8 HackContest_GetMoveExcitement(struct HackContestState* s, u16 move) //S3: AI（hack_contest_ai.c）が読む。原文 Contest_GetMoveExcitement
{
	return HC_GetMoveExcitement(s, move);
}

static bool8 HC_IsContestantAllowedToCombo(struct HackContestState* s, u8 contestant)
{
	return !(s->status[contestant].repeatedMove || s->status[contestant].nervous);
}

//原文 CalculateAppealMoveImpact（4425-4541）。gContestEffectFuncs[effect]() ＝ S3 の hack_contest_effect.c
static void HC_CalculateAppealMoveImpact(struct HackContestState* s, u8 contestant)
{
	struct HackContestStatus* st = &s->status[contestant];
	struct HackContestMoveInfo info;
	u16 move;
	u8 rnd;
	s32 i;

	st->appeal = 0;
	st->baseAppeal = 0;
	if (HC_IsMonsTurnDisabled(s, contestant))
		return;
	move = st->currMove;
	HackContest_GetMove(move, &info);
	st->moveCategory = info.category;
	if (st->currMove == st->prevMove && st->currMove != 0)
	{
		st->repeatedMove = TRUE;
		st->moveRepeatCount++;
	}
	else
		st->moveRepeatCount = 0;
	st->baseAppeal = HackContest_GetEffectAppeal(info.effect);
	st->appeal = st->baseAppeal;
	s->jam = HackContest_GetEffectJam(info.effect);
	s->jam2 = s->jam;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->status[i].jam = 0;
		s->status[i].unnervedPokes = 0;
	}
	if (st->hasJudgesAttention && !HackContest_AreMovesCombo(st->prevMove, st->currMove, s->mons[contestant].moves))
		st->hasJudgesAttention = FALSE;

	HackContest_RunEffect(s, contestant, info.effect); //原文 gContestEffectFuncs[effect]()

	if (st->conditionMod == 1)
		st->appeal += st->condition - 10;
	else if (st->appealTripleCondition)
		st->appeal += st->condition * 3;
	else
		st->appeal += st->condition;
	st->completedCombo = FALSE;
	st->usedComboMove = FALSE;
	if (HC_IsContestantAllowedToCombo(s, contestant))
	{
		bool8 completedCombo = HackContest_AreMovesCombo(st->prevMove, st->currMove, s->mons[contestant].moves);

		if (completedCombo && st->hasJudgesAttention)
		{
			st->completedCombo = completedCombo;
			st->usedComboMove = TRUE;
			st->hasJudgesAttention = FALSE;
			st->comboAppealBonus = st->baseAppeal * st->completedCombo;
			st->completedComboFlag = TRUE; //原文: completedCombo と同じ・AI が読む
		}
		else
		{
			if (HackContest_IsComboStarter(move, s->mons[contestant].moves)) //仮（hack_contest_move.c）: 原文 gContestMoves[].comboStarterId != 0
			{
				st->hasJudgesAttention = TRUE;
				st->usedComboMove = TRUE;
			}
			else
				st->hasJudgesAttention = FALSE;
		}
	}
	if (st->repeatedMove)
		st->repeatJam = (st->moveRepeatCount + 1) * 10;
	if (st->nervous)
	{
		st->hasJudgesAttention = FALSE;
		st->appeal = 0;
		st->baseAppeal = 0;
	}
	s->moveExcitement = HC_GetMoveExcitement(s, st->currMove);
	if (st->overrideCategoryExcitementMod)
		s->moveExcitement = 1;
	if (s->moveExcitement > 0)
		s->excitementAppealBonus = (s->applauseLevel + s->moveExcitement > 4) ? 60 : 10;
	else
		s->excitementAppealBonus = 0;
	rnd = Random() % (HACK_CONTEST_CONTESTANTS - 1);
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		if (i != contestant)
		{
			if (rnd == 0)
				break;
			rnd--;
		}
	}
	st->contestantAnimTarget = i;
}

static void HC_PrintAppealMoveResultText(struct HackContestState* s, u8 contestant, u8 stringId)
{
	StringCopy(gStringVar1, s->mons[contestant].nickname);
	StringCopy(gStringVar2, gMoveNames[s->status[contestant].currMove]);
	switch (s->status[s->currentContestant].moveCategory)
	{
	case 0: StringCopy(gStringVar3, gHackContest_Contest_Shyness); break;
	case 1: StringCopy(gStringVar3, gHackContest_Contest_Anxiety); break;
	case 2: StringCopy(gStringVar3, gHackContest_Contest_Laziness); break;
	case 3: StringCopy(gStringVar3, gHackContest_Contest_Hesitancy); break;
	default: StringCopy(gStringVar3, gHackContest_Contest_Fear); break;
	}
	StringExpandPlaceholders(gStringVar4, sAppealResultTexts[stringId]);
	HC_ClearGeneralTextWindow(s);
	HC_StartTextPrinter(s, gStringVar4, TRUE);
}

//---- ラウンドの終わりの計算（原文 RankContestants・SetAttentionLevels・SetContestantStatusesForNextRound）------------------
static void HC_SortContestantsByRanking(struct HackContestState* s)
{
	u8 scratch[HACK_CONTEST_CONTESTANTS];
	u16 randomOrdering[HACK_CONTEST_CONTESTANTS] = { 0 };
	s32 i, v3;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s32 j;

		randomOrdering[i] = Random();
		for (j = 0; j < i; j++)
		{
			if (randomOrdering[i] == randomOrdering[j])
			{
				i--;
				break;
			}
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		scratch[i] = HC_CONTESTANT_NONE;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		u8 j = s->status[i].ranking;

		while (1)
		{
			u8* ptr = &scratch[j];

			if (*ptr == HC_CONTESTANT_NONE)
			{
				*ptr = i;
				s->turnOrder[i] = j;
				break;
			}
			j++;
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS - 1; i++)
	{
		for (v3 = HACK_CONTEST_CONTESTANTS - 1; v3 > i; v3--)
		{
			if (s->status[v3 - 1].ranking == s->status[v3].ranking
			 && s->turnOrder[v3 - 1] < s->turnOrder[v3]
			 && randomOrdering[v3 - 1] < randomOrdering[v3])
			{
				u8 temp = s->turnOrder[v3];

				s->turnOrder[v3] = s->turnOrder[v3 - 1];
				s->turnOrder[v3 - 1] = temp;
			}
		}
	}
}

static void HC_RankContestants(struct HackContestState* s)
{
	s32 i, j;
	s16 arr[HACK_CONTEST_CONTESTANTS];

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->status[i].pointTotal += s->status[i].appeal;
		arr[i] = s->status[i].pointTotal;
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS - 1; i++)
	{
		for (j = HACK_CONTEST_CONTESTANTS - 1; j > i; j--)
		{
			if (arr[j - 1] < arr[j])
			{
				s16 temp = arr[j];

				arr[j] = arr[j - 1];
				arr[j - 1] = temp;
			}
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		for (j = 0; j < HACK_CONTEST_CONTESTANTS; j++)
		{
			if (s->status[i].pointTotal == arr[j])
			{
				s->status[i].ranking = j;
				break;
			}
		}
	}
	HC_SortContestantsByRanking(s);
	HC_ApplyNextTurnOrder(s);
}

static void HC_SetAttentionLevels(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct HackContestStatus* st = &s->status[i];
		u8 attentionLevel;

		if (st->currMove == 0) attentionLevel = 5;
		else if (st->appeal <= 0) attentionLevel = 0;
		else if (st->appeal < 30) attentionLevel = 1;
		else if (st->appeal < 60) attentionLevel = 2;
		else if (st->appeal < 80) attentionLevel = 3;
		else attentionLevel = 4;
		st->attentionLevel = attentionLevel;
	}
}

static void HC_SetContestantStatusesForNextRound(struct HackContestState* s)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct HackContestStatus* st = &s->status[i];

		st->appeal = 0;
		st->baseAppeal = 0;
		st->jamSafetyCount = 0;
		if (st->numTurnsSkipped > 0)
			st->numTurnsSkipped--;
		st->jam = 0;
		st->resistant = 0;
		st->jamReduction = 0;
		st->immune = 0;
		st->moreEasilyStartled = 0;
		st->usedRepeatableMove = 0;
		st->nervous = FALSE;
		st->effectStringId = HC_STRING_NONE;
		st->effectStringId2 = HC_STRING_NONE;
		st->conditionMod = 0;
		st->repeatedMove = FALSE;
		st->turnOrderModAction = 0;
		st->appealTripleCondition = 0;
		if (st->turnSkipped)
		{
			st->numTurnsSkipped = 1;
			st->turnSkipped = 0;
		}
		if (st->exploded)
		{
			st->noMoreTurns = TRUE;
			st->exploded = FALSE;
		}
		st->overrideCategoryExcitementMod = 0;
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->status[i].prevMove = s->status[i].currMove;
		s->moveHistory[s->appealNumber][i] = s->status[i].currMove; //原文 contest.c:3536
		s->excitementHistory[s->appealNumber][i] = HC_GetMoveExcitement(s, s->status[i].currMove);
		s->status[i].currMove = 0;
	}
	s->excitementFrozen = FALSE;
}

//---- 技を選んだあと〜ターンの処理 ---------------------------------------------------------------------------
//原文 Task_HideApplauseMeterForAppealStart／Task_WaitHideApplauseMeterForAppealStart／Task_AppealSetup
static void Task_HC_AppealSetup(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (++gTasks[taskId].data[0] > 19)
	{
		s->turnNumber = 0;
		gTasks[taskId].data[0] = 0; //APPEALSTATE_START_TURN
		gTasks[taskId].func = Task_HC_DoAppeals;
	}
}

static void Task_HC_WaitHideApplauseMeterForAppealStart(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (!s->applauseMeterIsMoving && !s->sliderHeartsAnimating)
		gTasks[taskId].func = Task_HC_AppealSetup;
}

static void Task_HC_HideApplauseMeterForAppealStart(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (++gTasks[taskId].data[0] > 2)
	{
		gTasks[taskId].data[0] = 0;
		if (++gTasks[taskId].data[1] == 2)
		{
			HC_SlideApplauseMeterOut(s);
			HC_AnimateSliderHearts(s, 1 /* DISAPPEAR */);
			gTasks[taskId].func = Task_HC_WaitHideApplauseMeterForAppealStart;
		}
	}
}

enum
{
	AS_START_TURN, AS_CHECK_SKIP_TURN, AS_SLIDE_MON_IN, AS_WAIT_SLIDE_MON, AS_PRINT_USED_MOVE_MSG, AS_WAIT_USED_MOVE_MSG,
	AS_MOVE_ANIM, AS_WAIT_MOVE_ANIM, AS_MOVE_ANIM_MULTITURN, AS_TRY_PRINT_MOVE_RESULT, AS_WAIT_MOVE_RESULT_MSG,
	AS_CHECK_TURN_ORDER_MOD, AS_WAIT_JUDGE_TURN_ORDER, AS_TRY_SHOW_NEXT_TURN_GFX, AS_UPDATE_MOVE_USERS_HEARTS, AS_WAIT_MOVE_USERS_HEARTS,
	AS_TRY_JUDGE_STAR, AS_WAIT_JUDGE_STAR, AS_UPDATE_MOVE_USERS_STARS, AS_WAIT_MOVE_USERS_STARS, AS_UPDATE_MOVE_USERS_STATUS,
	AS_UPDATE_OPPONENTS, AS_UPDATE_OPPONENT, AS_WAIT_OPPONENT_RESPONSE_MSG, AS_UPDATE_OPPONENT_HEARTS, AS_WAIT_OPPONENT_HEARTS,
	AS_UPDATE_OPPONENT_STARS, AS_WAIT_OPPONENT_STARS, AS_UPDATE_OPPONENT_STATUS, AS_TRY_PRINT_SKIP_NEXT_TURN_MSG, AS_WAIT_SKIP_NEXT_TURN_MSG,
	AS_PRINT_COMBO_MSG, AS_WAIT_JUDGE_COMBO, AS_TRY_UPDATE_HEARTS_FROM_COMBO, AS_WAIT_HEARTS_FROM_COMBO, AS_CHECK_REPEATED_MOVE,
	AS_WAIT_JUDGE_REPEATED_MOVE, AS_UPDATE_HEARTS_FROM_REPEAT, AS_WAIT_HEARTS_FROM_REPEAT, AS_UPDATE_CROWD, AS_DO_CROWD_UNEXCITED,
	AS_DO_CROWD_EXCITED, AS_WAIT_EXCITEMENT_HEARTS, AS_PRINT_CROWD_WATCHES_MSG, AS_PRINT_MON_MOVE_IGNORED_MSG, AS_WAIT_MON_MOVE_IGNORED_MSG,
	AS_PRINT_TOO_NERVOUS_MSG, AS_WAIT_TOO_NERVOUS_MSG, AS_SLIDE_APPLAUSE_OUT, AS_WAIT_SLIDE_APPLAUSE, AS_SLIDE_MON_OUT, AS_FREE_MON_SPRITE,
	AS_START_TURN_END_DELAY, AS_PRINT_SKIP_TURN_MSG, AS_WAIT_SKIP_TURN_MSG, AS_TURN_END_DELAY, AS_START_NEXT_TURN
};

#define tState data[0]
#define tMonSpriteId data[2]
#define tCounter data[10]

//原文 Task_DoAppeals（1727-2444・リンク通信の状態 APPEALSTATE_WAIT_LINK は無し）
static void Task_HC_DoAppeals(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u8 spriteId;
	s32 i;
	u8 contestant;
	struct HackContestStatus* st;
	s8 r3;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	contestant = s->currentContestant;
	st = &s->status[contestant];
	switch (gTasks[taskId].tState)
	{
	case AS_START_TURN:
		for (i = 0; s->turnNumber != s->turnOrder[i]; i++)
			;
		s->currentContestant = i;
		HC_CalculateAppealMoveImpact(s, s->currentContestant);
		gTasks[taskId].tState = AS_CHECK_SKIP_TURN;
		return;
	case AS_CHECK_SKIP_TURN:
		if (st->numTurnsSkipped != 0 || st->noMoreTurns)
			gTasks[taskId].tState = AS_PRINT_SKIP_TURN_MSG;
		else
		{
			HC_ClearGeneralTextWindow(s);
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_SLIDE_MON_IN;
		}
		return;
	case AS_SLIDE_MON_IN:
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
			gBattleMonForms[i] = 0;
		s->moveAnim.species = 0;
		s->moveAnim.targetSpecies = 0;
		s->moveAnim.hasTargetAnim = FALSE;
		HC_SetMoveAnimAttackerData(s, s->currentContestant);
		spriteId = HC_CreateContestantSprite(s->mons[s->currentContestant].species, s->mons[s->currentContestant].otId, s->mons[s->currentContestant].personality);
		gSprites[spriteId].pos2.x = 120;
		gSprites[spriteId].callback = SpriteCB_HC_MonSlideIn;
		gTasks[taskId].tMonSpriteId = spriteId;
		gBattlerSpriteIds[gBankAttacker] = spriteId;
		HC_BlinkContestantBox(s, HC_CreateContestantBoxBlinkSprites(s, s->currentContestant), FALSE);
		gTasks[taskId].tState = AS_WAIT_SLIDE_MON;
		return;
	case AS_WAIT_SLIDE_MON:
		spriteId = gTasks[taskId].tMonSpriteId;
		if (gSprites[spriteId].callback == SpriteCallbackDummy)
		{
			if (!s->boxBlinking[contestant])
				gTasks[taskId].tState = AS_PRINT_USED_MOVE_MSG;
		}
		return;
	case AS_PRINT_USED_MOVE_MSG:
		if (st->nervous)
			gTasks[taskId].tState = AS_PRINT_TOO_NERVOUS_MSG;
		else
		{
			HC_ClearGeneralTextWindow(s);
			StringCopy(gStringVar1, s->mons[contestant].nickname);
			StringCopy(gStringVar2, gMoveNames[st->currMove]);
			StringExpandPlaceholders(gStringVar4, gHackContest_MonAppealedWithMove);
			HC_StartTextPrinter(s, gStringVar4, TRUE);
			gTasks[taskId].tState = AS_WAIT_USED_MOVE_MSG;
		}
		return;
	case AS_WAIT_USED_MOVE_MSG:
		if (!HC_RunTextPrinters())
		{
			s->moveAnimTurnCount = 0;
			gTasks[taskId].tState = AS_MOVE_ANIM;
		}
		return;
	case AS_MOVE_ANIM:
	{
		u16 move = HC_SanitizeMove(st->currMove);

		HC_SetMoveSpecificAnimData(s, s->currentContestant);
		HC_SetMoveAnimAttackerData(s, s->currentContestant);
		HC_SetMoveTargetPosition(move);
		HC_WriteMoveAnimSpecies(s);
		DoMoveAnim(move);
		HC_WriteAnimBattlerSpecies(s);
		gTasks[taskId].tState = AS_WAIT_MOVE_ANIM;
		return;
	}
	case AS_WAIT_MOVE_ANIM:
		gAnimScriptCallback();
		if (!gAnimScriptActive)
		{
			HC_ClearMoveAnimData(s);
			HC_LoadContestBgAfterMoveAnim(s);
			if (s->moveAnimTurnCount != 0)
			{
				gTasks[taskId].tCounter = 0;
				gTasks[taskId].tState = AS_MOVE_ANIM_MULTITURN;
			}
			else
			{
				if (!st->hasJudgesAttention)
					HC_StopFlashJudgeAttentionEye(s, contestant);
				HC_DrawUnnervedSymbols(s);
				gTasks[taskId].tState = AS_TRY_PRINT_MOVE_RESULT;
			}
		}
		return;
	case AS_MOVE_ANIM_MULTITURN:
		if (gTasks[taskId].tCounter++ > 30)
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_MOVE_ANIM;
		}
		return;
	case AS_TRY_PRINT_MOVE_RESULT:
		gTasks[taskId].data[1] = 0;
		if (st->effectStringId != HC_STRING_NONE)
		{
			HC_PrintAppealMoveResultText(s, contestant, st->effectStringId);
			st->effectStringId = HC_STRING_NONE;
			gTasks[taskId].tState = AS_WAIT_MOVE_RESULT_MSG;
		}
		else if (st->effectStringId2 != HC_STRING_NONE)
		{
			for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
			{
				if (i != contestant && s->status[i].effectStringId != HC_STRING_NONE)
					break;
			}
			if (i == HACK_CONTEST_CONTESTANTS)
			{
				HC_PrintAppealMoveResultText(s, contestant, st->effectStringId2);
				st->effectStringId2 = HC_STRING_NONE;
				gTasks[taskId].tState = AS_WAIT_MOVE_RESULT_MSG;
			}
			else
				gTasks[taskId].tState = AS_CHECK_TURN_ORDER_MOD;
		}
		else
			gTasks[taskId].tState = AS_CHECK_TURN_ORDER_MOD;
		return;
	case AS_WAIT_MOVE_RESULT_MSG:
		if (!HC_RunTextPrinters())
			gTasks[taskId].tState = AS_TRY_PRINT_MOVE_RESULT;
		return;
	case AS_CHECK_TURN_ORDER_MOD:
		if (st->turnOrderModAction == 1)
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_NUMBER_ONE);
		else if (st->turnOrderModAction == 2)
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_NUMBER_FOUR);
		else if (st->turnOrderModAction == 3)
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_QUESTION);
		else
		{
			gTasks[taskId].tState = AS_TRY_SHOW_NEXT_TURN_GFX;
			return;
		}
		gTasks[taskId].tState = AS_WAIT_JUDGE_TURN_ORDER;
		return;
	case AS_WAIT_JUDGE_TURN_ORDER:
		if (!s->waitForJudgeSpeechBubble)
			gTasks[taskId].tState = AS_TRY_SHOW_NEXT_TURN_GFX;
		return;
	case AS_TRY_SHOW_NEXT_TURN_GFX:
		HC_ShowHideNextTurnGfx(s, TRUE);
		gTasks[taskId].tState = AS_UPDATE_MOVE_USERS_HEARTS;
		return;
	case AS_UPDATE_MOVE_USERS_HEARTS:
		HC_UpdateAppealHearts(s, 0, st->appeal, contestant);
		gTasks[taskId].tState = AS_WAIT_MOVE_USERS_HEARTS;
		return;
	case AS_WAIT_MOVE_USERS_HEARTS:
		if (!s->updatingAppealHearts[s->currentContestant])
			gTasks[taskId].tState = AS_TRY_JUDGE_STAR;
		return;
	case AS_TRY_JUDGE_STAR:
		if (st->conditionMod == 1)
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_STAR);
		gTasks[taskId].tState = AS_WAIT_JUDGE_STAR;
		return;
	case AS_WAIT_JUDGE_STAR:
		if (!s->waitForJudgeSpeechBubble)
			gTasks[taskId].tState = AS_UPDATE_MOVE_USERS_STARS;
		return;
	case AS_UPDATE_MOVE_USERS_STARS:
		if (HC_UpdateConditionStars(s, contestant, TRUE))
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_WAIT_MOVE_USERS_STARS;
		}
		else
			gTasks[taskId].tState = AS_UPDATE_MOVE_USERS_STATUS;
		return;
	case AS_WAIT_MOVE_USERS_STARS:
		if (++gTasks[taskId].tCounter > 20)
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_UPDATE_MOVE_USERS_STATUS;
		}
		return;
	case AS_UPDATE_MOVE_USERS_STATUS:
		if (HC_DrawStatusSymbol(s, contestant))
			PlaySE(HC_SE_ICON_CHANGE);
		gTasks[taskId].tState = AS_UPDATE_OPPONENTS;
		return;
	case AS_UPDATE_OPPONENTS:
		gTasks[taskId].data[1] = 0;
		gTasks[taskId].tState = AS_UPDATE_OPPONENT;
		return;
	case AS_UPDATE_OPPONENT:
	{
		s32 j = 0;

		r3 = FALSE;
		for (i = gTasks[taskId].data[1]; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			r3 = FALSE;
			for (j = 0; j < HACK_CONTEST_CONTESTANTS; j++)
			{
				if (j != contestant && s->turnOrder[j] == i && s->status[j].effectStringId != HC_STRING_NONE)
				{
					r3 = TRUE;
					break;
				}
			}
			if (r3)
				break;
		}
		if (r3)
		{
			gTasks[taskId].data[1] = s->turnOrder[j];
			HC_PrintAppealMoveResultText(s, j, s->status[j].effectStringId);
			s->status[j].effectStringId = HC_STRING_NONE;
			gTasks[taskId].tState = AS_WAIT_OPPONENT_RESPONSE_MSG;
		}
		else
		{
			gTasks[taskId].data[1] = 0;
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_TRY_PRINT_SKIP_NEXT_TURN_MSG;
			HC_DrawStatusSymbols(s);
		}
		return;
	}
	case AS_WAIT_OPPONENT_RESPONSE_MSG:
		if (!HC_RunTextPrinters())
			gTasks[taskId].tState = AS_UPDATE_OPPONENT_HEARTS;
		return;
	case AS_UPDATE_OPPONENT_HEARTS:
		for (i = 0; gTasks[taskId].data[1] != s->turnOrder[i]; i++)
			;
		HC_UpdateAppealHearts(s, s->status[i].appeal + s->status[i].jam, -s->status[i].jam, i);
		gTasks[taskId].tState = AS_WAIT_OPPONENT_HEARTS;
		return;
	case AS_WAIT_OPPONENT_HEARTS:
		for (i = 0; gTasks[taskId].data[1] != s->turnOrder[i]; i++)
			;
		if (!s->updatingAppealHearts[i])
			gTasks[taskId].tState = AS_UPDATE_OPPONENT_STARS;
		return;
	case AS_UPDATE_OPPONENT_STARS:
		for (i = 0; gTasks[taskId].data[1] != s->turnOrder[i]; i++)
			;
		if (HC_UpdateConditionStars(s, i, TRUE))
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_WAIT_OPPONENT_STARS;
		}
		else
			gTasks[taskId].tState = AS_UPDATE_OPPONENT_STATUS;
		return;
	case AS_WAIT_OPPONENT_STARS:
		if (++gTasks[taskId].tCounter > 20)
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_UPDATE_OPPONENT_STATUS;
		}
		return;
	case AS_UPDATE_OPPONENT_STATUS:
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			if (s->turnOrder[i] == gTasks[taskId].data[1])
				break;
		}
		if (HC_DrawStatusSymbol(s, i))
			PlaySE(HC_SE_ICON_CHANGE);
		else
			PlaySE(HC_SE_ICON_CLEAR);
		if (s->status[i].judgesAttentionWasRemoved)
		{
			HC_StopFlashJudgeAttentionEye(s, i);
			s->status[i].judgesAttentionWasRemoved = FALSE;
		}
		gTasks[taskId].data[1]++;
		gTasks[taskId].tState = AS_UPDATE_OPPONENT;
		return;
	case AS_TRY_PRINT_SKIP_NEXT_TURN_MSG:
		if (gTasks[taskId].tCounter++ > 9)
		{
			gTasks[taskId].tCounter = 0;
			if (st->numTurnsSkipped != 0 || st->turnSkipped)
			{
				HC_ClearGeneralTextWindow(s);
				StringCopy(gStringVar1, s->mons[contestant].nickname);
				StringExpandPlaceholders(gStringVar4, gHackContest_MonCantAppealNextTurn);
				HC_StartTextPrinter(s, gStringVar4, TRUE);
			}
			gTasks[taskId].tState = AS_WAIT_SKIP_NEXT_TURN_MSG;
		}
		return;
	case AS_WAIT_SKIP_NEXT_TURN_MSG:
		if (!HC_RunTextPrinters())
			gTasks[taskId].tState = st->usedComboMove ? AS_PRINT_COMBO_MSG : AS_CHECK_REPEATED_MOVE;
		return;
	case AS_PRINT_COMBO_MSG:
		if (st->completedCombo)
		{
			HC_ClearGeneralTextWindow(s);
			HC_StartTextPrinter(s, gHackContest_AppealComboWentOverWell, TRUE);
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_TWO_EXCL);
		}
		else
		{
			HC_ClearGeneralTextWindow(s);
			StringCopy(gStringVar1, s->mons[contestant].nickname);
			StringExpandPlaceholders(gStringVar4, gHackContest_JudgeLookedAtMonExpectantly);
			HC_StartTextPrinter(s, gStringVar4, TRUE);
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_ONE_EXCL);
		}
		gTasks[taskId].tCounter = 0;
		gTasks[taskId].tState = AS_WAIT_JUDGE_COMBO;
		return;
	case AS_WAIT_JUDGE_COMBO:
		if (!s->waitForJudgeSpeechBubble)
		{
			HC_StartStopFlashJudgeAttentionEye(s, s->currentContestant);
			gTasks[taskId].tState = AS_TRY_UPDATE_HEARTS_FROM_COMBO;
		}
		return;
	case AS_TRY_UPDATE_HEARTS_FROM_COMBO:
		if (!HC_RunTextPrinters())
		{
			if (++gTasks[taskId].tCounter > 50)
			{
				if (!st->hasJudgesAttention)
				{
					HC_UpdateAppealHearts(s, st->appeal, st->comboAppealBonus, contestant);
					st->appeal += st->comboAppealBonus;
				}
				gTasks[taskId].tState = AS_WAIT_HEARTS_FROM_COMBO;
			}
		}
		return;
	case AS_WAIT_HEARTS_FROM_COMBO:
		if (!s->updatingAppealHearts[contestant])
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_CHECK_REPEATED_MOVE;
		}
		return;
	case AS_CHECK_REPEATED_MOVE:
		if (st->repeatedMove)
		{
			HC_ClearGeneralTextWindow(s);
			StringCopy(gStringVar1, s->mons[contestant].nickname);
			StringExpandPlaceholders(gStringVar4, gHackContest_RepeatedAppeal);
			HC_StartTextPrinter(s, gStringVar4, TRUE);
			gTasks[taskId].tCounter = 0;
			HC_DoJudgeSpeechBubble(s, HC_JUDGE_SWIRL);
			gTasks[taskId].tState = AS_WAIT_JUDGE_REPEATED_MOVE;
		}
		else
			gTasks[taskId].tState = AS_UPDATE_CROWD;
		return;
	case AS_WAIT_JUDGE_REPEATED_MOVE:
		if (!s->waitForJudgeSpeechBubble)
			gTasks[taskId].tState = AS_UPDATE_HEARTS_FROM_REPEAT;
		return;
	case AS_UPDATE_HEARTS_FROM_REPEAT:
		if (!HC_RunTextPrinters())
		{
			HC_UpdateAppealHearts(s, st->appeal, -st->repeatJam, contestant);
			st->appeal -= st->repeatJam;
			gTasks[taskId].tState = AS_WAIT_HEARTS_FROM_REPEAT;
		}
		return;
	case AS_WAIT_HEARTS_FROM_REPEAT:
		if (!s->updatingAppealHearts[contestant])
		{
			gTasks[taskId].tCounter = 0;
			HC_ClearGeneralTextWindow(s);
			gTasks[taskId].tState = AS_UPDATE_CROWD;
		}
		return;
	case AS_UPDATE_CROWD:
		if (s->excitementFrozen && contestant != s->excitementFreezer)
			gTasks[taskId].tState = AS_PRINT_CROWD_WATCHES_MSG;
		else
		{
			r3 = s->moveExcitement;
			if (st->overrideCategoryExcitementMod)
			{
				r3 = 1;
				StringCopy(gStringVar3, gMoveNames[st->currMove]);
			}
			else
			{
				struct HackContestMoveInfo info;

				HackContest_GetMove(st->currMove, &info);
				StringCopy(gStringVar3, sContestConditions[info.category]);
			}
			if (r3 > 0 && st->repeatedMove)
				r3 = 0;
			HC_ClearGeneralTextWindow(s);
			StringCopy(gStringVar1, s->mons[contestant].nickname);
			s->applauseLevel += r3;
			if (s->applauseLevel < 0)
				s->applauseLevel = 0;
			if (r3 == 0)
				gTasks[taskId].tState = AS_SLIDE_APPLAUSE_OUT;
			else
			{
				if (r3 < 0)
					StringExpandPlaceholders(gStringVar4, gHackContest_MonsXDidntGoOverWell);
				else if (r3 > 0 && s->applauseLevel <= 4)
					StringExpandPlaceholders(gStringVar4, gHackContest_MonsXWentOverGreat);
				else
					StringExpandPlaceholders(gStringVar4, gHackContest_MonsXGotTheCrowdGoing);
				HC_StartTextPrinter(s, gStringVar4, TRUE);
				gTasks[taskId].tCounter = 0;
				gTasks[taskId].data[11] = 0;
				gTasks[taskId].tState = r3 < 0 ? AS_DO_CROWD_UNEXCITED : AS_DO_CROWD_EXCITED;
			}
		}
		return;
	case AS_DO_CROWD_UNEXCITED:
		switch (gTasks[taskId].tCounter)
		{
		case 0:
			HC_BlendAudienceBackground(s, -1, 1);
			PlayFanfare(HC_MUS_TOO_BAD);
			gTasks[taskId].tCounter++;
			break;
		case 1:
			if (!s->waitForAudienceBlend && !HC_RunTextPrinters())
			{
				HC_ShowAndUpdateApplauseMeter(s, -1);
				gTasks[taskId].tCounter++;
			}
			break;
		case 2:
			if (!s->isShowingApplauseMeter)
			{
				if (gTasks[taskId].data[11]++ > 29)
				{
					gTasks[taskId].data[11] = 0;
					HC_BlendAudienceBackground(s, -1, -1);
					gTasks[taskId].tCounter++;
				}
			}
			break;
		case 3:
			if (!gPaletteFade->active)
			{
				gTasks[taskId].tCounter = 0;
				gTasks[taskId].data[11] = 0;
				gTasks[taskId].tState = AS_WAIT_EXCITEMENT_HEARTS;
			}
			break;
		}
		return;
	case AS_DO_CROWD_EXCITED:
		switch (gTasks[taskId].tCounter)
		{
		case 0:
			if (!HC_RunTextPrinters())
			{
				HC_BlendAudienceBackground(s, 1, 1);
				gTasks[taskId].tCounter++;
			}
			break;
		case 1:
			if (!s->waitForAudienceBlend)
			{
				HC_AnimateAudience(s);
				PlaySE(HC_SE_ENCORE2);
				HC_ShowAndUpdateApplauseMeter(s, 1);
				gTasks[taskId].tCounter++;
			}
			break;
		case 2:
			if (!s->isShowingApplauseMeter)
			{
				if (gTasks[taskId].data[11]++ > 29)
				{
					gTasks[taskId].data[11] = 0;
					HC_UpdateAppealHearts(s, st->appeal, s->excitementAppealBonus, contestant);
					st->appeal += s->excitementAppealBonus;
					gTasks[taskId].tCounter++;
				}
			}
			break;
		case 3:
			if (!s->updatingAppealHearts[contestant])
			{
				if (!s->animatingAudience)
				{
					HC_BlendAudienceBackground(s, 1, -1);
					gTasks[taskId].tCounter++;
				}
			}
			break;
		case 4:
			if (!gPaletteFade->active)
			{
				gTasks[taskId].tCounter = 0;
				gTasks[taskId].data[11] = 0;
				gTasks[taskId].tState = AS_WAIT_EXCITEMENT_HEARTS;
			}
			break;
		}
		return;
	case AS_WAIT_EXCITEMENT_HEARTS:
		if (!s->updatingAppealHearts[contestant])
		{
			HC_ClearGeneralTextWindow(s);
			gTasks[taskId].tState = AS_SLIDE_APPLAUSE_OUT;
		}
		return;
	case AS_PRINT_CROWD_WATCHES_MSG:
		HC_ClearGeneralTextWindow(s);
		StringCopy(gStringVar3, s->mons[s->excitementFreezer].nickname);
		StringCopy(gStringVar1, s->mons[contestant].nickname);
		StringCopy(gStringVar2, gMoveNames[st->currMove]);
		StringExpandPlaceholders(gStringVar4, gHackContest_CrowdContinuesToWatchMon);
		HC_StartTextPrinter(s, gStringVar4, TRUE);
		gTasks[taskId].tState = AS_PRINT_MON_MOVE_IGNORED_MSG;
		return;
	case AS_PRINT_MON_MOVE_IGNORED_MSG:
		if (!HC_RunTextPrinters())
		{
			HC_ClearGeneralTextWindow(s);
			StringExpandPlaceholders(gStringVar4, gHackContest_MonsMoveIsIgnored);
			HC_StartTextPrinter(s, gStringVar4, TRUE);
			gTasks[taskId].tState = AS_WAIT_MON_MOVE_IGNORED_MSG;
		}
		return;
	case AS_WAIT_MON_MOVE_IGNORED_MSG:
		if (!HC_RunTextPrinters())
		{
			HC_ClearGeneralTextWindow(s);
			gTasks[taskId].tState = AS_SLIDE_APPLAUSE_OUT;
		}
		return;
	case AS_PRINT_TOO_NERVOUS_MSG:
		if (st->hasJudgesAttention)
			st->hasJudgesAttention = FALSE;
		HC_StartStopFlashJudgeAttentionEye(s, contestant);
		StringCopy(gStringVar1, s->mons[contestant].nickname);
		StringCopy(gStringVar2, gMoveNames[st->currMove]);
		StringExpandPlaceholders(gStringVar4, gHackContest_MonWasTooNervousToMove);
		HC_StartTextPrinter(s, gStringVar4, TRUE);
		gTasks[taskId].tState = AS_WAIT_TOO_NERVOUS_MSG;
		return;
	case AS_WAIT_TOO_NERVOUS_MSG:
		if (!HC_RunTextPrinters())
			gTasks[taskId].tState = AS_SLIDE_APPLAUSE_OUT;
		return;
	case AS_SLIDE_APPLAUSE_OUT:
		HC_SlideApplauseMeterOut(s);
		gTasks[taskId].tState = AS_WAIT_SLIDE_APPLAUSE;
		return;
	case AS_WAIT_SLIDE_APPLAUSE:
		if (!s->applauseMeterIsMoving)
		{
			if (s->applauseLevel > 4)
			{
				s->applauseLevel = 0;
				HC_UpdateApplauseMeter(s);
			}
			gTasks[taskId].tState = AS_SLIDE_MON_OUT;
		}
		return;
	case AS_SLIDE_MON_OUT:
		spriteId = gTasks[taskId].tMonSpriteId;
		gSprites[spriteId].callback = SpriteCB_HC_MonSlideOut;
		gTasks[taskId].tState = AS_FREE_MON_SPRITE;
		return;
	case AS_FREE_MON_SPRITE:
		spriteId = gTasks[taskId].tMonSpriteId;
		if (gSprites[spriteId].invisible)
		{
			FreeSpriteOamMatrix(&gSprites[spriteId]);
			DestroySprite(&gSprites[spriteId]);
			gTasks[taskId].tState = AS_START_TURN_END_DELAY;
		}
		return;
	case AS_START_TURN_END_DELAY:
		gTasks[taskId].tCounter = 0;
		gTasks[taskId].tState = AS_TURN_END_DELAY;
		return;
	case AS_PRINT_SKIP_TURN_MSG:
		HC_ClearGeneralTextWindow(s);
		StringCopy(gStringVar1, s->mons[contestant].nickname);
		StringExpandPlaceholders(gStringVar4, gHackContest_MonWasWatchingOthers);
		HC_StartTextPrinter(s, gStringVar4, TRUE);
		gTasks[taskId].tState = AS_WAIT_SKIP_TURN_MSG;
		return;
	case AS_WAIT_SKIP_TURN_MSG:
		if (!HC_RunTextPrinters())
			gTasks[taskId].tState = AS_TURN_END_DELAY;
		return;
	case AS_TURN_END_DELAY:
		if (++gTasks[taskId].tCounter > 29)
		{
			gTasks[taskId].tCounter = 0;
			gTasks[taskId].tState = AS_START_NEXT_TURN;
		}
		return;
	case AS_START_NEXT_TURN:
		if (++s->turnNumber == HACK_CONTEST_CONTESTANTS)
		{
			gTasks[taskId].tState = 0;
			gTasks[taskId].data[1] = 0;
			gTasks[taskId].tMonSpriteId = 0;
			gTasks[taskId].func = Task_HC_FinishRoundOfAppeals;
		}
		else
			gTasks[taskId].tState = AS_START_TURN;
		return;
	}
}

#undef tState
#undef tMonSpriteId
#undef tCounter

//---- ラウンドの終わり（原文 Task_FinishRoundOfAppeals〜Task_StartNewRoundOfAppeals）---------------------------------
static void Task_HC_DropCurtainAtRoundEnd(u8 taskId);
static void Task_HC_TryStartNextRoundOfAppeals(u8 taskId);

static void Task_HC_WaitForHeartSlidersAgain(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (HC_SlidersDoneUpdating(s))
	{
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].func = Task_HC_DropCurtainAtRoundEnd;
	}
}

static void Task_HC_ReUpdateHeartSliders(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (gTasks[taskId].data[0]++ > 29)
	{
		gTasks[taskId].data[0] = 0;
		HC_UpdateHeartSliders(s);
		gTasks[taskId].func = Task_HC_WaitForHeartSlidersAgain;
	}
}

static void Task_HC_PrintRoundResultText(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (gTasks[taskId].data[0] == 0)
	{
		u8 attention = s->status[s->playerIndex].attentionLevel;

		HC_ClearGeneralTextWindow(s);
		StringCopy(gStringVar1, s->mons[s->playerIndex].nickname);
		StringExpandPlaceholders(gStringVar4, sRoundResultTexts[attention]);
		HC_StartTextPrinter(s, gStringVar4, TRUE);
		gTasks[taskId].data[0]++;
	}
	else if (!HC_RunTextPrinters())
	{
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].func = Task_HC_ReUpdateHeartSliders;
	}
}

static void Task_HC_WaitPrintRoundResult(u8 taskId)
{
	if (++gTasks[taskId].data[0] > 2)
	{
		gTasks[taskId].data[0] = 0;
		if (--gTasks[taskId].data[1] == 0)
			gTasks[taskId].func = Task_HC_PrintRoundResultText;
	}
}

static void Task_HC_RestorePlttBufferUnfaded(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	CpuCopy32(s->cachedPlttUnfaded, gPlttBufferUnfaded, 0x400);
	gTasks[taskId].data[0] = 0;
	gTasks[taskId].data[1] = 2;
	gTasks[taskId].func = Task_HC_WaitPrintRoundResult;
}

static void Task_HC_WaitForHeartSliders(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (HC_SlidersDoneUpdating(s))
		gTasks[taskId].func = Task_HC_RestorePlttBufferUnfaded;
}

static void Task_HC_UpdateHeartSliders(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	switch (gTasks[taskId].data[0])
	{
	case 0:
		if (++gTasks[taskId].data[1] > 20)
		{
			HC_AnimateSliderHearts(s, 2 /* APPEAR */);
			gTasks[taskId].data[1] = 0;
			gTasks[taskId].data[0]++;
		}
		break;
	case 1:
		if (!s->sliderHeartsAnimating)
		{
			if (++gTasks[taskId].data[1] > 20)
			{
				gTasks[taskId].data[1] = 0;
				gTasks[taskId].data[0]++;
			}
		}
		break;
	case 2:
		HC_UpdateHeartSliders(s);
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].data[1] = 0;
		gTasks[taskId].func = Task_HC_WaitForHeartSliders;
		break;
	}
}

static void Task_HC_ReadyUpdateHeartSliders(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	HC_ShowHideNextTurnGfx(s, FALSE);
	gTasks[taskId].data[0] = 0;
	gTasks[taskId].data[1] = 0;
	gTasks[taskId].func = Task_HC_UpdateHeartSliders;
}

static void Task_HC_FinishRoundOfAppeals(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	HC_RankContestants(s);
	HC_SetAttentionLevels(s);
	gTasks[taskId].data[0] = 0;
	gTasks[taskId].func = Task_HC_ReadyUpdateHeartSliders;
}

//幕を下ろす→次のラウンドの準備→幕を上げる（原文 Task_DropCurtainAtRoundEnd〜Task_UpdateContestantBoxOrder）
static void Task_HC_UpdateContestantBoxOrder(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	HC_UpdateContestantBoxOrder(s);
	gTasks[taskId].func = Task_HC_TryStartNextRoundOfAppeals;
}

static void Task_HC_UpdateRaiseCurtainAtRoundEnd(u8 taskId)
{
	*(s16*) &gBattle_BG1_Y += 7;
	if ((s16) gBattle_BG1_Y > DISPLAY_HEIGHT)
		gTasks[taskId].func = Task_HC_UpdateContestantBoxOrder;
}

static void Task_HC_StartRaiseCurtainAtRoundEnd(u8 taskId)
{
	if (gTasks[taskId].data[2] < 10)
		gTasks[taskId].data[2]++;
	else
	{
		gTasks[taskId].data[2] = 0;
		ContestFr_PlaySE12WithPanning(HC_SE_CURTAIN_RISE, 0);
		gTasks[taskId].func = Task_HC_UpdateRaiseCurtainAtRoundEnd;
	}
}

static void Task_HC_WaitRaiseCurtainAtRoundEnd(u8 taskId)
{
	if (gTasks[taskId].data[2] < 10)
		gTasks[taskId].data[2]++;
	else if (gTasks[taskId].data[1] == 0)
	{
		if (gTasks[taskId].data[0] == 16)
			gTasks[taskId].data[1]++;
		else
			gTasks[taskId].data[0]++;
	}
	else if (gTasks[taskId].data[0] == 0)
	{
		gTasks[taskId].data[1] = 0;
		gTasks[taskId].data[2] = 0;
		gTasks[taskId].func = Task_HC_StartRaiseCurtainAtRoundEnd;
	}
	else
		gTasks[taskId].data[0]--;
}

static void Task_HC_ResetForNextRound(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	s32 i;

	switch (gTasks[taskId].data[0])
	{
	case 0:
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
			s->prevTurnOrder[i] = s->turnOrder[i];
		HC_FillContestantWindowBgs(s);
		HC_DrawConditionStars(s);
		HC_DrawContestantWindows(s);
		HC_ShowHideNextTurnGfx(s, TRUE);
		HC_UpdateSliderHeartSpriteYPositions(s);
		gTasks[taskId].data[0] = 1;
		break;
	case 1:
		HC_SetContestantStatusesForNextRound(s);
		gTasks[taskId].data[0] = 3;
		break;
	case 3:
		HC_DrawStatusSymbols(s);
		HC_SwapMoveDescAndContestTilemaps(s);
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].func = Task_HC_WaitRaiseCurtainAtRoundEnd;
		break;
	}
}

static void Task_HC_UpdateCurtainDropAtRoundEnd(u8 taskId)
{
	if ((s16) (gBattle_BG1_Y -= 7) < 0)
		gBattle_BG1_Y = 0;
	if (gBattle_BG1_Y == 0)
	{
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].data[1] = 0;
		gTasks[taskId].data[2] = 0;
		gTasks[taskId].func = Task_HC_ResetForNextRound;
	}
}

static void Task_HC_StartDropCurtainAtRoundEnd(u8 taskId)
{
	gBattle_BG1_X = 0;
	gBattle_BG1_Y = DISPLAY_HEIGHT;
	ContestFr_PlaySE12WithPanning(HC_SE_CURTAIN_FALL, 0);
	gTasks[taskId].func = Task_HC_UpdateCurtainDropAtRoundEnd;
}

static void Task_HC_DropCurtainAtRoundEnd(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	HC_SetBgForCurtainDrop(s);
	gTasks[taskId].func = Task_HC_StartDropCurtainAtRoundEnd;
}

static void Task_HC_StartNewRoundOfAppeals(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (!s->applauseMeterIsMoving)
		gTasks[taskId].func = Task_HC_DisplayAppealNumber;
}

static void Task_HC_EndAppeals(u8 taskId);

static void Task_HC_TryStartNextRoundOfAppeals(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	u16 bg0 = GetGpuReg(REG_OFFSET_BG0CNT);
	u16 bg2 = GetGpuReg(REG_OFFSET_BG2CNT);

	bg0 &= ~0x3;
	bg2 &= ~0x3;
	SetGpuReg(REG_OFFSET_BG0CNT, bg0);
	SetGpuReg(REG_OFFSET_BG2CNT, bg2);
	s->appealNumber++;
	if (s->appealNumber == HC_NUM_APPEALS)
		gTasks[taskId].func = Task_HC_EndAppeals;
	else
	{
		HC_SlideApplauseMeterIn(s);
		gTasks[taskId].func = Task_HC_StartNewRoundOfAppeals;
	}
}

//5 回終わったあと（原文 Task_EndAppeals〜Task_TryCommunicateFinalStandings）。結果発表・TV・リンクは範囲外＝幕を下ろして画面を閉じる
static void Task_HC_EndAppeals_Fade(u8 taskId);

static void Task_HC_DropCurtainAtAppealsEnd(u8 taskId)
{
	*(s16*) &gBattle_BG1_Y -= 7;
	if ((s16) gBattle_BG1_Y < 0)
		gBattle_BG1_Y = 0;
	if (gBattle_BG1_Y == 0)
	{
		gTasks[taskId].func = Task_HC_EndAppeals_Fade;
		gTasks[taskId].data[0] = 0;
	}
}

static void Task_HC_WaitForOutOfTimeMsg(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (!HC_RunTextPrinters())
	{
		HC_SetBgForCurtainDrop(s);
		gBattle_BG1_X = 0;
		gBattle_BG1_Y = DISPLAY_HEIGHT;
		ContestFr_PlaySE12WithPanning(HC_SE_CURTAIN_FALL, 0);
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].func = Task_HC_DropCurtainAtAppealsEnd;
	}
}

static void Task_HC_EndAppeals(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	gBattle_BG0_Y = 0;
	gBattle_BG2_Y = 0;
	HackContest_FinalizeScores(s); //S4: 最終点・順位を計算して Keep（固定 RAM 0x0203E800）へ（原文 contest.c:2666 CalculateFinalScores）
	HC_ClearGeneralTextWindow(s);
	StringExpandPlaceholders(gStringVar4, gHackContest_AllOutOfAppealTime);
	HC_StartTextPrinter(s, gStringVar4, TRUE);
	gTasks[taskId].data[2] = 0;
	gTasks[taskId].func = Task_HC_WaitForOutOfTimeMsg;
}

static void Task_HC_EndAppeals_Fade(u8 taskId)
{
	if (gTasks[taskId].data[0]++ >= 50)
	{
		gTasks[taskId].data[0] = 0;
		BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
		gTasks[taskId].data[0] = 1;
#ifdef HACK_DEBUG_MENU
		if (HACK_CONTEST_PTR != NULL)
			HACK_CONTEST_PTR->phase = 4; //S4b: 5 ラウンドの終わり＝画面を閉じたあと結果発表へ続ける（debug 版だけ。通常版は野外へ戻り台本が ShowContestResults を呼ぶ＝S5）
#endif
		gTasks[taskId].func = Task_HC_Idle;
	}
}

//---- S2c ここまで --------------------------------------------------------------------------------------------


static bool8 HC_SetupContestGraphics(struct HackContestState* s)
{
	u16 ALIGNED(4) tempPalette1[16];
	u16 ALIGNED(4) tempPalette2[16];

	switch (s->setupState)
	{
	case 0:
		gPaletteFade->bufferTransferDisabled = TRUE;
		ContestFr_RequestDma3Fill(0, (void*) VRAM, 0x8000, 1);
		ContestFr_RequestDma3Fill(0, (void*) VRAM + 0x8000, 0x8000, 1);
		ContestFr_RequestDma3Fill(0, (void*) VRAM + 0x10000, 0x8000, 1);
		break;
	case 1:
		LZDecompressVram((const u32*) gHackContestInterfaceGfx, (void*) VRAM);
		break;
	case 2:
		LZDecompressVram((const u32*) gHackContestAudienceGfx, (void*) BG_SCREEN_ADDR(4));
		CpuCopy32((void*) BG_SCREEN_ADDR(4), s->audienceGfx, 0x2000);
		break;
	case 3:
		CopyToBgTilemapBuffer(3, gHackContestAudienceTilemap, 0, 0);
		CopyBgTilemapBufferToVram(3);
		break;
	case 4:
		CopyToBgTilemapBuffer(2, gHackContestInterfaceTilemap, 0, 0);
		CopyBgTilemapBufferToVram(2);
		break;
	case 5:
		LoadCompressedPalette((const u32*) gHackContestInterfaceAudiencePal, 0, 0x200);
		CpuCopy32(&gPlttBufferUnfaded[16 * 8], tempPalette1, 32);
		CpuCopy32(&gPlttBufferUnfaded[16 * (5 + s->playerIndex)], tempPalette2, 32);
		CpuCopy32(tempPalette2, &gPlttBufferUnfaded[16 * 8], 32);
		CpuCopy32(tempPalette1, &gPlttBufferUnfaded[16 * (5 + s->playerIndex)], 32);
		CpuCopy32(gPlttBufferUnfaded, s->cachedWindowPalettes, sizeof(s->cachedWindowPalettes));
		HC_LoadContestPalettes();
		break;
	case 6:
		HC_DrawContestantWindows(s);
		HC_FillContestantWindowBgs(s);
		HC_SwapMoveDescAndContestTilemaps(s);
		s->judgeSpeechBubbleSpriteId = HC_CreateJudgeSpeechBubbleSprite();
			HC_CreateSliderHeartSprites(s);
			HC_CreateNextTurnSprites(s);
			HC_CreateApplauseMeterSprite(s);
			HC_CreateJudgeAttentionEyeTask(s);
			//（CreateUnusedBlendTask は原文でも何もしない）
			if (gBattleSpritesDataPtr == NULL)
			{
				//FR の技のアニメが読む（healthBoxesData・bankData 等）。vanilla の AllocateBattleSpritesData は ld に無いので自分で確保（余裕を持たせる）
				gBattleSpritesDataPtr = AllocZeroed(sizeof(struct BattleSpriteData));
				gBattleSpritesDataPtr->bankData = AllocZeroed(4 * 0x40);
				gBattleSpritesDataPtr->healthBoxesData = AllocZeroed(4 * 0x40);
				gBattleSpritesDataPtr->animationData = AllocZeroed(0x80);
				gBattleSpritesDataPtr->battleBars = AllocZeroed(4 * 0x40);
				s->battleSpritesAllocated = 1;
			}
		gBattlerPositions[0] = 0; //B_POSITION_PLAYER_LEFT
		gBattlerPositions[1] = 1; //B_POSITION_OPPONENT_LEFT
		gBattlerPositions[2] = 3; //B_POSITION_OPPONENT_RIGHT
		gBattlerPositions[3] = 2; //B_POSITION_PLAYER_RIGHT
		gBattleTypeFlags = 0;
		gBankAttacker = 2;
		gBankTarget = 3;
		s->judgeSpriteId = HC_CreateJudgeSprite();
		gBattlerSpriteIds[gBankAttacker] = s->judgeSpriteId;
			HC_CreateInvisibleBattleTargetSprite();
			//FR の IsBattlerSpriteVisible（battle_anim_mons.c:624）は「bankData.invisible かつ スプライトも invisible」で非表示。
			//  原文の IsBattlerSpriteVisible は IsContest のとき攻撃側以外を非表示にする。その代わりに、攻撃側（2）以外を非表示にして
			//  monbg（へんしん・おんがえしなど）が審査員の絵などを BG に写して画面を崩すのを防ぐ（S2c の実機で確認した崩れ）
			gBattlerSpriteIds[0] = gBattlerSpriteIds[3];
			gBattlerSpriteIds[1] = gBattlerSpriteIds[3];
			{
				s32 b;

				for (b = 0; b < 4; b++)
					gBattleSpritesDataPtr->bankData[b].invisible = (b != 2);
			}
CopyBgTilemapBufferToVram(3);
		CopyBgTilemapBufferToVram(2);
		CopyBgTilemapBufferToVram(1);
		ShowBg(3);
		ShowBg(2);
		ShowBg(0);
		ShowBg(1);
		break;
	default:
		s->setupState = 0;
		return TRUE;
	}

	s->setupState++;
	return FALSE;
}

static void HC_FreeAndLeave(void)
{
#ifdef HACK_DEBUG_MENU
	u8 toResults = HACK_CONTEST_PTR != NULL && HACK_CONTEST_PTR->phase == 4;
#endif
	ContestFr_gBattleAnimBgTileBuffer = NULL;
	ContestFr_gBattleAnimBgTilemapBuffer = NULL;
	FreeAllWindowBuffers();
	ContestFr_FreeMonSpritesGfx();
	if (HACK_CONTEST_PTR != NULL && HACK_CONTEST_PTR->battleSpritesAllocated && gBattleSpritesDataPtr != NULL)
	{
		Free(gBattleSpritesDataPtr->bankData);
		Free(gBattleSpritesDataPtr->healthBoxesData);
		Free(gBattleSpritesDataPtr->animationData);
		Free(gBattleSpritesDataPtr->battleBars);
		Free(gBattleSpritesDataPtr);
		gBattleSpritesDataPtr = NULL;
	}
	SetVBlankCallback(NULL);
	HackContest_Close(); //状態を Free して語を NULL に
#ifdef HACK_DEBUG_MENU
	if (toResults && !HACK_CONTEST_KEEP.fromScript) //S5c: 台本から入った回（startcontest）は結果発表を台本の showcontestresults に任せる（debug の入口だけ直接結果へ）
	{
		HACK_CONTEST_KEEP.resultsReturn = 0;
		SetMainCallback2(HackContest_ResultsStart);
		return;
	}
#endif
	if (HACK_CONTEST_KEEP.fromScript)
	{
		HACK_CONTEST_KEEP.fromScript = 0;
		SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic); //S5a: 台本の startcontest の続きへ（そのあと ShowContestResults）
		return;
	}
	SetMainCallback2(CB2_ReturnToField);
}

//原文 Task_RaiseCurtainAtStart（日本語版も同じ並び）の S2a 分: 待ち 60 コマ→SE→BG1 を上げる→待ち（拍手のメーター・出場者の枠の並べ直しは S2b 以降）
static void Task_HC_Idle(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	switch (gTasks[taskId].data[0])
	{
	case 0: //待ち（debug は B で出口へ）
		s->phase = 3;
		if (gMain.newKeys & B_BUTTON)
		{
			BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
			gTasks[taskId].data[0] = 1;
		}
		break;
	case 1:
		if (!gPaletteFade->active)
		{
			DestroyTask(taskId);
			HC_FreeAndLeave();
		}
		break;
	}
}

static void Task_HC_RaiseCurtain(u8 taskId)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		DestroyTask(taskId);
		return;
	}
	switch (gTasks[taskId].data[0])
	{
	case 0:
		if (gTasks[taskId].data[1]++ <= 60)
			break;
		gTasks[taskId].data[1] = 0;
		ContestFr_PlaySE12WithPanning(HC_SE_CURTAIN_RISE, 0);
		gTasks[taskId].data[0]++;
		break;
	case 1:
		*(s16*) &gBattle_BG1_Y += 7;
		if ((s16) gBattle_BG1_Y <= DISPLAY_HEIGHT)
			break;
		gTasks[taskId].data[0]++;
		break;
	case 2:
	{
		u16 bg0Cnt = GetGpuReg(REG_OFFSET_BG0CNT);
		u16 bg2Cnt = GetGpuReg(REG_OFFSET_BG2CNT);

		bg0Cnt &= ~0x3; //priority = 0
		bg2Cnt &= ~0x3;
			HC_UpdateContestantBoxOrder(s); //原文は幕が上がりきったあと（case 2）
			SetGpuReg(REG_OFFSET_BG0CNT, bg0Cnt);
			SetGpuReg(REG_OFFSET_BG2CNT, bg2Cnt);
			HC_SlideApplauseMeterIn(s);
			gTasks[taskId].data[0]++;
			break;
		}
		case 3:
			if (s->applauseMeterIsMoving)
				break;
			gTasks[taskId].data[0] = 0;
			gTasks[taskId].data[1] = 0;
			gTasks[taskId].func = Task_HC_DisplayAppealNumber;
			break;
	}
}

static void Task_HC_WaitFade(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		gTasks[taskId].data[0] = 0;
		gTasks[taskId].data[1] = 0;
		gTasks[taskId].func = Task_HC_RaiseCurtain;
	}
}

static void CB2_HC_Main(void)
{
	struct HackContestState* s = HACK_CONTEST_PTR;
	s32 i;

	AnimateSprites();
	RunTasks();
	BuildOamBuffer();
	UpdatePaletteFade();
	if (s == NULL)
		return;
	for (i = 0; i < 4; i++)
	{
		if ((s->bgCopyFlags >> i) & 1)
			CopyBgTilemapBufferToVram(i);
	}
	s->bgCopyFlags = 0;
}

static void VBlankCB_HC(void)
{
	SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
	SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
	SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
	SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
	SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
	SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);
	SetGpuReg(REG_OFFSET_BG3HOFS, gBattle_BG3_X);
	SetGpuReg(REG_OFFSET_BG3VOFS, gBattle_BG3_Y);
	SetGpuReg(REG_OFFSET_WIN0H, gBattle_WIN0H);
	SetGpuReg(REG_OFFSET_WIN0V, gBattle_WIN0V);
	SetGpuReg(REG_OFFSET_WIN1H, gBattle_WIN1H);
	SetGpuReg(REG_OFFSET_WIN1V, gBattle_WIN1V);
	ContestFr_TransferPlttBuffer();
	LoadOam();
	ProcessSpriteCopyRequests();
	ScanlineEffect_InitHBlankDmaTransfer();
}

//原文 CB2_StartContest（日本語版も同じ段）。Open 済み（HACK_CONTEST_PTR != NULL）で呼ぶ。出口は Task_HC_Idle の B（debug）→ HC_FreeAndLeave
void HackContest_CB2_Start(void)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
	{
		SetMainCallback2(CB2_ReturnToField);
		return;
	}
	switch (gMain.state)
	{
	case 0:
		s->bgCopyFlags = 0;
		HC_InitTurnOrder(s); //一次審査の点→順番（原文 InitContestResources）
		ContestFr_gBattleAnimBgTileBuffer = s->animBg;
		ContestFr_gBattleAnimBgTilemapBuffer = s->bgTilemap[1];
		ContestFr_AllocateMonSpritesGfx();
		Free(gMonSpritesGfxPtr->firstDecompressed);
		gMonSpritesGfxPtr->firstDecompressed = AllocZeroed(0x4000);
		gMonSpritesGfxPtr->sprites[0] = gMonSpritesGfxPtr->firstDecompressed;
		gMonSpritesGfxPtr->sprites[1] = (u8*) gMonSpritesGfxPtr->firstDecompressed + 0x2000;
		SetVBlankCallback(NULL);
		HC_InitBgs(s);
		HC_InitWindows(s);
		HC_SetupGpuRegs();
		ScanlineEffect_Clear();
		ResetPaletteFade();
		gPaletteFade->bufferTransferDisabled = TRUE;
		ResetSpriteData();
		ResetTasks();
		FreeAllSpritePalettes();
		(*(u8*) 0x03003E98 /* gReservedSpritePaletteCount（hoenn_dome.c と同じ） */) = 4;
		ContestFr_ClearBattleMonForms();
		s->phase = 1;
		s->setupState = 0;
		gMain.state++;
		break;
	case 1:
		gMain.state++;
		break;
	case 2:
		if (HC_SetupContestGraphics(s))
		{
			s->setupState = 0;
			gMain.state++;
		}
		break;
	case 3:
		HC_SetBgForCurtainDrop(s);
		gBattle_BG1_X = 0;
		gBattle_BG1_Y = 0;
		BeginFastPaletteFade(2);
		gPaletteFade->bufferTransferDisabled = FALSE;
		SetVBlankCallback(VBlankCB_HC);
		s->mainTaskId = CreateTask(Task_HC_WaitFade, 10);
		s->phase = 2;
		SetMainCallback2(CB2_HC_Main);
		break;
	}
}

//S5a: 台本の `startcontest`（原文 StartContest→CB2_StartContest）。受付で決めた Keep（出場者 3 人の番号・手持ちの番号・部門・ランク）から状態を作って会場の画面へ。
//  プレイヤーは 4 番目（原文 TryPutPlayerLast）。画面を暗くしてから入る。終わると台本の続き（HC_FreeAndLeave の fromScript）。
void __attribute__((long_call)) LockPlayerFieldControls(void);
static void Task_HC_StartFromScript(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		DestroyTask(taskId);
		gMain.state = 0;
		SetMainCallback2(HackContest_CB2_Start);
	}
}

void StartHackContest(void)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;
	struct HackContestState* s;
	s32 i;

	HackContest_Reset();
	if (k->magic != HACK_CONTEST_KEEP_MAGIC || !HackContest_Open())
		return;
	s = HACK_CONTEST_PTR;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		s->turnOrder[i] = i;
		if (i < 3)
			HackContest_BuildMonFromOpponent(&s->mons[i], k->opp[i]);
		else
			HackContest_BuildMonFromParty(&s->mons[i], k->partyIndex);
	}
	s->playerIndex = 3;
	s->category = k->category;
	k->fromScript = 1;
	LockPlayerFieldControls();
	CreateTask(Task_HC_StartFromScript, 10);
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
}

#ifdef HACK_DEBUG_MENU
//debug 入口: gMain.callback2 にこの関数を書くと 1 回だけ実行（Open→出場者を仮に作る→画面へ）。
//  Var8004 が 0 でなければ、出場者 0 の種族をその値にする（種族 1000 超の絵の確認用）。
void HackContest_DebugEnter(void)
{
	struct HackContestState* s;
	s32 i;
	u8 count = gPlayerPartyCount ? gPlayerPartyCount : 1;

	HackContest_Reset();
	if (!HackContest_Open())
	{
		SetMainCallback2(CB2_ReturnToField);
		return;
	}
	s = HACK_CONTEST_PTR;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct Pokemon* mon = &gPlayerParty[i % count];

		s->turnOrder[i] = i;
		s->mons[i].species = GetMonData(mon, MON_DATA_SPECIES, NULL);
		s->mons[i].otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
		s->mons[i].personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
		GetMonData(mon, MON_DATA_NICKNAME, s->mons[i].nickname);
		s->mons[i].nickname[POKEMON_NAME_LENGTH] = 0xFF;
		StringCopy(s->mons[i].trainerName, gSaveBlock2->playerName);
		s->mons[i].moves[0] = GetMonData(mon, MON_DATA_MOVE1, NULL);
		s->mons[i].moves[1] = GetMonData(mon, MON_DATA_MOVE2, NULL);
		s->mons[i].moves[2] = GetMonData(mon, MON_DATA_MOVE3, NULL);
		s->mons[i].moves[3] = GetMonData(mon, MON_DATA_MOVE4, NULL);
		//仮: コンディション 6 値（本物は hack_condition.c の読み口＝task/FEAT-pokeblock 69f718e8a）。順番の違いが見えるよう人ごとに変える
		s->mons[i].stats[0] = 40 + i * 10;
		s->mons[i].stats[1] = 90 - i * 15;
		s->mons[i].stats[2] = 30 + i * 5;
		s->mons[i].stats[3] = 50;
		s->mons[i].stats[4] = 20 + i * 20;
		s->mons[i].sheen = 10 + i * 30;
		s->mons[i].aiFlags = gHackContestOpponents[i].aiFlags; //S5a: 本物の aiFlags（日本語版 ROM の gContestOpponents・CONTEST_AI_SET_x）。debug の人はパーティの仮だが AI の旗は表の 0〜3 番
	}
	if (Var8004 != 0)
		s->mons[0].species = Var8004;
	if (Var8006 != 0) //へんしん・ものまねの確認用: 出場者 1〜3 の種族を Var8006, +1, +2 にする
	{
		for (i = 1; i < HACK_CONTEST_CONTESTANTS; i++)
			s->mons[i].species = Var8006 + i - 1;
	}
	if (Var8008 != 0) //効果の確認用（S3）: 全員の 4 つの技を Var8008, +1, +2, +3 にする（debug 版の効果＝技の番号 % 48）
	{
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			s32 k;

			for (k = 0; k < 4; k++)
				s->mons[i].moves[k] = Var8008 + k;
		}
	}
	if (Var8005 != 0) //技のアニメの確認用: 全員の 4 つの技を Var8005 にそろえる
	{
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			s32 k;

			for (k = 0; k < 4; k++)
				s->mons[i].moves[k] = Var8005;
		}
	}
	s->playerIndex = 0;
	gMain.state = 0;
	SetMainCallback2(HackContest_CB2_Start);
}
#endif
