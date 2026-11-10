#include "defines.h"
#include "defines_battle.h"
#include "lv200_custom_moves.h"
#include "lv200_custom_moves_a.h"
#include "lv200_custom_common.h"
#include "../include/battle_message.h"
#include "../include/battle_string_ids.h"
#include "../include/constants/items.h"
#include "../include/item.h"
#include "../include/new/ability_util.h"
#include "../include/new/battle_util.h"
#include "../include/new/battle_start_turn_start.h"
#include "../include/new/damage_calc.h"
#include "../include/new/dynamax.h"
#include "../include/new/move_tables.h"
#include "../include/new/move_battle_scripts.h"
#include "../include/new/switching.h"
#include "../include/new/terastal.h"
#include "../include/new/util.h"
#include "../include/random.h"

extern const u8 *gBattleScriptsForMoveEffects[];
extern const u8 BattleScript_Lv200A_Damage[];
extern const u8 BattleScript_Lv200A_Fighting180[];
extern const u8 BattleScript_Lv200A_Ground180[];
extern const u8 BattleScript_Lv200A_PoisonKing[];
extern const u8 BattleScript_Lv200A_EndTurnPoisonKingHeal[];
extern const u8 BattleScript_Lv200A_EndTurnPoisonKingEnd[];
extern const u8 BattleScript_Lv200A_Rock170[];
extern const u8 BattleScript_Lv200A_Status[];
extern const u8 BattleScript_Lv200A_RockCurtain[];
extern const u8 BattleScript_Lv200A_Multi[];
extern const u8 BattleScript_Lv200A_Counter[];
extern const u8 BattleScript_Lv200A_Pivot[];
extern const u8 BattleScript_Lv200A_PerishPivot[];
extern const u8 BattleScript_Lv200A_EndTurnDamage[];
extern const u8 BattleScript_Lv200A_EndTurnDrain[];
extern const u8 BattleScript_Lv200A_EndTurnDefDown[];
extern const u8 BattleScript_Lv200A_SwitchInSpeedDown[];
extern const u8 BattleScript_Lv200A_WingGuardForceSwitch[];
extern const u8 gText_Lv200A_FinalCountStart[];
extern const u8 gText_Lv200A_FlyingBarrierSet[];
extern const u8 gText_Lv200A_FlyingBarrierEnd[];
extern const u8 gText_Lv200A_HighSoar[];
extern const u8 gText_Lv200A_Molt[];
extern const u8 gText_Lv200A_FocusMind[];
extern const u8 gText_Lv200A_StanceSet[];
extern const u8 gText_Lv200A_Afterimage[];
extern const u8 gText_Lv200A_PupaSet[];
extern const u8 gText_Lv200A_Emerge[];
extern const u8 gText_Lv200A_PowderCurtainSet[];
extern const u8 gText_Lv200A_SteelWallSet[];
extern const u8 gText_Lv200A_QuartzSet[];
extern const u8 gText_Lv200A_RockFieldSet[];
extern const u8 gText_Lv200A_MudFieldSet[];
extern const u8 gText_Lv200A_DarkVeilSet[];
extern const u8 gText_Lv200A_EggSet[];
extern const u8 gText_Lv200A_SoulContractSet[];
extern const u8 gText_Lv200A_GrudgeSet[];
extern const u8 gText_Lv200A_StockVenomLayer[];
extern const u8 gText_Lv200A_CocoonArmorSet[];
extern const u8 gText_Lv200A_MirrorArmorSet[];
extern const u8 gText_Lv200A_MetalArmorSet[];
extern const u8 gText_Lv200A_TurboSet[];
extern const u8 gText_Lv200A_StanceEnd[];
extern const u8 gText_Lv200A_PowderCurtainEnd[];
extern const u8 gText_Lv200A_SteelWallEnd[];
extern const u8 gText_Lv200A_QuartzEnd[];
extern const u8 gText_Lv200A_RockFieldEnd[];
extern const u8 gText_Lv200A_MudFieldEnd[];
extern const u8 gText_Lv200A_DarkVeilEnd[];
extern const u8 gText_Lv200A_SoulContractEnd[];
extern const u8 gText_Lv200A_CocoonArmorEnd[];
extern const u8 gText_Lv200A_MirrorArmorEnd[];
extern const u8 gText_Lv200A_MetalArmorEnd[];
extern const u8 gText_Lv200A_TurboEnd[];
extern const u8 gText_Lv200A_VenomShift[];
extern const u8 gText_Lv200A_ToxicWorse[];
extern const u8 gText_Lv200A_GearSwap[];
extern const u8 gText_Lv200A_Overhaul[];
extern const u8 gText_Lv200A_FieldWipe[];
extern const u8 gText_Lv200A_MirrorRound[];
extern const u8 BattleScript_Lv200A_EndTurnMessage[];

/* ほりだしもの: 新しい道具番号を作らず、既存の通常入手品だけを使う。 */
static const u16 sLv200MiningGems[] =
{
	ITEM_NORMAL_GEM, ITEM_FIGHTING_GEM, ITEM_FLYING_GEM,
	ITEM_POISON_GEM, ITEM_GROUND_GEM, ITEM_ROCK_GEM,
	ITEM_BUG_GEM, ITEM_GHOST_GEM, ITEM_STEEL_GEM,
	ITEM_FIRE_GEM, ITEM_WATER_GEM, ITEM_GRASS_GEM,
	ITEM_ELECTRIC_GEM, ITEM_PSYCHIC_GEM, ITEM_ICE_GEM,
	ITEM_DRAGON_GEM, ITEM_DARK_GEM, ITEM_FAIRY_GEM,
};

static const u16 sLv200MiningStones[] =
{
	ITEM_SUN_STONE, ITEM_MOON_STONE, ITEM_FIRE_STONE,
	ITEM_THUNDER_STONE, ITEM_WATER_STONE, ITEM_LEAF_STONE,
	ITEM_SHINY_STONE, ITEM_DUSK_STONE, ITEM_DAWN_STONE,
	ITEM_ICE_STONE,
};

static const u16 sLv200MiningValuables[] =
{
	ITEM_PEARL, ITEM_BIG_PEARL, ITEM_STARDUST, ITEM_STAR_PIECE,
	ITEM_NUGGET, ITEM_PEARL_STRING, ITEM_COMET_SHARD, ITEM_BIG_NUGGET,
};

static u16 Lv200A_ChooseMiningReward(void)
{
	u16 category = Random() % 10;

	if (category < 5)
		return sLv200MiningGems[Random() % NELEMS(sLv200MiningGems)];
	if (category < 8)
		return sLv200MiningStones[Random() % NELEMS(sLv200MiningStones)];
	return sLv200MiningValuables[Random() % NELEMS(sLv200MiningValuables)];
}
extern const u8 BattleScript_Lv200A_EchoDamage[];
extern const u8 BattleScript_Lv200A_EndTurnHeal[];

typedef void (*Lv200AHandler)(void);

struct Lv200AMoveEntry
{
	u16 move;
	u16 baseMove;
	Lv200AHandler handler;
};

/* HP の増減は共通部品へ（P3）。戦闘中の値は即時に動かし、HP バー・手持ち・ひんしは MOVE_END で台本が出す。 */
static void Lv200A_HealBank(u8 bank, u32 amount)
{
	if (IsHealBlocked(bank)) /* 回復ふうじ中は技・持ち物・特性の回復ができない */
		return;
	Lv200Common_HealBank(bank, amount);
}

static void Lv200A_HurtBank(u8 bank, u32 amount)
{
	Lv200Common_HurtBank(bank, amount);
}

/* 戦闘中の status1 を手持ちのデータへも送る（交代・戦闘後に戻らない）。 */
static void Lv200A_SyncStatus(u8 bank)
{
	if (bank >= gBattlersCount)
		return;
	gActiveBattler = bank;
	EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[bank].status1);
	MarkBufferBankForExecution(gActiveBattler);
}

/* 設置技(まきびし・どくびし・ステルスロック・ねばねばネット・はがねのトゲ)が残っているか（消す前の確認・文を出すかの判断）。 */
static bool8 Lv200A_SideHasHazards(u8 side)
{
	return (gSideStatuses[side] & SIDE_STATUS_SPIKES) != 0
		|| gSideTimers[side].spikesAmount != 0 || gSideTimers[side].tspikesAmount != 0
		|| gSideTimers[side].srAmount != 0 || gSideTimers[side].stickyWeb != 0
		|| gSideTimers[side].steelsurge != 0;
}

/* 設置技・壁・しんぴのまもり・しろいきり のどれかが残っているか。 */
static bool8 Lv200A_SideHasFieldEffects(u8 side)
{
	return Lv200A_SideHasHazards(side)
		|| (gSideStatuses[side] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_SAFEGUARD | SIDE_STATUS_MIST)) != 0
		|| gNewBS->AuroraVeilTimers[side] != 0;
}

static void Lv200A_ClearHazards(u8 side)
{
	gSideStatuses[side] &= ~SIDE_STATUS_SPIKES;
	gSideTimers[side].spikesAmount = 0;
	gSideTimers[side].tspikesAmount = 0;
	gSideTimers[side].srAmount = 0;
	gSideTimers[side].stickyWeb = 0;
	gSideTimers[side].steelsurge = 0;
	Lv200CustomMovesA_OnHazardsCleared(side);
}

static void Lv200A_ClearBattlefield(void)
{
	u8 side;

	for (side = 0; side < NUM_BATTLE_SIDES; ++side)
	{
		Lv200A_ClearHazards(side);
		RemoveScreensFromSide(side);
		gSideStatuses[side] &= ~(SIDE_STATUS_SAFEGUARD | SIDE_STATUS_MIST);
		gSideTimers[side].safeguardTimer = 0;
		gSideTimers[side].mistTimer = 0;
	}
}

static void Lv200A_ClearTargetField(u8 bank)
{
	u8 side;

	if (bank >= gBattlersCount)
		return;
	side = SIDE(bank);
	Lv200A_ClearHazards(side);
	RemoveScreensFromSide(side);
	gSideStatuses[side] &= ~(SIDE_STATUS_SAFEGUARD | SIDE_STATUS_MIST);
	gSideTimers[side].safeguardTimer = 0;
	gSideTimers[side].mistTimer = 0;
	gTerrainType = 0;
	gNewBS->TerrainTimer = 0;
}

static void Lv200A_SetRandomWeather(u8 bank)
{
    /* 今の天気以外の4つ（晴れ・雨・砂あらし・あられ）から等確率。おおひでり等の天気は上書きしない。 */
    static const u16 sWeatherAny[] = {WEATHER_SUN_ANY, WEATHER_RAIN_ANY, WEATHER_SANDSTORM_ANY, WEATHER_HAIL_ANY};
    u8 candidates[ARRAY_COUNT(sWeatherAny)];
    u8 count = 0;
    u8 i;

    if (gBattleWeather & (WEATHER_PRIMAL_ANY | WEATHER_CIRCUS))
        return;
    for (i = 0; i < ARRAY_COUNT(sWeatherAny); ++i)
        if (!(gBattleWeather & sWeatherAny[i]))
            candidates[count++] = i; /* LV200_WEATHER_SUN/RAIN/SAND/HAIL の順 */
    if (count == 0)
        return;
    Lv200Common_QueueWeather(bank, candidates[Random() % count]);
}

/* おいかぜ（4ターン）。すでにあれば何もしない。起こしたら素の文と、かぜのりの特性の反応。 */
static void Lv200A_StartTailwind(u8 bank)
{
    u8 side = SIDE(bank);
    u8 i;

    if (gNewBS->TailwindTimers[side] != 0)
        return;
    gNewBS->TailwindTimers[side] = 4;
    Lv200Common_QueueMessage(bank, TailwindSetString);
    for (i = 0; i < gBattlersCount; ++i)
    {
        if (SIDE(i) != side || gBattleMons[i].hp == 0)
            continue;
        if (GetBankAbility(i) == ABILITY_WINDRIDER)
            Lv200Common_QueueStat(i, i, STAT_STAGE_ATK, 1, FALSE);
        else if (GetBankAbility(i) == ABILITY_WINDPOWER)
            gStatuses3[i] |= STATUS3_CHARGED_UP;
    }
}

static u8 Lv200A_CountFaintedMons(u8 bank)
{
	struct Pokemon *party = SIDE(bank) == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	u8 i;
	u8 count = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
		u16 hp = GetMonData(&party[i], MON_DATA_HP, NULL);
		if (species != SPECIES_NONE && hp == 0)
			++count;
	}
	return count;
}

static u8 Lv200A_CountLivingMons(u8 bank)
{
	struct Pokemon *party = SIDE(bank) == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	u8 i;
	u8 count = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
		u16 hp = GetMonData(&party[i], MON_DATA_HP, NULL);
		if (species != SPECIES_NONE && hp != 0)
			++count;
	}
	return count;
}

static u8 Lv200A_CountLivingTypeMons(u8 bank, u8 type)
{
	struct Pokemon *party = SIDE(bank) == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	u8 i;
	u8 count = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
		u16 hp = GetMonData(&party[i], MON_DATA_HP, NULL);
		if (species == SPECIES_NONE || hp == 0)
			continue;
		{
			/* 場に出ている個体は今のタイプ（メガシンカ・テラスタル・タイプ変化後）で数える */
			u8 b;
			bool8 onField = FALSE;
			for (b = 0; b < gBattlersCount; ++b)
			{
				if (SIDE(b) == SIDE(bank) && gBattlerPartyIndexes[b] == i && gBattleMons[b].hp != 0)
				{
					onField = TRUE;
					if (IsOfType(b, type))
						++count;
					break;
				}
			}
			if (!onField && IsMonOfType(&party[i], type))
				++count;
		}
	}
	return count;
}

static u32 Lv200A_TypeMultiplier(u8 moveType, u8 bankDef)
{
	u8 types[3];
	u8 i;
	u32 value = TYPE_MUL_NORMAL;

	types[0] = gBattleMons[bankDef].type1;
	types[1] = gBattleMons[bankDef].type2;
	types[2] = gBattleMons[bankDef].type3;
	{
		/* テラスタル後は、相性の判定と同じく今のタイプ（テラスタイプ）だけで引く */
		u8 teraType = GetBattlerTeraType(bankDef);
		if (!IS_BLANK_TYPE(teraType) && teraType != TYPE_STELLAR)
			types[0] = types[1] = teraType, types[2] = TYPE_BLANK;
	}
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

static bool8 Lv200A_TypeBlockedByAbility(u8 type, u8 bankDef, u32 multiplier)
{
	ability_t ability = GetBankAbility(bankDef);

	switch (ability)
	{
	case ABILITY_LEVITATE:
	case ABILITY_EARTHEATER:
		return type == TYPE_GROUND;
	case ABILITY_FLASHFIRE:
		return type == TYPE_FIRE;
	case ABILITY_WATERABSORB:
	case ABILITY_DRYSKIN:
	case ABILITY_STORMDRAIN:
		return type == TYPE_WATER;
	case ABILITY_VOLTABSORB:
	case ABILITY_MOTORDRIVE:
	case ABILITY_LIGHTNINGROD:
		return type == TYPE_ELECTRIC;
	case ABILITY_SAPSIPPER:
		return type == TYPE_GRASS;
	case ABILITY_WONDERGUARD:
		return multiplier <= TYPE_MUL_NORMAL;
	default:
		return FALSE;
	}
}

/* りんぷんのかぜ: ぼうじん・ぼうじんゴーグルの相手には効果なし（粉を無視する）。 */
static bool8 Lv200A_PowderCurtainAffects(u8 bankAtk)
{
	return GetBankAbility(bankAtk) != ABILITY_OVERCOAT
		&& ITEM_EFFECT(bankAtk) != ITEM_EFFECT_SAFETY_GOGGLES;
}

static void PrepareMove(u16 move)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 bank = gBankAttacker;

	if (state == NULL || bank >= MAX_BATTLERS_COUNT)
		return;

	state->pendingMove[bank] = move;
	state->pendingTarget[bank] = gBankTarget;
	if (move == MOVE_LV200_GROUND_120
	&& (gBattleMons[bank].status2 & STATUS2_MULTIPLETURNS)
	&& !gNewBS->MultiHitOn)
	{
		gMultiHitCounter = 2;
		PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0); /* #41 */
	}
}

static bool8 Lv200A_UsesMultiScript(u16 move)
{
	return move == MOVE_LV200_NORMAL_110
		|| move == MOVE_LV200_POISON_190
		|| move == MOVE_LV200_ROCK_200
		|| move == MOVE_LV200_STEEL_150;
}

void Lv200A_PrepareMultiHit(void)
{
	u8 count = 1;

	if (gNewBS->MultiHitOn)
		return;
	switch (gCurrentMove)
	{
	case MOVE_LV200_NORMAL_110:
		count = Lv200A_CountLivingMons(gBankAttacker);
		if (count < 1)
			count = 1;
		if (count > PARTY_SIZE)
			count = PARTY_SIZE;
		break;
	case MOVE_LV200_POISON_190:
		count = 3 + Random() % 3;
		break;
	case MOVE_LV200_ROCK_200:
		count = 5;
		break;
	case MOVE_LV200_STEEL_150:
		count = 3;
		break;
	}
	gMultiHitCounter = count;
	PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0); /* 「N かい あたった！」の文の元（#41） */
}

void Lv200A_PrepareCounter(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u32 damage = 0;

	if (gBankAttacker >= gBattlersCount || gBankTarget >= gBattlersCount)
	{
		gBattleMoveDamage = 0;
		return;
	}
	if (gCurrentMove == MOVE_LV200_GROUND_190)
	{
		/* がんじょうで防げる（一撃必殺と同じ）。かたやぶり系は貫通する。 */
		if (GetBankAbility(gBankTarget) == ABILITY_STURDY
		&& !IsMoldBreakerAbility(ABILITY(gBankAttacker))
		&& !IsMoldBreakerAbility(SecondAbility_MoldBreakerAbility(gBankAttacker)))
			damage = 0;
		else
			damage = gBattleMons[gBankTarget].hp;
	}
	else if (gCurrentMove == MOVE_LV200_FIGHTING_140)
	{
		/* カウンター／ミラーコートと同じ3条件: 味方から受けたダメージは返さない・返す先が生きている */
		u8 physBank = gProtectStructs[gBankAttacker].physicalBank;
		u8 specBank = gProtectStructs[gBankAttacker].specialBank;
		if (gProtectStructs[gBankAttacker].physicalDmg != 0
		&& physBank < gBattlersCount && SIDE(physBank) != SIDE(gBankAttacker) && gBattleMons[physBank].hp != 0)
		{
			damage = gProtectStructs[gBankAttacker].physicalDmg;
			gBankTarget = physBank;
		}
		else if (gProtectStructs[gBankAttacker].specialDmg != 0
		&& specBank < gBattlersCount && SIDE(specBank) != SIDE(gBankAttacker) && gBattleMons[specBank].hp != 0)
		{
			damage = gProtectStructs[gBankAttacker].specialDmg;
			gBankTarget = specBank;
		}
		if (damage > 0x7FFFFFFF)
			damage = 0xFFFFFFFF;
		else
			damage *= 2;
	}
	else if (gCurrentMove == MOVE_LV200_FIGHTING_200 && state != NULL)
	{
		damage = state->physicalDamageHistory[gBankAttacker][0]
			+ state->physicalDamageHistory[gBankAttacker][1];
		if (damage > 0x55555555)
			damage = 0xFFFFFFFF;
		else
			damage *= 3;
		if (damage > gBattleMons[gBankTarget].maxHP)
			damage = gBattleMons[gBankTarget].maxHP;
	}
	gBattleMoveDamage = damage;
}

static bool8 Lv200A_Ground190CanOHKO(void)
{
	return gBankTarget < gBattlersCount
		&& gBattleMons[gBankTarget].hp * 2 <= gBattleMons[gBankTarget].maxHP
		&& !IsDynamaxed(gBankTarget)
		&& !(IsRaidBattle() && gBankTarget == BANK_RAID_BOSS);
}

void Lv200A_ToggleBatonState(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 bank = gBankAttacker;
	u8 stat;
	u32 low = 0;
	u32 high = 0;

	if (state == NULL || bank >= gBattlersCount)
		return;
	if (state->rockHitIndex[bank] == 0)
	{
		for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_SPATK; ++stat)
			low |= (u32) gBattleMons[bank].statStages[stat - 1] << ((stat - 1) * 4);
		for (stat = STAT_STAGE_SPDEF; stat <= STAT_STAGE_EVASION; ++stat)
			high |= (u32) gBattleMons[bank].statStages[stat - 1] << ((stat - STAT_STAGE_SPDEF) * 4);
		state->physicalDamageHistory[bank][0] = low;
		state->physicalDamageHistory[bank][1] = high;
		state->rockHitIndex[bank] = 1;
	}
	else
	{
		low = state->physicalDamageHistory[bank][0];
		high = state->physicalDamageHistory[bank][1];
		for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_SPATK; ++stat)
			gBattleMons[bank].statStages[stat - 1] = (low >> ((stat - 1) * 4)) & 0xF;
		for (stat = STAT_STAGE_SPDEF; stat <= STAT_STAGE_EVASION; ++stat)
			gBattleMons[bank].statStages[stat - 1]
				= (high >> ((stat - STAT_STAGE_SPDEF) * 4)) & 0xF;
		state->physicalDamageHistory[bank][0] = 0;
		state->physicalDamageHistory[bank][1] = 0;
		state->rockHitIndex[bank] = 0;
	}
}

void Lv200A_ApplyPerishTarget(void)
{
	if (gBankTarget >= gBattlersCount)
		return;
	gStatuses3[gBankTarget] |= STATUS3_PERISH_SONG;
	gDisableStructs[gBankTarget].perishSongTimer = 3;
	gDisableStructs[gBankTarget].perishSongTimerStartValue = 3;
}

static bool8 Lv200A_UsesPlainDamageScript(u16 move)
{
	switch (move)
	{
	case MOVE_LV200_NORMAL_120:
	case MOVE_LV200_NORMAL_170:
	case MOVE_LV200_NORMAL_180:
	case MOVE_LV200_FIGHTING_110:
	case MOVE_LV200_FIGHTING_130:
	case MOVE_LV200_FIGHTING_160:
	case MOVE_LV200_FLYING_110:
	case MOVE_LV200_FLYING_130:
	case MOVE_LV200_FLYING_140:
	case MOVE_LV200_FLYING_160:
	case MOVE_LV200_FLYING_170:
	case MOVE_LV200_POISON_120:
	case MOVE_LV200_POISON_140:
	case MOVE_LV200_POISON_150:
	case MOVE_LV200_POISON_180:
	case MOVE_LV200_GROUND_160:
	case MOVE_LV200_ROCK_140:
	case MOVE_LV200_ROCK_160:
	case MOVE_LV200_BUG_120:
	case MOVE_LV200_BUG_130:
	case MOVE_LV200_BUG_150:
	case MOVE_LV200_BUG_160:
	case MOVE_LV200_BUG_200:
	case MOVE_LV200_GHOST_150:
	case MOVE_LV200_GHOST_190:
	case MOVE_LV200_STEEL_120:
	case MOVE_LV200_STEEL_200:
	case MOVE_LV200_FIRE_130:
	case MOVE_LV200_FIRE_140:
	case MOVE_LV200_FIRE_170:
	case MOVE_LV200_FIRE_180:
		return TRUE;
	default:
		return FALSE;
	}
}

static bool8 Lv200A_UsesCustomStatusScript(u16 move)
{
	switch (move)
	{
	case MOVE_LV200_NORMAL_130:
	case MOVE_LV200_NORMAL_150:
	case MOVE_LV200_NORMAL_160:
	case MOVE_LV200_NORMAL_190:
	case MOVE_LV200_FIGHTING_120:
	case MOVE_LV200_FIGHTING_150:
	case MOVE_LV200_FIGHTING_170:
	case MOVE_LV200_FLYING_180:
	case MOVE_LV200_FLYING_190:
	case MOVE_LV200_GROUND_130:
	case MOVE_LV200_POISON_130:
	case MOVE_LV200_POISON_170:
	case MOVE_LV200_POISON_200:
	case MOVE_LV200_GROUND_140:
	case MOVE_LV200_GROUND_180:
	case MOVE_LV200_GROUND_200:
	case MOVE_LV200_ROCK_170:
	case MOVE_LV200_ROCK_180:
	case MOVE_LV200_ROCK_190:
	case MOVE_LV200_BUG_110:
	case MOVE_LV200_BUG_170:
	case MOVE_LV200_BUG_180:
	case MOVE_LV200_BUG_190:
	case MOVE_LV200_GHOST_120:
	case MOVE_LV200_GHOST_130:
	case MOVE_LV200_GHOST_140:
	case MOVE_LV200_GHOST_170:
	case MOVE_LV200_GHOST_180:
	case MOVE_LV200_STEEL_110:
	case MOVE_LV200_STEEL_140:
	case MOVE_LV200_STEEL_160:
	case MOVE_LV200_STEEL_170:
	case MOVE_LV200_STEEL_180:
	case MOVE_LV200_STEEL_190:
	case MOVE_LV200_FIRE_120:
	case MOVE_LV200_FIRE_160:
	case MOVE_LV200_FIRE_190:
		return TRUE;
	default:
		return FALSE;
	}
}

static bool8 Lv200A_HasNegativeStage(u8 bank)
{
	u8 stat;

	for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
		if (gBattleMons[bank].statStages[stat - 1] < 6)
			return TRUE;
	return FALSE;
}

/* ヴェノムシフト: recipient が source の status を受け取れるか（タイプ・特性・テレイン等の無効判定を素の関数で）。 */
static bool8 Lv200A_CanReceiveStatus(u8 recipient, u8 source, u32 status)
{
	u32 saved = gBattleMons[recipient].status1;
	bool8 ok = TRUE;

	if (status == 0)
		return TRUE;
	gBattleMons[recipient].status1 = 0;
	if (status & STATUS1_SLEEP)
		ok = CanBePutToSleep(recipient, source, TRUE);
	else if (status & STATUS1_FREEZE)
		ok = CanBeFrozen(recipient, source, TRUE);
	else if (status & STATUS1_PARALYSIS)
		ok = CanBeParalyzed(recipient, source, TRUE);
	else if (status & STATUS1_BURN)
		ok = CanBeBurned(recipient, source, TRUE);
	else if (status & (STATUS1_POISON | STATUS1_TOXIC_POISON))
		ok = CanBePoisoned(recipient, source, TRUE);
	gBattleMons[recipient].status1 = saved;
	return ok;
}

static bool8 Lv200A_AllStagesMax(u8 bank)
{
	u8 stat;

	for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
		if (gBattleMons[bank].statStages[stat - 1] < STAT_STAGE_MAX)
			return FALSE;
	return TRUE;
}

void Lv200A_CheckStatusMove(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 bank = gBankAttacker;
	bool8 usable = state != NULL && bank < gBattlersCount;
	u32 cost;

	Lv200Common_Clear(); /* 前の技の積み残しを持ち越さない */
	if (!usable)
	{
		gBattleMoveDamage = 0;
		return;
	}
	switch (gCurrentMove)
	{
	case MOVE_LV200_NORMAL_130:
		cost = (gBattleMons[bank].maxHP + 3) / 4;
		usable = gBattleMons[bank].hp >= cost;
		break;
	case MOVE_LV200_NORMAL_150:
		/* みがわりで防げる */
		usable = gBankTarget < gBattlersCount
			&& !(gBattleMons[gBankTarget].status2 & STATUS2_SUBSTITUTE);
		break;
	case MOVE_LV200_NORMAL_160:
		usable = !Lv200A_AllStagesMax(bank);
		break;
	case MOVE_LV200_POISON_170:
		/* 入れ替える先がタイプ・特性・しんぴのまもりで受けられないときは入れ替わらず失敗 */
		usable = gBankTarget < gBattlersCount
			&& (gBattleMons[bank].status1 != 0 || gBattleMons[gBankTarget].status1 != 0)
			&& Lv200A_CanReceiveStatus(gBankTarget, bank, gBattleMons[bank].status1)
			&& Lv200A_CanReceiveStatus(bank, gBankTarget, gBattleMons[gBankTarget].status1)
			&& !(gBattleMons[bank].status1 != 0 && BankSideHasSafeguard(gBankTarget));
		break;
	case MOVE_LV200_GROUND_140:
		usable = state->mudFieldTimer == 0;
		break;
	case MOVE_LV200_GROUND_200:
		usable = gBankTarget < gBattlersCount
			&& !(state->sandHazardBits & gBitTable[SIDE(gBankTarget)]);
		break;
	case MOVE_LV200_GROUND_180:
		usable = (gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
			|| gBattleMons[bank].status1 != 0
			|| (gBattleMons[bank].status2 & STATUS2_CONFUSION);
		break;
	case MOVE_LV200_ROCK_150:
		usable = gBankTarget < gBattlersCount
			&& !(state->rockCurtainBits & gBitTable[SIDE(gBankTarget)]);
		break;
	case MOVE_LV200_ROCK_170:
		usable = (gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
			|| !Lv200A_AllStagesMax(bank);
		break;
	case MOVE_LV200_STEEL_190:
		/* 回復ふうじ中・満タンで、戻すマイナスのランクも無ければ失敗 */
		usable = (gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
			|| Lv200A_HasNegativeStage(bank);
		break;
	case MOVE_LV200_GHOST_170:
		usable = gBankTarget < gBattlersCount
			&& gBattleMons[bank].hp > gBattleMons[bank].maxHP / 2;
		break;
	case MOVE_LV200_GHOST_120:
		usable = gBankTarget < gBattlersCount && !IsOfType(gBankTarget, TYPE_GHOST);
		break;
	case MOVE_LV200_GHOST_130:
		cost = (gBattleMons[bank].maxHP + 7) / 8;
		usable = gDisableStructs[bank].substituteHP == 0
			&& gBattleMons[bank].hp > cost;
		break;
	case MOVE_LV200_BUG_190:
		usable = gBankTarget < gBattlersCount && !IsOfType(gBankTarget, TYPE_GRASS)
			&& !(gBattleMons[gBankTarget].status2 & STATUS2_SUBSTITUTE); /* みがわりで防げる */
		break;
	case MOVE_LV200_STEEL_180:
		usable = gBattleMons[bank].statStages[STAT_STAGE_DEF - 1] != 6
			|| gBattleMons[bank].statStages[STAT_STAGE_SPEED - 1] != 6;
		break;
	default:
		break;
	}
	gBattleMoveDamage = usable;
}

static void Lv200A_HealParty(u8 bank)
{
	struct Pokemon *party = SIDE(bank) == B_SIDE_PLAYER ? gPlayerParty : gEnemyParty;
	u8 i;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&party[i], MON_DATA_SPECIES, NULL);
		u16 hp = GetMonData(&party[i], MON_DATA_HP, NULL);
		u16 maxHP = GetMonData(&party[i], MON_DATA_MAX_HP, NULL);
		u32 status = 0;
		if (species == SPECIES_NONE || hp == 0)
			continue;
		hp += maxHP / 3;
		if (hp > maxHP)
			hp = maxHP;
		SetMonData(&party[i], MON_DATA_HP, &hp);
		SetMonData(&party[i], MON_DATA_STATUS, &status);
	}
	for (i = 0; i < gBattlersCount; ++i)
	{
		if (SIDE(i) != SIDE(bank) || gBattleMons[i].hp == 0)
			continue;
		Lv200A_HealBank(i, gBattleMons[i].maxHP / 3);
		Lv200Common_QueueCureStatus(i); /* 手持ちへ送り、文と枠の印を出す（#4 の形） */
	}
}

void Lv200A_ApplyStatusMove(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 bank = gBankAttacker;
	u8 target = gBankTarget;
	u8 stat;
	u8 chosen;
	u8 order[] =
	{
		STAT_STAGE_SPEED, STAT_STAGE_ATK, STAT_STAGE_SPATK, STAT_STAGE_DEF,
		STAT_STAGE_SPDEF, STAT_STAGE_ACC, STAT_STAGE_EVASION,
	};

	if (state == NULL || bank >= gBattlersCount)
		return;
	switch (gCurrentMove)
	{
	case MOVE_LV200_NORMAL_130:
		Lv200A_HurtBank(bank, (gBattleMons[bank].maxHP + 3) / 4);
		Lv200A_HealParty(bank);
		break;
	case MOVE_LV200_NORMAL_160:
		chosen = order[0];
		for (stat = 1; stat < ARRAY_COUNT(order); ++stat)
			if (gBattleMons[bank].statStages[order[stat] - 1]
			< gBattleMons[bank].statStages[chosen - 1])
				chosen = order[stat];
		Lv200Common_QueueStat(bank, bank, chosen, 3, FALSE); /* あまのじゃく(-3)・たんじゅん・+6 の文は素の流れ */
		break;
	case MOVE_LV200_NORMAL_190:
		if (target < gBattlersCount)
		{
			u32 average;
#define LV200_A_AVERAGE_FIELD(field) \
			average = ((u32) gBattleMons[bank].field + gBattleMons[target].field) / 2; \
			gBattleMons[bank].field = average; \
			gBattleMons[target].field = average
			LV200_A_AVERAGE_FIELD(attack);
			LV200_A_AVERAGE_FIELD(defense);
			LV200_A_AVERAGE_FIELD(speed);
			LV200_A_AVERAGE_FIELD(spAttack);
			LV200_A_AVERAGE_FIELD(spDefense);
#undef LV200_A_AVERAGE_FIELD
		}
		break;
	case MOVE_LV200_FIGHTING_120:
		gBattleMons[bank].status2 |= STATUS2_FOCUS_ENERGY; /* 急所+1はきあいだめと同じ扱い（文は新しい文・素のきあいだめの文は借りない・Q30） */
		Lv200Common_QueueMessage(bank, gText_Lv200A_FocusMind);
		break;
	case MOVE_LV200_FIGHTING_170:
		/* 3ターンの構え。急所+1はきあいだめと別枠（damage_calc.c で加算）。新しい文（Q30）。 */
		Lv200Common_QueueMessage(bank, gText_Lv200A_StanceSet);
		break;
	case MOVE_LV200_FLYING_180:
		/* あめ（5ターン）とおいかぜ（4ターン）。すでにある方はそのまま、もう一方は起きる。 */
		Lv200Common_QueueWeather(bank, LV200_WEATHER_RAIN);
		Lv200A_StartTailwind(bank);
		break;
	case MOVE_LV200_GROUND_130:
		/* すなあらし・HP1/4・ぼうぎょ+1。天気が変わらなくても回復と上昇は起きる。 */
		Lv200Common_QueueWeather(bank, LV200_WEATHER_SAND);
		Lv200A_HealBank(bank, gBattleMons[bank].maxHP / 4);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_DEF, 1, FALSE);
		break;
	case MOVE_LV200_FIRE_160:
		/* はれ・HP2/3・こうげき／とくこう+1。天気が変わらなくても回復と上昇は起きる。 */
		Lv200Common_QueueWeather(bank, LV200_WEATHER_SUN);
		Lv200A_HealBank(bank, (gBattleMons[bank].maxHP * 2) / 3);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_ATK, 1, TRUE);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPATK, 1, TRUE);
		break;
	case MOVE_LV200_FIGHTING_150:
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_ATK, 1, FALSE);
		if (target < gBattlersCount)
			Lv200Common_QueueStat(target, bank, STAT_STAGE_ATK, 1, FALSE);
		break;
	case MOVE_LV200_POISON_130:
		chosen = 1;
		for (stat = 0; stat < gBattlersCount; ++stat)
			if (gBattleMons[stat].status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON))
				chosen = 2;
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPATK, chosen, FALSE);
		break;
	case MOVE_LV200_POISON_170:
		if (target < gBattlersCount)
		{
			u32 status = gBattleMons[bank].status1;
			gBattleMons[bank].status1 = gBattleMons[target].status1;
			gBattleMons[target].status1 = status;
			Lv200A_SyncStatus(bank); /* 手持ちのデータへも送る */
			Lv200A_SyncStatus(target);
			Lv200Common_QueueMessageFrom(target, bank, gText_Lv200A_VenomShift);
		}
		break;
	case MOVE_LV200_POISON_200:
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_DEF, 1, TRUE);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPDEF, 1, TRUE);
		break;
	case MOVE_LV200_ROCK_180:
		gTerrainType = 0;
		gNewBS->TerrainTimer = 0;
		break;
	case MOVE_LV200_BUG_180:
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_DEF, 1, FALSE);
		break;
	case MOVE_LV200_BUG_110:
		if (!(state->pupaBits & gBitTable[bank]))
		{
			Lv200Common_QueueMessage(bank, gText_Lv200A_PupaSet);
			Lv200Common_QueueStat(bank, bank, STAT_STAGE_DEF, 2, TRUE);
			Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPDEF, 2, TRUE);
			gProtectStructs[bank].protected = TRUE;
			state->pupaBits |= gBitTable[bank];
			state->pupaTimer[bank] = 3;
			gBattleMons[bank].status2 |= STATUS2_MULTIPLETURNS;
			gLockedMoves[bank] = gCurrentMove;
		}
		else
		{
			Lv200Common_QueueMessage(bank, gText_Lv200A_Emerge);
			Lv200Common_QueueStat(bank, bank, STAT_STAGE_ATK, 1, TRUE);
			Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPATK, 1, TRUE);
			Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPEED, 1, TRUE);
			Lv200A_HealBank(bank, gBattleMons[bank].maxHP / 4);
			state->pupaBits &= ~gBitTable[bank];
			state->pupaTimer[bank] = 0;
			gBattleMons[bank].status2 &= ~STATUS2_MULTIPLETURNS;
			gLockedMoves[bank] = MOVE_NONE;
		}
		break;
	case MOVE_LV200_STEEL_110:
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_DEF, 1, TRUE);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPDEF, 1, TRUE);
		break;
	case MOVE_LV200_STEEL_180:
		chosen = gBattleMons[bank].statStages[STAT_STAGE_DEF - 1];
		gBattleMons[bank].statStages[STAT_STAGE_DEF - 1]
			= gBattleMons[bank].statStages[STAT_STAGE_SPEED - 1];
		gBattleMons[bank].statStages[STAT_STAGE_SPEED - 1] = chosen;
		if (GetBankAbility(bank) == ABILITY_CONTRARY) /* あまのじゃくでは反転して入れ替わる */
		{
			gBattleMons[bank].statStages[STAT_STAGE_DEF - 1] = 12 - gBattleMons[bank].statStages[STAT_STAGE_DEF - 1];
			gBattleMons[bank].statStages[STAT_STAGE_SPEED - 1] = 12 - gBattleMons[bank].statStages[STAT_STAGE_SPEED - 1];
		}
		Lv200Common_QueueMessage(bank, gText_Lv200A_GearSwap);
		break;
	case MOVE_LV200_STEEL_190:
		Lv200A_HealBank(bank, gBattleMons[bank].maxHP / 2);
		break;
	case MOVE_LV200_FIRE_120:
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPATK, 1, FALSE);
		break;
	case MOVE_LV200_GROUND_180:
		/* HP回復量は gBattleMoveDamage に負数で渡し、台本側で HPバー更新と文言を出す。 */
		gBattleMoveDamage = 0;
		if (gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
		{
			u32 heal = gBattleMons[bank].maxHP / 2;
			if (heal == 0)
				heal = 1;
			if (heal > gBattleMons[bank].maxHP - gBattleMons[bank].hp)
				heal = gBattleMons[bank].maxHP - gBattleMons[bank].hp;
			gBattleMoveDamage = -(s32) heal;
		}
		/* 状態異常・混乱の治療と防御/特防の上昇は、台本側で文とアニメつきで行う。 */
		break;
	case MOVE_LV200_ROCK_170:
		/* HP回復量は gBattleMoveDamage に負数で渡し、台本側で HPバー更新と文言を出す。 */
		gBattleMoveDamage = 0;
		if (gBattleMons[bank].hp < gBattleMons[bank].maxHP && !IsHealBlocked(bank))
		{
			u32 heal = gBattleMons[bank].maxHP / 2;
			if (heal == 0)
				heal = 1;
			if (heal > gBattleMons[bank].maxHP - gBattleMons[bank].hp)
				heal = gBattleMons[bank].maxHP - gBattleMons[bank].hp;
			gBattleMoveDamage = -(s32) heal;
		}
		/* 5能力の上昇は、台本側で文とアニメつきで行う。 */
		break;
	case MOVE_LV200_FIRE_190:
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_ATK, 1, TRUE);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPATK, 1, TRUE);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPEED, 1, TRUE);
		break;
	case MOVE_LV200_ROCK_150:
		/* いしくれのまく: ステルスロックとは別の専用の枠（rockCurtainBits）。置く文はステルスロックと同じ。 */
		if (target < gBattlersCount)
		{
			state->rockCurtainBits |= gBitTable[SIDE(target)];
			gSideStatuses[SIDE(target)] |= SIDE_STATUS_SPIKES; /* やきはらい・こうそくスピンで消える印 */
			gBattleStringLoader = StealthRockLayString;
		}
		break;
	default:
		break;
	}
}

/* だいちのめぐみ: 状態異常を治す。戦闘中の値だけでなく手持ちのデータにも送る
 * （素の atkCD_cureifburnedparalysedorpoisoned／battle_util.c の治療と同じ作り）。 */
void Lv200A_CureStatusGround180(void)
{
	u8 bank = gBankAttacker;

	if (bank >= gBattlersCount)
		return;
	Lv200Common_CureStatusBank(bank);
}

void Lv200A_CureConfusionGround180(void)
{
	Lv200Common_CureConfusionBank(gBankAttacker);
}

void Lv200A_PrepareFighting180(void)
{
	u8 cured = 0;

	if (gBankAttacker >= gBattlersCount)
		return;
	if (gBattleMons[gBankAttacker].status1 != 0)
	{
		Lv200Common_QueueCureStatus(gBankAttacker); /* 手持ちへも送り、文は MOVE_END で（交代・戦闘後に戻らない） */
		++cured;
	}
	if (gBattleMons[gBankAttacker].status2 & STATUS2_CONFUSION)
	{
		Lv200Common_QueueCureConfusion(gBankAttacker);
		++cured;
	}
	if (cured > 3)
		cured = 3;
	if (cured != 0)
		Lv200Common_QueueStat(gBankAttacker, gBankAttacker, STAT_STAGE_ATK, cured, FALSE);
}

#define LV200_A_MOVES(X) \
	X(MOVE_LV200_NORMAL_110, MOVE_BEATUP, normal_110) \
	X(MOVE_LV200_NORMAL_120, MOVE_JUDGMENT, normal_120) \
	X(MOVE_LV200_NORMAL_130, MOVE_HEALBELL, normal_130) \
	X(MOVE_LV200_NORMAL_140, MOVE_HAZE, normal_140) \
	X(MOVE_LV200_NORMAL_150, MOVE_PERISHSONG, normal_150) \
	X(MOVE_LV200_NORMAL_160, MOVE_GROWTH, normal_160) \
	X(MOVE_LV200_NORMAL_170, MOVE_TRIATTACK, normal_170) \
	X(MOVE_LV200_NORMAL_180, MOVE_ECHOEDVOICE, normal_180) \
	X(MOVE_LV200_NORMAL_190, MOVE_GUARDSPLIT, normal_190) \
	X(MOVE_LV200_NORMAL_200, MOVE_UTURN, normal_200) \
	X(MOVE_LV200_FIGHTING_110, MOVE_FURYCUTTER, fighting_110) \
	X(MOVE_LV200_FIGHTING_120, MOVE_LASERFOCUS, fighting_120) \
	X(MOVE_LV200_FIGHTING_130, MOVE_BODYPRESS, fighting_130) \
	X(MOVE_LV200_FIGHTING_140, MOVE_COUNTER, fighting_140) \
	X(MOVE_LV200_FIGHTING_150, MOVE_MEANLOOK, fighting_150) \
	X(MOVE_LV200_FIGHTING_160, MOVE_HEAVYSLAM, fighting_160) \
	X(MOVE_LV200_FIGHTING_170, MOVE_FOCUSENERGY, fighting_170) \
	X(MOVE_LV200_FIGHTING_180, MOVE_REFRESH, fighting_180) \
	X(MOVE_LV200_FIGHTING_190, MOVE_ENDURE, fighting_190) \
	X(MOVE_LV200_FIGHTING_200, MOVE_COUNTER, fighting_200) \
	X(MOVE_LV200_FLYING_110, MOVE_TAILWIND, flying_110) \
	X(MOVE_LV200_FLYING_120, MOVE_MAGNETRISE, flying_120) \
	X(MOVE_LV200_FLYING_130, MOVE_WEATHERBALL, flying_130) \
	X(MOVE_LV200_FLYING_140, MOVE_QUICKATTACK, flying_140) \
	X(MOVE_LV200_FLYING_150, MOVE_PROTECT, flying_150) \
	X(MOVE_LV200_FLYING_160, MOVE_BRAVEBIRD, flying_160) \
	X(MOVE_LV200_FLYING_170, MOVE_SMACKDOWN, flying_170) \
	X(MOVE_LV200_FLYING_180, MOVE_RAINDANCE, flying_180) \
	X(MOVE_LV200_FLYING_190, MOVE_SUBSTITUTE, flying_190) \
	X(MOVE_LV200_FLYING_200, MOVE_MAGNETRISE, flying_200) \
	X(MOVE_LV200_POISON_110, MOVE_SLUDGEWAVE, poison_110) \
	X(MOVE_LV200_POISON_120, MOVE_VENOSHOCK, poison_120) \
	X(MOVE_LV200_POISON_130, MOVE_QUIVERDANCE, poison_130) \
	X(MOVE_LV200_POISON_140, MOVE_HEALBLOCK, poison_140) \
	X(MOVE_LV200_POISON_150, MOVE_CORROSIVEGAS, poison_150) \
	X(MOVE_LV200_POISON_160, MOVE_POISONFANG, poison_160) \
	X(MOVE_LV200_POISON_170, MOVE_PSYCHOSHIFT, poison_170) \
	X(MOVE_LV200_POISON_180, MOVE_VENOSHOCK, poison_180) \
	X(MOVE_LV200_POISON_190, MOVE_PINMISSILE, poison_190) \
	X(MOVE_LV200_POISON_200, MOVE_ACIDARMOR, poison_200) \
	X(MOVE_LV200_GROUND_110, MOVE_DRAINPUNCH, ground_110) \
	X(MOVE_LV200_GROUND_120, MOVE_DIG, ground_120) \
	X(MOVE_LV200_GROUND_130, MOVE_SANDSTORM, ground_130) \
	X(MOVE_LV200_GROUND_140, MOVE_MUDSPORT, ground_140) \
	X(MOVE_LV200_GROUND_150, MOVE_SANDTOMB, ground_150) \
	X(MOVE_LV200_GROUND_160, MOVE_STEELROLLER, ground_160) \
	X(MOVE_LV200_GROUND_170, MOVE_EARTHQUAKE, ground_170) \
	X(MOVE_LV200_GROUND_180, MOVE_RECOVER, ground_180) \
	X(MOVE_LV200_GROUND_190, MOVE_FISSURE, ground_190) \
	X(MOVE_LV200_GROUND_200, MOVE_SPIKES, ground_200) \
	X(MOVE_LV200_ROCK_110, MOVE_ENDURE, rock_110) \
	X(MOVE_LV200_ROCK_120, MOVE_BODYPRESS, rock_120) \
	X(MOVE_LV200_ROCK_130, MOVE_PAYDAY, rock_130) \
	X(MOVE_LV200_ROCK_140, MOVE_ROCKSMASH, rock_140) \
	X(MOVE_LV200_ROCK_150, MOVE_STEALTHROCK, rock_150) \
	X(MOVE_LV200_ROCK_160, MOVE_RAPIDSPIN, rock_160) \
	X(MOVE_LV200_ROCK_170, MOVE_RECOVER, rock_170) \
	X(MOVE_LV200_ROCK_180, MOVE_GRASSYTERRAIN, rock_180) \
	X(MOVE_LV200_ROCK_190, MOVE_LIGHTSCREEN, rock_190) \
	X(MOVE_LV200_ROCK_200, MOVE_ROCKBLAST, rock_200) \
	X(MOVE_LV200_BUG_110, MOVE_HARDEN, bug_110) \
	X(MOVE_LV200_BUG_120, MOVE_STRINGSHOT, bug_120) \
	X(MOVE_LV200_BUG_130, MOVE_ATTACKORDER, bug_130) \
	X(MOVE_LV200_BUG_140, MOVE_SHELLSMASH, bug_140) \
	X(MOVE_LV200_BUG_150, MOVE_STICKYWEB, bug_150) \
	X(MOVE_LV200_BUG_160, MOVE_FURYCUTTER, bug_160) \
	X(MOVE_LV200_BUG_170, MOVE_POWDER, bug_170) \
	X(MOVE_LV200_BUG_180, MOVE_IRONDEFENSE, bug_180) \
	X(MOVE_LV200_BUG_190, MOVE_LEECHSEED, bug_190) \
	X(MOVE_LV200_BUG_200, MOVE_BUGBUZZ, bug_200) \
	X(MOVE_LV200_GHOST_110, MOVE_SHADOWSNEAK, ghost_110) \
	X(MOVE_LV200_GHOST_120, MOVE_MEANLOOK, ghost_120) \
	X(MOVE_LV200_GHOST_130, MOVE_SUBSTITUTE, ghost_130) \
	X(MOVE_LV200_GHOST_140, MOVE_GRUDGE, ghost_140) \
	X(MOVE_LV200_GHOST_150, MOVE_LASTRESPECTS, ghost_150) \
	X(MOVE_LV200_GHOST_160, MOVE_PERISHSONG, ghost_160) \
	X(MOVE_LV200_GHOST_170, MOVE_DESTINYBOND, ghost_170) \
	X(MOVE_LV200_GHOST_180, MOVE_TRICKROOM, ghost_180) \
	X(MOVE_LV200_GHOST_190, MOVE_NOBLEROAR, ghost_190) \
	X(MOVE_LV200_GHOST_200, MOVE_ENDURE, ghost_200) \
	X(MOVE_LV200_STEEL_110, MOVE_IRONDEFENSE, steel_110) \
	X(MOVE_LV200_STEEL_120, MOVE_MAX_STEELSPIKE_P, steel_120) \
	X(MOVE_LV200_STEEL_130, MOVE_IRONDEFENSE, steel_130) \
	X(MOVE_LV200_STEEL_140, MOVE_MAGICCOAT, steel_140) \
	X(MOVE_LV200_STEEL_150, MOVE_TRIPLEAXEL, steel_150) \
	X(MOVE_LV200_STEEL_160, MOVE_REFLECT, steel_160) \
	X(MOVE_LV200_STEEL_170, MOVE_TAILWIND, steel_170) \
	X(MOVE_LV200_STEEL_180, MOVE_SPEEDSWAP, steel_180) \
	X(MOVE_LV200_STEEL_190, MOVE_RECOVER, steel_190) \
	X(MOVE_LV200_STEEL_200, MOVE_CORROSIVEGAS, steel_200) \
	X(MOVE_LV200_FIRE_110, MOVE_FIRESPIN, fire_110) \
	X(MOVE_LV200_FIRE_120, MOVE_CHARGE, fire_120) \
	X(MOVE_LV200_FIRE_130, MOVE_WILLOWISP, fire_130) \
	X(MOVE_LV200_FIRE_140, MOVE_INFERNO, fire_140) \
	X(MOVE_LV200_FIRE_150, MOVE_BURNINGBULWARK, fire_150) \
	X(MOVE_LV200_FIRE_160, MOVE_SUNNYDAY, fire_160) \
	X(MOVE_LV200_FIRE_170, MOVE_FREEZEDRY, fire_170) \
	X(MOVE_LV200_FIRE_180, MOVE_DEFOG, fire_180) \
	X(MOVE_LV200_FIRE_190, MOVE_FIERYDANCE, fire_190) \
	X(MOVE_LV200_FIRE_200, MOVE_SOLARBEAM, fire_200)

#define DEFINE_HANDLER(move, baseMove, key) \
	static void Lv200A_Handle_##key(void) { PrepareMove(move); }
LV200_A_MOVES(DEFINE_HANDLER)
#undef DEFINE_HANDLER

#define MOVE_ENTRY(move, baseMove, key) {move, baseMove, Lv200A_Handle_##key},
static const struct Lv200AMoveEntry sLv200AMoves[] =
{
	LV200_A_MOVES(MOVE_ENTRY)
};
#undef MOVE_ENTRY

const u8 *Lv200CustomMovesA_GetScript(u16 move)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sLv200AMoves); ++i)
	{
		if (sLv200AMoves[i].move == move)
		{
			sLv200AMoves[i].handler();
			if (Lv200A_UsesMultiScript(move))
				return BattleScript_Lv200A_Multi;
			if (move == MOVE_LV200_FIGHTING_140 || move == MOVE_LV200_FIGHTING_200)
				return BattleScript_Lv200A_Counter;
			if (move == MOVE_LV200_GROUND_190)
				return Lv200A_Ground190CanOHKO()
					? BattleScript_Lv200A_Counter : BattleScript_Lv200A_Damage;
			if (move == MOVE_LV200_NORMAL_200)
				return BattleScript_Lv200A_Pivot;
			if (move == MOVE_LV200_GHOST_160)
				return BattleScript_Lv200A_PerishPivot;
			if (move == MOVE_LV200_GROUND_180)
				return BattleScript_Lv200A_Ground180;
			if (move == MOVE_LV200_POISON_200)
				return BattleScript_Lv200A_PoisonKing;
			if (move == MOVE_LV200_ROCK_170)
				return BattleScript_Lv200A_Rock170;
			if (move == MOVE_LV200_ROCK_150)
				return BattleScript_Lv200A_RockCurtain;
			if (Lv200A_UsesCustomStatusScript(move))
				return BattleScript_Lv200A_Status;
			if (move == MOVE_LV200_GHOST_110)
				return gBattleScriptsForMoveEffects[gBattleMoves[MOVE_FEINT].effect];
			if (move == MOVE_LV200_FIGHTING_180)
				return BattleScript_Lv200A_Fighting180;
			if (Lv200A_UsesPlainDamageScript(move))
				return BattleScript_Lv200A_Damage;
			return gBattleScriptsForMoveEffects[gBattleMoves[sLv200AMoves[i].baseMove].effect];
		}
	}

	return NULL;
}

/* どくのおうさま: 特性名のポップアップを「ポイズンヒール」で出す（BANK_SCRIPTING＝自分）。 */
void Lv200A_SetPoisonKingPopup(void)
{
	gBattleScripting.bank = gBankAttacker;
	gNewBS->abilityPopupOverwrite = ABILITY_POISONHEAL;
}

/* 効果が切れる文（開始の文がある場・壁・構え・状態だけ・Q30/24）。
 * この終端のあとの Tick で 0 になる（タイマーが 1）ものを 1 件ずつ返す。group: 0-5=個体ごと, 6-9=側ごと(bank 0/1), 10=場全体。 */
enum
{
	LV200_A_EXPIRY_STANCE,
	LV200_A_EXPIRY_METAL,
	LV200_A_EXPIRY_MIRROR,
	LV200_A_EXPIRY_TURBO,
	LV200_A_EXPIRY_COCOON,
	LV200_A_EXPIRY_SOUL,
	LV200_A_EXPIRY_POWDER,
	LV200_A_EXPIRY_STEELWALL,
	LV200_A_EXPIRY_QUARTZ,
	LV200_A_EXPIRY_FLYING,
	LV200_A_EXPIRY_FIELD,
	LV200_A_EXPIRY_GROUPS,
};

static const u8 *Lv200A_GetExpiryScript(struct Lv200CustomMovesAState *state)
{
	while (state->expiryCursor < MAX_BATTLERS_COUNT * LV200_A_EXPIRY_GROUPS)
	{
		u8 cursor = state->expiryCursor++;
		u8 bank = cursor % MAX_BATTLERS_COUNT;
		const u8 *text = NULL;

		switch (cursor / MAX_BATTLERS_COUNT)
		{
		case LV200_A_EXPIRY_STANCE:
			if (state->stanceTimer[bank] == 1) text = gText_Lv200A_StanceEnd;
			break;
		case LV200_A_EXPIRY_METAL:
			if (state->metalArmorTimer[bank] == 1) text = gText_Lv200A_MetalArmorEnd;
			break;
		case LV200_A_EXPIRY_MIRROR:
			if (state->mirrorArmorTimer[bank] == 1) text = gText_Lv200A_MirrorArmorEnd;
			break;
		case LV200_A_EXPIRY_TURBO:
			if (state->turboTimer[bank] == 1) text = gText_Lv200A_TurboEnd;
			break;
		case LV200_A_EXPIRY_COCOON:
			if (state->cocoonArmorTimer[bank] == 1) text = gText_Lv200A_CocoonArmorEnd;
			break;
		case LV200_A_EXPIRY_SOUL:
			if (state->soulContractTimer[bank] == 1) text = gText_Lv200A_SoulContractEnd;
			break;
		case LV200_A_EXPIRY_POWDER:
			if (bank < NUM_BATTLE_SIDES && state->powderCurtainTimer[bank] == 1) text = gText_Lv200A_PowderCurtainEnd;
			break;
		case LV200_A_EXPIRY_STEELWALL:
			if (bank < NUM_BATTLE_SIDES && state->steelWallTimer[bank] == 1) text = gText_Lv200A_SteelWallEnd;
			break;
		case LV200_A_EXPIRY_QUARTZ:
			if (bank < NUM_BATTLE_SIDES && state->quartzTimer[bank] == 1) text = gText_Lv200A_QuartzEnd;
			break;
		case LV200_A_EXPIRY_FLYING:
			if (bank < NUM_BATTLE_SIDES && state->flyingBarrierTimer[bank] == 1) text = gText_Lv200A_FlyingBarrierEnd;
			break;
		case LV200_A_EXPIRY_FIELD:
			if (bank == 0 && state->mudFieldTimer == 1) text = gText_Lv200A_MudFieldEnd;
			else if (bank == 1 && state->rockFieldTimer == 1) text = gText_Lv200A_RockFieldEnd;
			else if (bank == 2 && state->darkVeilTimer == 1) text = gText_Lv200A_DarkVeilEnd;
			break;
		}
		if (text == NULL)
			continue;
		if (cursor / MAX_BATTLERS_COUNT <= LV200_A_EXPIRY_SOUL
		&& (bank >= gBattlersCount || gBattleMons[bank].hp == 0))
			continue;
		gBankAttacker = bank;
		gBankTarget = bank;
		gBattleStringLoader = text;
		return BattleScript_Lv200A_EndTurnMessage;
	}
	return NULL;
}

const u8 *Lv200CustomMovesA_GetEndTurnScript(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 cursor;
	u8 bank;
	u32 damage;

	if (state == NULL)
		return NULL;

	while (state->endTurnCursor < MAX_BATTLERS_COUNT * 12)
	{
		cursor = state->endTurnCursor++;
		bank = cursor % MAX_BATTLERS_COUNT;
		if (bank >= gBattlersCount || gBattleMons[bank].hp == 0)
			continue;

		damage = 0;
		switch (cursor / MAX_BATTLERS_COUNT)
		{
		case 0:
			if (state->finalCount[bank] == 1 && gBattleMons[bank].hp > 1)
			{
				damage = gBattleMons[bank].maxHP / 2;
				if (damage >= gBattleMons[bank].hp)
					damage = gBattleMons[bank].hp - 1;
			}
			break;
		case 1:
			if (state->venomLayers[bank] != 0)
			{
				damage = (gBattleMons[bank].maxHP * state->venomLayers[bank]) / 32;
				/* どくくぐつ: 層の数ぶんのダメージを、回復に変える */
				if (GetBankAbility(bank) == ABILITY_POISONPUPPETEER)
				{
					if (damage == 0)
						damage = 1;
					if (damage > gBattleMons[bank].maxHP - gBattleMons[bank].hp)
						damage = gBattleMons[bank].maxHP - gBattleMons[bank].hp;
					if (damage != 0 && !IsHealBlocked(bank))
					{
						gBankAttacker = bank;
						gBankTarget = bank;
						gBattleMoveDamage = -(s32) damage;
						return BattleScript_Lv200A_EndTurnHeal;
					}
					damage = 0;
				}
			}
			break;
		case 2:
			if (state->aftershockTimer != 0 && CheckGrounding(bank) == GROUNDED)
				damage = gBattleMons[bank].maxHP / 16;
			break;
		case 3:
			if (state->lostTimer[bank] != 0)
				damage = gBattleMons[bank].maxHP / 16;
			break;
		case 4:
			if (state->turboTimer[bank] != 0)
				damage = gBattleMons[bank].maxHP / 16;
			break;
		case 5:
			if (state->blazeFormTimer[bank] != 0)
				damage = gBattleMons[bank].maxHP / 16;
			break;
		case 6:
			if (state->eggTimer[bank] == 1 && state->eggOwner[bank] != 0)
			{
				u8 owner = state->eggOwner[bank] - 1;
				if (owner < gBattlersCount && gBattleMons[owner].hp != 0)
				{
					damage = gBattleMons[bank].maxHP / 4;
					if (damage > gBattleMons[bank].hp)
						damage = gBattleMons[bank].hp;
					gBankAttacker = bank;
					gBankTarget = owner;
					gBattleMoveDamage = damage;
					return BattleScript_Lv200A_EndTurnDrain;
				}
			}
			break;
		case 7:
			if (state->sandTrapOwner[bank] != 0
			&& (gBattleMons[bank].status2 & STATUS2_WRAPPED))
			{
				gBankAttacker = bank;
				gBankTarget = bank;
				return BattleScript_Lv200A_EndTurnDefDown;
			}
			break;
		case 8:
			if (state->burnMarkTimer[bank] != 0
			&& GetBankAbility(bank) != ABILITY_MAGICGUARD
			&& (gBattleMons[bank].status1 & STATUS1_BURN))
				damage = gBattleMons[bank].maxHP / 16;
			break;
		case 9:
			if (state->pendingMove[bank] == MOVE_LV200_NORMAL_180
			&& state->echoMove[bank] != MOVE_NONE
			&& state->echoDamage[bank] != 0
			&& state->pendingTarget[bank] < gBattlersCount
			&& gBattleMons[state->pendingTarget[bank]].hp != 0)
			{
				gBankAttacker = bank;
				gBankTarget = state->pendingTarget[bank];
				gCurrentMove = state->echoMove[bank];
				gMoveResultFlags = 0;
				gNewBS->ResultFlags[gBankTarget] = 0;
				return BattleScript_Lv200A_EchoDamage;
			}
			break;
		case 10:
			if (state->poisonKingTimer[bank] != 0
			&& (gBattleMons[bank].status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON))
			&& gBattleMons[bank].hp < gBattleMons[bank].maxHP
			&& !IsHealBlocked(bank))
			{
				damage = gBattleMons[bank].maxHP / 8;
				if (damage == 0)
					damage = 1;
				if (damage > gBattleMons[bank].maxHP - gBattleMons[bank].hp)
					damage = gBattleMons[bank].maxHP - gBattleMons[bank].hp;
				gBankAttacker = bank;
				gBankTarget = bank;
				gBattleMoveDamage = -(s32) damage;
				Lv200A_SetPoisonKingPopup();
				return BattleScript_Lv200A_EndTurnPoisonKingHeal;
			}
			break;
		case 11:
			/* この終端のあとTickで0になる＝最後のターン。本来のとくせいが戻る知らせ。 */
			if (state->poisonKingTimer[bank] == 1)
			{
				gBankAttacker = bank;
				gBankTarget = bank;
				return BattleScript_Lv200A_EndTurnPoisonKingEnd;
			}
			break;
		}

		if (damage != 0)
		{
			gBankAttacker = bank;
			gBankTarget = bank;
			gBattleMoveDamage = damage;
			return BattleScript_Lv200A_EndTurnDamage;
		}
	}
	return Lv200A_GetExpiryScript(state);
}

const u8 *Lv200CustomMovesA_GetMoveEndScript(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 bank;

	if (state == NULL)
		return NULL;
	bank = state->forcedSwitchBank;
	state->forcedSwitchBank = 0xFF;
	if (bank >= gBattlersCount || gBattleMons[bank].hp == 0)
		return NULL;
	gBankAttacker = bank;
	return BattleScript_Lv200A_WingGuardForceSwitch;
}

const u8 *Lv200CustomMovesA_GetSwitchInScript(u8 bank)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 pending;

	if (state == NULL || bank >= gBattlersCount || gBattleMons[bank].hp == 0)
		return NULL;
	pending = state->pendingTarget[bank];
	if (pending & 1)
	{
		state->pendingTarget[bank] &= ~1;
		gBankAttacker = bank;
		gBankTarget = bank;
		gBattleMoveDamage = gBattleMons[bank].maxHP / 16;
		if (gBattleMoveDamage == 0)
			gBattleMoveDamage = 1;
		return BattleScript_Lv200A_EndTurnDamage;
	}
	if (pending & 2)
	{
		state->pendingTarget[bank] &= ~2;
		gBankAttacker = bank;
		gBankTarget = bank;
		return BattleScript_Lv200A_SwitchInSpeedDown;
	}
	if (pending & 4)
	{
		/* いしくれのまく: 岩の相性ダメージ（ステルスロックと同じ計算・同じ文）。飛んでいる子にも入る。 */
		state->pendingTarget[bank] &= ~4;
		gBankAttacker = bank;
		gBankTarget = bank;
		gBattleMoveDamage = CalcStealthRockDamage(bank);
		gNewBS->DamageTaken[bank] += gBattleMoveDamage;
		gSideStatuses[SIDE(bank)] |= SIDE_STATUS_SPIKES_DAMAGED;
		return BattleScript_SRHurt;
	}
	if (pending & 8)
	{
		/* いしくれのまく: すばやさ-1（接地を問わない）。 */
		state->pendingTarget[bank] &= ~8;
		gBankAttacker = bank;
		gBankTarget = bank;
		return BattleScript_Lv200A_SwitchInSpeedDown;
	}
	return NULL;
}

u16 Lv200CustomMovesA_ModifyPower(u16 move, u8 bankAtk, u8 bankDef, u16 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u32 scaled = value;
	u8 i;
	u8 count;

	if (state == NULL || bankAtk >= MAX_BATTLERS_COUNT || bankDef >= MAX_BATTLERS_COUNT)
		return value;
	if (state->pendingMove[bankAtk] == MOVE_LV200_NORMAL_180
	&& state->echoMove[bankAtk] == move)
	{
		scaled = (value + 1) / 2;
		if (scaled == 0)
			scaled = 1;
	}

	switch (move)
	{
	case MOVE_LV200_FIGHTING_110:
		scaled = value * (5 + state->fightingLink[bankAtk]) / 5;
		break;
	case MOVE_LV200_FIGHTING_160:
	{
		u32 defStat;
		APPLY_STAT_MOD(defStat, &gBattleMons[bankDef], gBattleMons[bankDef].defense, STAT_STAGE_DEF);
		scaled = defStat / 3;
		if (scaled < 20)
			scaled = 20;
		else if (scaled > 180)
			scaled = 180;
		break;
	}
	case MOVE_LV200_FLYING_140:
		if (gBattleMons[bankAtk].hp == gBattleMons[bankAtk].maxHP)
			scaled = value * 6 / 5;
		break;
	case MOVE_LV200_FLYING_160:
		if (gNewBS->TailwindTimers[SIDE(bankAtk)] != 0)
			scaled = value * 3 / 2;
		break;
	case MOVE_LV200_POISON_180:
		if (gBattleMons[bankDef].status1 & STATUS1_TOXIC_POISON)
		{
			count = (gBattleMons[bankDef].status1 & STATUS1_TOXIC_COUNTER) >> 8;
			if (count > 10)
				count = 10;
			scaled = value * (10 + count) / 10;
		}
		else if (gBattleMons[bankDef].status1 & STATUS1_POISON)
			scaled = value * 13 / 10;
		break;
	case MOVE_LV200_BUG_160:
		if (gBattleMons[bankAtk].hp * 3 <= gBattleMons[bankAtk].maxHP)
			scaled = value * 3 / 2;
		break;
	case MOVE_LV200_BUG_130:
		count = Lv200A_CountLivingTypeMons(bankAtk, TYPE_BUG);
		if (count == 0)
			count = 1;
		if (count > 5)
			count = 5;
		scaled = value * (3 + count) / 4;
		break;
	case MOVE_LV200_GHOST_150:
		count = Lv200A_CountFaintedMons(bankAtk) + Lv200A_CountFaintedMons(bankDef);
		if (count > 6)
			count = 6;
		scaled = value + 20 * count;
		break;
	case MOVE_LV200_ROCK_160:
		count = gSideTimers[SIDE(bankDef)].spikesAmount
			+ gSideTimers[SIDE(bankDef)].tspikesAmount
			+ (gSideTimers[SIDE(bankDef)].srAmount != 0)
			+ (gSideTimers[SIDE(bankDef)].stickyWeb != 0)
			+ (gSideTimers[SIDE(bankDef)].steelsurge != 0)
			+ ((state->sandHazardBits & gBitTable[SIDE(bankDef)]) != 0)
			+ ((state->rockCurtainBits & gBitTable[SIDE(bankDef)]) != 0);
		if (count > 4)
			count = 4;
		scaled = value * (4 + count) / 4;
		break;
	case MOVE_LV200_FIRE_140:
		count = 0;
		for (i = 0; i < gBattlersCount; ++i)
			if (gBattleMons[i].status1 & STATUS1_BURN)
				++count;
		if (count > 3)
			count = 3;
		scaled = value * (10 + 3 * count) / 10;
		break;
	}
	if ((state->focusBoostBits & gBitTable[bankAtk])
	&& gBattleMoves[move].type == TYPE_FIGHTING
	&& gBattleMoves[move].split != SPLIT_STATUS)
		scaled = scaled * 3 / 2;
	/* ロックフィールド: 地面にいる子が出すいわ技 ×1.3 */
	if (state->rockFieldTimer != 0 && gBattleMoves[move].type == TYPE_ROCK
	&& gBattleMoves[move].split != SPLIT_STATUS && CheckGrounding(bankAtk) == GROUNDED)
		scaled = scaled * 13 / 10;
	if (gBattleMoves[move].type == TYPE_FIRE)
	{
		if (state->fireChargeBits & gBitTable[bankAtk])
			scaled *= 2;
		if (state->fireCloakBoostBits & gBitTable[bankAtk])
			scaled = scaled * 6 / 5;
	}
	if (gBattleMoves[move].type == TYPE_STEEL
	&& (state->steelTuneBits & gBitTable[bankAtk]))
		scaled = scaled * 3 / 2;
	if (gBattleMoves[move].split != SPLIT_STATUS
	&& (state->lastStandBoostBits & gBitTable[bankAtk]))
		scaled *= 2;

	if (scaled > 0xFFFF)
		scaled = 0xFFFF;
	return (u16) scaled;
}

void Lv200A_ClearEchoReplay(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (state != NULL && gBankAttacker < MAX_BATTLERS_COUNT)
	{
		state->pendingMove[gBankAttacker] = MOVE_NONE;
		state->pendingTarget[gBankAttacker] = 0;
		state->echoMove[gBankAttacker] = MOVE_NONE; /* 再発動したら使い切る（同じ技を続けて再発動しない） */
		state->echoDamage[gBankAttacker] = 0;
	}
	gCurrentMove = MOVE_LV200_NORMAL_180;
}

/* ぶんしんのまい: 次に受ける攻撃技2回を外させる。実際の命中判定の中(accuracy_calc.c)からだけ呼ばれる（画面・AI は呼ばない）。
 * すりぬけ・かたやぶり・まもる貫通技は無効。 */
bool8 Lv200CustomMovesA_ConsumeDodge(u16 move, u8 bankAtk, u8 bankDef)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (state == NULL || bankAtk >= gBattlersCount || bankDef >= gBattlersCount || move == MOVE_NONE
	|| bankAtk == bankDef || state->afterimageUses[bankDef] == 0
	|| gBattleMoves[move].split == SPLIT_STATUS)
		return FALSE;
	if (BankHasAbility(bankAtk, ABILITY_INFILTRATOR)
	|| IsMoldBreakerAbility(ABILITY(bankAtk))
	|| IsMoldBreakerAbility(SecondAbility_MoldBreakerAbility(bankAtk))
	|| CheckTableForMove(move, gMovesThatLiftProtectTable))
		return FALSE;
	--state->afterimageUses[bankDef];
	return TRUE;
}

u32 Lv200CustomMovesA_ModifyAccuracy(u16 move, u8 bankAtk, u8 bankDef, u32 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (state != NULL && bankAtk < MAX_BATTLERS_COUNT
	&& move != MOVE_NONE && gBattleMoves[move].type == TYPE_FIGHTING
	&& (state->focusBoostBits & gBitTable[bankAtk]))
		return 0xFFFF;
	if (state == NULL || bankDef >= gBattlersCount || move == MOVE_NONE)
		return value;
	if (state->powderCurtainTimer[SIDE(bankDef)] != 0 && Lv200A_PowderCurtainAffects(bankAtk))
	{
		if (gBattleMoves[move].type == TYPE_FIRE)
		{
			if (move == gCurrentMove && bankAtk == gBankAttacker)
				gNewBS->ResultFlags[bankDef] = MOVE_RESULT_MISSED;
			return 0;
		}
		value = value * 9 / 10;
	}
	return value;
}

u8 Lv200CustomMovesA_ModifyType(u16 move, u8 bankAtk, u8 value)
{
	static const u8 sAdaptTypes[] =
	{
		TYPE_NORMAL, TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND,
		TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL, TYPE_FIRE, TYPE_WATER,
		TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON,
		TYPE_DARK, TYPE_FAIRY, TYPE_RIL,
	};
	u8 i;
	u8 bestType = value;
	u32 best = 0;
	(void) bankAtk;

	if (move != MOVE_LV200_NORMAL_120 || gBankTarget >= gBattlersCount)
		return value;
	for (i = 0; i < ARRAY_COUNT(sAdaptTypes); ++i)
	{
		u32 multiplier = Lv200A_TypeMultiplier(sAdaptTypes[i], gBankTarget);
		if (multiplier == TYPE_MUL_NO_EFFECT
		|| Lv200A_TypeBlockedByAbility(sAdaptTypes[i], gBankTarget, multiplier))
			continue;
		if (multiplier > best
		|| (multiplier == best && bankAtk < gBattlersCount
			&& IsOfType(bankAtk, sAdaptTypes[i])
			&& !IsOfType(bankAtk, bestType)))
		{
			best = multiplier;
			bestType = sAdaptTypes[i];
		}
	}
	if (best == 0)
		return value;
	return bestType;
}

u32 Lv200CustomMovesA_ModifySpeed(u8 bank, u32 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u32 scaled = value;

	if (state == NULL || bank >= gBattlersCount)
		return value;
	/* ぬかるみは地面にいる全員が対象(ふゆう・ひこう・エアバルーンは対象外)。 */
	if (state->mudFieldTimer != 0 && CheckGrounding(bank) == GROUNDED)
		scaled /= 2;
	if (state->turboTimer[bank] != 0)
		scaled = scaled * 3 / 2;
	return scaled == 0 ? 1 : scaled;
}

s8 Lv200CustomMovesA_ModifyPriority(u16 move, u8 bankAtk, u8 bankDef, s8 value)
{
	(void) bankDef;
	if (bankAtk >= MAX_BATTLERS_COUNT)
		return value;
	if (move == MOVE_LV200_FLYING_140)
		return gBattleMons[bankAtk].hp == gBattleMons[bankAtk].maxHP ? 1 : 0;
	if (move == MOVE_LV200_BUG_160
	&& gBattleMons[bankAtk].hp * 3 <= gBattleMons[bankAtk].maxHP)
		return 1;
	return value;
}

s32 Lv200CustomMovesA_ModifyDamage(u16 move, u8 bankAtk, u8 bankDef, s32 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 type;

	if (state == NULL || bankAtk >= MAX_BATTLERS_COUNT || bankDef >= MAX_BATTLERS_COUNT)
		return value;
	type = GetMoveTypeSpecial(bankAtk, move); /* 技のタイプはテラスタル後などの今のタイプで見る */
	/* ひこうのけっかい: じめん技の無効は CheckGrounding(浮遊)側で行う＝グラビティで解除される（P2・#35）。 */
	if (state->powderCurtainTimer[SIDE(bankDef)] != 0 && type == TYPE_FIRE
	&& Lv200A_PowderCurtainAffects(bankAtk))
		return 0;
	if (state->cocoonArmorTimer[bankDef] != 0
	&& (type == TYPE_FLYING || type == TYPE_ROCK || type == TYPE_FIRE))
		value /= 2;
	if (state->steelWallTimer[SIDE(bankDef)] != 0
	&& gBattleMoves[move].split == SPLIT_PHYSICAL
	&& !(gSideStatuses[SIDE(bankDef)] & SIDE_STATUS_REFLECT)
	&& gNewBS->AuroraVeilTimers[SIDE(bankDef)] == 0
	&& !BankHasAbility(bankAtk, ABILITY_INFILTRATOR)
	&& !IsMoldBreakerAbility(SecondAbility_MoldBreakerAbility(bankAtk)))
		value = value * 2 / 3;
	if (state->crackedTimer[bankDef] != 0)
		value = value * 5 / 4;
	if (state->rockFieldTimer != 0 && CheckGrounding(bankDef) == GROUNDED
	&& gBattleMoves[move].split == SPLIT_PHYSICAL)
		value = value * 5 / 6;
	if (state->quartzTimer[SIDE(bankDef)] != 0
	&& GetBankAbility(bankDef) != ABILITY_FILTER
	&& GetBankAbility(bankDef) != ABILITY_SOLIDROCK
	&& GetBankAbility(bankDef) != ABILITY_PRISMARMOR
	&& (gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE))
		value = value * 3 / 4;
	if (state->darkVeilTimer != 0)
	{
		if (type == TYPE_GHOST || type == TYPE_DARK)
			value = value * 13 / 10;
		else if (type == TYPE_FAIRY || type == TYPE_PSYCHIC || type == TYPE_ELECTRIC)
			value = value * 4 / 5;
	}
	return value;
}

bool8 Lv200CustomMovesA_ShouldConsumePP(u16 move, u8 bankAtk, bool8 value)
{
	(void) move;
	(void) bankAtk;
	return value;
}

u32 Lv200CustomMovesA_ModifyAttackValue(u16 move, u8 bankAtk, u8 bankDef, u32 value)
{
	(void) bankDef;
	if (bankAtk >= MAX_BATTLERS_COUNT)
		return value;
	if (move == MOVE_LV200_FIGHTING_130)
		return SpeedCalc(bankAtk);
	if (move == MOVE_LV200_ROCK_120)
		return gBattleMons[bankAtk].defense;
	return value;
}

u16 Lv200CustomMovesA_ModifyTypeEffectiveness(u16 move, u8 bankAtk, u8 bankDef, u16 value)
{
	(void) bankAtk;
	if (bankDef >= gBattlersCount)
		return value;
	if (move == MOVE_LV200_POISON_150 && IsOfType(bankDef, TYPE_STEEL))
		return TYPE_MUL_NORMAL;
	if (move == MOVE_LV200_FIRE_170
	&& (IsOfType(bankDef, TYPE_STEEL) || IsOfType(bankDef, TYPE_ICE)))
		return TYPE_MUL_SUPER_EFFECTIVE;
	return value;
}

static bool8 Lv200A_HasOnlySealedType(u8 bank, u8 sealedType)
{
	u8 teraType = GetBattlerTeraType(bank);
	u8 types[3];
	u8 i;
	bool8 found = FALSE;

	if (!IS_BLANK_TYPE(teraType) && teraType != TYPE_STELLAR)
		return teraType == sealedType;
	types[0] = gBattleMons[bank].type1;
	types[1] = gBattleMons[bank].type2;
	types[2] = gBattleMons[bank].type3;
	for (i = 0; i < NELEMS(types); ++i)
	{
		if (IS_BLANK_TYPE(types[i]))
			continue;
		if (types[i] != sealedType)
			return FALSE;
		found = TRUE;
	}
	return found;
}

/* 封印したタイプ(かぜのたいじ・さびつかせ)を除いたタイプを返す。全部無くなればノーマル。
 * 相性・タイプ一致の窓口 Lv200CustomMove_AdjustTypes から呼ばれる。 */
void Lv200CustomMovesA_AdjustTypes(u8 bank, u8 *type1, u8 *type2, u8 *type3)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 types[3];
	u8 kept[3];
	u8 count = 0;
	u8 i;

	if (state == NULL || bank >= MAX_BATTLERS_COUNT || state->typeSealTimer[bank] == 0)
		return;
	types[0] = *type1;
	types[1] = *type2;
	types[2] = *type3;
	for (i = 0; i < 3; ++i)
	{
		if (IS_BLANK_TYPE(types[i]) || types[i] == state->typeSealMask[bank])
			continue;
		kept[count++] = types[i];
	}
	if (count == 0)
		kept[count++] = TYPE_NORMAL;
	*type1 = kept[0];
	*type2 = count > 1 ? kept[1] : kept[0];
	*type3 = count > 2 ? kept[2] : TYPE_BLANK;
}

bool8 Lv200CustomMovesA_IsBattlerOfType(u8 bank, u8 type, bool8 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	if (state != NULL && bank < MAX_BATTLERS_COUNT
	&& state->typeSealTimer[bank] != 0)
	{
		if (state->typeSealMask[bank] == type)
			return FALSE;
		if (type == TYPE_NORMAL
		&& Lv200A_HasOnlySealedType(bank, state->typeSealMask[bank]))
			return TRUE;
	}
	return value;
}

/* どくのおうさま: 持続中は毒・猛毒の継続ダメージを受けない（回復は終端 case 10）。 */
bool8 Lv200CustomMovesA_IsPoisonHarmless(u8 bank)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	return state != NULL && bank < MAX_BATTLERS_COUNT
		&& state->poisonKingTimer[bank] != 0;
}

bool8 Lv200CustomMovesA_IsAbilityActive(u8 bank, bool8 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	if (state != NULL && bank < gBattlersCount)
	{
		if (state->poisonKingTimer[bank] != 0)
			return FALSE;
		if (state->typeSealTimer[bank] != 0
		&& state->typeSealMask[bank] == TYPE_FLYING
		&& gBattleMons[bank].ability == ABILITY_LEVITATE)
			return FALSE;
	}
	if (state != NULL && bank < gBattlersCount && gBankAttacker < gBattlersCount
	&& bank != gBankAttacker && gCurrentMove != MOVE_NONE
	&& gBattleMoves[gCurrentMove].split != SPLIT_STATUS
	&& state->stanceTimer[gBankAttacker] != 0)
		return FALSE;
	return value;
}

bool8 Lv200CustomMovesA_CanLowerStat(u8 bank, u8 source, bool8 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (!value || state == NULL || bank >= gBattlersCount || source >= gBattlersCount)
		return value;
	if (state->metalArmorTimer[bank] != 0 && SIDE(bank) != SIDE(source))
		return FALSE;
	return value;
}

bool8 Lv200CustomMovesA_CanFlinch(u8 bank, u8 source, bool8 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (!value || state == NULL || bank >= gBattlersCount || source >= gBattlersCount)
		return value;
	if (state->metalArmorTimer[bank] != 0 && SIDE(bank) != SIDE(source))
		return FALSE;
	return value;
}

bool8 Lv200CustomMovesA_ShouldReflectStatDrop(u8 bank, u8 source, bool8 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (value || state == NULL || bank >= gBattlersCount || source >= gBattlersCount)
		return value;
	if (state->mirrorArmorTimer[bank] != 0 && SIDE(bank) != SIDE(source))
		return TRUE;
	return value;
}

bool8 Lv200CustomMovesA_IsItemActive(u8 bank, bool8 value)
{
	(void) bank;
	return value;
}

static void ActivateSuccessfulMove(struct Lv200CustomMovesAState *state,
								   u16 move, u8 bank)
{
	switch (move)
	{
	case MOVE_LV200_NORMAL_140:
		Lv200A_ClearBattlefield();
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPEED, 1, FALSE);
		break;
	case MOVE_LV200_NORMAL_150:
		if (gBankTarget < gBattlersCount)
		{
			state->finalCount[gBankTarget] = 3;
			Lv200Common_QueueMessage(gBankTarget, gText_Lv200A_FinalCountStart); /* 「なにかが ちかづいている…」（仕様の文） */
		}
		break;
	case MOVE_LV200_FIGHTING_120:
		state->focusBoostBits |= gBitTable[bank];
		break;
	case MOVE_LV200_FIGHTING_170:
		state->stanceTimer[bank] = 3;
		break;
	case MOVE_LV200_FIGHTING_150:
		state->duelTimer[bank] = 3;
		if (gBankTarget < gBattlersCount)
			state->duelTimer[gBankTarget] = 3;
		break;
	case MOVE_LV200_FIGHTING_190:
		state->lastStandBits |= gBitTable[bank];
		break;
	case MOVE_LV200_FLYING_190:
		state->afterimageUses[bank] = 2;
		Lv200Common_QueueMessage(bank, gText_Lv200A_Afterimage);
		break;
	case MOVE_LV200_FLYING_120:
		Lv200Common_QueueMessage(bank, gText_Lv200A_HighSoar);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_ATK, 1, TRUE);
		Lv200Common_QueueStat(bank, bank, STAT_STAGE_SPEED, 1, TRUE);
		break;
	case MOVE_LV200_FLYING_200:
		state->flyingBarrierTimer[SIDE(bank)] = 5; /* 側のタイマー（交代で出た子も浮く） */
		Lv200Common_QueueMessage(bank, gText_Lv200A_FlyingBarrierSet);
		break;
	case MOVE_LV200_GROUND_140:
		state->mudFieldTimer = 5;
		Lv200Common_QueueMessage(bank, gText_Lv200A_MudFieldSet);
		break;
	case MOVE_LV200_GROUND_170:
		state->aftershockTimer = 3;
		break;
	case MOVE_LV200_GROUND_200:
		if (gBankTarget < gBattlersCount)
			state->sandHazardBits |= gBitTable[SIDE(gBankTarget)];
		break;
	case MOVE_LV200_ROCK_110:
		state->garnetGuardBits |= gBitTable[bank];
		break;
	case MOVE_LV200_ROCK_180:
		state->rockFieldTimer = 5;
		state->rockFieldOwner = bank + 1;
		Lv200Common_QueueMessage(bank, gText_Lv200A_RockFieldSet);
		break;
	case MOVE_LV200_ROCK_190:
		state->quartzTimer[SIDE(bank)] = 3;
		Lv200Common_QueueMessage(bank, gText_Lv200A_QuartzSet);
		break;
	case MOVE_LV200_BUG_140:
		Lv200Common_QueueMessage(bank, gText_Lv200A_Molt);
		Lv200Common_QueueCureStatus(bank); /* 手持ちへも送り、文も出す */
		Lv200Common_QueueCureConfusion(bank);
		break;
	case MOVE_LV200_BUG_170:
		state->powderCurtainTimer[SIDE(bank)] = 3;
		Lv200Common_QueueMessage(bank, gText_Lv200A_PowderCurtainSet);
		break;
	case MOVE_LV200_BUG_180:
		state->cocoonArmorTimer[bank] = 3;
		Lv200Common_QueueMessage(bank, gText_Lv200A_CocoonArmorSet);
		break;
	case MOVE_LV200_BUG_190:
		if (gBankTarget < gBattlersCount && !IsOfType(gBankTarget, TYPE_GRASS))
		{
			state->eggTimer[gBankTarget] = 3;
			state->eggOwner[gBankTarget] = bank + 1;
			Lv200Common_QueueMessageFrom(gBankTarget, bank, gText_Lv200A_EggSet);
		}
		break;
	case MOVE_LV200_GHOST_120:
		if (gBankTarget < gBattlersCount && !IsOfType(gBankTarget, TYPE_GHOST))
		{
			state->lostTimer[gBankTarget] = 5;
			state->lostOwner[gBankTarget] = bank + 1;
		}
		break;
	case MOVE_LV200_GHOST_130:
	{
		u32 substituteHP = (gBattleMons[bank].maxHP + 7) / 8;
		gDisableStructs[bank].substituteHP = substituteHP;
		gBattleMons[bank].status2 |= STATUS2_SUBSTITUTE;
		Lv200A_HurtBank(bank, substituteHP);
		state->huskSubstituteBits |= gBitTable[bank];
		break;
	}
	case MOVE_LV200_GHOST_180:
		state->darkVeilTimer = 5;
		Lv200Common_QueueMessage(bank, gText_Lv200A_DarkVeilSet);
		break;
	case MOVE_LV200_GHOST_140:
		state->grudgeBits |= gBitTable[bank];
		Lv200Common_QueueMessage(bank, gText_Lv200A_GrudgeSet);
		break;
	case MOVE_LV200_GHOST_170:
		if (gBankTarget < gBattlersCount
		&& gBattleMons[bank].hp > gBattleMons[bank].maxHP / 2)
		{
			Lv200A_HurtBank(bank, gBattleMons[bank].maxHP / 2);
			state->soulContractTimer[bank] = 3;
			state->contractPeer[bank] = gBankTarget + 1;
			state->soulContractTimer[gBankTarget] = 3;
			state->contractPeer[gBankTarget] = bank + 1;
			Lv200Common_QueueMessageFrom(gBankTarget, bank, gText_Lv200A_SoulContractSet);
		}
		break;
	case MOVE_LV200_GHOST_200:
		state->lastSpiritBits |= gBitTable[bank];
		break;
	case MOVE_LV200_STEEL_110:
		state->metalArmorTimer[bank] = 5;
		Lv200Common_QueueMessage(bank, gText_Lv200A_MetalArmorSet);
		break;
	case MOVE_LV200_STEEL_130:
		state->steelTuneBits |= gBitTable[bank];
		break;
	case MOVE_LV200_STEEL_140:
		state->mirrorArmorTimer[bank] = 5;
		Lv200Common_QueueMessage(bank, gText_Lv200A_MirrorArmorSet);
		break;
	case MOVE_LV200_STEEL_160:
		state->steelWallTimer[SIDE(bank)] = 5;
		Lv200Common_QueueMessage(bank, gText_Lv200A_SteelWallSet);
		break;
	case MOVE_LV200_STEEL_170:
		state->turboTimer[bank] = 3;
		Lv200Common_QueueMessage(bank, gText_Lv200A_TurboSet);
		break;
	case MOVE_LV200_STEEL_190:
	{
		u8 stat;
		bool8 restored = FALSE;
		for (stat = STAT_STAGE_ATK; stat <= STAT_STAGE_EVASION; ++stat)
			if (gBattleMons[bank].statStages[stat - 1] < 6)
			{
				gBattleMons[bank].statStages[stat - 1] = 6;
				restored = TRUE;
			}
		if (restored)
			Lv200Common_QueueMessage(bank, gText_Lv200A_Overhaul);
		break;
	}
	case MOVE_LV200_POISON_200:
		state->poisonKingTimer[bank] = 5;
		break;
	case MOVE_LV200_FIRE_120:
		state->fireChargeBits |= gBitTable[bank];
		break;
	case MOVE_LV200_FIRE_150:
		state->fireCloakBits |= gBitTable[bank];
		state->fireCloakBoostBits |= gBitTable[bank];
		break;
	case MOVE_LV200_FIRE_190:
		state->blazeFormTimer[bank] = 5;
		break;
	}
}

void Lv200CustomMovesA_OnMoveEnd(u16 move, u8 bankAtk, u8 bankDef,
								 bool8 succeeded, bool8 didDamage)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 type;
	(void) bankDef;

	if (state == NULL || bankAtk >= MAX_BATTLERS_COUNT || move == MOVE_NONE)
		return;
	/* 前のターンに出した技の記録: ダメージを与えた攻撃技だけを残す（変化技・外れ・失敗なら空にする） */
	if (move != MOVE_LV200_NORMAL_180 && !(succeeded && didDamage && gBattleMoves[move].split != SPLIT_STATUS))
	{
		state->echoMove[bankAtk] = MOVE_NONE;
		state->echoDamage[bankAtk] = 0;
	}
	if (succeeded && state->pendingMove[bankAtk] == move)
		ActivateSuccessfulMove(state, move, bankAtk);
	/* どくのいけにえ: 全部当たって最後の1発まで出たときだけ、もうどく（素の追加効果の流れ） */
	if (succeeded && didDamage && move == MOVE_LV200_POISON_190 && gMultiHitCounter == 1
	&& bankDef < gBattlersCount)
		Lv200Common_QueueInflict(bankDef, bankAtk, MOVE_EFFECT_TOXIC, FALSE);
	if (succeeded && didDamage && move == MOVE_LV200_STEEL_150
	&& bankDef < gBattlersCount)
		Lv200Common_QueueStat(bankDef, bankAtk, STAT_STAGE_DEF, -1, FALSE);
	if (move == MOVE_LV200_FLYING_150 && succeeded)
		state->wingGuardBits |= gBitTable[bankAtk];
	type = gBattleMoves[move].type;
	if (type == TYPE_FIGHTING && succeeded && didDamage)
	{
		if (state->fightingLink[bankAtk] < 5)
			++state->fightingLink[bankAtk];
	}
	else
		state->fightingLink[bankAtk] = 0;

	if (move != MOVE_LV200_FIGHTING_120 && type == TYPE_FIGHTING && succeeded)
		state->focusBoostBits &= ~gBitTable[bankAtk];
	if (move != MOVE_LV200_FIRE_120)
		state->fireChargeBits &= ~gBitTable[bankAtk];
	if (move != MOVE_LV200_STEEL_130 && type == TYPE_STEEL && succeeded && didDamage)
		state->steelTuneBits &= ~gBitTable[bankAtk];
	if (move != MOVE_LV200_FIRE_150 && type == TYPE_FIRE && succeeded && didDamage)
		state->fireCloakBoostBits &= ~gBitTable[bankAtk];
	if (move != MOVE_LV200_FIGHTING_190 && succeeded && didDamage)
		state->lastStandBoostBits &= ~gBitTable[bankAtk];
	if (move == MOVE_LV200_NORMAL_180 && succeeded && didDamage)
	{
		state->pendingMove[bankAtk] = move;
		state->pendingTarget[bankAtk] = bankDef;
	}
	else
	{
		state->pendingMove[bankAtk] = MOVE_NONE;
		state->pendingTarget[bankAtk] = 0;
	}
}

void Lv200CustomMovesA_OnContact(u16 move, u8 bankAtk, u8 bankDef, bool8 protected)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	(void) move;
	if (state == NULL || bankAtk >= gBattlersCount || bankDef >= gBattlersCount)
		return;
	/* ガーネットガードは「耐える」技＝攻撃が通るので、protected(止めた)を条件にしない（P2・#35）。 */
	if (state->garnetGuardBits & gBitTable[bankDef])
		Lv200A_HurtBank(bankAtk, gBattleMons[bankAtk].maxHP / 8);
	/* ほのおのころも: 接触のやけどは元技(ほのおのとりで)の枝＝cmd49 の ADVERSE_PROTECTION が文つきで行う（P2・#35）。 */
	if (protected && (state->wingGuardBits & gBitTable[bankDef])
	&& state->forcedSwitchBank == 0xFF)
		state->forcedSwitchBank = bankAtk;
	if (state->steelWallTimer[SIDE(bankDef)] != 0
	&& GetBankAbility(bankAtk) != ABILITY_MAGICGUARD
	&& !BankHasAbility(bankAtk, ABILITY_INFILTRATOR)
	&& !IsMoldBreakerAbility(SecondAbility_MoldBreakerAbility(bankAtk)))
		Lv200A_HurtBank(bankAtk, gBattleMons[bankAtk].maxHP / 16);
}

void Lv200CustomMovesA_OnFaint(u8 bank, u8 source, u16 move, u32 damage)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	if (state != NULL && bank < gBattlersCount && source < gBattlersCount
	&& SIDE(bank) != SIDE(source) && damage != 0)
	{
		if (state->grudgeBits & gBitTable[bank])
		{
			u32 reflected = damage > 0x7FFFFFFF ? 0xFFFFFFFF : damage * 2;
			if (reflected > gBattleMons[source].maxHP)
				reflected = gBattleMons[source].maxHP;
			Lv200A_HurtBank(source, reflected);
		}
	}
	(void) move;
	Lv200CustomMovesA_OnSwitchOut(bank);
}

bool8 Lv200CustomMovesA_CanSwitch(u8 bank, bool8 value)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	if (state != NULL && bank < MAX_BATTLERS_COUNT
	&& !IsOfType(bank, TYPE_GHOST)
	&& (state->duelTimer[bank] != 0 || state->lostTimer[bank] != 0))
		return FALSE;
	return value;
}

void Lv200CustomMovesA_OnBattleEnd(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (state != NULL && state->miningReward[B_SIDE_PLAYER] != ITEM_NONE
	&& (gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT))
		AddBagItem(state->miningReward[B_SIDE_PLAYER], 1);
	Lv200CustomMovesA_ResetState();
}

u32 Lv200CustomMovesA_ModifyLeechSeedDamage(u8 bank, u32 value)
{
	(void) bank;
	return value;
}

void Lv200CustomMovesA_OnMoveHit(u16 move, u8 bankAtk, u8 bankDef, u32 damage,
								 bool8 hitSubstitute, bool8 substituteBroke)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (state == NULL || bankAtk >= MAX_BATTLERS_COUNT || bankDef >= MAX_BATTLERS_COUNT)
		return;
	/* ミラーラウンドの再現打撃は元技の追加効果・状態予約を繰り返さない。 */
	if (state->pendingMove[bankAtk] == MOVE_LV200_NORMAL_180
	&& state->echoMove[bankAtk] == move)
		return;

	/* みがわりが受けた値は本体の被ダメージ履歴・対象状態へ入れない。 */
	if (hitSubstitute)
	{
		if (substituteBroke && (state->huskSubstituteBits & gBitTable[bankDef]))
		{
			state->huskSubstituteBits &= ~gBitTable[bankDef];
			Lv200Common_QueueStat(bankDef, bankDef, STAT_STAGE_SPEED, 1, FALSE);
		}
		return;
	}

	if (damage != 0)
	{
		u8 peer;
		u8 i;
		if (state->soulContractTimer[bankDef] != 0
		&& state->contractPeer[bankDef] != 0)
		{
			peer = state->contractPeer[bankDef] - 1;
			if (peer < gBattlersCount)
				Lv200A_HurtBank(peer, damage);
		}
		if (state->stitchedTimer[bankDef] != 0
		&& state->stitchedOwner[bankDef] != 0)
		{
			peer = state->stitchedOwner[bankDef] - 1;
			if (peer < gBattlersCount)
				Lv200A_HurtBank(peer, damage / 2);
		}
		for (i = 0; i < gBattlersCount; ++i)
		{
			if (i != bankDef && state->stitchedTimer[i] != 0
			&& state->stitchedOwner[i] == bankDef + 1)
				Lv200A_HurtBank(i, damage / 2);
		}
	}
	if (damage != 0 && move != MOVE_LV200_NORMAL_180
	&& gBattleMoves[move].split != SPLIT_STATUS)
	{
		state->echoMove[bankAtk] = move;
		state->echoDamage[bankAtk] = damage;
	}

	if (damage != 0 && gBattleMoves[move].split == SPLIT_PHYSICAL)
		state->physicalDamageHistory[bankDef][0] += damage;
	if (damage != 0 && gProtectStructs[bankDef].endured
	&& (state->lastStandBits & gBitTable[bankDef]))
	{
		state->lastStandBits &= ~gBitTable[bankDef];
		state->lastStandBoostBits |= gBitTable[bankDef];
	}
	if (damage != 0 && gProtectStructs[bankDef].endured
	&& (state->lastSpiritBits & gBitTable[bankDef]))
	{
		state->lastSpiritBits &= ~gBitTable[bankDef];
		Lv200A_HurtBank(bankAtk, gBattleMons[bankAtk].hp / 2);
	}
	switch (move)
	{
	case MOVE_LV200_NORMAL_170:
		/* 30%で出たら、8種類（まひ・やけど・こおり・どく・もうどく・ねむり・こんらん・ひるみ）から有効な物だけを集めて等確率で1つ。 */
		if (!SheerForceCheck() && Random() % 100 < 30)
		{
			u8 candidates[8];
			u8 count = 0;
			if (CanBeParalyzed(bankDef, bankAtk, TRUE))
				candidates[count++] = MOVE_EFFECT_PARALYSIS;
			if (CanBeBurned(bankDef, bankAtk, TRUE))
				candidates[count++] = MOVE_EFFECT_BURN;
			if (CanBeFrozen(bankDef, bankAtk, TRUE))
				candidates[count++] = MOVE_EFFECT_FREEZE;
			if (CanBePoisoned(bankDef, bankAtk, TRUE))
			{
				candidates[count++] = MOVE_EFFECT_POISON;
				candidates[count++] = MOVE_EFFECT_TOXIC;
			}
			if (CanBePutToSleep(bankDef, bankAtk, TRUE))
				candidates[count++] = MOVE_EFFECT_SLEEP;
			if (CanBeConfused(bankDef, bankAtk, TRUE))
				candidates[count++] = MOVE_EFFECT_CONFUSION;
			if (!IsDynamaxed(bankDef) && !BankHasAbility(bankDef, ABILITY_INNERFOCUS))
				candidates[count++] = MOVE_EFFECT_FLINCH;
			if (count != 0)
				Lv200Common_QueueInflict(bankDef, bankAtk, candidates[Random() % count], FALSE);
		}
		break;
	case MOVE_LV200_FLYING_110:
		/* すでにおいかぜ中なら効果なし（ダメージだけ）。起こす時は素の文。 */
		Lv200A_StartTailwind(bankAtk);
		break;
	case MOVE_LV200_FLYING_130:
		Lv200A_SetRandomWeather(bankAtk);
		break;
	case MOVE_LV200_NORMAL_150:
		state->finalCount[bankDef] = 3;
		break;
	case MOVE_LV200_FLYING_170:
		if (!IsDynamaxed(bankDef))
		{
			/* うちおとすと同じに浮遊を解き、ふうせんも割る（素のうちおとすの流れ）。封印を付ける前に浮いているか確かめる。 */
			if (CheckGrounding(bankDef) != GROUNDED)
				Lv200Common_QueueBringDown(bankDef, bankAtk);
			state->typeSealTimer[bankDef] = 3;
			state->typeSealMask[bankDef] = TYPE_FLYING;
		}
		Lv200Common_QueueStat(bankDef, bankAtk, STAT_STAGE_SPEED, -1, FALSE);
		break;
	case MOVE_LV200_POISON_120:
		/* どくタイプには層が付かない */
		if (!IsOfType(bankDef, TYPE_POISON) && state->venomLayers[bankDef] < 6)
		{
			++state->venomLayers[bankDef];
			Lv200Common_QueueMessageFrom(bankDef, bankAtk, gText_Lv200A_StockVenomLayer);
		}
		break;
	case MOVE_LV200_POISON_140:
		/* 3ターンの回復ふうじ（かいふくふうじと同じ状態・素の文） */
		if (gNewBS->HealBlockTimers[bankDef] == 0)
		{
			gNewBS->HealBlockTimers[bankDef] = 3;
			Lv200Common_QueueMessage(bankDef, HealBlockSetString);
		}
		break;
	case MOVE_LV200_POISON_150:
		/* はがね相手は必ずどく（はがねにも通る）、それ以外は30%。無効判定・文・手持ち送りは素の追加効果の流れ。 */
		if (!SheerForceCheck()
		&& (IsOfType(bankDef, TYPE_STEEL) || Random() % 100 < 30))
			Lv200Common_QueueInflict(bankDef, bankAtk, MOVE_EFFECT_POISON, IsOfType(bankDef, TYPE_STEEL));
		break;
	case MOVE_LV200_POISON_160:
		/* もうどくならカウンタ+3（上限15）、どくなら必ずもうどくに悪化。元技の追加効果は付けない。 */
		if (gBattleMons[bankDef].status1 & STATUS1_TOXIC_POISON)
		{
			u32 counter = ((gBattleMons[bankDef].status1 & STATUS1_TOXIC_COUNTER) >> 8) + 3;
			if (counter > 15)
				counter = 15;
			gBattleMons[bankDef].status1 = (gBattleMons[bankDef].status1 & ~STATUS1_TOXIC_COUNTER)
				| (counter << 8);
			Lv200A_SyncStatus(bankDef);
			Lv200Common_QueueMessage(bankDef, gText_Lv200A_ToxicWorse);
		}
		else if (gBattleMons[bankDef].status1 & STATUS1_POISON)
		{
			gBattleMons[bankDef].status1 = STATUS1_TOXIC_POISON;
			Lv200A_SyncStatus(bankDef);
			Lv200Common_QueueMessage(bankDef, gText_Lv200A_ToxicWorse);
		}
		break;
	case MOVE_LV200_BUG_120:
		state->stitchedOwner[bankDef] = bankAtk + 1;
		state->stitchedTimer[bankDef] = 3;
		break;
	case MOVE_LV200_GROUND_150:
		state->sandTrapOwner[bankDef] = bankAtk + 1;
		break;
	case MOVE_LV200_GROUND_110:
		/* 砂あらしの間は+1/4（合わせて3/4）。ヘドロえきなら逆に削れ、おおきなねっこで1.3倍。HP バーは共通部品。 */
		if (gBattleWeather & WEATHER_SANDSTORM_ANY)
		{
			u32 extra = damage / 4;
			if (ITEM_EFFECT(bankAtk) == ITEM_EFFECT_BIG_ROOT)
				extra = extra * 13 / 10;
			if (GetBankAbility(bankDef) == ABILITY_LIQUIDOOZE)
				Lv200A_HurtBank(bankAtk, extra);
			else
				Lv200A_HealBank(bankAtk, extra);
		}
		break;
	case MOVE_LV200_GROUND_160:
		if (Lv200A_SideHasFieldEffects(SIDE(bankAtk)) || Lv200A_SideHasFieldEffects(SIDE(bankDef))
		|| gTerrainType != 0 || gNewBS->TailwindTimers[B_SIDE_PLAYER] != 0 || gNewBS->TailwindTimers[B_SIDE_OPPONENT] != 0)
			Lv200Common_QueueMessage(bankDef, gText_Lv200A_FieldWipe);
		Lv200A_ClearBattlefield();
		gNewBS->TailwindTimers[B_SIDE_PLAYER] = 0;
		gNewBS->TailwindTimers[B_SIDE_OPPONENT] = 0;
		gTerrainType = 0;
		gNewBS->TerrainTimer = 0;
		break;
	case MOVE_LV200_ROCK_140:
		state->crackedTimer[bankDef] = 3;
		break;
	case MOVE_LV200_ROCK_130:
		if (state->miningReward[SIDE(bankAtk)] == ITEM_NONE)
			state->miningReward[SIDE(bankAtk)] = Lv200A_ChooseMiningReward();
		break;
	case MOVE_LV200_ROCK_160:
		if (Lv200A_SideHasHazards(SIDE(bankDef)))
			Lv200Common_QueueMessage(bankDef, gText_Lv200A_FieldWipe);
		Lv200A_ClearHazards(SIDE(bankDef));
		break;
	case MOVE_LV200_BUG_150:
		gSideTimers[SIDE(bankDef)].stickyWeb = 1;
		break;
	case MOVE_LV200_BUG_200:
		Lv200Common_QueueStat(bankAtk, bankAtk, STAT_STAGE_SPATK, 1, TRUE);
		Lv200Common_QueueStat(bankAtk, bankAtk, STAT_STAGE_SPEED, 1, TRUE);
		break;
	case MOVE_LV200_GHOST_110:
		Lv200Common_QueueStat(bankDef, bankAtk, STAT_STAGE_EVASION, -1, FALSE);
		break;
	case MOVE_LV200_GHOST_190:
		/* 能力ダウンは素の流れ（クリアボディ・ミラーアーマー・あまのじゃく等を通る） */
		Lv200Common_QueueStat(bankDef, bankAtk, STAT_STAGE_ATK, -1, TRUE);
		Lv200Common_QueueStat(bankDef, bankAtk, STAT_STAGE_SPATK, -1, TRUE);
		state->fearTimer[bankDef] = 3;
		break;
	case MOVE_LV200_STEEL_120:
		state->steelSpikesBits |= gBitTable[SIDE(bankDef)];
		gSideTimers[SIDE(bankDef)].steelsurge = 1;
		break;
	case MOVE_LV200_STEEL_200:
		state->typeSealTimer[bankDef] = 3;
		state->typeSealMask[bankDef] = TYPE_STEEL;
		Lv200Common_QueueStat(bankDef, bankAtk, STAT_STAGE_DEF, -1, FALSE);
		break;
	case MOVE_LV200_FIRE_130:
		/* 必ずやけど（無効なら付かない）。印は付けたときだけ。付与は素の追加効果の流れ（文・手持ち送り）。 */
		if (CanBeBurned(bankDef, bankAtk, TRUE))
		{
			Lv200Common_QueueInflict(bankDef, bankAtk, MOVE_EFFECT_BURN, FALSE);
			state->burnMarkTimer[bankDef] = 5;
		}
		break;
	case MOVE_LV200_FIRE_110:
		if (CanBeBurned(bankDef, bankAtk, TRUE))
			Lv200Common_QueueInflict(bankDef, bankAtk, MOVE_EFFECT_BURN, FALSE);
		break;
	case MOVE_LV200_FIRE_180:
		if (Lv200A_SideHasFieldEffects(SIDE(bankDef)) || gTerrainType != 0)
			Lv200Common_QueueMessage(bankDef, gText_Lv200A_FieldWipe);
		Lv200A_ClearTargetField(bankDef);
		break;
	case MOVE_LV200_FIRE_200:
		Lv200Common_QueueWeather(bankAtk, LV200_WEATHER_SUN);
		break;
	}
}

static void Tick(u8 *timer)
{
	if (*timer != 0)
		--*timer;
}

void Lv200CustomMovesA_OnTurnEnd(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 bank;
	u8 side;

	if (state == NULL)
		return;
	state->endTurnCursor = 0;
	state->expiryCursor = 0;
	state->wingGuardBits = 0;

	Tick(&state->mudFieldTimer);
	Tick(&state->aftershockTimer);
	Tick(&state->rockFieldTimer);
	if (state->rockFieldTimer == 0)
		state->rockFieldOwner = 0;
	Tick(&state->darkVeilTimer);
	for (side = 0; side < NUM_BATTLE_SIDES; ++side)
	{
		Tick(&state->quartzTimer[side]);
		Tick(&state->powderCurtainTimer[side]);
		Tick(&state->steelWallTimer[side]);
	}
	for (bank = 0; bank < MAX_BATTLERS_COUNT; ++bank)
	{
		if (state->fearTimer[bank] != 0 && gBattleMons[bank].hp != 0
		&& Random() % 100 < 30)
			gBattleMons[bank].status2 |= STATUS2_FLINCHED;
		Tick(&state->finalCount[bank]);
		Tick(&state->duelTimer[bank]);
		Tick(&state->stanceTimer[bank]);
		Tick(&state->typeSealTimer[bank]);
		if (state->typeSealTimer[bank] == 0)
			state->typeSealMask[bank] = 0;
		Tick(&state->flyingBarrierTimer[bank]);
		Tick(&state->poisonKingTimer[bank]);
		Tick(&state->eggTimer[bank]);
		if (state->eggTimer[bank] == 0)
			state->eggOwner[bank] = 0;
		Tick(&state->lostTimer[bank]);
		if (state->lostTimer[bank] == 0)
			state->lostOwner[bank] = 0;
		Tick(&state->soulContractTimer[bank]);
		if (state->soulContractTimer[bank] == 0)
			state->contractPeer[bank] = 0;
		Tick(&state->stitchedTimer[bank]);
		if (state->stitchedTimer[bank] == 0)
			state->stitchedOwner[bank] = 0;
		Tick(&state->fearTimer[bank]);
		Tick(&state->pupaTimer[bank]);
		if (state->pupaTimer[bank] == 0)
			state->pupaBits &= ~gBitTable[bank];
		Tick(&state->metalArmorTimer[bank]);
		Tick(&state->mirrorArmorTimer[bank]);
		Tick(&state->turboTimer[bank]);
		Tick(&state->crackedTimer[bank]);
		Tick(&state->cocoonArmorTimer[bank]);
		Tick(&state->burnMarkTimer[bank]);
		Tick(&state->blazeFormTimer[bank]);
		state->physicalDamageHistory[bank][1] = state->physicalDamageHistory[bank][0];
		state->physicalDamageHistory[bank][0] = 0;
		state->pendingMove[bank] = MOVE_NONE;
		state->pendingTarget[bank] = 0;
		/* echoMove／echoDamage は次のターンへ持ち越す（ミラーラウンド＝前のターンに出した技・Q5） */
	}
	state->garnetGuardBits = 0;
	state->lastSpiritBits = 0;
	state->grudgeBits = 0; /* うらみのこえは使ったターンだけ */
}

void Lv200CustomMovesA_OnSwitchOut(u8 bank)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 i;

	if (state == NULL || bank >= MAX_BATTLERS_COUNT)
		return;

	state->finalCount[bank] = 0;
	state->fightingLink[bank] = 0;
	state->duelTimer[bank] = 0;
	state->stanceTimer[bank] = 0;
	state->typeSealTimer[bank] = 0;
	state->typeSealMask[bank] = 0;
	state->afterimageUses[bank] = 0;
	/* flyingBarrierTimer は側のタイマーなので、交代では消さない（P2・#35） */
	state->venomLayers[bank] = 0;
	state->poisonKingTimer[bank] = 0;
	state->sandTrapOwner[bank] = 0;
	state->eggTimer[bank] = 0;
	state->eggOwner[bank] = 0;
	state->lostTimer[bank] = 0;
	state->lostOwner[bank] = 0;
	state->soulContractTimer[bank] = 0;
	state->contractPeer[bank] = 0;
	state->stitchedTimer[bank] = 0;
	state->stitchedOwner[bank] = 0;
	state->fearTimer[bank] = 0;
	state->pupaTimer[bank] = 0;
	state->metalArmorTimer[bank] = 0;
	state->mirrorArmorTimer[bank] = 0;
	state->turboTimer[bank] = 0;
	state->crackedTimer[bank] = 0;
	state->cocoonArmorTimer[bank] = 0;
	state->burnMarkTimer[bank] = 0;
	state->blazeFormTimer[bank] = 0;
	state->pendingMove[bank] = MOVE_NONE;
	state->pendingTarget[bank] = 0; /* 前の技の対象番号を、設置を踏んだ印(bit0-3)と取り違えない */
	state->echoMove[bank] = MOVE_NONE;
	state->echoDamage[bank] = 0;
	if (state->rockHitIndex[bank] == 0)
	{
		state->physicalDamageHistory[bank][0] = 0;
		state->physicalDamageHistory[bank][1] = 0;
	}
	state->focusBoostBits &= ~gBitTable[bank];
	state->lastStandBits &= ~gBitTable[bank];
	state->lastStandBoostBits &= ~gBitTable[bank];
	state->garnetGuardBits &= ~gBitTable[bank];
	state->wingGuardBits &= ~gBitTable[bank];
	state->pupaBits &= ~gBitTable[bank];
	state->huskSubstituteBits &= ~gBitTable[bank];
	state->grudgeBits &= ~gBitTable[bank];
	state->lastSpiritBits &= ~gBitTable[bank];
	state->steelTuneBits &= ~gBitTable[bank];
	state->fireChargeBits &= ~gBitTable[bank];
	state->fireCloakBits &= ~gBitTable[bank];
	state->fireCloakBoostBits &= ~gBitTable[bank];
	for (i = 0; i < MAX_BATTLERS_COUNT; ++i)
	{
		if (state->contractPeer[i] == bank + 1)
		{
			state->contractPeer[i] = 0;
			state->soulContractTimer[i] = 0;
		}
		if (state->stitchedOwner[i] == bank + 1)
		{
			state->stitchedOwner[i] = 0;
			state->stitchedTimer[i] = 0;
		}
	}
}

void Lv200CustomMovesA_OnSwitchIn(u8 bank)
{
	struct Lv200CustomMovesAState *state;
	u8 side;

	Lv200CustomMovesA_OnSwitchOut(bank);
	state = Lv200CustomMove_GetStateA();
	if (state == NULL || bank >= gBattlersCount || gBattleMons[bank].hp == 0)
		return;
	side = SIDE(bank);
	if ((state->sandHazardBits & gBitTable[side])
	&& CheckGrounding(bank) == GROUNDED
	&& GetBankAbility(bank) != ABILITY_MAGICGUARD
	&& SecondAbility_OfBank(bank) != ABILITY_MAGICGUARD //[hack] 2026-10-22 2 つ目
	&& ITEM_EFFECT(bank) != ITEM_EFFECT_HEAVY_DUTY_BOOTS)
	{
		state->pendingTarget[bank] |= 1;
		if (!IsOfType(bank, TYPE_ROCK) && !IsOfType(bank, TYPE_GROUND))
			state->pendingTarget[bank] |= 2;
	}
	/* いしくれのまく: ひこう・ふゆうでも踏む。ヘビーデューティブーツは両方防ぎ、マジックガードはダメージだけ防ぐ。 */
	if ((state->rockCurtainBits & gBitTable[side])
	&& ITEM_EFFECT(bank) != ITEM_EFFECT_HEAVY_DUTY_BOOTS)
	{
		if (GetBankAbility(bank) != ABILITY_MAGICGUARD)
			state->pendingTarget[bank] |= 4;
		state->pendingTarget[bank] |= 8;
	}
}

void Lv200CustomMovesA_OnStatusCured(u8 bank)
{
	(void) bank;
}

void Lv200CustomMovesA_OnHazardsCleared(u8 side)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();

	if (state == NULL || side >= NUM_BATTLE_SIDES)
		return;
	state->sandHazardBits &= ~gBitTable[side];
	state->rockCurtainBits &= ~gBitTable[side];
	state->steelSpikesBits &= ~gBitTable[side];
}

void Lv200CustomMovesA_OnScreensRemoved(u8 side)
{
	(void) side;
}

void Lv200CustomMovesA_ResetState(void)
{
	struct Lv200CustomMovesAState *state = Lv200CustomMove_GetStateA();
	u8 *bytes;
	u32 i;

	if (state == NULL)
		return;
	bytes = (u8 *) state;
	for (i = 0; i < sizeof(*state); ++i)
		bytes[i] = 0;
	state->forcedSwitchBank = 0xFF;
}

