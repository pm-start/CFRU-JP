#include "defines.h"
#include "../include/save.h"
#include "../include/pokemon.h"
#include "hack_ext_save.h"
#include "hack_box_shelf.h"
#include "hack_mon_id.h"
#include "hack_ribbon.h"

/*
 * リボンの表（[hack] patches/0977）。置き場・形・版の守りは hack_ribbon.h。作りは hack_move_count.c（交互 2 区画×ブロック）と同じ:
 *   書き込みは「前のレポートで正だった区画」でないほうへ書き、SaveBlock1 の sel／gen／count を更新する。
 *   使えない環境・壊れた区画・世代の合わない区画は「表は空」と見なす。読み書きは HackExtSave_*（スロットのバンクを通る）だけ。
 */

#define RIB1_MAGIC 0x4252             //'RB'
#define RIBB_MAGIC 0x424E4252         //'RBNB'
#define RIBBON_SIGNATURE 0x08012025   //ほかの追加の区画と同じ
#define RIBBON_CHUNK 8
#define RIB_SECTOR(b, sel) (RIBBON_SECTOR + (b) * 2 + ((sel) & 1))

typedef char RibbonEntrySize[(sizeof(struct RibbonEntry) == 12) ? 1 : -1];
typedef char RibbonSaveSize[(sizeof(struct RibbonSave) == 10) ? 1 : -1];
typedef char RibbonBlockFits[(sizeof(struct RibbonBlock) <= 0xFF4 && sizeof(struct RibbonBlock) == 16 + 12 * RIBBON_CAPACITY) ? 1 : -1];
typedef char RibbonSb1Fits[(RIBBON_SB1_OFF == 0x3A2A && RIBBON_SB1_OFF + sizeof(struct RibbonSave) <= 0x3A34) ? 1 : -1];   //SaveBlock1 の +0x3A08 の予備領域（0x44 バイト）の +0x22〜+0x2B（リライブボックスの予約の次・+0x3A34 から先は別の機能に空けてある）

#include "hack_save_version.h"
typedef char RibbonNeedsVersion[(HACK_SAVE_VERSION >= HACK_RIBBON_VERSION) ? 1 : -1];   //表を書く版以上でないと、古い版が表つきのセーブを「新しすぎる」と止められない

extern struct SaveSection gSaveDataBuffer;

#define SV ((struct RibbonSave*) ((u8*) gSaveBlock1 + RIBBON_SB1_OFF))

static u16 Sum16(const u8* p, u32 n)
{
	u32 i, t = 0x5242;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 EntryCheck(const struct RibbonEntry* e)
{
	return Sum16((const u8*) e, 8);
}

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static bool8 StateOk(const struct RibbonSave* s)
{
	u8 b;

	if (s->magic != RIB1_MAGIC || s->version != 1)
		return FALSE;
	for (b = 0; b < RIBBON_BLOCKS; ++b)
		if (s->count[b] > RIBBON_CAPACITY)
			return FALSE;
	return TRUE;
}

static u8 SelOf(const struct RibbonSave* s, u8 b)
{
	return (s->selBits >> b) & 1;
}

bool8 HackRibbon_Available(void)
{
	return BoxShelf_ExtAvailable();
}

bool8 HackRibbon_NotEmpty(void)
{
	const struct RibbonSave* s = SV;

	return StateOk(s) && (s->count[0] != 0 || s->count[1] != 0);
}

static bool8 HeaderOk(const struct RibbonBlock* h, const struct RibbonSave* s, u8 b)
{
	return h->magic == RIBB_MAGIC && h->version == 1 && (u8) h->gen == s->gen[b] && h->count == s->count[b];
}

static bool8 FindEntry(const struct RibbonSave* s, u32 monId, struct RibbonEntry* out)
{
	u32 head[4];                 //頭の 16 バイトだけ（RibbonBlock 全体を積まない＝スタック 1KB の決まり）
	struct RibbonEntry buf[RIBBON_CHUNK];
	u8 b = monId & 1;
	u16 i;
	u8 j, n;

	if (!StateOk(s) || s->count[b] == 0 || monId == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	if (!HackExtSave_ReadKnownAvail(RIB_SECTOR(b, SelOf(s, b)), 0, &head, 16) || !HeaderOk((const struct RibbonBlock*) head, s, b))
		return FALSE;
	for (i = 0; i < s->count[b]; i += RIBBON_CHUNK)
	{
		n = s->count[b] - i < RIBBON_CHUNK ? s->count[b] - i : RIBBON_CHUNK;
		if (!HackExtSave_ReadKnownAvail(RIB_SECTOR(b, SelOf(s, b)), 16 + i * 12, buf, n * 12))
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

bool8 HackRibbon_Find(u32 monId, u32* ribbons)
{
	struct RibbonEntry e;

	if (!FindEntry(SV, monId, &e))
		return FALSE;
	if (ribbons != NULL)
		*ribbons = e.ribbons & RIBBON_MASK;
	return TRUE;
}

static struct RibbonBlock* Work(void)
{
	return (struct RibbonBlock*) gSaveDataBuffer.data;
}

//入れ物 gSaveDataBuffer にブロック b の今の表を読む。空・壊れ・世代違いは空の入れ物を作る。使えない環境は FALSE
static bool8 LoadWork(u8 b)
{
	struct RibbonSave* s = SV;
	struct RibbonBlock* w = Work();

	if (!BoxShelf_ExtAvailable())
		return FALSE;
	if (StateOk(s) && s->count[b] != 0 && HackExtSave_ReadSector(RIB_SECTOR(b, SelOf(s, b)), &gSaveDataBuffer)
	 && HeaderOk(w, s, b) && Sum16((const u8*) w->e, s->count[b] * 12) == w->sum)
		return TRUE;
	Zero(&gSaveDataBuffer, sizeof(gSaveDataBuffer));
	w->magic = RIBB_MAGIC;
	w->version = 1;
	return TRUE;
}

//入れ物（件数 w->count）を、前のレポートの正でないほうの区画へ書き、SaveBlock1 の状態を更新する
static bool8 Commit(u8 b)
{
	struct RibbonSave* s = SV;
	struct RibbonBlock* w = Work();
	bool8 fresh = !StateOk(s);
	u8 sel = fresh ? 0 : SelOf(s, b);
	u8 committed = fresh ? 1 : ((s->selBits >> (2 + b)) & 1);      //空の状態からの最初の書き込みは区画 0 へ
	u8 target = (sel == committed) ? (sel ^ 1) : sel;
	u8 gen = fresh ? 1 : (u8) (s->gen[b] + 1);
	u32 i;

	for (i = w->count; i < RIBBON_CAPACITY; ++i)
		Zero(&w->e[i], sizeof(w->e[i]));
	w->magic = RIBB_MAGIC;
	w->version = 1;
	w->gen = gen;
	w->sum = Sum16((const u8*) w->e, w->count * 12);
	gSaveDataBuffer.signature = RIBBON_SIGNATURE;
	if (!HackExtSave_WriteSector(RIB_SECTOR(b, target), &gSaveDataBuffer))
		return FALSE;
	if (fresh)
	{
		Zero(s, sizeof(*s));                //まだレポートで確定していない: 正は未決定（committed=0）。次の書き込みは反対側の区画へ（今書いた区画を壊さない）
		s->magic = RIB1_MAGIC;
		s->version = 1;
	}
	s->selBits = (s->selBits & ~(1 << b)) | (target << b);
	s->gen[b] = gen;
	s->count[b] = w->count;
	return TRUE;
}

static void SetEntry(struct RibbonEntry* e, u32 monId, u32 ribbons)
{
	e->monId = monId;
	e->ribbons = ribbons;
	e->spare = 0;
	e->check = EntryCheck(e);
}

static s16 IndexIn(const struct RibbonBlock* w, u32 monId)
{
	u16 i;

	for (i = 0; i < w->count; ++i)
		if (w->e[i].monId == monId)
			return i;
	return -1;
}

static void RemoveAt(struct RibbonBlock* w, u16 i)
{
	for (; i + 1 < w->count; ++i)
		w->e[i] = w->e[i + 1];              //古い順を保つ（満杯は先頭＝いちばん古い件を捨てる）
	Zero(&w->e[w->count - 1], sizeof(w->e[0]));
	--w->count;
}

bool8 HackRibbon_Store(u32 monId, u32 ribbons)
{
	struct RibbonEntry cur;
	struct RibbonBlock* w;
	bool8 found;
	u8 b;
	s16 i;

	ribbons &= RIBBON_MASK;
	if (monId == 0 || !BoxShelf_ExtAvailable())
		return FALSE;
	b = monId & 1;
	found = FindEntry(SV, monId, &cur);
	if (found && (cur.ribbons & RIBBON_MASK) == ribbons)
		return TRUE;                       //同じ内容: 書かない
	if (!found && ribbons == 0)
		return TRUE;                       //無い件を消す: 何もしない
	if (!LoadWork(b))
		return FALSE;
	w = Work();
	i = IndexIn(w, monId);
	if (ribbons == 0)
	{
		if (i < 0)
			return TRUE;
		RemoveAt(w, i);
	}
	else
	{
		if (i < 0)
		{
			if (w->count >= RIBBON_CAPACITY)
				RemoveAt(w, 0);            //満杯: いちばん古い件を捨ててから入れる
			i = w->count++;
		}
		SetEntry(&w->e[i], monId, ribbons);
	}
	return Commit(b);
}

//---- 手持ち／80 バイト形との橋 ----

u32 HackRibbon_Bits(const struct BoxPokemon* mon)
{
	u32 v;

	Memcpy(&v, (const u8*) &mon->substruct3 + 8, 4);
	return v & RIBBON_MASK;
}

static void SetBits(struct BoxPokemon* mon, u32 bits)
{
	u32 v;

	Memcpy(&v, (const u8*) &mon->substruct3 + 8, 4);
	v = (v & ~RIBBON_MASK) | (bits & RIBBON_MASK);
	Memcpy((u8*) &mon->substruct3 + 8, &v, 4);
}

void HackRibbon_AddBits(struct BoxPokemon* mon, u32 bits)
{
	SetBits(mon, HackRibbon_Bits(mon) | bits);
}

//ふしぎなおくりものの配布リボン（エメラルドの GiveGiftRibbonToParty・giftRibbons[1〜7]）の口。手持ちの index 番の子に 80 バイト形の bit 20 + gift - 1 を付ける。
//呼び手はまだ無い（ふしぎなおくりものの台本からは作らない・FEAT-relive-ribbon 段 3）。付いた（新しく）なら TRUE
bool8 HackRibbon_GiveGift(u8 index, u8 gift)
{
	struct BoxPokemon* box;
	u32 bit;

	if (index >= PARTY_SIZE || gift < 1 || gift > 7)
		return FALSE;
	box = (struct BoxPokemon*) &gPlayerParty[index];
	if (box->substruct0.species == SPECIES_NONE || box->substruct3.isEgg)
		return FALSE;
	bit = 1u << (20 + gift - 1);
	if (HackRibbon_Bits(box) & bit)
		return FALSE;
	HackRibbon_AddBits(box, bit);
	return TRUE;
}

void HackRibbon_StoreFromMon(const struct BoxPokemon* mon)
{
	u32 id, bits;

	if (mon->substruct0.species == SPECIES_NONE || mon->substruct3.isEgg)
		return;
	bits = HackRibbon_Bits(mon);
	if (bits == 0 && !HackRibbon_NotEmpty())
		return;                            //表が空で付けるものも無い: 何もしない（熱い道・セーブも変わらない）
	id = HackMonId_Get(mon);
	if (id == 0)
		return;
	HackRibbon_Store(id, bits);
}

void HackRibbon_RestoreToMon(struct Pokemon* mon)
{
	struct BoxPokemon* box = (struct BoxPokemon*) mon;
	u32 id, bits;

	if (!HackRibbon_NotEmpty() || box->substruct0.species == SPECIES_NONE || box->substruct3.isEgg)
		return;
	id = HackMonId_Get(box);
	if (id != 0 && HackRibbon_Find(id, &bits))
		SetBits(box, HackRibbon_Bits(box) | bits);
}

//---- つづきから・レポート ----

static void SyncCommitted(void)
{
	struct RibbonSave* s = SV;

	if (s->magic == RIB1_MAGIC)
		s->selBits = (s->selBits & 0x03) | ((s->selBits & 0x03) << 2);
}

void HackRibbon_OnLoad(void)
{
	SyncCommitted();                       //つづきから: 読んだ sel が正（空のセーブは 1 バイトも書かない）
}

void HackRibbon_AfterSave(bool8 ok)
{
	if (ok)
		SyncCommitted();
}
