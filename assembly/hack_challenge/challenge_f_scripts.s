@ 縛り F 6 本（0.63.0・src/hack_challenge_f.c・docs/research/2026-11-03_challenge-f.md）の台本。文は strings/hack_challenge_f.string。
@ ・HackScr_ChallengeF_FinaleList: 最後のバッジのメッセージの ON の行（旗 0xFEA〜0xFEF・challenge_scripts.s の ChFinale_List から call）
@ ・EventScript_ChallengeBonus: しばりボーナスのバッジのごほうび（HackChallengeF_FieldTick が始める）
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global HackScr_ChallengeF_FinaleList
HackScr_ChallengeF_FinaleList:
	checkflag 0xFEA
	if 0x1 _call ChFinaleF_Inverse
	checkflag 0xFEB
	if 0x1 _call ChFinaleF_Bonus
	checkflag 0xFEC
	if 0x1 _call ChFinaleF_Blind
	checkflag 0xFED
	if 0x1 _call ChFinaleF_Oneshot
	checkflag 0xFEE
	if 0x1 _call ChFinaleF_Rotation
	checkflag 0xFEF
	if 0x1 _call ChFinaleF_Gacha
	return

ChFinaleF_Inverse:
	msgbox gText_ChallengeF_FinaleInverse MSG_NORMAL
	return

ChFinaleF_Bonus:
	msgbox gText_ChallengeF_FinaleBonus MSG_NORMAL
	return

ChFinaleF_Blind:
	msgbox gText_ChallengeF_FinaleBlind MSG_NORMAL
	return

ChFinaleF_Oneshot:
	msgbox gText_ChallengeF_FinaleOneshot MSG_NORMAL
	return

ChFinaleF_Rotation:
	msgbox gText_ChallengeF_FinaleRotation MSG_NORMAL
	return

ChFinaleF_Gacha:
	msgbox gText_ChallengeF_FinaleGacha MSG_NORMAL
	return

@ しばりボーナス: バッジを取ったあとの ごほうび。Var8004 = 新しく取ったバッジの数（HackChallengeF_FieldTick が入れる）
.align 2
.global EventScript_ChallengeBonus
EventScript_ChallengeBonus:
	lockall
	callasm HackChallengeF_BonusGiveA + 1	@ ふしぎなアメ。LASTRESULT 1 = 渡した（STRING 1 = 道具の名前・STRING 2 = 数）
	compare LASTRESULT 1
	if 0x1 _call ChBonus_Msg
	callasm HackChallengeF_BonusGiveB + 1	@ ポイントアップ／ポイントマックス
	compare LASTRESULT 1
	if 0x1 _call ChBonus_Msg
	releaseall
	end

ChBonus_Msg:
	msgbox gText_ChallengeF_BonusGet MSG_NORMAL
	return
