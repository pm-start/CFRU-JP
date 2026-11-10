#pragma once

#include "hack_stat_caps.h"

/*
 * 種族値の読み口（u16・[hack] patches/0678・設計 docs/research/2026-10-23_stat-level-16bit.md 2.4 章）
 *
 *   struct BaseStats（0x20 バイト・DPE のデータ・素の機械語が lsl #5 で読む）は触らない。
 *   並列の u16 表 gBaseStatsWide[種族][stat] を足した（src/hack_base_stats_wide.c・生成物・値は u8 の欄と同じ）。
 *   stat は STAT_HP（0）〜STAT_SPDEF（5）＝gBaseStats の先頭 6 バイト（baseHP・Attack・Defense・Speed・SpAttack・SpDefense）と同じ並び。
 *   計算・表示・比較は 全部 この窓口を通す（u8 の欄を直に読まない）。
 */

#define HACK_BASE_STAT_COUNT 6

extern const u16 gBaseStatsWide[][HACK_BASE_STAT_COUNT];

static inline u16 HackBaseStat(u16 species, u8 stat)
{
	return gBaseStatsWide[species][stat];
}

static inline u32 HackBaseStatsTotal(u16 species)
{
	u32 sum = 0;
	u8 i;

	for (i = 0; i < HACK_BASE_STAT_COUNT; ++i)
		sum += gBaseStatsWide[species][i];
	return sum;
}
