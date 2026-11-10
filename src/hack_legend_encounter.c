#include "defines.h"
#include "hack_base_stats_wide.h" //[hack] patches/0678 種族値の u16 の窓口
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/pokemon.h"
#include "../include/constants/flags.h"
#include "Tables/hoenn_script_flags.h"
#include "hack_legend_encounter.h"
#include "hack_stat_access.h" //[hack] patches/0677 u16 窓口

/*
	わたしたちの伝説 10 匹の出会い（段 C・2026-10-16 ユーザー確定）。
	  - 出会い: カントー 8 ＋ ホウエン 8 の 16 個のバッジをそろえたら、版ごとのランダムな場所（外か洞窟）に 1 匹ずつ現れる。
	    場所は tools/rom/legend_spot.py（人の表に道具の玉の絵の物体を足す＝build_hoenn.people_append_for）・
	    話しかけると assembly/hack_legend/legend_scripts.s が Lv180 の戦いを始める（種族は台本ごとに決め打ち）。
	  - 隠す: 人の表の旗は「立っていれば隠す」。バッジが 16 個そろうまで・捕まえた／倒したあとは LEGEND_FLAG_HIDE_FIRST+i を立てる。
	    フィールドの毎コマ（src/overworld_speedup.c）に 8 コマごと呼ぶ。地図に入るときの旗の状態で出る・出ないが決まる
	    （16 個目のバッジを取ったあと、その地図に入り直すと出る）。印の旗はセーブに残り、隠す旗も立てっぱなしで残る。
	  - 状態を持たない（6.13）。旗 FlagGet/FlagSet だけ。
*/

u8 HackLegend_BadgeCount(void)
{
	static const u16 sHoennBadgeFlags[8] =
	{
		HOENN_FLAG_BADGE1, HOENN_FLAG_BADGE2, HOENN_FLAG_BADGE3, HOENN_FLAG_BADGE4,
		HOENN_FLAG_BADGE5, HOENN_FLAG_BADGE6, HOENN_FLAG_BADGE7, HOENN_FLAG_BADGE8,
	};
	u8 n = 0;
	u32 i;

	for (i = 0; i < 8; ++i)
	{
		if (FlagGet(FLAG_BADGE01_GET + i))
			++n;
		if (FlagGet(sHoennBadgeFlags[i]))
			++n;
	}
	return n;
}

//殿堂入りの記録が終わったとき（HoennLeague_SetWarpsAfterHallOfFame から・カントー／ホウエン／ジョウト／シンオウとも通る）:
//  捕まえた印 10 個と隠す旗 10 個を下ろす＝また出会える（2026-10-16 ユーザー「一回捕まえたらフラグ立てて殿堂入りしたらフラグおって」）。
//  隠す旗は HackLegend_FieldTick が次の周期でバッジの数と印から立て直す。旗だけ（6.13）
void HackLegend_OnHallOfFame(void)
{
	u32 i;

	for (i = 0; i < LEGEND_COUNT; ++i)
	{
		FlagClear(LEGEND_FLAG_DONE_FIRST + i);
		FlagClear(LEGEND_FLAG_HIDE_FIRST + i);
	}
}

//フィールドの毎コマ（8 コマごと）: 16 バッジそろっていて まだ捕まえていない／倒していない子だけ隠す旗を下ろす
void HackLegend_FieldTick(void)
{
	u32 i;
	bool8 open;

	if ((gMain.vblankCounter2 & 7) != 5)
		return;
	open = HackLegend_BadgeCount() >= LEGEND_BADGES_NEEDED;
	for (i = 0; i < LEGEND_COUNT; ++i)
	{
		if (open && !FlagGet(LEGEND_FLAG_DONE_FIRST + i))
		{
			if (FlagGet(LEGEND_FLAG_HIDE_FIRST + i))
				FlagClear(LEGEND_FLAG_HIDE_FIRST + i);
		}
		else if (!FlagGet(LEGEND_FLAG_HIDE_FIRST + i))
			FlagSet(LEGEND_FLAG_HIDE_FIRST + i);
	}
}

//callasm: setwildbattle で作った gEnemyParty[0] の個体値を 31・努力値を種族値の高い 2 つに 252 ずつにして能力を計算し直す（設計資料 6 章）。
//  性格値・色違いには触らない（6.5）。個体値・努力値は色違いと無関係
void HackLegend_FixWild(void)
{
	struct Pokemon* mon = &gEnemyParty[0];
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u32 i, j, best, bestVal;
	u16 base[6]; //[hack] patches/0678
	u8 used = 0;
	u16 ev;
	u16 hp;

	if (species == SPECIES_NONE)
		return;
	for (i = 0; i < 6; ++i)
		base[i] = HackBaseStat(species, i); //[hack] patches/0678
	//並びは MON_DATA_HP_IV 〜 SPDEF_IV（39〜44）・HP_EV 〜 SPDEF_EV（26〜31）が HP・こうげき・ぼうぎょ・すばやさ・とくこう・とくぼう
	for (i = 0; i < 6; ++i)
	{
		ev = 0;
		HackMon_SetIv(mon, i, HACK_IV_MAX); //[hack] patches/0677
		HackMon_SetEv(mon, i, ev);
	}
	for (j = 0; j < 2; ++j)
	{
		best = 6;
		bestVal = 0;
		for (i = 0; i < 6; ++i)
		{
			if (!(used & (1 << i)) && base[i] >= bestVal)
			{
				best = i;
				bestVal = base[i];
			}
		}
		if (best < 6)
		{
			used |= 1 << best;
			HackMon_SetEv(mon, best, HACK_EV_PER_STAT_CAP); //[hack] patches/0677
		}
	}
	CalculateMonStats(mon);
	hp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	SetMonData(mon, MON_DATA_HP, &hp);
}

//callasm: Var8004 番目（0〜9）の子に話しかけてよいか → Var800D（LASTRESULT）= 1 ならよい。16 バッジそろっていて まだ捕まえていない／倒していない。
//  人の表の隠す旗は地図に入るときしか効かない（旗が 0 の古いセーブをその地図でつづきからした・バッジが足りないまま地図に残っている）ので、話しかけたときにも確かめる
void HackLegend_CanTalk(void)
{
	Var800D = (Var8004 < LEGEND_COUNT && HackLegend_BadgeCount() >= LEGEND_BADGES_NEEDED && !FlagGet(LEGEND_FLAG_DONE_FIRST + Var8004)) ? 1 : 0;
}
