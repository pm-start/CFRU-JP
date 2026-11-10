.text
.thumb
.align 2
/*
shadow_move_scripts.s
	ダーク技（段 D・patches/0955・src/shadow_moves.c）の台本。EFFECT_HACK_CUSTOM（252）の入口（Lv200CustomMove_Dispatch）から飛んでくる:
	ダークエンド（ふつうの攻撃＋反動は cmd49.c）・ダークウェザー（ダークの空）・ダークリムーブ（かべ消し）・ダークハーフ（全員 HP 半分）と、ダークの空の毎ターンの台本
*/

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.global BattleScript_ShadowEndHit
.global BattleScript_ShadowSkyMove
.global BattleScript_ShadowShedMove
.global BattleScript_ShadowHalfMove
.global BattleScript_ShadowHalfDone
.global BattleScript_ShadowFailed
.global BattleScript_ShadowSkyContinues
.global BattleScript_ShadowSkyEnds
.global BattleScript_ShadowSkyDamage

.equ BattleScript_DoTurnDmg, 0x81BD017

@;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@ ダークエンド: ふつうの攻撃（反動は cmd49.c の ATK49_MOVE_RECOIL）
BattleScript_ShadowEndHit:
	goto BS_STANDARD_HIT

@ ダークウェザー: 5 ターンの天気（callasm が立てるか失敗にする）
BattleScript_ShadowSkyMove:
	attackcanceler
	attackstringnoprotean
	ppreduce
	callasm Shadow_TrySetShadowSky
	attackanimation
	waitanimation
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END

@ ダークリムーブ: リフレクター・ひかりのかべ・しんぴのまもり が両側で消える
BattleScript_ShadowShedMove:
	attackcanceler
	attackstring
	ppreduce
	callasm Shadow_RemoveScreensBothSides
	attackanimation
	waitanimation
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END

@ ダークハーフ: 場の全員が残り HP の半分を失う
BattleScript_ShadowHalfMove:
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
ShadowHalfLoop:
	callasm Shadow_HalfNext
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_TARGET
	datahpupdate BANK_TARGET
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	goto ShadowHalfLoop

BattleScript_ShadowHalfDone:
	setword BATTLE_STRING_LOADER gText_ShadowHalfDone
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END

BattleScript_ShadowFailed:
	goto FAILED

@;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@ ターンの終わり（文は C 側が gBattleStringLoader に・ダメージは gBattleMoveDamage に・gBankAttacker ＝その子）
BattleScript_ShadowSkyDamage:
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto BattleScript_DoTurnDmg

BattleScript_ShadowSkyContinues:
BattleScript_ShadowSkyEnds:
	printstring 0x184
	waitmessage DELAY_1SECOND
	end2
