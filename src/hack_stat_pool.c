#include "defines.h"
#include "../include/save.h"
#include "hack_ext_save.h"
#include "hack_box_shelf.h"
#include "hack_stat_pool.h"

/*
 * 上の桁の表（[hack] patches/0760）。置き場・形・版の守りは hack_stat_pool.h と docs/research/2026-10-24_party-stat16-save.md「確定した形」。
 *   書き込みは卵専用ボックスと同じ交互 2 区画: 書いた時点で「前のレポートで正だった区画」でないほうへ書き、SaveBlock1 の sel／gen／sum／count を更新する。
 *   SaveBlock1 はレポートで保存されるので、レポートの前にリセットすれば SaveBlock1 は前の sel（無傷の区画）に戻る。
 *   使えない環境・壊れた区画・世代の合わない区画は「表は空」と見なす（影＝頭打ちで動く）。
 *   読み書きは HackExtSave_*（スロットのバンクを通る）だけ。区画番号を物理番地にしない＝ぼうけんのしょ 1・2 で表は別々。
 */

#define SPL1_MAGIC 0x314C5053      //'SPL1'
#define SPLB_MAGIC 0x424C5053      //'SPLB'
#define STATPOOL_SIGNATURE 0x08012025    //ほかの追加の区画と同じ（道具が署名つきの区画として読める）
#define STATPOOL_CHUNK 8

typedef char StatPoolEntrySize[(sizeof(struct StatPoolEntry) == 32) ? 1 : -1];
typedef char StatPoolSaveSize[(sizeof(struct StatPoolSave) == 12) ? 1 : -1];
typedef char StatPoolBlockFits[(sizeof(struct StatPoolBlock) <= 0xFF4 && sizeof(struct StatPoolBlock) == 16 + 32 * STATPOOL_CAPACITY) ? 1 : -1];
typedef char StatPoolSb1Fits[(STATPOOL_SB1_OFF == 0x448 && STATPOOL_SB1_OFF + sizeof(struct StatPoolSave) <= 0x458) ? 1 : -1];   //SaveBlock1 の卵ボックスの 12 バイトの次の空き

#include "hack_save_version.h"
typedef char StatPoolNeedsVersion[(HACK_SAVE_VERSION >= HACK_STAT_POOL_VERSION) ? 1 : -1];   //表を書く版以上でないと、古い版が表つきのセーブを「新しすぎる」と止められない

extern struct SaveSection gSaveDataBuffer;

#define SV ((struct StatPoolSave*) ((u8*) gSaveBlock1 + STATPOOL_SB1_OFF))

static u16 Sum16(const u8* p, u32 n)
{
	u32 i, t = 0x5350;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 EntryCheck(const struct StatPoolEntry* e)
{
	return Sum16((const u8*) e, 28);
}

//鍵 16 ビット: 親 ID の下位 16 ビット。0 は「旧件」の印と衝突するので 0xFFFF に置く
static u16 OtKey(u16 ot)
{
	return ot != 0 ? ot : 0xFFFF;
}

//件が (pid, ot) に当たるか。spare==0 の旧件（v0.59.6／7）は親を問わず pid だけで合う
static bool8 Match(const struct StatPoolEntry* e, u32 pid, u16 ot)
{
	return e->pid == pid && (e->spare == 0 || e->spare == OtKey(ot));
}

static bool8 StateOk(const struct StatPoolSave* s)
{
	return s->magic == SPL1_MAGIC && s->version == 1 && s->count <= STATPOOL_CAPACITY && s->sel <= 1;
}

bool8 HackStatPool_NotEmpty(void)
{
	struct StatPoolSave* s = SV;

	return s->magic == SPL1_MAGIC && s->count != 0;
}

u8 HackStatPool_Count(void)
{
	struct StatPoolSave* s = SV;

	return StateOk(s) ? s->count : 0;
}

static bool8 AnyWide(const u16* ev, const u16* iv)
{
	u8 i;

	for (i = 0; i < STATPOOL_STATS; ++i)
		if (ev[i] > STATPOOL_EV_WIDTH || iv[i] > STATPOOL_IV_WIDTH)
			return TRUE;
	return FALSE;
}

static bool8 HeaderOk(const struct StatPoolBlock* b, const struct StatPoolSave* s)
{
	return b->magic == SPLB_MAGIC && b->version == 1 && b->gen == s->gen && b->count == s->count;
}

//表の読み: 使えるときだけ。件ごとに検査（pid〜iv）を見る
static bool8 FindIndex(const struct StatPoolSave* s, u32 pid, u16 ot, struct StatPoolEntry* out, u8* index)
{
	bool8 haveLegacy = FALSE;
	struct StatPoolEntry legacy;
	u8 legacyIndex = 0;
	u32 head[4];                 //頭の 16 バイトだけ（StatPoolBlock 全体を積まない＝スタック 1KB の決まり・6.35）
	struct StatPoolEntry buf[STATPOOL_CHUNK];
	u8 i, j, n;

	if (!StateOk(s) || s->count == 0 || pid == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	if (!HackExtSave_ReadKnownAvail(STAT_POOL_SECTOR + (s->sel & 1), 0, &head, 16) || !HeaderOk((const struct StatPoolBlock*) head, s))
		return FALSE;
	for (i = 0; i < s->count; i += STATPOOL_CHUNK)
	{
		n = s->count - i < STATPOOL_CHUNK ? s->count - i : STATPOOL_CHUNK;
		if (!HackExtSave_ReadKnownAvail(STAT_POOL_SECTOR + (s->sel & 1), 16 + i * 32, buf, n * 32))
			return FALSE;
		for (j = 0; j < n; ++j)
			if (Match(&buf[j], pid, ot) && buf[j].check == EntryCheck(&buf[j]))
			{
				if (buf[j].spare == 0)          //旧件: 親の合う件が別にあればそちらを優先（最後まで探す）
				{
					if (!haveLegacy)
					{
						legacy = buf[j];
						legacyIndex = i + j;
						haveLegacy = TRUE;
					}
					continue;
				}
				if (out != NULL)
					*out = buf[j];
				if (index != NULL)
					*index = i + j;
				return TRUE;
			}
	}
	if (haveLegacy)
	{
		if (out != NULL)
			*out = legacy;
		if (index != NULL)
			*index = legacyIndex;
		return TRUE;
	}
	return FALSE;
}

bool8 HackStatPool_Find(u32 pid, u16 ot, u16* ev, u16* iv)
{
	struct StatPoolEntry e;
	u8 i;

	if (!FindIndex(SV, pid, ot, &e, NULL))
		return FALSE;
	for (i = 0; i < STATPOOL_STATS; ++i)
	{
		if (ev != NULL)
			ev[i] = e.ev[i];
		if (iv != NULL)
			iv[i] = e.iv[i];
	}
	return TRUE;
}

u16 HackStatPool_ResolveEv(u32 pid, u16 ot, u8 stat, u8 shadow)
{
	u16 ev[STATPOOL_STATS];

	if (shadow < STATPOOL_EV_WIDTH || stat >= STATPOOL_STATS || !HackStatPool_NotEmpty())
		return shadow;
	if (HackStatPool_Find(pid, ot, ev, NULL) && (ev[stat] < STATPOOL_EV_WIDTH ? ev[stat] : STATPOOL_EV_WIDTH) == shadow)
		return ev[stat];
	return shadow;                    //食い違い・見つからない: 影を採る（自己修復）
}

u16 HackStatPool_ResolveIv(u32 pid, u16 ot, u8 stat, u8 shadow)
{
	u16 iv[STATPOOL_STATS];

	if (shadow < STATPOOL_IV_WIDTH || stat >= STATPOOL_STATS || !HackStatPool_NotEmpty())
		return shadow;
	if (HackStatPool_Find(pid, ot, NULL, iv) && (iv[stat] < STATPOOL_IV_WIDTH ? iv[stat] : STATPOOL_IV_WIDTH) == shadow)
		return iv[stat];
	return shadow;
}

//---- 書き ----

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static struct StatPoolBlock* Work(void)
{
	return (struct StatPoolBlock*) gSaveDataBuffer.data;
}

//入れ物 gSaveDataBuffer に今の表を読む。空・壊れ・世代違いは空の入れ物を作る。使えない環境は FALSE
static bool8 LoadWork(void)
{
	struct StatPoolSave* s = SV;
	struct StatPoolBlock* w = Work();

	if (!BoxShelf_ExtAvailable())
		return FALSE;
	if (StateOk(s) && s->count != 0 && HackExtSave_ReadSector(STAT_POOL_SECTOR + (s->sel & 1), &gSaveDataBuffer)
	 && HeaderOk(w, s) && Sum16((const u8*) w->e, s->count * 32) == w->sum)
		return TRUE;
	Zero(&gSaveDataBuffer, sizeof(gSaveDataBuffer));
	w->magic = SPLB_MAGIC;
	w->version = 1;
	return TRUE;
}

//入れ物 gSaveDataBuffer（件数 w->count）を、前のレポートの正でないほうの区画へ書き、SaveBlock1 の状態を更新する
static bool8 Commit(void)
{
	struct StatPoolSave* s = SV;
	struct StatPoolBlock* w = Work();
	bool8 fresh = (s->magic != SPL1_MAGIC);
	u8 sel = StateOk(s) ? s->sel : 0;
	u8 committed = fresh ? 1 : (s->committedSel & 1);      //空の状態からの最初の書き込みは区画 0 へ
	u8 target = (sel == committed) ? (sel ^ 1) : sel;
	u16 gen = StateOk(s) ? s->gen + 1 : 1;
	u32 i;

	for (i = w->count; i < STATPOOL_CAPACITY; ++i)
		Zero(&w->e[i], sizeof(w->e[i]));
	w->magic = SPLB_MAGIC;
	w->version = 1;
	w->gen = gen;
	w->sum = Sum16((const u8*) w->e, w->count * 32);
	gSaveDataBuffer.signature = STATPOOL_SIGNATURE;
	if (!HackExtSave_WriteSector(STAT_POOL_SECTOR + target, &gSaveDataBuffer))
		return FALSE;
	if (fresh)
		s->committedSel = 0;               //まだレポートで確定していない: 区画 0 を書いたが「正」は未決定（committedSel=0）。次の書き込みは sel==committed で反対側の区画 1 へ（区画 0 を壊さない）。レポートのあと AfterSave で sel に揃う
	s->magic = SPL1_MAGIC;
	s->version = 1;
	s->gen = gen;
	s->sum = w->sum;
	s->sel = target;
	s->count = w->count;
	return TRUE;
}

static void SetEntry(struct StatPoolEntry* e, u32 pid, u16 ot, const u16* ev, const u16* iv)
{
	u8 i;

	e->pid = pid;
	for (i = 0; i < STATPOOL_STATS; ++i)
	{
		e->ev[i] = ev[i];
		e->iv[i] = iv[i];
	}
	e->spare = OtKey(ot);
	e->check = EntryCheck(e);
}

//(pid, ot) の件の位置。親まで合う件を優先し、無ければ旧件（spare==0）を返す（Store が昇格させる）
static s16 IndexIn(const struct StatPoolBlock* w, u32 pid, u16 ot)
{
	u8 i;
	s16 legacy = -1;

	for (i = 0; i < w->count; ++i)
		if (Match(&w->e[i], pid, ot))
		{
			if (w->e[i].spare != 0)
				return i;
			if (legacy < 0)
				legacy = i;
		}
	return legacy;
}

static void RemoveAt(struct StatPoolBlock* w, u8 i)
{
	if (i + 1 < w->count)
		w->e[i] = w->e[w->count - 1];       //最後の件を穴へ（件は詰めて持つ）
	Zero(&w->e[w->count - 1], sizeof(w->e[0]));
	--w->count;
}

void HackStatPool_Remove(u32 pid, u16 ot)
{
	s16 i;

	if (!HackStatPool_NotEmpty() || !FindIndex(SV, pid, ot, NULL, NULL) || !LoadWork())
		return;
	i = IndexIn(Work(), pid, ot);
	if (i < 0)
		return;
	RemoveAt(Work(), i);
	Commit();
}

u8 HackStatPool_Sweep(void)
{
	struct StatPoolOwner owners[40];      //8 バイト×40＝スタック 1KB の決まりの範囲（持ち主は最大 33 匹ぶん）
	u8 n, i, k, removed = 0;
	struct StatPoolBlock* w;

	if (!HackStatPool_NotEmpty() || !StateOk(SV))
		return 0;
	n = HackStatPool_CollectOwners(owners, 40);
	if (n == 0 || !LoadWork())                //持ち主が 1 匹も数えられないときは何も落とさない（列挙の失敗で全部消さない）
		return 0;
	w = Work();
	for (i = 0; i < w->count;)
	{
		bool8 owned = FALSE;

		for (k = 0; k < n; ++k)
			if (Match(&w->e[i], owners[k].pid, owners[k].ot))      //旧件は pid が合う持ち主が 1 匹でもいれば残す
				owned = TRUE;
		if (owned)
			++i;
		else
		{
			RemoveAt(w, i);
			++removed;
		}
	}
	if (removed != 0 && !Commit())
		return 0;
	return removed;
}

void HackStatPool_ReportBegin(void)
{
	if (HackStatPool_Count() >= STATPOOL_SWEEP_AT)
		HackStatPool_Sweep();
}

bool8 HackStatPool_Store(u32 pid, u16 ot, const u16* ev, const u16* iv)
{
	struct StatPoolEntry cur;
	struct StatPoolBlock* w;
	s16 i;
	u8 k;

	if (pid == 0)
		return FALSE;
	if (!AnyWide(ev, iv))
	{
		HackStatPool_Remove(pid, ot);      //全部入れ物に収まる: 件は要らない（表が空なら何もしない）
		return TRUE;
	}
	if (!BoxShelf_ExtAvailable())
		return FALSE;                      //使えない環境: 影（頭打ち）のまま
	if (FindIndex(SV, pid, ot, &cur, NULL) && cur.spare == OtKey(ot))
	{
		for (k = 0; k < STATPOOL_STATS && cur.ev[k] == ev[k] && cur.iv[k] == iv[k]; ++k);
		if (k == STATPOOL_STATS)
			return TRUE;                   //同じ内容: 書かない
	}
	if (HackStatPool_Count() >= STATPOOL_CAPACITY)
		HackStatPool_Sweep();              //満杯: 持ち主のいない件を落としてから取る
	if (!LoadWork())
		return FALSE;
	w = Work();
	i = IndexIn(w, pid, ot);               //旧件ならここで 1 匹だけが引き継ぎ（SetEntry が親の鍵を書く＝昇格）。同じ性格値のもう 1 匹は新しい件になる
	if (i < 0)
	{
		if (w->count >= STATPOOL_CAPACITY)
			return FALSE;                  //それでも満杯: 入れずに頭打ちのまま
		i = w->count++;
	}
	SetEntry(&w->e[i], pid, ot, ev, iv);
	return Commit();
}

//---- 値を書いたあとの更新 ----

static void Merge(u32 pid, u16 ot, u16* ev, u16* iv, const u8* shadowEv, const u8* shadowIv)
{
	u16 oe[STATPOOL_STATS], oi[STATPOOL_STATS];
	bool8 had = HackStatPool_Find(pid, ot, oe, oi);
	u8 i;

	for (i = 0; i < STATPOOL_STATS; ++i)
	{
		//表の値は、影が入れ物の最大のときだけ（かつ頭打ちの値が影と同じなら）採る。それ以外は影が正
		ev[i] = (had && shadowEv[i] == STATPOOL_EV_WIDTH && oe[i] >= STATPOOL_EV_WIDTH) ? oe[i] : shadowEv[i];
		iv[i] = (had && shadowIv[i] == STATPOOL_IV_WIDTH && oi[i] >= STATPOOL_IV_WIDTH) ? oi[i] : shadowIv[i];
	}
}

void HackStatPool_NoteEv(u32 pid, u16 ot, u8 stat, u16 value, const u8* shadowEv, const u8* shadowIv)
{
	u16 ev[STATPOOL_STATS], iv[STATPOOL_STATS];

	if (stat >= STATPOOL_STATS || (value <= STATPOOL_EV_WIDTH && !HackStatPool_NotEmpty()))
		return;
	Merge(pid, ot, ev, iv, shadowEv, shadowIv);
	ev[stat] = value;
	HackStatPool_Store(pid, ot, ev, iv);
}

void HackStatPool_NoteIv(u32 pid, u16 ot, u8 stat, u16 value, const u8* shadowEv, const u8* shadowIv)
{
	u16 ev[STATPOOL_STATS], iv[STATPOOL_STATS];

	if (stat >= STATPOOL_STATS || (value <= STATPOOL_IV_WIDTH && !HackStatPool_NotEmpty()))
		return;
	Merge(pid, ot, ev, iv, shadowEv, shadowIv);
	iv[stat] = value;
	HackStatPool_Store(pid, ot, ev, iv);
}

//---- つづきから・レポート ----

void HackStatPool_OnLoad(void)
{
	struct StatPoolSave* s = SV;

	if (s->magic == SPL1_MAGIC)
		s->committedSel = s->sel;          //つづきから: 読んだ sel が正（空のセーブは 1 バイトも書かない）
}

void HackStatPool_AfterSave(bool8 ok)
{
	struct StatPoolSave* s = SV;

	if (ok && s->magic == SPL1_MAGIC)
		s->committedSel = s->sel;
}
