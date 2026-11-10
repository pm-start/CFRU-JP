@ お母さんの貯金（金銀の「ははおやの ちょきん」・2026-10-06・src/hack_mom_savings.c）。
@ ユーザー「金銀の母親貯金みたいなの実装できそう？」→ 割合は自分で決める・電話の演出・道具は家に帰ったとき渡す。
@   EventScript_MomSavingsCall … フィールドの電話（src/hack_mom_savings.c HackMomSavings_FieldTick が始める・gStringVar1 = 道具の名前）
@   KantoHome1F_EventScript_MomSavings … 家のお母さん（kanto_home_scripts.s の いつもの会話の手前から）
@ 文は strings/kanto_mom_savings.string

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ TEXT_COLOR_FEMALE, 1
.equ TEXT_COLOR_NEUTRAL, 3
.equ SE_POKENAV_TWO_BEEPS, 0x66
.equ SP_ADD_MENU, 0x25
.equ MENU_4, 0x22
.equ MENU_5, 0x23
.equ MENU_6, 0x24
.equ MENU_7, 0x25
.equ KantoHome1F_EventScript_MomOriginal, 0x0817BCBB   @ 素のお母さんのスクリプト（kanto_home_scripts.s と同じ）

.macro menu_item n:req, text:req
	setvar 0x8006, \n
	loadpointer 0, \text
	special SP_ADD_MENU
.endm

.align 2
.global EventScript_MomSavingsCall
EventScript_MomSavingsCall:
	lockall
	playse SE_POKENAV_TWO_BEEPS
	waitse
	playse SE_POKENAV_TWO_BEEPS
	waitse
	textcolor TEXT_COLOR_NEUTRAL
	msgbox gText_MomCall_Ring MSG_KEEPOPEN
	textcolor TEXT_COLOR_FEMALE
	compare 0x8004 1                                 @ ★ 2026-10-11 1 = 買ったものは全部パソコンへ（src/hack_mom_savings.c の FieldTick が置く）
	if 0x1 _goto EventScript_MomSavingsCallPc
	msgbox gText_MomCall_Talk MSG_KEEPOPEN
	closemessage
	releaseall
	end
EventScript_MomSavingsCallPc:
	msgbox gText_MomCall_TalkPc MSG_KEEPOPEN
	closemessage
	releaseall
	end

.align 2
.global KantoHome1F_EventScript_MomSavings
KantoHome1F_EventScript_MomSavings:
	lock
	faceplayer
	setvar 0x8003 0                                    @ ★ 2026-10-16 電話から来たのではない印（Bye の分かれ目）
	textcolor TEXT_COLOR_FEMALE
	@ ★ 2026-10-10 ユーザー「ママのかいものを一気に渡すように」: まとめてバッグに入れる（HackMomSavings_GiveAll）
	callasm HackMomSavings_GiveAll
	compare LASTRESULT 0
	if 0x1 _goto KantoMomSav_Menu
	compare LASTRESULT 3
	if 0x1 _goto KantoMomSav_GiveFull
	copyvar 0x8001 LASTRESULT
	msgbox gText_MomSav_Gift MSG_KEEPOPEN
	fanfare 0x102 @ MUS_OBTAIN_ITEM
	textcolor TEXT_COLOR_NEUTRAL
	msgbox gText_MomSav_GotAll MSG_KEEPOPEN
	waitfanfare
	textcolor TEXT_COLOR_FEMALE
	compare 0x8001 2
	if 0x1 _goto KantoMomSav_GiveFull
	goto KantoMomSav_Menu
KantoMomSav_GiveFull:
	msgbox gText_MomSav_BagFull MSG_KEEPOPEN
	goto KantoMomSav_Menu
KantoMomSav_Menu:
	callasm HackMomSavings_BufferStatus
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_StatusOn
	msgbox gText_MomSav_StatusOff MSG_KEEPOPEN
	goto KantoMomSav_ShopNote
KantoMomSav_StatusOn:
	msgbox gText_MomSav_StatusOn MSG_KEEPOPEN
KantoMomSav_ShopNote:
	@ ★ 2026-10-15 かいものを おやすみ しているときだけ ひとこと（HackMomSavings_ShopStatus: LASTRESULT 1 = かいものを する）
	callasm HackMomSavings_ShopStatus
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_Ask
	msgbox gText_MomSav_ShopOffNote MSG_KEEPOPEN
KantoMomSav_Ask:
	callasm HackMomSavings_BufferStatus
	preparemsg gText_MomSav_What
	waitmsg
	menu_item 0, gText_MomSav_Withdraw
	menu_item 1, gText_MomSav_Deposit                  @ ★ 2026-10-16 ユーザー「あずける を足す」（電話と家の両方）
	menu_item 2, gText_MomSav_Percent
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_ItemStop
	menu_item 3, gText_MomSav_Start
	goto KantoMomSav_ItemEnd
KantoMomSav_ItemStop:
	menu_item 3, gText_MomSav_Stop
KantoMomSav_ItemEnd:
	callasm HackMomSavings_ShopStatus                  @ ★ 2026-10-15 3 つ目: かいものの する／しない（LASTRESULT 1 = いまは する）
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_ItemShopStop
	menu_item 4, gText_MomSav_ShopStart
	goto KantoMomSav_ItemShopEnd
KantoMomSav_ItemShopStop:
	menu_item 4, gText_MomSav_ShopStop
KantoMomSav_ItemShopEnd:
	menu_item 5, gText_MomSav_Cancel
	callasm HackPhone_LockField                        @ ★ 2026-10-17 電話から来たときも項目の選択中は主人公を歩かせない（家のお母さんでは元から止まっている＝何も変わらない）
	setvar 0x8006, 12                                  @ 窓の幅（タイル・自前の選択肢は出す直前に 0x8006 で渡す・「かいもの やすむ」が入る幅）
	multichoice 17, 0, MENU_6, 0
	compare LASTRESULT 0
	if 0x1 _goto KantoMomSav_DoWithdraw
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_DoDeposit
	compare LASTRESULT 2
	if 0x1 _goto KantoMomSav_DoPercent
	compare LASTRESULT 3
	if 0x1 _goto KantoMomSav_DoToggle
	compare LASTRESULT 4
	if 0x1 _goto KantoMomSav_DoShopToggle
	goto KantoMomSav_Bye

KantoMomSav_DoWithdraw:
	callasm HackMomSavings_Withdraw
	compare LASTRESULT 0
	if 0x1 _goto KantoMomSav_NoWithdraw
	msgbox gText_MomSav_Withdrew MSG_KEEPOPEN
	goto KantoMomSav_Bye
KantoMomSav_NoWithdraw:
	msgbox gText_MomSav_NoWithdraw MSG_KEEPOPEN
	goto KantoMomSav_Bye

KantoMomSav_DoDeposit:
	@ ★ 2026-10-16 あずける: 手持ちのお金（と貯金の上限）までの金額を数字で入れる（src/hack_mom_savings.c の HackMomSavings_Deposit*）
	callasm HackMomSavings_DepositMax                  @ 0x8004/0x8005 = 預けられる最大・LASTRESULT 0 = 預けられる／1 = お金が無い／2 = 貯金がいっぱい
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_DepNoMoney
	compare LASTRESULT 2
	if 0x1 _goto KantoMomSav_DepFull
	msgbox gText_MomSav_DepAsk MSG_KEEPOPEN
	callasm HackMomSavings_DepositOpen
	waitstate
	compare LASTRESULT 0
	if 0x1 _goto KantoMomSav_Bye                       @ B でやめた
	callasm HackMomSavings_Deposit                     @ gStringVar1 = 預けた額・gStringVar2 = 貯金
	msgbox gText_MomSav_Deposited MSG_KEEPOPEN
	goto KantoMomSav_Bye
KantoMomSav_DepNoMoney:
	msgbox gText_MomSav_DepNoMoney MSG_KEEPOPEN
	goto KantoMomSav_Bye
KantoMomSav_DepFull:
	msgbox gText_MomSav_DepFull MSG_KEEPOPEN
	goto KantoMomSav_Bye

KantoMomSav_DoPercent:
	preparemsg gText_MomSav_WhichPercent
	waitmsg
	menu_item 0, gText_MomSav_P10
	menu_item 1, gText_MomSav_P20
	menu_item 2, gText_MomSav_P30
	menu_item 3, gText_MomSav_P40
	menu_item 4, gText_MomSav_P50
	menu_item 5, gText_MomSav_P75
	menu_item 6, gText_MomSav_P100
	setvar 0x8006, 12
	multichoice 17, 0, MENU_7, 0
	compare LASTRESULT 0x7F
	if 0x1 _goto KantoMomSav_Bye
	callasm HackMomSavings_SetPercent
	msgbox gText_MomSav_PercentSet MSG_KEEPOPEN
	goto KantoMomSav_Bye

KantoMomSav_DoToggle:
	callasm HackMomSavings_ToggleOn
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_Started
	msgbox gText_MomSav_Stopped MSG_KEEPOPEN
	goto KantoMomSav_Bye
KantoMomSav_Started:
	callasm HackMomSavings_BufferStatus
	msgbox gText_MomSav_Started MSG_KEEPOPEN
	goto KantoMomSav_Bye

KantoMomSav_DoShopToggle:
	callasm HackMomSavings_ToggleShop
	compare LASTRESULT 1
	if 0x1 _goto KantoMomSav_ShopStarted
	msgbox gText_MomSav_ShopStopped MSG_KEEPOPEN
	goto KantoMomSav_Bye
KantoMomSav_ShopStarted:
	msgbox gText_MomSav_ShopStarted MSG_KEEPOPEN

KantoMomSav_Bye:
	compare 0x8003 1                                   @ ★ 2026-10-16 電話（HackScr_PhoneMomOutgoing）から来たときは 1
	if 0x1 _goto KantoMomSav_ByePhone
	msgbox gText_MomSav_Bye MSG_KEEPOPEN
	closemessage
	release
	end
KantoMomSav_ByePhone:
	setvar 0x8003 0
	msgbox gText_HackPhone_MomOut2 MSG_KEEPOPEN
	closemessage
	releaseall
	end

@ ★ 2026-10-16 ユーザー「ママに電話して、貯金のことを聞けるように」: 電話帳のママ（src/hack_phone.c・assembly/hack_phone/phone_scripts.s）から
@   家のお母さんと同じ貯金の相談（ひきだす・わりあい・ためる・かいもの）へ。道具を渡す処理（GiveAll）は通らない（電話では渡せない）。
@   0x8003 = 1 は電話の印（終わりの文だけ電話用・lockall の相手なので release ではなく releaseall）。家からは 0
.align 2
.global HackScr_PhoneMomSavingsEntry
HackScr_PhoneMomSavingsEntry:
	setvar 0x8003 1
	goto KantoMomSav_Menu
