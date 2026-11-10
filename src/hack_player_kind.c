#include "defines.h"
#include "hack_player_kind.h"

/* 印の置き場は SaveBlock2 +0x90（hack_player_kind.h）。メイは土台の playerGender が女（1）のときだけ有効 */
static u8* PlayerKindPtr(void)
{
	return ((u8*) gSaveBlock2) + HACK_PLAYER_KIND_OFFSET;
}

u8 GetPlayerGender3(void)
{
	if (gSaveBlock2->playerGender != FEMALE)
		return PLAYER_GENDER3_MALE;
	return *PlayerKindPtr() == PLAYER_KIND_MEI ? PLAYER_GENDER3_MEI : PLAYER_GENDER3_FEMALE;
}

void HackPlayerKind_Set(u8 kind)
{
	*PlayerKindPtr() = kind;
}
