@ しばり 縛り C（0.50.3・patches/0550・src/hack_challenge_c.c）の台本。文は strings/hack_challenge_c.string。
@ ・EventScript_ChallengeNurse: 素の看護師の台本 Common_EventScript_PkmnCenterNurse（08193E46）を call する 20 か所（repoints）の行き先。ポケセンきんし ON なら断る・OFF なら素の台本へ goto（素の台本の return で呼び元に戻る）
@ ・EventScript_ChallengeShopBan: おみせきんし ON のときの pokemart（HackChallengeC_ScrCmdPokemart が ScriptCall）。断りの文を出して return
@ ・EventScript_ChallengeWrath: ？？？の いかりのあと（HackChallengeC_FieldTick が自由なコマで 1 回）。全員ひんしの文
@ ・HackScr_ChallengeC_FinaleList: 最後のバッジの文の 縛り C の行（challenge_scripts.s の ChFinale_List から call）
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global EventScript_ChallengeNurse
EventScript_ChallengeNurse:
	checkflag 0xFBD
	if 0x1 _goto ChC_NurseRefuse
	call HackRelive_NurseTalk   @ [hack] patches/0965 段 G リライブの会話（ふだんは何も出さず return・assembly/hack_shadow_relive/relive_scripts.s）
	goto 0x08193E46

ChC_NurseRefuse:
	msgbox gText_ChallengeC_NurseRefuse MSG_NORMAL
	return

.align 2
.global EventScript_ChallengeShopBan
EventScript_ChallengeShopBan:
	msgbox gText_ChallengeC_ShopBan MSG_NORMAL
	return

.align 2
.global EventScript_ChallengeWrath
EventScript_ChallengeWrath:
	lockall
	callasm HackChallengeC_WrathKind + 1
	compare LASTRESULT 2
	if 0x1 _goto ChC_WrathHit
	compare LASTRESULT 3
	if 0x1 _goto ChC_WrathTime
	msgbox gText_ChallengeC_WrathTouch MSG_NORMAL
	releaseall
	end

ChC_WrathHit:
	msgbox gText_ChallengeC_WrathHit MSG_NORMAL
	releaseall
	end

ChC_WrathTime:
	msgbox gText_ChallengeC_WrathTime MSG_NORMAL
	releaseall
	end

@ 最後のバッジ: 縛り C の ON の行（旗 0xFBD〜0xFC1）
.align 2
.global HackScr_ChallengeC_FinaleList
HackScr_ChallengeC_FinaleList:
	checkflag 0xFBD
	if 0x1 _call ChFinaleC_CenterBan
	checkflag 0xFBE
	if 0x1 _call ChFinaleC_ShopBan
	checkflag 0xFBF
	if 0x1 _call ChFinaleC_NoRevive
	checkflag 0xFC0
	if 0x1 _call ChFinaleC_MonoType
	checkflag 0xFC1
	if 0x1 _call ChFinaleC_Random
	return

ChFinaleC_CenterBan:
	msgbox gText_ChallengeC_FinaleCenterBan MSG_NORMAL
	return

ChFinaleC_ShopBan:
	msgbox gText_ChallengeC_FinaleShopBan MSG_NORMAL
	return

ChFinaleC_NoRevive:
	msgbox gText_ChallengeC_FinaleNoRevive MSG_NORMAL
	return

ChFinaleC_MonoType:
	msgbox gText_ChallengeC_FinaleMonoType MSG_NORMAL
	return

ChFinaleC_Random:
	msgbox gText_ChallengeC_FinaleRandom MSG_NORMAL
	return