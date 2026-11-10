#pragma once

#include "../include/sprite.h"

/* Internal contracts shared by the hand-written follower modules.
 * Keep tags in sync with tools/bwfollow/build_bw_follower.py. */
#define BW_FOLLOWER_TILE_TAG 0x4F10
#define BW_FOLLOWER_FIRST_TABLE 1
/* 2026-10-15 (patches/0409) 通し番号 0〜1279 = 表 1〜5、1280 以降 = 表 9〜（表 6〜8 は BW の主人公・ホウエンの NPC・主人公の歩き）。
 * 通し番号 <-> 絵の番号は BwFollowerIndexOf / BwFollowerIndexToGfxId だけが変換する。tools/bwfollow/build_bw_follower.py と同じ値 */
#define BW_FOLLOWER_BASE_TABLES 5
#define BW_FOLLOWER_EXT_FIRST_TABLE 9
#define BW_FOLLOWER_EXT_TABLES 2
#define BW_FOLLOWER_PAL_SLOT 10
#define BW_PAL_TAG_BASE 0x2000

struct Pokemon;

struct Pokemon *BwFollowerLeadMon(void);
u16 BwFollowerGfxIdForSpecies(u16 species);
u16 BwFollowerGfxIdForMon(struct Pokemon *mon);
u16 BwFollowerIndexOf(u16 gfxId);
bool8 BwFollowerValidIndex(u16 index);
bool8 BwFollowerLeadShinyFor(u16 index);
u16 BwFollowerPalPos(u16 index, bool8 shiny);
bool8 BwFollowerLoadSheet(u16 gfxId);
bool8 BwFollowerLoadFSheet(const u8 *data, u16 rawSize, u16 tag);
bool8 BwFollowerBallBusy(void);
bool8 BwFollowerBannedHere(void);                 /* ★ #76 段 2: src/hack_heroine.c */
void BwFollowerUpdateFromParty(void);              /* ★ #76 段 2: src/hack_heroine.c から（わかれたときポケモンの連れ歩きを戻す） */
bool8 BwFollowerCutsceneFieldEffectActive(void);  /* ★ #76 段 2: src/hack_heroine.c */

extern const struct SpritePalette gBwFollowerOwPalTable[];
