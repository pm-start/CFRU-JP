#include "defines.h"
#include "defines_battle.h"
#include "../include/random.h"
#include "../include/constants/abilities.h"
#include "../include/new/ai_advanced.h"
#include "../include/new/ai_master.h"
#include "../include/new/ai_util.h"
#include "../include/new/battle_util.h"
#include "../include/new/damage_calc.h"
#include "../include/new/mega.h"
#include "../include/new/move_menu.h"
#include "../include/new/terastal.h"
#include "lv200_custom_moves.h"
#include "lv200_custom_move_ai.h"

/*
    戦闘 AI の手直し（2026-10-02）

    ユーザー「AI最適化できる？」→（AskUserQuestion）「AI そのものを作り直す」→ 調べた弱いところ
    （docs/research/2026-10-02_cfru-ai-audit.md）から「回復の道具を賢く」「ダイマとテラの乱数をやめる」
    「テラスタルを守りにも使う」「交代を賢く」の 4 つ。

    ★ 上流の AI（src/Battle_AI/ の .c）は差し込み口だけ（patches/0121）。判断はこのファイル。
    ★ 書き換え可能な static は使えない（CLAUDE.md 6.13）。どれも状態を持たない。
*/

#define HACK_SECOND_ABILITY_LEVEL 50     //src/Tables/second_abilities.h の SECOND_ABILITY_UNLOCK_LEVEL と同じ（検算が見る）

u16 SecondAbility_OfSpecies(u16 species);
u16 SecondAbility_OfMon(struct Pokemon* mon);

//[hack] patches/0915 相性の表の 1 マス（gTypeEffectiveness は 0＝等倍（TYPE_MUL_NO_DATA）・1＝効果なし（TYPE_MUL_NO_EFFECT）・500・2000）を倍率（1000＝等倍）に直す。
//  ★ 表の値をそのまま掛けると、等倍のマス（0）が 1 つでもあると積が 0 になる（前の HackAI_TypeMul の不具合・patches/0915 でヤーティ・patches/0916 でテラスタルの判断を直した）
static u32 HackAI_TableMul(u8 moveType, u8 defType)
{
	u32 v;

	if (moveType >= NUMBER_OF_MON_TYPES || defType >= NUMBER_OF_MON_TYPES)
		return TYPE_MUL_NORMAL;
	v = gTypeEffectiveness[moveType][defType];
	if (v == TYPE_MUL_NO_DATA)
		return TYPE_MUL_NORMAL;
	if (v == TYPE_MUL_NO_EFFECT)
		return 0;
	return v;
}

//技のタイプが、この 3 つのタイプ（防御）にどれだけ効くか（等倍＝1000・正しい倍率）
u32 HackAI_TypeMulReal(u8 moveType, u8 t1, u8 t2, u8 t3)
{
	u32 mul = TYPE_MUL_NORMAL;

	if (moveType >= NUMBER_OF_MON_TYPES)
		return mul;
	if (t1 < NUMBER_OF_MON_TYPES)
		mul = (mul * HackAI_TableMul(moveType, t1)) / TYPE_MUL_NORMAL;
	if (t2 < NUMBER_OF_MON_TYPES && t2 != t1)
		mul = (mul * HackAI_TableMul(moveType, t2)) / TYPE_MUL_NORMAL;
	if (t3 < NUMBER_OF_MON_TYPES && t3 != t1 && t3 != t2)
		mul = (mul * HackAI_TableMul(moveType, t3)) / TYPE_MUL_NORMAL;
	return mul;
}

//技そのもの（1 つ目のタイプ＋表の 2 つ目のタイプ type2）が、この 3 つのタイプの相手にどれだけ効くか。damage_calc.c の TypeDamageModificationByDefTypes と同じ 2 周
u32 HackAI_MoveMul(u16 move, u8 t1, u8 t2, u8 t3)
{
	u32 mul = HackAI_TypeMulReal(gBattleMoves[move].type, t1, t2, t3);
	u8 second = MOVE_SECOND_TYPE(move);

	if (second != 0xFF && second != gBattleMoves[move].type)
		mul = (mul * HackAI_TypeMulReal(second, t1, t2, t3)) / TYPE_MUL_NORMAL;
	return mul;
}

//テラスタルすると、相手の技（読み）の弱点が半減以下になるか（＝守りのテラスタル）
bool8 HackAI_TeraBlocksKO(u8 bankAtk, u8 bankDef, u16 foeMove)
{
	u8 tera, moveType;
	u32 now, after;

	if (foeMove == MOVE_NONE || SPLIT(foeMove) == SPLIT_STATUS || IsTerastal(bankAtk))
		return FALSE;

	tera = GetTeraType(bankAtk);
	if (tera >= NUMBER_OF_MON_TYPES || tera == TYPE_STELLAR || tera == TYPE_MYSTERY)
		return FALSE;

	moveType = GetMoveTypeSpecial(bankDef, foeMove);
	//[hack] patches/0916 表の 0（等倍）を 1000 に直してから掛ける（前は等倍のマスで積が 0 になり、「弱点を突かれる」を見落とし、「受けられる」を誤って足していた）
	now = HackAI_TypeMulReal(moveType, gBattleMons[bankAtk].type1, gBattleMons[bankAtk].type2, gBattleMons[bankAtk].type3);
	after = HackAI_TypeMulReal(moveType, tera, tera, tera);
	return now > TYPE_MUL_NORMAL && after < TYPE_MUL_NORMAL;
}

//ダイマックスもテラスタルもできるとき、テラスタルを選ぶか（素は 50% の乱数・battle_controller_opponent.c）
//→ 守りのテラスタルが効く場面だけテラスタル、ほかはダイマックス
bool8 HackAI_PreferTerastal(u8 bankAtk, u8 bankDef)
{
	if (bankAtk == bankDef || !BATTLER_ALIVE(bankDef))
		return FALSE;
	return HackAI_TeraBlocksKO(bankAtk, bankDef, IsValidMovePrediction(bankDef, bankAtk));
}

//技を決めたあとの「ギミック」（メガ・ウルトラバースト・Z・ダイマックス・テラスタル）を使うかの判定（patches/0631）
//→ 上流の OpponentHandleChooseMove（src/battle_controller_opponent.c）にあった判定をそのまま切り出した。通常戦の相手の動きは変わらない。
//→ バトルパレス（src/hoenn_palace.c）は両側（主人公側の自動操作も）これを呼ぶ。gBankTarget に狙う相手を入れてから呼ぶ
void HackAI_ChooseGimmicks(struct ChooseMoveStruct* moveInfo, u8 chosenMoveId, u16 chosenMove)
{
	//You get 1 of 3 of the following gimmicks per Pokemon
	if (moveInfo->possibleZMoves[chosenMoveId]) //Checked first b/c Rayquaza can do all 3
	{
		if (ShouldAIUseZMove(gActiveBattler, gBankTarget, moveInfo->moves[chosenMoveId]))
			gNewBS->zMoveData.toBeUsed[gActiveBattler] = TRUE;
	}
	else if (moveInfo->canMegaEvolve)
	{
		if (!ShouldAIDelayMegaEvolution(gActiveBattler, gBankTarget, chosenMove))
		{
			if (moveInfo->megaVariance != MEGA_VARIANT_ULTRA_BURST)
				gNewBS->megaData.chosen[gActiveBattler] = TRUE;
			else if (moveInfo->megaVariance == MEGA_VARIANT_ULTRA_BURST)
				gNewBS->ultraData.chosen[gActiveBattler] = TRUE;
		}
	}
	else if (moveInfo->possibleMaxMoves[chosenMoveId] && moveInfo->canterastal) //Handles the "Can I Dynamax" checks
	{
		if (ShouldAIDynamax(gActiveBattler, gBankTarget))
		{
			if (ShouldAITerastal(gActiveBattler, gBankTarget) && HackAI_PreferTerastal(gActiveBattler, gBankTarget)) //[hack] patches/0121 素は 50% の乱数
				gNewBS->terastalData.chosen[gActiveBattler] = TRUE;
			else
				gNewBS->dynamaxData.toBeUsed[gActiveBattler] = TRUE;
		}
		else if (ShouldAITerastal(gActiveBattler, gBankTarget))
		{
			gNewBS->terastalData.chosen[gActiveBattler] = TRUE;
		}
	}
	else if (moveInfo->possibleMaxMoves[chosenMoveId]) //Handles the "Can I Dynamax" checks
	{
		if (ShouldAIDynamax(gActiveBattler, gBankTarget))
			gNewBS->dynamaxData.toBeUsed[gActiveBattler] = TRUE;
	}
	else if (moveInfo->canterastal)
	{
		if (ShouldAITerastal(gActiveBattler, gBankTarget))
			gNewBS->terastalData.chosen[gActiveBattler] = TRUE;
	}
}

//HP を回復する道具（キズぐすりなど）を使うか（素は「減った HP が回復量より多い」だけで使う・ai_master.c）
bool8 HackAI_ShouldUseHealItem(u8 bank, u16 healAmount)
{
	u8 foes[2], i;
	bool8 recover = FALSE;

	if (gBattleMons[bank].hp * 2 > gBattleMons[bank].maxHP) //まだ半分より多い
		return FALSE;

	foes[0] = FOE(bank);
	foes[1] = IS_DOUBLE_BATTLE ? PARTNER(FOE(bank)) : FOE(bank);
	for (i = 0; i < 2; ++i)
	{
		if (!BATTLER_ALIVE(foes[i]))
			continue;
		if (CanKnockOutAfterHealing(foes[i], bank, healAmount, 1)) //回復しても次で倒される → 使わずに攻める
			return FALSE;
		if (ShouldRecover(bank, foes[i], 0xFFFF))
			recover = TRUE;
	}

	if (ViableMonCountFromBank(bank) <= 1) //最後の 1 匹
		return TRUE;
	return recover || gBattleMons[bank].hp * 4 < gBattleMons[bank].maxHP;
}

//主に使う攻撃（物理か特殊か）の能力が 2 段以上下がったら交代する（素には無い）
//★ 交代を繰り返さないよう: 出てきたばかりなら交代しない・HP が半分以上・相手を倒せるなら交代しない・半分の確率
bool8 HackAI_ShouldSwitchStatsDropped(void)
{
	u8 bank = gActiveBattler;
	u8 phys = 0, spec = 0, stage, i;
	u8 foe1 = FOE(bank), foe2 = IS_DOUBLE_BATTLE ? PARTNER(FOE(bank)) : FOE(bank);

	if (gNewBS->ai.switchingCooldown[bank] || !AIFlagsHaveStrategy(AI_THINKING_STRUCT->aiFlags))
		return FALSE;
	if (gBattleMons[bank].hp * 2 < gBattleMons[bank].maxHP)
		return FALSE;

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u16 move = GetBattleMonMove(bank, i);
		if (move == MOVE_NONE || gBattleMoves[move].power == 0)
			continue;
		if (CalcMoveSplit(bank, move, foe1) == SPLIT_PHYSICAL)
			++phys;
		else if (CalcMoveSplit(bank, move, foe1) == SPLIT_SPECIAL)
			++spec;
	}
	if (phys == 0 && spec == 0)
		return FALSE;

	stage = gBattleMons[bank].statStages[(phys >= spec) ? STAT_STAGE_ATK : STAT_STAGE_SPATK];
	if (stage > 6 - 2) //6 = 変化なし
		return FALSE;
	if ((BATTLER_ALIVE(foe1) && CanKnockOut(bank, foe1)) || (BATTLER_ALIVE(foe2) && CanKnockOut(bank, foe2)))
		return FALSE;
	if (Random() & 1)
		return FALSE;
	if (GetMostSuitableMonToSwitchInto() >= PARTY_SIZE)
		return FALSE;

	gBattleStruct->switchoutIndex[SIDE(bank)] = PARTY_SIZE;
	EmitTwoReturnValues(1, ACTION_SWITCH, 0);
	return TRUE;
}

/*
 * [hack] patches/0819 ロジックアリーナのロンジャの交代（役割論理）: 相手の場の子の一致技が自分の場の子に効果ばつぐんで、こちらがその場で倒せないとき、
 * 控えの中で「相手の一致技を（相手の一致タイプのどれも）半減以下で受けられ、しかも弱点を突ける技（威力 60 以上）を持つ」子に交代する。
 * ★ 繰り返しの上限: 出てきたばかりの子は交代しない（switchingCooldown）・1 回の戦闘で 3 回まで（回数は HOENN_VAR_YARTIE_OPP の 9〜11 ビット・src/hoenn_yartie.c）・シングルだけ。
 */
bool8 HoennYartie_InBattle(void);
u8 HoennYartie_SwitchCount(void);
void HoennYartie_NoteSwitch(void);
#define YARTIE_MAX_SWITCHES 3

static bool8 YartieStabHits(u8 foe, u8 t1, u8 t2, u8 t3, u32 relation) //relation: 1 = ばつぐん（2 倍をこえる）・0 = 半減以下（どの一致タイプも）
{
	u8 ft[3];
	u32 k;

	ft[0] = gBattleMons[foe].type1;
	ft[1] = gBattleMons[foe].type2;
	ft[2] = gBattleMons[foe].type3;
	for (k = 0; k < 3; ++k)
	{
		u32 mul;

		if (ft[k] >= NUMBER_OF_MON_TYPES || ft[k] == TYPE_MYSTERY || (k > 0 && (ft[k] == ft[0] || (k == 2 && ft[k] == ft[1]))))
			continue;
		mul = HackAI_TypeMulReal(ft[k], t1, t2, t3);   //[hack] patches/0915 等倍のマス（0）で積が 0 になる HackAI_TypeMul をやめた
		if (relation && mul > TYPE_MUL_NORMAL)
			return TRUE;
		if (!relation && mul > TYPE_MUL_NOT_EFFECTIVE)
			return FALSE;
	}
	return !relation;
}

/*
 * [hack] patches/0915 ロンジャの交代の「怖さ」: 相手の場の子の実際の技 4 つ（GetBattleMonMove＝普通の AI と同じ全部の技）を見る。
 * ★ 場の子（自分）への判定は、普通の AI の本物のダメージ計算（GetFinalAIMoveDamage）で、オリジナル技の特殊処理（威力・タイプ・相性の補正）も入る。
 *   「怖い」＝1 つの技で倒される、または最大 HP の半分以上を削られる。
 * ★ 控えの判定は表の type＋type2 の近似（HackAI_MoveMul）。控えには戦闘中の状態（足されるタイプ・威力の補正）が無い。
 *   過小評価が残る: 実行時にタイプが決まる技（ノーマル Lv120＝いちばん効くタイプ）は「受けられない」技として控えの条件から外す（それしか怖い技が無いときは交代しない）。
 *   水 Lv120 のように表の威力が 1 で実際の威力が実行時に決まる技は、場の子の判定（本物の計算）では拾うが、控えの判定は表のタイプだけ。
 */
static bool8 YartieMoveUnresistable(u16 move)
{
	return move == MOVE_LV200_NORMAL_120; //実行時にいちばん効くタイプへ変わる＝どの控えも半減で受けられない
}

//相手の場の子の技の中で、場の子（自分）にとって怖いもの（倒される／最大 HP の半分以上）を scary[] に入れる。戻り値＝その数。*readable＝攻撃技を 1 つでも読めたか
static u32 YartieScaryMoves(u8 foe, u8 bank, u16* scary, bool8* readable)
{
	u32 i, n = 0;

	*readable = FALSE;
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u16 move = GetBattleMonMove(foe, i);
		u32 dmg;

		if (move == MOVE_NONE)
			continue;
		if (gBattleMoves[move].power == 0 && gBattleMoves[Lv200MoveAIBase(move)].power == 0)
			continue;
		*readable = TRUE;
		dmg = GetFinalAIMoveDamage(move, foe, bank, 1, NULL);
		if (dmg >= gBattleMons[bank].hp || dmg * 2 >= gBattleMons[bank].maxHP)
			scary[n++] = move;
	}
	return n;
}

static bool8 YartieResistsMoves(const u16* moves, u32 n, u8 t1, u8 t2)
{
	u32 i;

	for (i = 0; i < n; ++i)
	{
		if (YartieMoveUnresistable(moves[i]))
			return FALSE;
		if (HackAI_MoveMul(moves[i], t1, t2, t1) > TYPE_MUL_NOT_EFFECTIVE)
			return FALSE;
	}
	return TRUE;
}

bool8 HackAI_ShouldSwitchYartie(void)
{
	u8 bank = gActiveBattler, foe = FOE(bank);
	u8 first, last;
	struct Pokemon* party;
	u8 i, best = PARTY_SIZE;
	u16 scary[MAX_MON_MOVES];
	u32 nScary, nResist, k;
	bool8 readable;
	u8 ft1, ft2, ft3;

	if (!HoennYartie_InBattle() || IS_DOUBLE_BATTLE || SIDE(bank) != B_SIDE_OPPONENT)
		return FALSE;
	if (gNewBS->ai.switchingCooldown[bank] || !BATTLER_ALIVE(foe) || HoennYartie_SwitchCount() >= YARTIE_MAX_SWITCHES)
		return FALSE;
	nScary = YartieScaryMoves(foe, bank, scary, &readable);
	if (readable)
	{
		if (nScary == 0)
			return FALSE;   //相手の技は怖くない
	}
	else if (!YartieStabHits(foe, gBattleMons[bank].type1, gBattleMons[bank].type2, gBattleMons[bank].type3, 1))
		return FALSE;   //技が読めないときの代わり（今までの一致技とみなす判定）
	if (CanKnockOut(bank, foe))
		return FALSE;   //その場で倒せるなら撃つ
	//受けられない技（実行時にタイプが決まる技）は控えの条件から外す。残りが無ければ交代しない
	for (k = 0, nResist = 0; k < nScary; ++k)
	{
		if (!YartieMoveUnresistable(scary[k]))
			scary[nResist++] = scary[k];
	}
	if (readable && nResist == 0)
		return FALSE;
	ft1 = gBattleMons[foe].type1;
	ft2 = gBattleMons[foe].type2;
	ft3 = gBattleMons[foe].type3;
	Lv200CustomMove_AdjustTypes(foe, &ft1, &ft2, &ft3);   //足されるタイプ・封印されたタイプも防御の相性に入れる（場の子どうし）
	if (IS_BLANK_TYPE(ft3))
		ft3 = ft1;
	party = LoadPartyRange(bank, &first, &last);
	for (i = first; i < last; ++i)
	{
		u16 sp = GetMonData(&party[i], MON_DATA_SPECIES2, NULL);
		u32 m;
		bool8 hits = FALSE;

		if (party[i].hp == 0 || sp == SPECIES_NONE || GetMonData(&party[i], MON_DATA_IS_EGG, NULL) || i == gBattlerPartyIndexes[bank])
			continue;
		if (readable)
		{
			if (!YartieResistsMoves(scary, nResist, gBaseStats[sp].type1, gBaseStats[sp].type2))
				continue;   //相手の怖い技を半減以下で受けられない
		}
		else if (!YartieStabHits(foe, gBaseStats[sp].type1, gBaseStats[sp].type2, gBaseStats[sp].type2, 0))
			continue;   //相手の一致技を半減以下で受けられない
		for (m = 0; m < MAX_MON_MOVES && !hits; ++m)
		{
			u16 move = GetMonData(&party[i], MON_DATA_MOVE1 + m, NULL);

			if (move != MOVE_NONE && MOVE_POWER(move) >= 60 && gBattleMoves[move].accuracy != 0 && gBattleMoves[move].accuracy < 70)
				continue;
			if (move != MOVE_NONE && MOVE_POWER(move) >= 60 && HackAI_MoveMul(move, ft1, ft2, ft3) > TYPE_MUL_NORMAL)
				hits = TRUE;
		}
		if (hits)
		{
			best = i;
			break;
		}
	}
	if (best >= PARTY_SIZE)
		return FALSE;

	HoennYartie_NoteSwitch();
	gBattleStruct->switchoutIndex[SIDE(bank)] = best;
	EmitTwoReturnValues(1, ACTION_SWITCH, 0);
	return TRUE;
}

//交代先の点数: 場に出たときの 2 つ目の特性（いかく系・天気・フィールド）があれば +1（src/second_ability.c）
s8 HackAI_SecondAbilitySwitchBonus(struct Pokemon* mon)
{
	u16 ability;

	if (GetMonData(mon, MON_DATA_LEVEL, NULL) < HACK_SECOND_ABILITY_LEVEL)
		return 0;
	ability = SecondAbility_OfMon(mon);
	if (ability == ABILITY_NONE || ability == GetMonAbility(mon))
		return 0;

	switch (ability)
	{
		case ABILITY_INTIMIDATE:
		case ABILITY_SCARE:
		case ABILITY_MIWAKUNOUROKO:   //★ 2026-10-11 ミワクノウロコ
		case ABILITY_DRIZZLE:
		case ABILITY_SUN_AVATAR:   //★ 2026-10-11 たいようのけしん＝ひでり
		case ABILITY_DROUGHT:
		case ABILITY_SANDSTREAM:
		case ABILITY_SNOWWARNING:
		case ABILITY_ELECTRICSURGE:
		case ABILITY_PSYCHICSURGE:
		case ABILITY_MISTYSURGE:
		case ABILITY_GRASSYSURGE:
			return 1;
	}
	return 0;
}
