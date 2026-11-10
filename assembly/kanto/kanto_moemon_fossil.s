@ グレンの研究所（ポケモンけんきゅうじょ じっけんしつ・群 12 の 4）の かせきの研究員（素の FossilScientist 08189234）の前に、
@ Moemonかせき（ITEM_MOEMON_FOSSIL 0x34B）を その場で よみがえらせる話を足す（2026-10-10・src/hack_moemon_fossil.c）。
@ 研究員の人物の表の台本の語（083807D4・素の値 08189234）を repoints でここへ。持っていなければ・いいえ なら素の台本へ。
@ 文は strings/kanto_moemon_fossil.string

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ ITEM_MOEMON_FOSSIL, 0x34B
.equ VANILLA_FOSSIL_SCIENTIST, 0x08189234

.global KantoMoemonFossil_EventScript_Scientist
KantoMoemonFossil_EventScript_Scientist:
	checkitem ITEM_MOEMON_FOSSIL 1
	compare LASTRESULT 0
	if equal _goto KantoFossilTable_Check
	lock
	faceplayer
	msgbox gText_MoeFossil_Ask MSG_YESNO
	compare LASTRESULT 0
	if equal _goto KantoMoemonFossil_Vanilla
	callasm HackMoemonFossil_Revive
	compare LASTRESULT 2
	if equal _goto KantoMoemonFossil_Full
	copyvar 0x8001 LASTRESULT
	removeitem ITEM_MOEMON_FOSSIL 1
	msgbox gText_MoeFossil_Wait MSG_KEEPOPEN
	fanfare 0x101
	msgbox gText_MoeFossil_Revived MSG_KEEPOPEN
	waitfanfare
	compare 0x8001 1
	if equal _goto KantoMoemonFossil_ToPC
	release
	end

KantoMoemonFossil_ToPC:
	msgbox gText_MoeFossil_ToPC MSG_NORMAL
	release
	end

KantoMoemonFossil_Full:
	msgbox gText_MoeFossil_Full MSG_NORMAL
	release
	end

KantoMoemonFossil_Vanilla:
	release
	goto VANILLA_FOSSIL_SCIENTIST

@ [hack] patches/0952 2026-11-04 化石の表（src/hack_orcaiser.c の sFossilRevive: きょがのカセキ・ずがい・たて・ふた・はね・アゴ・ヒレ）。
@ Moemonかせきを持っていないとき、表を先頭から順に「持っているか」を調べ、はい なら その場で よみがえらせる（Lv5）。いいえ なら 次の化石へ。
@ どれも持っていなければ・全部 いいえ なら 素の台本へ（ひみつのコハク・かいのかせき・こうらのかせきは素の台本・ツメ／ねっこはホウエンのデボン 2F）
KantoFossilTable_Check:
	setvar 0x8004 0
	callasm HackFossil_FindNext
	compare LASTRESULT 0
	if equal _goto KantoFossilPair_Check
	lock
	faceplayer
KantoFossilTable_Ask:
	bufferitem 0 0x8005
	msgbox gText_Fossil_Ask MSG_YESNO
	compare LASTRESULT 0
	if equal _goto KantoFossilTable_Decline
	callasm HackFossil_Revive
	compare LASTRESULT 2
	if equal _goto KantoMoemonFossil_Full
	copyvar 0x8001 LASTRESULT
	msgbox gText_MoeFossil_Wait MSG_KEEPOPEN
	fanfare 0x101
	msgbox gText_MoeFossil_Revived MSG_KEEPOPEN
	waitfanfare
	compare 0x8001 1
	if equal _goto KantoMoemonFossil_ToPC
	release
	end

KantoFossilTable_Decline:
	addvar 0x8004 1
	callasm HackFossil_FindNext
	compare LASTRESULT 0
	if equal _goto KantoFossilPair_CheckLocked
	goto KantoFossilTable_Ask

@ [hack] patches/0952 2026-11-04 かけらの組（剣盾・ウカッツ）。表（単独）のあと・素の台本の前。上=とり／さかな × 下=りゅう／きょうりゅう。
@ 持っている組を HackFossil_FindPair（src/hack_orcaiser.c の sFossilPair）で順に探し、はい／いいえで聞く（いいえ は次の組へ・最大 4 回）。
@ KantoFossilPair_Check = まだ話しかけを固定していない入口（無ければ素の台本へ）／KantoFossilPair_CheckLocked = 表で いいえ のあと（固定済み・無ければ release して素の台本へ）
KantoFossilPair_Check:
	setvar 0x8004 0
	callasm HackFossil_FindPair
	compare LASTRESULT 0
	if equal _goto KantoFossilKase_Check
	lock
	faceplayer
	goto KantoFossilPair_Ask

KantoFossilPair_CheckLocked:
	setvar 0x8004 0
	callasm HackFossil_FindPair
	compare LASTRESULT 0
	if equal _goto KantoFossilKase_CheckLocked
KantoFossilPair_Ask:
	bufferitem 0 0x8005
	bufferitem 1 0x8006
	msgbox gText_FossilPair_Ask MSG_YESNO
	compare LASTRESULT 0
	if equal _goto KantoFossilPair_Decline
	callasm HackFossil_RevivePair
	compare LASTRESULT 2
	if equal _goto KantoMoemonFossil_Full
	copyvar 0x8001 LASTRESULT
	msgbox gText_MoeFossil_Wait MSG_KEEPOPEN
	fanfare 0x101
	msgbox gText_MoeFossil_Revived MSG_KEEPOPEN
	waitfanfare
	compare 0x8001 1
	if equal _goto KantoMoemonFossil_ToPC
	release
	end

KantoFossilPair_Decline:
	addvar 0x8004 1
	callasm HackFossil_FindPair
	compare LASTRESULT 0
	if equal _goto KantoFossilKase_CheckLocked
	goto KantoFossilPair_Ask

@ [hack] patches/0968 2026-11-08 カセキメラ 4 種（やん之助 氏）: 組（上）のあと・素の台本の前。かけら（とり→パチニクス・さかな→ダンクルオ・りゅう→ステゴラドン・くびなが→プレチルド）1 つで
@ よみがえらせる（Lv5・src/hack_orcaiser.c の sFossilKase）。組を いいえ にした人／組にならない人が ここへ来る。いいえ なら次のかけらへ・全部 いいえ／無ければ素の台本へ。
@ KantoFossilKase_Check = まだ話しかけを固定していない入口（無ければ素の台本へ）／KantoFossilKase_CheckLocked = 表・組で いいえ のあと（固定済み・無ければ release して素の台本へ）
KantoFossilKase_Check:
	setvar 0x8004 0
	callasm HackFossil_FindKase
	compare LASTRESULT 0
	if equal _goto VANILLA_FOSSIL_SCIENTIST
	lock
	faceplayer
	goto KantoFossilKase_Ask

KantoFossilKase_CheckLocked:
	setvar 0x8004 0
	callasm HackFossil_FindKase
	compare LASTRESULT 0
	if equal _goto KantoMoemonFossil_Vanilla
KantoFossilKase_Ask:
	bufferitem 0 0x8005
	msgbox gText_Fossil_Ask MSG_YESNO
	compare LASTRESULT 0
	if equal _goto KantoFossilKase_Decline
	callasm HackFossil_ReviveKase
	compare LASTRESULT 2
	if equal _goto KantoMoemonFossil_Full
	copyvar 0x8001 LASTRESULT
	msgbox gText_MoeFossil_Wait MSG_KEEPOPEN
	fanfare 0x101
	msgbox gText_MoeFossil_Revived MSG_KEEPOPEN
	waitfanfare
	compare 0x8001 1
	if equal _goto KantoMoemonFossil_ToPC
	release
	end

KantoFossilKase_Decline:
	addvar 0x8004 1
	callasm HackFossil_FindKase
	compare LASTRESULT 0
	if equal _goto KantoMoemonFossil_Vanilla
	goto KantoFossilKase_Ask
