#include "defines.h"
#include "defines_battle.h"
#include "hack_move_alias.h"
#include "hack_difficulty.h"
#include "hack_debug_menu.h"   //[hack] patches/0331 デバッグメニューの捕獲 100%（-DHACK_DEBUG_MENU のときだけ）
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/new/item.h"
#include "../include/random.h"
#include "../include/constants/items.h"   //[hack] 2026-10-11 ITEM_MOMO_SHERBET・liru さんの回復のきのみ（patches/0320）
#include "../include/item.h"
#include "../include/constants/hold_effects.h"   //[hack] 2026-10-19 しゃかりき: 回復の道具は キズぐすりだけ（patches/0616）
#include "../include/constants/item_effects.h"
#include "../include/new/rom_locs.h"
#include "../include/task.h"
#include "../include/new/util.h"
#include "../include/new/frontier.h"
#include "../include/new/learn_move.h"   //GiveBoxMonInitialMoveset（ももアイスの覚える技 +10・2026-10-11）
#include "../include/constants/trainer_classes.h"
#include "Tables/hoenn_script_flags.h"
bool8 HoennYartie_InBattle(void); //[hack] patches/0915 src/hoenn_yartie.c

/*
 * なんいど「やさしい」（2026-10-03 ユーザー「難易度さげたバージョン」→「ROM は 1 本・せっていで選ぶ・A＋B＋C」）。
 * 変数 HOENN_VAR_DIFFICULTY（0 ふつう / 1 やさしい・BW 風せっていの 6 行目「なんいど」・src/bw_option_menu.c）。
 *
 * やさしいのとき、**ふつうのトレーナー戦だけ**（バトル施設 FLAG_BATTLE_FACILITY の戦いは今までどおり）:
 *   A. 相手の AI をいちばん弱い段（AI_SCRIPT_CHECK_BAD_MOVE＝役に立たない技を避けるだけ）
 *      … src/Battle_AI/ai_master.c の GetAIFlags の最後（patches/0132）
 *   B. 相手のポケモンの努力値を 0・持ち物を外す（技はそのまま。メガストーン・Z クリスタル・オーブも外れる）
 *   C. 相手のポケモンのレベルを 3 下げる（手持ちに合わせて上がったあと・経験値で下げて能力を計算し直す）
 *      … B と C は src/build_pokemon.c の CreateNPCTrainerParty の、能力を計算する直前（patches/0132）
 *   ★ 一緒に戦う味方（タッグ）のポケモンは触らない
 *
 * ★ 2026-10-04 なんいど「ほどほど」（2・やさしい と ふつう の間・ユーザー「メガ進化系はもたせる　AI と努力値はふつう。レベルを 2 下げる」）:
 *   努力値は ふつう のまま（AI は 2026-10-05 から かしこい段を外す）。持ち物は メガストーン と ゲンシカイキの玉 だけ残して外す。レベルを 2 下げる（C と同じ下げ方）
 */

#define EASY_LEVEL_DOWN 3
#define MEDIUM_LEVEL_DOWN 2
/* ★ 2026-10-11 ユーザー「難易度ももアイス LV＋3 で覚える技＋10」（patches/0314）:
   ももアイス（4）のとき、トレーナー戦の相手（施設も）に レベル +3（上限 MAX_LEVEL）。技が「レベルで覚える技」のまま（型の技ではない）なら、
   上げたあとのレベル +10 で覚える技に入れ替える（型＝Smogon の技を持つ相手はそのまま）。施設にも効く（ユーザーの答え 2026-10-11） */
#define MOMO_LEVEL_UP 5                  //★ 2026-10-11 ユーザーの判断（5e 経由）: +5（liru さんの 3 回目と同じ・前は +3）。マージでは liru さんの +5 と 1 つにする
#define MOMO_MOVES_LEVEL_UP 10
/* ★ 2026-10-11 ユーザー「難易度りるアイス やさしいより やさしい 購入金額とか個体値の高いポケモンや対戦相手のやさしい 塩梅は決めて」（patches/0321）:
   りるアイス（5）= やさしい の中身（AI 最弱・努力値 0・持ち物なし・レベル −3＝やさしいと同じ・2026-10-16 ユーザー決定で −5 から）に加えて 野生は 2V 保証（6 つの能力からランダムに 2 つを 31・残りは生成のまま・CreateWildMon のあと）・
   お店の買い値は半額（1 円より下にはしない・売値はそのまま）。施設は今までどおり。AI の段階は config.h の表の 6 つ目（−24・0・0 ＝ いつも 0） */
/* ★ 2026-10-18 ユーザー「難易度しゃかりき追加できる？ももアイスをベースに難しくして」（枝 feature/difficulty-shakariki）:
   しゃかりき（6）= ももアイスの中身（施設も Lv が上がる・AI・野生のむれ・野生のボス・道具の数・ひかえめにしない入れ替え など全部）に、下の 6 つを足して強くしたもの。
   数字は全部ここ（と下の表）に名前で置いてある＝遊んで合わなければ ここだけ直す。
     SHAKARIKI_LEVEL_UP          トレーナー戦の相手（施設も）のレベルを上げる数（ももアイスは MOMO_LEVEL_UP = 5）
     SHAKARIKI_MOVES_LEVEL_UP    レベルで覚える技のままなら 上げたレベル + この数 で覚える技に入れ替える（ももアイスは 10）
     SHAKARIKI_HELD_ITEM         ふつうのトレーナーの相手が持つ道具（ももアイスのときは ももシャーベット）。メガストーン・ゲンシの玉・Z クリスタルを持つ子は変えない
     SHAKARIKI_EV                ふつうのトレーナーの相手の努力値（ベース種族値の高い 2 つの能力に・合計 504 ≦ 510）。型の努力値を持つ子（Smogon の型）は変えない
     sCatchOddsTenths[6]         捕獲率 ×0.3（ももアイスは ×0.5）
     sBuyPriceMul[6]             お店の買い値 ×5（ももアイスは ×3）*/
#define SHAKARIKI_LEVEL_UP 10
#define SHAKARIKI_MOVES_LEVEL_UP 20
#define SHAKARIKI_HELD_ITEM ITEM_MOMO_ICE
#define SHAKARIKI_EV HACK_EV_PER_STAT_CAP //[hack] patches/0677
#define RIRU_LEVEL_DOWN 3                //2026-10-16 ユーザー決定: −5 → −3（やさしい EASY_LEVEL_DOWN と同じ値）
#define HARD_LEVEL_UP 1                  //liru さん（patches/0320）: むずかしい のふつうのトレーナーの相手は レベル +1（ももアイスの +5 は MOMO_LEVEL_UP）

static bool8 OrdinaryTrainerBattle(void)
{
	return (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
	    && !FlagGet(FLAG_BATTLE_FACILITY)
	    && !(gBattleTypeFlags & BATTLE_TYPE_FRONTIER);
}

static bool8 EasyTrainerBattle(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_EASY && OrdinaryTrainerBattle();
}

static bool8 RiruIceTrainerBattle(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_RIRU_ICE && OrdinaryTrainerBattle();
}

static bool8 MediumTrainerBattle(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_MEDIUM && OrdinaryTrainerBattle();
}

//★ 施設（バトルフロンティアなど）も含む（ユーザーの答え 2026-10-11・dd → 5e 経由「ももアイスは施設にも効かせる」＝CLAUDE.md #33 のとおり）
static bool8 MomoIceTrainerBattleAny(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_MOMO_ICE && (gBattleTypeFlags & BATTLE_TYPE_TRAINER);
}

//しゃかりき（ももアイスと同じ形・施設も含む）
static bool8 ShakarikiTrainerBattleAny(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_SHAKARIKI && (gBattleTypeFlags & BATTLE_TYPE_TRAINER);
}

static bool8 ShakarikiTrainerBattle(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_SHAKARIKI && OrdinaryTrainerBattle();
}

//★ 2026-10-11 なんいど ももアイス のふつうのトレーナー戦: 相手のポケモンに ももシャーベット（ももアイスの下位版・src/hack_momo_ice.c）。
//  施設の相手は build_pokemon.c の CreateNPCTrainerParty で IsFrontierTrainerId → BuildFrontierParty へ先に抜けるので、ここには来ない（5e の指摘）
static bool8 MomoSherbetBattle(void)
{
	return VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_MOMO_ICE && OrdinaryTrainerBattle();
}

static void SetTrainerMonLevel(struct Pokemon* mon, u32 level)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);

	SetMonData(mon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][level]);
	SetMonData(mon, MON_DATA_LEVEL, &level);
}

static void LowerTrainerMonLevel(struct Pokemon* mon, u32 down)
{
	u32 level = GetMonData(mon, MON_DATA_LEVEL, NULL);

	//★ 2026-10-11 5e の指摘: 「level > down + 1」だと Lv5・6 の相手が −5 で下がらず やさしい（−3）より強かった（今は りるアイスも −3 だが下限 2 の処理はそのまま） → 下限 2 まで下げる
	if (level > 2)
		SetTrainerMonLevel(mon, MathMax((s32) level - (s32) down, 2));
}

static void RaiseTrainerMonLevel(struct Pokemon* mon, u32 up)
{
	u32 level = GetMonData(mon, MON_DATA_LEVEL, NULL) + up;

	SetTrainerMonLevel(mon, MathMin(level, MAX_LEVEL));
}

//技が「今のレベルで覚える技」のままか（CreateMon が入れたもの＝型の技ではない）。同じ手順で作って 4 つとも同じなら
//  ★ CFRU の struct Pokemon は平ら（暗号化なし）なので moves[] を直に読める。写しは約 100 バイト（6.35 の 1,024 B より下）
static bool8 HasNaturalMoveset(struct Pokemon* mon)
{
	struct Pokemon tmp = *mon;
	u32 i;

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		tmp.moves[i] = MOVE_NONE;
		tmp.pp[i] = 0;
	}
	GiveBoxMonInitialMoveset((struct BoxPokemon*) &tmp);
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		if (tmp.moves[i] != mon->moves[i])
			return FALSE;
	}
	return TRUE;
}

//レベル level で覚える技（最後の 4 つ）に入れ替える。レベル・経験値は変えない（GiveBoxMonInitialMoveset は経験値からレベルを見るので一時的に差し替える）
static void GiveNaturalMovesetAtLevel(struct Pokemon* mon, u32 level)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u32 exp = GetMonData(mon, MON_DATA_EXP, NULL);
	u32 i;

	level = MathMin(level, MAX_LEVEL);
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		mon->moves[i] = MOVE_NONE;
		mon->pp[i] = 0;
	}
	mon->ppBonuses = 0;
	SetMonData(mon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][level]);
	GiveBoxMonInitialMoveset((struct BoxPokemon*) mon);
	SetMonData(mon, MON_DATA_EXP, &exp);
}

u8 HackAI_GetRank(const struct AIRankContext* context)
{
    static const s8 offsets[DIFFICULTY_COUNT] = AI_RANK_DIFFICULTY_OFFSETS;
    static const u8 floors[DIFFICULTY_COUNT] = AI_RANK_DIFFICULTY_FLOORS;
    static const u8 caps[DIFFICULTY_COUNT] = AI_RANK_DIFFICULTY_CAPS;
    static const s8 wildOffsets[DIFFICULTY_COUNT] = AI_RANK_WILD_DIFFICULTY_OFFSETS;
    static const u8 wildFloors[DIFFICULTY_COUNT] = AI_RANK_WILD_DIFFICULTY_FLOORS;
    static const u8 wildCaps[DIFFICULTY_COUNT] = AI_RANK_WILD_DIFFICULTY_CAPS;
    u16 difficulty = context->difficulty < DIFFICULTY_COUNT ? context->difficulty : DIFFICULTY_NORMAL;
    s16 rank = MathMin(context->level, AI_RANK_LEVEL_CAP) / (context->wild ? AI_RANK_WILD_LEVEL_STEP : AI_RANK_LEVEL_STEP)
             + (context->wild ? wildOffsets[difficulty] : offsets[difficulty]);

    u16 points = (context->importantTrainer ? AI_RANK_IMPORTANT_TRAINER_POINTS : 0)
               + (context->lowHp ? AI_RANK_LOW_HP_POINTS : 0)
               + (context->statused ? AI_RANK_STATUS_POINTS : 0)
               + (context->prolongedBattle ? AI_RANK_LONG_BATTLE_POINTS : 0)
               + (context->boostedFoe ? AI_RANK_BOOSTED_FOE_POINTS : 0);
    rank += MathMin(points / AI_RANK_POINTS_PER_STEP, AI_RANK_MAX_CONTEXT_BOOST);
    u8 floor = context->wild ? wildFloors[difficulty] : floors[difficulty];
    u8 cap = context->wild ? wildCaps[difficulty] : caps[difficulty];
    //★ 2026-10-11 MathMin／MathMax は u32（include/new/util.h）。rank が負（やさしい Lv39 以下・ほどほど Lv19 以下）だと
    //  巨大な値になり cap（いちばん賢い段）が返っていた → s16 のまま挟む（patches/0311）
    if (rank < floor)
        rank = floor;
    if (rank > cap)
        rank = cap;
    return rank;
}

static u32 RankAIFlags(u8 rank)
{
    u32 flags = AI_SCRIPT_CHECK_BAD_MOVE;
    if (rank >= AI_RANK_SEMI_SMART) flags |= AI_SCRIPT_SEMI_SMART;
    if (rank >= AI_RANK_STANDARD) flags |= AI_SCRIPT_CHECK_GOOD_MOVE;
    if (rank >= AI_RANK_ADVANCED) flags |= AI_SCRIPT_ADVANCED;
    if (rank >= AI_RANK_EXPERT) flags |= AI_SCRIPT_EXPERT;
    return flags;
}

static u8 CommonBattlerRank(u8 bank, bool8 wild)
{
    struct AIRankContext context = {0};
    context.difficulty = VarGet(HOENN_VAR_DIFFICULTY);
    context.wild = wild;
    struct BattlePokemon* mon = &gBattleMons[bank];
    context.level = BM_LEVEL(*mon);
    context.lowHp = mon->hp != 0 && (u32)mon->hp * 100 <= (u32)mon->maxHP * AI_RANK_LOW_HP_PERCENT;
    context.statused = (mon->status1 & STATUS1_ANY) != 0;
    context.prolongedBattle = gBattleResults.battleTurnCounter >= AI_RANK_ADAPT_TURN;
    if (!wild && OrdinaryTrainerBattle() && !IsFrontierTrainerId(gTrainerBattleOpponent_A))
        switch (gTrainers[gTrainerBattleOpponent_A].trainerClass)
        {
        case CLASS_LEADER: case CLASS_ELITE_FOUR: case CLASS_CHAMPION:
        case CLASS_LEADER_RS: case CLASS_ELITE_FOUR_RS: case CLASS_CHAMPION_RS:
        case CLASS_AQUA_LEADER: case CLASS_MAGMA_LEADER:
            context.importantTrainer = TRUE;
            break;
        }
    for (u8 foe = 0; foe < gBattlersCount; ++foe)
        if (SIDE(foe) != SIDE(bank) && gBattleMons[foe].hp != 0
        && ((wild && BM_LEVEL(gBattleMons[foe]) >= BM_LEVEL(*mon) + AI_RANK_WILD_THREAT_LEVEL_GAP)
         || gBattleMons[foe].statStages[STAT_ATK] >= 6 + AI_RANK_FOE_STAT_BOOST
         || gBattleMons[foe].statStages[STAT_SPATK] >= 6 + AI_RANK_FOE_STAT_BOOST
         || gBattleMons[foe].statStages[STAT_SPEED] >= 6 + AI_RANK_FOE_STAT_BOOST))
            context.boostedFoe = TRUE;
    return HackAI_GetRank(&context);
}

u8 HackAI_GetTrainerRank(u8 bank)
{
    return CommonBattlerRank(bank, FALSE);
}

u8 HackAI_GetWildRank(u8 bank)
{
    return CommonBattlerRank(bank, TRUE);
}

u8 HackAI_GetBattlerRank(u8 bank)
{
    return (gBattleTypeFlags & BATTLE_TYPE_TRAINER) ? HackAI_GetTrainerRank(bank) : HackAI_GetWildRank(bank);
}

// Easy/medium/normal scale ordinary trainers. Hard/Momo also upgrade facilities.
u32 HackAI_TrainerFlags(u32 flags, u8 bank)
{
    u16 difficulty = VarGet(HOENN_VAR_DIFFICULTY);
    if (difficulty != DIFFICULTY_HARD && !HackDifficulty_IsMomoClass(difficulty)
    && (!OrdinaryTrainerBattle() || IsFrontierTrainerId(gTrainerBattleOpponent_A)))
        return flags;
    u32 rankMask = RankAIFlags(AI_RANK_EXPERT);
    return (flags & ~rankMask) | RankAIFlags(HackAI_GetTrainerRank(bank));
}

// Wild temperament/survival has its own script, never the trainer's prediction layer.
u32 HackAI_WildFlags(u32 flags, u8 bank)
{
    if (flags == 0)
        return flags;
    u32 rankMask = RankAIFlags(AI_RANK_EXPERT) | AI_SCRIPT_WILD_INSTINCT;
    u32 rankFlags = RankAIFlags(HackAI_GetWildRank(bank)) & ~(AI_SCRIPT_ADVANCED | AI_SCRIPT_EXPERT);
    return (flags & ~rankMask) | rankFlags | AI_SCRIPT_WILD_INSTINCT;
}

u32 HackDifficulty_AIFlags(u32 flags)
{
    if (gBattleTypeFlags & (BATTLE_TYPE_SAFARI | BATTLE_TYPE_ROAMER | BATTLE_TYPE_OAK_TUTORIAL))
        return flags;
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        return HackAI_TrainerFlags(flags, gActiveBattler);
    if ((gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) && SIDE(gActiveBattler) == B_SIDE_PLAYER)
        return flags;
    return HackAI_WildFlags(flags, gActiveBattler);
}

/* ★ 2026-10-16 ユーザー決定（patches/0435・0436）: むずかしい以下の相手の AI のゆるめ。
   0435: ふつうのトレーナー（モブ）は AI が自分から手持ちを入れ替えない（りるアイス・やさしい・ほどほど・ふつう）。
         ジムリーダー・四天王・チャンピオン・ライバル・わたしたち・モモカ博士・物語の大事な相手と、施設・通信は今までどおり。
         ひんしのあとの次の子・ほえる・とんぼがえり・だっしゅつボタンなど技や道具の入れ替えは触らない（止めるのは ai_master.c の ShouldSwitch の入口だけ）。
   0436: 相手の AI は 1 匹ごと、まもる系は 1 回の戦闘で 3 回まで・能力アップ系は 2 回まで・同じ攻撃技は続けて 2 回まで
         （りるアイス〜むずかしい・ももアイス・施設・野生・通信・味方のタッグは対象外）。FindBestLegalMoves が候補から外す（ほかに使える技が無ければ今どおり）。
   数えるのは相手が選んだとき（OpponentHandleChooseMove・gNewBS->hackAiLog・相手側の手持ちの位置ごと・6.13: static に置かない）。 */
#define HACK_AI_PROTECT_MAX 3
#define HACK_AI_BOOST_MAX 2
#define HACK_AI_REPEAT_MAX 2

//物語の大事な相手か（クラスで決める＝全地方。番号がかぶる所は同じ番号が同じ意味なのを確かめてある・docs/research/2026-10-16_mob-no-switch.md）
bool8 HackDifficulty_IsStoryTrainerId(u16 id)
{
	if (id == 936 || id == 937)   //きんこんしきの夫婦（クラス 28 はシンオウの町の人と同じなので番号で・ユーザー 2026-10-16）
		return TRUE;
	switch (gTrainers[id].trainerClass)
	{
	case CLASS_AQUA_LEADER: case CLASS_MAGMA_LEADER: case CLASS_AQUA_ADMIN: case CLASS_MAGMA_ADMIN:   //アオギリ・マツブサ・イズミ・ウシオ
	case CLASS_ELITE_FOUR_RS: case CLASS_LEADER_RS: case CLASS_CHAMPION_RS:                           //ホウエンのジムリーダー・四天王・チャンピオン
	case CLASS_PKMN_TRAINER_3:   //44: ハルカ・ヒカリ・ヒガン・わたしたち 10 人（物語や特別な相手）
	case CLASS_RIVAL: case CLASS_RIVAL_2: case CLASS_BOSS: case CLASS_LEADER: case CLASS_ELITE_4: case CLASS_CHAMPION:   //カントー・ジョウト・シンオウのジム・四天王・チャンピオン・ライバル・サカキ
	case CLASS_PKMN_PROF:        //モモカ博士
	case CLASS_GALACTIC_ADMIN: case CLASS_GALACTIC_BOSS:   //ギンガ団のかんぶ・ボス
		return TRUE;
	}
	return FALSE;
}

//りるアイス・やさしい・ほどほど・ふつう・むずかしい のふつうのトレーナー戦で、相手側のバトラーか（ゆるめの対象）
static bool8 HackAI_EasierRulesApply(u8 bank, bool8 includeHard)
{
	u16 difficulty = VarGet(HOENN_VAR_DIFFICULTY);
	bool8 wild = includeHard && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER);   //0436 は野生（伝説・レイドも）にも効く（ユーザー 2026-10-16）

	if (SIDE(bank) != B_SIDE_OPPONENT)
		return FALSE;
	if (wild)
	{
		if (FlagGet(FLAG_BATTLE_FACILITY) || (gBattleTypeFlags & BATTLE_TYPE_FRONTIER))
			return FALSE;
	}
	else if (!OrdinaryTrainerBattle())
		return FALSE;
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_OAK_TUTORIAL | BATTLE_TYPE_SAFARI | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_MOCK_BATTLE))
		return FALSE;
	if (!wild && (gTrainerBattleOpponent_A == SECRET_BASE_OPPONENT || IsFrontierTrainerId(gTrainerBattleOpponent_A)))
		return FALSE;
	if (HackDifficulty_IsMomoClass(difficulty) || difficulty >= DIFFICULTY_COUNT)
		return FALSE;
	return includeHard || difficulty != DIFFICULTY_HARD;
}

//0435: AI が自分から入れ替えるのを止めるか（ShouldSwitch の入口から）。むずかしい・ももアイスは今までどおり
bool8 HackAI_MobNoSwitch(u8 bank)
{
	if (!HackAI_EasierRulesApply(bank, FALSE))
		return FALSE;
	if (HackDifficulty_IsStoryTrainerId(gTrainerBattleOpponent_A))
		return FALSE;
	if ((gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) && HackDifficulty_IsStoryTrainerId(VarGet(VAR_SECOND_OPPONENT)))
		return FALSE;
	return TRUE;
}

//まもる系（効果が EFFECT_PROTECT の 14 技: まもる・みきり・こらえる・キングシールド・ニードルガード・トーチカ・トリックガード・たたみがえし・ファストガード・ワイドガード・ブロッキング・ダイウォール・スレッドトラップ・かえんのまもり）
static bool8 HackAI_IsProtectMove(u16 move)
{
	return move != MOVE_NONE && gBattleMoves[move].effect == EFFECT_PROTECT;
}

//自分の能力ランクを上げる変化技（自分だけが対象・攻撃技の追加効果・てだすけなど味方向けは含めない）
static bool8 HackAI_IsBoostMove(u8 bank, u16 move)
{
	if (move == MOVE_NONE || gBattleMoves[move].split != SPLIT_STATUS)
		return FALSE;
	switch (gBattleMoves[move].effect)
	{
	case EFFECT_CURSE:   //のろい: ゴースト以外が使うと能力アップ（ゴーストは自分の HP を削る呪いで能力アップではない）
		return gBattleMons[bank].type1 != TYPE_GHOST && gBattleMons[bank].type2 != TYPE_GHOST;
	case EFFECT_ATK_ACC_UP: case EFFECT_ATK_SPATK_UP: case EFFECT_ATTACK_UP: case EFFECT_ATTACK_UP_2: case EFFECT_BELLY_DRUM:
	case EFFECT_BULK_UP: case EFFECT_CALM_MIND: case EFFECT_CHARGE: case EFFECT_COSMIC_POWER: case EFFECT_DEFENSE_CURL:
	case EFFECT_DEFENSE_UP: case EFFECT_DEFENSE_UP_2: case EFFECT_DRAGON_DANCE: case EFFECT_EVASION_UP: case EFFECT_EXTREME_EVOBOOST:
	case EFFECT_MINIMIZE: case EFFECT_SPECIAL_ATTACK_UP_2: case EFFECT_SPECIAL_DEFENSE_UP_2: case EFFECT_SPEED_UP_2: case EFFECT_STOCKPILE:
		//自分が対象の技だけ（ロトティラ・ギアアップ・フラワーガード・じばりょく は味方も含む対象なので除く・アロマミストは効果が別）
		return (gBattleMoves[move].target & MOVE_TARGET_USER) != 0 || MoveIs(move, MOVE_ACUPRESSURE);
	}
	return FALSE;
}

static bool8 HackAI_IsAttackMove(u16 move)
{
	return move != MOVE_NONE && gBattleMoves[move].split != SPLIT_STATUS;
}

//ダイマックス中・Z 技・こだわりで縛られているときは、続けて選ぶ数を数えない・外さない（技が限られる）
static bool8 HackAI_RepeatExempt(u8 bank)
{
	return IsDynamaxed(bank) || gNewBS->dynamaxData.toBeUsed[bank] || gNewBS->zMoveData.toBeUsed[bank] || CHOICED_MOVE(bank) != MOVE_NONE;
}

//0436: この技を AI の候補から外すか（FindBestLegalMoves から）。ほかに候補が無いときは呼び元が外さずに選び直す
bool8 HackAI_MoveBanned(u8 bank, u16 move)
{
	struct HackAiMoveLog* log;

	if (move == MOVE_NONE || !HackAI_EasierRulesApply(bank, TRUE) || gBattlerPartyIndexes[bank] >= PARTY_SIZE)
		return FALSE;
	log = &gNewBS->hackAiLog[gBattlerPartyIndexes[bank]];
	if (HackAI_IsProtectMove(move) && log->protectUsed >= HACK_AI_PROTECT_MAX)
		return TRUE;
	if (HackAI_IsBoostMove(bank, move) && log->boostUsed >= HACK_AI_BOOST_MAX)
		return TRUE;
	if (HackAI_IsAttackMove(move) && !HackAI_RepeatExempt(bank) && log->repeatMove == move && log->repeatCount >= HACK_AI_REPEAT_MAX)
		return TRUE;
	return FALSE;
}

//0436: 相手が技を選んだ（OpponentHandleChooseMove から）。ダブルで味方の選び直しが同じターンにもう一度来たら、前に数えた分を戻して数え直す
void HackAI_NoteMoveChosen(u8 bank, u16 move)
{
	struct HackAiMoveLog* log;
	u8 turn = gBattleResults.battleTurnCounter + 1;

	if (!HackAI_EasierRulesApply(bank, TRUE) || gBattlerPartyIndexes[bank] >= PARTY_SIZE)
		return;
	log = &gNewBS->hackAiLog[gBattlerPartyIndexes[bank]];
	if (log->notedTurn == turn)
	{
		if (log->lastKind == 1 && log->protectUsed != 0)
			--log->protectUsed;
		else if (log->lastKind == 2 && log->boostUsed != 0)
			--log->boostUsed;
		log->repeatMove = log->prevRepeatMove;
		log->repeatCount = log->prevRepeatCount;
	}
	log->prevRepeatMove = log->repeatMove;
	log->prevRepeatCount = log->repeatCount;
	log->notedTurn = turn;
	log->lastNoted = move;
	log->lastKind = 0;

	if (HackAI_IsProtectMove(move))
	{
		log->lastKind = 1;
		if (log->protectUsed < 0xFF)
			++log->protectUsed;
	}
	else if (HackAI_IsBoostMove(bank, move))
	{
		log->lastKind = 2;
		if (log->boostUsed < 0xFF)
			++log->boostUsed;
	}

	if (HackAI_IsAttackMove(move) && !HackAI_RepeatExempt(bank))
	{
		if (log->repeatMove == move)
		{
			if (log->repeatCount < 0xFF)
				++log->repeatCount;
		}
		else
		{
			log->repeatMove = move;
			log->repeatCount = 1;
		}
	}
	else
	{
		log->repeatMove = MOVE_NONE;   //別の技（変化技・縛り）を選んだら数え直す
		log->repeatCount = 0;
	}
}

//ももアイス: レベル +3・レベルで覚える技のままなら上げたレベル +10 の技（ふつうのトレーナーも施設も同じ）
static void MomoIceAdjust(struct Pokemon* mon)
{
	//★ 比べるのはレベルを上げる前（上げたあとは「覚える技」が変わる）
	bool8 natural = HasNaturalMoveset(mon);
	bool8 shakariki = VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_SHAKARIKI;   //しゃかりきは上げる数・技のレベルの足し分だけ違う
	u32 up = shakariki ? SHAKARIKI_LEVEL_UP : MOMO_LEVEL_UP;
	u32 movesUp = shakariki ? SHAKARIKI_MOVES_LEVEL_UP : MOMO_MOVES_LEVEL_UP;

	RaiseTrainerMonLevel(mon, up);
	if (natural)
		GiveNaturalMovesetAtLevel(mon, GetMonData(mon, MON_DATA_LEVEL, NULL) + movesUp);
}

//しゃかりき: ふつうのトレーナーの相手の努力値を、ベース種族値の高い 2 つの能力に SHAKARIKI_EV ずつ（CFRU の GiveMon2BestBaseStatEVs は VAR_GAME_DIFFICULTY が無いと入らないので自前）。
//  もともと努力値を持つ子（Smogon の型・CFRU の Trainers with EVs）は触らない。ベースの同点は先に出た能力（HP→こうげき→ぼうぎょ→すばやさ→とくこう→とくぼう）
static void ShakarikiGiveEVs(struct Pokemon* mon)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u8 best1 = STAT_HP, best2 = STAT_ATK;
	u8 statId;
	u16 ev = SHAKARIKI_EV;

	if (GetMonEVCount(mon) != 0)
		return;
	if (GetVisualBaseStat(STAT_HP, species) < GetVisualBaseStat(STAT_ATK, species))
	{
		best1 = STAT_ATK;
		best2 = STAT_HP;
	}
	for (statId = STAT_DEF; statId <= STAT_SPDEF; ++statId)
	{
		u16 base = GetVisualBaseStat(statId, species);

		if (base > GetVisualBaseStat(best1, species))
		{
			best2 = best1;
			best1 = statId;
		}
		else if (base > GetVisualBaseStat(best2, species))
			best2 = statId;
	}
	SetMonData(mon, MON_DATA_HP_EV + best1, &ev);   //MON_DATA_*_EV の並び（HP・こうげき・ぼうぎょ・すばやさ・とくこう・とくぼう）は STAT_* の並びと同じ
	SetMonData(mon, MON_DATA_HP_EV + best2, &ev);
}

//施設（バトルフロンティア・トレーナーヒル）の相手: src/build_pokemon.c の CreateFrontierMon（能力を計算する直前・BuildFrontierParty は CreateNPCTrainerParty より先に抜けるので
//  HackDifficulty_AdjustTrainerMon は通らない＝5e の指摘 2026-10-11）。レンタル（forPlayer）は触らない。施設の型の技は変えない（HasNaturalMoveset が偽）
void HackDifficulty_AdjustFrontierMon(struct Pokemon* mon, bool8 forPlayer, bool8 isBrain)
{
	//★ 5e の指摘 2026-10-11: マルチの相方（0x396）とレイドの助っ人（0x395）は forPlayer = FALSE で gPlayerParty に作られる → 味方側の入れ物なら触らない
	u16 difficulty = VarGet(HOENN_VAR_DIFFICULTY);

	if (forPlayer || (mon >= gPlayerParty && mon < gPlayerParty + PARTY_SIZE))
		return;
	if (HackDifficulty_IsMomoClass(difficulty))
	{
		MomoIceAdjust(mon);
		return;
	}
	//★ 2026-10-21 ユーザー「バトルフロンティア 戦闘時に難易度設定のレベル調整が反映されるように」（BUG-frontier-difficulty-level）:
	//  りるアイス・やさしい −3・ほどほど −2・むずかしい +1 も施設の相手に掛ける（レベルだけ。持ち物・努力値・AI は触らない＝施設の型を壊さない。
	//  技は型の技のまま＝レベルを下げても入れ替えない）。下げる側の下限は 2・上げる側の上限は MAX_LEVEL（ふつうのトレーナー戦と同じ関数）。
	//  ファクトリー・ドームの相手も BuildFrontierParty（forPlayer = FALSE）を通るのでここに来る。自分のレンタル・相方・助っ人は上で除外。
	//[hack] patches/0924 ユーザー 2026-10-27「ブレーンは難易度でレベルを下げない」: フロンティアブレーン（isBrain）も −3／−2 を掛けない（2 つ目の特性は Lv50 から効くため。上げる側 むずかしい +1・ももアイス +5 は今のまま）
	//[hack] patches/0915 ユーザー「難易度によるレベル低下なくしてほしい ヤーティだけ」: ロジックアリーナのロンジャは −3／−2 を掛けない（上げる側 むずかしい +1・ももアイス +5 は今のまま）
	switch (difficulty)
	{
	case DIFFICULTY_EASY:
		if (!HoennYartie_InBattle() && !isBrain)
			LowerTrainerMonLevel(mon, EASY_LEVEL_DOWN);
		break;
	case DIFFICULTY_RIRU_ICE:
		if (!HoennYartie_InBattle() && !isBrain)
			LowerTrainerMonLevel(mon, RIRU_LEVEL_DOWN);
		break;
	case DIFFICULTY_MEDIUM:
		if (!HoennYartie_InBattle() && !isBrain)
			LowerTrainerMonLevel(mon, MEDIUM_LEVEL_DOWN);
		break;
	case DIFFICULTY_HARD:
		RaiseTrainerMonLevel(mon, HARD_LEVEL_UP);
		break;
	}
}

//B・C. src/build_pokemon.c の CreateNPCTrainerParty（相手側のポケモン・能力を計算する直前）
static void AdjustTrainerMonCore(struct Pokemon* mon, u8 side)
{
	u16 none = ITEM_NONE;
	u8 zero = 0;
	u32 i;

	if (side != B_SIDE_OPPONENT)
		return;

	if (MomoIceTrainerBattleAny() || ShakarikiTrainerBattleAny())
	{
		MomoIceAdjust(mon);   //ももアイス: レベル +5（MOMO_LEVEL_UP＝liru さんの +5 と同じ・二重に上げない）・技 +10・施設も（しゃかりきは +10・技 +20・MomoIceAdjust の中で切り替え）
		//★ return しない（5e の指摘 2026-10-11）: f8 のももシャーベット（持ち物）のブロックが同じ所に入るので、先の return で飛ばさない。
		//  下の ほどほど／やさしい は なんいど の値が違うので通らない
	}
	if (MomoSherbetBattle())
	{
		u16 item = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
		u16 sherbet = ITEM_MOMO_SHERBET;

		if (!IsMegaStone(item) && !IsPrimalOrb(item) && !IsZCrystal(item))
			SetMonData(mon, MON_DATA_HELD_ITEM, &sherbet);   //能力の計算の前なので 1.5 倍が効く
	}
	if (ShakarikiTrainerBattle())
	{
		u16 item = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
		u16 held = SHAKARIKI_HELD_ITEM;

		if (!IsMegaStone(item) && !IsPrimalOrb(item) && !IsZCrystal(item))
			SetMonData(mon, MON_DATA_HELD_ITEM, &held);   //しゃかりき: ももアイス（HP・5 つの能力 2 倍・状態異常と能力ダウンを受けない・毎ターン 1/3 回復）
		ShakarikiGiveEVs(mon);                            //能力の計算の前なので努力値が効く
	}
	if (OrdinaryTrainerBattle() && VarGet(HOENN_VAR_DIFFICULTY) == DIFFICULTY_HARD)   //liru さん（patches/0320）: むずかしい +1 レベル（ももアイスの +5 は上の MomoIceAdjust）
	{
		RaiseTrainerMonLevel(mon, HARD_LEVEL_UP);
		return;
	}

	if (MediumTrainerBattle())
	{
		u16 item = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);

		if (!IsMegaStone(item) && !IsPrimalOrb(item))
			SetMonData(mon, MON_DATA_HELD_ITEM, &none);
		LowerTrainerMonLevel(mon, MEDIUM_LEVEL_DOWN);
		return;
	}
	if (!EasyTrainerBattle() && !RiruIceTrainerBattle())
		return;

	for (i = 0; i < NUM_STATS; ++i)
		SetMonData(mon, MON_DATA_HP_EV + i, &zero);
	SetMonData(mon, MON_DATA_HELD_ITEM, &none);

	LowerTrainerMonLevel(mon, RiruIceTrainerBattle() ? RIRU_LEVEL_DOWN : EASY_LEVEL_DOWN);
}

/* ★ 2026-10-11 ユーザー「難易度によって捕獲率を上げる 難易度が下がるほど補正がかかるように」（patches/0326）:
   src/catching.c の捕獲の式（ボール・状態異常・レイドの倍率のあと・0xFF で頭打ちの手前）で odds に掛ける。10 分の 1 の単位。
   りるアイス 3.0・やさしい 2.0・ほどほど 1.5・ふつう 1.0・むずかしい 0.7・ももアイス 0.5（下げるのは 2026-10-11 ユーザー「難易度むずかしいとももアイス捕獲率下げるようにしてほしい」）。サファリにも効く。
   マスターボールは式のあとで必ず捕まえる（catching.c の BALL_TYPE_MASTER_BALL の分岐）ので下げても捕まる。伝説レイド（0181）はこのあと 0xFF にするので関係ない。
   odds は GetBaseBallCatchOdds の値（255 × ボールの倍率 ÷ 10 ほど・数千まで）なので 3 倍でも u32 はあふれない。施設では捕まえられないので影響なし */
static const u8 sCatchOddsTenths[DIFFICULTY_COUNT] = {10, 20, 15, 7, 5, 30, 3};   //並びは enum HackDifficulty（0 ふつう・1 やさしい・2 ほどほど・3 むずかしい・4 ももアイス・5 りるアイス・6 しゃかりき ×0.3）

u32 HackDifficulty_CatchOdds(u32 odds)
{
	u16 difficulty = VarGet(HOENN_VAR_DIFFICULTY);

#ifdef HACK_DEBUG_MENU
	if (HackDebugMenu_CatchAll())   //[hack] patches/0331 デバッグメニューの「ほかく100」（印 HACK_FLAG_DEBUG_CATCH・ふつうの版では見ない）
		return 0xFF;
#endif

	u32 before = odds;

	if (difficulty >= DIFFICULTY_COUNT)
		difficulty = DIFFICULTY_NORMAL;
	odds = odds * sCatchOddsTenths[difficulty] / 10;
	if (odds == 0 && before != 0)
		odds = 1;   //5e の指摘: 元が 1〜2 のとき 0 になると揺れの判定（catching.c は 0xFF0000 / odds）が壊れる＝下げても最低 1
	return odds > 0xFF ? 0xFF : odds;
}

//お店の買い値（src/mega_shop.c の MegaShop_ItemIdGetPrice（買う画面のとき）と MegaShop_Price から・引数の形は f8 の liru さん 3 回目と同じ）
//  りるアイス: 半額（0 になったら 1）。ほかの なんいど はそのまま（liru さんの むずかしい／ももアイス の割引はユーザー判断でやめた・2026-10-11 5e 経由＝せっていで切り替えると 1/5 で買って 1/2 で売れるため）
//★ 2026-10-12 ユーザー「なんいど むずかしいでショップ販売価格 ×1.5・ももアイス ×3」: 買い値の倍率を なんいど の表で（分子／分母・端数は切り上げ・6.79 で長さを書く）。
//  りるアイスの半額は切り捨て（0 なら 1）のまま。売り値（バッグ・売る画面）とお母さんの貯金の買い物は買う画面を通らないので変わらない。
//  上限は お金の上限 9,999,999（ももアイス × あぶないはっぱ の積み上げなど）。u16 の道（ふつうの店）は呼び元が 65535 で止める
#define HACK_BUY_PRICE_MAX 9999999
static const u8 sBuyPriceMul[DIFFICULTY_COUNT][2] =
{
	[DIFFICULTY_NORMAL] = {1, 1},
	[DIFFICULTY_EASY] = {1, 1},
	[DIFFICULTY_MEDIUM] = {1, 1},
	[DIFFICULTY_HARD] = {3, 2},
	[DIFFICULTY_MOMO_ICE] = {3, 1},
	[DIFFICULTY_RIRU_ICE] = {1, 2},
	[DIFFICULTY_SHAKARIKI] = {5, 1},   //2026-10-18 しゃかりき ×5（ももアイスは ×3）
};

u32 HackDifficulty_BuyPrice(u32 price, u16 difficulty)
{
	u32 num, den;

	if (price == 0 || difficulty >= DIFFICULTY_COUNT)
		return price;
	if (difficulty == DIFFICULTY_RIRU_ICE)
		return price >= 2 ? price / 2 : 1;
	num = sBuyPriceMul[difficulty][0];
	den = sBuyPriceMul[difficulty][1];
	if (price > HACK_BUY_PRICE_MAX / num)
		return HACK_BUY_PRICE_MAX;
	price = (price * num + den - 1) / den;
	return price > HACK_BUY_PRICE_MAX ? HACK_BUY_PRICE_MAX : price;
}

//りるアイス: 野生（ずかんナビ・レイドも CreateWildMon を通る）は 2V 保証（6 つの能力からランダムに 2 つを 31・重ならない・残りは生成のまま＝高個体値パッチの値を残す・2026-10-16・patches/0432）。性格値は触らない（6.5）
void HackDifficulty_WildMon(struct Pokemon* mon)
{
	u8 iv = HACK_IV_MAX; //[hack] patches/0677
	u32 a, b;

	if (VarGet(HOENN_VAR_DIFFICULTY) != DIFFICULTY_RIRU_ICE)
		return;
	a = Random() % NUM_STATS;
	b = Random() % (NUM_STATS - 1);   //0〜4 から選び、a 以上なら 1 つ飛ばす＝a と重ならない（すでに 31 の能力でも 2 つ目は別の能力）
	if (b >= a)
		++b;
	SetMonData(mon, MON_DATA_HP_IV + a, &iv);
	SetMonData(mon, MON_DATA_HP_IV + b, &iv);
	CalculateMonStats(mon);   //個体値を変えたので能力を計算し直す（5e の指摘 2026-10-11）
}
bool8 HackDifficulty_BagLimitReached(u16 difficulty, u8 usedCount)
{
	if (difficulty == DIFFICULTY_HARD)
		return usedCount >= 3;
	if (HackDifficulty_IsMomoClass(difficulty))
		return usedCount >= 1;
	return FALSE;
}

bool8 HackDifficulty_ForceSetStyle(u16 difficulty)
{
	return difficulty == DIFFICULTY_HARD || HackDifficulty_IsMomoClass(difficulty);
}

u16 HackDifficulty_RecoveryItem(u16 currentItem, u16 level, u8 roll, u16 difficulty)
{
	//★ なんいどの並び（0 ふつう・1 やさしい・2 ほどほど・3 むずかしい・4 ももアイス）。なんいどを足したら必ずここにも 1 つ足す（6.79・5e）
	static const u8 chances[DIFFICULTY_COUNT] = {30, 10, 20, 50, 70, 0, 70};   //6 つ目 りるアイス（配らない）・7 つ目 しゃかりき（ももアイスと同じ）
	if (currentItem != ITEM_NONE)
		return currentItem;
	if (difficulty >= DIFFICULTY_COUNT)
		difficulty = DIFFICULTY_NORMAL;
	//★ やさしい は「持ち物なし」・ほどほど は「メガストーンとゲンシの玉だけ」（CLAUDE.md #33・ユーザー判断）なので、外した持ち物が
	//  きのみとして戻らないように配らない（liru さんの表の 10・20% は使わない・5e の指摘 2026-10-11）
	if (difficulty != DIFFICULTY_HARD && !HackDifficulty_IsMomoClass(difficulty))
		return ITEM_NONE;   //ふつう も今までどおり（持ち物を足さない・5e）
	if (roll >= chances[difficulty])
		return ITEM_NONE;
	return level <= 20 ? ITEM_ORAN_BERRY : ITEM_SITRUS_BERRY;
}

void HackDifficulty_AdjustTrainerMon(struct Pokemon* mon, u8 side)
{
	AdjustTrainerMonCore(mon, side);
	if (side == B_SIDE_OPPONENT && OrdinaryTrainerBattle())
	{
		u16 item = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
		if (item == ITEM_NONE)
		{
			u16 level = GetMonData(mon, MON_DATA_LEVEL, NULL);
			item = HackDifficulty_RecoveryItem(item, level, Random() % 100, VarGet(HOENN_VAR_DIFFICULTY));
			if (item != ITEM_NONE)
				SetMonData(mon, MON_DATA_HELD_ITEM, &item);
		}
	}
}

/* ★ 2026-10-03 ユーザー「設定の難易度デフォルトやさしいで」: はじめからのとき、なんいどを やさしい にしておく。
   src/save.c の NewGameWipeNewSaveData（拡張のセーブ 0x0203B0E8〜を 0 で消す・拡張変数 0x5000〜も中）の直後に呼ぶ（patches/0146）。
   ★ 値の意味（0 ふつう / 1 やさしい）は変えない（前のセーブで選んだ値はそのまま） */
void HackDifficulty_SetNewGameDefault(void)
{
	VarSet(HOENN_VAR_DIFFICULTY, DIFFICULTY_EASY);
}

/* ★ 2026-10-19 ユーザー「難易度しゃかりき 回復アイテムはキズぐすりのみ」（枝 feature/shakariki-potion-only・patches/0616）:
   しゃかりきのあいだ、自分の側（プレイヤーのポケモン）は キズぐすり（ITEM_POTION）以外の「回復の道具」が使えない。相手（トレーナー・野生）は今までどおり。
     ・バッグ／きのみぶくろの「つかう」（戦闘・フィールドとも）: 使えない（ひとつも消費しない）… HackDifficulty_ItemBlocked
         入口 3 つ: hack_challenge.c の HackChallenge_ItemMenuUse*（バッグの「つかう」）・素の ItemId_GetFieldFunc／ItemId_GetBattleFunc（functionrewrites）＝きのみぶくろの「つかう」・SELECT の登録
     ・もたせた回復のきのみ（HP・PP・状態異常・こんらん）・オボン／ラム系・きのみジュース・メンタルハーブ・ナゾのみ: 自分の側は こうかなし … HackDifficulty_HeldRecoveryNullified
         入口 2 つ: battle_util.c の GetBankItemEffect・item_battle_effects.c の ItemBattleEffects の doPluck（むしくい・ついばむ など相手のきのみを食べる側）
     ・ももアイス／ももシャーベット／くりーぱーアイスの 毎ターンの回復だけ: 自分の側は 回復しない（ほかの効果は そのまま）… HackDifficulty_HeldRegenBlocked
   何が「回復の道具」か: ゲームが使う道具の効果の表（gItemEffectTable）で HP・PP・状態異常・ひんしを治す印があるもの（ふしぎなアメは除く）＋ もたせたときの効果が回復のきのみ系。
     ＝新しい回復の道具を足しても表に印があれば自動で止まる（verify_shakariki_items.py が表から数え直して突き合わせる）。
   決めたこと（5e に報告）: ピーピーアップ／マックスは回復ではない（使える）・EV を下げるきのみ（ポメグ など）は使える・たべのこし／くろいヘドロ／かいがらのすずは使える・
     通信対戦は もたせたきのみの停止から除く（ずれるため）・施設・ポケセン 3F のダミー戦は どちらも対象（バッグも）。 */
extern const u8 gText_Shakariki_CantUseItem[];
extern const struct Item gItemData[];   //src/Tables/item_tables.c
#define DisplayItemMessageInBerryPouch ((void (*)(u8, u8, const u8*, TaskFunc)) (0x0813F240 | 1))   //きのみぶくろの文の窓（素の FieldUseFunc_OakStopsYou 080A34F8 の中の BL の行き先）
#define BerryPouchDestroyDialogueAndRefreshList ((TaskFunc) (0x0813E9D8 | 1))                      //〃 文のあと一覧に戻る（Task_BerryPouch_DestroyDialogueWindowAndRefreshListMenu）

static bool8 EffectTableRecovers(u16 item)
{
	const u8* e;

	if (item < ITEM_POTION || item >= ITEMS_COUNT)
		return FALSE;
	e = gItemEffectTable[item - ITEM_POTION];
	if (e == NULL || (e[3] & ITEM3_LEVEL_UP))   //ふしぎなアメ（ひんし／HP の印も持つが、回復の道具ではない）
		return FALSE;
	return (e[0] & (ITEM0_SACRED_ASH | ITEM0_INFATUATION)) != 0
	    || (e[3] & ITEM3_STATUS_ALL) != 0
	    || (e[4] & (ITEM4_HEAL_HP | ITEM4_HEAL_PP_ALL | ITEM4_HEAL_PP_ONE | ITEM4_REVIVE)) != 0;
}

//もたせたときの効果が 回復のきのみ系か（1〜9: HP・状態異常・PP・こんらん・ラム／10〜14: HP を回復してこんらんするきのみ／メンタルハーブ／ナゾのみ）
static bool8 HoldEffectRecovers(u8 effect)
{
	return (effect >= ITEM_EFFECT_RESTORE_HP && effect <= ITEM_EFFECT_CONFUSE_SOUR)
	    || effect == ITEM_EFFECT_CURE_ATTRACT
	    || effect == ITEM_EFFECT_ENIGMA_BERRY;
}

//バッグ・きのみぶくろで使えない道具か（しゃかりき以外は いつも FALSE）
bool8 HackDifficulty_ItemBlocked(u16 item)
{
	if (item == ITEM_NONE || item == ITEM_POTION || VarGet(HOENN_VAR_DIFFICULTY) != DIFFICULTY_SHAKARIKI)
		return FALSE;
	return item == ITEM_ENIGMA_BERRY || EffectTableRecovers(item) || HoldEffectRecovers(ItemId_GetHoldEffect(item));
}

static bool8 PlayerOwnSide(u8 bank)
{
	if (SIDE(bank) != B_SIDE_PLAYER || (gBattleTypeFlags & BATTLE_TYPE_LINK))
		return FALSE;
	if ((gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)) && bank == GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT))
		return FALSE;   //味方（タッグ）のポケモンは自分のではない
	return TRUE;
}

//自分の側の もたせた回復のきのみ系の こうかを消すか（battle_util.c の GetBankItemEffect・item_battle_effects.c の ItemBattleEffects）
bool8 HackDifficulty_HeldRecoveryNullified(u8 bank, u8 effect)
{
	if (effect == 0 || !HoldEffectRecovers(effect) || VarGet(HOENN_VAR_DIFFICULTY) != DIFFICULTY_SHAKARIKI)
		return FALSE;
	return PlayerOwnSide(bank);
}

//ももアイス／ももシャーベット／くりーぱーアイスの 毎ターンの回復だけ止めるか（item_battle_effects.c の 3 つの case）
bool8 HackDifficulty_HeldRegenBlocked(u8 bank, u8 effect)
{
	if ((effect != ITEM_EFFECT_MOMO_ICE && effect != ITEM_EFFECT_MOMO_SHERBET && effect != ITEM_EFFECT_CREEPER_ICE)
	|| VarGet(HOENN_VAR_DIFFICULTY) != DIFFICULTY_SHAKARIKI)
		return FALSE;
	return PlayerOwnSide(bank);
}

//止められた道具を ItemId_GetFieldFunc／GetBattleFunc から受け取った側（きのみぶくろ・SELECT の登録）が呼ぶ: 一言を出して元の画面へ
static void BlockedItemTask(u8 taskId)
{
	if (GetPocketByItemId(gSpecialVar_ItemId) == POCKET_BERRY_POUCH)
		DisplayItemMessageInBerryPouch(taskId, 4, gText_Shakariki_CantUseItem, BerryPouchDestroyDialogueAndRefreshList);
	else
		DisplayItemMessageInBag(taskId, 2, gText_Shakariki_CantUseItem, Task_ReturnToBagFromContextMenu);
}

//functionrewrites: ItemId_GetFieldFunc 0809A430（素は gItems[SanitizeItemId(id)].fieldUseFunc を返すだけ・引数 1・戻り値あり）
ItemUseFunc HackShakariki_ItemGetFieldFunc(u16 itemId)
{
	if (HackDifficulty_ItemBlocked(itemId))
		return BlockedItemTask;
	return gItemData[SanitizeItemId(itemId)].fieldUseFunc;
}

//functionrewrites: ItemId_GetBattleFunc 0809A47C（素は battleUseFunc を返すだけ）
ItemUseFunc HackShakariki_ItemGetBattleFunc(u16 itemId)
{
	if (HackDifficulty_ItemBlocked(itemId))
		return BlockedItemTask;
	return gItemData[SanitizeItemId(itemId)].battleUseFunc;
}
