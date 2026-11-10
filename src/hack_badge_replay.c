#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "Tables/hoenn_script_flags.h"
#include "Tables/hack_badge_reset.h"
#include "hack_badge_replay.h"

u16 HoennTrainerFlag(u16 trainerId);
extern const u8 gText_BwOpt_BadgeReplayDefeat[];

struct BadgeReplayEntry
{
	u16 trainerId;
	u16 doneFlag;
};

struct BadgeReplayScript
{
	u8 command;
	u8 type;
	u16 trainerId;
	u16 filler;
	const u8* defeat;
	u8 release;
	u8 end;
} __attribute__((packed));

_Static_assert(sizeof(struct BadgeReplayScript) == 12, "badge replay script must be 12 bytes");

#define REPLAY_SCRIPT(id, flag) {0x5C, 3, id, 0, gText_BwOpt_BadgeReplayDefeat, 0x6C, 0x02}

static const struct BadgeReplayEntry sHoenn[BADGE_REPLAY_HOENN_COUNT] = {BADGE_REPLAY_HOENN};
static const struct BadgeReplayEntry sSinnoh[BADGE_REPLAY_SINNOH_COUNT] = {BADGE_REPLAY_SINNOH};
static const struct BadgeReplayScript sHoennScripts[BADGE_REPLAY_HOENN_COUNT] = {BADGE_REPLAY_HOENN_SCRIPTS};
static const struct BadgeReplayScript sSinnohScripts[BADGE_REPLAY_SINNOH_COUNT] = {BADGE_REPLAY_SINNOH_SCRIPTS};

static const struct BadgeReplayEntry* Entries(u8 region)
{
	return region == 0 ? sHoenn : sSinnoh;
}

u8 HackBadgeReplay_Count(u8 region)
{
	return region == 0 ? BADGE_REPLAY_HOENN_COUNT : BADGE_REPLAY_SINNOH_COUNT;
}

u16 HackBadgeReplay_Trainer(u8 region, u8 index)
{
	if (index >= HackBadgeReplay_Count(region))
		return 0;
	return Entries(region)[index].trainerId;
}

bool8 HackBadgeReplay_Available(u8 region, u8 index)
{
	const struct BadgeReplayEntry* e;
	u16 resetFlag = region == 0 ? HACK_FLAG_BADGE_RESET_HOENN : HACK_FLAG_BADGE_RESET_SINNOH;
	if (!FlagGet(resetFlag) || index >= HackBadgeReplay_Count(region))
		return FALSE;
	e = &Entries(region)[index];
	return FlagGet(HoennTrainerFlag(e->trainerId)) && !FlagGet(e->doneFlag);
}

u8 HackBadgeReplay_Next(u8 region, u8 index, s8 direction)
{
	u8 count = HackBadgeReplay_Count(region);
	u8 i;
	if (count == 0)
		return 0xFF;
	for (i = 0; i < count; ++i)
	{
		index = direction < 0 ? (index == 0 ? count - 1 : index - 1)
		                      : (index + 1 == count ? 0 : index + 1);
		if (HackBadgeReplay_Available(region, index))
			return index;
	}
	return 0xFF;
}

const u8* HackBadgeReplay_Name(u8 region, u8 index)
{
	u16 trainer = HackBadgeReplay_Trainer(region, index);
	return trainer == 0 ? NULL : gTrainers[trainer].trainerName;
}

const u8* HackBadgeReplay_BattleScript(u8 region, u8 index)
{
	if (!HackBadgeReplay_Available(region, index))
		return NULL;
	return (const u8*)(region == 0 ? &sHoennScripts[index] : &sSinnohScripts[index]);
}

void HackBadgeReplay_Reset(u8 region)
{
	const struct BadgeReplayEntry* e = Entries(region);
	u32 i;
	for (i = 0; i < HackBadgeReplay_Count(region); ++i)
		FlagClear(e[i].doneFlag);
}

void HackBadgeReplay_OnBattleEnd(void)
{
	u8 region, i;
	if (gBattleOutcome != B_OUTCOME_WON || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
		return;
	for (region = 0; region < 2; ++region)
	{
		const struct BadgeReplayEntry* e = Entries(region);
		u16 resetFlag = region == 0 ? HACK_FLAG_BADGE_RESET_HOENN : HACK_FLAG_BADGE_RESET_SINNOH;
		if (!FlagGet(resetFlag))
			continue;
		for (i = 0; i < HackBadgeReplay_Count(region); ++i)
			if (e[i].trainerId == gTrainerBattleOpponent_A)
			{
				FlagSet(e[i].doneFlag);
				return;
			}
	}
}
