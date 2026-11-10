#pragma once
/*
 * ロジックアリーナ（役割論理＝ヤーティの専門の施設・Redmine #116・patches/0817〜0819）の番号。
 * 旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存・旧セーブは 0）。台帳: hack_oak_phone.h の 0xFD0〜0xFD5・BP の店の枝の 0xFD6／0xFD7 の次 → 0xFD8（ここ）。
 * 変数 0x50C6／0x50C7 は tools/rom/build_hoenn_scripts.py の RESERVED_VARS（HOENN_VAR_YARTIE_OPP／HOENN_VAR_YARTIE_PROGRESS・Tables/hoenn_script_flags.h が生成）。
 * 次の空きの旗は 0xFDA。
 */
#define HACK_FLAG_YARTIE_CLEARED 0xFD8 /* ロンジャ 7 人に勝った */
#define HACK_FLAG_YARTIE_ACTIVE  0xFD9 /* 挑戦中（受付でレポートして つづきから 再開するための印） */

#define HOENN_FACILITY_YARTIE 7        /* HOENN_VAR_FRONTIER_FACILITY の値（タワー〜ピラミッドの次） */
#define YARTIE_BATTLES 7
#define YARTIE_CLEAR_BP 10   /* 全員に勝ったときの上乗せ（ブレーン戦の +10 と同じ）。1 戦ごとは 3 BP（src/hoenn_tower.c の表の 8 行目） */
#define YARTIE_SELECT_MARK 0x8000      /* HOENN_VAR_YARTIE_OPP の最上位ビット＝見せ合いの画面を通信なしで出す印 */
