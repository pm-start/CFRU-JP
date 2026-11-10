.text
.thumb
.align 2
@ れーとのおうの 2 回行動・振り払いの台本（src/hack_gible_ai.c・patches/0792・2026-10-23）

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.global BattleScript_GibleDoubleAction
.global BattleScript_GibleResist
.global BattleScript_GibleResistWoke
.global BattleScript_GibleResistUnfroze

@ 2 回目の前に特性の札「れーとのおう」を出す（abilityPopupOverwrite は C が入れてある）
BattleScript_GibleDoubleAction:
	call BattleScript_AbilityPopUp
	pause 0x20
	call BattleScript_AbilityPopUpRevert
	end3

BattleScript_GibleResist:
	setword BATTLE_STRING_LOADER gText_GibleResisted
	printstring 0x184
	waitmessage DELAY_1SECOND
	return

@ ねむり・こおりは C が状態を消してある。振り払いの文のあと、既存の目覚め／解凍の文
BattleScript_GibleResistWoke:
	setword BATTLE_STRING_LOADER gText_GibleResisted
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto 0x81BCFF2 @ BattleScript_MoveUsedWokeUp（attackcanceler_battle_scripts.h の固定番地）

BattleScript_GibleResistUnfroze:
	setword BATTLE_STRING_LOADER gText_GibleResisted
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto 0x81BD049 @ BattleScript_MoveUsedUnfroze（同上）
