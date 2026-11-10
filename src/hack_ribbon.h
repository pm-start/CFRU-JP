#pragma once

/*
 * リボンの表（[hack] FEAT-relive-ribbon・patches/0977）
 * 設計: 箱の縮めた形（78 バイト）はリボン 32 ビットを捨てる（include/new/pokemon_storage_system.h）。預けて引き出すと消えるので、
 *   ポケモン自身の ID（HackMonId_Get・手持ちも箱も同じ 32 ビット）を鍵に、リボンのビット（80 バイト形の +0x08 の 27 ビット）を別の表に持つ。
 *   作りは hack_move_count.c（patches/0878）と同じ: 追加のセーブ領域の区画を、ID % 2 の 2 つの論理ブロック × 交互 2 区画（RIBBON_SECTOR 85〜88）。
 *   状態（各ブロックの正の区画・世代・件数）は SaveBlock1 + 0x3A2A の 10 バイト（手持ちと同じレポートで一緒に保存される）。
 *   表が空のうちは SaveBlock1 のその 10 バイトも区画も 0 のまま。追加の領域が使えない環境では表は無い＝今までどおり手持ちのビットだけ。
 *   手持ち→箱（預ける）で Store・箱→手持ち（引き出す）で Restore。箱から箱へ動かしても鍵が同じなので何もしなくてよい。
 *   溢れたら（1 ブロック 339 件）いちばん古い件から捨てる。
 */

#define RIBBON_BLOCKS       2
#define RIBBON_CAPACITY     339     /* 1 区画（0xFF4）に入る件数: 16 + 12 × 339 = 4084 */
#define RIBBON_SB1_OFF      0x3A2A
#define RIBBON_MASK         0x07FFFFFFu    /* 80 バイト形 +0x08 の下位 27 ビット（コンテスト 15・チャンピオン〜がんばり 5・配布 7）。上位はうんめいの印などなので触らない */
#define HACK_RIBBON_NATIONAL  (1u << 24)    /* 80 バイト形 +0x08 の bit24 = giftRibbon5＝ナショナルリボン（エメラルドの MON_DATA_NATIONAL_RIBBON・リライブで付く） */
#define HACK_RIBBON_VERSION 0x4200   /* 表を書く最初の版（0.66.0）。これより古い版は表を読まない */

struct RibbonEntry              /* 12 バイト */
{
	u32 monId;
	u32 ribbons;
	u16 check;                  /* 先頭 8 バイトの和 */
	u16 spare;
};

struct RibbonSave               /* SaveBlock1 + 0x3A2A（10 バイト） */
{
	u16 magic;                  /* 'RB'。0 = 空 */
	u8 version;
	u8 selBits;                 /* 下位 2 ビット = ブロックごとの正の区画。上位 2 ビット = RAM の意味だけ（レポートで確定した区画） */
	u8 gen[RIBBON_BLOCKS];
	u16 count[RIBBON_BLOCKS];
};

struct RibbonBlock              /* 追加の区画の先頭から */
{
	u32 magic;                  /* 'RBNB' */
	u16 version;
	u16 sum;
	u16 gen;
	u16 count;
	u32 spare;
	struct RibbonEntry e[RIBBON_CAPACITY];
};

struct BoxPokemon;
struct Pokemon;

bool8 HackRibbon_Available(void);                           /* 追加の領域が使えるか */
bool8 HackRibbon_NotEmpty(void);                            /* 表に 1 件でもあるか（SaveBlock1 を見るだけ） */
bool8 HackRibbon_Find(u32 monId, u32* ribbons);             /* 見つかり、検査が合えば TRUE */
bool8 HackRibbon_Store(u32 monId, u32 ribbons);             /* ribbons = 0 なら件を消す。同じなら書かない。書けなかったら FALSE */
void HackRibbon_StoreFromMon(const struct BoxPokemon* mon); /* 預ける: 手持ち／80 バイト形のビットを表へ（卵・ID なし・表なしは何もしない） */
void HackRibbon_RestoreToMon(struct Pokemon* mon);          /* 引き出す: 表のビットを手持ちの形へ OR（表が空・無い子は何もしない） */
u32 HackRibbon_Bits(const struct BoxPokemon* mon);          /* 80 バイト形のリボンのビット（RIBBON_MASK） */
void HackRibbon_AddBits(struct BoxPokemon* mon, u32 bits);   /* 80 バイト形にリボンを付ける（RIBBON_MASK の中だけ） */
bool8 HackRibbon_GiveGift(u8 index, u8 gift);               /* 配布リボン 1〜7 を手持ちの index 番に付ける（新しく付いたら TRUE・呼び手はまだ無い） */
void HackRibbon_OnLoad(void);                               /* つづきから */
void HackRibbon_AfterSave(bool8 ok);                        /* レポートのあと */
