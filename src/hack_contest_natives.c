#include "defines.h"
#include "../include/event_data.h"
#include "../include/fieldmap.h"
#include "../include/new/ram_locs.h"
#include "../include/string_util.h"
#include "../include/new/character_customization.h"
#include "hack_contest.h"

/*
 * コンテスト移植 S5（FEAT-contest・patches/0985）: 台本（エメラルドの contest_hall.inc）の `special X` を `callnative HoennNative_X` に読み替える窓口。
 * callnative に渡す C の関数は HoennNative_ で始める決まり（docs/claude/06_risks.md 6.42）。中身は原文の同名の special（hack_contest_entry.c・hack_contest_result.c・hack_contest_resultscreen.c）。
 * 戻り値のある special（specialvar）は Var800D（VAR_RESULT）に入れる。
 */
void HoennNative_TryEnterContestMon(void)
{
	TryEnterContestMon();
}

void HoennNative_SetContestTrainerGfxIds(void)
{
	SetContestTrainerGfxIds();
}

void HoennNative_ShowContestEntryMonPic(void)
{
	ShowContestEntryMonPic();
}

void HoennNative_HideContestEntryMonPic(void)
{
	HideContestEntryMonPic();
}

void HoennNative_BufferContestTrainerAndMonNames(void)
{
	BufferContestTrainerAndMonNames();
}

void HoennNative_GenerateContestRand(void)
{
	GenerateContestRand();
}

void HoennNative_ShouldReadyContestArtist(void)
{
	ShouldReadyContestArtist();
}

void HoennNative_GetContestWinnerId(void)
{
	GetContestWinnerId();
}

void HoennNative_GetContestPlayerId(void)
{
	GetContestPlayerId();
}

void HoennNative_BufferContestWinnerTrainerName(void)
{
	BufferContestWinnerTrainerName();
}

void HoennNative_BufferContestWinnerMonName(void)
{
	BufferContestWinnerMonName();
}

void HoennNative_BufferContestantTrainerName(void)
{
	BufferContestantTrainerName();
}

void HoennNative_BufferContestantMonNickname(void)
{
	BufferContestantMonNickname();
}

void HoennNative_BufferContestantMonSpecies(void)
{
	BufferContestantMonSpecies();
}

void HoennNative_GetContestMonCondition(void)
{
	GetContestMonCondition();
}

void HoennNative_GiveMonContestRibbon(void)
{
	GiveMonContestRibbon();
}

void HoennNative_ShowContestResults(void)
{
	ShowContestResults();
}

//specialvar VAR_RESULT, HasMonWonThisContestBefore（原文: Var8004 の手持ちが今回の部門・ランクで勝ったことがあるか）
void HoennNative_HasMonWonThisContestBefore(void)
{
	Var800D = HasMonWonThisContestBefore();
}

// ---- S5c: 会場（ContestHall）の台本が使う窓口 ----

//台本の `startcontest`（素の FR の命令は止まる stub）→ 会場の画面へ（S5a の StartHackContest・終わると台本の続きへ戻る）
void HoennNative_StartContest(void)
{
	StartHackContest();
}

//台本の `bufferstdstring STR_VAR_3, STDSTRING_NORMAL〜MASTER`（FR の標準文字列の表にランク名が無い）→ gStringVar3 にランクの名前（日本語版エメラルドの gText_Normal〜Master と同じ字）
static const u8 sHackContestRankNormal[] = {0x69, 0xAE, 0x6F, 0x79, 0xFF}; //ノーマル
static const u8 sHackContestRankSuper[] = {0x5D, 0xAE, 0x9B, 0xAE, 0xFF}; //スーパー
static const u8 sHackContestRankHyper[] = {0x6A, 0x52, 0x9B, 0xAE, 0xFF}; //ハイパー
static const u8 sHackContestRankMaster[] = {0x6F, 0x5D, 0x60, 0xAE, 0xFF}; //マスター
void HoennNative_BufferContestRankName(void)
{
	static const u8* const names[] = {sHackContestRankNormal, sHackContestRankSuper, sHackContestRankHyper, sHackContestRankMaster};
	u8 rank = HACK_CONTEST_KEEP.rank;

	StringCopy(gStringVar3, names[rank & 3]);
}

//ロビーの台本の SetPlayerGfx（原文: VAR_OBJ_GFX_ID_3 にユウキ／ハルカ＝エメラルドの主人公）→ 今の主人公の歩き姿を会場の出場者 4 番（VAR_OBJ_GFX_ID_3＝0x4013）の絵に（Dome・パレスの HoennPalace_SetPlayerGfx と同じ考え方）
void HoennNative_SetContestPlayerGfx(void)
{
	VarSet(0x4013, GetEventObjectGraphicsId(&gEventObjects[gPlayerAvatar->eventObjectId]));
}

// ---- S6: 優勝者の記録・絵の画面・絵描き（ロビーの台本）が使う窓口 ----

//台本の `showcontestpainting X`（素の FR に無い）→ Var8004＝X にして絵の画面へ（0＝絵描きの絵・1〜13＝記録）。台本は waitstate で待つ（閉じると続きへ戻る）
void HoennNative_ShowContestPainting(void)
{
	ShowContestPainting();
}

void HoennNative_SaveMuseumContestPainting(void)
{
	SaveMuseumContestPainting();
}

//specialvar VAR_0x8004, CountPlayerMuseumPaintings
void HoennNative_CountPlayerMuseumPaintings(void)
{
	Var8004 = CountPlayerMuseumPaintings();
}

//specialvar VAR_RESULT, GiveMonArtistRibbon
void HoennNative_GiveMonArtistRibbon(void)
{
	Var800D = GiveMonArtistRibbon();
}
