@ ロジックアリーナ（役割論理＝ヤーティの専門の施設・Redmine #116・patches/0817〜0819）の受付の台本。受付は群 69 の 42（交換所）の (14,6)。
@ 流れ（1 戦ごとに完結）: 受付で話す → 見せ合い（通信対戦の選出画面を通信なしで使う）で相手の 6 匹を見て 3 匹を選ぶ → SP_SAVE_PARTY → PrepareTeam（全員 Lv50）→
@   戦闘 → 勝っても負けても SP_LOAD_PARTY。戦と戦の間は手持ちが元なので、受付でのレポートは素のままで安全（やすむ＝レポートを書いて終える）。
@ 状態: 旗 0xFD9（挑戦中）・0xFD8（全員に勝った）・変数 0x50C7（何人目か）。src/hoenn_yartie.c

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ FLAG_YARTIE_CLEARED, 0xFD8
.equ FLAG_YARTIE_ACTIVE, 0xFD9
.equ FLAG_BATTLE_FACILITY, 0x930   @ src/config.h
.equ TOWER_TRAINER, 0x398          @ CFRU の BATTLE_TOWER_SPECIAL_TID（相手は特別なトレーナーの表の 370〜376・VAR_FACILITY_TRAINER_ID）
.equ VAR_TEMP_0, 0x4000
.equ VAR_TEMP_1, 0x4001
.equ SP_HEAL, 0x0
.equ SP_SAVE_PARTY, 0x27
.equ SP_LOAD_PARTY, 0x28
.equ SP_INTRO, 0x53
.equ SP_ADD_MENU, 0x25
.equ MENU_3, 0x21
.equ SE_SAVE, 48

.align 2
.global HoennYartie_EventScript_Reception
HoennYartie_EventScript_Reception:
	lock
	faceplayer
	callasm HoennYartie_SetUpFacility
	callasm HoennFacility_RestoreIfNotRestored   @ 挑戦のチームのまま着いたときの守り（patches/0731 と同じ呼び方）
	clearflag FLAG_BATTLE_FACILITY
	checkflag FLAG_YARTIE_ACTIVE
	if 0x1 _goto YartieScr_Resume
	msgbox gText_Yartie_Welcome MSG_KEEPOPEN
YartieScr_AskStart:
	msgbox gText_Yartie_Ask MSG_YESNO
	compare LASTRESULT 0
	if equal _goto YartieScr_Bye
	callasm HoennTower_CheckIneligible
	compare 0x8004 1
	if equal _goto YartieScr_NotEnough
	callasm HoennYartie_Begin
	goto YartieScr_Next

YartieScr_Resume:
	callasm HoennYartie_BufferProgress           @ gStringVar2 = 何人目か
	msgbox gText_Yartie_WelcomeBack MSG_KEEPOPEN
	msgbox gText_Yartie_Continue MSG_YESNO
	compare LASTRESULT 1
	if equal _goto YartieScr_Next
	msgbox gText_Yartie_Quit MSG_YESNO
	compare LASTRESULT 1
	if equal _goto YartieScr_Retire
	goto YartieScr_Bye

YartieScr_NotEnough:
	msgbox gText_Yartie_NotEnough MSG_KEEPOPEN
	goto YartieScr_Done

YartieScr_Bye:
	msgbox gText_Yartie_Bye MSG_KEEPOPEN
YartieScr_Done:
	closemessage
	release
	end

YartieScr_Retire:
	callasm HoennYartie_End
	msgbox gText_Yartie_Quit_Done MSG_KEEPOPEN
	goto YartieScr_Done

@ ---- 1 戦 ----
YartieScr_Next:
	callasm HoennYartie_SetUpFacility
	callasm HoennTower_CheckIneligible
	compare 0x8004 1
	if equal _goto YartieScr_NotEnoughMidway
	callasm HoennYartie_SetOpponent              @ VAR_FACILITY_TRAINER_ID・gStringVar1 = 相手の名前・gStringVar2 = 何人目か
	msgbox gText_Yartie_Next MSG_KEEPOPEN
	closemessage
	fadescreen 1
	callasm HoennYartie_MarkSelectOffline
	callasm HoennYartie_OpenSelectScreen         @ 見せ合いの画面（通信なし・src/link_select_screen.c の offline）
	waitstate
	callasm HoennTower_PackSelection             @ 選んだ順 → 変数・0x8004 = 選んだか
	compare 0x8004 0
	if equal _goto YartieScr_Cancelled
	callasm HoennYartie_PickOpponent3            @ 相手が 6 匹から 3 匹（元の手持ちの 6 匹を見て）
	callasm HoennYartie_SetOpponent
	setflag FLAG_BATTLE_FACILITY
	special SP_SAVE_PARTY
	callasm HoennTower_PrepareTeam               @ 全員ちょうど Lv50（ロンジャの分岐）
	setvar 0x8000 0
	setvar 0x8001 1
	special SP_INTRO
	callstd MSG_NORMAL
	closemessage
	special SP_HEAL
	callasm HoennTower_SetBattleLevel
	trainerbattle9 0x9, TOWER_TRAINER, 1, HoennTowerText_Dummy, HoennTowerText_Dummy
	copyvar VAR_TEMP_0 LASTRESULT                @ 1 = 負け
	callasm HoennTower_ResetSketch
	special SP_LOAD_PARTY                        @ 勝敗にかかわらず元の手持ちへ
	clearflag FLAG_BATTLE_FACILITY
	callasm HoennTower_SetRecordLevel
	callasm HoennYartie_SetUpFacility
	compare VAR_TEMP_0 1
	if equal _goto YartieScr_Lost
	callasm HoennYartie_Advance                  @ 0x50C7 を 1 足す（LASTRESULT = 勝った数）
	copyvar VAR_TEMP_1 LASTRESULT
	msgbox gText_Yartie_Won MSG_KEEPOPEN
	callasm HoennYartie_GiveBattlePoints         @ 1 戦ごとの BP（全員に勝ったときは上乗せ）→ gStringVar1
	msgbox gText_Yartie_BP MSG_KEEPOPEN
	compare VAR_TEMP_1 7
	if equal _goto YartieScr_AllWon
	special SP_HEAL
YartieScr_AfterWinMenu:
	@ つづける／やすむ（レポートを書く）／あきらめる
	callasm HoennYartie_SetUpFacility
	setvar 0x8006, 0
	loadpointer 0, HoennTowerText_MenuGoOn
	special SP_ADD_MENU
	setvar 0x8006, 1
	loadpointer 0, HoennTowerText_MenuRest
	special SP_ADD_MENU
	setvar 0x8006, 2
	loadpointer 0, HoennTowerText_MenuRetire
	special SP_ADD_MENU
	setvar 0x8006, 7
	multichoice 20, 6, MENU_3, 1
	compare LASTRESULT, 0
	if equal _goto YartieScr_Next
	compare LASTRESULT, 1
	if equal _goto YartieScr_Rest
	goto YartieScr_AskRetire

YartieScr_Rest:
	message gText_Yartie_Saving
	waitmessage
	callasm HoennTower_SaveGame
	playse SE_SAVE
	waitse
	goto YartieScr_Done

YartieScr_AskRetire:
	msgbox gText_Yartie_Quit MSG_YESNO
	compare LASTRESULT 1
	if equal _goto YartieScr_Retire
	goto YartieScr_AfterWinMenu

YartieScr_Cancelled:
	clearflag FLAG_BATTLE_FACILITY
	goto YartieScr_Bye

YartieScr_NotEnoughMidway:
	msgbox gText_Yartie_NotEnough MSG_KEEPOPEN
	goto YartieScr_Done

YartieScr_Lost:
	callasm HoennYartie_End
	msgbox gText_Yartie_Lost MSG_KEEPOPEN
	goto YartieScr_Done

YartieScr_AllWon:
	setflag FLAG_YARTIE_CLEARED
	callasm HoennYartie_End
	msgbox gText_Yartie_Cleared MSG_KEEPOPEN
	goto YartieScr_Done
