@ 看護師のリライブの会話（段 G・patches/0965・src/shadow_relive.c）。**仮の場所＝看護師**。
@ 素のポケセンの看護師の台本は どの町も「lock／faceplayer／call 0x08193E46」で、その call の行き先が 20 か所とも EventScript_ChallengeNurse（patches/0550・
@ assembly/hack_challenge/challenge_c_scripts.s）に付け替えてある。そこの「ポケセンきんし OFF」の道（素の台本へ goto する前）に 1 行 `call HackRelive_NurseTalk` を足した。
@ ふだん（手持ちにダークがいない・初回の贈り物の時期（ダークと戦ったあと）が来ていない・または渡し済み）は callasm と compare だけで return する＝今までの会話・動きのまま。
@ 台本は「話す場所」だけ。リライブ・補充・箱は全部 C（callasm）なので、場所が決まったらこの台本の呼び元だけ差し替える。
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global HackRelive_NurseTalk
HackRelive_NurseTalk:
	callasm Relive_ScriptPrepare
	compare LASTRESULT 1
	if equal _goto HackRelive_Talk
	return

HackRelive_Talk:
	callasm Relive_ScriptGiftNeeded
	compare LASTRESULT 1
	if equal _goto HackRelive_FirstGift
	callasm Relive_ScriptReadyCount
	compare LASTRESULT 0
	if equal _goto HackRelive_NotReady
	msgbox gText_Relive_Ask MSG_YESNO
	compare LASTRESULT 1
	if equal _goto HackRelive_Purify
	goto HackRelive_Supply

HackRelive_FirstGift:
	callasm Relive_ScriptGiveSnatch
	compare LASTRESULT 0xFFFF
	if equal _goto HackRelive_BagFull
	callasm Relive_ScriptGiftDone
	msgbox gText_Relive_FirstGift MSG_KEEPOPEN
	waitmessage
	closemessage
	return

HackRelive_Purify:
	callasm Relive_ScriptPurifyNext
	compare LASTRESULT 0
	if equal _goto HackRelive_AllDone
	fanfare 0x101
	msgbox gText_Relive_Done MSG_KEEPOPEN
	waitfanfare
	closemessage
	goto HackRelive_Purify
HackRelive_AllDone:
	msgbox gText_Relive_AllDone MSG_KEEPOPEN
	closemessage
	goto HackRelive_Supply

HackRelive_NotReady:
	msgbox gText_Relive_NotReady MSG_KEEPOPEN
	closemessage
HackRelive_Supply:
	msgbox gText_Relive_SupplyAsk MSG_YESNO
	compare LASTRESULT 1
	if notequal _goto HackRelive_End
	callasm Relive_ScriptGiveSnatch
	compare LASTRESULT 0xFFFF
	if equal _goto HackRelive_BagFull
	compare LASTRESULT 0
	if equal _goto HackRelive_Enough
	msgbox gText_Relive_Supplied MSG_KEEPOPEN
	closemessage
	return
HackRelive_Enough:
	msgbox gText_Relive_SupplyEnough MSG_KEEPOPEN
	closemessage
	return
HackRelive_BagFull:
	msgbox gText_Relive_BagFull MSG_KEEPOPEN
	closemessage
	return
HackRelive_End:
	return
