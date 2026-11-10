#pragma once

#include "defines.h"
#include "defines_battle.h"

/* Lv200 B 側90件の技効果と、共有する戦闘中状態。 */

enum Lv200CustomMovesBField
{
	LV200_B_FIELD_NONE,
	LV200_B_FIELD_DARKNESS,
	LV200_B_FIELD_NIGHT_SEA,
};

enum Lv200CustomMovesBTerrainOwner
{
	LV200_B_TERRAIN_OWNER_NONE,
	LV200_B_TERRAIN_OWNER_FOREST_LIGHT,
	LV200_B_TERRAIN_OWNER_THUNDEROUS_ECHO,
	LV200_B_TERRAIN_OWNER_FAIRY_RING,
};

enum Lv200CustomMovesBSubstituteEffect
{
	LV200_B_SUBSTITUTE_EFFECT_NONE,
	LV200_B_SUBSTITUTE_EFFECT_REGEN,
	LV200_B_SUBSTITUTE_EFFECT_ATK_DROP,
};

enum Lv200CustomMovesBPendingPositionEffect
{
	LV200_B_PENDING_SPEED_UP = 1 << 0,
	LV200_B_PENDING_CHARGE = 1 << 1,
	LV200_B_PENDING_OMNIBOOST = 1 << 2,
	LV200_B_PENDING_ICE_HAZARD = 1 << 3,
};

#include "../include/new/lv200_custom_moves_state.h"

void Lv200CustomMovesB_ResetStateData(struct Lv200CustomMovesBState* state);
void Lv200CustomMovesB_ClearBattler(struct Lv200CustomMovesBState* state, u8 bank);
void Lv200CustomMovesB_RollRoundState(struct Lv200CustomMovesBState* state);
void Lv200CustomMovesB_RecordDamage(struct Lv200CustomMovesBState* state, u8 bank, u32 damage);
void Lv200CustomMovesB_AddPendingPositionEffect(struct Lv200CustomMovesBState* state, u8 position, u8 effects);
u8 Lv200CustomMovesB_TakePendingPositionEffects(struct Lv200CustomMovesBState* state, u8 position);

/* 共通 Lv200 dispatcher から呼ぶ B 側 interface。 */
struct Lv200CustomMovesBState* Lv200CustomMove_GetStateB(void);
const u8* Lv200CustomMovesB_GetScript(u16 move);
u16 Lv200CustomMovesB_ModifyPower(u16 move, u8 atk, u8 def, u16 value);
u32 Lv200CustomMovesB_ModifyAccuracy(u16 move, u8 atk, u8 def, u32 value);
u8 Lv200CustomMovesB_ModifyType(u16 move, u8 atk, u8 value);
s8 Lv200CustomMovesB_ModifyPriority(u16 move, u8 atk, u8 def, s8 value);
s32 Lv200CustomMovesB_ModifyDamage(u16 move, u8 atk, u8 def, s32 value);
u32 Lv200CustomMovesB_ModifyLeechSeedDamage(u8 bank, u32 value);
bool8 Lv200CustomMovesB_ShouldConsumePP(u16 move, u8 atk, bool8 value);
u32 Lv200CustomMovesB_ModifyAttackValue(u16 move, u8 atk, u8 def, u32 value);
u32 Lv200CustomMovesB_ModifySpeed(u8 bank, u32 value);
u16 Lv200CustomMovesB_ModifyTypeEffectiveness(u16 move, u8 atk, u8 def, u16 value);
bool8 Lv200CustomMovesB_IsBattlerOfType(u8 bank, u8 type, bool8 value);
bool8 Lv200CustomMovesB_IsAbilityActive(u8 bank, bool8 value);
bool8 Lv200CustomMovesB_CanLowerStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_CanRaiseStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_CanFlinch(u8 bank, u8 source, bool8 value);
void Lv200CustomMovesB_RecordFlinch(u8 bank);
bool8 Lv200CustomMovesB_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_IsItemActive(u8 bank, bool8 value);
u16 Lv200CustomMovesB_GetItemForEffect(u8 bank, u16 value);
void Lv200CustomMovesB_OnMoveHit(u16 move, u8 atk, u8 def, u32 damage,
								 bool8 hitSubstitute, bool8 substituteBroke);
void Lv200CustomMovesB_OnMoveEnd(u16 move, u8 atk, u8 def,
								 bool8 succeeded, bool8 didDamage);
void Lv200CustomMovesB_OnContact(u16 move, u8 atk, u8 def, bool8 protected);
void Lv200CustomMovesB_OnFaint(u8 bank, u8 source, u16 move, u32 damage);
bool8 Lv200CustomMovesB_CanSwitch(u8 bank, bool8 value);
const u8* Lv200CustomMovesB_GetEndTurnScript(void);
const u8* Lv200CustomMovesB_GetMoveEndScript(void);
const u8* Lv200CustomMovesB_GetSwitchInScript(u8 bank);
void Lv200CustomMovesB_OnBattleEnd(void);
void Lv200CustomMovesB_OnTurnEnd(void);
void Lv200CustomMovesB_OnSwitchOut(u8 bank);
void Lv200CustomMovesB_OnSwitchIn(u8 bank);
void Lv200CustomMovesB_OnStatusCured(u8 bank);
void Lv200CustomMovesB_OnHazardsCleared(u8 side);
void Lv200CustomMovesB_OnScreensRemoved(u8 side);
void Lv200CustomMovesB_ResetState(void);

/* B側専用 battle script から呼ぶ準備処理。 */
void Lv200B_PrepareWaterCounter(void);
void Lv200B_CheckDeepSeaMemory(void);
void Lv200B_PrepareGreatWhirlpool(void);
void Lv200B_PrepareDeepSeaSongHeal(void);
void Lv200B_FixDeepSeaSongSleep(void);
void Lv200B_CheckSeaGodFeast(void);
void Lv200B_HealSeaGodReserves(void);
void Lv200B_PrepareSeaGodUserHeal(void);
void Lv200B_PrepareSeaGodPartnerHeal(void);
void Lv200B_PrepareSummitIcicle(void);
void Lv200B_PrepareTwinDragonClaw(void);
void Lv200B_PrepareDragonSoul(void);
void Lv200B_PrepareDragonCurseCost(void);
void Lv200B_PrepareDragonCurseBackfire(void);
void Lv200B_PrepareDragonMelody(void);
void Lv200B_RecordDragonReversalFlinch(void);
void Lv200B_PrepareDragonTemptation(void);
void Lv200B_PrepareTempest(void);
void Lv200B_PrepareHeartGuard(void);
void Lv200B_PrepareRootDrain(void);
void Lv200B_PreparePhotosynthesis(void);
void Lv200B_PrepareForestLight(void);
void Lv200B_PrepareUprootAttack(void);
void Lv200B_PrepareThunderousEcho(void);
void Lv200B_PrepareTempestTerrain(void);
void Lv200B_PrepareBlackHole(void);
void Lv200B_PrepareEdgeOfMindHeal(void);
void Lv200B_ApplyEdgeOfMindSleep(void);
void Lv200B_PrepareBlizzardArmor(void);
void Lv200B_CheckStatusMove(void);
void Lv200B_PrepareSeaOfIce(void);
void Lv200B_PrepareQuarterRecoil(void);
void Lv200B_RollParalysis40(void);

