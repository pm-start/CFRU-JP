#include "defines.h"
#include "defines_battle.h"
#include "../include/constants/abilities.h"
#include "../include/new/battle_util.h"

/*
    ウォームアップ（ABILITY_WARMUP 327）・しごとのむし（ABILITY_WORKBUG 328）（2026-10-17・ユーザー決定）。
    どちらも本家には無い自作の特性で、「2 つ目（場に出たとき）」の表（tools/abilities/second_ability_sheet.json）に入る。
      ・ウォームアップ: 持ち主がかくとうタイプの技を出すとき 威力 1.3 倍
      ・しごとのむし: 持ち主がむしタイプの技を出すとき 威力 1.5 倍
    ★ 2 つ目の特性の仕組み（second_ability.c）は「場に出たときの効果だけ」を動かす。この 2 つは出たときの効果が無く、
       技を出すたびに効く特性なので、威力の計算（damage_calc.c の AdjustBasePower）から毎回ここを呼ぶ。
       1 つ目の特性としても同じに効く（ものまね・なりきり・トレースで渡ったとき）。
    状態は持たない（6.13）・セーブの形は変えない。
*/

u16 SecondAbility_OfBank(u8 bank);   //src/second_ability.c（Lv50 未満・特性が封じられていれば NONE・1 つ目と同じなら NONE）

static bool8 HasAbility(u8 bank, u16 ability)
{
	return ABILITY(bank) == ability || SecondAbility_OfBank(bank) == ability;
}

u16 HackWarmupWorkbug_AdjustPower(u8 bank, u8 moveType, u16 power)
{
	if (moveType == TYPE_FIGHTING && HasAbility(bank, ABILITY_WARMUP))
		power = (power * 13) / 10;
	if (moveType == TYPE_BUG && HasAbility(bank, ABILITY_WORKBUG))
		power = (power * 15) / 10;
	return power;
}
