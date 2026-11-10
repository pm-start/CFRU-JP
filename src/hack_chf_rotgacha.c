#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/random.h"
#include "hack_challenge.h"
#include "hack_challenge_f.h"      //[hack] 担当 A の旗の台帳（HACK_FLAG_CHF_ROTATION・HACK_FLAG_CHF_ROT_BIT0〜2・HACK_FLAG_CHF_GACHA）。ここでは番地を決めない
#include "hack_chf_rotgacha.h"
#include "hack_dummy_battle.h"

/*
 * 縛り F: ローテーション（案 12）と わざガチャ（案 15）。資料 docs/research/2026-11-03_challenge-f-rotation-gacha.md
 *
 * ローテーション: 戦闘の初期化で「最初に出す子」を決めた直後（multi.c の MultiInitPokemonOrder・hooks MultiInitPokemonOrderHook）に
 *   gBattlerPartyIndexes を書き換える。手持ちの並び（gPlayerParty）は動かさない。番号 n（旗 3 ビット・0〜5）から数えて 最初の「出せる子」
 *   （種族あり・タマゴでない・HP 1 以上）を先頭に。ダブルは次の「出せる子」も（2 匹目）。選んだ最後の子の次の番号を旗に書く（戦闘の始まりで進める＝逃げても進む）。
 *   モノタイプで出せない子は 戦闘の入口（CreateBattleStartTask）で HP 0 にされているので 自然に飛ばされる。
 *
 * わざガチャ: 自分の側の gBattleMons の技を「全部の技」から ランダム 4 つ（Random()）に差し替える。手持ち（gPlayerParty）の技・PP は戦闘のあと必ず元に戻す。
 *   戦闘中の PP 減少は素の ROM が 手持ちの同じ枠の PP に書き戻す（EmitSetMonData REQUEST_PPMOVE1_BATTLE＋n）ので、子ごとに「元の技・PP・PP ポイントアップ」を
 *   gNewBS->hackChfGacha に控え（性格値で探す＝戦闘中に並びが入れ替わる）、EndOfBattleThings の頭で 技が元のままの枠の PP とポイントアップを戻す。
 *   技そのものが変わった枠（戦闘中のレベルアップで覚えた技）は触らない。置き場は gNewBS（書き込める static は 6.13 で ROM になり使えない）。
 */

extern const u16 gFieldMoves[];   //src/party_menu.c（フィールドで使う技の表・15 個）
#define HACK_CHF_FIELD_MOVES 15

/* ---- 共通 ---- */

//ローテーション・ガチャの対象の戦いか。通信・施設・フロンティア・チュートリアル・パートナー/マルチ・レイド・サファリ・ダミー・？？？戦は対象外
bool8 HackChF_BattleAllowed(void)
{
	if (HackDummy_Active() || HackChallenge_IsLentrantBattle())
		return FALSE;
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_OAK_TUTORIAL
						  | BATTLE_TYPE_MOCK_BATTLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI | BATTLE_TYPE_TOWER_LINK_MULTI | BATTLE_TYPE_DYNAMAX
						  | BATTLE_TYPE_SAFARI | BATTLE_TYPE_POKE_DUDE | BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_BENJAMIN_BUTTERFREE))
		return FALSE;
	if (FlagGet(FLAG_BATTLE_FACILITY))
		return FALSE;
	if (ExtensionState.partyBackup != NULL || ExtensionState.skyBattlePartyBackup != NULL)
		return FALSE;
	return TRUE;
}

/* ---- ローテーション ---- */

static u8 RotGet(void)
{
	u8 v = (FlagGet(HACK_FLAG_CHF_ROT_BIT0) ? 1 : 0) | (FlagGet(HACK_FLAG_CHF_ROT_BIT1) ? 2 : 0) | (FlagGet(HACK_FLAG_CHF_ROT_BIT2) ? 4 : 0);

	return v < HACK_CHF_ROT_SLOTS ? v : 0;   //6・7 は来ないはず（来たら 0 から）
}

static void RotSet(u8 v)
{
	if (v & 1) FlagSet(HACK_FLAG_CHF_ROT_BIT0); else FlagClear(HACK_FLAG_CHF_ROT_BIT0);
	if (v & 2) FlagSet(HACK_FLAG_CHF_ROT_BIT1); else FlagClear(HACK_FLAG_CHF_ROT_BIT1);
	if (v & 4) FlagSet(HACK_FLAG_CHF_ROT_BIT2); else FlagClear(HACK_FLAG_CHF_ROT_BIT2);
}

static bool8 CanLead(u8 i)   //出せる子: 種族あり・タマゴでない・HP 1 以上
{
	struct Pokemon* mon = &gPlayerParty[i];
	u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);

	return species != SPECIES_NONE && species != SPECIES_EGG && GetMonData(mon, MON_DATA_HP, NULL) != 0;
}

void HackChFRotation_OnPartyIndexes(void)
{
	u8 want, got = 0, k, i, last = 0, start, picked[2] = {0, 0};

	if (!FlagGet(HACK_FLAG_CHF_ROTATION) || !HackChF_BattleAllowed())
		return;
	want = (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) ? 2 : 1;
	start = RotGet();
	for (k = 0; k < HACK_CHF_ROT_SLOTS && got < want; ++k)
	{
		i = (start + k) % HACK_CHF_ROT_SLOTS;
		if (CanLead(i))
		{
			picked[got++] = i;
			last = i;
		}
	}
	if (got == 0)
		return;   //出せる子がいない（素のまま）
	gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT] = picked[0];
	if (got == 2)
		gBattlerPartyIndexes[B_POSITION_PLAYER_RIGHT] = picked[1];
	RotSet((last + 1) % HACK_CHF_ROT_SLOTS);
}

/* ---- わざガチャ ---- */

#define POOL_FIRST  1
#define POOL_LAST   LAST_MOVE_INDEX
#define SKIP_Z_LO   FIRST_Z_MOVE
#define SKIP_Z_HI   LAST_G_MAX_MOVE          /* Z 技 → ダイマックス技 → キョダイマックス技は 0x2FF〜0x39A で連続（検算で確かめる） */
#if HACK_CHF_GACHA_EXCLUDE_LV100
#define SKIP_L_LO   MOVE_LV100_NORMAL_1
#define SKIP_L_HI   MOVE_LV100_RIL_100
#define SKIP_L_N    (SKIP_L_HI - SKIP_L_LO + 1)
#else
#define SKIP_L_N    0
#endif
#define POOL_TOTAL  ((POOL_LAST - POOL_FIRST + 1) - (SKIP_Z_HI - SKIP_Z_LO + 1) - SKIP_L_N)

//範囲の外でも 山に入れない技（ひでん・何もしない技・わるあがき・スケッチ・特定の子専用）
static bool8 MoveOk(u16 move)
{
	u32 i;

	if (move == MOVE_NONE || move == MOVE_STRUGGLE || move == MOVE_SKETCH || move == MOVE_SPLASH || move == MOVE_CELEBRATE || move == MOVE_HOLDHANDS
	 || move == MOVE_DARKVOID || move == MOVE_HYPERSPACEFURY || move == MOVE_AURAWHEEL)
		return FALSE;
	for (i = 0; i < HACK_CHF_FIELD_MOVES; ++i)
		if (gFieldMoves[i] == move)
			return FALSE;
	return TRUE;
}

static u16 RollMove(const u16* have, u8 n)
{
	u32 tries, j;

	for (tries = 0; tries < 256; ++tries)
	{
		u32 id = POOL_FIRST + (Random() % POOL_TOTAL);
		bool8 dup = FALSE;

		if (id >= SKIP_Z_LO)
			id += SKIP_Z_HI - SKIP_Z_LO + 1;
#if HACK_CHF_GACHA_EXCLUDE_LV100
		if (id >= SKIP_L_LO)
			id += SKIP_L_N;
#endif
		if (id > POOL_LAST || !MoveOk(id))
			continue;
		for (j = 0; j < n; ++j)
			if (have[j] == id)
				dup = TRUE;
		if (!dup)
			return id;
	}
	return MOVE_TACKLE;   //起きないはずの保険
}

bool8 HackChFGacha_Active(void)
{
	return gNewBS != NULL && gNewBS->hackChfGachaOn;
}

static struct HackChfGachaEntry* FindEntry(u32 pid)
{
	u32 i;

	for (i = 0; i < PARTY_SIZE; ++i)
		if (gNewBS->hackChfGacha[i].used && gNewBS->hackChfGacha[i].pid == pid)
			return &gNewBS->hackChfGacha[i];
	return NULL;
}

//gBattleMons[bank]（手持ちから読んだ直後＝元の技）を ガチャの技にする。初めての子は元の技・PP を控えて 4 つ引く。2 回目からは同じ 4 つ・出るときに控えた PP
static void ApplyToBattler(u8 bank)
{
	struct HackChfGachaEntry* e;
	u32 i;

	if (SIDE(bank) != B_SIDE_PLAYER || gBattleMons[bank].species == SPECIES_NONE)
		return;
	e = FindEntry(gBattleMons[bank].personality);
	if (e == NULL)
	{
		for (i = 0; i < PARTY_SIZE && gNewBS->hackChfGacha[i].used; ++i) {}
		if (i >= PARTY_SIZE)
			return;   //7 匹目（起きない）は何もしない
		e = &gNewBS->hackChfGacha[i];
		e->pid = gBattleMons[bank].personality;
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			e->realMoves[i] = gBattleMons[bank].moves[i];
			e->realPP[i] = gBattleMons[bank].pp[i];
		}
		e->realBonus = gBattleMons[bank].ppBonuses;
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			e->gachaMoves[i] = RollMove(e->gachaMoves, i);
			e->gachaPP[i] = gBattleMoves[e->gachaMoves[i]].pp;
		}
		e->used = TRUE;
	}
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		gBattleMons[bank].moves[i] = e->gachaMoves[i];
		gBattleMons[bank].pp[i] = e->gachaPP[i];
	}
	gBattleMons[bank].ppBonuses = 0;   //ガチャの技の PP の上限は技そのものの PP（ポイントアップは元の技のもの）
}

void HackChFGacha_OnBattleStart(void)
{
	u32 b;

	if (gNewBS == NULL || !FlagGet(HACK_FLAG_CHF_GACHA) || !HackChF_BattleAllowed())
		return;
	gNewBS->hackChfGachaOn = TRUE;
	for (b = 0; b < gBattlersCount; ++b)
		ApplyToBattler(b);
}

void HackChFGacha_OnSwitchOut(u8 bank)
{
	struct HackChfGachaEntry* e;
	u32 i;

	if (!HackChFGacha_Active() || SIDE(bank) != B_SIDE_PLAYER)
		return;
	e = FindEntry(gBattleMons[bank].personality);
	if (e == NULL)
		return;
	for (i = 0; i < MAX_MON_MOVES; ++i)
		if (gBattleMons[bank].moves[i] == e->gachaMoves[i])   //ものまね・へんしんで変わった枠の PP は控えない
			e->gachaPP[i] = gBattleMons[bank].pp[i];
}

void HackChFGacha_OnSwitchIn(u8 bank)
{
	if (HackChFGacha_Active())
		ApplyToBattler(bank);
}

//戦闘のあと: 手持ちの技が元のままの枠だけ PP とポイントアップを戻す（技が変わった枠＝戦闘中に覚えた技は触らない）
void HackChFGacha_OnBattleEnd(void)
{
	u32 n, i, j;

	if (gNewBS == NULL || !gNewBS->hackChfGachaOn)
		return;
	for (n = 0; n < PARTY_SIZE; ++n)
	{
		struct HackChfGachaEntry* e = &gNewBS->hackChfGacha[n];
		struct Pokemon* mon = NULL;
		u8 bonus;

		if (!e->used)
			continue;
		e->used = FALSE;
		for (j = 0; j < PARTY_SIZE; ++j)
		{
			if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) != SPECIES_NONE
			 && GetMonData(&gPlayerParty[j], MON_DATA_PERSONALITY, NULL) == e->pid)
			{
				mon = &gPlayerParty[j];
				break;
			}
		}
		if (mon == NULL)
			continue;   //手持ちにいない（逃がした・ボックスへ）
		bonus = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			u8 pp = e->realPP[i];

			if (GetMonData(mon, MON_DATA_MOVE1 + i, NULL) != e->realMoves[i])
				continue;
			if (GetMonData(mon, MON_DATA_PP1 + i, NULL) != pp)
				SetMonData(mon, MON_DATA_PP1 + i, &pp);
			bonus = (bonus & ~(3 << (i * 2))) | (e->realBonus & (3 << (i * 2)));
		}
		SetMonData(mon, MON_DATA_PP_BONUSES, &bonus);
	}
	gNewBS->hackChfGachaOn = FALSE;
}
