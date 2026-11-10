// tools/kanto/badge_level_cap.py が生成（手で編集しないこと）。バッジによるレベル制限の表（#124）
#pragma once

#define BADGE_CAP_REGION_COUNT 3
#define BADGE_CAP_MARGIN 10      //次のジムリーダー（8 個そろったあとは四天王・チャンピオン）の最高レベルに足す分
//行の地方（map_region.h の MAP_REGION_*）: KANTO=0 / HOENN=1 / SINNOH=3
#define BADGE_CAP_REGION_IDS {0, 1, 3}

//行 = 地方・列 = その地方のバッジの数 0〜8（8 個は四天王・チャンピオンの最高 + 10）。実物の手持ちの表の初回のレベルから
#define BADGE_CAP_TABLE \
	{24, 31, 34, 39, 53, 53, 57, 60, 73}, /*KANTO*/ \
	{80, 80, 86, 86, 95, 95, 105, 107, 120}, /*HOENN*/ \
	{125, 125, 132, 132, 142, 142, 153, 155, 170} /*SINNOH*/ \

