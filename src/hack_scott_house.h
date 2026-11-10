#ifndef GUARD_HACK_SCOTT_HOUSE_H
#define GUARD_HACK_SCOTT_HOUSE_H

/*
 * エニシダの家（群 69 の 46・エメラルドの BattleFrontier_ScottsHouse）のシンボルの報酬（2026-10-25・Redmine #115・patches/0816）。
 * 拡張フラグ 0xFD6／0xFD7 は台帳の次の空き（0xFD0〜0xFD2・0xFD5＝hack_oak_phone.h・0xFD3／0xFD4＝カントー HG の枝のデバッグ用）。定義はここだけ。
 * ユーザー決定: BP の贈り物は入れない（0xFD8 は使わない）。
 */
#define HACK_FLAG_SCOTT_SILVER_BERRY 0xFD6   /* 銀のシンボルが 7 つそろって サンのみ をもらった（エメラルドの FLAG_COLLECTED_ALL_SILVER_SYMBOLS） */
#define HACK_FLAG_SCOTT_GOLD_BERRY   0xFD7   /* 金のシンボルが 7 つそろって スターのみ をもらった（FLAG_COLLECTED_ALL_GOLD_SYMBOLS） */

#endif
