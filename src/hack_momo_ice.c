#include "defines.h"
#include "hack_delta.h" //[hack] patches/0962
#include "hack_base_stats_wide.h" //[hack] patches/0678 種族値の u16 の窓口
#include "hack_stat_calc.h" //[hack] patches/0678
#include "../include/main.h" //gMain.vblankCounter2（2026-11-03 perf-region-2）
#include "../include/pokemon.h"
#include "../include/constants/items.h"
#include "../include/constants/species.h"
#include "hack_iv_over100.h" //[hack] patches/0591 #128
#include "hack_kind.h" //[hack] patches/0964 ダークは交換に出せない

/*
hack_momo_ice.c
    ももアイス（ITEM_MOMO_ICE 0x343・ITEM_EFFECT_MOMO_ICE・2026-10-05・patches/0175）

    ユーザー「アイスはとりあえずもたせると全能力2倍体力も状態異常無効、能力が下がらない相手からの技や自分の技でも
    毎ターン1/3回復、泥棒はたき落とす系を無効、トリックも」→ 体力も能力の値そのものを 2 倍（画面にも出る）・ふしぎなおくりもの。

    ・能力 2 倍: CalculateMonStatsNew（src/build_pokemon.c）が持っている間だけ HP と 5 つの能力を 2 倍にする（ヌケニンの HP は 1 のまま）
    ・持たせた／外したときに能力を計算し直す: 素の SetBoxMonData の中（持ち物の欄）では計算し直せない（中身を戻している途中）。
      → フィールドの毎フレーム（src/overworld_speedup.c）とステータス画面を開くとき、手持ちの最大 HP が
        「持っているかどうか」に合っていなければ計算し直す（状態は持たない・6.13）
    ・状態異常・能力ダウン・毎ターン回復・うばわれない は戦いの処理（src/battle_util.c・stat_buffs.c・item_battle_effects.c）
*/

bool8 HackMomoIce_Held(struct Pokemon* mon)
{
	return mon->item == ITEM_MOMO_ICE; //CFRU の手持ちは暗号化されていない（include/pokemon.h）。GetMonData を通すと重い
}

//★ 2026-10-11 ユーザー「難易度ももアイスの場合に ももアイスの下位互換アイテムの実装 絶対に入手不可 相手（トレーナー）のポケモンが所持するように」:
//  ももシャーベット（ITEM_MOMO_SHERBET・ITEM_EFFECT_MOMO_SHERBET）。HP と 5 つの能力 1.5 倍・毎ターン 1/6 回復（item_battle_effects.c）・
//  うばわれない（battle_util.c の CanTransferItem）。状態異常・こんらん・能力ダウン・メガ／ゲンシの代わりは ももアイス だけ（下位版には無い）。
//  持たせるのは src/hack_difficulty.c（なんいど ももアイス のトレーナー戦の相手・メガストーン／ゲンシの玉／Z クリスタルの子は持ち物を変えない）。
//  店・贈り物・ふしぎなおくりもの・ミラクル交換のどこにも出さない＝手に入らない（verify_momo_sherbet.py が見る）
bool8 HackMomoSherbet_Held(struct Pokemon* mon)
{
	return mon->item == ITEM_MOMO_SHERBET;
}

static u32 MomoMultiply(struct Pokemon* mon, u32 value)
{
	if (HackMomoIce_Held(mon))
		return value * 2;
	if (HackMomoSherbet_Held(mon))
		return value * 3 / 2;
	return value;
}

//CalculateMonStatsNew の最大 HP を出した直後（持っていれば 2 倍）
u16 HackMomoIce_MaxHP(struct Pokemon* mon, u16 species, u16 maxHP)
{
	#ifdef SPECIES_SHEDINJA
	if (species == SPECIES_SHEDINJA)
		return maxHP;
	#endif
	if (!HackMomoIce_Held(mon) && !HackMomoSherbet_Held(mon))
		return maxHP;
	{
		u32 v = MomoMultiply(mon, maxHP);
		return (v > 0xFFFF) ? 0xFFFF : v;
	}
}

//CalculateMonStatsNew の 5 つの能力を出した直後
void HackMomoIce_Stats(struct Pokemon* mon)
{
	u32 i;

	if (!HackMomoIce_Held(mon) && !HackMomoSherbet_Held(mon))
		return;
	for (i = MON_DATA_ATK; i <= MON_DATA_SPDEF; ++i)
	{
		u32 stat = MomoMultiply(mon, GetMonData(mon, i, NULL));
		if (stat > 0xFFFF)
			stat = 0xFFFF;
		SetMonData(mon, i, &stat);
	}
}

//計算し直さずに、持ち物と最大 HP が合っているか（素の式・ハイパートレーニングの個体値）
//★ 2026-10-07 欄を直接読む（CFRU の手持ちは暗号化されていない）。前は GetMonData を 1 匹 8 回ほど呼んでいて、
//  フィールドの毎コマで約 44,500 サイクル（1 コマの 16%）を使い、ホウエンの重い曲と重なって処理落ちした（デモ版の報告「処理落ち」）
static bool8 MaxHPMatchesItem(struct Pokemon* mon)
{
	u16 species = mon->species;
	u32 iv, level, maxHP;

	if (species == SPECIES_NONE || mon->isEgg)
		return TRUE;
	#ifdef SPECIES_SHEDINJA
	if (species == SPECIES_SHEDINJA)
		return TRUE;
	#endif
	iv = (mon->hyperTraining & gBitTable[STAT_HP]) ? HACK_IV_MAX : mon->hpIV; //[hack] patches/0677
	level = mon->level;
	iv = HackIv_Effective(iv, level); //[hack] patches/0591 #128 CalculateMonStatsNew と同じ実効の個体値（違うと毎コマ計算し直しになる）
	maxHP = HackStat_MaxHp(2 * HackNushi_Base(mon, HackBaseStat(species, STAT_HP)), iv, mon->hpEv, level); //[hack] patches/0678 CalculateMonStatsNew と同じ式・patches/0962 ヌシは HP の種族値も ×11/10（外すと毎コマ計算し直しになる）
	return mon->maxHP == HackMomoIce_MaxHP(mon, species, maxHP);
}

//フィールドの毎フレーム・ステータス画面を開くとき
void HackMomoIce_RefreshParty(void)
{
	u32 i;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];

		if (mon->species == SPECIES_NONE)
			continue;
		if (!MaxHPMatchesItem(mon))
			CalculateMonStats(mon);
	}
}

//フィールドの毎コマ（overworld_speedup.c）: 1 コマに 1 匹だけ見る（2026-11-03 perf-region-2）。全員を毎コマ見ると約 640 サイクル（1 匹 約 100）・
//1 匹なら約 110。手持ちは 8 コマで一巡する（GetMonData ではなく欄を直接読む MaxHPMatchesItem なので 1 匹の判定は RefreshParty と同じ）。
//ももアイスを持たせた／外したのはバッグ・手持ちの画面で、野外に戻ってから反映まで最大 8 コマ（0.13 秒）。ステータス画面は
//今までどおり HackMomoIce_RefreshParty で全員（bw_summary_screen.c）
void HackMomoIce_RefreshPartyTick(void)
{
	u32 counter = gMain.vblankCounter2;
	u32 slot = (counter + (counter >> 3)) & 7;   //割り算（ソフトの除算）を避ける。処理落ちで呼びが 2 コマおき・4 コマおきでも かたよらない（8 コマごとに 1 ずつずらす＝全部の枠に回る・状態は持たない＝6.13）・6 と 7 は休み
	struct Pokemon* mon;

	if (slot >= PARTY_SIZE)
		return;
	mon = &gPlayerParty[slot];
	if (mon->species == SPECIES_NONE)
		return;
	if (!MaxHPMatchesItem(mon))
		CalculateMonStats(mon);
}

/* ---- 交換に出せない（2026-10-09 ユーザー「ももアイスをトレード不可アイテムに」→「持たせた子を交換に出せない」・
 * 通信交換・ミラクル交換・ゲームの中の人との交換）---- */

//通信交換: 素の CanTradeSelectedMon（0804F1E4）の入口（assembly/hack_momo_ice/momo_ice_hooks.s）から。TRUE なら交換に出せない
bool8 HackMomoIce_TradeBlocked(struct Pokemon* party, u32 idx)
{
	return idx < PARTY_SIZE && (HackMomoIce_Held(&party[idx]) || HackKind_IsTradeRefused((struct BoxPokemon*) &party[idx]));   //[hack] patches/0964 ダークも出せない（段 I）
}

//ゲームの中の人との交換: 素の GetTradeSpecies（special 0xFF・080535F0）の置き換え（routinepointers 08163464）。
//ももアイスを持っていれば種族なし＝交換の人は「たのんだ ポケモンじゃない」の返事になる（人ごとの台本は書き換えない）
u16 HackMomoIce_GetTradeSpecies(void)
{
	struct Pokemon* mon = &gPlayerParty[*(u16*) 0x02036FF6];   //gSpecialVar_0x8005

	if (GetMonData(mon, MON_DATA_IS_EGG, NULL) || HackMomoIce_Held(mon) || HackKind_IsTradeRefused((struct BoxPokemon*) mon))   //[hack] patches/0964 ダークも出せない（段 I）
		return SPECIES_NONE;
	return GetMonData(mon, MON_DATA_SPECIES, NULL);
}
