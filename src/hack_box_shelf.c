#include "defines.h"
#include "../include/save.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "../include/string_util.h"
#include "../include/sound.h"
#include "../include/text.h"
#include "../include/text_window.h"
#include "../include/window.h"
#include "hack_box_shelf.h"
#include "hack_ext_save.h"
#include "hack_ramshift.h"
#include "shadow_relive.h"                     //[hack] patches/0965 段 G リライブボックス

/*
 * ボックスの仮想化（棚・付け替え方式・2026-10-06 ユーザー「本番お願い」「F=5・39箱」）。
 * 設計: docs/research/2026-10-06_box-virtualization-design.md
 * ★ 2026-10-23 X 案（箱の努力値・個体値を 16 ビットに・箱の 1 匹 = 78 バイト・patches/0680・docs/research/2026-10-23_stat-level-16bit.md 3 章）: 63 → 78 バイト（W の先頭 63 バイトに evHi 6・ivHi 9 を足す）。
 *   1 ページ 52 匹・ページ 0 は 8 匹・本体の棚の箱は 26（780 匹）・追加のセーブ領域は 27〜50 箱（箱 id 26〜49・720 匹・14 ページ）＝追加の区画 128〜155（ページ 14 × 2）・箱ページの控えは 156〜185。
 *   RAM の入れ物は 17 → 13 個（2,340 バイトずつ）。印は 'SHL4'。W（'SHL3'）の棚は読まない＝止める画面（セーブデータ移行ツールで X にする・自動では変えない）。
 * ★ 2026-10-17 W 案（箱のわざを 16 ビットに・箱の 1 匹 = 63 バイト・patches/0583・docs/research/2026-10-17_move-table-widen.md 4 章）: 60 → 63 バイト（末尾に moveHi 3 バイト）。
 *   1 ページ 64 匹・ページ 0 は 11 匹・本体の棚の箱は 32（960 匹）・追加のセーブ領域は 33〜50 箱（箱 id 32〜49）＝追加の 9 ページ（追加の区画 8〜25）・箱ページの控えは追加の区画 26〜55。
 *   RAM の入れ物は 18 → 17 個（1,890 バイトずつ）。印は 'SHL3'（'SHL2'＝L 案・'SHEL'＝58 バイトは C は読まない＝止める画面・セーブデータ移行ツールで変える）。
 * ★ 2026-10-16 L 案（箱の 1 匹 = 60 バイト・patches/0430・docs/research/2026-10-16_mon-data-l-format.md）: 全 50 箱を棚に入れ直した。
 *   20〜25 箱の固定の場所（SaveBlock1 +0x18E8・SaveBlock2 +0xF8・もとの区画 30 の +0xB10〜）はもう使わない（60 バイトの箱は入らない）。
 *
 * フラッシュ（4KB × 32 区画）:
 *   0〜4 / 5〜9   本体の組 A / B（src/save.c の NUM_SECTORS_PER_SAVE_SLOT 5＝SaveBlock2・SaveBlock1・parasite 1/2）
 *   10〜31        棚（物理 22）。論理ページ 18（0〜17）・予備 4
 *   ★ もとの CFRU の区画 30・31（RAM 0x0203BFAC〜0x1FE0・バッグ・ずかんナビなど）は棚のページ 16・17 に入れた
 *   ★ 殿堂入りの記録（もと 28・29）はもう書かない
 * 論理ページ（0xFF0 = 4,080 バイト）:
 *   ページ 0 = 今の箱の番号・箱の名前と壁紙・parasite 3（もとは組の id 13 の空きに置いていた 0xBA0）・ポケモン 11 匹
 *   ページ 1〜15 = ポケモン 68 匹ずつ。本体の棚の箱（箱 id 0〜33 = 34 箱 = 1,020 匹）は、ポケモンの帯（ページ 0 の 11 匹 → ページ 1 から 68 匹ずつ）に続けて並ぶ
 * 対応表（論理 → 物理）は SaveBlock1 + 0x1400（questLog の跡の空き・本体の二重保存の中）。印は 'SHL2'（古い形は 'SHEL'＝C は読まない・BoxShelf_IsOldFormat）。
 * レポート: 汚れたページ（＋ページ 0）だけを、今の対応表で使っていない物理区画へ書いてから本体の組を書く（本体を書き終えたら切り替わり）。
 *
 * RAM: 素の PokemonStorage（0x0202924C〜）を使い直す
 *   +0         今の箱の番号（素と同じ）
 *   +4         箱の入れ物 18 個（1,800 バイトずつ・1 箱の 30 匹が続けて並ぶ＝ステータス画面などがポインタを足し算する）→ +0x7E94 まで
 *   +0x7E94    入れ物の管理（struct ShelfCache・+0x8128 まで）
 *   +0x8128    15〜25 箱の壁紙（CFRU のまま）
 *   +0x8134    26〜39 箱の名前（9 バイトずつ）・+0x81B2 26〜39 箱の壁紙
 *   +0x82E1    15〜25 箱の名前・+0x8344 1〜14 箱の名前・+0x83C2 1〜14 箱の壁紙（素・CFRU のまま）
 *
 * 35〜50 箱（箱 id 34〜49・2026-10-09 ユーザー「ボックス50に拡張できる？」・L 案で 40 → 35 箱から）:
 *   直した mGBA の追加のセーブ領域（src/hack_ext_save.c）だけ。ポケモンの帯の 1,020 匹目から、
 *   拡張のページ 0〜7（論理ページ 18〜25・68 匹ずつ）・ページ 7 の EXT_META_OFF から 40〜50 箱（箱 id 39〜49）の名前と壁紙。
 *   物理は追加の区画 8〜23（16）。対応表はページ 0 の後ろ（P0_EXT・印 'X' ＋ 8 バイト）＝本体 → ページ 0 → 拡張とたどるので、
 *   付け替え方式の強さは棚と同じ。追加の領域が使えない版（32MB・実機・SuperCard）では 35〜50 箱は触れない（中身は空に見え、
 *   対応表はそのまま残す＝直した mGBA に戻せばまた見える）。
 *   RAM: +0x8260 40〜50 箱の名前（9 バイトずつ）・+0x82C3 40〜50 箱の壁紙
 */

#define STORAGE_BASE HACK_STORAGE_BASE      //PokemonStorage の今の番地（0x0202924C + 版ごとのずらし・src/hack_ramshift.h・patches/0622）。gPokemonStoragePtr と同じ値
//SHELF_PHYS_FIRST／COUNT・EXT_PHYS_FIRST／COUNT・OVFL_PHYS_FIRST／COUNT は hack_ext_sector_map.h（生成物・正本 tools/save_layout/ext_sector_map.json）
#define SHELF_PAGES 18
#define BOX_PAGES 16                //ページ 0〜15 がポケモンの帯（0 は名前なども）
#define REGION_RAM 0x0203BFAC       //もとの CFRU の区画 30・31 の中身（SAVE_BLOCK_PARASITE + PARASITE_SIZE）→ ページ 16・17
#define SHELF_PAGE_SIZE 0xFF0
#define SHELF_FOOTER_ID 0x40
#define SHELF_SIGNATURE 0x08012025
#define SHELF_MAP_MAGIC 0x344C4853     // 'SHL4'（X 案・78 バイト・箱の努力値・個体値 16 ビット）
#define SHELF_MAP_MAGIC_W 0x334C4853   // 'SHL3'（W 案・63 バイト・C は読まない＝止める画面）
#define SHELF_MAP_MAGIC_L 0x324C4853   // 'SHL2'（L 案・60 バイト・C は読まない＝止める画面）
#define SHELF_MAP_MAGIC_OLD 0x4C454853 // 'SHEL'（58 バイトの古い形・C は読まない＝止める画面）
#define SHELF_CACHE_MAGIC 0x45484353   // 'SCHE'
#define SHELF_FRAMES 13                             // X 案: 17 → 13（入れ物 13 個 = 30,420 バイト + ShelfCache が +0x7E88 の HFR の写しの手前に収まる最大）
#define MON_SIZE sizeof(struct CompressedPokemon)   // 78
#define BOX_BYTES (IN_BOX_COUNT * MON_SIZE)         // 2,340
#define PAGE0_MONS 8
#define PAGE_MONS 52
#define NO_PAGE 0xFF
#define NO_BOX 0xFF

//33〜50 箱（追加のセーブ領域）
#define EXT_BOX_FIRST 26                        //箱の id
#define EXT_BOXES 24
#define EXT_SHELF_FIRST 26                      //棚の箱の番号（= 箱の id）
#define EXT_G_FIRST (EXT_SHELF_FIRST * IN_BOX_COUNT)   //780
#define EXT_PAGES 14
#define EXT_PAGE_BASE SHELF_PAGES               //論理ページ 18〜31
#define EXT_PAGE_LAST (EXT_PAGE_BASE + EXT_PAGES - 1)
//★ 2026-10-26 FEAT-box100（0.61.0・ボックス 100 箱・patches/0913）: 51〜100 箱（箱 id 50〜99・g＝1,500〜2,999）は論理ページ 32〜60（29 枚・52 匹ずつ）。
//  50 箱目までの並び（本体の棚 26 箱・追加の 14 ページ・EXT_PAGES・P0_EXT・P0_OVFL）は 1 バイトも動かさない。物理は追加の区画 EXT2（186〜243・29 × 2）。対応表はページ 0 の P0_NEW（印 'Z' ＋ 29 バイト）。
//  名前・壁紙（50 箱ぶん）は論理ページ 60 の 44 匹の後ろ（NEW_META_OFF）・RAM は PokemonStorage + BOX2_NAMES_OFF（ShelfCache の後ろ・HFR の写しの手前の空き）
#define NEW_BOX_FIRST 50
#define NEW_BOXES 50
#define NEW_G_FIRST (NEW_BOX_FIRST * IN_BOX_COUNT)   //1500
#define NEW_PAGES 29
#define NEW_PAGE_BASE (EXT_PAGE_BASE + EXT_PAGES)    //32
#define NEW_PAGE_LAST (NEW_PAGE_BASE + NEW_PAGES - 1) //60
#define ALL_PAGES (SHELF_PAGES + EXT_PAGES + NEW_PAGES)
#define ALL_EXT_PAGES (EXT_PAGES + NEW_PAGES)       //追加の区画に置くページ（論理 18〜60）の数
#define IS_NEW_PAGE(p) ((p) >= NEW_PAGE_BASE)
#define NEW_META_OFF ((NEW_BOXES * IN_BOX_COUNT - (NEW_PAGES - 1) * PAGE_MONS) * MON_SIZE)   //最後の新ページ（論理 60）の 44 匹の後ろ（3,432）
#define NEW_META_SIZE (NEW_BOXES * 10)               //名前 9 バイト × 50 ＋ 壁紙 1 バイト × 50
#define NEW_NAMES_RAM ((u8*) STORAGE_BASE + BOX2_NAMES_OFF)
#define NEW_WALLS_RAM ((u8*) STORAGE_BASE + BOX2_NAMES_OFF + NEW_BOXES * 9)
#define NEW_MAP_MAGIC 0x5A                          //'Z'
#define EXT_META_BOXES 11                       //名前・壁紙を持つ箱（箱 id 39〜49）
#define EXT_META_OFF ((EXT_BOXES * IN_BOX_COUNT - (EXT_PAGES - 1) * PAGE_MONS) * MON_SIZE)   //最後の追加ページ（論理 31）の 44 匹の後ろ（3,432）
#define EXT_META_SIZE (EXT_META_BOXES * 9 + EXT_META_BOXES)
#define EXT_NAMES_RAM ((u8*) STORAGE_BASE + 0x8260)
#define EXT_WALLS_RAM ((u8*) STORAGE_BASE + 0x82C3)
#define EXT_MAP_MAGIC 0x58                      //'X'
#define IS_EXT_PAGE(p) ((p) >= EXT_PAGE_BASE)

//予備を超えた箱のページ（2026-10-15 ユーザー「予備のページを増やす」・docs/research/2026-10-15_box-shelf-spare-pages.md）:
//  追加のセーブ領域が使えるときだけ、本体の予備（4）に収まらないページ 1〜15 を追加の区画 24〜53（30）へ書く。
//  本体の区画の古い写しはそのまま残す（追加の領域が無い版で開いても壊れず、古い中身で読める）。どちらが新しいかはページの印の counter で決める。
//  対応表は拡張のページ 7（論理 25）の OVFL_OFF（印 'Y' ＋ ページ 1〜15 の区画・0xFF = 無し）＝ページ 0 の 'X' の対応表を通して本体の組と一緒に切り替わる。
//  ページ 0 の P0_OVFL に印 'O'（追加の区画に新しい写しがあるかも）。追加の領域が無い版ではその印があれば 1〜34 箱を読むだけにする（古い写しの上に書かない）。
#define OVFL_FIRST_PAGE 1
//OVFL_PHYS_FIRST 156／OVFL_PHYS_COUNT 30（X 案: 追加のページ 14 枚 × 2 = 追加の区画 128〜155 の後ろ。ページ 15 枚 × 2 で書き換え中の写しを残す）
#define OVFL_OFF (EXT_META_OFF + EXT_META_SIZE)  //最後の追加ページの中: [0] = 印 'Y'・[p] = ページ p の区画（3,542）
#define OVFL_MAGIC 0x59                         //'Y'
#define OVFL_FLAG 0x4F                          //'O'
#define BOX_PAGES_MASK 0xFFFE                   //論理ページ 1〜15

//ページ 0 の並び
#define P0_HEAD 0                       //+0〜4（今の箱の番号）
#define P0_TAIL_A 4                     //+0x8128〜+0x81C0（152）
#define P0_TAIL_B (4 + 0x98)            //+0x82E1〜+0x83D0（239）
#define P0_PARASITE (P0_TAIL_B + 0xEF)  //parasite 3（0xBA0）
#define P0_MONS (P0_PARASITE + 0xBA0)   //11 匹（〜4,031）
#define P0_EXT (P0_MONS + PAGE0_MONS * MON_SIZE)   //33〜50 箱の対応表（印 1 ＋ 9 バイト・〜4,074）
#define P0_OVFL (P0_EXT + 1 + EXT_PAGES)           //追加の区画に新しい写しがあるかもの印（1 バイト・〜4,041）
#define P0_NEW (P0_OVFL + 1)                       //51〜100 箱の対応表（印 'Z' ＋ NEW_PAGES バイト・〜4,040）
#define PARASITE3_RAM (0x0203B0E8 + 0xCC + 0x258)
#define PARASITE3_SIZE 0xBA0

struct ShelfMap        //SaveBlock1 + 0x1400（本体の組に入る）
{
	u32 magic;
	u32 seq;
	u8 phys[SHELF_PAGES];
};

struct ShelfCache      //PokemonStorage + SHELF_CACHE_OFF（保存しない・+0x8128 の壁紙の手前まで）
{
	u32 magic;
	u32 legacyMask;              //★ 2026-10-21 続きからで読んだ 署名のない（前の版の）ページ（ビット = 論理ページ）。レポートで書き直したら下ろす（BoxShelf_Commit の「署名の書き直し」）
	u32 sum16;                   //ページ 17 の RAM の和（最後に書き終えたレポートのとき）→ 変わったかの目安
	u32 pendingSum16;
	u32 lateMask;                //予備が足りず、本体のあとに今の区画へ上書きするページ（ビット = ページ・起きないはず・M2）
	u8 frameBox[SHELF_FRAMES];
	u8 frameDirty[SHELF_FRAMES];
	u8 frameStamp[SHELF_FRAMES];
	u8 stamp;
	u8 cursor;
	u8 committed[ALL_PAGES];     //最後に書き終えたレポートの対応表（18〜25 は追加の区画の番号）
	u8 written[ALL_PAGES];       //このレポートで書いたページ
	u8 overBudget;
	u8 extPending[ALL_EXT_PAGES];    //このレポートの 27〜100 箱の対応表（ページ 0 に書く・[0〜13] = P0_EXT・[14〜42] = P0_NEW）
	u8 ovfl[BOX_PAGES];          //ページ 1〜15 の追加の区画の写し（無ければ NO_PAGE・追加の領域が使えるときだけ）
	u8 ovflPending[BOX_PAGES];   //このレポートの対応表
	u8 extOk;                    //HackExtSave_Available の答えの覚え（0 = まだ・1 = 使える・2 = 使えない）
	u8 ovflUsed;                 //ページ 0 の印 'O'（追加の区画に新しい写しがあるかも）
	u8 scrubOk;                  //★ 2026-10-21 このセッションで 署名のない前の版のページを読んだ（古い写しを潰してよい・ScrubStaleLegacy）
	u8 tamper;                   //★ 2026-10-21 続きからで読んだ棚のページに MAC が合わないものがあった（BoxShelf_TamperSeen・src/hack_tamper.c が印を付ける）
	u8 oldFormat;                //続きからで見た対応表が古い形 'SHEL'（58 バイト）か 'SHL2'（60 バイト）だった＝C は読まず・書かない（BoxShelf_IsOldFormat）
	u8 visState;                 //★ FEAT-box100-unlock 51〜100 箱を出すか（0 = まだ決めていない・1 = 隠す（1〜50 が満杯でない）・2 = 出す＝このセッションでは戻さない）
	u16 visEpoch;                //visState = 1 を決めたときの writeEpoch（変わるまで数え直さない）
	u16 writeEpoch;              //箱の中身を書くたびに増やす（BoxShelf_BoxMons の書き用・BoxShelf_MarkPtrDirty）
};

#define SHELF_CACHE_OFF ((4 + SHELF_FRAMES * BOX_BYTES + 3) & ~3)   //0x76D8（入れ物 13 個の後ろを 4 の倍数に切り上げ）
#define HFR_RAM_START 0x7E88                                         //src/hoenn_frontier_rec.c の HFR_RAM_OFF（レベル49・100 の記録の RAM の写し）
typedef char ShelfCacheFits[(sizeof(struct ShelfCache) <= 0x8128 - SHELF_CACHE_OFF && (SHELF_CACHE_OFF & 3) == 0) ? 1 : -1];   //+0x8128 から 15〜25 箱の壁紙
typedef char ShelfCacheLeavesBox2Names[(SHELF_CACHE_OFF + sizeof(struct ShelfCache) <= BOX2_NAMES_OFF && BOX2_NAMES_OFF + NEW_META_SIZE <= HFR_RAM_START && (BOX2_NAMES_OFF & 3) == 0) ? 1 : -1];   //51〜100 箱の名前・壁紙は ShelfCache の後ろ・HFR の写し（+0x7E88）の手前
typedef char ShelfCacheLeavesFrontierRec[(SHELF_CACHE_OFF + sizeof(struct ShelfCache) <= HFR_RAM_START) ? 1 : -1];   //HFR の RAM の写し（+0x7E88〜）の手前に収まる
typedef char FramesFitX[((SHELF_FRAMES + 1) * BOX_BYTES + 4 + sizeof(struct ShelfCache) > HFR_RAM_START) ? 1 : -1];   //もう 1 個は入らない（13 が最大）
//★ X 案（78 バイト）の版の守り: 箱の形を X で書く最初の版は 0.59.2（0x3B02）。セーブデータ移行ツール（tools/save_migrate_l/lformat.py の VER_FORMAT_X）が W → X のとき最後にレポートした版を 0x3B02（0.59.2）以上に上げ、
//  W を読む古い ROM（0.53〜0.59.1）は「新しい版のレポート」と見て止まる。この ROM の版がそれより小さいと、移行したセーブを「新しすぎる」と自分で止めてしまう＝版を上げてから組む
#include "hack_save_version.h"
#define HACK_BOX_FORMAT_X_VERSION 0x3B02
typedef char BoxFormatXNeedsVersion[(HACK_SAVE_VERSION >= HACK_BOX_FORMAT_X_VERSION) ? 1 : -1];
typedef char FramesFit[(SHELF_CACHE_OFF <= 0x8128) ? 1 : -1];
typedef char PageFits[(P0_NEW + 1 + NEW_PAGES <= SHELF_PAGE_SIZE && NEW_G_FIRST == EXT_G_FIRST + EXT_BOXES * IN_BOX_COUNT && NEW_PAGES * PAGE_MONS >= NEW_BOXES * IN_BOX_COUNT && NEW_META_OFF + NEW_META_SIZE <= SHELF_PAGE_SIZE && (NEW_PAGES - 1) * PAGE_MONS < NEW_BOXES * IN_BOX_COUNT && ALL_PAGES == 61 && NEW_PAGE_BASE + NEW_PAGES - 1 == NEW_PAGE_LAST && EXT2_PHYS_COUNT == NEW_PAGES * 2 && EXT2_PHYS_FIRST > OVFL_PHYS_FIRST + OVFL_PHYS_COUNT - 1 && P0_OVFL < SHELF_PAGE_SIZE && PAGE_MONS * MON_SIZE <= SHELF_PAGE_SIZE && EXT_G_FIRST == PAGE0_MONS + 15 * PAGE_MONS - 8 && EXT_PAGES * PAGE_MONS >= EXT_BOXES * IN_BOX_COUNT) ? 1 : -1];   //帯 788 匹のうち 780 匹（26 箱）を使う（余り 8 匹）
typedef char ExtMetaFits[(EXT_META_OFF + EXT_META_SIZE <= SHELF_PAGE_SIZE && P0_EXT + 1 + EXT_PAGES <= SHELF_PAGE_SIZE) ? 1 : -1];
typedef char OvflMetaFits[(OVFL_OFF + BOX_PAGES <= SHELF_PAGE_SIZE && P0_OVFL < SHELF_PAGE_SIZE && OVFL_PHYS_FIRST > EXT_PHYS_FIRST + EXT_PHYS_COUNT - 1) ? 1 : -1];

#define SHELF_MAP ((struct ShelfMap*) ((u8*) gSaveBlock1 + 0x1400))
#define REGION16_RAM ((const u32*) (REGION_RAM + SHELF_PAGE_SIZE))
#define SHELF_CACHE ((struct ShelfCache*) (STORAGE_BASE + SHELF_CACHE_OFF))
#define FRAME_PTR(f) ((struct CompressedPokemon*) (STORAGE_BASE + 4 + (f) * BOX_BYTES))

extern struct SaveSection gSaveDataBuffer;
#include "hack_save_crypt.h"
#define ReadFlash HackSaveCrypt_ReadFlash   //★ 2026-10-09 かき混ぜを戻して読む（src/hack_save_crypt.c）・もとは素の ReadFlash 081C2A54
#ifdef HACK_BOX_PROTO
u8 BoxShelf_TryWriteSector(u8 sector, u8* data);
#define SHELF_WRITE BoxShelf_TryWriteSector
#else
#define SHELF_WRITE TryWriteSector
#endif

static u32 FreePhysCount(void);

static void CopyBytes(u8* dst, const u8* src, u32 n)
{
	while (n-- != 0)
		*dst++ = *src++;
}

static void ZeroBytes(u8* dst, u32 n)
{
	while (n-- != 0)
		*dst++ = 0;
}

//棚の箱の番号（0〜49 = 箱の id）・箱でなければ -1（L 案・W 案: 全部の箱が棚の帯）
s32 BoxShelf_ShelfIndex(u8 boxId)
{
	return (boxId < NEW_BOX_FIRST + NEW_BOXES) ? boxId : -1;
}

//ポケモンの帯の g 番目 → (ページ, ページの中のバイト)
static void MonLocation(u32 g, u32* page, u32* off)
{
	if (g >= NEW_G_FIRST)
	{
		g -= NEW_G_FIRST;
		*page = NEW_PAGE_BASE + g / PAGE_MONS;
		*off = (g % PAGE_MONS) * MON_SIZE;
	}
	else if (g >= EXT_G_FIRST)
	{
		g -= EXT_G_FIRST;
		*page = EXT_PAGE_BASE + g / PAGE_MONS;
		*off = (g % PAGE_MONS) * MON_SIZE;
	}
	else if (g < PAGE0_MONS)
	{
		*page = 0;
		*off = P0_MONS + g * MON_SIZE;
	}
	else
	{
		g -= PAGE0_MONS;
		*page = 1 + g / PAGE_MONS;
		*off = (g % PAGE_MONS) * MON_SIZE;
	}
}

//ページ p の off から n バイト（追加の区画のページは使えるときだけ・使えなければ 0）
//追加の領域が使えるか（覚える・ShelfCache が無い間は毎回確かめる）
static bool8 ExtOk(void)
{
	struct ShelfCache* c = SHELF_CACHE;

	if (c->magic != SHELF_CACHE_MAGIC)
		return HackExtSave_Available();
	if (c->extOk == 0)
		c->extOk = HackExtSave_Available() ? 1 : 2;
	return c->extOk == 1;
}

//★ 2026-10-24 patches/0732: 熱い道（箱の 1 匹ずつ読む・空き探し）で HackExtSave_Available（バンクの確かめ＝約 34 回のフラッシュ読み）を毎回呼ばない。
//  「使える」と分かったら入れ物のキャッシュに覚える（使える版は途中で使えなくならない）。「使えない」は覚えない（最初のレポートの前は決められず、あとで使えるようになる）
static bool8 ExtAvail(void)
{
	struct ShelfCache* c = SHELF_CACHE;

	if (c->magic == SHELF_CACHE_MAGIC && c->extOk == 1)
		return TRUE;
	if (!HackExtSave_Available())
		return FALSE;
	if (c->magic == SHELF_CACHE_MAGIC)
		c->extOk = 1;
	return TRUE;
}

//他のファイルの熱い道（ボックスジャンプの箱の数）用: 答えは HackExtSave_Available と同じ、「使える」は覚える
bool8 BoxShelf_ExtAvailable(void)
{
	return ExtAvail();
}

static void ReadPage(u32 p, u32 off, u8* dst, u32 n)
{
	u8 phys = SHELF_CACHE->committed[p];

	//★ 2026-10-24 patches/0740: ExtAvail が TRUE（使える・覚えてある）と分かったあとは、HackExtSave_Read の中の確かめ（約 34 回のフラッシュ読み）を繰り返さない
	//  （前は 1 匹読むたびに確かめていた＝追加の領域の箱を 1 匹ずつ読む道［ボックスジャンプの 5 箱の見本・前からいた子の一覧づくり・箱の中身の確かめ］で 1 匹約 0.1 フレーム）。読んだ中身は同じ
	if (p < BOX_PAGES && SHELF_CACHE->ovfl[p] != NO_PAGE) //追加の区画に新しい写し（読み込みのとき使える版だけ入れる）
	{
		if (!ExtAvail() || !HackExtSave_ReadKnownAvail(SHELF_CACHE->ovfl[p], off, dst, n))
			ZeroBytes(dst, n);
		return;
	}
	if (phys == NO_PAGE || (IS_EXT_PAGE(p) && !ExtAvail()))
		ZeroBytes(dst, n);
	else if (IS_EXT_PAGE(p))
		HackExtSave_ReadKnownAvail(phys, off, dst, n);
	else
		ReadFlash(phys, off, dst, n);
}

static void ReadMonsFromFlash(u32 g, u32 count, u8* dst)
{
	while (count != 0)
	{
		u32 page, off, n;
		MonLocation(g, &page, &off);
		n = ((page == 0) ? P0_MONS + PAGE0_MONS * MON_SIZE - off : PAGE_MONS * MON_SIZE - off) / MON_SIZE;
		if (n > count)
			n = count;
		ReadPage(page, off, dst, n * MON_SIZE);
		dst += n * MON_SIZE;
		g += n;
		count -= n;
	}
}

static s32 FindFrame(u8 shelf)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 f;
	for (f = 0; f < SHELF_FRAMES; ++f)
	{
		if (c->frameBox[f] == shelf)
			return f;
	}
	return -1;
}

//入れ物に読み込む（無ければ）→ 入れ物の番号
static u32 LoadFrame(u8 shelf)
{
	struct ShelfCache* c = SHELF_CACHE;
	s32 f = FindFrame(shelf);
	s32 pin = BoxShelf_ShelfIndex(StorageGetCurrentBox());
	u32 i;

	if (f < 0)
	{
		s32 best = -1;
		for (i = 0; i < SHELF_FRAMES; ++i)
		{
			if (c->frameBox[i] == NO_BOX)
			{
				best = i;
				break;
			}
			if (c->frameDirty[i] || c->frameBox[i] == pin)
				continue;
			if (best < 0 || (u8) (c->stamp - c->frameStamp[i]) > (u8) (c->stamp - c->frameStamp[best]))
				best = i;
		}
		if (best < 0)
		{
			//★ 2026-10-24 patches/0732: 汚れていない入れ物が無い＝今の箱（pin）だけが汚れていない、のときは今の箱の入れ物を譲る（汚れた中身を捨てるよりまし）。
			//  全部汚れているのは BoxShelf_CanTouchBox の上限（SHELF_MAX_DIRTY）が止めるので起きないはず（起きたら overBudget の印）
			for (i = 0; i < SHELF_FRAMES; ++i)
			{
				if (c->frameDirty[i])
					continue;
				if (best < 0 || (u8) (c->stamp - c->frameStamp[i]) > (u8) (c->stamp - c->frameStamp[best]))
					best = i;
			}
			if (best < 0)
			{
				c->overBudget = TRUE;
				best = 0;
			}
		}
		f = best;
		c->frameBox[f] = shelf;
		c->frameDirty[f] = FALSE;
		ReadMonsFromFlash(shelf * IN_BOX_COUNT, IN_BOX_COUNT, (u8*) FRAME_PTR(f));
	}
	c->frameStamp[f] = ++c->stamp;
	return f;
}

//箱の 30 匹の先頭（続けて並ぶ）。forWrite なら汚れの印
struct CompressedPokemon* BoxShelf_BoxMons(u8 boxId, bool8 forWrite)
{
	s32 shelf = BoxShelf_ShelfIndex(boxId);
	u32 f;

	if (shelf < 0)
		return NULL;
	f = LoadFrame(shelf);
	if (forWrite && !SHELF_CACHE->oldFormat && (shelf < EXT_SHELF_FIRST || ExtAvail()))   //35〜50 箱は追加の領域が無ければ書けない（汚れを残さない）・古い形は書かない
		SHELF_CACHE->frameDirty[f] = TRUE;
	if (forWrite)
	{
		SHELF_CACHE->writeEpoch++;   //★ FEAT-box100-unlock 51〜100 箱を出すかの数え直しの目印
		if (shelf >= NEW_BOX_FIRST && SHELF_CACHE->visState != 2)
			SHELF_CACHE->visState = 2;   //51 箱目以降に書く＝もう出ている
	}
	return FRAME_PTR(f);
}

//★ 2026-10-09 #70: 入れ物にもう読み込んである箱だけ（読み込まない・入れ物を動かさない）。無ければ NULL・棚でない箱も NULL
struct CompressedPokemon* BoxShelf_PeekBoxMons(u8 boxId)
{
	s32 shelf = BoxShelf_ShelfIndex(boxId);
	s32 f = (shelf >= 0) ? FindFrame(shelf) : -1;
	return (f >= 0) ? FRAME_PTR(f) : NULL;
}

//1 匹を読む（入れ物に無ければフラッシュから直に・入れ物を動かさない）
void BoxShelf_ReadMon(u8 boxId, u8 pos, struct CompressedPokemon* out)
{
	s32 shelf = BoxShelf_ShelfIndex(boxId);
	s32 f;

	if (shelf < 0)
		return;
	f = FindFrame(shelf);
	if (f >= 0)
		CopyBytes((u8*) out, (u8*) &FRAME_PTR(f)[pos], MON_SIZE);
	else
		ReadMonsFromFlash(shelf * IN_BOX_COUNT + pos, 1, (u8*) out);
}

//★ 0.63.1 Moémon の世代の控え（src/hack_moemon_gen_owners.c）: 箱の first 匹目から n 匹を続けて読む（入れ物にあればそこから・無ければフラッシュから 1 回で・入れ物を動かさない）。棚でない箱・範囲外は FALSE
bool8 BoxShelf_ReadBoxMons(u8 boxId, u8 first, u8 n, struct CompressedPokemon* out)
{
	s32 shelf = BoxShelf_ShelfIndex(boxId), f;

	if (shelf < 0 || first >= IN_BOX_COUNT || n > IN_BOX_COUNT - first)
		return FALSE;
	f = FindFrame(shelf);
	if (f >= 0)
		CopyBytes((u8*) out, (u8*) &FRAME_PTR(f)[first], n * MON_SIZE);
	else
		ReadMonsFromFlash(shelf * IN_BOX_COUNT + first, n, (u8*) out);
	return TRUE;
}

//★ 2026-10-24 patches/0732: 箱の最初の空き（種族 0）の位置・無ければ -1。1 匹ずつ GetBoxMonDataAt で読むと、フラッシュから読む箱は 1 匹ごとに
//  追加の領域の確かめ＋読みで約 150 フレームかかった（約 1000 匹の箱で捕まえた子の箱探し）→ 入れ物にあればそこから・無ければ 6 匹ずつ読む（入れ物を動かさない）
#define EMPTY_CHUNK 6
s32 BoxShelf_FindEmptyPos(u8 boxId)
{
	s32 shelf = BoxShelf_ShelfIndex(boxId), f;
	u32 pos, k;
	struct CompressedPokemon buf[EMPTY_CHUNK];

	if (shelf < 0)
		return -1;
	f = FindFrame(shelf);
	if (f >= 0)
	{
		for (pos = 0; pos < IN_BOX_COUNT; ++pos)
		{
			if (FRAME_PTR(f)[pos].substruct0.species == SPECIES_NONE)
				return pos;
		}
		return -1;
	}
	for (pos = 0; pos < IN_BOX_COUNT; pos += EMPTY_CHUNK)
	{
		u32 n = (IN_BOX_COUNT - pos < EMPTY_CHUNK) ? IN_BOX_COUNT - pos : EMPTY_CHUNK;

		ReadMonsFromFlash(shelf * IN_BOX_COUNT + pos, n, (u8*) buf);
		for (k = 0; k < n; ++k)
		{
			if (buf[k].substruct0.species == SPECIES_NONE)
				return pos + k;
		}
	}
	return -1;
}

//入れ物の中を直に書いたとき（ステータス画面のわざの並べ替え）
void BoxShelf_MarkPtrDirty(const void* ptr)
{
	u32 a = (u32) ptr;
	u32 f;

	if (a < STORAGE_BASE + 4 || a >= STORAGE_BASE + 4 + SHELF_FRAMES * BOX_BYTES)
		return;
	f = (a - STORAGE_BASE - 4) / BOX_BYTES;
	if (SHELF_CACHE->frameBox[f] != NO_BOX)
		SHELF_CACHE->frameDirty[f] = TRUE;
	SHELF_CACHE->writeEpoch++;
}

//★ FEAT-box100-unlock（0.61.0・ユーザー決定 2026-10-27）: 1〜50 箱目がポケモンで全部埋まるまで 51〜100 箱目は出さない（箱の切り替え・箱を選ぶ画面・預ける先は 50 箱まで）。
//  出す条件: 51 箱目以降にポケモンを置いたことがある（保存済みの新ページが在る・今の箱が 51 以降・入れ物に 51 以降の箱が載っている）か、1〜50 が満杯。
//  一度出たら（このセッションでは）隠さない。数えるのは箱 50 → 1 の順（途中で空きのある箱が見つかれば止める）・箱の中身を書くまで（writeEpoch）数え直さない。
//  旗・保存する値は無い（毎回の起動で保存済みのページから決め直す）。
u8 BoxShelf_VisibleBoxes(void)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 i;
	s32 box;

	if (c->magic != SHELF_CACHE_MAGIC)
		return NEW_BOX_FIRST;
	if (c->visState == 2)
		return TOTAL_BOXES_COUNT;
	if (StorageGetCurrentBox() >= NEW_BOX_FIRST)
		goto show;
	for (i = NEW_PAGE_BASE; i < ALL_PAGES; ++i)
	{
		if (c->committed[i] != NO_PAGE || c->extPending[i - EXT_PAGE_BASE] != NO_PAGE)
			goto show;
	}
	for (i = 0; i < SHELF_FRAMES; ++i)
	{
		if (c->frameBox[i] != NO_BOX && c->frameBox[i] >= NEW_BOX_FIRST && c->frameDirty[i])
			goto show;
	}
	if (c->visState == 1 && c->visEpoch == c->writeEpoch)
		return NEW_BOX_FIRST;
	if (c->oldFormat)
		return NEW_BOX_FIRST;
	for (box = NEW_BOX_FIRST - 1; box >= 0; --box)
	{
		if (BoxShelf_FindEmptyPos(box) >= 0)
		{
			c->visState = 1;
			c->visEpoch = c->writeEpoch;
			return NEW_BOX_FIRST;
		}
	}
show:
	c->visState = 2;
	return TOTAL_BOXES_COUNT;
}

//次の箱（折り返しは見えている箱の数・今の箱が見えている範囲の外なら折り返さない）
u8 BoxShelf_WrapNextBox(u8 box)
{
	return (box >= BoxShelf_VisibleBoxes()) ? 0 : box;
}

static void ResetCache(void)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 i;

	ZeroBytes((u8*) c, sizeof(*c));
	c->magic = SHELF_CACHE_MAGIC;
	for (i = 0; i < SHELF_FRAMES; ++i)
		c->frameBox[i] = NO_BOX;
	for (i = 0; i < ALL_PAGES; ++i)
		c->committed[i] = NO_PAGE;
	for (i = 0; i < ALL_EXT_PAGES; ++i)
		c->extPending[i] = NO_PAGE;
	for (i = 0; i < BOX_PAGES; ++i)
	{
		c->ovfl[i] = NO_PAGE;
		c->ovflPending[i] = NO_PAGE;
	}
}

//古い形（'SHEL'・'SHL2'）を続きからで見たか（C は読まない・止める画面用）
bool8 BoxShelf_IsOldFormat(void)
{
	return SHELF_CACHE->magic == SHELF_CACHE_MAGIC && SHELF_CACHE->oldFormat;
}

//はじめから（src/save.c の NewGameWipeNewSaveData）: 棚は空（対応表なし）
void BoxShelf_NewGame(void)
{
	struct ShelfMap* m = SHELF_MAP;
	u32 i;

	ResetCache();
	m->magic = SHELF_MAP_MAGIC;
	m->seq = 0;
	for (i = 0; i < SHELF_PAGES; ++i)
		m->phys[i] = NO_PAGE;
}

//26〜39 箱の名前が空（終わりの印 0xFF が無い）なら「ボックスNN」・壁紙は素と同じ（番号 % 4）。
//はじめからは素の ResetPokemonStorageSystem（CFRU が箱の数を書き換え）が全部の箱に付けるので、要るのは引っ越したセーブだけ。
//★ 名前に終わりの印が無いと、ボックスの画面で名前を描くときに後ろまで読んで、画面が操作を受け付けなくなった（2026-10-06 mGBA）
#define gText_Box ((const u8*) 0x083DF249)
static void FixNewBoxNames(void)
{
	u32 i, k;
	for (i = 0; i < 14 + EXT_META_BOXES; ++i)   //26〜39 箱・40〜50 箱
	{
		u8* name = (i < 14) ? (u8*) STORAGE_BASE + 0x8134 + 9 * i : EXT_NAMES_RAM + 9 * (i - 14);
		u8* wall = (i < 14) ? (u8*) STORAGE_BASE + 0x81B2 + i : EXT_WALLS_RAM + (i - 14);
		bool8 ok = FALSE;
		for (k = 0; k < 9; ++k)
		{
			if (name[k] == 0xFF)
				ok = TRUE;
		}
		if (!ok)
		{
			u8* end = StringCopy(name, gText_Box);
			ConvertIntToDecimalStringN(end, 26 + i, STR_CONV_MODE_LEFT_ALIGN, 2);
		}
		if (*wall >= 19)   //★ 壁紙は 18 番まで（17 = もも・18 = オリポケーズ・src/hack_box_wallpaper.c）
			*wall = (25 + i) % 4;
	}
	for (i = 0; i < NEW_BOXES; ++i)   //★ FEAT-box100: 51〜100 箱（箱 id 50〜99）。名前の既定は「ボックス51」〜「ボックス100」（3 桁）・壁紙は 番号 % 4
	{
		u8* name = NEW_NAMES_RAM + 9 * i;
		u8* wall = NEW_WALLS_RAM + i;
		bool8 ok = FALSE;
		for (k = 0; k < 9; ++k)
		{
			if (name[k] == 0xFF)
				ok = TRUE;
		}
		if (!ok)
		{
			u8* end = StringCopy(name, gText_Box);
			ConvertIntToDecimalStringN(end, NEW_BOX_FIRST + 1 + i, STR_CONV_MODE_LEFT_ALIGN, (NEW_BOX_FIRST + 1 + i >= 100) ? 3 : 2);
			*wall = (NEW_BOX_FIRST + i) % 4;
		}
		if (*wall >= 19)
			*wall = (NEW_BOX_FIRST + i) % 4;
	}
}

static u32 Region16Sum(void)
{
	const u32* w = REGION16_RAM;
	u32 i, s = 0;
	for (i = 0; i < SHELF_PAGE_SIZE / 4; ++i)
		s += w[i] * (i + 1); //並べ替えにも気づく重み付きの和
	return s;
}

//ext = 追加の区画か。counter があれば、ページの印の counter（どちらの写しが新しいかの比べ）を返す
static bool8 PageValidC(bool8 ext, u8 phys, u32 page, u32* counter)
{
	struct SaveSection* s = &gSaveDataBuffer;

	bool8 valid;

	if (ext)
	{
		if (!HackExtSave_ReadSectorChecked(phys, s)) //★ 2026-10-21 改造の検出つきの読み
			return FALSE;
	}
	else
		HackSaveCrypt_ReadFlashChecked(phys, s);
	if (counter != NULL)
		*counter = s->counter;
	valid = s->id == SHELF_FOOTER_ID + page && s->signature == SHELF_SIGNATURE
		&& s->checksum == CalculateSaveChecksum(s->data, SHELF_PAGE_SIZE);
	if (valid && SHELF_CACHE->magic == SHELF_CACHE_MAGIC) //★ 読めたページだけ数える（壊れたページは今までどおり空として扱う）
	{
		if (HackSaveCrypt_IsTampered(s))
			SHELF_CACHE->tamper = 1;
		if (HackSaveCrypt_IsUnsigned(s) && page < 32)
		{
			SHELF_CACHE->legacyMask |= 1u << page;
			SHELF_CACHE->scrubOk = 1;
		}
	}
	return valid;
}

//続きからで読んだ棚のページに MAC が合わないものがあったか（src/save.c の HandleLoadSector が BoxShelf_OnLoad のあとで見る）
bool8 BoxShelf_TamperSeen(void)
{
	return SHELF_CACHE->magic == SHELF_CACHE_MAGIC && SHELF_CACHE->tamper;
}

static bool8 PageValid(u8 phys, u32 page)
{
	return PageValidC(IS_EXT_PAGE(page), phys, page, NULL);
}

//追加の区画のページ page の物理区画 p が、そのページの置き場（既存の 14 ページは 128〜155・新しい 29 ページは EXT2 の 186〜243）の中か
static bool8 ExtPhysInPool(u32 page, u8 p)
{
	if (IS_NEW_PAGE(page))
		return p >= EXT2_PHYS_FIRST && p < EXT2_PHYS_FIRST + EXT2_PHYS_COUNT;
	return p >= EXT_PHYS_FIRST && p < EXT_PHYS_FIRST + EXT_PHYS_COUNT;
}

//続きから（src/save.c の HandleLoadSector・本体の組を読んだあと）
void BoxShelf_OnLoad(void)
{
	struct ShelfMap* m = SHELF_MAP;
	struct ShelfCache* c;
	u32 i;
	u32 mainCtr[BOX_PAGES];

	if (m->magic == SHELF_MAP_MAGIC_OLD || m->magic == SHELF_MAP_MAGIC_L || m->magic == SHELF_MAP_MAGIC_W)
	{
		//★ X 案: W 案の形（63 バイト 'SHL3'）も読まない（セーブデータ移行ツールで X にする）。★ L 案・W 案: 古い形（58 バイト 'SHEL'）と L 案の形（60 バイト 'SHL2'）の棚は読まない。対応表も RAM の印も書き換えない（NewGame を呼ぶと印が 'SHL3' に変わり、見分けが消えて書き壊す）。
		//  入れ物は空・レポートは BoxShelf_Commit が何も書かない。止める画面は src/bw_main_menu.c（BoxShelf_IsOldFormat）
		ResetCache();
		SHELF_CACHE->oldFormat = TRUE;
		return;
	}
	if (m->magic != SHELF_MAP_MAGIC)
	{
		BoxShelf_NewGame(); //棚を使う前のセーブ（引っ越していない）: 棚は空
		return;
	}
	ResetCache();
	c = SHELF_CACHE;
	for (i = 0; i < BOX_PAGES; ++i)
		mainCtr[i] = 0;
	for (i = 0; i < SHELF_PAGES; ++i)
	{
		u8 p = m->phys[i];
		if (p != NO_PAGE && (p < SHELF_PHYS_FIRST || p >= SHELF_PHYS_FIRST + SHELF_PHYS_COUNT
		 || !PageValidC(FALSE, p, i, (i < BOX_PAGES) ? &mainCtr[i] : NULL)))
			p = NO_PAGE; //壊れたページ（起きないはず）: 空として扱う
		c->committed[i] = m->phys[i] = p;
	}

	if (c->committed[0] != NO_PAGE) //ページ 0 → 今の箱の番号・名前と壁紙・parasite 3
	{
		u8* d = gSaveDataBuffer.data;
		DoReadFlashWholeSection(c->committed[0], &gSaveDataBuffer);
		CopyBytes((u8*) STORAGE_BASE, d + P0_HEAD, 4);
		CopyBytes((u8*) STORAGE_BASE + 0x8128, d + P0_TAIL_A, 0x98);
		CopyBytes((u8*) STORAGE_BASE + 0x82E1, d + P0_TAIL_B, 0xEF);
		CopyBytes((u8*) PARASITE3_RAM, d + P0_PARASITE, PARASITE3_SIZE);
		c->ovflUsed = (d[P0_OVFL] == OVFL_FLAG);
		if (d[P0_EXT] == EXT_MAP_MAGIC) //40〜50 箱の対応表（使えない版では確かめずにそのまま残す）
		{
			for (i = 0; i < EXT_PAGES; ++i)
				c->committed[EXT_PAGE_BASE + i] = d[P0_EXT + 1 + i];
		}
		if (d[P0_NEW] == NEW_MAP_MAGIC) //51〜100 箱の対応表（0.61.0〜・無ければ全部空）
		{
			for (i = 0; i < NEW_PAGES; ++i)
				c->committed[NEW_PAGE_BASE + i] = d[P0_NEW + 1 + i];
		}
	}
	ZeroBytes(EXT_NAMES_RAM, EXT_META_SIZE);
	ZeroBytes(NEW_NAMES_RAM, NEW_META_SIZE);
	if (HackExtSave_Available())
	{
		for (i = EXT_PAGE_BASE; i < ALL_PAGES; ++i)
		{
			u8 p = c->committed[i];
			if (p != NO_PAGE && (!ExtPhysInPool(i, p) || !PageValid(p, i)))
				c->committed[i] = NO_PAGE; //壊れたページ: 空として扱う
		}
		ReadPage(EXT_PAGE_LAST, EXT_META_OFF, EXT_NAMES_RAM, EXT_META_SIZE);
		ReadPage(NEW_PAGE_LAST, NEW_META_OFF, NEW_NAMES_RAM, NEW_META_SIZE);
		{
			u8 map[BOX_PAGES];   //ページ 1〜15 の追加の区画の写し（本体の写しより新しいものだけ採る）
			ReadPage(EXT_PAGE_LAST, OVFL_OFF, map, BOX_PAGES);
			if (map[0] == OVFL_MAGIC)
			{
				for (i = OVFL_FIRST_PAGE; i < BOX_PAGES; ++i)
				{
					u8 q = map[i];
					u32 oc = 0;
					if (q < OVFL_PHYS_FIRST || q >= OVFL_PHYS_FIRST + OVFL_PHYS_COUNT || !PageValidC(TRUE, q, i, &oc))
						continue;
					if (c->committed[i] != NO_PAGE && mainCtr[i] >= oc)
						continue; //本体のほうが新しい（追加の領域の無い版で書いた）
					c->ovfl[i] = q;
				}
			}
		}
	}
	for (i = BOX_PAGES; i < SHELF_PAGES; ++i) //ページ 16・17 → もとの区画 30・31 の RAM
	{
		if (c->committed[i] != NO_PAGE)
			ReadFlash(c->committed[i], 0, (u8*) REGION_RAM + (i - BOX_PAGES) * SHELF_PAGE_SIZE, SHELF_PAGE_SIZE);
		else
			ZeroBytes((u8*) REGION_RAM + (i - BOX_PAGES) * SHELF_PAGE_SIZE, SHELF_PAGE_SIZE); //書いたことのないページ = 0
	}
	c->sum16 = Region16Sum();
	FixNewBoxNames();
}

static void PageRange(u32 p, u32* gStart, u32* gEnd)
{
	if (IS_NEW_PAGE(p))
	{
		*gStart = NEW_G_FIRST + (p - NEW_PAGE_BASE) * PAGE_MONS;
		*gEnd = *gStart + PAGE_MONS;
		if (*gEnd > NEW_G_FIRST + NEW_BOXES * IN_BOX_COUNT)
			*gEnd = NEW_G_FIRST + NEW_BOXES * IN_BOX_COUNT;
	}
	else if (IS_EXT_PAGE(p))
	{
		*gStart = EXT_G_FIRST + (p - EXT_PAGE_BASE) * PAGE_MONS;
		*gEnd = *gStart + PAGE_MONS;
		if (*gEnd > NEW_G_FIRST)   //★ FEAT-box100: 論理ページ 31 は 1,500 匹目（51 箱目）までの 44 匹。頭打ちしないと 1,500〜1,507 が MonLocation で新ページの先頭に行き、ページ 31 の画像に重なる
			*gEnd = NEW_G_FIRST;
	}
	else if (p == 0)
	{
		*gStart = 0;
		*gEnd = PAGE0_MONS;
	}
	else
	{
		*gStart = PAGE0_MONS + (p - 1) * PAGE_MONS;
		*gEnd = *gStart + PAGE_MONS;
		//★ ページ 15 は 963〜1030 になるが、1,020〜（35 箱目から）は追加の区画のページ（MonLocation）。
		//  頭打ちしないと 35 箱の 1〜11 匹目がページ 15 の後ろ（帯の余り）に書かれる
		if (*gEnd > EXT_G_FIRST)
			*gEnd = EXT_G_FIRST;
	}
}

//ページ p を gSaveDataBuffer に組み立てる（今のフラッシュ ＋ 入れ物の箱 ＋ ページ 0 の中身）
static void BuildPage(u32 p)
{
	struct ShelfCache* c = SHELF_CACHE;
	u8* d = gSaveDataBuffer.data;
	u32 f, gStart, gEnd;

	Memset(&gSaveDataBuffer, 0, sizeof(gSaveDataBuffer));
	if (p >= BOX_PAGES && !IS_EXT_PAGE(p)) //ページ 16・17 = もとの区画 30・31 の RAM をそのまま（18〜25 は 35〜50 箱）
	{
		CopyBytes(d, (u8*) REGION_RAM + (p - BOX_PAGES) * SHELF_PAGE_SIZE, SHELF_PAGE_SIZE);
		goto FOOTER;
	}
	ReadPage(p, 0, d, SHELF_PAGE_SIZE);
	PageRange(p, &gStart, &gEnd);
	for (f = 0; f < SHELF_FRAMES; ++f)
	{
		u32 b = c->frameBox[f], g, page, off;
		if (b == NO_BOX)
			continue;
		for (g = b * IN_BOX_COUNT; g < (b + 1) * IN_BOX_COUNT; ++g)
		{
			if (g < gStart || g >= gEnd)
				continue;
			MonLocation(g, &page, &off);
			CopyBytes(d + off, (u8*) &FRAME_PTR(f)[g - b * IN_BOX_COUNT], MON_SIZE);
		}
	}
	if (p == 0)
	{
		CopyBytes(d + P0_HEAD, (u8*) STORAGE_BASE, 4);
		CopyBytes(d + P0_TAIL_A, (u8*) STORAGE_BASE + 0x8128, 0x98);
		CopyBytes(d + P0_TAIL_B, (u8*) STORAGE_BASE + 0x82E1, 0xEF);
		CopyBytes(d + P0_PARASITE, (u8*) PARASITE3_RAM, PARASITE3_SIZE);
		d[P0_EXT] = EXT_MAP_MAGIC;
		CopyBytes(d + P0_EXT + 1, c->extPending, EXT_PAGES);
		d[P0_NEW] = NEW_MAP_MAGIC;
		CopyBytes(d + P0_NEW + 1, c->extPending + EXT_PAGES, NEW_PAGES);
		d[P0_OVFL] = 0;
		for (f = 0; f < BOX_PAGES; ++f)
		{
			if (c->ovflPending[f] != NO_PAGE)
				d[P0_OVFL] = OVFL_FLAG;
		}
		if (c->ovflUsed && !ExtOk()) //追加の領域が無い版: 印をそのまま残す（古い写しがあるかもしれない）
			d[P0_OVFL] = OVFL_FLAG;
	}
	if (p == NEW_PAGE_LAST)
		CopyBytes(d + NEW_META_OFF, NEW_NAMES_RAM, NEW_META_SIZE);
	if (p == EXT_PAGE_LAST)
	{
		CopyBytes(d + EXT_META_OFF, EXT_NAMES_RAM, EXT_META_SIZE);
		for (f = 0; f < BOX_PAGES; ++f)
			d[OVFL_OFF + f] = (f == 0) ? OVFL_MAGIC : c->ovflPending[f];
	}
FOOTER:
	gSaveDataBuffer.id = SHELF_FOOTER_ID + p;
	gSaveDataBuffer.signature = SHELF_SIGNATURE;
	gSaveDataBuffer.counter = gSaveCounter;
	gSaveDataBuffer.checksum = CalculateSaveChecksum(d, SHELF_PAGE_SIZE);
}

static bool8 PageDirty(u32 p)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 f, gStart, gEnd;

	if (IS_EXT_PAGE(p))
	{
		if (!ExtAvail())
			return FALSE;
		if (p == EXT_PAGE_LAST || p == NEW_PAGE_LAST)
			return TRUE; //40〜50 箱・51〜100 箱の名前と壁紙（同じなら書かない）
	}
	else if (p == 0 || p >= BOX_PAGES)
		return TRUE; //ページ 0（変数）・16・17（バッグなど）: 中身をフラッシュと比べるのは BoxShelf_Commit（同じなら書かない）
	PageRange(p, &gStart, &gEnd);
	for (f = 0; f < SHELF_FRAMES; ++f)
	{
		u32 b = c->frameBox[f];
		if (b != NO_BOX && c->frameDirty[f] && b * IN_BOX_COUNT < gEnd && (b + 1) * IN_BOX_COUNT > gStart)
			return TRUE;
	}
	return FALSE;
}

//汚れたページの数（ページ 0 を入れる）
u32 BoxShelf_DirtyPageCount(void)
{
	u32 p, n = 0;
	for (p = 0; p < SHELF_PAGES; ++p)
		n += PageDirty(p);
	return n;
}

//ページ p を書こうとしている中身が、今のフラッシュと同じか（同じなら書かない＝予備を使わない）
static bool8 SameAsFlash(u32 p, const u8* data)
{
	u8 tmp[64];
	u32 off, i;

	if (SHELF_CACHE->committed[p] == NO_PAGE) //書いたことのないページ（読むと 0）: 0 だけなら書かない
	{
		for (i = 0; i < SHELF_PAGE_SIZE; ++i)
		{
			if (data[i] != 0)
				return FALSE;
		}
		return TRUE;
	}
	for (off = 0; off < SHELF_PAGE_SIZE; off += sizeof(tmp))
	{
		u32 n = (SHELF_PAGE_SIZE - off < sizeof(tmp)) ? SHELF_PAGE_SIZE - off : sizeof(tmp);
		ReadPage(p, off, tmp, n);
		for (i = 0; i < n; ++i)
		{
			if (tmp[i] != data[off + i])
				return FALSE;
		}
	}
	return TRUE;
}

static s32 TakeFreePhys(void)
{
	struct ShelfCache* c = SHELF_CACHE;
	struct ShelfMap* m = SHELF_MAP;
	u32 k, i;

	for (k = 0; k < SHELF_PHYS_COUNT; ++k)
	{
		u8 phys = SHELF_PHYS_FIRST + (c->cursor + k) % SHELF_PHYS_COUNT;
		bool8 used = FALSE;
		for (i = 0; i < SHELF_PAGES; ++i)
		{
			if (c->committed[i] == phys || m->phys[i] == phys)
				used = TRUE;
		}
		if (!used)
		{
			c->cursor = (c->cursor + k + 1) % SHELF_PHYS_COUNT;
			return phys;
		}
	}
	return -1;
}

//箱のページ用の追加の区画（今使っている写しと、このレポートで決めた写しは避ける）
static s32 TakeFreeOvfl(void)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 k, i;

	for (k = 0; k < OVFL_PHYS_COUNT; ++k)
	{
		u8 phys = OVFL_PHYS_FIRST + (c->stamp + c->cursor + k) % OVFL_PHYS_COUNT;
		bool8 used = FALSE;
		for (i = OVFL_FIRST_PAGE; i < BOX_PAGES; ++i)
		{
			if (c->ovfl[i] == phys || c->ovflPending[i] == phys)
				used = TRUE;
		}
		if (!used)
			return phys;
	}
	return -1;
}

static s32 TakeFreeExt(u32 page)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 k, i;
	u32 first = IS_NEW_PAGE(page) ? EXT2_PHYS_FIRST : EXT_PHYS_FIRST;   //★ FEAT-box100: 新ページは別の置き場（186〜243）
	u32 count = IS_NEW_PAGE(page) ? EXT2_PHYS_COUNT : EXT_PHYS_COUNT;

	for (k = 0; k < count; ++k)
	{
		u8 phys = first + (c->stamp + k) % count;
		bool8 used = FALSE;
		for (i = 0; i < ALL_EXT_PAGES; ++i)
		{
			if (c->committed[EXT_PAGE_BASE + i] == phys || c->extPending[i] == phys)
				used = TRUE;
		}
		if (!used)
			return phys;
	}
	return -1;
}

//★ 2026-10-16 ぼうけんのしょ 2・40〜50 箱の「使えるか」の確かめ（src/hack_ext_save.c の HackExtSave_Check）は、本体のバンク 0 の s 番と
//  バンク 1 の s 番がどちらも消えていないことを比べる相手にする。バンク 1（区画 16〜31）は棚の後ろのほうで、はじめからの最初のレポートではまだ誰も使わない
//  （最初のレポートで書く棚のページはページ 0 の 1 枚＝区画 10 だけ）。そこで s = 10 の組（区画 10 と 26）が消えたままなら、
//  どのページも指していない（＝いつ上書きされてもよい）空の区画だけに、空のページ（ページ 17 の形・counter 0＝修復の道具が選ばない）を 1 度だけ書く。
//  消えたままの区画（頭が 0xFF）は誰も指していない・書くのは本体の 128KB の中だけ・64MB の後ろ半分が見えるときだけ。次からは頭が埋まっているので書かない
#define PROBE_FILL_A 10
#define PROBE_FILL_B 26

static void EnsureProbeSectors(void)
{
	static const u8 fill[2] = {PROBE_FILL_A, PROBE_FILL_B};
	struct ShelfCache* c = SHELF_CACHE;
	struct ShelfMap* m = SHELF_MAP;
	u32 k, i;

	if (!HackExtSave_BackHalfVisible())
		return;
	for (k = 0; k < 2; ++k)
	{
		u8 phys = fill[k];
		bool8 used = FALSE;

		for (i = 0; i < SHELF_PAGES; ++i)
		{
			if (c->committed[i] == phys || m->phys[i] == phys)
				used = TRUE;
		}
		if (used || !HackExtSave_BaseHeadErased(phys))
			continue;
		Memset(&gSaveDataBuffer, 0, sizeof(gSaveDataBuffer));
		gSaveDataBuffer.id = SHELF_FOOTER_ID + SHELF_PAGES - 1;
		gSaveDataBuffer.signature = SHELF_SIGNATURE;
		gSaveDataBuffer.counter = 0;
		gSaveDataBuffer.checksum = CalculateSaveChecksum(gSaveDataBuffer.data, SHELF_PAGE_SIZE);
		SHELF_WRITE(phys, gSaveDataBuffer.data);
	}
}

//署名のない（前の版の）ページの印（legacyMask は u32＝論理ページ 0〜31 だけ。新ページ 32〜60 は 0.61.0 で作った＝署名なしの前の版が無いので いつも 0）
static bool8 LegacyBit(u32 p)
{
	return p < 32 && (SHELF_CACHE->legacyMask & (1u << p)) != 0;
}

//レポート（src/save.c の SaveWriteToFlash・gSaveCounter を進めたあと・本体の組を書く前）
void BoxShelf_Commit(void)
{
	struct ShelfCache* c = SHELF_CACHE;
	struct ShelfMap* m = SHELF_MAP;
	u32 p;

	if (c->magic == SHELF_CACHE_MAGIC && c->oldFormat)
		return; //古い形のセーブは棚を何も書かない（本体の組の中の対応表 'SHEL' もそのまま）
	if (c->magic != SHELF_CACHE_MAGIC)
	{
		BoxShelf_OnLoad();
		if (c->oldFormat)
			return;
	}
	else if (m->magic != SHELF_MAP_MAGIC)
	{
		//★ 2026-10-09 ユーザー「ボックスにあずけるとレポート書いて再開とするとボックスが消失する」: はじめからは
		//  BoxShelf_NewGame（NewGameInitData の頭のフック）のあとに素の ClearSav1 が SaveBlock1 を 0 で消し、対応表の印も消える。
		//  そこで OnLoad → NewGame を呼ぶと入れ物（預けた箱）まで捨てるので、入れ物は残して印だけ付け直す（対応表は下で committed から）
		m->magic = SHELF_MAP_MAGIC;
		m->seq = 0;
	}
	c->overBudget = FALSE;
	c->lateMask = 0;
	for (p = 0; p < SHELF_PAGES; ++p)
		m->phys[p] = c->committed[p];
	for (p = 0; p < ALL_PAGES; ++p)
		c->written[p] = FALSE;
	for (p = 0; p < ALL_EXT_PAGES; ++p)
		c->extPending[p] = c->committed[EXT_PAGE_BASE + p];
	for (p = 0; p < BOX_PAGES; ++p)
		c->ovflPending[p] = c->ovfl[p];
	c->pendingSum16 = Region16Sum();

	//ページ 0・16・17 は本体の区画に書く（ページ 0 は最後）→ 先にその分の予備を取っておく。
	//箱のページ（1〜15）: 予備があれば本体の新しい区画（今までと同じ・そのまま新しい）。無ければ、追加の区画に新しい写しを書いて（これが本物の控え）、
	//  レポートのあと（AfterSave）に本体の今の区画へ上書きして追いつかせる＝本体はいつも新しい中身になる（古い版・追加の領域が無い版が古い中身を見て書き戻さない）。
	//  上書きの途中で電源が切れたら、本体の写しは壊れるが、追加の区画の写しが生きている（続きからで counter が大きいほうを採り、次のレポートでまた追いつかせる）
	{
		u32 reserve = 1, spare, free = FreePhysCount(), pass;
		bool8 useExt = ExtOk();

		for (p = BOX_PAGES; p < SHELF_PAGES; ++p)
		{
			BuildPage(p);
			if (!SameAsFlash(p, gSaveDataBuffer.data))
				++reserve;
		}
		spare = (free > reserve) ? free - reserve : 0;
		for (pass = 0; pass < 2; ++pass) //pass 0 = まだ本体の区画が無いページ（上書きできないので予備を先に）
		{
			for (p = OVFL_FIRST_PAGE; p < BOX_PAGES; ++p)
			{
				s32 phys;
				bool8 carried = (c->ovfl[p] != NO_PAGE);   //追加の区画の写しのほうが新しい（前のレポートで追いつけなかった）
				bool8 same;
				bool8 upg = useExt && (c->legacyMask & (1u << p)) && c->committed[p] != NO_PAGE && !carried;   //★ 2026-10-21 署名のない（前の版の）ページを 署名つきで書き直す（追加の領域が使えるときだけ＝予備が足りなくても 追加の区画の控え＋あとで今の区画へ上書きの道がある）
				if ((c->committed[p] == NO_PAGE) != (pass == 0) || !(PageDirty(p) || carried || upg))
					continue;
				BuildPage(p);
				same = SameAsFlash(p, gSaveDataBuffer.data) && !upg;   //upg は中身が同じでも書く
				if (same && !carried)
				{
					c->written[p] = TRUE; //今のフラッシュと同じ（書かない・書いたのと同じ）
					continue;
				}
				if (same && c->committed[p] != NO_PAGE)
				{
					c->lateMask |= 1 << p; //追加の区画の写しは今のまま・本体の今の区画へ追いつかせるだけ
					c->written[p] = TRUE;
					continue;
				}
				if (spare > 0 && (phys = TakeFreePhys()) >= 0)
				{
					--spare;
					SHELF_WRITE(phys, gSaveDataBuffer.data);
					m->phys[p] = phys;
					c->ovflPending[p] = NO_PAGE; //本体の写しが新しい＝追加の区画の写しは使わない
					c->written[p] = TRUE;
					continue;
				}
				phys = (useExt && c->committed[p] != NO_PAGE) ? TakeFreeOvfl() : -1;
				if (phys >= 0 && HackExtSave_WriteSector(phys, gSaveDataBuffer.data))
				{
					c->ovflPending[p] = phys;
					c->lateMask |= 1 << p;
					c->written[p] = TRUE;
					continue;
				}
				c->overBudget = TRUE; //予備も追加の領域も使えない（追加の領域が無い版は箱の画面で止めるので、起きないはず）
				if (c->committed[p] != NO_PAGE && c->ovfl[p] == NO_PAGE)
					c->lateMask |= 1 << p; //本体を書き終えたあとに今の区画へ上書き（追加の領域が無い版・今までと同じ）
			}
		}
	}
	for (p = EXT_PAGE_BASE; p < ALL_PAGES; ++p) //35〜50 箱（ページ 0 に対応表を書くので先に）
	{
		s32 phys;
		bool8 upgExt = LegacyBit(p) && c->committed[p] != NO_PAGE;   //★ 署名のない追加の区画のページ（2 倍の区画があるので 一度に全部）
		if (!PageDirty(p) && !upgExt)
			continue;
		BuildPage(p);
		if (!upgExt && SameAsFlash(p, gSaveDataBuffer.data))
		{
			c->written[p] = TRUE;
			continue;
		}
		phys = TakeFreeExt(p);
		if (phys < 0 || !HackExtSave_WriteSector(phys, gSaveDataBuffer.data))
		{
			c->overBudget = TRUE; //書けなかった: 前の中身のまま（箱の汚れは残る）
			continue;
		}
		c->extPending[p - EXT_PAGE_BASE] = phys;
		c->written[p] = TRUE;
	}
	for (p = BOX_PAGES; p <= SHELF_PAGES; ++p) //16・17・最後にページ 0（ページ 0 は追加の区画の対応表が決まってから）
	{
		s32 phys;
		u32 q = (p == SHELF_PAGES) ? 0 : p;
		bool8 upg = LegacyBit(q) && c->committed[q] != NO_PAGE;   //★ 署名のない前の版のページ（中身が同じでも 署名つきで書き直す）
		if (!PageDirty(q) && !upg)
			continue;
		BuildPage(q);
		if (!upg && SameAsFlash(q, gSaveDataBuffer.data))
		{
			c->written[q] = TRUE; //中身が今のフラッシュと同じ（書かない・書いたのと同じ＝箱の汚れを取ってよい）
			continue;
		}
		phys = TakeFreePhys();
		if (phys < 0) //予備が足りない（ボックスの画面で止めるので、起きるのはレポートのあとで 16・17 が変わったときくらい）
		{
			c->overBudget = TRUE;
			if (c->committed[q] != NO_PAGE)
				c->lateMask |= 1 << q; //本体を書き終えたあとに今の区画へ上書き（CFRU のもとの区画 30・31 と同じ弱さ・失いはしない）
			continue;
		}
		SHELF_WRITE(phys, gSaveDataBuffer.data);
		m->phys[q] = phys;
		c->written[q] = TRUE;
	}
	EnsureProbeSectors();
	m->seq = gSaveCounter;
}

//★ 2026-10-21 だれも指していない前の版の写し（書き直したページの古い置き場・古い控え）に 前の版のまま残る箱の中身を、署名つきの空のページで潰す。
//  本体の組が書けたあと（AfterSave ok）だけ・1 回のレポートで 6 区画まで・署名なしの中身を持つ区画（id が 空きの印 0x51 でないもの）だけ
#define SHELF_EXT_BASE_SECTOR 32     //追加の区画 n = フラッシュの区画 32 + n（src/hack_ext_save.c）
#define SCRUB_PER_SAVE 16
static bool8 PhysReferenced(u8 phys)
{
	struct ShelfCache* c = SHELF_CACHE;
	struct ShelfMap* m = SHELF_MAP;
	u32 i;
	for (i = 0; i < SHELF_PAGES; ++i)
	{
		if (c->committed[i] == phys || m->phys[i] == phys)
			return TRUE;
	}
	return FALSE;
}

static bool8 ExtReferenced(u8 n)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 i;
	for (i = EXT_PAGE_BASE; i < ALL_PAGES; ++i)
	{
		if (c->committed[i] == n || c->extPending[i - EXT_PAGE_BASE] == n)
			return TRUE;
	}
	for (i = 0; i < BOX_PAGES; ++i)
	{
		if (c->ovfl[i] == n || c->ovflPending[i] == n)
			return TRUE;
	}
	return FALSE;
}

static void BuildFillerPage(void)
{
	Memset(&gSaveDataBuffer, 0, sizeof(gSaveDataBuffer));
	gSaveDataBuffer.id = SHELF_FOOTER_ID + SHELF_PAGES - 1;
	gSaveDataBuffer.signature = SHELF_SIGNATURE;
	gSaveDataBuffer.counter = 0;
	gSaveDataBuffer.checksum = CalculateSaveChecksum(gSaveDataBuffer.data, SHELF_PAGE_SIZE);
}

static void ScrubStaleLegacy(void)
{
	u32 budget = SCRUB_PER_SAVE, i;

	if (!SHELF_CACHE->scrubOk)
		return;   //このセッションで 署名のない前の版のページを読んだときだけ（はじめから では 別のレポートの古い写しを壊さない）
	for (i = SHELF_PHYS_FIRST; i < SHELF_PHYS_FIRST + SHELF_PHYS_COUNT && budget > 0; ++i)
	{
		if (PhysReferenced(i) || !HackSaveCrypt_IsLegacyData(i, SHELF_FOOTER_ID + SHELF_PAGES - 1))
			continue;
		BuildFillerPage();
		SHELF_WRITE(i, gSaveDataBuffer.data);
		--budget;
	}
	if (!ExtOk())
		return;
	for (i = EXT_PHYS_FIRST; i < OVFL_PHYS_FIRST + OVFL_PHYS_COUNT && budget > 0; ++i)
	{
		if (ExtReferenced(i) || !HackSaveCrypt_IsLegacyData(SHELF_EXT_BASE_SECTOR + i, SHELF_FOOTER_ID + SHELF_PAGES - 1))
			continue;
		BuildFillerPage();
		HackExtSave_WriteSector(i, gSaveDataBuffer.data);
		--budget;
	}
}

//レポートのあと（src/save.c の SaveWriteToFlash の最後）
void BoxShelf_AfterSave(bool8 ok)
{
	struct ShelfCache* c = SHELF_CACHE;
	struct ShelfMap* m = SHELF_MAP;
	u32 p, f;

	if (c->magic == SHELF_CACHE_MAGIC && c->oldFormat)
		return; //古い形は何も書いていない（対応表 'SHEL' を触らない）
	if (!ok)
	{
		for (p = 0; p < SHELF_PAGES; ++p)
			m->phys[p] = c->committed[p];
		for (p = 0; p < ALL_EXT_PAGES; ++p)
			c->extPending[p] = c->committed[EXT_PAGE_BASE + p];
		for (p = 0; p < BOX_PAGES; ++p)
			c->ovflPending[p] = c->ovfl[p];
		return;
	}
	for (p = 0; p < SHELF_PAGES; ++p)
		c->committed[p] = m->phys[p];
	for (p = 0; p < ALL_EXT_PAGES; ++p)
		c->committed[EXT_PAGE_BASE + p] = c->extPending[p];
	for (p = 0; p < BOX_PAGES; ++p)
		c->ovfl[p] = c->ovflPending[p];
	c->ovflUsed = c->ovflUsed && !ExtOk();   //追加の領域が無い版は印を残す（BuildPage と同じ）
	for (p = 0; p < BOX_PAGES; ++p)
		c->ovflUsed |= (c->ovfl[p] != NO_PAGE);
	for (p = 0; p < SHELF_PAGES; ++p) //予備が足りなかったページ: 今の区画へ上書き
	{
		if (c->lateMask & (1 << p))
		{
			BuildPage(p);
			if (SHELF_WRITE(c->committed[p], gSaveDataBuffer.data) == SAVE_STATUS_OK)
			{
				c->written[p] = TRUE;
				if (p < BOX_PAGES)
					c->ovfl[p] = NO_PAGE; //本体の区画が追いついた（追加の区画の写しは次のレポートで対応表から外す・失敗したら残す）
			}
			c->lateMask &= ~(1 << p);
		}
	}
	for (p = 0; p < ALL_PAGES; ++p) //署名つきで書き直したページは 署名のない印を下ろす（書いたのと同じで書かなかったページ＝署名の対象でないものは元から立っていない）
	{
		if (c->written[p] && p < 32)
			c->legacyMask &= ~(1u << p);
	}
	ScrubStaleLegacy(); //★ 2026-10-21 本体の組が書けたあと 古い写しを潰す（レポートのたびに 区画の頭 16 バイトを読むだけ・潰すのは 古い写しがあるときだけ）
	if (c->written[BOX_PAGES + 1])
		c->sum16 = c->pendingSum16;
	for (f = 0; f < SHELF_FRAMES; ++f) //書いたページだけでできている箱は汚れが取れる
	{
		u32 b = c->frameBox[f], g, page, off;
		bool8 all = TRUE;
		if (b == NO_BOX || !c->frameDirty[f])
			continue;
		for (g = b * IN_BOX_COUNT; g < (b + 1) * IN_BOX_COUNT; ++g)
		{
			MonLocation(g, &page, &off);
			if (!c->written[page])
				all = FALSE;
		}
		if (all)
			c->frameDirty[f] = FALSE;
	}
}

/* ---- 1 回のレポートで書けるページの数を守る（M2・2026-10-06 ユーザー「レポートを書いてくださいと出して止める」） ----------
 * 書けるのは予備（どのページも指していない物理区画）の数まで（ふつう 22 − 18 = 4）。
 * 数えるページ: 0（いつも）・16（バッグ・ずかんナビ＝ほぼ毎回変わるので、いつも数える）・汚れた箱のページ 1〜15・17（17 の中身が変わったら）。
 * 箱を変える操作の前に「その箱のページを足しても予備に収まるか」を見て、収まらなければ止める（ボックスの画面）／その箱を使わない（捕まえたとき）。 */

static u32 FreePhysCount(void)
{
	struct ShelfCache* c = SHELF_CACHE;
	u32 i, used = 0;
	for (i = 0; i < SHELF_PAGES; ++i)
		used += (c->committed[i] != NO_PAGE);
	return SHELF_PHYS_COUNT - used;
}

static u32 PagesToWrite(void)
{
	u32 p, mask = (1 << 0) | (1 << BOX_PAGES);
	for (p = 1; p < BOX_PAGES; ++p)
	{
		if (PageDirty(p))
			mask |= 1 << p;
	}
	if (Region16Sum() != SHELF_CACHE->sum16)
		mask |= 1 << (BOX_PAGES + 1);
	return mask;
}

static u32 BoxPageMask(u8 boxId)
{
	s32 shelf = BoxShelf_ShelfIndex(boxId);
	u32 p0, p1, off, mask = 0;

	if (shelf >= 0)
	{
		MonLocation(shelf * IN_BOX_COUNT, &p0, &off);
		MonLocation(shelf * IN_BOX_COUNT + IN_BOX_COUNT - 1, &p1, &off);
		for (; p0 <= p1; ++p0)
			mask |= (p0 < 32) ? (1u << p0) : 0;   //★ FEAT-box100: ページ 32 以上は 32 ビットに入らない。ここは CanTouchBox が 26 箱目以降を先に返すので 0〜25 箱だけ（新ページは written[]・箱の汚れで扱う）
		return mask;
	}
	return 0; //L 案: 固定の箱（20〜25 箱）はもう無い
}

static u32 CountBits(u32 x)
{
	u32 n = 0;
	for (; x != 0; x &= x - 1)
		++n;
	return n;
}

//★ 2026-10-24 patches/0732: レポート前の変更（汚れ）を持てる入れ物の数。入れ物は 13 個で、汚れた入れ物は次のレポートまで追い出せない。
//  読むだけで新しい箱を読み込むとき（ステータス画面・箱を開く）に追い出せる入れ物が要る＝汚れていない入れ物 1 個 ＋ 今の箱（pin）の 1 個を空けておく（13 − 2）
#define SHELF_MAX_DIRTY (SHELF_FRAMES - 2)

//この箱を新しく汚しても入れ物が足りるか（もう汚れている箱は増えないので TRUE）
static bool8 FrameBudgetOk(u8 boxId)
{
	struct ShelfCache* c = SHELF_CACHE;
	s32 shelf = BoxShelf_ShelfIndex(boxId), f;
	u32 i, dirty = 0;

	if (shelf < 0 || c->magic != SHELF_CACHE_MAGIC)
		return TRUE;
	f = FindFrame(shelf);
	if (f >= 0 && c->frameDirty[f])
		return TRUE;
	for (i = 0; i < SHELF_FRAMES; ++i)
		dirty += (c->frameBox[i] != NO_BOX && c->frameDirty[i]);
	return dirty < SHELF_MAX_DIRTY;
}

//この箱を変えても、次のレポートで書くページが予備に収まるか
bool8 BoxShelf_CanTouchBox(u8 boxId)
{
	u32 used, box;

	if (SHELF_CACHE->magic == SHELF_CACHE_MAGIC && SHELF_CACHE->oldFormat)
		return FALSE; //古い形のセーブは箱を触らない（止める画面が出る）
	if (!FrameBudgetOk(boxId))
		return FALSE; //レポート前に汚せる箱の数の上限（入れ物 13 個）。レポートを書けば汚れが取れる
	if (boxId >= EXT_BOX_FIRST)
		return ExtAvail(); //35〜50 箱: 追加の区画は 16・ページは 8（予備はいつも足りる）
	if (SHELF_CACHE->magic != SHELF_CACHE_MAGIC)
		return TRUE;
	box = BoxPageMask(boxId);
	if (ExtOk())
		return TRUE; //本体の予備に収まらないページは追加の区画へ（30 区画・ページ 15 枚 × 2＝いつも足りる）
	if (SHELF_CACHE->ovflUsed && (box & BOX_PAGES_MASK) != 0)
		return FALSE; //追加の領域が無い版: 追加の区画に新しい写しがある箱は読むだけ（古い写しの上に書かない）
	used = PagesToWrite();
	if ((used | box) == used)
		return TRUE; //もう書くことになっているページだけ（増えない）
	return CountBits(used | box) <= FreePhysCount();
}

/* ボックスの画面（assembly/hack_box_shelf/box_shelf_hooks.s から） */
#define sCursorArea (*(s8*) 0x0203976C)            //0 = 箱の中・1 = 手持ち（素の CanShiftMon 08093370 が読む）
#define sIsMonBeingMoved (*(u8*) 0x0203976E)
#define gStorageState (**(u8**) 0x020396FC)       //gStorage->state
#define HandleInput ((u8 (*)(void)) (0x080942E8 | 1))
#define TryStorePartyMonInBox ((bool8 (*)(u8)) (0x08092B18 | 1))
#define CURSOR_AREA_IN_BOX 0
#define CURSOR_AREA_IN_PARTY 1   //[hack] patches/0981 素の CURSOR_AREA_IN_PARTY（1）
#define SE_FAILURE 26
extern const u8 gText_BoxShelf_SaveFirst[];
extern const u8 gText_BoxShelf_ExtOnly[];
extern const u8 gText_BoxShelf_ReadOnly[];
extern const u8 gText_BoxShelf_PlaceElsewhere[];
extern const u8 gText_Relive_DarkOnlyHere[];   //[hack] patches/0965 strings/hack_shadow_relive.string
bool8 HackNushi_PssPlaceRefused(s8 cursorPos);   //[hack] patches/0981 src/hack_nushi_party.c
extern const u8 gText_Relive_NoMulti[];        //[hack] patches/0975

//[hack] patches/0975 案 B: 箱 26（0.65.0 の専用の箱）に 0.65.0 から残ったダークがいるか。ダークの印は箱の縮めた形の ext（+0x3A）の bit9〜10 = 3（src/hack_kind.h・Relive_CompIsDark）
static bool8 LegacyBoxHasDark(void)
{
	struct CompressedPokemon buf[8];
	u8 first, n, i;

	for (first = 0; first < IN_BOX_COUNT; first += 8)
	{
		n = (IN_BOX_COUNT - first < 8) ? IN_BOX_COUNT - first : 8;
		if (!BoxShelf_ReadBoxMons(RELIVE_LEGACY_BOX_ID, first, n, buf))
			return FALSE;
		for (i = 0; i < n; ++i)
			if (Relive_CompIsDark(&buf[i]))
				return TRUE;
	}
	return FALSE;
}
#define sCursorPosition (*(s8*) 0x0203976D)       //[hack] 素の TryStorePartyMonInBox が読む（0x08092B18 の literal で確認）
#define sMovingMon ((struct BoxPokemon*) (*(u8**) 0x020396FC + 0x20A0))   //[hack] gPSSData->movingMon（つかんでいる子・include/pokemon_storage_system_internal.h の /* 20a0 */）
#define sPartyMon(i) ((struct BoxPokemon*) (0x020241E4 + (i) * 100))      //[hack] gPlayerParty[i]

enum //素の HandleInput の答え（pokefirered の pokemon_storage_system_internal.h）
{
	INPUT_NONE,
	INPUT_IN_MENU = 8,
	INPUT_DEPOSIT = 11,
	INPUT_WITHDRAW,
	INPUT_MOVE_MON,
	INPUT_SHIFT_MON,
	INPUT_PLACE_MON,
	INPUT_TAKE_ITEM,
	INPUT_GIVE_ITEM,
	INPUT_SWITCH_ITEMS,
	INPUT_PRESSED_B,
	INPUT_MULTIMOVE_START,
	INPUT_MULTIMOVE_CHANGE_SELECTION,
	INPUT_MULTIMOVE_SINGLE,
	INPUT_MULTIMOVE_GRAB_SELECTION,
	INPUT_MULTIMOVE_UNABLE,
	INPUT_MULTIMOVE_MOVE_MONS,
	INPUT_MULTIMOVE_PLACE_MONS,
};

//素の PrintStorageMessage（0808F5E0）と同じ窓・同じ書き方で、自前の文を出す
static void PrintShelfText(const u8* text);
void BoxShelf_PrintNushiRefusal(void);

static void PrintShelfMessage(u8 boxId)
{
	const u8* text = gText_BoxShelf_SaveFirst;

	if (!FrameBudgetOk(boxId))
		;   //★ 2026-10-24 patches/0732: 汚せる箱の数の上限 → 「さきに レポートを かいてね」
	else if (boxId >= EXT_BOX_FIRST)
		text = gText_BoxShelf_ExtOnly;
	else if (SHELF_CACHE->magic == SHELF_CACHE_MAGIC && SHELF_CACHE->ovflUsed && !ExtOk())
		text = gText_BoxShelf_ReadOnly;   //追加の領域が無い版で、追加の区画に新しい写しがある箱
	else if (sIsMonBeingMoved)
		text = gText_BoxShelf_PlaceElsewhere;   //★ つかんだままではレポートを書けない（PC から出られない）→ 書くよう言わず、置ける所を勧める
	PrintShelfText(text);
}

static void PrintShelfText(const u8* text)
{
	PlaySE(SE_FAILURE);
	FillWindowPixelBuffer(1, 0x11);
	AddTextPrinterParameterized(1, 1, text, 0, 2, 0xFF, NULL);
	DrawTextBorderOuter(1, 2, 13);
	PutWindowTilemap(1);
	CopyWindowToVram(1, 2);
	ScheduleBgCopyTilemapToVram(0);
}

//[hack] patches/0981 ヌシの断りの文を PC の下の窓に出す（src/hack_nushi_party.c の BoxShelf_WithdrawCount からも）
extern const u8 gText_Nushi_PartyOnly[];
void BoxShelf_PrintNushiRefusal(void)
{
	PrintShelfText(gText_Nushi_PartyOnly);
}

//Task_PokeStorageMain の state 0。今の箱の中身を変える入力（メニューを開くのも＝にがす・もちもの・マークなど）を、予備に収まらなければ止める
u8 BoxShelf_PssHandleInput(void)
{
	u8 input = HandleInput();

	//[hack] patches/0975 案 B リライブボックス: ダークの子は PC のどの箱にも置けない（つかんだ子を置く／入れ替える入力で見る）。
	//  箱 26 に 0.65.0 から残ったダークがいるあいだは、箱 26 での まとめて つかむ（MULTIMOVE）を断る（ダークがほかの箱へ まとめて動くのを防ぐ）
	if (sCursorArea == CURSOR_AREA_IN_BOX)
	{
		u8 box = StorageGetCurrentBox();
		bool8 refuse = FALSE;
		const u8* text = gText_Relive_DarkOnlyHere;

		switch (input)
		{
		case INPUT_IN_MENU:   //つかんでいる子をこの箱へおくメニュー（「ここにおく」）を開く入力。つかんでいなければ（子を つれていく メニュー）は止めない
			if (!sIsMonBeingMoved)
				break;
			//fallthrough
		case INPUT_PLACE_MON:
		case INPUT_SHIFT_MON:
			if (!Relive_BoxAccepts(box, sMovingMon))
			{
				refuse = TRUE;
			}
			break;
		case INPUT_MULTIMOVE_START:
			if (box == RELIVE_LEGACY_BOX_ID && LegacyBoxHasDark())
			{
				refuse = TRUE;
				text = gText_Relive_NoMulti;
			}
			break;
		}
		if (refuse)
		{
			PrintShelfText(text);
			gStorageState = 3; //素の state 3: ボタンで窓を消して state 0 へ
			return INPUT_NONE;
		}
	}

	//[hack] patches/0981 ヌシは手持ちに 1 匹だけ: つかんだ子を手持ちの枠へ おく・入れかえる（メニューを開く入力も＝メニューの中身は見えないので開く前に止める）で、
	//  つかんだ子がヌシなのに手持ちにもうヌシがいれば断る。入れかえで出ていく子（カーソルの枠の子）は数えない・空の枠なら除外なし。
	//  つかんだ子は元の枠が空になっている（素の PurgeMonOrBoxMon）ので、手持ちからつかんだヌシを手持ちへ戻すのは通る。箱へ おく・あずけるは制限しない。
	//  箱から手持ちへの「てもちに くわえる」は Task_WithdrawMon の hooks（BoxShelf_WithdrawCount）で止める
	if ((input == INPUT_PLACE_MON || input == INPUT_SHIFT_MON || input == INPUT_IN_MENU)
	&& sCursorArea == CURSOR_AREA_IN_PARTY && sIsMonBeingMoved && HackNushi_PssPlaceRefused(sCursorPosition))
	{
		BoxShelf_PrintNushiRefusal();
		gStorageState = 3; //素の state 3: ボタンで窓を消して state 0 へ
		return INPUT_NONE;
	}

	switch (input)
	{
	case INPUT_IN_MENU:
	case INPUT_WITHDRAW:
	case INPUT_MOVE_MON:
	case INPUT_SHIFT_MON:
	case INPUT_PLACE_MON:
	case INPUT_TAKE_ITEM:
	case INPUT_GIVE_ITEM:
	case INPUT_SWITCH_ITEMS:
	case INPUT_MULTIMOVE_START:
	case INPUT_MULTIMOVE_SINGLE:
	case INPUT_MULTIMOVE_GRAB_SELECTION:
	case INPUT_MULTIMOVE_PLACE_MONS:
		if (sCursorArea == CURSOR_AREA_IN_BOX && !BoxShelf_CanTouchBox(StorageGetCurrentBox()))
		{
			PrintShelfMessage(StorageGetCurrentBox());
			gStorageState = 3; //素の state 3: ボタンで窓を消して state 0 へ
			return INPUT_NONE;
		}
		break;
	}
	return input;
}

//Task_DepositMenu（あずける箱を選んだ）: 0 = いっぱい・1 = あずけた（素と同じ）・2 = 止めた
u8 BoxShelf_DepositTry(u8 boxId)
{
	//[hack] patches/0975 案 B リライブボックス: あずける子（手持ちの sCursorPosition 番）がダークなら断る（PC の箱には入れない）
	if (sCursorPosition >= 0 && sCursorPosition < 6   /*手持ちの数*/
	&& !Relive_BoxAccepts(boxId, sPartyMon(sCursorPosition)))
	{
		PrintShelfText(gText_Relive_DarkOnlyHere);
		gStorageState = 4; //素の state 4: ボタンで「どの ボックスに？」へ戻る
		return 2;
	}
	if (!BoxShelf_CanTouchBox(boxId))
	{
		PrintShelfMessage(boxId);
		gStorageState = 4; //素の state 4: ボタンで「どの ボックスに？」へ戻る
		return 2;
	}
	return TryStorePartyMonInBox(boxId);
}

//素の IsPokemonStorageFull（08040694）の置き換え（hooks）: 予備に収まる箱だけで空きを探す（SendMonToPC と同じ）
bool8 BoxShelf_IsPokemonStorageFull(void)
{
	u32 box;
	for (box = 0; box < TOTAL_BOXES_COUNT; ++box)
	{
		if (!BoxShelf_CanTouchBox(box))
			continue;
		if (BoxShelf_FindEmptyPos(box) >= 0)   //★ 2026-10-24 patches/0732: 箱ごとにまとめて読む
			return FALSE;
	}
	return TRUE;
}

#ifdef HACK_BOX_PROTO
/* ---- 試験のビルドだけ（tools/boxvirt/shelf_test.py）: 電源断のまね ---------------------------------------- */
struct ShelfDebug
{
	u32 crashAfter;
	u32 writes;
	u32 crashed;
	u32 done;
};
#define SHELF_DBG ((struct ShelfDebug*) 0x0203FE00)

u8 BoxShelf_TryWriteSector(u8 sector, u8* data)
{
	struct ShelfDebug* dbg = SHELF_DBG;

	dbg->writes++;
	if (dbg->crashed)
		return 1;
	if (dbg->crashAfter != 0 && dbg->writes == dbg->crashAfter)
	{
		Memset(data, 0xFF, sizeof(struct SaveSection));
		TryWriteSector(sector, data);
		dbg->crashed = 1;
		return 1;
	}
	return TryWriteSector(sector, data);
}

//試験: 箱から箱へ 2 匹動かす（1→2・5→39・それぞれの先頭）
void BoxShelfTest_Move(void)
{
	static const u8 moves[2][2] = {{0, 1}, {4, 33}}; //汚れるページ 0・1・2・3・15（予備 4 の中）
	u32 i;
	for (i = 0; i < 2; ++i)
	{
		struct BoxPokemon mon = *GetBoxedMonPtr(moves[i][0], 0);
		SetBoxMonAt(moves[i][1], 0, &mon);
		ZeroBoxMonAt(moves[i][0], 0);
	}
}

//試験: 全部の箱の中身の指紋（箱 × 30 匹の 性格値 ^ 種族）を 0x0201C000（gDecompressionBuffer）へ
void BoxShelfTest_Snapshot(void)
{
	u32* out = (u32*) 0x0201C000;
	u32 b, p;
	for (b = 0; b < TOTAL_BOXES_COUNT; ++b)
		for (p = 0; p < IN_BOX_COUNT; ++p)
			*out++ = GetBoxMonDataAt(b, p, MON_DATA_PERSONALITY) ^ (GetBoxMonDataAt(b, p, MON_DATA_SPECIES) << 16)
				^ (GetBoxMonDataAt(b, p, MON_DATA_MARKINGS) << 28);
}

//試験（M2）: 0x0203FE20 の箱（[0] = 数・[1..] = 箱の id）の 1 匹目のマークを変えて汚し、
//全部の箱の「触れるか」を 0x0203F500（100 バイト）・1 匹目の種族を 0x0203F600（u16 × 100）・書くページの印を 0x0203FED0 へ
void BoxShelfTest_Budget(void)
{
	const u8* req = (const u8*) 0x0203FE20;
	u32 i;
	for (i = 0; i < req[0]; ++i)
	{
		u8 mark = GetBoxMonDataAt(req[1 + i], 0, MON_DATA_MARKINGS) ^ 1;
		SetBoxMonDataAt(req[1 + i], 0, MON_DATA_MARKINGS, &mark);
	}
	for (i = 0; i < TOTAL_BOXES_COUNT; ++i)
	{
		((u8*) 0x0203F500)[i] = BoxShelf_CanTouchBox(i);   //★ FEAT-box100: 100 箱ぶん（u8 × 100）。以前の 0x0203FE40／FE80 は 100 本で重なる
		((u16*) 0x0203F600)[i] = GetBoxMonDataAt(i, 0, MON_DATA_SPECIES);   //u16 × 100 = 200 バイト
	}
	*(u32*) 0x0203FED0 = PagesToWrite();
	*(u32*) 0x0203FED4 = FreePhysCount();
}

//試験（M2）: 手持ちの 1 匹目の写しを SendMonToPC へ（送る先の探し始めは 0x0203FE20 の箱）→ 0x0203FEE0 に 結果・箱・位置・IsPokemonStorageFull
void BoxShelfTest_SendToPC(void)
{
	struct Pokemon mon = gPlayerParty[0];
	u8* out = (u8*) 0x0203FEE0;
	VarSet(VAR_PC_BOX_TO_SEND_MON, *(u8*) 0x0203FE20);
	out[3] = IsPokemonStorageFull();
	out[0] = SendMonToPC(&mon);
	out[1] = gSpecialVar_MonBoxId;
	out[2] = gSpecialVar_MonBoxPos;
}

//試験（M3）: はじめからの消去（src/save.c の NewGameWipeNewSaveData）と素の ResetPokemonStorageSystem（0808C21C・CFRU が箱の数を 39 に）
void NewGameWipeNewSaveData(void);
void BoxShelfTest_NewGame(void)
{
	NewGameWipeNewSaveData();
	((void (*)(void)) (0x0808C21C | 1))();
}

//試験（M3）: 1・25・26・39 箱の名前（9 バイトずつ）を 0x0203FF00 へ
void BoxShelfTest_Names(void)
{
	static const u8 boxes[4] = {0, 24, 25, 38};
	u32 i;
	for (i = 0; i < 4; ++i)
		CopyBytes((u8*) 0x0203FF00 + 9 * i, GetBoxNamePtr(boxes[i]), 9);
}

void BoxShelfTest_Save(void)
{
	SHELF_DBG->writes = 0;
	SHELF_DBG->crashed = 0;
	SHELF_DBG->done = 0;
	((void (*)(void)) (0x08058994 | 1))(); //SaveMapView（メニューのレポートと同じ準備）
	TrySavingData(SAVE_NORMAL);
	SHELF_DBG->done = 1;
}
#endif

/* ---- 通信のあとの「少しずつのレポート」（素の LinkFullSave_*・ユニオンルームなど） ---------------------------------
 * 素は区画を 1 つずつ書き換える（組を 14 区画と決め打ち・gFirstSaveSector を 14 で回す）→ 棚の区画を壊す。
 * hooks（patches/0196）で入口を置き換え、入口で棚も込みの普通のレポートをし、あとの手順は「済んだ」と返す */
bool8 HackLinkSave_Init(void)          //LinkFullSave_Init 080DB394
{
	TrySavingData(SAVE_NORMAL);
	return FALSE;
}

bool8 HackLinkSave_WriteSector(void)   //LinkFullSave_WriteSector 080DB3C0（TRUE = 書き終えた）
{
	return TRUE;
}

bool8 HackLinkSave_NoOp(void)          //LinkFullSave_ReplaceLastSector 080DB3F4・LinkFullSave_SetLastSectorSignature 080DB41C
{
	return FALSE;
}
