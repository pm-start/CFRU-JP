#pragma once

#include "defines.h"

struct Lv200CustomMovesAState;
struct Lv200CustomMovesBState;

/* EFFECT_HACK_CUSTOM の callasm 入口。未登録の技は失敗台本へ送る。 */
void Lv200CustomMove_Dispatch(void);

/* A/B の実装が返す技ごとの台本。担当外の技は NULL を返す。 */
const u8 *Lv200CustomMovesA_GetScript(u16 move);
const u8 *Lv200CustomMovesB_GetScript(u16 move);

u16 Lv200CustomMovesA_ModifyPower(u16 move, u8 bankAtk, u8 bankDef, u16 value);
u16 Lv200CustomMovesB_ModifyPower(u16 move, u8 bankAtk, u8 bankDef, u16 value);
u32 Lv200CustomMovesA_ModifyAccuracy(u16 move, u8 bankAtk, u8 bankDef, u32 value);
u32 Lv200CustomMovesB_ModifyAccuracy(u16 move, u8 bankAtk, u8 bankDef, u32 value);
u8 Lv200CustomMovesA_ModifyType(u16 move, u8 bankAtk, u8 value);
u8 Lv200CustomMovesB_ModifyType(u16 move, u8 bankAtk, u8 value);
u32 Lv200CustomMovesA_ModifySpeed(u8 bank, u32 value);
u32 Lv200CustomMovesB_ModifySpeed(u8 bank, u32 value);
u32 Lv200CustomMovesA_ModifyLeechSeedDamage(u8 bank, u32 value);
u32 Lv200CustomMovesB_ModifyLeechSeedDamage(u8 bank, u32 value);
void Lv200CustomMovesA_ResetState(void);
void Lv200CustomMovesB_ResetState(void);
void Lv200CustomMovesA_OnTurnEnd(void);
void Lv200CustomMovesB_OnTurnEnd(void);
const u8 *Lv200CustomMovesA_GetEndTurnScript(void);
const u8 *Lv200CustomMovesB_GetEndTurnScript(void);
const u8 *Lv200CustomMovesA_GetSwitchInScript(u8 bank);
const u8 *Lv200CustomMovesB_GetSwitchInScript(u8 bank);
const u8 *Lv200CustomMovesA_GetMoveEndScript(void);
const u8 *Lv200CustomMovesB_GetMoveEndScript(void);
void Lv200CustomMovesA_OnSwitchOut(u8 bank);
void Lv200CustomMovesB_OnSwitchOut(u8 bank);
void Lv200CustomMovesA_OnSwitchIn(u8 bank);
void Lv200CustomMovesB_OnSwitchIn(u8 bank);
void Lv200CustomMovesA_OnMoveHit(u16 move, u8 bankAtk, u8 bankDef, u32 damage,
								 bool8 hitSubstitute, bool8 substituteBroke);
void Lv200CustomMovesB_OnMoveHit(u16 move, u8 bankAtk, u8 bankDef, u32 damage,
								 bool8 hitSubstitute, bool8 substituteBroke);
#define LV200_DECLARE_SIDE_API(side) \
	s8 Lv200CustomMoves##side##_ModifyPriority(u16 move, u8 bankAtk, u8 bankDef, s8 value); \
	s32 Lv200CustomMoves##side##_ModifyDamage(u16 move, u8 bankAtk, u8 bankDef, s32 value); \
	bool8 Lv200CustomMoves##side##_ShouldConsumePP(u16 move, u8 bankAtk, bool8 value); \
	void Lv200CustomMoves##side##_OnMoveEnd(u16 move, u8 bankAtk, u8 bankDef, bool8 succeeded, bool8 didDamage); \
	void Lv200CustomMoves##side##_OnContact(u16 move, u8 bankAtk, u8 bankDef, bool8 protected); \
	void Lv200CustomMoves##side##_OnFaint(u8 bank, u8 source, u16 move, u32 damage); \
	bool8 Lv200CustomMoves##side##_CanSwitch(u8 bank, bool8 value); \
	void Lv200CustomMoves##side##_OnBattleEnd(void); \
	u32 Lv200CustomMoves##side##_ModifyAttackValue(u16 move, u8 bankAtk, u8 bankDef, u32 value); \
	u16 Lv200CustomMoves##side##_ModifyTypeEffectiveness(u16 move, u8 bankAtk, u8 bankDef, u16 value); \
	bool8 Lv200CustomMoves##side##_IsBattlerOfType(u8 bank, u8 type, bool8 value); \
	bool8 Lv200CustomMoves##side##_IsAbilityActive(u8 bank, bool8 value); \
	bool8 Lv200CustomMoves##side##_IsItemActive(u8 bank, bool8 value)

LV200_DECLARE_SIDE_API(A);
LV200_DECLARE_SIDE_API(B);
#undef LV200_DECLARE_SIDE_API
bool8 Lv200CustomMovesA_CanLowerStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_CanLowerStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_CanRaiseStat(u8 bank, u8 source, bool8 value);
u16 Lv200CustomMovesB_GetItemForEffect(u8 bank, u16 value);
bool8 Lv200CustomMovesA_CanFlinch(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_CanFlinch(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesA_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMovesB_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value);
void Lv200CustomMovesA_OnStatusCured(u8 bank);
void Lv200CustomMovesB_OnStatusCured(u8 bank);
void Lv200CustomMovesA_OnHazardsCleared(u8 side);
void Lv200CustomMovesB_OnHazardsCleared(u8 side);
void Lv200CustomMovesA_OnScreensRemoved(u8 side);
void Lv200CustomMovesB_OnScreensRemoved(u8 side);

/* 計算途中の値を順番に渡す。該当しない技は value をそのまま返す。 */
u16 Lv200CustomMove_ModifyPower(u16 move, u8 bankAtk, u8 bankDef, u16 value);
u32 Lv200CustomMove_ModifyAccuracy(u16 move, u8 bankAtk, u8 bankDef, u32 value);
u8 Lv200CustomMove_ModifyType(u16 move, u8 bankAtk, u8 value);
u32 Lv200CustomMove_ModifySpeed(u8 bank, u32 value);
u32 Lv200CustomMove_ModifyLeechSeedDamage(u8 bank, u32 value);

/* 状態の寿命を管理する共通phase。 */
void Lv200CustomMove_ResetState(void);
void Lv200CustomMove_OnTurnEnd(void);
bool8 Lv200CustomMove_TryEndTurnEffect(void);
const u8 *Lv200CustomMove_GetSwitchInScript(u8 bank);
const u8 *Lv200CustomMove_GetMoveEndScript(void);
void Lv200CustomMove_OnSwitchOut(u8 bank);
void Lv200CustomMove_OnSwitchIn(u8 bank);
void Lv200CustomMove_OnMoveHit(u16 move, u8 bankAtk, u8 bankDef, u32 damage,
							  bool8 hitSubstitute, bool8 substituteBroke);
s8 Lv200CustomMove_ModifyPriority(u16 move, u8 bankAtk, u8 bankDef, s8 value);
s32 Lv200CustomMove_ModifyDamage(u16 move, u8 bankAtk, u8 bankDef, s32 value);
/* #35 P5: 場の効果で加わる／外れるタイプを相性・一致ボーナスに反映する窓口（ひょうがじだいなど）。 */
void Lv200CustomMove_AdjustTypes(u8 bank, u8 *type1, u8 *type2, u8 *type3);
void Lv200CustomMovesB_AdjustTypes(u8 bank, u8 *type1, u8 *type2, u8 *type3);
u32 Lv200CustomMovesB_SingleScreenDamage(u32 damage, u8 bankDef);
u32 Lv200CustomMovesB_GrassyTerrainPower(u32 power, u32 boost);
/* 1 体のときの壁のダメージ（ミントのかべの2枚は2/3）・草フィールドの強化（じゅかいのひかりは1.5）。 */
u32 Lv200CustomMove_SingleScreenDamage(u32 damage, u8 bankDef);
u32 Lv200CustomMove_GrassyTerrainPower(u32 power, u32 boost);
bool8 Lv200CustomMove_ShouldConsumePP(u16 move, u8 bankAtk, bool8 value);
void Lv200CustomMove_OnMoveEnd(u16 move, u8 bankAtk, u8 bankDef, bool8 succeeded, bool8 didDamage);
void Lv200CustomMove_OnContact(u16 move, u8 bankAtk, u8 bankDef, bool8 protected);
void Lv200CustomMove_OnFaint(u8 bank, u8 source, u16 move, u32 damage);
bool8 Lv200CustomMove_CanSwitch(u8 bank, bool8 value);
void Lv200CustomMove_OnBattleEnd(void);
void Lv200CustomMove_OnStatusCured(u8 bank);
void Lv200CustomMove_OnHazardsCleared(u8 side);
void Lv200CustomMove_OnScreensRemoved(u8 side);
u32 Lv200CustomMove_ModifyAttackValue(u16 move, u8 bankAtk, u8 bankDef, u32 value);
u16 Lv200CustomMove_ModifyTypeEffectiveness(u16 move, u8 bankAtk, u8 bankDef, u16 value);
bool8 Lv200CustomMove_IsBattlerOfType(u8 bank, u8 type, bool8 value);
bool8 Lv200CustomMove_IsAbilityActive(u8 bank, bool8 value);
bool8 Lv200CustomMove_IsPoisonHarmless(u8 bank);
/* P2: 独自効果の「元技の枝に入らない」穴を埋める問い合わせ（#35）。 */
bool8 Lv200CustomMove_IsHeartGuard(u8 bank);         /* ハートのまもりで構え中（接触した相手をメロメロに・#35 P5） */
bool8 Lv200CustomMove_IsFixedFreeze(u8 bank);        /* アブソルートゼロの固定凍結中（20%の解凍をしない・#35 P5） */
bool8 Lv200CustomMovesB_IsHeartGuard(u8 bank);
bool8 Lv200CustomMovesB_IsFixedFreeze(u8 bank);
bool8 Lv200CustomMove_IsWingGuard(u8 bank);          /* つばさのまもりで構え中（接触した相手を強制交代） */
bool8 Lv200CustomMove_IsFlyingBarrier(u8 bank);      /* ひこうのけっかい（bank の側に5ターンの印がある） */
void Lv200CustomMove_ClearFlyingBarrier(void);       /* グラビティで両側の印を消す */
bool8 Lv200CustomMove_IsItemActive(u8 bank, bool8 value);
u16 Lv200CustomMove_GetItemForEffect(u8 bank, u16 value);
bool8 Lv200CustomMove_CanLowerStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMove_CanRaiseStat(u8 bank, u8 source, bool8 value);
bool8 Lv200CustomMove_CanFlinch(u8 bank, u8 source, bool8 value);
void Lv200CustomMove_RecordFlinch(u8 bank);
bool8 Lv200CustomMove_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value);

/* 固定writable pointerを持たず、gNewBS末尾を都度参照する。 */
struct Lv200CustomMovesAState *Lv200CustomMove_GetStateA(void);
struct Lv200CustomMovesBState *Lv200CustomMove_GetStateB(void);

/* #35 P4: ぶんしんのまいの回避（必中技も）・ぶんどうのかまえの急所+1・ぬかるみ中のどろあそび */
bool8 Lv200CustomMove_ConsumeDodge(u16 move, u8 bankAtk, u8 bankDef);
u8 Lv200CustomMove_StanceCritBonus(u8 bank);
bool8 Lv200CustomMove_IsMudFieldActive(void);
