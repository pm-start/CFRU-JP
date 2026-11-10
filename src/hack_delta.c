#include "defines.h"
#include "hack_kind.h"
#include "hack_delta.h"

/* デルタ種のタイプ（hack_delta.h に仕様）。読むのは 印の窓口（HackKind_*）と生成表だけ */

u8 HackDelta_SpeciesType(u16 species, u8 slot)
{
	if (species == SPECIES_NONE || species >= NUM_SPECIES)
		return TYPE_NORMAL;
	return gHackDeltaTypes[species][slot ? 1 : 0];
}

u8 HackDelta_Type(const struct Pokemon* mon, u16 species, u8 slot, u8 base)
{
	u8 v;

	if (mon == NULL || HackKind_Get((const struct BoxPokemon*) mon) != HACK_KIND_DELTA)
		return base;
	v = HackKind_Value((const struct BoxPokemon*) mon);
	if (v != 0 && v <= NUMBER_OF_MON_TYPES)
		return v - 1;
	return HackDelta_SpeciesType(species, slot);
}

bool8 HackDelta_CoversFlying(const struct Pokemon* mon, u8 baseType1, u8 baseType2)
{
	if (mon == NULL || HackKind_Get((const struct BoxPokemon*) mon) != HACK_KIND_DELTA)
		return FALSE;
	return baseType1 == TYPE_FLYING || baseType2 == TYPE_FLYING;
}

void HackDelta_Assign(struct BoxPokemon* mon, u8 typeOverride)
{
	HackKind_Set(mon, HACK_KIND_DELTA, typeOverride < NUMBER_OF_MON_TYPES ? typeOverride + 1 : 0);
}

u32 HackNushi_Base(const struct Pokemon* mon, u32 base)
{
	if (mon != NULL && HackKind_Get((const struct BoxPokemon*) mon) == HACK_KIND_NUSHI)
		return (base * HACK_NUSHI_MUL) / HACK_NUSHI_DIV; //[hack] patches/0980 11/10（0962 では 13/10）
	return base;
}

//手持ち（party の PARTY_SIZE 匹・exceptSlot 以外）にヌシがいれば TRUE。exceptSlot に PARTY_SIZE を渡すと全部を見る（patches/0981 ヌシは手持ちに 1 匹だけ）
bool8 HackNushi_PartyHasNushi(const struct Pokemon* party, u8 exceptSlot)
{
	u32 i;

	if (party == NULL)
		return FALSE;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (i != exceptSlot && party[i].species != SPECIES_NONE
		&& HackKind_Get((const struct BoxPokemon*) &party[i]) == HACK_KIND_NUSHI)
			return TRUE;
	}
	return FALSE;
}

//この子がヌシで、手持ち（exceptSlot 以外）にもうヌシがいるなら TRUE（2 匹目のヌシは手持ちに入れない）
bool8 HackNushi_PartyHasOther(const struct Pokemon* party, const struct Pokemon* mon, u8 exceptSlot)
{
	if (mon == NULL || HackKind_Get((const struct BoxPokemon*) mon) != HACK_KIND_NUSHI)
		return FALSE;
	return HackNushi_PartyHasNushi(party, exceptSlot);
}
