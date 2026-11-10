#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "hack_kind.h"
#include "hack_delta.h"
#include "../include/new/terastal.h"

/*
 * ひこうタイプ扱い（[hack] patches/0962 の追加）: 元の種族のタイプにひこうがあるデルタ種は、「地面に着いているか」と「じめん技の無効」だけ
 * ひこうタイプと同じに扱う（弱点・耐性・タイプ一致はデルタのタイプのまま）。窓口は HackDelta_CoversFlying（印とタイプの判定・src/hack_delta.c）。
 */
bool8 HackDelta_MonFlyingCover(const struct Pokemon* mon, u16 species)
{
	if (mon == NULL || species == SPECIES_NONE || species >= NUM_SPECIES)
		return FALSE;
	return HackDelta_CoversFlying(mon, gBaseStats[species].type1, gBaseStats[species].type2);
}

bool8 HackDelta_BankFlyingCover(u8 bank)
{
	const struct Pokemon* mon;
	u16 species;

	if (bank >= gBattlersCount
	|| (gBattleTypeFlags & BATTLE_TYPE_CAMOMONS)
	|| (gNewBS != NULL && (gNewBS->hackDeltaRoost & gBitTable[bank]))   //はねやすめ中はひこうタイプが外れる（ひこうタイプの SetRoostFunc と同じ）
	|| (gBattleMons[bank].status2 & STATUS2_TRANSFORMED)                  //へんしんしていれば相手のタイプそのまま（その中にひこうがあれば IsOfType が見る）
	|| IsTerastal(bank))                                                  //テラスタルしていればテラスタイプ
		return FALSE;
	mon = GetBankPartyData(bank);
	species = gBattleMons[bank].species;
	return HackDelta_MonFlyingCover(mon, species);
}
