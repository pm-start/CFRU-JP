#include "defines.h"
#include "../include/save.h"
#include "hack_ext_save.h"
#include "hack_box_shelf.h"
#include "hack_move_count.h"

/*
 * 技進化（独自）の使用回数の表（[hack] patches/0878）。置き場・形・版の守りは hack_move_count.h と work/tmp/move_evo_counter/PLAN.md。
 * 作りは hack_stat_pool.c（上の桁の表・patches/0760）と同じ交互 2 区画を、ブロック（ポケモン ID % 4）ごとに持つ:
 *   書き込みは「前のレポートで正だった区画」でないほうへ書き、SaveBlock1 の sel／gen／count を更新する。
 *   SaveBlock1 はレポートで保存されるので、レポートの前にリセットすれば SaveBlock1 は前の sel（無傷の区画）に戻る。
 *   使えない環境・壊れた区画・世代の合わない区画は「表は空」と見なす（回数なし＝確率は基本% だけで動く）。
 *   読み書きは HackExtSave_*（スロットのバンクを通る）だけ。区画番号を物理番地にしない＝ぼうけんのしょ 1・2 で表は別々。
 */

#define MVC1_MAGIC 0x564D             //'MV'
#define MVCB_MAGIC 0x4243564D         //'MVCB'
#define MOVECOUNT_SIGNATURE 0x08012025   //ほかの追加の区画と同じ（道具が署名つきの区画として読める）
#define MOVECOUNT_CHUNK 8

typedef char MoveCountEntrySize[(sizeof(struct MoveCountEntry) == 20) ? 1 : -1];
typedef char MoveCountSaveSize[(sizeof(struct MoveCountSave) == 12) ? 1 : -1];
typedef char MoveCountBlockFits[(sizeof(struct MoveCountBlock) <= 0xFF4 && sizeof(struct MoveCountBlock) == 16 + 20 * MOVECOUNT_CAPACITY) ? 1 : -1];
typedef char MoveCountSb1Fits[(MOVECOUNT_SB1_OFF == 0x3A08 && MOVECOUNT_SB1_OFF + sizeof(struct MoveCountSave) <= 0x3A4C) ? 1 : -1];   //SaveBlock1 の filler_3A08（0x44）の先頭 12 バイト

#include "hack_save_version.h"
typedef char MoveCountNeedsVersion[(HACK_SAVE_VERSION >= HACK_MOVE_COUNT_VERSION) ? 1 : -1];   //表を書く版以上でないと、古い版が表つきのセーブを「新しすぎる」と止められない

extern struct SaveSection gSaveDataBuffer;

#define SV ((struct MoveCountSave*) ((u8*) gSaveBlock1 + MOVECOUNT_SB1_OFF))
#define SECTOR(b, sel) (MOVE_COUNT_SECTOR + (b) * 2 + ((sel) & 1))

static u16 Sum16(const u8* p, u32 n)
{
	u32 i, t = 0x4D56;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 EntryCheck(const struct MoveCountEntry* e)
{
	return Sum16((const u8*) e, 16);
}

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static bool8 StateOk(const struct MoveCountSave* s)
{
	u8 b;

	if (s->magic != MVC1_MAGIC || s->version != 1)
		return FALSE;
	for (b = 0; b < MOVECOUNT_BLOCKS; ++b)
		if (s->count[b] > MOVECOUNT_CAPACITY)
			return FALSE;
	return TRUE;
}

static u8 SelOf(const struct MoveCountSave* s, u8 b)
{
	return (s->selBits >> b) & 1;
}

bool8 HackMoveCount_Available(void)
{
	return BoxShelf_ExtAvailable();
}

bool8 HackMoveCount_NotEmpty(void)
{
	const struct MoveCountSave* s = SV;

	return StateOk(s) && HackMoveCount_Total() != 0;
}

u16 HackMoveCount_Total(void)
{
	const struct MoveCountSave* s = SV;
	u16 t = 0;
	u8 b;

	if (!StateOk(s))
		return 0;
	for (b = 0; b < MOVECOUNT_BLOCKS; ++b)
		t += s->count[b];
	return t;
}

static bool8 HeaderOk(const struct MoveCountBlock* h, const struct MoveCountSave* s, u8 b)
{
	return h->magic == MVCB_MAGIC && h->version == 1 && (u8) h->gen == s->gen[b] && h->count == s->count[b];
}

static bool8 Normalize(struct MoveCountRec* n, const struct MoveCountRec* r)
{
	u8 i;
	bool8 any = FALSE;

	n->monId = r->monId;
	n->flags = 0;
	for (i = 0; i < MOVECOUNT_SLOTS; ++i)
	{
		bool8 mastered = (r->flags >> i) & 1;               /* patches/0932 「じゅくれん」は回数 0 でも欄を残す */
		bool8 live = r->move[i] != 0 && (r->count[i] != 0 || mastered);

		n->move[i] = live ? r->move[i] : 0;
		n->count[i] = live ? r->count[i] : 0;
		if (live)
		{
			any = TRUE;
			if (mastered)
				n->flags |= 1 << i;
		}
	}
	return any;
}

static void FromEntry(struct MoveCountRec* out, const struct MoveCountEntry* e)
{
	u8 i;

	out->monId = e->monId;
	for (i = 0; i < MOVECOUNT_SLOTS; ++i)
	{
		out->move[i] = e->move[i];
		out->count[i] = e->count[i];
	}
	out->flags = e->spare & MOVECOUNT_FLAG_MASK;
}

static bool8 SameRec(const struct MoveCountRec* a, const struct MoveCountRec* b)
{
	u8 i;

	for (i = 0; i < MOVECOUNT_SLOTS; ++i)
		if (a->move[i] != b->move[i] || a->count[i] != b->count[i])
			return FALSE;
	return a->monId == b->monId && a->flags == b->flags;
}

//表の読み: 使えるときだけ。件ごとに検査（先頭 16 バイトの和）を見る
static bool8 FindEntry(const struct MoveCountSave* s, u32 monId, struct MoveCountEntry* out)
{
	u32 head[4];                 //頭の 16 バイトだけ（MoveCountBlock 全体を積まない＝スタック 1KB の決まり・6.35）
	struct MoveCountEntry buf[MOVECOUNT_CHUNK];
	u8 b = monId & 3;
	u8 i, j, n;

	if (!StateOk(s) || s->count[b] == 0 || monId == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	if (!HackExtSave_ReadKnownAvail(SECTOR(b, SelOf(s, b)), 0, &head, 16) || !HeaderOk((const struct MoveCountBlock*) head, s, b))
		return FALSE;
	for (i = 0; i < s->count[b]; i += MOVECOUNT_CHUNK)
	{
		n = s->count[b] - i < MOVECOUNT_CHUNK ? s->count[b] - i : MOVECOUNT_CHUNK;
		if (!HackExtSave_ReadKnownAvail(SECTOR(b, SelOf(s, b)), 16 + i * 20, buf, n * 20))
			return FALSE;
		for (j = 0; j < n; ++j)
			if (buf[j].monId == monId && buf[j].check == EntryCheck(&buf[j]))
			{
				if (out != NULL)
					*out = buf[j];
				return TRUE;
			}
	}
	return FALSE;
}

bool8 HackMoveCount_Find(u32 monId, struct MoveCountRec* out)
{
	struct MoveCountEntry e;

	if (!FindEntry(SV, monId, &e))
		return FALSE;
	if (out != NULL)
		FromEntry(out, &e);
	return TRUE;
}

//---- 書き ----

static struct MoveCountBlock* Work(void)
{
	return (struct MoveCountBlock*) gSaveDataBuffer.data;
}

//入れ物 gSaveDataBuffer にブロック b の今の表を読む。空・壊れ・世代違いは空の入れ物を作る。使えない環境は FALSE
static bool8 LoadWork(u8 b)
{
	struct MoveCountSave* s = SV;
	struct MoveCountBlock* w = Work();

	if (!BoxShelf_ExtAvailable())
		return FALSE;
	if (StateOk(s) && s->count[b] != 0 && HackExtSave_ReadSector(SECTOR(b, SelOf(s, b)), &gSaveDataBuffer)
	 && HeaderOk(w, s, b) && Sum16((const u8*) w->e, s->count[b] * 20) == w->sum)
		return TRUE;
	Zero(&gSaveDataBuffer, sizeof(gSaveDataBuffer));
	w->magic = MVCB_MAGIC;
	w->version = 1;
	return TRUE;
}

//入れ物 gSaveDataBuffer（件数 w->count）を、前のレポートの正でないほうの区画へ書き、SaveBlock1 の状態を更新する
static bool8 Commit(u8 b)
{
	struct MoveCountSave* s = SV;
	struct MoveCountBlock* w = Work();
	bool8 fresh = !StateOk(s);
	u8 sel = fresh ? 0 : SelOf(s, b);
	u8 committed = fresh ? 1 : ((s->selBits >> (4 + b)) & 1);      //空の状態からの最初の書き込みは区画 0 へ
	u8 target = (sel == committed) ? (sel ^ 1) : sel;
	u8 gen = fresh ? 1 : (u8) (s->gen[b] + 1);
	u32 i;

	for (i = w->count; i < MOVECOUNT_CAPACITY; ++i)
		Zero(&w->e[i], sizeof(w->e[i]));
	w->magic = MVCB_MAGIC;
	w->version = 1;
	w->gen = gen;
	w->sum = Sum16((const u8*) w->e, w->count * 20);
	gSaveDataBuffer.signature = MOVECOUNT_SIGNATURE;
	if (!HackExtSave_WriteSector(SECTOR(b, target), &gSaveDataBuffer))
		return FALSE;
	if (fresh)
	{
		Zero(s, sizeof(*s));                //まだレポートで確定していない: 正は未決定（committed=0）。次の書き込みは sel==committed で反対側の区画へ（今書いた区画を壊さない）。レポートのあと AfterSave で sel に揃う
		s->magic = MVC1_MAGIC;
		s->version = 1;
	}
	s->selBits = (s->selBits & ~(1 << b)) | (target << b);
	s->gen[b] = gen;
	s->count[b] = w->count;
	return TRUE;
}

static void SetEntry(struct MoveCountEntry* e, const struct MoveCountRec* r)
{
	u8 i;

	e->monId = r->monId;
	for (i = 0; i < MOVECOUNT_SLOTS; ++i)
	{
		e->move[i] = r->move[i];
		e->count[i] = r->count[i];
	}
	e->spare = r->flags & MOVECOUNT_FLAG_MASK;
	e->check = EntryCheck(e);
}

static s16 IndexIn(const struct MoveCountBlock* w, u32 monId)
{
	u8 i;

	for (i = 0; i < w->count; ++i)
		if (w->e[i].monId == monId)
			return i;
	return -1;
}

static u16 TotalOf(const struct MoveCountEntry* e)
{
	u16 t = 0;
	u8 i;

	for (i = 0; i < MOVECOUNT_SLOTS; ++i)
		t += e->count[i];
	if (e->spare & MOVECOUNT_FLAG_MASK)
		t += 2000;                         /* 「じゅくれん」の件は最後に捨てる（patches/0932・J8）。最大 255 × 4 + 2000 < 0xFFFF */
	return t;
}

static void RemoveAt(struct MoveCountBlock* w, u8 i)
{
	for (; i + 1 < w->count; ++i)
		w->e[i] = w->e[i + 1];              //古い順を保つ（満杯の捨て方は「少なくて古い」）
	Zero(&w->e[w->count - 1], sizeof(w->e[0]));
	--w->count;
}

//満杯: 回数の合計がいちばん少ない件（同じなら古いほう）を捨てる
static void DropLeast(struct MoveCountBlock* w)
{
	u8 i, least = 0;
	u16 best = 0xFFFF;

	for (i = 0; i < w->count; ++i)
	{
		u16 t = TotalOf(&w->e[i]);

		if (t < best)
		{
			best = t;
			least = i;
		}
	}
	RemoveAt(w, least);
}

bool8 HackMoveCount_Store(const struct MoveCountRec* rec)
{
	struct MoveCountRec n, was;
	struct MoveCountEntry cur;
	struct MoveCountBlock* w;
	bool8 any, found;
	u8 b;
	s16 i;

	if (rec->monId == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	b = rec->monId & 3;
	any = Normalize(&n, rec);
	found = FindEntry(SV, rec->monId, &cur);
	if (found)
	{
		FromEntry(&was, &cur);
		if (any && SameRec(&was, &n))
			return TRUE;                   //同じ内容: 書かない
	}
	else if (!any)
		return TRUE;                       //無い件を消す: 何もしない
	if (!LoadWork(b))
		return FALSE;
	w = Work();
	i = IndexIn(w, rec->monId);
	if (!any)
	{
		if (i < 0)
			return TRUE;
		RemoveAt(w, i);
	}
	else
	{
		if (i < 0)
		{
			if (w->count >= MOVECOUNT_CAPACITY)
				DropLeast(w);              //満杯: 少なくて古い件を捨ててから入れる
			i = w->count++;
		}
		SetEntry(&w->e[i], &n);
	}
	return Commit(b);
}

//---- つづきから・レポート ----

static void SyncCommitted(void)
{
	struct MoveCountSave* s = SV;

	if (s->magic == MVC1_MAGIC)
		s->selBits = (s->selBits & 0x0F) | ((s->selBits & 0x0F) << 4);
}

void HackMoveCount_OnLoad(void)
{
	SyncCommitted();                       //つづきから: 読んだ sel が正（空のセーブは 1 バイトも書かない）
}

void HackMoveCount_AfterSave(bool8 ok)
{
	if (ok)
		SyncCommitted();
}
