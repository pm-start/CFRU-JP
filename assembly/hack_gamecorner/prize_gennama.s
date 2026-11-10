@ ゲームコーナー（タマムシ）の景品交換「どうぐ」の台に げんなま を 1 行足す（2026-10-26・patches/0868・FEAT-gennama・ユーザー決定 work/tmp/gennama/DECISIONS.md）。
@ 素の台本（0x08185E2A）は multichoice 41 番（6F 0A 00 29 00・5 バイト・けむりだま／きせきのタネ／もくたん／しんぴのしずく／きいろビードロ／いらない の 6 行）。
@ 素の表は 6 行で固定なので、その 5 バイトを goto（05 + この HackScr_PrizeItemMenu）に差し替え（bytereplacement 08185E2A・repoints 08185E2B）、
@ CFRU の自由な選択肢 0x25（gMultiChoice・7 行）に「素の 5 品の文字（素の ROM にそのまま残っている）＋げんなま＋いらない」を並べて出す。
@ 選んだ番号: 0〜4 = 素の 5 品（素の台本の続き 08185E2F へ戻る）・5 = げんなま（ここで品と値段を決めて素の「買う」共通部 08185D94 へ）・6 = いらない（5 に直して素の続きへ）・0x7F = B（そのまま）。
@ 素の共通部は VAR 0x4001 = 道具・0x4002 = 値段（u16・コイン）で買う。コインの足りない・袋がいっぱい・「いいんだね？」の文もそのまま通る。
@ 何回でも買える（旗なし）。値段は GENNAMA_PRICE（下）と strings/hack_gamecorner_prize.string の表示の数字を同じにする（tools/rom/verify_gennama.py が突き合わせる）。

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ SP_ADD_MENU, 0x25

@ 素の台本の飛び先（tools/rom/verify_gennama.py が素の台本の形を突き合わせる）
.equ PRIZE_SCR_AFTER_MENU, 0x08185E2F   @ copyvar 0x8000 LASTRESULT（素の multichoice の直後）
.equ PRIZE_SCR_BUY,        0x08185D94   @ bufferitem 0 [0x4001] → 「いいんだね？」→ checkcoins → removecoins → 道具を渡す
@ 素のメニューの文字（素の ROM の gMultiChoice 41 番の 6 行の文字列）
.equ TXT_SMOKE_BALL,   0x083DECB3
.equ TXT_MIRACLE_SEED, 0x083DEC6B
.equ TXT_CHARCOAL,     0x083DEC7D
.equ TXT_MYSTIC_WATER, 0x083DEC8F
.equ TXT_YELLOW_SHARD, 0x083DECA1
.equ TXT_CANCEL,       0x083DEB6E

.equ GENNAMA_ITEM,  0x36F
.equ GENNAMA_PRICE, 9999

.align 2
.global HackScr_PrizeItemMenu
HackScr_PrizeItemMenu:
	setvar 0x8006 0
	loadpointer 0 TXT_SMOKE_BALL
	special SP_ADD_MENU
	setvar 0x8006 1
	loadpointer 0 TXT_MIRACLE_SEED
	special SP_ADD_MENU
	setvar 0x8006 2
	loadpointer 0 TXT_CHARCOAL
	special SP_ADD_MENU
	setvar 0x8006 3
	loadpointer 0 TXT_MYSTIC_WATER
	special SP_ADD_MENU
	setvar 0x8006 4
	loadpointer 0 TXT_YELLOW_SHARD
	special SP_ADD_MENU
	setvar 0x8006 5
	loadpointer 0 gText_PrizeItem_Gennama
	special SP_ADD_MENU
	setvar 0x8006 6
	loadpointer 0 TXT_CANCEL
	special SP_ADD_MENU
	setvar 0x8006 0	@ 窓の幅は自動（素と同じ）
	multichoice 10, 0, 0x25, 0	@ 0x25 = CFRU の自由な選択肢 7 つ
	compare LASTRESULT 5
	if 0x1 _goto HackScr_PrizeItem_Gennama
	compare LASTRESULT 6
	if 0x1 _goto HackScr_PrizeItem_Cancel
	goto PRIZE_SCR_AFTER_MENU

HackScr_PrizeItem_Cancel:
	setvar LASTRESULT 5	@ 素の台本の「いらない」（5）に直す
	goto PRIZE_SCR_AFTER_MENU

HackScr_PrizeItem_Gennama:
	setvar 0x4001 GENNAMA_ITEM
	setvar 0x4002 GENNAMA_PRICE
	goto PRIZE_SCR_BUY
