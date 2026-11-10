#include "defines.h"
#include "../include/event_data.h"
#include "../include/constants/flags.h"
#include "map_region.h"
#include "hack_daycare_region.h"

/*
 * 育て屋の地方ごとの入れ替え（2026-10-21・BUG-daycare-shared-A・案 A）。
 * 4 のしま（カントー）とホウエン 117 の育て屋が SaveBlock1 + 0x2F80 の 1 組を共有していた。
 *
 * - 本体 gSaveBlock1->daycare ＋ 旗 FLAG_PENDING_DAYCARE_EGG（0x266）が「いまの地方」の育て屋。
 * - 控えの置き場は SaveBlock1 + 0x361C の ramScript（1004B・ふしぎなおくりものの RAM スクリプト用。このハックは
 *   その経路を差し替え済みで使わない）。868B を使う（16B のヘッダー + 284B × 3）。
 *   ★ ワイヤレス版のふしぎなおくりもの（RAM スクリプトの受信）を復活させると衝突する。
 * - 地方 0〜3（カントー・ホウエン・ジョウト・シンオウ）が本体 1 組 + 控え 3 組に収まる。キタカミ（4）以降は育て屋なし＝何もしない。
 * - 古いセーブ（ramScript がゼロ）: ヘッダー不正として初期化し、今の本体と旗を「初めて走った地方」のものにする。
 * - 入れ替えは RAM 上の同期処理（保存と同時に走らない）。保存はスロット世代の切り替えなので、読み込まれるのは
 *   いつも入れ替え前か後の一貫した 1 世代。
 * - 呼び元: src/overworld.c RunOnTransitionMapScript／RunOnResumeMapScript の先頭（patches/0640）
 */

#define DAYCARE_REGIONS     4
#define STASH_SLOTS         3
#define STASH_MAGIC         0x57535944  /* 'DYSW' */
#define STASH_VERSION       1
#define STASH_EMPTY         0xFF

struct HackDaycareStash
{
	u32 magic;
	u8 version;
	u8 active;              /* 本体 daycare と旗が今どの地方のものか */
	u8 tag[STASH_SLOTS];    /* 控えの地方（0xFF = 空き） */
	u8 eggBits;             /* bit i = 控え i の「タマゴが来ている」旗 */
	u16 sum;
	u8 pad[4];
	struct DayCare slot[STASH_SLOTS];
};

/* 控えが ramScript（1004B）に収まる */
typedef char HackDaycareStashFits[(sizeof(struct HackDaycareStash) <= sizeof(struct RamScript)) ? 1 : -1];
typedef char HackDaycareStashSize[(sizeof(struct HackDaycareStash) == 16 + 3 * 284) ? 1 : -1];

#define STASH ((struct HackDaycareStash*) &gSaveBlock1->ramScript)

static u16 StashSum(const struct HackDaycareStash *s)
{
	const u8 *b = (const u8*) s;
	u16 sum = 0x5A;
	u32 i;

	for (i = 4; i < 10; ++i)
		sum = (u16) (sum + b[i] * (i + 1));
	return sum ^ 0xA5A5;
}

static bool8 StashValid(const struct HackDaycareStash *s)
{
	u32 i;

	if (s->magic != STASH_MAGIC || s->version != STASH_VERSION || s->sum != StashSum(s) || s->active >= DAYCARE_REGIONS)
		return FALSE;
	for (i = 0; i < STASH_SLOTS; ++i)
	{
		if (s->tag[i] != STASH_EMPTY && (s->tag[i] >= DAYCARE_REGIONS || s->tag[i] == s->active))
			return FALSE;
	}
	return TRUE;
}

static void StashInit(struct HackDaycareStash *s, u8 region)
{
	u32 *w = (u32*) s;
	u32 i;

	for (i = 0; i < sizeof(*s) / 4; ++i)
		w[i] = 0;
	s->magic = STASH_MAGIC;
	s->version = STASH_VERSION;
	s->active = region;
	for (i = 0; i < STASH_SLOTS; ++i)
		s->tag[i] = STASH_EMPTY;
	s->sum = StashSum(s);
}

void HackDaycare_SyncRegion(void)
{
	struct HackDaycareStash *s = STASH;
	u8 r = GetCurrentMapRegion();
	u32 *a, *b;
	u32 i, k, t;
	u8 cur, next;

	if (r >= DAYCARE_REGIONS)
		return;
	if (!StashValid(s))
	{
		StashInit(s, r);    /* 今の本体と旗を、この地方のものとして引き取る */
		return;
	}
	if (s->active == r)
		return;

	for (i = 0; i < STASH_SLOTS && s->tag[i] != r; ++i);
	if (i == STASH_SLOTS)
	{
		for (i = 0; i < STASH_SLOTS && s->tag[i] != STASH_EMPTY; ++i);
		if (i == STASH_SLOTS)   /* 起きないはず（ヘッダーの検査で止まる） */
		{
			StashInit(s, r);
			return;
		}
	}

	a = (u32*) &gSaveBlock1->daycare;
	b = (u32*) &s->slot[i];
	for (k = 0; k < sizeof(struct DayCare) / 4; ++k)
	{
		t = a[k];
		a[k] = b[k];
		b[k] = t;
	}

	cur = FlagGet(FLAG_PENDING_DAYCARE_EGG) ? 1 : 0;
	next = (s->eggBits >> i) & 1;
	s->eggBits = (u8) ((s->eggBits & ~(1 << i)) | (cur << i));
	if (next)
		FlagSet(FLAG_PENDING_DAYCARE_EGG);
	else
		FlagClear(FLAG_PENDING_DAYCARE_EGG);

	s->tag[i] = s->active;
	s->active = r;
	s->sum = StashSum(s);
}

struct DayCare *HackDaycare_GetStashed(u8 i)
{
	struct HackDaycareStash *s = STASH;

	if (i >= STASH_SLOTS || !StashValid(s) || s->tag[i] == STASH_EMPTY)
		return NULL;
	return &s->slot[i];
}
