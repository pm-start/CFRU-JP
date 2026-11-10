#pragma once

/*
 * 技進化（独自）の使用回数の表（[hack] patches/0878・FEAT-move-evo-s2・2026-10-26）
 * 設計: work/tmp/move_evo_counter/PLAN.md 案 A'（hack_stat_pool.c の作りを写す）。
 *   ポケモン自身の ID（HackMonId_Get・手持ちも箱も同じ 32 ビット）を鍵に、1 匹ぶん {技・回数} × 4 を持つ。
 *   置き場は追加のセーブ領域の 8 区画 MOVE_COUNT_SECTOR（73〜80）。ID % 4 で 4 つの論理ブロックに振り、
 *   各ブロックが交互 2 区画（MOVE_COUNT_SECTOR + 2b と +1）＝書くのは 1 区画だけ・読むのも 1 区画だけ。
 *   状態（各ブロックの正の区画・世代・件数）は SaveBlock1 + 0x3A08 の 12 バイト＝手持ちと同じレポートで一緒に保存される。
 *   表が空のうちは SaveBlock1 のその 12 バイトも区画も 0 のまま（遅延初期化＝セーブのバイトが変わらない）。
 *   追加の領域が使えない環境では表は無いものとして回数なし（確率は基本% だけ）。
 *   溢れたら（1 ブロック 203 件）回数の合計が少なく古い件から捨てる。
 */

#define MOVECOUNT_BLOCKS      4
#define MOVECOUNT_SLOTS       4       /* 1 匹の持てる技の数（手持ちの技と同じ） */
#define MOVECOUNT_CAPACITY    203     /* 1 区画（0xFF4）に入る件数: 16 + 20 × 203 = 4076 */
#define MOVECOUNT_SB1_OFF     0x3A08
#define MOVECOUNT_FLAG_MASK   0x000F    /* spare／flags のうち意味のあるビット（技の欄 0〜3）。それ以外は読まず、書くときは 0 */
#define HACK_MOVE_COUNT_VERSION 0x3D00   /* 表を書く最初の版（v0.61.0）。これより古い版は表を読まない＝つづきからで止める */

struct MoveCountRec             /* 1 匹ぶん（RAM の写しと表の件の中身） */
{
	u32 monId;                  /* 鍵（HackMonId_Get）。0 は使わない */
	u16 move[MOVECOUNT_SLOTS];  /* 0 = 空き */
	u8 count[MOVECOUNT_SLOTS];
	u16 flags;                  /* 下位 4 ビット = 技の欄ごとの「じゅくれん」（patches/0932・1 = じゅくれん＝進化しない）。ほかのビットは 0 */
};

struct MoveCountEntry           /* 20 バイト */
{
	u32 monId;
	u16 move[MOVECOUNT_SLOTS];
	u8 count[MOVECOUNT_SLOTS];
	u16 check;                  /* 先頭 16 バイトの和 */
	u16 spare;                  /* 下位 4 ビット = 技の欄ごとの「じゅくれん」（patches/0932）。0.61.0／0.61.1 は 0 のまま書く＝全部「しんか」。検査の和に入っていない */
};

struct MoveCountSave            /* SaveBlock1 + 0x3A08（12 バイト） */
{
	u16 magic;                  /* 'MV'。0 = 空 */
	u8 version;
	u8 selBits;                 /* 下位 4 ビット = ブロックごとの正の区画（0／1）。上位 4 ビット = RAM の意味だけ（つづきからで sel に直す・保存されても読まない） */
	u8 gen[MOVECOUNT_BLOCKS];   /* ブロックごとの世代（書くたび +1・ブロックの頭の世代の下位 8 ビットと合う） */
	u8 count[MOVECOUNT_BLOCKS]; /* ブロックごとの件数 */
};

struct MoveCountBlock           /* 追加の区画の先頭から */
{
	u32 magic;                  /* 'MVCB' */
	u16 version;
	u16 sum;                    /* 件（count 件ぶん）の和 */
	u16 gen;
	u16 count;
	u32 spare;
	struct MoveCountEntry e[MOVECOUNT_CAPACITY];
};

bool8 HackMoveCount_Available(void);                                        /* 追加の領域が使えるか */
bool8 HackMoveCount_NotEmpty(void);                                         /* 表に 1 件でもあるか（SaveBlock1 を見るだけ） */
u16 HackMoveCount_Total(void);                                              /* 全ブロックの件数の合計 */
bool8 HackMoveCount_Find(u32 monId, struct MoveCountRec* out);             /* 見つかり、件の検査が合えば TRUE */
bool8 HackMoveCount_Store(const struct MoveCountRec* rec);                 /* 全部の回数が 0 で「じゅくれん」の印も無ければ件を消す。同じ内容なら書かない。書けなかったら FALSE */
void HackMoveCount_OnLoad(void);                                            /* つづきから */
void HackMoveCount_AfterSave(bool8 ok);                                     /* レポートのあと */
