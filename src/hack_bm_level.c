#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"

/*
 * 戦闘中のレベルの上位 8 ビット（[hack] patches/0796・liru 氏の 0679 の levelHi の置き場所の直し）
 *
 * 0679 は BattlePokemon の +0x3A（素の nickname[11] の 11 バイト目）を levelHi にしたが、素の機械語は戦闘の始めに
 * スタックの struct BattlePokemon を初期化しないまま 0x58 バイト丸ごと gBattleMons へ写す（REQUEST_ALL_BATTLE→switching.c）ので、
 * +0x3A にスタックのゴミが入り BM_LEVEL が 3333 などになって 1 発で倒れる・経験値が異常になる不具合だった（2026-10-24）。
 * 上位は素の機械語が知らない gNewBS（戦闘の始めに 0 埋め）に置く。MAX_LEVEL ≦ 254 のあいだは常に 0。
 * gBattleMons の外の BattlePokemon（写し）・gNewBS が NULL のときは上位 0・書かない。
 */

static bool8 HackBmBank(const struct BattlePokemon* b, u8* bank)
{
	u32 d = (u32) b - (u32) gBattleMons;
	if (d >= sizeof(struct BattlePokemon) * MAX_BATTLERS_COUNT || d % sizeof(struct BattlePokemon) != 0 || gNewBS == NULL)
		return FALSE;
	*bank = (u8) (d / sizeof(struct BattlePokemon));
	return TRUE;
}

u16 HackBmGetLevel(const struct BattlePokemon* b)
{
	u8 bank;
	u16 hi = 0;
	if (HackBmBank(b, &bank))
		hi = gNewBS->hackLevelHi[bank];
	return (u16) (b->levelLo | (hi << 8));
}

void HackBmSetLevel(struct BattlePokemon* b, u16 level)
{
	u8 bank;
	b->levelLo = (u8) (level & 0xFF);
	if (HackBmBank(b, &bank))
		gNewBS->hackLevelHi[bank] = (u8) ((level >> 8) & 0xFF);
}
