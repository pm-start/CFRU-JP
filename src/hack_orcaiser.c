#include "defines.h"
#include "../include/data2.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/constants/items.h"
#include "../include/new/catching.h"
#include "../include/pokemon.h"
#include "../include/pokedex.h"
#include "../include/random.h"
#include "../include/string_util.h"
#include "../include/constants/species.h"
#include "hack_orcaiser.h"

/*
 * ★ 2026-11-02 きょがのカセキ（ITEM_KYOGA_FOSSIL 0x371・Redmine #230・patches/0952）ほか、グレンの研究所の かせきの研究員
 * （assembly/kanto/kanto_moemon_fossil.s）が その場で よみがえらせる化石の表。前例は Moemonかせき（src/hack_moemon_fossil.c）。
 * ★ 2026-11-04 ユーザー決定: 素の台本ではよみがえらせられない 6 種（ずがい・たて・ふた・はね・アゴ・ヒレ）も この表に足した（素の台本は
 *   ひみつのコハク・かいのかせき・こうらのかせき、ホウエンのデボン 2F は ツメ・ねっこ）。きょがのカセキは Moémon の絵が無いので ふつうの姿のコシャル。
 * 台本は 表を 先頭から順に「持っているか」を HackFossil_FindNext で調べ、はい なら HackFossil_Revive（Lv5・道具を 1 つ減らす）。
 * いいえ なら 次の化石へ。どれも持っていなければ・全部いいえ なら素の台本へ。
 * HackFossil_FindNext: 入力 VAR 0x8004 = 調べ始めの添字。LASTRESULT 0 = 無し／1 = 有り（VAR 0x8004 = 添字・VAR 0x8005 = 道具番号）
 * HackFossil_Revive: 入力 VAR 0x8004 = 添字。LASTRESULT: 0 手持ち / 1 パソコン / 2 どちらもいっぱい（道具は減らさない）。gStringVar1 = 種族名
 */
#define FOSSIL_REVIVE_LEVEL 5

static const struct { u16 item; u16 species; } sFossilRevive[] =
{
	{ITEM_KYOGA_FOSSIL, SPECIES_KOSHAL},
	{ITEM_SKULL_FOSSIL, SPECIES_CRANIDOS},
	{ITEM_ARMOR_FOSSIL, SPECIES_SHIELDON},
	{ITEM_COVER_FOSSIL, SPECIES_TIRTOUGA},
	{ITEM_PLUME_FOSSIL, SPECIES_ARCHEN},
	{ITEM_JAW_FOSSIL, SPECIES_TYRUNT},
	{ITEM_SAIL_FOSSIL, SPECIES_AMAURA},
};

void HackFossil_FindNext(void)
{
	u16 i;

	for (i = Var8004; i < ARRAY_COUNT(sFossilRevive); ++i)
	{
		if (CheckBagHasItem(sFossilRevive[i].item, 1))
		{
			Var8004 = i;
			Var8005 = sFossilRevive[i].item;
			gSpecialVar_LastResult = 1;
			return;
		}
	}
	gSpecialVar_LastResult = 0;
}

static u8 GiveFossilMon(u16 species)   //手持ち／パソコンへ渡し、図鑑・名前の文まで。戻り値は LASTRESULT と同じ（0 手持ち／1 パソコン／2 どちらもいっぱい）
{
	struct Pokemon mon;
	u8 result;

	CreateMon(&mon, species, FOSSIL_REVIVE_LEVEL, 32, FALSE, 0, OT_ID_PLAYER_ID, 0);
	result = GiveMonToPlayer(&mon);
	species = GetMonData(&mon, MON_DATA_SPECIES, NULL);   //[hack] しばり『ぜんぶレントラー』ON のときは レントラーになっている（OFF は同じ種族）＝名前の文と図鑑もその種族
	gSpecialVar_LastResult = result;
	StringCopy(gStringVar1, gSpeciesNames[species]);
	if (result < 2)
	{
		HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_SEEN, mon.personality);
		HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT, mon.personality);
	}
	return result;
}

void HackFossil_Revive(void)
{
	u16 index = Var8004;

	if (index >= ARRAY_COUNT(sFossilRevive))
		index = 0;
	if (GiveFossilMon(sFossilRevive[index].species) < 2)
		RemoveBagItem(sFossilRevive[index].item, 1);   //どちらもいっぱい(2)のときは減らさない
}

/*
 * ★ 2026-11-04 ユーザー決定: 剣盾のかけら（とり・さかな・りゅう・きょうりゅう＝本流の 0x27A〜0x27D）を ウカッツ（剣盾）と同じに、
 * 上（とり／さかな）と 下（りゅう／きょうりゅう）を 1 つずつ組み合わせて よみがえらせる。同じ研究員（kanto_moemon_fossil.s の KantoFossilPair_*）。
 * HackFossil_FindPair: 入力 VAR 0x8004 = 調べ始めの組。LASTRESULT 0 = 無し／1 = 有り（VAR 0x8004 = 組・0x8005 = 上の道具・0x8006 = 下の道具）
 * HackFossil_RevivePair: 入力 VAR 0x8004 = 組。LASTRESULT は HackFossil_Revive と同じ。2 つとも 1 つずつ減らす
 */
static const struct { u16 top; u16 bottom; u16 species; } sFossilPair[] =
{
	{ITEM_FOSSILIZED_BIRD, ITEM_FOSSILIZED_DRAKE, SPECIES_DRACOZOLT},
	{ITEM_FOSSILIZED_BIRD, ITEM_FOSSILIZED_DINO, SPECIES_ARCTOZOLT},
	{ITEM_FOSSILIZED_FISH, ITEM_FOSSILIZED_DRAKE, SPECIES_DRACOVISH},
	{ITEM_FOSSILIZED_FISH, ITEM_FOSSILIZED_DINO, SPECIES_ARCTOVISH},
};

void HackFossil_FindPair(void)
{
	u16 i;

	for (i = Var8004; i < ARRAY_COUNT(sFossilPair); ++i)
	{
		if (CheckBagHasItem(sFossilPair[i].top, 1) && CheckBagHasItem(sFossilPair[i].bottom, 1))
		{
			Var8004 = i;
			Var8005 = sFossilPair[i].top;
			Var8006 = sFossilPair[i].bottom;
			gSpecialVar_LastResult = 1;
			return;
		}
	}
	gSpecialVar_LastResult = 0;
}

void HackFossil_RevivePair(void)
{
	u16 index = Var8004;

	if (index >= ARRAY_COUNT(sFossilPair))
		index = 0;
	if (GiveFossilMon(sFossilPair[index].species) < 2)
	{
		RemoveBagItem(sFossilPair[index].top, 1);
		RemoveBagItem(sFossilPair[index].bottom, 1);
	}
}

/*
 * ★ 2026-11-08 カセキメラ 4 種（patches/0968）: 剣盾のかけら 1 つで よみがえらせる（Lv5）。聞く順は 組（上の FindPair）のあと・素の台本の前。
 * HackFossil_FindKase: 入力 VAR 0x8004 = 調べ始めの添字。LASTRESULT 0 = 無し／1 = 有り（VAR 0x8004 = 添字・VAR 0x8005 = 道具番号）
 * HackFossil_ReviveKase: 入力 VAR 0x8004 = 添字。LASTRESULT は HackFossil_Revive と同じ。かけらを 1 つ減らす
 */
static const struct { u16 item; u16 species; } sFossilKase[] =
{
	{ITEM_FOSSILIZED_BIRD, SPECIES_PACHINIKUSU},
	{ITEM_FOSSILIZED_FISH, SPECIES_DANKURUO},
	{ITEM_FOSSILIZED_DRAKE, SPECIES_SUTEGORADON},
	{ITEM_FOSSILIZED_DINO, SPECIES_PURECHIRUDO},
};

void HackFossil_FindKase(void)
{
	u16 i;

	for (i = Var8004; i < ARRAY_COUNT(sFossilKase); ++i)
	{
		if (CheckBagHasItem(sFossilKase[i].item, 1))
		{
			Var8004 = i;
			Var8005 = sFossilKase[i].item;
			gSpecialVar_LastResult = 1;
			return;
		}
	}
	gSpecialVar_LastResult = 0;
}

void HackFossil_ReviveKase(void)
{
	u16 index = Var8004;

	if (index >= ARRAY_COUNT(sFossilKase))
		index = 0;
	if (GiveFossilMon(sFossilKase[index].species) < 2)
		RemoveBagItem(sFossilKase[index].item, 1);   //どちらもいっぱい(2)のときは減らさない
}
