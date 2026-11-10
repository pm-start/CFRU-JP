#include "defines.h"
#include "../include/save.h"
#include "../include/pokemon.h"
#include "hack_ext_save.h"
#include "hack_box_shelf.h"
#include "hack_mon_id.h"
#include "hack_condition.h"

/*
 * ポケモンのコンディション 6 値の表（[hack] FEAT-pokeblock 段 3・patches/0978）。置き場・形・版の守りは hack_condition.h。
 * 作りは hack_ribbon.c（リボンの表・patches/0977）と同じ（交互 2 区画×ブロック）:
 *   書き込みは「前のレポートで正だった区画」でないほうへ書き、SaveBlock1 の sel／gen／count を更新する。
 *   使えない環境・壊れた区画・世代の合わない区画は「表は空」と見なす。読み書きは HackExtSave_*（スロットのバンクを通る）だけ。
 *   リボンとの違い: 手持ちの形（80 バイト）とのビットのやり取りは無く、鍵（ポケモンの ID）で直接 6 値を引く。
 */

#define CND1_MAGIC 0x43               //'C'
#define CNDB_MAGIC 0x42444E43         //'CNDB'
#define CONDITION_SIGNATURE 0x08012025   //ほかの追加の区画と同じ
#define CONDITION_CHUNK 8
#define CND_SECTOR(b, sel) (CONDITION_SECTOR + (b) * 2 + ((sel) & 1))

typedef char ConditionEntrySize[(sizeof(struct ConditionEntry) == 12) ? 1 : -1];
typedef char ConditionSaveSize[(sizeof(struct ConditionSave) == 8) ? 1 : -1];
typedef char ConditionBlockFits[(sizeof(struct ConditionBlock) <= 0xFF4 && sizeof(struct ConditionBlock) == 16 + 12 * CONDITION_CAPACITY) ? 1 : -1];
typedef char ConditionSb1Fits[(CONDITION_SB1_OFF == 0x3A44 && CONDITION_SB1_OFF + sizeof(struct ConditionSave) == 0x3A4C) ? 1 : -1];   //SaveBlock1 の +0x3A08 の予備領域（0x44 バイト）の最後の 8 バイト（+0x3A4C は rivalName）。これで予備は 0

#include "hack_save_version.h"
typedef char ConditionNeedsVersion[(HACK_SAVE_VERSION >= HACK_CONDITION_VERSION) ? 1 : -1];   //表を書く版以上でないと、古い版が表つきのセーブを「新しすぎる」と止められない

extern struct SaveSection gSaveDataBuffer;

#define SV ((struct ConditionSave*) ((u8*) gSaveBlock1 + CONDITION_SB1_OFF))

static u16 Sum16(const u8* p, u32 n)
{
	u32 i, t = 0x4E43;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 EntryCheck(const struct ConditionEntry* e)
{
	return Sum16((const u8*) e, 10);
}

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static bool8 StateOk(const struct ConditionSave* s)
{
	u8 b;

	if (s->magic != CND1_MAGIC)
		return FALSE;
	for (b = 0; b < CONDITION_BLOCKS; ++b)
		if (s->count[b] > CONDITION_CAPACITY)
			return FALSE;
	return TRUE;
}

static u8 SelOf(const struct ConditionSave* s, u8 b)
{
	return (s->selBits >> b) & 1;
}

bool8 HackCondition_Available(void)
{
	return BoxShelf_ExtAvailable();
}

bool8 HackCondition_NotEmpty(void)
{
	const struct ConditionSave* s = SV;

	return StateOk(s) && (s->count[0] != 0 || s->count[1] != 0);
}

static bool8 HeaderOk(const struct ConditionBlock* h, const struct ConditionSave* s, u8 b)
{
	return h->magic == CNDB_MAGIC && h->version == 1 && (u8) h->gen == s->gen[b] && h->count == s->count[b];
}

static bool8 FindEntry(const struct ConditionSave* s, u32 monId, struct ConditionEntry* out)
{
	u32 head[4];                 //頭の 16 バイトだけ（ConditionBlock 全体を積まない＝スタック 1KB の決まり）
	struct ConditionEntry buf[CONDITION_CHUNK];
	u8 b = monId & 1;
	u16 i;
	u8 j, n;

	if (!StateOk(s) || s->count[b] == 0 || monId == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	if (!HackExtSave_ReadKnownAvail(CND_SECTOR(b, SelOf(s, b)), 0, &head, 16) || !HeaderOk((const struct ConditionBlock*) head, s, b))
		return FALSE;
	for (i = 0; i < s->count[b]; i += CONDITION_CHUNK)
	{
		n = s->count[b] - i < CONDITION_CHUNK ? s->count[b] - i : CONDITION_CHUNK;
		if (!HackExtSave_ReadKnownAvail(CND_SECTOR(b, SelOf(s, b)), 16 + i * 12, buf, n * 12))
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

static struct ConditionBlock* Work(void)
{
	return (struct ConditionBlock*) gSaveDataBuffer.data;
}

//入れ物 gSaveDataBuffer にブロック b の今の表を読む。空・壊れ・世代違いは空の入れ物を作る。使えない環境は FALSE
static bool8 LoadWork(u8 b)
{
	struct ConditionSave* s = SV;
	struct ConditionBlock* w = Work();

	if (!BoxShelf_ExtAvailable())
		return FALSE;
	if (StateOk(s) && s->count[b] != 0 && HackExtSave_ReadSector(CND_SECTOR(b, SelOf(s, b)), &gSaveDataBuffer)
	 && HeaderOk(w, s, b) && Sum16((const u8*) w->e, s->count[b] * 12) == w->sum)
		return TRUE;
	Zero(&gSaveDataBuffer, sizeof(gSaveDataBuffer));
	w->magic = CNDB_MAGIC;
	w->version = 1;
	return TRUE;
}

//入れ物（件数 w->count）を、前のレポートの正でないほうの区画へ書き、SaveBlock1 の状態を更新する
static bool8 Commit(u8 b)
{
	struct ConditionSave* s = SV;
	struct ConditionBlock* w = Work();
	bool8 fresh = !StateOk(s);
	u8 sel = fresh ? 0 : SelOf(s, b);
	u8 committed = fresh ? 1 : ((s->selBits >> (2 + b)) & 1);      //空の状態からの最初の書き込みは区画 0 へ
	u8 target = (sel == committed) ? (sel ^ 1) : sel;
	u8 gen = fresh ? 1 : (u8) (s->gen[b] + 1);
	u32 i;

	for (i = w->count; i < CONDITION_CAPACITY; ++i)
		Zero(&w->e[i], sizeof(w->e[i]));
	w->magic = CNDB_MAGIC;
	w->version = 1;
	w->gen = gen;
	w->sum = Sum16((const u8*) w->e, w->count * 12);
	gSaveDataBuffer.signature = CONDITION_SIGNATURE;
	if (!HackExtSave_WriteSector(CND_SECTOR(b, target), &gSaveDataBuffer))
		return FALSE;
	if (fresh)
	{
		Zero(s, sizeof(*s));                //まだレポートで確定していない: 正は未決定（committed=0）。次の書き込みは反対側の区画へ（今書いた区画を壊さない）
		s->magic = CND1_MAGIC;
	}
	s->selBits = (s->selBits & ~(1 << b)) | (target << b);
	s->gen[b] = gen;
	s->count[b] = w->count;
	return TRUE;
}

static bool8 AllZero(const u8* v)
{
	u8 i;

	for (i = 0; i < CONDITION_COUNT_VALUES; ++i)
		if (v[i] != 0)
			return FALSE;
	return TRUE;
}

static void SetEntry(struct ConditionEntry* e, u32 monId, const u8* v)
{
	u8 i;

	e->monId = monId;
	for (i = 0; i < CONDITION_COUNT_VALUES; ++i)
		e->v[i] = v[i];
	e->check = EntryCheck(e);
}

static s16 IndexIn(const struct ConditionBlock* w, u32 monId)
{
	u16 i;

	for (i = 0; i < w->count; ++i)
		if (w->e[i].monId == monId)
			return i;
	return -1;
}

static void RemoveAt(struct ConditionBlock* w, u16 i)
{
	for (; i + 1 < w->count; ++i)
		w->e[i] = w->e[i + 1];              //古い順を保つ（満杯は先頭＝いちばん古い件を捨てる）
	Zero(&w->e[w->count - 1], sizeof(w->e[0]));
	--w->count;
}

static bool8 IsUsable(const struct BoxPokemon* mon)
{
	return mon->substruct0.species != SPECIES_NONE && !mon->substruct3.isEgg;
}

void HackCondition_Get(const struct BoxPokemon* mon, u8* out)
{
	struct ConditionEntry e;
	u32 id;
	u8 i;

	for (i = 0; i < CONDITION_COUNT_VALUES; ++i)
		out[i] = 0;
	if (!IsUsable(mon))
		return;
	id = HackMonId_Get(mon);
	if (id == 0 || !FindEntry(SV, id, &e))
		return;
	for (i = 0; i < CONDITION_COUNT_VALUES; ++i)
		out[i] = e.v[i];
}

u8 HackCondition_GetOne(const struct BoxPokemon* mon, u8 i)
{
	u8 v[CONDITION_COUNT_VALUES];

	if (i >= CONDITION_COUNT_VALUES)
		return 0;
	HackCondition_Get(mon, v);
	return v[i];
}

static bool8 StoreById(u32 monId, const u8* v)
{
	struct ConditionEntry cur;
	struct ConditionBlock* w;
	bool8 found;
	u8 b, i;
	s16 idx;

	if (monId == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	b = monId & 1;
	found = FindEntry(SV, monId, &cur);
	if (found)
	{
		for (i = 0; i < CONDITION_COUNT_VALUES; ++i)
			if (cur.v[i] != v[i])
				break;
		if (i == CONDITION_COUNT_VALUES)
			return TRUE;                   //同じ内容: 書かない
	}
	if (!found && AllZero(v))
		return TRUE;                       //無い件を消す: 何もしない
	if (!LoadWork(b))
		return FALSE;
	w = Work();
	idx = IndexIn(w, monId);
	if (AllZero(v))
	{
		if (idx < 0)
			return TRUE;
		RemoveAt(w, idx);
	}
	else
	{
		if (idx < 0)
		{
			if (w->count >= CONDITION_CAPACITY)
				RemoveAt(w, 0);            //満杯: いちばん古い件を捨ててから入れる
			idx = w->count++;
		}
		SetEntry(&w->e[idx], monId, v);
	}
	return Commit(b);
}

bool8 HackCondition_Set(struct BoxPokemon* mon, const u8* v)
{
	u32 id;

	if (!IsUsable(mon) || !BoxShelf_ExtAvailable())
		return FALSE;
	id = HackMonId_Get(mon);
	if (id == 0)
	{
		if (AllZero(v))
			return TRUE;                   //ID の無い子に「全部 0」を書く: 何もしない（ID も付けない）
		HackMonId_Ensure(mon);
		id = HackMonId_Get(mon);
	}
	return StoreById(id, v);
}

//---- つづきから・レポート ----

static void SyncCommitted(void)
{
	struct ConditionSave* s = SV;

	if (s->magic == CND1_MAGIC)
		s->selBits = (s->selBits & 0x03) | ((s->selBits & 0x03) << 2);
}

void HackCondition_OnLoad(void)
{
	SyncCommitted();                       //つづきから: 読んだ sel が正（空のセーブは 1 バイトも書かない）
}

void HackCondition_AfterSave(bool8 ok)
{
	if (ok)
		SyncCommitted();
}
