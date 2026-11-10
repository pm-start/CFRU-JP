#include "defines.h"
#include "defines_battle.h"
#include "../include/random.h"
#include "../include/constants/abilities.h"
#include "../include/constants/hold_effects.h"
#include "../include/new/battle_util.h"
#include "../include/new/move_tables.h"
#include "../include/new/util.h"

/*
    想像ポケモン イワオロチ・ゲンシガチゴラス（2026-10-25・Redmine #93・#94・patches/0800〜0803）の特性の戦いの処理。
      ・ほむらのあご（ABILITY_HOMURANOAGO 331・ゲンシガチゴラス）: かむわざ 1.5 倍（damage_calc.c の ストロングジョー の case）と、
        かむわざを当てた相手を 30% でやけど（HackFakemon_TryHomuraBurn・cmd49.c の ATK49_HOMURA_BURN から・ふつうの技の台本は新しい技ほど後ろの段まで通る）
      ・はくあのおう（ABILITY_HAKUANOOU 332・ゲンシガチゴラスの 2 つ目）: いわタイプの技もタイプ一致（HackFakemon_StabTypes・damage_calc.c の 4 か所）と、
        いわタイプの技の命中 1.3 倍（accuracy_calc.c）
      ・げきこう（ABILITY_GEKIKOU 330・イワオロチの かくれ特性）は ability_battle_effects.c（ごうじょう型の ABILITYEFFECT_CONTACT）
    ★ ほむらのあごの やけどは「追加の 30% の判定」。もとのやけどの追加効果を持つかむわざ（フロストバイト・ブラックバイト）は技の判定と独立の 2 回判定（0.61.0・ユーザー決定）。
    状態は持たない（6.13）・セーブの形は変えない。
*/

u16 SecondAbility_OfBank(u8 bank);   //src/second_ability.c（Lv50 未満・特性が封じられていれば NONE・1 つ目と同じなら NONE）

static bool8 HasAbility(u8 bank, u16 ability)
{
	return ABILITY(bank) == ability || SecondAbility_OfBank(bank) == ability;
}

//はくあのおう: 技を出す側のタイプに いわ を足す（3 つ目の枠が空いていれば 3 つ目・埋まっていれば何もしない）
void HackFakemon_StabTypes(u8 bank, u8* type1, u8* type2, u8* type3)
{
	if (*type1 == TYPE_ROCK || *type2 == TYPE_ROCK || *type3 == TYPE_ROCK)
		return;
	if (!HasAbility(bank, ABILITY_HAKUANOOU))
		return;
	if (IS_BLANK_TYPE(*type3))
		*type3 = TYPE_ROCK;
	else if (*type2 == *type1)
		*type2 = TYPE_ROCK;
}

//ほむらのあご: かむわざで ダメージを与えたあと、30% で相手をやけど。台本を積んだら TRUE
bool8 HackFakemon_TryHomuraBurn(void)
{
	u8 def = gBankTarget;

	if (!HasAbility(gBankAttacker, ABILITY_HOMURANOAGO)
	|| !CheckTableForMove(gCurrentMove, gBitingMoves)
	|| !MOVE_HAD_EFFECT
	|| gBankAttacker == def
	|| !BATTLER_ALIVE(def)
	|| !TOOK_DAMAGE(def)
	|| SheerForceCheck()
	|| HasAbility(def, ABILITY_SHIELDDUST)
	|| ITEM_EFFECT(def) == ITEM_EFFECT_COVERT_CLOAK
	|| MoveBlockedBySubstitute(gCurrentMove, gBankAttacker, def)
	|| (BankSideHasSafeguard(def) && !HasAbility(gBankAttacker, ABILITY_INFILTRATOR))
	|| !CanBeBurned(def, gBankAttacker, TRUE))
		return FALSE;

	if (Random() % 100 >= 30)
		return FALSE;

	return TRUE;
}
