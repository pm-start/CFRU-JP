@ 縛り「ぜんぶレントラー」（patches/0609・src/hack_challenge_luxray.c）の台本。文は strings/hack_challenge_luxray.string。
@ ・HackScr_ChallengeE_FinaleList: 最後のバッジのメッセージの ON の行（旗 0xFC7・challenge_scripts.s の ChFinale_List から call）
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global HackScr_ChallengeE_FinaleList
HackScr_ChallengeE_FinaleList:
	checkflag 0xFC7
	if 0x1 _call ChFinaleE_Luxray
	return

ChFinaleE_Luxray:
	msgbox gText_ChallengeE_FinaleLuxray MSG_NORMAL
	return
