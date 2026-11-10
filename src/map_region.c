#include "defines.h"
#include "map_region.h"
#include "../include/constants/region_map_sections.h"
#include "Tables/sinnoh_townmap.h"
#include "Tables/kitakami_townmap.h"
#include "hack_johto.h"   //[hack] patches/0986

/*
map_region.c
    地方と地名の番号の読み替え（2026-09-29・ユーザー「先に地名問題片付けよう」）。
    ★ 考え方は src/map_region.h の頭・詳細は docs/research/2026-09-29_mapsec-regions.md
*/

extern const u8 *const gExpandedMapSecNames[];   /* assembly/hoenn/mapsec.s（0x00〜0xFC） */

static bool8 InGroupRange(u8 group, u8 first, u8 count)
{
    return count != 0 && group >= first && group < first + count;
}

u8 GetMapGroupRegion(u8 group)
{
    if (InGroupRange(group, HOENN_MAP_GROUP_FIRST, HOENN_MAP_GROUP_COUNT))
        return MAP_REGION_HOENN;
    if (InGroupRange(group, JOHTO_MAP_GROUP_FIRST, JOHTO_MAP_GROUP_COUNT))
        return MAP_REGION_JOHTO;
    if (InGroupRange(group, SINNOH_MAP_GROUP_FIRST, SINNOH_MAP_GROUP_COUNT) || group == SINNOH_MAP_GROUP_SECOND)
        return MAP_REGION_SINNOH;
#ifdef HACK_DEBUG_MENU
    /* ★ 2026-11-10 FEAT-unova-graft-2（デバッグ版だけ）: イッシュ（群 81・82・tools/unova/graft.py で接ぎ木した版）は今までどおりカントー扱い。
       どの範囲にも入らない群は下の既定でカントーになるが、イッシュの群であることを残すために明示する（通常版の機械語は変わらない） */
    if (group == 81 || group == 82)
        return MAP_REGION_KANTO;
#endif
    return MAP_REGION_KANTO;
}

/* ★ 2026-10-02 キタカミ（ユーザー「空きの地名の拡張はできないか？」→ キタカミだけの番号）: ジョウトの群の 148〜164 */
u8 GetMapRegionOf(u8 group, u8 num)
{
    u8 r = GetMapGroupRegion(group);

    if (r == MAP_REGION_JOHTO && num >= KITAKAMI_MAP_NUM_FIRST && num <= KITAKAMI_MAP_NUM_LAST)
        return MAP_REGION_KITAKAMI;
    return r;
}

/* 保存済みの訪問フラグに関係なく、ジョウト行きだけを止める。 */
bool8 MapRegion_IsTravelMapAllowed(u8 group, u8 num)
{
    /* ★ patches/0986（FEAT-johto-j0）: HACK_OMIT_JOHTO（コンパイル時）を実行時の判定 HackJohto_Open に置き換えた。J7 まで常に偽＝今までの配る版と同じ */
    return GetMapRegionOf(group, num) != MAP_REGION_JOHTO || HackJohto_Open();
}

u8 GetCurrentMapRegion(void)
{
    return GetMapRegionOf((u8) gSaveBlock1->location.mapGroup, (u8) gSaveBlock1->location.mapNum);
}

/* その地図がバトルフロンティア（群 69 の 4〜55 のうち 9〜13 を除く）か。トレーナーヒルなど群 69 の残り・南の孤島・サファリゾーンは含まない */
bool8 MapRegion_IsFrontierMap(u8 group, u8 num)
{
    return group == HOENN_FRONTIER_MAP_GROUP && num >= HOENN_FRONTIER_MAP_FIRST && num <= HOENN_FRONTIER_MAP_LAST
        && !(num >= HOENN_FRONTIER_MAP_HOLE_FIRST && num <= HOENN_FRONTIER_MAP_HOLE_LAST);
}

/* 今いる所がバトルフロンティアか */
bool8 MapRegion_CurrentIsFrontier(void)
{
    return MapRegion_IsFrontierMap((u8) gSaveBlock1->location.mapGroup, (u8) gSaveBlock1->location.mapNum);
}

/* 番号 → 地名の文字。シンオウはシンオウだけの番号として引く（範囲の外はいつもの表） */
const u8 *MapRegion_GetName(u16 mapsec, u8 region)
{
    if (mapsec >= MAPSEC_NAME_COUNT)   /* L 案（2026-10-16）: 出会った場所は 16 ビット。表に無い番号は「ふめい」（MAPSEC_NONE）の名前に */
        mapsec = MAPSEC_NONE;
    if (region == MAP_REGION_SINNOH && mapsec < SINNOH_TM_NAME_COUNT)
        return gSinnohMapSecNames[mapsec];
    if (region == MAP_REGION_KITAKAMI && mapsec < KITAKAMI_TM_NAME_COUNT)
        return gKitakamiMapSecNames[mapsec];
    return gExpandedMapSecNames[mapsec];
}

/* ここで作ったポケモンの親の版（CreateBoxMon・タマゴがかえったとき）。シンオウだけ 12 */
u8 MapRegion_MetGameHere(void)
{
    switch (GetCurrentMapRegion())
    {
    case MAP_REGION_SINNOH:
        return MET_GAME_SINNOH;
    case MAP_REGION_KITAKAMI:
        return MET_GAME_KITAKAMI;
    default:
        return gGameVersion;
    }
}

u8 MapRegion_FromMetGame(u8 metGame)
{
    if (metGame == MET_GAME_KITAKAMI)
        return MAP_REGION_KITAKAMI;
    return (metGame == MET_GAME_SINNOH) ? MAP_REGION_SINNOH : MAP_REGION_KANTO;
}

/* 親の版（4 ビット）。★ GetMonData(MON_DATA_MET_GAME) は CFRU の bytereplacement（0803F7A6・LSL #22／LSR #29）で
   下 3 ビットしか返さず、12 → 4・13 → 5 になる（2026-10-15 シンオウで捕まえた子の出会った場所がホウエンの地名に・patches/0379）。
   CFRU のポケモンは暗号なし・並び固定なので欄をそのまま読む（+0x46 の u16 の bit7〜10・src/hack_mon_id.c の B_METINFO と同じ所） */
u8 MapRegion_MonMetGame(const struct Pokemon *mon)
{
    return mon->metGame;
}

/* このゲームで出会ったか（FR・LG・シンオウの印）。ステータス画面の文の選び分けに使う */
bool8 MapRegion_IsOurMetGame(u8 metGame)
{
    return metGame == VERSION_FIRE_RED || metGame == VERSION_LEAF_GREEN || metGame == MET_GAME_SINNOH || metGame == MET_GAME_KITAKAMI;
}
