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
#include "../include/random.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/pokedex.h"
#include "../include/pokemon.h"
#include "../include/pokemon_icon.h"
#include "../include/new/Vanilla_functions.h"
#include "hack_contest.h"
#include "hack_contest_addr.h"
#include "hack_contest_text.h"

/*
 * コンテスト移植 S4b（FEAT-contest・patches/0985）: 結果発表の画面（原文 参考/pokeemerald/src/contest_util.c 49〜1955 の結果の部分）。
 * 日本語版エメラルドの contest_link_80F57C4.s（0x080F5FBC〜）と 1 つずつ突き合わせた（突き合わせ表は docs/research/2026-11-09_contest-s1-foundation.md の S4b 節）。
 * 日本語版が原文と違う所: 窓の表（出場者 4 窓 幅 5・トレーナー名 4 窓 幅 6・x=12）・題の帯の大きさ（ランク 8x2・部門 5x2・「けっか」5x2）・
 *   文字窓は窓の文字描画でなく 1 文字 8x16 を手で並べる（RenderTextFont9）→ FR では窓に描いた文字をスプライトのタイルへ並べ替える（同じ配置・同じ式）・BGM は別の曲（FR に無い＝仮で曲は変えない）。
 * 通信・無線・インタビュー・TV・ファン・GAME_STAT は書かない（S1 で範囲外）。結果の元データは gContest* ではなく Keep（hack_contest.h の HackContestKeep）。
 * 状態は全部ヒープ（AllocZeroed・出るとき Free）と Keep の中の語 screen（static の書き換えは使えない・6.13）。
 */

enum
{
	SLIDING_TEXT_OFFSCREEN,
	SLIDING_TEXT_ENTERING,
	SLIDING_TEXT_ARRIVED,
	SLIDING_TEXT_EXITING,
};

enum
{
	SLIDING_MON_ENTERED = 1,
	SLIDING_MON_EXITED,
};

#define GET_CONTEST_WINNER_ID(i) { for ((i) = 0; (i) < HACK_CONTEST_CONTESTANTS && KEEP->finalStandings[(i)] != 0; (i)++); }

#define KEEP (&HACK_CONTEST_KEEP)
#define RES (HACK_CONTEST_KEEP.screen)

#define TAG_TEXT_WINDOW_BASE 3009 /* 4 枚のスプライト＝タグ 3009〜3012（原文はリンク用の 4 枚も持つ＝書かない） */
#define TAG_CONFETTI 3017

#define NUM_BAR_SEGMENTS 11
#define BAR_SEGMENT_LENGTH 8
#define MAX_BAR_LENGTH (NUM_BAR_SEGMENTS * BAR_SEGMENT_LENGTH)

#define TEXT_BOX_X (DISPLAY_WIDTH + 32)
#define TEXT_BOX_Y (DISPLAY_HEIGHT - 16)

//音・曲（S9: 本物。EM と FR の SE は同じ音だが番号が違う＝FR の番号。tools/rom/verify_contest_music.py が EM と突き合わせる）
#define HC_SE_PIN 21            /* 原文 SE_PIN（ハートが増える・EM 21 と FR 21 はトラック同一） */
#define HC_SE_BOO 22            /* 原文 SE_BOO（二次でマイナス・EM 22 と FR 22 はトラック同一） */
#define HC_SE_PLACE 24          /* 原文 SE_CONTEST_PLACE（順位の数字が出る・FR 24 も同名＝編曲が EM と少し違う〈FR 4 トラック・EM 3 トラック〉・ユーザー決定 2026-11-10） */
#define HC_MUS_CONTEST_RESULTS 551 /* 原文 MUS_CONTEST_RESULTS（EM 446）。曲の表の末尾に移植（S9・build_bgm.py の tail） */

#define HC_RESULTS_TILE_BASE 1000 /* 文字窓の描画用の窓の baseBlock（BG の絵 0〜0x100・アイコン 0x200〜・窓 770〜910・地図 1536〜 のどれとも重ならない） */

struct HackContestResultsInternal
{
	u8 slidingTextBoxSpriteId;
	u8 showResultsTaskId;
	u8 highlightWinnerTaskId;
	u8 slidingTextBoxState;
	u8 numStandingsPrinted;
	u8 winnerMonSlidingState;
	u8 confettiCount;
	u8 winnerMonSpriteId;
	u8 destroyConfetti;
	u8 pointsFlashing;
	u8 numBarsUpdating;
	u8 returnToScript;
	s16 barLength[HACK_CONTEST_CONTESTANTS];
};

struct HackContestMonResults
{
	int relativePreliminaryPoints;
	int relativeRound2Points;
	u32 barLengthPreliminary;
	u32 barLengthRound2;
	u8 lostPoints;
	u8 numStars;
	u8 numHearts;
	u8 pad;
};

struct HackContestResults
{
	struct HackContestResultsInternal data;
	struct HackContestMonResults monResults[HACK_CONTEST_CONTESTANTS];
	u16 tilemapBuffers[4][0x400]; /* BG 0〜3 の tilemap（0x800 バイト） */
	u8 scratch[8]; /* （予備） */
};

#define D (RES->data)

u16 __attribute__((long_call)) LoadCompressedSpritePalette(const struct CompressedSpritePalette* src);
void __attribute__((long_call)) LockPlayerFieldControls(void);
void __attribute__((long_call)) CopyToBgTilemapBufferRect_ChangePalette(u8 bg, const void* src, u8 destX, u8 destY, u8 rectWidth, u8 rectHeight, u8 palette);
s16 __attribute__((long_call)) Sine(s16 index, s16 amplitude);
#define gMonFrontPicTable ((const struct CompressedSpriteSheet*) *((u32*) 0x8000128))

static void Task_ShowContestResults(u8 taskId);
static void Task_AnnouncePreliminaryResults(u8 taskId);
static void Task_ShowPreliminaryResults(u8 taskId);
static void Task_AnnounceRound2Results(u8 taskId);
static void Task_ShowRound2Results(u8 taskId);
static void Task_AnnounceWinner(u8 taskId);
static void Task_ShowWinnerMonBanner(u8 taskId);
static void Task_SetSeenWinnerMon(u8 taskId);
static void Task_EndShowContestResults(u8 taskId);
static void Task_SlideContestResultsBg(u8 taskId);
static void Task_FlashStarsAndHearts(u8 taskId);
static void Task_DrawFinalStandingNumber(u8 taskId);
static void Task_StartHighlightWinnersBox(u8 taskId);
static void Task_HighlightWinnersBox(u8 taskId);
static void Task_CreateConfetti(u8 taskId);
static void Task_BounceMonIconInBox(u8 taskId);
static void Task_UpdateContestResultBar(u8 taskId);
static void SpriteCB_TextBoxSlideIn(struct Sprite* sprite);
static void SpriteCB_EndTextBoxSlideIn(struct Sprite* sprite);
static void SpriteCB_TextBoxSlideOut(struct Sprite* sprite);
static void SpriteCB_WinnerMonSlideIn(struct Sprite* sprite);
static void SpriteCB_WinnerMonSlideOut(struct Sprite* sprite);
static void SpriteCB_Confetti(struct Sprite* sprite);

static const u8 sBlankGfx[0x400] __attribute__((aligned(4))) = { 0 };
//文字窓のスプライトのパレット（日本語版 0x0856927C・白地に濃い灰の文字）
extern const u8 gHackContestResultsTextWindowSpritePal[];

static const struct OamData sOamData_ResultsTextWindow =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = 0,
	.shape = 1, .x = 0, .matrixNum = 0, .size = 3, .tileNum = 0, .priority = 3, .paletteNum = 2,
};

static const struct SpriteTemplate sSpriteTemplate_ResultsTextWindow =
{
	.tileTag = TAG_TEXT_WINDOW_BASE, .paletteTag = TAG_TEXT_WINDOW_BASE, .oam = &sOamData_ResultsTextWindow,
	.anims = (const union AnimCmd* const*) 0x081F11E4 /* gDummySpriteAnimTable */, .images = NULL,
	.affineAnims = (const union AffineAnimCmd* const*) 0x081F11F0 /* gDummySpriteAffineAnimTable */,
	.callback = SpriteCallbackDummy,
};

static const struct SpriteSheet sSpriteSheets_ResultsTextWindow[4] =
{
	{ sBlankGfx, 0x400, TAG_TEXT_WINDOW_BASE + 0 }, { sBlankGfx, 0x400, TAG_TEXT_WINDOW_BASE + 1 },
	{ sBlankGfx, 0x400, TAG_TEXT_WINDOW_BASE + 2 }, { sBlankGfx, 0x400, TAG_TEXT_WINDOW_BASE + 3 },
};

static const struct OamData sOamData_Confetti =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = 0,
	.shape = 0, .x = 0, .matrixNum = 0, .size = 0, .tileNum = 0, .priority = 0, .paletteNum = 0,
};

static const struct SpriteTemplate sSpriteTemplate_Confetti =
{
	.tileTag = TAG_CONFETTI, .paletteTag = TAG_CONFETTI, .oam = &sOamData_Confetti,
	.anims = (const union AnimCmd* const*) 0x081F11E4, .images = NULL,
	.affineAnims = (const union AffineAnimCmd* const*) 0x081F11F0,
	.callback = SpriteCB_Confetti,
};

static const struct CompressedSpriteSheet sSpriteSheet_Confetti = { gHackContestConfettiGfx, 0x220, TAG_CONFETTI };
static const struct CompressedSpritePalette sSpritePalette_Confetti = { gHackContestConfettiPal, TAG_CONFETTI };

//日本語版 0x0856944C（原文と同じ）
static const struct BgTemplate sBgTemplates[] =
{
	{ .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0 },
	{ .bg = 1, .charBaseIndex = 0, .mapBaseIndex = 24, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0 },
	{ .bg = 2, .charBaseIndex = 0, .mapBaseIndex = 28, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0 },
	{ .bg = 3, .charBaseIndex = 0, .mapBaseIndex = 26, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0 },
};

//日本語版 0x0856945C（8 窓＝出場者 0〜3 のニックネーム・4〜7 のトレーナー名・原文は 4 窓で幅 12）
static const struct WindowTemplate sWindowTemplates[] =
{
	{ 1, 7, 4, 5, 2, 15, 770 }, { 1, 7, 7, 5, 2, 15, 790 }, { 1, 7, 10, 5, 2, 15, 810 }, { 1, 7, 13, 5, 2, 15, 830 },
	{ 1, 12, 4, 6, 2, 15, 850 }, { 1, 12, 7, 6, 2, 15, 866 }, { 1, 12, 10, 6, 2, 15, 882 }, { 1, 12, 13, 6, 2, 15, 898 },
	DUMMY_WIN_TEMPLATE
};

//---- 点の計算（Keep から） ----------------------------------------------------------------------------------

//一次の星の数（原文 GetNumPreliminaryPoints・日本語版 0x080F7BA0）
static u8 GetNumPreliminaryPoints(u8 monIndex, bool8 capPoints)
{
	u32 condition = KEEP->round1Points[monIndex] << 16;
	u32 numStars = condition / 0x3F;

	if (numStars & 0xFFFF)
		numStars += 0x10000;
	numStars >>= 16;
	if (numStars == 0 && condition)
		numStars = 1;
	if (capPoints && numStars > 10)
		numStars = 10;
	return numStars;
}

//二次のハートの数（負＝減った・原文 GetNumRound2Points・日本語版 0x080F7BF4）
static s8 GetNumRound2Points(u8 monIndex, bool8 capPoints)
{
	u32 r4, numHearts;
	s16 results;
	s8 points;

	results = KEEP->round2Points[monIndex];
	if (results < 0)
		r4 = -results << 16;
	else
		r4 = results << 16;
	numHearts = r4 / 80;
	if (numHearts & 0xFFFF)
		numHearts += 0x10000;
	numHearts >>= 16;
	if (numHearts == 0 && r4 != 0)
		numHearts = 1;
	if (capPoints && numHearts > 10)
		numHearts = 10;
	if (KEEP->round2Points[monIndex] < 0)
		points = -numHearts;
	else
		points = numHearts;
	return points;
}

//原文 CalculateContestantsResultData
static void CalculateContestantsResultData(void)
{
	int i, relativePoints;
	u32 barLength;
	s16 highestPoints;
	s8 round2Points;

	highestPoints = KEEP->totalPoints[0];
	for (i = 1; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		if (highestPoints < KEEP->totalPoints[i])
			highestPoints = KEEP->totalPoints[i];
	}
	if (highestPoints < 0)
	{
		highestPoints = KEEP->totalPoints[0];
		for (i = 1; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			if (highestPoints > KEEP->totalPoints[i])
				highestPoints = KEEP->totalPoints[i];
		}
	}
	if (highestPoints == 0) //（原文は 0 で割る＝全員 0 点は起きない・FR で固まらないように 1 にする。通常の点では通らない）
		highestPoints = 1;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct HackContestMonResults* r = &RES->monResults[i];

		relativePoints = (KEEP->round1Points[i] * 1000) / abs(highestPoints);
		if (relativePoints % 10 > 4)
			relativePoints += 10;
		r->relativePreliminaryPoints = relativePoints / 10;

		relativePoints = (abs(KEEP->round2Points[i]) * 1000) / abs(highestPoints);
		if (relativePoints % 10 > 4)
			relativePoints += 10;
		r->relativeRound2Points = relativePoints / 10;

		if (KEEP->round2Points[i] < 0)
			r->lostPoints = TRUE;

		barLength = (r->relativePreliminaryPoints * 0x5800) / 100;
		if ((barLength & 0xFF) > 0x7F)
			barLength += 0x100;
		r->barLengthPreliminary = barLength >> 8;

		barLength = (r->relativeRound2Points * 0x5800) / 100;
		if ((barLength & 0xFF) > 0x7F)
			barLength += 0x100;
		r->barLengthRound2 = barLength >> 8;

		r->numStars = GetNumPreliminaryPoints(i, TRUE);
		round2Points = GetNumRound2Points(i, TRUE);
		r->numHearts = abs(round2Points);

		if (KEEP->finalStandings[i])
		{
			s16 barLengthPreliminary = r->barLengthPreliminary;
			s16 barLengthRound2 = r->barLengthRound2;

			if (r->lostPoints)
				barLengthRound2 *= -1;
			if (barLengthPreliminary + barLengthRound2 == MAX_BAR_LENGTH)
			{
				if (barLengthRound2 > 0)
					r->barLengthRound2--;
				else if (barLengthPreliminary > 0)
					r->barLengthPreliminary--;
			}
		}
	}
}

//---- 画面の作成 --------------------------------------------------------------------------------------------

static void InitContestResultsDisplay(void)
{
	s32 i;

	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP);
	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sBgTemplates, 4);
	for (i = 0; i < 4; i++)
		SetBgTilemapBuffer(i, RES->tilemapBuffers[i]);
	InitWindows(sWindowTemplates);
	DeactivateAllTextPrinters();
	SetGpuReg(REG_OFFSET_MOSAIC, 0);
	SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR);
	SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_CLR | WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
	SetGpuReg(REG_OFFSET_WIN0H, 0);
	SetGpuReg(REG_OFFSET_WIN0V, 0);
	SetGpuReg(REG_OFFSET_WIN1H, 0);
	SetGpuReg(REG_OFFSET_WIN1V, 0);
	SetGpuReg(REG_OFFSET_BLDCNT, 0);
	SetGpuReg(REG_OFFSET_BLDALPHA, 0);
	SetGpuReg(REG_OFFSET_BLDY, 0);
	SetGpuReg(REG_OFFSET_BG0HOFS, 0);
	SetGpuReg(REG_OFFSET_BG0VOFS, 0);
	SetGpuReg(REG_OFFSET_BG1HOFS, 0);
	SetGpuReg(REG_OFFSET_BG1VOFS, 0);
	SetGpuReg(REG_OFFSET_BG2HOFS, 0);
	SetGpuReg(REG_OFFSET_BG2VOFS, 0);
	SetGpuReg(REG_OFFSET_BG3HOFS, 0);
	SetGpuReg(REG_OFFSET_BG3VOFS, 0);
	SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJWIN_ON);
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

//題の帯（日本語版 0x080F7A3C・ランク 8x2 を (5,1)・部門 5x2 を x=13（通信なら 14・かわいさ／かしこさは −1）・「けっか」5x2・最後に 4 行 32 列のパレット替え）
static void LoadContestResultsTitleBarTilemaps(void)
{
	u8 palette;
	s32 x, y;
	const u8* rank;
	const u8* cat;

	x = 5;
	y = 1;
	switch (KEEP->rank)
	{
	case 0: rank = gHackContestResultsTitleNormal; break;
	case 1: rank = gHackContestResultsTitleSuper; break;
	case 2: rank = gHackContestResultsTitleHyper; break;
	default: rank = gHackContestResultsTitleMaster; break;
	}
	ContestFr_CopyToBgTilemapBufferRect(2, rank, 5, 1, 8, 2);
	x = 13;
	switch (KEEP->category)
	{
	case 0: palette = 0; cat = gHackContestResultsTitleCool; break;
	case 1: palette = 1; cat = gHackContestResultsTitleBeauty; break;
	case 2: palette = 2; cat = gHackContestResultsTitleCute; x -= 1; break;
	case 3: palette = 3; cat = gHackContestResultsTitleSmart; x -= 1; break;
	default: palette = 4; cat = gHackContestResultsTitleTough; break;
	}
	ContestFr_CopyToBgTilemapBufferRect(2, cat, x, y, 5, 2);
	x += 5;
	ContestFr_CopyToBgTilemapBufferRect(2, gHackContestResultsTitle, x, y, 5, 2);
	CopyToBgTilemapBufferRect_ChangePalette(2, RES->tilemapBuffers[2], 0, 0, 32, 4, palette);
}

//日本語版 0x080F6114
static void LoadContestResultsBgGfx(void)
{
	s32 i, j;
	s8 numStars, round2Points;
	u16 tile1, tile2;

	LZDecompressVram((const u32*) gHackContestResultsGfx, (void*) BG_CHAR_ADDR(0));
	CopyToBgTilemapBuffer(3, gHackContestResultsBgTilemap, 0, 0);
	CopyToBgTilemapBuffer(2, gHackContestResultsInterfaceTilemap, 0, 0);
	CopyToBgTilemapBuffer(0, gHackContestResultsWinnerBannerTilemap, 0, 0);
	LoadContestResultsTitleBarTilemaps();
	LoadCompressedPalette((const u32*) gHackContestResultsPal, 0, 0x200);
	LoadPalette(gHackContestResultsTextWindowPal, 16 * 15, 32);

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		numStars = GetNumPreliminaryPoints(i, TRUE);
		round2Points = GetNumRound2Points(i, TRUE);
		for (j = 0; j < 10; j++)
		{
			tile1 = 0x60B2;
			if (j < numStars)
				tile1 += 2;
			if (j < abs(round2Points))
			{
				tile2 = 0x60A4;
				if (round2Points < 0)
					tile2 += 2;
			}
			else
			{
				tile2 = 0x60A2;
			}
			FillBgTilemapBufferRect_Palette0(1, tile1, j + 19, i * 3 + 5, 1, 1);
			FillBgTilemapBufferRect_Palette0(1, tile2, j + 19, i * 3 + 6, 1, 1);
		}
	}
	CopyBgTilemapBufferToVram(0);
	CopyBgTilemapBufferToVram(1);
	CopyBgTilemapBufferToVram(2);
	CopyBgTilemapBufferToVram(3);
	ShowBg(0);
	ShowBg(1);
	ShowBg(2);
	ShowBg(3);
}

//日本語版 0x080F8760（font 1・x 0・y 2・fg 1 bg 0 shadow 8・窓の番号の窓に出す）
static void AddContestTextPrinter(u8 windowId, const u8* str)
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
	t.fgColor = 1;
	t.bgColor = 0;
	t.shadowColor = 8;
	ContestFr_AddTextPrinter(&t, 0, NULL);
	PutWindowTilemap(windowId);
}

//日本語版 0x080F626C（プレイヤー自身のニックネームとトレーナー名は先に ColorDarkGray）
static void LoadContestMonName(u8 monIndex)
{
	const struct HackContestKeepMon* mon = &KEEP->mons[monIndex];
	u8* str = gDisplayedStringBattle;

	if (monIndex == KEEP->playerIndex)
		str = StringCopy(gDisplayedStringBattle, gHackContest_ColorDarkGray);
	StringCopy(str, mon->nickname);
	AddContestTextPrinter(monIndex, gDisplayedStringBattle);

	str = gDisplayedStringBattle;
	if (monIndex == KEEP->playerIndex)
		str = StringCopy(gDisplayedStringBattle, gHackContest_ColorDarkGray);
	str = StringCopy(str, gHackContest_Slash);
	StringAppend(str, mon->trainerName);
	AddContestTextPrinter(monIndex + 4, gDisplayedStringBattle);
}

static void LoadAllContestMonNames(void)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		LoadContestMonName(i);
	CopyBgTilemapBufferToVram(1);
}

//日本語版 0x080F71DC（枠の中のアイコン・プレイヤー自身は 2 コマ目から）
static void LoadContestMonIcon(u16 species, u8 monIndex, u8 srcOffset, u8 useDmaNow, u32 personality)
{
	const u8* iconPtr;
	u16 frameNum = (monIndex == KEEP->playerIndex) ? 1 : 0;

	iconPtr = ContestFr_GetMonIconPtr(species, personality, frameNum);
	iconPtr += srcOffset * 0x200 + 0x80;
	ContestFr_RequestDma3Copy(iconPtr, (void*) BG_CHAR_ADDR(1) + monIndex * 0x200, 0x180, 1);
	if (useDmaNow)
		ContestFr_WriteSequenceToBgTilemapBuffer(1, (monIndex * 0x10 + 0x200) | ((monIndex + 10) << 12), 3, monIndex * 3 + 4, 4, 3, 17, 1);
}

static void LoadAllContestMonIcons(u8 srcOffset, bool8 useDmaNow)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		LoadContestMonIcon(KEEP->mons[i].species, i, srcOffset, useDmaNow, KEEP->mons[i].personality);
}

//日本語版 0x080F72C0（アイコンのパレットは BG パレット 10〜13）
static void LoadAllContestMonIconPalettes(void)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		LoadPalette(GetValidMonIconPalettePtr(GetIconSpecies(KEEP->mons[i].species, 0)), 16 * (10 + i), 32);
}

//---- 文字窓（スプライト 4 枚・日本語版 0x080F739C／0x080F760C／0x080F7708〜） ----------------------------------

//文字を窓に描き、その 8x16 の升をスプライトのタイル（4 枚×0x400）へ並べる。戻り値は箱の x（日本語版 0x080F76E8＝0x78 − 文字数×4 − 8）
static s32 DrawResultsTextWindow(const u8* text)
{
	struct WindowTemplate tpl;
	struct TextPrinterTemplate t;
	u8 windowId;
	s32 tileWidth, i;
	u8* spriteTilePtrs[4];
	const u8* src = gHackContestResultsTextWindowGfx;
	const u8* windowTiles;
	struct Sprite* sprite;
	u8* dst;

	tileWidth = (GetStringWidth(1, text, 0) + 7) / 8;
	if (tileWidth > 30)
		tileWidth = 30;

	memset(&tpl, 0, sizeof(tpl));
	tpl.bg = 0;
	tpl.width = 30;
	tpl.height = 3; //上 2 段＋下の枠の最初の 2 行ぶん（日本語版の 16 行の升が 3 つのタイルにまたがる）
	tpl.baseBlock = HC_RESULTS_TILE_BASE;
	windowId = AddWindow(&tpl);
	FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
	t.currentChar = text;
	t.windowId = windowId;
	t.fontId = 1;
	t.x = 0;
	t.y = 2;
	t.currentX = 0;
	t.currentY = 2;
	t.letterSpacing = 0;
	t.lineSpacing = 0;
	t.unk = 0;
	t.fgColor = 0xF;
	t.bgColor = 1;
	t.shadowColor = 0xE;
	ContestFr_AddTextPrinter(&t, 0, NULL);
	windowTiles = (const u8*) GetWindowAttribute(windowId, WINDOW_TILE_DATA);

	sprite = &gSprites[D.slidingTextBoxSpriteId];
	spriteTilePtrs[0] = (u8*) (sprite->oam.tileNum * 32 + OBJ_VRAM0);
	for (i = 1; i < 4; i++)
		spriteTilePtrs[i] = (u8*) (gSprites[sprite->data[i - 1]].oam.tileNum * 32 + OBJ_VRAM0);
	for (i = 0; i < 4; i++)
		CpuFill32(0, spriteTilePtrs[i], 0x400);

	dst = spriteTilePtrs[0];
	CpuCopy32(src, dst, 0x20);
	CpuCopy32(src + 128, dst + 0x100, 0x20);
	CpuCopy32(src + 128, dst + 0x200, 0x20);
	CpuCopy32(src + 64, dst + 0x300, 0x20);
	for (i = 0; i < tileWidth; i++)
	{
		dst = &spriteTilePtrs[(i + 1) / 8][((i + 1) % 8) * 32];
		CpuCopy32(src + 192, dst, 0x20);
		CpuCopy32(windowTiles + i * 0x20, dst + 0x100, 0x20);
		CpuCopy32(windowTiles + 30 * 0x20 + i * 0x20, dst + 0x200, 0x20);
		CpuCopy32(src + 224, dst + 0x300, 0x20);
		CpuCopy32(windowTiles + 60 * 0x20 + i * 0x20, dst + 0x300, 8); //升の下 2 行（日本語版 0x080F7564 の 8 バイト）
	}
	dst = &spriteTilePtrs[(i + 1) / 8][((i + 1) % 8) * 32];
	CpuCopy32(src + 32, dst, 0x20);
	CpuCopy32(src + 160, dst + 0x100, 0x20);
	CpuCopy32(src + 160, dst + 0x200, 0x20);
	CpuCopy32(src + 96, dst + 0x300, 0x20);
	RemoveWindow(windowId);
	return (DISPLAY_WIDTH - (tileWidth + 2) * 8) / 2;
}

//日本語版 0x080F760C（スプライト 4 枚を画面の右外に作る・左端のスプライトが他の 3 枚の番号を持つ）
static void CreateResultsTextWindowSprites(void)
{
	s32 i;
	struct SpriteTemplate tpl;
	u8 spriteIds[4];

	tpl = sSpriteTemplate_ResultsTextWindow;
	for (i = 0; i < 4; i++)
		LoadSpriteSheet(&sSpriteSheets_ResultsTextWindow[i]);
	{
		struct SpritePalette pal;

		pal.data = (const u16*) gHackContestResultsTextWindowSpritePal;
		pal.tag = TAG_TEXT_WINDOW_BASE;
		LoadSpritePalette(&pal);
	}
	for (i = 0; i < 4; i++)
	{
		spriteIds[i] = CreateSprite(&tpl, TEXT_BOX_X, TEXT_BOX_Y, 10);
		tpl.tileTag++;
	}
	gSprites[spriteIds[0]].data[0] = spriteIds[1];
	gSprites[spriteIds[0]].data[1] = spriteIds[2];
	gSprites[spriteIds[0]].data[2] = spriteIds[3];
	D.slidingTextBoxSpriteId = spriteIds[0];
	D.slidingTextBoxState = SLIDING_TEXT_OFFSCREEN;
}

#define sTargetX data[4]
#define sSlideOutTimer data[5]
#define sSlideIncrement data[6]
#define sDistance data[7]

static void StartTextBoxSlideOut(u16 slideIncrement);

//slideOutTimer が -1 なら自動では出ていかない
static void StartTextBoxSlideIn(s16 x, u16 y, u16 slideOutTimer, u16 slideIncrement)
{
	struct Sprite* sprite = &gSprites[D.slidingTextBoxSpriteId];

	sprite->pos1.x = TEXT_BOX_X;
	sprite->pos1.y = y;
	sprite->pos2.x = 0;
	sprite->pos2.y = 0;
	sprite->sTargetX = x + 32;
	sprite->sSlideOutTimer = slideOutTimer;
	sprite->sSlideIncrement = slideIncrement;
	sprite->sDistance = 0;
	sprite->callback = SpriteCB_TextBoxSlideIn;
	D.slidingTextBoxState = SLIDING_TEXT_ENTERING;
}

static void StartTextBoxSlideOut(u16 slideIncrement)
{
	struct Sprite* sprite = &gSprites[D.slidingTextBoxSpriteId];

	sprite->pos1.x += sprite->pos2.x;
	sprite->pos1.y += sprite->pos2.y;
	sprite->pos2.y = 0;
	sprite->pos2.x = 0;
	sprite->sSlideIncrement = slideIncrement;
	sprite->sDistance = 0;
	sprite->callback = SpriteCB_TextBoxSlideOut;
	D.slidingTextBoxState = SLIDING_TEXT_EXITING;
}

static void EndTextBoxSlideOut(struct Sprite* sprite)
{
	sprite->pos1.x = TEXT_BOX_X;
	sprite->pos1.y = TEXT_BOX_Y;
	sprite->pos2.y = 0;
	sprite->pos2.x = 0;
	sprite->callback = SpriteCallbackDummy;
	D.slidingTextBoxState = SLIDING_TEXT_OFFSCREEN;
}

static void SpriteCB_TextBoxSlideIn(struct Sprite* sprite)
{
	s32 i;
	s16 delta = sprite->sDistance + sprite->sSlideIncrement;

	sprite->pos1.x -= delta >> 8;
	sprite->sDistance += sprite->sSlideIncrement;
	sprite->sDistance &= 0xFF;
	if (sprite->pos1.x < sprite->sTargetX)
		sprite->pos1.x = sprite->sTargetX;
	for (i = 0; i < 3; i++)
	{
		struct Sprite* sprite2 = &gSprites[sprite->data[i]];

		sprite2->pos1.x = sprite->pos1.x + sprite->pos2.x + (i + 1) * 64;
	}
	if (sprite->pos1.x == sprite->sTargetX)
		sprite->callback = SpriteCB_EndTextBoxSlideIn;
}

static void SpriteCB_EndTextBoxSlideIn(struct Sprite* sprite)
{
	D.slidingTextBoxState = SLIDING_TEXT_ARRIVED;
	if ((u16) sprite->sSlideOutTimer != 0xFFFF)
	{
		if (--sprite->sSlideOutTimer == -1)
			StartTextBoxSlideOut(sprite->sSlideIncrement);
	}
}

static void SpriteCB_TextBoxSlideOut(struct Sprite* sprite)
{
	s32 i;
	s16 delta = sprite->sDistance + sprite->sSlideIncrement;

	sprite->pos1.x -= delta >> 8;
	sprite->sDistance += sprite->sSlideIncrement;
	sprite->sDistance &= 0xFF;
	for (i = 0; i < 3; i++)
	{
		struct Sprite* sprite2 = &gSprites[sprite->data[i]];

		sprite2->pos1.x = sprite->pos1.x + sprite->pos2.x + (i + 1) * 64;
	}
	if (sprite->pos1.x + sprite->pos2.x < -224)
		EndTextBoxSlideOut(sprite);
}

#undef sTargetX
#undef sSlideOutTimer
#undef sSlideIncrement
#undef sDistance

//---- 作成と CB2 ---------------------------------------------------------------------------------------------

static void AllocContestResults(void)
{
	RES = AllocZeroed(sizeof(struct HackContestResults));
	ContestFr_AllocateMonSpritesGfx();
	Free(gMonSpritesGfxPtr->firstDecompressed); //会場と同じ（FR の既定の塊は小さい＝0x4000 に取り直して 2 枚ぶんに分ける）
	gMonSpritesGfxPtr->firstDecompressed = AllocZeroed(0x4000);
	gMonSpritesGfxPtr->sprites[0] = gMonSpritesGfxPtr->firstDecompressed;
	gMonSpritesGfxPtr->sprites[1] = (u8*) gMonSpritesGfxPtr->firstDecompressed + 0x2000;
}

static void FreeContestResults(void)
{
	struct HackContestResults* r = RES;

	RES = NULL;
	Free(r);
	ContestFr_FreeMonSpritesGfx();
}

static void CB2_ShowContestResults(void)
{
	AnimateSprites();
	BuildOamBuffer();
	RunTasks();
	UpdatePaletteFade();
	CopyBgTilemapBufferToVram(1);
	CopyBgTilemapBufferToVram(2);
}

static void VBlankCB_ShowContestResults(void)
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
	LoadOam();
	ProcessSpriteCopyRequests();
	ContestFr_TransferPlttBuffer();
	ScanlineEffect_InitHBlankDmaTransfer();
}

//原文 CB2_StartShowContestResults（日本語版 0x080F6318）。Keep が有効（magic）のときだけ。無効なら野外へ戻る
void HackContest_ResultsStart(void)
{
	if (KEEP->magic != HACK_CONTEST_KEEP_MAGIC)
	{
		SetMainCallback2(CB2_ReturnToField);
		return;
	}
	gPaletteFade->bufferTransferDisabled = TRUE;
	SetVBlankCallback(NULL);
	AllocContestResults();
	InitContestResultsDisplay();
	ScanlineEffect_Clear();
	ResetPaletteFade();
	gPaletteFade->bufferTransferDisabled = TRUE;
	ResetSpriteData();
	ResetTasks();
	FreeAllSpritePalettes();
	LoadContestResultsBgGfx();
	LoadAllContestMonIconPalettes();
	LoadAllContestMonIcons(0, TRUE);
	LoadAllContestMonNames();
	memset(&D, 0, sizeof(D));
	memset(RES->monResults, 0, sizeof(RES->monResults));
	D.returnToScript = KEEP->resultsReturn;
	CreateResultsTextWindowSprites();
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
	gPaletteFade->bufferTransferDisabled = FALSE;
	D.showResultsTaskId = CreateTask(Task_ShowContestResults, 5);
	SetMainCallback2(CB2_ShowContestResults);
	gBattle_WIN1H = WIN_RANGE(0, DISPLAY_WIDTH);
	gBattle_WIN1V = WIN_RANGE(DISPLAY_HEIGHT - 32, DISPLAY_HEIGHT);
	CreateTask(Task_SlideContestResultsBg, 20);
	CalculateContestantsResultData();
	PlayBGM(HC_MUS_CONTEST_RESULTS); //原文 contest_util.c:553（通信でない側。S9 までは流さなかった）
	SetVBlankCallback(VBlankCB_ShowContestResults);
}

//原文 ShowContestResults（special）。台本から呼ぶ（呼び出しは S5）。画面を暗くしてから結果発表へ。終わると台本の続きへ戻る
static void Task_StartShowContestResults(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		DestroyTask(taskId);
		SetMainCallback2(HackContest_ResultsStart);
	}
}

void ShowContestResults(void)
{
	HACK_CONTEST_KEEP.resultsReturn = 1;
	LockPlayerFieldControls();
	CreateTask(Task_StartShowContestResults, 10);
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
}

//---- タスク ---------------------------------------------------------------------------------------------------

#define tState data[0]
#define tTimer data[1]
#define tCounter data[2]

//原文 Task_ShowContestResults の通信でない側（TV・ファン・GAME_STAT は書かない。勝者の保存は S6＝contest_util.c:662〜666 の SaveContestWinner 2 本）
static void Task_ShowContestResults(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		HackContest_SaveWinner();   //S6: 優勝者をロビーの絵の記録へ（NPC の優勝も記録）・絵描きに見せる一時の優勝者を作る
		gTasks[taskId].tState = 0;
		gTasks[taskId].func = Task_AnnouncePreliminaryResults;
	}
}

static void Task_AnnouncePreliminaryResults(u8 taskId)
{
	s16 x;

	if (gTasks[taskId].tState == 0)
	{
		CreateTask(Task_FlashStarsAndHearts, 20);
		x = DrawResultsTextWindow(gHackContest_AnnouncingResults);
		StartTextBoxSlideIn(x, TEXT_BOX_Y, 120, 1088);
		gTasks[taskId].tState++;
	}
	else if (gTasks[taskId].tState == 1)
	{
		if (D.slidingTextBoxState == SLIDING_TEXT_OFFSCREEN)
		{
			gTasks[taskId].tTimer = 0;
			gTasks[taskId].tState++;
		}
	}
	else if (gTasks[taskId].tState == 2)
	{
		if (++gTasks[taskId].tTimer == 21)
		{
			gTasks[taskId].tTimer = 0;
			gTasks[taskId].tState++;
		}
	}
	else if (gTasks[taskId].tState == 3)
	{
		x = DrawResultsTextWindow(gHackContest_PreliminaryResults);
		StartTextBoxSlideIn(x, TEXT_BOX_Y, -1, 1088);
		gTasks[taskId].tState++;
	}
	else if (gTasks[taskId].tState == 4)
	{
		if (D.slidingTextBoxState == SLIDING_TEXT_ARRIVED)
		{
			gTasks[taskId].tState = 0;
			gTasks[taskId].func = Task_ShowPreliminaryResults;
		}
	}
}

static void UpdateContestResultBars(bool8 isRound2, u8 numUpdates);

static void Task_ShowPreliminaryResults(u8 taskId)
{
	switch (gTasks[taskId].tState)
	{
	case 0:
		if (!D.pointsFlashing)
		{
			UpdateContestResultBars(FALSE, gTasks[taskId].tCounter++);
			if (D.numBarsUpdating == 0)
				gTasks[taskId].tState = 2;
			else
				gTasks[taskId].tState++;
		}
		break;
	case 1:
		if (D.numBarsUpdating == 0)
			gTasks[taskId].tState = 0;
		break;
	case 2:
		StartTextBoxSlideOut(1088);
		gTasks[taskId].tState = 0;
		gTasks[taskId].tCounter = 0;
		gTasks[taskId].func = Task_AnnounceRound2Results;
		break;
	}
}

static void Task_AnnounceRound2Results(u8 taskId)
{
	s16 x;

	if (D.slidingTextBoxState == SLIDING_TEXT_OFFSCREEN)
	{
		if (++gTasks[taskId].tTimer == 21)
		{
			gTasks[taskId].tTimer = 0;
			x = DrawResultsTextWindow(gHackContest_Round2Results);
			StartTextBoxSlideIn(x, TEXT_BOX_Y, -1, 1088);
		}
	}
	else if (D.slidingTextBoxState == SLIDING_TEXT_ARRIVED)
	{
		gTasks[taskId].func = Task_ShowRound2Results;
	}
}

static void Task_ShowRound2Results(u8 taskId)
{
	switch (gTasks[taskId].tState)
	{
	case 0:
		if (!D.pointsFlashing)
		{
			UpdateContestResultBars(TRUE, gTasks[taskId].tCounter++);
			if (D.numBarsUpdating == 0)
				gTasks[taskId].tState = 2;
			else
				gTasks[taskId].tState++;
		}
		break;
	case 1:
		if (D.numBarsUpdating == 0)
			gTasks[taskId].tState = 0;
		break;
	case 2:
		StartTextBoxSlideOut(1088);
		gTasks[taskId].tState = 0;
		gTasks[taskId].func = Task_AnnounceWinner;
		break;
	}
}

static void BounceMonIconInBox(u8 monIndex, u8 numFrames);

#define tFinalStanding data[0]
#define tMonIndex data[1]

static void Task_AnnounceWinner(u8 taskId)
{
	s32 i;

	switch (gTasks[taskId].tState)
	{
	case 0:
		if (D.slidingTextBoxState == SLIDING_TEXT_OFFSCREEN)
			gTasks[taskId].tState++;
		break;
	case 1:
		if (++gTasks[taskId].tTimer == 31)
		{
			gTasks[taskId].tTimer = 0;
			gTasks[taskId].tState++;
		}
		break;
	case 2:
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			u8 newTaskId = CreateTask(Task_DrawFinalStandingNumber, 10);

			gTasks[newTaskId].tFinalStanding = KEEP->finalStandings[i];
			gTasks[newTaskId].tMonIndex = i;
		}
		gTasks[taskId].tState++;
		break;
	case 3:
		if (D.numStandingsPrinted == HACK_CONTEST_CONTESTANTS)
		{
			if (++gTasks[taskId].tTimer == 31)
			{
				gTasks[taskId].tTimer = 0;
				CreateTask(Task_StartHighlightWinnersBox, 10);
				gTasks[taskId].tState++;
				GET_CONTEST_WINNER_ID(i);
				BounceMonIconInBox(i, 14);
			}
		}
		break;
	case 4:
		if (++gTasks[taskId].tTimer == 21)
		{
			u8 winnerTextBuffer[100];
			s16 x;

			gTasks[taskId].tTimer = 0;
			GET_CONTEST_WINNER_ID(i);
			StringCopy(gStringVar1, KEEP->mons[i].trainerName); //（日本語版は ConvertInternationalContestantName を呼ばない）
			StringCopy(gStringVar2, KEEP->mons[i].nickname);
			StringExpandPlaceholders(winnerTextBuffer, gHackContest_ContestantsMonWon);
			x = DrawResultsTextWindow(winnerTextBuffer);
			StartTextBoxSlideIn(x, TEXT_BOX_Y, -1, 1088);
			gTasks[taskId].tState++;
		}
		break;
	case 5:
		gTasks[taskId].tState = 0;
		gTasks[taskId].func = Task_ShowWinnerMonBanner;
		break;
	}
}

#undef tFinalStanding
#undef tMonIndex

static void Task_ShowWinnerMonBanner(u8 taskId)
{
	s32 i;
	u8 spriteId;
	u16 species;
	u32 otId;
	u32 personality;
	const struct CompressedSpritePalette* pokePal;

	switch (gTasks[taskId].tState)
	{
	case 0:
		gBattle_WIN0H = WIN_RANGE(0, DISPLAY_WIDTH);
		gBattle_WIN0V = WIN_RANGE(DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2);
		GET_CONTEST_WINNER_ID(i);
		species = KEEP->mons[i].species;
		if (species >= NUM_SPECIES)
			species = 0;
		personality = KEEP->mons[i].personality;
		otId = KEEP->mons[i].otId;
		HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species], gMonSpritesGfxPtr->sprites[1], species, personality);
		pokePal = GetMonSpritePalStructFromOtIdPersonality(species, otId, personality);
		LoadCompressedSpritePalette(pokePal);
		ContestFr_SetMultiuseSpriteTemplateToPokemon(species, 1); //B_POSITION_OPPONENT_LEFT
		((struct SpriteTemplate*) 0x0202443C)->paletteTag = pokePal->tag; //gMultiuseSpriteTemplate
		spriteId = CreateSprite((const struct SpriteTemplate*) 0x0202443C, DISPLAY_WIDTH + 32, DISPLAY_HEIGHT / 2, 10);
		gSprites[spriteId].data[1] = species;
		gSprites[spriteId].oam.priority = 0;
		gSprites[spriteId].callback = SpriteCB_WinnerMonSlideIn;
		D.winnerMonSpriteId = spriteId;
		LoadCompressedSpriteSheet(&sSpriteSheet_Confetti);
		LoadCompressedSpritePalette(&sSpritePalette_Confetti);
		CreateTask(Task_CreateConfetti, 10);
		gTasks[taskId].tState++;
		break;
	case 1:
		if (++gTasks[taskId].data[3] == 1)
		{
			u8 counter;

			gTasks[taskId].data[3] = 0;
			gTasks[taskId].tCounter += 2;
			if (gTasks[taskId].tCounter > 32)
				gTasks[taskId].tCounter = 32;
			counter = gTasks[taskId].tCounter;
			gBattle_WIN0V = WIN_RANGE(DISPLAY_HEIGHT / 2 - counter, DISPLAY_HEIGHT / 2 + counter);
			if (counter == 32)
				gTasks[taskId].tState++;
		}
		break;
	case 2:
		if (D.winnerMonSlidingState == SLIDING_MON_ENTERED)
			gTasks[taskId].tState++;
		break;
	case 3:
		if (++gTasks[taskId].tTimer == 121)
		{
			gTasks[taskId].tTimer = 0;
			gSprites[D.winnerMonSpriteId].callback = SpriteCB_WinnerMonSlideOut;
			gTasks[taskId].tState++;
		}
		break;
	case 4:
		if (D.winnerMonSlidingState == SLIDING_MON_EXITED)
		{
			u8 top = (gBattle_WIN0V >> 8);

			top += 2;
			if (top > DISPLAY_HEIGHT / 2)
				top = DISPLAY_HEIGHT / 2;
			gBattle_WIN0V = WIN_RANGE(top, DISPLAY_HEIGHT - top);
			if (top == DISPLAY_HEIGHT / 2)
				gTasks[taskId].tState++;
		}
		break;
	case 5:
		if (D.winnerMonSlidingState == SLIDING_MON_EXITED)
		{
			D.destroyConfetti = TRUE;
			gTasks[taskId].tState = 0;
			gTasks[taskId].func = Task_SetSeenWinnerMon;
		}
		break;
	}
}

//原文 Task_SetSeenWinnerMon（A で 4 匹を「みた」に）→ TV のインタビュー（書かない）→ 暗転して終わり
static void Task_SetSeenWinnerMon(u8 taskId)
{
	s32 i;

	if (gMain.newKeys & A_BUTTON)
	{
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			u16 nationalDexNum = SpeciesToNationalPokedexNum(KEEP->mons[i].species);

			if (nationalDexNum != 0)
				GetSetPokedexFlag(nationalDexNum, FLAG_SET_SEEN);
		}
		BeginHardwarePaletteFade(0xFF, 0, 0, 16, 0);
		gTasks[taskId].func = Task_EndShowContestResults;
	}
}

static void Task_EndShowContestResults(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		if (gTasks[taskId].tTimer == 0)
		{
			DestroyTask(D.highlightWinnerTaskId);
			BlendPalettes(PALETTES_BG, 16, RGB_BLACK);
			gTasks[taskId].tTimer++;
		}
		else if (gTasks[taskId].tTimer == 1)
		{
			BlendPalettes(PALETTES_OBJECTS, 16, RGB_BLACK);
			gTasks[taskId].tTimer++;
		}
		else
		{
			u8 toScript = D.returnToScript;

			SetGpuReg(REG_OFFSET_BLDCNT, 0);
			SetGpuReg(REG_OFFSET_BLDY, 0);
			DestroyTask(taskId);
			FreeAllWindowBuffers();
			SetVBlankCallback(NULL);
			SetMainCallback2(toScript ? CB2_ReturnToFieldContinueScriptPlayMapMusic : CB2_ReturnToField);
			FreeContestResults();
		}
	}
}

#undef tState
#undef tTimer
#undef tCounter

//背景（BG3）をななめに流す
static void Task_SlideContestResultsBg(u8 taskId)
{
	(void) taskId;
	gBattle_BG3_X += 2;
	gBattle_BG3_Y += 1;
	if (gBattle_BG3_X > 255)
		gBattle_BG3_X -= 255;
	if (gBattle_BG3_Y > 255)
		gBattle_BG3_Y -= 255;
}

#define tDelay data[0]
#define tCoeff data[1]
#define tDecreasing data[2]

//星とハートの色の点滅（日本語版 0x080F7114・BG パレット 6 の 11・8・14 番）
static void Task_FlashStarsAndHearts(u8 taskId)
{
	if (++gTasks[taskId].tDelay == 2)
	{
		gTasks[taskId].tDelay = 0;
		if (!gTasks[taskId].tDecreasing)
			gTasks[taskId].tCoeff++;
		else
			gTasks[taskId].tCoeff--;
		if (gTasks[taskId].tCoeff == 16)
			gTasks[taskId].tDecreasing = TRUE;
		else if (gTasks[taskId].tCoeff == 0)
			gTasks[taskId].tDecreasing = FALSE;
		BlendPalette(16 * 6 + 11, 1, gTasks[taskId].tCoeff, RGB(30, 22, 11));
		BlendPalette(16 * 6 + 8, 1, gTasks[taskId].tCoeff, RGB_WHITE);
		BlendPalette(16 * 6 + 14, 1, gTasks[taskId].tCoeff, RGB(30, 29, 29));
	}
	if (gTasks[taskId].tCoeff == 0)
		D.pointsFlashing = FALSE;
	else
		D.pointsFlashing = TRUE;
}

#undef tDelay
#undef tCoeff
#undef tDecreasing

#define tFinalStanding data[0]
#define tMonIndex data[1]
#define tState data[10]

//順位の数字（3−順位 × 40 コマ待って出る。1 位ほど遅い）
static void Task_DrawFinalStandingNumber(u8 taskId)
{
	u16 firstTileNum;

	if (gTasks[taskId].tState == 0)
	{
		gTasks[taskId].data[11] = (3 - gTasks[taskId].tFinalStanding) * 40;
		gTasks[taskId].tState++;
	}
	else if (gTasks[taskId].tState == 1)
	{
		if (--gTasks[taskId].data[11] == -1)
		{
			firstTileNum = gTasks[taskId].tFinalStanding * 2 + 0x5043;
			ContestFr_WriteSequenceToBgTilemapBuffer(2, firstTileNum, 1, gTasks[taskId].tMonIndex * 3 + 5, 2, 1, 17, 1);
			ContestFr_WriteSequenceToBgTilemapBuffer(2, firstTileNum + 0x10, 1, gTasks[taskId].tMonIndex * 3 + 6, 2, 1, 17, 1);
			D.numStandingsPrinted++;
			DestroyTask(taskId);
			PlaySE(HC_SE_PLACE);
		}
	}
}

#undef tFinalStanding
#undef tMonIndex
#undef tState

//優勝の箱を点滅させる
static void Task_StartHighlightWinnersBox(u8 taskId)
{
	s32 i;

	GET_CONTEST_WINNER_ID(i);
	CopyToBgTilemapBufferRect_ChangePalette(2, (u8*) RES->tilemapBuffers[2] + i * 0xC0 + 0x100, 0, i * 3 + 4, 32, 3, 9);
	gTasks[taskId].data[10] = i;
	gTasks[taskId].data[12] = 1;
	gTasks[taskId].func = Task_HighlightWinnersBox;
	D.highlightWinnerTaskId = taskId;
}

static void Task_HighlightWinnersBox(u8 taskId)
{
	if (++gTasks[taskId].data[11] == 1)
	{
		gTasks[taskId].data[11] = 0;
		BlendPalette(16 * 9 + 1, 1, gTasks[taskId].data[12], RGB(13, 28, 27));
		if (gTasks[taskId].data[13] == 0)
		{
			if (++gTasks[taskId].data[12] == 16)
				gTasks[taskId].data[13] = 1;
		}
		else
		{
			if (--gTasks[taskId].data[12] == 0)
				gTasks[taskId].data[13] = 0;
		}
	}
}

//優勝ポケモンの絵（右から入る・10 コマ待って鳴き声）
static void SpriteCB_WinnerMonSlideIn(struct Sprite* sprite)
{
	if (sprite->data[0] < 10)
	{
		if (++sprite->data[0] == 10)
		{
			PlayCry1(sprite->data[1], 0);
			sprite->data[1] = 0;
		}
	}
	else
	{
		s16 delta = sprite->data[1] + 0x600;

		sprite->pos1.x -= delta >> 8;
		sprite->data[1] += 0x600;
		sprite->data[1] &= 0xFF;
		if (sprite->pos1.x < DISPLAY_WIDTH / 2)
			sprite->pos1.x = DISPLAY_WIDTH / 2;
		if (sprite->pos1.x == DISPLAY_WIDTH / 2)
		{
			sprite->callback = SpriteCallbackDummy;
			sprite->data[1] = 0;
			D.winnerMonSlidingState = SLIDING_MON_ENTERED;
		}
	}
}

static void SpriteCB_WinnerMonSlideOut(struct Sprite* sprite)
{
	s16 delta = sprite->data[1] + 0x600;

	sprite->pos1.x -= delta >> 8;
	sprite->data[1] += 0x600;
	sprite->data[1] &= 0xFF;
	if (sprite->pos1.x < -32)
	{
		sprite->callback = SpriteCallbackDummy;
		sprite->invisible = TRUE;
		D.winnerMonSlidingState = SLIDING_MON_EXITED;
	}
}

//紙吹雪（5 コマごとに 1 枚・同時に 40 枚まで）
static void Task_CreateConfetti(u8 taskId)
{
	if (++gTasks[taskId].data[0] == 5)
	{
		gTasks[taskId].data[0] = 0;
		if (D.confettiCount < 40)
		{
			u8 spriteId = CreateSprite(&sSpriteTemplate_Confetti, (Random() % DISPLAY_WIDTH) - 20, 44, 5);

			gSprites[spriteId].data[0] = Random() % 512;
			gSprites[spriteId].data[1] = (Random() % 24) + 16;
			gSprites[spriteId].data[2] = (Random() % 256) + 48;
			gSprites[spriteId].oam.tileNum += Random() % 17;
			D.confettiCount++;
		}
	}
	if (D.destroyConfetti)
		DestroyTask(taskId);
}

static void SpriteCB_Confetti(struct Sprite* sprite)
{
	s16 delta;

	sprite->data[3] += sprite->data[0];
	sprite->pos2.x = Sine(sprite->data[3] >> 8, sprite->data[1]);
	delta = sprite->data[4] + sprite->data[2];
	sprite->pos1.x += delta >> 8;
	sprite->data[4] += sprite->data[2];
	sprite->data[4] &= 0xff;
	sprite->pos1.y++;
	if (D.destroyConfetti)
		sprite->invisible = TRUE;
	if (sprite->pos1.x > DISPLAY_WIDTH + 8 || sprite->pos1.y > 116)
	{
		DestroySprite(sprite);
		D.confettiCount--;
	}
}

#define tMonIndex data[0]
#define tNumFrames data[1]
#define tSpecies data[2]
#define tTimer data[10]
#define tBounced data[11]

//優勝の箱のアイコンを 2 コマ交互に（原文 BounceMonIconInBox）
static void BounceMonIconInBox(u8 monIndex, u8 numFrames)
{
	u8 taskId = CreateTask(Task_BounceMonIconInBox, 8);

	gTasks[taskId].tMonIndex = monIndex;
	gTasks[taskId].tNumFrames = numFrames;
	gTasks[taskId].tSpecies = KEEP->mons[monIndex].species;
}

static void Task_BounceMonIconInBox(u8 taskId)
{
	u8 monIndex = gTasks[taskId].tMonIndex;

	if (gTasks[taskId].tTimer++ == gTasks[taskId].tNumFrames)
	{
		gTasks[taskId].tTimer = 0;
		LoadContestMonIcon(gTasks[taskId].tSpecies, monIndex, gTasks[taskId].tBounced, FALSE, KEEP->mons[monIndex].personality);
		gTasks[taskId].tBounced ^= 1;
	}
}

#undef tMonIndex
#undef tNumFrames
#undef tSpecies
#undef tTimer
#undef tBounced

#define tMonId data[0]
#define tTarget data[1]
#define tDecreasing data[2]

//棒を伸ばす（一次＝星・二次＝ハートが 1 つずつ増えるたびに目標の長さへ）
static void UpdateContestResultBars(bool8 isRound2, u8 numUpdates)
{
	s32 i, taskId;
	u32 target;
	u8 numIncreasing = 0, numDecreasing = 0;

	if (!isRound2)
	{
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			u8 numStars = RES->monResults[i].numStars;

			if (numUpdates < numStars)
			{
				FillBgTilemapBufferRect_Palette0(1, 0x60B3, ((19 + numStars) - numUpdates) - 1, i * 3 + 5, 1, 1);
				taskId = CreateTask(Task_UpdateContestResultBar, 10);
				target = ((RES->monResults[i].barLengthPreliminary << 16) / RES->monResults[i].numStars) * (numUpdates + 1);
				if ((target & 0xFFFF) > 0x7FFF)
					target += 0x10000;
				gTasks[taskId].tMonId = i;
				gTasks[taskId].tTarget = target >> 16;
				D.numBarsUpdating++;
				numIncreasing++;
			}
		}
	}
	else
	{
		for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		{
			s8 numHearts = RES->monResults[i].numHearts;
			u32 tile = RES->monResults[i].lostPoints ? 0x60A5 : 0x60A3;

			if (numUpdates < numHearts)
			{
				FillBgTilemapBufferRect_Palette0(1, tile, ((19 + numHearts) - numUpdates) - 1, i * 3 + 6, 1, 1);
				taskId = CreateTask(Task_UpdateContestResultBar, 10);
				target = ((RES->monResults[i].barLengthRound2 << 16) / RES->monResults[i].numHearts) * (numUpdates + 1);
				if ((target & 0xFFFF) > 0x7FFF)
					target += 0x10000;
				gTasks[taskId].tMonId = i;
				if (RES->monResults[i].lostPoints)
				{
					gTasks[taskId].tDecreasing = TRUE;
					numDecreasing++;
				}
				else
				{
					numIncreasing++;
				}
				if (RES->monResults[i].lostPoints)
					gTasks[taskId].tTarget = -(target >> 16) + RES->monResults[i].barLengthPreliminary;
				else
					gTasks[taskId].tTarget = (target >> 16) + RES->monResults[i].barLengthPreliminary;
				D.numBarsUpdating++;
			}
		}
	}
	if (numDecreasing)
		PlaySE(HC_SE_BOO);
	if (numIncreasing)
		PlaySE(HC_SE_PIN);
}

//棒 1 本が目標の長さまで 1 コマに 1 ずつ伸びる（縮む）・11 個のタイルを描き直す
static void Task_UpdateContestResultBar(u8 taskId)
{
	s32 i;
	bool32 minMaxReached = FALSE;
	bool32 targetReached = FALSE;
	u8 monId = gTasks[taskId].tMonId;
	s16 target = gTasks[taskId].tTarget;
	s16 decreasing = gTasks[taskId].tDecreasing;

	if (decreasing)
	{
		if (D.barLength[monId] <= 0)
			minMaxReached = TRUE;
	}
	else
	{
		if (D.barLength[monId] >= MAX_BAR_LENGTH)
			minMaxReached = TRUE;
	}
	if (D.barLength[monId] == target)
		targetReached = TRUE;
	if (!targetReached)
	{
		if (minMaxReached)
			D.barLength[monId] = target;
		else if (decreasing)
			D.barLength[monId]--;
		else
			D.barLength[monId]++;
	}
	if (!minMaxReached && !targetReached)
	{
		u8 tileOffset;
		u16 tileNum;

		for (i = 0; i < NUM_BAR_SEGMENTS; i++)
		{
			if (D.barLength[monId] >= (i + 1) * BAR_SEGMENT_LENGTH)
				tileOffset = 8;
			else if (D.barLength[monId] >= i * BAR_SEGMENT_LENGTH)
				tileOffset = D.barLength[monId] % 8;
			else
				tileOffset = 0;
			if (tileOffset < 4)
				tileNum = 0x504C + tileOffset;
			else
				tileNum = 0x5057 + tileOffset;
			FillBgTilemapBufferRect_Palette0(2, tileNum, i + 7, monId * 3 + 6, 1, 1);
		}
	}
	if (targetReached)
	{
		D.numBarsUpdating--;
		DestroyTask(taskId);
	}
}

#undef tMonId
#undef tTarget
#undef tDecreasing

#ifdef HACK_DEBUG_MENU
//debug 入口（結果だけ・tools/test/contest_results_mgba.py）: gMain.callback2 にこの関数を書くと 1 回だけ実行。
//  Var8009: 0＝プレイヤー（出場者 0）が優勝・1＝優勝しない・2＝全員同点（一次も二次も同じ→乱数で順位）・3＝プレイヤーの二次がマイナス
//  Var8008: 部門 0〜4・Var800A: ランク 0〜3。出場者はパーティの仮。終わると野外へ（台本なし）
void HackContest_DebugResults(void)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;
	u8 count = gPlayerPartyCount ? gPlayerPartyCount : 1;
	s32 i;
	static const s16 r1Tab[4][4] = { { 300, 200, 100, 30 }, { 50, 300, 200, 100 }, { 200, 200, 200, 200 }, { 300, 100, 200, 150 } };
	static const s16 r2Tab[4][4] = { { 500, 300, 100, 20 }, { 80, 400, 300, 160 }, { 400, 400, 400, 400 }, { -160, 480, 160, 400 } };
	u8 v = Var8009 & 3;

	HackContest_Reset();
	memset(k, 0, sizeof(*k));
	k->magic = HACK_CONTEST_KEEP_MAGIC;
	k->playerIndex = 0;
	k->category = Var8008 % 5;
	k->rank = Var800A & 3;
	k->resultsReturn = 0;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		struct Pokemon* mon = &gPlayerParty[i % count];

		k->round1Points[i] = r1Tab[v][i];
		k->round2Points[i] = r2Tab[v][i];
		k->totalPoints[i] = k->round1Points[i] + k->round2Points[i];
		k->mons[i].species = GetMonData(mon, MON_DATA_SPECIES, NULL);
		k->mons[i].otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
		k->mons[i].personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
		GetMonData(mon, MON_DATA_NICKNAME, k->mons[i].nickname);
		k->mons[i].nickname[POKEMON_NAME_LENGTH] = 0xFF;
		StringCopy(k->mons[i].trainerName, gSaveBlock2->playerName);
	}
	HackContest_DetermineStandings(k->round1Points, k->totalPoints, k->finalStandings);
	SetMainCallback2(HackContest_ResultsStart);
}
#endif
