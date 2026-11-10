#include "defines.h"
#include "bw_follower_internal.h"
#include "../include/pokemon.h"
#include "../include/pokemon_icon.h"
#include "../include/sprite.h"
#include "../include/main.h"
#include "hack_flag_fast.h" //[hack] 2026-11-03 perf-region-3 旗の直読み

extern const u16 gBwFollowerSpeciesToGfx[];
extern const u16 gBwFollowerSpeciesToGfxCount;
extern const u16 gBwFollowerCount;

/* いま連れ歩くべき絵の番号（0 = 連れ歩けない）
   ★ **「手持ちの 1 番目」ではなく「最初の生きているポケモン」**。HGSS 本家がそうしている
   （`参考/pokeheartgold` の `FollowMon_ChangeMon()` が `GetFirstAliveMonInParty_CrashIfNone`)。
   1 番目が瀕死・タマゴでも、次の元気な子が付いてくる。 */
/* 連れ歩く 1 匹（NULL = いない）。**最初の生きているポケモン** */
struct Pokemon *BwFollowerLeadMon(void)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; ++i)
    {
        struct Pokemon *mon = &gPlayerParty[i];

        /* ★ 2026-10-07 欄を直接読む（CFRU の手持ちは暗号化されていない）。毎コマ呼ばれるので GetMonData だと重い（デモ版の報告「処理落ち」）。
           タマゴの種族も 0 ではないので、SPECIES2（タマゴなら SPECIES_EGG）で空きを見ていたのと同じ */
        if (mon->species == SPECIES_NONE)
            break;                              /* ここから先は空 */
        if (mon->isEgg || mon->hp == 0)
            continue;                           /* タマゴ・瀕死は飛ばす */

        return mon;
    }

    return NULL;
}

/* ---- わたしたちの伝説 10 匹（0x5D6〜0x5DF・#111）の連れ歩きの切り替え ----
   ★ 2026-10-16 ユーザー「連れ歩き ON に戻して」で 1（restored32 の絵）。止めたいときは 0 にする: 0 のあいだ、10 匹は先頭でも連れ歩きを出さない（ふだんの姿も Moémon の姿も）。
   絵の表（32x32・gBwFollowerSpeciesToGfx・Moémon の枠）はそのまま残してあるので、1 に戻すだけで出る。
   出ない子の扱い（絵の番号 0 ＝ 先頭でもボールの中・サーフの絵も出さない）は、絵を持たない種族と同じ */
#define BW_FOLLOWER_LEGENDS_ENABLED 1
#define BW_FOLLOWER_LEGEND_FIRST 0x5D6
#define BW_FOLLOWER_LEGEND_LAST 0x5DF

static bool8 BwFollowerLegendDisabled(u16 species)
{
    return !BW_FOLLOWER_LEGENDS_ENABLED && species >= BW_FOLLOWER_LEGEND_FIRST && species <= BW_FOLLOWER_LEGEND_LAST;
}

/* ---- 種族 -> 絵の番号 ---- */
u16 BwFollowerGfxIdForSpecies(u16 species)
{
    if (BwFollowerLegendDisabled(species))
        return 0;

    /* ★ 2026-10-11 バイバニラ（かいはつしゃのすがた）は絵を持たず、ふつうのバイバニラの絵で連れ歩く
       （ユーザー「連れ歩きは普通のバイバニラで」・tools/custom_mon/vanilluxe_dev。色違いも同じ絵の番号から引かれる） */
    if (species == SPECIES_VANILLUXE_DEV)
        species = SPECIES_VANILLUXE;

    /* ★ 2026-10-16 デバッグモン（SPECIES_DEBUGMON_MAX／MIN・tools/custom_mon/debug_mon・patches/0422）も絵を持たず、レントラーの絵で連れ歩く
       （表を増やさない・色違いも同じ絵の番号から引かれる） */
    if (species == SPECIES_DEBUGMON_MAX || species == SPECIES_DEBUGMON_MIN)
        species = SPECIES_LUXRAY;

    /* ★ 2026-10-25 ゲンシガチゴラス（戦闘中だけの姿・patches/0801）の連れ歩きはガチゴラスのまま（ユーザー） */
    if (species == SPECIES_GENSHIGACHIGORAS)
        species = SPECIES_TYRANTRUM;

    if (species >= gBwFollowerSpeciesToGfxCount)
        return 0;

    return gBwFollowerSpeciesToGfx[species];
}

/* 絵の番号（表 << 8 | 位置）-> 通し番号 */
u16 BwFollowerIndexOf(u16 gfxId)
{
    u16 table = gfxId >> 8;

    /* 表 9 以降 = 通し番号 1280 以降（表 6〜8 は連れ歩きではない） */
    if (table >= BW_FOLLOWER_EXT_FIRST_TABLE)
        return ((table - BW_FOLLOWER_EXT_FIRST_TABLE + BW_FOLLOWER_BASE_TABLES) << 8) | (gfxId & 0xFF);
    return ((table - BW_FOLLOWER_FIRST_TABLE) << 8) | (gfxId & 0xFF);
}

/* ---- Moémon の連れ歩き（#55・2026-10-08 ユーザー「アイコンと連れ歩きもあるならいれていい」）----
   絵は assembly/hack_moemon_follow/moemon_followers.s（tools/sprites/build_moemon_followers.py・--split-pics の版だけ・
   ほかの版は tools/build/cfru_runner.py が空の表に）。通し番号は連れ歩きの続き（gBwFollowerCount + j・表 1〜5 の空き枠・
   絵の情報と色の表は tools/bwfollow/build_bw_follower.py が作る）。
   どの子が萌えもんかは DPE だけが知っているので、CFRU から呼べる GetIconSpecies（08096988・DPE のフック）が返す
   萌えもんのアイコンの番号（DPE の NUM_SPECIES + k）で決める＝戦いの絵・アイコンと同じ決め方（ON/OFF・前からいた子・通信）。 */
extern const u32 gMoemonFollowerCount;
extern const u16 gMoemonFollowerIconBase;
extern const u16 gMoemonFollowerIconCount;
extern const u8 gMoemonFollowerByIcon[];
extern const u16 gBwFollowerMoeSlots;
extern const u16 gBwFollowerOwPalMoeFirst;

static u16 BwFollowerIndexToGfxId(u16 index)
{
    u16 table = index >> 8;

    if (table >= BW_FOLLOWER_BASE_TABLES)
        return ((table - BW_FOLLOWER_BASE_TABLES + BW_FOLLOWER_EXT_FIRST_TABLE) << 8) | (index & 0xFF);
    return ((table + BW_FOLLOWER_FIRST_TABLE) << 8) | (index & 0xFF);
}

bool8 BwFollowerValidIndex(u16 index)
{
    return index < gBwFollowerCount
        || ((u32) (index - gBwFollowerCount) < gMoemonFollowerCount && (u32) (index - gBwFollowerCount) < gBwFollowerMoeSlots);
}

/* ★ 2026-11-03 perf-region-3: BwFollowerGfxIdForMon は GetMonData＋GetIconSpecies（DPE の HackMoemon_Slot＝裏 ID の探索・前からいた子の一覧）で 1 回約 1,000 サイクル、
   連れ歩きの毎コマの処理（BwFollowerHideForFieldEffect）で 1 コマに約 2 回（絵の番号の確かめ＋色違いの確かめ）呼ばれていた。
   → 「同じコマの中で、同じ子・同じ入力」のときだけ前の答えを使う 1 件の覚え（EWRAM の固定番地・static は捨てられる＝6.13）。
   ・置き場: 0x0203EFC0〜0x0203EFDF（32 バイト・_Static_assert で確かめる）。空き 0x0203E274〜0x0203F0E8 の中・RAM チートの影 0x0203EF80〜9F・計測 0x0203F800〜・BGM 0x0203E300〜0x0203E800 の外。
     電源を入れ直すと 0（valid = 0）。セーブには入らない
   ・無効にする合図: 覚えが使えるのは「コマの番号（gMain.vblankCounter2）・子の番地・性格値・種族・+0x13 の 1 バイト（タマゴ／壊れたタマゴ／萌えもんの通信の印）・
     同じ性格値の最初の手持ちの裏 ID・萌えもんの旗 0x12E9／しばりの旗 0xFA9・前からいた子の一覧の頭（印と件数）」が全部同じときだけ。どれか 1 つでも違えば計算し直す。
     通信中（通信対戦・通信交換）は覚えない（相手の手持ちの印を読むので）。コマが替われば必ず捨てる＝同じコマの中で 手持ち・旗が替わっても、次のコマには残らない
   ・答えは計算し直した値と同じ（検算 tools/rom/verify_perf_region_3.py が 前の版と ホストで比べる） */
struct BwGfxMemo
{
    u32 frame;
    u32 mon;            /* 子の番地 */
    u32 personality;
    u32 id;             /* 手持ちで同じ性格値の最初の子の裏 ID（+0x3E） */
    u32 oldMagic;       /* 前からいた子の一覧の印 */
    u16 species;        /* 欄の種族（+0x20） */
    u16 oldCount;
    u16 gfxId;          /* 答え */
    u8 flags13;         /* +0x13 */
    u8 on;              /* bit0 = 旗 0x12E9・bit1 = 旗 0xFA9 */
    u8 valid;
    u8 pad[3];
};
_Static_assert(sizeof(struct BwGfxMemo) == 32, "BwGfxMemo must fit 0x0203EFC0-0x0203EFDF");
#ifndef BW_GFX_MEMO   /* 検算（ホスト）だけが差し替える */
#define BW_GFX_MEMO ((volatile struct BwGfxMemo *) 0x0203EFC0)
#endif
#ifndef BW_GFX_MEMO_LINK_ACTIVE   /* 検算（ホスト）だけが差し替える */
#define BW_GFX_MEMO_LINK_ACTIVE (*(const volatile u8 *) 0x03003FA4)        /* gReceivedRemoteLinkPlayers（src/bw_summary_screen.c と同じ） */
#endif
#ifndef BW_GFX_MEMO_BATTLE_FLAGS   /* 検算（ホスト）だけが差し替える */
#define BW_GFX_MEMO_BATTLE_FLAGS (*(const volatile u32 *) 0x02022AAC)      /* gBattleTypeFlags（DPE の src/hack_moemon.c と同じ・bit1 = 通信） */
#endif
#define BW_GFX_MEMO_OLD_LIST(sb1) ((const u32 *) ((const u8 *) (sb1) + 0x2680))   /* 前からいた子の一覧（src/hack_moemon_switch.c の MoemonOldList: magic(u32)・count(u16)） */
#define BW_GFX_FLAG_MOEMON_ON 0x12E9
#define BW_GFX_FLAG_CH_BATTLE 0xFA9

static u16 BwFollowerGfxIdForMonCompute(struct Pokemon *mon);

/* 裏 ID は +0x3E（2 バイト境界）の 32 ビット＝u16 を 2 回で読む（ARM は 4 バイトに揃っていない読みで値が回る） */
static u32 BwGfxMemoId(const u8 *raw)
{
    return *(const u16 *) (raw + 0x3E) | ((u32) *(const u16 *) (raw + 0x40) << 16);
}

/* DPE の HackMonId_MoemonKey は「手持ちの中で同じ性格値の最初の子」の裏 ID を見る（子自身とは限らない）。覚えの鍵にはその子の裏 ID を入れる */
static u32 BwGfxMemoKeyId(const u8 *raw)
{
    u32 pers = *(const u32 *) raw;
    u32 i;

    for (i = 0; i < PARTY_SIZE; ++i)
    {
        const u8 *m = (const u8 *) &gPlayerParty[i];

        if (*(const u32 *) m == pers && *(const u16 *) (m + 0x20) != SPECIES_NONE)
            return BwGfxMemoId(m);
    }
    return 0;
}

u16 BwFollowerGfxIdForMon(struct Pokemon *mon)
{
    volatile struct BwGfxMemo *memo = BW_GFX_MEMO;
    const u8 *raw = (const u8 *) mon;
    const u32 *oldList = BW_GFX_MEMO_OLD_LIST(gSaveBlock1);
    u16 gfxId;

    if (BW_GFX_MEMO_LINK_ACTIVE == 1 || (BW_GFX_MEMO_BATTLE_FLAGS & 0x0002))
    {
        memo->valid = 0;
        return BwFollowerGfxIdForMonCompute(mon);
    }
    if (memo->valid
     && memo->frame == gMain.vblankCounter2
     && memo->mon == (u32) mon
     && memo->personality == *(const u32 *) raw
     && memo->species == *(const u16 *) (raw + 0x20)
     && memo->flags13 == raw[0x13]
     && memo->id == BwGfxMemoKeyId(raw)
     && memo->on == ((HackXFlagGet(BW_GFX_FLAG_MOEMON_ON) ? 1 : 0) | (HackXFlagGet(BW_GFX_FLAG_CH_BATTLE) ? 2 : 0))
     && memo->oldMagic == oldList[0]
     && memo->oldCount == (u16) oldList[1])
        return memo->gfxId;

    gfxId = BwFollowerGfxIdForMonCompute(mon);
    memo->valid = 0;
    memo->frame = gMain.vblankCounter2;
    memo->mon = (u32) mon;
    memo->personality = *(const u32 *) raw;
    memo->species = *(const u16 *) (raw + 0x20);
    memo->flags13 = raw[0x13];
    memo->id = BwGfxMemoKeyId(raw);
    memo->on = (HackXFlagGet(BW_GFX_FLAG_MOEMON_ON) ? 1 : 0) | (HackXFlagGet(BW_GFX_FLAG_CH_BATTLE) ? 2 : 0);
    memo->oldMagic = oldList[0];
    memo->oldCount = (u16) oldList[1];
    memo->gfxId = gfxId;
    memo->valid = 1;
    return gfxId;
}

static u16 BwFollowerGfxIdForMonCompute(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);

    if (BwFollowerLegendDisabled(species))
        return 0;   /* Moémon の姿も出さない（GetIconSpecies の前に止める） */

    if (gMoemonFollowerCount != 0)
    {
        u16 icon = GetIconSpecies(species, mon->personality);

        if (icon >= gMoemonFollowerIconBase && icon < gMoemonFollowerIconBase + gMoemonFollowerIconCount)
        {
            u8 j = gMoemonFollowerByIcon[icon - gMoemonFollowerIconBase];

            if (j != 0 && j <= gBwFollowerMoeSlots)
                return BwFollowerIndexToGfxId(gBwFollowerCount + j - 1);
        }
    }
    return BwFollowerGfxIdForSpecies(species);
}

/* ---- 色違い（2026-09-28 ユーザー「連れ歩きの色違い進めて」）----------------
   パレットは**札**で引かれる（連れ歩きは 0x2000 + 通し番号・生成物 src/bw_follower_gfx.c）。
   札から表の何件目かを探す素の関数 FindEventObjectPaletteIndexByTag（0805EE60）を**丸ごと置き換え**
   （hooks の BWFOLLOW ブロック・r3）、連れ歩きの札で、いま連れ歩く子（最初の生きているポケモン）が
   その絵で色違いなら、色違いの件（表の後ろ・札 0x2800 + 通し番号）を返す。
   ★ 札から色を積む道は全部この関数を通る: 素の PatchObjectPalette / LoadObjectEventPalette（呼び元は BL 2 つだけ）、
     CFRU の動的パレット（dynamic_ow_pals.c の LoadNPCPalette = PatchObjectPalette）、こちらの BwFollowerLoadOwnPalette
   ★ 絵の番号（gfxId）は色違いでも同じなので、札も同じ。動的パレットは札で置き場を使い回すため、
     同じ種族で色違いとそうでない子を入れ替えると古い色が残る → BwFollowerCheckShinyPalette が積み直す
   ★ 素の関数は表の終わりを 0x11FF で見ていたが、写した表の終わりは { NULL, 0 }。こちらは data が NULL で止める */
extern const struct SpritePalette gBwFollowerOwPalTable[];
extern const u16 gBwFollowerOwPalFirst;
extern const u16 gBwFollowerOwPalShinyFirst;
#define BW_SHINY_PAL_TAG_BASE 0x2800        /* tools/bwfollow/build_bw_follower.py の BWF_SHINY_PAL_TAG_BASE と同じ */

/* 通し番号 index の絵を出している子が色違いか。
   ★ 2026-10-05 ユーザー「連れ歩きポケモンが色違い同士だと入れ替えたとき通常色になる」:
     前は「いまの先頭」だけを見ていた。手持ちを入れ替えた直後は、絵が新しい先頭に替わるまで前の子の絵が出ている
     （先頭が絵を出せない種族なら前の子の絵のまま）。そのあいだ前の子の色違いの色が「先頭と違う」として通常の色に
     塗り直され、入れ替えた先でもその色が残った。→ 生きている手持ちを前から見て、その絵の子を最初に見つけたら、その子で決める */
bool8 BwFollowerLeadShinyFor(u16 index)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; ++i)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        u16 gfxId;

        if (mon->species == SPECIES_NONE)       /* ★ 2026-10-07 欄を直接読む（毎コマ・BwFollowerLeadMon と同じ） */
            break;
        if (mon->isEgg || mon->hp == 0)
            continue;
        gfxId = BwFollowerGfxIdForMon(mon);
        if (gfxId != 0 && BwFollowerIndexOf(gfxId) == index)
            return IsMonShiny(mon);
    }
    return FALSE;
}

/* 通し番号 index の色が gBwFollowerOwPalTable の何件目か（Moémon は表の後ろ・2026-10-08） */
u16 BwFollowerPalPos(u16 index, bool8 shiny)
{
    if (index >= gBwFollowerCount)
        return gBwFollowerOwPalMoeFirst + (shiny ? gBwFollowerMoeSlots : 0) + (index - gBwFollowerCount);
    return (shiny ? gBwFollowerOwPalShinyFirst : gBwFollowerOwPalFirst) + index;
}

/* 0805EE60 FindEventObjectPaletteIndexByTag の代わり。→ 表の何件目か（無ければ 0xFF。素と同じ） */
u16 BwFollowerFindOwPalIndex(u16 tag)
{
    u16 i;

    if (tag >= BW_PAL_TAG_BASE && tag < BW_PAL_TAG_BASE + gBwFollowerCount + gBwFollowerMoeSlots)
    {
        i = tag - BW_PAL_TAG_BASE;
        return BwFollowerPalPos(i, BwFollowerLeadShinyFor(i));
    }
    if (tag >= BW_SHINY_PAL_TAG_BASE && tag < BW_SHINY_PAL_TAG_BASE + gBwFollowerCount + gBwFollowerMoeSlots)
        return BwFollowerPalPos(tag - BW_SHINY_PAL_TAG_BASE, TRUE);

    for (i = 0; gBwFollowerOwPalTable[i].data != NULL && gBwFollowerOwPalTable[i].tag != 0x11FF; ++i)
    {
        if (gBwFollowerOwPalTable[i].tag == tag)
            return i;
    }
    return 0xFF;
}
