#ifndef GUARD_HACK_BADGE_REPLAY_H
#define GUARD_HACK_BADGE_REPLAY_H

#include "defines.h"

u8 HackBadgeReplay_Count(u8 region);
u16 HackBadgeReplay_Trainer(u8 region, u8 index);
bool8 HackBadgeReplay_Available(u8 region, u8 index);
u8 HackBadgeReplay_Next(u8 region, u8 index, s8 direction);
const u8* HackBadgeReplay_Name(u8 region, u8 index);
const u8* HackBadgeReplay_BattleScript(u8 region, u8 index);
void HackBadgeReplay_Reset(u8 region);
void HackBadgeReplay_OnBattleEnd(void);

#endif
