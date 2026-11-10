#include "defines.h"
#include "defines_battle.h"
#include "lv200_custom_moves.h"
#include "shadow_moves.h" //[hack] patches/0955 段 D ダーク技
#include "lv200_custom_common.h"
#include "../include/battle_util.h"

/*
 * battle command callasm は関数から戻ったあと gBattlescriptCurrInstr を5進める。
 * そのため、ここでは選んだ台本の5バイト手前を設定する。
 */
void Lv200CustomMove_Dispatch(void)
{
	const u8 *script = Lv200CustomMovesA_GetScript(gCurrentMove);

	if (script == NULL)
		script = Lv200CustomMovesB_GetScript(gCurrentMove);
	if (script == NULL)
		script = Shadow_GetScript(gCurrentMove); //[hack] patches/0955 段 D ダーク技の特殊 4（0x1447〜）
	if (script == NULL)
		script = BattleScript_ButItFailed;

	gBattlescriptCurrInstr = script - 5;
}

struct Lv200CustomMovesAState *Lv200CustomMove_GetStateA(void)
{
	return gNewBS == NULL ? NULL : &gNewBS->lv200CustomMovesA;
}

struct Lv200CustomMovesBState *Lv200CustomMove_GetStateB(void)
{
	return gNewBS == NULL ? NULL : &gNewBS->lv200CustomMovesB;
}

void Lv200CustomMove_AdjustTypes(u8 bank, u8 *type1, u8 *type2, u8 *type3)
{
	Lv200CustomMovesA_AdjustTypes(bank, type1, type2, type3); /* 封印タイプ(かぜのたいじ・さびつかせ)を先に除く */
	Lv200CustomMovesB_AdjustTypes(bank, type1, type2, type3); /* そのあと場で加わるタイプ(ひょうがじだい) */
}

u32 Lv200CustomMove_SingleScreenDamage(u32 damage, u8 bankDef)
{
	return Lv200CustomMovesB_SingleScreenDamage(damage, bankDef);
}

u32 Lv200CustomMove_GrassyTerrainPower(u32 power, u32 boost)
{
	return Lv200CustomMovesB_GrassyTerrainPower(power, boost);
}

u16 Lv200CustomMove_ModifyPower(u16 move, u8 bankAtk, u8 bankDef, u16 value)
{
	value = Lv200CustomMovesA_ModifyPower(move, bankAtk, bankDef, value);
	return Lv200CustomMovesB_ModifyPower(move, bankAtk, bankDef, value);
}

u32 Lv200CustomMove_ModifyAccuracy(u16 move, u8 bankAtk, u8 bankDef, u32 value)
{
	value = Lv200CustomMovesA_ModifyAccuracy(move, bankAtk, bankDef, value);
	return Lv200CustomMovesB_ModifyAccuracy(move, bankAtk, bankDef, value);
}

u8 Lv200CustomMove_ModifyType(u16 move, u8 bankAtk, u8 value)
{
	value = Lv200CustomMovesA_ModifyType(move, bankAtk, value);
	return Lv200CustomMovesB_ModifyType(move, bankAtk, value);
}

u32 Lv200CustomMove_ModifySpeed(u8 bank, u32 value)
{
	value = Lv200CustomMovesA_ModifySpeed(bank, value);
	return Lv200CustomMovesB_ModifySpeed(bank, value);
}

u32 Lv200CustomMove_ModifyLeechSeedDamage(u8 bank, u32 value)
{
	value = Lv200CustomMovesA_ModifyLeechSeedDamage(bank, value);
	return Lv200CustomMovesB_ModifyLeechSeedDamage(bank, value);
}

void Lv200CustomMove_ResetState(void)
{
	Lv200CustomMovesA_ResetState();
	Lv200CustomMovesB_ResetState();
	Lv200Common_Reset();
}

void Lv200CustomMove_OnTurnEnd(void)
{
	Lv200CustomMovesA_OnTurnEnd();
	Lv200CustomMovesB_OnTurnEnd();
	Lv200Common_Clear(); //ターンの切れ目に積み残しを持ち越さない
}

bool8 Lv200CustomMove_TryEndTurnEffect(void)
{
	const u8 *script = Lv200CustomMovesA_GetEndTurnScript();

	if (script == NULL)
		script = Lv200CustomMovesB_GetEndTurnScript();
	if (script == NULL)
		return FALSE;

	BattleScriptExecute(script);
	return TRUE;
}

const u8 *Lv200CustomMove_GetSwitchInScript(u8 bank)
{
	const u8 *script = Lv200CustomMovesA_GetSwitchInScript(bank);
	if (script == NULL)
		script = Lv200CustomMovesB_GetSwitchInScript(bank);
	return script;
}

const u8 *Lv200CustomMove_GetMoveEndScript(void)
{
	const u8 *script = Lv200Common_GetMoveEndScript(); //P3 共通部品の効果（能力変化・HP・治療）を先に
	if (script == NULL)
		script = Lv200CustomMovesA_GetMoveEndScript();
	if (script == NULL)
		script = Lv200CustomMovesB_GetMoveEndScript();
	return script;
}

void Lv200CustomMove_OnSwitchOut(u8 bank)
{
	Lv200CustomMovesA_OnSwitchOut(bank);
	Lv200CustomMovesB_OnSwitchOut(bank);
	Lv200Common_OnSwitchOut(bank);
}

void Lv200CustomMove_OnSwitchIn(u8 bank)
{
	Lv200CustomMovesA_OnSwitchIn(bank);
	Lv200CustomMovesB_OnSwitchIn(bank);
}

void Lv200CustomMove_OnMoveHit(u16 move, u8 bankAtk, u8 bankDef, u32 damage,
							  bool8 hitSubstitute, bool8 substituteBroke)
{
	Lv200CustomMovesA_OnMoveHit(move, bankAtk, bankDef, damage, hitSubstitute, substituteBroke);
	Lv200CustomMovesB_OnMoveHit(move, bankAtk, bankDef, damage, hitSubstitute, substituteBroke);
}

#define LV200_CHAIN_VALUE(name, type, args, callArgs) \
	type Lv200CustomMove_##name args \
	{ \
		value = Lv200CustomMovesA_##name callArgs; \
		return Lv200CustomMovesB_##name callArgs; \
	}

LV200_CHAIN_VALUE(ModifyPriority, s8, (u16 move, u8 bankAtk, u8 bankDef, s8 value),
				  (move, bankAtk, bankDef, value))
LV200_CHAIN_VALUE(ModifyDamage, s32, (u16 move, u8 bankAtk, u8 bankDef, s32 value),
				  (move, bankAtk, bankDef, value))
LV200_CHAIN_VALUE(ShouldConsumePP, bool8, (u16 move, u8 bankAtk, bool8 value),
				  (move, bankAtk, value))
LV200_CHAIN_VALUE(CanSwitch, bool8, (u8 bank, bool8 value), (bank, value))
LV200_CHAIN_VALUE(ModifyAttackValue, u32, (u16 move, u8 bankAtk, u8 bankDef, u32 value),
				  (move, bankAtk, bankDef, value))
LV200_CHAIN_VALUE(ModifyTypeEffectiveness, u16, (u16 move, u8 bankAtk, u8 bankDef, u16 value),
				  (move, bankAtk, bankDef, value))
LV200_CHAIN_VALUE(IsBattlerOfType, bool8, (u8 bank, u8 type, bool8 value), (bank, type, value))
LV200_CHAIN_VALUE(IsAbilityActive, bool8, (u8 bank, bool8 value), (bank, value))
LV200_CHAIN_VALUE(IsItemActive, bool8, (u8 bank, bool8 value), (bank, value))
LV200_CHAIN_VALUE(CanLowerStat, bool8, (u8 bank, u8 source, bool8 value), (bank, source, value))
LV200_CHAIN_VALUE(CanFlinch, bool8, (u8 bank, u8 source, bool8 value), (bank, source, value))

void Lv200CustomMove_RecordFlinch(u8 bank)
{
	Lv200CustomMovesB_RecordFlinch(bank);
}
LV200_CHAIN_VALUE(ShouldReflectStatDrop, bool8, (u8 bank, u8 source, bool8 value), (bank, source, value))

#undef LV200_CHAIN_VALUE

bool8 Lv200CustomMove_IsHeartGuard(u8 bank)
{
	return Lv200CustomMovesB_IsHeartGuard(bank);
}

bool8 Lv200CustomMove_IsFixedFreeze(u8 bank)
{
	return Lv200CustomMovesB_IsFixedFreeze(bank);
}

bool8 Lv200CustomMove_IsWingGuard(u8 bank)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	return state != NULL && bank < MAX_BATTLERS_COUNT && (state->wingGuardBits & gBitTable[bank]) != 0;
}

/* ひこうのけっかいの印は flyingBarrierTimer[SIDE] で持つ（側のタイマー・交代で出た子も浮く）。 */
bool8 Lv200CustomMove_IsFlyingBarrier(u8 bank)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	return state != NULL && bank < MAX_BATTLERS_COUNT && state->flyingBarrierTimer[SIDE(bank)] != 0;
}

void Lv200CustomMove_ClearFlyingBarrier(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	if (state != NULL)
	{
		state->flyingBarrierTimer[B_SIDE_PLAYER] = 0;
		state->flyingBarrierTimer[B_SIDE_OPPONENT] = 0;
	}
}

bool8 Lv200CustomMove_IsPoisonHarmless(u8 bank)
{
	return Lv200CustomMovesA_IsPoisonHarmless(bank);
}

bool8 Lv200CustomMove_CanRaiseStat(u8 bank, u8 source, bool8 value)
{
	return Lv200CustomMovesB_CanRaiseStat(bank, source, value);
}

u16 Lv200CustomMove_GetItemForEffect(u8 bank, u16 value)
{
	return Lv200CustomMovesB_GetItemForEffect(bank, value);
}

void Lv200CustomMove_OnMoveEnd(u16 move, u8 bankAtk, u8 bankDef, bool8 succeeded, bool8 didDamage)
{
	Lv200CustomMovesA_OnMoveEnd(move, bankAtk, bankDef, succeeded, didDamage);
	Lv200CustomMovesB_OnMoveEnd(move, bankAtk, bankDef, succeeded, didDamage);
}

void Lv200CustomMove_OnContact(u16 move, u8 bankAtk, u8 bankDef, bool8 protected)
{
	Lv200CustomMovesA_OnContact(move, bankAtk, bankDef, protected);
	Lv200CustomMovesB_OnContact(move, bankAtk, bankDef, protected);
}

void Lv200CustomMove_OnFaint(u8 bank, u8 source, u16 move, u32 damage)
{
	Lv200CustomMovesA_OnFaint(bank, source, move, damage);
	Lv200CustomMovesB_OnFaint(bank, source, move, damage);
}

void Lv200CustomMove_OnBattleEnd(void)
{
	Lv200CustomMovesA_OnBattleEnd();
	Lv200CustomMovesB_OnBattleEnd();
}

void Lv200CustomMove_OnStatusCured(u8 bank)
{
	Lv200CustomMovesA_OnStatusCured(bank);
	Lv200CustomMovesB_OnStatusCured(bank);
}

void Lv200CustomMove_OnHazardsCleared(u8 side)
{
	Lv200CustomMovesA_OnHazardsCleared(side);
	Lv200CustomMovesB_OnHazardsCleared(side);
}

void Lv200CustomMove_OnScreensRemoved(u8 side)
{
	Lv200CustomMovesA_OnScreensRemoved(side);
	Lv200CustomMovesB_OnScreensRemoved(side);
}

bool8 Lv200CustomMove_ConsumeDodge(u16 move, u8 bankAtk, u8 bankDef)
{
	return Lv200CustomMovesA_ConsumeDodge(move, bankAtk, bankDef);
}

u8 Lv200CustomMove_StanceCritBonus(u8 bank)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	return state != NULL && bank < MAX_BATTLERS_COUNT && state->stanceTimer[bank] != 0;
}

bool8 Lv200CustomMove_IsMudFieldActive(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	return state != NULL && state->mudFieldTimer != 0;
}
