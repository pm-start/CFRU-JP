#include "defines.h"
#include "../include/pokemon.h"
#include "hack_stat_pool.h"

/*
 * 上の桁の表の更新（値を書いたあと）。影（今の欄）を 12 個集めて HackStatPool_Note* へ渡す。遅い道（入れ物を超える値か表が空でないときだけ）。
 * [hack] patches/0760。窓口 hack_stat_access.h の HackMon_SetEv／SetIv／HackBoxMon_SetEv／SetIv から。
 */

void HackMon_PoolNote(struct Pokemon* mon, u8 isIv, u8 stat, u16 value)
{
	u8 ev[STATPOOL_STATS], iv[STATPOOL_STATS], i;

	for (i = 0; i < STATPOOL_STATS; ++i)
	{
		ev[i] = GetMonData(mon, MON_DATA_HP_EV + i, NULL);
		iv[i] = GetMonData(mon, MON_DATA_HP_IV + i, NULL);
	}
	if (isIv)
		HackStatPool_NoteIv(mon->personality, (u16) mon->otid, stat, value, ev, iv);
	else
		HackStatPool_NoteEv(mon->personality, (u16) mon->otid, stat, value, ev, iv);
}

void HackBoxMon_PoolNote(struct BoxPokemon* mon, u8 isIv, u8 stat, u16 value)
{
	u8 ev[STATPOOL_STATS], iv[STATPOOL_STATS], i;

	for (i = 0; i < STATPOOL_STATS; ++i)
	{
		ev[i] = GetBoxMonData(mon, MON_DATA_HP_EV + i, NULL);
		iv[i] = GetBoxMonData(mon, MON_DATA_HP_IV + i, NULL);
	}
	if (isIv)
		HackStatPool_NoteIv(mon->personality, (u16) mon->otId, stat, value, ev, iv);
	else
		HackStatPool_NoteEv(mon->personality, (u16) mon->otId, stat, value, ev, iv);
}
