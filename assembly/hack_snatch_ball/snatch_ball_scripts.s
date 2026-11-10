.thumb
.text
.align 2
/*
snatch_ball_scripts.s（スナッチボール・src/hack_snatch_ball.c・patches/0961）
	組織のトレーナーのダークの個体にスナッチボールを投げて捕まえたとき: 戦闘は終わらず続く。
	捕まえる所は野生の成功の台本（BattleScript_SuccessBallThrow・fainting_battle_scripts.s）と同じ命令を 1 回ずつ。最後の
	「setbyte gBattleOutcome B_OUTCOME_CAUGHT／finishturn」の代わりに、捕まえた子を ひんしの子と同じく相手の手持ちから外し、この行動だけ終える。
	経験値はターンの終わりの「ひんしの処理」（end_turn.c HandleFaintedMonActions）が 1 回だけ配り、次の子を出す／残りが無ければ勝ちにする。
	図鑑のページ・ニックネームの確認は戦闘の画面を出てしまうので出さない（CFRU の FLAG_DONT_OFFER_NICKNAMES_BATTLE と同じ）。
	ジングルは hook（snatch_ball_hooks.s）が戦闘 BGM を止めるだけにし、HackSnatch_WaitCaughtJingle が終わるまで待つ。
*/

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.global BattleScript_SnatchCaught
.global BattleScript_SnatchCaught_Remove
.global BattleScript_SnatchNoTarget

BattleScript_SnatchCaught:
	callasm HackSnatch_Start
	incrementgamestat 0xB @;GAME_STAT_POKEMON_CAPTURES
	setword BATTLE_STRING_LOADER gText_Snatch_Gotcha
	printstring 0x184 @;STRINGID_GOTCHAPKMNCAUGHT の代わり（捕獲の曲の命令なし・strings/hack_snatch_ball.string）
	callasm HackSnatch_WaitCaughtJingle @;ジングルが終わるまで
	copyarray BATTLE_SCRIPTING_BANK TARGET_BANK 0x1
	callasm MakeScriptingBankInvisible
	callasm ApplyBallSpecialEffect
	trysetcaughtmondexflags BattleScript_SnatchCaught_Give
	printstring 0x10F @;STRINGID_PKMNDATAADDEDTODEX
	waitmessage 0x40

BattleScript_SnatchCaught_Give:
	catchpoke
	callasm HackSnatch_SkipPcMessageIfInParty
	printfromtable 0x83C4620 @;gCaughtMonStringIds（箱へ送った）
	waitmessage 0x40

BattleScript_SnatchCaught_Remove:
	callasm HackSnatch_RemoveCaughtMon
	cleareffectsonfaint BANK_TARGET
	trytrainerslidefirstdownmsg BANK_TARGET
	finishaction

@ 投げる時点で相手のダークがもういない（同じターンの 2 つ目のボールなど・バッグが止めるのでふだんは来ない）: 投げずに文を出し、ボールを返す
BattleScript_SnatchNoTarget:
	setword BATTLE_STRING_LOADER gText_Snatch_OnlyDark
	printstring 0x184
	waitmessage 0x40
	callasm HackSnatch_ReturnBall
	finishaction
