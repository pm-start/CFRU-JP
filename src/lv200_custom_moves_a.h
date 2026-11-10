#pragma once

#include "defines.h"
#include "defines_battle.h"

/*
 * A側100技が使う戦闘中だけの状態。
 * 固定RAMやwritable globalへ置かず、共通側の
 * Lv200CustomMove_GetStateA() が gNewBS 末尾を都度返す。
 */
#include "../include/new/lv200_custom_moves_state.h"

const u8 *Lv200CustomMovesA_GetScript(u16 move);
const u8 *Lv200CustomMovesA_GetEndTurnScript(void);
const u8 *Lv200CustomMovesA_GetMoveEndScript(void);
const u8 *Lv200CustomMovesA_GetSwitchInScript(u8 bank);
void Lv200A_PrepareFighting180(void);
void Lv200A_CureStatusGround180(void);
void Lv200A_CureConfusionGround180(void);
void Lv200A_CheckStatusMove(void);
void Lv200A_ApplyStatusMove(void);
void Lv200A_SetPoisonKingPopup(void);
void Lv200A_PrepareMultiHit(void);
void Lv200A_PrepareCounter(void);
void Lv200A_ToggleBatonState(void);
void Lv200A_ApplyPerishTarget(void);
void Lv200A_ClearEchoReplay(void);
u16 Lv200CustomMovesA_ModifyPower(u16 move, u8 bankAtk, u8 bankDef, u16 value);
u32 Lv200CustomMovesA_ModifyAccuracy(u16 move, u8 bankAtk, u8 bankDef, u32 value);
u8 Lv200CustomMovesA_ModifyType(u16 move, u8 bankAtk, u8 value);
u32 Lv200CustomMovesA_ModifySpeed(u8 bank, u32 value);
s8 Lv200CustomMovesA_ModifyPriority(u16 move, u8 bankAtk, u8 bankDef, s8 value);
s32 Lv200CustomMovesA_ModifyDamage(u16 move, u8 bankAtk, u8 bankDef, s32 value);
bool8 Lv200CustomMovesA_ShouldConsumePP(u16 move, u8 bankAtk, bool8 value);
u32 Lv200CustomMovesA_ModifyAttackValue(u16 move, u8 bankAtk, u8 bankDef, u32 value);
u32 Lv200CustomMovesA_ModifyLeechSeedDamage(u8 bank, u32 value);
u16 Lv200CustomMovesA_ModifyTypeEffectiveness(u16 move, u8 bankAtk, u8 bankDef, u16 value);
bool8 Lv200CustomMovesA_IsBattlerOfType(u8 bank, u8 type, bool8 value);
bool8 Lv200CustomMovesA_ConsumeDodge(u16 move, u8 bankAtk, u8 bankDef);
void Lv200CustomMovesA_AdjustTypes(u8 bank, u8 *type1, u8 *type2, u8 *type3);
bool8 Lv200CustomMovesA_IsAbilityActive(u8 bank, bool8 value);
bool8 Lv200CustomMovesA_IsPoisonHarmless(u8 bank);
bool8 Lv200CustomMovesA_CanLowerStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesA_CanFlinch(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesA_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesA_IsItemActive(u8 bank, bool8 value);
void Lv200CustomMovesA_OnTurnEnd(void);
void Lv200CustomMovesA_OnSwitchOut(u8 bank);
void Lv200CustomMovesA_OnSwitchIn(u8 bank);
void Lv200CustomMovesA_OnStatusCured(u8 bank);
void Lv200CustomMovesA_OnHazardsCleared(u8 side);
void Lv200CustomMovesA_OnScreensRemoved(u8 side);
void Lv200CustomMovesA_ResetState(void);
void Lv200CustomMovesA_OnMoveEnd(u16 move, u8 bankAtk, u8 bankDef,
								 bool8 succeeded, bool8 didDamage);
void Lv200CustomMovesA_OnContact(u16 move, u8 bankAtk, u8 bankDef, bool8 protected);
void Lv200CustomMovesA_OnFaint(u8 bank, u8 source, u16 move, u32 damage);
bool8 Lv200CustomMovesA_CanSwitch(u8 bank, bool8 value);
void Lv200CustomMovesA_OnBattleEnd(void);

/* 命中確定後の接続点。共通側へ未接続の間はmanifestを完成扱いにしない。 */
void Lv200CustomMovesA_OnMoveHit(u16 move, u8 bankAtk, u8 bankDef, u32 damage,
								 bool8 hitSubstitute, bool8 substituteBroke);

