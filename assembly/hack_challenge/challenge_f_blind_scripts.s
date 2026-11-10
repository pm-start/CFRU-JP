@ しばり「いっぱつしょうぶ」（縛り F・patches/0951 仮・src/hack_challenge_f_blind.c）の戦闘の台本。文は strings/hack_challenge_f_blind.string。
@ catching.c の atkEF_handleballthrow が『外れたふつうの野生』で HackChallengeF_OneShotApplies() のとき ここへ飛ばす。
@ 作りは BattleScript_RaidMonEscapeBall（et_battle_scripts.s）と同じ: 素の揺れの文 → 相手の絵を消す → 『○○は にげだした！』 → 戦いを終える。
@ 終え方だけ違う: B_OUTCOME_PLAYER_TELEPORTED（5）＝ テレポートで野生が去るときと同じ（素の終わりは文を足さず捕獲にも勝ちにもならない・経験値なし・ローミングは残る）。
.text
.thumb
.align 2

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.align 2
.global BattleScript_OneShotEscapeBall
BattleScript_OneShotEscapeBall:
	printfromtable 0x83C45BE @;gBallEscapeStringIds（素の『ああ！ もうすこしで…』など・揺れた回数で選ぶ）
	waitmessage DELAY_1SECOND
	copybyte BATTLE_SCRIPTING_BANK TARGET_BANK
	callasm MakeScriptingBankInvisible
	setword BATTLE_STRING_LOADER gText_OneShotFled
	printstring 0x184
	waitmessage DELAY_1SECOND
	setbyte BATTLE_OUTCOME 0x5 @B_OUTCOME_PLAYER_TELEPORTED
	finishaction
