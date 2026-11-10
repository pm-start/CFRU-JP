#include "defines.h"
#include "../include/save.h"
#include "../include/event_data.h"
#include "Tables/hoenn_script_flags.h"
#include "hack_ext_save.h"
#include "hack_box_shelf.h"
#include "hack_moemon_gen.h"

/*
 * Moémon の絵の世代の控え（[hack] 0.63.1）。置き場・形・決まりは hack_moemon_gen.h と docs/research/2026-11-03_moemon-generations.md。
 *   読み（DPE の HackMoemon_Slot が当たりを通ったあと、絵の枚数が世代で変わる種族のときだけ聞く）: 性格値の二分探索（件は昇順・1 件 8 バイト）。
 *     壊れ・世代の合わない区画・読めない・使えない環境は「世代 1」（今の絵のまま）。控え待ち（doneGen = 0）も世代 1。
 *     控え済みで表にない子は最新の世代。件 0 の控え済み（はじめから）は区画を見ずに最新の世代。
 *   書き（つづきから後に 1 回だけ）: 持ち主の列挙（hack_moemon_gen_owners.c）で今いる Moémon の当たりの子の性格値を集め、
 *     表にあった子はその世代・無い子は「それまでの世代」で 1 区画にまとめて書く。書くのは「前のレポートで正だった区画でないほう」＝
 *     レポートの前にリセットすれば SaveBlock1 の状態は元の sel（無傷の区画）に戻り、次のつづきからでやり直す（何回やっても同じ結果）。
 *     使えない環境・集められなかったとき・書けなかったときは何も変えない（控え待ちのまま）。
 *   1 回で終わらせる理由: 集めた性格値を置く入れ物が、フィールドの何コマにもまたがる形では安全に持てない（ヒープは地図の読み込みで消える・
 *     gSaveDataBuffer はレポートや他の表の書き込みが使う・static は ROM 宛て＝6.13）。中の読みは 5 匹ずつの一括読みで、1 回に約 1 秒。
 */

#define MGN1_MAGIC 0x314E474D      //'MGN1'
#define MGNB_MAGIC 0x424E474D      //'MGNB'
#define MOEMONGEN_SIGNATURE 0x08012025    //ほかの追加の区画と同じ（道具が署名つきの区画として読める）

typedef char MoemonGenEntrySize[(sizeof(struct MoemonGenEntry) == 8) ? 1 : -1];
typedef char MoemonGenSaveSize[(sizeof(struct MoemonGenSave) == 12) ? 1 : -1];
typedef char MoemonGenBlockFits[(sizeof(struct MoemonGenBlock) <= 0xFF4 && sizeof(struct MoemonGenBlock) == 16 + 8 * MOEMONGEN_CAPACITY) ? 1 : -1];
typedef char MoemonGenSb1Fits[(MOEMONGEN_SB1_OFF == 0x3A14 && MOEMONGEN_SB1_OFF + sizeof(struct MoemonGenSave) <= 0x3A4C) ? 1 : -1];   //SaveBlock1 の +0x3A08〜+0x3A4C（rivalName の手前）の中・技の回数の状態（+0x3A08〜+0x3A14）の次

#include "hack_save_version.h"
typedef char MoemonGenNeedsVersion[(HACK_SAVE_VERSION >= HACK_MOEMON_GEN_VERSION) ? 1 : -1];   //この版以上でないと はじめからの見分け（HACK_VAR_SAVE_VERSION_FIRST）も、古い版が表つきのセーブを「新しすぎる」と止めることもできない

extern struct SaveSection gSaveDataBuffer;

#define SV ((struct MoemonGenSave*) ((u8*) gSaveBlock1 + MOEMONGEN_SB1_OFF))

static u16 Sum16(const u8* p, u32 n)
{
	u32 i, t = 0x4D47;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static u16 EntryCheck(const struct MoemonGenEntry* e)
{
	return Sum16((const u8*) e, 6);
}

static bool8 StateOk(const struct MoemonGenSave* s)
{
	return s->magic == MGN1_MAGIC && s->count <= MOEMONGEN_CAPACITY && s->sel <= 1;
}

static u8 Latest(const struct MoemonGenSave* s)
{
	return s->doneGen >= MOEMONGEN_GEN_LATEST ? MOEMONGEN_GEN_LATEST : s->doneGen;
}

//---- 読み ----

#define FIND_NO 0          //表に無い
#define FIND_YES 1         //表にある（*gen）
#define FIND_BAD 2         //読めない・壊れ・使えない環境

static u8 FindGen(const struct MoemonGenSave* s, u32 pid, u8* gen)
{
	u32 head[4];                 //頭の 16 バイトだけ（MoemonGenBlock 全体を積まない＝スタック 1KB の決まり・6.35）
	struct MoemonGenEntry e;
	s32 lo = 0, hi = (s32) s->count - 1;

	if (!BoxShelf_ExtAvailable())
		return FIND_BAD;
	if (!HackExtSave_ReadKnownAvail(MOEMON_GEN_SECTOR + (s->sel & 1), 0, head, 16))
		return FIND_BAD;
	if (head[0] != MGNB_MAGIC || (head[1] & 0xFFFF) != 1 || (head[2] & 0xFFFF) != s->gen || (head[2] >> 16) != s->count)
		return FIND_BAD;         //'MGNB'・版 1・世代・件数（head[1] の上 16 ビットは和）
	while (lo <= hi)
	{
		s32 mid = (lo + hi) / 2;

		if (!HackExtSave_ReadKnownAvail(MOEMON_GEN_SECTOR + (s->sel & 1), 16 + mid * 8, &e, 8) || e.check != EntryCheck(&e))
			return FIND_BAD;
		if (e.pid == pid)
		{
			*gen = e.gen;
			return FIND_YES;
		}
		if (e.pid < pid)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return FIND_NO;
}

u32 HackMoemonGen_Of(u32 personality)
{
	struct MoemonGenSave* s = SV;
	u8 gen = 0;

	if (!StateOk(s) || s->doneGen == 0)
		return 1;                          //控え待ち（まだ控えていない・使えない環境）: 全員 世代 1＝今の絵のまま
	if (s->count == 0)
		return Latest(s);                  //表を持たない（はじめから・今いる当たりの子が 0 匹）: 全員が最新の世代
	switch (FindGen(s, personality, &gen))
	{
		case FIND_YES:
			return gen == 0 ? 1 : gen;
		case FIND_NO:
			return Latest(s);              //控えにない子＝それ以降に来た子
		default:
			return 1;                      //読めない: 今の絵の側に倒す（新しい絵は出さない）
	}
}

//---- 書き ----

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

//性格値を昇順の並びに足す（重複なし）。入りきらない子は数えるだけ（最新の世代として見える側に倒れる）
void HackMoemonGen_AddHit(struct MoemonGenCollect* c, u32 pid)
{
	s32 lo = 0, hi = (s32) c->n - 1, i;

	if (pid == 0)
		return;
	while (lo <= hi)
	{
		s32 mid = (lo + hi) / 2;

		if (c->pids[mid] == pid)
			return;
		if (c->pids[mid] < pid)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	if (c->n >= MOEMONGEN_CAPACITY)
	{
		++c->overflow;
		return;
	}
	for (i = (s32) c->n; i > lo; --i)
		c->pids[i] = c->pids[i - 1];
	c->pids[lo] = pid;
	++c->n;
}

//状態だけ「控え済み・件 0」にする（区画は書かない）
static void MarkDoneEmpty(struct MoemonGenSave* s)
{
	s->magic = MGN1_MAGIC;
	s->gen = 1;
	s->sum = 0;
	s->count = 0;
	s->sel = 0;
	s->doneGen = MOEMONGEN_GEN_LATEST;
}

static void Scan(struct MoemonGenSave* s)
{
	struct MoemonGenCollect c;
	struct MoemonGenBlock* w;
	bool8 valid = StateOk(s);
	u8 prevGen = (valid && s->doneGen != 0) ? s->doneGen : 1;     //控えにない子がいた世代（まだ控えていないセーブは 世代 1）
	u8 target = (valid && s->count != 0) ? (s->sel ^ 1) : 0;       //前のレポートで正だった区画でないほう
	u16 gen = valid ? s->gen + 1 : 1;
	u32 i;

	c.pids = Malloc(MOEMONGEN_CAPACITY * sizeof(u32));
	if (c.pids == NULL)
		return;
	c.n = 0;
	c.overflow = 0;
	if (!HackMoemonGen_CollectAll(&c))
	{
		Free(c.pids);
		return;
	}
	if (c.n == 0)
	{
		MarkDoneEmpty(s);                  //今いる当たりの子が 1 匹もいない: 表は要らない（区画を書かない）
		Free(c.pids);
		return;
	}
	Zero(&gSaveDataBuffer, sizeof(gSaveDataBuffer));
	w = (struct MoemonGenBlock*) gSaveDataBuffer.data;
	for (i = 0; i < c.n; ++i)
	{
		u8 g = prevGen;
		struct MoemonGenEntry* e = &w->e[i];

		if (valid && s->count != 0)        //前の世代の表にあった子はその世代のまま持ち越す
		{
			u8 old = 0;

			if (FindGen(s, c.pids[i], &old) == FIND_YES && old != 0)
				g = old;
		}
		e->pid = c.pids[i];
		e->gen = g;
		e->check = EntryCheck(e);
	}
	// FindGen は区画を読む（gSaveDataBuffer は触らない）ので、上の組み立てと混ざらない
	w->magic = MGNB_MAGIC;
	w->version = 1;
	w->gen = gen;
	w->count = c.n;
	w->sum = Sum16((const u8*) w->e, c.n * 8);
	gSaveDataBuffer.signature = MOEMONGEN_SIGNATURE;
	Free(c.pids);
	if (!HackExtSave_WriteSector(MOEMON_GEN_SECTOR + target, &gSaveDataBuffer))
		return;                            //書けなかった: 何も変えない（控え待ちのまま・次の機会にやり直す）
	s->magic = MGN1_MAGIC;
	s->gen = gen;
	s->sum = w->sum;
	s->count = c.n;
	s->sel = target;
	s->doneGen = MOEMONGEN_GEN_LATEST;
}

//---- 毎コマ ----

void HackMoemonGen_FieldTick(void)
{
	struct MoemonGenSave* s = SV;

	if (StateOk(s) && s->doneGen >= MOEMONGEN_GEN_LATEST)
		return;                            //控え済み（毎コマの固定費はここまで）
	if ((gMain.vblankCounter2 & 0xF) != 0)
		return;
	//★ 2026-11-08 FEAT-moemon-scovillain（世代 3・LATEST を初めて上げた）: はじめからで「控え済み・件 0・前の世代（doneGen が 0 でない）」のセーブを
	//  ここで最新の世代に上げると、今いる子の絵の選びが変わる（世代 3 は 2 の並びに 1 枚足すので、同じ性格値でも絵が替わる）。
	//  前の世代で印を付け済み（doneGen != 0）のセーブは下の控える処理（Scan）に回し、今いる子を前の世代（doneGen）で控える。
	if (VarGet(HACK_VAR_SAVE_VERSION_FIRST) >= HACK_MOEMON_GEN_VERSION && !(StateOk(s) && (s->count != 0 || s->doneGen != 0)))
	{
		MarkDoneEmpty(s);                  //この版以降にはじめたセーブ: 今いる子は全部新しい世代の子（表は要らない）
		return;
	}
	if ((gMain.vblankCounter2 & 0x7F) != 0x10)
		return;                            //控える処理は 128 コマに 1 回だけ試す（使えない環境の確かめを毎コマしない）
	if (!BoxShelf_ExtAvailable())
		return;                            //使えない環境: 控え待ちのまま（全部 世代 1＝今の絵）
	Scan(s);
}
