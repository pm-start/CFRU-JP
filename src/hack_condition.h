#pragma once

/*
 * ポケモンのコンディション 6 値の表（[hack] FEAT-pokeblock 段 3・patches/0978）
 * 設計: エメラルドは かっこよさ・うつくしさ・かわいさ・かしこさ・たくましさ（MON_DATA_COOL 等）と けづや（SHEEN）を BoxPokemon の substruct に持つが、
 *   この改造では手持ちの +0x3E〜+0x41 がポケモン自身の表 ID・裏 ID（src/hack_mon_id.c）に使われているので、**MON_DATA_COOL〜SMART には書かない・読まない**。
 *   代わりに ポケモン自身の ID（HackMonId_Get・手持ちも箱も同じ 32 ビット）を鍵にした表に 6 値を持つ（リボンの表 hack_ribbon.c と同じ作り・
 *   手持ち⇔箱の同期は要らない＝鍵で直接引く）。ポロックをあげる画面（hack_pokeblock_use.c）とコンテストの台本が読む。
 *   置き場: 追加のセーブ領域の区画を ID の下 1 ビットの 2 ブロック × 交互 2 区画（CONDITION_SECTOR 89〜92）。
 *   状態（各ブロックの正の区画・世代・件数）は SaveBlock1 + 0x3A44 の 8 バイト（手持ちと同じレポートで一緒に保存される・filler_3A08 の最後）。
 *   全部 0 の子は件を持たない。表が空のうちは SaveBlock1 のその 8 バイトも区画も 0 のまま。追加の領域が使えない環境では表は無い（全部 0・書くは FALSE）。
 *   溢れたら（1 ブロック 339 件）いちばん古い件から捨てる。卵・種族なし・ID のまだ無い子は読むと全部 0。
 */

#define CONDITION_BLOCKS       2
#define CONDITION_CAPACITY     339      /* 1 区画（0xFF4）に入る件数: 16 + 12 × 339 = 4084 */
#define CONDITION_SB1_OFF      0x3A44
#define CONDITION_COUNT_VALUES 6        /* かっこよさ・うつくしさ・かわいさ・かしこさ・たくましさ・けづや */
#define HACK_CONDITION_VERSION 0x4200   /* 表を書く最初の版（0.66.0・リボンと同じ）。これより古い版は表を読まない */

//値の並び（u8 v[6] の添字）＝原文の MON_DATA の順: かっこよさ・うつくしさ・かわいさ・かしこさ・たくましさ・けづや
//（原文 use_pokeblock.c のゲージ用の CONDITION_*（COOL・TOUGH・SMART・CUTE・BEAUTY）は別の並び＝画面側で読み替える）
enum { HCOND_COOL, HCOND_BEAUTY, HCOND_CUTE, HCOND_SMART, HCOND_TOUGH, HCOND_SHEEN };

struct ConditionEntry           /* 12 バイト */
{
	u32 monId;
	u8 v[CONDITION_COUNT_VALUES];
	u16 check;                  /* 先頭 10 バイトの和 */
};

struct ConditionSave            /* SaveBlock1 + 0x3A44（8 バイト） */
{
	u8 magic;                   /* 'C'。0 = 空 */
	u8 selBits;                 /* 下位 2 ビット = ブロックごとの正の区画。上位 2 ビット = RAM の意味だけ（レポートで確定した区画） */
	u8 gen[CONDITION_BLOCKS];
	u16 count[CONDITION_BLOCKS];
};

struct ConditionBlock           /* 追加の区画の先頭から */
{
	u32 magic;                  /* 'CNDB' */
	u16 version;
	u16 sum;
	u16 gen;
	u16 count;
	u32 spare;
	struct ConditionEntry e[CONDITION_CAPACITY];
};

struct BoxPokemon;

bool8 HackCondition_Available(void);                                          /* 追加の領域が使えるか */
bool8 HackCondition_NotEmpty(void);                                           /* 表に 1 件でもあるか（SaveBlock1 を見るだけ） */
void HackCondition_Get(const struct BoxPokemon* mon, u8* out);                /* out[6]。無い・卵・種族なし・ID なし・使えない環境は全部 0 */
u8 HackCondition_GetOne(const struct BoxPokemon* mon, u8 i);                  /* i は HCOND_* */
bool8 HackCondition_Set(struct BoxPokemon* mon, const u8* v);                 /* v[6]。全部 0 なら件を消す。同じなら書かない。卵・使えない環境・書けなかったら FALSE（状態は変えない）。ID が無ければ HackMonId_Ensure で付ける */
void HackCondition_OnLoad(void);                                              /* つづきから（src/save.c） */
void HackCondition_AfterSave(bool8 ok);                                       /* レポートのあと（src/save.c） */
