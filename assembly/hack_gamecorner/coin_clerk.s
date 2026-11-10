@ ゲームコーナーのコイン売りの選択肢（2026-10-15 ユーザー確定 案 c・docs/research/2026-10-15_coin-purchase-options.md）
@ 「50まい」「500まい」「1000まい」「さいだいまで」「かずを えらぶ」＋「やめる」。
@ 素のタマムシの店員の台本（CeladonCity_GameCorner_EventScript_CoinsClerk 0818509C）は、あいさつのあと 081850B9 の multichoice
@ （6F 0D 00 1B 00 の 5 バイト）で選ばせ、50 枚 1000 円と 500 枚 10000 円を 9950／9500 と決め打ちで比べていた。
@ その 5 バイトを goto（05 + この HackScr_CoinClerkMenu）に差し替える（bytereplacement 081850B9・repoints 081850BA）。
@ キンセツの店員（mauville_gamecorner.s の HackScr_MauvilleCoinsClerk）は goto 081850B9 で素の台本の続きを使うので同じ道を通る。
@ 「コインケースが ない」「いっぱい」「おかね たりない」「まいど あり」「やめる」の文と、おかね・コインの箱の更新は素の台本のものをそのまま使う。
@ 買う計算は src/hack_coin_clerk.c（枚数は Var8004 = 下 16 ビット・Var8005 = 上 16 ビット・LASTRESULT 0 = 買えた／1 = コインがいっぱい／2 = おかね不足）

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ SP_ADD_MENU, 0x25

@ 素の台本の飛び先（素の BPRJ・tools/rom/verify_coin_purchase.py が素の台本の形を突き合わせる）
.equ COIN_SCR_NO_CASE,   0x0818519F   @ 「コインケースが ないよ！」（hidemoneybox・hidecoinsbox・release まで）
.equ COIN_SCR_FULL,      0x081851AD   @ 「そんなに コイン はいらないよ！」
.equ COIN_SCR_POOR,      0x081851BB   @ 「おかね たりないよ！」
.equ COIN_SCR_THANKS,    0x08185171   @ おかねとコインの箱を更新して「まいど あり！」
.equ COIN_SCR_DECLINED,  0x08185189   @ 箱を消して release
.equ COIN_FLAG_CASE,     0x243        @ コインケースを持っている（素の台本の checkflag）

.align 2
.global HackScr_CoinClerkMenu
HackScr_CoinClerkMenu:
	setvar 0x8006 0
	loadpointer 0 gText_CoinClerk_50
	special SP_ADD_MENU
	setvar 0x8006 1
	loadpointer 0 gText_CoinClerk_500
	special SP_ADD_MENU
	setvar 0x8006 2
	loadpointer 0 gText_CoinClerk_1000
	special SP_ADD_MENU
	setvar 0x8006 3
	loadpointer 0 gText_CoinClerk_Max
	special SP_ADD_MENU
	setvar 0x8006 4
	loadpointer 0 gText_CoinClerk_Choose
	special SP_ADD_MENU
	setvar 0x8006 5
	loadpointer 0 gText_CoinClerk_Cancel
	special SP_ADD_MENU
	setvar 0x8006 12	@ 窓の幅（タイル・「かずを えらぶ」が入る）
	multichoice 13, 0, 0x24, 0	@ 0x24 = CFRU の自由な選択肢 6 つ
	setvar 0x8006 0	@ 幅を素に戻す（ほかの自由な選択肢に持ち越さない）
	compare LASTRESULT 5
	if 0x1 _goto COIN_SCR_DECLINED
	compare LASTRESULT 0x7F
	if 0x1 _goto COIN_SCR_DECLINED
	checkflag COIN_FLAG_CASE
	if 0x0 _goto COIN_SCR_NO_CASE
	compare LASTRESULT 0
	if 0x1 _goto HackScr_CoinClerk_50
	compare LASTRESULT 1
	if 0x1 _goto HackScr_CoinClerk_500
	compare LASTRESULT 2
	if 0x1 _goto HackScr_CoinClerk_1000
	compare LASTRESULT 3
	if 0x1 _goto HackScr_CoinClerk_Max
	goto HackScr_CoinClerk_Choose

HackScr_CoinClerk_50:
	setvar 0x8004 50
	setvar 0x8005 0
	goto HackScr_CoinClerk_Buy

HackScr_CoinClerk_500:
	setvar 0x8004 500
	setvar 0x8005 0
	goto HackScr_CoinClerk_Buy

HackScr_CoinClerk_1000:
	setvar 0x8004 1000
	setvar 0x8005 0
	goto HackScr_CoinClerk_Buy

@ さいだいまで: おかねとコインの上限の範囲で買えるだけ（0 枚なら断りの文）
HackScr_CoinClerk_Max:
	callasm HackCoinClerk_CalcMax
	compare LASTRESULT 0
	if 0x1 _goto HackScr_CoinClerk_Buy
	goto HackScr_CoinClerk_Refuse

@ かずを えらぶ: 上下 ±1・左右 ±10・A で決める・B でやめる（買えるのが 0 枚なら画面を開かず断りの文）
HackScr_CoinClerk_Choose:
	callasm HackCoinClerk_CalcMax
	compare LASTRESULT 0
	if 0x5 _goto HackScr_CoinClerk_Refuse
	callasm HackCoinClerk_OpenNumber
	waitstate
	compare LASTRESULT 0
	if 0x1 _goto COIN_SCR_DECLINED
	goto HackScr_CoinClerk_Buy

HackScr_CoinClerk_Buy:
	callasm HackCoinClerk_Buy
	compare LASTRESULT 0
	if 0x1 _goto COIN_SCR_THANKS
	goto HackScr_CoinClerk_Refuse

@ LASTRESULT 1 = コインがいっぱい・それ以外 = おかね不足
HackScr_CoinClerk_Refuse:
	compare LASTRESULT 1
	if 0x1 _goto COIN_SCR_FULL
	goto COIN_SCR_POOR
