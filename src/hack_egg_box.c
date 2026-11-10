#include "defines.h"
#include "../include/save.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "hack_ext_save.h"
#include "hack_ramshift.h"
#include "hack_egg_box.h"

/*
 * 卵専用ボックス（2026-10-23 FEAT-save16-B4・Redmine #38・設計書 docs/research/2026-10-22_rom128-save16-design.md §8・ユーザー決定 q115・q116）。
 *   1 つ・30 匹まで・箱の中で孵る・孵った子は箱に残る（手持ちへ出すのは画面から）。mGBA の追加のセーブ領域が使えるときだけ。
 *   入口は「べんりきのう」の「たまごボックス」（src/bw_start_menu.c・画面は src/hack_egg_box_screen.c）。
 *
 * 置き場と、レポートとの一緒の保存（卵を箱に入れてレポートを書かずにリセットしても、手持ちの卵と箱の卵が両方に残らない）:
 *   - 中身は追加の区画 EGG_BOX_SECTOR と +1 の「交互 2 区画」（ブロック 1 つ＝EggBoxBlock・約 2KB）。
 *   - 「どちらが正か」「卵の時計」「1 サイクルの途中の歩数」「入っている数」は SaveBlock1 + 0x043C（filler_40A の 28 バイトのうち 12 バイト）に持つ
 *     ＝本体の組の中なので、レポートを書いたときに手持ちと一緒に書かれる（EggBoxSave）。
 *   - 変えるたびに「正でないほうの区画」へ書き、SaveBlock1 の sel をそちらへ向ける。レポートの前にリセットすれば、SaveBlock1 は前のレポートの
 *     sel（無傷の区画）に戻る。レポートのあとは sel が正（committedSel・RAM に覚える）。レポートの前にもう一度変えるときは、前のレポートで正だった区画
 *     を壊さないよう、今の sel の区画（まだ正でない）へ上書きする。
 *   - 入っている数 0 のときは区画の中身を信じない（はじめから・別のセーブの古い中身を拾わない）。
 *
 * 歩数: 手持ちの卵と同じ規則（256 歩で 1 サイクル・ほのおのからだ等で 2 ずつ・src/daycare.c GetEggStepsToSubtract）。卵を入れた時に
 *   孵る時刻 hatchAt = 時計 + 残りサイクル を持ち、時計が hatchAt に届いたら孵った（hatchAt が 0 = 空き）。
 *   RAM の置き場: PokemonStorage の +0x8124〜0x8127（src/hoenn_frontier_rec.c の RAM の写しの終わり〜+0x8128 の壁紙の手前の 4 バイト）。保存しない。
 */

#define EGGBOX_SB1_MAGIC 0x31424745      //'EGB1'
#define EGGBOX_BLOCK_MAGIC 0x58424745    //'EGBX'
#define EGGBOX_BLOCK_VERSION 2           //★ stat16 WP4（patches/0680）: 1 = W（63 バイト × 30）・2 = X（78 バイト × 30・努力値・個体値 16 ビット）
//★ 2026-10-24 BUG-eggbox-unreadable（Redmine #82・patches/0797）: 版 1（W）のブロックも読む。0.59.1 のセーブを変換ツールで X にしたとき、直す前のツールが卵ブロックを
//  W のまま残した（和の式の食い違い）ので、「よみこめません」になっていた。版 1 は Load が X の入れ物に広げて（先頭 63 バイトはそのまま・後ろ 15 バイトは 0）書き直す。
#define EGGBOX_BLOCK_VERSION_W 1
#define EGGBOX_W_MON_SIZE 63
#define EGGBOX_W_SIZE 2028               //W のブロックの大きさ（16 + 30×4 + 30×63 = 2026 を、構造体の 4 バイト境界で切り上げた sizeof）＝和が足す範囲。tools/save_migrate_l/lformat.py の egg_sum と同じ
#define EGGBOX_SB1_OFF 0x043C
#define EGGBOX_SIGNATURE 0x08012025      //ほかの追加の区画と同じ（道具が署名つきの区画として読める）
#define EGGBOX_RAM_MAGIC 0xE6

struct EggBoxSave               //SaveBlock1 + 0x043C（12 バイト・本体の組に入る）
{
	u32 magic;
	u32 clock;                  //卵の時計（サイクル）
	u8 sel;                     //今の中身がある区画（0 = EGG_BOX_SECTOR・1 = +1）
	u8 steps;                   //1 サイクルの途中の歩数（0〜255・256 で 1 サイクル）
	u8 count;                   //入っている数
	u8 version;
};

struct EggBoxRam                //PokemonStorage + 0x8124（保存しない・4 バイト）
{
	u8 magic;
	u8 committedSel;            //最後に書き終えたレポートの sel
	u8 pad[2];
};

struct EggBoxBlock              //追加の区画 1 つに書く（先頭から）
{
	u32 magic;
	u16 version;
	u16 sum;                    //先頭 16 バイトより後ろの和
	u32 spare[2];
	u32 hatchAt[EGGBOX_SLOTS];  //孵る時刻（0 = 空き）
	struct CompressedPokemon mons[EGGBOX_SLOTS];
};

#define EGGBOX_RAM ((struct EggBoxRam*) (HACK_STORAGE_BASE + 0x8124))
#define EGGBOX_SB1 ((struct EggBoxSave*) ((u8*) gSaveBlock1 + EGGBOX_SB1_OFF))
#define HATCH_AT_OFF 16

typedef char EggBoxSaveFits[(sizeof(struct EggBoxSave) <= 0x1C) ? 1 : -1];                   //filler_40A（0x1C）に収まる
typedef char EggBoxBlockFits[(sizeof(struct EggBoxBlock) <= 0xFF4 && HATCH_AT_OFF == 16) ? 1 : -1];   //gSaveDataBuffer.data（0xFF4）に収まる
typedef char EggBoxRamFits[(sizeof(struct EggBoxRam) == 4) ? 1 : -1];

extern struct SaveSection gSaveDataBuffer;
u8 HackEggBox_StepsPerCycle(void);                           //src/daycare.c（手持ちの卵と同じ 1 サイクルで引く数）
void HackEggBox_MarkHatched(const struct CompressedPokemon* mon);   //src/hack_egg_box_screen.c（図鑑・孵化数）

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static struct EggBoxSave* Sv(void)
{
	struct EggBoxSave* s = EGGBOX_SB1;

	//印と版が合わない・中身がありえない値（古いセーブのごみ・壊れ）なら、空の状態で作り直す（合言葉の確かめ）
	if (s->magic != EGGBOX_SB1_MAGIC || s->version != 1 || s->count > EGGBOX_SLOTS || s->sel > 1)
	{
		Zero(s, sizeof(*s));
		s->magic = EGGBOX_SB1_MAGIC;
		s->version = 1;
	}
	return s;
}

static struct EggBoxRam* RamState(void)
{
	struct EggBoxRam* r = EGGBOX_RAM;

	if (r->magic != EGGBOX_RAM_MAGIC)
	{
		r->magic = EGGBOX_RAM_MAGIC;
		r->committedSel = Sv()->sel;       //つづきから以外で入った（はじめから）: 今の sel を正とみなす
	}
	return r;
}

static u16 SumBytes(const void* b, u32 size)
{
	const u8* p = (const u8*) b + 16;
	u32 n = size - 16, i, t = 0x4547;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 BlockSum(const struct EggBoxBlock* b)
{
	return SumBytes(b, sizeof(*b));
}

//版 1（W）のブロック（b の先頭から W の並び）を、その場で X の並びに広げる。後ろのスロットから動かす（動かす先が手前の読み元に重ならない）
static void WidenW(struct EggBoxBlock* b)
{
	u8* base = (u8*) b + 16 + 4 * EGGBOX_SLOTS;
	s32 slot;
	u32 i;

	for (slot = EGGBOX_SLOTS - 1; slot >= 0; --slot)
	{
		u8* dst = base + slot * sizeof(struct CompressedPokemon);
		const u8* src = base + slot * EGGBOX_W_MON_SIZE;

		for (i = EGGBOX_W_MON_SIZE; i > 0; --i)           //後ろから写す（同じスロットの中で重なっても壊れない）
			dst[i - 1] = src[i - 1];
		for (i = EGGBOX_W_MON_SIZE; i < sizeof(struct CompressedPokemon); ++i)
			dst[i] = 0;                                    //evHi・ivHi は 0（零拡張）
	}
	b->version = EGGBOX_BLOCK_VERSION;
}

static u8 CountOf(const struct EggBoxBlock* b)
{
	u8 i, n = 0;

	for (i = 0; i < EGGBOX_SLOTS; ++i)
		if (b->hatchAt[i] != 0)
			++n;
	return n;
}

u8 EggBox_Usable(void)
{
	u8 check = HackExtSave_Check();

	if (check == 1)
		return EGGBOX_OK;
	return check == 2 ? EGGBOX_NEED_REPORT : EGGBOX_NO_EXT;
}

u8 EggBox_Count(void)
{
	return Sv()->count;
}

u32 EggBox_Clock(void)
{
	return Sv()->clock;
}

bool8 EggBox_IsHatched(u32 hatchAt)
{
	return hatchAt != 0 && Sv()->clock >= hatchAt;
}

u32 EggBox_ImageSize(void)
{
	return sizeof(struct EggBoxBlock);
}

u32 EggBox_SlotHatchAt(const void* image, u8 slot)
{
	return slot < EGGBOX_SLOTS ? ((const struct EggBoxBlock*) image)->hatchAt[slot] : 0;
}

const struct CompressedPokemon* EggBox_SlotMon(const void* image, u8 slot)
{
	return slot < EGGBOX_SLOTS ? &((const struct EggBoxBlock*) image)->mons[slot] : NULL;
}

static bool8 Commit(struct EggBoxBlock* b);

bool8 EggBox_Load(void* image)
{
	struct EggBoxBlock* b = image;
	struct EggBoxSave* sv = Sv();
	bool8 read;

	Zero(b, sizeof(*b));
	b->magic = EGGBOX_BLOCK_MAGIC;
	b->version = EGGBOX_BLOCK_VERSION;
	if (sv->count == 0)
		return TRUE;                       //空（区画の古い中身は信じない）
	read = HackExtSave_Read(EGG_BOX_SECTOR + (sv->sel & 1), 0, b, sizeof(*b));
	if (read && b->magic == EGGBOX_BLOCK_MAGIC && b->version == EGGBOX_BLOCK_VERSION_W && b->sum == SumBytes(b, EGGBOX_W_SIZE))
	{
		//版 1（W）: X の入れ物に広げ、書き直す。Commit は「前のレポートで正だった区画でないほう」へ書き sel を向ける＝前のレポートの W の区画は壊さない
		//（レポートの前にリセットすれば W に戻り、次に開いたときまた広げる）。書けなくても入れ物（RAM）は広げたまま返す＝中身は見える・フラッシュは元のまま
		WidenW(b);
		Commit(b);
		sv->count = CountOf(b);
		return TRUE;
	}
	if (!read || b->magic != EGGBOX_BLOCK_MAGIC || b->version != EGGBOX_BLOCK_VERSION || b->sum != BlockSum(b))
	{
		Zero(b, sizeof(*b));
		b->magic = EGGBOX_BLOCK_MAGIC;
		b->version = EGGBOX_BLOCK_VERSION;
		return FALSE;
	}
	sv->count = CountOf(b);                //区画の中身を正とする（SaveBlock1 の数は目安）
	return TRUE;
}

//入れ物 b を、「前のレポートで正だった区画」でないほうへ書き、sel をそちらへ向ける。書けなければ FALSE（sel・数は変えない）
static bool8 Commit(struct EggBoxBlock* b)
{
	struct EggBoxSave* sv = Sv();
	struct EggBoxRam* ram = RamState();
	u8 target = (sv->sel == ram->committedSel) ? (sv->sel ^ 1) : sv->sel;
	u8* dst = (u8*) &gSaveDataBuffer;
	const u8* src = (const u8*) b;
	u32 i;

	b->magic = EGGBOX_BLOCK_MAGIC;
	b->version = EGGBOX_BLOCK_VERSION;
	b->sum = BlockSum(b);
	for (i = 0; i < sizeof(gSaveDataBuffer); ++i)
		dst[i] = 0;
	for (i = 0; i < sizeof(*b); ++i)
		dst[i] = src[i];
	gSaveDataBuffer.signature = EGGBOX_SIGNATURE;
	if (!HackExtSave_WriteSector(EGG_BOX_SECTOR + target, &gSaveDataBuffer))
		return FALSE;
	sv->sel = target;
	sv->count = CountOf(b);
	return TRUE;
}

bool8 EggBox_Put(void* image, const struct CompressedPokemon* mon, u8 cyclesLeft, u8* slotOut)
{
	struct EggBoxBlock* b = image;
	u8* m;
	const u8* s = (const u8*) mon;
	u8 slot;
	u32 i;

	for (slot = 0; slot < EGGBOX_SLOTS && b->hatchAt[slot] != 0; ++slot);
	if (slot >= EGGBOX_SLOTS)
		return FALSE;
	m = (u8*) &b->mons[slot];
	for (i = 0; i < sizeof(*mon); ++i)
		m[i] = s[i];
	b->hatchAt[slot] = Sv()->clock + (cyclesLeft != 0 ? cyclesLeft : 1);   //0 は空きの印なので、必ず 1 以上先
	if (!Commit(b))
	{
		b->hatchAt[slot] = 0;
		Zero(m, sizeof(*mon));
		return FALSE;
	}
	if (slotOut != NULL)
		*slotOut = slot;
	return TRUE;
}

bool8 EggBox_Take(void* image, u8 slot)
{
	struct EggBoxBlock* b = image;
	struct CompressedPokemon keep;
	u32 keepAt, i;
	u8* m;
	u8* k = (u8*) &keep;

	if (slot >= EGGBOX_SLOTS || b->hatchAt[slot] == 0)
		return FALSE;
	m = (u8*) &b->mons[slot];
	keepAt = b->hatchAt[slot];
	for (i = 0; i < sizeof(keep); ++i)
		k[i] = m[i];
	b->hatchAt[slot] = 0;
	Zero(m, sizeof(keep));
	if (!Commit(b))
	{
		b->hatchAt[slot] = keepAt;
		for (i = 0; i < sizeof(keep); ++i)
			m[i] = k[i];
		return FALSE;
	}
	return TRUE;
}

bool8 EggBox_OnStep(void)
{
	struct EggBoxSave* sv = Sv();
	u32 old, hatchAt[EGGBOX_SLOTS], stride = sizeof(struct CompressedPokemon);
	u8 slot, head[8];
	bool8 any = FALSE;

	if (sv->count == 0)
		return FALSE;
	if ((u8) (sv->steps + 1) != 0)
	{
		sv->steps = sv->steps + 1;
		return FALSE;
	}
	sv->steps = 0;                         //256 歩目 = 1 サイクル
	old = sv->clock;
	sv->clock = old + HackEggBox_StepsPerCycle();
	if (!HackExtSave_Read(EGG_BOX_SECTOR + (sv->sel & 1), HATCH_AT_OFF, hatchAt, sizeof(hatchAt)))
		return FALSE;
	if (HackExtSave_Read(EGG_BOX_SECTOR + (sv->sel & 1), 0, head, sizeof(head)) && (head[4] | (head[5] << 8)) == EGGBOX_BLOCK_VERSION_W)
		stride = EGGBOX_W_MON_SIZE;        //まだ広げていない版 1（W）の区画: 子は 63 バイト間隔（広げる前に歩いても、孵った子を正しく図鑑に付ける）
	for (slot = 0; slot < EGGBOX_SLOTS; ++slot)
	{
		if (hatchAt[slot] != 0 && hatchAt[slot] > old && hatchAt[slot] <= sv->clock)
		{
			struct CompressedPokemon mon;

			Zero(&mon, sizeof(mon));
			if (HackExtSave_Read(EGG_BOX_SECTOR + (sv->sel & 1), HATCH_AT_OFF + sizeof(hatchAt) + slot * stride, &mon, stride))
				HackEggBox_MarkHatched(&mon);
			any = TRUE;
		}
	}
	return any;
}

void EggBox_OnLoad(void)
{
	struct EggBoxRam* r = EGGBOX_RAM;

	r->magic = EGGBOX_RAM_MAGIC;
	r->committedSel = Sv()->sel;
}

void EggBox_AfterSave(bool8 ok)
{
	if (ok)
		RamState()->committedSel = Sv()->sel;
}
