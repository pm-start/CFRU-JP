#include "defines.h"
#include "../include/event_data.h"
#include "../include/global.h"
#include "../include/save.h"
#include "Tables/hoenn_script_flags.h"
#include "hack_ext_save.h"
#include "hack_ramshift.h"
#include "hoenn_frontier_rec.h"

/*
 * フロンティアの記録・状態の入れ物（2026-10-20 ユーザー「レベル49と100は別の記録・追加のセーブ領域へ」「CFRU のタワー以外は全部移す」）。
 * 追加のセーブ領域（src/hack_ext_save.c・直した mGBA だけ）の 1 区画に、次の 2 つを置く。
 *   (1) レベル49・100 の連勝／最高の連勝（HoennFrontierRec_Get/Set）
 *   (2) 移した拡張変数: 0x5100〜0x513F（ファクトリー・チューブ・アリーナ・パレス・挑戦者の表）と 0x5151〜0x51ED（ドーム・ピラミッドの構造体とバッグ）。
 *       GetExpandedVarPointer（src/save.c）が、この範囲の変数番号をここの RAM の写しへ向ける。VarGet/VarSet・スクリプトの setvar/compare・
 *       DOME->／PYR->（構造体の重ね書き）は同じ道を通るので、施設のコードは変えなくてよい。
 *   CFRU のタワー（gBattleTowerStreaks・0x5140〜0x5146 の挑戦中の状態）・BP（0x514E）・シンボル（0x514F）・ほかの 0x5147〜0x5150 は動かさない。
 *
 * 置き場（実測・検算は tools/frontier/verify_frontier_rec.py）:
 *   追加の区画 HFR_SECTOR = 64（フラッシュの区画 96 = バンク 6 の頭）。ぼうけんのしょ 2 のときは SwitchFlashBank が +48 バンクにするので 2 つは重ならない。
 *   使っている区画: 0（6MB 版の確かめ）・1／735（HackExtSave_DebugTest）・8〜25（33〜50 箱）・26〜55（箱のページの写し）。56〜734 は空き＝その先頭 64 を取り、前後に余白。
 * 区画の中身（4,096 バイト・書くときはまるごと）:
 *   +0x000〜0x00F ヘッダ  magic 'HFR1'(u32) / version(u16) / bodySize(u16) / trainerId(u32) / checksum(u16) / reserved(u16)
 *   +0x010〜 本体 v3（bodySize = 642 バイト）
 *       streak[7 施設][2 シングル・ダブル][2 レベル（49・100）][2 今・最高] の u16 = 112 バイト
 *       vars[64 + 157] の u16 = 442 バイト（先頭 64 = 0x5100〜0x513F・続く 157 = 0x5151〜0x51ED）
 *       rents[2 シングル・ダブル][2 レベル（49・100）] の u16 = 8 バイト（v3・2026-10-21 バトルファクトリーの交換の回数。レベル50・オープンは今までの 0x5111 台）
 *       pyrBag[2 レベル（49・100）][10 枠][2 = 道具番号・個数] の u16 = 80 バイト（v3・ピラミッドのバトルバッグ。50・オープンは今までの 0x51C6〜0x51ED）
 *   +0x292〜0x7FF 予約（2KB まで）… 版を上げて本体を伸ばす
 *   v2（本体 554 バイト・rents が無い）のセーブも読める: 和を 554 バイトで確かめ、rents は 0 で読み込み、次のレポートで v3 に書き直す
 *   +0x800〜0xFFF 未割り当て（+0xFF8 は書くときの署名）
 *   checksum は本体の 16 ビットの和（位置で重みを付ける）。magic・version・bodySize・trainerId・checksum が合わなければ「記録なし」。
 * trainerId は主人公の ID（さいしょから でほかの主人公が同じ区画を見ても 0 から始まる）。
 *
 * 自動の引っ越し（古いセーブ）: 移す前のセーブは、変数が本体の拡張変数（gExpandedVars）に入っている。入れ物が無い（初めて）ときは、追加の領域が使えるなら
 *   本体に残っている値を入れ物へ写して（dirty）、次のレポートで書く。以後は入れ物が正（本体の古い値は読まれない）。追加の領域が使えない版（実機・素の mGBA）は
 *   入れ物を使わず今までの場所（本体の拡張変数）のまま（VarPtr が NULL）。使えるようになったとき（最初のレポートのあと）に写して切り替える。
 *
 * RAM の写し: PokemonStorage の棚の入れ物（hack_box_shelf.c の ShelfCache）の後ろ（+0x7E88）。保存しない場所。
 *   最初に触れたときにフラッシュから読み、変えた（変数への触れも含む）ら dirty。レポートが成功したあとに書く。
 */
//HFR_SECTOR（64）は hack_ext_sector_map.h（生成物・正本 tools/save_layout/ext_sector_map.json）
#define HFR_MAGIC 0x31524648          /* 'HFR1' */
#define HFR_VERSION 3
#define HFR_VERSION_V2 2
#define HFR_STREAK_COUNT (HFR_FAC_COUNT * 2 * 2 * 2)
#define HFR_VARS_A_FIRST 0x5100       /* 移す変数（先頭の範囲 0x5100〜0x513F） */
#define HFR_VARS_A_COUNT 64
#define HFR_VARS_B_FIRST 0x5151       /* 続きの範囲 0x5151〜0x51ED */
#define HFR_VARS_B_COUNT 157
#define HFR_VARS_COUNT (HFR_VARS_A_COUNT + HFR_VARS_B_COUNT)
#define HFR_RENTS_COUNT 4            /* rents[2][2]（v3 で追加・本体の最後） */
#define HFR_BODY_SIZE_V2 ((HFR_STREAK_COUNT + HFR_VARS_COUNT) * 2)   /* 554 */
#define HFR_PYRBAG_COUNT 40          /* pyrBag[2][10][2]（v3 で追加・rents の後） */
#define HFR_BODY_SIZE (HFR_BODY_SIZE_V2 + HFR_RENTS_COUNT * 2 + HFR_PYRBAG_COUNT * 2)   /* 642 */
#define HFR_SIGNATURE 0x08012025
#define STORAGE_BASE HACK_STORAGE_BASE   /* PokemonStorage の今の番地（版ごとにずらす・hack_ramshift.h・本流 0.55.0 以降）。固定番地 0x0202924C だと棚の入れ物と重なる */
#define HFR_RAM_OFF (0x7D88 + 0x100)  /* ShelfCache（src/hack_box_shelf.c・168 バイト）の後ろ・+0x8128 の壁紙の手前まで */
#define HFR_RAM_SIG 0x52464C48        /* 'HLFR' */

struct HfrBlock
{
	u32 magic;
	u16 version;
	u16 bodySize;
	u32 trainerId;
	u16 checksum;
	u16 reserved;
	u16 streak[HFR_FAC_COUNT][2][2][2];
	u16 vars[HFR_VARS_COUNT];
	u16 rents[2][2];                /* v3: 交換の回数 [シングル・ダブル][49・100]（バトルファクトリー） */
	u16 pyrBag[2][10][2];           /* v3: ピラミッドのバトルバッグ [49・100][10 枠][道具番号・個数] */
};

struct HfrRam
{
	u32 sig;
	u8 dirty;
	u8 ext;        /* 1 = 移した変数を入れ物に持つ・0 = 追加の領域が使えない（今までの場所のまま） */
	u8 pad[2];
	struct HfrBlock blk;
};

typedef char HfrBlockSize[(sizeof(struct HfrBlock) == 0x10 + ((HFR_BODY_SIZE + 3) & ~3) && sizeof(struct HfrBlock) <= 0x800) ? 1 : -1];
typedef char HfrRamFits[(HFR_RAM_OFF + sizeof(struct HfrRam) <= 0x8128) ? 1 : -1];

extern struct SaveSection gSaveDataBuffer;
#define HFR_RAM ((struct HfrRam*) (STORAGE_BASE + HFR_RAM_OFF))

static u32 TrainerKey(void)
{
	const u8* id = gSaveBlock2->playerTrainerId;

	return id[0] | (id[1] << 8) | (id[2] << 16) | ((u32) id[3] << 24);
}

static u16 BodySumN(const struct HfrBlock* b, u32 size)
{
	const u16* p = &b->streak[0][0][0][0];   /* streak の後ろに vars・rents が続く */
	u32 i, sum = 0x4852;

	for (i = 0; i < size / 2; ++i)
		sum += p[i] + i;
	return sum;
}

static u16 BodySum(const struct HfrBlock* b)
{
	return BodySumN(b, HFR_BODY_SIZE);
}

static void ZeroBlock(struct HfrRam* r)
{
	u8* p = (u8*) r;
	u32 i;

	for (i = 0; i < sizeof(*r); ++i)
		p[i] = 0;
	r->sig = HFR_RAM_SIG;
	r->blk.magic = HFR_MAGIC;
	r->blk.version = HFR_VERSION;
	r->blk.bodySize = HFR_BODY_SIZE;
	r->blk.trainerId = TrainerKey();
}

//移した変数の、本体の今までの場所（gExpandedVars・古いセーブの値が残っている）
#define LEGACY_VARS ((const u16*) (0x0203B0E8 + 0x200))

static void SeedFromLegacy(struct HfrRam* r)
{
	u32 i;

	for (i = 0; i < HFR_VARS_A_COUNT; ++i)
		r->blk.vars[i] = LEGACY_VARS[HFR_VARS_A_FIRST - 0x5000 + i];
	for (i = 0; i < HFR_VARS_B_COUNT; ++i)
		r->blk.vars[HFR_VARS_A_COUNT + i] = LEGACY_VARS[HFR_VARS_B_FIRST - 0x5000 + i];
	r->ext = TRUE;
	r->dirty = TRUE;
}

//RAM の写し。無ければフラッシュから（合わない・壊れ・読めないなら 0 から・移した変数は本体の古い値を写す）
static struct HfrRam* Ram(void)
{
	struct HfrRam* r = HFR_RAM;

	if (r->sig != HFR_RAM_SIG || r->blk.trainerId != TrainerKey())
	{
		struct HfrBlock* tmp;
		u32 i;

		ZeroBlock(r);
		if (!HackExtSave_Available())
			return r;   //入れ物を使わない（ext = 0）
		tmp = &r->blk;
		if (HackExtSave_Read(HFR_SECTOR, 0, tmp, sizeof(*tmp))
		 && tmp->magic == HFR_MAGIC && tmp->version == HFR_VERSION && tmp->bodySize == HFR_BODY_SIZE
		 && tmp->trainerId == TrainerKey() && tmp->checksum == BodySum(tmp))
			r->ext = TRUE;
		else if (tmp->magic == HFR_MAGIC && tmp->version == HFR_VERSION_V2 && tmp->bodySize == HFR_BODY_SIZE_V2
		 && tmp->trainerId == TrainerKey() && tmp->checksum == BodySumN(tmp, HFR_BODY_SIZE_V2))
		{
			//v2 のセーブ: 今までの中身はそのまま・rents は 0 から・次のレポートで v3 に書き直す（記録を失わない）
			for (i = 0; i < 4; ++i)
				((u16*) tmp->rents)[i] = 0;
			for (i = 0; i < HFR_PYRBAG_COUNT; ++i)
				((u16*) tmp->pyrBag)[i] = 0;   //空のバッグ
			tmp->version = HFR_VERSION;
			tmp->bodySize = HFR_BODY_SIZE;
			r->ext = TRUE;
			r->dirty = TRUE;
		}
		else
		{
			ZeroBlock(r);
			SeedFromLegacy(r);
		}
	}
	return r;
}

//移した変数の番号 → 入れ物の中の場所（範囲の外は NULL）。入れ物を使わない版も NULL（今までの場所を使う）
u16* HoennFrontierRec_VarPtr(u16 id)
{
	struct HfrRam* r;
	u16* p;

	if (id >= HFR_VARS_A_FIRST && id < HFR_VARS_A_FIRST + HFR_VARS_A_COUNT)
		p = (u16*) (u32) (id - HFR_VARS_A_FIRST);
	else if (id >= HFR_VARS_B_FIRST && id < HFR_VARS_B_FIRST + HFR_VARS_B_COUNT)
		p = (u16*) (u32) (HFR_VARS_A_COUNT + id - HFR_VARS_B_FIRST);
	else
		return NULL;
	r = Ram();
	if (!r->ext)
		return NULL;
	r->dirty = TRUE;   //ポインタで書かれるかもしれない（読むだけでも次のレポートで書き直す・施設の中でしか触れない）
	return r->blk.vars + (u32) p;
}

u16 HoennFrontierRec_Get(u8 fac, u8 dbl, u8 mode, bool8 max)
{
	if (fac >= HFR_FAC_COUNT || !HOENN_LVL_SEPARATE(mode))
		return 0;
	return Ram()->blk.streak[fac][dbl != 0][mode == HOENN_LVL_MODE_100][max != 0];
}

void HoennFrontierRec_Set(u8 fac, u8 dbl, u8 mode, bool8 max, u16 value)
{
	struct HfrRam* r;
	u16* p;

	if (fac >= HFR_FAC_COUNT || !HOENN_LVL_SEPARATE(mode))
		return;
	r = Ram();
	p = &r->blk.streak[fac][dbl != 0][mode == HOENN_LVL_MODE_100][max != 0];
	if (*p != value)
	{
		*p = value;
		r->dirty = TRUE;
	}
}

//バトルファクトリーの交換の回数（レベル49・100 だけ。それ以外は 0）
u16 HoennFrontierRec_GetRents(u8 dbl, u8 mode)
{
	if (!HOENN_LVL_SEPARATE(mode))
		return 0;
	return Ram()->blk.rents[dbl != 0][mode == HOENN_LVL_MODE_100];
}

void HoennFrontierRec_SetRents(u8 dbl, u8 mode, u16 value)
{
	struct HfrRam* r;
	u16* p;

	if (!HOENN_LVL_SEPARATE(mode))
		return;
	r = Ram();
	p = &r->blk.rents[dbl != 0][mode == HOENN_LVL_MODE_100];
	if (*p != value)
	{
		*p = value;
		r->dirty = TRUE;
	}
}

//ピラミッドのバトルバッグ（レベル49・100 だけ）→ 10 枠 × {道具番号, 個数} の u16 × 20 の先頭。49・100 以外は NULL。
//ポインタで書かれるかもしれないので dirty にする（次のレポートで書く）。50・オープンは今までの 0x51C6〜0x51ED（HoennFrontierRec_ExpVar）
u16* HoennFrontierRec_PyramidBag(u8 mode)
{
	struct HfrRam* r;

	if (!HOENN_LVL_SEPARATE(mode))
		return NULL;
	r = Ram();
	r->dirty = TRUE;
	return &r->blk.pyrBag[mode == HOENN_LVL_MODE_100][0][0];
}

//拡張変数 id の置き場（構造体の重ね書き用）: 移した範囲で入れ物を使うなら入れ物の中・そうでなければ今までの gExpandedVars
u16* HoennFrontierRec_ExpVar(u16 id)
{
	u16* moved = HoennFrontierRec_VarPtr(id);

	return moved != NULL ? moved : (u16*) (0x0203B0E8 + 0x200) + (id - 0x5000);
}

void HoennFrontierRec_OnLoad(void)
{
	HFR_RAM->sig = 0;   //次に触れたときに読み直す（読んだセーブの記録に戻す）
}

void HoennFrontierRec_AfterSave(bool8 ok)
{
	struct HfrRam* r = HFR_RAM;
	u8* p;
	u32 i;

	if (!ok || r->sig != HFR_RAM_SIG)
		return;
	if (!r->ext && HackExtSave_Available())
		SeedFromLegacy(r);   //最初のレポートのあとで追加の領域が使えるようになった: 本体の値を入れ物へ移して切り替える
	if (!r->dirty)
		return;
	r->blk.checksum = BodySum(&r->blk);
	p = (u8*) &gSaveDataBuffer;
	for (i = 0; i < sizeof(gSaveDataBuffer); ++i)
		p[i] = 0;
	for (i = 0; i < sizeof(r->blk); ++i)
		p[i] = ((const u8*) &r->blk)[i];
	gSaveDataBuffer.signature = HFR_SIGNATURE;
	if (HackExtSave_WriteSector(HFR_SECTOR, &gSaveDataBuffer))
		r->dirty = FALSE;
}

//タワー（記録は CFRU の gBattleTowerStreaks）が 49・100 のとき、CFRU の読み出し（src/frontier.c）をここへ回す
bool8 HoennFrontierRec_TowerStreak(u8 currentOrMax, u16* out)
{
	u16 mode = VarGet(HOENN_VAR_TOWER_LVL_MODE);

	if (VarGet(HOENN_VAR_FRONTIER_FACILITY) != HFR_FAC_TOWER || !HOENN_LVL_SEPARATE(mode))
		return FALSE;
	*out = HoennFrontierRec_Get(HFR_FAC_TOWER, VarGet(HOENN_VAR_TOWER_MODE) == 1, mode, currentOrMax != 0);
	return TRUE;
}
