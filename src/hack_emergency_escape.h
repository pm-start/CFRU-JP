#pragma once
#include "../include/global.h"

/*
    緊急脱出（patches/0750〜0753・docs/research/2026-10-24_emergency-escape.md・TODO #144）。
    壁に埋まった・台本が止まった・出られない地図に入ったなど、詰んだときに「最後に回復した所に近いポケモンセンターの中（看護師の前）」へ送る。
    旗・変数・セーブの置き場は増やさない（確かめの状態はタイトルのタスクの欄・せっていの画面のメモリ（ヒープ）だけ＝保存しない）。

    入口 2 つ:
      ・タイトルのメインメニューで「つづきから」を L と R を押したまま選ぶ → 確かめ → はい なら、セーブを読んだあと（地図に入る前）
        HackEscape_OnContinue が 後始末 ＋ 続きからのワープを最寄りのポケセンにする（src/bw_main_menu.c）
      ・せっていの「どくじきのう」フォルダの先頭の仮の行（0xF5）→ 確かめ → はい で せっていを閉じ、フィールドに戻ってから
        HackEscape_WarpNow が 後始末 ＋ SetWarpDestination ＋ DoWarp（src/bw_option_menu.c・tools/settings_plus/prepare_ui_patch.py）
*/

/* 行き先を決める（pure に近い・ホストの検算 tools/rom/verify_emergency_escape.py が本物の C を clang で動かす）。
   regionNow は map_region.h の MAP_REGION_*。heal は gSaveBlock1->lastHealLocation。homeOnly は「ポケセンの縛りで戻り先がマサラの家」。
   out に ポケセンの中（群・番号・warpId -1・x・y）を入れる。戻り値は 0 = 最後の回復地点から決めた / 1 = 今いる地方の最初の町のポケセン /
   2 = マサラタウンの家（カントーの最初の町・ポケセンの縛りのとき） */
u8 HackEscape_Decide(struct WarpData *out, u8 regionNow, const struct WarpData *heal, bool8 homeOnly);

bool8 HackEscape_FieldAvailable(void);   /* せっていの行を出してよいか（フィールドからのせっていで・戦闘中でも通信中でもない） */
void HackEscape_Cleanup(bool8 live);     /* 全滅のあとと同じ後始末 ＋ 施設の挑戦をやめた扱い ＋ ワープで残ると困る状態を戻す（live = 地図が読み込まれている） */
void HackEscape_OnContinue(void);        /* タイトル: 後始末 ＋ 続きからのワープを最寄りのポケセンに */
void HackEscape_WarpNow(void);           /* フィールド: 後始末 ＋ ワープ */
