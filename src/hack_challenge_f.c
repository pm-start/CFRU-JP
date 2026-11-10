#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/constants/items.h"
#include "hack_challenge.h"
#include "hack_challenge_f.h"
#include "hack_dummy_battle.h"

/*
 * 縛り F 6 本の 共通の部分（0.63.0・docs/research/2026-11-03_challenge-f.md・旗の台帳 docs/research/flag-ledger.md）。
 *   ・旗を読む入口（On／CountOn／各縛りの Get）と ローテーションの番号（旗の 3 ビット）
 *   ・さかさバトル（担当 A）: 戦闘に入るとき FLAG_INVERSE を立てる（戦闘が終わると end_battle.c の gEndBattleFlagClearTable が下ろす）
 *   ・しばりボーナス（担当 A）: トレーナー戦の賞金 +10%×(ほかに ON の しばりの数)・バッジを取るたびに ON の数に応じた道具
 *   めかくしバトル（担当 B）・いっぱつしょうぶ／ローテーション／わざガチャ（担当 C）の本体は それぞれの .c（このファイルは 旗の読み書きだけ受け持つ）
 *
 * ★ CFRU 6.13: 書き込める static／.bss は ROM に置かれ 書き込みが捨てられる（0.62.6 sScriptedWild）。このファイルの状態は 全部 旗（セーブされる）か 台本の変数 0x8004 だけ。
 *   static const の表は 読むだけなので問題ない。
 */

u8 GetAllBadgeCount(void);   //src/badge_count.c（カントー・ホウエン・ジョウト・シンオウの合計）
bool8 HackMomSavings_AddPcItem(u16 item);   //src/hack_mom_savings.c（バッグが いっぱいなら パソコンへ）
extern const u8 EventScript_ChallengeBonus[];

bool8 HackChallengeF_Inverse(void)
{
	return FlagGet(HACK_FLAG_CHF_INVERSE) != 0;
}

bool8 HackChallengeF_Blind(void)
{
	return FlagGet(HACK_FLAG_CHF_BLIND) != 0;
}

bool8 HackChallengeF_Oneshot(void)
{
	return FlagGet(HACK_FLAG_CHF_ONESHOT) != 0;
}

bool8 HackChallengeF_Rotation(void)
{
	return FlagGet(HACK_FLAG_CHF_ROTATION) != 0;
}

bool8 HackChallengeF_Gacha(void)
{
	return FlagGet(HACK_FLAG_CHF_GACHA) != 0;
}

u8 HackChallengeF_CountOn(void)
{
	u8 n = 0;

	if (FlagGet(HACK_FLAG_CHF_INVERSE))
		++n;
	if (FlagGet(HACK_FLAG_CHF_BONUS))
		++n;
	if (FlagGet(HACK_FLAG_CHF_BLIND))
		++n;
	if (FlagGet(HACK_FLAG_CHF_ONESHOT))
		++n;
	if (FlagGet(HACK_FLAG_CHF_ROTATION))
		++n;
	if (FlagGet(HACK_FLAG_CHF_GACHA))
		++n;
	return n;
}

bool8 HackChallengeF_On(void)
{
	return HackChallengeF_CountOn() != 0;
}

/* ---- ローテーションの番号（旗の 3 ビット・保存枠は使わない）---- */

u8 HackChallengeF_RotationGet(void)
{
	u8 v = 0;

	if (FlagGet(HACK_FLAG_CHF_ROT_BIT0))
		v |= 1;
	if (FlagGet(HACK_FLAG_CHF_ROT_BIT1))
		v |= 2;
	if (FlagGet(HACK_FLAG_CHF_ROT_BIT2))
		v |= 4;
	return v;
}

void HackChallengeF_RotationSet(u8 index)
{
	if (index & 1)
		FlagSet(HACK_FLAG_CHF_ROT_BIT0);
	else
		FlagClear(HACK_FLAG_CHF_ROT_BIT0);
	if (index & 2)
		FlagSet(HACK_FLAG_CHF_ROT_BIT1);
	else
		FlagClear(HACK_FLAG_CHF_ROT_BIT1);
	if (index & 4)
		FlagSet(HACK_FLAG_CHF_ROT_BIT2);
	else
		FlagClear(HACK_FLAG_CHF_ROT_BIT2);
}

/* ---- さかさバトル ---- */

//challenge_c_hooks.s の HackChallengeC_BattleStartHook（素の CreateBattleStartTask の入口＝野生・トレーナー・ジム・四天王・台本の戦いの全部が通る）から。
//通信・フロンティアなどの施設（FLAG_BATTLE_FACILITY・BATTLE_TYPE_FRONTIER）・ポケセン 3F のダミー・オーキドのチュートリアルは 対象外。
//立てた旗は 戦闘が終わる所（end_battle.c の gEndBattleFlagClearTable の FLAG_INVERSE）で下りる。
void HackChallengeF_OnBattleStart(void)
{
	if (!FlagGet(HACK_FLAG_CHF_INVERSE))
		return;
	if (FlagGet(FLAG_BATTLE_FACILITY) || HackDummy_Active())
		return;
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_OAK_TUTORIAL | BATTLE_TYPE_POKE_DUDE))
		return;
	FlagSet(FLAG_INVERSE);
}

/* ---- しばりボーナス ---- */

#define PRIZE_MONEY_MAX 9999999   //multi.c の PRIZE_MONEY_MAX と同じ（トレーナーの賞金の上限）

//ボーナスで数える「ほかに ON の しばり」の数。ボーナス自身は数えない・しばりを OFF にしたことがあるセーブ（BROKEN）は 0
u8 HackChallengeF_BonusCount(void)
{
	u8 n;

	if (!FlagGet(HACK_FLAG_CHF_BONUS) || FlagGet(HACK_FLAG_CH_BROKEN))
		return 0;
	n = HackChallenge_CountAllOn();
	return n > 0 ? n - 1 : 0;   //ボーナス自身（ON なので 1 本入っている）を引く
}

//(a) 賞金: ON の しばり 1 つにつき +10%（最大 25 本で +250%）。呼ぶ側（multi.c PrizeForOneTrainer）が最後に 上限 9,999,999 で止める
u32 HackChallengeF_PrizeBonus(u32 money)
{
	u32 n = HackChallengeF_BonusCount();

	if (n == 0)
		return money;
	if (money > PRIZE_MONEY_MAX)
		money = PRIZE_MONEY_MAX;   //掛け算があふれないように先に止める（9,999,999 × 250 は u32 に収まる）
	return money + money * (n * HACK_CHF_BONUS_PERCENT) / 100;
}

//(b) バッジの道具。ON の数（ボーナス自身を除く）→ ふしぎなアメ ×N ＋ ポイントアップ／ポイントマックス（既定の案・要確認: docs/research/2026-11-03_challenge-f.md）
//   1〜3  : ふしぎなアメ 1
//   4〜7  : ふしぎなアメ 2 ＋ ポイントアップ 1
//   8〜12 : ふしぎなアメ 3 ＋ ポイントアップ 2
//   13〜19: ふしぎなアメ 4 ＋ ポイントマックス 1
//   20〜  : ふしぎなアメ 5 ＋ ポイントマックス 2
//新しいバッジが 2 つ以上まとめて見つかったときは 数を バッジの数だけ掛ける（最大 8）
struct BonusTier
{
	u8 minCount;
	u8 candy;
	u16 ppItem;
	u8 ppQty;
};

static const struct BonusTier sBonusTiers[] =
{
	{20, 5, ITEM_PP_MAX, 2},
	{13, 4, ITEM_PP_MAX, 1},
	{8, 3, ITEM_PP_UP, 2},
	{4, 2, ITEM_PP_UP, 1},
	{1, 1, ITEM_NONE, 0},
};

static const struct BonusTier* BonusTierFor(u8 n)
{
	u32 i;

	for (i = 0; i < NELEMS(sBonusTiers); ++i)
	{
		if (n >= sBonusTiers[i].minCount)
			return &sBonusTiers[i];
	}
	return NULL;
}

static u8 BonusPaid(void)
{
	u8 v = 0;
	u8 i;

	for (i = 0; i < 6; ++i)
	{
		if (FlagGet(HACK_FLAG_CHF_BONUS_CNT0 + i))
			v |= 1 << i;
	}
	return v;
}

static void BonusPaidSet(u8 v)
{
	u8 i;

	for (i = 0; i < 6; ++i)
	{
		bool8 want = (v >> i) & 1;

		if ((FlagGet(HACK_FLAG_CHF_BONUS_CNT0 + i) != 0) != want)
		{
			if (want)
				FlagSet(HACK_FLAG_CHF_BONUS_CNT0 + i);
			else
				FlagClear(HACK_FLAG_CHF_BONUS_CNT0 + i);
		}
	}
}

//バッグへ。いっぱいなら 1 つずつパソコンのどうぐへ。渡せた個数を返す（1 つも入らなければ 0）
static u8 BonusGive(u16 item, u8 qty)
{
	u8 i;
	u8 given = 0;

	if (item == ITEM_NONE || qty == 0)
		return 0;
	if (AddBagItem(item, qty))
		return qty;
	for (i = 0; i < qty; ++i)
	{
		if (AddBagItem(item, 1) || HackMomSavings_AddPcItem(item))
			++given;
		else
			break;
	}
	return given;
}

static u8 BonusBadges(void)
{
	u16 v = Var8004;

	if (v == 0)
		v = 1;
	return v > 8 ? 8 : v;
}

//渡せた個数（0 なら 1 つも渡せなかった）を返す。gStringVar2 には渡せた個数を入れる
static u8 BonusGiveCommon(u16 item, u8 qty)
{
	u8 given = BonusGive(item, qty);

	if (given != 0)
	{
		CopyItemName(item, gStringVar1);
		ConvertIntToDecimalStringN(gStringVar2, given, STR_CONV_MODE_LEFT_ALIGN, 3);
		gSpecialVar_LastResult = 1;
	}
	else
		gSpecialVar_LastResult = 0;
	return given;
}

//ふしぎなアメが 1 つも渡せなかった（バッグもパソコンもいっぱい）ときは、先に進めた払った数を戻して 次の機会に渡す（Var8005 = 1 でポイントアップ側も飛ばす）
static void BonusUndoPaid(void)
{
	u8 paid = BonusPaid();
	u8 n = Var8004 > 63 ? 63 : (u8)Var8004;

	BonusPaidSet(paid > n ? paid - n : 0);
	Var8005 = 1;
}

//callasm（台本 EventScript_ChallengeBonus）: Var8004 = 新しく取ったバッジの数
void HackChallengeF_BonusGiveA(void)
{
	const struct BonusTier* tier = BonusTierFor(HackChallengeF_BonusCount());

	if (tier == NULL)
	{
		gSpecialVar_LastResult = 0;
		return;
	}
	Var8005 = 0;
	if (BonusGiveCommon(ITEM_RARE_CANDY, tier->candy * BonusBadges()) == 0)
		BonusUndoPaid();
}

void HackChallengeF_BonusGiveB(void)
{
	const struct BonusTier* tier = BonusTierFor(HackChallengeF_BonusCount());

	if (tier == NULL)
	{
		gSpecialVar_LastResult = 0;
		return;
	}
	if (Var8005 == 1)
	{
		gSpecialVar_LastResult = 0;   //ふしぎなアメが渡せなかった回は ポイントアップも渡さない（払った数を戻したので 次の機会に まとめて）
		return;
	}
	BonusGiveCommon(tier->ppItem, tier->ppQty * BonusBadges());
}

//HackChallenge_FieldTick から（毎コマ呼ばれる・16 コマごと・FinaleTick とは 8 コマずらす＝同じコマに 2 つの台本を始めない）。
//ごほうびを払ったバッジの数は 旗の 6 ビット。ボーナスが OFF／BROKEN／ほかの縛りが無いあいだは 払わずに 数だけ追いつかせる
//（あとから ON にしても それまでのバッジの分は出ない・バッジリセットで数が減っても 払った数は減らさない＝取り直しで二重に払わない）
//払った数を 今のバッジの数にそろえる（あとから ON にしても それまでのバッジの分は出ない）。せっていでボーナスを ON にする ほぞんの直前に呼ぶ
void HackChallengeF_BonusSyncPaid(void)
{
	u8 count = GetAllBadgeCount();

	if (count > 63)
		count = 63;
	if (count > BonusPaid())
		BonusPaidSet(count);
}

void HackChallengeF_FieldTick(void)
{
	u8 count, paid, newBadges;

	if (!FlagGet(HACK_FLAG_CHF_BONUS))
		return;   //ボーナスが OFF のあいだは何もしない（バッジの数を毎 16 コマ読まない）。払った数は ON にする せっていの ほぞんで そろえる（HackChallengeF_BonusSyncPaid）
	if ((gMain.vblankCounter2 & 0xF) != 8)
		return;
	count = GetAllBadgeCount();
	if (count > 63)
		count = 63;
	paid = BonusPaid();
	if (count <= paid)
		return;
	if (HackChallengeF_BonusCount() == 0)
	{
		BonusPaidSet(count);
		return;
	}
	if (gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld)
		return;   //台本が終わるまで待つ（次の周期でまた見る）
	newBadges = count - paid;
	BonusPaidSet(count);   //先に払った数を進める（台本の途中で電源が切れても 二重に払わない）
	Var8004 = newBadges;
	ScriptContext1_SetupScript(EventScript_ChallengeBonus);
}
