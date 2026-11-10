#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/constants/flags.h"
#include "Tables/hoenn_script_flags.h"
#include "Tables/johto_flags.h"
#include "Tables/sinnoh_flags.h"
#include "Tables/hack_gym_tiers.h"

/*
 * ジムリーダー 32 人の再戦の「段階」（0〜9・#117・2026-10-16 ユーザー決定・tools/kanto/gym_tiers.py が数字の表と src/Tables/hack_gym_tiers.h を作る）。
 *
 *  - 再戦で勝つたびに段階が +1（最大 GYM_TIER_MAX）。次の再戦から、段階に合った「顔ぶれ」のトレーナー（5 通り × 32 人・GYM_TIER_VARIANT_IDS）で戦い、
 *    手持ちを作るとき段階のぶんのレベルを足す（HackGymTier_AdjustMon・GYM_TIER_LEVEL_SHIFT）。
 *    スクリプトは今までの再戦の番号（段階 0）で戦いを始めるだけ。手持ちを作るとき（CreateNPCTrainerParty）に顔ぶれのトレーナーへ差し替える（HackGymTier_MapTrainer）
 *    ＝名前・絵・クラス・音楽・ダイマックスの印・トレーナーの旗は段階 0 のまま変わらない。
 *  - 状態は予約変数 HACK_VAR_GYM_TIER_0〜10（11 本・1 人 5 ビット × 3 人）。値 = 段階 × 3 + 状態（0 まだ／1 さそった／2 もう戦った）≤ 29。
 *    0 は「段階 0・まだ」＝古いセーブはそのまま段階 0 から。セーブの形は変わらない（使っていない予約変数）。
 *  - 再戦を始められるのは「さそった」とき（電話帳の「さいせんに さそう」）。勝つと 段階 +1・「もう戦った」。どの地方のリーグでも殿堂入り（HoennLeague_SetWarpsAfterHallOfFame）
 *    するたびに「もう戦った」→「まだ」に戻る（＝リーグをクリアするたびに もう 1 回）。誘えるのは、その人の地方の最初の再戦の条件（HackGymTier_RegionOpen）を満たしたあと。
 *  - 賞金は段階ごとの倍率（GYM_TIER_PRIZE_PERCENT・src/multi.c の PrizeForOneTrainer）・最終レベルは「バッジの数のレベルの上限」（src/hack_level_cap.c・32 個で 180）で止める（HackGymTier_AdjustMon）。
 *  - 状態は VarGet／VarSet だけ（CFRU は書き換えられる static を置けない・6.13）。
 */

enum { ST_IDLE, ST_INVITED, ST_DONE };

#define GYM_TIER_CODES 30      //段階 0〜9 × 状態 3
#define NOT_LEADER 0xFF

u8 GetAllBadgeCount(void);                //src/badge_count.c
u8 HackLevelCap_FromBadges(u8 badges);   //src/hack_level_cap.c

static const u16 sBaseIds[GYM_TIER_COUNT] = {GYM_TIER_BASE_IDS};
static const u16 sVariantIds[GYM_TIER_VARIANTS][GYM_TIER_COUNT] = {GYM_TIER_VARIANT_IDS};
static const u8 sLevelShift[GYM_TIER_COUNT][GYM_TIER_MAX + 1] = {GYM_TIER_LEVEL_SHIFT};
static const u8 sVariantOfTier[GYM_TIER_MAX + 1] = GYM_TIER_VARIANT_OF_TIER;
static const u16 sPrizePercent[GYM_TIER_MAX + 1] = GYM_TIER_PRIZE_PERCENT;
static const u8 sCodeTier[GYM_TIER_CODES] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9};   //割り算を使わない（6.83）

//1 人 5 ビット × 3 人 / 本。本の番号 = 番号 ÷ 3（0〜31 では (番号 × 43) >> 7 と同じ・検算が確かめる）
static u32 VarIndex(u32 idx)
{
	return (idx * 43) >> 7;
}

static u32 GetCode(u32 idx)
{
	u32 i = VarIndex(idx);
	u32 code = (VarGet(HACK_VAR_GYM_TIER_0 + i) >> ((idx - i * 3) * 5)) & 0x1F;

	return (code < GYM_TIER_CODES) ? code : 0;
}

static void SetCode(u32 idx, u32 code)
{
	u32 i = VarIndex(idx);
	u16 var = HACK_VAR_GYM_TIER_0 + i;
	u32 shift = (idx - i * 3) * 5;
	u32 v = VarGet(var);

	v = (v & ~(0x1F << shift)) | (code << shift);
	VarSet(var, v);
}

static u32 TierOf(u32 idx)
{
	return sCodeTier[GetCode(idx)];
}

static u32 StateOf(u32 idx)
{
	u32 code = GetCode(idx);

	return code - sCodeTier[code] * 3;
}

//段階 0 の番号 → ジムリーダーの番号（0〜31）。違えば NOT_LEADER
static u32 FindBase(u16 id)
{
	u32 i;

	for (i = 0; i < GYM_TIER_COUNT; ++i)
		if (sBaseIds[i] == id)
			return i;
	return NOT_LEADER;
}

//段階 0 の番号か、顔ぶれのトレーナーの番号 → ジムリーダーの番号（0〜31）
static u32 FindAny(u16 id)
{
	u32 v, i;

	i = FindBase(id);
	if (i != NOT_LEADER)
		return i;
	for (v = 0; v < GYM_TIER_VARIANTS; ++v)
		for (i = 0; i < GYM_TIER_COUNT; ++i)
			if (sVariantIds[v][i] == id)
				return i;
	return NOT_LEADER;
}

//その人の地方の「最初の再戦」の条件（0 カントー＝殿堂入り・1 ホウエン＝リーグ制覇 1 回以上・2 ジョウト＝ジョウトのバッジ 8 つ・3 シンオウ＝殿堂入り）
//  ジョウトにはリーグの部屋が無いので殿堂入りの旗が立たない。シンオウの SINNOH_FLAG_LEAGUE_HOF は殿堂入りのあと消える一度きりの印なので
//  HACK_FLAG_SINNOH_CLEARED（HackGymTier_OnLeagueClear が立てる）を見る
bool8 HackGymTier_RegionOpen(u8 region)
{
	u32 i;

	switch (region) {
		case 1:
			return VarGet(HOENN_VAR_LEAGUE_WINS) >= 1;
		case 2:
			for (i = 0; i < 8; ++i)
				if (!FlagGet(JOHTO_FLAG_BADGE1 + i))
					return FALSE;
			return TRUE;
		case 3:
			return FlagGet(HACK_FLAG_SINNOH_CLEARED) || FlagGet(SINNOH_FLAG_LEAGUE_HOF);
		default:
			return FlagGet(FLAG_SYS_GAME_CLEAR) != 0;
	}
}

//src/build_pokemon.c の CreateNPCTrainerParty の頭: 段階 0 のジムリーダーの番号なら、今の段階の「顔ぶれ」のトレーナーの番号に差し替える（手持ちの種族・型だけ変わる）
u16 HackGymTier_MapTrainer(u16 id)
{
	u32 idx = FindBase(id), variant;

	if (idx == NOT_LEADER)
		return id;
	variant = sVariantOfTier[TierOf(idx)];
	if (variant == 0)
		return id;
	return sVariantIds[variant - 1][idx];
}

//同じ所（ももアイス +5・むずかしい +1・レベルスケーリングのあと・能力を計算する直前）: ジムリーダーの再戦の手持ち（i 番目）のレベルを
//  段階のぶん引き上げ（段階 0 のレベル + 足し分より低いとき）、最後に「バッジの数のレベルの上限」（src/hack_level_cap.c・32 個で GYM_TIER_LEVEL_CAP）で止める
void HackGymTier_AdjustMon(struct Pokemon* mon, u16 id, u8 side, u8 i)
{
	u32 idx, level, target, cap, tier;
	u16 species;
	const struct Trainer* base;

	if (side != B_SIDE_OPPONENT)
		return;
	idx = FindAny(id);
	if (idx == NOT_LEADER)
		return;
	level = GetMonData(mon, MON_DATA_LEVEL, NULL);
	tier = TierOf(idx);
	base = &gTrainers[sBaseIds[idx]];
	if (tier > 0 && base->partyFlags == (PARTY_FLAG_CUSTOM_MOVES | PARTY_FLAG_HAS_ITEM) && i < base->partySize)
	{
		target = base->party.ItemCustomMoves[i].lvl + sLevelShift[idx][tier];
		if (level < target)
			level = target;
	}
	cap = HackLevelCap_FromBadges(GetAllBadgeCount());
	if (cap > GYM_TIER_LEVEL_CAP)
		cap = GYM_TIER_LEVEL_CAP;
	if (level > cap)
		level = cap;
	if (level == GetMonData(mon, MON_DATA_LEVEL, NULL))
		return;
	species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	SetMonData(mon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][level]);
	SetMonData(mon, MON_DATA_LEVEL, &level);
}

//src/multi.c の PrizeForOneTrainer: 今の段階の賞金の倍率（%）。ジムリーダーの再戦でなければ 100。
//  勝ったあとに段階が上がる（HackGymTier_OnBattleEnd）ので、賞金はいま戦った段階の倍率
u32 HackGymTier_PrizePercent(u16 id)
{
	u32 idx = FindBase(id);

	if (idx == NOT_LEADER)
		return 100;
	return sPrizePercent[TierOf(idx)];
}

//src/end_battle.c の EndOfBattleThings: ジムリーダーの再戦に勝ったら 段階 +1（最大まで）・「もう戦った」。負けたら（さそった のまま）次もう一度戦える
void HackGymTier_OnBattleEnd(void)
{
	u32 idx, tier;

	if ((gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK)) != BATTLE_TYPE_TRAINER || gBattleOutcome != B_OUTCOME_WON)
		return;
	idx = FindBase(gTrainerBattleOpponent_A);
	if (idx == NOT_LEADER)
		return;
	tier = TierOf(idx);
	if (tier < GYM_TIER_MAX)
		++tier;
	SetCode(idx, tier * 3 + ST_DONE);
}
//殿堂入りのあと（src/hoenn_league.c の HoennLeague_SetWarpsAfterHallOfFame・どの地方のリーグでも）:
//  シンオウの殿堂入りの印を残し、「もう戦った」→「まだ」に戻す（リーグをクリアするたびに もう 1 回）。さそった人はそのまま
void HackGymTier_OnLeagueClear(void)
{
	u32 idx;

	if (FlagGet(SINNOH_FLAG_LEAGUE_HOF))      //この印は このあとの処理で消える（一度きり）
	{
		FlagSet(HACK_FLAG_SINNOH_CLEARED);
		FlagClear(HACK_FLAG_BADGE_RESET_SINNOH);   //#125: もう一度リーグをクリアしたら、バッジリセットのあとの印を下ろす（#124 の上限は殿堂入りで外れる）
	}
	if (FlagGet(HOENN_FLAG_HOF_IN_HOENN))
		FlagClear(HACK_FLAG_BADGE_RESET_HOENN);
	for (idx = 0; idx < GYM_TIER_COUNT; ++idx)
		if (StateOf(idx) == ST_DONE)
			SetCode(idx, TierOf(idx) * 3 + ST_IDLE);
}

/* ---- スクリプトから（callasm・callnative。番号は VAR 0x8004 = 0〜31・結果は LASTRESULT） ---- */

//再戦をはじめられるか（さそってあって、その地方の最初の再戦の条件を満たしている）→ 1 / 0
static void CanFight(void)
{
	u32 idx = VarGet(0x8004);

	gSpecialVar_LastResult = (idx < GYM_TIER_COUNT && HackGymTier_RegionOpen(idx >> 3) && StateOf(idx) == ST_INVITED);
}

void HackGymTier_CanFight(void)
{
	CanFight();
}

void HoennNative_GymTierCanFight(void)     //ホウエンのジムの台本（エメラルドから移した台本・callnative は HoennNative_ の名前だけ）
{
	CanFight();
}

//電話帳の「さいせんに さそう」→ 0 さそった（再戦できる）／1 まだ条件を満たしていない／2 もう戦った（次のリーグクリアまで）／3 もう さそってある
void HackGymTier_TryInvite(void)
{
	u32 idx = VarGet(0x8004), state;

	if (idx >= GYM_TIER_COUNT || !HackGymTier_RegionOpen(idx >> 3))
	{
		gSpecialVar_LastResult = 1;
		return;
	}
	state = StateOf(idx);
	if (state == ST_DONE)
		gSpecialVar_LastResult = 2;
	else if (state == ST_INVITED)
		gSpecialVar_LastResult = 3;
	else
	{
		SetCode(idx, TierOf(idx) * 3 + ST_INVITED);
		gSpecialVar_LastResult = 0;
	}
}

//電話の会話: LASTRESULT = 今の状態（0 まだ／1 さそってある／2 もう戦った）
void HackGymTier_GetState(void)
{
	u32 idx = VarGet(0x8004);

	gSpecialVar_LastResult = (idx < GYM_TIER_COUNT) ? StateOf(idx) : 0;
}

//電話の会話の「つよくなったぞ」: LASTRESULT = 今の段階（0〜4）
void HackGymTier_GetTier(void)
{
	u32 idx = VarGet(0x8004);

	gSpecialVar_LastResult = (idx < GYM_TIER_COUNT) ? TierOf(idx) : 0;
}
