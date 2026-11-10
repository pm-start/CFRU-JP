#include "defines.h"
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
#include "../include/string_util.h"
#include "../include/pokemon.h"
#include "../include/battle.h"
#include "../include/new/Vanilla_functions.h"
#include "../include/new/ram_locs.h"
#include "hack_contest.h"
#include "hack_contest_addr.h"
#include "hack_contest_imgfx.h"
#include "hack_contest_painting_data.h"

/*
 * コンテスト移植 S6（FEAT-contest・patches/0985）: 優勝した絵を描く画面。原文 参考/pokeemerald/src/contest_painting.c をそのまま移植し、
 * 日本語版エメラルドの contest_painting.s（0x0812FDE0〜）と突き合わせた（突き合わせ表は docs/research/2026-11-09_contest-s1-foundation.md の S6 節）。
 * 日本語版が原文（英語版）と違う所: 字幕は左寄せ（ホール x=8・美術館 x=0・y=1）で窓は 左 5・幅 20（HACK_PAINT_WIN_*）・「かっこよさ ぶもん ランク」の組み立て
 *   （部門名＋「ぶもん」＋空白＋ランク名・名前の変換 ConvertInternationalContestantName は無い）。素材は日本語版 ROM から（tools/contest/extract_jp_painting.py）。
 * FR との差: 台本は `callnative`＋`waitstate`（素の FR に showcontestpainting が無い）・終わりは CB2_ReturnToFieldContinueScriptPlayMapMusic・
 *   画像処理は src/hack_contest_imgfx.c（原文の移植）・SeedRng は絵に効かない（点描は個性値から）ので呼ばない。
 * 状態は全部ヒープ（AllocZeroed・出るとき Free）と Keep の中の語 screen（static の書き換えは使えない・6.13。結果発表の画面とは同時に出ない）。
 * 画面の RAM: 作業領域＋ピクセル 0x2000＋パレット 0x200＋MonSpritesGfx（FR 標準）。
 */

struct HackPaint
{
	struct HackContestImgCanvas canvas;   //★ 先頭（hack_contest_imgfx.c が HC_CV で読む）
	u16* pixels;                          //0x2000 バイト（絵の画素 64×64・額縁の地図の置き場にも使う）
	u16* palette;                         //0x200 バイト
	struct HackContestWinner winner;
	u16 mosaicVal;
	u16 fadeCounter;
	u8 saveIdx;                           //記録の番号 0〜12（絵描きの絵は部門の枠 8〜12）
	u8 forArtist;
	u8 holdState;
	u8 windowId;
};
#define P ((struct HackPaint*) HACK_CONTEST_KEEP.screen)

#define gMonFrontPicTable ((const struct CompressedSpriteSheet*) *((u32*) 0x8000128))

//日本語版の BG の枠（0x0858F5F8）: bg1・文字 1・地図 10・priority 1
static const struct BgTemplate sBgTemplates[] =
{
	{
		.bg = 1,
		.charBaseIndex = 1,
		.mapBaseIndex = 10,
		.screenSize = 0,
		.paletteMode = 0,
		.priority = 1,
		.baseTile = 0,
	},
};

static const struct WindowTemplate sWindowTemplate =
{
	.bg = HACK_PAINT_WIN_BG,
	.tilemapLeft = HACK_PAINT_WIN_LEFT,
	.tilemapTop = HACK_PAINT_WIN_TOP,
	.width = HACK_PAINT_WIN_WIDTH,
	.height = HACK_PAINT_WIN_HEIGHT,
	.paletteNum = HACK_PAINT_WIN_PAL,
	.baseBlock = HACK_PAINT_WIN_BASEBLOCK,
};

//BIOS の RLUnCompWram（swi 0x14）／RLUnCompVram（swi 0x15）。素の FR の同名の関数は BPRJ.ld に無いので、BIOS を直に呼ぶ（Thumb）
static void HcRLUnCompWram(const void* src, void* dest)
{
	register const void* r0 __asm__("r0") = src;
	register void* r1 __asm__("r1") = dest;

	__asm__ volatile("swi 0x14" : "+r"(r0), "+r"(r1) : : "r2", "r3", "memory");
}

static void HcRLUnCompVram(const void* src, void* dest)
{
	register const void* r0 __asm__("r0") = src;
	register void* r1 __asm__("r1") = dest;

	__asm__ volatile("swi 0x15" : "+r"(r0), "+r"(r1) : : "r2", "r3", "memory");
}

static void CB2_ShowPainting(void);
static void CB2_HoldPainting(void);

//---- 絵 ------------------------------------------------------------------------------------------------

static void InitMonPixels(void)
{
	struct HackPaint* p = P;
	const u8* spriteGfx;
	u16 species = p->winner.species;
	u16 tileY, tileX, pixelY, pixelX;
	u8 colorIndex;

	if (species >= NUM_SPECIES)
		species = 0;
	LZDecompressWram(GetMonSpritePalStructFromOtIdPersonality(species, p->winner.trainerId, p->winner.personality)->data, p->palette);
	HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species], gMonSpritesGfxPtr->sprites[1], species, p->winner.personality);
	spriteGfx = gMonSpritesGfxPtr->sprites[1];
	for (tileY = 0; tileY < 8; tileY++)
		for (tileX = 0; tileX < 8; tileX++)
			for (pixelY = 0; pixelY < 8; pixelY++)
				for (pixelX = 0; pixelX < 8; pixelX++)
				{
					colorIndex = spriteGfx[32 * (tileY * 8 + tileX) + (pixelY << 2) + (pixelX >> 1)];
					if (pixelX & 1)
						colorIndex >>= 4;
					else
						colorIndex &= 0xF;
					if (colorIndex == 0)   //透明
						p->pixels[(8 * tileY + pixelY) * 64 + tileX * 8 + pixelX] = 0x8000;
					else
						p->pixels[(8 * tileY + pixelY) * 64 + tileX * 8 + pixelX] = p->palette[colorIndex];
				}
}

static u8 EffectCategory(void)
{
	struct HackPaint* p = P;

	return p->saveIdx < HACK_WINNER_HALL_COUNT ? p->winner.category : p->winner.category / HACK_PAINTING_CAPTIONS;
}

static u8 GetImageEffect(void)
{
	switch (EffectCategory())
	{
	case 0: return IMAGE_EFFECT_OUTLINE_COLORED;       //かっこよさ
	case 1: return IMAGE_EFFECT_SHIMMER;               //うつくしさ
	case 2: return IMAGE_EFFECT_POINTILLISM;           //かわいさ
	case 3: return IMAGE_EFFECT_CHARCOAL;              //かしこさ
	case 4: return IMAGE_EFFECT_GRAYSCALE_LIGHT;       //たくましさ
	}
	return EffectCategory();
}

static void DoImageProcessing(u8 imageEffect)
{
	struct HackPaint* p = P;
	struct ImageProcessingContext c;

	{
		u8* z = (u8*) &c;
		u32 zi;

		for (zi = 0; zi < sizeof(c); ++zi)
			z[zi] = 0;
	}
	c.canvasPixels = p->pixels;
	c.canvasPalette = p->palette;
	c.paletteStart = 0;
	c.personality = p->winner.personality % 256;
	c.columnStart = 0;
	c.rowStart = 0;
	c.columnEnd = 64;
	c.rowEnd = 64;
	c.canvasWidth = 64;
	c.canvasHeight = 64;
	switch (imageEffect)
	{
	case IMAGE_EFFECT_CHARCOAL:
	case IMAGE_EFFECT_GRAYSCALE_LIGHT:
		c.quantizeEffect = QUANTIZE_EFFECT_GRAYSCALE;
		break;
	default:
		c.quantizeEffect = QUANTIZE_EFFECT_STANDARD_LIMITED_COLORS;
		break;
	}
	c.var_16 = 2;
	c.effect = imageEffect;
	c.dest = (void*) OBJ_VRAM0;
	ApplyImageProcessingEffects(&c);
	ApplyImageProcessingQuantization(&c);
	ConvertImageProcessingToGBA(&c);
	LoadPalette(p->palette, 256, 0x200);   //OBJ パレット 0〜15
}

#define VRAM_PICTURE_DATA(x, y) (((u16*) (BG_SCREEN_ADDR(12)))[(y) * 32 + (x)])

static void LoadFrame(void)
{
	struct HackPaint* p = P;
	u8 x, y;
	u8 cat = p->winner.category / HACK_PAINTING_CAPTIONS;

	if (cat > 4)
		cat = 4;
	LoadPalette(gHackPaintFramePal, 0, 0x100);
	if (p->forArtist)
	{
		//絵描きの額縁: 地図は作業領域へ展開して、絵の枠のぶんだけ BG の地図へ写す
		HcRLUnCompVram(gHackPaintFrameTiles[cat], (void*) VRAM);
		HcRLUnCompWram(gHackPaintFrameMap[cat], p->pixels);
		for (y = 0; y < 20; y++)
			for (x = 0; x < 32; x++)
				VRAM_PICTURE_DATA(x, y) = 0x1015;
		for (y = 0; y < 10; y++)
			for (x = 0; x < 18; x++)
				VRAM_PICTURE_DATA(x + 6, y + 2) = p->pixels[(y + 2) * 32 + (x + 6)];
		for (x = 0; x < 16; x++)
			VRAM_PICTURE_DATA(x + 7, 2) = p->pixels[2 * 32 + 7];   //一番上の列は最初の枠の部品で埋め直す
	}
	else if (p->saveIdx < HACK_WINNER_HALL_COUNT)
	{
		HcRLUnCompVram(gHackPaintFrameTiles[5], (void*) VRAM);
		HcRLUnCompVram(gHackPaintFrameMap[5], (void*) BG_SCREEN_ADDR(12));
	}
	else
	{
		HcRLUnCompVram(gHackPaintFrameTiles[cat], (void*) VRAM);
		HcRLUnCompVram(gHackPaintFrameMap[cat], (void*) BG_SCREEN_ADDR(12));
	}
}

#undef VRAM_PICTURE_DATA

static void CreatePicture(void)
{
	struct HackPaint* p = P;

	p->palette = AllocZeroed(0x200);
	p->pixels = AllocZeroed(0x2000);
	InitMonPixels();
	DoImageProcessing(GetImageEffect());
	gMain.oamBuffer[0] = *(const struct OamData*) gHackPaintMonOam;   //日本語版の OAM（8bpp・64x64・モザイク・priority 0）
	gMain.oamBuffer[0].tileNum = 0;
	gMain.oamBuffer[0].x = 88;
	gMain.oamBuffer[0].y = 24;
	LoadFrame();
}

static void PrintCaption(void)
{
	struct HackPaint* p = P;
	u8 cat, rank;

	if (p->forArtist)
		return;                              //絵描きの絵に字幕は無い
	if (p->saveIdx < HACK_WINNER_HALL_COUNT)
	{
		cat = p->winner.category > 4 ? 4 : p->winner.category;
		rank = p->winner.rank > 4 ? 4 : p->winner.rank;
		StringCopy(gStringVar1, gHackPaintCategoryNames[cat]);
		StringAppend(gStringVar1, gHackPaintBumon);
		StringAppend(gStringVar1, gHackPaintSpace);
		StringAppend(gStringVar1, gHackPaintRankNames[rank]);
		StringCopy(gStringVar2, p->winner.trainerName);
		StringCopy(gStringVar3, p->winner.monName);
		StringExpandPlaceholders(gStringVar4, gHackPaintCaptionHall);
		AddTextPrinterParameterized(p->windowId, 1, gStringVar4, 8, 1, 0, NULL);
	}
	else
	{
		cat = p->winner.category >= HACK_PAINTING_CAPTIONS * 5 ? HACK_PAINTING_CAPTIONS * 5 - 1 : p->winner.category;
		StringCopy(gStringVar1, p->winner.monName);
		StringExpandPlaceholders(gStringVar4, gHackPaintMuseumNames[cat]);
		AddTextPrinterParameterized(p->windowId, 1, gStringVar4, 0, 1, 0, NULL);
	}
	CopyBgTilemapBufferToVram(1);
}

//---- 画面 ----------------------------------------------------------------------------------------------

static void UpdateMosaic(void)
{
	struct HackPaint* p = P;

	if (p == NULL)
		return;
	SetGpuReg(REG_OFFSET_BG1CNT, 1 | (1 << 2) | (10 << 8) | 0x40);   //priority 1・文字 1・地図 10・モザイク
	p->mosaicVal = p->fadeCounter / 2;
	SetGpuReg(REG_OFFSET_MOSAIC, (p->mosaicVal << 12) | (p->mosaicVal << 8) | (p->mosaicVal << 4) | (p->mosaicVal << 0));
}

static void VBlankCB_Painting(void)
{
	UpdateMosaic();
	LoadOam();
	ProcessSpriteCopyRequests();
	ContestFr_TransferPlttBuffer();
}

static void InitBg(void)
{
	SetGpuReg(REG_OFFSET_DISPCNT, 0);
	REG_IE |= INTR_FLAG_VBLANK;
	SetGpuReg(REG_OFFSET_BG0CNT, 2 | (0 << 2) | (12 << 8) | 0x40);   //priority 2・文字 0・地図 12・モザイク
	SetGpuReg(REG_OFFSET_BG1CNT, 1 | (1 << 2) | (10 << 8) | 0x40);
	SetGpuReg(REG_OFFSET_BLDCNT, 0);
	SetGpuReg(REG_OFFSET_BLDALPHA, 0);
	SetGpuReg(REG_OFFSET_BLDY, 0);
}

static void InitWindow(void)
{
	struct HackPaint* p = P;

	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sBgTemplates, 1);
	ChangeBgX(1, 0, 0);
	ChangeBgY(1, 0, 0);
	SetBgTilemapBuffer(1, AllocZeroed(0x800));
	p->windowId = AddWindow(&sWindowTemplate);
	DeactivateAllTextPrinters();
	FillWindowPixelBuffer(p->windowId, 0);
	PutWindowTilemap(p->windowId);
	CopyWindowToVram(p->windowId, 3);
	ShowBg(1);
}

//台本の showcontestpainting の読み替え（HoennNative_ShowContestPainting）。Var8004＝0 絵描き／1〜13 記録。画面を出して、閉じると台本の続きへ戻る
void ShowContestPainting(void)
{
	struct HackPaint* p;
	u16 id = Var8004;
	struct HackContestWinner w13[HACK_WINNER_COUNT];

	if (HACK_CONTEST_KEEP.screen != NULL)
		return;
	p = AllocZeroed(sizeof(struct HackPaint));
	HACK_CONTEST_KEEP.screen = (void*) p;
	if (id == HACK_WINNER_ARTIST)
	{
		p->winner = HACK_ARTIST_WINNER;
		p->forArtist = TRUE;
		p->saveIdx = HackContest_WinnerSaveIdx(HACK_SAVE_FOR_ARTIST);
	}
	else
	{
		if (id > HACK_WINNER_COUNT)
			id = HACK_WINNER_COUNT;
		HackContest_LoadWinners(w13);
		p->winner = w13[id - 1];
		p->forArtist = FALSE;
		p->saveIdx = id - 1;
	}
	if (p->winner.species == 0 || p->winner.species >= NUM_SPECIES)   //絵が無い（記録の無い絵描き）は何も出さない
	{
		HACK_CONTEST_KEEP.screen = NULL;
		Free(p);
		return;
	}
	gMain.state = 0;
	SetMainCallback2(CB2_ShowPainting);
}

static void CB2_QuitPainting(void)
{
	struct HackPaint* p = P;

	SetVBlankCallback(NULL);
	SetGpuReg(REG_OFFSET_MOSAIC, 0);
	Free(p->palette);
	Free(p->pixels);
	RemoveWindow(p->windowId);
	Free(ContestFr_GetBgTilemapBuffer(1));
	FreeAllWindowBuffers();
	ContestFr_FreeMonSpritesGfx();
	HACK_CONTEST_KEEP.screen = NULL;
	Free(p);
	SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static void CB2_HoldPainting(void)
{
	struct HackPaint* p = P;

	switch (p->holdState)
	{
	case 0:
		if (!gPaletteFade->active)
			p->holdState = 1;
		if (p->fadeCounter)
			p->fadeCounter--;
		break;
	case 1:
		if (gMain.newKeys & (A_BUTTON | B_BUTTON))
		{
			p->holdState++;
			BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, 0);
		}
		p->fadeCounter = 0;
		break;
	case 2:
		if (!gPaletteFade->active)
			SetMainCallback2(CB2_QuitPainting);
		if (p->fadeCounter < 30)
			p->fadeCounter++;
		break;
	}
	RunTextPrinters();
	UpdatePaletteFade();
}

static void CB2_ShowPainting(void)
{
	struct HackPaint* p = P;

	switch (gMain.state)
	{
	case 0:
		ScanlineEffect_Clear();
		SetVBlankCallback(NULL);
		ContestFr_AllocateMonSpritesGfx();
		p->mosaicVal = 15;
		p->fadeCounter = 30;
		InitBg();
		gMain.state++;
		break;
	case 1:
		ResetPaletteFade();
		DmaClearLarge32(3, (void*) VRAM, VRAM_SIZE, 0x1000);
		ResetSpriteData();
		gMain.state++;
		break;
	case 2:
		ContestFr_InitKeys();
		InitWindow();
		gMain.state++;
		break;
	case 3:
		CreatePicture();
		gMain.state++;
		break;
	case 4:
		PrintCaption();
		gPlttBufferUnfaded[0] = 0;   //背景は黒
		gPlttBufferFaded[0] = 0;
		DmaClear32(3, (void*) PLTT, PLTT_SIZE);
		BeginFastPaletteFade(2);
		SetVBlankCallback(VBlankCB_Painting);
		p->holdState = 0;
		SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_BG1_ON | DISPCNT_OBJ_ON);
		SetMainCallback2(CB2_HoldPainting);
		break;
	}
}
