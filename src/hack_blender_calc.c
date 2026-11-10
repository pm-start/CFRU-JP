#include "defines.h"
#include "../include/berry.h"
#include "../include/constants/items.h"
#include "../include/item.h"
#include "../include/malloc.h"
#include "../include/random.h"
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"
#include "hack_blender_calc.h"
#include "hack_blender_berry_table.h"

/*
 * ★ 2026-11-09 FEAT-pokeblock 段 4a（patches/0978）: きのみブレンダーの計算と結果。原文 pokeemerald src/berry_blender.c の
 *   L2246-L2495（CalculatePokeblock ほか）・L1542（SetOpponentsBerryData）・L1210（ConvertItemToBlenderBerry）・L3449（TryUpdateBerryBlenderRecord）。
 *   原文との差:
 *     - 書き換え可能な static（sPokeblockFlavors・sPokeblockPresentFlavors）は ROM に置かれる（6.13）ので関数の中の配列にした（中身の計算は同じ）。
 *     - Debug_*（sDebug_* の変数・Debug_SetStageVars など）は持ち込まない（原文でも debug 専用）。
 *     - struct BlenderBerry から名前を除いた。AreBlenderBerriesSame は原文の呼び方（itemId が同じときだけ呼ぶ）では「味が全部同じか」だけが効くので、名前の比較は落とした。
 *     - きのみの味は gBerries を読まず、原文 berry.c の値を表（hack_blender_berry_table.h・生成器 build_blender_berry_table.py）に写した。
 *       エメラルドの GetBerryInfo はなぞのみ（Enigma）だけ e-Reader の味に差し替えるが、この改造にはその仕組みが無い＝表のなぞのみ（全部 40）を使う。
 *     - 名人は「引数 master」で渡す（原文は FLAG_HIDE_LILYCOVE_CONTEST_HALL_BLEND_MASTER と gSpecialVar_0x8004 == 1 から決める＝台本の都合は段 5）。
 */

#define FIRST_BLENDER_BERRY ITEM_CHERI_BERRY
#define ITEM_TO_BLENDER_BERRY(item) (((item) - FIRST_BLENDER_BERRY) + 1)    //原文 ITEM_TO_BERRY

enum { FL_SPICY, FL_DRY, FL_SWEET, FL_BITTER, FL_SOUR, FL_FEEL };

//原文 sOpponentBerrySets（NUM_NPC_BERRIES * 2 行・各 3 個）。値は「ITEM_TO_BERRY(item) - 1」＝ Cheri 0・Chesto 1・Pecha 2・Rawst 3・Aspear 4
static const u8 sOpponentBerrySets[BLENDER_NPC_BERRIES * 2][3] =
{
	{4, 3, 2},   //プレイヤーが Cheri
	{0, 4, 3},   //Chesto
	{1, 0, 4},   //Pecha
	{2, 1, 0},   //Rawst
	{3, 2, 1},   //Aspear
	{0, 2, 3},   //それ以外（プレイヤーのきのみ % 5 で選ぶ）: Leppa, Figy, ...
	{1, 3, 4},   //Oran, Wiki, ...
	{2, 4, 0},   //Persim, Mago, ...
	{3, 0, 1},   //Lum, Aguav, ...
	{4, 1, 2},   //Sitrus, Iapapa, ...
};

//原文 sBerryMasterBerries（Spelon・Pamtre・Watmel・Durin・Belue）
static const u8 sBerryMasterBerries[5] = {
	ITEM_SPELON_BERRY - ITEM_CHERI_BERRY,
	ITEM_PAMTRE_BERRY - ITEM_CHERI_BERRY,
	ITEM_WATMEL_BERRY - ITEM_CHERI_BERRY,
	ITEM_DURIN_BERRY - ITEM_CHERI_BERRY,
	ITEM_BELUE_BERRY - ITEM_CHERI_BERRY,
};

//原文 sBlackPokeblockFlavorFlags
static const u8 sBlackPokeblockFlavorFlags[10] = {
	(1 << FL_SOUR) | (1 << FL_BITTER) | (1 << FL_SWEET),
	(1 << FL_SOUR) | (1 << FL_SWEET) | (1 << FL_DRY),
	(1 << FL_SOUR) | (1 << FL_DRY) | (1 << FL_SPICY),
	(1 << FL_SOUR) | (1 << FL_BITTER) | (1 << FL_DRY),
	(1 << FL_SOUR) | (1 << FL_BITTER) | (1 << FL_SPICY),
	(1 << FL_BITTER) | (1 << FL_SWEET) | (1 << FL_DRY),
	(1 << FL_BITTER) | (1 << FL_SWEET) | (1 << FL_SPICY),
	(1 << FL_BITTER) | (1 << FL_DRY) | (1 << FL_SPICY),
	(1 << FL_SWEET) | (1 << FL_DRY) | (1 << FL_SPICY),
	(1 << FL_SOUR) | (1 << FL_SWEET) | (1 << FL_SPICY),
};

//スキップの「腕前ふつう」: 原文の NPC（名人以外）だけで回したときの最高の回転数の平均（0.01 RPM 単位・2・3・4 人）。tools/pokeblock/sim_blender_npc.py 3000 ゲーム・種 1 の平均を丸めた
static const u16 sSkipMaxRPM[3] = {9383, 9327, 9329};
//名人（NPC が必ず Best）で回したときの最高の回転数（同じシミュレーターの master・乱数が要らないので毎回同じ値）。名人が 1・2・3 人（全 2・3・4 人）。
static const u16 sSkipMaxRPMMaster[3] = {15748, 14243, 14293};   //3 人は FEAT-pokeblock 段 5（ミナモのロビーの 3 人用の台＝相手が全員名人・ユーザー決定 2026-11-09）。sim_blender_npc.py の名人 N 人

bool8 HackBlender_IsBlendableBerry(u16 itemId)
{
	return itemId >= ITEM_CHERI_BERRY && itemId <= ITEM_ENIGMA_BERRY && ITEM_ENIGMA_BERRY - ITEM_CHERI_BERRY + 1 == BLENDER_BERRY_COUNT;
}

bool8 HackBlender_ConvertItem(struct BlenderBerry* berry, u16 itemId)
{
	u8 i;
	const u8* f;

	if (!HackBlender_IsBlendableBerry(itemId))
		return FALSE;
	f = sBlenderBerryFlavors[itemId - FIRST_BLENDER_BERRY];
	berry->itemId = itemId;
	for (i = 0; i < BLENDER_FLAVORS; i++)
		berry->flavors[i] = f[i];
	return TRUE;
}

void HackBlender_SetOpponentBerries(u16 playerItemId, u8 players, bool8 master, struct BlenderBerry* berries)
{
	u16 opponentSetId = 0;
	u16 opponentBerryId;
	u16 berryMasterDiff;
	u16 i;

	if (playerItemId == ITEM_ENIGMA_BERRY)
	{
		for (i = 0; i < 5; i++)
		{
			if (berries[0].flavors[opponentSetId] > berries[0].flavors[i])
				opponentSetId = i;
		}
		opponentSetId += BLENDER_NPC_BERRIES;
	}
	else
	{
		opponentSetId = ITEM_TO_BLENDER_BERRY(playerItemId) - 1;
		if (opponentSetId >= BLENDER_NPC_BERRIES)
			opponentSetId = (opponentSetId % BLENDER_NPC_BERRIES) + BLENDER_NPC_BERRIES;
	}
	for (i = 0; i < players - 1; i++)
	{
		opponentBerryId = sOpponentBerrySets[opponentSetId][i];
		berryMasterDiff = ITEM_TO_BLENDER_BERRY(playerItemId) - ITEM_TO_BLENDER_BERRY(ITEM_SPELON_BERRY);
		if (master)   //段 5: 名人は台によって 1 人でも 3 人でも（原文は 2 人だけ）。きのみの選び方は原文の名人と同じ規則を相手ごとに当てる
		{
			opponentSetId %= 5;
			opponentBerryId = sBerryMasterBerries[opponentSetId];

			//プレイヤーのきのみが名人のきのみのどれかなら、1 つ下の組（5 を引く）
			if (berryMasterDiff < 5)
				opponentBerryId -= 5;
		}
		HackBlender_ConvertItem(&berries[i + 1], opponentBerryId + FIRST_BLENDER_BERRY);
	}
}

static bool8 AreBlenderBerriesSame(const struct BlenderBerry* berries, u8 a, u8 b)
{
	u8 i;

	if (berries[a].itemId != berries[b].itemId)
		return TRUE;   //原文: 最初の itemId の比較が真なら TRUE を返す（呼ばれるのは itemId が同じときだけ＝ここには来ない）
	for (i = 0; i < BLENDER_FLAVORS; i++)
	{
		if (berries[a].flavors[i] != berries[b].flavors[i])
			return FALSE;
	}
	return TRUE;
}

static u32 CalculatePokeblockColor(const struct BlenderBerry* berries, const s16* _flavors, u8 numPlayers, u8 negativeFlavors)
{
	s16 flavors[BLENDER_FLAVORS];
	s16 present[5];
	s32 i, j;
	u8 numFlavors;

	for (i = 0; i < BLENDER_FLAVORS; i++)
		flavors[i] = _flavors[i];

	j = 0;
	for (i = 0; i < 5; i++)
	{
		if (flavors[i] == 0)
			j++;
	}

	//5 つとも 0・4〜5 つが負・同じきのみを使った、のどれかなら くろ
	if (j == 5 || negativeFlavors > 3)
		return PBLOCK_CLR_BLACK;

	for (i = 0; i < numPlayers; i++)
	{
		for (j = 0; j < numPlayers; j++)
		{
			if (berries[i].itemId == berries[j].itemId && i != j
			 && (berries[i].itemId != ITEM_ENIGMA_BERRY || AreBlenderBerriesSame(berries, i, j)))
				return PBLOCK_CLR_BLACK;
		}
	}

	numFlavors = 0;
	for (i = 0; i < 5; i++)
	{
		if (flavors[i] > 0)
			numFlavors++;
	}

	if (numFlavors > 3)
		return PBLOCK_CLR_WHITE;

	if (numFlavors == 3)
		return PBLOCK_CLR_GRAY;

	for (i = 0; i < 5; i++)
	{
		if (flavors[i] > 50)
			return PBLOCK_CLR_GOLD;
	}

	if (numFlavors == 1 && flavors[FL_SPICY] > 0)
		return PBLOCK_CLR_RED;
	if (numFlavors == 1 && flavors[FL_DRY] > 0)
		return PBLOCK_CLR_BLUE;
	if (numFlavors == 1 && flavors[FL_SWEET] > 0)
		return PBLOCK_CLR_PINK;
	if (numFlavors == 1 && flavors[FL_BITTER] > 0)
		return PBLOCK_CLR_GREEN;
	if (numFlavors == 1 && flavors[FL_SOUR] > 0)
		return PBLOCK_CLR_YELLOW;

	if (numFlavors == 2)
	{
		s32 idx = 0;

		for (i = 0; i < 5; i++)
		{
			if (flavors[i] > 0)
				present[idx++] = i;
		}
		//強いほうの味で色が決まる（弱いほうは上の 16 ビットに入るが、色の判定では使われない）
		if (flavors[present[0]] >= flavors[present[1]])
		{
			if (present[0] == FL_SPICY)
				return (present[1] << 16) | PBLOCK_CLR_PURPLE;
			if (present[0] == FL_DRY)
				return (present[1] << 16) | PBLOCK_CLR_INDIGO;
			if (present[0] == FL_SWEET)
				return (present[1] << 16) | PBLOCK_CLR_BROWN;
			if (present[0] == FL_BITTER)
				return (present[1] << 16) | PBLOCK_CLR_LITE_BLUE;
			if (present[0] == FL_SOUR)
				return (present[1] << 16) | PBLOCK_CLR_OLIVE;
		}
		else
		{
			if (present[1] == FL_SPICY)
				return (present[0] << 16) | PBLOCK_CLR_PURPLE;
			if (present[1] == FL_DRY)
				return (present[0] << 16) | PBLOCK_CLR_INDIGO;
			if (present[1] == FL_SWEET)
				return (present[0] << 16) | PBLOCK_CLR_BROWN;
			if (present[1] == FL_BITTER)
				return (present[0] << 16) | PBLOCK_CLR_LITE_BLUE;
			if (present[1] == FL_SOUR)
				return (present[0] << 16) | PBLOCK_CLR_OLIVE;
		}
	}
	return PBLOCK_CLR_NONE;
}

void HackBlender_Calculate(const struct BlenderBerry* berries, struct Pokeblock* pokeblock, u8 numPlayers, u8* flavors, u16 maxRPM)
{
	s16 pf[BLENDER_FLAVORS];   //原文 sPokeblockFlavors
	s32 i, j;
	s32 multiuseVar;
	u8 numNegatives;

	for (i = 0; i < BLENDER_FLAVORS; i++)
		pf[i] = 0;

	//全員のきのみの味と なめらかさ を足す
	for (i = 0; i < numPlayers; i++)
	{
		for (j = 0; j < BLENDER_FLAVORS; j++)
			pf[j] += berries[i].flavors[j];
	}

	//味の合計を 1 つ前の味の合計から引く（順番に意味がある・原文のまま）
	multiuseVar = pf[0];
	pf[FL_SPICY] -= pf[FL_DRY];
	pf[FL_DRY] -= pf[FL_SWEET];
	pf[FL_SWEET] -= pf[FL_BITTER];
	pf[FL_BITTER] -= pf[FL_SOUR];
	pf[FL_SOUR] -= multiuseVar;

	//負になった味を数えて 0 にする
	multiuseVar = 0;
	for (i = 0; i < 5; i++)
	{
		if (pf[i] < 0)
		{
			pf[i] = 0;
			multiuseVar++;
		}
	}
	numNegatives = multiuseVar;

	//正の味から、負の味の数を引く（0 未満にはしない）
	for (i = 0; i < 5; i++)
	{
		if (pf[i] > 0)
		{
			if (pf[i] < multiuseVar)
				pf[i] = 0;
			else
				pf[i] -= multiuseVar;
		}
	}

	//最高の回転数で補正して四捨五入
	multiuseVar = maxRPM / 333 + 100;
	for (i = 0; i < 5; i++)
	{
		s32 remainder;
		s32 flavor = pf[i];

		flavor = (flavor * multiuseVar) / 10;
		remainder = flavor % 10;
		flavor /= 10;
		if (remainder > 4)
			flavor++;
		pf[i] = flavor;
	}

	//色と なめらかさ
	pokeblock->color = CalculatePokeblockColor(berries, pf, numPlayers, numNegatives);
	pf[FL_FEEL] = (pf[FL_FEEL] / numPlayers) - numPlayers;

	if (pf[FL_FEEL] < 0)
		pf[FL_FEEL] = 0;

	if (pokeblock->color == PBLOCK_CLR_BLACK)
	{
		//くろ いポロックは、味が無作為に決め直される
		multiuseVar = Random() % (sizeof(sBlackPokeblockFlavorFlags) / sizeof(sBlackPokeblockFlavorFlags[0]));
		for (i = 0; i < 5; i++)
		{
			if ((sBlackPokeblockFlavorFlags[multiuseVar] >> i) & 1)
				pf[i] = 2;
			else
				pf[i] = 0;
		}
	}

	for (i = 0; i < BLENDER_FLAVORS; i++)
	{
		if (pf[i] > 255)
			pf[i] = 255;
	}

	pokeblock->spicy = pf[FL_SPICY];
	pokeblock->dry = pf[FL_DRY];
	pokeblock->sweet = pf[FL_SWEET];
	pokeblock->bitter = pf[FL_BITTER];
	pokeblock->sour = pf[FL_SOUR];
	pokeblock->feel = pf[FL_FEEL];

	for (i = 0; i < BLENDER_FLAVORS; i++)
		flavors[i] = pf[i];
}

u16 HackBlender_SkipMaxRPM(u8 players, bool8 master)
{
	if (master && players >= 2 && players <= 4)
		return sSkipMaxRPMMaster[players - 2];
	if (players < 2)
		players = 2;
	if (players > 4)
		players = 4;
	return sSkipMaxRPM[players - 2];
}

void HackBlender_TryUpdateRecord(u8 players, u16 maxRPM)
{
	u8 i;

	if (players < 2 || players > 4)
		return;
	i = players - 2;
	if (Pokeblock_GetRecord(i) < maxRPM)
		Pokeblock_SetRecord(i, maxRPM);
}

bool8 HackBlender_CaseHasRoom(void)
{
	return Pokeblock_Usable() == POKEBLOCK_OK && Pokeblock_Count() < POKEBLOCK_CASE_SLOTS;
}

bool8 HackBlender_StoreAndConsume(const struct Pokeblock* pokeblock, u16 playerItemId)
{
	struct PokeblockImage* image;
	u8 slot = 0;
	bool8 ok = FALSE;

	if (!HackBlender_CaseHasRoom())
		return FALSE;
	image = AllocZeroed(sizeof(struct PokeblockImage));
	if (image == NULL)
		return FALSE;
	if (Pokeblock_Load(image) && Pokeblock_Add(image, pokeblock, &slot))
	{
		if (RemoveBagItem(playerItemId, 1))
			ok = TRUE;
		else
			Pokeblock_Remove(image, slot);   //きのみを外せなかった＝足したポロックを戻す（戻せなければそのまま＝まれ）
	}
	Free(image);
	return ok;
}
