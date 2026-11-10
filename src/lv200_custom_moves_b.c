#include "lv200_custom_moves_b.h"
#include "lv200_custom_moves.h"
#include "lv200_custom_common.h"
#include "hack_move_alias.h"
#include "../include/battle_message.h"
#include "../include/new/ability_tables.h"
#include "../include/new/battle_util.h"
#include "../include/new/move_battle_scripts.h"
#include "../include/new/move_tables.h"
#include "../include/new/util.h"
#include "../include/random.h"

extern const u8 BattleScript_Lv200B_EndTurnDamage[];
extern const u8 BattleScript_Lv200B_EndTurnMessage[];
extern const u8 gText_Lv200B_DeepSeaSet[];
extern const u8 gText_Lv200B_DeepSeaEnd[];
extern const u8 gText_Lv200B_UprootBuffsGone[];
extern const u8 gText_Lv200B_WhirlpoolMirror[];
extern const u8 gText_Lv200B_FrostbiteSet[];
extern const u8 gText_Lv200B_FrostbiteTick[];
extern const u8 gText_Lv200B_FrozenHazardSet[];
extern const u8 gText_Lv200B_IceAgeSet[];
extern const u8 gText_Lv200B_IceAgeEnd[];
extern const u8 gText_Lv200B_DarknessSet[];
extern const u8 gText_Lv200B_DarknessEnd[];
extern const u8 gText_Lv200B_NightSeaSet[];
extern const u8 gText_Lv200B_NightSeaEnd[];
extern const u8 gText_Lv200B_AbilityItemNullSet[];
extern const u8 gText_Lv200B_AbilityItemNullEnd[];
extern const u8 gText_Lv200B_DragonPalaceSet[];
extern const u8 gText_Lv200B_DragonPalaceEnd[];
extern const u8 gText_Lv200B_RilGazeSet[];
extern const u8 gText_Lv200B_RilGazeEnd[];
extern const u8 gText_Lv200B_KeepsakeSet[];
extern const u8 BattleScript_Lv200B_EndTurnHeal[];
extern const u8 BattleScript_Lv200B_EndTurnStatDown[];
extern const u8 BattleScript_Lv200B_MirrorDamage[];
extern const u8 BattleScript_Lv200B_MoveEndStatDown[];
extern const u8 BattleScript_Lv200B_MoveEndStatUp[];
extern const u8 BattleScript_Lv200B_SwitchInCharge[];
extern const u8 BattleScript_Lv200B_SwitchInOmniboost[];
extern const u8 BattleScript_Lv200B_SwitchInStatUp[];
extern const u8 GrassyTerrainSetString[];
extern const u8 ElectricTerrainSetString[];
extern const u8 MistyTerrainSetString[];
extern const u8 GravitySetString[];
extern const u8 TerrainEndString[];

/* 状態寿命の共通処理だけ。90件の技効果とdispatcherはまだ実装していない。 */

static void ClearBankBit(u8* bits, u8 bank)
{
	*bits &= ~gBitTable[bank];
}

void Lv200CustomMovesB_ResetStateData(struct Lv200CustomMovesBState* state)
{
	u8* bytes;
	u32 i;

	if (state == NULL)
		return;

	bytes = (u8*) state;
	for (i = 0; i < sizeof(*state); ++i)
		bytes[i] = 0;
}

void Lv200CustomMovesB_ClearBattler(struct Lv200CustomMovesBState* state, u8 bank)
{
	if (state == NULL || bank >= MAX_BATTLERS_COUNT)
		return;

	state->deepSeaTimer[bank] = 0;
	state->timedTrapTimer[bank] = 0;
	state->timedTrapOwner[bank] = 0;
	state->chargeStacks[bank] = 0;
	state->electricPpPhase[bank] = 0;
	state->microwaveTimer[bank] = 0;
	state->mirrorTimer[bank] = 0;
	state->mirrorSource[bank] = 0;
	state->coldGazeTimer[bank] = 0;
	state->fixedFreezeTimer[bank] = 0;
	state->selfSureHitTimer[bank] = 0;
	state->timedAbilitySuppress[bank] = 0;
	state->anxietyTimer[bank] = 0;
	state->mimicTimer[bank] = 0;
	state->mimicSavedPp[bank] = 0;
	state->mimicSlot[bank] = 0;
	state->temporaryRilTimer[bank] = 0;
	state->statRiseBlockedTimer[bank] = 0;
	state->substituteEffect[bank] = LV200_B_SUBSTITUTE_EFFECT_NONE;
	state->mimicSavedMove[bank] = MOVE_NONE;
	state->copiedItem[bank] = ITEM_NONE;
	state->mirrorSpecialDamage[bank] = 0;
	state->currentRoundDamage[bank] = 0;
	state->previousRoundDamage[bank] = 0;
	state->lastDamageTaken[bank] = 0;
	state->lastDamageSource[bank] = 0;

	ClearBankBit(&state->strongLeechBits, bank);
	ClearBankBit(&state->armoredRootBits, bank);
	ClearBankBit(&state->suspiciousSleepBits, bank);
	ClearBankBit(&state->frostbiteBits, bank);
	ClearBankBit(&state->frostbitePendingBits, bank);
	ClearBankBit(&state->dragonSoulBits, bank);
	ClearBankBit(&state->dragon130SpeedDownBits, bank);
	ClearBankBit(&state->dragon170SpeedUpBits, bank);
	ClearBankBit(&state->flinchedThisRoundBits, bank);
	ClearBankBit(&state->flinchedPreviousRoundBits, bank);
	ClearBankBit(&state->heartGuardBits, bank);
	ClearBankBit(&state->dreamPrincessBits, bank);
	ClearBankBit(&state->statRiseBlockedBits, bank);
	ClearBankBit(&state->abilitySuppressOwnedBits, bank);
	ClearBankBit(&state->darknessHoleItemSuppressedBits, bank);
	ClearBankBit(&state->destinySuccessorBits, bank);
	ClearBankBit(&state->tearMoonArmedBits, bank);
	ClearBankBit(&state->tearMoonBoostBits, bank);
	ClearBankBit(&state->psychic170BoostBits, bank);
}

void Lv200CustomMovesB_RollRoundState(struct Lv200CustomMovesBState* state)
{
	u8 bank;

	if (state == NULL)
		return;

	state->flinchedPreviousRoundBits = state->flinchedThisRoundBits;
	state->flinchedThisRoundBits = 0;
	state->heartGuardBits = 0;

	for (bank = 0; bank < MAX_BATTLERS_COUNT; ++bank)
	{
		state->previousRoundDamage[bank] = state->currentRoundDamage[bank];
		state->currentRoundDamage[bank] = 0;
	}
}

void Lv200CustomMovesB_RecordDamage(struct Lv200CustomMovesBState* state, u8 bank, u32 damage)
{
	if (state == NULL || bank >= MAX_BATTLERS_COUNT)
		return;

	if (0xFFFFFFFF - state->currentRoundDamage[bank] < damage)
		state->currentRoundDamage[bank] = 0xFFFFFFFF;
	else
		state->currentRoundDamage[bank] += damage;
}

void Lv200CustomMovesB_AddPendingPositionEffect(struct Lv200CustomMovesBState* state, u8 position, u8 effects)
{
	if (state == NULL || position >= MAX_BATTLERS_COUNT)
		return;

	state->pendingPositionEffects[position] |= effects;
}

u8 Lv200CustomMovesB_TakePendingPositionEffects(struct Lv200CustomMovesBState* state, u8 position)
{
	u8 effects;

	if (state == NULL || position >= MAX_BATTLERS_COUNT)
		return 0;

	effects = state->pendingPositionEffects[position];
	state->pendingPositionEffects[position] = 0;
	return effects;
}

#define LV200_B_MOVE_LIST(X) \
	X(MOVE_LV200_WATER_110, water_tide) \
	X(MOVE_LV200_WATER_120, water_counter) \
	X(MOVE_LV200_WATER_130, deep_sea_memory) \
	X(MOVE_LV200_WATER_140, stream_doll_pivot) \
	X(MOVE_LV200_WATER_150, great_whirlpool) \
	X(MOVE_LV200_WATER_160, drop_judgment) \
	X(MOVE_LV200_WATER_170, deep_sea_song) \
	X(MOVE_LV200_WATER_180, whirlpool_mirror) \
	X(MOVE_LV200_WATER_190, sea_god_feast) \
	X(MOVE_LV200_WATER_200, atlantis) \
	X(MOVE_LV200_GRASS_110, root_drain) \
	X(MOVE_LV200_GRASS_120, binding_seed) \
	X(MOVE_LV200_GRASS_130, uproot_attack) \
	X(MOVE_LV200_GRASS_140, dividing_breath) \
	X(MOVE_LV200_GRASS_150, leech_seed_rain) \
	X(MOVE_LV200_GRASS_160, great_tree_armor) \
	X(MOVE_LV200_GRASS_170, photosynthesis) \
	X(MOVE_LV200_GRASS_180, shower_forest) \
	X(MOVE_LV200_GRASS_190, forest_light) \
	X(MOVE_LV200_GRASS_200, forest_judgment) \
	X(MOVE_LV200_ELECTRIC_110, charge_spark) \
	X(MOVE_LV200_ELECTRIC_120, electric_tackle) \
	X(MOVE_LV200_ELECTRIC_130, sealing_shock) \
	X(MOVE_LV200_ELECTRIC_140, magnetic_whip) \
	X(MOVE_LV200_ELECTRIC_150, thunderous_echo) \
	X(MOVE_LV200_ELECTRIC_160, floating_shock) \
	X(MOVE_LV200_ELECTRIC_170, electromagnetic_blizzard) \
	X(MOVE_LV200_ELECTRIC_180, microwave) \
	X(MOVE_LV200_ELECTRIC_190, dead_heat) \
	X(MOVE_LV200_ELECTRIC_200, tempest) \
	X(MOVE_LV200_PSYCHIC_110, mirror_mirror) \
	X(MOVE_LV200_PSYCHIC_120, prophetic_heart) \
	X(MOVE_LV200_PSYCHIC_130, mint_wall) \
	X(MOVE_LV200_PSYCHIC_140, edge_of_mind) \
	X(MOVE_LV200_PSYCHIC_150, vacuum_attack) \
	X(MOVE_LV200_PSYCHIC_160, mind_sharing) \
	X(MOVE_LV200_PSYCHIC_170, source_wave) \
	X(MOVE_LV200_PSYCHIC_180, black_hole) \
	X(MOVE_LV200_PSYCHIC_190, suspicious_sleep) \
	X(MOVE_LV200_PSYCHIC_200, psychic_unison) \
	X(MOVE_LV200_ICE_110, frostbite) \
	X(MOVE_LV200_ICE_120, blizzard_armor) \
	X(MOVE_LV200_ICE_130, glacier_awakening) \
	X(MOVE_LV200_ICE_140, freezing_master) \
	X(MOVE_LV200_ICE_150, sea_of_ice) \
	X(MOVE_LV200_ICE_160, iceberg_collapse) \
	X(MOVE_LV200_ICE_170, summit_icicle) \
	X(MOVE_LV200_ICE_180, cold_body_gaze) \
	X(MOVE_LV200_ICE_190, ice_age) \
	X(MOVE_LV200_ICE_200, absolute_zero) \
	X(MOVE_LV200_DRAGON_110, dragon_soul) \
	X(MOVE_LV200_DRAGON_120, dragon_reversal) \
	X(MOVE_LV200_DRAGON_130, last_dragon) \
	X(MOVE_LV200_DRAGON_140, dragon_palace) \
	X(MOVE_LV200_DRAGON_150, dragon_curse) \
	X(MOVE_LV200_DRAGON_160, time_tyrant) \
	X(MOVE_LV200_DRAGON_170, twin_dragon_claw) \
	X(MOVE_LV200_DRAGON_180, dragon_melody) \
	X(MOVE_LV200_DRAGON_190, dragon_temptation) \
	X(MOVE_LV200_DRAGON_200, dragon_end) \
	X(MOVE_LV200_DARK_110, copy_theft) \
	X(MOVE_LV200_DARK_120, cowardice_of_heart) \
	X(MOVE_LV200_DARK_130, dark_ambush) \
	X(MOVE_LV200_DARK_140, spreading_darkness) \
	X(MOVE_LV200_DARK_150, crumbling_boundary) \
	X(MOVE_LV200_DARK_160, betrayal_song) \
	X(MOVE_LV200_DARK_170, soul_eater) \
	X(MOVE_LV200_DARK_180, world_of_darkness) \
	X(MOVE_LV200_DARK_190, king_of_night) \
	X(MOVE_LV200_DARK_200, darkness_hole) \
	X(MOVE_LV200_FAIRY_110, heartthrob_spark) \
	X(MOVE_LV200_FAIRY_120, heart_guard) \
	X(MOVE_LV200_FAIRY_130, magical_sugar) \
	X(MOVE_LV200_FAIRY_140, love_flash) \
	X(MOVE_LV200_FAIRY_150, fairy_ring) \
	X(MOVE_LV200_FAIRY_160, plush_shadow) \
	X(MOVE_LV200_FAIRY_170, dream_princess) \
	X(MOVE_LV200_FAIRY_180, small_miracle) \
	X(MOVE_LV200_FAIRY_190, milky_gate) \
	X(MOVE_LV200_FAIRY_200, happy_breath) \
	X(MOVE_LV200_RIL_110, put_away_memory) \
	X(MOVE_LV200_RIL_120, phantom_memory) \
	X(MOVE_LV200_RIL_130, ril_gaze) \
	X(MOVE_LV200_RIL_140, lonely_prayer) \
	X(MOVE_LV200_RIL_150, tear_moon) \
	X(MOVE_LV200_RIL_160, lonely_shadow) \
	X(MOVE_LV200_RIL_170, law_of_the_moon) \
	X(MOVE_LV200_RIL_180, keepsake) \
	X(MOVE_LV200_RIL_190, until_goodbye) \
	X(MOVE_LV200_RIL_200, return_to_night)

#define DECLARE_LV200_B_SCRIPT(move, name) \
	extern const u8 BattleScript_Lv200B_##name[];
LV200_B_MOVE_LIST(DECLARE_LV200_B_SCRIPT)

const u8* Lv200CustomMovesB_GetScript(u16 move)
{
	switch (move)
	{
#define LV200_B_SCRIPT_CASE(constant, name) \
		case constant: return BattleScript_Lv200B_##name;
	LV200_B_MOVE_LIST(LV200_B_SCRIPT_CASE)
#undef LV200_B_SCRIPT_CASE
	default:
		return NULL;
	}
}

/* 威力の補正に使う技のタイプ。今撃っている技は特性などで変わった後のタイプ（dynamicMoveType）を見る。 */
static u8 Lv200B_PowerMoveType(u16 move, u8 atk)
{
	u8 type = gBattleMoves[move].type;
	if (move == gCurrentMove && atk == gBankAttacker && (gBattleStruct->dynamicMoveType & 0x3F) != 0)
		type = gBattleStruct->dynamicMoveType & 0x3F;
	return Lv200CustomMovesB_ModifyType(move, atk, type);
}

/* 使用者との性別の関係（どちらかが性別なしなら FALSE）。 */
static bool8 Lv200B_OppositeGender(u8 atk, u8 def)
{
	u8 a = GetGenderFromSpeciesAndPersonality(gBattleMons[atk].species, gBattleMons[atk].personality);
	u8 d = GetGenderFromSpeciesAndPersonality(gBattleMons[def].species, gBattleMons[def].personality);
	return (a == MON_MALE && d == MON_FEMALE) || (a == MON_FEMALE && d == MON_MALE);
}

static bool8 Lv200B_SameGender(u8 atk, u8 def)
{
	u8 a = GetGenderFromSpeciesAndPersonality(gBattleMons[atk].species, gBattleMons[atk].personality);
	u8 d = GetGenderFromSpeciesAndPersonality(gBattleMons[def].species, gBattleMons[def].personality);
	return (a == MON_MALE || a == MON_FEMALE) && a == d;
}

static u16 Lv200B_Scale16(u16 value, u16 numerator, u16 denominator)
{
	u32 scaled = (u32) value * numerator;
	return scaled / denominator;
}

static u32 Lv200B_Scale32(u32 value, u16 numerator, u16 denominator)
{
	return (value * numerator) / denominator;
}

static bool8 Lv200B_IsAlive(u8 bank)
{
	return bank < gBattlersCount && gBattleMons[bank].hp != 0;
}

static bool8 Lv200B_IsBMove(u16 move)
{
	return Lv200CustomMovesB_GetScript(move) != NULL;
}

/* HP の増減は共通部品へ（P3）。戦闘中の値は即時に動かし、HP バー・手持ち・ひんしは MOVE_END で台本が出す。 */
static void Lv200B_HealBank(u8 bank, u32 amount)
{
	if (bank >= gBattlersCount || IsHealBlocked(bank))
		return;
	Lv200Common_HealBank(bank, amount);
}

static void Lv200B_HurtBank(u8 bank, u32 amount)
{
	Lv200Common_HurtBank(bank, amount);
}

static void Lv200B_ChangeStage(u8 bank, u8 stat, s8 delta)
{
	s16 stage;

	if (bank >= gBattlersCount || stat < STAT_STAGE_ATK || stat >= BATTLE_STATS_NO)
		return;
	if (delta > 0 && !Lv200CustomMovesB_CanRaiseStat(bank, bank, TRUE))
		return;
	stage = gBattleMons[bank].statStages[stat - 1] + delta;
	if (stage < STAT_STAGE_MIN)
		stage = STAT_STAGE_MIN;
	else if (stage > STAT_STAGE_MAX)
		stage = STAT_STAGE_MAX;
	gBattleMons[bank].statStages[stat - 1] = stage;
}

static void Lv200B_HealParty(u8 bank, u8 divisor, bool8 waterGetsHalf, bool8 cure)
{
	struct Pokemon* party;
	u8 side;
	u8 i;

	side = SIDE(bank);
	party = side == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u8 activeBank = MAX_BATTLERS_COUNT;
		u8 scanBank;
		u16 hp = GetMonData(&party[i], MON_DATA_HP, NULL);
		u16 maxHP = GetMonData(&party[i], MON_DATA_MAX_HP, NULL);
		u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
		u32 status = 0;
		u32 amount;

		if (species == SPECIES_NONE || hp == 0 || maxHP == 0)
			continue;
		for (scanBank = 0; scanBank < gBattlersCount; ++scanBank)
		{
			if (SIDE(scanBank) == side && gBattlerPartyIndexes[scanBank] == i)
			{
				activeBank = scanBank;
				break;
			}
		}
		if (activeBank < gBattlersCount)
		{
			if (!IsHealBlocked(activeBank))
			{
				amount = gBattleMons[activeBank].maxHP / divisor;
				if (waterGetsHalf && IsOfType(activeBank, TYPE_WATER))
					amount = gBattleMons[activeBank].maxHP / 2;
				Lv200B_HealBank(activeBank, MathMax(1, amount));
			}
			if (cure)
				Lv200Common_QueueCureStatus(activeBank); /* 手持ちへ送り・文・枠の印まで（OnStatusCured も内側で呼ぶ） */
			hp = gBattleMons[activeBank].hp;
			SetMonData(&party[i], MON_DATA_HP, &hp);
			if (cure)
				SetMonData(&party[i], MON_DATA_STATUS, &status);
			continue;
		}
		amount = maxHP / divisor;
		if (waterGetsHalf && IsMonOfType(&party[i], TYPE_WATER))
			amount = maxHP / 2;
		hp += amount;
		if (hp > maxHP)
			hp = maxHP;
		SetMonData(&party[i], MON_DATA_HP, &hp);
		if (cure)
			SetMonData(&party[i], MON_DATA_STATUS, &status);
	}
}

static bool8 Lv200B_TargetBehindSubstitute(u8 bank)
{
	return bank < gBattlersCount && gDisableStructs[bank].substituteHP != 0;
}

static void Lv200B_SetSleep(u8 bank, u8 turns)
{
	if (bank < gBattlersCount && gBattleMons[bank].status1 == 0)
		gBattleMons[bank].status1 = turns & STATUS1_SLEEP;
}

/* 反動で次のターン動けない（set_effect.c の MOVE_EFFECT_RECHARGE と同じ3つ）。 */
static void Lv200B_SetRecharge(u8 bank)
{
	gBattleMons[bank].status2 |= STATUS2_RECHARGE;
	gDisableStructs[bank].rechargeTimer = 2;
	gLockedMoves[bank] = gCurrentMove;
}

/* 眠りのカウンタを turns に（本家の数え方: カウンタ N は N-1 回休んで N 回目で起きる）。 */
static void Lv200B_FixSleepTurns(u8 bank, u8 turns)
{
	if (bank < gBattlersCount && (gBattleMons[bank].status1 & STATUS1_SLEEP))
		gBattleMons[bank].status1 = (gBattleMons[bank].status1 & ~STATUS1_SLEEP) | (turns & STATUS1_SLEEP);
}

/* まぼろしのきおく: 相手の最後の技を借りられるか（ものまねと同じ条件）。 */
static bool8 Lv200B_CanMimic(u8 atk, u8 def)
{
	u16 move = gLastUsedMoves[def];
	u8 i;

	if (atk >= gBattlersCount || def >= gBattlersCount || (gBattleMons[atk].status2 & STATUS2_TRANSFORMED)
	|| move == MOVE_NONE || move == 0xFFFF || CheckTableForMove(move, gMimicBannedMoves)
	|| IsZMove(move) || IsAnyMaxMove(move))
		return FALSE;
	for (i = 0; i < MAX_MON_MOVES; ++i)
		if (gBattleMons[atk].moves[i] == move)
			return FALSE;
	return TRUE;
}

static void Lv200B_SetTerrain(struct Lv200CustomMovesBState* state,
							 u8 terrain, u8 owner, u8 bank, u8 turns)
{
	gTerrainType = terrain;
	gNewBS->TerrainTimer = turns;
	state->terrainOwner = owner;
	state->terrainOwnerSide = SIDE(bank);
}

static void Lv200B_SetWeather(u16 weather, u8 turns)
{
	if (!(gBattleWeather & WEATHER_PRIMAL_ANY))
	{
		gBattleWeather = weather;
		gWishFutureKnock.weatherDuration = turns;
	}
}

static void Lv200B_ClearHazards(u8 side)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	gSideStatuses[side] &= ~SIDE_STATUS_SPIKES;
	gSideTimers[side].spikesAmount = 0;
	gSideTimers[side].tspikesAmount = 0;
	gSideTimers[side].srAmount = 0;
	gSideTimers[side].stickyWeb = 0;
	gSideTimers[side].steelsurge = 0;
	Lv200CustomMovesA_OnHazardsCleared(side); /* A側の設置（すなのわな・いしくれのまく）も消す */
	if (state != NULL && side < NUM_BATTLE_SIDES)
	{
		state->frozenHazardPresent[side] = FALSE;
		state->frozenHazardBank[side] = 0;
	}
}

static void Lv200B_PrepareFixedMultiHit(u8 count)
{
	if (!gNewBS->MultiHitOn)
	{
		gMultiHitCounter = count;
		/* 「N かい あたった！」の文の元。回数を決める所と対で作る（作らないと前の値・ごみが出る・#41） */
		PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0);
	}
}

void Lv200B_PrepareSummitIcicle(void)
{
	Lv200B_PrepareFixedMultiHit(5);
}

void Lv200B_PrepareTwinDragonClaw(void)
{
	Lv200B_PrepareFixedMultiHit(2);
}

void Lv200B_PrepareDragonSoul(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u32 cost;

	gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	gBattleMoveDamage = 0;
	if (state == NULL || gBankAttacker >= gBattlersCount)
		return;
	cost = MathMax(1, gBattleMons[gBankAttacker].maxHP / 4);
	if (gBattleMons[gBankAttacker].hp <= cost)
		return;
	gBattleMoveDamage = cost;
	state->dragonSoulBits |= gBitTable[gBankAttacker];
	gBattleCommunication[MULTISTRING_CHOOSER] = 1;
}

void Lv200B_PrepareDragonCurseCost(void)
{
	gBattleMoveDamage = gBankAttacker < gBattlersCount
		? MathMax(1, gBattleMons[gBankAttacker].maxHP / 3) : 0;
}

/* Q26: 相手がドラゴンなら、使った側が最大HPの1/8を呪い返しで受ける（HP を払ったあとの 2 度目の HP 更新）。
 * MULTISTRING_CHOOSER=1 のときだけ台本が HP バーを動かす。使った側が先に倒れていたら何もしない。 */
void Lv200B_PrepareDragonCurseBackfire(void)
{
	gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	gBattleMoveDamage = 0;
	if (gBankAttacker >= gBattlersCount || gBankTarget >= gBattlersCount
	|| gBattleMons[gBankAttacker].hp == 0 || !IsOfType(gBankTarget, TYPE_DRAGON))
		return;
	gBattleMoveDamage = MathMax(1, gBattleMons[gBankAttacker].maxHP / 8);
	gBattleCommunication[MULTISTRING_CHOOSER] = 1;
}

void Lv200B_PrepareDragonMelody(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && gBankAttacker < gBattlersCount)
		state->selfSureHitTimer[gBankAttacker] = 2;
}

void Lv200B_RecordDragonReversalFlinch(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && gBankTarget < gBattlersCount
	&& (gBattleMons[gBankTarget].status2 & STATUS2_FLINCHED))
		state->flinchedThisRoundBits |= gBitTable[gBankTarget];
}

void Lv200B_PrepareDragonTemptation(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 atk = gBankAttacker;
	u8 def = gBankTarget;
	u16 item;
	bool8 stoleItem = FALSE;
	bool8 suppressedAbility = FALSE;

	gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	if (state == NULL || atk >= gBattlersCount || def >= gBattlersCount
	|| Lv200B_TargetBehindSubstitute(def))
		return;
	if (!(state->abilitySuppressOwnedBits & gBitTable[def])
	&& (ABILITY(def) == ABILITY_NONE
	 || CheckTableForAbility(ABILITY(def), gGastroAcidBannedAbilities)))
		return;

	item = gBattleMons[def].item;
	if (gBattleMons[atk].item == ITEM_NONE && item != ITEM_NONE
	&& ABILITY(def) != ABILITY_STICKYHOLD
	&& CanTransferItem(gBattleMons[def].species, item)
	&& CanTransferItem(gBattleMons[atk].species, item))
	{
		u8 oldDefEffect = ITEM_EFFECT(def);
		gLastUsedItem = item;
		gBattleMons[atk].item = item;
		gBattleMons[def].item = ITEM_NONE;
		HandleUnburdenBoost(atk);
		HandleUnburdenBoost(def);
		if (oldDefEffect == ITEM_EFFECT_ABILITY_SHIELD)
		{
			HandleAbilityShieldGetAndLost(atk);
			HandleAbilityShieldGetAndLost(def);
		}
		RecordItemEffectBattle(atk, ITEM_EFFECT(atk));
		RecordItemEffectBattle(def, ITEM_EFFECT(def));
		gActiveBattler = atk;
		EmitSetMonData(0, REQUEST_HELDITEM_BATTLE, 0, 2, &gBattleMons[atk].item);
		MarkBufferBankForExecution(atk);
		gActiveBattler = def;
		EmitSetMonData(0, REQUEST_HELDITEM_BATTLE, 0, 2, &gBattleMons[def].item);
		MarkBufferBankForExecution(def);
		stoleItem = TRUE;
	}

	if (state->abilitySuppressOwnedBits & gBitTable[def])
	{
		state->timedAbilitySuppress[def] = 3;
		suppressedAbility = TRUE;
	}
	else if (ABILITY(def) != ABILITY_NONE
	&& ITEM_EFFECT(def) != ITEM_EFFECT_ABILITY_SHIELD
	&& !CheckTableForAbility(ABILITY(def), gGastroAcidBannedAbilities))
	{
		gNewBS->SuppressedAbilities[def] = gBattleMons[def].ability;
		gBattleMons[def].ability = ABILITY_NONE;
		gStatuses3[def] |= STATUS3_ABILITY_SUPPRESS;
		state->timedAbilitySuppress[def] = 3;
		state->abilitySuppressOwnedBits |= gBitTable[def];
		ResetVarsForAbilityChange(def);
		gBattleScripting.bank = def;
		gBattleStringLoader = AbilitySuppressedString;
		suppressedAbility = TRUE;
	}

	if (suppressedAbility)
		gBattleCommunication[MULTISTRING_CHOOSER] = stoleItem ? 2 : 1;
}

void Lv200B_PrepareTempest(void)
{
	Lv200B_PrepareFixedMultiHit(2);
}

void Lv200B_PrepareWaterCounter(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 source;
	u32 damage;

	gBattleMoveDamage = 0;
	if (state == NULL || gBankAttacker >= gBattlersCount)
		return;
	source = state->lastDamageSource[gBankAttacker];
	damage = state->lastDamageTaken[gBankAttacker];
	if (damage == 0 || source >= gBattlersCount || SIDE(source) == SIDE(gBankAttacker)
	|| !Lv200B_IsAlive(source))
		return;
	gBankTarget = source;
	gBattleMoveDamage = damage > 0x3FFFFFFF ? 0x7FFFFFFF : damage * 2;
}

void Lv200B_CheckDeepSeaMemory(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();

	gBattleMoveDamage = state != NULL && gBankAttacker < gBattlersCount
						&& state->deepSeaTimer[gBankAttacker] == 0;
}

void Lv200B_PrepareGreatWhirlpool(void)
{
	u8 bank = gBankAttacker;

	if (bank >= gBattlersCount)
		return;
	if (gDisableStructs[bank].rolloutTimerStartValue == 5
	&& gDisableStructs[bank].rolloutTimer == 4)
	{
		gDisableStructs[bank].rolloutTimerStartValue = 3;
		gDisableStructs[bank].rolloutTimer = 2;
	}
}

void Lv200B_PrepareDeepSeaSongHeal(void)
{
	u8 bank = gBankAttacker;

	gBattleMoveDamage = 0;
	if (bank >= gBattlersCount || !Lv200B_IsAlive(bank)
	|| gBattleMons[bank].hp >= gBattleMons[bank].maxHP || IsHealBlocked(bank))
		return;
	gBattleMoveDamage = -1 * MathMax(1, gBattleMons[bank].maxHP / 2);
}

void Lv200B_FixDeepSeaSongSleep(void)
{
	Lv200B_FixSleepTurns(gBankTarget, 3); /* 2ターン休む＝カウンタ3（本家の数え方・Q17） */
}

void Lv200B_FixDreamPrincessSleep(void)
{
	Lv200B_FixSleepTurns(gBankTarget, 3);
}

static bool8 Lv200B_IsPartySlotActive(u8 side, u8 slot)
{
	u8 bank;

	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (SIDE(bank) == side && gBattlerPartyIndexes[bank] == slot)
			return TRUE;
	}
	return FALSE;
}

static u32 Lv200B_SeaGodActiveHealAmount(u8 bank)
{
	return MathMax(1, IsOfType(bank, TYPE_WATER)
					 ? gBattleMons[bank].maxHP / 2 : gBattleMons[bank].maxHP / 3);
}

void Lv200B_CheckSeaGodFeast(void)
{
	struct Pokemon* party;
	u8 side = SIDE(gBankAttacker);
	u8 bank;
	u8 slot;

	gBattleMoveDamage = 0;
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (SIDE(bank) == side && Lv200B_IsAlive(bank)
		&& gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
		{
			gBattleMoveDamage = 1;
			return;
		}
	}
	party = side == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	for (slot = 0; slot < PARTY_SIZE; ++slot)
	{
		u16 hp = GetMonData(&party[slot], MON_DATA_HP, NULL);
		u16 maxHP = GetMonData(&party[slot], MON_DATA_MAX_HP, NULL);
		u16 species = GetMonData(&party[slot], MON_DATA_SPECIES, NULL);

		if (!Lv200B_IsPartySlotActive(side, slot) && species != SPECIES_NONE
		&& hp != 0 && hp < maxHP)
		{
			gBattleMoveDamage = 1;
			return;
		}
	}
}

void Lv200B_HealSeaGodReserves(void)
{
	struct Pokemon* party;
	u8 side = SIDE(gBankAttacker);
	u8 slot;

	party = side == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	for (slot = 0; slot < PARTY_SIZE; ++slot)
	{
		u16 hp;
		u16 maxHP;
		u16 species;
		u32 amount;

		if (Lv200B_IsPartySlotActive(side, slot))
			continue;
		hp = GetMonData(&party[slot], MON_DATA_HP, NULL);
		maxHP = GetMonData(&party[slot], MON_DATA_MAX_HP, NULL);
		species = GetMonData(&party[slot], MON_DATA_SPECIES, NULL);
		if (species == SPECIES_NONE || hp == 0 || hp >= maxHP)
			continue;
		amount = IsMonOfType(&party[slot], TYPE_WATER) ? maxHP / 2 : maxHP / 3;
		hp = MathMin(maxHP, hp + MathMax(1, amount));
		SetMonData(&party[slot], MON_DATA_HP, &hp);
	}
}

void Lv200B_PrepareSeaGodUserHeal(void)
{
	u8 bank = gBankAttacker;

	gBattleMoveDamage = 0;
	if (Lv200B_IsAlive(bank) && gBattleMons[bank].hp < gBattleMons[bank].maxHP
	&& !IsHealBlocked(bank))
		gBattleMoveDamage = -1 * Lv200B_SeaGodActiveHealAmount(bank);
}

void Lv200B_PrepareSeaGodPartnerHeal(void)
{
	u8 bank = PARTNER(gBankAttacker);

	gBankTarget = bank;
	gBattleMoveDamage = 0;
	if (bank < gBattlersCount && SIDE(bank) == SIDE(gBankAttacker)
	&& Lv200B_IsAlive(bank) && gBattleMons[bank].hp < gBattleMons[bank].maxHP
	&& !IsHealBlocked(bank))
		gBattleMoveDamage = -1 * Lv200B_SeaGodActiveHealAmount(bank);
}

void Lv200B_PrepareHeartGuard(void)
{
	if (gBankAttacker < gBattlersCount
	&& gLastResultingMoves[gBankAttacker] == MOVE_LV200_FAIRY_120)
		gLastResultingMoves[gBankAttacker] = MOVE_PROTECT;
}

void Lv200B_PrepareRootDrain(void)
{
	u32 amount;

	gBattleMoveDamage = 0;
	gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	if (!Lv200B_IsAlive(gBankAttacker) || gHpDealt == 0 || IsHealBlocked(gBankAttacker))
		return;
	/* 満タンでもヘドロえきのダメージは出す（上流のドレインと同じ）。回復の文だけ省く */
	if (gBattleMons[gBankAttacker].hp == gBattleMons[gBankAttacker].maxHP)
		gBattleCommunication[MULTISTRING_CHOOSER] = 1;

	if (IsOfType(gBankTarget, TYPE_GRASS) || IsOfType(gBankTarget, TYPE_WATER) || IsOfType(gBankTarget, TYPE_GROUND))
		amount = gHpDealt / 2;
	else
		amount = (gHpDealt * 3) / 4;
	if (ITEM_EFFECT(gBankAttacker) == ITEM_EFFECT_BIG_ROOT)
		amount = (amount * 13) / 10;
	gBattleMoveDamage = -(s32) MathMax(1, amount);
}

/* フォトシンセの事前判定（PP を引く前）: 回復も治療も起きないなら失敗。 */
void Lv200B_CheckPhotosynthesis(void)
{
	u32 statusMask = STATUS1_BURN | STATUS1_POISON | STATUS1_TOXIC_POISON | STATUS1_PARALYSIS;
	u8 bank = gBankAttacker;

	gBattleMoveDamage = bank < gBattlersCount
		&& ((gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
		 || (gBattleMons[bank].status1 & statusMask) != 0);
}

void Lv200B_PreparePhotosynthesis(void)
{
	u32 statusMask = STATUS1_BURN | STATUS1_POISON | STATUS1_TOXIC_POISON | STATUS1_PARALYSIS;
	u32 amount;

	gBattleCommunication[MULTISTRING_CHOOSER] = (gBattleMons[gBankAttacker].status1 & statusMask) != 0;
	if (gBattleCommunication[MULTISTRING_CHOOSER])
	{
		gBattleMons[gBankAttacker].status1 &= ~(statusMask | STATUS_TOXIC_COUNTER);
		Lv200CustomMove_OnStatusCured(gBankAttacker);
		gActiveBattler = gBankAttacker; /* 手持ちにも治療を送る */
		EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gBankAttacker].status1);
		MarkBufferBankForExecution(gActiveBattler);
	}
	gBattleMoveDamage = 0;
	if (!Lv200B_IsAlive(gBankAttacker)
	 || gBattleMons[gBankAttacker].hp == gBattleMons[gBankAttacker].maxHP
	 || IsHealBlocked(gBankAttacker))
		return;

	amount = (WEATHER_HAS_EFFECT && (gBattleWeather & WEATHER_SUN_ANY))
		? gBattleMons[gBankAttacker].maxHP
		: (gBattleMons[gBankAttacker].maxHP * 3) / 4;
	gBattleMoveDamage = -(s32) MathMax(1, amount);
}

void Lv200B_PrepareForestLight(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();

	if (state == NULL)
		return;
	Lv200B_SetTerrain(state, GRASSY_TERRAIN,
		LV200_B_TERRAIN_OWNER_FOREST_LIGHT, gBankAttacker, 8);
	gBattleScripting.animArg1 = B_ANIM_GRASSY_SURGE;
	gBattleStringLoader = GrassyTerrainSetString;
}

void Lv200B_PrepareFairyRing(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();

	if (state == NULL)
		return;
	Lv200B_SetTerrain(state, MISTY_TERRAIN, LV200_B_TERRAIN_OWNER_FAIRY_RING, gBankAttacker, 8);
	gBattleScripting.animArg1 = B_ANIM_MISTY_SURGE;
	gBattleStringLoader = MistyTerrainSetString;
}

void Lv200B_PrepareUprootAttack(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 stat;
	bool8 erased = FALSE;

	gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	if (state == NULL || gBankTarget >= gBattlersCount)
		return;

	if (gTerrainType == GRASSY_TERRAIN)
	{
		gTerrainType = 0;
		gNewBS->TerrainTimer = 0;
		state->terrainOwner = LV200_B_TERRAIN_OWNER_NONE;
		gBattleScripting.animArg1 = B_ANIM_LOAD_DEFAULT_BG;
		gBattleStringLoader = TerrainEndString;
		gBattleCommunication[MULTISTRING_CHOOSER] = 1;
	}
	for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
	{
		if (gBattleMons[gBankTarget].statStages[stat - 1] > 6)
		{
			gBattleMons[gBankTarget].statStages[stat - 1] = 6;
			erased = TRUE;
		}
	}
	if (erased)
		Lv200Common_QueueMessage(gBankTarget, gText_Lv200B_UprootBuffsGone);
}

static void Lv200B_PrepareElectricTerrain(u8 owner)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();

	if (state == NULL)
		return;
	Lv200B_SetTerrain(state, ELECTRIC_TERRAIN, owner, gBankAttacker, 5);
	gBattleScripting.animArg1 = B_ANIM_ELECTRIC_SURGE;
	gBattleStringLoader = ElectricTerrainSetString;
}

void Lv200B_PrepareThunderousEcho(void)
{
	Lv200B_PrepareElectricTerrain(LV200_B_TERRAIN_OWNER_THUNDEROUS_ECHO);
}

void Lv200B_PrepareTempestTerrain(void)
{
	Lv200B_PrepareElectricTerrain(LV200_B_TERRAIN_OWNER_NONE);
}

void Lv200B_PrepareBlackHole(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 bank;

	if (state == NULL)
		return;
	gNewBS->targetsToBringDown = 0;
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (Lv200B_IsAlive(bank) && !CheckGrounding(bank))
			gNewBS->targetsToBringDown |= gBitTable[bank];
	}
	gNewBS->GravityTimer = 5;
	state->gravityOwner = gBankAttacker + 1;
	gBattleStringLoader = GravitySetString;
}

void Lv200B_PrepareEdgeOfMindHeal(void)
{
	gBattleMoveDamage = 0;
	if (Lv200B_IsAlive(gBankAttacker)
	&& gBattleMons[gBankAttacker].hp < gBattleMons[gBankAttacker].maxHP
	&& !IsHealBlocked(gBankAttacker))
		gBattleMoveDamage = -1 * MathMax(1, gBattleMons[gBankAttacker].maxHP / 2);
}

void Lv200B_ApplyEdgeOfMindSleep(void)
{
	/* ひとしれず1ターン眠る＝1回休み＝カウンタ2。眠れない状況（ふみん・さわぎ・ミスト等）では眠らない。 */
	if (Lv200B_IsAlive(gBankAttacker) && CanBePutToSleep(gBankAttacker, gBankAttacker, FALSE))
		gBattleMons[gBankAttacker].status1 = 2;
}

static bool8 Lv200B_PrepareSnow(u8 turns, bool8 owned)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL)
		return FALSE;
	if (gBattleWeather & WEATHER_PRIMAL_ANY)
	{
		gBattleCommunication[MULTISTRING_CHOOSER] = 2;
		return FALSE;
	}
	gBattleWeather = WEATHER_HAIL_TEMPORARY;
	gWishFutureKnock.weatherDuration = turns;
	gBattleCommunication[MULTISTRING_CHOOSER] = 5;
	gBattleScripting.animArg1 = B_ANIM_HAIL_CONTINUES;
	gNewBS->hailStart = TRUE;
	state->snowOwner = owned ? gBankAttacker + 1 : 0;
	return TRUE;
}

void Lv200B_PrepareBlizzardArmor(void)
{
	Lv200B_PrepareSnow(5, FALSE);
}

void Lv200B_PrepareSeaOfIce(void)
{
	if (!Lv200B_PrepareSnow(8, TRUE))
		gMoveResultFlags |= MOVE_RESULT_FAILED;
}

void Lv200B_PrepareQuarterRecoil(void)
{
	gBattleMoveDamage = 0;
	if (!Lv200B_IsAlive(gBankAttacker) || gHpDealt == 0
	 || BankHasAbility(gBankAttacker, ABILITY_ROCKHEAD)
	 || BankHasAbility(gBankAttacker, ABILITY_MAGICGUARD))
		return;
	gBattleMoveDamage = MathMax(1, gHpDealt / 4);
}

void Lv200B_PrepareThirdRecoil(void)
{
	gBattleMoveDamage = 0;
	if (!Lv200B_IsAlive(gBankAttacker) || gHpDealt == 0
	 || BankHasAbility(gBankAttacker, ABILITY_ROCKHEAD)
	 || BankHasAbility(gBankAttacker, ABILITY_MAGICGUARD))
		return;
	gBattleMoveDamage = MathMax(1, gHpDealt / 3);
}

/* ゆうだちのもり: 雨か晴れ（天気が有効）ならため無し。MULTISTRING_CHOOSER=1 でため省略。 */
void Lv200B_CheckShowerForestWeather(void)
{
	gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	if (WEATHER_HAS_EFFECT && (gBattleWeather & (WEATHER_SUN_ANY | WEATHER_RAIN_ANY)))
		gBattleCommunication[MULTISTRING_CHOOSER] = 1;
}

void Lv200B_RollParalysis40(void)
{
	gBattleCommunication[MULTISTRING_CHOOSER] = Random() < 26214;
}

static u32 Lv200B_TypeMultiplier(u8 moveType, u8 def)
{
	u8 types[3];
	u8 i;
	u32 value = TYPE_MUL_NORMAL;

	types[0] = gBattleMons[def].type1;
	types[1] = gBattleMons[def].type2;
	types[2] = gBattleMons[def].type3;
	for (i = 0; i < 3; ++i)
	{
		u16 part;
		if (IS_BLANK_TYPE(types[i])
		|| (i != 0 && (types[i] == types[0] || (i == 2 && types[i] == types[1]))))
			continue;
		part = gTypeEffectiveness[moveType][types[i]];
		if (part == TYPE_MUL_NO_DATA)
			part = TYPE_MUL_NORMAL;
		value = (value * part) / TYPE_MUL_NORMAL;
	}
	return value;
}

/* md 詳細: 4つの技のうち攻撃技のタイプそれぞれの相性のうち、最も高い倍率を採用して威力に掛ける（最大4倍・変化技は数えない）。 */
static u32 Lv200B_PsychicUnisonMultiplier(u8 atk, u8 def)
{
	u8 slot;
	u32 best = 0;

	for (slot = 0; slot < MAX_MON_MOVES; ++slot)
	{
		u16 move = gBattleMons[atk].moves[slot];
		u32 part;
		if (move == MOVE_NONE || SPLIT(move) == SPLIT_STATUS)
			continue;
		part = Lv200B_TypeMultiplier(gBattleMoves[move].type, def);
		if (part > best)
			best = part;
	}
	if (best == 0)
		return TYPE_MUL_NORMAL;
	return best > 4 * TYPE_MUL_NORMAL ? 4 * TYPE_MUL_NORMAL : best;
}

static void Lv200B_ApplyCompletion(u16 move)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 atk = gBankAttacker;
	u8 def = gBankTarget;
	u8 stat;

	if (state == NULL || atk >= gBattlersCount || def >= gBattlersCount)
		return;

	switch (move)
	{
	case MOVE_LV200_WATER_110:
		state->tideTimer[SIDE(atk)] = 5;
		break;
	case MOVE_LV200_WATER_130:
		if (state->deepSeaTimer[atk] == 0)
		{
			state->deepSeaTimer[atk] = 3;
			Lv200Common_QueueMessage(atk, gText_Lv200B_DeepSeaSet);
		}
		break;
	case MOVE_LV200_WATER_140:
		Lv200CustomMovesB_AddPendingPositionEffect(state, GetBattlerPosition(atk), LV200_B_PENDING_SPEED_UP);
		break;
	case MOVE_LV200_WATER_180:
		for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
		{
			gBattleMons[atk].statStages[stat - 1] = gBattleMons[def].statStages[stat - 1];
			gBattleMons[def].statStages[stat - 1] = 6;
		}
		Lv200Common_QueueMessage(atk, gText_Lv200B_WhirlpoolMirror);
		break;
	case MOVE_LV200_WATER_200:
		Lv200B_ClearHazards(B_SIDE_PLAYER);
		Lv200B_ClearHazards(B_SIDE_OPPONENT);
		Lv200B_SetWeather(WEATHER_RAIN_TEMPORARY, 5);
		break;
	case MOVE_LV200_GRASS_120:
		if (gBattleCommunication[MULTISTRING_CHOOSER] == 0
		 && !Lv200B_TargetBehindSubstitute(def)
		 && (gStatuses3[def] & STATUS3_LEECHSEED)
		 && (gStatuses3[def] & STATUS3_LEECHSEED_BATTLER) == atk)
		{
			gBattleMons[def].status2 |= STATUS2_ESCAPE_PREVENTION;
			state->strongLeechBits |= gBitTable[def];
		}
		break;
	case MOVE_LV200_GRASS_140:
		if (gDisableStructs[atk].substituteHP != 0)
			state->substituteEffect[atk] = LV200_B_SUBSTITUTE_EFFECT_REGEN;
		break;
	case MOVE_LV200_GRASS_160:
		gStatuses3[atk] |= STATUS3_ROOTED;
		state->armoredRootBits |= gBitTable[atk];
		break;
	case MOVE_LV200_ELECTRIC_160:
		Lv200CustomMovesB_AddPendingPositionEffect(state, GetBattlerPosition(atk), LV200_B_PENDING_CHARGE);
		break;
	case MOVE_LV200_ICE_200:
		/* 凍らせた直後に3ターン固定（凍結を付ける seteffectwithchancetarget の後ろで呼ばれる） */
		if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && (gBattleMons[def].status1 & STATUS1_FREEZE))
			state->fixedFreezeTimer[def] = 3;
		break;
	case MOVE_LV200_ELECTRIC_180:
		if (!Lv200B_TargetBehindSubstitute(def))
			state->microwaveTimer[def] = 3;
		break;
	case MOVE_LV200_ELECTRIC_200:
		/* 最後のヒットの後だけエレキフィールド（台本は MULTISTRING_CHOOSER が 0 なら場張りを飛ばす） */
		gBattleCommunication[MULTISTRING_CHOOSER] = 0;
		if (gMultiHitCounter <= 1 || gBattleMons[def].hp == 0)
		{
			Lv200B_PrepareTempestTerrain();
			gBattleCommunication[MULTISTRING_CHOOSER] = 1;
		}
		break;
	case MOVE_LV200_PSYCHIC_110:
		state->mirrorTimer[atk] = 3;
		break;
	case MOVE_LV200_PSYCHIC_130:
	{
		/* md: 片方が既にあれば足りない方だけ張る。両方この技で張った時だけ「2枚」の印（シングルでも2/3） */
		u8 side = SIDE(atk);
		bool8 hadReflect = (gSideStatuses[side] & SIDE_STATUS_REFLECT) != 0;
		bool8 hadScreen = (gSideStatuses[side] & SIDE_STATUS_LIGHTSCREEN) != 0;
		if (!hadReflect)
		{
			gSideStatuses[side] |= SIDE_STATUS_REFLECT;
			gSideTimers[side].reflectTimer = 5;
			gSideTimers[side].reflectBank = atk;
		}
		if (!hadScreen)
		{
			gSideStatuses[side] |= SIDE_STATUS_LIGHTSCREEN;
			gSideTimers[side].lightscreenTimer = 5;
			gSideTimers[side].lightscreenBank = atk;
		}
		if (!hadReflect && !hadScreen)
			state->psychicBarrierBits |= gBitTable[side];
		break;
	}
	case MOVE_LV200_PSYCHIC_160:
		gBattleMons[atk].attack = gBattleMons[def].attack = (gBattleMons[atk].attack + gBattleMons[def].attack) / 2;
		gBattleMons[atk].defense = gBattleMons[def].defense = (gBattleMons[atk].defense + gBattleMons[def].defense) / 2;
		gBattleMons[atk].spAttack = gBattleMons[def].spAttack = (gBattleMons[atk].spAttack + gBattleMons[def].spAttack) / 2;
		gBattleMons[atk].spDefense = gBattleMons[def].spDefense = (gBattleMons[atk].spDefense + gBattleMons[def].spDefense) / 2;
		break;
	case MOVE_LV200_PSYCHIC_190:
		if (!Lv200B_TargetBehindSubstitute(def)
		&& !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
		&& (gBattleMons[def].status1 & STATUS1_SLEEP))
		{
			state->suspiciousSleepBits |= gBitTable[def]; /* 眠りの長さは固定しない（上流の眠りのまま） */
		}
		break;
	case MOVE_LV200_ICE_140:
		/* md 詳細: 最大2枚（frozenHazardPresent は枚数）。出てきた相手の素早さは枚数ぶん下がる */
		if (state->frozenHazardPresent[SIDE(def)] < 2)
		{
			++state->frozenHazardPresent[SIDE(def)];
			state->frozenHazardBank[SIDE(def)] = atk;
			Lv200Common_QueueMessageFrom(def, atk, gText_Lv200B_FrozenHazardSet);
		}
		else
			gMoveResultFlags |= MOVE_RESULT_FAILED;
		break;
	case MOVE_LV200_ICE_180:
		if (!Lv200B_TargetBehindSubstitute(def))
			state->coldGazeTimer[def] = 3;
		break;
	case MOVE_LV200_ICE_190:
		state->iceAgeTimer = 5;
		Lv200Common_QueueMessage(atk, gText_Lv200B_IceAgeSet);
		break;
	case MOVE_LV200_DRAGON_140:
		state->dragonPalaceTimer = 5;
		state->dragonPalaceSide = SIDE(atk);
		Lv200Common_QueueMessage(atk, gText_Lv200B_DragonPalaceSet);
		break;
	case MOVE_LV200_DRAGON_160:
		if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
			Lv200B_SetRecharge(atk);
		break;
	case MOVE_LV200_DRAGON_200:
		gBattleCommunication[MULTISTRING_CHOOSER] = 0;
		if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
			Lv200B_SetRecharge(atk);
		if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)
		&& !Lv200B_TargetBehindSubstitute(def))
		{
			for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
				gBattleMons[def].statStages[stat - 1] = 6;
			gBattleCommunication[MULTISTRING_CHOOSER] = 1;
		}
		break;
	case MOVE_LV200_DARK_120:
	{
		u8 slot;
		u8 strongestSlot = 0;
		bool8 found = FALSE;
		for (slot = 0; slot < MAX_MON_MOVES; ++slot)
		{
			if (gBattleMons[def].moves[slot] == MOVE_NONE)
				continue;
			if (!found || gBattleMoves[gBattleMons[def].moves[slot]].power
			> gBattleMoves[gBattleMons[def].moves[strongestSlot]].power)
			{
				strongestSlot = slot;
				found = TRUE;
			}
		}
		if (found)
		{
			gDisableStructs[def].disabledMove = gBattleMons[def].moves[strongestSlot];
			gDisableStructs[def].disableTimer1 = 3;
		}
		else
			gMoveResultFlags |= MOVE_RESULT_FAILED;
		break;
	}
	case MOVE_LV200_DARK_140:
		state->customFieldKind = LV200_B_FIELD_DARKNESS;
		state->customFieldTimer = 5;
		Lv200Common_QueueMessage(atk, gText_Lv200B_DarknessSet);
		break;
	case MOVE_LV200_DARK_160:
		/* 混乱は共通部品の QueueInflict（素の追加効果の流れ・素の文）。能力は各1段階（md 詳細・いばる＋おだてる）を共通部品で */
		if (CanBeConfused(def, atk, TRUE))
		{
			Lv200Common_QueueInflict(def, atk, MOVE_EFFECT_CONFUSION, FALSE);
			Lv200Common_QueueStat(def, atk, STAT_STAGE_ATK, 1, FALSE);
			Lv200Common_QueueStat(def, atk, STAT_STAGE_SPATK, 1, FALSE);
		}
		else
			gMoveResultFlags |= MOVE_RESULT_FAILED;
		break;
	case MOVE_LV200_DARK_180:
		state->abilityItemNullTimer = 5;
		Lv200Common_QueueMessage(atk, gText_Lv200B_AbilityItemNullSet);
		break;
	case MOVE_LV200_DARK_190:
		Lv200Common_QueueStat(atk, atk, STAT_STAGE_ATK, 2, FALSE);
		Lv200Common_QueueStat(atk, atk, STAT_STAGE_SPATK, 2, FALSE);
		if (!Lv200B_TargetBehindSubstitute(def))
			state->anxietyTimer[def] = 3;
		break;
	case MOVE_LV200_DARK_200:
		if (!Lv200B_TargetBehindSubstitute(def))
		{
			if (state->abilitySuppressOwnedBits & gBitTable[def])
				state->timedAbilitySuppress[def] = 0;
			else if (ABILITY(def) != ABILITY_NONE
			&& ITEM_EFFECT(def) != ITEM_EFFECT_ABILITY_SHIELD
			&& !CheckTableForAbility(ABILITY(def), gGastroAcidBannedAbilities))
			{
				gNewBS->SuppressedAbilities[def] = gBattleMons[def].ability;
				gBattleMons[def].ability = ABILITY_NONE;
				gStatuses3[def] |= STATUS3_ABILITY_SUPPRESS;
				state->abilitySuppressOwnedBits |= gBitTable[def];
				ResetVarsForAbilityChange(def);
			}
			state->darknessHoleItemSuppressedBits |= gBitTable[def];
		}
		state->customFieldKind = LV200_B_FIELD_NONE;
		state->customFieldTimer = 0;
		gTerrainType = 0;
		gNewBS->TerrainTimer = 0;
		state->terrainOwner = LV200_B_TERRAIN_OWNER_NONE;
		/* md 詳細: 壁・設置技も全部消す（はたきおとす＋いえき＋かわらわり＋こうそくスピン） */
		for (stat = 0; stat < NUM_BATTLE_SIDES; ++stat)
		{
			gSideStatuses[stat] &= ~(SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN);
			gSideTimers[stat].reflectTimer = 0;
			gSideTimers[stat].lightscreenTimer = 0;
			gNewBS->AuroraVeilTimers[stat] = 0;
			Lv200CustomMovesB_OnScreensRemoved(stat);
			Lv200B_ClearHazards(stat);
		}
		break;
	case MOVE_LV200_FAIRY_120:
		if (gProtectStructs[atk].protected)
			state->heartGuardBits |= gBitTable[atk];
		break;
	case MOVE_LV200_FAIRY_130:
		Lv200B_HealParty(atk, 4, FALSE, TRUE);
		break;
	case MOVE_LV200_FAIRY_150:
		break; /* ミストフィールドの設置は台本（Lv200B_PrepareFairyRing）で素の流れ */
	case MOVE_LV200_FAIRY_160:
		/* みがわり本体は台本の setsubstituteeffect（スプライト・behindSubstitute・HPバー）。ここは壊れた時の印だけ */
		if (gBattleCommunication[MULTISTRING_CHOOSER] != 1)
			state->substituteEffect[atk] = LV200_B_SUBSTITUTE_EFFECT_ATK_DROP;
		break;
	case MOVE_LV200_FAIRY_170:
		if (gBattleMons[def].status1 & STATUS1_SLEEP)
		{
			Lv200B_FixSleepTurns(def, 3); /* 本家の数え方で2ターン（Q17） */
			state->dreamPrincessBits |= gBitTable[def];
		}
		else
			gMoveResultFlags |= MOVE_RESULT_FAILED;
		break;
	case MOVE_LV200_FAIRY_180:
		Lv200CustomMovesB_AddPendingPositionEffect(state, GetBattlerPosition(atk), LV200_B_PENDING_OMNIBOOST);
		gNewBS->HealingWishLoc |= gBitTable[GetBattlerPosition(atk)]; /* 上流のいやしのねがいと同じビット（ダブルの右側も合う） */
		break;
	case MOVE_LV200_FAIRY_190:
		state->abilityItemNullTimer = 5;
		Lv200Common_QueueMessage(atk, gText_Lv200B_AbilityItemNullSet);
		gNewBS->FairyLockTimer = 2; /* 逃げ止めは既存フェアリーロック準拠の2ターン（Q26） */
		break;
	case MOVE_LV200_RIL_120:
		if (Lv200B_CanMimic(atk, def))
		{
			u8 slot = state->mimicTimer[atk] == 0 ? gCurrMovePos : state->mimicSlot[atk];
			if (state->mimicTimer[atk] == 0)
			{
				state->mimicSavedMove[atk] = gBattleMons[atk].moves[slot];
				state->mimicSavedPp[atk] = gBattleMons[atk].pp[slot];
				state->mimicSlot[atk] = slot;
			}
			gBattleMons[atk].moves[slot] = gLastUsedMoves[def];
			gBattleMons[atk].pp[slot] = gBattleMoves[gLastUsedMoves[def]].pp;
			gDisableStructs[atk].mimickedMoves |= gBitTable[slot]; /* PP を手持ちへ書き戻さない印（ものまねと同じ） */
			state->mimicTimer[atk] = 3;
		}
		else
			gMoveResultFlags |= MOVE_RESULT_FAILED;
		break;
	case MOVE_LV200_RIL_130:
		if (!Lv200B_TargetBehindSubstitute(def))
		{
			if (!IsOfType(def, TYPE_GHOST)) /* ゴーストは逃げ止めが効かない（上流のクモのすと同じ） */
			{
				gBattleMons[def].status2 |= STATUS2_ESCAPE_PREVENTION;
				state->timedTrapTimer[def] = 3;
				state->timedTrapOwner[def] = atk + 1;
			}
			state->statRiseBlockedBits |= gBitTable[def];
			state->statRiseBlockedTimer[def] = 3; /* md 詳細: 3ターン */
			Lv200Common_QueueMessageFrom(def, atk, gText_Lv200B_RilGazeSet);
		}
		break;
	case MOVE_LV200_RIL_140:
		if (ViableMonCountFromBank(atk) <= 1)
		{
			Lv200B_HealBank(atk, gBattleMons[atk].maxHP);
			Lv200Common_QueueCureStatus(atk);
		}
		else
			Lv200B_HealBank(atk, gBattleMons[atk].maxHP / 4);
		break;
	case MOVE_LV200_RIL_150:
		state->tearMoonArmedBits |= gBitTable[atk];
		state->tearMoonBoostBits &= ~gBitTable[atk];
		break;
	case MOVE_LV200_RIL_180:
		state->destinySuccessorBits |= gBitTable[atk];
		Lv200Common_QueueMessage(atk, gText_Lv200B_KeepsakeSet);
		break;
	case MOVE_LV200_RIL_190:
	{
		u8 both[2] = { atk, def };
		u8 i;
		for (i = 0; i < 2; ++i) /* 既にほろび中の個体は上書きしない */
		{
			if (gStatuses3[both[i]] & STATUS3_PERISH_SONG)
				continue;
			gStatuses3[both[i]] |= STATUS3_PERISH_SONG;
			gDisableStructs[both[i]].perishSongTimer = 2;
			gDisableStructs[both[i]].perishSongTimerStartValue = 2;
			state->perishOwnerBits |= gBitTable[both[i]];
		}
		break;
	}
	case MOVE_LV200_RIL_200:
		for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
		{
			u8 bank;
			u8 total = 0;
			for (bank = 0; bank < gBattlersCount; ++bank)
			{
				if (gBattleMons[bank].statStages[stat - 1] > 6)
				{
					total += gBattleMons[bank].statStages[stat - 1] - 6;
					gBattleMons[bank].statStages[stat - 1] = 6;
				}
			}
			Lv200B_ChangeStage(atk, stat, total);
		}
		state->customFieldKind = LV200_B_FIELD_NIGHT_SEA;
		state->customFieldTimer = 5;
		Lv200Common_QueueMessage(atk, gText_Lv200B_NightSeaSet);
		break;
	default:
		break;
	}
}

/* 変化技の事前の失敗判定（A の Lv200A_CheckStatusMove と同じ位置・同じ約束）。
 * 技の PP を引き・アニメを見せる前に、効果が起きない条件を見て gBattleMoveDamage に 1(使える)／0(失敗) を返す。
 * 条件は Lv200B_ApplyCompletion の MOVE_RESULT_FAILED と同じ（そちらは安全網として残す）。 */
void Lv200B_CheckStatusMove(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 atk = gBankAttacker;
	u8 def = gBankTarget;
	bool8 usable = TRUE;
	u8 slot;

	Lv200Common_Clear(); /* 前の技の積み残しを持ち越さない */
	if (state == NULL || atk >= gBattlersCount || def >= gBattlersCount)
	{
		gBattleMoveDamage = 0;
		return;
	}
	switch (gCurrentMove)
	{
	case MOVE_LV200_ICE_140:
		usable = state->frozenHazardPresent[SIDE(def)] < 2;
		break;
	case MOVE_LV200_PSYCHIC_140: /* 満タンで状態異常なしは失敗（上流のねむると同じ・D-12） */
		usable = gBattleMons[atk].hp < gBattleMons[atk].maxHP || gBattleMons[atk].status1 != 0;
		break;
	case MOVE_LV200_RIL_190:
		usable = !((gStatuses3[atk] & STATUS3_PERISH_SONG) && (gStatuses3[def] & STATUS3_PERISH_SONG));
		break;
	case MOVE_LV200_DARK_120:
		usable = FALSE;
		for (slot = 0; slot < MAX_MON_MOVES; ++slot)
			if (gBattleMons[def].moves[slot] != MOVE_NONE)
				usable = TRUE;
		break;
	case MOVE_LV200_DARK_160:
		usable = !Lv200B_TargetBehindSubstitute(def) && CanBeConfused(def, atk, TRUE);
		break;
	case MOVE_LV200_FAIRY_170:
		usable = !Lv200B_TargetBehindSubstitute(def) && CanBePutToSleep(def, atk, TRUE);
		break;
	case MOVE_LV200_RIL_120:
		usable = Lv200B_CanMimic(atk, def);
		break;
	default:
		break;
	}
	gBattleMoveDamage = usable;
}

#define DEFINE_LV200_B_HANDLER(move, name) \
	void Lv200B_Handle_##name(void) { Lv200B_ApplyCompletion(move); }
LV200_B_MOVE_LIST(DEFINE_LV200_B_HANDLER)

u16 Lv200CustomMovesB_ModifyPower(u16 move, u8 atk, u8 def, u16 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 type;
	u32 average;

	if (state == NULL || atk >= gBattlersCount || def >= gBattlersCount)
		return value;
	type = Lv200B_PowerMoveType(move, atk);

	/* 場・状態の補正は技のタイプで全部の技に効く（Q14・新技90件だけにしない）。個別の技の補正はこの下（IsBMove の門の後ろ）。 */
	if (type == TYPE_WATER && state->tideTimer[SIDE(atk)] != 0)
		value = Lv200B_Scale16(value, 13, 10);
	if (type == TYPE_FIRE && state->tideTimer[SIDE(def)] != 0)
		value = Lv200B_Scale16(value, 4, 5);
	if (state->deepSeaTimer[atk] != 0 && type == TYPE_WATER)
		value = Lv200B_Scale16(value, 3, 2);
	if (state->customFieldKind == LV200_B_FIELD_DARKNESS && type == TYPE_DARK)
		value = Lv200B_Scale16(value, 3, 2);
	if (state->customFieldKind == LV200_B_FIELD_NIGHT_SEA
	&& (type == TYPE_RIL || type == TYPE_GHOST || type == TYPE_DARK))
		value = Lv200B_Scale16(value, 6, 5);
	if ((state->dragonSoulBits & gBitTable[atk]) && type == TYPE_DRAGON)
		value = Lv200B_Scale16(value, 6, 5);
	if (SPLIT(move) != SPLIT_STATUS)
	{
		if (type == TYPE_ELECTRIC && state->chargeStacks[atk] != 0)
			value = Lv200B_Scale16(value, 10 + state->chargeStacks[atk] * 2, 10);
		if (state->tearMoonBoostBits & gBitTable[atk])
			value = Lv200B_Scale16(value, 2, 1);
		if (state->gravityOwner != 0 && gNewBS->GravityTimer != 0
		&& (IsOfType(def, TYPE_FLYING) || ABILITY(def) == ABILITY_LEVITATE))
			value = Lv200B_Scale16(value, 2, 1);
	}
	if (!Lv200B_IsBMove(move))
		return value;

	switch (move)
	{
	case MOVE_LV200_PSYCHIC_120:
		value = Lv200B_Scale16(value, 6, 5);
		break;
	case MOVE_LV200_PSYCHIC_200:
		value = Lv200B_Scale16(value, Lv200B_PsychicUnisonMultiplier(atk, def), TYPE_MUL_NORMAL);
		break;
	case MOVE_LV200_ICE_170:
		if (gNewBS->MultiHitOn && gMultiHitCounter < 5)
		{
			u8 hit;
			for (hit = gMultiHitCounter; hit < 5; ++hit)
				value = Lv200B_Scale16(value, 11, 10);
		}
		break;
	case MOVE_LV200_WATER_150:
		if (gDisableStructs[atk].rolloutTimerStartValue == 3
		&& gDisableStructs[atk].rolloutTimer == 1)
			value = Lv200B_Scale16(value, 3, 2);
		else if (gDisableStructs[atk].rolloutTimerStartValue == 3
		&& gDisableStructs[atk].rolloutTimer == 0)
			value = Lv200B_Scale16(value, 2, 1);
		break;
	case MOVE_LV200_GRASS_200:
		if (gBattleMons[def].hp * 5 >= gBattleMons[def].maxHP * 4)
			value = Lv200B_Scale16(value, 3, 2);
		else if (gBattleMons[def].hp * 2 <= gBattleMons[def].maxHP)
			value = Lv200B_Scale16(value, 4, 5);
		break;
	case MOVE_LV200_ELECTRIC_140:
		if (IsOfType(def, TYPE_STEEL) || IsOfType(def, TYPE_ELECTRIC) || IsOfType(def, TYPE_GROUND))
			value = Lv200B_Scale16(value, 3, 2);
		break;
	case MOVE_LV200_ELECTRIC_170:
		if (IsOfType(def, TYPE_FLYING))
			value = Lv200B_Scale16(value, 3, 2);
		break;
	case MOVE_LV200_ELECTRIC_200:
		if (gNewBS->MultiHitOn && gMultiHitCounter < 2) /* 2発目は威力半減（Q16） */
			value = Lv200B_Scale16(value, 1, 2);
		break;
	case MOVE_LV200_ELECTRIC_190:
		if (gBattleMons[atk].speed > gBattleMons[def].speed && gBattleMons[def].speed != 0)
		{
			u32 scaled = (u32) value * gBattleMons[atk].speed / gBattleMons[def].speed;
			value = scaled > value * 2 ? value * 2 : scaled;
		}
		break;
	case MOVE_LV200_PSYCHIC_170:
		if (gTerrainType == PSYCHIC_TERRAIN && CheckGrounding(atk))
			value = Lv200B_Scale16(value, 3, 2);
		break;
	case MOVE_LV200_ICE_160:
		if (state->previousRoundDamage[atk] * 4 >= gBattleMons[atk].maxHP)
			value = Lv200B_Scale16(value, 2, 1);
		break;
	case MOVE_LV200_DRAGON_120:
		if (state->flinchedPreviousRoundBits & gBitTable[def])
			value = Lv200B_Scale16(value, 2, 1);
		break;
	case MOVE_LV200_DRAGON_130:
		value = Lv200B_Scale16(value, gBattleMons[atk].maxHP + gBattleMons[atk].hp, gBattleMons[atk].maxHP);
		break;
	case MOVE_LV200_DARK_110:
		if (gBattleMons[atk].item != ITEM_NONE)
			value = Lv200B_Scale16(value, 13, 10);
		break;
	case MOVE_LV200_DARK_130:
		if (gBattleMons[def].hp * 2 <= gBattleMons[def].maxHP)
			value = Lv200B_Scale16(value, 6, 5);
		break;
	case MOVE_LV200_DARK_150:
		average = (gBattleMons[def].defense + gBattleMons[def].spDefense) / 2;
		if (average >= 400)
			value = Lv200B_Scale16(value, 3, 2);
		else if (average > 200)
			value = Lv200B_Scale16(value, 400 + average - 200, 400);
		break;
	case MOVE_LV200_FAIRY_110:
		if (Lv200B_OppositeGender(atk, def))
			value = Lv200B_Scale16(value, 13, 10);
		break;
	case MOVE_LV200_FAIRY_140:
		if (gBattleMons[def].status2 & STATUS2_INFATUATION)
			value = Lv200B_Scale16(value, 3, 2);
		break;
	case MOVE_LV200_FAIRY_200:
		if (IsOfType(def, TYPE_POISON) || IsOfType(def, TYPE_STEEL) || IsOfType(def, TYPE_FIRE))
			value = Lv200B_Scale16(value, 3, 2);
		break;
	case MOVE_LV200_RIL_160:
		value = Lv200B_Scale16(value, ViableMonCountFromBank(def) <= 1 ? 2 : 6, ViableMonCountFromBank(def) <= 1 ? 1 : 5);
		break;
	default:
		break;
	}
	return value;
}

u32 Lv200CustomMovesB_ModifyAccuracy(u16 move, u8 atk, u8 def, u32 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) def;
	(void) move;
	/* 状態の補正は全部の技に効く（Q14）。必中は 0 でなく 0xFFFF（外れ判定は Random32()%101 > calc）。
	 * 雨の必中・晴れの50は上流の表（gAlwaysHitInRainMoves）に任せる。 */
	if (state == NULL || atk >= gBattlersCount)
		return value;
	if (state->selfSureHitTimer[atk] != 0)
		return 0xFFFF;
	if (state->anxietyTimer[atk] != 0)
		value = Lv200B_Scale32(value, 4, 5); /* md 詳細: 命中率×0.8 */
	return value;
}

u8 Lv200CustomMovesB_ModifyType(u16 move, u8 atk, u8 value)
{
	(void) atk;
	if (move == MOVE_LV200_WATER_120)
		return TYPE_WATER;
	return value;
}

s8 Lv200CustomMovesB_ModifyPriority(u16 move, u8 atk, u8 def, s8 value)
{
	(void) atk;
	if (move == MOVE_LV200_DARK_130 && def < gBattlersCount
	&& gBattleMons[def].hp * 2 <= gBattleMons[def].maxHP)
		return 2;
	return value;
}

s32 Lv200CustomMovesB_ModifyDamage(u16 move, u8 atk, u8 def, s32 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 type;
	if (state == NULL || value <= 0 || atk >= gBattlersCount || def >= gBattlersCount)
		return value;
	type = Lv200B_PowerMoveType(move, atk);
	if (state->temporaryRilTimer[def] != 0 && type == TYPE_RIL) /* ツキノコトワリ: リルの印→リル技で受けるダメージ×1.3（md 詳細） */
		value = Lv200B_Scale32(value, 13, 10);
	if (state->deepSeaTimer[def] != 0 && type == TYPE_FIRE)
		value = Lv200B_Scale32(value, 2, 3);
	if ((state->armoredRootBits & gBitTable[def]) && (gStatuses3[def] & STATUS3_ROOTED))
		value = Lv200B_Scale32(value, 2, 3);
	return value;
}

/* ScreensWeakenDamage（damage_calc.c）の1体のときの壁。ミントのかべの2枚は2/3（急所・すりぬけでは呼ばれない）。 */
u32 Lv200CustomMovesB_SingleScreenDamage(u32 damage, u8 bankDef)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && bankDef < gBattlersCount
	&& (state->psychicBarrierBits & gBitTable[SIDE(bankDef)])
	&& (gSideStatuses[SIDE(bankDef)] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN))
	== (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN))
		return (damage * 2) / 3;
	return damage / 2;
}

/* グラスフィールドの草技の強化。じゅかいのひかりで張った間は1.3→1.5（置き換え・Q15）。 */
u32 Lv200CustomMovesB_GrassyTerrainPower(u32 power, u32 boost)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && state->terrainOwner == LV200_B_TERRAIN_OWNER_FOREST_LIGHT && gTerrainType == GRASSY_TERRAIN)
		boost = 15;
	return (power * boost) / 10;
}

/* 場の効果で加わるタイプ（相性・一致ボーナス用）。ひょうがじだい: 場の全員に「こおり」。 */
void Lv200CustomMovesB_AdjustTypes(u8 bank, u8 *type1, u8 *type2, u8 *type3)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount || state->iceAgeTimer == 0)
		return;
	if (*type1 == TYPE_ICE || *type2 == TYPE_ICE || *type3 == TYPE_ICE)
		return;
	if (IS_BLANK_TYPE(*type3))
		*type3 = TYPE_ICE;
	else if (*type2 == *type1)
		*type2 = TYPE_ICE;
}

bool8 Lv200CustomMovesB_IsHeartGuard(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	return state != NULL && bank < gBattlersCount && (state->heartGuardBits & gBitTable[bank]) != 0;
}

bool8 Lv200CustomMovesB_IsFixedFreeze(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	return state != NULL && bank < gBattlersCount && state->fixedFreezeTimer[bank] != 0;
}

u32 Lv200CustomMovesB_ModifyLeechSeedDamage(u8 bank, u32 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();

	if (state != NULL && bank < gBattlersCount
	 && (state->strongLeechBits & gBitTable[bank])
	 && (gStatuses3[bank] & STATUS3_LEECHSEED))
		return MathMax(1, GetBaseMaxHP(bank) / 6);
	return value;
}

bool8 Lv200CustomMovesB_ShouldConsumePP(u16 move, u8 atk, bool8 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 type;
	if (state == NULL || atk >= gBattlersCount || !value)
		return value;
	type = Lv200CustomMovesB_ModifyType(move, atk, gBattleMoves[move].type);
	if (state->terrainOwner == LV200_B_TERRAIN_OWNER_THUNDEROUS_ECHO
	&& gTerrainType == ELECTRIC_TERRAIN && type == TYPE_ELECTRIC)
	{
		++state->electricPpPhase[atk];
		if (state->electricPpPhase[atk] >= 3)
		{
			state->electricPpPhase[atk] = 0;
			return FALSE;
		}
	}
	return value;
}

u32 Lv200CustomMovesB_ModifyAttackValue(u16 move, u8 atk, u8 def, u32 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) def;
	if (state == NULL || atk >= gBattlersCount)
		return value;
	if (state->coldGazeTimer[atk] != 0)
		value /= 2;
	/* エレキタックル: やけどの攻撃半減を打ち消す（やけどのみ。ガッツは上流が既に半減しないので掛けない） */
	if (move == MOVE_LV200_ELECTRIC_120
	&& (gBattleMons[atk].status1 & STATUS1_BURN)
	&& !BankHasAbility(atk, ABILITY_GUTS))
		value *= 2;
	return value;
}

u32 Lv200CustomMovesB_ModifySpeed(u8 bank, u32 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && bank < gBattlersCount && state->snowOwner != 0
	&& WEATHER_HAS_EFFECT && (gBattleWeather & WEATHER_HAIL_ANY)
	&& IsOfType(bank, TYPE_ICE))
		value = Lv200B_Scale32(value, 3, 2);
	return value;
}

u16 Lv200CustomMovesB_ModifyTypeEffectiveness(u16 move, u8 atk, u8 def, u16 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) atk;
	if (def >= gBattlersCount)
		return value;
	if (move == MOVE_LV200_PSYCHIC_200)
		return TYPE_MUL_NORMAL;
	if (move == MOVE_LV200_WATER_160
	&& value > TYPE_MUL_NO_EFFECT && value < TYPE_MUL_NORMAL)
		return TYPE_MUL_NORMAL;
	if (move == MOVE_LV200_ICE_130 /* Q28: 5 タイプ以外は通常のタイプ表どおり（抵抗・無効もそのまま） */
	&& (IsOfType(def, TYPE_WATER) || IsOfType(def, TYPE_FLYING)
	 || IsOfType(def, TYPE_GROUND) || IsOfType(def, TYPE_GRASS)
	 || IsOfType(def, TYPE_DRAGON)))
		return TYPE_MUL_SUPER_EFFECTIVE;
	/* りゅうのみや: ひこうの部分だけ 1倍→2倍（既存の相性には掛け合わせて足す・免疫は0のまま）。自分側のドラゴン技のみ。 */
	if (state != NULL && state->dragonPalaceTimer != 0 && atk < gBattlersCount
	&& SIDE(atk) == state->dragonPalaceSide
	&& gBattleMoves[move].type == TYPE_DRAGON && IsOfType(def, TYPE_FLYING)
	&& value != 0)
		return value * 2;
	return value;
}

bool8 Lv200CustomMovesB_IsBattlerOfType(u8 bank, u8 type, bool8 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount)
		return value;
	if (type == TYPE_ICE && state->iceAgeTimer != 0)
		return TRUE;
	return value;
}

bool8 Lv200CustomMovesB_IsAbilityActive(u8 bank, bool8 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount)
		return value;
	if (state->abilityItemNullTimer != 0
	|| (state->abilitySuppressOwnedBits & gBitTable[bank]))
		return FALSE;
	return value;
}

bool8 Lv200CustomMovesB_CanLowerStat(u8 bank, u8 source, bool8 value)
{
	(void) bank;
	(void) source;
	return value;
}

bool8 Lv200CustomMovesB_CanRaiseStat(u8 bank, u8 source, bool8 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) source;
	if (state != NULL && bank < gBattlersCount
	&& (state->statRiseBlockedBits & gBitTable[bank]))
		return FALSE;
	return value;
}

bool8 Lv200CustomMovesB_CanFlinch(u8 bank, u8 source, bool8 value)
{
	(void) bank;
	(void) source;
	return value;
}

/* りゅうぎゃくりん（Q27）: どの技でひるませても、次のターンの威力2倍の対象に数える。付いたときだけ呼ばれる。 */
void Lv200CustomMovesB_RecordFlinch(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && bank < gBattlersCount)
		state->flinchedThisRoundBits |= gBitTable[bank];
}

bool8 Lv200CustomMovesB_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value)
{
	(void) bank;
	(void) source;
	return value;
}

bool8 Lv200CustomMovesB_IsItemActive(u8 bank, bool8 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount)
		return value;
	if (state->abilityItemNullTimer != 0
	|| (state->darknessHoleItemSuppressedBits & gBitTable[bank]))
		return FALSE;
	return value;
}

u16 Lv200CustomMovesB_GetItemForEffect(u8 bank, u16 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state != NULL && bank < gBattlersCount && value == ITEM_NONE)
		return state->copiedItem[bank];
	return value;
}

void Lv200CustomMovesB_OnMoveHit(u16 move, u8 atk, u8 def, u32 damage,
								 bool8 hitSubstitute, bool8 substituteBroke)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 slot;

	if (state == NULL || atk >= gBattlersCount || def >= gBattlersCount || damage == 0)
		return;
	if (!hitSubstitute) /* みがわりが受けた分は「受けたダメージ」に数えない（Q20） */
		Lv200CustomMovesB_RecordDamage(state, def, damage);
	state->lastDamageTaken[def] = damage;
	state->lastDamageSource[def] = atk;
	if (state->mirrorTimer[def] != 0 && SPLIT(move) == SPLIT_SPECIAL && !hitSubstitute)
	{
		u32 reflected = damage > 0x3FFFFFFF ? 0x7FFFFFFF : damage * 2;
		if (state->mirrorSpecialDamage[def] > 0x7FFFFFFF - reflected)
			state->mirrorSpecialDamage[def] = 0x7FFFFFFF;
		else
			state->mirrorSpecialDamage[def] += reflected;
		state->mirrorSource[def] = atk;
	}
	if ((state->tearMoonArmedBits & gBitTable[def]) && !hitSubstitute)
	{
		state->tearMoonArmedBits &= ~gBitTable[def];
		state->tearMoonBoostBits |= gBitTable[def];
	}
	/* 壊れたみがわりの印は、壊した技が何でも処理する（IsBMove の門の前・ぬいぐるみのかげ／ぶんかつのいぶき） */
	if (substituteBroke)
	{
		if (state->substituteEffect[def] == LV200_B_SUBSTITUTE_EFFECT_ATK_DROP)
			Lv200Common_QueueStat(atk, def, STAT_STAGE_ATK, -2, FALSE);
		state->substituteEffect[def] = LV200_B_SUBSTITUTE_EFFECT_NONE;
	}
	if (!Lv200B_IsBMove(move))
		return;
	if (hitSubstitute)
		return;

	switch (move)
	{
	case MOVE_LV200_DARK_110:
		if (gBattleMons[atk].item == ITEM_NONE && gBattleMons[def].item != ITEM_NONE)
		{
			state->copiedItem[atk] = gBattleMons[def].item;
			RecordItemEffectBattle(atk, ItemId_GetHoldEffect(state->copiedItem[atk]));
		}
		break;
	case MOVE_LV200_GRASS_150:
		if (!IsOfType(def, TYPE_GRASS) && !(gStatuses3[def] & STATUS3_LEECHSEED))
			gStatuses3[def] |= STATUS3_LEECHSEED | atk;
		break;
	case MOVE_LV200_GRASS_180:
		state->timedTrapTimer[def] = 3;
		state->timedTrapOwner[def] = atk + 1;
		gBattleMons[def].status2 |= STATUS2_ESCAPE_PREVENTION;
		break;
	case MOVE_LV200_ELECTRIC_110:
		if (state->chargeStacks[atk] < 3)
			++state->chargeStacks[atk];
		break;
	case MOVE_LV200_ELECTRIC_130:
		state->timedTrapTimer[def] = 3;
		state->timedTrapOwner[def] = atk + 1;
		gBattleMons[def].status2 |= STATUS2_ESCAPE_PREVENTION;
		break;
	case MOVE_LV200_PSYCHIC_170:
		state->psychic170BoostBits |= gBitTable[atk]; /* エスパーの味方のとくこう+1 は常時（md 詳細） */
		break;
	case MOVE_LV200_ICE_110:
		if (!(state->frostbiteBits & gBitTable[def]) && umodsi(Random(), 100) < 30)
		{
			state->frostbitePendingBits |= gBitTable[def];
			Lv200Common_QueueMessageFrom(def, atk, gText_Lv200B_FrostbiteSet);
		}
		break;
	case MOVE_LV200_DRAGON_120:
		break;
	case MOVE_LV200_DRAGON_170:
		if (gMultiHitCounter <= 1)
			state->dragon170SpeedUpBits |= gBitTable[atk];
		break;
	case MOVE_LV200_DARK_170:
		Lv200B_HealBank(atk, gBattleMons[def].hp == 0 ? gBattleMons[atk].maxHP : (damage * 3) / 4);
		break;
	case MOVE_LV200_FAIRY_110:
		/* 同性の相手に10%でメロメロ（性別なしは対象外・どんかん等は CanBeInfatuated が見る） */
		if (Lv200B_SameGender(atk, def) && umodsi(Random(), 100) < 10)
			Lv200Common_QueueInfatuate(def, atk, TRUE); /* 可否(どんかん・アロマベール・すでにメロメロ)は部品が素の判定で見る */
		break;
	case MOVE_LV200_RIL_110:
		for (slot = 0; slot < MAX_MON_MOVES; ++slot)
		{
			if (gBattleMons[def].moves[slot] == gLastUsedMoves[def])
			{
				gBattleMons[def].pp[slot] = gBattleMons[def].pp[slot] > 4
					? gBattleMons[def].pp[slot] - 4 : 0;
				gDisableStructs[def].disabledMove = gBattleMons[def].moves[slot];
				gDisableStructs[def].disableTimer1 = 2;
				break;
			}
		}
		break;
	case MOVE_LV200_RIL_170:
		state->temporaryRilTimer[def] = 3;
		break;
	default:
		break;
	}
}

void Lv200CustomMovesB_OnMoveEnd(u16 move, u8 atk, u8 def,
								 bool8 succeeded, bool8 didDamage)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 type;
	u16 baseMove;
	if (state == NULL || !succeeded || atk >= gBattlersCount)
		return;
	type = Lv200B_PowerMoveType(move, atk);
	if (move != MOVE_LV200_ELECTRIC_110 && type == TYPE_ELECTRIC && didDamage)
		state->chargeStacks[atk] = 0;
	if (didDamage) /* なみだのつき: 被弾後の次の攻撃技1回で使い切る（この技自身も含む・Q2） */
		state->tearMoonBoostBits &= ~gBitTable[atk];
	if (move == MOVE_LV200_DRAGON_130)
		state->dragon130SpeedDownBits |= gBitTable[atk];
	if (move == MOVE_LV200_FAIRY_200 && didDamage)
	{
		/* 場の味方（自分＋ダブルの相方）が最大HPの1/8回復（Q21） */
		Lv200B_HealBank(atk, MathMax(1, gBattleMons[atk].maxHP / 8));
		if (IS_DOUBLE_BATTLE && PARTNER(atk) < gBattlersCount && Lv200B_IsAlive(PARTNER(atk)))
			Lv200B_HealBank(PARTNER(atk), MathMax(1, gBattleMons[PARTNER(atk)].maxHP / 8));
	}
	baseMove = EffectiveMove(move); //[hack] 効果元が同じ新技もこれらの効果を持つ
	if ((baseMove == MOVE_RAPIDSPIN || baseMove == MOVE_MORTALSPIN) && didDamage)
		Lv200B_ClearHazards(SIDE(atk));
	else if (baseMove == MOVE_DEFOG || baseMove == MOVE_TIDYUP)
	{
		Lv200B_ClearHazards(B_SIDE_PLAYER);
		Lv200B_ClearHazards(B_SIDE_OPPONENT);
	}
	else if (baseMove == MOVE_COURTCHANGE && def < gBattlersCount)
	{
		u8 present = state->frozenHazardPresent[B_SIDE_PLAYER];
		u8 owner = state->frozenHazardBank[B_SIDE_PLAYER];
		state->frozenHazardPresent[B_SIDE_PLAYER] = state->frozenHazardPresent[B_SIDE_OPPONENT];
		state->frozenHazardBank[B_SIDE_PLAYER] = state->frozenHazardBank[B_SIDE_OPPONENT];
		state->frozenHazardPresent[B_SIDE_OPPONENT] = present;
		state->frozenHazardBank[B_SIDE_OPPONENT] = owner;
	}
}

void Lv200CustomMovesB_OnContact(u16 move, u8 atk, u8 def, bool8 protected)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) move;
	/* protected は touchedProtectLike（accuracy_calc.c の ProtectAffects が Lv200CustomMove_IsHeartGuard で立てる） */
	if (state != NULL && protected && def < gBattlersCount
	&& (state->heartGuardBits & gBitTable[def]))
		Lv200Common_QueueInfatuate(atk, def, FALSE);
}

void Lv200CustomMovesB_OnFaint(u8 bank, u8 source, u16 move, u32 damage)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) source;
	(void) move;
	(void) damage;
	if (state != NULL && bank < gBattlersCount
	&& (state->destinySuccessorBits & gBitTable[bank]))
	{
		Lv200CustomMovesB_AddPendingPositionEffect(state, GetBattlerPosition(bank), LV200_B_PENDING_OMNIBOOST);
		state->destinySuccessorBits &= ~gBitTable[bank];
	}
}

bool8 Lv200CustomMovesB_CanSwitch(u8 bank, bool8 value)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	(void) bank;
	if (state != NULL && gNewBS->FairyLockTimer != 0)
		return FALSE;
	return value;
}

enum Lv200BEndTurnEffect
{
	LV200_B_ENDTURN_MICROWAVE_ATK,
	LV200_B_ENDTURN_MICROWAVE_SPATK,
	LV200_B_ENDTURN_FROSTBITE,
	LV200_B_ENDTURN_SUSPICIOUS_SLEEP,
	LV200_B_ENDTURN_SUBSTITUTE_REGEN,
	LV200_B_ENDTURN_FAIRY_RING,
	LV200_B_ENDTURN_EFFECT_COUNT,
};

static const u8* Lv200B_PrepareEndTurnStatDown(u8 bank, u8 stat)
{
	gBankAttacker = bank;
	gBankTarget = bank;
	gBattleScripting.statChanger = stat | DECREASE_1;
	return BattleScript_Lv200B_EndTurnStatDown;
}

static const u8* Lv200B_PrepareEndTurnDamage(u8 bank)
{
	gBankAttacker = bank;
	gBankTarget = bank;
	gBattleMoveDamage = MathMax(1, gBattleMons[bank].maxHP / 16);
	gNewBS->turnDamageTaken[bank] = gBattleMoveDamage;
	return BattleScript_Lv200B_EndTurnDamage;
}

static const u8* Lv200B_PrepareEndTurnHeal(u8 bank)
{
	gBankAttacker = bank;
	gBankTarget = bank;
	gBattleMoveDamage = -1 * MathMax(1, gBattleMons[bank].maxHP / 16);
	return BattleScript_Lv200B_EndTurnHeal;
}

const u8* Lv200CustomMovesB_GetMoveEndScript(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 bank;

	if (state == NULL)
		return NULL;
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		u8 source;
		u32 damage = state->mirrorSpecialDamage[bank];
		if (damage == 0)
			continue;
		source = state->mirrorSource[bank];
		state->mirrorSpecialDamage[bank] = 0;
		state->mirrorSource[bank] = 0;
		if (!Lv200B_IsAlive(source))
			continue;
		gBankAttacker = source;
		gBankTarget = source;
		gBattleMoveDamage = damage;
		return BattleScript_Lv200B_MirrorDamage;
	}
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (!(state->frostbitePendingBits & gBitTable[bank]))
			continue;
		state->frostbitePendingBits &= ~gBitTable[bank];
		if (!Lv200B_IsAlive(bank))
			continue;
		state->frostbiteBits |= gBitTable[bank];
		gBattleScripting.bank = bank;
		gBattleScripting.statChanger = STAT_STAGE_SPEED | DECREASE_1;
		return BattleScript_Lv200B_MoveEndStatDown;
	}
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (!(state->dragon130SpeedDownBits & gBitTable[bank]))
			continue;
		state->dragon130SpeedDownBits &= ~gBitTable[bank];
		if (!Lv200B_IsAlive(bank))
			continue;
		gBattleScripting.bank = bank;
		gBattleScripting.statChanger = STAT_STAGE_SPEED | DECREASE_1;
		return BattleScript_Lv200B_MoveEndStatDown;
	}
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (!(state->dragon170SpeedUpBits & gBitTable[bank]))
			continue;
		state->dragon170SpeedUpBits &= ~gBitTable[bank];
		if (!Lv200B_IsAlive(bank))
			continue;
		gBattleScripting.bank = bank;
		gBattleScripting.statChanger = STAT_STAGE_SPEED | INCREASE_1;
		return BattleScript_Lv200B_MoveEndStatUp;
	}
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		u8 partner;
		if (!(state->psychic170BoostBits & gBitTable[bank]))
			continue;
		state->psychic170BoostBits &= ~gBitTable[bank];
		partner = PARTNER(bank);
		if (!Lv200B_IsAlive(partner) || !IsOfType(partner, TYPE_PSYCHIC))
			continue;
		gBattleScripting.bank = partner;
		gBattleScripting.statChanger = STAT_STAGE_SPATK | INCREASE_1;
		return BattleScript_Lv200B_MoveEndStatUp;
	}
	return NULL;
}

/* 効果が切れる文（開始の文がある場・状態だけ・Q30/24）。この終端のあとの Tick で 0 になる（タイマーが 1）ものを 1 件ずつ。
 * group: 0=しんかいのきおく, 1=リルのまなざし(個体ごと), 2=場全体(bank 0..3 = ひょうがじだい・りゅうのみや・やみ/よるのうみ・とくせい/どうぐ) */
#define LV200_B_EXPIRY_GROUPS 3

static const u8* Lv200B_GetExpiryScript(struct Lv200CustomMovesBState* state)
{
	while (state->expiryCursor < MAX_BATTLERS_COUNT * LV200_B_EXPIRY_GROUPS)
	{
		u8 cursor = state->expiryCursor++;
		u8 bank = cursor % MAX_BATTLERS_COUNT;
		const u8* text = NULL;

		switch (cursor / MAX_BATTLERS_COUNT)
		{
		case 0:
			if (state->deepSeaTimer[bank] == 1) text = gText_Lv200B_DeepSeaEnd;
			break;
		case 1:
			if (state->statRiseBlockedTimer[bank] == 1) text = gText_Lv200B_RilGazeEnd;
			break;
		case 2:
			if (bank == 0 && state->iceAgeTimer == 1) text = gText_Lv200B_IceAgeEnd;
			else if (bank == 1 && state->dragonPalaceTimer == 1) text = gText_Lv200B_DragonPalaceEnd;
			else if (bank == 2 && state->customFieldTimer == 1)
			{
				if (state->customFieldKind == LV200_B_FIELD_DARKNESS) text = gText_Lv200B_DarknessEnd;
				else if (state->customFieldKind == LV200_B_FIELD_NIGHT_SEA) text = gText_Lv200B_NightSeaEnd;
			}
			else if (bank == 3 && state->abilityItemNullTimer == 1) text = gText_Lv200B_AbilityItemNullEnd;
			break;
		}
		if (text == NULL)
			continue;
		if (cursor / MAX_BATTLERS_COUNT <= 1 && !(bank < gBattlersCount && Lv200B_IsAlive(bank)))
			continue;
		gBankAttacker = bank;
		gBankTarget = bank;
		gBattleStringLoader = text;
		return BattleScript_Lv200B_EndTurnMessage;
	}
	return NULL;
}

const u8* Lv200CustomMovesB_GetEndTurnScript(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 bank;
	u8 effect;

	if (state == NULL)
		return NULL;

	while (state->endTurnCursor < MAX_BATTLERS_COUNT * LV200_B_ENDTURN_EFFECT_COUNT)
	{
		bank = state->endTurnCursor / LV200_B_ENDTURN_EFFECT_COUNT;
		effect = state->endTurnCursor % LV200_B_ENDTURN_EFFECT_COUNT;
		++state->endTurnCursor;

		if (bank >= gBattlersCount || !Lv200B_IsAlive(bank))
			continue;

		switch (effect)
		{
		case LV200_B_ENDTURN_MICROWAVE_ATK:
			if (state->microwaveTimer[bank] != 0
			&& gBattleMons[bank].statStages[STAT_STAGE_ATK - 1] != 0)
				return Lv200B_PrepareEndTurnStatDown(bank, STAT_STAGE_ATK);
			break;
		case LV200_B_ENDTURN_MICROWAVE_SPATK:
			if (state->microwaveTimer[bank] != 0
			&& gBattleMons[bank].statStages[STAT_STAGE_SPATK - 1] != 0)
				return Lv200B_PrepareEndTurnStatDown(bank, STAT_STAGE_SPATK);
			break;
		case LV200_B_ENDTURN_FROSTBITE:
			if ((state->frostbiteBits & gBitTable[bank])
			&& !BankHasAbility(bank, ABILITY_MAGICGUARD))
				return Lv200B_PrepareEndTurnDamage(bank);
			break;
		case LV200_B_ENDTURN_SUSPICIOUS_SLEEP:
			if ((state->suspiciousSleepBits & gBitTable[bank])
			&& (gBattleMons[bank].status1 & STATUS1_SLEEP)
			&& gBattleMons[bank].statStages[STAT_STAGE_SPDEF - 1] != 0)
				return Lv200B_PrepareEndTurnStatDown(bank, STAT_STAGE_SPDEF);
			break;
		case LV200_B_ENDTURN_SUBSTITUTE_REGEN:
			if (state->substituteEffect[bank] == LV200_B_SUBSTITUTE_EFFECT_REGEN
			&& gDisableStructs[bank].substituteHP != 0
			&& gBattleMons[bank].hp < gBattleMons[bank].maxHP
			&& !IsHealBlocked(bank))
				return Lv200B_PrepareEndTurnHeal(bank);
			break;
		case LV200_B_ENDTURN_FAIRY_RING:
			if (state->terrainOwner == LV200_B_TERRAIN_OWNER_FAIRY_RING
			&& SIDE(bank) == state->terrainOwnerSide && IsOfType(bank, TYPE_FAIRY)
			&& gBattleMons[bank].hp < gBattleMons[bank].maxHP
			&& !IsHealBlocked(bank))
				return Lv200B_PrepareEndTurnHeal(bank);
			break;
		}
	}

	return Lv200B_GetExpiryScript(state);
}

static void Lv200B_TickTimer(u8* timer)
{
	if (*timer != 0)
		--*timer;
}

void Lv200CustomMovesB_OnTurnEnd(void)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 bank;
	u8 side;
	bool8 trapWasTicking;
	if (state == NULL)
		return;

	for (side = 0; side < NUM_BATTLE_SIDES; ++side)
	{
		Lv200B_TickTimer(&state->tideTimer[side]);
		if ((gSideStatuses[side] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN))
		 != (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN))
			state->psychicBarrierBits &= ~gBitTable[side];
	}
	Lv200B_TickTimer(&state->customFieldTimer);
	Lv200B_TickTimer(&state->abilityItemNullTimer);
	Lv200B_TickTimer(&state->iceAgeTimer);
	Lv200B_TickTimer(&state->dragonPalaceTimer);
	if (state->customFieldTimer == 0)
		state->customFieldKind = LV200_B_FIELD_NONE;
	if (gNewBS->GravityTimer == 0)
		state->gravityOwner = 0;
	if (!(gBattleWeather & WEATHER_HAIL_ANY))
		state->snowOwner = 0;
	if ((state->terrainOwner == LV200_B_TERRAIN_OWNER_FOREST_LIGHT
	  && gTerrainType != GRASSY_TERRAIN)
	 || (state->terrainOwner == LV200_B_TERRAIN_OWNER_THUNDEROUS_ECHO
	  && gTerrainType != ELECTRIC_TERRAIN)
	 || (state->terrainOwner == LV200_B_TERRAIN_OWNER_FAIRY_RING
	  && gTerrainType != MISTY_TERRAIN))
		state->terrainOwner = LV200_B_TERRAIN_OWNER_NONE;

	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		if (!(gStatuses3[bank] & STATUS3_LEECHSEED))
			ClearBankBit(&state->strongLeechBits, bank);
		state->lastDamageTaken[bank] = 0;
		state->lastDamageSource[bank] = 0;
		Lv200B_TickTimer(&state->deepSeaTimer[bank]);
		trapWasTicking = (state->timedTrapTimer[bank] != 0);
		Lv200B_TickTimer(&state->timedTrapTimer[bank]);
		Lv200B_TickTimer(&state->microwaveTimer[bank]);
		Lv200B_TickTimer(&state->mirrorTimer[bank]);
		Lv200B_TickTimer(&state->coldGazeTimer[bank]);
		Lv200B_TickTimer(&state->selfSureHitTimer[bank]);
		Lv200B_TickTimer(&state->anxietyTimer[bank]);
		Lv200B_TickTimer(&state->temporaryRilTimer[bank]);
		if (state->statRiseBlockedTimer[bank] != 0)
		{
			Lv200B_TickTimer(&state->statRiseBlockedTimer[bank]);
			if (state->statRiseBlockedTimer[bank] == 0)
				ClearBankBit(&state->statRiseBlockedBits, bank);
		}

		if (state->timedTrapTimer[bank] == 0)
		{
			state->timedTrapOwner[bank] = 0;
			//[hack] FIX-moves-P6: 3 ターンの逃げ止めがちょうど終わった時だけ外す（前は毎ターン全員から外し、くろいまなざし・クモのす・GRASS_120・RIL_130 の逃げ止めが 1 ターンで消えていた）
			if (trapWasTicking)
				gBattleMons[bank].status2 &= ~STATUS2_ESCAPE_PREVENTION;
		}
		if ((state->suspiciousSleepBits & gBitTable[bank]))
		{
			if (!(gBattleMons[bank].status1 & STATUS1_SLEEP))
				state->suspiciousSleepBits &= ~gBitTable[bank];
		}
		if ((state->dreamPrincessBits & gBitTable[bank])
		&& !(gBattleMons[bank].status1 & STATUS1_SLEEP))
			state->dreamPrincessBits &= ~gBitTable[bank];
		if (state->fixedFreezeTimer[bank] != 0)
		{
			gBattleMons[bank].status1 = STATUS1_FREEZE;
			Lv200B_TickTimer(&state->fixedFreezeTimer[bank]);
			if (state->fixedFreezeTimer[bank] == 0)
				gBattleMons[bank].status1 &= ~STATUS1_FREEZE;
		}
		if (state->timedAbilitySuppress[bank] != 0)
		{
			Lv200B_TickTimer(&state->timedAbilitySuppress[bank]);
			if (state->timedAbilitySuppress[bank] == 0)
			{
				if (state->abilitySuppressOwnedBits & gBitTable[bank])
				{
					gBattleMons[bank].ability = gNewBS->SuppressedAbilities[bank];
					gNewBS->SuppressedAbilities[bank] = ABILITY_NONE;
					gStatuses3[bank] &= ~STATUS3_ABILITY_SUPPRESS;
					ResetVarsForAbilityChange(bank);
				}
				state->abilitySuppressOwnedBits &= ~gBitTable[bank];
			}
		}
		if (state->mimicTimer[bank] != 0)
		{
			Lv200B_TickTimer(&state->mimicTimer[bank]);
			if (state->mimicTimer[bank] == 0)
			{
				u8 slot = state->mimicSlot[bank];
				gBattleMons[bank].moves[slot] = state->mimicSavedMove[bank];
				gBattleMons[bank].pp[slot] = state->mimicSavedPp[bank];
			}
		}
	}
	state->expiryCursor = 0;
	state->destinySuccessorBits = 0; /* わすれがたみ: 使ったターンに倒された時だけ（md） */
	Lv200CustomMovesB_RollRoundState(state);
	state->endTurnCursor = 0;
}

void Lv200CustomMovesB_OnSwitchOut(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount)
		return;
	if (state->mimicTimer[bank] != 0)
	{
		u8 slot = state->mimicSlot[bank];
		gBattleMons[bank].moves[slot] = state->mimicSavedMove[bank];
		gBattleMons[bank].pp[slot] = state->mimicSavedPp[bank];
		gDisableStructs[bank].mimickedMoves &= ~gBitTable[slot];
	}
	if (state->abilitySuppressOwnedBits & gBitTable[bank])
	{
		gBattleMons[bank].ability = gNewBS->SuppressedAbilities[bank];
		gNewBS->SuppressedAbilities[bank] = ABILITY_NONE;
		gStatuses3[bank] &= ~STATUS3_ABILITY_SUPPRESS;
		ResetVarsForAbilityChange(bank);
	}
	gStatuses3[bank] &= ~STATUS3_PERISH_SONG;
	state->perishOwnerBits &= ~gBitTable[bank];
	Lv200CustomMovesB_ClearBattler(state, bank);
}

void Lv200CustomMovesB_OnStatusCured(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount)
		return;
	state->frostbiteBits &= ~gBitTable[bank];
	state->frostbitePendingBits &= ~gBitTable[bank];
	state->fixedFreezeTimer[bank] = 0; /* 治療・ほのお技の解凍で固定凍結も終わる */
}

void Lv200CustomMovesB_OnHazardsCleared(u8 side)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || side >= NUM_BATTLE_SIDES)
		return;
	state->frozenHazardPresent[side] = FALSE;
	state->frozenHazardBank[side] = 0;
}

void Lv200CustomMovesB_OnScreensRemoved(u8 side)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || side >= NUM_BATTLE_SIDES)
		return;
	state->psychicBarrierBits &= ~gBitTable[side];
}

void Lv200CustomMovesB_OnSwitchIn(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	if (state == NULL || bank >= gBattlersCount)
		return;
	if (state->frozenHazardPresent[SIDE(bank)] && CheckGrounding(bank)) /* ひこう・ふゆうは影響を受けない（md 詳細） */
		Lv200CustomMovesB_AddPendingPositionEffect(state, GetBattlerPosition(bank), LV200_B_PENDING_ICE_HAZARD);
}

const u8* Lv200CustomMovesB_GetSwitchInScript(u8 bank)
{
	struct Lv200CustomMovesBState* state = Lv200CustomMove_GetStateB();
	u8 position;
	u8 effects;

	if (state == NULL || bank >= gBattlersCount || !Lv200B_IsAlive(bank))
		return NULL;
	position = GetBattlerPosition(bank);
	effects = state->pendingPositionEffects[position];
	if (effects == 0)
		return NULL;

	gBankAttacker = bank;
	gBankTarget = bank;
	if (effects & LV200_B_PENDING_SPEED_UP)
	{
		state->pendingPositionEffects[position] &= ~LV200_B_PENDING_SPEED_UP;
		gBattleScripting.statChanger = STAT_STAGE_SPEED | INCREASE_1;
		return BattleScript_Lv200B_SwitchInStatUp;
	}
	if (effects & LV200_B_PENDING_CHARGE)
	{
		state->pendingPositionEffects[position] &= ~LV200_B_PENDING_CHARGE;
		gStatuses3[bank] |= STATUS3_CHARGED_UP; /* atkCB_setcharge と同じ（damage_calc の ×2 が見るのはこの印） */
		#ifdef GEN9_CHARGE
		gDisableStructs[bank].chargeTimer = 0;
		#else
		gDisableStructs[bank].chargeTimer = 2;
		#endif
		return BattleScript_Lv200B_SwitchInCharge;
	}
	if (effects & LV200_B_PENDING_OMNIBOOST)
	{
		state->pendingPositionEffects[position] &= ~LV200_B_PENDING_OMNIBOOST;
		return BattleScript_Lv200B_SwitchInOmniboost;
	}
	state->pendingPositionEffects[position] &= ~LV200_B_PENDING_ICE_HAZARD;
	gBattleScripting.statChanger = STAT_STAGE_SPEED | (state->frozenHazardPresent[SIDE(bank)] >= 2 ? DECREASE_2 : DECREASE_1);
	return BattleScript_Lv200B_EndTurnStatDown;
}

void Lv200CustomMovesB_ResetState(void)
{
	Lv200CustomMovesB_ResetStateData(Lv200CustomMove_GetStateB());
}

void Lv200CustomMovesB_OnBattleEnd(void)
{
	Lv200CustomMovesB_ResetState();
}

