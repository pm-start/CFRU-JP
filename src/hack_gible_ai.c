#include "defines.h"
#include "hack_move_alias.h"
#include "defines_battle.h"
#include "../include/random.h"
#include "../include/constants/items.h"
#include "../include/new/ai_util.h"
#include "../include/new/battle_util.h"
#include "../include/new/damage_calc.h"
#include "../include/new/dynamax.h"
#include "hack_gible_ai_core.h"

/*
 * ★ 2026-10-23 FEAT-gible-reito-double-action（Redmine #58 とくべつなガブリアス 段階 2・patches/0792）
 *   1) れーとのおうの子は 1 ターンに 2 回動く（1 回目の直後に続けて 2 回目・同じ技でもよい）
 *      形は TrySetupRaidBossRepeatedAttack（battle_start_turn_start.c）と同じ。呼ぶのは HackGibleAI_TryDouble（RunTurnActionsFunctions の中）
 *   2) 2 回目の技と対象は専用の AI が決める（点数の本体は hack_gible_ai_core.h の純関数＝ホストで単体テスト）
 *   3) 行動を止められる場面（ひるみ・まひ・こんらん・メロメロ・ねむり・こおり）を 25% で振り払う（HackGibleAI_TryResist）
 * 決定性（通信対戦で両機が同じ結果になる）:
 *   - AI は Random() を使わない。ダメージの見積もり（AI_CalcDmg は内部で Random() を呼ぶ）は gRngValue を保存・固定して呼び、必ず元に戻す＝消費ゼロ
 *   - 見るのは場に出ている情報と、すでに使われた技（gLastUsedMoves）だけ。このターンに相手が選んだだけの技は見ない
 *   - 同点は技の並び順
 *   - 振り払いの乱数は各判定で Random() をちょうど 1 回（止められる条件が成り立ったときだけ・順は attackcanceler の並び）
 * 対象: れーとのおうの特性の子（ガブリアス・メガ・メガZ）。主人公側（マルチバトルの味方・レイドの相方は除く）と、通信対戦の相手側。
 */

#define gRngValue (*(vu32*) 0x3005040)   /* Random() が読み書く乱数の状態（BPRJ の 0x804448C を読んで確認・リンカーに名前が無い） */
#define GIBLE_RESIST_PERCENT 25   /* 振り払う確率（ユーザー決定 2026-10-23 q179） */

extern u8 BattleScript_GibleDoubleAction[];
void HackGibleSwitch_PrepareSecond(u8 bank, u16 move);

static bool8 IsGibleSpecies(u16 species)
{
	return species == SPECIES_GARCHOMP || species == SPECIES_GARCHOMP_MEGA || species == SPECIES_GARCHOMP_MEGA_Z;
}

bool8 HackGibleAI_IsReito(u8 bank)
{
	if (bank >= gBattlersCount || !IsGibleSpecies(gBattleMons[bank].species) || ABILITY(bank) != ABILITY_REITONOOU)
		return FALSE;
	if (gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_TOWER_LINK_MULTI | BATTLE_TYPE_TWO_OPPONENTS))
		return FALSE;
	if (SIDE(bank) == B_SIDE_PLAYER)
	{
		if ((gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) && GetBattlerPosition(bank) == B_POSITION_PLAYER_RIGHT)
			return FALSE;   //マルチバトルの味方
		return !IsRaidBattle();
	}
	return (gBattleTypeFlags & BATTLE_TYPE_LINK) != 0;   //通信対戦の相手の子は、相手機の側でも同じく 2 回動く
}

/* ---- 振り払い（attackcanceler.c から）---- */
bool8 HackGibleAI_TryResist(u8 bank)
{
	if (!HackGibleAI_IsReito(bank))
		return FALSE;
	return Random() % 100 < GIBLE_RESIST_PERCENT;   //止められる条件が成り立ったときだけ呼ばれる＝呼ぶ回数と順は両機で同じ
}

/* ---- 乱数を使わないダメージの見積もり ---- */
static u16 EstimateMinDamage(u16 move, u8 bankAtk, u8 bankDef)
{
	u32 savedRng = gRngValue, seed = 0x5EED, dmg;
	u16 realHp = gBattleMons[bankDef].hp;
	u8 i;

	for (i = 0; i < 16; ++i)   //最初の Random() が奇数（クリティカルの 50% 判定が当たらない）になる種を探す
	{
		gRngValue = seed;
		if (Random() & 1)
			break;
		seed += 0x101;
	}
	gRngValue = seed;   //探した種から本番（決まった値）
	dmg = CalcFinalAIMoveDamage(move, bankAtk, bankDef, 1, NULL);
	if (dmg >= realHp && realHp != 0 && gBattleMons[bankDef].maxHP < 0xFFFF)
	{
		/* CalcFinalAIMoveDamage は結果を守る側の今の HP で切る（倒せるときは HP そのものが返る）。
		   そのまま 85/93 倍すると「倒される脅威」が今の HP を下回って危険と見なされないので、切られないように HP を最大+1 にして測り直す
		   （最大 HP ちょうどでなければ、マルチスケイル系の半減や HP 条件の技の威力変化は入らない） */
		u32 uncapped;
		gRngValue = seed;
		gBattleMons[bankDef].hp = gBattleMons[bankDef].maxHP + 1;
		uncapped = CalcFinalAIMoveDamage(move, bankAtk, bankDef, 1, NULL);
		gBattleMons[bankDef].hp = realHp;
		if (uncapped > dmg)
			dmg = uncapped;
	}
	gRngValue = savedRng;   //消費ゼロ
	if (dmg >= gBattleMons[bankDef].maxHP)
		return gBattleMons[bankDef].maxHP;   //最大 HP で切られた＝どの HP でも倒せる（85/93 倍すると、満タンの相手を倒せない見積もりになる）
	return (u16) (dmg * 85 / 93);   //AI_CalcDmg は 93% の目安。最低ダメージ（85%）に直す
}

static bool8 IsBadForSecond(u16 move)
{
	switch (gBattleMoves[move].effect) {
		case EFFECT_RAZOR_WIND: case EFFECT_SKY_ATTACK: case EFFECT_SOLARBEAM: case EFFECT_SKULL_BASH: case EFFECT_SEMI_INVULNERABLE:
		case EFFECT_SKY_DROP: case EFFECT_RECHARGE: case EFFECT_FAKE_OUT: case EFFECT_FOCUS_PUNCH: case EFFECT_EXPLOSION:
		case EFFECT_MEMENTO: case EFFECT_BATON_PASS: case EFFECT_SUCKER_PUNCH: case EFFECT_FUTURE_SIGHT: case EFFECT_RAMPAGE:
		case EFFECT_ROLLOUT: case EFFECT_BIDE: case EFFECT_TELEPORT: case EFFECT_UPROAR: case EFFECT_LAST_RESORT:
			return TRUE;
	}
	switch (EffectiveMove(move)) {   //新技の別名も効果元の技で見る（hack_move_alias.h）
		case MOVE_UTURN: case MOVE_VOLTSWITCH: case MOVE_FLIPTURN: case MOVE_PARTINGSHOT: case MOVE_SHEDTAIL: case MOVE_CHILLYRECEPTION:
			return TRUE;
	}
	return FALSE;
}

static u8 KindOfMove(u16 move)
{
	if (gBattleMoves[move].power != 0)
		return GIBLE_KIND_DAMAGE;
	switch (gBattleMoves[move].effect) {
		case EFFECT_RESTORE_HP: case EFFECT_MORNING_SUN: case EFFECT_REST: return GIBLE_KIND_HEAL;
		case EFFECT_PROTECT: return GIBLE_KIND_PROTECT;
		case EFFECT_ATTACK_UP_2: case EFFECT_SPECIAL_ATTACK_UP_2: case EFFECT_SPEED_UP_2: case EFFECT_BULK_UP: case EFFECT_CALM_MIND:
		case EFFECT_DRAGON_DANCE: case EFFECT_ATTACK_UP: case EFFECT_SPECIAL_ATTACK_UP: case EFFECT_COSMIC_POWER: case EFFECT_EXTREME_EVOBOOST:
			return GIBLE_KIND_SETUP;
		case EFFECT_SLEEP: case EFFECT_TOXIC: case EFFECT_POISON: case EFFECT_PARALYZE: case EFFECT_WILL_O_WISP:
			return GIBLE_KIND_STATUS;
	}
	return GIBLE_KIND_OTHER;
}

static bool8 StatusWorks(u16 move, u8 atk, u8 def)
{
	if (gBattleMons[def].status1 != 0 || MoveBlockedBySubstitute(move, atk, def))
		return FALSE;
	switch (gBattleMoves[move].effect) {
		case EFFECT_SLEEP: return CanBePutToSleep(def, atk, TRUE);
		case EFFECT_TOXIC: case EFFECT_POISON: return CanBePoisoned(def, atk, TRUE);
		case EFFECT_PARALYZE: return CanBeParalyzed(def, atk, TRUE);
		case EFFECT_WILL_O_WISP: return CanBeBurned(def, atk, TRUE);
	}
	return FALSE;
}

static u16 HealAmount(u8 bank)
{
	u16 room = gBattleMons[bank].maxHP - gBattleMons[bank].hp;
	if (IsHealBlocked(bank))
		return 0;
	return room > gBattleMons[bank].maxHP / 2 ? gBattleMons[bank].maxHP / 2 : room;
}

/* ---- 2 回目 ---- */
static bool8 FirstActionAllowsSecond(u8 bank)
{
	if (gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE)
		return FALSE;   //まひ・ねむり・ひるみなど
	if (!BATTLER_ALIVE(bank))
		return FALSE;   //倒れた
	if (gBattleMons[bank].status2 & (STATUS2_MULTIPLETURNS | STATUS2_RECHARGE))
		return FALSE;   //溜め技・ロック技・反動
	if (IsBadForSecond(gCurrentMove) || gProtectStructs[bank].onlyStruggle)
		return FALSE;   //交代技など
	if (CountAliveMonsInBattle(BATTLE_ALIVE_DEF_SIDE, bank, FOE(bank)) == 0)
		return FALSE;   //相手が全員倒れた（シングルで倒したときも 2 回目はしない）
	return TRUE;
}

/* 行動が終わった直後（ACTION_FINISHED）に呼ぶ。2 回目を仕込んだら TRUE。savedActionFuncId は TrySetupRaidBossRepeatedAttack と同じ */
bool8 HackGibleAI_TryDouble(u8 actionFuncId)
{
	u8 bank, tgtList[2], nTgt, t, pos, k, limit;
	struct GibleCand cands[MAX_MON_MOVES * 2];
	struct GibleCtx ctxs[MAX_MON_MOVES * 2];
	u8 posOf[MAX_MON_MOVES * 2], tgtOf[MAX_MON_MOVES * 2];
	u8 n = 0, firstTarget, firstPos;
	u16 firstMove, est;
	int pick;

	if (actionFuncId != ACTION_FINISHED || gCurrentTurnActionNumber == 0)
		return FALSE;
	bank = gBanksByTurnOrder[gCurrentTurnActionNumber - 1];
	if (!HackGibleAI_IsReito(bank) || (gNewBS->hackGibleDone & gBitTable[bank]))
		return FALSE;
	if ((gBattleStruct->field_91 & gBitTable[bank]) || (gNewBS->commandingDondozo & gBitTable[bank]))
		return FALSE;
	gBankAttacker = bank;
	if (!FirstActionAllowsSecond(bank))
		return FALSE;

	firstTarget = gBankTarget;
	firstPos = gCurrMovePos;
	firstMove = gCurrentMove;
	est = EstimateMinDamage(firstMove, bank, firstTarget);

	limit = CheckMoveLimitations(bank, 0, 0xFF);
	nTgt = 0;
	if (BATTLER_ALIVE(FOE(bank)))
		tgtList[nTgt++] = FOE(bank);
	if (IS_DOUBLE_BATTLE && BATTLER_ALIVE(PARTNER(FOE(bank))))
		tgtList[nTgt++] = PARTNER(FOE(bank));
	if (nTgt == 0)
		return FALSE;

	for (pos = 0; pos < MAX_MON_MOVES; ++pos)
	{
		u16 move = gBattleMons[bank].moves[pos];
		if (move == MOVE_NONE)
			continue;
		for (t = 0; t < nTgt; ++t)
		{
			u8 def = tgtList[t];
			struct GibleCand* c = &cands[n];
			struct GibleCtx* x = &ctxs[n];
			u32 given = gNewBS->totalDamageGiven;

			c->usable = !(gBitTable[pos] & limit) && !IsBadForSecond(move);
			c->kind = KindOfMove(move);
			c->pos = pos;
			c->target = def;
			c->power = MOVE_POWER(move);
			c->priority = gBattleMoves[move].priority;
			c->accuracy = gBattleMoves[move].accuracy ? gBattleMoves[move].accuracy : 100;
			c->immune = 0; c->minDmg = 0; c->allyDmg = 0; c->healAmount = 0; c->statusOk = 0;
			if (c->kind == GIBLE_KIND_DAMAGE)
			{
				c->minDmg = EstimateMinDamage(move, bank, def);
				c->immune = (c->minDmg == 0);
				if (IS_DOUBLE_BATTLE && (gBattleMoves[move].target & MOVE_TARGET_FOES_AND_ALLY) && BATTLER_ALIVE(PARTNER(bank)))
					c->allyDmg = EstimateMinDamage(move, bank, PARTNER(bank));   //味方が無効なら 0＝差し引かない
			}
			else if (c->kind == GIBLE_KIND_HEAL)
				c->healAmount = HealAmount(bank);
			else if (c->kind == GIBLE_KIND_STATUS)
				c->statusOk = StatusWorks(move, bank, def);

			x->myHp = gBattleMons[bank].hp; x->myMaxHp = gBattleMons[bank].maxHP;
			x->foeHp = gBattleMons[def].hp; x->foeMaxHp = gBattleMons[def].maxHP;
			x->foeThreat = 0;
			k = def;   //相手が すでに使った技（gLastUsedMoves）だけを見る。このターンに選んだだけの技は見ない
			if (gLastUsedMoves[k] != MOVE_NONE && SPLIT(gLastUsedMoves[k]) != SPLIT_STATUS)
				x->foeThreat = EstimateMinDamage(gLastUsedMoves[k], k, bank);
			x->iAmFaster = MoveWouldHitFirst(move, bank, def);
			x->isDouble = IS_DOUBLE_BATTLE;
			x->setupStage = gBattleMons[bank].statStages[STAT_STAGE_ATK - 1];
			x->firstWasProtect = (gBattleMoves[firstMove].effect == EFFECT_PROTECT);
			x->hadFirst = 1;
			x->firstPos = firstPos;
			x->firstTarget = firstTarget;
			x->firstRatioPct = est ? (given * 100 / est > 100 ? 100 : given * 100 / est) : 100;
			x->takenPct = (gNewBS->hackGibleHpStart[bank] > gBattleMons[bank].hp) ? (gNewBS->hackGibleHpStart[bank] - gBattleMons[bank].hp) * 100 / gBattleMons[bank].maxHP : 0;
			x->firstKilledFoe = (gBattleMons[firstTarget].hp == 0);
			posOf[n] = pos; tgtOf[n] = def;
			++n;
		}
	}
	pick = GibleAI_Pick(cands, n, ctxs);
	if (pick < 0)
		return FALSE;   //使える技がない＝2 回目はしない

	gNewBS->hackGibleDone |= gBitTable[bank];
	HackGibleSwitch_PrepareSecond(bank, gBattleMons[bank].moves[posOf[pick]]);   //[hack] patches/0793 2 回目の前にも姿を切り替える判断（2 回目の行動の頭の Mega_Check が行う）
	gNewBS->abilityPopupOverwrite = ABILITY_REITONOOU;
	gBattleScripting.bank = bank;
	gCurrentActionFuncId = gActionsByTurnOrder[--gCurrentTurnActionNumber] = ACTION_USE_MOVE;
	gBattleStruct->chosenMovePositions[bank] = posOf[pick];
	gCurrentMove = gBattleMons[bank].moves[posOf[pick]];
	gBattleStruct->moveTarget[bank] = tgtOf[pick];
	gBankTarget = tgtOf[pick];
	gHitMarker &= ~(HITMARKER_NO_ATTACKSTRING);
	gHitMarker &= ~(HITMARKER_UNABLE_TO_USE_MOVE);
	return TRUE;
}

/* ターンの始まりに呼ぶ（段階 3 取り込み後に配線） */
void HackGibleAI_TurnStart(void)
{
	u8 i;
	gNewBS->hackGibleDone = 0;
	for (i = 0; i < gBattlersCount; ++i)
		gNewBS->hackGibleHpStart[i] = gBattleMons[i].hp;
}
