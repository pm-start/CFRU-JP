#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/script.h"
#include "../include/constants/items.h"
#include "../include/constants/moves.h"
#include "../include/constants/pokemon.h"
#include "../include/constants/species.h"
#include "../include/new/ram_locs.h"
#include "../include/new/rom_locs.h"
#include "../include/new/pokemon_storage_system.h"
#include "../include/money.h"
#include "hack_dummy_battle.h"
#include "hack_box_shelf.h"
#include "hack_tamper.h"
#include "hack_ram_cheat_seed.h"
#include "hack_ram_cheat.h"

/*
    RAM チート検出（patches/0682・docs/research/2026-10-21_ram-cheat-detect.md）

    毎コマの処理は 0。確かめるのは「地図の読み込み」「戦闘の開始」「セーブ」の 3 つの機会だけ（通常版）。デバッグ版は自動の入口が空で、
    デバッグメニューの「かいぞう チェック」が HackRamCheat_RunNow を 1 回呼ぶ（印は付けない）。
    反応は止めず・書き換えず。同じ区分が 2 回続けて（別の機会・300 コマ以上あけて）見つかったら HackTamper_Mark(RAM_CHEAT)（印は理由の bit だけ足す共用）。
    お金・コインの影は「平文の値を版ごとの種でまぜたハッシュ」。更新するのは正当な書き手だけ（素の SetMoney の置き換え・SetCoins）。
    影は EWRAM の固定番地（ヒープは地図の読み込みで消え、static は捨てられる＝CLAUDE.md 6.13）。電源を切れば消える＝セーブの形は変わらない。
    魔法の値が無い間（起動直後・つづきから前）は影の確かめを省く（範囲の確かめだけ）。
*/

#define HACK_RC_NUM_SPECIES   NUM_SPECIES
#define HACK_RC_MOVES_COUNT   MOVES_COUNT
#define HACK_RC_ITEMS_COUNT   ITEMS_COUNT
#define HACK_RC_MAX_LEVEL     MAX_LEVEL
#define HACK_RC_EV_CAP        EV_CAP
#define HACK_RC_EV_TOTAL      MAX_TOTAL_EVS
#include "hack_ram_cheat_core.h"

void CreateBoxMonFromCompressedMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);   //src/pokemon_storage_system.c

#define HACK_RC_SHADOW ((struct HackRcShadow*) 0x0203EF80)   /* 24 バイト（0x0203EF80〜0x0203EF98）。空きの根拠は資料 */

u32 GetCoins(void);   //src/scripting.c

static u32 Hash(u32 v)
{
	return HackRc_Hash(v, HACK_RAMCHEAT_SEED_A, HACK_RAMCHEAT_SEED_B);
}

static bool8 Valid(void)
{
	return HACK_RC_SHADOW->magic == HACK_RC_MAGIC;
}

//素の SetMoney（0809FC50）の置き換え。式は素と同じ（*p = 鍵 ^ 値）。お金のほんとうの置き場のときだけ影を更新する
void HackRamCheat_SetMoney(u32* moneyPtr, u32 newValue)
{
	*moneyPtr = gSaveBlock2->encryptionKey ^ newValue;
	if (moneyPtr == &gSaveBlock1->money && Valid())
		HACK_RC_SHADOW->moneyH = Hash(newValue);
}

void HackRamCheat_NoteCoins(u32 numCoins)
{
	if (Valid())
		HACK_RC_SHADOW->coinsH = Hash(numCoins);
}

void HackRamCheat_Resync(void)
{
	struct HackRcShadow* s = HACK_RC_SHADOW;

	s->moneyH = Hash(GetMoney(&gSaveBlock1->money));
	s->coinsH = Hash(GetCoins());
	s->lastFrame = 0;
	s->prevMask = 0;
	s->hasLast = 0;
	s->marked = 0;
	s->reserved = 0;
	s->magic = HACK_RC_MAGIC;
}

//手持ちが一時的に別物になる場面（施設の貸し出し・ダミー戦・手持ちの入れ替え中）は手持ちを見ない
static bool8 PartyIsTemporary(void)
{
	if (ExtensionState.partyBackup != NULL || ExtensionState.skyBattlePartyBackup != NULL)
		return TRUE;
	if (FlagGet(FLAG_BATTLE_FACILITY) || HackDummy_Active())
		return TRUE;
	return (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_MOCK_BATTLE
							  | BATTLE_TYPE_MULTI | BATTLE_TYPE_TOWER_LINK_MULTI | BATTLE_TYPE_INGAME_PARTNER)) != 0;
}

static u32 ExpMax(u16 species)
{
	if (species == 0 || species >= HACK_RC_NUM_SPECIES)
		return 0xFFFFFFFFU;
	return gExperienceTables[gBaseStats[species].growthRate][HACK_RC_MAX_LEVEL];
}

//手持ち 1 匹。値は素の GetMonData（平文・暗号も checksum も無い形）で読む。検算が 素の ROM の GetMonData で本物の形の子を通す（verify_ram_cheat.py D）
u8 HackRamCheat_CheckPartyMon(struct Pokemon* mon)
{
	struct HackRcMon m;
	u32 i;
	u16 species;

	Memset(&m, 0, sizeof(m));
	species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	if (species == SPECIES_NONE)
		return HRC_OK;   //空きの枠
	m.species = species;
	m.item = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
	m.exp = GetMonData(mon, MON_DATA_EXP, NULL);
	m.expMax = ExpMax(species);
	m.level = mon->level;
	m.hp = mon->hp;
	m.maxHP = mon->maxHP;
	m.skipStats = GetMonData(mon, MON_DATA_IS_EGG, NULL) != 0;
	if (!m.skipStats && species < HACK_RC_NUM_SPECIES)
		m.expLevel = GetLevelFromMonExp(mon);
	for (i = 0; i < 4; ++i)
	{
		m.moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
		m.pp[i] = GetMonData(mon, MON_DATA_PP1 + i, NULL);
		if (m.moves[i] != 0 && m.moves[i] < HACK_RC_MOVES_COUNT)
			m.ppMax[i] = CalculatePPWithBonus(m.moves[i], GetMonData(mon, MON_DATA_PP_BONUSES, NULL), i);
	}
	for (i = 0; i < 6; ++i)
		m.ev[i] = GetMonData(mon, MON_DATA_HP_EV + i, NULL);
	return HackRc_CheckMon(&m);
}

//入れ物に載っている箱だけ（フラッシュの箱は読まない＝遅い・書き換えられていない）
//箱の 1 匹（縮めた形 78 バイト・X 案 patches/0680。努力値の上位 evHi が 0 でなければ CreateBoxMonFromCompressedMon が 255 に頭打ちするので、上限 252 をこえて「改造」と見る＝正しい）。持ち物・経験値などの上位ビットは ID の入れ物、技は 16 ビットを 3 バイトに分けて持つので、
//生のバイトは読まず 素の CreateBoxMonFromCompressedMon で 80 バイトの形に戻してから GetBoxMonData で読む。縮めた形は checksum もレベルも持たない
u8 HackRamCheat_CheckCompMon(struct CompressedPokemon* c)
{
	struct HackRcMon m;
	struct BoxPokemon box;
	u32 i;

	if (c->substruct0.species == 0)
		return HRC_OK;   //空き（種族の欄は ID の入れ物ではない）
	Memset(&m, 0, sizeof(m));
	CreateBoxMonFromCompressedMon(&box, c);
	m.skipStats = 1;
	m.species = GetBoxMonData(&box, MON_DATA_SPECIES, NULL);
	m.item = GetBoxMonData(&box, MON_DATA_HELD_ITEM, NULL);
	m.exp = GetBoxMonData(&box, MON_DATA_EXP, NULL);
	m.expMax = ExpMax(m.species);
	for (i = 0; i < 4; ++i)
		m.moves[i] = GetBoxMonData(&box, MON_DATA_MOVE1 + i, NULL);
	for (i = 0; i < 6; ++i)
		m.ev[i] = GetBoxMonData(&box, MON_DATA_HP_EV + i, NULL);
	return HackRc_CheckMon(&m);
}

static void RunChecks(struct HackRamCheatReport* r, bool8 withBox)
{
	u32 money, coins, i, pos;
	struct HackRcShadow* s = HACK_RC_SHADOW;

	Memset(r, 0, sizeof(*r));
	r->partySlot = 0xFF;
	r->boxId = 0xFF;
	r->shadowValid = Valid();
	money = GetMoney(&gSaveBlock1->money);
	coins = GetCoins();
	if (money > HACK_RC_MONEY_MAX)
		r->mask |= HACK_RC_MONEY_RANGE;
	if (coins > HACK_RC_COIN_MAX)
		r->mask |= HACK_RC_COIN_RANGE;
	if (r->shadowValid)
	{
		if (Hash(money) != s->moneyH)
			r->mask |= HACK_RC_MONEY_SHADOW;
		if (Hash(coins) != s->coinsH)
			r->mask |= HACK_RC_COIN_SHADOW;
	}
	if (PartyIsTemporary())
		return;
	if (gPlayerPartyCount > PARTY_SIZE)
	{
		r->mask |= HACK_RC_PARTY;
		r->partySlot = PARTY_SIZE;
		r->partyReason = HRC_SPECIES;
	}
	for (i = 0; i < PARTY_SIZE && !(r->mask & HACK_RC_PARTY); ++i)
	{
		u8 why = HackRamCheat_CheckPartyMon(&gPlayerParty[i]);

		if (why != HRC_OK)
		{
			r->mask |= HACK_RC_PARTY;
			r->partySlot = i;
			r->partyReason = why;
		}
	}
	if (!withBox)
		return;
	for (i = 0; i < TOTAL_BOXES_COUNT && !(r->mask & HACK_RC_BOX); ++i)
	{
		struct CompressedPokemon* b = BoxShelf_PeekBoxMons(i);

		if (b == NULL)
			continue;
		for (pos = 0; pos < IN_BOX_COUNT; ++pos)
		{
			u8 why = HackRamCheat_CheckCompMon(&b[pos]);

			if (why != HRC_OK)
			{
				r->mask |= HACK_RC_BOX;
				r->boxId = i;
				r->boxPos = pos;
				r->boxReason = why;
				break;
			}
		}
	}
}

void HackRamCheat_RunNow(struct HackRamCheatReport* r)
{
	RunChecks(r, TRUE);
}

#ifndef HACK_DEBUG_MENU
static void Evaluate(bool8 withBox, bool8 bypassGap)
{
	struct HackRamCheatReport r;

	if (HackTamper_Reasons() & HACK_TAMPER_REASON_RAM_CHEAT)
		return;   //もう印がある
	RunChecks(&r, withBox);
	if (!r.shadowValid)
	{
		if (r.mask == 0)
			HackRamCheat_Resync();   //魔法の値が無いまま遊びが始まった（偽陽性を出さない＝作り直すだけ）
		return;
	}
	if (HackRc_Step(HACK_RC_SHADOW, r.mask, gMain.vblankCounter2, bypassGap))
		HackTamper_Mark(HACK_TAMPER_REASON_RAM_CHEAT);
}

void HackRamCheat_OnMapLoad(void)
{
	Evaluate(FALSE, FALSE);
}

void HackRamCheat_OnBattleStart(void)
{
	Evaluate(FALSE, FALSE);
}

void HackRamCheat_OnSave(void)
{
	Evaluate(TRUE, TRUE);
}
#else
void HackRamCheat_OnMapLoad(void) {}
void HackRamCheat_OnBattleStart(void) {}
void HackRamCheat_OnSave(void) {}
#endif
