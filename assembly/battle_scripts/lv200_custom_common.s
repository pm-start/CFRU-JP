.text
.thumb
.align 2
.include "../asm_defines.s"
.include "../battle_script_macros.s"
.include "../lv200_custom_common.inc"

@ 新規技(Lv200)の共通台本（P3）。lv200_custom_common.c の Lv200Common_GetMoveEndScript が 1 件ずつ返す。
@ cmd49 の MOVE_END で pushcursor されて来るので、最後は return。文は既存の文(gStatUpStringIds ほか)だけ。

@ 能力変化。statChanger と gBankAttacker／gBankTarget は C が設定済み。
@ statbuffchange が あまのじゃく・たんじゅん・クリアボディ・ミラーアーマー・ミスト・+6／-6 を素の流れで判断する。
@ quiet=0 のときは、これ以上上がらない／下がらない文も素の流れで出す（アニメは出さない）。
.macro LV200_COMMON_STAT name, userflag, table, bank, quiet
	.global BattleScript_Lv200Common_\name
BattleScript_Lv200Common_\name:
	statbuffchange \userflag | STAT_BS_PTR Lv200Common_\name\()_End
	.if \quiet
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200Common_\name\()_End
	.else
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200Common_\name\()_Print
	.endif
	setgraphicalstatchangevalues
	playanimation \bank ANIM_STAT_BUFF ANIM_ARG_1
Lv200Common_\name\()_Print:
	printfromtable \table
	waitmessage DELAY_1SECOND
Lv200Common_\name\()_End:
	return
.endm

LV200_COMMON_STAT SelfUp, STAT_ATTACKER, gStatUpStringIds, BANK_ATTACKER, 0
LV200_COMMON_STAT SelfUpQuiet, STAT_ATTACKER, gStatUpStringIds, BANK_ATTACKER, 1
LV200_COMMON_STAT SelfDown, STAT_ATTACKER, gStatDownStringIds, BANK_ATTACKER, 0
LV200_COMMON_STAT SelfDownQuiet, STAT_ATTACKER, gStatDownStringIds, BANK_ATTACKER, 1
LV200_COMMON_STAT FoeUp, STAT_TARGET, gStatUpStringIds, BANK_TARGET, 0
LV200_COMMON_STAT FoeUpQuiet, STAT_TARGET, gStatUpStringIds, BANK_TARGET, 1
LV200_COMMON_STAT FoeDown, STAT_TARGET, gStatDownStringIds, BANK_TARGET, 0
LV200_COMMON_STAT FoeDownQuiet, STAT_TARGET, gStatDownStringIds, BANK_TARGET, 1

@ HP 回復: gBattleMoveDamage に負数（C が設定）。HP バー・手持ちのデータ(datahpupdate)・文。
.global BattleScript_Lv200Common_Heal
BattleScript_Lv200Common_Heal:
	playanimation BANK_ATTACKER, ANIM_HEALING_SPARKLES, 0x0
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @;STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	return

@ HP ダメージ（代償・反射・返しダメージ・共有ダメージ）: HP バー・手持ち・ひんしの処理。
.global BattleScript_Lv200Common_Hurt
BattleScript_Lv200Common_Hurt:
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	faintpokemon BANK_ATTACKER, FALSE, 0x0
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	return

@ 状態異常の治療（手持ちへの送りは C 側で済み）: 文と体力の枠の印。
.global BattleScript_Lv200Common_CureStatus
BattleScript_Lv200Common_CureStatus:
	printstring 0xA7 @;STRINGID_PKMNSTATUSNORMAL
	waitmessage DELAY_1SECOND
	refreshhpbar BANK_ATTACKER
	return

.global BattleScript_Lv200Common_CureConfusion
BattleScript_Lv200Common_CureConfusion:
	printstring 0x42 @;STRINGID_PKMNHEALEDCONFUSION
	waitmessage DELAY_1SECOND
	return

@ 文だけ（場・状態の知らせ）。文は C が文字列の読み込み先（gBattleStringLoader）に入れてある。
.global BattleScript_Lv200Common_Message
BattleScript_Lv200Common_Message:
	printstring 0x184
	waitmessage DELAY_1SECOND
	return

@ 状態異常・こんらん・ひるみの付与。素の追加効果と同じ流れ（無効判定・文・手持ちへ送る）。EFFECT_BYTE は C が設定済み。
.global BattleScript_Lv200Common_Inflict
BattleScript_Lv200Common_Inflict:
	seteffectsecondary
	callasm Lv200Common_ClearPoisonBypass
	return

@ メロメロ。付与は C が済ませてある。文は素のアタックの「メロメロに なった」(0x45)。
.global BattleScript_Lv200Common_Infatuate
BattleScript_Lv200Common_Infatuate:
	printstring 0x45
	waitmessage DELAY_1SECOND
	return

@ 天気の開始。文の番号は C が MULTISTRING_CHOOSER に入れてある（素の gMoveWeatherChangeStringIds）。
.global BattleScript_Lv200Common_Weather
BattleScript_Lv200Common_Weather:
	printfromtable 0x83C44F4 @ gMoveWeatherChangeStringIds
	waitmessage DELAY_1SECOND
	call BS_WEATHER_FORM_CHANGES
	callasm ClearHailStartFlag
	return
