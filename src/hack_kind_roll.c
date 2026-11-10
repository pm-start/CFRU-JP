#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/random.h"
#include "../include/constants/trainer_classes.h"
#include "hack_kind.h"
#include "hack_kind_roll.h"
#include "hack_delta.h"
#include "hack_dummy_battle.h"

/*
 * 個体の印のランダム付与（src/hack_kind_roll.h に仕様）。状態を持たない（6.13）・除算は 1 関数に 1 回（6.83）。
 */

//確率表（万分率・dark, nushi, delta）。設計書 NUSHI_DESIGN.md 2.2 章の案のまま（ユーザーが後で決める＝ここだけ直す）
#ifdef HACK_KIND_DEBUG_BOOST
//デバッグ版の確率増し（実機の確かめ用・build_rom.py --debug-menu --kind-boost のときだけ・配る版・通常版には入らない＝verify_shadow_roll が見る）
static const struct HackKindBands sBands[HACK_KIND_SCENE_COUNT] =
{
	[HACK_KIND_SCENE_WILD] = {0, 5000, 5000},
	[HACK_KIND_SCENE_TRAINER] = {0, 5000, 5000},
	[HACK_KIND_SCENE_GRUNT] = {10000, 0, 0},
	[HACK_KIND_SCENE_BOSS] = {10000, 0, 0},
};
#else
static const struct HackKindBands sBands[HACK_KIND_SCENE_COUNT] =
{
	[HACK_KIND_SCENE_WILD] = {0, 50, 150},
	[HACK_KIND_SCENE_TRAINER] = {0, 30, 100},
	[HACK_KIND_SCENE_GRUNT] = {1500, 30, 100},
	[HACK_KIND_SCENE_BOSS] = {3000, 0, 0},
};
#endif

//組織ごとに「ストーリーで最初に戦うしたっぱ」（先頭 1 匹は必ずダーク）。根拠: アクア＝1 章の博物館の 1 人目（hoenn_story.py:443・TRAINER_GRUNT_MUSEUM_1）・
//マグマ＝2 章のえんとつやま（目が合うと戦う 2 人のどちらも・避けられても片方は残る・hoenn_story.py:828-829）・ロケット＝おつきみやま B2F の 1 人目（pokefirered MtMoon_B2F/scripts.inc:119・目が合うと戦う人で避けられるかもしれない）＋ 24 ばんどうろのナゲットばしの終わり（避けられない・橋の終わりの座標トリガー Route24/scripts.inc:57・TRAINER_TEAM_ROCKET_GRUNT_6）・
//ギンガ＝1 章の最初のしたっぱ（sinnoh_story.py:313・TRAINER_STORY_GRUNT_ACUITY_1）
static const u16 sForcedDark[] =
{
	1332,   //アクア TRAINER_GRUNT_MUSEUM_1
	1334,   //マグマ TRAINER_GRUNT_MT_CHIMNEY_1
	1335,   //マグマ TRAINER_GRUNT_MT_CHIMNEY_2
	351,    //ロケット TRAINER_TEAM_ROCKET_GRUNT（おつきみやま B2F）
	356,    //ロケット TRAINER_TEAM_ROCKET_GRUNT_6（24 ばんどうろ・避けられない）
	950,    //ギンガ TRAINER_STORY_GRUNT_ACUITY_1
};

const struct HackKindBands* HackKind_Bands(u8 scene)
{
	return &sBands[scene < HACK_KIND_SCENE_COUNT ? scene : HACK_KIND_SCENE_WILD];
}

//帯の順: ダーク → ヌシ → デルタ（重ならない）
u8 HackKind_Pick(const struct HackKindBands* b, u16 r)
{
	u32 edge = b->dark;

	if (r < edge)
		return HACK_KIND_DARK;
	edge += b->nushi;
	if (r < edge)
		return HACK_KIND_NUSHI;
	edge += b->delta;
	if (r < edge)
		return HACK_KIND_DELTA;
	return HACK_KIND_NONE;
}

//「したっぱ」（0x0C 0x10 0x50 0x4B・hack_grunt_stats.c と同じ）
static bool8 IsGruntName(const u8* name)
{
	return name[0] == 0x0C && name[1] == 0x10 && name[2] == 0x50 && name[3] == 0x4B;
}

u8 HackKind_TrainerOrg(u8 trainerClass, const u8* trainerName)
{
	(void) trainerName;
	switch (trainerClass)
	{
		case CLASS_TEAM_AQUA:
		case CLASS_AQUA_ADMIN:
		case CLASS_AQUA_LEADER:
			return HACK_KIND_ORG_AQUA;
		case CLASS_TEAM_MAGMA:
		case CLASS_MAGMA_ADMIN:
		case CLASS_MAGMA_LEADER:
			return HACK_KIND_ORG_MAGMA;
		case CLASS_TEAM_ROCKET:
		case CLASS_BOSS:
			return HACK_KIND_ORG_ROCKET;
		case CLASS_GALACTIC_GRUNT:
		case CLASS_GALACTIC_ADMIN:
		case CLASS_GALACTIC_BOSS:
			return HACK_KIND_ORG_GALACTIC;
	}
	return HACK_KIND_ORG_NONE;
}

u8 HackKind_TrainerTier(u8 trainerClass, const u8* trainerName)
{
	switch (trainerClass)
	{
		//主要トレーナー（印を付けない）
		case CLASS_LEADER:
		case CLASS_LEADER_RS:
		case CLASS_ELITE_4:
		case CLASS_ELITE_FOUR_RS:
		case CLASS_CHAMPION:
		case CLASS_CHAMPION_RS:
		case CLASS_RIVAL:
		case CLASS_RIVAL_2:
		case CLASS_PKMN_TRAINER_1:
		case CLASS_PKMN_TRAINER_2:
		case CLASS_PKMN_TRAINER_3:
		case CLASS_PLAYER:
		case CLASS_PKMN_PROF:
			return HACK_KIND_TIER_EXCLUDED;
		//組織のしたっぱ
		case CLASS_TEAM_AQUA:
		case CLASS_TEAM_MAGMA:
		case CLASS_GALACTIC_GRUNT:
			return HACK_KIND_TIER_GRUNT;
		case CLASS_TEAM_ROCKET:
			return IsGruntName(trainerName) ? HACK_KIND_TIER_GRUNT : HACK_KIND_TIER_BOSS;   //同じクラスの「かんぶ」は幹部の帯
		//組織の幹部・ボス
		case CLASS_AQUA_ADMIN:
		case CLASS_AQUA_LEADER:
		case CLASS_MAGMA_ADMIN:
		case CLASS_MAGMA_LEADER:
		case CLASS_BOSS:
		case CLASS_GALACTIC_ADMIN:
		case CLASS_GALACTIC_BOSS:
			return HACK_KIND_TIER_BOSS;
	}
	return HACK_KIND_TIER_NORMAL;
}

bool8 HackKind_IsForcedDark(u16 trainerId)
{
	u32 i;

	for (i = 0; i < sizeof(sForcedDark) / sizeof(sForcedDark[0]); ++i)
		if (sForcedDark[i] == trainerId)
			return TRUE;
	return FALSE;
}

static void Mark(struct Pokemon* mon, u8 kind)
{
	if (kind == HACK_KIND_NONE)
		return;
	if (kind == HACK_KIND_DELTA)
	{
		HackDelta_Assign((struct BoxPokemon*) mon, 0xFF);   //[hack] まとめの枝: デルタは段 B の口で付ける（値 0＝タイプは種族から自動・0xFF＝上書きなし）
		return;
	}
	HackKind_Set((struct BoxPokemon*) mon, kind, kind == HACK_KIND_DARK ? HACK_KIND_DARK_FULL : 0);   //ダークの値は満タン・ヌシは値を使わない
}

static u8 RollScene(u8 scene)
{
	return HackKind_Pick(HackKind_Bands(scene), Random() % HACK_KIND_DENOM);
}

//野生（CreateWildMon の終わりから）。台本の野生・レイド・幽霊戦はここを通らない
void HackKind_RollWild(struct Pokemon* mon)
{
	if (FlagGet(FLAG_BATTLE_FACILITY) || FlagGet(FLAG_SYS_SAFARI_MODE) || HackDummy_Active())
		return;
	Mark(mon, RollScene(HACK_KIND_SCENE_WILD));
}

//トレーナーの手持ち 1 匹（CreateNPCTrainerParty の手持ちループから・partyIndex はそのトレーナーの何匹目か）
void HackKind_RollTrainer(struct Pokemon* mon, u8 side, u8 trainerClass, const u8* trainerName, u16 trainerId, u32 partyIndex)
{
	u8 tier;

	if (side != B_SIDE_OPPONENT)
		return;
	if ((gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_OAK_TUTORIAL | BATTLE_TYPE_MOCK_BATTLE))
	|| FlagGet(FLAG_BATTLE_FACILITY) || FlagGet(FLAG_SYS_SAFARI_MODE) || HackDummy_Active())
		return;
	tier = HackKind_TrainerTier(trainerClass, trainerName);
	if (tier == HACK_KIND_TIER_EXCLUDED)
		return;
	if (partyIndex == 0 && tier == HACK_KIND_TIER_GRUNT && HackKind_IsForcedDark(trainerId))
	{
		Mark(mon, HACK_KIND_DARK);   //組織ごとの最低 1 体
		return;
	}
	Mark(mon, RollScene(tier == HACK_KIND_TIER_GRUNT ? HACK_KIND_SCENE_GRUNT : (tier == HACK_KIND_TIER_BOSS ? HACK_KIND_SCENE_BOSS : HACK_KIND_SCENE_TRAINER)));
}
