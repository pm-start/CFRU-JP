#pragma once

/*
    育て屋を地方ごとに独立させる（2026-10-21・BUG-daycare-shared-A・案 A の入れ替え）
    ★ 設計: docs/research/2026-10-21_daycare-region-swap.md
    本体 `gSaveBlock1->daycare` と旗 FLAG_PENDING_DAYCARE_EGG は「いまの地方の育て屋」。別の地方へ入るとき、
    SaveBlock1 + 0x361C（ramScript・1004B）の控えと中身ごと入れ替える。
*/

void HackDaycare_SyncRegion(void);
/* 控えの i 番（0〜2）。空き・ヘッダー不正なら NULL（育て屋のポケモンを数える所用）*/
struct DayCare *HackDaycare_GetStashed(u8 i);
