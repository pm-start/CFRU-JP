#include "defines.h"
#include "../include/coins.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/money.h"
#include "../include/script.h"
#include "../include/sound.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/string_util.h"
#include "../include/constants/songs.h"

/*
 * ゲームコーナーのコイン売りの選択肢（2026-10-15 ユーザー確定 案 c・docs/research/2026-10-15_coin-purchase-options.md）。
 * 「50まい」「500まい」「1000まい」「さいだいまで」「かずを えらぶ」。タマムシ（素の台本の multichoice 081850B9 を
 * assembly/hack_gamecorner/coin_clerk.s へ goto）とキンセツ（素の台本の続きから使っているので同じ）の両方に効く。
 *
 *  - 値段は素のまま 1 枚 20 円（50 枚 1000 円・500 枚 10000 円）。
 *  - コインの上限は scripting.c の MAX_COINS（GiveCoins の頭打ち）と同じ。この ROM は MAX_COINS_DIGITS 5 = 99,999 枚
 *    （素 FRLG は 9,999 枚）。素の台本は 9950／9500 と決め打ちで比べていたので、ここで上限から計算する。
 *  - 枚数は 65535 をこえる（99,999）ので Var8004（下 16 ビット）／Var8005（上 16 ビット）に分けて渡す。
 *  - 状態は書き換え可能な static に置けない（6.13）ので、枚数を選ぶ画面はタスクの data[] に持つ。
 *  - 枚数を選ぶ画面は素のお店の個数の画面（Task_BuyHowManyDialogue）が gShopData・買い物の窓に依存しているので流用せず、
 *    同じ操作（上下 ±1・左右 ±10・A 決定・B やめる）で新しく作った。
 *    2026-10-15 patches/0406 ユーザー要望: L＋左右 ±100・R＋左右 ±1000 を足した（窓に案内の文字は無いので足していない）。
 * 割り算は 1 か所に 1 つ（6.83: a / b と a % b を同じ関数に並べない）。
 */

#define COIN_YEN 20            //1 枚の値段（素 FRLG は 50 枚 1000 円）

#ifndef SAVE_BLOCK_EXPANSION
	#define COIN_CAP_DIGITS 4
#elif MAX_COINS_DIGITS >= 5 && MAX_COINS_DIGITS <= 9
	#define COIN_CAP_DIGITS MAX_COINS_DIGITS
#else
	#define COIN_CAP_DIGITS 4
#endif

//結果（LASTRESULT）
#define COIN_OK    0
#define COIN_FULL  1            //コインが入りきらない（「そんなに コイン はいらないよ！」）
#define COIN_POOR  2            //おかねが足りない（「おかね たりないよ！」）

#define NUM_DIGITS (COIN_CAP_DIGITS + 2)   //値段の桁（上限 × 20 は上限より 2 桁まで多い）
#define WIN_LEFT 13
#define WIN_WIDTH 15

extern const u8 gText_CoinClerk_Mai[];
extern const u8 gText_CoinClerk_Yen[];

//コインの上限（scripting.c の MAX_COINS と同じ。tools/rom/verify_coin_purchase.py が突き合わせる）
u32 HackCoinClerk_Cap(void)
{
	u32 cap = 1, i;

	for (i = 0; i < COIN_CAP_DIGITS; ++i)
		cap *= 10;
	return cap - 1;
}

//買える最大の枚数 = min(所持金 / 20, 上限 - 持っているコイン)
u32 HackCoinClerk_MaxFor(u32 money, u32 coins, u32 cap)
{
	u32 room, afford;

	if (coins >= cap)
		return 0;
	room = cap - coins;
	afford = money / COIN_YEN;
	return (afford < room) ? afford : room;
}

//amount 枚買えるか: COIN_OK / COIN_FULL（持っている + amount が上限をこえる）/ COIN_POOR（amount × 20 円に足りない）
//amount × 20 は掛け算せず money / 20 と比べる（桁あふれしない）
u8 HackCoinClerk_Verdict(u32 money, u32 coins, u32 cap, u32 amount)
{
	if (amount > cap || coins > cap - amount)
		return COIN_FULL;
	if (amount > money / COIN_YEN)
		return COIN_POOR;
	return COIN_OK;
}

static u32 GetAmountVars(void)
{
	return Var8004 | ((u32) Var8005 << 16);
}

static void SetAmountVars(u32 n)
{
	Var8004 = n & 0xFFFF;
	Var8005 = n >> 16;
}

//callasm: Var8004/Var8005 = いま買える最大の枚数。LASTRESULT = 0（1 枚以上買える）／ COIN_FULL ／ COIN_POOR（0 枚）
void HackCoinClerk_CalcMax(void)
{
	u32 money = GetMoney(&gSaveBlock1->money);
	u32 coins = GetCoins();
	u32 cap = HackCoinClerk_Cap();
	u32 max = HackCoinClerk_MaxFor(money, coins, cap);

	SetAmountVars(max);
	gSpecialVar_LastResult = (max != 0) ? COIN_OK : HackCoinClerk_Verdict(money, coins, cap, 1);
}

//callasm: Var8004/Var8005 枚を買う。LASTRESULT = COIN_OK（お金を引いてコインを足した）／ COIN_FULL ／ COIN_POOR（何も変えない）
void HackCoinClerk_Buy(void)
{
	u32 amount = GetAmountVars();
	u32 money = GetMoney(&gSaveBlock1->money);
	u8 verdict = HackCoinClerk_Verdict(money, GetCoins(), HackCoinClerk_Cap(), amount);

	if (verdict == COIN_OK && amount != 0)
	{
		GiveCoins(amount);
		RemoveMoney(&gSaveBlock1->money, amount * COIN_YEN);
	}
	gSpecialVar_LastResult = (amount == 0 && verdict == COIN_OK) ? COIN_POOR : verdict;
}

/* ---- 枚数を選ぶ画面 -------------------------------------------------------------------------------------- */

#define tWindow  data[0]
#define tNumLo   data[1]
#define tNumHi   data[2]
#define tMaxLo   data[3]
#define tMaxHi   data[4]

static u32 Get32(s16* lo)
{
	return (u16) lo[0] | ((u32) (u16) lo[1] << 16);
}

static void Put32(s16* lo, u32 v)
{
	lo[0] = v & 0xFFFF;
	lo[1] = v >> 16;
}

static void DrawNumber(u8 window, u32 n)
{
	u8 buf[NUM_DIGITS + 4];

	FillWindowPixelBuffer(window, PIXEL_FILL(1));
	ConvertIntToDecimalStringN(buf, n, STR_CONV_MODE_RIGHT_ALIGN, NUM_DIGITS);
	StringAppend(buf, gText_CoinClerk_Mai);
	AddTextPrinterParameterized(window, 2, buf, 8, 0, 0xFF, NULL);
	ConvertIntToDecimalStringN(buf, n * COIN_YEN, STR_CONV_MODE_RIGHT_ALIGN, NUM_DIGITS);
	StringAppend(buf, gText_CoinClerk_Yen);
	AddTextPrinterParameterized(window, 2, buf, 8, 16, 0xFF, NULL);
	PutWindowTilemap(window);
	CopyWindowToVram(window, COPYWIN_BOTH);
}

//上下 ±1・左右 ±10・L＋左右 ±100・R＋左右 ±1000（1〜max に収める・L と R が両方なら L）。keys には L／R の押しっぱなしも入れる
u32 HackCoinClerk_Step(u32 n, u32 max, u16 keys)
{
	u32 big = (keys & L_BUTTON) ? 100 : (keys & R_BUTTON) ? 1000 : 10;

	if (keys & DPAD_UP)
		n = (n + 1 > max) ? max : n + 1;
	if (keys & DPAD_DOWN)
		n = (n > 1) ? n - 1 : 1;
	if (keys & DPAD_RIGHT)
		n = (n + big > max) ? max : n + big;
	if (keys & DPAD_LEFT)
		n = (n > big) ? n - big : 1;
	return n;
}

static void Task_CoinClerkNumber(u8 taskId)
{
	s16* data = gTasks[taskId].data;
	u32 n = Get32(&tNumLo);
	//L／R は押しっぱなしを生のキーから読む（newAndRepeatedKeys には十字キーしか入らない）
	u32 next = HackCoinClerk_Step(n, Get32(&tMaxLo), (gMain.newAndRepeatedKeys & (DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT)) | (gMain.heldKeysRaw & (L_BUTTON | R_BUTTON)));
	bool8 decided = (gMain.newKeys & A_BUTTON) != 0;

	if (next != n)
	{
		PlaySE(SE_SELECT);
		Put32(&tNumLo, next);
		DrawNumber(tWindow, next);
		return;
	}
	if (!decided && !(gMain.newKeys & B_BUTTON))
		return;

	PlaySE(SE_SELECT);
	if (decided)
	{
		SetAmountVars(n);
		gSpecialVar_LastResult = 1;
	}
	else
		gSpecialVar_LastResult = 0;
	ClearStdWindowAndFrameToTransparent(tWindow, TRUE);
	RemoveWindow(tWindow);
	DestroyTask(taskId);
	EnableBothScriptContexts();
}

//callasm（Var8004/Var8005 = 買える最大の枚数・1 以上）→ waitstate。LASTRESULT = 1（決めた・Var8004/Var8005 に枚数）／ 0（B でやめた）
void HackCoinClerk_OpenNumber(void)
{
	struct WindowTemplate win;
	u32 max = GetAmountVars();
	u8 windowId, taskId;
	s16* data;

	win.bg = 0;
	win.tilemapLeft = WIN_LEFT;                 //画面の右上（お金の窓・コインの窓は左）
	win.tilemapTop = 1;
	win.width = WIN_WIDTH;
	win.height = 4;
	win.paletteNum = 15;
	win.baseBlock = 0x038;                      //素の選択肢の窓と同じ（その窓は閉じたあと）
	windowId = AddWindow(&win);
	DrawStdWindowFrame(windowId, FALSE);

	taskId = CreateTask(Task_CoinClerkNumber, 80);
	data = gTasks[taskId].data;
	tWindow = windowId;
	Put32(&tNumLo, 1);
	Put32(&tMaxLo, max);
	DrawNumber(windowId, 1);
}
