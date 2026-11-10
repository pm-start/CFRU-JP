#pragma once

/*
 * 技の熟練度（しんか／じゅくれん・10 段階）の計算（[hack] patches/0932・FEAT-move-mastery・2026-10-28）
 * 仕様: work/tmp/next_patch/move_mastery_spec.md（11 章のユーザーの決定が最優先）。
 *   熟練度は使用回数（hack_move_count.c の表・技進化と同じ数え方）から計算する＝段は保存しない。
 *   「じゅくれん」にした技は進化しない（一方通行・J2）。戦闘中だけ、技ごとの表（src/Tables/move_mastery_table.h・
 *   tools/moves/build_move_mastery.py が作る）の足し算が効く。技の表（battle_moves.c）は書き換えない。
 *   ここは純粋な計算だけ（ゲームの RAM を読まない）＝検算 tools/rom/verify_move_mastery.py がホストでそのまま動かす。
 */

#define MASTERY_LEVELS 10

enum
{
	MASTERY_KIND_NONE,      /* 必ず当たる技: 威力の足し算だけ */
	MASTERY_KIND_ACC,       /* 命中が 100 未満の技: 命中が上がる */
	MASTERY_KIND_EFFECT,    /* 命中 100 で追加効果の確率が 1% 以上の技: 追加効果の確率が上がる */
	MASTERY_KIND_CRIT,      /* どちらでもない技: 急所に当たりやすくなる */
};

struct MoveMasteryEntry     /* 技進化の表（sMoveEvoTable）と同じ行順 */
{
	u8 kind;
	u8 power10;             /* 10 段目の威力の足し算（進化先との差 × 0.4 を 3〜15 に） */
	u8 extra10;             /* 10 段目の個性: 命中型＝命中の足し算（min(15, 100 − 命中)）／追加効果型＝確率の足し算（15） */
	u8 critMax;             /* 急所型: 急所ランクの上限（進化先がもともと急所に当たりやすい技なら 1・ほかは 2） */
};

/* 段に必要な使用回数（累計・J3） */
static const u8 sMasteryThreshold[MASTERY_LEVELS] = {8, 20, 35, 55, 80, 110, 145, 185, 225, 250};

static inline u8 Mastery_LevelOfCount(u8 count)
{
	u8 lv = 0;

	while (lv < MASTERY_LEVELS && count >= sMasteryThreshold[lv])
		++lv;
	return lv;
}

/* 10 段目の値 × 段 ÷ 10（四捨五入）。途中の段の値 */
static inline u8 Mastery_Scale(u8 value10, u8 lv)
{
	return (u8) ((value10 * lv + 5) / 10);
}

/* 急所ランクの足し算: 5 段目で +1・10 段目でさらに +1（J5・進化先が急所に当たりやすい技なら +1 まで） */
static inline u8 Mastery_CritBonus(const struct MoveMasteryEntry* e, u8 lv)
{
	u8 r;

	if (e->kind != MASTERY_KIND_CRIT || lv < 5)
		return 0;
	r = lv >= 10 ? 2 : 1;
	return r > e->critMax ? e->critMax : r;
}
