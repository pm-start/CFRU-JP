#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/random.h"
#include "../include/string_util.h"
#include "../include/text.h"
#include "hack_contest.h"

/*
 * コンテスト移植 S4（FEAT-contest・patches/0985）: 結果の記録（Keep）と、台本から呼ぶ special の口。
 * 原文 参考/pokeemerald/src/contest.c 2666〜3633（CalculateFinalScores・DetermineFinalStandings）と
 * contest_util.c 1958〜2160（GiveMonContestRibbon・GetContestWinnerId ほかの special）。日本語版の ROM も同じ並び
 * （参考/pokeemerald-jp/asm/contest.s の DetermineFinalStandings 0x080DB4B0）。
 * 通信（LINK_CONTEST_FLAG）・TV・ファン・GAME_STAT は FR に相手が無い／S1 で範囲外と決定済み＝書かない。
 */

#define GET_WINNER(i) { for ((i) = 0; (i) < HACK_CONTEST_CONTESTANTS && HACK_CONTEST_KEEP.finalStandings[(i)] != 0; (i)++); }

struct HackFinalStanding
{
	s16 totalPoints;
	s16 round1Points;
	u16 random;
	u8 contestant;
};

//原文 DidContestantPlaceHigher（a が b より下位なら TRUE・合計→一次→乱数の順）
static bool8 DidContestantPlaceHigher(s32 a, s32 b, struct HackFinalStanding* st)
{
	if (st[a].totalPoints < st[b].totalPoints)
		return TRUE;
	if (st[a].totalPoints > st[b].totalPoints)
		return FALSE;
	if (st[a].round1Points < st[b].round1Points)
		return TRUE;
	if (st[a].round1Points > st[b].round1Points)
		return FALSE;
	return st[a].random < st[b].random;
}

//原文 DetermineFinalStandings。standingsOut[出場者] = 順位（0＝優勝）
void HackContest_DetermineStandings(const s16* round1, const s16* total, u8* standingsOut)
{
	u16 randomOrdering[HACK_CONTEST_CONTESTANTS] = {0};
	struct HackFinalStanding st[HACK_CONTEST_CONTESTANTS];
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++) //同点のときの乱数（全員違う値になるまで引き直す）
	{
		s32 j;

		randomOrdering[i] = Random();
		for (j = 0; j < i; j++)
		{
			if (randomOrdering[i] == randomOrdering[j])
			{
				i--;
				break;
			}
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		st[i].totalPoints = total[i];
		st[i].round1Points = round1[i];
		st[i].random = randomOrdering[i];
		st[i].contestant = i;
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS - 1; i++)
	{
		s32 j;

		for (j = HACK_CONTEST_CONTESTANTS - 1; j > i; j--)
		{
			if (DidContestantPlaceHigher(j - 1, j, st))
			{
				struct HackFinalStanding tmp = st[j - 1];

				st[j - 1] = st[j];
				st[j] = tmp;
			}
		}
	}
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
		standingsOut[st[i].contestant] = i;
}

//原文 contest.c:2666 gContestMonAppealPointTotals＝pointTotal → CalculateFinalScores（二次＝アピールの合計×2・合計＝一次＋二次）→ Keep
void HackContest_FinalizeScores(struct HackContestState* s)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;
	s32 i;

	k->magic = HACK_CONTEST_KEEP_MAGIC;
	k->playerIndex = s->playerIndex;
	k->category = s->category;
	k->isLink = 0;
	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		k->round1Points[i] = s->round1Points[i];
		k->round2Points[i] = s->status[i].pointTotal * 2;
		k->totalPoints[i] = k->round1Points[i] + k->round2Points[i];
		k->mons[i].species = s->mons[i].species;
		k->mons[i].otId = s->mons[i].otId;
		k->mons[i].personality = s->mons[i].personality;
		memcpy(k->mons[i].nickname, s->mons[i].nickname, sizeof(k->mons[i].nickname));
		memcpy(k->mons[i].trainerName, s->mons[i].trainerName, sizeof(k->mons[i].trainerName));
	}
	HackContest_DetermineStandings(k->round1Points, k->totalPoints, k->finalStandings);
}

// ---- special の口（原文 contest_util.c の同名の関数・引数と戻りは Var8004〜8006・VAR_RESULT で原文と同じ） ----

void GetContestWinnerId(void) //Var8005 = 優勝の出場者の番号
{
	u8 i;

	GET_WINNER(i);
	Var8005 = i;
}

void GetContestPlayerId(void) //Var8004 = プレイヤーの番号
{
	Var8004 = HACK_CONTEST_KEEP.playerIndex;
}

void BufferContestWinnerTrainerName(void) //StringVar3
{
	u8 i;

	GET_WINNER(i);
	StringCopy(gStringVar3, HACK_CONTEST_KEEP.mons[i].trainerName);
}

void BufferContestWinnerMonName(void) //StringVar1
{
	u8 i;

	GET_WINNER(i);
	StringCopy(gStringVar1, HACK_CONTEST_KEEP.mons[i].nickname);
}

void BufferContestantTrainerName(void) //Var8006 の出場者 → StringVar1
{
	StringCopy(gStringVar1, HACK_CONTEST_KEEP.mons[Var8006 & 3].trainerName);
}

void BufferContestantMonNickname(void) //Var8006 の出場者 → StringVar3
{
	StringCopy(gStringVar3, HACK_CONTEST_KEEP.mons[Var8006 & 3].nickname);
}

void BufferContestantMonSpecies(void) //Var8006 の出場者 → Var8004
{
	Var8004 = HACK_CONTEST_KEEP.mons[Var8006 & 3].species;
}

void GetContestMonCondition(void) //Var8006 の出場者の一次審査の点 → Var8004
{
	Var8004 = HACK_CONTEST_KEEP.round1Points[Var8006 & 3];
}

//原文 HasMonWonThisContestBefore: 部門のリボンのランク > 今回のランク なら TRUE（読み口は 1 か所＝仮）。partyIndex は Keep.partyIndex（原文 gContestMonPartyIndex。台本の Var8004 は会場の途中で上書きされる＝S5c の実機で見つけた）
u16 HasMonWonThisContestBefore(void)
{
	return HackContest_GetCategoryRibbonRank(HACK_CONTEST_KEEP.partyIndex, HACK_CONTEST_KEEP.category) > HACK_CONTEST_KEEP.rank;
}

//原文 GiveMonContestRibbon: 優勝していなければ何もしない。渡す口は HackContest_AwardRibbon の 1 か所
void GiveMonContestRibbon(void)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;

	if (k->finalStandings[k->playerIndex] != 0)
		return;
	HackContest_AwardRibbon(k->partyIndex, k->category, k->rank);
}

// ---- リボンの口（まとめ 0.66.0: FEAT-relive-ribbon の本物につないだ。部門のリボン＝手持ちの MON_DATA_COOL_RIBBON＋部門（0 かっこよさ・1 うつくしさ・2 かわいさ・3 かしこさ・4 たくましさ）に 0〜4 のランク＋1 を持つ。
//      箱へ預けても消えない表は src/hack_ribbon.c が持つ＝預ける・引き出すときの Store／Restore が受け持つ） ----

//原文 GiveMonContestRibbon の中身（contest_util.c:2010〜）: いまのリボンの値 <= 今回のランク かつ <= マスターなら 1 つ上げる。debug 版だけ Var800F に 部門×10＋ランク＋1 を残す（実機で優勝の口が呼ばれたことを見る）
void HackContest_AwardRibbon(u8 partyIndex, u8 category, u8 rank)
{
	u8 ribbonData;

	if (category > 4)
		return;
	ribbonData = GetMonData(&gPlayerParty[partyIndex], MON_DATA_COOL_RIBBON + category, NULL);
	if (ribbonData <= rank && ribbonData <= 3)
	{
		ribbonData++;
		SetMonData(&gPlayerParty[partyIndex], MON_DATA_COOL_RIBBON + category, &ribbonData);
	}
#ifdef HACK_DEBUG_MENU
	Var800F = category * 10 + rank + 1;
#endif
}

//部門のリボンの値（0＝無し・1〜4＝ノーマル〜マスター）。手持ちの MON_DATA_COOL_RIBBON＋部門を読む（まとめ 0.66.0）
u8 HackContest_GetCategoryRibbonRank(u8 partyIndex, u8 category)
{
#ifdef HACK_DEBUG_MENU
	//debug 版だけ（実機の確認用）: ニックネームの 10 文字目（終わりの印の後ろ＝画面に出ない）が 0x7F の手持ちは「どの部門・ランクでも勝ったことがある」（4＝全ランクより上）。賞品のゴージャスボール・「それでもいいですか」の確認に使う。通常版には入らない
	if (((u8*) &gPlayerParty[partyIndex])[17] == 0x7F)
		return 4;
#else
#endif
	if (category > 4)
		return 0;
	return GetMonData(&gPlayerParty[partyIndex], MON_DATA_COOL_RIBBON + category, NULL);
}

//勝者の記録（区画 93〜94）は S6＝hack_contest_winner.c の HackContest_SaveWinner
