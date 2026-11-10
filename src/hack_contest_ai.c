#include "defines.h"
#include "../include/random.h"
#include "hack_contest.h"

/*
 * コンテスト移植 S3（FEAT-contest・patches/0985）: NPC の出場者が技を選ぶ AI。
 * 原文 pokeemerald/src/contest_ai.c（全 1785 行）を写した。日本語版エメラルドの contest_ai.s と関数の大きさが全部同じ（141 本）。
 * 台本（原文 data/contest_ai_scripts.s）は日本語版 ROM の 0x0828C958〜0x0828D2B3（2396 バイト）と同じ中身を tools/contest/extract_jp_ai.py が
 *   src/hack_contest_ai_data.c に書く（飛び先は台本の先頭からの相対＝static 変数も絶対番地も要らない）。台本の命令は 136 種（0x00〜0x87）。
 * 原文との差（Commander の決定・2026-11-09）:
 *   ・if_random_less_than／greater_than（0x7D／0x7E）は原文の UBFIX の側（台本の定数と比べる）。原文の既定は eContestAI.vars[台本の定数]（s16 が 3 つ）の範囲外読み。
 *   ・check_user_has_move（0x85）・if_used_combo_starter は原文の BUGFIX 無しのまま（JP ROM と同じ動き）。
 * 技の表（gContestMoves）は仮の読み口（hack_contest_move.c）。
 */

#define AI_ACTION_DONE 1
#define MAX_MOVES 4

extern const u8 gHackContestAIScripts[];
extern const u16 gHackContestAIScriptOffsets[32];

#define S struct HackContestState* s
#define AI (s->ai)
#define P (s->ai.scriptPtr)
#define R16(p) ((p)[0] | ((p)[1] << 8))
#define R32(p) ((u32) (p)[0] | ((u32) (p)[1] << 8) | ((u32) (p)[2] << 16) | ((u32) (p)[3] << 24))
#define PTR(p) (gHackContestAIScripts + R32(p))
#define OWN_MOVE(i) (s->mons[AI.contestantId].moves[i])
#define CUR_MOVE OWN_MOVE(AI.nextMoveIndex)

static u8 GetContestantIdByTurn(S, u8 turn)
{
	s32 i;

	for (i = 0; i < HACK_CONTEST_CONTESTANTS; i++)
	{
		if (s->turnOrder[i] == turn)
			break;
	}
	return i;
}

static void AIStackPushVar(S, const u8* ptr)
{
	AI.stack[AI.stackSize++] = ptr;
}

static bool8 AIStackPop(S)
{
	if (AI.stackSize != 0)
	{
		--AI.stackSize;
		P = AI.stack[AI.stackSize];
		return TRUE;
	}
	return FALSE;
}

//---- 共通の分岐（原文は命令ごとに同じ形を 1 本ずつ書いている）-------------------------------------------------------
#define BR_NUM_LT(name, getf) static void name(S) { getf(s); if (AI.scriptResult < P[0]) P = PTR(P + 1); else P += 5; }
#define BR_NUM_GT(name, getf) static void name(S) { getf(s); if (AI.scriptResult > P[0]) P = PTR(P + 1); else P += 5; }
#define BR_NUM_EQ(name, getf) static void name(S) { getf(s); if (AI.scriptResult == P[0]) P = PTR(P + 1); else P += 5; }
#define BR_NUM_NE(name, getf) static void name(S) { getf(s); if (AI.scriptResult != P[0]) P = PTR(P + 1); else P += 5; }
#define BR_S8_LT(name, getf) static void name(S) { getf(s); if (AI.scriptResult < (s8) P[0]) P = PTR(P + 1); else P += 5; }
#define BR_S8_GT(name, getf) static void name(S) { getf(s); if (AI.scriptResult > (s8) P[0]) P = PTR(P + 1); else P += 5; }
#define BR_S8_EQ(name, getf) static void name(S) { getf(s); if (AI.scriptResult == (s8) P[0]) P = PTR(P + 1); else P += 5; }
#define BR_S8_NE(name, getf) static void name(S) { getf(s); if (AI.scriptResult != (s8) P[0]) P = PTR(P + 1); else P += 5; }
#define BR_16_LT(name, getf) static void name(S) { getf(s); if (AI.scriptResult < (s16) R16(P)) P = PTR(P + 2); else P += 6; }
#define BR_16_GT(name, getf) static void name(S) { getf(s); if (AI.scriptResult > (s16) R16(P)) P = PTR(P + 2); else P += 6; }
#define BR_16_EQ(name, getf) static void name(S) { getf(s); if (AI.scriptResult == (s16) R16(P)) P = PTR(P + 2); else P += 6; }
#define BR_16_NE(name, getf) static void name(S) { getf(s); if (AI.scriptResult != (s16) R16(P)) P = PTR(P + 2); else P += 6; }
#define BR_TRUE(name, getf) static void name(S) { getf(s); if (AI.scriptResult) P = PTR(P + 0); else P += 4; }
#define BR_FALSE(name, getf) static void name(S) { getf(s); if (!AI.scriptResult) P = PTR(P + 0); else P += 4; }
#define BR_DIFF_LT0(name, getf) static void name(S) { getf(s); if (AI.scriptResult < 0) P = PTR(P + 0); else P += 4; }
#define BR_DIFF_GT0(name, getf) static void name(S) { getf(s); if (AI.scriptResult > 0) P = PTR(P + 0); else P += 4; }
#define BR_DIFF_EQ0(name, getf) static void name(S) { getf(s); if (AI.scriptResult == 0) P = PTR(P + 0); else P += 4; }
#define BR_DIFF_NE0(name, getf) static void name(S) { getf(s); if (AI.scriptResult != 0) P = PTR(P + 0); else P += 4; }

#define BR_NUM_4(prefix, getf) \
	BR_NUM_LT(prefix##_less_than, getf) BR_NUM_GT(prefix##_more_than, getf) BR_NUM_EQ(prefix##_eq, getf) BR_NUM_NE(prefix##_not_eq, getf)

//---- 命令 ----------------------------------------------------------------------------------------------------

static void Cmd_score(S)
{
	s16 score = AI.moveScores[AI.nextMoveIndex] + (s8) P[1];

	if (score > 255)
		score = 255;
	else if (score < 0)
		score = 0;
	AI.moveScores[AI.nextMoveIndex] = score;
	P += 2;
}

static void Get_appeal_num(S) { AI.scriptResult = s->appealNumber; P += 1; }
static void Get_excitement(S) { AI.scriptResult = s->applauseLevel; P += 1; }
static void Get_user_order(S) { AI.scriptResult = s->turnOrder[AI.contestantId]; P += 1; }
static void Get_user_condition(S) { AI.scriptResult = s->status[AI.contestantId].condition / 10; P += 1; }
static void Get_points(S) { AI.scriptResult = s->status[AI.contestantId].pointTotal; P += 1; }
static void Get_preliminary_points(S) { AI.scriptResult = s->round1Points[AI.contestantId]; P += 1; }
static void Get_contest_type(S) { AI.scriptResult = s->category; P += 1; }

BR_NUM_4(Cmd_if_appeal_num, Get_appeal_num)
BR_NUM_4(Cmd_if_excitement, Get_excitement)
BR_NUM_4(Cmd_if_user_order, Get_user_order)
BR_NUM_4(Cmd_if_user_condition, Get_user_condition)
BR_16_LT(Cmd_if_points_less_than, Get_points)
BR_16_GT(Cmd_if_points_more_than, Get_points)
BR_16_EQ(Cmd_if_points_eq, Get_points)
BR_16_NE(Cmd_if_points_not_eq, Get_points)
BR_16_LT(Cmd_if_preliminary_points_less_than, Get_preliminary_points)
BR_16_GT(Cmd_if_preliminary_points_more_than, Get_preliminary_points)
BR_16_EQ(Cmd_if_preliminary_points_eq, Get_preliminary_points)
BR_16_NE(Cmd_if_preliminary_points_not_eq, Get_preliminary_points)
BR_NUM_EQ(Cmd_if_contest_type_eq, Get_contest_type)
BR_NUM_NE(Cmd_if_contest_type_not_eq, Get_contest_type)

static void Get_move_excitement(S) { AI.scriptResult = HackContest_GetMoveExcitement(s, CUR_MOVE); P += 1; }
BR_S8_LT(Cmd_if_move_excitement_less_than, Get_move_excitement)
BR_S8_GT(Cmd_if_move_excitement_more_than, Get_move_excitement)
BR_S8_EQ(Cmd_if_move_excitement_eq, Get_move_excitement)
BR_S8_NE(Cmd_if_move_excitement_not_eq, Get_move_excitement)

static u8 MoveEffect(u16 move)
{
	struct HackContestMoveInfo info;

	HackContest_GetMove(move, &info);
	return info.effect;
}

static void Get_move_effect(S) { AI.scriptResult = MoveEffect(CUR_MOVE); P += 1; }
BR_NUM_EQ(Cmd_if_move_effect_eq, Get_move_effect)
BR_NUM_NE(Cmd_if_move_effect_not_eq, Get_move_effect)

static void Get_move_effect_type(S) { AI.scriptResult = HackContest_GetEffectType(MoveEffect(CUR_MOVE)); P += 1; }
BR_NUM_EQ(Cmd_if_move_effect_type_eq, Get_move_effect_type)
BR_NUM_NE(Cmd_if_move_effect_type_not_eq, Get_move_effect_type)

static void Check_most_appealing_move(S)
{
	s32 i;
	u8 appeal = HackContest_GetEffectAppeal(MoveEffect(CUR_MOVE));

	for (i = 0; i < MAX_MOVES; i++)
	{
		u16 newMove = OWN_MOVE(i);

		if (newMove != 0 && appeal < HackContest_GetEffectAppeal(MoveEffect(newMove)))
			break;
	}
	AI.scriptResult = (i == MAX_MOVES) ? TRUE : FALSE;
	P += 1;
}
BR_TRUE(Cmd_if_most_appealing_move, Check_most_appealing_move)

static void Check_most_jamming_move(S)
{
	s32 i;
	u8 jam = HackContest_GetEffectJam(MoveEffect(CUR_MOVE));

	for (i = 0; i < MAX_MOVES; i++)
	{
		u16 newMove = OWN_MOVE(i);

		if (newMove != 0 && jam < HackContest_GetEffectJam(MoveEffect(newMove)))
			break;
	}
	AI.scriptResult = (i == MAX_MOVES) ? TRUE : FALSE;
	P += 1;
}
//原文のとおり（飛び先を +1 から読み +5 進める。台本は使わない）
static void Cmd_if_most_jamming_move(S)
{
	Check_most_jamming_move(s);
	if (AI.scriptResult != FALSE)
		P = PTR(P + 1);
	else
		P += 5;
}

static void Get_num_move_hearts(S) { AI.scriptResult = HackContest_GetEffectAppeal(MoveEffect(CUR_MOVE)) / 10; P += 1; }
BR_NUM_4(Cmd_if_num_move_hearts, Get_num_move_hearts)
static void Get_num_move_jam_hearts(S) { AI.scriptResult = HackContest_GetEffectJam(MoveEffect(CUR_MOVE)) / 10; P += 1; }
BR_NUM_4(Cmd_if_num_move_jam_hearts, Get_num_move_jam_hearts)

static void Get_move_used_count(S)
{
	s16 result;
	u16 move = CUR_MOVE;

	if (move != s->status[AI.contestantId].prevMove)
		result = 0;
	else
		result = s->status[AI.contestantId].moveRepeatCount + 1;
	AI.scriptResult = result;
	P += 1;
}
BR_NUM_4(Cmd_if_most_used_count, Get_move_used_count)

static void Check_combo_starter(S)
{
	u8 result = 0;
	s32 i;
	u16 move = CUR_MOVE;

	for (i = 0; i < MAX_MOVES; i++)
	{
		if (OWN_MOVE(i))
		{
			result = HackContest_AreMovesCombo(move, OWN_MOVE(i), s->mons[AI.contestantId].moves);
			if (result)
			{
				result = 1;
				break;
			}
		}
	}
	if (result)
		result = 1;
	AI.scriptResult = result;
	P += 1;
}
BR_TRUE(Cmd_if_combo_starter, Check_combo_starter)
BR_FALSE(Cmd_if_not_combo_starter, Check_combo_starter)

static void Check_combo_finisher(S)
{
	u8 result = 0;
	s32 i;
	u16 move = CUR_MOVE;

	for (i = 0; i < MAX_MOVES; i++)
	{
		if (OWN_MOVE(i))
		{
			result = HackContest_AreMovesCombo(OWN_MOVE(i), move, s->mons[AI.contestantId].moves);
			if (result)
			{
				result = 1;
				break;
			}
		}
	}
	if (result)
		result = 1;
	AI.scriptResult = result;
	P += 1;
}
BR_TRUE(Cmd_if_combo_finisher, Check_combo_finisher)
BR_FALSE(Cmd_if_not_combo_finisher, Check_combo_finisher)

static void Check_would_finish_combo(S)
{
	u8 result = 0;
	u16 move = CUR_MOVE;

	if (s->status[AI.contestantId].prevMove)
		result = HackContest_AreMovesCombo(s->status[AI.contestantId].prevMove, move, s->mons[AI.contestantId].moves);
	if (result)
		result = 1;
	AI.scriptResult = result;
	P += 1;
}
BR_TRUE(Cmd_if_would_finish_combo, Check_would_finish_combo)
BR_FALSE(Cmd_if_would_not_finish_combo, Check_would_finish_combo)

static void Get_condition(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);

	AI.scriptResult = s->status[contestant].condition / 10;
	P += 2;
}
BR_NUM_4(Cmd_if_condition, Get_condition)

static void Get_used_combo_starter(S)
{
	u16 result = FALSE;
	u8 contestant = GetContestantIdByTurn(s, P[1]);

	if (!(s->status[contestant].repeatedMove || s->status[contestant].nervous)) //原文 IsContestantAllowedToCombo
		result = HackContest_IsComboStarter(s->status[contestant].prevMove, s->mons[contestant].moves) ? TRUE : FALSE;
	AI.scriptResult = result;
	P += 2;
}
BR_NUM_4(Cmd_if_used_combo_starter, Get_used_combo_starter)

static void Check_can_participate(S)
{
	if (HackContest_IsMonsTurnDisabled(s, GetContestantIdByTurn(s, P[1])))
		AI.scriptResult = FALSE;
	else
		AI.scriptResult = TRUE;
	P += 2;
}
BR_TRUE(Cmd_if_can_participate, Check_can_participate)
BR_FALSE(Cmd_if_cannot_participate, Check_can_participate)

static void Get_completed_combo(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);

	AI.scriptResult = s->status[contestant].completedComboFlag;
	P += 2;
}
BR_TRUE(Cmd_if_completed_combo, Get_completed_combo)
BR_FALSE(Cmd_if_not_completed_combo, Get_completed_combo)

static void Get_points_diff(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);

	AI.scriptResult = s->status[contestant].pointTotal - s->status[AI.contestantId].pointTotal;
	P += 2;
}
BR_DIFF_LT0(Cmd_if_points_more_than_mon, Get_points_diff)
BR_DIFF_GT0(Cmd_if_points_less_than_mon, Get_points_diff)
BR_DIFF_EQ0(Cmd_if_points_eq_mon, Get_points_diff)
BR_DIFF_NE0(Cmd_if_points_not_eq_mon, Get_points_diff)

static void Get_preliminary_points_diff(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);

	AI.scriptResult = s->round1Points[contestant] - s->round1Points[AI.contestantId];
	P += 2;
}
BR_DIFF_LT0(Cmd_if_preliminary_points_more_than_mon, Get_preliminary_points_diff)
BR_DIFF_GT0(Cmd_if_preliminary_points_less_than_mon, Get_preliminary_points_diff)
BR_DIFF_EQ0(Cmd_if_preliminary_points_eq_mon, Get_preliminary_points_diff)
BR_DIFF_NE0(Cmd_if_preliminary_points_not_eq_mon, Get_preliminary_points_diff)

static void Get_used_moves_effect(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);
	u8 round = P[2];
	u16 move = s->moveHistory[round][contestant];

	AI.scriptResult = MoveEffect(move);
	P += 3;
}
BR_NUM_4(Cmd_if_used_moves_effect, Get_used_moves_effect)

static void Get_used_moves_excitement(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);
	u8 round = P[2];
	s8 result = s->excitementHistory[round][contestant];

	AI.scriptResult = result;
	P += 3;
}
BR_NUM_4(Cmd_if_used_moves_excitement, Get_used_moves_excitement)

static void Get_used_moves_effect_type(S)
{
	u8 contestant = GetContestantIdByTurn(s, P[1]);
	u8 round = P[2];
	u16 move = s->moveHistory[round][contestant];

	AI.scriptResult = HackContest_GetEffectType(MoveEffect(move));
	P += 3;
}
BR_NUM_EQ(Cmd_if_used_moves_effect_type_eq, Get_used_moves_effect_type)
BR_NUM_NE(Cmd_if_used_moves_effect_type_not_eq, Get_used_moves_effect_type)

static void Cmd_save_result(S) { AI.vars[P[1]] = AI.scriptResult; P += 2; }
static void Cmd_setvar(S) { AI.vars[P[1]] = R16(P + 2); P += 4; }
static void Cmd_add(S) { AI.vars[P[1]] += ((s8) P[2] | P[3] << 8); P += 4; }
static void Cmd_addvar(S) { AI.vars[P[1]] += AI.vars[P[2]]; P += 3; }
static void Cmd_if_less_than(S) { if (AI.vars[P[1]] < R16(P + 2)) P = PTR(P + 4); else P += 8; }
static void Cmd_if_greater_than(S) { if (AI.vars[P[1]] > R16(P + 2)) P = PTR(P + 4); else P += 8; }
static void Cmd_if_eq(S) { if (AI.vars[P[1]] == R16(P + 2)) P = PTR(P + 4); else P += 8; }
static void Cmd_if_not_eq(S) { if (AI.vars[P[1]] != R16(P + 2)) P = PTR(P + 4); else P += 8; }
static void Cmd_if_less_than_var(S) { if (AI.vars[P[1]] < AI.vars[P[2]]) P = PTR(P + 3); else P += 7; }
static void Cmd_if_greater_than_var(S) { if (AI.vars[P[1]] > AI.vars[P[2]]) P = PTR(P + 3); else P += 7; }
static void Cmd_if_eq_var(S) { if (AI.vars[P[1]] == AI.vars[P[2]]) P = PTR(P + 3); else P += 7; }
static void Cmd_if_not_eq_var(S) { if (AI.vars[P[1]] != AI.vars[P[2]]) P = PTR(P + 3); else P += 7; }
//UBFIX（Commander の決定）: 台本の定数 P[1] と比べる（原文の既定は vars[P[1]] の範囲外読み）
static void Cmd_if_random_less_than(S) { if ((Random() & 0xFF) < P[1]) P = PTR(P + 2); else P += 6; }
static void Cmd_if_random_greater_than(S) { if ((Random() & 0xFF) > P[1]) P = PTR(P + 2); else P += 6; }
static void Cmd_goto(S) { P = PTR(P + 1); }
static void Cmd_call(S) { AIStackPushVar(s, P + 5); P = PTR(P + 1); }
static void Cmd_end(S)
{
	if (!AIStackPop(s))
		AI.aiAction |= AI_ACTION_DONE;
}

static void Check_user_has_exciting_move(S)
{
	s32 result = 0;
	s32 i;

	for (i = 0; i < MAX_MOVES; i++)
	{
		if (OWN_MOVE(i))
		{
			if (HackContest_GetMoveExcitement(s, OWN_MOVE(i)) == 1)
			{
				result = 1;
				break;
			}
		}
	}
	AI.scriptResult = result;
	P += 1;
}
BR_TRUE(Cmd_if_user_has_exciting_move, Check_user_has_exciting_move)
BR_FALSE(Cmd_if_user_doesnt_have_exciting_move, Check_user_has_exciting_move)

//原文のバグのまま（BUGFIX 無し）: 「技の番号」と台本が渡す 16 ビットの値を比べる（台本は効果の番号を渡す）。JP ROM も同じ
static void Check_user_has_move(S)
{
	s32 hasMove = FALSE;
	s32 i;
	u16 targetMove = R16(P + 1);

	for (i = 0; i < MAX_MOVES; i++)
	{
		u16 move = OWN_MOVE(i);

		if (move == targetMove)
		{
			hasMove = TRUE;
			break;
		}
	}
	AI.scriptResult = hasMove;
	P += 3;
}
BR_TRUE(Cmd_if_user_has_move, Check_user_has_move)
BR_FALSE(Cmd_if_user_doesnt_have_move, Check_user_has_move)

typedef void (*CmdFn)(struct HackContestState*);

//原文 sContestAICmdTable（0x00〜0x87）
static const CmdFn sCmdTable[0x88] =
{
	Cmd_score,                                  // 0x00
	(CmdFn) Get_appeal_num,                     // 0x01
	Cmd_if_appeal_num_less_than,                // 0x02
	Cmd_if_appeal_num_more_than,                // 0x03
	Cmd_if_appeal_num_eq,                       // 0x04
	Cmd_if_appeal_num_not_eq,                   // 0x05
	Get_excitement,                             // 0x06
	Cmd_if_excitement_less_than,                // 0x07
	Cmd_if_excitement_more_than,                // 0x08
	Cmd_if_excitement_eq,                       // 0x09
	Cmd_if_excitement_not_eq,                   // 0x0A
	Get_user_order,                             // 0x0B
	Cmd_if_user_order_less_than,                // 0x0C
	Cmd_if_user_order_more_than,                // 0x0D
	Cmd_if_user_order_eq,                       // 0x0E
	Cmd_if_user_order_not_eq,                   // 0x0F
	Get_user_condition,                         // 0x10
	Cmd_if_user_condition_less_than,            // 0x11
	Cmd_if_user_condition_more_than,            // 0x12
	Cmd_if_user_condition_eq,                   // 0x13
	Cmd_if_user_condition_not_eq,               // 0x14
	Get_points,                                 // 0x15
	Cmd_if_points_less_than,                    // 0x16
	Cmd_if_points_more_than,                    // 0x17
	Cmd_if_points_eq,                           // 0x18
	Cmd_if_points_not_eq,                       // 0x19
	Get_preliminary_points,                     // 0x1A
	Cmd_if_preliminary_points_less_than,        // 0x1B
	Cmd_if_preliminary_points_more_than,        // 0x1C
	Cmd_if_preliminary_points_eq,               // 0x1D
	Cmd_if_preliminary_points_not_eq,           // 0x1E
	Get_contest_type,                           // 0x1F
	Cmd_if_contest_type_eq,                     // 0x20
	Cmd_if_contest_type_not_eq,                 // 0x21
	Get_move_excitement,                        // 0x22
	Cmd_if_move_excitement_less_than,           // 0x23
	Cmd_if_move_excitement_more_than,           // 0x24
	Cmd_if_move_excitement_eq,                  // 0x25
	Cmd_if_move_excitement_not_eq,              // 0x26
	Get_move_effect,                            // 0x27
	Cmd_if_move_effect_eq,                      // 0x28
	Cmd_if_move_effect_not_eq,                  // 0x29
	Get_move_effect_type,                       // 0x2A
	Cmd_if_move_effect_type_eq,                 // 0x2B
	Cmd_if_move_effect_type_not_eq,             // 0x2C
	Check_most_appealing_move,                  // 0x2D
	Cmd_if_most_appealing_move,                 // 0x2E
	Check_most_jamming_move,                    // 0x2F
	Cmd_if_most_jamming_move,                   // 0x30
	Get_num_move_hearts,                        // 0x31
	Cmd_if_num_move_hearts_less_than,           // 0x32
	Cmd_if_num_move_hearts_more_than,           // 0x33
	Cmd_if_num_move_hearts_eq,                  // 0x34
	Cmd_if_num_move_hearts_not_eq,              // 0x35
	Get_num_move_jam_hearts,                    // 0x36
	Cmd_if_num_move_jam_hearts_less_than,       // 0x37
	Cmd_if_num_move_jam_hearts_more_than,       // 0x38
	Cmd_if_num_move_jam_hearts_eq,              // 0x39
	Cmd_if_num_move_jam_hearts_not_eq,          // 0x3A
	Get_move_used_count,                        // 0x3B
	Cmd_if_most_used_count_less_than,           // 0x3C
	Cmd_if_most_used_count_more_than,           // 0x3D
	Cmd_if_most_used_count_eq,                  // 0x3E
	Cmd_if_most_used_count_not_eq,              // 0x3F
	Check_combo_starter,                        // 0x40
	Cmd_if_combo_starter,                       // 0x41
	Cmd_if_not_combo_starter,                   // 0x42
	Check_combo_finisher,                       // 0x43
	Cmd_if_combo_finisher,                      // 0x44
	Cmd_if_not_combo_finisher,                  // 0x45
	Check_would_finish_combo,                   // 0x46
	Cmd_if_would_finish_combo,                  // 0x47
	Cmd_if_would_not_finish_combo,              // 0x48
	Get_condition,                              // 0x49
	Cmd_if_condition_less_than,                 // 0x4A
	Cmd_if_condition_more_than,                 // 0x4B
	Cmd_if_condition_eq,                        // 0x4C
	Cmd_if_condition_not_eq,                    // 0x4D
	Get_used_combo_starter,                     // 0x4E
	Cmd_if_used_combo_starter_less_than,        // 0x4F
	Cmd_if_used_combo_starter_more_than,        // 0x50
	Cmd_if_used_combo_starter_eq,               // 0x51
	Cmd_if_used_combo_starter_not_eq,           // 0x52
	Check_can_participate,                      // 0x53
	Cmd_if_can_participate,                     // 0x54
	Cmd_if_cannot_participate,                  // 0x55
	Get_completed_combo,                        // 0x56
	Cmd_if_completed_combo,                     // 0x57
	Cmd_if_not_completed_combo,                 // 0x58
	Get_points_diff,                            // 0x59
	Cmd_if_points_more_than_mon,                // 0x5A
	Cmd_if_points_less_than_mon,                // 0x5B
	Cmd_if_points_eq_mon,                       // 0x5C
	Cmd_if_points_not_eq_mon,                   // 0x5D
	Get_preliminary_points_diff,                // 0x5E
	Cmd_if_preliminary_points_more_than_mon,    // 0x5F
	Cmd_if_preliminary_points_less_than_mon,    // 0x60
	Cmd_if_preliminary_points_eq_mon,           // 0x61
	Cmd_if_preliminary_points_not_eq_mon,       // 0x62
	Get_used_moves_effect,                      // 0x63
	Cmd_if_used_moves_effect_less_than,         // 0x64
	Cmd_if_used_moves_effect_more_than,         // 0x65
	Cmd_if_used_moves_effect_eq,                // 0x66
	Cmd_if_used_moves_effect_not_eq,            // 0x67
	Get_used_moves_excitement,                  // 0x68
	Cmd_if_used_moves_excitement_less_than,     // 0x69
	Cmd_if_used_moves_excitement_more_than,     // 0x6A
	Cmd_if_used_moves_excitement_eq,            // 0x6B
	Cmd_if_used_moves_excitement_not_eq,        // 0x6C
	Get_used_moves_effect_type,                 // 0x6D
	Cmd_if_used_moves_effect_type_eq,           // 0x6E
	Cmd_if_used_moves_effect_type_not_eq,       // 0x6F
	Cmd_save_result,                            // 0x70
	Cmd_setvar,                                 // 0x71
	Cmd_add,                                    // 0x72
	Cmd_addvar,                                 // 0x73
	Cmd_addvar,                                 // 0x74（原文 addvar_duplicate）
	Cmd_if_less_than,                           // 0x75
	Cmd_if_greater_than,                        // 0x76
	Cmd_if_eq,                                  // 0x77
	Cmd_if_not_eq,                              // 0x78
	Cmd_if_less_than_var,                       // 0x79
	Cmd_if_greater_than_var,                    // 0x7A
	Cmd_if_eq_var,                              // 0x7B
	Cmd_if_not_eq_var,                          // 0x7C
	Cmd_if_random_less_than,                    // 0x7D
	Cmd_if_random_greater_than,                 // 0x7E
	Cmd_goto,                                   // 0x7F
	Cmd_call,                                   // 0x80
	Cmd_end,                                    // 0x81
	Check_user_has_exciting_move,               // 0x82
	Cmd_if_user_has_exciting_move,              // 0x83
	Cmd_if_user_doesnt_have_exciting_move,      // 0x84
	Check_user_has_move,                        // 0x85
	Cmd_if_user_has_move,                       // 0x86
	Cmd_if_user_doesnt_have_move,               // 0x87
};

static void ZeroAI(struct HackContestAI* ai)
{
	u8* p = (u8*) ai;
	u32 i;

	for (i = 0; i < sizeof(struct HackContestAI); i++)
		p[i] = 0;
}

//原文 ContestAI_ResetAI（contest_ai.c:164）
void HackContest_AIReset(S, u8 contestant)
{
	s32 i;

	ZeroAI(&AI);
	for (i = 0; i < MAX_MOVES; i++)
		AI.moveScores[i] = 100;
	AI.contestantId = contestant;
	AI.stackSize = 0;
	AI.aiFlags = s->mons[contestant].aiFlags;
}

//原文 ContestAI_DoAIProcessing（contest_ai.c:343）
static void DoAIProcessing(S)
{
	while (AI.aiState != 2 /* FINISHED */)
	{
		switch (AI.aiState)
		{
		case 3: /* DO_NOT_PROCESS */
			break;
		case 0: /* SETTING_UP */
			P = gHackContestAIScripts + gHackContestAIScriptOffsets[AI.currentAIFlag];
			if (OWN_MOVE(AI.nextMoveIndex) == 0)
				AI.nextMove = 0;
			else
				AI.nextMove = OWN_MOVE(AI.nextMoveIndex);
			AI.aiState++;
			break;
		case 1: /* PROCESSING */
			if (AI.nextMove != 0)
				sCmdTable[*P](s);
			else
			{
				AI.moveScores[AI.nextMoveIndex] = 0;
				AI.aiAction |= AI_ACTION_DONE;
			}
			if (AI.aiAction & AI_ACTION_DONE)
			{
				AI.nextMoveIndex++;
				if (AI.nextMoveIndex < MAX_MOVES)
					AI.aiState = 0;
				else
					AI.aiState++;
				AI.aiAction &= ~AI_ACTION_DONE;
			}
			break;
		}
	}
}

//原文 ContestAI_GetActionToUse（contest_ai.c:180）
u8 HackContest_AIGetActionToUse(S)
{
	while (AI.aiFlags != 0)
	{
		if (AI.aiFlags & 1)
		{
			AI.aiState = 0;
			DoAIProcessing(s);
		}
		AI.aiFlags >>= 1;
		AI.currentAIFlag++;
		AI.nextMoveIndex = 0;
	}
	while (1)
	{
		u8 moveIndex = Random() % MAX_MOVES;
		u8 score = AI.moveScores[moveIndex];
		s32 i;

		for (i = 0; i < MAX_MOVES; i++)
		{
			if (score < AI.moveScores[i])
				break;
		}
		if (i == MAX_MOVES)
			return moveIndex;
	}
}
