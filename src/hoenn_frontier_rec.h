#ifndef HOENN_FRONTIER_REC_H
#define HOENN_FRONTIER_REC_H
#include "hoenn_frontier_level.h"

/* フロンティアの記録の入れ物（追加のセーブ領域・src/hoenn_frontier_rec.c）。
 * レベル49・レベル100（HOENN_VAR_TOWER_LVL_MODE が 2・3）の連勝と最高の連勝、v3 からファクトリーの交換の回数を持つ。
 * レベル50・オープン（0・1）の記録は今までどおり予約変数 0x5100 台と CFRU の gBattleTowerStreaks（ここには置かない・動かさない）。 */
#define HFR_FAC_TOWER 0       /* HOENN_FACILITY_* と同じ番号 */
#define HFR_FAC_PALACE 1
#define HFR_FAC_ARENA 2
#define HFR_FAC_PIKE 3
#define HFR_FAC_FACTORY 4
#define HFR_FAC_DOME 5
#define HFR_FAC_PYRAMID 6
#define HFR_FAC_COUNT 7

#define HOENN_LVL_SEPARATE(m) ((m) == HOENN_LVL_MODE_49 || (m) == HOENN_LVL_MODE_100)   /* 別の記録を持つ設定（2・3） */

/* fac = HFR_FAC_*・dbl = 0 シングル / 1 ダブル・mode = HOENN_LVL_MODE_49 か _100・max = 最高の連勝か。
 * 読み書きは RAM の写しに対して。フラッシュへはレポートのあと（HoennFrontierRec_AfterSave）。 */
u16 HoennFrontierRec_Get(u8 fac, u8 dbl, u8 mode, bool8 max);
void HoennFrontierRec_Set(u8 fac, u8 dbl, u8 mode, bool8 max, u16 value);
u16 HoennFrontierRec_GetRents(u8 dbl, u8 mode);                 /* バトルファクトリーの交換の回数（49・100 だけ・v3） */
void HoennFrontierRec_SetRents(u8 dbl, u8 mode, u16 value);
u16* HoennFrontierRec_PyramidBag(u8 mode);                       /* ピラミッドのバトルバッグ（49・100 だけ・v3）: 10 枠 × {道具番号 u16, 個数 u16} = u16 × 20 の先頭。それ以外は NULL。書いてよい（dirty にする） */
u16* HoennFrontierRec_VarPtr(u16 id);          /* 移した拡張変数（0x5100〜0x513F・0x5151〜0x51ED）の置き場。範囲の外・入れ物を使わない版は NULL */
u16* HoennFrontierRec_ExpVar(u16 id);          /* 構造体の重ね書き用: 入れ物の中か、今までの gExpandedVars の中 */
void HoennFrontierRec_OnLoad(void);             /* セーブを読んだ（RAM の写しを捨てる） */
void HoennFrontierRec_AfterSave(bool8 ok);      /* レポートを書き終えた（変わっていれば追加の区画へ） */
bool8 HoennFrontierRec_TowerStreak(u8 currentOrMax, u16* out);   /* タワーで 49・100 のとき、今の連勝／最高を out へ（CFRU の読み出しの口） */
#endif
