#pragma once

/*
    地方（カントー・ホウエン・ジョウト・シンオウ）と地名の番号（MAPSEC）の読み替え（2026-09-29）
    ★ 詳細は docs/research/2026-09-29_mapsec-regions.md

    地名の番号は 1 バイトで、空きは残り 4 個（CLAUDE.md 6.44）。シンオウの地名（78 個）は
    **シンオウだけの番号 0〜77** で持つ。同じ数がホウエンの番号（0x00〜0x57）とぶつかるので、
    番号を名前に直すとき・ホウエンの番号として読むときは、どの地方の番号かを見る。
      ・今いるマップの地名（マップに入ったときの表示・レポート・メニューの帯・ずかんナビ）… 今いるマップの地方
      ・タウンマップ … 見ているページ（シンオウは 6 枚目）
      ・ポケモンの「出会った場所」… 親の版（MET_GAME）。シンオウで出会ったらプラチナの番号 12（ユーザー判断）
    ★ カントー（0x58〜0xC4）とジョウト（0x4E〜0x56・0xD1〜0xF8）の番号はシンオウの 0〜77 とぶつからない
*/

#define MAP_REGION_KANTO   0   /* カントー・ななしま（素の群 0〜42） */
#define MAP_REGION_HOENN   1
#define MAP_REGION_JOHTO   2
#define MAP_REGION_SINNOH  3
#define MAP_REGION_KITAKAMI 4  /* ★ 2026-10-02 ジョウトの群の地図 KITAKAMI_MAP_NUM_FIRST〜LAST（Tables/kitakami_townmap.h）。地名はキタカミだけの番号 0〜3 */

/* マップの群 → 地方。★ ジョウト・シンオウの地図を足したら群の範囲をここに書く
   （tools/townmap/verify_map_region.py がホウエンの範囲を build_hoenn.py と突き合わせる） */
#define HOENN_MAP_GROUP_FIRST   43      /* build_hoenn.py の GROUP_BASE（エメラルドの群 g → 43 + g） */
#define HOENN_MAP_GROUP_COUNT   34
#define JOHTO_MAP_GROUP_FIRST   78      /* tools/sinnoh/sinnoh_maps.py の JOHTO_GROUP（シンオウのすぐ後ろ・2026-09-29 アサギシティ） */
#define JOHTO_MAP_GROUP_COUNT   1
#define SINNOH_MAP_GROUP_FIRST  77      /* tools/sinnoh/sinnoh_maps.py の SINNOH_GROUP（ホウエンのすぐ後ろ・2026-09-29 キッサキシティ） */
#define SINNOH_MAP_GROUP_COUNT  1
#define SINNOH_MAP_GROUP_SECOND 80      /* ★ 2026-10-11 シンオウの建物の中（tools/sinnoh/sinnoh_interiors.py の群 SINNOH_GROUP2・78 ジョウト・79 カントーの追加の次） */

/* 「出会った場所」の地方の印（親の版 MET_GAME・4 ビット）。第 4 世代のプラチナの番号 */
#define MET_GAME_SINNOH  12
#define MET_GAME_KITAKAMI 13   /* ★ 2026-10-02 キタカミで出会った（13 は第 4 世代でも使われていない番号） */

u8 GetMapGroupRegion(u8 group);
/* ジョウトへの通常移動を一時停止。キタカミは別地方として維持する。 */
bool8 MapRegion_IsTravelMapAllowed(u8 group, u8 num);
u8 GetMapRegionOf(u8 group, u8 num);   /* 群と番号 → 地方（キタカミはジョウトの群の中の番号で決まる） */
u8 GetCurrentMapRegion(void);
/* バトルフロンティアの地図（ホウエンの特別地区＝群 69 の 4〜55 のうち 9〜13 を除く）。群は「ホウエン」でも地方としては別扱いにしたい所（バッジのレベル制限・#124）と
   連れ歩きを出さない所（src/bw_follower.c）が使う＝判定は MapRegion_IsFrontierMap の 1 か所。
   ★ 群 69 の 9・10 は南の孤島、11〜13 はサファリゾーンの休憩所・北東・南東（エメラルドの群 26 の並びのまま）で、フロンティアの番号の間に挟まっている。
     4〜55 をまるごとフロンティアにすると、サファリの北東・南東に入った連れ歩きが消えた（0.53.1・ユーザー報告） */
#define HOENN_FRONTIER_MAP_GROUP  69
#define HOENN_FRONTIER_MAP_FIRST  4
#define HOENN_FRONTIER_MAP_LAST   55
#define HOENN_FRONTIER_MAP_HOLE_FIRST  9    /* 南の孤島 2 枚（9・10）とサファリゾーン 3 枚（11〜13）はフロンティアではない */
#define HOENN_FRONTIER_MAP_HOLE_LAST   13
bool8 MapRegion_IsFrontierMap(u8 group, u8 num);
bool8 MapRegion_CurrentIsFrontier(void);
const u8 *MapRegion_GetName(u16 mapsec, u8 region);   /* 253 件（0x00〜0xFC）の外は MAPSEC_NONE の名前 */
#define MAPSEC_NAME_COUNT 0xFD
u8 MapRegion_MetGameHere(void);
u8 MapRegion_FromMetGame(u8 metGame);
bool8 MapRegion_IsOurMetGame(u8 metGame);

/* ★ 2026-10-15 親の版を 4 ビットのまま読む（patches/0379）。CFRU の bytereplacement「Old PokeBall Expansion Data Reset」
   （0803F7A6）で GetMonData(MON_DATA_MET_GAME) は下 3 ビットしか返さない＝12（シンオウ）は 4（FR）・13（キタカミ）は 5（LG）
   になる。親の版で地方を決める所（ステータス画面の出会った場所）は GetMonData ではなくこれで読む */
struct Pokemon;
u8 MapRegion_MonMetGame(const struct Pokemon *mon);
