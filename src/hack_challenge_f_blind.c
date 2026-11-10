#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "hack_challenge.h"
#include "hack_challenge_f.h"
#include "hack_challenge_f_blind.h"
#include "hack_dummy_battle.h"

/*
    しばり「めかくしバトル」「いっぱつしょうぶ」の判定（縛り F・patches/0951 仮）。
    資料: docs/research/2026-11-03_challenge-f-blind-oneshot.md。検算: tools/rom/verify_challenge_f_blind.py
    状態は持たない（旗と gBattleTypeFlags・gMain.savedCallback だけを見る＝CLAUDE.md 6.13 の static 禁止に合う）。
*/

void CB2_EndWildBattle(void);                 /* 素: 野生戦の終わり（BPRJ.ld） */
void HackWildChain_CB2_EndWildBattle(void);   /* src/hack_wild_chain.c: 野生のむれ付きの終わり */

//戦闘の種類に関わらず 両方の縛りの対象外にする戦い（通信・施設・チュートリアル・ダミー戦・？？？）
static bool8 ChfBattleExcluded(void)
{
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_OAK_TUTORIAL | BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_POKE_DUDE))
		return TRUE;
	if (FlagGet(FLAG_BATTLE_FACILITY) || HackDummy_Active() || FlagGet(HACK_FLAG_CH_BATTLE)) //？？？との戦いは そのまま（Lv9999 の見かけ・捕まえられない）
		return TRUE;
	return FALSE;
}

//めかくし: 相手側（野生・トレーナー・ジム・四天王・ダブル・レイドを含む）の名前・Lv・HP バー・印を隠す。味方側はふつう
bool8 HackChallengeF_BlindBattler(u8 battlerId)
{
	if (!FlagGet(HACK_FLAG_CHF_BLIND))
		return FALSE;
	if (battlerId >= gBattlersCount || GetBattlerSide(battlerId) != B_SIDE_OPPONENT)
		return FALSE;
	return !ChfBattleExcluded();
}

//いっぱつ: atkEF_handleballthrow の「外れた」（揺れの回数が足りない）のうち トレーナー戦・レイドは前の分岐で済んでいる。ここはふつうの野生にだけ。
//イベントの野生（台本の野生 setwildbattle→dowildbattle＝CB2_EndScriptedWildBattle・伝説 sp138＝CB2_EndScriptedWildBattle_2・ゴースト・？？？・ダミー戦・バトルチューブ）は
//  gMain.savedCallback が『ふつうの野生の終わり』ではないので外れる（戦いの始めに DoStandardWildBattle／StartRoamerBattle が置く 2 つだけを許す）。マスターボールは前の分岐で必ず捕まる。
bool8 HackChallengeF_OneShotApplies(void)
{
	if (!FlagGet(HACK_FLAG_CHF_ONESHOT))
		return FALSE;
	if (ChfBattleExcluded())
		return FALSE;
	if (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_SAFARI | BATTLE_TYPE_GHOST | BATTLE_TYPE_SCRIPTED_WILD_1 | BATTLE_TYPE_SCRIPTED_WILD_2
							| BATTLE_TYPE_LEGENDARY_FRLG | BATTLE_TYPE_DYNAMAX | BATTLE_TYPE_MOCK_BATTLE | BATTLE_TYPE_KYOGRE_GROUDON | BATTLE_TYPE_REGI
							| BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI))
		return FALSE;
	return gMain.savedCallback == CB2_EndWildBattle || gMain.savedCallback == HackWildChain_CB2_EndWildBattle;
}
