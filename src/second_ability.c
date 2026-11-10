#include "defines.h"
#include "defines_battle.h"
#include "../include/constants/abilities.h"
#include "../include/new/battle_util.h"
#include "../include/new/mega.h"
#include "../include/new/ability_battle_scripts.h"
#include "../include/constants/region_map_sections.h"
#include "Tables/second_abilities.h"

/*
    2 つ目の特性（場に出たときだけ効く・2026-10-01）

    ユーザー「建物の中とレベルで特性が増える」→（AskUserQuestion）「2 つ目は場に出たときだけ」「全部最終進化とメガ進化に」
    「タイプごとに決める」「Lv 50」。考えは SoulGold（参考/soulgold の innates）から借りた（コードは自前）。

    ★ CFRU の特性は 1 つだけ（ABILITY(bank)）。2 つ目は「場に出たときの効果だけ」を、その特性を名指しして
       AbilityBattleEffects(ABILITYEFFECT_ON_SWITCHIN, bank, 0, 特性, 0) で動かす（4 つ目の引数 special がそのまま
       gLastUsedAbility になる仕組みが元からある）。ポップアップの名前は gNewBS->abilityPopupOverwrite。
    ★ いかく・おどかしは素の台本が「本当の特性が いかく か」を見るので、自前の台本（second_ability_scripts.s）。
    ★ 1 回だけ: gNewBS->secondAbilitySpecies[bank] に出した種族を覚える（入れ替えで 0・ClearSwitchBytes）。
       メガシンカで種族が変わると、メガの 2 つ目の特性がもう 1 回出る。
    ★ 呼ぶ所: 入れ替え（switching.c の SwitchIn_Abilities）・戦闘の始め（battle_start_turn_start.c の
       BTSTART_SWITCH_IN_ABILITIES の 2 周目）・メガシンカ（Mega_SwitchInAbilities の 2 周目）
*/

bool8 Hack_StartIntimidateLike(u8 bank, bool8 atk);

u16 SecondAbility_OfSpecies(u16 species)
{
	u32 lo = 0, hi = SECOND_ABILITY_COUNT;

	while (lo < hi)
	{
		u32 mid = (lo + hi) / 2;
		if (sSecondAbilities[mid][0] == species)
			return sSecondAbilities[mid][1];
		if (sSecondAbilities[mid][0] < species)
			lo = mid + 1;
		else
			hi = mid;
	}
	return ABILITY_NONE;
}

/* ★ 2026-10-03 ユーザー「お母さんのピカチュウだけ」→「マサラタウンで出会ったの条件を」:
   お母さんのピカチュウ（src/kanto_mom_pikachu.c・自宅でもらう）にも 2 つ目の特性。印は持たず、もらったときの記録で見分ける
   ＝ ピカチュウ・出会った場所がマサラタウン（MAPSEC 0x58）・出会ったレベルが 0 でない（タマゴから孵った子でない）・親が主人公。
   前のセーブのピカチュウにもそのまま効く。マサラタウンには草むらもトレーナーもいないので、ほかのピカチュウはまず当たらない */
#define MOM_PIKACHU_SECOND_ABILITY ABILITY_ELECTRICSURGE    /* でんき → エレキメイカー（TYPE_ABILITY と同じ） */

static bool8 IsMomPikachu(u16 species, struct Pokemon* mon)
{
	u32 otId;

	if ((species != SPECIES_PIKACHU && species != SPECIES_PIKACHU_POP_STAR) || mon == NULL)   //★ 2026-10-09 今のママのピカチュウはポップスター・前のセーブはふつうのピカチュウ
		return FALSE;
	if (GetMonData(mon, MON_DATA_MET_LOCATION, NULL) != MAPSEC_PALLET_TOWN || GetMonData(mon, MON_DATA_MET_LEVEL, NULL) == 0)
		return FALSE;
	otId = gSaveBlock2->playerTrainerId[0] | (gSaveBlock2->playerTrainerId[1] << 8)
	     | (gSaveBlock2->playerTrainerId[2] << 16) | ((u32) gSaveBlock2->playerTrainerId[3] << 24);
	return GetMonData(mon, MON_DATA_OT_ID, NULL) == otId;
}

/* ★ 2026-10-03 トキワシティの人（v0.7 だけの期間限定・2026-10-03 に外した・docs/research/2026-10-03_pikachu-ability-npc.md）が付けた印
   （MON_DATA_OBEDIENCE）。もらった子は外したあとも特性を持ち続けるので、ここは残す。
   素の FRLG がこの印を見るのはミュウ・デオキシスが言うことを聞くかとユニオンルームの掲示板だけ */
bool8 HackMark_IsPlayersMarkedMon(const struct Pokemon* mon); //src/hack_gible_egg.c（★ 2026-10-05 フロンティアの相手も印が付いているので、親が主人公・施設の外だけ）

static bool8 HasPikachuAbilityMark(u16 species, struct Pokemon* mon)
{
	return species == SPECIES_PIKACHU && mon != NULL && HackMark_IsPlayersMarkedMon(mon);
}

static u16 MomPikachuSecondAbility(u16 species, struct Pokemon* mon)
{
	return (IsMomPikachu(species, mon) || HasPikachuAbilityMark(species, mon)) ? MOM_PIKACHU_SECOND_ABILITY : ABILITY_NONE;
}


u16 HackDragonBoulder_SecondAbility(const struct Pokemon* mon, u16 species);
u16 HackVolcarona_SecondAbility(u16 species, const struct Pokemon* mon);   //src/hack_volcarona.c（2026-10-11 特別なウルガモスの 2 つ目 たいようのちょう・#95） //src/hack_dragon_boulder.c（2026-10-07 特別なガブリアスZ → りゅうのいわお）

/* 1 匹の 2 つ目の特性（レベルは見ない・ステータス画面と AI 用） */
u16 SecondAbility_OfMon(struct Pokemon* mon)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u16 ability = HackDragonBoulder_SecondAbility(mon, species);

	if (ability == ABILITY_NONE)
		ability = HackVolcarona_SecondAbility(species, mon);   //[hack] patches/0794 印のあるウルガモスは表（もらいび）より先・たいようのちょう
	if (ability != ABILITY_NONE)
		return ability;
	ability = SecondAbility_OfSpecies(species);

	if (ability == ABILITY_NONE)
		ability = MomPikachuSecondAbility(species, mon);
	return ability;
}

/* ★ 2026-10-20 手持ちの 1 匹の 2 つ目の特性。場に出ていなくても使う AI の先読み（交代先の先制度・すばやさ・型）用。
   Lv が足りない・1 つ目と同じなら ABILITY_NONE（かたやぶり・いえきは場にいないので見ない） */
u16 SecondAbility_OfMonActive(struct Pokemon* mon)
{
	u16 ability;

	if (mon == NULL || GetMonData(mon, MON_DATA_LEVEL, NULL) < SECOND_ABILITY_UNLOCK_LEVEL)
		return ABILITY_NONE;
	ability = SecondAbility_OfMon(mon);
	if (ability == GetMonAbility(mon))
		return ABILITY_NONE;
	return ability;
}

/* ★ 2026-10-03 ステータス画面（src/bw_summary_screen.c）用: そのレベルで 2 つ目の特性が働くか */
bool8 SecondAbility_IsUnlocked(u16 level)
{
	return level >= SECOND_ABILITY_UNLOCK_LEVEL;
}

/* この場のポケモンの 2 つ目の特性（無ければ ABILITY_NONE） */
bool8 SecondAbility_IsIgnored(u8 bank);

u16 SecondAbility_OfBank(u8 bank)
{
	u16 ability;

	if (BM_LEVEL(gBattleMons[bank]) < SECOND_ABILITY_UNLOCK_LEVEL || IsAbilitySuppressed(bank) || SecondAbility_IsIgnored(bank)) //★ 2026-10-20 かたやぶり・かがくへんかガスでも止まる
		return ABILITY_NONE;
	ability = HackDragonBoulder_SecondAbility(SIDE(bank) == B_SIDE_PLAYER ? GetBankPartyData(bank) : NULL, SPECIES(bank));
	if (ability == ABILITY_NONE)
		ability = HackVolcarona_SecondAbility(SPECIES(bank), GetBankPartyData(bank));   //[hack] patches/0794 印のあるウルガモスは表（もらいび）より先（相手側の個体にも効く）
	if (ability == ABILITY_NONE)
		ability = SecondAbility_OfSpecies(SPECIES(bank));
	if (ability == ABILITY_NONE)
		ability = MomPikachuSecondAbility(SPECIES(bank), SIDE(bank) == B_SIDE_PLAYER ? GetBankPartyData(bank) : NULL);
	/* 1 つ目と同じなら無し扱い。1 つ目がかたやぶりで一時的に ABILITY_NONE になっていても、元の 1 つ目と比べる */
	if (ability == ABILITY(bank) || (gNewBS->DisabledMoldBreakerAbilities[bank] != ABILITY_NONE && ability == gNewBS->DisabledMoldBreakerAbilities[bank]))
		return ABILITY_NONE;
	return ability;
}

/* ★ 2026-10-03 ユーザーの動画（ライチュウ Lv58・エレキメイカー）「フィールド系の特性が発動しない」「次に特性が出たポケモンの
   札がエレキメイカーになる」: AbilityBattleEffects に名指しで渡すとフィールド系だけ台本が走らない（原因は未特定・
   docs/research/2026-10-01_second-ability.md）。フィールドは ability_battle_effects.c の TryActivateTerrainAbility と同じ手順を
   ここで踏む（いかく系と同じく自前で台本を積む） */
static bool8 TryStartTerrain(u8 bank, u8 terrain, u8 anim, const u8* string)
{
	if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_CIRCUS && gBattleCircusFlags & BATTLE_CIRCUS_TERRAIN)
		return FALSE; //Can't be removed
	if (gTerrainType == terrain)
		return FALSE;

	gTerrainType = terrain;
	#ifdef INFINITE_TERRAIN
		gNewBS->TerrainTimer = 0;
	#else
		gNewBS->TerrainTimer = (ITEM_EFFECT(bank) == ITEM_EFFECT_TERRAIN_EXTENDER) ? 8 : 5;
	#endif
	gBattleStringLoader = string;
	gBattleScripting.animArg1 = anim;
	gBattleScripting.bank = bank;
	*((u32*) gSeedHelper) = (u32) gBattleStringLoader; //Backup String
	BattleScriptPushCursorAndCallback(BattleScript_TerrainFromAbility);
	return TRUE;
}

/* 場に出たとき（1 回だけ）。効果が出て台本が始まったら TRUE */
bool8 SecondAbility_TrySwitchIn(u8 bank)
{
	u16 species = SPECIES(bank);
	u16 ability;
	u32 done;
	u16 prev;
	bool8 effect;

	if (!BATTLER_ALIVE(bank) || species == SPECIES_NONE || gNewBS->secondAbilitySpecies[bank] == species)
		return FALSE;
	prev = gNewBS->secondAbilitySpecies[bank];
	gNewBS->secondAbilitySpecies[bank] = species;

	/* ★ 2026-10-17 種族が変わったから出し直す、はメガシンカ・ウルトラバースト・ゲンシカイキだけ。
	   チェリムは ひでりの日ざしでポジフォルム（2 つ目 グラスメイカー）に変わったあと、テラスタル／メガシンカの後処理の 2 周目で
	   新しい種族を「場に出た」と見て グラスメイカーが出ていた。フォルムチェンジ（チェリム・ギルガルド等）や
	   テラスタルでは出さない（種族は覚え直すだけ）。
	   ★ 例外: テラパゴスのテラスタルでの姿（ステラフォルム・SPECIES_TERAPAGOS_STELLAR）は出す（ユーザー決定 2026-10-17）。
	   テラスタルフォルム（SPECIES_TERAPAGOS_TERASTAL・テラスシフトで場に出たときの姿）は含めない */
	if (prev != SPECIES_NONE && !IsMegaSpecies(species) && !IsBluePrimalSpecies(species)
	&& !IsRedPrimalSpecies(species) && !IsUltraNecrozmaSpecies(species)
	&& species != SPECIES_TERAPAGOS_STELLAR)
		return FALSE;

	ability = SecondAbility_OfBank(bank);
	if (ability == ABILITY_NONE)
		return FALSE;

	gNewBS->abilityPopupOverwrite = ability;
	switch (ability)
	{
		case ABILITY_INTIMIDATE:
			effect = Hack_StartIntimidateLike(bank, TRUE);
			break;
		case ABILITY_SCARE:
			effect = Hack_StartIntimidateLike(bank, FALSE);
			break;
		case ABILITY_MIWAKUNOUROKO:   //★ 2026-10-11 ミワクノウロコ（ツキノウロコ・いかく と同じ）
			effect = Hack_StartIntimidateLike(bank, TRUE);
			break;
		case ABILITY_ELECTRICSURGE:
			effect = TryStartTerrain(bank, ELECTRIC_TERRAIN, B_ANIM_ELECTRIC_SURGE, ElectricTerrainSetString);
			break;
		case ABILITY_GRASSYSURGE:
			effect = TryStartTerrain(bank, GRASSY_TERRAIN, B_ANIM_GRASSY_SURGE, GrassyTerrainSetString);
			break;
		case ABILITY_MISTYSURGE:
			effect = TryStartTerrain(bank, MISTY_TERRAIN, B_ANIM_MISTY_SURGE, MistyTerrainSetString);
			break;
		case ABILITY_PSYCHICSURGE:
			effect = TryStartTerrain(bank, PSYCHIC_TERRAIN, B_ANIM_PSYCHIC_SURGE, PsychicTerrainSetString);
			break;
		default:
			/* 1 つ目の特性で「場に出たときの特性は済んだ」印が立っていると AbilityBattleEffects が何もしないので、呼ぶ間だけ外す */
			done = gStatuses3[bank] & STATUS3_SWITCH_IN_ABILITY_DONE;
			gStatuses3[bank] &= ~STATUS3_SWITCH_IN_ABILITY_DONE;
			effect = AbilityBattleEffects(ABILITYEFFECT_ON_SWITCHIN, bank, 0, ability, 0);
			/* 印は呼ぶ前のとおりに戻す（AbilityBattleEffects が立てたままだと、あとの段で動く 1 つ目の特性が飛ばされる） */
			gStatuses3[bank] = (gStatuses3[bank] & ~STATUS3_SWITCH_IN_ABILITY_DONE) | done;
			break;
	}

	if (!effect)
		gNewBS->abilityPopupOverwrite = 0;
	return effect;
}

/* ================================================================================================
   2 つ目の特性を『常に効く』ようにする（FEAT-second-ability-passive・2026-10-20）
   ユーザー「特性2が元々場に出たときの効果用に作っていたから、そこカスタマイズしておいて」。
   方針（docs/research/2026-10-20_second-ability-passive.md）:
   ★ 判定は『1 つ目 ABILITY(bank) か 2 つ目 SecondAbility_OfBank(bank)』。2 つ目は ABILITY(bank) を書き換えない。
   ★ 止まる条件は 1 つ目と同じ: いえき（Gastro Acid）・かがくへんかガス・かたやぶり系に無視される（secondAbilityIgnored）。
   ★ 特性を入れ替える技（スキルスワップ・なかまづくり・トレース・シンプルビーム等）は 1 つ目だけに効く。2 つ目は残る。
   ★ 1 つ目と同じ特性なら 2 つ目は無い扱い（SecondAbility_OfBank が ABILITY_NONE を返す）＝二重に効かない。
   ★ 入口は 3 つ: (1) BankHasAbility（比較 ABILITY(x)==ABILITY_T を tools/abilities/route_second_abilities.py が置き換える）
     (2) AbilityBattleEffects の包み（技を受けた・当てた・守る・場の特性の数え上げ）(3) end_turn.c の SecondAbility_TryEndTurn
   ================================================================================================ */

/* ★ 2026-10-22 BUG-ability2-followup: かたやぶり系は 1 つ目と 2 つ目を別々に見る（無視できる特性だけ無視）。
   以前は 1 つ目が無視されている（DisabledMoldBreakerAbilities）だけで 2 つ目まで巻き添えにしていた */
bool8 SecondAbility_IsIgnored(u8 bank)
{
	return gNewBS->neutralizingGasBlockedAbilities[bank] != ABILITY_NONE
		|| (gNewBS->hackSecondAbilityIgnored & gBitTable[bank]) != 0;
}

bool8 BankHasAbility(u8 bank, u16 ability)
{
	if (ABILITY(bank) == ability)
		return TRUE;
	return ability != ABILITY_NONE && SecondAbility_OfBank(bank) == ability;
}

u16 SecondAbility_AsIf(u8 bank, u16 ability)
{
	return BankHasAbility(bank, ability) ? ability : ABILITY(bank);
}

void Hack_SturdyPopupName(void)
{
	u8 bank = gBattleScripting.bank;

	if (bank < gBattlersCount && ABILITY(bank) != ABILITY_STURDY && SecondAbility_OfBank(bank) == ABILITY_STURDY)
		gNewBS->abilityPopupOverwrite = ABILITY_STURDY;
}

/* かたやぶり系の判定に渡す特性: 1 つ目がかたやぶり系ならそれ・そうでなく 2 つ目がかたやぶり系なら 2 つ目 */
u16 SecondAbility_MoldBreakerAbility(u8 bank)
{
	u16 ability = ABILITY(bank);

	if (!IsMoldBreakerAbility(ability))
	{
		u16 second = SecondAbility_OfBank(bank);
		if (IsMoldBreakerAbility(second))
			return second;
	}
	return ability;
}

extern u8 AbilityBattleEffectsCore(u8 caseID, u8 bank, u16 ability, u16 special, u16 moveArg);

/* ★ 2026-10-22 BUG-ability2-followup: 接触系（技を受けた・当てた）は 1 つ目が出ても 2 つ目も出す。
   台本を 2 つ同時に積むと MOVE_EFFECT_BYTE・gBattleMoveDamage が共有で上書きし合う（2 つ目せいでんき＋1 つ目どくのトゲ等）ので、
   1 つ目の台本が終わって呼び元（cmd49.c／new_bs_commands.c の段）に戻ってから、同じ段でもう 1 回呼んで 2 つ目だけを走らせる。
   1 つ目が出たら『順番待ち』を置き、呼び元は待ちの間だけ段を進めない（SecondAbility_PendingNext）。
   待ちは技・持ち主・ターンの手番が変わったら捨てる（持ち越さない）。呼び元の段の頭（技の始まり）でも消す（SecondAbility_ClearPending）。 */
static bool8 SecondAbility_PendKey(u8 caseID, u8 bank)
{
	return gNewBS->hackSecondPendCase == caseID + 1 && gNewBS->hackSecondPendBank == bank
		&& gNewBS->hackSecondPendAct == gCurrentTurnActionNumber && gNewBS->hackSecondPendMove == gCurrentMove;
}

void SecondAbility_ClearPending(void)
{
	gNewBS->hackSecondPendCase = 0;
}

/* 呼び元用: 直前の AbilityBattleEffects が 1 つ目を出し 2 つ目が順番待ちなら TRUE（段を進めずに同じ段を続ける） */
bool8 SecondAbility_PendingNext(void)
{
	return gNewBS->hackSecondPendCase != 0;
}

/* AbilityBattleEffects の本体（上流・ability_battle_effects.c の AbilityBattleEffectsCore）を 1 つ目で呼び、
   効果が出なかったら 2 つ目の特性を special に渡してもう 1 回呼ぶ。special が 0 でない呼びは素通し（2 つ目自身の呼び・天気） */
u8 AbilityBattleEffects(u8 caseID, u8 bank, u16 ability, u16 special, u16 moveArg)
{
	u8 effect;
	u16 second;
	u8 i, side;

	if (special == 0 && (caseID == ABILITYEFFECT_CONTACT || caseID == ABILITYEFFECT_MOVE_END_ATTACKER)
	&& gNewBS->hackSecondPendCase != 0)
	{
		bool8 mine = SecondAbility_PendKey(caseID, bank);

		gNewBS->hackSecondPendCase = 0; //待ちは 1 回で消す（合わない待ちは捨てて、ふつうの呼びとして続ける）
		if (mine)
		{
			if (bank >= gBattlersCount)
				return 0;
			second = SecondAbility_OfBank(bank);
			if (second == ABILITY_NONE)
				return 0;
			effect = AbilityBattleEffectsCore(caseID, bank, ability, second, moveArg);
			if (effect)
				gNewBS->abilityPopupOverwrite = second;
			return effect;
		}
	}

	effect = AbilityBattleEffectsCore(caseID, bank, ability, special, moveArg);

	if (special != 0)
		return effect;

	switch (caseID)
	{
		case ABILITYEFFECT_CONTACT:
		case ABILITYEFFECT_MOVE_END_ATTACKER:
			if (effect)
			{
				if (bank < gBattlersCount && SecondAbility_OfBank(bank) != ABILITY_NONE)
				{
					gNewBS->hackSecondPendCase = caseID + 1;
					gNewBS->hackSecondPendBank = bank;
					gNewBS->hackSecondPendAct = gCurrentTurnActionNumber;
					gNewBS->hackSecondPendMove = gCurrentMove;
				}
				return effect;
			}
			/* fallthrough: 1 つ目が出なかったら 2 つ目を試す */
		case ABILITYEFFECT_MOVES_BLOCK:
		case ABILITYEFFECT_MOVES_BLOCK_PARTNER:
		case ABILITYEFFECT_ABSORBING:
			if (effect || bank >= gBattlersCount)
				return effect;
			second = SecondAbility_OfBank(bank);
			if (second == ABILITY_NONE)
				return 0;
			effect = AbilityBattleEffectsCore(caseID, bank, ability, second, moveArg);
			if (effect)
				gNewBS->abilityPopupOverwrite = second;
			return effect;

		case ABILITYEFFECT_IMMUNITY:   //場の全員を ABILITY(bank) で見る → 2 つ目を持つ子は呼ぶ間だけ 1 つ目の位置に置く
			if (effect)
				return effect;
			for (i = 0; i < gBattlersCount; ++i)
			{
				u16 saved = ABILITY(i);

				second = SecondAbility_OfBank(i);
				if (second == ABILITY_NONE)
					continue;
				ABILITY(i) = second;
				effect = AbilityBattleEffectsCore(caseID, 0, 0, 0, 0);
				ABILITY(i) = saved;
				if (effect)
				{
					gNewBS->abilityPopupOverwrite = second;
					return effect;
				}
			}
			return 0;

		case ABILITYEFFECT_CHECK_OTHER_SIDE:
		case ABILITYEFFECT_CHECK_BANK_SIDE:
		case ABILITYEFFECT_CHECK_FIELD_EXCEPT_BANK:
		case ABILITYEFFECT_CHECK_ON_FIELD:
		case ABILITYEFFECT_COUNT_OTHER_SIDE:
		case ABILITYEFFECT_COUNT_BANK_SIDE:
		case ABILITYEFFECT_COUNT_ON_FIELD:
			if (ability == ABILITY_NONE)
				return effect;
			side = SIDE(bank);
			for (i = 0; i < gBattlersCount; ++i)
			{
				bool8 match;

				if (SecondAbility_OfBank(i) != ability)
					continue;
				switch (caseID)
				{
					case ABILITYEFFECT_CHECK_OTHER_SIDE:
					case ABILITYEFFECT_COUNT_OTHER_SIDE:
						match = SIDE(i) != side;
						break;
					case ABILITYEFFECT_CHECK_BANK_SIDE:
					case ABILITYEFFECT_COUNT_BANK_SIDE:
						match = SIDE(i) == side;
						break;
					case ABILITYEFFECT_CHECK_FIELD_EXCEPT_BANK:
					case ABILITYEFFECT_COUNT_ON_FIELD:
						match = i != bank;
						break;
					default:   //CHECK_ON_FIELD
						match = BATTLER_ALIVE(i);
						break;
				}
				if (!match)
					continue;
				if (caseID >= ABILITYEFFECT_COUNT_OTHER_SIDE && caseID <= ABILITYEFFECT_COUNT_ON_FIELD)
					++effect;
				else if (effect == 0)
				{
					gLastUsedAbility = ability;
					effect = i + 1;
				}
			}
			return effect;
	}
	return effect;
}

/* ターンの終わり（end_turn.c の各段）: 1 つ目の特性の段のすぐ後ろで呼ぶ。段ごとに 1 つ目と同じ特性の一覧 */
bool8 SecondAbility_TryEndTurn(u8 bank, u8 slot)
{
	u16 second = SecondAbility_OfBank(bank);
	bool8 ok = FALSE;
	u8 effect;

	switch (slot)
	{
		case SECOND_ET_WEATHER:
			ok = second == ABILITY_RAINDISH || second == ABILITY_DRYSKIN || second == ABILITY_ICEBODY || second == ABILITY_SOLARPOWER;
			break;
		case SECOND_ET_SPEEDBOOST:
			ok = second == ABILITY_SPEEDBOOST || second == ABILITY_TRUANT || second == ABILITY_MOODY
			  || second == ABILITY_BADDREAMS || second == ABILITY_SLOWSTART || second == ABILITY_CUDCHEW;
			break;
		case SECOND_ET_HYDRATION:
			ok = second == ABILITY_SHEDSKIN || second == ABILITY_HYDRATION || second == ABILITY_HEALER;
			break;
		case SECOND_ET_HARVEST:
			ok = second == ABILITY_HARVEST || second == ABILITY_PICKUP || second == ABILITY_BALLFETCH;
			break;
	}
	if (!ok)
		return FALSE;
	effect = AbilityBattleEffectsCore(ABILITYEFFECT_ENDTURN, bank, 0, second, 0);
	if (effect)
		gNewBS->abilityPopupOverwrite = second;
	return effect != 0;
}
