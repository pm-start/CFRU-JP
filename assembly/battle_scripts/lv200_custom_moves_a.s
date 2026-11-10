.text
.thumb
.align 2
.include "../asm_defines.s"
.include "../battle_script_macros.s"
.include "../lv200_custom_common.inc"

@ 元技の余計な副作用を持ち込まない標準ダメージ台本。
.global BattleScript_Lv200A_Damage
BattleScript_Lv200A_Damage:
	attackcanceler
	accuracycheck BS_MOVE_MISSED, 0x0
	call STANDARD_DAMAGE
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE, 0x0
	cmd49 0x0, 0x0
	goto BS_MOVE_END

.global BattleScript_Lv200A_Fighting180
BattleScript_Lv200A_Fighting180:
	attackcanceler
	callasm Lv200A_PrepareFighting180
	accuracycheck BS_MOVE_MISSED, 0x0
	call STANDARD_DAMAGE
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE, 0x0
	cmd49 0x0, 0x0
	goto BS_MOVE_END

.global BattleScript_Lv200A_Status
BattleScript_Lv200A_Status:
	attackcanceler
	callasm Lv200A_CheckStatusMove
	jumpifword EQUALS, DAMAGE_LOC, 0x0, FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED, 0x0
	attackanimation
	waitanimation
	callasm Lv200A_ApplyStatusMove
	goto BS_MOVE_END

@ いしくれのまく: ステルスロックとは別枠の設置。置いたあとの文はステルスロックと同じ（C が BATTLE_STRING_LOADER に入れる）。
.global BattleScript_Lv200A_RockCurtain
BattleScript_Lv200A_RockCurtain:
	attackcanceler
	callasm Lv200A_CheckStatusMove
	jumpifword EQUALS, DAMAGE_LOC, 0x0, FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED, 0x0
	attackanimation
	waitanimation
	callasm Lv200A_ApplyStatusMove
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END

@ どくのおうさま: 特性名のポップアップ付きで「ポイズンヒールになった」を出す。
.global BattleScript_Lv200A_PoisonKing
BattleScript_Lv200A_PoisonKing:
	attackcanceler
	callasm Lv200A_CheckStatusMove
	jumpifword EQUALS, DAMAGE_LOC, 0x0, FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED, 0x0
	attackanimation
	waitanimation
	callasm Lv200A_ApplyStatusMove
	callasm Lv200A_SetPoisonKingPopup
	call BattleScript_AbilityPopUp
	setword BATTLE_STRING_LOADER gText_Lv200A_PoisonKingStart
	printstring 0x184
	waitmessage DELAY_1SECOND
	call BattleScript_AbilityPopUpRevert
	goto BS_MOVE_END

.global BattleScript_Lv200A_Ground180
BattleScript_Lv200A_Ground180:
	attackcanceler
	callasm Lv200A_CheckStatusMove
	jumpifword EQUALS, DAMAGE_LOC, 0x0, FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED, 0x0
	attackanimation
	waitanimation
	callasm Lv200A_ApplyStatusMove
	jumpifstatus BANK_ATTACKER, STATUS_ANY, Lv200A_Ground180_CureStatus
Lv200A_Ground180_AfterStatus:
	jumpifsecondarystatus BANK_ATTACKER, STATUS2_CONFUSION, Lv200A_Ground180_CureConfusion
Lv200A_Ground180_AfterConfusion:
	jumpifword EQUALS, DAMAGE_LOC, 0x0, Lv200A_Ground180_Stats
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B
	waitmessage DELAY_1SECOND
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
Lv200A_Ground180_Stats:
	LV200_SELF_STAT_ANIM STAT_ANIM_DEF | STAT_ANIM_SPDEF
	LV200_SELF_STAT_STEP STAT_DEF, INCREASE_1, 1
	LV200_SELF_STAT_STEP STAT_SPDEF, INCREASE_1, 1
	goto BS_MOVE_END

@ 状態異常の治療: 手持ちのデータにも送り、文と体力の枠の印を更新する（素の RefreshBS と同じ流れ）。
Lv200A_Ground180_CureStatus:
	callasm Lv200A_CureStatusGround180
	printstring 0xA7 @;STRINGID_PKMNSTATUSNORMAL
	waitmessage DELAY_1SECOND
	refreshhpbar BANK_ATTACKER
	goto Lv200A_Ground180_AfterStatus

Lv200A_Ground180_CureConfusion:
	callasm Lv200A_CureConfusionGround180
	printstring 0x42 @;STRINGID_PKMNHEALEDCONFUSION
	waitmessage DELAY_1SECOND
	goto Lv200A_Ground180_AfterConfusion

.global BattleScript_Lv200A_Rock170
BattleScript_Lv200A_Rock170:
	attackcanceler
	callasm Lv200A_CheckStatusMove
	jumpifword EQUALS, DAMAGE_LOC, 0x0, FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED, 0x0
	attackanimation
	waitanimation
	callasm Lv200A_ApplyStatusMove
	jumpifword EQUALS, DAMAGE_LOC, 0x0, Lv200A_Rock170_Stats
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B
	waitmessage DELAY_1SECOND
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
Lv200A_Rock170_Stats:
	LV200_SELF_STAT_ANIM STAT_ANIM_ATK | STAT_ANIM_DEF | STAT_ANIM_SPD | STAT_ANIM_SPATK | STAT_ANIM_SPDEF
	LV200_SELF_STAT_STEP STAT_ATK, INCREASE_1, 1
	LV200_SELF_STAT_STEP STAT_DEF, INCREASE_1, 1
	LV200_SELF_STAT_STEP STAT_SPD, INCREASE_1, 1
	LV200_SELF_STAT_STEP STAT_SPATK, INCREASE_1, 1
	LV200_SELF_STAT_STEP STAT_SPDEF, INCREASE_1, 1
	goto BS_MOVE_END

.global BattleScript_Lv200A_Multi
BattleScript_Lv200A_Multi:
	attackcanceler
	callasm Lv200A_PrepareMultiHit
	accuracycheck BS_MOVE_MISSED, 0x0
	call STANDARD_DAMAGE
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE, 0x0
	cmd49 0x0, 0x0
	goto BS_MOVE_END

.global BattleScript_Lv200A_Counter
BattleScript_Lv200A_Counter:
	attackcanceler
	callasm Lv200A_PrepareCounter
	jumpifword EQUALS, DAMAGE_LOC, 0x0, FAILED_PRE
	accuracycheck BS_MOVE_MISSED, 0x0
	attackstring
	ppreduce
	typecalc2
	bicbyte OUTCOME, OUTCOME_SUPER_EFFECTIVE | OUTCOME_NOT_VERY_EFFECTIVE
	adjustsetdamage
	goto BS_HIT_FROM_ATTACKANIMATION

.global BattleScript_Lv200A_Pivot
BattleScript_Lv200A_Pivot:
	attackcanceler
	accuracycheck BS_MOVE_MISSED, 0x0
	call STANDARD_DAMAGE
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE, 0x0
	cmd49 0x0, 0x0
	copybyte SWITCHING_BANK, USER_BANK
	jumpifcannotswitch BANK_SWITCHING | ATK4F_DONT_CHECK_STATUSES, BS_MOVE_END
	jumpifnoviablemonsleft BANK_TARGET, BS_MOVE_END
	jumpiffainted BANK_SWITCHING, BS_MOVE_END
	callasm Lv200A_ToggleBatonState
	openpartyscreen BANK_SWITCHING, BS_MOVE_END
	switchoutabilities BANK_SWITCHING
	waitstateatk
	switchhandleorder BANK_SWITCHING, 0x2
	returntoball BANK_SWITCHING
	callasm TryRemovePrimalWeatherSwitchingBank
	getswitchedmondata BANK_SWITCHING
	switchindataupdate BANK_SWITCHING
	hpthresholds BANK_SWITCHING
	printstring 0x3
	switchinanim BANK_SWITCHING, 0x1
	waitstateatk
	callasm Lv200A_ToggleBatonState
	switchineffects BANK_SWITCHING
	goto BS_MOVE_END

.global BattleScript_Lv200A_PerishPivot
BattleScript_Lv200A_PerishPivot:
	attackcanceler
	copybyte SWITCHING_BANK, USER_BANK
	jumpifcannotswitch BANK_SWITCHING | ATK4F_DONT_CHECK_STATUSES, FAILED_PRE
	jumpifnoviablemonsleft BANK_TARGET, FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED, 0x0
	attackanimation
	waitanimation
	callasm Lv200A_ApplyPerishTarget
	openpartyscreen BANK_SWITCHING, BS_MOVE_END
	switchoutabilities BANK_SWITCHING
	waitstateatk
	switchhandleorder BANK_SWITCHING, 0x2
	returntoball BANK_SWITCHING
	callasm TryRemovePrimalWeatherSwitchingBank
	getswitchedmondata BANK_SWITCHING
	switchindataupdate BANK_SWITCHING
	hpthresholds BANK_SWITCHING
	printstring 0x3
	switchinanim BANK_SWITCHING, 0x1
	waitstateatk
	switchineffects BANK_SWITCHING
	goto BS_MOVE_END

@ A側の終端継続ダメージ。量と対象はGetEndTurnScriptが設定する。
.global BattleScript_Lv200A_EndTurnDamage
BattleScript_Lv200A_EndTurnDamage:
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	faintpokemon BANK_ATTACKER, FALSE, 0x0
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	end2

.global BattleScript_Lv200A_EndTurnHeal
BattleScript_Lv200A_EndTurnHeal:
	playanimation BANK_ATTACKER, ANIM_HEALING_SPARKLES, 0x0
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B
	waitmessage DELAY_1SECOND
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	end2

@ どくのおうさまの毎ターン回復（特性名のポップアップ付き・ほかの技の終端回復は変えない）
.global BattleScript_Lv200A_EndTurnPoisonKingHeal
BattleScript_Lv200A_EndTurnPoisonKingHeal:
	call BattleScript_AbilityPopUp
	playanimation BANK_ATTACKER, ANIM_HEALING_SPARKLES, 0x0
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B
	waitmessage DELAY_1SECOND
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE
	call BattleScript_AbilityPopUpRevert
	end2

@ どくのおうさまの最後のターン: 本来のとくせいが戻る知らせ
.global BattleScript_Lv200A_EndTurnPoisonKingEnd
BattleScript_Lv200A_EndTurnPoisonKingEnd:
	setword BATTLE_STRING_LOADER gText_Lv200A_PoisonKingEnd
	printstring 0x184
	waitmessage DELAY_1SECOND
	end2

.global BattleScript_Lv200A_EndTurnDrain
BattleScript_Lv200A_EndTurnDrain:
	orword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	manipulatedamage 0x0
	graphicalhpupdate BANK_TARGET
	datahpupdate BANK_TARGET
	faintpokemon BANK_ATTACKER, FALSE, 0x0
	bicword HIT_MARKER, HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	end2

.global BattleScript_Lv200A_EndTurnDefDown
BattleScript_Lv200A_EndTurnDefDown:
	setbyte STAT_ANIM_PLAYED, 0x0
	playstatchangeanimation BANK_ATTACKER, STAT_ANIM_DEF, STAT_ANIM_DOWN
	setstatchanger STAT_DEF | DECREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR | STAT_NOT_PROTECT_AFFECTED BattleScript_Lv200A_EndTurnDefDownEnd
	printfromtable 0x83C4554
	waitmessage DELAY_1SECOND
BattleScript_Lv200A_EndTurnDefDownEnd:
	end2

.global BattleScript_Lv200A_SwitchInSpeedDown
BattleScript_Lv200A_SwitchInSpeedDown:
	setbyte STAT_ANIM_PLAYED, 0x0
	playstatchangeanimation BANK_ATTACKER, STAT_ANIM_SPD, STAT_ANIM_DOWN
	setstatchanger STAT_SPD | DECREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR | STAT_NOT_PROTECT_AFFECTED BattleScript_Lv200A_SwitchInSpeedDownEnd
	printfromtable 0x83C4554
	waitmessage DELAY_1SECOND
BattleScript_Lv200A_SwitchInSpeedDownEnd:
	end2

@ ターンの終わりの文だけ（効果が切れる知らせ）。文は C が gBattleStringLoader に入れてある。
.global BattleScript_Lv200A_EndTurnMessage
BattleScript_Lv200A_EndTurnMessage:
	printstring 0x184
	waitmessage DELAY_1SECOND
	end2

.global BattleScript_Lv200A_EchoDamage
BattleScript_Lv200A_EchoDamage:
	setword BATTLE_STRING_LOADER gText_Lv200A_MirrorRound
	printstring 0x184
	waitmessage DELAY_1SECOND
	attackstring
	calculatedamage
	attackanimation
	waitanimation
	effectivenesssound
	flash BANK_TARGET
	waitstateatk
	graphicalhpupdate BANK_TARGET
	datahpupdate BANK_TARGET
	critmessage
	waitmessage DELAY_1SECOND
	resultmessage
	waitmessage DELAY_1SECOND
	faintpokemon BANK_TARGET, FALSE, 0x0
	callasm Lv200A_ClearEchoReplay
	end2

@ つばさのまもりで防いだ接触攻撃の使用者を、レッドカードと同じ条件で交代させる。
.global BattleScript_Lv200A_WingGuardForceSwitch
BattleScript_Lv200A_WingGuardForceSwitch:
	jumpifdynamaxed BANK_ATTACKER BattleScript_Lv200A_WingGuardForceSwitchEnd
	jumpifspecialstatusflag BANK_ATTACKER STATUS3_ROOTED 0x0 BattleScript_Lv200A_WingGuardForceSwitchEnd
	jumpifability BANK_ATTACKER ABILITY_SUCTIONCUPS BattleScript_Lv200A_WingGuardForceSwitchEnd
	jumpifcannotswitch BANK_ATTACKER | ATK4F_DONT_CHECK_STATUSES BattleScript_Lv200A_WingGuardForceSwitchEnd
	setbyte FORCE_SWITCH_HELPER 0x2
	forcerandomswitch BANK_ATTACKER BANK_TARGET BattleScript_Lv200A_WingGuardForceSwitchReset
BattleScript_Lv200A_WingGuardForceSwitchReset:
	setbyte FORCE_SWITCH_HELPER 0x0
BattleScript_Lv200A_WingGuardForceSwitchEnd:
	end2
