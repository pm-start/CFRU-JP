#ifndef HOENN_FRONTIER_LEVEL_H
#define HOENN_FRONTIER_LEVEL_H
/*
 * フロンティア施設のレベル設定（2026-10-20 ユーザー「全施設、レベルは上げない仕様、今の下げる仕組みを使用して、ブレーンはレベルに合わせて」）
 * HOENN_VAR_TOWER_LVL_MODE(0x5143) の値:
 *   0 = レベル50（50 をこえる子を 50 に下げる）  ← 今までと同じ
 *   1 = オープンレベル（手持ちに追従・60〜）      ← 今までと同じ
 *   2 = レベル49（49 をこえる子を 49 に下げる・相手も 49）
 *   3 = レベル100（100 をこえる子を 100 に下げる・上げはしない・相手は 100）
 * 連勝の記録・対戦部屋・敵の表などの「2 つに分ける」所は、2 を 0 の仲間・3 を 1 の仲間として扱う（セーブの変数は増やさない）:
 *   HOENN_LVL_OPENCLASS = 1 なら「オープン側」（1・3）。0 なら「50 側」（0・2）。
 * 決まったレベルの設定（0・2・3）は HOENN_LVL_FIXED(mode) が上限＝相手のレベル。オープン（1）は使わない。
 */
#define HOENN_LVL_MODE_50   0
#define HOENN_LVL_MODE_OPEN 1
#define HOENN_LVL_MODE_49   2
#define HOENN_LVL_MODE_100  3

#define HOENN_LVL_OPENCLASS(m) ((m) == HOENN_LVL_MODE_OPEN || (m) == HOENN_LVL_MODE_100)
#define HOENN_LVL_FIXED(m) ((m) == HOENN_LVL_MODE_49 ? 49 : ((m) == HOENN_LVL_MODE_100 ? 100 : 50))
#endif
