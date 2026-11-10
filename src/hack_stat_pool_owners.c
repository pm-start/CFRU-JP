#include "defines.h"
#include "../include/pokemon.h"
#include "hack_stat_pool.h"

/*
 * 上の桁の表の「持ち主」の列挙（[hack] patches/0762・設計 docs/research/2026-10-24_party-stat16-save.md 4 章・6 章 4）。
 * 80 バイト形・100 バイト形が住む所の性格値（PID）を集める。掃除（HackStatPool_Sweep）が「この中に無い件」を落とす（patches/0799: 性格値と親 ID の下位 16 ビットの組・旧件は性格値だけで残す）。
 *   RAM の手持ち 6・SaveBlock1 の手持ち 6・safeBackupParty 6・育て屋（いまの 2 匹＋控え 3 地方の各 2 匹）・5 ばんどうろの預かり 1・
 *   合体ポケモン（SaveBlock1 の 4 匹＋SaveBlock2 の 2 匹）。
 * 数えない（件を持たない形）: 箱の X 形・卵ボックス・gTempTeamBackup（X 形は 16 ビットを自分で持つ）。
 * PSS の持ち上げ中の 1 匹（gPSSData->movingMon）は gPSSData が使える間だけ確かめられないので数えない＝掃除は満杯のときだけなので実害は小さい（推測）。
 * 列挙漏れは「満杯のときだけ」件が早く消える（影＝頭打ちで安全側）。
 */

struct DayCare* HackDaycare_GetStashed(u8 i);   //src/hack_daycare_region.c

static void Add(struct StatPoolOwner* out, u8* n, u8 max, u32 pid, u32 otId)
{
	u8 i;

	if (pid == 0 || *n >= max)
		return;
	for (i = 0; i < *n; ++i)
		if (out[i].pid == pid && out[i].ot == (u16) otId)
			return;
	out[*n].pid = pid;
	out[*n].ot = (u16) otId;
	++*n;
}

static void AddDaycare(struct StatPoolOwner* out, u8* n, u8 max, const struct DayCare* d)
{
	u8 i;

	if (d == NULL)
		return;
	for (i = 0; i < DAYCARE_MON_COUNT; ++i)
		Add(out, n, max, d->mons[i].mon.personality, d->mons[i].mon.otId);
}

u8 HackStatPool_CollectOwners(struct StatPoolOwner* out, u8 max)
{
	u8 n = 0, i;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		Add(out, &n, max, gPlayerParty[i].personality, gPlayerParty[i].otid);
		Add(out, &n, max, gSaveBlock1->playerParty[i].personality, gSaveBlock1->playerParty[i].otid);
		Add(out, &n, max, gSaveBlock1->safeBackupParty[i].personality, gSaveBlock1->safeBackupParty[i].otId);
	}
	AddDaycare(out, &n, max, &gSaveBlock1->daycare);
	for (i = 0; i < 3; ++i)
		AddDaycare(out, &n, max, HackDaycare_GetStashed(i));
	Add(out, &n, max, gSaveBlock1->route5DayCareMon.mon.personality, gSaveBlock1->route5DayCareMon.mon.otId);
	Add(out, &n, max, gSaveBlock1->fusedReshiram.personality, gSaveBlock1->fusedReshiram.otid);
	Add(out, &n, max, gSaveBlock1->fusedZekrom.personality, gSaveBlock1->fusedZekrom.otid);
	Add(out, &n, max, gSaveBlock1->fusedSolgaleo.personality, gSaveBlock1->fusedSolgaleo.otid);
	Add(out, &n, max, gSaveBlock1->fusedLunala.personality, gSaveBlock1->fusedLunala.otid);
	Add(out, &n, max, gSaveBlock2->fusedSpectrier.personality, gSaveBlock2->fusedSpectrier.otid);
	Add(out, &n, max, gSaveBlock2->fusedGlastrier.personality, gSaveBlock2->fusedGlastrier.otid);
	return n;
}
