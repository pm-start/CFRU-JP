#include "defines.h"
#include "defines_battle.h"
#include "../include/random.h"
#include "../include/constants/moves.h"
#include "../include/new/battle_script_util.h"
#include "../include/new/battle_util.h"
#include "../include/new/util.h"
#include "shadow_moves.h"
#include "hack_kind.h"

/*
 * shadow_moves.c — ダーク技 18 個の戦いの処理（段 D・patches/0955）。キット（koro 氏・ポケモン XD の仕組み）の shadow_battle.c の技・ダークの空の部分を
 * 本流の形（EFFECT_HACK_CUSTOM・新タイプ TYPE_SHADOW・自作名）に合わせて移した。心のゲージ・リバース・よびかける・技の組み立ては段 E 以降。
 */

extern const u8 BattleScript_ShadowSkyContinues[];
extern const u8 BattleScript_ShadowSkyEnds[];
extern const u8 BattleScript_ShadowSkyDamage[];
extern const u8 BattleScript_ShadowFailed[];
extern const u8 BattleScript_ShadowHalfDone[];
extern const u8 BattleScript_ShadowEndHit[];
extern const u8 BattleScript_ShadowSkyMove[];
extern const u8 BattleScript_ShadowShedMove[];
extern const u8 BattleScript_ShadowHalfMove[];
extern const u8 gText_ShadowSkyContinues[];
extern const u8 gText_ShadowSkyEnds[];
extern const u8 gText_ShadowSkyDamage[];
extern const u8 gText_ShadowSkyStarts[];
extern const u8 gText_ShadowShedDone[];

bool8 IsShadowMove(u16 move)
{
	return move >= FIRST_SHADOW_MOVE && move <= LAST_SHADOW_MOVE;
}

//「ダーク状態の個体か」の窓口（1 か所）。個体の印（段 A1・src/hack_kind.h）の種類がダークなら TRUE ＝ ダーク技が いまひとつ（×0.5）になる（段 E・patches/0963）
bool8 Shadow_IsMonDark(unusedArg struct Pokemon* mon)
{
	return mon != NULL && HackKind_Get((const struct BoxPokemon*) mon) == HACK_KIND_DARK;
}

bool8 Shadow_IsBattlerDark(u8 bank)
{
	if (bank >= gBattlersCount)
		return FALSE;

	return Shadow_IsMonDark(GetBankPartyData(bank));
}

u32 Shadow_MoveMultiplier(bool8 targetIsDark)
{
	return targetIsDark ? TYPE_MUL_NOT_EFFECTIVE : TYPE_MUL_SUPER_EFFECTIVE;
}

//ダークエンドの反動: 自分の残り HP の半分（cmd49.c の ATK49_MOVE_RECOIL・いしあたまでは防げない）
u32 Shadow_EndRecoil(u8 bank)
{
	return gBattleMons[bank].hp / 2;
}

//EFFECT_HACK_CUSTOM（252）の入口から呼ばれる。ダーク技でなければ NULL（A・B の台本を探す側へ）
const u8* Shadow_GetScript(u16 move)
{
	switch (move)
	{
		case MOVE_YAMITOTSUGEKI:
			return BattleScript_ShadowEndHit;
		case MOVE_YAMINOSORA:
			return BattleScript_ShadowSkyMove;
		case MOVE_YAMINOUZU:
			return BattleScript_ShadowShedMove;
		case MOVE_YAMINOHANBUN:
			return BattleScript_ShadowHalfMove;
	}

	return NULL;
}

//---- ダークの空 --------------------------------------------------------------------------------------------------------

//ET_Weather: 5 ターンで終わる
bool8 Shadow_EndTurnShadowSky(void)
{
	if (!(gBattleWeather & WEATHER_SHADOW_SKY))
		return FALSE;

	if (--gWishFutureKnock.weatherDuration == 0)
	{
		gBattleWeather &= ~WEATHER_SHADOW_SKY;
		gBattleStringLoader = gText_ShadowSkyEnds;
		BattleScriptExecute(BattleScript_ShadowSkyEnds);
	}
	else
	{
		gBattleStringLoader = gText_ShadowSkyContinues;
		BattleScriptExecute(BattleScript_ShadowSkyContinues);
	}

	return TRUE;
}

//ET_Weather_Damage_Health_Abilities: ダークでない子は最大 HP の 1/16 を失う（マジックガードは受けない）
bool8 Shadow_EndTurnShadowSkyDamage(u8 bank)
{
	if (!(gBattleWeather & WEATHER_SHADOW_SKY) || !WEATHER_HAS_EFFECT || !BATTLER_ALIVE(bank)
	|| Shadow_IsBattlerDark(bank) || BankHasAbility(bank, ABILITY_MAGICGUARD))
		return FALSE;

	gBattleMoveDamage = MathMax(1, gBattleMons[bank].maxHP / 16);
	gBankAttacker = bank;
	gBattleStringLoader = gText_ShadowSkyDamage;
	BattleScriptExecute(BattleScript_ShadowSkyDamage);
	return TRUE;
}

//---- 特殊 3 技の台本から callasm で呼ばれる -----------------------------------------------------------------------------------

//ダークウェザー: すでにダークの空・ゲンシ系の天気・サーカスの天気のときは失敗
void Shadow_TrySetShadowSky(void)
{
	if (gBattleWeather & (WEATHER_SHADOW_SKY | WEATHER_PRIMAL_ANY | WEATHER_CIRCUS))
	{
		gBattlescriptCurrInstr = BattleScript_ShadowFailed - 5;
		return;
	}

	gBattleWeather = WEATHER_SHADOW_SKY;
	gWishFutureKnock.weatherDuration = 5;
	gBattleStringLoader = gText_ShadowSkyStarts;
}

//ダークリムーブ: リフレクター・ひかりのかべ（オーロラベール）・しんぴのまもりを両側で消す
void Shadow_RemoveScreensBothSides(void)
{
	u32 side;
	bool8 removed = FALSE;

	for (side = 0; side < NUM_BATTLE_SIDES; ++side)
	{
		if (gSideTimers[side].reflectTimer || gSideTimers[side].lightscreenTimer || gNewBS->AuroraVeilTimers[side]
		|| gSideStatuses[side] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_SAFEGUARD))
		{
			RemoveScreensFromSide(side);
			gSideStatuses[side] &= ~SIDE_STATUS_SAFEGUARD;
			gSideTimers[side].safeguardTimer = 0;
			removed = TRUE;
		}
	}

	if (!removed)
		gBattlescriptCurrInstr = BattleScript_ShadowFailed - 5;
	else
		gBattleStringLoader = gText_ShadowShedDone;
}

//ダークハーフ: 場の全員（自分・味方も）が残り HP の半分を失う（HP 1 は 1 のまま・まもるで防げる）。1 回の呼び出しで 1 匹・終わったら BattleScript_ShadowHalfDone
void Shadow_HalfNext(void)
{
	while (gNewBS->shadowHalfNext < gBattlersCount)
	{
		u8 bank = gNewBS->shadowHalfNext++;

		if (!BATTLER_ALIVE(bank) || gBattleMons[bank].hp <= 1
		|| (bank != gBankAttacker && gProtectStructs[bank].protected))
			continue;

		gBankTarget = bank;
		gBattleMoveDamage = gBattleMons[bank].hp / 2;
		return; //台本が gBankTarget の HP 枠を更新してもう一度呼ぶ
	}

	gNewBS->shadowHalfNext = 0;
	gBankTarget = gBankAttacker;
	gBattlescriptCurrInstr = BattleScript_ShadowHalfDone - 5;
}
