#pragma once

#include "hack_stat_caps.h"
#include "hack_stat_pool.h"

/*
 * 努力値・個体値の読み書きの窓口（u16・[hack] patches/0677・設計 docs/research/2026-10-23_stat-level-16bit.md 2.1 章）
 *
 *   ★ patches/0760〜: 入れ物（u8／5 ビット）を超える値は「上の桁の表」（hack_stat_pool.c・性格値が鍵）に持つ。影（入れ物の中）は頭打ち。
 *   読むとき: 影が入れ物の最大ちょうどのときだけ表を引く（表が空なら引かない）。書くとき: 影＝頭打ち・上限（HACK_*_CAP）を超えた値は上限に丸める・入れ物を超えたら表へ。
 *   一時コピー（施設の貸し出し・難易度・ダミー戦・敵）は ...Shadow（影だけ書く＝同じ性格値の本物の表の件を書き換えない）。
 *   （前の方針）手持ち（struct Pokemon 100 バイト）・80 バイト形（struct BoxPokemon）は太らせない（SaveBlock1 がぴったり＝セーブの形は 1 バイトも変えない）。
 *   だから保存の中身は今のまま（努力値 u8・個体値 5 ビット）で、窓口の戻り・引数の型だけを u16 にする。
 *   上限は hack_stat_caps.h（stat_caps.json の生成物）。上限を保存の入れ物より上げるとビルドが止まる。
 *   書くときは 上限（HACK_EV_PER_STAT_CAP・HACK_IV_MAX）で頭打ちしてから入れ物に入れる＝あふれて切り捨てられない。
 *   stat は STAT_HP（0）〜STAT_SPDEF（5）。MON_DATA_HP_EV〜SPDEF_EV（26〜31）・MON_DATA_HP_IV〜SPDEF_IV（39〜44）の並びと同じ。
 */

void HackMon_PoolNote(struct Pokemon* mon, u8 isIv, u8 stat, u16 value);                 //src/hack_stat_pool_mon.c（影を集めて表を更新）
void HackBoxMon_PoolNote(struct BoxPokemon* mon, u8 isIv, u8 stat, u16 value);

static inline u16 HackMon_GetEv(struct Pokemon* mon, u8 stat)
{
	u8 shadow = GetMonData(mon, MON_DATA_HP_EV + stat, NULL);

	return (shadow < STATPOOL_EV_WIDTH) ? shadow : HackStatPool_ResolveEv(mon->personality, (u16) mon->otid, stat, shadow);
}

static inline void HackMon_SetEvShadow(struct Pokemon* mon, u8 stat, u16 value)   //影だけ（一時コピー用）
{
	u8 field = (value > HACK_EV_PER_STAT_CAP) ? HACK_EV_PER_STAT_CAP : (u8) value;   //入れ物（u8）はキャップ以上なら切り捨てにならない。キャップが入れ物を超える日は下の SetEv が表に持つ
	if (value > STATPOOL_EV_WIDTH)
		field = STATPOOL_EV_WIDTH;
	SetMonData(mon, MON_DATA_HP_EV + stat, &field);
}

static inline void HackMon_SetEv(struct Pokemon* mon, u8 stat, u16 value)
{
	u16 v = (value > HACK_EV_PER_STAT_CAP) ? HACK_EV_PER_STAT_CAP : value;
	u8 field = (v > STATPOOL_EV_WIDTH) ? STATPOOL_EV_WIDTH : (u8) v;

	SetMonData(mon, MON_DATA_HP_EV + stat, &field);
	if (v > STATPOOL_EV_WIDTH || HackStatPool_NotEmpty())
		HackMon_PoolNote(mon, 0, stat, v);
}

static inline u16 HackMon_GetIv(struct Pokemon* mon, u8 stat)
{
	u8 shadow = GetMonData(mon, MON_DATA_HP_IV + stat, NULL);

	return (shadow < STATPOOL_IV_WIDTH) ? shadow : HackStatPool_ResolveIv(mon->personality, (u16) mon->otid, stat, shadow);
}

static inline void HackMon_SetIvShadow(struct Pokemon* mon, u8 stat, u16 value)
{
	u8 field = (value > HACK_IV_MAX) ? HACK_IV_MAX : (u8) value;

	if (value > STATPOOL_IV_WIDTH)
		field = STATPOOL_IV_WIDTH;
	SetMonData(mon, MON_DATA_HP_IV + stat, &field);
}

static inline void HackMon_SetIv(struct Pokemon* mon, u8 stat, u16 value)
{
	u16 v = (value > HACK_IV_MAX) ? HACK_IV_MAX : value;
	u8 field = (v > STATPOOL_IV_WIDTH) ? STATPOOL_IV_WIDTH : (u8) v;

	SetMonData(mon, MON_DATA_HP_IV + stat, &field);
	if (v > STATPOOL_IV_WIDTH || HackStatPool_NotEmpty())
		HackMon_PoolNote(mon, 1, stat, v);
}

//80 バイト形（育て屋の親・safeBackupParty・箱から出した直後など）
static inline u16 HackBoxMon_GetEv(struct BoxPokemon* mon, u8 stat)
{
	u8 shadow = GetBoxMonData(mon, MON_DATA_HP_EV + stat, NULL);

	return (shadow < STATPOOL_EV_WIDTH) ? shadow : HackStatPool_ResolveEv(mon->personality, (u16) mon->otId, stat, shadow);
}

static inline u16 HackBoxMon_GetIv(struct BoxPokemon* mon, u8 stat)
{
	u8 shadow = GetBoxMonData(mon, MON_DATA_HP_IV + stat, NULL);

	return (shadow < STATPOOL_IV_WIDTH) ? shadow : HackStatPool_ResolveIv(mon->personality, (u16) mon->otId, stat, shadow);
}

static inline void HackBoxMon_SetEv(struct BoxPokemon* mon, u8 stat, u16 value)
{
	u16 v = (value > HACK_EV_PER_STAT_CAP) ? HACK_EV_PER_STAT_CAP : value;
	u8 field = (v > STATPOOL_EV_WIDTH) ? STATPOOL_EV_WIDTH : (u8) v;

	SetBoxMonData(mon, MON_DATA_HP_EV + stat, &field);
	if (v > STATPOOL_EV_WIDTH || HackStatPool_NotEmpty())
		HackBoxMon_PoolNote(mon, 0, stat, v);
}

static inline void HackBoxMon_SetIv(struct BoxPokemon* mon, u8 stat, u16 value)
{
	u16 v = (value > HACK_IV_MAX) ? HACK_IV_MAX : value;
	u8 field = (v > STATPOOL_IV_WIDTH) ? STATPOOL_IV_WIDTH : (u8) v;

	SetBoxMonData(mon, MON_DATA_HP_IV + stat, &field);
	if (v > STATPOOL_IV_WIDTH || HackStatPool_NotEmpty())
		HackBoxMon_PoolNote(mon, 1, stat, v);
}
