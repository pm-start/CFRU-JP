#include "defines.h"
#include "../include/bg.h"
#include "../include/event_data.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/palette.h"
#include "../include/region_map.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/new/ram_locs.h"

/*
 * ★ 2026-10-10 ユーザー「BW系これになおして」→「全部そちらに置き換える」・動画（マサラタウンの地名が黒い帯で出る）:
 * 地名の帯を BW の黒い上下の帯に。bw-reloaded（https://github.com/pm-start/bw-reloaded 0e7888f）の src/map_popup.c を
 * CFRU の名前で書き直したもの（元の絵は Bw-map-pops・クレジットは tools/bwreload/CREDITS_bw-reloaded.md）。
 *
 * - 素の ShowMapNamePopup（08097B00）・DismissMapNamePopup（08097CDC）・IsMapNamePopupTaskActive（08097D10）を functionrewrites で置き換える
 * - 上の帯（窓 0・y 0〜2）に地名と階、下の帯（y 17〜19）は絵だけ。30 コマ待ってから 2 画素ずつ滑り込み、120 コマ出して滑り出る
 * - 帯を滑らせるのは BG0 の縦のずらし（HBlank で 80 行目までと 160 行目からは +、間は −）。前の HBlank の処理は残して先に呼ぶ
 * - 地名は GetMapName（ホウエン・シンオウ・キタカミの読み替えは中の HoennTownMapNameHook がする）
 * - 状態はタスクの data[]（6.13）。窓は DMA の写しが終わってから外す
 */

extern const u8 gBwrPopupPrimary[];
extern const u8 gBwrPopupSecondary[];
extern const u8 gBwrPopupPal[];
extern const u8 gText_BwrPopupRooftop[];

#define QUEST_LOG_STATE (*(vu8 *) 0x0203AD72)   //素の gQuestLogState（2・3 は ふりかえり の再生中）
#define FLAG_HIDE_MAP_NAME_POPUP 0x4000

enum { STATE, TIMER, OFFSET, RESHOW, PRIMARY, SECONDARY, PAL_FADE, WINDOWS, OLD_HBLANK_LO, OLD_HBLANK_HI, OLD_IE, OLD_DISPSTAT, HOOKED,
	PRINTED, PRINT_DONE, PUT };   //PRINTED・PRINT_DONE・PUT は 2026-11-04 に足した（地名を DELAY の間に 1 文字ずつ描く・窓を画面に載せたか）
enum { DELAY, PRINT, SLIDE_IN, SHOWN, SLIDE_OUT, CLEAR, FREE };   //PRINT は 2026-11-02 に足した（帯を作る仕事を 2 コマに分ける）

//タスクの 1 回 = 1 tick（ふつうは 1 コマ・いどうそくど が速いと 1 コマに 2 回）
#define POPUP_TASK_PRIORITY 90
#define POPUP_DELAY_TICKS   30    //地図に入ってから帯を出すまで待つ
#define POPUP_SHOWN_TICKS   120   //止まっている長さ
#define POPUP_SLIDE_STEP    2     //1 tick に滑る画素
#define POPUP_HIDDEN_OFFSET 24    //隠れているときの縦のずれ（帯の高さ 3 マス = 24 画素）
#define POPUP_PAL_NUM       14    //帯のパレット
#define POPUP_TILES_W       30    //帯の幅（マス）
#define POPUP_TILES_H       3     //帯の高さ（マス）
#define POPUP_BAND_BYTES    (POPUP_TILES_W * POPUP_TILES_H * 32)   //帯 1 本の絵（4bpp・マス × 32 バイト）
#define POPUP_BASE_PRIMARY   1    //窓のタイルの置き場（マス 1〜90）
#define POPUP_BASE_SECONDARY 91   //（マス 91〜180）
#define POPUP_BOTTOM_TOP     17   //下の帯の位置（マス）
#define WINDOW_NONE         0xFF  //AddWindow が窓を作れなかった印

/*
 * ★ 2026-11-04 ユーザー「リファクタまたは最適化」（PERF-map-name-popup）: 帯を出す前の仕事を、まだ画面に出ない DELAY の待ちの間に散らす。
 *   計測（mGBA・走査線の数・割り込みを含む）で、PRINT の 1 コマ 88 本のうち字を描く AddTextPrinterParameterized が 69 本、窓を作る
 *   DELAY の最後が 18 本、CLEAR が 20 本だった。窓の作成と字の描画は画面に載せる前（PutWindowTilemap の前）なら見えないので、
 *   - POPUP_T_WINDOWS tick 目に窓を作り、次の 2 tick で帯の絵を 1 本ずつ写す
 *   - POPUP_T_PRINT tick 目から 1 tick に 1 文字ずつ描く（字ごとに x を GetStringWidth で進める＝1 回で描くのと同じ絵）
 *   - 30 tick 目に（描き終わっていれば）パレットを決めて PRINT へ＝画面に出るタイミングは前と同じ。長い地名で描き終わらないときだけ待つ
 *   - 画面に載せていない窓（PUT が FALSE）は消す（ClearWindowTilemap）必要が無い。消すときの 2 本は 1 tick のまま
 *     （CLEAR は 20 本ほどで軽く、2 tick に分けると HBlank が外れるのが 1 tick 遅れる＝BG0 のほかの字が 1 tick 余計にずれるため）
 */
#define POPUP_T_WINDOWS     8     //この tick に窓を 2 つ作る（9・10 tick 目に絵を写す）
#define POPUP_T_PRINT       11    //この tick から 1 文字ずつ描く
#define POPUP_TEXT_X        8     //地名を描く位置（窓の中・画素）
#define POPUP_TEXT_Y        2
#define POPUP_FONT          2

static void Task_BwMapPopup(u8 taskId);
static void HBlankCB_BwMapPopup(void);

#ifdef HACK_PROFILE   //計測用のビルドだけ: 各状態にかかった走査線の数の最大を 0x0203F800 + 4 * (374〜380・381〜383・365〜369) に（tools/rom/profile_map_popup.py が読む）
static u16 ProfLines(u16 t0)
{
	u16 v = REG_VCOUNT & 0xFF;
	return v >= t0 ? v - t0 : v + 228 - t0;
}
#define PROF_SLOT(i, expr) do { u16 t0_ = REG_VCOUNT & 0xFF; expr; { u32 *q_ = (u32 *) 0x0203F800 + (i); u16 d_ = ProfLines(t0_); if (d_ > *q_) *q_ = d_; } } while (0)
#else
#define PROF_SLOT(i, expr) expr
#endif


static const struct WindowTemplate sPrimaryWindow = {.bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = POPUP_TILES_W, .height = POPUP_TILES_H, .paletteNum = POPUP_PAL_NUM, .baseBlock = POPUP_BASE_PRIMARY};
static const struct WindowTemplate sSecondaryWindow = {.bg = 0, .tilemapLeft = 0, .tilemapTop = POPUP_BOTTOM_TOP, .width = POPUP_TILES_W, .height = POPUP_TILES_H, .paletteNum = POPUP_PAL_NUM, .baseBlock = POPUP_BASE_SECONDARY};

static IntrCallback OldHBlank(const s16 *data)
{
	return (IntrCallback) ((u32) (u16) data[OLD_HBLANK_LO] | ((u32) (u16) data[OLD_HBLANK_HI] << 16));
}

/*
 * ★ 2026-11-02 ユーザー「マップ切り替え時のマップ名表示が重い」（docs/research/2026-11-02_map-popup-perf.md）:
 *   前は帯が出ている間ずっと、走査線 1 本ごと（1 コマ 228 回）に FindTaskIdByFunc でタスクを探し直して BG0VOFS を書いていた。
 *   ・見える行の値が変わるのは 80 行目の HBlank（81 行目から −）と 227 行目の HBlank（次のコマの 0 行目から +）だけなので、
 *     前の HBlank の処理が無いとき（ふつうの野外）は その 2 行だけタスクを探して書く（HBlankCB_BwMapPopup・見える値は前と同じ）。
 *   ・前の HBlank の処理があるときは、その処理を毎行呼ぶ必要がある（呼び先はタスクの data にしか無い・static は使えない＝6.13）ので
 *     前と同じ毎行の処理（HBlankCB_BwMapPopupChain）のまま。
 *   ・帯が止まっている間（SHOWN・120 コマ）は上下とも 0 なので HBlank を外す（Task_BwMapPopup の SLIDE_IN の終わり・SLIDE_OUT の頭）。
 */
#define POPUP_LINE_MID 80    //この行の HBlank で 81 行目からを − にする（前の「line < 80 は +」と同じ境目）
#define POPUP_LINE_TOP 227   //この行の HBlank で 次のコマの 0 行目からを + にする（前は 160〜227 行で毎行 + を書いていた）
#define POPUP_LINE_BOTTOM 160   //この行からは また + （下の帯の行）

//行 line の BG0VOFS（上の帯と下の帯は +OFFSET・間は −OFFSET）。InstallHBlank・毎行の処理が同じ式を使う
//（関数にすると型の変換で機械語が変わるので、式のまま置き換える macro）
#define POPUP_VOFS(data, line) (((line) < POPUP_LINE_MID || (line) >= POPUP_LINE_BOTTOM) ? (data)[OFFSET] : (512 - (data)[OFFSET]) & 511)

static void HBlankCB_BwMapPopupChain(void);

static bool8 IsPopupHBlank(IntrCallback cb)
{
	return cb == HBlankCB_BwMapPopup || cb == HBlankCB_BwMapPopupChain;
}

static void InstallHBlank(s16 *data)
{
	u32 previous = (u32) gMain.hblankCallback;
	data[OLD_HBLANK_LO] = (s16) previous;
	data[OLD_HBLANK_HI] = (s16) (previous >> 16);
	data[OLD_IE] = REG_IE & INTR_FLAG_HBLANK;
	data[OLD_DISPSTAT] = REG_DISPSTAT & DISPSTAT_HBLANK_INTR;
	data[HOOKED] = TRUE;
	SetHBlankCallback(previous == 0 ? HBlankCB_BwMapPopup : HBlankCB_BwMapPopupChain);
	REG_DISPSTAT |= DISPSTAT_HBLANK_INTR;
	REG_IE |= INTR_FLAG_HBLANK;
	//付けたコマの残りの行も前と同じ値に（次の 80／227 行目を待つと、そのあいだ帯が 1 コマずれて見えることがある）
	{
		u16 line = REG_VCOUNT & 0xFF;
		REG_BG0VOFS = POPUP_VOFS(data, line);
	}
}

static void RestoreHBlank(s16 *data)
{
	if (data[HOOKED] && IsPopupHBlank(gMain.hblankCallback))
	{
		SetHBlankCallback(OldHBlank(data));
		if (!data[OLD_IE])
			REG_IE &= ~INTR_FLAG_HBLANK;
		if (!data[OLD_DISPSTAT])
			REG_DISPSTAT &= ~DISPSTAT_HBLANK_INTR;
		SetGpuReg(REG_OFFSET_BG0VOFS, 0);
		REG_BG0VOFS = 0;   //★ 2026-11-02 SHOWN で外すとき、最後に書いた −2 が次の VBlank まで画面に残らないように
		ChangeBgY(0, 0, 0);
	}
	data[HOOKED] = FALSE;
}

//タスクが見つからないときの片付け（前の HBlankCB_BwMapPopup の頭と同じ）
static bool8 PopupTaskGone(u8 taskId)
{
	if (taskId < NUM_TASKS)
		return FALSE;
	//★ 2026-10-10 帯が出ている間にタスクごと消された（BW のスタートメニューなど全画面の画面は開くとき ResetTasks する＝
	//  RestoreHBlank が呼ばれない）。BG0 の縦のずれが最後に書いた値のまま残り、メニューの文字がずれた（ユーザーの動画・
	//  地図に入った直後にメニュー）。画面側の値（SetGpuReg のバッファ）に戻し、この HBlank の処理を外す
	REG_BG0VOFS = GetGpuReg(REG_OFFSET_BG0VOFS);
	if (IsPopupHBlank(gMain.hblankCallback))
		SetHBlankCallback(NULL);
	return TRUE;
}

//前の HBlank の処理が無いとき: 80 行目と 227 行目だけ
static void HBlankCB_BwMapPopup(void)
{
	u16 line = REG_VCOUNT & 0xFF;
	u8 taskId;
	s16 *data;

	if (line != POPUP_LINE_MID && line != POPUP_LINE_TOP)
		return;
	taskId = FindTaskIdByFunc(Task_BwMapPopup);
	if (PopupTaskGone(taskId))
		return;
	data = gTasks[taskId].data;
	REG_BG0VOFS = line == POPUP_LINE_MID ? (512 - data[OFFSET]) & 511 : data[OFFSET];
}

//前の HBlank の処理があるとき: 前と同じ毎行の処理
static void HBlankCB_BwMapPopupChain(void)
{
	u8 taskId = FindTaskIdByFunc(Task_BwMapPopup);
	s16 *data;
	IntrCallback previous;
	u16 line;

	if (PopupTaskGone(taskId))
		return;
	data = gTasks[taskId].data;
	previous = OldHBlank(data);
	if (previous != NULL && !IsPopupHBlank(previous))
		previous();
	line = REG_VCOUNT & 0xFF;
	REG_BG0VOFS = POPUP_VOFS(data, line);
}

//帯に出す文字（色の指定 9 バイト＋地名＋階）を作って、長さ（終わりの EOS の位置）を返す。毎回同じ文字になる（gMapHeader だけで決まる）
static u32 BuildName(u8 *text)
{
	u32 i;
	s8 floor;

	text[0] = EXT_CTRL_CODE_BEGIN; text[1] = 1; text[2] = 2;   //字の色 2（白）
	text[3] = EXT_CTRL_CODE_BEGIN; text[4] = 2; text[5] = 0;   //地の色 0（透明）
	text[6] = EXT_CTRL_CODE_BEGIN; text[7] = 3; text[8] = 1;   //影の色 1
	PROF_SLOT(366, GetMapName(text + 9, gMapHeader.regionMapSectionId, 0));
	for (i = 9; i < 80 && text[i] != EOS; ++i)
		;
	floor = (s8) gMapHeader.floorNum;
	if (floor != 0)
	{
		text[i++] = 0;   //空白
		if (floor == 127)
		{
			const u8 *s = gText_BwrPopupRooftop;
			while (*s != EOS && i < 101)
				text[i++] = *s++;
		}
		else
		{
			u32 tens = 0;
			if (floor < 0)
			{
				text[i++] = 0xBC;   //B（地下）
				floor = -floor;
			}
			if (floor > 99)
				floor = 99;
			while (floor >= 10)
			{
				floor -= 10;
				++tens;
			}
			if (tens)
				text[i++] = 0xA1 + tens;
			text[i++] = 0xA1 + floor;
			text[i++] = 0xC0;   //F
		}
	}
	text[i] = EOS;
	return i;
}

//地名を 1 文字（地名・階の字は 1 バイト 1 文字で、制御の符号は入らない）描く。x は前の字までの幅だけ進める。全部描いたら TRUE
static bool8 PrintNameStep(s16 *data)
{
	u8 text[104];
	u8 piece[12];
	u32 len = BuildName(text);
	u32 pos = 9 + (u32) data[PRINTED];
	s32 x;
	u32 k;

	if (pos >= len)
		return TRUE;
	for (k = 0; k < 9; ++k)
		piece[k] = text[k];   //色の指定
	piece[9] = text[pos];
	piece[10] = EOS;
	text[pos] = EOS;
	x = POPUP_TEXT_X + GetStringWidth(POPUP_FONT, text, -1);   //前の字までの幅（頭の色の指定は幅に入らない）
	PROF_SLOT(367, AddTextPrinterParameterized(data[PRIMARY], POPUP_FONT, piece, x, POPUP_TEXT_Y, TEXT_SPEED_FF, NULL));
	++data[PRINTED];
	return pos + 1 >= len;
}

//窓を 2 つ作る（作れなければ FALSE）。絵は次の 2 tick で写す（CopyBand）
static bool8 CreateWindows(s16 *data)
{
	u8 a = AddWindow(&sPrimaryWindow), b;

	if (a == WINDOW_NONE)
		return FALSE;
	b = AddWindow(&sSecondaryWindow);
	if (b == WINDOW_NONE)
	{
		RemoveWindow(a);
		return FALSE;
	}
	data[PRIMARY] = a;
	data[SECONDARY] = b;
	data[WINDOWS] = TRUE;
	return TRUE;
}

//帯の絵を窓へ写す（上の帯・下の帯を別の tick で）
static void CopyBand(u8 windowId, const u8 *band)
{
	PROF_SLOT(368, CopyToWindowPixelBuffer(windowId, band, POPUP_BAND_BYTES, 0));
}

//★ 2026-10-10 ユーザーの動画（そらをとぶで グレンじま に着くと下の帯が白・上の帯が透けた）: 帯を出すのは 30 コマ待ってから
//  なので、素と同じ「呼ばれたときの引数」で決めると暗転が終わったあとに暗転前の入れ物（Unfaded）だけへ書いて画面に出なかった。
//  いま暗転しているかで決める（暗転中は終わりに写されるので Unfaded だけ・暗転していなければ両方）
//  （窓を早く作るようにしても、これは 30 tick 目＝前と同じ時点で決める）
static void LoadBandPalette(const s16 *data)
{
	u32 i;

	if (data[PAL_FADE] || !gPaletteFade->active)
		LoadPalette(gBwrPopupPal, POPUP_PAL_NUM * 16, 32);
	else
		for (i = 0; i < 16; ++i)
			gPlttBufferUnfaded[POPUP_PAL_NUM * 16 + i] = ((const u16 *) gBwrPopupPal)[i];
}

//★ 2026-11-02 前は CreateWindows と同じコマで地名を描いていた（地図に入って 30 コマ目の山）。画面に載せる・VRAM へ送るのは次のコマに分ける
static void PrintAndShow(s16 *data)
{
	PROF_SLOT(382, PutWindowTilemap(data[PRIMARY]));
	PROF_SLOT(382, PutWindowTilemap(data[SECONDARY]));
	data[PUT] = TRUE;
	PROF_SLOT(383, CopyWindowToVram(data[PRIMARY], 3));
	PROF_SLOT(383, CopyWindowToVram(data[SECONDARY], 3));
	PROF_SLOT(365, InstallHBlank(data));
}

static void Task_BwMapPopup(u8 taskId)
{
	s16 *data = gTasks[taskId].data;
#ifdef HACK_PROFILE
	u16 profT0 = REG_VCOUNT & 0xFF;
	u8 profState = data[STATE];
#endif

	switch (data[STATE])
	{
	case DELAY:
		if (data[TIMER] < POPUP_DELAY_TICKS)
			++data[TIMER];   //字が描き終わるまでは POPUP_DELAY_TICKS のまま待つ
		if (data[TIMER] == POPUP_T_WINDOWS)
		{
			if (!CreateWindows(data))
			{
				DestroyTask(taskId);
				return;
			}
		}
		else if (data[TIMER] == POPUP_T_WINDOWS + 1)
			CopyBand(data[PRIMARY], gBwrPopupPrimary);
		else if (data[TIMER] == POPUP_T_WINDOWS + 2)
			CopyBand(data[SECONDARY], gBwrPopupSecondary);
		else if (data[TIMER] >= POPUP_T_PRINT && !data[PRINT_DONE])
			PROF_SLOT(369, data[PRINT_DONE] = PrintNameStep(data));
		if (data[TIMER] >= POPUP_DELAY_TICKS && data[PRINT_DONE])
		{
			LoadBandPalette(data);
			data[TIMER] = 0;
			data[STATE] = PRINT;
		}
		break;
	case PRINT:
		PrintAndShow(data);
		data[STATE] = SLIDE_IN;
		break;
	case SLIDE_IN:
		if (IsDma3ManagerBusyWithBgCopy())
			break;
		data[OFFSET] -= POPUP_SLIDE_STEP;
		if (data[OFFSET] <= 0)
		{
			data[OFFSET] = 0;
			data[TIMER] = 0;
			data[STATE] = SHOWN;
			RestoreHBlank(data);   //★ 2026-11-02 止まっている間は上下とも 0＝HBlank は要らない（SLIDE_OUT でまた付ける）
		}
		break;
	case SHOWN:
		if (++data[TIMER] >= POPUP_SHOWN_TICKS)
			data[STATE] = SLIDE_OUT;
		break;
	case SLIDE_OUT:
		if (!data[HOOKED] && data[WINDOWS] && data[OFFSET] < POPUP_HIDDEN_OFFSET)
			InstallHBlank(data);   //SHOWN で外した HBlank を付け直す（帯が無い・もう隠れているときは付けない）
		data[OFFSET] += POPUP_SLIDE_STEP;
		if (data[OFFSET] >= POPUP_HIDDEN_OFFSET)
		{
			data[OFFSET] = POPUP_HIDDEN_OFFSET;
			data[STATE] = CLEAR;
		}
		break;
	case CLEAR:
		if (data[PUT])
		{
			ClearWindowTilemap(data[PRIMARY]);
			ClearWindowTilemap(data[SECONDARY]);
			CopyWindowToVram(data[PRIMARY], 1);
			CopyWindowToVram(data[SECONDARY], 1);
		}
		data[STATE] = FREE;
		break;
	case FREE:
		if (IsDma3ManagerBusyWithBgCopy())
			break;
		if (data[WINDOWS])
		{
			RemoveWindow(data[PRIMARY]);
			RemoveWindow(data[SECONDARY]);
			data[WINDOWS] = FALSE;
		}
		data[PRINTED] = 0;
		data[PRINT_DONE] = FALSE;
		data[PUT] = FALSE;
		RestoreHBlank(data);
		if (data[RESHOW])
		{
			data[RESHOW] = FALSE;
			data[TIMER] = 0;
			data[OFFSET] = POPUP_HIDDEN_OFFSET;
			data[STATE] = DELAY;
		}
		else
			DestroyTask(taskId);
		break;
	}
#ifdef HACK_PROFILE
	{
		u32 *q = (u32 *) 0x0203F800 + 374 + profState;
		u16 d = ProfLines(profT0);
		if (d > *q)
			*q = d;
	}
#endif
}

//functionrewrites: ShowMapNamePopup 08097B00
void BwShowMapNamePopup(bool32 palIntoFadedBuffer)
{
	u8 taskId;

	if (FlagGet(FLAG_HIDE_MAP_NAME_POPUP) || QUEST_LOG_STATE == 2 || QUEST_LOG_STATE == 3)
		return;
	taskId = FindTaskIdByFunc(Task_BwMapPopup);
	if (taskId < NUM_TASKS)
	{
		s16 *data = gTasks[taskId].data;
		data[RESHOW] = TRUE;
		data[PAL_FADE] = palIntoFadedBuffer != 0;
		if (data[STATE] < SLIDE_OUT)
			data[STATE] = SLIDE_OUT;
	}
	else
	{
		taskId = CreateTask(Task_BwMapPopup, POPUP_TASK_PRIORITY);
		if (taskId >= NUM_TASKS)
			return;
		gTasks[taskId].data[OFFSET] = POPUP_HIDDEN_OFFSET;
		gTasks[taskId].data[PAL_FADE] = palIntoFadedBuffer != 0;
	}
}

//functionrewrites: DismissMapNamePopup 08097CDC
void BwDismissMapNamePopup(void)
{
	u8 taskId = FindTaskIdByFunc(Task_BwMapPopup);
	if (taskId < NUM_TASKS)
	{
		gTasks[taskId].data[RESHOW] = FALSE;
		if (gTasks[taskId].data[STATE] < CLEAR)
			gTasks[taskId].data[STATE] = CLEAR;
	}
}

//functionrewrites: IsMapNamePopupTaskActive 08097D10
bool8 BwIsMapNamePopupTaskActive(void)
{
	return FindTaskIdByFunc(Task_BwMapPopup) < NUM_TASKS;
}
