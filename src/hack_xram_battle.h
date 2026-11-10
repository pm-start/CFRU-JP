#ifndef GUARD_HACK_XRAM_BATTLE_H
#define GUARD_HACK_XRAM_BATTLE_H

struct NewBattleStruct;

struct NewBattleStruct* HackXramBattle_Alloc(void);
void HackXramBattle_Free(struct NewBattleStruct* state);
void* HackXramBattle_TempTeam(void);

#endif
