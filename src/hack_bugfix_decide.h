#ifndef GUARD_HACK_BUGFIX_DECIDE_H
#define GUARD_HACK_BUGFIX_DECIDE_H

#include <stdbool.h>

/* 道具を使った枠が土台（キュレム等）。選んだ枠が合体の相手。 */
static inline void Fuse_BaseAndPartner(unsigned itemSlot, unsigned pickedSlot, unsigned *baseSlot, unsigned *partnerSlot)
{
	*baseSlot = itemSlot;
	*partnerSlot = pickedSlot;
}

/* すでに合体している相手か。ネクロズマの相手がルナアーラならルナアーラの控えを見る。ゼクロムではない。 */
static inline bool Fuse_AlreadyPartner(unsigned base, unsigned partner,
	unsigned kyurem, unsigned reshiram, unsigned zekrom,
	unsigned necrozma, unsigned solgaleo, unsigned lunala,
	unsigned calyrex, unsigned glastrier, unsigned spectrier,
	bool reshiramFull, bool zekromFull, bool solgaleoFull, bool lunalaFull,
	bool glastrierFull, bool spectrierFull)
{
	if (base == kyurem && partner == reshiram)
		return reshiramFull;
	if (base == kyurem && partner == zekrom)
		return zekromFull;
	if (base == necrozma && partner == solgaleo)
		return solgaleoFull;
	if (base == necrozma && partner == lunala)
		return lunalaFull;
	if (base == calyrex && partner == spectrier)
		return spectrierFull;
	if (base == calyrex && partner == glastrier)
		return glastrierFull;
	return false;
}

static inline bool Fuse_SpeciesMatch(unsigned base, unsigned fuse,
	unsigned kyurem, unsigned reshiram, unsigned zekrom,
	unsigned necrozma, unsigned solgaleo, unsigned lunala,
	unsigned calyrex, unsigned glastrier, unsigned spectrier)
{
	if (base == kyurem && (fuse == reshiram || fuse == zekrom))
		return true;
	if (base == necrozma && (fuse == solgaleo || fuse == lunala))
		return true;
	if (base == calyrex && (fuse == glastrier || fuse == spectrier))
		return true;
	return false;
}

/* partySize を先に見る。満杯のとき speciesAt(partySize) は呼ばない。 */
static inline unsigned Fuse_EmptySlot(unsigned partySize, unsigned none, unsigned (*speciesAt)(unsigned slot))
{
	unsigned slotId;

	for (slotId = 0; slotId < partySize && speciesAt(slotId) != none; ++slotId)
		;
	return slotId;
}

/* 台本の野生は入れ替えない。草むらなどは randomWild を通す。 */
static inline unsigned Wild_SpeciesAfterChallengeC(unsigned species, bool scripted, unsigned (*randomWild)(unsigned))
{
	if (scripted)
		return species;
	return randomWild(species);
}

/* 伝説・幻・空き・タマゴは入れ替えない（RandomSpecies の先頭と同じ条件）。 */
static inline bool ChallengeC_KeepSpecies(unsigned species, unsigned none, unsigned egg, unsigned numSpecies, bool legend, bool debugMon, bool raidLegend)
{
	return species == none || species == egg || species >= numSpecies || legend || debugMon || raidLegend;
}

/* 1 = 入れ替わった種族、0 = 今の商品の名前、2 = 出さない。使ったあと swapped は消す。 */
static inline unsigned FlyingShop_TakeName(unsigned *swapped, unsigned idx, unsigned count)
{
	unsigned kind;

	if (*swapped != 0)
		kind = 1;
	else if (idx < count)
		kind = 0;
	else
		kind = 2;
	*swapped = 0;
	return kind;
}

/* 無い表は未完了にしない。ある表に未捕獲が残るときだけ未完了。 */
static inline bool DexNav_MapComplete(bool landOn, bool landOk, bool waterOn, bool waterOk, bool fishOn, bool fishOk)
{
	if (landOn && !landOk)
		return false;
	if (waterOn && !waterOk)
		return false;
	if (fishOn && !fishOk)
		return false;
	return true;
}

#endif
