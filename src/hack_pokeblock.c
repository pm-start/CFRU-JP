#include "defines.h"
#include "../include/save.h"
#include "hack_ext_save.h"
#include "hack_pokeblock.h"

/*
 * ポロックケース（2026-11-09 FEAT-pokeblock・patches/0978）。見本は src/hack_relive_box.c（リライブボックス）と同じ置き方:
 *   - 中身は追加の区画 POKEBLOCK_SECTOR と +1 の「交互 2 区画」（PokeblockImage 1 つ＝296 バイト・40 個）。空きは color == 0（PBLOCK_CLR_NONE）。
 *   - 「どちらが正か」「入っている数」は SaveBlock1 + 0x3A34（global.h の +0x3A08 の帯の 6 バイト）に持つ＝本体の組の中なので、レポートで手持ちと一緒に書かれる。
 *   - 変えるたびに「前のレポートで正だった区画でないほう」へ書き、sel をそちらへ向ける。レポートの前にリセットすれば SaveBlock1 は前の sel（無傷の区画）に戻る。
 *   - 入っている数 0 のときは区画の中身を信じない（はじめから・別のセーブの古い中身を拾わない）。
 *   - 状態は「使うまで書かない」: 読むだけの関数（Count・Load）は SaveBlock1 を書き換えない（ポロックを持たないセーブは 1 バイトも変わらない）。
 *   - きのみブレンダーの記録 u16×3 は SaveBlock1 + 0x3A3A（区画を使わない・追加の領域が無い環境でも保存される）。
 */

#define PBK_SB1_MAGIC 0x4250         //'PB'
#define PBK_BLOCK_MAGIC 0x4B424B50   //'PKBK'
#define PBK_BLOCK_VERSION 1
#define PBK_SB1_OFF 0x3A34
#define PBK_REC_OFF 0x3A3A
#define PBK_POS_OFF 0x3A40               //ケースの画面の前回の位置 4 バイト（row・scroll・戻り先の種類・予備）
#define PBK_SIGNATURE 0x08012025     //ほかの追加の区画と同じ

struct PokeblockSave                 //SaveBlock1 + 0x3A34（6 バイト・本体の組に入る）
{
	u16 magic;
	u8 sel;                          //今の中身がある区画（0 = POKEBLOCK_SECTOR・1 = +1）
	u8 count;                        //入っている数
	u8 version;
	u8 committedSel;                 //最後に書き終えたレポートの sel（つづきから／レポートのあとに揃える）
};

#define PBK_SB1 ((struct PokeblockSave*) ((u8*) gSaveBlock1 + PBK_SB1_OFF))
#define PBK_REC ((u16*) ((u8*) gSaveBlock1 + PBK_REC_OFF))
#define PBK_POS ((u8*) gSaveBlock1 + PBK_POS_OFF)

typedef char PokeblockSaveFits[(sizeof(struct PokeblockSave) == 6 && PBK_SB1_OFF == 0x3A34 && PBK_REC_OFF == PBK_SB1_OFF + 6 && PBK_REC_OFF + POKEBLOCK_RECORD_COUNT * 2 <= PBK_POS_OFF && PBK_POS_OFF + 4 <= 0x3A44) ? 1 : -1];   //+0x3A34〜+0x3A44（リボンの表の次・コンディションの表の状態 +0x3A44 の手前）の 16 バイトに収まる
typedef char PokeblockImageFits[(sizeof(struct Pokeblock) == 7 && sizeof(struct PokeblockImage) == 16 + 7 * POKEBLOCK_CASE_SLOTS && sizeof(struct PokeblockImage) <= 0xFF4) ? 1 : -1];   //gSaveDataBuffer.data（0xFF4）に収まる

extern struct SaveSection gSaveDataBuffer;

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static bool8 StateOk(const struct PokeblockSave* s)
{
	return s->magic == PBK_SB1_MAGIC && s->version == 1 && s->count <= POKEBLOCK_CASE_SLOTS && s->sel <= 1 && s->committedSel <= 1;
}

//書く側（Commit）だけが呼ぶ: 印が合わなければ空の状態で作り直す。sel = committedSel = 1 から始める＝最初の書き込みは区画 0（target = sel ^ 1）へ
static struct PokeblockSave* SvForWrite(void)
{
	struct PokeblockSave* s = PBK_SB1;

	if (!StateOk(s))
	{
		Zero(s, sizeof(*s));
		s->magic = PBK_SB1_MAGIC;
		s->version = 1;
		s->sel = 1;
		s->committedSel = 1;
	}
	return s;
}

static u16 BlockSum(const struct PokeblockImage* b)
{
	const u8* p = (const u8*) b + 16;
	u32 n = sizeof(*b) - 16, i, t = 0x4250;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static void MakeEmpty(struct PokeblockImage* b)
{
	Zero(b, sizeof(*b));
	b->magic = PBK_BLOCK_MAGIC;
	b->version = PBK_BLOCK_VERSION;
}

u8 Pokeblock_ImageCount(const struct PokeblockImage* b)
{
	u8 i, n = 0;

	for (i = 0; i < POKEBLOCK_CASE_SLOTS; ++i)
		if (b->blocks[i].color != 0)
			++n;
	return n;
}

u8 Pokeblock_Usable(void)
{
	u8 check = HackExtSave_Check();

	if (check == 1)
		return POKEBLOCK_OK;
	return check == 2 ? POKEBLOCK_NEED_REPORT : POKEBLOCK_NO_EXT;
}

u8 Pokeblock_Count(void)
{
	const struct PokeblockSave* s = PBK_SB1;

	return StateOk(s) ? s->count : 0;
}

u32 Pokeblock_ImageSize(void)
{
	return sizeof(struct PokeblockImage);
}

bool8 Pokeblock_Load(struct PokeblockImage* b)
{
	const struct PokeblockSave* s = PBK_SB1;

	if (!StateOk(s) || s->count == 0)
	{
		MakeEmpty(b);                      //空（区画の古い中身は信じない）
		return TRUE;
	}
	if (!HackExtSave_Read(POKEBLOCK_SECTOR + (s->sel & 1), 0, b, sizeof(*b))
	 || b->magic != PBK_BLOCK_MAGIC || b->version != PBK_BLOCK_VERSION || b->sum != BlockSum(b))
	{
		MakeEmpty(b);
		return FALSE;
	}
	return TRUE;
}

//入れ物 image を、「前のレポートで正だった区画」でないほうへ書き、sel をそちらへ向ける。書けなければ FALSE（sel・数は変えない）
bool8 Pokeblock_Commit(struct PokeblockImage* b)
{
	bool8 wasOk = StateOk(PBK_SB1);
	struct PokeblockSave* s = SvForWrite();
	u8 target = (s->sel == s->committedSel) ? (s->sel ^ 1) : s->sel;
	u8* dst = (u8*) &gSaveDataBuffer;
	const u8* src = (const u8*) b;
	u32 i;

	b->magic = PBK_BLOCK_MAGIC;
	b->version = PBK_BLOCK_VERSION;
	b->sum = BlockSum(b);
	for (i = 0; i < sizeof(gSaveDataBuffer); ++i)
		dst[i] = 0;                        //ブロックの外は 0（古い読み込みの残りを書かない）
	for (i = 0; i < sizeof(*b); ++i)
		dst[i] = src[i];
	gSaveDataBuffer.signature = PBK_SIGNATURE;
	if (!HackExtSave_WriteSector(POKEBLOCK_SECTOR + target, &gSaveDataBuffer))
	{
		if (!wasOk)
			Zero(s, sizeof(*s));           //書けなかったときは状態も作らない（空のセーブは 1 バイトも変わらない）
		return FALSE;
	}
	s->sel = target;
	s->count = Pokeblock_ImageCount(b);
	return TRUE;
}

bool8 Pokeblock_Add(struct PokeblockImage* b, const struct Pokeblock* pb, u8* slotOut)
{
	const u8* s = (const u8*) pb;
	u8* m;
	u8 slot;
	u32 i;

	for (slot = 0; slot < POKEBLOCK_CASE_SLOTS && b->blocks[slot].color != 0; ++slot);
	if (slot >= POKEBLOCK_CASE_SLOTS || pb->color == 0)
		return FALSE;
	m = (u8*) &b->blocks[slot];
	for (i = 0; i < sizeof(struct Pokeblock); ++i)
		m[i] = s[i];
	if (!Pokeblock_Commit(b))
	{
		Zero(m, sizeof(struct Pokeblock));
		return FALSE;
	}
	if (slotOut != NULL)
		*slotOut = slot;
	return TRUE;
}

bool8 Pokeblock_Remove(struct PokeblockImage* b, u8 slot)
{
	struct Pokeblock keep;
	u8* m;
	u8* k = (u8*) &keep;
	u32 i;

	if (slot >= POKEBLOCK_CASE_SLOTS || b->blocks[slot].color == 0)
		return FALSE;
	m = (u8*) &b->blocks[slot];
	for (i = 0; i < sizeof(keep); ++i)
		k[i] = m[i];
	Zero(m, sizeof(keep));
	if (!Pokeblock_Commit(b))
	{
		for (i = 0; i < sizeof(keep); ++i)
			m[i] = k[i];
		return FALSE;
	}
	return TRUE;
}

void Pokeblock_OnLoad(void)
{
	struct PokeblockSave* s = PBK_SB1;

	if (StateOk(s))
		s->committedSel = s->sel;          //つづきから: 読んだ sel が正（空のセーブは 1 バイトも書かない）
}

void Pokeblock_AfterSave(bool8 ok)
{
	struct PokeblockSave* s = PBK_SB1;

	if (ok && StateOk(s))
		s->committedSel = s->sel;
}

u16 Pokeblock_GetRecord(u8 i)
{
	return i < POKEBLOCK_RECORD_COUNT ? PBK_REC[i] : 0;
}

void Pokeblock_SetRecord(u8 i, u16 v)
{
	if (i < POKEBLOCK_RECORD_COUNT)
		PBK_REC[i] = v;
}

//ケースの画面の前回の位置。row・scroll は開くたびに画面側が丸める（LimitMenuScrollAndRow）。同じ値なら書かない（ケースを開くまで SaveBlock1 を変えない）
void Pokeblock_GetCasePos(u8* row, u8* scroll)
{
	*row = PBK_POS[0];
	*scroll = PBK_POS[1];
}

void Pokeblock_SetCasePos(u8 row, u8 scroll)
{
	if (PBK_POS[0] != row)
		PBK_POS[0] = row;
	if (PBK_POS[1] != scroll)
		PBK_POS[1] = scroll;
}

//閉じたあとの戻り先の種類（PBC_RET_BAG／PBC_RET_FIELD）。「つかう」でケースを閉じて ポケモンを選ぶ画面へ行き、戻ってきたときにケースを同じ場所へ開き直すため（原文 sSavedPokeblockData.callback・段 3）
u8 Pokeblock_GetCaseReturn(void)
{
	return PBK_POS[2];
}

void Pokeblock_SetCaseReturn(u8 kind)
{
	if (PBK_POS[2] != kind)
		PBK_POS[2] = kind;
}
