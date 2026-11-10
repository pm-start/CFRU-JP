#pragma once

/*
 * Moémon の絵の「世代」の控え（[hack] 0.63.1・docs/research/2026-11-03_moemon-generations.md・設計 D:\agents\moemon-import\DESIGN.md）。
 *   世代 1 = 0.63.0 までの絵の組（今の gMoemonSpeciesFirst／Count／EntryGender）・世代 2 = 0.63.1 の別案 4 枚・描き直し 13 枚。
 *   「0.63.0 より前から続くセーブにいた Moémon の当たりの子は前の絵のまま・それ以降に入手した子は新しい絵」を、
 *   性格値（PID）→ 世代 の表で持つ。表にない子は最新の世代。個体には何も書かない（性格値・表 ID・裏 ID は変えない＝6.5）。
 *   置き場は追加のセーブ領域の 2 区画 MOEMON_GEN_SECTOR（81・82 の交互 2 区画・ぼうけんのしょごとに別）。
 *   状態（正の区画・世代・件数・控え済みの世代）は SaveBlock1 + 0x3A14 の 12 バイト＝手持ちと同じレポートで一緒に保存される。
 *   はじめから（HACK_VAR_SAVE_VERSION_FIRST が HACK_MOEMON_GEN_VERSION 以上）のセーブは表を持たない（状態だけ「控え済み・件 0」＝全員が最新の世代）。
 *   追加の領域が使えない環境では控えられない＝控え待ちのまま、表にない子も表にある子も全部 世代 1（今の絵のまま）。
 */

#define MOEMONGEN_GEN_LATEST      3        /* 今の絵の組の世代（次に絵の組を更新するときは上げて、DPE の上書き表に世代を足す） */
#define MOEMONGEN_SB1_OFF         0x3A14
#define MOEMONGEN_CAPACITY        508      /* 1 区画（0xFF4）に入る件数: 16 + 8 × 508 = 4080 */
#define HACK_MOEMON_GEN_VERSION   0x3F01   /* 表を扱う最初の版（v0.63.1）。これ以上の版ではじめたセーブは表を持たない */

struct MoemonGenSave            /* SaveBlock1 + 0x3A14（12 バイト） */
{
	u32 magic;                  /* 'MGN1'。0 = まだ無い */
	u16 gen;                    /* 書くたびに +1（区画の頭と合わなければ空と見なす） */
	u16 sum;                    /* 区画の件の和 */
	u16 count;                  /* 件数（0〜MOEMONGEN_CAPACITY） */
	u8 sel;                     /* 正の区画（0 = MOEMON_GEN_SECTOR・1 = +1） */
	u8 doneGen;                 /* 控えを終えた世代（0 = 控え待ち） */
};

struct MoemonGenEntry           /* 8 バイト（性格値の昇順に並べる） */
{
	u32 pid;
	u8 gen;
	u8 pad;
	u16 check;                  /* 先頭 6 バイトの和 */
};

struct MoemonGenBlock           /* 追加の区画の先頭から */
{
	u32 magic;                  /* 'MGNB' */
	u16 version;
	u16 sum;
	u16 gen;
	u16 count;
	u32 spare;
	struct MoemonGenEntry e[MOEMONGEN_CAPACITY];
};

struct MoemonGenCollect         /* 控える子の性格値の集め方（昇順・重複なし・溢れは数える） */
{
	u32* pids;
	u16 n;
	u16 overflow;
};

/* DPE の HackMoemon_Slot が（当たりを通ったあとで）聞く: この性格値の子の絵の世代（1〜MOEMONGEN_GEN_LATEST）。gHackMoemonGenBridge 経由 */
u32 HackMoemonGen_Of(u32 personality);
void HackMoemonGen_FieldTick(void);                                       /* 毎コマ（src/hack_moemon_switch.c から） */
void HackMoemonGen_AddHit(struct MoemonGenCollect* c, u32 personality);   /* 持ち主の列挙（src/hack_moemon_gen_owners.c）が呼ぶ */
bool8 HackMoemonGen_CollectAll(struct MoemonGenCollect* c);               /* 持ち主の列挙。FALSE = 集められなかった（あとでやり直す） */
