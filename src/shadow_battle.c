#include "defines.h"
#include "defines_battle.h"
#include "hack_kind.h"
#include "shadow_gauge.h"
#include "shadow_moves.h"
#include "shadow_battle.h"
#include "shadow_relive.h"   //[hack] patches/0965 段 G

struct Pokemon* GetBankPartyData(u8 bank); //include/new/battle_util.h

/*
 * ダーク技の組み立て・心のゲージの戦闘側（src/shadow_battle.h に仕様）。
 * 戦闘用の技の並び gBattleMons[bank].moves／pp／ppBonuses だけを書き換える。手持ちの技欄は動かさない。
 * 戦闘中の PP の書き戻し（コントローラーが手持ちの同じ枠に書く）は並びの枠と元の枠がずれるので、元の技の PP は gNewBS の控えで持ち、戦闘のあと手持ちへ戻す。
 */

static bool8 IsDarkMon(struct Pokemon* mon)
{
	return mon != NULL && HackKind_Get((const struct BoxPokemon*) mon) == HACK_KIND_DARK;
}

bool8 Shadow_RefusesGrowth(struct Pokemon* mon)
{
	return IsDarkMon(mon);
}

bool8 Shadow_RefusesGrowthBox(struct BoxPokemon* mon)
{
	return mon != NULL && HackKind_Get(mon) == HACK_KIND_DARK;
}

static u8 DarkMovesOf(struct Pokemon* mon, u16* out)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	const struct BaseStats* st = &gBaseStats[species];
	u16 bst = st->baseHP + st->baseAttack + st->baseDefense + st->baseSpeed + st->baseSpAttack + st->baseSpDefense;

	return Shadow_AutoMoves(st->type1, st->type2, st->baseAttack, st->baseSpAttack, bst, out);
}

bool8 ShadowSummary_Build(struct Pokemon* mon, u16* moves, u8* src)
{
	u16 dark[SHADOW_DARK_MOVES_MAX], own[MAX_MON_MOVES];
	u8 n, i;

	if (!IsDarkMon(mon))
		return FALSE;

	for (i = 0; i < MAX_MON_MOVES; ++i)
		own[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);

	n = DarkMovesOf(mon, dark);
	Shadow_BuildMoveList(HackKind_Value((const struct BoxPokemon*) mon), dark, n, own, TRUE, moves, src);
	return TRUE;
}

static struct HackShadowViewEntry* FindEntry(u8 side, u32 pid)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(gNewBS->hackShadowView); ++i)
		if (gNewBS->hackShadowView[i].used && gNewBS->hackShadowView[i].side == side && gNewBS->hackShadowView[i].pid == pid)
			return &gNewBS->hackShadowView[i];
	return NULL;
}

static bool8 Usable(void)
{
	return gNewBS != NULL && !gNewBS->hackChfGachaOn && !(gBattleTypeFlags & BATTLE_TYPE_LINK);
}

//場に出た子（プレイヤー側）のゲージを その戦闘で初めてのときだけ 1 回分減らす
static void ReduceOnSendOut(u8 bank, struct Pokemon* mon)
{
	u8 slot;
	u8 nature, gauge;

	if (SIDE(bank) != B_SIDE_PLAYER)
		return;

	slot = gBattlerPartyIndexes[bank];
	if (slot >= PARTY_SIZE || mon != &gPlayerParty[slot] || (gNewBS->hackShadowSent & gBitTable[slot]))
		return;

	gNewBS->hackShadowSent |= gBitTable[slot];
	nature = GetNatureFromPersonality(GetMonData(mon, MON_DATA_PERSONALITY, NULL));
	gauge = HackKind_Value((const struct BoxPokemon*) mon);
	HackKind_SetValue((struct BoxPokemon*) mon, Shadow_GaugeAfter(gauge, nature, SHADOW_ACTION_BATTLE));
}

/*
 * checkTransformed: 戦闘の最初と進化のあと（gBattleMons が組み上がっている）は へんしん中の子を除く。
 * 入れ替えの atk4D（switching.c）の口では見ない: その時点の gBattleMons[bank].status2 は、素の switchindataupdate が
 * SwitchInClearSetData で 0 にする前で、バッファの未初期化の中身（スタックの残り）が入っている。
 * そこで TRANSFORMED の bit が偶然立つと、ダーク技・ゲージ減りが乱数的に抜けた（INT-shadow-pokenav・実機でブレークポイントを置いて確認）。
 * 出てきた直後の子は へんしんしていない（へんしんは交代で切れる）。
 */
static void ApplyToBattler(u8 bank, bool8 checkTransformed)
{
	struct Pokemon* mon;
	struct HackShadowViewEntry* e;
	u16 dark[SHADOW_DARK_MOVES_MAX], moves[MAX_MON_MOVES];
	u8 src[MAX_MON_MOVES];
	u8 n, i, side, bonus = 0;

	if (!Usable() || bank >= gBattlersCount || gBattleMons[bank].species == SPECIES_NONE
	|| (checkTransformed && (gBattleMons[bank].status2 & STATUS2_TRANSFORMED)))
		return;

	mon = GetBankPartyData(bank);
	if (!IsDarkMon(mon))
		return;

	side = SIDE(bank);
	e = FindEntry(side, gBattleMons[bank].personality);
	if (e == NULL)
	{
		for (i = 0; i < ARRAY_COUNT(gNewBS->hackShadowView) && gNewBS->hackShadowView[i].used; ++i) {}
		if (i >= ARRAY_COUNT(gNewBS->hackShadowView))
			return; //13 匹目（起きない）は何もしない
		e = &gNewBS->hackShadowView[i];
		e->pid = gBattleMons[bank].personality;
		e->side = side;
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			e->own[i] = gBattleMons[bank].moves[i];
			e->realPP[i] = gBattleMons[bank].pp[i];
		}
		e->realBonus = gBattleMons[bank].ppBonuses;
		e->used = TRUE;
	}

	ReduceOnSendOut(bank, mon);

	n = DarkMovesOf(mon, dark);
	Shadow_BuildMoveList(HackKind_Value((const struct BoxPokemon*) mon), dark, n, e->own, FALSE, moves, src);

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		e->src[i] = src[i];
		e->view[i] = moves[i];
		gBattleMons[bank].moves[i] = moves[i];

		if (src[i] < MAX_MON_MOVES) //元の技: 控えの PP とポイントアップ
		{
			gBattleMons[bank].pp[i] = e->realPP[src[i]];
			bonus |= ((e->realBonus >> (src[i] * 2)) & 3) << (i * 2);
		}
		else if (src[i] == SHADOW_SRC_DARK) //ダーク技は PP を使わない（表の PP を見せるだけ）
			gBattleMons[bank].pp[i] = gBattleMoves[moves[i]].pp;
		else
			gBattleMons[bank].pp[i] = 0;
	}
	gBattleMons[bank].ppBonuses = bonus;
}

void ShadowBattle_OnBattleStart(void)
{
	u32 b;

	if (!Usable())
		return;
	for (b = 0; b < gBattlersCount; ++b)
		ApplyToBattler(b, TRUE);
}

void ShadowBattle_OnSwitchIn(u8 bank)
{
	ApplyToBattler(bank, FALSE);   //入れ替え（atk4D）: status2 はまだ未初期化＝見ない
}

void ShadowBattle_OnEvolved(u8 bank)
{
	ApplyToBattler(bank, TRUE);    //戦闘中の進化のあと: gBattleMons は正しい＝へんしん中は除く
}

//今の PP を控えへ（並びの枠が変わっていない元の技だけ・ものまね等で変わった枠は触らない）
void ShadowBattle_OnSwitchOut(u8 bank)
{
	struct HackShadowViewEntry* e;
	u32 i;

	if (!Usable() || bank >= gBattlersCount)
		return;
	e = FindEntry(SIDE(bank), gBattleMons[bank].personality);
	if (e == NULL)
		return;
	for (i = 0; i < MAX_MON_MOVES; ++i)
		if (e->src[i] < MAX_MON_MOVES && gBattleMons[bank].moves[i] == e->view[i])
			e->realPP[e->src[i]] = gBattleMons[bank].pp[i];
}

//戦闘のあと: 場にいる子の PP を控えへ → プレイヤー側の控えを手持ちの同じ子へ戻す（技が変わっていない枠だけ）
void ShadowBattle_OnBattleEnd(void)
{
	u32 n, i, j;

	if (gNewBS == NULL)
		return;

	for (i = 0; i < gBattlersCount; ++i)
		ShadowBattle_OnSwitchOut(i);

	for (n = 0; n < ARRAY_COUNT(gNewBS->hackShadowView); ++n)
	{
		struct HackShadowViewEntry* e = &gNewBS->hackShadowView[n];
		struct Pokemon* mon = NULL;

		if (!e->used)
			continue;
		e->used = FALSE;
		if (e->side != B_SIDE_PLAYER)
		{
			Relive_NoteDarkMet();   //[hack] patches/0965 段 G: 相手にダークが出た戦闘が終わった（看護師がスナッチボールを渡せるようになる）
			continue;
		}
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
			continue; //手持ちにいない（逃がした・ボックスへ）
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			u8 pp = e->realPP[i];

			if (GetMonData(mon, MON_DATA_MOVE1 + i, NULL) != e->own[i])
				continue;
			SetMonData(mon, MON_DATA_PP1 + i, &pp);
		}
		SetMonData(mon, MON_DATA_PP_BONUSES, &e->realBonus);
	}
}
