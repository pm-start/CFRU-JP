#include "defines.h"
#include "defines_battle.h"
#include "hack_difficulty.h"
#include "hack_wild_chain.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/sprite.h"
#include "../include/safari_zone.h"
#include "../include/field_player_avatar.h"
#include "../include/wild_encounter.h"
#include "../include/new/Vanilla_functions_battle.h"
#include "Tables/hoenn_script_flags.h"

/*
 * 野生のむれ（#110・2026-10-12・ユーザーの仕様）: なんいど別（表 sChainOneIn／sChainMaxExtra・0 は むれ なし）: むずかしい 1/8・最大 2 戦（追加 1）／ももアイス 1/4・最大 3 戦（追加 2）。
 * 草むら・洞窟の地面・なみのりの普通の野生の戦いに勝つと、抽選が当たれば同じ地図の表からすぐ次の野生の戦い。間に回復は無い。負け・逃げた・捕まえた・抽選が外れた・上限まで やった で終わり。
 * 対象外: サファリ・バトル施設・バトルチューブの野生の部屋・2 匹の野生・徘徊・スクリプトの野生（dowildbattle）・つり・いわくだき・あまいかおり・通信。
 *
 * 流れ: StandardWildEncounter が HackWildChain_MarkEligible（旗 0x4002）→ DoStandardWildBattle が旗を見て savedCallback を
 * HackWildChain_CB2_EndWildBattle に（旗は消す）→ 勝って抽選が当たれば 0x4003 を立て素の戻し方で野外へ → 野外の最初の空いたコマで
 * HackWildChain_FieldTick が次の 1 匹を作って戦いを始める（0x4002 を立てて同じ流れ）。
 * 旗は EWRAM の sSpecialFlags（0x4000〜）で、素の GetFlagPointer が扱う。セーブされない・つづきから で HackWildChain_Reset が消す。
 */

//なんいどごとの むれ（長さは明示・6.79）。OneIn: 0 = なし・N = 1/N。MaxExtra: ふえる回数の上限（2 ビットの数え 0〜3 に収まる）
static const u8 sChainOneIn[DIFFICULTY_COUNT] =
{
	[DIFFICULTY_HARD] = 8,
	[DIFFICULTY_MOMO_ICE] = 4,
	[DIFFICULTY_SHAKARIKI] = 4,   //しゃかりき（ももアイスと同じ）
};

static const u8 sChainMaxExtra[DIFFICULTY_COUNT] =
{
	[DIFFICULTY_HARD] = 1,
	[DIFFICULTY_MOMO_ICE] = 2,
	[DIFFICULTY_SHAKARIKI] = 2,
};

static u8 ChainDifficulty(void)
{
	u16 d = VarGet(HOENN_VAR_DIFFICULTY);

	return d < DIFFICULTY_COUNT ? d : DIFFICULTY_NORMAL;
}

static bool8 ChainEnabled(void)
{
	return sChainOneIn[ChainDifficulty()] != 0;
}

static u8 CountGet(void)
{
	return (FlagGet(HACK_FLAG_WILD_CHAIN_CNT0) ? 1 : 0) | (FlagGet(HACK_FLAG_WILD_CHAIN_CNT1) ? 2 : 0);
}

static void CountSet(u8 n)
{
	if (n & 1) FlagSet(HACK_FLAG_WILD_CHAIN_CNT0); else FlagClear(HACK_FLAG_WILD_CHAIN_CNT0);
	if (n & 2) FlagSet(HACK_FLAG_WILD_CHAIN_CNT1); else FlagClear(HACK_FLAG_WILD_CHAIN_CNT1);
}

u8 HackWildChain_Count(void)
{
	return CountGet();
}

void HackWildChain_Reset(void)
{
	FlagClear(HACK_FLAG_WILD_CHAIN_ELIGIBLE);
	FlagClear(HACK_FLAG_WILD_CHAIN_START_NEXT);
	CountSet(0);
}

//新しい出会い（StandardWildEncounter の普通の野生）の直前。対象外ならそのまま（旗は立てない）
void HackWildChain_MarkEligible(void)
{
	FlagClear(HACK_FLAG_WILD_CHAIN_ELIGIBLE);
	FlagClear(HACK_FLAG_WILD_CHAIN_START_NEXT);
	CountSet(0);
	if (!ChainEnabled())
		return;
	if (GetSafariZoneFlag() || FlagGet(FLAG_BATTLE_FACILITY) || HoennPike_InWildRoom() || FlagGet(FLAG_DOUBLE_WILD_BATTLE))
		return;
	FlagSet(HACK_FLAG_WILD_CHAIN_ELIGIBLE);
}

//src/hoenn_pike.c の HoennPike_CB2_EndWildBattle と同じ戻し方（勝ったときだけここ・それ以外は素の CB2_EndWildBattle）
void HackWildChain_CB2_EndWildBattle(void)
{
	u8 n = CountGet();
	u8 d = ChainDifficulty();

	if (gBattleOutcome == B_OUTCOME_WON && sChainOneIn[d] != 0 && n < sChainMaxExtra[d]
	&& (FlagGet(HACK_FLAG_WILD_CHAIN_FORCE) || Random() % sChainOneIn[d] == 0))
	{
		CountSet(n + 1);
		FlagSet(HACK_FLAG_WILD_CHAIN_START_NEXT);
		CpuFill16(0, (void*) BG_PLTT, BG_PLTT_SIZE);
		ResetOamRange(0, 128);
		SetMainCallback2(CB2_ReturnToField);
		return;
	}

	HackWildChain_Reset();
	CB2_EndWildBattle();   //素: 負けなら全滅・それ以外は野外へ
}

//野外の毎コマ（overworld_speedup.c）。次の戦いの始まり
void HackWildChain_FieldTick(void)
{
	bool8 water;

	if (!FlagGet(HACK_FLAG_WILD_CHAIN_START_NEXT))
		return;
	if (gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld)
		return;
	FlagClear(HACK_FLAG_WILD_CHAIN_START_NEXT);
	if (!ChainEnabled() || FlagGet(FLAG_BATTLE_FACILITY))
	{
		HackWildChain_Reset();
		return;
	}
	water = TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING) != 0;
	if (!HackWildChain_GenerateNext(water))
	{
		HackWildChain_Reset();
		return;
	}
	FlagSet(HACK_FLAG_WILD_CHAIN_ELIGIBLE);
	BattleSetup_StartWildBattle();
}
