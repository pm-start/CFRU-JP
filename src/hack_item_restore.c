#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/constants/hold_effects.h"
#include "../include/new/battle_util.h"
#include "../include/new/item.h"
#include "hack_item_restore.h"

/*
    使いきった持ち物（きのみ以外）を戦いのあとに戻す。説明は hack_item_restore.h・docs/research/2026-10-19_consumed-items-restore.md

    なぜ要るか: CFRU の RestoreNonConsumableItems（end_battle.c）は「消えない道具」だけ戻し、gConsumableItemEffects の道具（だっしゅつパック・じゃくてんほけん・
    ふうせん・レッドカード・のどスプレー・ハーブ…）は戻さない。しかも野生戦では何も戻さない。きあいのタスキは表に無いのでトレーナー戦で戻っていただけ。
*/

static bool8 CanRestoreHere(void)
{
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_POKE_DUDE | BATTLE_TYPE_OLD_MAN))
		return FALSE; //通信・施設・チュートリアルは今までどおり（フロンティアは end_battle.c が全部戻す・バトルチューブは戻さない決まり）
	#ifdef FLAG_BATTLE_FACILITY
	if (FlagGet(FLAG_BATTLE_FACILITY))
		return FALSE;
	#endif
	return TRUE;
}

bool8 HackItemRestore_IsRestorable(u16 item)
{
	u8 effect;

	if (item == ITEM_NONE || IsBerry(item))
		return FALSE; //きのみは今までどおり（使ったらなくなる）

	effect = ItemId_GetHoldEffect(item);
	if (effect == ITEM_EFFECT_DANGER_LEAF || effect == ITEM_EFFECT_SECOND_ARROW)
		return FALSE; //あぶないはっぱ・にのやは「使ったら戦いのあとも戻らない」決まり（ユーザー 2026-10-09／10）
	return TRUE;
}

static u8 FindBackupIndex(u32 personality)
{
	u8 i;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (gNewBS->itemBackupPid[i] == personality)
			return i;
	}
	return PARTY_SIZE;
}

void HackItemRestore_Save(void)
{
	u8 i;

	gNewBS->consumedItemMask = 0;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) == SPECIES_NONE)
			gNewBS->itemBackupPid[i] = 0; //空きの枠は PID 0（探さない）
		else
			gNewBS->itemBackupPid[i] = GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY, NULL);
	}
}

void HackItemRestore_NoteConsumed(u8 bank, u16 item)
{
	u32 personality;
	u8 index;

	if (gNewBS == NULL || SIDE(bank) != B_SIDE_PLAYER || !HackItemRestore_IsRestorable(item))
		return;

	personality = GetMonData(GetBankPartyData(bank), MON_DATA_PERSONALITY, NULL);
	if (personality == 0)
		return;

	index = FindBackupIndex(personality);
	if (index < PARTY_SIZE && gNewBS->itemBackup[index] == item) //もとの持ち物が消えたときだけ（交換で受け取った物・拾った物は対象外）
		gNewBS->consumedItemMask |= gBitTable[index];
}

void HackItemRestore_Apply(void)
{
	u8 i, index;
	u32 personality;
	u16 item;

	if (gNewBS->consumedItemMask == 0 || !CanRestoreHere())
		return;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) == SPECIES_NONE)
			continue;

		personality = GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY, NULL);
		index = FindBackupIndex(personality);
		if (personality == 0 || index >= PARTY_SIZE || !(gNewBS->consumedItemMask & gBitTable[index]))
			continue;

		if (GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, NULL) != ITEM_NONE)
			continue; //戦いの中で別の物を拾った子は そのまま（重ねて 2 つにしない）

		item = gNewBS->itemBackup[index];
		SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &item);
		gNewBS->consumedItemMask &= ~gBitTable[index]; //1 回だけ
	}
}
