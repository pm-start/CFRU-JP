#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "hack_contest.h"
#include "hack_contest_text.h"

/*
 * コンテスト移植 S2b（FEAT-contest・patches/0985）: 技のコンテスト用の設定の読み口。
 * まとめ 0.66.0: 技ごとの分類・効果・コンボと効果の表（appeal・jam・effectType）は、task/FEAT-contest-move-data（patches/0984）の
 *   src/Tables/contest_moves.c の gContestMoves／gContestEffects を読む（仮の表と、ここにあった効果 48 行の写しは捨てた＝1 本）。
 *   説明文の表（gHackContestEffectDescriptions）は日本語版エメラルドの ROM（0x08560BB4）と同じ中身（tools/rom/verify_contest_screen.py が突き合わせる）。
 */

/* src/Tables/contest_moves.c（自動生成）の定義と同じ並び */
struct ContestMove
{
	u8 effect;
	u8 contestCategory;
	u8 comboStarterId;
	u8 comboMoves[4];
};
struct ContestEffect
{
	u8 effectType;
	u8 appeal;
	u8 jam;
};
#define HACK_CONTEST_MOVE_TABLE_COUNT 5209
extern const struct ContestMove gContestMoves[HACK_CONTEST_MOVE_TABLE_COUNT];
extern const struct ContestEffect gContestEffects[HACK_CONTEST_EFFECT_COUNT];

const u8* const gHackContestEffectDescriptions[HACK_CONTEST_EFFECT_COUNT] =
{
	gHackContest_HighlyAppealingMove,
	gHackContest_UserMoreEasilyStartled,
	gHackContest_GreatAppealButNoMoreToEnd,
	gHackContest_UsedRepeatedlyWithoutBoringJudge,
	gHackContest_AvoidStartledByOthersOnce,
	gHackContest_AvoidStartledByOthers,
	gHackContest_AvoidStartledByOthersLittle,
	gHackContest_UserLessLikelyStartled,
	gHackContest_SlightlyStartleFrontMon,
	gHackContest_SlightlyStartleAppealed,
	gHackContest_StartleAppealedBeforeUser,
	gHackContest_StartleAllAppealed,
	gHackContest_BadlyStartleFrontMon,
	gHackContest_BadlyStartleAppealed,
	gHackContest_StartleAppealedBeforeUser2,
	gHackContest_StartleAllAppealed2,
	gHackContest_ShiftJudgesAttentionFromOthers,
	gHackContest_StartleMonHasJudgesAttention,
	gHackContest_JamOthersMissesTurn,
	gHackContest_StartleMonsMadeSameTypeAppeal,
	gHackContest_BadlyStartleCoolAppeals,
	gHackContest_BadlyStartleBeautyAppeals,
	gHackContest_BadlyStartleCuteAppeals,
	gHackContest_BadlyStartleSmartAppeals,
	gHackContest_BadlyStartleToughAppeals,
	gHackContest_MakeMonAfterUserNervous,
	gHackContest_MakeAllMonsAfterUserNervous,
	gHackContest_WorsenConditionOfThoseMadeAppeals,
	gHackContest_BadlyStartleMonsGoodCondition,
	gHackContest_AppealGreatIfPerformedFirst,
	gHackContest_AppealGreatIfPerformedLast,
	gHackContest_AppealAsGoodAsThoseBeforeIt,
	gHackContest_AppealAsGoodAsOneBeforeIt,
	gHackContest_AppealBetterLaterItsPerformed,
	gHackContest_AppealVariesDependingOnTiming,
	gHackContest_WorksWellIfSameTypeAsBefore,
	gHackContest_WorksWellIfDifferentTypeAsBefore,
	gHackContest_AffectedByAppealInFront,
	gHackContest_UpsConditionHelpsPreventNervousness,
	gHackContest_AppealWorksWellIfConditionGood,
	gHackContest_NextAppealMadeEarlier,
	gHackContest_NextAppealMadeLater,
	gHackContest_TurnOrderMoreEasilyScrambled,
	gHackContest_ScrambleOrderOfNextAppeals,
	gHackContest_AppealExcitesAudienceInAnyContest,
	gHackContest_BadlyStartlesMonsGoodAppeals,
	gHackContest_AppealBestMoreCrowdExcited,
	gHackContest_TemporarilyStopCrowdExcited,
};

u8 HackContest_GetEffectAppeal(u8 effect)
{
	return effect < HACK_CONTEST_EFFECT_COUNT ? gContestEffects[effect].appeal : 0;
}

u8 HackContest_GetEffectJam(u8 effect)
{
	return effect < HACK_CONTEST_EFFECT_COUNT ? gContestEffects[effect].jam : 0;
}

u8 HackContest_GetEffectType(u8 effect)
{
	return effect < HACK_CONTEST_EFFECT_COUNT ? gContestEffects[effect].effectType : 0;
}

//技の分類（0 かっこよさ〜4 たくましさ）・効果・アピール・ジャマー（原文 gContestMoves／gContestEffects）。表の外の技は 0 番の技（なし）と同じ
void HackContest_GetMove(u16 move, struct HackContestMoveInfo* out)
{
	const struct ContestMove* m = &gContestMoves[move < HACK_CONTEST_MOVE_TABLE_COUNT ? move : 0];

	out->category = m->contestCategory;
	out->effect = m->effect;
	out->appeal = HackContest_GetEffectAppeal(out->effect);
	out->jam = HackContest_GetEffectJam(out->effect);
}

//原文 AreMovesContestCombo（contest_effect.c:60）: 前の技のコンボ始点の番号が 0 でなく、次の技の comboMoves 4 つのどれかと同じなら真（gComboStarterLookupTable は 0 以外が全部 TRUE）。ownMoves は使わない（仮の時代の引数・呼び元を変えないため残した）
bool8 HackContest_AreMovesCombo(u16 prev, u16 move, const u16* ownMoves)
{
	u8 starter;
	const struct ContestMove* n;

	(void) ownMoves;
	if (prev >= HACK_CONTEST_MOVE_TABLE_COUNT || move >= HACK_CONTEST_MOVE_TABLE_COUNT)
		return FALSE;
	starter = gContestMoves[prev].comboStarterId;
	if (starter == 0)
		return FALSE;
	n = &gContestMoves[move];
	return starter == n->comboMoves[0] || starter == n->comboMoves[1] || starter == n->comboMoves[2] || starter == n->comboMoves[3];
}

//原文 gContestMoves[].comboStarterId != 0（gComboStarterLookupTable[id]）。ownMoves は使わない
bool8 HackContest_IsComboStarter(u16 move, const u16* ownMoves)
{
	(void) ownMoves;
	return move != 0 && move < HACK_CONTEST_MOVE_TABLE_COUNT && gContestMoves[move].comboStarterId != 0;
}
