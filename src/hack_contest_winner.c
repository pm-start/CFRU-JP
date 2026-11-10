#include "defines.h"
#include "../include/save.h"
#include "../include/random.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "hack_ext_save.h"
#include "hack_contest.h"
#include "hack_contest_painting_data.h"

/*
 * コンテスト移植 S6（FEAT-contest・patches/0985）: 優勝者の記録。原文 参考/pokeemerald/src/contest.c:5522〜5636（SaveContestWinner・GetContestWinnerSaveIdx・
 * ClearContestWinnerPicsInContestHall）・contest_util.c:2362〜2392（SaveMuseumContestPainting・CountPlayerMuseumPaintings）・2549（GiveMonArtistRibbon）。
 * 見本は src/hack_relive_box.c（追加の区画の交互 2 区画）。
 *   - 中身は追加の区画 CONTEST_WINNER_SECTOR（93）と +1 の交互 2 区画（ブロック 1 つ＝432 バイト＝ヘッダ 16＋記録 13 件×32）。13 件＝ホール 8（絵は 6 枚＋予備 2）＋美術館 5（部門ごと）。
 *   - 「どちらが正か」は SaveBlock1 +0x0454 の 4 バイト（stat_pool の予約の残り・印・sel・committedSel・版）＝本体の組の中なので、レポートで手持ちと一緒に書かれる。
 *   - 変えるたびに「前のレポートで正だった区画でないほう」へ書き、sel をそちらへ向ける。レポートの前にリセットすれば SaveBlock1 は前の sel（無傷の区画）に戻る。
 *   - 状態が無い（はじめから・別のセーブ）・追加の領域が使えない・壊れ のときは、ホールは日本語版の初期の 8 件・美術館は空として読む＝区画の古い中身は信じない。
 *   - 読むだけの関数は SaveBlock1 を書き換えない。
 * 使えない環境（SuperCard・ふつうの mGBA）は記録しない（案 A・ユーザー確認待ち）。絵描きの一時の絵（保存しない物）は固定 RAM HACK_ARTIST_WINNER に作るので使える。
 */

#define WIN_SB1_MAGIC 0x57
#define WIN_SB1_OFF 0x0454
#define WIN_BLOCK_MAGIC 0x314E5743   //'CWN1'
#define WIN_BLOCK_VERSION 2             //2＝S8（ミナモのおねえさんの状態を後ろに足した）。1 は優勝者だけ（読める・おねえさんは「無し」＝初期化）
#define WIN_BLOCK_VERSION_V1 1
#define WIN_V1_SIZE (16 + 32 * 13)      //version 1 の和をとる長さ（ヘッダ 16 を含む）
#define LADY_MAGIC 0x4C
#define WIN_SIGNATURE 0x08012025     //ほかの追加の区画と同じ

struct WinSave                       //SaveBlock1 + 0x0454（4 バイト・本体の組に入る）
{
	u8 magic;
	u8 sel;                          //今の中身がある区画（0 = CONTEST_WINNER_SECTOR・1 = +1）
	u8 committedSel;                 //最後に書き終えたレポートの sel
	u8 version;
};

struct WinBlock                      //追加の区画 1 つに書く（先頭から）
{
	u32 magic;
	u16 version;
	u16 sum;                         //先頭 16 バイトより後ろの和
	u32 spare[2];
	struct HackContestWinner w[HACK_WINNER_COUNT];
	u32 ladyMagic;                   //S8（version 2）: 'LADY'＝後ろの 0x40 バイトに中身あり
	struct HackLadySave lady;
};

#define WIN_SB1 ((struct WinSave*) ((u8*) gSaveBlock1 + WIN_SB1_OFF))

typedef char WinSaveFits[(sizeof(struct WinSave) == 4 && WIN_SB1_OFF == 0x454 && WIN_SB1_OFF + 4 <= 0x458) ? 1 : -1];   //stat_pool の 0x0448＋12 の次〜safeBackupParty（0x0458）の手前
typedef char WinBlockFits[(sizeof(struct WinBlock) == WIN_V1_SIZE + 4 + 0x40 && sizeof(struct WinBlock) <= 0xFF4) ? 1 : -1];     //gSaveDataBuffer.data（0xFF4）に収まる

extern struct SaveSection gSaveDataBuffer;

static u16 SumBytes(const struct WinBlock* b, u32 total)   //total＝ヘッダを含む長さ（version 1＝WIN_V1_SIZE・version 2＝sizeof）
{
	const u8* p = (const u8*) b + 16;
	u32 n = total - 16, i, t = 0x4357;

	for (i = 0; i < n; ++i)
		t += p[i] + (i & 0xFF);
	return t & 0xFFFF;
}

static bool8 StateOk(const struct WinSave* s)
{
	return s->magic == WIN_SB1_MAGIC && s->version == 1 && s->sel <= 1 && s->committedSel <= 1;
}

static void Zero(void* p, u32 n)
{
	u8* b = p;
	u32 i;

	for (i = 0; i < n; ++i)
		b[i] = 0;
}

static void CopyBytes(void* dst, const void* src, u32 n)
{
	u8* d = dst;
	const u8* s = src;
	u32 i;

	for (i = 0; i < n; ++i)
		d[i] = s[i];
}

bool8 HackContest_WinnersUsable(void)
{
	return HackExtSave_Check() == 1;
}

//今の正の区画を b に読む（version 1・2 とも）。状態が無い・使えない・壊れ＝FALSE。version 1 は後ろ（おねえさん）をゼロにして返す
static bool8 ReadBlock(struct WinBlock* b)
{
	const struct WinSave* s = WIN_SB1;

	if (!StateOk(s) || !HackContest_WinnersUsable())
		return FALSE;
	Zero(b, sizeof(*b));
	if (!HackExtSave_Read(CONTEST_WINNER_SECTOR + (s->sel & 1), 0, b, sizeof(*b)))
		return FALSE;
	if (b->magic != WIN_BLOCK_MAGIC)
		return FALSE;
	if (b->version == WIN_BLOCK_VERSION_V1)
	{
		if (b->sum != SumBytes(b, WIN_V1_SIZE))
			return FALSE;
		b->ladyMagic = 0;
		Zero(&b->lady, sizeof(b->lady));
		return TRUE;
	}
	return b->version == WIN_BLOCK_VERSION && b->sum == SumBytes(b, sizeof(*b));
}

//記録 13 件を out へ。使えない・状態が無い・壊れ＝ホールは初期の 8 件・美術館は空
void HackContest_LoadWinners(struct HackContestWinner* out)
{
	struct WinBlock b;
	u32 i;

	Zero(out, sizeof(struct HackContestWinner) * HACK_WINNER_COUNT);
	for (i = 0; i < HACK_WINNER_HALL_COUNT; ++i)
		out[i] = gHackDefaultContestWinners[i];
	if (!ReadBlock(&b))
		return;
	CopyBytes(out, b.w, sizeof(b.w));
}

//おねえさんの状態を区画から（S8）。version 2 で中身あり＝TRUE
bool8 HackContest_LoadLady(struct HackLadySave* out)
{
	struct WinBlock b;

	if (!ReadBlock(&b) || b.ladyMagic != LADY_MAGIC || b.lady.magic != LADY_MAGIC)
		return FALSE;
	CopyBytes(out, &b.lady, sizeof(*out));
	return TRUE;
}

//今の区画のおねえさんの状態を b へ写す（無ければ何もしない）。読み込みの入れ物（WinBlock）を CommitEx の枠に持たない（まとめ 0.66.0: 2 つ重なって枠が 1024 バイトを越えた＝verify_stack_usage）ので、別の関数に分ける
static void __attribute__((noinline)) CarryLady(struct WinBlock* b)
{
	struct WinBlock old;

	if (ReadBlock(&old))
	{
		b->ladyMagic = old.ladyMagic;
		CopyBytes(&b->lady, &old.lady, sizeof(b->lady));
	}
}

//out の 13 件を、「前のレポートで正だった区画」でないほうへ書き、sel をそちらへ向ける。書けなければ FALSE（sel は変えない）。
//おねえさんの状態（lady）は NULL なら今の区画のものを引き継ぐ（無ければ「無し」）
static bool8 CommitEx(const struct HackContestWinner* w, const struct HackLadySave* lady)
{
	struct WinBlock b;
	bool8 wasOk = StateOk(WIN_SB1);
	struct WinSave* s = WIN_SB1;
	u8 target;
	u8* dst = (u8*) &gSaveDataBuffer;
	u32 i;

	if (!wasOk)
	{
		Zero(s, sizeof(*s));
		s->magic = WIN_SB1_MAGIC;
		s->version = 1;
		s->sel = 1;
		s->committedSel = 1;           //最初の書き込みは区画 0（target = sel ^ 1）へ
	}
	target = (s->sel == s->committedSel) ? (s->sel ^ 1) : s->sel;
	if (lady == NULL)
	{
		Zero(&b, sizeof(b));
		CarryLady(&b);                 //今の区画のおねえさんの状態を引き継ぐ（無ければ「無し」）
	}
	else
	{
		Zero(&b, sizeof(b));
		b.ladyMagic = LADY_MAGIC;
		CopyBytes(&b.lady, lady, sizeof(b.lady));
	}
	b.magic = WIN_BLOCK_MAGIC;
	b.version = WIN_BLOCK_VERSION;
	CopyBytes(b.w, w, sizeof(b.w));
	b.sum = SumBytes(&b, sizeof(b));
	for (i = 0; i < sizeof(gSaveDataBuffer); ++i)
		dst[i] = 0;
	CopyBytes(dst, &b, sizeof(b));
	gSaveDataBuffer.signature = WIN_SIGNATURE;
	if (!HackExtSave_WriteSector(CONTEST_WINNER_SECTOR + target, &gSaveDataBuffer))
	{
		if (!wasOk)
			Zero(s, sizeof(*s));           //書けなかったときは状態も作らない（空のセーブは 1 バイトも変わらない）
		return FALSE;
	}
	s->sel = target;
	return TRUE;
}

static bool8 Commit(const struct HackContestWinner* w)
{
	return CommitEx(w, NULL);
}

//おねえさんの状態を書く（優勝者の記録は今のものを引き継ぐ）。使えない環境は FALSE
bool8 HackContest_CommitLady(const struct HackLadySave* lady)
{
	struct HackContestWinner w13[HACK_WINNER_COUNT];

	if (!HackContest_WinnersUsable())
		return FALSE;
	HackContest_LoadWinners(w13);
	return CommitEx(w13, lady);
}

//原文 GetContestWinnerSaveIdx（shift なし）: ランク 0〜3＝ホールの 0 番・それ以外（美術館・絵描き）＝部門の枠 8〜12
u8 HackContest_WinnerSaveIdx(u8 rank)
{
	u8 cat = HACK_CONTEST_KEEP.category;

	if (rank <= 3)
		return 0;
	return HACK_WINNER_HALL_COUNT + (cat > 4 ? 4 : cat);
}

//原文 SaveContestWinner（contest.c:5522）。優勝者は NPC でも記録する（原文どおり）。Keep（結果の記録）から読む
bool8 SaveContestWinner(u8 rank)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;
	struct HackContestWinner w13[HACK_WINNER_COUNT];
	struct HackContestWinner* dst;
	struct HackContestKeepMon* m;
	u8 captionId, cat, i, id;

	if (k->magic != HACK_CONTEST_KEEP_MAGIC)
		return FALSE;
	cat = k->category > 4 ? 4 : k->category;
	captionId = Random() % HACK_PAINTING_CAPTIONS + HACK_PAINTING_CAPTIONS * cat;   //原文: 乱数の字幕に部門ぶんを足す
	for (i = 0; i < HACK_CONTEST_CONTESTANTS - 1; ++i)
		if (k->finalStandings[i] == 0)
			break;
	if (rank == HACK_SAVE_FOR_MUSEUM && i != k->playerIndex)
		return FALSE;                  //美術館へ入れられるのは自分のポケモンだけ
	m = &k->mons[i];
	if (rank == HACK_SAVE_FOR_ARTIST)
	{
		dst = &HACK_ARTIST_WINNER;     //絵描きに見せる直近の優勝者（保存しない）
		Zero(dst, sizeof(*dst));
		dst->personality = m->personality;
		dst->trainerId = m->otId;
		dst->species = m->species;
		CopyBytes(dst->monName, m->nickname, sizeof(dst->monName));
		CopyBytes(dst->trainerName, m->trainerName, sizeof(dst->trainerName));
		dst->category = captionId;
		return TRUE;
	}
	if (!HackContest_WinnersUsable())
		return FALSE;                  //使えない環境は記録しない（案 A）
	HackContest_LoadWinners(w13);
	id = HackContest_WinnerSaveIdx(rank);
	if (rank <= 3)
		for (i = HACK_WINNER_HALL_COUNT - 1; i > 0; --i)
			w13[i] = w13[i - 1];       //ホールの履歴を 1 つずつ後ろへ（原文 contest.c:5602）
	dst = &w13[id];
	Zero(dst, sizeof(*dst));
	dst->personality = m->personality;
	dst->species = m->species;
	dst->trainerId = m->otId;
	CopyBytes(dst->monName, m->nickname, sizeof(dst->monName));
	CopyBytes(dst->trainerName, m->trainerName, sizeof(dst->trainerName));
	dst->rank = k->rank;               //通信は未対応＝CONTEST_RANK_LINK は無い
	dst->category = (rank != HACK_SAVE_FOR_MUSEUM) ? cat : captionId;
	return Commit(w13);
}

//原文 contest_util.c:2362
void SaveMuseumContestPainting(void)
{
	SaveContestWinner(HACK_SAVE_FOR_MUSEUM);
}

//原文 contest_util.c:2381: 美術館の 5 枠（8〜12）のうち絵のある数
u8 CountPlayerMuseumPaintings(void)
{
	struct HackContestWinner w13[HACK_WINNER_COUNT];
	u8 i, n = 0;

	HackContest_LoadWinners(w13);
	for (i = HACK_WINNER_HALL_COUNT; i < HACK_WINNER_COUNT; ++i)
		if (w13[i].species != 0)
			++n;
	return n;
}

//結果発表の始め（原文 contest_util.c:662〜666。通信でない側）: 優勝者をホールの履歴へ・絵描きに見せる一時の優勝者も作る
void HackContest_SaveWinner(void)
{
	SaveContestWinner(HACK_CONTEST_KEEP.rank);
	SaveContestWinner(HACK_SAVE_FOR_ARTIST);
}

//原文 GiveMonArtistRibbon（contest_util.c:2549）。リボンを付ける所は仮の口 1 か所
bool8 GiveMonArtistRibbon(void)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;

	if (!HackContest_HasArtistRibbon(k->partyIndex) && k->finalStandings[k->playerIndex] == 0 && k->rank == 3 && k->totalPoints[k->playerIndex] >= 800)
		return HackContest_AwardArtistRibbon(k->partyIndex);
	return FALSE;
}

//げいじゅつかリボンを付ける（原文 GiveMonArtistRibbon の SetMonData・まとめ 0.66.0: 本物の手持ちのリボンに）。付けたら TRUE（リボンの場面が出る）。debug 版は Var800F に印
bool8 HackContest_AwardArtistRibbon(u8 partyIndex)
{
	u8 one = 1;

	SetMonData(&gPlayerParty[partyIndex], MON_DATA_ARTIST_RIBBON, &one);
#ifdef HACK_DEBUG_MENU
	Var800F = 99;
#endif
	return TRUE;
}

u8 HackContest_HasArtistRibbon(u8 partyIndex)
{
	return GetMonData(&gPlayerParty[partyIndex], MON_DATA_ARTIST_RIBBON, NULL) != 0;
}

void Winner_OnLoad(void)
{
	struct WinSave* s = WIN_SB1;

	HackLady_Invalidate();                 //S8: おねえさんの RAM の写しを捨てる（次に読むとき区画から）
	if (StateOk(s))
		s->committedSel = s->sel;          //つづきから: 読んだ sel が正（状態が無いセーブは 1 バイトも書かない）
}

void Winner_AfterSave(bool8 ok)
{
	struct WinSave* s = WIN_SB1;

	if (ok && StateOk(s))
		s->committedSel = s->sel;
}
