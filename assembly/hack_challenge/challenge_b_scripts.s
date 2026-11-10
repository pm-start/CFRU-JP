@ しばり B（#118・0.50.3・src/hack_challenge_b.c）の台本。文は strings/hack_challenge_b.string。
@ ・HackScr_ChallengeB_FinaleList: 最後のバッジの台本（challenge_scripts.s の EventScript_ChallengeFinale）から call。ON の しばり B ごとに 1 行
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global HackScr_ChallengeB_FinaleList
HackScr_ChallengeB_FinaleList:
	checkflag 0xFB5
	if 0x1 _call ChbFinale_LevelCap
	checkflag 0xFB6
	if 0x1 _call ChbFinale_NoSwitch
	checkflag 0xFB7
	if 0x1 _call ChbFinale_NoWildExp
	checkflag 0xFB8
	if 0x1 _call ChbFinale_DoubleDmg
	checkflag 0xFB9
	if 0x1 _call ChbFinale_NoFly
	return

ChbFinale_LevelCap:
	msgbox gText_Challenge_FinaleLevelCap MSG_NORMAL
	return

ChbFinale_NoSwitch:
	msgbox gText_Challenge_FinaleNoSwitch MSG_NORMAL
	return

ChbFinale_NoWildExp:
	msgbox gText_Challenge_FinaleNoWildExp MSG_NORMAL
	return

ChbFinale_DoubleDmg:
	msgbox gText_Challenge_FinaleDoubleDmg MSG_NORMAL
	return

ChbFinale_NoFly:
	msgbox gText_Challenge_FinaleNoFly MSG_NORMAL
	return
