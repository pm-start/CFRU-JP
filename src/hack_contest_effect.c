#include "defines.h"
#include "../include/random.h"
#include "hack_contest.h"

/*
 * コンテスト移植 S3（FEAT-contest・patches/0985）: 技のコンテストの効果 48 本。
 * 原文 pokeemerald/src/contest_effect.c（58〜1093 行）を 1 本ずつ写した。日本語版エメラルドの contest_effect.s と関数の大きさが全部同じ（50 本）。
 * 原文は eContestantStatus／eContestAppealResults／eContest などの共有変数を直に読み書きする。ここでは状態構造体 s と「今技を出した人」me を引数で渡す
 *   （状態は固定 RAM の語のさき・static 変数は使えない＝6.13）。
 *   eContestAppealResults.turnOrder／gContestantTurnOrder は s->turnOrder・eContestAppealResults.unnervedPokes[i] は s->status[i].unnervedPokes・
 *   eContestExcitement は s の excitement*・eContest.appealNumber／applauseLevel は s->appealNumber／applauseLevel。
 * 技の表（gContestMoves）は仮の読み口 HackContest_GetMove（hack_contest_move.c・本物は task/FEAT-contest-move-data で差し替え）。
 */

#define NONE 0xFF
#define COUNT HACK_CONTEST_CONTESTANTS
#define ST(i) (s->status[i])

static void SetString(struct HackContestState* s, u8 c, u8 id)
{
	ST(c).effectStringId = id;
}

static void SetString2(struct HackContestState* s, u8 c, u8 id)
{
	ST(c).effectStringId2 = id;
}

//原文 SetStartledString（contest.c:4553）
static void SetStartledString(struct HackContestState* s, u8 c, u8 jam)
{
	if (jam >= 60)
		SetString(s, c, HCS_TRIPPED_OVER);
	else if (jam >= 40)
		SetString(s, c, HCS_LEAPT_UP);
	else if (jam >= 30)
		SetString(s, c, HCS_UTTER_CRY);
	else if (jam >= 20)
		SetString(s, c, HCS_TURNED_BACK);
	else if (jam >= 10)
		SetString(s, c, HCS_LOOKED_DOWN);
}

//原文 MakeContestantNervous（contest.c:4586）
static void MakeContestantNervous(struct HackContestState* s, u8 c)
{
	ST(c).nervous = TRUE;
	ST(c).currMove = 0;
}

//原文 Contest_IsMonsTurnDisabled（contest.c:3524）
bool8 HackContest_IsMonsTurnDisabled(struct HackContestState* s, u8 contestant)
{
	return ST(contestant).numTurnsSkipped != 0 || ST(contestant).noMoreTurns;
}

//原文 IsContestantAllowedToCombo（contest.c:5057）
static bool8 IsContestantAllowedToCombo(struct HackContestState* s, u8 c)
{
	return !(ST(c).repeatedMove || ST(c).nervous);
}

static u8 MoveCategory(u16 move)
{
	struct HackContestMoveInfo info;

	HackContest_GetMove(move, &info);
	return info.category;
}

static u8 MoveEffectAppeal(u16 move)
{
	struct HackContestMoveInfo info;

	HackContest_GetMove(move, &info);
	return HackContest_GetEffectAppeal(info.effect);
}

static s16 RoundTowardsZero(s16 score)
{
	s16 absScore = (score < 0 ? -score : score) % 10;

	if (score < 0)
	{
		if (absScore != 0)
			score -= 10 - absScore;
	}
	else
		score -= absScore;
	return score;
}

static s16 RoundUp(s16 score)
{
	s16 absScore = (score < 0 ? -score : score) % 10;

	if (absScore != 0)
		score += 10 - absScore;
	return score;
}

static bool8 CanUnnerveContestant(struct HackContestState* s, u8 i)
{
	ST(i).unnervedPokes = 1;
	if (ST(i).immune)
	{
		SetString(s, i, HCS_AVOID_SEEING);
		return FALSE;
	}
	else if (ST(i).jamSafetyCount != 0)
	{
		ST(i).jamSafetyCount--;
		SetString(s, i, HCS_AVERT_GAZE);
		return FALSE;
	}
	else if (!ST(i).noMoreTurns && ST(i).numTurnsSkipped == 0)
		return TRUE;
	else
		return FALSE;
}

static void JamContestant(struct HackContestState* s, u8 i, u8 jam)
{
	ST(i).appeal -= jam;
	ST(i).jam += jam;
}

static bool8 WasAtLeastOneOpponentJammed(struct HackContestState* s)
{
	s16 jamBuffer[COUNT] = { 0 };
	s32 i;

	for (i = 0; s->jamQueue[i] != NONE; i++)
	{
		u8 contestant = s->jamQueue[i];

		if (CanUnnerveContestant(s, contestant))
		{
			s->jam2 = s->jam;
			if (ST(contestant).moreEasilyStartled)
				s->jam2 *= 2;
			if (ST(contestant).resistant)
			{
				s->jam2 = 10;
				SetString(s, contestant, HCS_LITTLE_DISTRACTED);
			}
			else
			{
				s->jam2 -= ST(contestant).jamReduction;
				if (s->jam2 <= 0)
				{
					s->jam2 = 0;
					SetString(s, contestant, HCS_NOT_FAZED);
				}
				else
				{
					JamContestant(s, contestant, s->jam2);
					SetStartledString(s, contestant, s->jam2);
					jamBuffer[contestant] = s->jam2;
				}
			}
		}
	}
	for (i = 0; i < COUNT; i++)
	{
		if (jamBuffer[i] != 0)
			return TRUE;
	}
	return FALSE;
}

static void JamByMoveCategory(struct HackContestState* s, u8 me, u8 category)
{
	s32 i;
	s32 numJammed = 0;

	for (i = 0; i < COUNT; i++)
	{
		if (s->turnOrder[me] > s->turnOrder[i])
		{
			if (category == MoveCategory(ST(i).currMove))
				s->jam = 40;
			else
				s->jam = 10;
			s->jamQueue[0] = i;
			s->jamQueue[1] = NONE;
			if (WasAtLeastOneOpponentJammed(s))
				numJammed++;
		}
	}
	if (numJammed == 0)
		SetString2(s, me, HCS_MESSED_UP2);
}

//---- 効果 --------------------------------------------------------------------------------------------------

static void Eff_HighlyAppealing(struct HackContestState* s, u8 me)
{
}

static void Eff_UserMoreEasilyStartled(struct HackContestState* s, u8 me)
{
	ST(me).moreEasilyStartled = TRUE;
	SetString(s, me, HCS_MORE_CONSCIOUS);
}

static void Eff_GreatAppealButNoMoreMoves(struct HackContestState* s, u8 me)
{
	ST(me).exploded = TRUE;
	SetString(s, me, HCS_NO_APPEAL);
}

static void Eff_RepetitionNotBoring(struct HackContestState* s, u8 me)
{
	ST(me).usedRepeatableMove = TRUE;
	ST(me).repeatedMove = FALSE;
	ST(me).moveRepeatCount = 0;
}

static void Eff_AvoidStartleOnce(struct HackContestState* s, u8 me)
{
	ST(me).jamSafetyCount = 1;
	SetString(s, me, HCS_SETTLE_DOWN);
}

static void Eff_AvoidStartle(struct HackContestState* s, u8 me)
{
	ST(me).immune = TRUE;
	SetString(s, me, HCS_OBLIVIOUS_TO_OTHERS);
}

static void Eff_AvoidStartleSlightly(struct HackContestState* s, u8 me)
{
	ST(me).jamReduction = 20;
	SetString(s, me, HCS_LESS_AWARE);
}

static void Eff_UserLessEasilyStartled(struct HackContestState* s, u8 me)
{
	ST(me).resistant = TRUE;
	SetString(s, me, HCS_STOPPED_CARING);
}

static void Eff_StartleFrontMon(struct HackContestState* s, u8 me)
{
	u8 idx = 0;
	u8 a = me;

	if (s->turnOrder[a] != 0)
	{
		s32 i;

		for (i = 0; i < COUNT; i++)
		{
			if (s->turnOrder[a] - 1 == s->turnOrder[i])
				break;
		}
		s->jamQueue[0] = i;
		s->jamQueue[1] = NONE;
		idx = WasAtLeastOneOpponentJammed(s);
	}
	if (idx == 0)
		SetString2(s, me, HCS_MESSED_UP2);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartlePrevMons(struct HackContestState* s, u8 me)
{
	u8 idx = 0;
	u8 contestant = me;

	if (s->turnOrder[contestant] != 0)
	{
		s32 i, j;

		for (i = 0, j = 0; i < COUNT; i++)
		{
			if (s->turnOrder[contestant] > s->turnOrder[i])
				s->jamQueue[j++] = i;
		}
		s->jamQueue[j] = NONE;
		idx = WasAtLeastOneOpponentJammed(s);
	}
	if (idx == 0)
		SetString2(s, me, HCS_MESSED_UP2);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartlePrevMon2(struct HackContestState* s, u8 me)
{
	u8 rval = Random() % 10;
	s32 jam;

	if (rval < 2)
		jam = 20;
	else if (rval < 8)
		jam = 40;
	else
		jam = 60;
	s->jam = jam;
	Eff_StartleFrontMon(s, me);
}

static void Eff_StartlePrevMons2(struct HackContestState* s, u8 me)
{
	u8 numStartled = 0;
	u8 contestant = me;
	u8 turnOrder = s->turnOrder[contestant];

	if (turnOrder != 0)
	{
		s32 i;

		for (i = 0; i < 4; i++)
		{
			if (s->turnOrder[contestant] > s->turnOrder[i])
			{
				u8 rval, jam;

				s->jamQueue[0] = i;
				s->jamQueue[1] = NONE;
				rval = Random() % 10;
				if (rval == 0)
					jam = 0;
				else if (rval <= 2)
					jam = 10;
				else if (rval <= 4)
					jam = 20;
				else if (rval <= 6)
					jam = 30;
				else if (rval <= 8)
					jam = 40;
				else
					jam = 60;
				s->jam = jam;
				if (WasAtLeastOneOpponentJammed(s))
					numStartled++;
			}
		}
	}
	SetString(s, me, HCS_ATTEMPT_STARTLE);
	if (numStartled == 0)
		SetString2(s, me, HCS_MESSED_UP2);
}

static void Eff_ShiftJudgeAttention(struct HackContestState* s, u8 me)
{
	bool32 hitAny = FALSE;
	u8 contestant = me;

	if (s->turnOrder[me] != 0)
	{
		s32 i;

		for (i = 0; i < 4; i++)
		{
			if (s->turnOrder[contestant] > s->turnOrder[i] && ST(i).hasJudgesAttention && CanUnnerveContestant(s, i))
			{
				ST(i).hasJudgesAttention = FALSE;
				ST(i).judgesAttentionWasRemoved = TRUE;
				SetString(s, i, HCS_JUDGE_LOOK_AWAY2);
				hitAny = TRUE;
			}
		}
	}
	SetString(s, me, HCS_DAZZLE_ATTEMPT);
	if (!hitAny)
		SetString2(s, me, HCS_MESSED_UP2);
}

static void Eff_StartleMonWithJudgesAttention(struct HackContestState* s, u8 me)
{
	u8 numStartled = 0;
	u8 contestant = me;

	if (s->turnOrder[me] != 0)
	{
		s32 i;

		for (i = 0; i < 4; i++)
		{
			if (s->turnOrder[contestant] > s->turnOrder[i])
			{
				if (ST(i).hasJudgesAttention)
					s->jam = 50;
				else
					s->jam = 10;
				s->jamQueue[0] = i;
				s->jamQueue[1] = NONE;
				if (WasAtLeastOneOpponentJammed(s))
					numStartled++;
			}
		}
	}
	SetString(s, me, HCS_ATTEMPT_STARTLE);
	if (numStartled == 0)
		SetString2(s, me, HCS_MESSED_UP2);
}

static void Eff_JamsOthersButMissOneTurn(struct HackContestState* s, u8 me)
{
	ST(me).turnSkipped = TRUE;
	Eff_StartlePrevMons(s, me);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartleMonsSameTypeAppeal(struct HackContestState* s, u8 me)
{
	u16 move = ST(me).currMove;

	JamByMoveCategory(s, me, MoveCategory(move));
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartleMonsCoolAppeal(struct HackContestState* s, u8 me)
{
	JamByMoveCategory(s, me, 0);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartleMonsBeautyAppeal(struct HackContestState* s, u8 me)
{
	JamByMoveCategory(s, me, 1);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartleMonsCuteAppeal(struct HackContestState* s, u8 me)
{
	JamByMoveCategory(s, me, 2);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartleMonsSmartAppeal(struct HackContestState* s, u8 me)
{
	JamByMoveCategory(s, me, 3);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_StartleMonsToughAppeal(struct HackContestState* s, u8 me)
{
	JamByMoveCategory(s, me, 4);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_MakeFollowingMonNervous(struct HackContestState* s, u8 me)
{
	bool32 hitAny = FALSE;

	if (s->turnOrder[me] != 3)
	{
		s32 i;

		for (i = 0; i < 4; i++)
		{
			if (s->turnOrder[me] + 1 == s->turnOrder[i])
			{
				if (CanUnnerveContestant(s, i))
				{
					MakeContestantNervous(s, i);
					SetString(s, i, HCS_NERVOUS);
					hitAny = TRUE;
				}
				else
				{
					SetString(s, i, HCS_UNAFFECTED);
					hitAny = TRUE;
				}
			}
		}
	}
	SetString(s, me, HCS_UNNERVE_ATTEMPT);
	if (!hitAny)
		SetString2(s, me, HCS_MESSED_UP2);
}

static void Eff_MakeFollowingMonsNervous(struct HackContestState* s, u8 me)
{
	u8 numUnnerved = 0;
	bool32 contestantUnnerved = FALSE;
	u8 contestantIds[5];
	s32 i;
	s32 numAfter;
	s16 oddsMod[COUNT];
	s16 odds[COUNT];

	for (i = 0; i < 5; i++)
		contestantIds[i] = NONE;
	for (i = 0, numAfter = 0; i < COUNT; i++)
	{
		if (s->turnOrder[me] < s->turnOrder[i] && !ST(i).nervous && !HackContest_IsMonsTurnDisabled(s, i))
			contestantIds[numAfter++] = i;
	}
	if (numAfter == 1)
	{
		odds[0] = 60;
	}
	else if (numAfter == 2)
	{
		odds[0] = 30;
		odds[1] = 30;
	}
	else if (numAfter == 3)
	{
		odds[0] = 20;
		odds[1] = 20;
		odds[2] = 20;
	}
	else
	{
		for (i = 0; i < COUNT; i++)
			odds[i] = 0;
	}
	for (i = 0; i < COUNT; i++)
	{
		if (ST(i).hasJudgesAttention && IsContestantAllowedToCombo(s, i))
			oddsMod[i] = (HackContest_IsComboStarter(ST(i).prevMove, s->mons[i].moves) ? 1 : 0) * 10; //仮: 原文 gComboStarterLookupTable[comboStarterId]（値は 0／TRUE だけ）
		else
			oddsMod[i] = 0;
		oddsMod[i] -= (ST(i).condition / 10) * 10;
	}
	if (odds[0] != 0)
	{
		for (i = 0; contestantIds[i] != NONE; i++)
		{
			if (Random() % 100 < odds[i] + oddsMod[contestantIds[i]])
			{
				if (CanUnnerveContestant(s, contestantIds[i]))
				{
					MakeContestantNervous(s, contestantIds[i]);
					SetString(s, contestantIds[i], HCS_NERVOUS);
					numUnnerved++;
				}
				else
					contestantUnnerved = TRUE;
			}
			else
				contestantUnnerved = TRUE;
			if (contestantUnnerved)
			{
				contestantUnnerved = FALSE;
				SetString(s, contestantIds[i], HCS_UNAFFECTED);
				numUnnerved++;
			}
			ST(contestantIds[i]).unnervedPokes = 1;
		}
	}
	SetString(s, me, HCS_UNNERVE_WAITING);
	if (numUnnerved == 0)
		SetString2(s, me, HCS_MESSED_UP2);
}

static void Eff_WorsenConditionOfPrevMons(struct HackContestState* s, u8 me)
{
	u8 numHit = 0;
	s32 i;

	for (i = 0; i < COUNT; i++)
	{
		if (s->turnOrder[me] > s->turnOrder[i] && ST(i).condition > 0 && CanUnnerveContestant(s, i))
		{
			ST(i).condition = 0;
			ST(i).conditionMod = 2; /* CONDITION_LOSE */
			SetString(s, i, HCS_REGAINED_FORM);
			numHit++;
		}
	}
	SetString(s, me, HCS_TAUNT_WELL);
	if (numHit == 0)
		SetString2(s, me, HCS_IGNORED);
}

static void Eff_BadlyStartlesMonsInGoodCondition(struct HackContestState* s, u8 me)
{
	u8 numHit = 0;
	s32 i;

	for (i = 0; i < COUNT; i++)
	{
		if (s->turnOrder[me] > s->turnOrder[i])
		{
			if (ST(i).condition > 0)
				s->jam = 40;
			else
				s->jam = 10;
			s->jamQueue[0] = i;
			s->jamQueue[1] = NONE;
			if (WasAtLeastOneOpponentJammed(s))
				numHit++;
		}
	}
	SetString(s, me, HCS_JAM_WELL);
	if (numHit == 0)
		SetString2(s, me, HCS_IGNORED);
}

static void Eff_BetterIfFirst(struct HackContestState* s, u8 me)
{
	if (s->turnOrder[me] == 0)
	{
		u16 move = ST(me).currMove;

		ST(me).appeal += 2 * MoveEffectAppeal(move);
		SetString(s, me, HCS_HUSTLE_STANDOUT);
	}
}

static void Eff_BetterIfLast(struct HackContestState* s, u8 me)
{
	if (s->turnOrder[me] == 3)
	{
		u16 move = ST(me).currMove;

		ST(me).appeal += 2 * MoveEffectAppeal(move);
		SetString(s, me, HCS_WORK_HARD_UNNOTICED);
	}
}

static void Eff_AppealAsGoodAsPrevOnes(struct HackContestState* s, u8 me)
{
	s32 i;
	s32 appealSum;

	for (i = 0, appealSum = 0; i < COUNT; i++)
	{
		if (s->turnOrder[me] > s->turnOrder[i])
			appealSum += ST(i).appeal;
	}
	if (appealSum < 0)
		appealSum = 0;
	if (s->turnOrder[me] == 0 || appealSum == 0)
		SetString(s, me, HCS_APPEAL_NOT_WELL);
	else
	{
		ST(me).appeal += appealSum / 2;
		SetString(s, me, HCS_WORK_BEFORE);
	}
	ST(me).appeal = RoundTowardsZero(ST(me).appeal);
}

static void Eff_AppealAsGoodAsPrevOne(struct HackContestState* s, u8 me)
{
	s16 appeal = 0;

	if (s->turnOrder[me] != 0)
	{
		s32 i;

		for (i = 0; i < COUNT; i++)
		{
			if (s->turnOrder[me] - 1 == s->turnOrder[i])
				appeal = ST(i).appeal;
		}
	}
	if (s->turnOrder[me] == 0 || appeal <= 0)
		SetString(s, me, HCS_APPEAL_NOT_WELL2);
	else
	{
		ST(me).appeal += appeal;
		SetString(s, me, HCS_WORK_PRECEDING);
	}
}

static void Eff_BetterWhenLater(struct HackContestState* s, u8 me)
{
	u8 whichTurn = s->turnOrder[me];

	if (whichTurn == 0)
		ST(me).appeal = 10;
	else
		ST(me).appeal = 20 * whichTurn;
	if (whichTurn == 0)
		SetString(s, me, HCS_APPEAL_NOT_SHOWN_WELL);
	else if (whichTurn == 1)
		SetString(s, me, HCS_APPEAL_SLIGHTLY_WELL);
	else if (whichTurn == 2)
		SetString(s, me, HCS_APPEAL_PRETTY_WELL);
	else
		SetString(s, me, HCS_APPEAL_EXCELLENTLY);
}

static void Eff_QualityDependsOnTiming(struct HackContestState* s, u8 me)
{
	u8 rval = Random() % 10;
	s16 appeal;

	if (rval < 3)
	{
		appeal = 10;
		SetString(s, me, HCS_APPEAL_NOT_VERY_WELL);
	}
	else if (rval < 6)
	{
		appeal = 20;
		SetString(s, me, HCS_APPEAL_SLIGHTLY_WELL2);
	}
	else if (rval < 8)
	{
		appeal = 40;
		SetString(s, me, HCS_APPEAL_PRETTY_WELL2);
	}
	else if (rval < 9)
	{
		appeal = 60;
		SetString(s, me, HCS_APPEAL_VERY_WELL);
	}
	else
	{
		appeal = 80;
		SetString(s, me, HCS_APPEAL_EXCELLENTLY2);
	}
	ST(me).appeal = appeal;
}

static void Eff_BetterIfSameType(struct HackContestState* s, u8 me)
{
	s8 turnOrder = s->turnOrder[me];
	s8 i = turnOrder - 1, j;
	u16 move;

	if (turnOrder == 0)
		return;
	while (1)
	{
		for (j = 0; j < COUNT; j++)
		{
			if (s->turnOrder[j] == i)
				break;
		}
		if (ST(j).noMoreTurns || ST(j).nervous || ST(j).numTurnsSkipped)
		{
			if (--i < 0)
				return;
		}
		else
			break;
	}
	move = ST(me).currMove;
	if (MoveCategory(move) == MoveCategory(ST(j).currMove))
	{
		ST(me).appeal += MoveEffectAppeal(move) * 2;
		SetString(s, me, HCS_SAME_TYPE_GOOD);
	}
}

static void Eff_BetterIfDiffType(struct HackContestState* s, u8 me)
{
	if (s->turnOrder[me] != 0)
	{
		u16 move = ST(me).currMove;
		s32 i;

		for (i = 0; i < COUNT; i++)
		{
			if (s->turnOrder[me] - 1 == s->turnOrder[i] && MoveCategory(move) != MoveCategory(ST(i).currMove))
			{
				ST(me).appeal += MoveEffectAppeal(move) * 2;
				SetString(s, me, HCS_DIFF_TYPE_GOOD);
				break;
			}
		}
	}
}

static void Eff_AffectedByPrevAppeal(struct HackContestState* s, u8 me)
{
	if (s->turnOrder[me] != 0)
	{
		s32 i;

		for (i = 0; i < COUNT; i++)
		{
			if (s->turnOrder[me] - 1 == s->turnOrder[i])
			{
				if (ST(me).appeal > ST(i).appeal)
				{
					ST(me).appeal *= 2;
					SetString(s, me, HCS_STOOD_OUT_AS_MUCH);
				}
				else if (ST(me).appeal < ST(i).appeal)
				{
					ST(me).appeal = 0;
					SetString(s, me, HCS_NOT_AS_WELL);
				}
			}
		}
	}
}

static void Eff_ImproveConditionPreventNervousness(struct HackContestState* s, u8 me)
{
	if (ST(me).condition < 30)
	{
		ST(me).condition += 10;
		ST(me).conditionMod = 1; /* CONDITION_GAIN */
		SetString(s, me, HCS_CONDITION_ROSE);
	}
	else
		SetString(s, me, HCS_NO_CONDITION_IMPROVE);
}

static void Eff_BetterWithGoodCondition(struct HackContestState* s, u8 me)
{
	ST(me).appealTripleCondition = TRUE;
	if (ST(me).condition != 0)
		SetString(s, me, HCS_HOT_STATUS);
	else
		SetString(s, me, HCS_BAD_CONDITION_WEAK_APPEAL);
}

static void Eff_NextAppealEarlier(struct HackContestState* s, u8 me)
{
	s8 i;
	s8 j;
	u8 turnOrder[COUNT];

	if (s->appealNumber != 4 /* CONTEST_LAST_APPEAL */)
	{
		for (i = 0; i < COUNT; i++)
			turnOrder[i] = ST(i).nextTurnOrder;
		turnOrder[me] = NONE;
		for (i = 0; i < COUNT; i++)
		{
			for (j = 0; j < COUNT; j++)
			{
				if (j != me && i == turnOrder[j] && turnOrder[j] == ST(j).nextTurnOrder)
				{
					turnOrder[j]++;
					break;
				}
			}
			if (j == COUNT)
				break;
		}
		turnOrder[me] = 0;
		ST(me).turnOrderMod = 1;
		for (i = 0; i < COUNT; i++)
			ST(i).nextTurnOrder = turnOrder[i];
		ST(me).turnOrderModAction = 1;
		SetString(s, me, HCS_MOVE_UP_LINE);
	}
}

static void Eff_NextAppealLater(struct HackContestState* s, u8 me)
{
	s8 i;
	s8 j;
	u8 turnOrder[COUNT];

	if (s->appealNumber != 4 /* CONTEST_LAST_APPEAL */)
	{
		for (i = 0; i < COUNT; i++)
			turnOrder[i] = ST(i).nextTurnOrder;
		turnOrder[me] = NONE;
		for (i = COUNT - 1; i > -1; i--)
		{
			for (j = 0; j < COUNT; j++)
			{
				if (j != me && i == turnOrder[j] && turnOrder[j] == ST(j).nextTurnOrder)
				{
					turnOrder[j]--;
					break;
				}
			}
			if (j == COUNT)
				break;
		}
		turnOrder[me] = COUNT - 1;
		ST(me).turnOrderMod = 1;
		for (i = 0; i < COUNT; i++)
			ST(i).nextTurnOrder = turnOrder[i];
		ST(me).turnOrderModAction = 2;
		SetString(s, me, HCS_MOVE_BACK_LINE);
	}
}

static void Eff_MakeScramblingTurnOrderEasier(struct HackContestState* s, u8 me)
{
	//原文のとおり何もしない（dummied out?）
}

static void Eff_ScrambleNextTurnOrder(struct HackContestState* s, u8 me)
{
	s8 i;
	s8 j;
	u8 turnOrder[COUNT];
	u8 unselectedContestants[COUNT];

	if (s->appealNumber != 4 /* CONTEST_LAST_APPEAL */)
	{
		for (i = 0; i < COUNT; i++)
		{
			turnOrder[i] = ST(i).nextTurnOrder;
			unselectedContestants[i] = i;
		}
		for (i = 0; i < COUNT; i++)
		{
			u8 rval = Random() % (COUNT - i);

			for (j = 0; j < COUNT; j++)
			{
				if (unselectedContestants[j] != NONE)
				{
					if (rval == 0)
					{
						turnOrder[j] = i;
						unselectedContestants[j] = NONE;
						break;
					}
					else
						rval--;
				}
			}
		}
		for (i = 0; i < COUNT; i++)
		{
			ST(i).nextTurnOrder = turnOrder[i];
			ST(i).turnOrderMod = 2;
		}
		ST(me).turnOrderModAction = 3;
		SetString(s, me, HCS_SCRAMBLE_ORDER);
	}
}

static void Eff_ExciteAudienceInAnyContest(struct HackContestState* s, u8 me)
{
	if (MoveCategory(ST(me).currMove) != s->category)
		ST(me).overrideCategoryExcitementMod = TRUE;
}

static void Eff_BadlyStartleMonsWithGoodAppeals(struct HackContestState* s, u8 me)
{
	s32 i;
	u8 numJammed = 0;

	for (i = 0; i < COUNT; i++)
	{
		if (s->turnOrder[me] > s->turnOrder[i])
		{
			if (ST(i).appeal > 0)
			{
				s->jam = ST(i).appeal / 2;
				s->jam = RoundUp(s->jam);
			}
			else
				s->jam = 10;
			s->jamQueue[0] = i;
			s->jamQueue[1] = NONE;
			if (WasAtLeastOneOpponentJammed(s))
				numJammed++;
		}
	}
	if (numJammed == 0)
		SetString2(s, me, HCS_MESSED_UP2);
	SetString(s, me, HCS_ATTEMPT_STARTLE);
}

static void Eff_BetterWhenAudienceExcited(struct HackContestState* s, u8 me)
{
	s16 appeal;

	if (s->applauseLevel == 0)
	{
		appeal = 10;
		SetString(s, me, HCS_APPEAL_NOT_VERY_WELL);
	}
	else if (s->applauseLevel == 1)
	{
		appeal = 20;
		SetString(s, me, HCS_APPEAL_SLIGHTLY_WELL2);
	}
	else if (s->applauseLevel == 2)
	{
		appeal = 30;
		SetString(s, me, HCS_APPEAL_PRETTY_WELL2);
	}
	else if (s->applauseLevel == 3)
	{
		appeal = 50;
		SetString(s, me, HCS_APPEAL_VERY_WELL);
	}
	else
	{
		appeal = 60;
		SetString(s, me, HCS_APPEAL_EXCELLENTLY2);
	}
	ST(me).appeal = appeal;
}

static void Eff_DontExciteAudience(struct HackContestState* s, u8 me)
{
	if (!s->excitementFrozen)
	{
		s->excitementFrozen = TRUE;
		s->excitementFreezer = me;
		SetString(s, me, HCS_ATTRACTED_ATTENTION);
	}
}

//原文 contest_moves.h:3198 gContestEffectFuncs の並び（効果の番号 0〜47）。8/10/12 は同じ関数・9/11/13 も同じ関数（効果表の jam の値だけ違う）
typedef void (*HackContestEffectFn)(struct HackContestState*, u8);
static const HackContestEffectFn sEffectFuncs[HACK_CONTEST_EFFECT_COUNT] =
{
	Eff_HighlyAppealing,
	Eff_UserMoreEasilyStartled,
	Eff_GreatAppealButNoMoreMoves,
	Eff_RepetitionNotBoring,
	Eff_AvoidStartleOnce,
	Eff_AvoidStartle,
	Eff_AvoidStartleSlightly,
	Eff_UserLessEasilyStartled,
	Eff_StartleFrontMon,
	Eff_StartlePrevMons,
	Eff_StartleFrontMon,
	Eff_StartlePrevMons,
	Eff_StartleFrontMon,
	Eff_StartlePrevMons,
	Eff_StartlePrevMon2,
	Eff_StartlePrevMons2,
	Eff_ShiftJudgeAttention,
	Eff_StartleMonWithJudgesAttention,
	Eff_JamsOthersButMissOneTurn,
	Eff_StartleMonsSameTypeAppeal,
	Eff_StartleMonsCoolAppeal,
	Eff_StartleMonsBeautyAppeal,
	Eff_StartleMonsCuteAppeal,
	Eff_StartleMonsSmartAppeal,
	Eff_StartleMonsToughAppeal,
	Eff_MakeFollowingMonNervous,
	Eff_MakeFollowingMonsNervous,
	Eff_WorsenConditionOfPrevMons,
	Eff_BadlyStartlesMonsInGoodCondition,
	Eff_BetterIfFirst,
	Eff_BetterIfLast,
	Eff_AppealAsGoodAsPrevOnes,
	Eff_AppealAsGoodAsPrevOne,
	Eff_BetterWhenLater,
	Eff_QualityDependsOnTiming,
	Eff_BetterIfSameType,
	Eff_BetterIfDiffType,
	Eff_AffectedByPrevAppeal,
	Eff_ImproveConditionPreventNervousness,
	Eff_BetterWithGoodCondition,
	Eff_NextAppealEarlier,
	Eff_NextAppealLater,
	Eff_MakeScramblingTurnOrderEasier,
	Eff_ScrambleNextTurnOrder,
	Eff_ExciteAudienceInAnyContest,
	Eff_BadlyStartleMonsWithGoodAppeals,
	Eff_BetterWhenAudienceExcited,
	Eff_DontExciteAudience,
};

void HackContest_RunEffect(struct HackContestState* s, u8 contestant, u8 effect)
{
	if (effect < HACK_CONTEST_EFFECT_COUNT)
		sEffectFuncs[effect](s, contestant);
}
