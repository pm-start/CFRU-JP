// tools/sinnoh/sinnoh_underground.py が生成。手で編集しないこと。
// ちかの ひろば（ちかつうろ・FEAT-sinnoh-underground）の地図番号とかべの座標。地図の番号は tools/sinnoh/sinnoh_interiors.py の並び（sinnoh_maps.map_id）から引く。
#pragma once
#define HACK_UG_GROUP 80
#define HACK_UG_NUM 111
#define HACK_UG_ARRIVE_X 3
#define HACK_UG_ARRIVE_Y 9
#define HACK_UG_WALL_COUNT 8
// かべの座標 {x, y}（地図の座標・並びが かべの番号＝旗 HACK_FLAG_MINING_WALL_BASE + 番号）
#define HACK_UG_WALLS_INIT {{3, 1}, {5, 1}, {8, 1}, {11, 1}, {13, 1}, {16, 1}, {18, 1}, {21, 1}}
