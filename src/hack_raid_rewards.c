#include "defines.h"
#include "../include/event_data.h"
#include "../include/random.h"
#include "../include/constants/items.h"
#include "hack_challenge_luxray.h" //HackMomSavings_AddPcItem（バッグが満杯のときパソコンのどうぐへ）

/*
 * 伝説レイドの報酬（FEAT-raid-rewards・patches/0863 の台帳の行・2026-10-26・ユーザー決定 10/26）。
 * 広場の人（assembly/hack_legend_raid/legend_raid_scripts.s）のレイドで 勝った（結果コード 1）・捕まえた（7）ときだけ、
 * 下の表を 1 行ずつ 別々に抽せんして 1 つずつ渡す（負け・にげた・テレポートは渡さない）。
 * 表は 案 work/tmp/raid_rewards/raid_rewards_table.md の ★6 から: きんのおうかん・努力きのみ・ドーピング・ダイマックスアメは入れない。ぎんのおうかんは 1%。
 * 保存場所・旗・変数は増やさない（行の位置だけ一時変数 VAR_TEMP_0 に持つ）。CFRU の報酬（sp11C・地名の表）は使わない。
 * 検算: tools/rom/verify_raid_rewards.py（この表を読む）
 */

#define HACK_VAR_RAID_REWARD_INDEX 0x4000 //VAR_TEMP_0

struct HackRaidReward
{
	const u16* items; //この中から 1 つ（1 つだけなら件数 1）
	u8 itemCount;
	u8 amount;
	u8 percent; //100 = 必ず
};

static const u16 sEnergyBerries5[] = { ITEM_LIECHI_BERRY, ITEM_GANLON_BERRY, ITEM_SALAC_BERRY, ITEM_PETAYA_BERRY, ITEM_APICOT_BERRY }; //チイラ・リュガ・カムラ・ヤタピ・ズア
static const u16 sPowerItems6[] = { ITEM_POWER_BRACER, ITEM_POWER_BELT, ITEM_POWER_LENS, ITEM_POWER_BAND, ITEM_POWER_ANKLET, ITEM_POWER_WEIGHT };
static const u16 sMints21[] =
{
	ITEM_LONELY_MINT, ITEM_ADAMANT_MINT, ITEM_NAUGHTY_MINT, ITEM_BRAVE_MINT, ITEM_BOLD_MINT, ITEM_IMPISH_MINT, ITEM_LAX_MINT,
	ITEM_RELAXED_MINT, ITEM_MODEST_MINT, ITEM_MILD_MINT, ITEM_RASH_MINT, ITEM_QUIET_MINT, ITEM_CALM_MINT, ITEM_GENTLE_MINT,
	ITEM_CAREFUL_MINT, ITEM_SASSY_MINT, ITEM_TIMID_MINT, ITEM_HASTY_MINT, ITEM_JOLLY_MINT, ITEM_NAIVE_MINT, ITEM_SERIOUS_MINT,
};
static const u16 sExpCandyXL[] = { ITEM_EXP_CANDY_XL };
static const u16 sPpUp[] = { ITEM_PP_UP };
static const u16 sBigNugget[] = { ITEM_BIG_NUGGET };
static const u16 sRareCandy[] = { ITEM_RARE_CANDY };
static const u16 sPpMax[] = { ITEM_PP_MAX };
static const u16 sExpCandyXXL[] = { ITEM_EXP_CANDY_XXL };
static const u16 sSilverCap[] = { ITEM_BOTTLE_CAP }; //ぎんのおうかん（ROM 内部名は BOTTLE_CAP）
static const u16 sAbilityCapsule[] = { ITEM_ABILITY_CAPSULE };
static const u16 sAbilityPatch[] = { ITEM_ABILITY_PATCH };

#define ROW(arr, amount, percent) { arr, ARRAY_COUNT(arr), amount, percent }

static const struct HackRaidReward sHackRaidRewards[] =
{
	ROW(sExpCandyXL,      2, 100), //けいけんアメXL（必ず）
	ROW(sPpUp,            2, 100), //ポイントアップ（必ず）
	ROW(sBigNugget,       1, 50),  //でかいきんのたま
	ROW(sEnergyBerries5,  2, 40),  //チイラ・リュガ・カムラ・ヤタピ・ズアのみ（1 種）
	ROW(sRareCandy,       3, 25),  //ふしぎなアメ
	ROW(sPpMax,           1, 20),  //ポイントマックス
	ROW(sExpCandyXXL,     1, 20),  //けいけんアメXXL
	ROW(sMints21,         1, 15),  //ミント（21 種から 1 つ）
	ROW(sAbilityCapsule,  1, 10),  //とくせいカプセル
	ROW(sPowerItems6,     1, 10),  //パワー系（6 種から 1 つ）
	ROW(sAbilityPatch,    1, 3),   //とくせいパッチ
	ROW(sSilverCap,       1, 1),   //ぎんのおうかん
};

//スクリプトから（callasm・VAR_TEMP_0 = 0 から始める）: 次に当たった行を Var8000（道具）・Var8001（個数）に入れて LASTRESULT = 1。もう無ければ 0
void HackRaidRewards_Next(void)
{
	u16 i = VarGet(HACK_VAR_RAID_REWARD_INDEX);

	for (; i < ARRAY_COUNT(sHackRaidRewards); ++i)
	{
		const struct HackRaidReward* r = &sHackRaidRewards[i];
		if (r->percent >= 100 || Random32() % 100 < r->percent)
		{
			Var8000 = r->items[r->itemCount > 1 ? Random() % r->itemCount : 0];
			Var8001 = r->amount;
			VarSet(HACK_VAR_RAID_REWARD_INDEX, i + 1);
			gSpecialVar_LastResult = 1;
			return;
		}
	}
	VarSet(HACK_VAR_RAID_REWARD_INDEX, i);
	gSpecialVar_LastResult = 0;
}

//スクリプトから（callasm・バッグに入らなかったとき）: Var8004（道具）を Var8005 個、パソコンのどうぐへ。全部入れば LASTRESULT = 1、入りきらなければ 0
void HackRaidRewards_ToPc(void)
{
	u16 n;
	gSpecialVar_LastResult = 1;
	for (n = 0; n < Var8005; ++n)
	{
		if (!HackMomSavings_AddPcItem(Var8004))
		{
			gSpecialVar_LastResult = 0;
			return;
		}
	}
}
