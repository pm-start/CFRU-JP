#include "defines.h"
#include "defines_battle.h"
#include "../include/data2.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/constants/items.h"
#include "hack_fuuin.h"

/*
    縛り「ふういんルーレット」（#123）。説明と記録の形は hack_fuuin.h。docs/research/2026-10-17_fuuin-roulette.md

    ・レベルアップの数を filler の記録の [132] に足す（戦闘中も ここに足すだけ＝戦闘のあいだ手持ち・道具・技を触らない）
    ・フィールドで 16 コマごとに 1 つずつ（HackFuuin_FieldTick）: 未処理があれば台本 EventScript_FuuinSpin（ルーレット 1 回）を始める
    ・ついでに 手持ちの 1 匹ぶん（16 コマごとに 1 匹・6.80）が封印された子なら ボックスへ戻す（古い版で戻された・ボックスから出した）
    ・状態は filler_B20 の記録だけ（書き換え可能な static は使わない・6.13）
*/

extern const u16 gFieldMoves[];   //src/party_menu.c（フィールドで使う技の表・15 個＝ひでん技・いあいぎり・フラッシュ・テレポート・あなをほる・タマゴうみ など）
#define HACK_FUUIN_FIELD_MOVES 15

extern u8 gMoveNames[][MOVE_NAME_LENGTH + 1];   //わざの名前（battle_strings.c などと同じ宣言）
u8 SendMonToPC(struct Pokemon* mon);   //include/new/pokemon_storage_system.h と同じ
u8* __attribute__((long_call)) GetMonNickname(struct Pokemon* mon, u8* dest);   //include/party_menu.h と同じ

extern const u8 EventScript_FuuinSpin[];
extern const u8 EventScript_FuuinReseal[];

struct FuuinBagPocket
{
	struct ItemSlot* slots;
	u32 amount;
};
#define sFuuinBag ((struct FuuinBagPocket*) 0x20397D8)   //CFRU の src/item.c の gBagPockets（achievements.c と同じ・BPRJ.ld に無い）
#define FUUIN_POCKET_ITEMS 0
#define FUUIN_POCKET_TM 3
#define FUUIN_POCKET_BERRY 4

/* ---- 記録（filler_B20 の空き 136 バイトを 1 本の論理領域として読み書き）---- */

static u8* FuPtr(u8 i)
{
	return i < 128 ? gSaveBlock2->filler_B20 + HACK_FUUIN_BASE_A + i : gSaveBlock2->filler_B20 + HACK_FUUIN_BASE_B + (i - 128);
}

static u8 FuRd(u8 i)
{
	return *FuPtr(i);
}

static void FuWr(u8 i, u8 v)
{
	*FuPtr(i) = v;
}

static u16 FuRd16(u8 i)
{
	return FuRd(i) | (FuRd(i + 1) << 8);
}

static void FuWr16(u8 i, u16 v)
{
	FuWr(i, v & 0xFF);
	FuWr(i + 1, v >> 8);
}

static u32 FuRd32(u8 i)
{
	return FuRd16(i) | ((u32) FuRd16(i + 2) << 16);
}

static void FuWr32(u8 i, u32 v)
{
	FuWr16(i, v & 0xFFFF);
	FuWr16(i + 2, v >> 16);
}

static bool8 FuValid(void)
{
	return FuRd(0) == HACK_FUUIN_MAGIC;
}

static void FuClear(void)
{
	u8 i;

	for (i = 0; i < HACK_FUUIN_LEN; ++i)
		FuWr(i, 0);
}

static void FuEnsure(void)
{
	if (!FuValid())
	{
		FuClear();
		FuWr(0, HACK_FUUIN_MAGIC);
	}
}

//数の欄（[1] 子・[2] 道具・[3] 技）は 範囲をこえていたら上限にする（壊れた記録で表の外を読まない）
static u8 FuCount(u8 index, u8 max)
{
	u8 n = FuValid() ? FuRd(index) : 0;

	return n > max ? max : n;
}

/* ---- 封印されているか ---- */

static bool8 MonSealed(u32 pid)
{
	u8 i, n = FuCount(1, HACK_FUUIN_MAX_MON);

	for (i = 0; i < n; ++i)
	{
		if (FuRd32(HACK_FUUIN_O_MON + i * 4) == pid)
			return TRUE;
	}
	return FALSE;
}

static bool8 ItemInList(u16 item)
{
	u8 i, n = FuCount(2, HACK_FUUIN_MAX_ITEM);

	for (i = 0; i < n; ++i)
	{
		if (FuRd16(HACK_FUUIN_O_ITEM + i * 2) == item)
			return TRUE;
	}
	return FALSE;
}

static bool8 MoveInList(u16 move)
{
	u8 i, n = FuCount(3, HACK_FUUIN_MAX_MOVE);

	for (i = 0; i < n; ++i)
	{
		if (FuRd16(HACK_FUUIN_O_MOVE + i * 2) == move)
			return TRUE;
	}
	return FALSE;
}

bool8 HackFuuin_On(void)
{
	return FlagGet(HACK_FLAG_CHD_FUUIN) != 0;
}

//バッグの「つかう」（hack_challenge.c）: 封印された道具は使えない
bool8 HackFuuin_ItemSealed(u16 item)
{
	return HackFuuin_On() && item != ITEM_NONE && ItemInList(item);
}

//戦闘の技の選択（battle_util.c IsUnusableMove・move_menu.c TrySetCantSelectMoveBattleScript）: 自分の側の封印された技は出せない。
//通信・フロンティア・バトル施設（借りた手持ち）は対象外。技が全部封印されたら 素の「どれも出せない」の道（わるあがき）
bool8 HackFuuin_MoveSealed(u8 bank, u16 move)
{
	if (!HackFuuin_On() || move == MOVE_NONE || SIDE(bank) != B_SIDE_PLAYER)
		return FALSE;
	if ((gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)) || FlagGet(FLAG_BATTLE_FACILITY))
		return FALSE;
	return MoveInList(move);
}

/* ---- レベルアップを数える ---- */

//exp.c（戦闘のレベルアップ 1 回ごと）・party_menu.c／exp_candy.c（ふしぎなアメ・けいけんアメ）。数えるだけ（処理は戦闘のあと／フィールドで）
//0.63.0（challenge-balance）: 封印は『レベルが 10 の倍数をまたぐたび』に 1 回（Lv10・20・30…に達したとき）。
//数え方は oldLevel/10 と newLevel/10 の差だけ＝状態を増やさない（記録の形はそのまま）。1 回で 10 の倍数を複数またいだら（けいけんアメ・ふしぎなアメ 9→25 など）またいだ数だけ数える
void HackFuuin_OnLevelsGained(u8 oldLevel, u8 newLevel)
{
	u16 pending;
	u8 n = newLevel > oldLevel ? newLevel / HACK_FUUIN_LEVEL_STEP - oldLevel / HACK_FUUIN_LEVEL_STEP : 0;

	if (n == 0 || !HackFuuin_On() || FlagGet(FLAG_BATTLE_FACILITY) || (gBattleTypeFlags & BATTLE_TYPE_LINK))
		return;
	FuEnsure();
	pending = FuRd(HACK_FUUIN_O_PENDING) + n;
	FuWr(HACK_FUUIN_O_PENDING, pending > 0xFF ? 0xFF : pending);
}

/* ---- 候補を数える・n 番目を探す（候補の一覧は持たない＝スタックを使わない・6.35）---- */

static bool8 PartySlotUsable(u8 slot)
{
	u16 species = GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES2, NULL);

	return species != SPECIES_NONE && species != SPECIES_EGG;
}

//封印されていない（タマゴでない）手持ちの子の数
static u8 PartyUnsealedCount(void)
{
	u8 i, n = 0;

	for (i = 0; i < gPlayerPartyCount && i < PARTY_SIZE; ++i)
	{
		if (PartySlotUsable(i) && !MonSealed(GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY, NULL)))
			++n;
	}
	return n;
}

//pick 番目の候補の手持ちの番号（pick が 0xFF なら数えるだけ）。*total に候補の数
static u8 ScanMons(u8 pick, u8* total)
{
	u8 i, n = 0, found = 0xFF;

	for (i = 0; i < gPlayerPartyCount && i < PARTY_SIZE; ++i)
	{
		if (!PartySlotUsable(i) || MonSealed(GetMonData(&gPlayerParty[i], MON_DATA_PERSONALITY, NULL)))
			continue;
		if (n == pick)
			found = i;
		++n;
	}
	*total = n;
	return found;
}

//ひでんマシンの道具は選ばない（フィールドで使う技を覚えさせられなくなる）
static bool8 IsHmItem(u16 item)
{
	return item >= ITEM_HM01_CUT && item <= ITEM_HM08_ROCK_CLIMB;
}

//バッグの「どうぐ」「わざマシン」「きのみ」の枠から（ボール・たいせつなものは見ない）
static u16 ScanItems(u16 pick, u16* total)
{
	static const u8 pockets[] = {FUUIN_POCKET_ITEMS, FUUIN_POCKET_TM, FUUIN_POCKET_BERRY};
	u16 n = 0, found = ITEM_NONE;
	u8 p;
	u32 i;

	for (p = 0; p < NELEMS(pockets); ++p)
	{
		struct FuuinBagPocket* bag = &sFuuinBag[pockets[p]];

		for (i = 0; i < bag->amount; ++i)
		{
			u16 item = bag->slots[i].itemId;

			if (item == ITEM_NONE || IsHmItem(item) || ItemInList(item) || GetBagItemQuantity(&bag->slots[i].quantity) == 0)
				continue;
			if (n == pick)
				found = item;
			++n;
		}
	}
	*total = n;
	return found;
}

static bool8 IsFieldMove(u16 move)
{
	u8 i;

	for (i = 0; i < HACK_FUUIN_FIELD_MOVES; ++i)
	{
		if (gFieldMoves[i] == move)
			return TRUE;
	}
	return FALSE;
}

//手持ちの子が覚えている技（フィールドで使う技・封印済みは除く）。同じ技を複数の子が覚えていれば その数だけ候補に数える
static u16 ScanMoves(u8 pick, u8* total)
{
	u8 i, j, n = 0;
	u16 found = MOVE_NONE;

	for (i = 0; i < gPlayerPartyCount && i < PARTY_SIZE; ++i)
	{
		if (!PartySlotUsable(i))
			continue;
		for (j = 0; j < MAX_MON_MOVES; ++j)
		{
			u16 move = GetMonData(&gPlayerParty[i], MON_DATA_MOVE1 + j, NULL);

			if (move == MOVE_NONE || IsFieldMove(move) || MoveInList(move))
				continue;
			if (n == pick)
				found = move;
			++n;
		}
	}
	*total = n;
	return found;
}

/* ---- ボックスへ送る ---- */

//手持ちの slot をボックスへ。送れなければ FALSE（ボックスがいっぱい）。名前を gStringVar1 に
static bool8 SendPartyMonToBox(u8 slot)
{
	GetMonNickname(&gPlayerParty[slot], gStringVar1);
	if (SendMonToPC(&gPlayerParty[slot]) == MON_CANT_GIVE)
		return FALSE;
	ZeroMonData(&gPlayerParty[slot]);
	CompactPartySlots();
	CalculatePlayerPartyCount();
	return TRUE;
}

/* ---- ルーレット 1 回 ---- */

//callasm（台本 EventScript_FuuinSpin）。LASTRESULT: 0 空振り（封印できるものが無い・封印の枠がいっぱい・ボックスがいっぱい）／1 ポケモン／2 どうぐ／3 わざ
//（名前は gStringVar1）。どの種類を封印するかは「封印できる種類」から同じ確率で選ぶ。手持ちが 1 匹（封印されていないタマゴでない子が 1 匹以下）のときは
//ポケモンを選ばない（手持ちが空になる・ほかの種類から選ぶ）
void HackFuuin_Spin(void)
{
	u8 kinds[3];
	u8 nKinds = 0, k, total8, pick8, slot;
	u16 total16, item;

	gSpecialVar_LastResult = 0;
	if (!HackFuuin_On())
		return;
	FuEnsure();
	if (FuRd(HACK_FUUIN_O_PENDING) != 0)
		FuWr(HACK_FUUIN_O_PENDING, FuRd(HACK_FUUIN_O_PENDING) - 1);

	if (FuCount(1, HACK_FUUIN_MAX_MON) < HACK_FUUIN_MAX_MON)
	{
		ScanMons(0xFF, &total8);
		if (total8 >= 2)
			kinds[nKinds++] = 1;
	}
	if (FuCount(2, HACK_FUUIN_MAX_ITEM) < HACK_FUUIN_MAX_ITEM)
	{
		ScanItems(0xFFFF, &total16);
		if (total16 > 0)
			kinds[nKinds++] = 2;
	}
	if (FuCount(3, HACK_FUUIN_MAX_MOVE) < HACK_FUUIN_MAX_MOVE)
	{
		ScanMoves(0xFF, &total8);
		if (total8 > 0)
			kinds[nKinds++] = 3;
	}
	if (nKinds == 0)
		return;   //空振り

	k = kinds[Random() % nKinds];
	if (k == 1)
	{
		ScanMons(0xFF, &total8);
		pick8 = Random() % total8;
		slot = ScanMons(pick8, &total8);
		if (slot == 0xFF)
			return;
		{
			u32 pid = GetMonData(&gPlayerParty[slot], MON_DATA_PERSONALITY, NULL);
			u8 n = FuRd(1);

			if (!SendPartyMonToBox(slot))
				return;   //ボックスがいっぱい＝空振り
			FuWr32(HACK_FUUIN_O_MON + n * 4, pid);
			FuWr(1, n + 1);
		}
		gSpecialVar_LastResult = 1;
	}
	else if (k == 2)
	{
		u8 n = FuRd(2);

		ScanItems(0xFFFF, &total16);
		item = ScanItems(Random() % total16, &total16);
		if (item == ITEM_NONE)
			return;
		FuWr16(HACK_FUUIN_O_ITEM + n * 2, item);
		FuWr(2, n + 1);
		CopyItemName(item, gStringVar1);
		gSpecialVar_LastResult = 2;
	}
	else
	{
		u8 n = FuRd(3);
		u16 move;

		ScanMoves(0xFF, &total8);
		pick8 = Random() % total8;
		move = ScanMoves(pick8, &total8);
		if (move == MOVE_NONE)
			return;
		FuWr16(HACK_FUUIN_O_MOVE + n * 2, move);
		FuWr(3, n + 1);
		StringCopy(gStringVar1, gMoveNames[move]);
		gSpecialVar_LastResult = 3;
	}
}

/* ---- フィールドの毎コマ ---- */

static bool8 FieldReadyFu(void)
{
	return !(gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld || gNewBS != NULL);
}

//16 コマに 1 回（コマの下位 4 ビットが 8）。OFF なら記録を消す（「OFF にすると全部とける」）。ON なら 未処理のレベルアップがあればルーレット 1 回・
//なければ 手持ちの 1 匹ぶん（16 コマごとに 1 匹・GetMonData は暗号を解くので重い・6.80）を調べ、封印された子が手持ちに戻っていたらボックスへ戻す
void HackFuuin_FieldTick(void)
{
	u8 slot;
	u32 pid;

	if ((gMain.vblankCounter2 & 0xF) != 8)
		return;
	if (!HackFuuin_On())
	{
		if (FuValid())
			FuClear();
		return;
	}
	if (FlagGet(FLAG_BATTLE_FACILITY) || !FieldReadyFu())
		return;
	if (FuValid() && FuRd(HACK_FUUIN_O_PENDING) != 0)
	{
		ScriptContext1_SetupScript(EventScript_FuuinSpin);
		return;
	}
	if (FuCount(1, HACK_FUUIN_MAX_MON) == 0)
		return;
	slot = (gMain.vblankCounter2 >> 4) % PARTY_SIZE;
	if (slot >= gPlayerPartyCount || !PartySlotUsable(slot))
		return;
	pid = GetMonData(&gPlayerParty[slot], MON_DATA_PERSONALITY, NULL);
	if (!MonSealed(pid) || PartyUnsealedCount() < 1)
		return;   //全員が封印された子なら 手持ちが空にならないよう そのまま（封印された子だけの手持ちは 1 つ前の「ほかに戦える子」を残す）
	if (SendPartyMonToBox(slot))
		ScriptContext1_SetupScript(EventScript_FuuinReseal);
}
