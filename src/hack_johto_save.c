#include "defines.h"
#include "../include/save.h"
#include "hack_ext_save.h"
#include "hack_box_shelf.h"
#include "hack_johto_save.h"
#include "hack_save_version.h"

/*
 * ジョウト J0（FEAT-johto-j0）の旗・変数の置き場。形・置き場の理由は hack_johto_save.h。作りは hoenn_frontier_rec.c（RAM の写し・主人公 ID・レポートのあとに書く）と
 * 交互 2 区画（書く区画は前の正の反対側＝書いている途中で電源が切れても正の区画は無傷）を合わせた形:
 *   変数 0x5200〜0x52FF は RAM の写し（JOHTO_VAR_RAM_ADDR）を直に読み書きされる（GetExpandedVarPointer が写しの中を返す）。
 *   写しは最初に触れたときにフラッシュから読む: 両方の区画の頭を見て、主人公の ID・和が合う区画のうち世代の新しいほうが正。無い・使えない環境・壊れ・別のセーブは全部 0。
 *   レポートが成功したあと、写しが読み込み／前回の書き込みから変わっていれば、正でない区画へ世代 +1 で書く。変わっていなければ書かない。
 *   読み書きは HackExtSave_*（スロットのバンクを通る）だけ＝ぼうけんのしょ 1・2 で別々。
 *   旗 0x1900〜0x1AFF は SaveBlock1 の中の 64 バイトを直に指すだけ（保存は本体の組）。
 *   ★ FEAT-johto-p1: 旗 0x1B00〜0x1CFF（64 バイト）は変数の写しの後ろ（r->f）に持ち、変数と同じ区画・同じ書き込みで保存（版 2・版 1 の区画は旗 0 で読める）。
 */

#define JVB_MAGIC 0x3142564A            //'JVB1'
#define JV_SIGNATURE 0x08012025         //ほかの追加の区画と同じ
#define JV_RAM_SIG 0x4A565241           //'ARVJ'
#define JV_VERSION 2                    //版 2: 変数の後ろに旗 64 バイト（FEAT-johto-p1）。版 1＝旗なし
#define JV_SECTOR(sel) (JOHTO_VAR_SECTOR + ((sel) & 1))

struct JohtoVarRam
{
	u32 sig;
	u32 base;                           //読み込み／最後に書いたときの写しの 32 ビットの印（これと違えば「変えた」）
	u32 key;                            //この写しの持ち主の主人公の ID
	u8 sel;                             //正の区画（0／1）
	u8 loaded;                          //1 = フラッシュに正の区画がある（gen が有効）
	u16 gen;                            //正の区画の世代
	u16 v[JOHTO_VAR_COUNT];
	u8 f[JOHTO2_FLAG_BYTES];            //旗 0x1B00〜0x1CFF（版 2 から）
};

typedef char JohtoNeedsVersion[(HACK_SAVE_VERSION >= HACK_JOHTO_VERSION) ? 1 : -1];   //区画を書く版以上でないと、古い版がジョウトつきのセーブを「新しすぎる」と止められない（hack_move_count.c の MoveCountNeedsVersion と同じ作法）
typedef char JohtoVarBlockSize[(sizeof(struct JohtoVarBlock) == 16 + 2 * JOHTO_VAR_COUNT + JOHTO2_FLAG_BYTES && sizeof(struct JohtoVarBlock) <= 0xFF4) ? 1 : -1];
typedef char JohtoVarRamSize[(sizeof(struct JohtoVarRam) == 16 + 2 * JOHTO_VAR_COUNT + JOHTO2_FLAG_BYTES) ? 1 : -1];
typedef char JohtoFlagSb1Fits[(JOHTO_FLAG_SB1_OFF == 0x3A94 && JOHTO_FLAG_BYTES * 8 == JOHTO_FLAG_LAST - JOHTO_FLAG_FIRST + 1) ? 1 : -1];   //filler_3A94（0x40）＝+0x3AD4 の registeredTexts の手前

extern struct SaveSection gSaveDataBuffer;

#define RAM ((struct JohtoVarRam*) JOHTO_VAR_RAM_ADDR)

static u32 TrainerKey(void)
{
	const u8* id = gSaveBlock2->playerTrainerId;

	return id[0] | (id[1] << 8) | (id[2] << 16) | ((u32) id[3] << 24);
}

//変数だけの印（版 1 の区画の和と同じ式）
static u32 HashVars(const u16* v)
{
	u32 i, h = 0x811C9DC5;

	for (i = 0; i < JOHTO_VAR_COUNT; ++i)
	{
		h = (h ^ (v[i] & 0xFF)) * 16777619;
		h = (h ^ (v[i] >> 8)) * 16777619;
	}
	return h;
}

//変数 + 旗（版 2 の区画の和・写しの「変えた」の印）
static u32 Hash32(const struct JohtoVarRam* r)
{
	u32 i, h = HashVars(r->v);

	for (i = 0; i < JOHTO2_FLAG_BYTES; ++i)
		h = (h ^ r->f[i]) * 16777619;
	return h;
}

static u16 Fold16(u32 h)
{
	return (h ^ (h >> 16)) & 0xFFFF;
}

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static bool8 HeadOk(const struct JohtoVarBlock* h, u32 key)
{
	return h->magic == JVB_MAGIC && (h->version == 1 || h->version == JV_VERSION) && h->trainerId == key;
}

//区画 sel の頭が合い、本体の和も合えば r->v に読んで TRUE
static bool8 TryRead(struct JohtoVarRam* r, u8 sel, const struct JohtoVarBlock* h)
{
	if (!HackExtSave_ReadKnownAvail(JV_SECTOR(sel), 16, r->v, sizeof(r->v)))
		return FALSE;
	if (h->version >= 2 && !HackExtSave_ReadKnownAvail(JV_SECTOR(sel), 16 + sizeof(r->v), r->f, sizeof(r->f)))
	{
		Zero(r->v, sizeof(r->v));
		return FALSE;
	}
	if (Fold16(h->version >= 2 ? Hash32(r) : HashVars(r->v)) != h->sum)
	{
		Zero(r->v, sizeof(r->v));
		Zero(r->f, sizeof(r->f));
		return FALSE;
	}
	r->sel = sel;
	r->gen = h->gen;
	r->loaded = TRUE;
	return TRUE;
}

//RAM の写しを作る: 無い・使えない環境・壊れ・別のセーブは全部 0
static void LoadRam(struct JohtoVarRam* r)
{
	u32 key = TrainerKey();

	Zero(r, sizeof(*r));
	r->key = key;
	r->sel = 1;                          //正の区画が無いときの最初の書き込みは区画 0 へ
	if (BoxShelf_ExtAvailable())
	{
		u32 head0[4], head1[4];
		const struct JohtoVarBlock* a = (const struct JohtoVarBlock*) head0;
		const struct JohtoVarBlock* b = (const struct JohtoVarBlock*) head1;
		bool8 okA = HackExtSave_ReadKnownAvail(JV_SECTOR(0), 0, head0, 16) && HeadOk(a, key);
		bool8 okB = HackExtSave_ReadKnownAvail(JV_SECTOR(1), 0, head1, 16) && HeadOk(b, key);
		bool8 bNewer = okA && okB ? (s16) (b->gen - a->gen) > 0 : okB;

		if (bNewer)
		{
			if (!TryRead(r, 1, b) && okA)
				TryRead(r, 0, a);
		}
		else if (okA)
		{
			if (!TryRead(r, 0, a) && okB)
				TryRead(r, 1, b);
		}
	}
	r->base = Hash32(r);
	r->sig = JV_RAM_SIG;
}

static struct JohtoVarRam* Ram(void)
{
	struct JohtoVarRam* r = RAM;

	if (r->sig != JV_RAM_SIG || r->key != TrainerKey())
		LoadRam(r);
	return r;
}

u8* HackJohto_FlagPtr(u16 id)
{
	if (id < JOHTO_FLAG_FIRST || id > JOHTO_FLAG_LAST)
		return NULL;
	return (u8*) gSaveBlock1 + JOHTO_FLAG_SB1_OFF + ((id - JOHTO_FLAG_FIRST) >> 3);
}

u8* HackJohto_Flag2Ptr(u16 id)
{
	if (id < JOHTO2_FLAG_FIRST || id > JOHTO2_FLAG_LAST)
		return NULL;
	if (!BoxShelf_ExtAvailable())
		return NULL;                          //窓は無い（読めば 0・書きは捨てる）
	return &Ram()->f[(id - JOHTO2_FLAG_FIRST) >> 3];
}

u16* HackJohto_VarPtr(u16 id)
{
	if (id < JOHTO_VAR_FIRST || id >= JOHTO_VAR_FIRST + JOHTO_VAR_COUNT)
		return NULL;
	if (!BoxShelf_ExtAvailable())
		return (u16*) 1;                      //窓は無い（読めば 0・書きは捨てる）
	return &Ram()->v[id - JOHTO_VAR_FIRST];
}

//レポートが成功したあと: 変えた写しを正でない区画へ書く（書けなかったら写しは変えたまま＝次のレポートでまた試す）
void HackJohtoVar_AfterSave(bool8 ok)
{
	struct JohtoVarRam* r = RAM;
	struct JohtoVarBlock* w = (struct JohtoVarBlock*) gSaveDataBuffer.data;
	u32 h, i;
	u8 target;
	u16 gen;

	if (!ok || r->sig != JV_RAM_SIG || r->key != TrainerKey())
		return;                               //つづきから／はじめから以来 1 度も触れていない: 何も変わっていない
	h = Hash32(r);
	if (h == r->base || !BoxShelf_ExtAvailable())
		return;
	target = r->loaded ? (r->sel ^ 1) : 0;
	gen = r->loaded ? (u16) (r->gen + 1) : 1;
	Zero(&gSaveDataBuffer, sizeof(gSaveDataBuffer));
	w->magic = JVB_MAGIC;
	w->version = JV_VERSION;
	w->gen = gen;
	w->trainerId = r->key;
	w->sum = Fold16(h);
	for (i = 0; i < JOHTO_VAR_COUNT; ++i)
		w->vars[i] = r->v[i];
	for (i = 0; i < JOHTO2_FLAG_BYTES; ++i)
		w->flags[i] = r->f[i];
	gSaveDataBuffer.signature = JV_SIGNATURE;
	if (!HackExtSave_WriteSector(JV_SECTOR(target), &gSaveDataBuffer))
		return;
	r->sel = target;
	r->gen = gen;
	r->loaded = TRUE;
	r->base = h;
}

void HackJohtoVar_OnLoad(void)
{
	RAM->sig = 0;                             //次に触れたときに読み直す（読んだセーブの変数に戻す）
}

void HackJohtoVar_NewGame(void)
{
	RAM->sig = 0;                             //はじめから: 前のセーブの写しを引きずらない（主人公の ID が違う区画は読まれない）
}
