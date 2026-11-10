.text
.thumb
.align 2
/*
fuuin_battle_scripts.s
	縛り「ふういんルーレット」（#123・src/hack_fuuin.c）: 封印された技を選んだとき（move_menu.c の TrySetCantSelectMoveBattleScript）
*/

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.global BattleScript_SelectingSealedMove

BattleScript_SelectingSealedMove:
	setword BATTLE_STRING_LOADER gText_Fuuin_MoveSealed
	printselectionstring 0x184
	endselectionscript