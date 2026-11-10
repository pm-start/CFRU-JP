@ 縛り「ふういんルーレット」（#123・src/hack_fuuin.c）の台本。文は strings/hack_fuuin.string。
@ ・EventScript_FuuinSpin: レベルアップの未処理があるときフィールドで始まる（HackFuuin_FieldTick）。ルーレット 1 回（callasm HackFuuin_Spin）
@ ・EventScript_FuuinReseal: 封印された子が手持ちに戻っていたのでボックスへ戻したあと（名前は gStringVar1）
@ ・HackScr_ChallengeD_FinaleList: 最後のバッジのメッセージの ON の行
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global EventScript_FuuinSpin
EventScript_FuuinSpin:
	lockall
	msgbox gText_Fuuin_Spin MSG_NORMAL
	callasm HackFuuin_Spin + 1	@ LASTRESULT: 0 空振り / 1 ポケモン / 2 どうぐ / 3 わざ（名前は gStringVar1）
	compare LASTRESULT 1
	if 0x1 _goto Fuuin_Mon
	compare LASTRESULT 2
	if 0x1 _goto Fuuin_Item
	compare LASTRESULT 3
	if 0x1 _goto Fuuin_Move
	msgbox gText_Fuuin_Miss MSG_NORMAL
	releaseall
	end

Fuuin_Mon:
	msgbox gText_Fuuin_Mon MSG_NORMAL
	releaseall
	end

Fuuin_Item:
	msgbox gText_Fuuin_Item MSG_NORMAL
	releaseall
	end

Fuuin_Move:
	msgbox gText_Fuuin_Move MSG_NORMAL
	releaseall
	end

.align 2
.global EventScript_FuuinReseal
EventScript_FuuinReseal:
	lockall
	msgbox gText_Fuuin_Reseal MSG_NORMAL
	releaseall
	end

.align 2
.global HackScr_ChallengeD_FinaleList
HackScr_ChallengeD_FinaleList:
	checkflag 0xFC5
	if 0x1 _call ChFinaleD_Fuuin
	return

ChFinaleD_Fuuin:
	msgbox gText_Fuuin_Finale MSG_NORMAL
	return