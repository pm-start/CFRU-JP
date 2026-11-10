.text
.thumb
.align 2
.include "../asm_defines.s"
.include "../battle_script_macros.s"
.include "../lv200_custom_common.inc"

@ ターンの終わりの文だけ（効果が切れる知らせ）。文は C が gBattleStringLoader に入れてある。
.global BattleScript_Lv200B_EndTurnMessage
BattleScript_Lv200B_EndTurnMessage:
	printstring 0x184
	waitmessage DELAY_1SECOND
	end2

@ しもやけの毎ターンのダメージ（文「しもやけの ダメージを うけた！」→ HP バー）
.global BattleScript_Lv200B_EndTurnDamage
BattleScript_Lv200B_EndTurnDamage:
	setword BATTLE_STRING_LOADER gText_Lv200B_FrostbiteTick
	printstring 0x184
	waitmessage DELAY_1SECOND
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	faintpokemon BANK_ATTACKER 0x0 0x0
	end2

.global BattleScript_Lv200B_MirrorDamage
BattleScript_Lv200B_MirrorDamage:
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	faintpokemon BANK_ATTACKER 0x0 0x0
	bicword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	end2

.global BattleScript_Lv200B_MoveEndStatUp
BattleScript_Lv200B_MoveEndStatUp:
	copybyte BATTLE_COMMUNICATION + 6 USER_BANK
	copybyte USER_BANK BATTLE_SCRIPTING_BANK
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_MoveEndStatUpEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_MoveEndStatUpEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_MoveEndStatUpEnd:
	copybyte USER_BANK BATTLE_COMMUNICATION + 6
	end2

.global BattleScript_Lv200B_MoveEndStatDown
BattleScript_Lv200B_MoveEndStatDown:
	copybyte BATTLE_COMMUNICATION + 6 USER_BANK
	copybyte USER_BANK BATTLE_SCRIPTING_BANK
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_MoveEndStatDownEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_MoveEndStatDownEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatDownStringIds
	waitmessage DELAY_1SECOND
Lv200B_MoveEndStatDownEnd:
	copybyte USER_BANK BATTLE_COMMUNICATION + 6
	end2

.global BattleScript_Lv200B_EndTurnHeal
BattleScript_Lv200B_EndTurnHeal:
	playanimation BANK_ATTACKER ANIM_HEALING_SPARKLES 0x0
	orword HIT_MARKER 0x100
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
	end2

.global BattleScript_Lv200B_EndTurnStatDown
BattleScript_Lv200B_EndTurnStatDown:
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_EndTurnStatDownEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_EndTurnStatDownEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatDownStringIds
	waitmessage DELAY_1SECOND
Lv200B_EndTurnStatDownEnd:
	end2

.global BattleScript_Lv200B_SwitchInStatUp
BattleScript_Lv200B_SwitchInStatUp:
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_SwitchInStatUpEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_SwitchInStatUpEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_SwitchInStatUpEnd:
	end2

.global BattleScript_Lv200B_SwitchInCharge
BattleScript_Lv200B_SwitchInCharge:
	playanimation BANK_ATTACKER ANIM_CHARGE 0x0
	end2

.global BattleScript_Lv200B_SwitchInOmniboost
BattleScript_Lv200B_SwitchInOmniboost:
	setstatchanger STAT_ATK | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_SwitchInOmniboostDef
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_SwitchInOmniboostDef
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_SwitchInOmniboostDef:
	setstatchanger STAT_DEF | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_SwitchInOmniboostSpeed
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_SwitchInOmniboostSpeed
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_SwitchInOmniboostSpeed:
	setstatchanger STAT_SPD | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_SwitchInOmniboostSpAtk
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_SwitchInOmniboostSpAtk
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_SwitchInOmniboostSpAtk:
	setstatchanger STAT_SPATK | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_SwitchInOmniboostSpDef
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_SwitchInOmniboostSpDef
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_SwitchInOmniboostSpDef:
	setstatchanger STAT_SPDEF | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_SwitchInOmniboostEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_SwitchInOmniboostEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_SwitchInOmniboostEnd:
	end2

@ B側は各技が独立した完全な台本を持つ。既存の技アニメ表をそのまま使う。
.macro LV200_B_DAMAGE name
	.global BattleScript_Lv200B_\name
BattleScript_Lv200B_\name:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_Handle_\name
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
.endm

.macro LV200_B_STATUS name
	.global BattleScript_Lv200B_\name
BattleScript_Lv200B_\name:
	attackcanceler
	callasm Lv200B_CheckStatusMove
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	attackanimation
	waitanimation
	callasm Lv200B_Handle_\name
	goto BS_MOVE_END
.endm

.macro LV200_B_MULTI name, prepare
	.global BattleScript_Lv200B_\name
BattleScript_Lv200B_\name:
	attackcanceler
	callasm \prepare
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_Handle_\name
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
.endm

.macro LV200_B_PIVOT name
	.global BattleScript_Lv200B_\name
BattleScript_Lv200B_\name:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	copybyte SWITCHING_BANK USER_BANK
	jumpifcannotswitch BANK_SWITCHING | ATK4F_DONT_CHECK_STATUSES BS_MOVE_END
	jumpifnoviablemonsleft BANK_TARGET BS_MOVE_END
	jumpiffainted BANK_SWITCHING BS_MOVE_END
	callasm Lv200B_Handle_\name
	openpartyscreen BANK_SWITCHING BS_MOVE_END
	switchoutabilities BANK_SWITCHING
	waitstateatk
	switchhandleorder BANK_SWITCHING 0x2
	returntoball BANK_SWITCHING
	callasm TryRemovePrimalWeatherSwitchingBank
	getswitchedmondata BANK_SWITCHING
	switchindataupdate BANK_SWITCHING
	hpthresholds BANK_SWITCHING
	printstring 0x3
	switchinanim BANK_SWITCHING 0x1
	waitstateatk
	switchineffects BANK_SWITCHING
	goto BS_MOVE_END
.endm

.macro LV200_B_QUARTER_RECOIL name, prepare=Lv200B_PrepareQuarterRecoil
	.global BattleScript_Lv200B_\name
BattleScript_Lv200B_\name:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	callasm \prepare
	jumpifword EQUALS DAMAGE_LOC 0x0 Lv200B_QuarterRecoilEnd_\name
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	healthbarupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 100 @ STRINGID_PKMNHITWITHRECOIL
	waitmessage DELAY_1SECOND
	faintpokemon BANK_ATTACKER 0x0 0x0
Lv200B_QuarterRecoilEnd_\name:
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
.endm

LV200_B_DAMAGE water_tide
@ 直近被ダメージの2倍を固定ダメージとして返す。
.global BattleScript_Lv200B_water_counter
BattleScript_Lv200B_water_counter:
	attackcanceler
	callasm Lv200B_PrepareWaterCounter
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED_PRE
	accuracycheck BS_MOVE_MISSED 0x0
	attackstring
	ppreduce
	typecalc2
	bicbyte OUTCOME OUTCOME_SUPER_EFFECTIVE | OUTCOME_NOT_VERY_EFFECTIVE
	adjustsetdamage
	callasm Lv200B_Handle_water_counter
	goto BS_HIT_FROM_ATTACKANIMATION
.global BattleScript_Lv200B_deep_sea_memory
BattleScript_Lv200B_deep_sea_memory:
	attackcanceler
	attackstring
	ppreduce
	callasm Lv200B_CheckDeepSeaMemory
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED
	attackanimation
	waitanimation
	callasm Lv200B_Handle_deep_sea_memory
	goto BS_MOVE_END
LV200_B_PIVOT stream_doll_pivot
.global BattleScript_Lv200B_great_whirlpool
BattleScript_Lv200B_great_whirlpool:
	attackcanceler
	attackstring
	jumpifsecondarystatus BANK_ATTACKER STATUS2_MULTIPLETURNS Lv200B_GreatWhirlpoolCheckAccuracy
	ppreduce
Lv200B_GreatWhirlpoolCheckAccuracy:
	accuracycheck Lv200B_GreatWhirlpoolResolve 0x0
Lv200B_GreatWhirlpoolResolve:
	typecalc2
	rolloutdamagecalculation
	callasm Lv200B_PrepareGreatWhirlpool
	goto BS_HIT_FROM_DAMAGE_CALC
LV200_B_DAMAGE drop_judgment
.global BattleScript_Lv200B_deep_sea_song
BattleScript_Lv200B_deep_sea_song:
	attackcanceler
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	jumpifstatus BANK_TARGET STATUS_SLEEP Lv200B_DeepSeaSongHeal
	jumpifbehindsubstitute BANK_TARGET FAILED
	trysetsleep BANK_TARGET FAILED
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_SLEEP
	seteffectprimary
	callasm Lv200B_FixDeepSeaSongSleep
	goto BS_MOVE_END
Lv200B_DeepSeaSongHeal:
	callasm Lv200B_PrepareDeepSeaSongHeal
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED
	attackanimation
	waitanimation
	orword HIT_MARKER 0x100
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END
LV200_B_STATUS whirlpool_mirror
.global BattleScript_Lv200B_sea_god_feast
BattleScript_Lv200B_sea_god_feast:
	attackcanceler
	attackstring
	ppreduce
	callasm Lv200B_CheckSeaGodFeast
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED
	attackanimation
	waitanimation
	callasm Lv200B_HealSeaGodReserves
	callasm Lv200B_PrepareSeaGodUserHeal
	jumpifword EQUALS DAMAGE_LOC 0x0 Lv200B_SeaGodFeastPartner
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
Lv200B_SeaGodFeastPartner:
	callasm Lv200B_PrepareSeaGodPartnerHeal
	jumpifword EQUALS DAMAGE_LOC 0x0 BS_MOVE_END
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_TARGET
	datahpupdate BANK_TARGET
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END
LV200_B_DAMAGE atlantis

.global BattleScript_Lv200B_root_drain
BattleScript_Lv200B_root_drain:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	jumpiffainted BANK_ATTACKER Lv200B_RootDrainFinish
	callasm Lv200B_PrepareRootDrain
	jumpifword EQUALS DAMAGE_LOC 0x0 Lv200B_RootDrainFinish
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE
	jumpifability BANK_TARGET ABILITY_LIQUIDOOZE Lv200B_RootDrainLiquidOoze
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x1 Lv200B_RootDrainFinish
	playanimation BANK_ATTACKER ANIM_HEALING_SPARKLES 0x0
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
	goto Lv200B_RootDrainFinish
Lv200B_RootDrainLiquidOoze:
	copybyte BATTLE_SCRIPTING_BANK TARGET_BANK
	call BattleScript_AbilityPopUp
	manipulatedamage 0x0
	setbyte MULTISTRING_CHOOSER 0x1
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printfromtable 0x83C45A8 @ gLeechSeedDrainStringIds
	waitmessage DELAY_1SECOND
	call BattleScript_AbilityPopUpRevert
	faintpokemon BANK_ATTACKER 0x0 0x0
Lv200B_RootDrainFinish:
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END

.global BattleScript_Lv200B_binding_seed
BattleScript_Lv200B_binding_seed:
	attackcanceler
	jumpifbehindsubstitute BANK_TARGET FAILED_PRE
	attackstring
	ppreduce
	accuracycheck Lv200B_BindingSeedApply 0x0
Lv200B_BindingSeedApply:
	setleechseed
	attackanimation
	waitanimation
	callasm Lv200B_Handle_binding_seed
	printfromtable 0x83C4524 @ gLeechSeedStringIds
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END

.global BattleScript_Lv200B_uproot_attack
BattleScript_Lv200B_uproot_attack:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_PrepareUprootAttack
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x0 Lv200B_UprootAttackFinish
	callasm TransferTerrainData
	waitstateatk
	playanimation 0x0 ANIM_LOAD_DEFAULT_BG 0x0
	printstring 0x184
	waitmessage DELAY_1SECOND
	callasm TryActivateMimicry
Lv200B_UprootAttackFinish:
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
.global BattleScript_Lv200B_dividing_breath
BattleScript_Lv200B_dividing_breath:
	attackcanceler
	ppreduce
	attackstring
	waitstateatk
	jumpifbehindsubstitute BANK_ATTACKER 0x81BB2DD
	setsubstituteeffect
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x1 Lv200B_DividingBreathFailed
	callasm Lv200B_Handle_dividing_breath
	goto 0x81BB2CA
Lv200B_DividingBreathFailed:
	pause DELAY_HALFSECOND
	goto 0x81BB2D0

@ ぬいぐるみのかげ: ぶんかつのいぶきと同じ素のみがわり台本（スプライト・behindSubstitute・HPバー）。壊れた時の攻撃-2は C の OnMoveHit。
.global BattleScript_Lv200B_plush_shadow
BattleScript_Lv200B_plush_shadow:
	attackcanceler
	ppreduce
	attackstring
	waitstateatk
	jumpifbehindsubstitute BANK_ATTACKER 0x81BB2DD
	setsubstituteeffect
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x1 Lv200B_PlushShadowFailed
	callasm Lv200B_Handle_plush_shadow
	goto 0x81BB2CA
Lv200B_PlushShadowFailed:
	pause DELAY_HALFSECOND
	goto 0x81BB2D0

LV200_B_DAMAGE leech_seed_rain
.global BattleScript_Lv200B_great_tree_armor
BattleScript_Lv200B_great_tree_armor:
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
	setstatchanger STAT_DEF | INCREASE_2
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_GreatTreeArmorSpDef
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_GreatTreeArmorSpDef
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_GreatTreeArmorSpDef:
	setstatchanger STAT_SPDEF | INCREASE_2
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_GreatTreeArmorRoot
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_GreatTreeArmorRoot
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_GreatTreeArmorRoot:
	callasm Lv200B_Handle_great_tree_armor
	goto BS_MOVE_END

.global BattleScript_Lv200B_photosynthesis
BattleScript_Lv200B_photosynthesis:
	attackcanceler
	callasm Lv200B_CheckPhotosynthesis
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED_PRE
	attackstring
	ppreduce
	attackanimation
	waitanimation
	callasm Lv200B_PreparePhotosynthesis
	jumpifword EQUALS DAMAGE_LOC 0x0 Lv200B_PhotosynthesisStatus
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
Lv200B_PhotosynthesisStatus:
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x0 Lv200B_PhotosynthesisEnd
	refreshhpbar BANK_ATTACKER
	printstring 0xA7 @ STRINGID_PKMNSTATUSNORMAL
	waitmessage DELAY_1SECOND
Lv200B_PhotosynthesisEnd:
	goto BS_MOVE_END

@ ゆうだちのもり: 雨でも晴れでもためなし。それ以外はソーラービームと同じ2ターン（ため→2ターン目）。md 詳細
.global BattleScript_Lv200B_shower_forest
BattleScript_Lv200B_shower_forest:
	callasm Lv200B_CheckShowerForestWeather
	jumpifbyte NOTEQUALS MULTISTRING_CHOOSER 0x0 Lv200B_ShowerForestInstant
	jumpifsecondarystatus BANK_ATTACKER STATUS2_MULTIPLETURNS Lv200B_ShowerForestSecond
	jumpifword ANDS HIT_MARKER HITMARKER_NO_ATTACKSTRING Lv200B_ShowerForestSecond
	setbyte TWOTURN_STRINGID, 0x1
	call BattleScript_FirstChargingTurn
	call BattleScript_CheckPowerHerb
	goto BS_MOVE_END
Lv200B_ShowerForestInstant:
	orword HIT_MARKER HITMARKER_CHARGING
	setmoveeffect MOVE_EFFECT_CHARGING | MOVE_EFFECT_AFFECTS_USER
	seteffectprimary
	ppreduce
	callasm ClearCalculatedSpreadMoveData
Lv200B_ShowerForestSecond:
	attackcanceler
	setmoveeffect MOVE_EFFECT_CHARGING
	setbyte ANIM_TURN 0x1
	clearstatus BANK_ATTACKER
	orword HIT_MARKER HITMARKER_NO_PPDEDUCT
	clearsemiinvulnerablebit
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_Handle_shower_forest
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
.global BattleScript_Lv200B_forest_light
BattleScript_Lv200B_forest_light:
	attackcanceler
	attackstringnoprotean
	ppreduce
	callasm Lv200B_PrepareForestLight
	callasm TransferTerrainData
	waitstateatk
	attackanimation
	waitanimation
	printstring 0x184
	waitmessage DELAY_1SECOND
	setbyte SEED_HELPER 0x0
	callasm SeedRoomServiceLooper
	copybyte USER_BANK TARGET_BANK
	callasm TryActivateMimicry
	goto BS_MOVE_END

LV200_B_DAMAGE forest_judgment

LV200_B_DAMAGE charge_spark
LV200_B_QUARTER_RECOIL electric_tackle, Lv200B_PrepareThirdRecoil @ ボルテッカーと同じ反動1/3（md 詳細）
LV200_B_DAMAGE sealing_shock
LV200_B_DAMAGE magnetic_whip
.global BattleScript_Lv200B_thunderous_echo
BattleScript_Lv200B_thunderous_echo:
	attackcanceler
	attackstringnoprotean
	ppreduce
	attackanimation
	waitanimation
	callasm Lv200B_PrepareThunderousEcho
	callasm TransferTerrainData
	waitstateatk
	playanimation2 BANK_ATTACKER ANIM_ARG_1 0x0
	printstring 0x184
	waitmessage DELAY_1SECOND
	setbyte SEED_HELPER 0x0
	callasm SeedRoomServiceLooper
	copybyte USER_BANK TARGET_BANK
	callasm TryActivateMimicry
	setstatchanger STAT_SPD | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_ThunderousEchoEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_ThunderousEchoEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_ThunderousEchoEnd:
	goto BS_MOVE_END

LV200_B_PIVOT floating_shock
.global BattleScript_Lv200B_electromagnetic_blizzard
BattleScript_Lv200B_electromagnetic_blizzard:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	setmoveeffect MOVE_EFFECT_PARALYSIS
	seteffectwithchancetarget @ 表の確率40（りんぷん・おんみつマント・しんぴのまもりを通る）
Lv200B_ElectromagneticBlizzardFinish:
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END

.global BattleScript_Lv200B_microwave
BattleScript_Lv200B_microwave:
	attackcanceler
	jumpifbehindsubstitute BANK_TARGET FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_PARALYSIS
	seteffectprimary
	callasm Lv200B_Handle_microwave
	goto BS_MOVE_END

LV200_B_QUARTER_RECOIL dead_heat
.global BattleScript_Lv200B_tempest
BattleScript_Lv200B_tempest:
	attackcanceler
	callasm Lv200B_PrepareTempest
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_Handle_tempest
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x0 Lv200B_TempestNoTerrain @ 場張りは最後のヒットの後だけ
	callasm TransferTerrainData
	waitstateatk
	playanimation2 BANK_ATTACKER ANIM_ARG_1 0x0
	printstring 0x184
	waitmessage DELAY_1SECOND
	setbyte SEED_HELPER 0x0
	callasm SeedRoomServiceLooper
	copybyte USER_BANK TARGET_BANK
	callasm TryActivateMimicry
Lv200B_TempestNoTerrain:
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END

LV200_B_STATUS mirror_mirror
@ 既存のみらいよち枠へ登録し、着弾時は現在の場の相手へ当てる。
.global BattleScript_Lv200B_prophetic_heart
BattleScript_Lv200B_prophetic_heart:
	attackcanceler
	attackstringnoprotean
	ppreduce
	setfutureattack FAILED
	callasm Lv200B_Handle_prophetic_heart
	tryactivateprotean
	attackanimation
	waitanimation
	printfromtable 0x83C45BA
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END
LV200_B_STATUS mint_wall
.global BattleScript_Lv200B_edge_of_mind
BattleScript_Lv200B_edge_of_mind:
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
	cureprimarystatus BANK_ATTACKER Lv200B_EdgeOfMindHeal
	refreshhpbar BANK_ATTACKER
Lv200B_EdgeOfMindHeal:
	callasm Lv200B_PrepareEdgeOfMindHeal
	jumpifword EQUALS DAMAGE_LOC 0x0 Lv200B_EdgeOfMindSleep
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	printstring 0x4B @ STRINGID_PKMNREGAINEDHEALTH
	waitmessage DELAY_1SECOND
Lv200B_EdgeOfMindSleep:
	callasm Lv200B_ApplyEdgeOfMindSleep
	pause DELAY_HALFSECOND
	printfromtable 0x83C452E @ gRestUsedStringIds
	waitmessage DELAY_1SECOND
	refreshhpbar BANK_ATTACKER
	waitstateatk
	goto BS_MOVE_END
LV200_B_DAMAGE vacuum_attack
LV200_B_STATUS mind_sharing
LV200_B_DAMAGE source_wave
.global BattleScript_Lv200B_black_hole
BattleScript_Lv200B_black_hole:
	attackcanceler
	attackstring
	ppreduce
	callasm Lv200B_PrepareBlackHole
	attackanimation
	waitanimation
	printstring 0x184
	waitmessage DELAY_1SECOND
	callasm BringDownMons
	goto BS_MOVE_END
.global BattleScript_Lv200B_suspicious_sleep
BattleScript_Lv200B_suspicious_sleep:
	attackcanceler
	jumpifbehindsubstitute BANK_TARGET FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	trysetsleep BANK_TARGET FAILED
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_SLEEP
	seteffectprimary
	callasm Lv200B_Handle_suspicious_sleep
	goto BS_MOVE_END
LV200_B_DAMAGE psychic_unison

LV200_B_DAMAGE frostbite
.global BattleScript_Lv200B_blizzard_armor
BattleScript_Lv200B_blizzard_armor:
	attackcanceler
	attackstringnoprotean
	ppreduce
	callasm Lv200B_PrepareBlizzardArmor
	attackanimation
	waitanimation
	printfromtable 0x83C44F4 @ gMoveWeatherChangeStringIds
	waitmessage DELAY_1SECOND
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_BlizzardArmorDefense
	call BS_WEATHER_FORM_CHANGES
	callasm ClearHailStartFlag
Lv200B_BlizzardArmorDefense:
	setstatchanger STAT_DEF | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_BlizzardArmorSpDef
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_BlizzardArmorSpDef
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_BlizzardArmorSpDef:
	setstatchanger STAT_SPDEF | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_BlizzardArmorEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_BlizzardArmorEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_BlizzardArmorEnd:
	goto BS_MOVE_END
LV200_B_DAMAGE glacier_awakening
LV200_B_STATUS freezing_master
.global BattleScript_Lv200B_sea_of_ice
BattleScript_Lv200B_sea_of_ice:
	attackcanceler
	attackstringnoprotean
	ppreduce
	callasm Lv200B_PrepareSeaOfIce
	attackanimation
	waitanimation
	printfromtable 0x83C44F4 @ gMoveWeatherChangeStringIds
	waitmessage DELAY_1SECOND
	jumpifmovehadnoeffect Lv200B_SeaOfIceEnd
	call BS_WEATHER_FORM_CHANGES
	callasm ClearHailStartFlag
Lv200B_SeaOfIceEnd:
	goto BS_MOVE_END
LV200_B_DAMAGE iceberg_collapse
LV200_B_MULTI summit_icicle, Lv200B_PrepareSummitIcicle
LV200_B_STATUS cold_body_gaze
LV200_B_STATUS ice_age
.global BattleScript_Lv200B_absolute_zero
BattleScript_Lv200B_absolute_zero:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	jumpiffainted BANK_TARGET Lv200B_AbsoluteZeroFinish
	setmoveeffect MOVE_EFFECT_FREEZE
	seteffectwithchancetarget @ 表の確率100
	callasm Lv200B_Handle_absolute_zero
Lv200B_AbsoluteZeroFinish:
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END

.global BattleScript_Lv200B_dragon_soul
BattleScript_Lv200B_dragon_soul:
	attackcanceler
	attackstring
	ppreduce
	callasm Lv200B_PrepareDragonSoul
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x0 FAILED
	attackanimation
	waitanimation
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	healthbarupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	bicword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	setstatchanger STAT_ATK | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonSoulDefense
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonSoulDefense
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonSoulDefense:
	setstatchanger STAT_DEF | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonSoulSpeed
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonSoulSpeed
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonSoulSpeed:
	setstatchanger STAT_SPD | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonSoulSpAtk
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonSoulSpAtk
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonSoulSpAtk:
	setstatchanger STAT_SPATK | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonSoulSpDef
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonSoulSpDef
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonSoulSpDef:
	setstatchanger STAT_SPDEF | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonSoulEnd
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonSoulEnd
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonSoulEnd:
	goto BS_MOVE_END

.global BattleScript_Lv200B_dragon_reversal
BattleScript_Lv200B_dragon_reversal:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	setmoveeffect MOVE_EFFECT_FLINCH
	seteffectwithchancetarget
	callasm Lv200B_RecordDragonReversalFlinch
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
LV200_B_DAMAGE last_dragon
LV200_B_STATUS dragon_palace
.global BattleScript_Lv200B_dragon_curse
BattleScript_Lv200B_dragon_curse:
	attackcanceler
	attackstring
	ppreduce
	cursetarget FAILED
	callasm Lv200B_PrepareDragonCurseCost
	attackanimation
	waitanimation
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	healthbarupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	bicword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	printstring 0x92 @ STRINGID_PKMNLAIDCURSE
	waitmessage DELAY_1SECOND
	callasm Lv200B_PrepareDragonCurseBackfire @ Q26: 相手がドラゴンなら使った側が最大HPの1/8（呪い返し）
	jumpifbyte NOTEQUALS MULTISTRING_CHOOSER 0x1 Lv200B_DragonCurseDone
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	healthbarupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	bicword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
Lv200B_DragonCurseDone:
	faintpokemon BANK_ATTACKER 0x0 0x0
	goto BS_MOVE_END
LV200_B_DAMAGE time_tyrant
LV200_B_MULTI twin_dragon_claw, Lv200B_PrepareTwinDragonClaw
.global BattleScript_Lv200B_dragon_melody
BattleScript_Lv200B_dragon_melody:
	attackcanceler
	attackstring
	ppreduce
	attackanimation
	waitanimation
	setstatchanger STAT_ATK | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonMelodySpAtk
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonMelodySpAtk
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonMelodySpAtk:
	setstatchanger STAT_SPATK | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonMelodySpeed
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonMelodySpeed
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonMelodySpeed:
	setstatchanger STAT_SPD | INCREASE_1
	statbuffchange STAT_ATTACKER | STAT_BS_PTR Lv200B_DragonMelodySureHit
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonMelodySureHit
	setgraphicalstatchangevalues
	playanimation BANK_ATTACKER ANIM_STAT_BUFF ANIM_ARG_1
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200B_DragonMelodySureHit:
	callasm Lv200B_PrepareDragonMelody
	goto BS_MOVE_END

.global BattleScript_Lv200B_dragon_temptation
BattleScript_Lv200B_dragon_temptation:
	attackcanceler
	jumpifbehindsubstitute BANK_TARGET FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	attackanimation
	waitanimation
	callasm Lv200B_PrepareDragonTemptation
	waitstateatk
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x0 FAILED
	jumpifbyte NOTEQUALS MULTISTRING_CHOOSER 0x2 Lv200B_DragonTemptationAbility
	playanimation BANK_TARGET ANIM_STEAL_ITEM 0x0
	printstring 0x8E
	waitmessage DELAY_1SECOND
	callasm TryRemovePrimalWeatherAfterItemChange
	tryactivateswitchinability BANK_ATTACKER
	call BS_WEATHER_FORM_CHANGES
Lv200B_DragonTemptationAbility:
	copybyte BATTLE_SCRIPTING_BANK TARGET_BANK
	setword BATTLE_STRING_LOADER AbilitySuppressedString
	printstring 0x184
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END

.global BattleScript_Lv200B_dragon_end
BattleScript_Lv200B_dragon_end:
	attackcanceler
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_Handle_dragon_end
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x0 Lv200B_DragonEndFinish
	printstring 0xF9
	waitmessage DELAY_1SECOND
Lv200B_DragonEndFinish:
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END

LV200_B_DAMAGE copy_theft
LV200_B_STATUS cowardice_of_heart
@ ふいうちと同じ失敗条件（相手が攻撃技を選んでいて、まだ動いていない時だけ成功）
.global BattleScript_Lv200B_dark_ambush
BattleScript_Lv200B_dark_ambush:
	attackcanceler
	callasm TrySuckerPunch
	accuracycheck BS_MOVE_MISSED 0x0
	call STANDARD_DAMAGE
	callasm Lv200B_Handle_dark_ambush
	prefaintmoveendeffects 0x0
	faintpokemonaftermove
	setbyte CMD49_STATE 0x0
	cmd49 0x0 0x0
	goto BS_MOVE_END
LV200_B_STATUS spreading_darkness
LV200_B_DAMAGE crumbling_boundary
.global BattleScript_Lv200B_betrayal_song
BattleScript_Lv200B_betrayal_song:
	attackcanceler
	callasm Lv200B_CheckStatusMove
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	attackanimation
	waitanimation
	callasm Lv200B_Handle_betrayal_song
	goto BS_MOVE_END
LV200_B_DAMAGE soul_eater
LV200_B_STATUS world_of_darkness
LV200_B_STATUS king_of_night
LV200_B_DAMAGE darkness_hole

LV200_B_DAMAGE heartthrob_spark
@ 既存まもる判定を使い、成功時だけ接触メロメロ用bitを立てる。
.global BattleScript_Lv200B_heart_guard
BattleScript_Lv200B_heart_guard:
	attackcanceler
	attackstringnoprotean
	ppreduce
	callasm Lv200B_PrepareHeartGuard
	setprotect
	callasm Lv200B_Handle_heart_guard
	tryactivateprotean
	attackanimation
	waitanimation
	printfromtable 0x83C4512
	waitmessage DELAY_1SECOND
	goto BS_MOVE_END
LV200_B_STATUS magical_sugar
LV200_B_DAMAGE love_flash
@ ようせいのわ: ミストフィールドを素の設置流れで張る（文・絵・フィールド特性の再評価）。回復は終了時。
.global BattleScript_Lv200B_fairy_ring
BattleScript_Lv200B_fairy_ring:
	attackcanceler
	attackstringnoprotean
	ppreduce
	callasm Lv200B_PrepareFairyRing
	callasm TransferTerrainData
	waitstateatk
	attackanimation
	waitanimation
	printstring 0x184
	waitmessage DELAY_1SECOND
	setbyte SEED_HELPER 0x0
	callasm SeedRoomServiceLooper
	copybyte USER_BANK TARGET_BANK
	callasm TryActivateMimicry
	goto BS_MOVE_END
.global BattleScript_Lv200B_dream_princess
BattleScript_Lv200B_dream_princess:
	attackcanceler
	callasm Lv200B_CheckStatusMove
	jumpifword EQUALS DAMAGE_LOC 0x0 FAILED_PRE
	attackstring
	ppreduce
	accuracycheck BS_MOVE_MISSED 0x0
	attackanimation
	waitanimation
	setmoveeffect MOVE_EFFECT_SLEEP
	seteffectprimary
	callasm Lv200B_FixDreamPrincessSleep
	callasm Lv200B_Handle_dream_princess
	goto BS_MOVE_END
.global BattleScript_Lv200B_small_miracle
BattleScript_Lv200B_small_miracle:
	attackcanceler
	jumpifcannotswitch BANK_ATTACKER | ATK4F_DONT_CHECK_STATUSES FAILED_PRE
	attackstring
	ppreduce
	attackanimation
	waitanimation
	callasm Lv200B_Handle_small_miracle
	setuserhptozero
	graphicalhpupdate BANK_ATTACKER
	datahpupdate BANK_ATTACKER
	faintpokemon BANK_ATTACKER 0x0 0x0
	goto BS_MOVE_END
LV200_B_STATUS milky_gate
LV200_B_DAMAGE happy_breath

LV200_B_DAMAGE put_away_memory
LV200_B_STATUS phantom_memory
LV200_B_STATUS ril_gaze
LV200_B_STATUS lonely_prayer
LV200_B_DAMAGE tear_moon
LV200_B_DAMAGE lonely_shadow
LV200_B_DAMAGE law_of_the_moon
LV200_B_STATUS keepsake
LV200_B_STATUS until_goodbye
LV200_B_DAMAGE return_to_night
