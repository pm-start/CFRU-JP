#include "defines.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "hack_box_shelf.h"
#include "hack_egg_box.h"
#include "hack_mon_id.h"
#include "hack_moemon_gen.h"

/*
 * Moémon の世代の控え（hack_moemon_gen.c）の「持ち主」の列挙（[hack] 0.63.1）。今いる Moémon の当たりの子（HackMonId_IsMoemonHit の定義そのまま）の性格値を集める。
 *   手持ち RAM・SaveBlock1 の手持ち・safeBackupParty・育て屋（いまの 2 匹＋控え 3 地方の各 2 匹）・5 ばんどうろの預かり 1・合体ポケモン 6 匹・
 *   卵専用ボックス 30 匹・箱（見える箱ぜんぶ＝50 か 100 箱）。上の桁の表の列挙（hack_stat_pool_owners.c）に 卵ボックスと箱を足した形。
 *   当たりでない子は絵が無い＝世代は見た目に関係しないので控えない。
 * 数えない: gTempTeamBackup（施設の挑戦中の退避＝セーブされるのは safeBackupParty）・PSS の持ち上げ中の 1 匹（フィールドでは無い）。
 */

#define CHUNK 5                           //箱を 5 匹ずつ一括で読む（78 バイト × 5 = 390 バイト・読みの深い呼び出しの上に載るので小さく＝スタック 1KB の決まり・6.35）

struct DayCare* HackDaycare_GetStashed(u8 i);   //src/hack_daycare_region.c

static void AddBox(struct MoemonGenCollect* c, const struct BoxPokemon* m)
{
	if (m->substruct0.species != SPECIES_NONE && HackMonId_IsMoemonHit(m))
		HackMoemonGen_AddHit(c, m->personality);
}

static void AddComp(struct MoemonGenCollect* c, const struct CompressedPokemon* m)
{
	if (m->substruct0.species != SPECIES_NONE && HackMonId_CompIsMoemonHit(m))
		HackMoemonGen_AddHit(c, m->personality);
}

static void AddDaycare(struct MoemonGenCollect* c, const struct DayCare* d)
{
	u8 i;

	if (d == NULL)
		return;
	for (i = 0; i < DAYCARE_MON_COUNT; ++i)
		AddBox(c, &d->mons[i].mon);
}

static bool8 AddEggBox(struct MoemonGenCollect* c)
{
	void* image;
	u8 i;

	if (EggBox_Usable() != EGGBOX_OK || EggBox_Count() == 0)
		return TRUE;
	image = Malloc(EggBox_ImageSize());
	if (image == NULL)
		return FALSE;                     //取れなかった: あとでやり直す
	if (EggBox_Load(image))               //読めない（壊れ）箱は空として数える
	{
		for (i = 0; i < EGGBOX_SLOTS; ++i)
			if (EggBox_SlotHatchAt(image, i) != 0)
				AddComp(c, EggBox_SlotMon(image, i));
	}
	Free(image);
	return TRUE;
}

//箱の中身（見える箱ぜんぶ）。入れ物 buf をここだけに持つ（持ち主の列挙の枠に載せない）
static bool8 __attribute__((noinline)) AddBoxes(struct MoemonGenCollect* c)
{
	struct CompressedPokemon buf[CHUNK];
	u8 box, first, k, boxes = BoxShelf_VisibleBoxes();

	for (box = 0; box < boxes; ++box)
	{
		for (first = 0; first < IN_BOX_COUNT; first += CHUNK)
		{
			if (!BoxShelf_ReadBoxMons(box, first, CHUNK, buf))
				return FALSE;
			for (k = 0; k < CHUNK; ++k)
				AddComp(c, &buf[k]);
		}
	}
	return TRUE;
}

bool8 HackMoemonGen_CollectAll(struct MoemonGenCollect* c)
{
	u8 i;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		AddBox(c, (const struct BoxPokemon*) &gPlayerParty[i]);
		AddBox(c, (const struct BoxPokemon*) &gSaveBlock1->playerParty[i]);
		AddBox(c, &gSaveBlock1->safeBackupParty[i]);
	}
	AddDaycare(c, &gSaveBlock1->daycare);
	for (i = 0; i < 3; ++i)
		AddDaycare(c, HackDaycare_GetStashed(i));
	AddBox(c, &gSaveBlock1->route5DayCareMon.mon);
	AddBox(c, (const struct BoxPokemon*) &gSaveBlock1->fusedReshiram);
	AddBox(c, (const struct BoxPokemon*) &gSaveBlock1->fusedZekrom);
	AddBox(c, (const struct BoxPokemon*) &gSaveBlock1->fusedSolgaleo);
	AddBox(c, (const struct BoxPokemon*) &gSaveBlock1->fusedLunala);
	AddBox(c, (const struct BoxPokemon*) &gSaveBlock2->fusedSpectrier);
	AddBox(c, (const struct BoxPokemon*) &gSaveBlock2->fusedGlastrier);
	if (!AddEggBox(c))
		return FALSE;
	return AddBoxes(c);
}
