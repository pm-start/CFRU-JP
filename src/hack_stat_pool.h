#pragma once

/*
 * 手持ち・育て屋・80 バイト形の努力値・個体値の「上の桁」の表（プール・[hack] patches/0760）
 * 設計・確定した形: docs/research/2026-10-24_party-stat16-save.md「確定した形」。
 *   80 バイト・100 バイト形は太らせられない（SaveBlock1 がぴったり）ので、入れ物（努力値 u8・個体値 5 ビット）を超える値は
 *   追加のセーブ領域の区画 STAT_POOL_SECTOR（と +1 の交互 2 区画）の表に、性格値（PID）と親 ID の下位 16 ビット（spare・patches/0799）を鍵にして持つ。80 バイトの中には頭打ちの「影」を入れる。
 *   表の状態（正の区画・世代・件数）は SaveBlock1 + 0x0448 の 12 バイト＝手持ちと同じレポートで一緒に保存される。
 *   表が空のうちは SaveBlock1 のその 12 バイトも区画も 0 のまま（遅延初期化＝セーブのバイトが変わらない）。
 *   追加の領域が使えない環境では表は無いものとして影（頭打ち）だけで動く。
 */

#define STATPOOL_STATS      6
#define STATPOOL_EV_WIDTH   255     /* 80 バイト形の努力値の欄（u8）の最大＝影の頭打ち */
#define STATPOOL_IV_WIDTH   ((1 << 5) - 1)   /* 80 バイト形の個体値の欄（5 ビット）の最大（=31・上限 HACK_IV_MAX ではなく入れ物の幅） */
#define STATPOOL_SB1_OFF    0x0448
#define STATPOOL_CAPACITY   127     /* 1 区画（0xFF4）に入る件数 */
#define STATPOOL_SWEEP_AT   100     /* レポートの頭でこの件数以上なら掃除する */
#define HACK_STAT_POOL_VERSION 0x3B06   /* 表を書く最初の版（v0.59.6）。これより古い版は表を読まない＝つづきからで止める */

struct StatPoolEntry            /* 32 バイト */
{
	u32 pid;                    /* 鍵（性格値）。0 は使わない */
	u16 ev[STATPOOL_STATS];
	u16 iv[STATPOOL_STATS];
	u16 check;
	u16 spare;                  /* 親 ID（otId の下位 16 ビット・0 は 0xFFFF に置換）。0 = 旧件（v0.59.6／7 の件・親を問わず pid だけで合う）。check（先頭 28 バイト）の外＝古い件の検査を壊さない（patches/0799） */
};

struct StatPoolOwner            /* 持ち主 1 匹（掃除の突き合わせ用） */
{
	u32 pid;
	u16 ot;                     /* otId の下位 16 ビット（そのまま） */
};

struct StatPoolSave             /* SaveBlock1 + 0x0448（12 バイト） */
{
	u32 magic;                  /* 'SPL1'。0 = 空 */
	u16 gen;
	u16 sum;
	u8 sel;
	u8 count;
	u8 version;
	u8 committedSel;            /* RAM の意味だけ（つづきからで sel に直す・保存されても読まない） */
};

struct StatPoolBlock            /* 追加の区画の先頭から */
{
	u32 magic;                  /* 'SPLB' */
	u16 version;
	u16 sum;
	u16 gen;
	u16 count;
	u32 spare;
	struct StatPoolEntry e[STATPOOL_CAPACITY];
};

bool8 HackStatPool_NotEmpty(void);                                               /* 表に 1 件でもあるか（SaveBlock1 を見るだけ） */
u8 HackStatPool_Count(void);
bool8 HackStatPool_Find(u32 pid, u16 ot, u16* ev, u16* iv);                              /* 見つかり、件の検査が合えば TRUE */
bool8 HackStatPool_Store(u32 pid, u16 ot, const u16* ev, const u16* iv);                 /* 全 12 欄が入れ物に収まるなら件を消す。入れられなかったら FALSE（頭打ちのまま） */
void HackStatPool_Remove(u32 pid, u16 ot);
u8 HackStatPool_Sweep(void);                                                     /* 持ち主のいない件を落とす。落とした数 */
void HackStatPool_ReportBegin(void);                                             /* レポートの頭（満杯に近ければ掃除） */
void HackStatPool_OnLoad(void);
void HackStatPool_AfterSave(bool8 ok);

/* 影が入れ物の最大ちょうどのときだけ表を引く（影と食い違えば影を採る＝自己修復） */
u16 HackStatPool_ResolveEv(u32 pid, u16 ot, u8 stat, u8 shadow);
u16 HackStatPool_ResolveIv(u32 pid, u16 ot, u8 stat, u8 shadow);
/* 値を書いたあとの表の更新。shadowEv／shadowIv はそのポケモンの今の影（6 欄ずつ） */
void HackStatPool_NoteEv(u32 pid, u16 ot, u8 stat, u16 value, const u8* shadowEv, const u8* shadowIv);
void HackStatPool_NoteIv(u32 pid, u16 ot, u8 stat, u16 value, const u8* shadowEv, const u8* shadowIv);

/* 持ち主の列挙（src/hack_stat_pool_owners.c）。out に (PID, 親 ID の下位 16 ビット) を最大 max 個。個数を返す */
u8 HackStatPool_CollectOwners(struct StatPoolOwner* out, u8 max);
