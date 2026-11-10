#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "hack_stat_caps.h"
#include "hack_kind.h"
#include "hack_delta.h"
#include "hack_nushi_npc.h"

/*
 * ヌシにしてくれる人（2026-11-09・FEAT-nushi-npc・patches/0982・ユーザー「1度だけヌシポケモンにしてくれるNPCの追加／条件lv200」）。
 * 手持ちのレベル 200（HACK_MAX_LEVEL）の 1 匹を、セーブにつき 1 度だけ（旗 HACK_FLAG_NUSHI_NPC＝0x13B8）ヌシにする。
 * 印の付け方はダンバルの贈り物（hack_beldum_gift.c）と同じ: HackKind_Set(NUSHI, 0) → CalculateMonStats。
 * 断る子: たまご・レベル不足・もうヌシ・ダーク・デルタ（種類は 1 つだけ＝上書きしない）・手持ちに別のヌシがいる。
 * ★ 「手持ちにヌシは 1 匹だけ」の判定は hack_delta.c の HackNushi_PartyHasNushi（FEAT-nushi-1-1・patches/0981）を使う（まとめ 0.66.0 で、この枝の static の写しを捨てた）。
 */

#define HACK_FLAG_NUSHI_NPC 0x13B8   //build_hoenn_scripts.RESERVED_FLAGS・台帳 docs/research/flag-ledger.md

void HackNushiNpc_Check(void)
{
	u16 slot = Var8004;
	struct Pokemon* mon;
	u8 kind;

	if (slot >= PARTY_SIZE || gPlayerParty[slot].species == SPECIES_NONE)
	{
		gSpecialVar_LastResult = NUSHI_NPC_BAD_SLOT;
		return;
	}
	mon = &gPlayerParty[slot];
	kind = HackKind_Get((const struct BoxPokemon*) mon);
	if (GetMonData(mon, MON_DATA_IS_EGG, NULL))
		gSpecialVar_LastResult = NUSHI_NPC_EGG;
	else if (GetMonData(mon, MON_DATA_LEVEL, NULL) < HACK_MAX_LEVEL)
		gSpecialVar_LastResult = NUSHI_NPC_LOW_LEVEL;
	else if (kind == HACK_KIND_NUSHI)
		gSpecialVar_LastResult = NUSHI_NPC_ALREADY;
	else if (kind == HACK_KIND_DARK)
		gSpecialVar_LastResult = NUSHI_NPC_DARK;
	else if (kind == HACK_KIND_DELTA)
		gSpecialVar_LastResult = NUSHI_NPC_DELTA;
	else if (HackNushi_PartyHasNushi(gPlayerParty, (u8) slot))
		gSpecialVar_LastResult = NUSHI_NPC_OTHER;
	else
		gSpecialVar_LastResult = NUSHI_NPC_OK;
}

void HackNushiNpc_Apply(void)
{
	u16 slot = Var8004;

	//台本が Check を通してから呼ぶが、念のため同じ確かめをもう一度（通らなければ何もしない・旗も立てない）
	HackNushiNpc_Check();
	if (gSpecialVar_LastResult != NUSHI_NPC_OK)
		return;
	HackKind_Set((struct BoxPokemon*) &gPlayerParty[slot], HACK_KIND_NUSHI, 0);   //ヌシは値を使わない（hack_kind_roll.c の Mark と同じ）
	CalculateMonStats(&gPlayerParty[slot]);                                       //ヌシの能力の反映（印を付けたあとに計算し直す）
	FlagSet(HACK_FLAG_NUSHI_NPC);
}
