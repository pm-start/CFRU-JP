@ 通信対戦の受付: 「どの ルールで たたかいますか？」（Lv50／Lv100／Lv200）。Redmine #50・patches/0672
@ 素の日本語版のケーブルクラブの受付の台本（081A2994 のモード選択）の、シングル（081A29F5）とダブル（081A2A0E）が
@ `setvar 0x8004, n; goto 081A2A32`（セーブの確認 → 通信 → コロシアムへ）で進む所の goto の語（081A29FB・081A2A14）を、
@ repoints でここへ向ける。マルチ（081A2A27）は触らない（選出画面が無いのでルールも無い）。
@ 選んだ値は VAR_LINK_BATTLE_RULE（0=Lv50・1=Lv100・2=Lv200）に入れる。選出画面（src/link_select_screen.c）が
@ 親機の値を子機へ送って、両方その値のレベルにそろえる。
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ VAR_LINK_BATTLE_RULE, 0x50C5
.equ SP_ADD_MENU, 0x25
.equ MENU_4, 0x22
.equ CABLE_CLUB_MODE_MENU, 0x081A2994        @ 素: 「どの たいせんに しますか？」のモード選択
.equ CABLE_CLUB_TRY_ENTER_COLOSSEUM, 0x081A2A32  @ 素: セーブの確認 → TryBattleLinkup → コロシアムへ

.align 2
.global HackLinkRule_Select
HackLinkRule_Select:
	message gText_HackLinkRule_Ask
	waitmessage
	setvar 0x8006, 0
	loadpointer 0, gText_HackLinkRule_Lv50
	special SP_ADD_MENU
	setvar 0x8006, 1
	loadpointer 0, gText_HackLinkRule_Lv100
	special SP_ADD_MENU
	setvar 0x8006, 2
	loadpointer 0, gText_HackLinkRule_Lv200
	special SP_ADD_MENU
	setvar 0x8006, 3
	loadpointer 0, gText_HackLinkRule_Cancel
	special SP_ADD_MENU
	setvar 0x8006, 17	@ 窓の幅（CFRU は選択肢を出す瞬間の 0x8006 を幅にする）
	multichoice 12, 0, MENU_4, 0
	compare LASTRESULT, 3
	if lessthan _goto HackLinkRule_Chosen
	goto CABLE_CLUB_MODE_MENU		@ やめる／B ボタン → モード選択へ戻る

HackLinkRule_Chosen:
	copyvar VAR_LINK_BATTLE_RULE, LASTRESULT
	goto CABLE_CLUB_TRY_ENTER_COLOSSEUM
