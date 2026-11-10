#pragma once
#include "../include/global.h"

/* 上限をこえた子を戦闘のあいだだけ上限のレベルにする（#124・src/hack_battle_squash.c） */
bool8 HackBadgeSquash_BattleAllowed(void);   /* 入れ替えてよい戦いか（通信・フロンティア・施設・パートナー戦などは対象外） */
u16 HackBadgeSquash_CapLevel(u16 level);       /* 手持ちのレベルを数える所（相手のレベルスケーリング）から: 上限をこえた分は上限として数える */
void HackBadgeSquash_OnBattleStart(void);    /* CreateBattleStartTask の入口（challenge_c_hooks.s） */
void HackBadgeSquash_Restore(void);          /* 戦闘のあと（end_battle.c の EndOfBattleThings）／フィールドの安全装置。印が無ければ何もしない */
bool8 HackBadgeSquash_Active(void);          /* 入れ替え中か（印が立っている） */
void HackBadgeSquash_FieldTick(void);        /* フィールドの毎コマ（HackChallengeC_FieldTick から） */