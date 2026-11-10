#include "defines.h"
#include "../include/save.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "hack_ext_save.h"
#include "hack_relive_box.h"

/*
 * リライブボックス（2026-11-09 FEAT-relive-box-ext・patches/0975・ユーザー決定「案B」）。見本は src/hack_egg_box.c（卵専用ボックス）と同じ置き方:
 *   - 中身は追加の区画 RELIVE_BOX_SECTOR と +1 の「交互 2 区画」（ブロック 1 つ＝ReliveBlock・約 2.4KB・30 匹・箱の縮めた形 78 バイトのまま）。空きは species == 0。
 *   - 「どちらが正か」「入っている数」は SaveBlock1 + 0x3A22（global.h の +0x3A08 の帯の未使用 8 バイトのうち 6 バイト）に持つ＝本体の組の中なので、レポートで手持ちと一緒に書かれる。
 *   - 変えるたびに「前のレポートで正だった区画でないほう」へ書き、sel をそちらへ向ける。レポートの前にリセットすれば SaveBlock1 は前の sel（無傷の区画）に戻る。
 *   - 入っている数 0 のときは区画の中身を信じない（はじめから・別のセーブの古い中身を拾わない）。
 *   - 状態は「使うまで書かない」: 読むだけの関数（Count）は SaveBlock1 を書き換えない（ダークのいないセーブは 1 バイトも変わらない）。
 */

#define RLB_SB1_MAGIC 0x4C52         //'RL'
#define RLB_BLOCK_MAGIC 0x58424C52   //'RLBX'
#define RLB_BLOCK_VERSION 1
#define RLB_SB1_OFF 0x3A22
#define RLB_SIGNATURE 0x08012025     //ほかの追加の区画と同じ

struct ReliveBoxSave                 //SaveBlock1 + 0x3A22（6 バイト・本体の組に入る）
{
	u16 magic;
	u8 sel;                          //今の中身がある区画（0 = RELIVE_BOX_SECTOR・1 = +1）
	u8 count;                        //入っている数
	u8 version;
	u8 committedSel;                 //最後に書き終えたレポートの sel（つづきから／レポートのあとに揃える）
};

struct ReliveBlock                   //追加の区画 1 つに書く（先頭から）
{
	u32 magic;
	u16 version;
	u16 sum;                         //先頭 16 バイトより後ろの和
	u32 spare[2];
	struct CompressedPokemon mons[RELIVE_BOX_SLOTS];
};

#define RLB_SB1 ((struct ReliveBoxSave*) ((u8*) gSaveBlock1 + RLB_SB1_OFF))

typedef char ReliveBoxSaveFits[(sizeof(struct ReliveBoxSave) == 6 && RLB_SB1_OFF == 0x3A22 && RLB_SB1_OFF + 8 <= 0x3A4C) ? 1 : -1];   //未使用の帯（歩数・印の次・rivalName の手前）に収まる
typedef char ReliveBlockFits[(sizeof(struct ReliveBlock) <= 0xFF4) ? 1 : -1];                                                         //gSaveDataBuffer.data（0xFF4）に収まる

extern struct SaveSection gSaveDataBuffer;

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static bool8 StateOk(const struct ReliveBoxSave* s)
{
	return s->magic == RLB_SB1_MAGIC && s->version == 1 && s->count <= RELIVE_BOX_SLOTS && s->sel <= 1 && s->committedSel <= 1;
}

//書く側（Commit）だけが呼ぶ: 印が合わなければ空の状態で作り直す。sel = committedSel = 1 から始める＝最初の書き込みは区画 0（target = sel ^ 1）へ
static struct ReliveBoxSave* SvForWrite(void)
{
	struct ReliveBoxSave* s = RLB_SB1;

	if (!StateOk(s))
	{
		Zero(s, sizeof(*s));
		s->magic = RLB_SB1_MAGIC;
		s->version = 1;
		s->sel = 1;
		s->committedSel = 1;
	}
	return s;
}

static u16 SumBytes(const void* b, u32 size)
{
	const u8* p = (const u8*) b + 16;
	u32 n = size - 16, i, t = 0x4C52;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 BlockSum(const struct ReliveBlock* b)
{
	return SumBytes(b, sizeof(*b));
}

static u8 CountOf(const struct ReliveBlock* b)
{
	u8 i, n = 0;

	for (i = 0; i < RELIVE_BOX_SLOTS; ++i)
		if (b->mons[i].substruct0.species != 0)
			++n;
	return n;
}

u8 ReliveBox_Usable(void)
{
	u8 check = HackExtSave_Check();

	if (check == 1)
		return RELIVE_BOX_OK;
	return check == 2 ? RELIVE_BOX_NEED_REPORT : RELIVE_BOX_NO_EXT;
}

u8 ReliveBox_Count(void)
{
	const struct ReliveBoxSave* s = RLB_SB1;

	return StateOk(s) ? s->count : 0;
}

u32 ReliveBox_ImageSize(void)
{
	return sizeof(struct ReliveBlock);
}

void* ReliveBox_ScratchImage(void)
{
	return &gSaveDataBuffer;
}

bool8 ReliveBox_SlotUsed(const void* image, u8 slot)
{
	return slot < RELIVE_BOX_SLOTS && ((const struct ReliveBlock*) image)->mons[slot].substruct0.species != 0;
}

struct CompressedPokemon* ReliveBox_SlotMon(void* image, u8 slot)
{
	return slot < RELIVE_BOX_SLOTS ? &((struct ReliveBlock*) image)->mons[slot] : NULL;
}

bool8 ReliveBox_Load(void* image)
{
	struct ReliveBlock* b = image;
	const struct ReliveBoxSave* s = RLB_SB1;
	bool8 read;

	if (!StateOk(s) || s->count == 0)
	{
		Zero(b, sizeof(*b));               //空（区画の古い中身は信じない）
		b->magic = RLB_BLOCK_MAGIC;
		b->version = RLB_BLOCK_VERSION;
		return TRUE;
	}
	if (image == &gSaveDataBuffer)
		read = HackExtSave_ReadSector(RELIVE_BOX_SECTOR + (s->sel & 1), &gSaveDataBuffer);
	else
		read = HackExtSave_Read(RELIVE_BOX_SECTOR + (s->sel & 1), 0, b, sizeof(*b));
	if (!read || b->magic != RLB_BLOCK_MAGIC || b->version != RLB_BLOCK_VERSION || b->sum != BlockSum(b))
	{
		Zero(b, sizeof(*b));
		b->magic = RLB_BLOCK_MAGIC;
		b->version = RLB_BLOCK_VERSION;
		return FALSE;
	}
	return TRUE;
}

//入れ物 image を、「前のレポートで正だった区画」でないほうへ書き、sel をそちらへ向ける。書けなければ FALSE（sel・数は変えない）
bool8 ReliveBox_Commit(void* image)
{
	struct ReliveBlock* b = image;
	bool8 wasOk = StateOk(RLB_SB1);
	struct ReliveBoxSave* s = SvForWrite();
	u8 target = (s->sel == s->committedSel) ? (s->sel ^ 1) : s->sel;
	u8* dst = (u8*) &gSaveDataBuffer;
	const u8* src = (const u8*) b;
	u32 i;

	b->magic = RLB_BLOCK_MAGIC;
	b->version = RLB_BLOCK_VERSION;
	b->sum = BlockSum(b);
	if (image != &gSaveDataBuffer)
	{
		for (i = 0; i < sizeof(gSaveDataBuffer); ++i)
			dst[i] = 0;
		for (i = 0; i < sizeof(*b); ++i)
			dst[i] = src[i];
	}
	else
	{
		for (i = sizeof(*b); i < sizeof(gSaveDataBuffer); ++i)
			dst[i] = 0;                    //ブロックの外は 0（古い読み込みの残りを書かない）
	}
	gSaveDataBuffer.signature = RLB_SIGNATURE;
	if (!HackExtSave_WriteSector(RELIVE_BOX_SECTOR + target, &gSaveDataBuffer))
	{
		if (!wasOk)
			Zero(s, sizeof(*s));           //書けなかったときは状態も作らない（空のセーブは 1 バイトも変わらない）
		return FALSE;
	}
	s->sel = target;
	s->count = CountOf(b);
	return TRUE;
}

bool8 ReliveBox_Put(void* image, const struct CompressedPokemon* mon, u8* slotOut)
{
	struct ReliveBlock* b = image;
	const u8* s = (const u8*) mon;
	u8* m;
	u8 slot;
	u32 i;

	for (slot = 0; slot < RELIVE_BOX_SLOTS && b->mons[slot].substruct0.species != 0; ++slot);
	if (slot >= RELIVE_BOX_SLOTS || mon->substruct0.species == 0)
		return FALSE;
	m = (u8*) &b->mons[slot];
	for (i = 0; i < sizeof(*mon); ++i)
		m[i] = s[i];
	if (!ReliveBox_Commit(b))
	{
		Zero(m, sizeof(*mon));
		return FALSE;
	}
	if (slotOut != NULL)
		*slotOut = slot;
	return TRUE;
}

bool8 ReliveBox_Take(void* image, u8 slot)
{
	struct ReliveBlock* b = image;
	struct CompressedPokemon keep;
	u8* m;
	u8* k = (u8*) &keep;
	u32 i;

	if (slot >= RELIVE_BOX_SLOTS || b->mons[slot].substruct0.species == 0)
		return FALSE;
	m = (u8*) &b->mons[slot];
	for (i = 0; i < sizeof(keep); ++i)
		k[i] = m[i];
	Zero(m, sizeof(keep));
	if (!ReliveBox_Commit(b))
	{
		for (i = 0; i < sizeof(keep); ++i)
			m[i] = k[i];
		return FALSE;
	}
	return TRUE;
}

void ReliveBox_OnLoad(void)
{
	struct ReliveBoxSave* s = RLB_SB1;

	if (StateOk(s))
		s->committedSel = s->sel;          //つづきから: 読んだ sel が正（空のセーブは 1 バイトも書かない）
}

void ReliveBox_AfterSave(bool8 ok)
{
	struct ReliveBoxSave* s = RLB_SB1;

	if (ok && StateOk(s))
		s->committedSel = s->sel;
}
