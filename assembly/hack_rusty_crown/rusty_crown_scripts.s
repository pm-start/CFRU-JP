@ さびたおうかん（ITEM_RUSTY_CROWN 0x365・2026-10-16・patches/0433・src/hack_rusty_crown.c の FieldUseFunc_RustyCrown が始める）。
@ バッグで つかう → ポケモンを選ぶ → 能力を選ぶ（HP・こうげき・ぼうぎょ・とくこう・とくぼう・すばやさ）→ 今の個体値を見せて はい／いいえ → 0 にする。
@ 流れは 2 のしまの すごいとっくんの人（assembly/kanto/kanto_hyper_training.s）の ぎんのおうかんと同じ（手持ちの画面・選択肢の窓）。
@ 文は strings/hack_rusty_crown.string。能力の名前は kanto_hyper_training.string のもの（gText_KantoHyper_HP など）を使う。
@ 0x8004 = 手持ちの番号・0x8005 = 能力の番号（CFRU の個体値の並び: HP 0・こうげき 1・ぼうぎょ 2・すばやさ 3・とくこう 4・とくぼう 5）。

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ SP_CHOOSE_PARTY_MON, 0x9F
.equ SP_IS_SELECTED_MON_EGG, 0x148
.equ SP_ADD_MENU, 0x25
.equ MENU_6, 0x24

.macro rusty_item n:req, text:req
	setvar 0x8006, \n
	loadpointer 0, \text
	special SP_ADD_MENU
.endm

.align 2
.global EventScript_RustyCrown
EventScript_RustyCrown:
	lockall
RustyCrown_Script_Choose:
	msgbox gText_RustyCrown_WhichMon MSG_KEEPOPEN
	closemessage
	special SP_CHOOSE_PARTY_MON
	waitstate
	callasm HackPartyChoice_RestoreSlot                @ 0x8004 を手持ちの画面の選んだ番号に戻す（やめたら 7）・src/hack_mystery_gift.c
	compare 0x8004, 6
	if greaterorequal _goto RustyCrown_Script_End
	special SP_IS_SELECTED_MON_EGG
	compare LASTRESULT, 1
	if equal _goto RustyCrown_Script_Egg
RustyCrown_Script_ChooseStat:
	preparemsg gText_RustyCrown_WhichStat
	waitmsg
	rusty_item 0, gText_KantoHyper_HP
	rusty_item 1, gText_KantoHyper_Atk
	rusty_item 2, gText_KantoHyper_Def
	rusty_item 3, gText_KantoHyper_SpAtk
	rusty_item 4, gText_KantoHyper_SpDef
	rusty_item 5, gText_KantoHyper_Spe
	setvar 0x8006, 10
	multichoice 19, 0, MENU_6, 0
	@ 選択肢の並び（HP・こうげき・ぼうぎょ・とくこう・とくぼう・すばやさ）→ 能力の番号（HP 0・攻 1・防 2・素早 3・特攻 4・特防 5）
	compare LASTRESULT, 6
	if greaterorequal _goto RustyCrown_Script_Choose   @ B（0x7F）: 手持ちを選び直す
	copyvar 0x8005, LASTRESULT
	compare LASTRESULT, 3
	if equal _call RustyCrown_Script_StatSpAtk
	compare LASTRESULT, 4
	if equal _call RustyCrown_Script_StatSpDef
	compare LASTRESULT, 5
	if equal _call RustyCrown_Script_StatSpe
	callasm HackRustyCrown_Prepare                     @ [BUFFER1〜3] を用意して 0 にできるか（LASTRESULT = 1）
	compare LASTRESULT, 0
	if equal _goto RustyCrown_Script_NoEffect
	msgbox gText_RustyCrown_Confirm MSG_YESNO
	compare LASTRESULT, 1
	if notequal _goto RustyCrown_Script_ChooseStat
	callasm HackRustyCrown_Apply                       @ 道具を 1 つ減らせたときだけ 0 にする（LASTRESULT = 1）
	compare LASTRESULT, 1
	if notequal _goto RustyCrown_Script_NoEffect
	msgbox gText_RustyCrown_Done MSG_KEEPOPEN
	closemessage
RustyCrown_Script_End:
	releaseall
	end

RustyCrown_Script_StatSpAtk:
	setvar 0x8005, 4
	return
RustyCrown_Script_StatSpDef:
	setvar 0x8005, 5
	return
RustyCrown_Script_StatSpe:
	setvar 0x8005, 3
	return

RustyCrown_Script_Egg:
	msgbox gText_RustyCrown_Egg MSG_KEEPOPEN
	closemessage
	goto RustyCrown_Script_Choose

RustyCrown_Script_NoEffect:
	msgbox gText_RustyCrown_NoEffect MSG_KEEPOPEN
	closemessage
	goto RustyCrown_Script_ChooseStat
