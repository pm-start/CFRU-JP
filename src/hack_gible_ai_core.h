#ifndef HACK_GIBLE_AI_CORE_H
#define HACK_GIBLE_AI_CORE_H
/*
 * ★ 2026-10-23 FEAT-gible-reito-double-action（Redmine #58 段階 2・patches/0792）
 * れーとのおうの子の 2 回目の行動を選ぶ点数の計算（ゲームの型を使わない純関数・乱数なし・ホストで単体テストできる）。
 * 呼び出し側（src/hack_gible_ai.c）が、場の値を struct に詰めてここへ渡す。同点は技の並び順（pos の小さい方）。
 */
#include <stdint.h>

#define GIBLE_KIND_DAMAGE  0
#define GIBLE_KIND_HEAL    1
#define GIBLE_KIND_PROTECT 2
#define GIBLE_KIND_SETUP   3
#define GIBLE_KIND_STATUS  4
#define GIBLE_KIND_OTHER   5

#define GIBLE_LAYER 100000   /* 層の幅。層の中の点はこれより小さい */

struct GibleCand {
	uint8_t usable;      /* 0 = 使えない・向かない（CheckMoveLimitations・溜め技・交代技など） */
	uint8_t kind;
	uint8_t immune;      /* 相手に効かない（タイプ・特性） */
	uint8_t accuracy;    /* 命中 0〜100（必中は 100） */
	int8_t  priority;
	uint16_t power;
	uint16_t minDmg;     /* 相手に与える最低ダメージ（見積もり） */
	uint16_t allyDmg;    /* 味方を巻き込むとき、味方への最低ダメージ（無効なら 0） */
	uint16_t healAmount; /* 回復技が実際に回復する量 */
	uint8_t statusOk;    /* 状態異常の技が効く */
	uint8_t pos;         /* 技の並び順 0〜3 */
	uint8_t target;      /* 対象のバンク（同じ技の別の相手の区別） */
};

struct GibleCtx {
	uint16_t myHp, myMaxHp;
	uint16_t foeHp, foeMaxHp;     /* この候補の相手の今の HP */
	uint16_t foeThreat;           /* 相手が次に自分へ与える見積もり（相手の使った技・場の情報から） */
	uint8_t  iAmFaster;
	uint8_t  isDouble;
	uint8_t  setupStage;          /* 自分の攻撃の段階（-6〜+6 を +6 して 0〜12） */
	uint8_t  firstWasProtect;
	uint8_t  hadFirst;            /* 1 回目の記録がある（2 回目の判断） */
	uint8_t  firstPos, firstTarget;
	uint8_t  firstRatioPct;       /* 1 回目の実際の減り ÷ 見積もり（%・見積もりが 0 のときは 100） */
	uint8_t  takenPct;            /* このターンに自分が受けたダメージ ÷ 最大 HP（%） */
	uint8_t  firstKilledFoe;
};

#define GIBLE_RATIO_LOW   60   /* 1 回目の実際の減りがこれ未満なら 同じ技・同じ相手の点を下げる */
#define GIBLE_TAKEN_HIGH  40   /* このターンの被ダメがこれ以上なら守りの点を上げる */

static inline int32_t GibleAI_Score(const struct GibleCand *c, const struct GibleCtx *x)
{
	int32_t s = 0;
	uint8_t lowHp = (uint32_t) x->myHp * 3 <= x->myMaxHp;
	uint8_t danger = x->foeThreat >= x->myHp;   /* 次の攻撃で倒されそう */
	uint8_t guard = (lowHp && danger) || x->takenPct >= GIBLE_TAKEN_HIGH;

	if (!c->usable)
		return -1;
	if (c->immune)
		return 0;

	switch (c->kind) {
	case GIBLE_KIND_DAMAGE:
		if (c->minDmg >= x->foeHp && c->accuracy > 0) {
			/* a. 倒せる: 命中が高い → 威力が低い（PP の節約）。先制技で自分が遅いときはさらに上 */
			s = 4 * GIBLE_LAYER + c->accuracy * 100 + (999 - (c->power > 999 ? 999 : c->power)) / 10;
			if (c->priority > 0 && !x->iAmFaster)
				s += GIBLE_LAYER / 2;
			if (c->allyDmg > 0)
				s -= GIBLE_LAYER;   /* 味方を巻き込む倒せる技は、巻き込まない倒せる技より下 */
		} else {
			uint32_t ratio = x->foeHp ? (uint32_t) c->minDmg * 100 / x->foeHp : 0;   /* e. ダメージ÷残り HP */
			if (ratio > 100) ratio = 100;
			s = 1 * GIBLE_LAYER / 2 + (int32_t) (ratio * c->accuracy / 100) * 100;
			if (c->allyDmg > 0) {
				int32_t pen = (int32_t) c->allyDmg * 100;
				s -= pen > 40000 ? 40000 : pen;     /* 味方へのダメージを差し引く */
			}
		}
		break;
	case GIBLE_KIND_HEAL:
		if (c->healAmount == 0)
			return 0;
		s = guard ? 3 * GIBLE_LAYER + (int32_t) c->healAmount : 1000;
		if (!guard && x->firstKilledFoe && !x->isDouble)
			s = 1 * GIBLE_LAYER / 2 + 20000;   /* 1 回目で倒した → 回復の点を上げる */
		break;
	case GIBLE_KIND_PROTECT:
		if (x->firstWasProtect)
			return 0;
		s = guard ? 3 * GIBLE_LAYER - 1 : 500;
		break;
	case GIBLE_KIND_SETUP:
		if (x->setupStage >= 8)   /* +2 以上 */
			return 100;
		if (!x->isDouble && x->foeThreat * 2 < x->myHp)
			s = 2 * GIBLE_LAYER - GIBLE_LAYER / 2 + 10000;   /* b/c 積み技 */
		else
			s = 3000;
		if (x->firstKilledFoe && !x->isDouble)
			s += 15000;
		break;
	case GIBLE_KIND_STATUS:
		s = c->statusOk ? (x->iAmFaster ? 20000 : 40000) : 0;
		break;
	default:
		return 1;
	}

	/* 1 回目の結果で変える: 見積もりより効かなかった 同じ技・同じ相手 は下げる */
	if (x->hadFirst && c->kind == GIBLE_KIND_DAMAGE && c->pos == x->firstPos && c->target == x->firstTarget
	 && x->firstRatioPct < GIBLE_RATIO_LOW && s < 4 * GIBLE_LAYER)
		s = s / 4;
	return s;
}

/* 一番高い点の候補の番号（同点は並び順 pos の小さい方・点が 0 以下だけなら -1） */
static inline int GibleAI_Pick(const struct GibleCand *cands, int n, const struct GibleCtx *ctxs)
{
	int i, best = -1;
	int32_t bestScore = 0;

	for (i = 0; i < n; ++i) {
		int32_t sc = GibleAI_Score(&cands[i], &ctxs[i]);
		if (sc > bestScore || (sc == bestScore && best >= 0 && sc > 0 && cands[i].pos < cands[best].pos)) {
			best = i;
			bestScore = sc;
		}
	}
	return best;
}
#endif
