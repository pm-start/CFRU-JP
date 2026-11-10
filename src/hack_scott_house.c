#include "defines.h"
#include "../include/event_data.h"
#include "../include/random.h"
#include "Tables/hoenn_script_flags.h"
#include "hack_scott_house.h"

/*
 * エニシダの家（群 69 の 46）の台本 assembly/hack_scott_house/scott_house_scripts.s が callasm で呼ぶ（2026-10-25・Redmine #115・patches/0816）。
 * 原文 参考/pokeemerald/data/maps/BattleFrontier_ScottsHouse/scripts.inc の CheckGiveItems の順を守る:
 *   銀の旗が未 → 7 施設の銀が全部そろっていたら サンのみ／そうでなければ ふだんの話
 *   銀の旗が済 かつ 金の旗が未 → 7 施設の金が全部そろっていたら スターのみ
 * シンボルは HOENN_VAR_FRONTIER_SYMBOLS に 2 ビットずつ（0 無し／1 銀／2 金・施設 7 つ＝14 ビット・src/hoenn_tower.c）
 */
#define SCOTT_FACILITIES 7

//LASTRESULT: 1 = サンのみを渡す／2 = スターのみを渡す／0 = どちらでもない（ふだんの話）
void HackScott_Check(void)
{
	u16 symbols = VarGet(HOENN_VAR_FRONTIER_SYMBOLS);
	u8 i, silver = 0, gold = 0;

	for (i = 0; i < SCOTT_FACILITIES; ++i)
	{
		u8 grade = (symbols >> (i * 2)) & 3; //シンボルの段階（0 なし・1 銀・2 金）。ポケモンのレベルではない

		if (grade >= 1)
			++silver;
		if (grade >= 2)
			++gold;
	}

	gSpecialVar_LastResult = 0;
	if (!FlagGet(HACK_FLAG_SCOTT_SILVER_BERRY))
	{
		if (silver == SCOTT_FACILITIES)
			gSpecialVar_LastResult = 1;
	}
	else if (!FlagGet(HACK_FLAG_SCOTT_GOLD_BERRY))
	{
		if (gold == SCOTT_FACILITIES)
			gSpecialVar_LastResult = 2;
	}
}

//ふだんの話（原文の RandomComment の 3 種に、初回の挨拶と BP の説明を足した 4 択）→ LASTRESULT 0〜3
//（初回だけの挨拶は BP の贈り物とセットの旗が要るが、ユーザーが贈り物を入れないと決めたので旗は作らず、4 択の 1 つにした）
void HackScott_RandomLine(void)
{
	gSpecialVar_LastResult = Random() % 4;
}
