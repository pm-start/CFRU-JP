.text
.thumb
.align 2
@ 技進化（src/hack_move_evo.c・patches/0864）の文。枠の差し替えは C が先に済ませてある（ここは文と音だけ）

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.global BattleScript_MoveEvo

@ ターンの終わりに技が進化したときの文（BattleScriptExecute）
BattleScript_MoveEvo:
	fanfare 0x101
	setword BATTLE_STRING_LOADER gText_MoveEvo
	printstring 0x184
	waitmessage DELAY_1SECOND
	waitfanfare BANK_SCRIPTING
	end2

.global BattleScript_MoveMastery
.global BattleScript_MoveMasteryMax

@ 技の熟練度の段が上がったときの文（src/hack_move_evo.c の HackMoveMastery_TryAnnounce・patches/0932）。文字の入れ物は C が先に済ませてある
BattleScript_MoveMastery:
	setword BATTLE_STRING_LOADER gText_MoveMastery
	printstring 0x184
	waitmessage DELAY_1SECOND
	end2

@ 10 段目（きわめた）はファンファーレつき
BattleScript_MoveMasteryMax:
	fanfare 0x101
	setword BATTLE_STRING_LOADER gText_MoveMasteryMax
	printstring 0x184
	waitmessage DELAY_1SECOND
	waitfanfare BANK_SCRIPTING
	end2
