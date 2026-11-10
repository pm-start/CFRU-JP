#pragma once

#include "hack_stat_caps.h" //[hack] patches/0677 上限（IV_MAX・MAX_LEVEL）は stat_caps.json の生成物から

/*
 * Lv100 をこえた子の「実効の個体値」（#128・patches/0591・2026-10-17・ユーザー「個体値の上限を 63 に・31 をこえる分はレベルが 100 をこえてから少しずつ」）
 *
 *   セーブの形は変えない: 保存する個体値は今までどおり 5 ビット（0〜31）。計算（CalculateMonStatsNew）と画面の数だけが、
 *   Lv101〜200 のあいだ「生の個体値（王冠で鍛えた能力は 31）に ボーナスを足した値」を使う。
 *
 *   Lv ≤ 100 : 実効 = 生の個体値（今までと 1 バイトも変わらない）
 *   Lv 101〜200（200 より上は 200 と同じ）:
 *     HACK_IV_OVER100_MODE 0（既定・差を保つ）: 実効 = 生 + floor(32 × (Lv − 100) / 100)   → 31 は Lv200 で 63・0 は 32
 *     HACK_IV_OVER100_MODE 1（63 にそろえる）  : 実効 = 生 + floor((63 − 生) × (Lv − 100) / 100) → どちらも Lv200 で 63
 *
 *   式はこの 1 か所だけ。切り替えは下の MODE の数字を変えるだけ（セーブには影響しない）。
 *   使わない: めざめるパワーのタイプ・威力（生の個体値の下位ビット）／育て屋の遺伝／ミラクル交換の 3V・6V／実績（個体値 31）。
 *   性格値（PID）には触らない（6.5 色違い保護）。
 */

#ifndef HACK_IV_OVER100_MODE
#define HACK_IV_OVER100_MODE        0     //0 = 差を保つ（+32 まで）・1 = 63 にそろえる（検算がコマンドラインの -D で両方を試す）
#endif
#define HACK_IV_OVER100_START_LEVEL 100   //この Lv までは変わらない
#define HACK_IV_OVER100_END_LEVEL   HACK_MAX_LEVEL   //この Lv でボーナスが最大（レベル上限と同じ・[hack] patches/0677 stat_caps.json から）
#define HACK_IV_OVER100_MAX_BONUS   HACK_IV_BONUS_EXTRA    //差を保つ方式の最大のボーナス（31 + 32 = 63・[hack] patches/0677 IV_MAX + 1）
#define HACK_IV_OVER100_CEILING     HACK_IV_BONUS_CEILING    //実効の個体値の上限（63・[hack] patches/0677 IV_MAX から導く）

static inline u16 HackIv_Effective(u16 baseIv, u16 level) //[hack] patches/0678
{
	u32 lv, bonus;

	if (level <= HACK_IV_OVER100_START_LEVEL)
		return baseIv;
	lv = (level > HACK_IV_OVER100_END_LEVEL) ? HACK_IV_OVER100_END_LEVEL : level;
	lv -= HACK_IV_OVER100_START_LEVEL;
	#if HACK_IV_OVER100_MODE == 1
	bonus = ((u32) (HACK_IV_OVER100_CEILING - baseIv) * lv) / (HACK_IV_OVER100_END_LEVEL - HACK_IV_OVER100_START_LEVEL);
	#else
	bonus = ((u32) HACK_IV_OVER100_MAX_BONUS * lv) / (HACK_IV_OVER100_END_LEVEL - HACK_IV_OVER100_START_LEVEL);
	#endif
	bonus += baseIv;
	return (bonus > HACK_IV_OVER100_CEILING) ? HACK_IV_OVER100_CEILING : (u16) bonus;
}
