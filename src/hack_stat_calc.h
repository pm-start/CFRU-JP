#pragma once

#include "hack_stat_caps.h"

/*
 * 能力値の計算の共通部（u16 の入れ物・途中は u32・65535 で頭打ち・[hack] patches/0678・設計 docs/research/2026-10-23_stat-level-16bit.md 2.3 章）
 *
 *   式（素の FR と同じ）:  最大 HP = ((2 * 種族値 + 個体値 + 努力値 / 4) * Lv) / 100 + Lv + 10
 *                          ほかの能力 = (((2 * 種族値 + 個体値 + 努力値 / 4) * Lv) / 100 + 5) に 性格の補正（×1.1／×0.9）
 *   入れ物が u16（種族値・個体値・努力値・Lv が最大 65535）になっても u32 であふれないよう、(a * Lv) / 100 を
 *   「a を 100 で割った商 q と余り r」に分けて  q * Lv + (r * Lv) / 100  と計算する（数学として a * Lv / 100 の切り捨てと同じ・q * Lv は a ≦ 約 21 万・Lv ≦ 65535 でも u32 に入る）。
 *   ★ 6.83: 同じ数の / と % を並べて書かない（CFRU の __aeabi_uidivmod は余りしか返さない）。余りは a - q * 100 で出す。
 *   従来の範囲（種族値 ≦ 255・個体値 ≦ 63・努力値 ≦ 255・Lv ≦ 255）では 昔の u8／u32 の式と同じ値（tools/rom/verify_calc_wide.py が全部を比べる）。
 */

//(a * level) / 100 の切り捨て（あふれない）
static inline u32 HackStat_Scaled(u32 a, u32 level)
{
	u32 q = a / 100;
	u32 r = a - q * 100;

	return q * level + (r * level) / 100;
}

//最大 HP（65535 で頭打ち）。baseTwice = 2 * 種族値（相棒の +20 など足したあと）
static inline u16 HackStat_MaxHp(u32 baseTwice, u32 iv, u32 ev, u32 level)
{
	u32 hp = HackStat_Scaled(baseTwice + iv + ev / 4, level) + level + 10;

	return hp > 0xFFFF ? 0xFFFF : (u16) hp;
}

//性格の補正の前の能力（+5 まで・65535 で頭打ち）
static inline u16 HackStat_Raw(u32 base, u32 iv, u32 ev, u32 level)
{
	u32 n = HackStat_Scaled(2 * base + iv + ev / 4, level) + 5;

	return n > 0xFFFF ? 0xFFFF : (u16) n;
}

//性格の補正（percent は 110／100／90）。65535 で頭打ち（素の ModifyStatByNature は u16 であふれて折り返す）
static inline u16 HackStat_ApplyPercent(u16 n, u32 percent)
{
	u32 v;

	if (percent == 100)
		return n;
	v = ((u32) n * percent) / 100;
	return v > 0xFFFF ? 0xFFFF : (u16) v;
}

//ほかの能力の完成形。ModifyStatByNature(nature, 100, statIndex) は素の表を引いて 110／100／90 を返す（表は素の ROM のまま使う）
#define HACK_STAT_NATURE_PERCENT(nature, statIndex) ((u32) ModifyStatByNature((nature), 100, (statIndex)))
