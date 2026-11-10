#include "defines.h"
#include "defines_battle.h"
#include "../include/item.h"
#include "../include/new/item.h"
#include "../include/menu.h"
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/event_data.h"
#include "hack_debug_menu.h"
#include "hack_debug_catch_info.h"

/*
	デバッグ版だけ: 戦闘中の捕獲情報（2026-10-16 ユーザー・0.50.0「戦闘中に捕獲率などが見える画面」→ 戦闘の操作を止めないオーバーレイ）。
	資料: docs/research/2026-10-16_debug-catch-info.md

	- 出し方: デバッグメニュー（野外の B＋Select）の「せんとうじょうほう」を オン にすると、戦闘の「どうする？」が出る頃（HandleTurnActionSelectionState）から
	  画面の右上に常に出る（技・バッグ・アニメを止めない）。オフにすると消える。置き場は画面の右上 128x64（相手の HP バーは左上なので重ならない・相手のポケモンの絵には少し重なる）。
	- 作り: スプライト 2 枚（64x64 を横に並べる）。文字は ability popup と同じ作り（ウィンドウのタイル 16x8 に文字を描き、スプライトのタイルへ写す）。
	  背景は透明・文字は白に黒の影。OBJ のタイル 128 個とパレット 1 枠を戦闘の間つかう（技のアニメで足りなくなるときはオフにする）。
	- 計算は src/catching.c の HackDebugCatchInfo_Calc（捕獲の式と同じ関数を通す）。ここは表示だけ。
	- 計算し直す間隔は 6 コマ。内容の印（hackCatchInfoSig）が変わったときだけ描き直す。状態は gNewBS に持つ（static の書き込み変数は使えない・6.13）。
	- 表示（1 行 約 16 字）:
	    1 行め  ほかく255 HP100 1。5   = 種族の捕獲率・相手の HP の割合（%）・状態異常などの補正（倍）
	    2〜5 行め  ボールの名前の頭 3 字・ボールの補正値（倍）・そのまま投げたときの捕まる確率（%）・A 連打 10 回の場合の確率（%）
	  日本語版の文字の表に「.」「%」が無いので、小数点は 。(0xAD)・% は付けない。
	  トレーナー戦は「トレーナーせんは つかまえられない」。
*/

#ifdef HACK_DEBUG_MENU

#define CI_TILE_TAG 0x5E50
#define CI_PAL_TAG  0x5E51
#define CI_X        112          //左端（右端 240）
#define CI_Y        0
#define CI_LINE_H   12
#define CI_RECALC_FRAMES 6

extern const u8 gText_HackDebug_CI_Rate[];
extern const u8 gText_HackDebug_CI_Hp[];
extern const u8 gText_HackDebug_CI_Trainer[];
extern const u8 gText_HackDebug_CI_Cant[];
extern const u8 gText_HackDebug_CI_NoBall[];
extern void HandleTurnActionSelectionState(void);

#define CH_SPACE 0x00
#define CH_DOT   0xAD
#define CH_ZERO  0xA1

//0 = 透明・1 = 影（黒）・2 = 文字（白）・3 = 連打の欄（黄）
static const u16 sCatchInfoPal[16] = {0x0000, 0x0000, 0x7FFF, 0x03FF};

static const struct OamData sCatchInfoOam =
{
	.shape = ST_OAM_SQUARE,
	.size = 3,
	.priority = 0,
};

static const union AnimCmd sCatchInfoAnimA[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd sCatchInfoAnimB[] = {ANIMCMD_FRAME(64, 0), ANIMCMD_END};
static const union AnimCmd *const sCatchInfoAnimsA[] = {sCatchInfoAnimA};
static const union AnimCmd *const sCatchInfoAnimsB[] = {sCatchInfoAnimB};

static const struct SpriteTemplate sCatchInfoTemplateA =
{
	.tileTag = CI_TILE_TAG,
	.paletteTag = CI_PAL_TAG,
	.oam = &sCatchInfoOam,
	.anims = sCatchInfoAnimsA,
	.images = NULL,
	.affineAnims = gDummySpriteAffineAnimTable,
	.callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sCatchInfoTemplateB =
{
	.tileTag = CI_TILE_TAG,
	.paletteTag = CI_PAL_TAG,
	.oam = &sCatchInfoOam,
	.anims = sCatchInfoAnimsB,
	.images = NULL,
	.affineAnims = gDummySpriteAffineAnimTable,
	.callback = SpriteCallbackDummy,
};

static const struct SpritePalette sCatchInfoSpritePal = {sCatchInfoPal, CI_PAL_TAG};

//---- 行の組み立て ----

//v（0.1 単位）を 小数 1 桁で書く（intWidth 桁ぶんの右詰め・100 以上は整数部だけ増える）
static u8 *AppendTenths(u8 *p, u32 v, u8 intWidth)
{
	u32 ip = v / 10;
	u32 fp = v - ip * 10;

	p = ConvertIntToDecimalStringN(p, ip, STR_CONV_MODE_RIGHT_ALIGN, intWidth);
	*p++ = CH_DOT;
	*p++ = CH_ZERO + fp;
	*p = EOS;
	return p;
}

//確率（0.1% 単位）を 4 字で: 12.3 / 100
static u8 *AppendPermille(u8 *p, u16 pm)
{
	if (pm >= 1000)
		return ConvertIntToDecimalStringN(p, 100, STR_CONV_MODE_RIGHT_ALIGN, 4);
	return AppendTenths(p, pm, 2);
}

static void PrintLine(u8 windowId, u8 line, const u8 *text, u8 fg)
{
	u8 color[3] = {0, fg, 1};

	AddTextPrinterParameterized4(windowId, 0, 1, line * CI_LINE_H, 0, 0, color, -1, text);
}

static void DrawInfo(u8 windowId, const struct HackDebugCatchInfo *info)
{
	u8 buf[48];
	u8 *p;
	u8 i, k;

	FillWindowPixelBuffer(windowId, PIXEL_FILL(0));

	if (info->state != DEBUG_CATCH_OK)
	{
		PrintLine(windowId, 0, info->state == DEBUG_CATCH_TRAINER ? gText_HackDebug_CI_Trainer : gText_HackDebug_CI_Cant, 2);
		return;
	}

	//1 行め: ほかく255 HP100% 1.5
	p = StringCopy(buf, gText_HackDebug_CI_Rate);
	p = ConvertIntToDecimalStringN(p, info->catchRate, STR_CONV_MODE_LEFT_ALIGN, 3);
	*p++ = CH_SPACE;
	p = StringCopy(p, gText_HackDebug_CI_Hp);
	p = ConvertIntToDecimalStringN(p, info->hpPercent, STR_CONV_MODE_LEFT_ALIGN, 3);
	*p++ = CH_SPACE;
	AppendTenths(p, info->otherMult10, 1);
	PrintLine(windowId, 0, buf, 2);

	if (info->count == 0)
		PrintLine(windowId, 1, gText_HackDebug_CI_NoBall, 2);

	for (i = 0; i < info->count; ++i)
	{
		const struct DebugCatchRow *row = &info->rows[i];
		const u8 *name = ItemId_GetName(row->item);

		p = buf;
		for (k = 0; k < 3 && name[k] != EOS; ++k)
			*p++ = name[k];
		*p = EOS;
		p = AppendTenths(p, row->multiplier, 1);
		*p++ = CH_SPACE;
		*p = EOS;
		p = AppendPermille(p, row->permille);
		*p++ = CH_SPACE;
		*p = EOS;
		AppendPermille(p, row->mashPermille);
		PrintLine(windowId, 1 + i, buf, 2);
	}
}

//---- スプライト ----

static bool8 CatchInfoAlive(void)
{
	u8 a, b;

	if (gNewBS == NULL || !gNewBS->hackCatchInfoSprite[0] || !gNewBS->hackCatchInfoSprite[1])
		return FALSE;

	a = gNewBS->hackCatchInfoSprite[0] - 1;
	b = gNewBS->hackCatchInfoSprite[1] - 1;
	return gSprites[a].inUse && gSprites[a].template == &sCatchInfoTemplateA
		&& gSprites[b].inUse && gSprites[b].template == &sCatchInfoTemplateB;
}

static void CatchInfoDestroy(void)
{
	u8 i;

	if (gNewBS == NULL)
		return;

	for (i = 0; i < 2; ++i)
	{
		if (gNewBS->hackCatchInfoSprite[i])
		{
			struct Sprite *s = &gSprites[gNewBS->hackCatchInfoSprite[i] - 1];

			if (s->inUse && (s->template == &sCatchInfoTemplateA || s->template == &sCatchInfoTemplateB))
				DestroySprite(s);
			gNewBS->hackCatchInfoSprite[i] = 0;
		}
	}
	FreeSpriteTilesByTag(CI_TILE_TAG);
	FreeSpritePaletteByTag(CI_PAL_TAG);
	gNewBS->hackCatchInfoSig = 0;
}

//窓（16x8 タイル・行ごとに並ぶ）のタイルを 2 枚のスプライト（8x8 タイルずつ）へ写す
static void CopyWindowToSprites(const u8 *tileData)
{
	u8 half, row;

	for (half = 0; half < 2; ++half)
	{
		struct Sprite *s = &gSprites[gNewBS->hackCatchInfoSprite[half] - 1];
		u8 *dst = (u8 *) OBJ_VRAM0 + s->oam.tileNum * 32;

		for (row = 0; row < 8; ++row)
			CpuCopy32(tileData + (row * 16 + half * 8) * 32, dst + row * 8 * 32, 8 * 32);
	}
}

static u32 InfoSig(const struct HackDebugCatchInfo *info)
{
	const u8 *b = (const u8 *) info;
	u32 h = 2166136261u;
	u32 i;

	for (i = 0; i < sizeof(*info); ++i)
		h = (h ^ b[i]) * 16777619u;
	return h ? h : 1;
}

static bool8 CatchInfoCreate(const u8 *firstTiles)
{
	struct SpriteSheet sheet = {firstTiles, 4096, CI_TILE_TAG};
	u8 a, b;

	if (LoadSpriteSheet(&sheet) == 0xFFFF)
		return FALSE;
	if (LoadSpritePalette(&sCatchInfoSpritePal) == 0xFF)
	{
		FreeSpriteTilesByTag(CI_TILE_TAG);
		return FALSE;
	}

	a = CreateSprite(&sCatchInfoTemplateA, CI_X + 32, CI_Y + 32, 0);
	b = CreateSprite(&sCatchInfoTemplateB, CI_X + 96, CI_Y + 32, 0);
	if (a >= MAX_SPRITES || b >= MAX_SPRITES)
	{
		if (a < MAX_SPRITES)
			DestroySprite(&gSprites[a]);
		if (b < MAX_SPRITES)
			DestroySprite(&gSprites[b]);
		FreeSpriteTilesByTag(CI_TILE_TAG);
		FreeSpritePaletteByTag(CI_PAL_TAG);
		return FALSE;
	}

	gNewBS->hackCatchInfoSprite[0] = a + 1;
	gNewBS->hackCatchInfoSprite[1] = b + 1;
	StartSpriteAnim(&gSprites[a], 0);
	StartSpriteAnim(&gSprites[b], 0);
	return TRUE;
}

void HackDebugCatchInfo_Tick(void)
{
	struct HackDebugCatchInfo info;
	struct WindowTemplate tmpl = {0};
	u32 sig;
	u16 windowId;
	bool8 alive;

	if (gNewBS == NULL)
		return;

	alive = CatchInfoAlive();
	if (!FlagGet(HACK_FLAG_DEBUG_CATCHINFO))
	{
		if (alive || gNewBS->hackCatchInfoSprite[0] || gNewBS->hackCatchInfoSprite[1])
			CatchInfoDestroy();
		return;
	}

	if (!alive && (gNewBS->hackCatchInfoSprite[0] || gNewBS->hackCatchInfoSprite[1]))
		CatchInfoDestroy(); //外から消された（作り直す）
	if (!alive && gBattleMainFunc != HandleTurnActionSelectionState)
		return; //戦闘の準備（登場のアニメ）が終わってから

	if (alive && ++gNewBS->hackCatchInfoTimer < CI_RECALC_FRAMES)
		return;
	gNewBS->hackCatchInfoTimer = 0;

	HackDebugCatchInfo_Calc(&info);
	sig = InfoSig(&info);
	if (alive && sig == gNewBS->hackCatchInfoSig)
		return;

	tmpl.width = 16;
	tmpl.height = 8;
	windowId = AddWindow(&tmpl);
	if (windowId == 0xFF)
		return;

	DrawInfo(windowId, &info);
	if (!alive && !CatchInfoCreate((const u8 *) GetWindowAttribute(windowId, WINDOW_TILE_DATA)))
	{
		RemoveWindow(windowId);
		gNewBS->hackCatchInfoTimer = 0;
		return;
	}
	CopyWindowToSprites((const u8 *) GetWindowAttribute(windowId, WINDOW_TILE_DATA));
	RemoveWindow(windowId);
	gNewBS->hackCatchInfoSig = sig;
}

#endif
