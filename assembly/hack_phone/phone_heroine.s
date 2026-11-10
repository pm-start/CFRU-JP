@ ハルカ・ヒカリの電話の台本（Redmine #76 段 1・FEAT-heroine-76-s1・patches/0866・手書き）。文は strings/hack_heroine.string
@ 呼ぶのは src/hack_phone.c（電話帳の相手メニュー・着信）。本体は src/hack_heroine.c（好感度・段・貢ぎ・うけとる）。
@   メニュー（電話帳で相手を選ぶと）: かける／あげる／うけとる（渡す物があるときだけ）／やめる
@   かける: 30 分の枠に 1 回だけ好感度 +2・会話は段で 2 通り
@   あげる: どうぐ（バッグから 1 つ選ぶ）／おかね（1,000 円単位・最大 100,000 円）。上がり幅と上限の表は src/Tables/hack_heroine_table.h
@   うけとる: 段 2〜5 のごほうびを渡す（バッグ→いっぱいならパソコンへ・タマゴは手持ち→ボックスへ）。段 2 で連れ歩き中に話しかけたときに差し替える
@   着信（専用経路・通常の抽選に入れない）: 段が上がった次の 30 分の枠で「でんわだ！」→ はい で 一言 or プレゼントの知らせ
@ 台本の頭で Var8003 に 0（ハルカ）か 1（ヒカリ）を置き、callasm はそれを見る。

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ TEXT_COLOR_FEMALE, 1
.equ SP_ADD_MENU, 0x25
.equ SP_CHOOSE_ITEM_FROM_BAG, 0xB1
.equ MENU_3, 0x21
.equ MENU_4, 0x22
.equ MENU_5, 0x23

.macro menu_item n:req, text:req
	setvar 0x8006, \n
	loadpointer 0, \text
	special SP_ADD_MENU
.endm

@ 電話帳のメニューの共通部（項目を並べる前と選ぶ）
.macro menu_head
	preparemsg gText_HackPhone_MenuWhich
	waitmsg
.endm
.macro menu_tail N:req
	callasm HackPhone_LockField
	setvar 0x8006, 11
	multichoice 18, 0, \N, 0
.endm

.macro heroine_scripts P:req, IDX:req

@ ---- かける ----
.align 2
.global HackScr_Phone\P\()_Out
HackScr_Phone\P\()_Out:
	lockall
	textcolor TEXT_COLOR_FEMALE
	message gText_Hr\P\()_Dial
	waitmessage
	pause 120
	setvar 0x8003, \IDX
	callasm HackHeroine_Phone
	compare LASTRESULT 3
	if 0x4 _goto HackScr_Phone\P\()_ChatB
	message gText_Hr\P\()_ChatA1
	waitmessage
	waitkeypress
	message gText_Hr\P\()_ChatA2
	waitmessage
	waitkeypress
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_ChatB:
	message gText_Hr\P\()_ChatB1
	waitmessage
	waitkeypress
	message gText_Hr\P\()_ChatB2
	waitmessage
	waitkeypress
HackScr_Phone\P\()_End:
	closemessage
	releaseall
	end

@ ---- かかってくる（段が上がったとき・専用経路） ----
.align 2
.global HackScr_Phone\P\()_In
HackScr_Phone\P\()_In:
	lockall
	textcolor TEXT_COLOR_FEMALE
	setvar 0x8003, \IDX
	callasm HackHeroine_CallBegin
	message gText_Hr\P\()_Ring
	waitmessage
	yesnobox 20 8
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_End
	compare 0x8007 2
	if 0x1 _goto HackScr_Phone\P\()_InGift
	message gText_Hr\P\()_Kind1
	waitmessage
	waitkeypress
	message gText_Hr\P\()_Kind1b
	waitmessage
	waitkeypress
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_InGift:
	message gText_Hr\P\()_Kind2
	waitmessage
	waitkeypress
	goto HackScr_Phone\P\()_End

@ ---- 電話帳で選んだとき ----
@ 項目: かける／あげる／[うけとる＝渡す物があるとき]／[つれあるく か かえってもらう＝段 3 以上のハルカだけ]／やめる（3〜5 項目）。
@ 項目の組み合わせは HackHeroine_MenuBegin が Var8007（ビット 0 = うけとる・ビット 1〜2 = つれあるく 1／かえってもらう 2）に置く。
@ 選んだ番号は HackHeroine_MenuMap が 動作（0 かける・1 あげる・2 うけとる・3 つれあるく／かえってもらう・4 やめる）に直す。
.align 2
.global HackScr_Phone\P\()_Menu
HackScr_Phone\P\()_Menu:
	lockall
	textcolor TEXT_COLOR_FEMALE
	setvar 0x8003, \IDX
	callasm HackHeroine_MenuBegin
	compare 0x8007 1
	if 0x1 _goto HackScr_Phone\P\()_Menu1
	compare 0x8007 2
	if 0x1 _goto HackScr_Phone\P\()_Menu2
	compare 0x8007 3
	if 0x1 _goto HackScr_Phone\P\()_Menu3
	compare 0x8007 4
	if 0x1 _goto HackScr_Phone\P\()_Menu4
	compare 0x8007 5
	if 0x1 _goto HackScr_Phone\P\()_Menu5
	menu_head
	menu_item 0, gText_HrMenuCall
	menu_item 1, gText_HrMenuGive
	menu_item 2, gText_HrMenuCancel
	menu_tail MENU_3
	goto HackScr_Phone\P\()_Dispatch
HackScr_Phone\P\()_Menu1:
	menu_head
	menu_item 0, gText_HrMenuCall
	menu_item 1, gText_HrMenuGive
	menu_item 2, gText_HrMenuReceive
	menu_item 3, gText_HrMenuCancel
	menu_tail MENU_4
	goto HackScr_Phone\P\()_Dispatch
HackScr_Phone\P\()_Menu2:
	menu_head
	menu_item 0, gText_HrMenuCall
	menu_item 1, gText_HrMenuGive
	menu_item 2, gText_HrMenuFollow
	menu_item 3, gText_HrMenuCancel
	menu_tail MENU_4
	goto HackScr_Phone\P\()_Dispatch
HackScr_Phone\P\()_Menu3:
	menu_head
	menu_item 0, gText_HrMenuCall
	menu_item 1, gText_HrMenuGive
	menu_item 2, gText_HrMenuReceive
	menu_item 3, gText_HrMenuFollow
	menu_item 4, gText_HrMenuCancel
	menu_tail MENU_5
	goto HackScr_Phone\P\()_Dispatch
HackScr_Phone\P\()_Menu4:
	menu_head
	menu_item 0, gText_HrMenuCall
	menu_item 1, gText_HrMenuGive
	menu_item 2, gText_HrMenuDismiss
	menu_item 3, gText_HrMenuCancel
	menu_tail MENU_4
	goto HackScr_Phone\P\()_Dispatch
HackScr_Phone\P\()_Menu5:
	menu_head
	menu_item 0, gText_HrMenuCall
	menu_item 1, gText_HrMenuGive
	menu_item 2, gText_HrMenuReceive
	menu_item 3, gText_HrMenuDismiss
	menu_item 4, gText_HrMenuCancel
	menu_tail MENU_5
HackScr_Phone\P\()_Dispatch:
	callasm HackHeroine_MenuMap
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_Out
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_Give
	compare LASTRESULT 2
	if 0x1 _goto HackScr_Phone\P\()_Receive
	compare LASTRESULT 3
	if 0x1 _goto HackScr_Phone\P\()_FollowToggle
	goto HackScr_Phone\P\()_End

@ ---- つれあるく／かえってもらう ----
HackScr_Phone\P\()_FollowToggle:
	setvar 0x8003, \IDX
	callasm HackHeroine_FollowToggle
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_FollowOk
	compare LASTRESULT 2
	if 0x1 _goto HackScr_Phone\P\()_FollowByeCall
	compare LASTRESULT 3
	if 0x1 _goto HackScr_Phone\P\()_FollowBusy
	msgbox gText_Hr\P\()_FollowNo MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_FollowBusy:
	msgbox gText_Hr\P\()_FollowBusy MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_FollowOk:
	msgbox gText_Hr\P\()_FollowOk MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_FollowByeCall:
	msgbox gText_Hr\P\()_FollowBye MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End

@ ---- 連れ歩き中に話しかけたとき（ヒカリの分も同じ形で用意するが、段 3 まで連れ歩きに出ない） ----
.align 2
.global HackScr_Phone\P\()_Talk
HackScr_Phone\P\()_Talk:
	lock
	faceplayer
	textcolor TEXT_COLOR_FEMALE
	setvar 0x8003, \IDX
	callasm HackHeroine_TalkBegin
	msgbox gText_Hr\P\()_TalkHi MSG_KEEPOPEN
	callasm HackHeroine_HasPending
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_TalkMenuP
	preparemsg gText_HackPhone_MenuWhich
	waitmsg
	menu_item 0, gText_HrMenuGive
	menu_item 1, gText_HrMenuLeave
	menu_item 2, gText_HrMenuCancel
	callasm HackPhone_LockField
	setvar 0x8006, 11
	multichoice 18, 0, MENU_3, 0
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_Give
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_Dismiss
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_TalkMenuP:
	preparemsg gText_HackPhone_MenuWhich
	waitmsg
	menu_item 0, gText_HrMenuGive
	menu_item 1, gText_HrMenuReceive
	menu_item 2, gText_HrMenuLeave
	menu_item 3, gText_HrMenuCancel
	callasm HackPhone_LockField
	setvar 0x8006, 11
	multichoice 18, 0, MENU_4, 0
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_Give
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_Receive
	compare LASTRESULT 2
	if 0x1 _goto HackScr_Phone\P\()_Dismiss
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_Dismiss:
	callasm HackHeroine_Dismiss
	msgbox gText_Hr\P\()_FollowBye MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End

@ ---- あげる ----
HackScr_Phone\P\()_Give:
	preparemsg gText_HrGiveWhich
	waitmsg
	menu_item 0, gText_HrGiveItem
	menu_item 1, gText_HrGiveMoney
	menu_item 2, gText_HrMenuCancel
	callasm HackPhone_LockField
	setvar 0x8006, 8
	multichoice 20, 0, MENU_3, 0
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_GiveItem
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_GiveMoney
	goto HackScr_Phone\P\()_End

HackScr_Phone\P\()_GiveItem:
	setvar 0x8000, 0
	fadescreen 1                                       @ ★ 黒くしてから開く（special 0xB1 は自分では暗くしない・しないと野外→バッグの 2 コマ、地図と電話の絵の残りが縞とタイルの断片で見えた・BUG-heroine-gift-gfx #194）
	special SP_CHOOSE_ITEM_FROM_BAG
	waitstate
	setvar 0x8003, \IDX
	textcolor TEXT_COLOR_FEMALE
	callasm HackHeroine_GiveItem
	compare LASTRESULT 4
	if 0x1 _goto HackScr_Phone\P\()_End
	compare LASTRESULT 5
	if 0x1 _goto HackScr_Phone\P\()_Gennama
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_ItemOk
	compare LASTRESULT 3
	if 0x1 _goto HackScr_Phone\P\()_Enough
	compare LASTRESULT 2
	if 0x1 _goto HackScr_Phone\P\()_ItemNo
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_ItemOk:
	msgbox gText_Hr\P\()_ItemThanks MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_Gennama:
	msgbox gText_Hr\P\()_Gennama MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_ItemNo:
	msgbox gText_Hr\P\()_ItemNo MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_Enough:
	msgbox gText_Hr\P\()_Enough MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End

HackScr_Phone\P\()_GiveMoney:
	setvar 0x8003, \IDX
	callasm HackHeroine_MoneyMax
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_MoneyNone
	compare LASTRESULT 2
	if 0x1 _goto HackScr_Phone\P\()_Enough
	msgbox gText_Hr\P\()_MoneyAsk MSG_KEEPOPEN
	callasm HackHeroine_MoneyOpen
	waitstate
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_End
	callasm HackHeroine_MoneyApply
	compare LASTRESULT 1
	if 0x1 _goto HackScr_Phone\P\()_MoneyOk
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_MoneyOk:
	msgbox gText_Hr\P\()_MoneyThanks MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_MoneyNone:
	msgbox gText_Hr\P\()_MoneyNone MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End

@ ---- うけとる ----
HackScr_Phone\P\()_Receive:
	setvar 0x8003, \IDX
	callasm HackHeroine_Receive
	compare LASTRESULT 2
	if 0x1 _goto HackScr_Phone\P\()_RecvFull
	compare LASTRESULT 0
	if 0x1 _goto HackScr_Phone\P\()_End
	msgbox gText_Hr\P\()_RecvLead MSG_KEEPOPEN
	compare 0x8004 3
	if 0x1 _goto HackScr_Phone\P\()_Recv3
	compare 0x8004 4
	if 0x1 _goto HackScr_Phone\P\()_Recv4
	compare 0x8004 5
	if 0x1 _goto HackScr_Phone\P\()_Recv5
	msgbox gText_Hr\P\()_Recv2 MSG_KEEPOPEN
	goto HackScr_Phone\P\()_RecvPost
HackScr_Phone\P\()_Recv3:
	msgbox gText_Hr\P\()_Recv3 MSG_KEEPOPEN
	goto HackScr_Phone\P\()_RecvPost
HackScr_Phone\P\()_Recv4:
	msgbox gText_Hr\P\()_Recv4 MSG_KEEPOPEN
	goto HackScr_Phone\P\()_RecvPost
HackScr_Phone\P\()_Recv5:
	msgbox gText_Hr\P\()_Recv5 MSG_KEEPOPEN
HackScr_Phone\P\()_RecvPost:
	compare 0x8006 1
	if 0x1 _goto HackScr_Phone\P\()_RecvPc
	compare 0x8006 2
	if 0x1 _goto HackScr_Phone\P\()_RecvBox
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_RecvPc:
	msgbox gText_HrPcNote MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_RecvBox:
	msgbox gText_HrBoxNote MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
HackScr_Phone\P\()_RecvFull:
	msgbox gText_Hr\P\()_Full MSG_KEEPOPEN
	goto HackScr_Phone\P\()_End
.endm

heroine_scripts Ha, 0
heroine_scripts Hi, 1

@ ---- 連れ歩きの台本（src/hack_heroine.c の gHeroineFollowerTemplate と gFollowerState.script が指す） ----
.align 2
.global HackScr_HeroineTalk
HackScr_HeroineTalk:
	callasm HackHeroine_WhoIsFollowing
	compare LASTRESULT 2
	if 0x1 _goto HackScr_PhoneHi_Talk
	goto HackScr_PhoneHa_Talk

.align 2
.global HackScr_HeroineWait
HackScr_HeroineWait:
	lockall
	textcolor TEXT_COLOR_FEMALE
	callasm HackHeroine_WhoIsFollowing
	compare LASTRESULT 2
	if 0x1 _goto HackScr_HeroineWaitHi
	msgbox gText_HrHa_Wait MSG_KEEPOPEN
	goto HackScr_HeroineWaitEnd
HackScr_HeroineWaitHi:
	msgbox gText_HrHi_Wait MSG_KEEPOPEN
HackScr_HeroineWaitEnd:
	closemessage
	releaseall
	end

@ 戦闘のあとの回復の言葉（patches/0875）。src/hack_heroine.c の HackHeroine_FieldTick が起こす。LASTRESULT = (人 - 1) * 4 + 回復の段階
.align 2
.global HackScr_HeroineHeal
HackScr_HeroineHeal:
	lockall
	textcolor TEXT_COLOR_FEMALE
	callasm HackHeroine_HealPick
	compare LASTRESULT 0
	if 0x1 _goto HackScr_HeroineHealHa0
	compare LASTRESULT 1
	if 0x1 _goto HackScr_HeroineHealHa1
	compare LASTRESULT 2
	if 0x1 _goto HackScr_HeroineHealHa2
	compare LASTRESULT 3
	if 0x1 _goto HackScr_HeroineHealHa3
	compare LASTRESULT 4
	if 0x1 _goto HackScr_HeroineHealHi0
	compare LASTRESULT 5
	if 0x1 _goto HackScr_HeroineHealHi1
	compare LASTRESULT 6
	if 0x1 _goto HackScr_HeroineHealHi2
	msgbox gText_HrHi_Heal3 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHa0:
	msgbox gText_HrHa_Heal0 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHa1:
	msgbox gText_HrHa_Heal1 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHa2:
	msgbox gText_HrHa_Heal2 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHa3:
	msgbox gText_HrHa_Heal3 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHi0:
	msgbox gText_HrHi_Heal0 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHi1:
	msgbox gText_HrHi_Heal1 MSG_KEEPOPEN
	goto HackScr_HeroineHealEnd
HackScr_HeroineHealHi2:
	msgbox gText_HrHi_Heal2 MSG_KEEPOPEN
HackScr_HeroineHealEnd:
	closemessage
	releaseall
	end
