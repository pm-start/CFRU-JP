@ しばり（#118・2026-10-16・src/hack_challenge.c）の台本。文は strings/hack_challenge.string。
@ ・EventScript_ChallengeCenterEntry: ポケモンセンターに入った直後（HackChallenge_FieldTick）。りょうきん 10000 えん／払えなければ ？？？／スイッチの ？？？
@ ・HackScr_Challenge_PcGate: パソコンを使うとき（overworld.c の [MB_PC] と EventScript_BoxLink から call）。りょうきん 50000 えん／払えなければ ？？？
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

@ HackChallenge_CenterCheck の LASTRESULT: 0 何もない / 1 りょうきんを払った / 2 払った＋スイッチ / 3 払えない（罰金）/ 4 スイッチだけ

.align 2
.global EventScript_ChallengeCenterEntry
EventScript_ChallengeCenterEntry:
	lockall
	callasm HackChallenge_CenterCheck + 1
	compare LASTRESULT 1
	if 0x1 _goto ChCenter_Paid
	compare LASTRESULT 2
	if 0x1 _goto ChCenter_PaidThenBattle
	compare LASTRESULT 3
	if 0x1 _goto ChCenter_CantPay
	compare LASTRESULT 4
	if 0x1 _goto ChCenter_Battle
	releaseall
	end

ChCenter_Paid:
	msgbox gText_Challenge_PaidCenter MSG_NORMAL
	releaseall
	end

ChCenter_PaidThenBattle:
	msgbox gText_Challenge_PaidCenter MSG_NORMAL
	goto ChCenter_Battle

ChCenter_CantPay:
	msgbox gText_Challenge_CantPayCenter MSG_NORMAL
	goto ChCenter_Battle

ChCenter_Battle:
	call FollowerPositionFixScript
	msgbox gText_Challenge_Appear MSG_NORMAL
	closemessage
	callasm HackChallenge_StartLentrantBattle + 1
	waitstate
	releaseall
	end

@ パソコンを使う前に call する。LASTRESULT = 0 でもどる（払った／しばりなし）。払えないときは ？？？の戦いを始めて 台本を終える
.align 2
.global HackScr_Challenge_PcGate
HackScr_Challenge_PcGate:
	callasm HackChallenge_PcCheck + 1
	compare LASTRESULT 0
	if 0x1 _goto ChPc_Go
	compare LASTRESULT 1
	if 0x1 _goto ChPc_Paid
	lockall
	msgbox gText_Challenge_CantPayPc MSG_NORMAL
	call FollowerPositionFixScript
	msgbox gText_Challenge_Appear MSG_NORMAL
	closemessage
	callasm HackChallenge_StartLentrantBattle + 1
	waitstate
	releaseall
	end

ChPc_Paid:
	lockall
	msgbox gText_Challenge_PaidPc MSG_NORMAL
	releaseall
	setvar LASTRESULT 0
	return

ChPc_Go:
	return

@ 素のパソコンの台本（overworld.c の [MB_PC] の元の値）の前にりょうきんを取る
.align 2
.global HackScr_Challenge_Pc
HackScr_Challenge_Pc:
	call HackScr_Challenge_PcGate
	goto 0x08194221
@ 最後のバッジ（しばりをずっと ON にしたまま・HackChallenge_FieldTick の FinaleTick が始める・1 回だけ）
.align 2
.global EventScript_ChallengeFinale
EventScript_ChallengeFinale:
	lockall
	msgbox gText_Challenge_Finale1 MSG_NORMAL
	callasm HackChallenge_FinaleCount + 1	@ gStringVar1 = ずっと ON の しばりの数・LASTRESULT 1 = 5 つ全部
	compare LASTRESULT 1
	if 0x1 _goto ChFinale_All
	msgbox gText_Challenge_FinaleCount MSG_NORMAL
	goto ChFinale_List

ChFinale_All:
	msgbox gText_Challenge_FinaleAll MSG_NORMAL

ChFinale_List:
	checkflag 0xFA4
	if 0x1 _call ChFinale_Items
	checkflag 0xFA5
	if 0x1 _call ChFinale_Held
	checkflag 0xFA6
	if 0x1 _call ChFinale_Center
	checkflag 0xFA7
	if 0x1 _call ChFinale_Pc
	checkflag 0xFA8
	if 0x1 _call ChFinale_Lentrant
	checkflag 0xFAD
	if 0x1 _call ChFinale_Death
	checkflag 0xFAE
	if 0x1 _call ChFinale_FirstOnly
	checkflag 0xFAF
	if 0x1 _call ChFinale_Nickname
	call HackScr_ChallengeB_FinaleList	@ patches/0540 しばり B（assembly/hack_challenge/challenge_b_scripts.s）
	call HackScr_ChallengeC_FinaleList	@ 縛り C の ON の行（patches/0550・challenge_c_scripts.s）
	call HackScr_ChallengeD_FinaleList	@ ふういんルーレットの ON の行（patches/0560・fuuin_scripts.s）
	call HackScr_ChallengeE_FinaleList	@ ぜんぶレントラーの ON の行（patches/0609・luxray_scripts.s）
	call HackScr_ChallengeF_FinaleList	@ 縛り F 6 本の ON の行（0.63.0・challenge_f_scripts.s）
	msgbox gText_Challenge_FinaleCall MSG_NORMAL
	msgbox gText_Challenge_FinaleProf MSG_NORMAL
	releaseall
	end

ChFinale_Items:
	msgbox gText_Challenge_FinaleItems MSG_NORMAL
	return

ChFinale_Held:
	msgbox gText_Challenge_FinaleHeld MSG_NORMAL
	return

ChFinale_Center:
	msgbox gText_Challenge_FinaleCenter MSG_NORMAL
	return

ChFinale_Pc:
	msgbox gText_Challenge_FinalePc MSG_NORMAL
	return

ChFinale_Lentrant:
	msgbox gText_Challenge_FinaleLentrant MSG_NORMAL
	return
ChFinale_Death:
	msgbox gText_Challenge_FinaleDeath MSG_NORMAL
	return

ChFinale_FirstOnly:
	msgbox gText_Challenge_FinaleFirstOnly MSG_NORMAL
	return

ChFinale_Nickname:
	msgbox gText_Challenge_FinaleNickname MSG_NORMAL
	return

@ 縛り A（0.50.3・patches/0530）の台本。HackChallenge_ChallengeATick（src/hack_challenge.c）が始める
@ ひんしでお別れ: 戦闘のあと（または フィールドで）ひんしの子を手持ちから外したあとの 1 回のメッセージ
.align 2
.global EventScript_ChallengeFarewell
EventScript_ChallengeFarewell:
	lockall
	msgbox gText_Challenge_Farewell MSG_NORMAL
	releaseall
	end

@ ゲームオーバー（手持ち全員がひんし）: メッセージのあとソフトリセット＝最後のレポートから
.align 2
.global EventScript_ChallengeGameOver
EventScript_ChallengeGameOver:
	lockall
	msgbox gText_Challenge_GameOver MSG_NORMAL
	closemessage
	callasm HackChallenge_GameOverReset + 1
	end

@ ニックネームひっす: ニックネームなし（種族名のまま）の手持ちを 1 匹ずつ。名前を入れ終わると この台本の続きに戻る（またなしなら 次の周期で もう一度）
.align 2
.global EventScript_ChallengeNickname
EventScript_ChallengeNickname:
	lockall
	callasm HackChallenge_NickPrepare + 1	@ Var8004 = 番号・Var8003 = 0（手持ち）・gStringVar1 = 名前・LASTRESULT 1 = 見つけた
	compare LASTRESULT 1
	if 0x1 _goto ChNick_Ask
	releaseall
	end

ChNick_Ask:
	msgbox gText_Challenge_NickAsk MSG_NORMAL
	special 0x9E
	waitstate
	releaseall
	end
