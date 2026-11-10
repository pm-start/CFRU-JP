#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/pokemon.h"
#include "../include/script.h"
#include "../include/constants/pokemon.h"
#include "../include/new/rom_locs.h"
#include "hack_battle_squash.h"
#include "hack_dummy_battle.h" //[hack] patches/0602 ポケセン 3F のダミー

/*
 * 上限をこえた子を「戦闘のあいだだけ」上限のレベルにする（#124 の続き・2026-10-17 ユーザー「建前上そのレベルに収める」）。docs/research/2026-10-17_badge-level-cap.md 5〜6 章
 *
 *  案 A（手持ちの入れ替え）: 戦闘の入口（CreateBattleStartTask の hook・assembly/hack_challenge/challenge_c_hooks.s）で、上限（HackChallengeB_LevelCap）より
 *  レベルが高い子だけ レベル・経験値を上限のものに書き、能力値を作り直す（HP は割合）。戦闘は 1 つの整合した手持ちで進む＝gBattleMons・体力バー・メガ・ダイマ・どうぐが素のまま。
 *  戦闘のあと（end_battle.c の EndOfBattleThings・RecalcAllStats のあと）で 元の経験値・レベルに戻し、能力値を作り直し、HP は割合で戻す。
 *  それ以外（状態異常・PP・持ち物・努力値・なつき度・ひんし）は 戦闘のあとの値のまま。
 *
 *  置き場: 追加の RAM（src/hack_xram.c・0x01000100〜・直した mGBA だけ・電源を切れば消える＝セーブに入らない・ステートには入る）。
 *    #123 ふういんが SaveBlock2 の予備の欄（filler_B 20）をほぼ使い切ったので、戦闘のあいだだけの値はセーブの外に置く（static は使えない 6.13・gNewBS は入口のあとに作られる）。
 *    追加の RAM が無い環境（実機・SuperCard・素の mGBA）では入れ替えない（経験値が入らないだけ・相手のスケーリングも上限で数えない）。
 *    +0 魔法の値（HACK_BSQ_MAGIC ＝ 入れ替え中・0 ＝ していない）・+4 数・+8 + 8n 子の性格値（u32）・+12 + 8n 元の経験値（u32）。元のレベルは経験値から出す。
 *  戻しの出口（全部 HackBadgeSquash_Restore を通る・印が立っているあいだだけ 1 回・二重に走っても何も起きない）:
 *    ① EndOfBattleThings（勝ち・負け・逃げる・捕獲・引き分け・テレポート・？？？戦の畳み＝戦闘のあとは全部 EndBattleFlagClearHook がここを通る）
 *    ② フィールドの毎コマの安全装置（HackChallengeC_FieldTick・戦闘が終わって野外に戻っているのに印が残っているとき）
 *    ③ 電源を切る／リセットしたときは 追加の RAM ごと消えるが、入れ替えた手持ちも RAM だけでセーブされていない（戦闘中はセーブできない）ので 元のセーブに戻る
 *  子は性格値で探す（戦闘中に並びが入れ替わる）。戦闘中の経験値は上限の子には入らない（HackChallengeB_ExpBlocked）ので 戦闘中にレベルが上がることはなく 進化の判定も起きない。
 */

#ifndef HACK_BSQ_BASE
#define HACK_BSQ_BASE     ((u8*) 0x01000100)
#endif
#define HACK_BSQ_MAGIC    0x31515342u   /* 'BSQ1' */
#define HACK_BSQ_SLOTS    PARTY_SIZE

u8 HackChallengeB_LevelCap(void);   //src/hack_challenge_b.c（場所の上限と縛り B の低いほう・0 = 上限なし）
bool8 HackXram_Available(void);     //src/hack_xram.c

static u32 ReadU32(const u8* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32) p[3] << 24);
}

static void WriteU32(u8* p, u32 v)
{
	p[0] = v;
	p[1] = v >> 8;
	p[2] = v >> 16;
	p[3] = v >> 24;
}

static bool8 Marked(void)   //追加の RAM があり、印が立っている
{
	return HackXram_Available() && ReadU32(HACK_BSQ_BASE) == HACK_BSQ_MAGIC;
}

//入れ替えてよい戦いか（追加の RAM が無い環境・通信・フロンティア・施設・トレーナーヒルなどレベルを決め打ちする戦い・パートナー/空中戦のように手持ちを組み替える戦いは対象外）
bool8 HackBadgeSquash_BattleAllowed(void)
{
	if (HackDummy_Active()) //[hack] patches/0602 ポケセン 3F のダミーとの練習は 自分の子を上限のレベルにしない（ほんとうの強さで戦う）
		return FALSE;
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_OAK_TUTORIAL
						  | BATTLE_TYPE_MOCK_BATTLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI | BATTLE_TYPE_TOWER_LINK_MULTI | BATTLE_TYPE_DYNAMAX | BATTLE_TYPE_SAFARI))
		return FALSE;
	if (FlagGet(FLAG_BATTLE_FACILITY))
		return FALSE;
	if (ExtensionState.partyBackup != NULL || ExtensionState.skyBattlePartyBackup != NULL)
		return FALSE;
	return HackXram_Available();
}

//相手のレベルスケーリングなど 手持ちのレベルを数える所から（build_pokemon.c）: 上限をこえた分は上限のレベルとして数える（戦闘に入るとその子は上限のレベルで戦う）
u16 HackBadgeSquash_CapLevel(u16 level)
{
	u8 cap;

	if (!HackBadgeSquash_BattleAllowed())
		return level;
	cap = HackChallengeB_LevelCap();
	return (cap != 0 && level > cap) ? cap : level;
}

//HP を新しい最大に割合で合わせる（四捨五入）。0 は 0（ひんし）のまま・1 以上は 1 以上・満タンは満タン
static u16 ScaleHp(u16 hp, u16 oldMax, u16 newMax)
{
	u32 v;

	if (hp == 0 || oldMax == 0)
		return hp == 0 ? 0 : newMax;
	if (hp >= oldMax)
		return newMax;
	v = ((u32) hp * newMax + (oldMax >> 1)) / oldMax;
	if (v == 0)
		v = 1;
	if (v > newMax)
		v = newMax;
	return v;
}

//レベルと経験値を書いて能力値を作り直し、HP は元の割合に合わせる（入れ替え・戻しの両方が使う）
static void SetLevelKeepHpRatio(struct Pokemon* mon, u16 level, u32 exp)
{
	u16 oldHp = GetMonData(mon, MON_DATA_HP, NULL);
	u16 oldMax = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	u16 newMax, newHp;

	SetMonData(mon, MON_DATA_EXP, &exp);
	SetMonData(mon, MON_DATA_LEVEL, &level);
	CalculateMonStats(mon);
	newMax = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	newHp = ScaleHp(oldHp, oldMax, newMax);
	SetMonData(mon, MON_DATA_HP, &newHp);
}

//CreateBattleStartTask の入口から（相手の手持ちを作る前＝レベルスケーリングが見る前）。印が立っていれば何もしない（二重に走らない）
void HackRamCheat_OnBattleStart(void);   //[hack] patches/0660 src/hack_ram_cheat.c
void HackBadgeSquash_OnBattleStart(void)
{
	u8* d = HACK_BSQ_BASE;
	u8 cap, n = 0;
	u32 i;

	HackRamCheat_OnBattleStart();   //[hack] patches/0660 RAM チート検出（戦闘の開始の機会・手持ちを上限のレベルに書き換える前に見る）

	if (!HackBadgeSquash_BattleAllowed() || ReadU32(d) == HACK_BSQ_MAGIC)
		return;
	cap = HackChallengeB_LevelCap();
	if (cap == 0 || cap > MAX_LEVEL)
		return;
	for (i = 0; i < PARTY_SIZE && n < HACK_BSQ_SLOTS; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];
		u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);

		if (species == SPECIES_NONE || species == SPECIES_EGG || GetMonData(mon, MON_DATA_LEVEL, NULL) <= cap)
			continue;
		WriteU32(d + 8 + 8 * n, GetMonData(mon, MON_DATA_PERSONALITY, NULL));
		WriteU32(d + 12 + 8 * n, GetMonData(mon, MON_DATA_EXP, NULL));
		++n;
		SetLevelKeepHpRatio(mon, cap, gExperienceTables[gBaseStats[species].growthRate][cap]);
	}
	if (n != 0)
	{
		WriteU32(d + 4, n);
		WriteU32(d, HACK_BSQ_MAGIC);
	}
}

bool8 HackBadgeSquash_Active(void)
{
	return Marked();
}

//元に戻す。印が無ければ何もしない＝何度呼んでも壊れない。子は性格値で探す
void HackBadgeSquash_Restore(void)
{
	u8* d = HACK_BSQ_BASE;
	u32 i, j, n;

	if (!Marked())
		return;
	n = ReadU32(d + 4);
	if (n > HACK_BSQ_SLOTS)
		n = HACK_BSQ_SLOTS;
	WriteU32(d, 0);   //先に印を消す（途中で何かが起きても二重に戻さない）
	for (i = 0; i < n; ++i)
	{
		u32 pid = ReadU32(d + 8 + 8 * i);
		u32 exp = ReadU32(d + 12 + 8 * i);

		for (j = 0; j < PARTY_SIZE; ++j)
		{
			struct Pokemon* mon = &gPlayerParty[j];
			u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);

			if (species != SPECIES_NONE && GetMonData(mon, MON_DATA_PERSONALITY, NULL) == pid)
			{
				u32 now = GetMonData(mon, MON_DATA_EXP, NULL);
				u16 level;

				SetMonData(mon, MON_DATA_EXP, &exp);   //元の経験値でのレベルを出すため 一時的に差し替える
				level = GetLevelFromMonExp(mon);
				SetMonData(mon, MON_DATA_EXP, &now);
				SetLevelKeepHpRatio(mon, level, exp);
				break;
			}
		}
		WriteU32(d + 8 + 8 * i, 0);
		WriteU32(d + 12 + 8 * i, 0);
	}
	WriteU32(d + 4, 0);
}

//フィールドの安全装置（HackChallengeC_FieldTick から）: 戦闘が終わって野外に戻っているのに印が残っているとき
void HackBadgeSquash_FieldTick(void)
{
	if (Marked() && gNewBS == NULL && !ScriptContext2_IsEnabled() && gMain.callback2 == CB2_Overworld)
		HackBadgeSquash_Restore();
}
