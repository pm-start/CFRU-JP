#pragma once
#include "defines.h"

/*
 * ダーク技の組み立て（N8）の戦闘・つよさの画面の側（段 E・patches/0963・src/shadow_battle.c）。並びの規則は src/shadow_gauge.h。
 * 手持ちの技欄・PP は戦闘のあいだ書き換えない（控えは gNewBS->hackShadowView）。戦闘のあと元の PP を手持ちに戻す。
 * 縛り F わざガチャ（src/hack_chf_rotgacha.c）と同じ 5 か所に口を並べた（ガチャ中は何もしない＝ガチャが優先）。
 */
void ShadowBattle_OnBattleStart(void);          //battle_start_turn_start.c BattleBeginFirstTurn: 場に出た子のゲージを引いて並びを作る（両陣営）
void ShadowBattle_OnSwitchOut(u8 bank);         //switching.c／hack_mid_battle_evo.c: 出ていく子の PP を控える
void ShadowBattle_OnSwitchIn(u8 bank);          //同: 出てきた子の並びを作る（ゲージはその戦闘で初めて出たときだけ減る）
void ShadowBattle_OnEvolved(u8 bank);           //hack_mid_battle_evo.c: 戦闘中の進化のあと 並びを作り直す（status2 が正しい口＝へんしん中は除く）
void ShadowBattle_OnBattleEnd(void);            //end_battle.c EndOfBattleThings の頭: 手持ちの PP・ポイントアップを元に戻す

//つよさの画面: ダークの子なら 4 行の並び（moves／src）を作って 1 を返す。ダークでなければ 0（moves・src は触らない）
bool8 ShadowSummary_Build(struct Pokemon* mon, u16* moves, u8* src);

//ダークの子の成長を止める（経験値・努力値・ふしぎなアメ・ビタミン・努力値きのみ・進化・育て屋。リライブ＝段 G まで）
bool8 Shadow_RefusesGrowth(struct Pokemon* mon);
bool8 Shadow_RefusesGrowthBox(struct BoxPokemon* mon);
