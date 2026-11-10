#include "defines.h"
#include "hack_base_stats_wide.h" //[hack] patches/0678 種族値の u16 の窓口
#include "defines_battle.h"
#include "../include/random.h"
#include "../include/daycare.h"
#include "../include/malloc.h"
#include "../include/new/build_pokemon.h"
#include "../include/new/learn_move.h"

u8 GetAllBadgeCount(void);      //src/badge_count.c（全地方の合計）

/*
    トレーナーのクラスごとの手持ちの候補（2026-10-11 ユーザー「list_trainer_pokemon.md これ前のきんこんしきみたいにやってほしい」）。
    資料: docs/research/2026-10-11_class-pools.md

    選んだこと: もらった表のクラスのトレーナー・手持ちの数とレベルは元のまま・Smogon の型・戦うたびに選び直す。
    CFRU の CreateNPCTrainerParty（src/build_pokemon.c・patches/0305）が 1 匹ずつ作ったあと、難易度の調整（HackDifficulty_AdjustTrainerMon）の前に
    これを呼ぶ。そのトレーナーのクラスに候補があれば、その子を候補から選んだ種族で作り直す（レベル・親の名前と ID・性格値・個体値・ボール・なつき度は
    元の子のまま）。型（技・持ち物・性格・特性・努力値・テラ）は tools/rom/build_class_pools.py の表。型の無い種族はレベルで覚える技・持ち物なし。
    同じ手持ちの中では同じ種族を選ばない（候補が足りなければ重なってよい）。
    やさしい・ほどほどの難易度は、このあとの HackDifficulty_AdjustTrainerMon が努力値・持ち物を外す（今までどおり）。
*/

struct HackClassPoolSet
{
	u16 species;
	u16 item;
	u16 moves[MAX_MON_MOVES];
	u8 nature;
	u8 ability;          //0 / 1 / 0xFF（隠れ）
	u8 evs[NUM_STATS];   //HP 攻撃 防御 素早さ 特攻 特防
	u8 tera;             //0 = 元のタイプ・それ以外はタイプ + 1
	bool8 noSet;         //型なし（レベルで覚える技・持ち物なし）
};

struct HackClassPool
{
	const u16* sets;      //1 匹ずつの候補（型の表の番号）
	u16 count;
	const u16 (*pairs)[2]; //2 匹の組（ふたごちゃん・カップル・きんこんしき）
	u16 pairCount;
};

//ジムの地図ごとの候補（2026-10-11 #97・tools/rom/gym_pools.md）
struct HackGymPool
{
	u8 mapGroup;
	u8 mapNum;
	const u16* sets;
	u16 count;
};

#define HACK_CLASS_POOL_CLASSES 107   //ROM のクラス名の表の数（tools/data_editor/trainerlib.py の CLASS_NAME_COUNT）
#include "Tables/hack_class_pools.h"

//候補を使わないトレーナー（自分の作りがある人）
static const u16 sExcludedTrainers[] =
{
	935, //モモカ博士（tools/kanto/build_kanto_trainers.py の momoka_target）
	936, //きんこんしき ゲンゾウ（src/hack_golden_wedding.c）
	937, //きんこんしき ハナ
	938, //ヒガン（作者 ふたーら 氏・tools/oritore/higan）
	939, //ヒガン（再戦）
	940, 941, 942, 943, 944, 945, 946, 947, 948, 949, //隠しトレーナー わたしたち（#99・src/hack_watashitachi.c・レントラーの♀は並びで決めている）
};

static bool8 IsExcluded(u16 trainerId)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sExcludedTrainers); ++i)
	{
		if (sExcludedTrainers[i] == trainerId)
			return TRUE;
	}
	return FALSE;
}

static bool8 SpeciesUsedBefore(struct Pokemon* party, u8 index, u16 species)
{
	u32 i;

	for (i = 0; i < index; ++i)
	{
		if (GetMonData(&party[i], MON_DATA_SPECIES, NULL) == species)
			return TRUE;
	}
	return FALSE;
}

//Lv50 未満だけ、序盤に出る種族の強さを段階的に抑える。Lv50 以上は 0 = 制限なし。
u16 HackClassPool_EarlyBstCap(u16 level)
{
	if (level < 15)
		return 350;
	if (level < 30)
		return 450;
	if (level < 50)
		return 550;
	return 0;
}

//ホスト検算からも境界を直接確かめられる、表に依存しない判定。
bool8 HackClassPool_EarlyBstAllowed(u16 bst, u16 level)
{
	u16 cap = HackClassPool_EarlyBstCap(level);
	return cap == 0 || bst <= cap;
}

//トレーナーの手持ちを合計で選ぶ所が読む合計。種族値の底上げ（案 C）の対象 141 匹は底上げ前の合計（ユーザー「トレーナーはもとの表のままで」・表は hack_class_pools.h の sHackClassPoolBasisBst）。ほかは今の種族値の合計。
u32 HackTrainerBasisBst(u16 species)
{
	u32 i;

	for (i = 0; i < HACK_CLASS_POOL_BASIS_BST; ++i)
	{
		if (sHackClassPoolBasisBst[i].species == species)
			return sHackClassPoolBasisBst[i].total;
	}
	return HackBaseStatsTotal(species); //[hack] patches/0678
}

static u16 SpeciesBst(u16 species)
{
	return HackTrainerBasisBst(species);
}

static bool8 IsPermanentEvolution(u16 method)
{
	return method != EVO_NONE && method < EVO_TERASTAL;
}

struct EarlyEvolutionInfo
{
	u16 previous;
	u8 stage;
};

//第1 = 未進化、第2 = 1 回進化、第3 = 2 回以上。候補を調べる前に 1 回だけ全種族分を作る。
static void BuildEvolutionInfo(struct EarlyEvolutionInfo* info)
{
	u32 pass, species, i;

	for (species = 0; species < NUM_SPECIES; ++species)
	{
		info[species].previous = species;
		info[species].stage = 1;
	}
	//表の並びに依存しないよう 3 回緩和する。3 段目より先は同じ扱いでよい。
	for (pass = 0; pass < 3; ++pass)
	{
		for (species = 0; species < NUM_SPECIES; ++species)
		{
			for (i = 0; i < EVOS_PER_MON; ++i)
			{
				const struct Evolution* evo = &gEvolutionTable[species][i];
				u8 nextStage;

				if (!IsPermanentEvolution(evo->method) || evo->targetSpecies >= NUM_SPECIES)
					continue;
				nextStage = info[species].stage < 3 ? info[species].stage + 1 : 3;
				if (info[evo->targetSpecies].stage < nextStage)
				{
					info[evo->targetSpecies].stage = nextStage;
					info[evo->targetSpecies].previous = species;
				}
			}
		}
	}
}

bool8 HackClassPool_EarlyStageAllowed(u8 stage, u8 badges)
{
	if (badges >= 3)
		return TRUE;
	return stage <= (badges >= 2 ? 2 : 1);
}

static bool8 StageAllowedWithBadges(u16 species, u8 badges, const struct EarlyEvolutionInfo* info)
{
	return badges >= 3 || HackClassPool_EarlyStageAllowed(info[species].stage, badges);
}

//候補が強すぎるときは、同じ進化系を前へ戻して最初に条件を満たす姿を使う。
static u16 EarlySpeciesAtLevel(u16 species, u16 level, u8 badges, const struct EarlyEvolutionInfo* info)
{
	u32 tries;

	for (tries = 0; tries < 3 && species != SPECIES_NONE && species < NUM_SPECIES; ++tries)
	{
		if (HackClassPool_EarlyBstAllowed(SpeciesBst(species), level)
		 && StageAllowedWithBadges(species, badges, info))
			return species;
		if (info[species].previous == species)
			break;
		species = info[species].previous;
	}
	return SPECIES_NONE;
}

static u16 SetSpeciesAtLevel(const struct HackClassPoolSet* set, u16 level, u8 badges, const struct EarlyEvolutionInfo* info)
{
	return EarlySpeciesAtLevel(set->species, level, badges, info);
}

u8 HackClassPool_EarlyMovePowerCap(u16 level)
{
	if (level < 15)
		return 60;
	if (level < 30)
		return 80;
	if (level < 50)
		return 110;
	return 120;
}

static bool8 MoveAlreadyChosen(const u16 chosen[MAX_MON_MOVES], u16 move)
{
	u32 i;
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		if (chosen[i] == move)
			return TRUE;
	}
	return FALSE;
}

static bool8 IsEarlyRiskySetup(u16 move, u16 level)
{
	u8 effect;

	if (level >= 30 || gBattleMoves[move].power != 0)
		return FALSE;
	effect = gBattleMoves[move].effect;
	return effect == EFFECT_CALM_MIND || effect == EFFECT_DRAGON_DANCE
		|| effect == EFFECT_EXTREME_EVOBOOST || effect == EFFECT_BULK_UP
		|| effect == EFFECT_ATK_SPATK_UP || effect == EFFECT_ATK_ACC_UP;
}

static bool8 IsEarlyRiskyAttack(u16 move, u16 level)
{
	u8 effect;

	if (level >= 15)
		return FALSE;
	effect = gBattleMoves[move].effect;
	return effect == EFFECT_0HKO || effect == EFFECT_DRAGON_RAGE || effect == EFFECT_SONICBOOM
		|| effect == EFFECT_ROLLOUT || effect == EFFECT_FURY_CUTTER;
}

static bool8 IsLevelLegalMove(const struct MovePair* pair, u16 level)
{
	return pair->move != MOVE_NONE && pair->num <= level;
}

static u16 BestEarlyAttack(const struct MovePair* moves, u16 count, u16 level, u8 type1, u8 type2,
	const u16 chosen[MAX_MON_MOVES], bool8 wantStab, bool8 avoidChosenTypes, bool8 weakest)
{
	u32 i;
	u16 best = MOVE_NONE;
	u16 bestPower = weakest ? 0xFFFF : 0; //[hack] patches/0582 (#126): 威力は 255 をこえる
	u8 cap = HackClassPool_EarlyMovePowerCap(level);

	for (i = 0; i < count; ++i)
	{
		u16 move = moves[i].move;
		u8 type;
		u16 power; //[hack] patches/0582 (#126)
		u32 j;
		bool8 usedType = FALSE;

		if (!IsLevelLegalMove(&moves[i], level) || MoveAlreadyChosen(chosen, move))
			continue;
		power = MOVE_POWER(move); //[hack] patches/0582 (#126)
		type = gBattleMoves[move].type;
		if (power == 0 || IsEarlyRiskyAttack(move, level) || (!weakest && power > cap))
			continue;
		if (wantStab && type != type1 && type != type2)
			continue;
		if (avoidChosenTypes)
		{
			for (j = 0; j < MAX_MON_MOVES; ++j)
			{
				if (chosen[j] != MOVE_NONE && gBattleMoves[chosen[j]].power != 0
				 && gBattleMoves[chosen[j]].type == type)
					usedType = TRUE;
			}
			if (usedType)
				continue;
		}
		if ((weakest && power < bestPower) || (!weakest && power > bestPower))
		{
			best = move;
			bestPower = power;
		}
	}
	return best;
}

static void BalanceEarlyMoves(struct Pokemon* mon, u16 level)
{
	//[hack] patches/0598: 学習表は最大 約 410 件＋覚えている 4 技。スタックに置かず（6.35）ヒープへ。件数は u16
	struct MovePair* moves = Calloc(sizeof(struct MovePair) * (MAX_LEVEL_UP_MOVES + MAX_MON_MOVES));
	u16 chosen[MAX_MON_MOVES] = {MOVE_NONE};
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u16 count;
	u8 type1 = gBaseStats[species].type1;
	u8 type2 = gBaseStats[species].type2;
	u8 slots = 0, statusCount = 0;
	u32 i;
	u16 move;

	if (moves == NULL)
		return;
	count = GetLevelUpMovePairsBySpecies(species, moves);

	//Lv50〜65 は従来の競技型も候補に残し、役割と補完技を落としすぎない。
	//新しい技を作るのではなく、この時点ですでに入っている 4 技だけを足す。
	if (level >= 50)
	{
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			u32 j;
			bool8 duplicate = FALSE;
			move = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
			for (j = 0; j < count; ++j)
			{
				if (moves[j].move == move)
					duplicate = TRUE;
			}
			if (move != MOVE_NONE && !duplicate && count < MAX_LEVEL_UP_MOVES + MAX_MON_MOVES)
			{
				moves[count].move = move;
				moves[count].num = 0;
				++count;
			}
		}
	}

	move = BestEarlyAttack(moves, count, level, type1, type2, chosen, TRUE, FALSE, FALSE);
	if (move != MOVE_NONE)
		chosen[slots++] = move;
	move = BestEarlyAttack(moves, count, level, type1, type2, chosen, FALSE, TRUE, FALSE);
	if (move != MOVE_NONE && slots < MAX_MON_MOVES)
		chosen[slots++] = move;

	//新しく覚える方から、極端な積み技ではない補助技を 1 つ。
	for (i = count; i > 0 && slots < MAX_MON_MOVES; --i)
	{
		move = moves[i - 1].move;
		if (IsLevelLegalMove(&moves[i - 1], level) && !MoveAlreadyChosen(chosen, move)
		 && gBattleMoves[move].power == 0 && !IsEarlyRiskySetup(move, level))
		{
			chosen[slots++] = move;
			statusCount = 1;
			break;
		}
	}
	move = BestEarlyAttack(moves, count, level, type1, type2, chosen, FALSE, FALSE, FALSE);
	if (move != MOVE_NONE && slots < MAX_MON_MOVES)
		chosen[slots++] = move;

	//上限内の攻撃が 1 つも無い種族でも、自然習得の最弱攻撃を 1 つは残す。
	if (slots == statusCount)
	{
		move = BestEarlyAttack(moves, count, level, type1, type2, chosen, FALSE, FALSE, TRUE);
		if (move != MOVE_NONE && slots < MAX_MON_MOVES)
			chosen[slots++] = move;
	}

	//残りは新しく覚える方から。補助技は合計 2 つまで。
	for (i = count; i > 0 && slots < MAX_MON_MOVES; --i)
	{
		move = moves[i - 1].move;
		if (!IsLevelLegalMove(&moves[i - 1], level) || MoveAlreadyChosen(chosen, move)
		 || IsEarlyRiskySetup(move, level) || IsEarlyRiskyAttack(move, level))
			continue;
		if (gBattleMoves[move].power == 0 && statusCount >= 2)
			continue;
		if (MOVE_POWER(move) > HackClassPool_EarlyMovePowerCap(level)) //[hack] patches/0582 (#126)
			continue;
		if (gBattleMoves[move].power == 0)
			++statusCount;
		chosen[slots++] = move;
	}

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u8 pp = chosen[i] == MOVE_NONE ? 0 : gBattleMoves[chosen[i]].pp;
		SetMonData(mon, MON_DATA_MOVE1 + i, &chosen[i]);
		SetMonData(mon, MON_DATA_PP1 + i, &pp);
	}
	Free(moves);
}

//[hack] patches/0600: 敵トレーナーに持たせない新技（CSV の npc_ok=N・Lv1〜100 と Lv101〜200）。
//新技はレベルアップの習得表に入っていて、トレーナーの手持ちは CreateMon の自然な技・BalanceEarlyMoves（威力の上限つき）で決まる。
//そのうち大博打・まもる系・ひんしになる技などは敵に使わせない。ここで外して、その子が覚える安全な最新の技に入れ替える。
//表は tools/moves/build_npc_unsafe_moves.py の生成物（技番号の昇順・二分探索）。型の決まったトレーナーの技は元から新技を含まない。
#include "Tables/hack_npc_unsafe_moves.h"

#ifdef EXPAND_MOVESETS
	extern const struct LevelUpMove* const gLevelUpLearnsets[];
#else
	#define gLevelUpLearnsets ((struct LevelUpMove**) *((u32*) 0x804346C))
#endif

static bool8 IsNpcUnsafeMove(u16 move)
{
	u32 low = 0, high = HACK_NPC_UNSAFE_MOVE_COUNT;

	while (low < high)
	{
		u32 mid = (low + high) >> 1;

		if (sHackNpcUnsafeMoves[mid] == move)
			return TRUE;
		if (sHackNpcUnsafeMoves[mid] < move)
			low = mid + 1;
		else
			high = mid;
	}
	return FALSE;
}

//level 以下で覚える技のうち、いちばん新しい（表の後ろ）安全な技。威力は早期の上限以下・積み技／固定ダメージの禁止も従来どおり。無ければ MOVE_NONE
static u16 NewestSafeLearnsetMove(u16 species, u16 level, const u16 kept[MAX_MON_MOVES])
{
	const struct LevelUpMove* learnset = gLevelUpLearnsets[species];
	u16 best = MOVE_NONE;
	u32 i;

	for (i = 0; !(learnset[i].move == 0 && learnset[i].level == 0xFF); ++i)
	{
		u16 move = learnset[i].move;

		if (learnset[i].level > level || move == MOVE_NONE || MoveAlreadyChosen(kept, move) || IsNpcUnsafeMove(move))
			continue;
		if (MOVE_POWER(move) > HackClassPool_EarlyMovePowerCap(level)
		 || IsEarlyRiskySetup(move, level) || IsEarlyRiskyAttack(move, level))
			continue;
		best = move;
	}
	return best;
}

//build_pokemon.c の CreateNPCTrainerParty の最後（難易度・ジムの再戦のレベル調整のあと）に呼ぶ。相手側の NPC だけ・自作の作りの人（IsExcluded）は触らない
void HackClassPool_FixNpcMoves(struct Pokemon* mon, u16 trainerId, u8 side)
{
	u16 kept[MAX_MON_MOVES] = {MOVE_NONE};
	u16 species, move;
	u16 level, total = 0, n = 0;
	u32 i;

	if (side != B_SIDE_OPPONENT || IsExcluded(trainerId))
		return;
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		move = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
		if (move == MOVE_NONE)
			continue;
		++total;
		if (!IsNpcUnsafeMove(move))
			kept[n++] = move;
	}
	if (n == total)
		return; //外す技が無い（ほとんどの手持ちはここで終わる）

	species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	level = GetMonData(mon, MON_DATA_LEVEL, NULL);
	while (n < total)
	{
		move = NewestSafeLearnsetMove(species, level, kept);
		if (move == MOVE_NONE)
			break;
		kept[n++] = move;
	}
	if (n == 0)
		kept[n++] = MOVE_TACKLE; //安全な技が 1 つも無い（ほぼ起きない）ときの保険

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u8 pp = kept[i] == MOVE_NONE ? 0 : gBattleMoves[kept[i]].pp;
		SetMonData(mon, MON_DATA_MOVE1 + i, &kept[i]);
		SetMonData(mon, MON_DATA_PP1 + i, &pp);
	}
}

static u16 CountAllowedSets(const u16* sets, u16 count, u16 level, u8 badges, const struct EarlyEvolutionInfo* info)
{
	u16 i, allowed = 0;
	for (i = 0; i < count; ++i)
	{
		if (SetSpeciesAtLevel(&gHackClassPoolSets[sets[i]], level, badges, info) != SPECIES_NONE)
			++allowed;
	}
	return allowed;
}

static const struct HackClassPoolSet* AllowedSetAt(const u16* sets, u16 count, u16 level, u8 badges, const struct EarlyEvolutionInfo* info, u16 ordinal)
{
	u16 i;
	for (i = 0; i < count; ++i)
	{
		const struct HackClassPoolSet* set = &gHackClassPoolSets[sets[i]];
		if (SetSpeciesAtLevel(set, level, badges, info) != SPECIES_NONE && ordinal-- == 0)
			return set;
	}
	return NULL;
}

static bool8 PairAllowedAtLevel(const u16 pair[2], u16 level, u8 badges, const struct EarlyEvolutionInfo* info)
{
	return SetSpeciesAtLevel(&gHackClassPoolSets[pair[0]], level, badges, info) != SPECIES_NONE
		&& SetSpeciesAtLevel(&gHackClassPoolSets[pair[1]], level, badges, info) != SPECIES_NONE;
}

static u16 CountAllowedPairs(const struct HackClassPool* pool, u16 level, u8 badges, const struct EarlyEvolutionInfo* info, u16 firstSpecies)
{
	u16 i, allowed = 0;
	for (i = 0; i < pool->pairCount; ++i)
	{
		if (PairAllowedAtLevel(pool->pairs[i], level, badges, info)
		 && (firstSpecies == SPECIES_NONE || SetSpeciesAtLevel(&gHackClassPoolSets[pool->pairs[i][0]], level, badges, info) == firstSpecies))
			++allowed;
	}
	return allowed;
}

static const struct HackClassPoolSet* AllowedPairSetAt(const struct HackClassPool* pool, u16 level, u8 badges, const struct EarlyEvolutionInfo* info, u16 firstSpecies, u16 ordinal, u8 member)
{
	u16 i;
	for (i = 0; i < pool->pairCount; ++i)
	{
		if (PairAllowedAtLevel(pool->pairs[i], level, badges, info)
		 && (firstSpecies == SPECIES_NONE || SetSpeciesAtLevel(&gHackClassPoolSets[pool->pairs[i][0]], level, badges, info) == firstSpecies)
		 && ordinal-- == 0)
			return &gHackClassPoolSets[pool->pairs[i][member]];
	}
	return NULL;
}

//ジムリーダーのクラス（カントー・ジョウト・シンオウ = 84、ホウエン = 24）。1 回目も再戦も
bool8 HackClassPool_IsGymLeaderClass(u8 trainerClass)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sHackGymLeaderClasses); ++i)
	{
		if (sHackGymLeaderClasses[i] == trainerClass)
			return TRUE;
	}
	return FALSE;
}

//今いる地図がジムで、リーダーでなければ、その町のジムの候補
static const struct HackGymPool* GymPoolFor(u8 trainerClass)
{
	u32 i;
	u8 group = gSaveBlock1->location.mapGroup;
	u8 num = (u8) gSaveBlock1->location.mapNum;   //6.60: mapNum は s8

	if (HackClassPool_IsGymLeaderClass(trainerClass))
		return NULL;
	for (i = 0; i < HACK_GYM_POOL_MAPS; ++i)
	{
		if (gHackGymPools[i].mapGroup == group && gHackGymPools[i].mapNum == num)
			return &gHackGymPools[i];
	}
	return NULL;
}

void HackClassPool_ReplaceMon(struct Pokemon* mon, struct Pokemon* party, u8 index, u16 trainerId, u8 side)
{
	const struct HackClassPool* pool;
	const struct HackGymPool* gym;
	const struct HackClassPoolSet* set;
	struct HackClassPoolSet fallbackSet = {0};
	struct EarlyEvolutionInfo* evoInfo = NULL;
	u8 trainerClass, level, badges = 3, otGender, ball, friendship, fixedIv, otName[PLAYER_NAME_LENGTH + 1];
	u32 otId, personality, tries, i;
	u16 move, allowed, earlySpecies = SPECIES_NONE;
	bool8 early;

	if (side != B_SIDE_OPPONENT || IsExcluded(trainerId))
		return;
	//ジムリーダー（1 回目・再戦とも）は表の手持ちをそのまま使う。序盤の退化・種族値の上限・段階の制限と、技の整え直しも掛けない
	//（#130 カントーのジムのバランス: カスミのスターミーがヒトデマンに、マチスのライチュウがピカチュウに戻されていた）
	if (HackClassPool_IsGymLeaderClass(gTrainers[trainerId].trainerClass))
		return;
	level = GetMonData(mon, MON_DATA_LEVEL, NULL);
	early = level < 50;
	if (early)
	{
		badges = GetAllBadgeCount();
		evoInfo = AllocZeroed(NUM_SPECIES * sizeof(*evoInfo));
		if (evoInfo == NULL)
			return;
		BuildEvolutionInfo(evoInfo);
	}
	trainerClass = gTrainers[trainerId].trainerClass;
	set = NULL;
	gym = GymPoolFor(trainerClass);
	if (gym != NULL)
	{
		//ジムトレーナー: クラスの候補より先にジムの候補（組のクラスでも 1 匹ずつ）
		if (early)
		{
			allowed = CountAllowedSets(gym->sets, gym->count, level, badges, evoInfo);
			if (allowed == 0)
				goto CLEANUP;
			set = AllowedSetAt(gym->sets, gym->count, level, badges, evoInfo, Random() % allowed);
			earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
			for (tries = 0; tries < 20 && SpeciesUsedBefore(party, index, earlySpecies); ++tries)
			{
				set = AllowedSetAt(gym->sets, gym->count, level, badges, evoInfo, Random() % allowed);
				earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
			}
		}
		else
		{
			set = &gHackClassPoolSets[gym->sets[Random() % gym->count]];
			for (tries = 0; tries < 20 && SpeciesUsedBefore(party, index, set->species); ++tries)
				set = &gHackClassPoolSets[gym->sets[Random() % gym->count]];
		}
		goto REPLACE;
	}
	if (trainerClass >= HACK_CLASS_POOL_CLASSES)
		goto CLEANUP;
	pool = &gHackClassPools[trainerClass];
	if (!early && pool->count == 0 && pool->pairCount == 0)
	{
		//候補表が空のクラスも Lv50〜65 は元個体の種族・型を保ち、技だけ整える。
		if (level <= 65)
			BalanceEarlyMoves(mon, level);
		goto CLEANUP;
	}
	if (early && pool->count == 0 && pool->pairCount == 0)
	{
		earlySpecies = EarlySpeciesAtLevel(GetMonData(mon, MON_DATA_SPECIES, NULL), level, badges, evoInfo);
		if (earlySpecies == SPECIES_NONE)
			goto CLEANUP;
		fallbackSet.species = earlySpecies;
		fallbackSet.noSet = TRUE;
		set = &fallbackSet;
		goto REPLACE;
	}
	if (pool->pairCount != 0)
	{
		/* 組のクラス: 偶数番目（1・3・5 匹目）は組を選んでその 1 匹目、奇数番目は 1 つ前の子が 1 匹目の組の 2 匹目
		 * （状態を持てない＝6.13 ので、前の子の種族から組を探す・同じ 1 匹目の組が複数あれば乱数で） */
		if (index & 1)
		{
			u16 prev = GetMonData(&party[index - 1], MON_DATA_SPECIES, NULL);
			u32 n = 0, k;

			if (early)
			{
				n = CountAllowedPairs(pool, level, badges, evoInfo, prev);
				if (n == 0)
					goto CLEANUP;
				set = AllowedPairSetAt(pool, level, badges, evoInfo, prev, Random() % n, 1);
				earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
				goto REPLACE;
			}

			for (i = 0; i < pool->pairCount; ++i)
				if (gHackClassPoolSets[pool->pairs[i][0]].species == prev)
					++n;
			if (n != 0)
			{
				k = Random() % n;
				for (i = 0; i < pool->pairCount; ++i)
				{
					if (gHackClassPoolSets[pool->pairs[i][0]].species == prev && k-- == 0)
					{
						set = &gHackClassPoolSets[pool->pairs[i][1]];
						break;
					}
				}
			}
		}
		if (set == NULL)
		{
			if (early)
			{
				allowed = CountAllowedPairs(pool, level, badges, evoInfo, SPECIES_NONE);
				if (allowed == 0)
					goto CLEANUP;
				set = AllowedPairSetAt(pool, level, badges, evoInfo, SPECIES_NONE, Random() % allowed, 0);
				earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
				for (tries = 0; tries < 20 && SpeciesUsedBefore(party, index, earlySpecies); ++tries)
				{
					set = AllowedPairSetAt(pool, level, badges, evoInfo, SPECIES_NONE, Random() % allowed, 0);
					earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
				}
			}
			else
			{
				set = &gHackClassPoolSets[pool->pairs[Random() % pool->pairCount][0]];
				for (tries = 0; tries < 20 && SpeciesUsedBefore(party, index, set->species); ++tries)
					set = &gHackClassPoolSets[pool->pairs[Random() % pool->pairCount][0]];
			}
		}
	}
	else
	{
		if (pool->count == 0 || pool->sets == NULL)
			goto CLEANUP;
		if (early)
		{
			allowed = CountAllowedSets(pool->sets, pool->count, level, badges, evoInfo);
			if (allowed == 0)
				goto CLEANUP;
			set = AllowedSetAt(pool->sets, pool->count, level, badges, evoInfo, Random() % allowed);
			earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
			//同じ手持ちで重ならない種族を選ぶ（20 回で見つからなければ重なってよい）
			for (tries = 0; tries < 20 && SpeciesUsedBefore(party, index, earlySpecies); ++tries)
			{
				set = AllowedSetAt(pool->sets, pool->count, level, badges, evoInfo, Random() % allowed);
				earlySpecies = SetSpeciesAtLevel(set, level, badges, evoInfo);
			}
		}
		else
		{
			//Lv50 以上は従来と同じ候補・Random の呼び順
			set = &gHackClassPoolSets[pool->sets[Random() % pool->count]];
			for (tries = 0; tries < 20 && SpeciesUsedBefore(party, index, set->species); ++tries)
				set = &gHackClassPoolSets[pool->sets[Random() % pool->count]];
		}
	}

REPLACE:
	if (evoInfo != NULL)
	{
		Free(evoInfo);
		evoInfo = NULL;
	}
	otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
	personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
	otGender = GetMonData(mon, MON_DATA_OT_GENDER, NULL);
	ball = GetMonData(mon, MON_DATA_POKEBALL, NULL);
	friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
	fixedIv = GetMonData(mon, MON_DATA_HP_IV, NULL);   //トレーナーの個体値は 6 つ同じ値
	GetMonData(mon, MON_DATA_OT_NAME, otName);

	CreateMon(mon, early ? earlySpecies : set->species, level, fixedIv, TRUE, personality, OT_ID_PRESET, otId);
	SetMonData(mon, MON_DATA_OT_NAME, otName);
	SetMonData(mon, MON_DATA_OT_GENDER, &otGender);
	SetMonData(mon, MON_DATA_POKEBALL, &ball);
	SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
	if (early)
	{
		BalanceEarlyMoves(mon, level);
		return; //EV 0・持ち物なし・自然な性格/特性/テラを CreateMon のまま使う
	}

	if (set->noSet)
	{
		if (level <= 65)
			BalanceEarlyMoves(mon, level);
		return;   //型なし: CreateMon がレベルで覚える技を入れている・持ち物なし
	}

	for (i = 0; i < NUM_STATS; ++i)
	{
		u8 ev = set->evs[i];
		SetMonData(mon, MON_DATA_HP_EV + i, &ev);
	}
	GiveMonNatureAndAbility(mon, set->nature, set->ability, FALSE, TRUE, FALSE);
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u8 pp;

		move = set->moves[i];
		pp = (move != MOVE_NONE) ? gBattleMoves[move].pp : 0;
		SetMonData(mon, MON_DATA_MOVE1 + i, &move);
		SetMonData(mon, MON_DATA_PP1 + i, &pp);
	}
	SetMonData(mon, MON_DATA_HELD_ITEM, &set->item);
	if (set->tera != 0)
		mon->teratype = set->tera - 1;
	if (level <= 65)
		BalanceEarlyMoves(mon, level); //Lv50〜65 は種族・型を保ち、技だけ自然技＋従来の型から整える
	return;

CLEANUP:
	if (evoInfo != NULL)
		Free(evoInfo);
}
