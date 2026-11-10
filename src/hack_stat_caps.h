#pragma once

//★ 生成物（手で直さない）: tools/build/gen_stat_caps.py が tools/stat_caps/stat_caps.json から作る。
//  努力値・個体値・種族値・レベルの上限の正本は stat_caps.json（[hack] patches/0677・設計 docs/research/2026-10-23_stat-level-16bit.md 6 章）。
//  純粋な #define と #if だけ（config.h を通して .string の前処理にも読まれるので、型や関数は書かない）。

#define HACK_EV_PER_STAT_CAP     252    //努力値 1 つの上限
#define HACK_EV_TOTAL_CAP        510    //努力値の合計の上限
#define HACK_IV_MAX              31     //個体値の上限（保存する値）
#define HACK_IV_BONUS_EXTRA      32     //Lv100 をこえたとき個体値に足せる最大（差を保つ方式・#128）
#define HACK_IV_BONUS_CEILING    63     //実効の個体値の上限（IV_MAX + BONUS_EXTRA）
#define HACK_BASE_STAT_MAX       255    //種族値（u8 の欄）の上限・スケールモンの頭打ち
#define HACK_MAX_LEVEL           200    //レベルの上限（MAX_LEVEL）
#define HACK_EV_DIGITS           3      //努力値の表示の桁数
#define HACK_IV_DIGITS           2      //個体値の表示の桁数
#define HACK_LEVEL_DIGITS        3      //レベルの表示の桁数
#define HACK_EV_FIELD_MAX        255    //努力値の u8 の欄（手持ち・BattleTowerSpread など）の入れ物の上限＝上限そのものではない
#define HACK_STAT_VALUE_MAX      65535  //能力値・種族値・努力値・個体値の u16 の入れ物の上限

//入れ物の幅の守り: 上限を上げて入れ物を超えるとここでビルドが止まる
#if !(HACK_EV_PER_STAT_CAP <= 255)
#error "stat caps: HACK_EV_PER_STAT_CAP <= 255"
#endif
//  ↑ 努力値 1 つが手持ち・80 バイト形の u8 の欄に入らない（段階 T1 の実装が要る）
#if !(HACK_IV_MAX <= 31)
#error "stat caps: HACK_IV_MAX <= 31"
#endif
//  ↑ 個体値が手持ち・80 バイト形の 5 ビットの欄に入らない（段階 T1 の実装が要る）
#if !(HACK_EV_TOTAL_CAP <= 6 * HACK_EV_PER_STAT_CAP)
#error "stat caps: HACK_EV_TOTAL_CAP <= 6 * HACK_EV_PER_STAT_CAP"
#endif
//  ↑ 努力値の合計が 6 つの上限の和をこえている（意味が無い設定）
#if !(HACK_BASE_STAT_MAX <= 255)
#error "stat caps: HACK_BASE_STAT_MAX <= 255"
#endif
//  ↑ 種族値の u8 の欄（gBaseStats・DPE のデータ）に入らない。gBaseStatsWide の上書きを使う
#if !(HACK_MAX_LEVEL <= 254)
#error "stat caps: HACK_MAX_LEVEL <= 254"
#endif
//  ↑ 手持ちの保存のレベル（経験値の表は 254 番まで・+0x54 の u8）に入らない。案を再検討する
#if !(HACK_MAX_LEVEL <= 65535)
#error "stat caps: HACK_MAX_LEVEL <= 65535"
#endif
//  ↑ 戦闘・計算のレベル（u16）に入らない
#if !(HACK_IV_BONUS_CEILING <= 65535)
#error "stat caps: HACK_IV_BONUS_CEILING <= 65535"
#endif
//  ↑ 計算の u16 の入れ物に入らない
