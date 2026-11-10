#pragma once
#include "../include/global.h"

/*
    セーブの改造の検出（チート対策 段階 3 の続き）。docs/research/2026-10-21_save-obfuscation.md の 11 章。

    区画ごとの MAC（src/hack_save_obf.h）が合わない区画を つづきから で読んだら、**遊びは止めず・消さず**、セーブの中に消えない印を付ける。
    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: 金の前口上の印 0xFC8〜0xFCE（build_hoenn_scripts.LEDGER_FLAGS）の次 → **0xFCF（ここ）** → 次の空きは 0xFD0。
    変数は 0x50BC〜0x50BE の 3 本（build_hoenn_scripts.py の RESERVED_VARS に登録・hoenn_script_flags.h に生成。ジムの再戦の段階は 0x50B1〜0x50BB で重ならない＝verify_gym_tiers.py。hack_surf_sprite.c の 0x50C4 の手前）。
    理由の bit は 2 つの検出が共有する: **bit0 = セーブの MAC が合わない（本ファイルの src/save.c の読み）・bit1 = RAM の改造（feature/ram-cheat-detect）**・bit2〜15 は空き（足すときはここに書く）。
    理由は OR で貯める（後から別の理由で見つかっても足される）。版とプレイ時間は最初の 1 回だけ。
    セーブの大きさ・形は変わらない（旗 1 つと予約変数 2 つ＝もとからあるセーブの欄の中）。はじめから（NewGameWipeNewSaveData）で消える。
*/
#define HACK_FLAG_TAMPERED        0xFCF   /* 改造ありの印（立てたら下ろさない＝ゲームは下ろす道を持たない・再署名しても残る） */
#define HACK_VAR_TAMPER_VERSION   0x50BC  /* 初めて見つけた版（hack_save_version.h と同じ数え方・0 = まだ） */
#define HACK_VAR_TAMPER_HOURS     0x50BD  /* 初めて見つけたときのプレイ時間（時間・gSaveBlock2->playTimeHours。時計が無いゲームなので日付の代わり） */
#define HACK_VAR_TAMPER_REASON    0x50BE  /* 見つけた理由の bit の OR（下の HACK_TAMPER_REASON_*）。0 = まだ */
#define HACK_TAMPER_REASON_SAVE_MAC   0x0001  /* 区画の MAC が合わない（外の道具で書き換えたセーブ） */
#define HACK_TAMPER_REASON_RAM_CHEAT  0x0002  /* RAM の改造（チートコード・feature/ram-cheat-detect が使う） */

void HackTamper_Mark(u16 reasons);   /* 印を付ける。旗が無ければ 立てて 版とプレイ時間を残す（最初の 1 回だけ）。理由の bit は毎回 OR で足す */
bool8 HackTamper_IsMarked(void);
u16 HackTamper_Reasons(void);        /* 見つかった理由の bit（HACK_TAMPER_REASON_*） */
