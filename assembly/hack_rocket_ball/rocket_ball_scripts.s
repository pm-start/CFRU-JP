.text
.thumb
.align 2
@ ロケットボール（src/hack_rocket_ball.c・2026-10-26・patches/0914）: ロケット団が 場の自分の子を ボールで奪う台本。
@ 入口は item_battle_scripts.s の BattleScript_AIUseGuardSpec の頭（callasm HackRocketBall_ScriptEntry が 予約があるときだけ ここへ飛ばす）

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.global BattleScript_HackRocketBall

BattleScript_HackRocketBall:
	callasm HackRocketBall_Roll
	@ 結果（BATTLE_COMMUNICATION[0]）は printstring／waitmessage が 状態の数として書き換える＝先に分けてから 文を出す（mGBA 2026-10-27: 後で見ると 全部 奪われた）
	jumpifbyte EQUALS BATTLE_COMMUNICATION 0x1 BattleScript_HackRocketBallMiss
	jumpifbyte EQUALS BATTLE_COMMUNICATION 0x2 BattleScript_HackRocketBallRepel
	setword BATTLE_STRING_LOADER gText_RocketBall_Throw
	printstring 0x184
	waitmessage DELAY_1SECOND
@ 奪われる: 吸い込まれて ひんしの流れ（文は「たおれた」でなく「うばわれた」・なつき度とひんしの回数は動かさない）
	callasm HackRocketBall_Steal
	playanimation BANK_SCRIPTING ANIM_CALL_BACK_POKEMON 0x0 @ ANIM_TURN_INTO_POKEBALL は 投げたボールの絵が前に無いと 終わらず 止まった（mGBA 2026-10-27）＝呼び戻しの絵 → returntoball
	waitanimation
	pause DELAY_HALFSECOND
	returntoball BANK_SCRIPTING
	setword BATTLE_STRING_LOADER gText_RocketBall_Stolen
	printstring 0x184
	waitmessage DELAY_1SECOND
	orword HIT_MARKER HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_NON_ATTACK_DMG
	graphicalhpupdate BANK_SCRIPTING
	datahpupdate BANK_SCRIPTING
	copybyte FAINTED_BANK BATTLE_SCRIPTING_BANK
	cleareffectsonfaint BANK_FAINTED
	printstring 0x130
	trytrainerslidefirstdownmsg BANK_FAINTED
	finishaction

@ よけた（なつき度が最大より下で 外れた）
BattleScript_HackRocketBallMiss:
	setword BATTLE_STRING_LOADER gText_RocketBall_Throw
	printstring 0x184
	waitmessage DELAY_1SECOND
	setword BATTLE_STRING_LOADER gText_RocketBall_Miss
	printstring 0x184
	waitmessage DELAY_1SECOND
	finishaction

@ はねかえした（なつき度が最大）: きらめきと ハートの文
BattleScript_HackRocketBallRepel:
	setword BATTLE_STRING_LOADER gText_RocketBall_Throw
	printstring 0x184
	waitmessage DELAY_1SECOND
	playanimation BANK_SCRIPTING ANIM_HEALING_SPARKLES 0x0
	setword BATTLE_STRING_LOADER gText_RocketBall_Repel
	printstring 0x184
	waitmessage DELAY_1SECOND
	playanimation BANK_SCRIPTING ANIM_LEVEL_UP_SHINE 0x0
	finishaction
