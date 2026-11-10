#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/event_object_movement.h"
#include "../include/field_player_avatar.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/money.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/sound.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/constants/songs.h"
#include "../include/string_util.h"
#include "../include/constants/items.h"
#include "hack_challenge_luxray.h"   //[hack] patches/0617 しばり「ぜんぶレントラー」: 買った道具は リルの あいじょう

/*
 * お母さんの貯金（金銀の「ははおやの ちょきん」・2026-10-06）。
 * ユーザー「金銀の母親貯金みたいなの実装できそう？」→ 割合は自分で決める（10〜50%）・
 * 買い物の知らせは電話の演出・道具は家に帰ったとき渡す。
 *
 *  - トレーナー戦の賞金: 戦闘命令 0x5D（素の getmoneyreward 080250A0）を HackMomSavings_GetMoneyReward に向け
 *    （CFRU assembly/data/battle_script_commands_table.s・patches/0190）、素の処理のあと増えた分の割合を貯金へ。
 *  - 貯金が表の次の値段に届くとお母さんが道具を買う（貯金から引く）→ 電話の印を立てる。
 *  - 電話: フィールドの毎コマ（src/overworld_speedup.c）。スクリプトが動いていない・主人公が止まっているときに
 *    EventScript_MomSavingsCall（assembly/kanto/kanto_mom_savings.s）を始める。
 *  - 家のお母さん（kanto_home_scripts.s の いつもの会話の手前）: 買った道具を渡す・貯金の額・ひきだす・割合・はじめる／やめる。
 *  状態は予約変数（build_hoenn_scripts.RESERVED_VARS・状態は書き換え可能な static に置けない＝6.13）。
 */

#define VAR_MOM_SAVINGS_LO 0x50F8
#define VAR_MOM_SAVINGS_HI 0x50F9
#define VAR_MOM_SETTINGS   0x50FA   //bit 15 = ためる・bit 14 = かいものをしない（0 = する＝前のセーブも「する」）・bit 8〜13 = 次に買える日（2026-10-16・0 = すぐ買える）・下 8 ビット = 割合（%）
#define VAR_MOM_PENDING    0x50FB   //家で渡す道具（0 = なし）
#define VAR_MOM_STEP       0x50FC   //下 8 ビット = 次に買う表の番号・bit 15 = 電話がまだ
#define VAR_MOM_PENDING_COUNT 0x50ED   //★ 2026-10-10 まだ渡していない数（表の「次に買う番号」の手前から数える）・build_hoenn_scripts.RESERVED_VARS
#define MOM_PENDING_MAX 99
extern const u8 gText_MomSav_Etc[];

#define SETTING_ON 0x8000
#define SETTING_NEXT_SHIFT 8      //★ 2026-10-16 ユーザー「ママが物を買ってくる頻度が高すぎる」→「1〜3日に1回」: 次に買える日（ゲームの日付 mod 63 を 1〜63 に・0 = 決めていない＝すぐ買える）を VAR_MOM_SETTINGS の bit 8〜13 に置く
#define SETTING_NEXT_MASK 0x3F00
#define SHOP_WAIT_MAX 3           //買った日の 1〜3 日後（乱数）
#define DAY_CYCLE 63
#define SETTING_SHOP_OFF 0x4000   //★ 2026-10-15 ユーザー「お母さんの買い物をする／しないを選べるように」: 立っていると買わない・電話もかからない（貯金はたまる）
#define CALL_PENDING 0x8000
#define DEFAULT_PERCENT 20
#define WILD_YEN_PER_LEVEL 10   //野生に勝ったとき 相手のレベル × これ（2026-10-08）
#define SAVINGS_MAX 9999999
#define PLAYER_MONEY_MAX 9999999   //AddMoney（0809FC80）の上限 0x0098967F

extern const u8 EventScript_MomSavingsCall[];

struct MomPurchase
{
	u32 price;
	u16 item;
};

//買う順（最後まで買ったら最初へ）。値段は貯金から引く
//★ 2026-10-10 ユーザー「ママの買い物リスト マスターボール・タマムシデパートの商品（4F 進化の石・つながりのヒモ・5F ドーピング・どりょくリセット・
//  とくせいパッチ・カプセル）・王冠・ミント」・値段は「Claude におまかせ」→ 値段 0 は道具の表の値段（お店と同じ）。
//  並びの順に買う（次に買う番号は VAR_MOM_STEP の下 8 ビット・表を伸ばしても前のセーブの番号はそのまま使える）。
#define ITEM_EV_RESET 0x340
static const struct MomPurchase sMomPurchases[] =
{
	{3000,   ITEM_EXP_CANDY_S},
	{10000,  ITEM_EXP_CANDY_M},
	{20000,  ITEM_BOTTLE_CAP},
	{30000,  ITEM_PP_UP},
	{50000,  ITEM_ABILITY_CAPSULE},
	{100000, ITEM_GOLD_BOTTLE_CAP},
	{100000, ITEM_ABILITY_PATCH},
	{0, ITEM_FIRE_STONE}, {0, ITEM_THUNDER_STONE}, {0, ITEM_WATER_STONE}, {0, ITEM_LEAF_STONE}, {0, ITEM_MOON_STONE},
	{0, ITEM_SUN_STONE}, {0, ITEM_SHINY_STONE}, {0, ITEM_DUSK_STONE}, {0, ITEM_DAWN_STONE}, {0, ITEM_ICE_STONE},
	{0, ITEM_LINK_CABLE},
	{0, ITEM_HP_UP}, {0, ITEM_PROTEIN}, {0, ITEM_IRON}, {0, ITEM_CALCIUM}, {0, ITEM_ZINC}, {0, ITEM_CARBOS},
	{0, ITEM_EV_RESET},
	{0, ITEM_LONELY_MINT}, {0, ITEM_ADAMANT_MINT}, {0, ITEM_NAUGHTY_MINT}, {0, ITEM_BRAVE_MINT}, {0, ITEM_BOLD_MINT},
	{0, ITEM_IMPISH_MINT}, {0, ITEM_LAX_MINT}, {0, ITEM_RELAXED_MINT}, {0, ITEM_MODEST_MINT}, {0, ITEM_MILD_MINT},
	{0, ITEM_RASH_MINT}, {0, ITEM_QUIET_MINT}, {0, ITEM_CALM_MINT}, {0, ITEM_GENTLE_MINT}, {0, ITEM_CAREFUL_MINT},
	{0, ITEM_SASSY_MINT}, {0, ITEM_TIMID_MINT}, {0, ITEM_HASTY_MINT}, {0, ITEM_JOLLY_MINT}, {0, ITEM_NAIVE_MINT},
	{0, ITEM_SERIOUS_MINT},
	{32768, ITEM_MASTER_BALL},   //シルフカンパニー 1F の売り場（65535）の半分（2026-10-10 ユーザー「シルフカンパニーの金額から半額で」）
	{0, ITEM_MOEMON_FOSSIL},    //Moemonかせき（10000 円・手に入るのはここだけ＝2026-10-10 ユーザー「「Moemonかせき」はママのみで」・src/hack_moemon_fossil.c）
	{0, ITEM_GOLD_CAPSULE},     //おうごんのカプセル（15000 円・手に入るのはここだけ＝2026-10-10 ユーザー「お母さんの買い物」）
	{0, ITEM_HISUI_ROCK},       //ヒスイのいし（10000 円・手に入るのはここだけ＝2026-10-11 ユーザー「ままで」・ヒスイの姿 7 つとリンドリスの進化の石・patches/0241）
	{0, ITEM_GIMMIGHOUL_COIN},  //コレクレーのコイン（3000 円・コレクレー → サーフゴー・2026-10-11 ユーザー「コレクレーのコインをママについか」）
	{0, ITEM_ALOLA_STONE},      //アローラのいし（10000 円・手に入るのはここだけ＝2026-10-11 ユーザー「ままに」・patches/0327）
	{20000, ITEM_RUSTY_CROWN},  //さびたおうかん（ぎんのおうかんと同じ 20000 円＝王冠を買えるのと同じ店・2026-10-16 ユーザー・patches/0433。表の後ろに足す＝前のセーブの「次に買う番号」はそのまま）
};

static u32 GetSavings(void)
{
	return VarGet(VAR_MOM_SAVINGS_LO) | ((u32) VarGet(VAR_MOM_SAVINGS_HI) << 16);
}

static void SetSavings(u32 v)
{
	if (v > SAVINGS_MAX)
		v = SAVINGS_MAX;
	VarSet(VAR_MOM_SAVINGS_LO, v & 0xFFFF);
	VarSet(VAR_MOM_SAVINGS_HI, v >> 16);
}

static u8 GetPercent(void)
{
	u8 p = VarGet(VAR_MOM_SETTINGS) & 0xFF;
	return (p == 0) ? DEFAULT_PERCENT : p;
}

static u32 PurchasePrice(u8 index)
{
	u32 p = sMomPurchases[index].price;
	if (p == 0)
		p = ItemId_GetPrice(sMomPurchases[index].item);
	return (p == 0) ? 1000 : p;
}

//★ 2026-10-10 ユーザー「ママのかいものを一気に渡すように」: 1 つずつ（家で受け取るまで次を買わない）をやめ、貯金で買えるだけ続けて買う
//  （まだ渡していない数は VAR_MOM_PENDING_COUNT・99 まで）。家で全部まとめて渡す（HackMomSavings_GiveAll）
//★ 2026-10-11 ユーザー「でんわで ママが かったアイテムが PC に おくられるように」: 買った道具はパソコンの どうぐ（gSaveBlock1->pcItems・30 枠・
//  1 枠 9999 まで（2026-10-15 patches/0367・前は 999）・素と同じく数は暗号化しない）へ直に入れる。入らなければ今までどおり「家で渡す」（VAR_MOM_PENDING_COUNT）に残す。
//  素の AddPCItem は BPRJ.ld に無い（6.29）ので同じことをここで書く
#define PC_ITEM_MAX 9999  // ★ 2026-10-15 patches/0367: PC 本体の上限と合わせる
static bool8 MomAddPcItem(u16 item)
{
	u32 i;
	struct ItemSlot* pc = gSaveBlock1->pcItems;

	for (i = 0; i < PC_ITEMS_COUNT; ++i)
		if (pc[i].itemId == item && pc[i].quantity < PC_ITEM_MAX)
		{
			++pc[i].quantity;
			return TRUE;
		}
	for (i = 0; i < PC_ITEMS_COUNT; ++i)
		if (pc[i].itemId == ITEM_NONE || pc[i].quantity == 0)
		{
			pc[i].itemId = item;
			pc[i].quantity = 1;
			return TRUE;
		}
	return FALSE;
}

//[hack] patches/0617 リルの あいじょう（src/hack_luxray_love.c）が 同じ入れ方を使う（持ち物をバッグへ移せないとき）
bool8 HackMomSavings_AddPcItem(u16 item)
{
	return MomAddPcItem(item);
}

//電話の文の選び方: この回に買ったものが全部パソコンに入ったか（bit 14 を電話の印と一緒に立てる）・この回に買った数（bit 8〜13・63 まで・「〜など」の判定）
#define CALL_TO_PC 0x4000
#define CALL_COUNT_SHIFT 8
#define CALL_COUNT_MASK 0x3F00

//ゲームの時計（gClock・リアルタイムでもプレイ時間の時計でも同じ・6.57）から、2000 年 1 月 1 日からの日数。差だけを使う（2100 年の閏年は無視）
static u32 MomDayNumber(void)
{
	static const u16 sCum[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	u32 y = (gClock.year >= 2000) ? gClock.year - 2000 : 0;
	u32 m = (gClock.month >= 1 && gClock.month <= 12) ? gClock.month - 1 : 0;
	u32 d = y * 365 + (y + 3) / 4 + sCum[m] + gClock.day;
	if (m >= 2 && (y % 4) == 0)
		++d;
	return d;
}

//買える日か。置き場が 0（前のセーブ・まだ買っていない）なら買える。次の日までの残りが 1〜3 日のときだけ待つ
//（日付を大きく戻したときは 1〜3 日に入らず、すぐ買える）。買える日になったら置き場を消す＝買えなくて何日たっても数え直しで待たされない
static bool8 MomShopIsDue(void)
{
	u16 s = VarGet(VAR_MOM_SETTINGS);
	u32 stored = (s & SETTING_NEXT_MASK) >> SETTING_NEXT_SHIFT;
	u32 diff;

	if (stored == 0)
		return TRUE;
	diff = (stored - 1 + DAY_CYCLE - MomDayNumber() % DAY_CYCLE) % DAY_CYCLE;
	if (diff >= 1 && diff <= SHOP_WAIT_MAX)
		return FALSE;
	VarSet(VAR_MOM_SETTINGS, s & ~SETTING_NEXT_MASK);
	return TRUE;
}

//買ったら、次に買える日を 1〜3 日後に決める
static void MomShopSetNext(void)
{
	u32 wait = Random() % SHOP_WAIT_MAX + 1;
	u32 stored = (MomDayNumber() + wait) % DAY_CYCLE + 1;
	VarSet(VAR_MOM_SETTINGS, (VarGet(VAR_MOM_SETTINGS) & ~SETTING_NEXT_MASK) | (stored << SETTING_NEXT_SHIFT));
}

static bool8 TryMomPurchaseNow(void)
{
	u16 step = VarGet(VAR_MOM_STEP);
	u8 index = (step & 0xFF) % ARRAY_COUNT(sMomPurchases);
	u32 savings = GetSavings();
	u16 count = VarGet(VAR_MOM_PENDING_COUNT);
	bool8 bought = FALSE;
	bool8 allToPc = TRUE;

	u16 boughtNow = 0;     //電話の「〜など」用（bit 8〜13・63 まで）
	u16 boughtTotal = 0;   //1 回の戦闘で買う数の上限（99・パソコンに入る分は count が増えないので別に数える・2026-10-11 5e）

	while (count < MOM_PENDING_MAX && boughtTotal < MOM_PENDING_MAX && savings >= PurchasePrice(index))
	{
		savings -= PurchasePrice(index);
		CopyItemName(HackLuxrayLove_ShopItem(sMomPurchases[index].item), gStringVar1);   //電話で言う名前（最後に買ったもの・しばり「ぜんぶレントラー」のあいだは リルの あいじょう）
		//★ 家待ち（count）が 1 つでもある間は、新しい買い物もパソコンに入れず家待ちに足す。家待ちは「index の手前から count 個」で
		//  復元する（HackMomSavings_GiveAll）ので、パソコンに入った分と混ざると別の品を指す（5e の指摘 2026-10-11）
		if (count == 0 && VarGet(VAR_MOM_PENDING) == ITEM_NONE && MomAddPcItem(HackLuxrayLove_ShopItem(sMomPurchases[index].item)))
			;   //パソコンへ（2026-10-11）
		else
		{
			++count;   //入らなかった分（と、そのあとの分）は家で
			allToPc = FALSE;
		}
		index = (index + 1) % ARRAY_COUNT(sMomPurchases);
		++boughtTotal;
		if (boughtNow < 63)
			++boughtNow;
		bought = TRUE;
	}
	if (!bought)
		return FALSE;
	SetSavings(savings);
	VarSet(VAR_MOM_PENDING_COUNT, count);
	VarSet(VAR_MOM_STEP, index | CALL_PENDING | (boughtNow << CALL_COUNT_SHIFT)
	       | ((allToPc && count == 0 && VarGet(VAR_MOM_PENDING) == ITEM_NONE) ? CALL_TO_PC : 0));
	return TRUE;
}

//買い物の入口（賞金・野生・家での渡し終わり・「かいものを する」に戻したとき）はここを通る。
//かいものを しない（VAR_MOM_SETTINGS の bit 14）なら何もせず戻る＝貯金はそのまま・買わない・電話の印も立てない
static void TryMomPurchase(void)
{
	if (VarGet(VAR_MOM_SETTINGS) & SETTING_SHOP_OFF)
		return;
	if (!MomShopIsDue())
		return;   //★ 2026-10-16 まだ次に買える日ではない（貯金はそのままたまる）
	if (TryMomPurchaseNow())
		MomShopSetNext();
}

//★ 2026-10-09 ユーザー「ママの貯金金額を表示するようにしてほしい」→「おかね送る状態のときにひょうじしてほしい」:
//  賞金を送ったとき、素の「〜は ○えん もらった！」のあとに「ママに ○えん おくった！ ちょきんは いま ○えん」を出す。
//  素の台本は getmoneyreward のあと「printstring STRINGID_PLAYERGOTMONEY（10 xx xx）/ waitmessage（12 xx xx）」の 6 バイト。
//  その 6 バイトの代わりに BattleScript_HackMomSavingsSent（assembly/hack_mom_savings/）を通し、終わったらその次へ戻る。並びが違えば何もしない
extern const u8 BattleScript_HackMomSavingsSent[];

static void ShowSentMessage(u32 put)
{
	const u8* next = gBattlescriptCurrInstr;

	if (next[0] != 0x10 || next[3] != 0x12)
		return;
	//★ 2026-10-15 おくった額は gStringVar3 へ（文は [BUFFER][04]）。gStringVar1 だと、先の printstring 30（もらった）が BUFF1＝賞金の
	//  数を gStringVar1 へ展開して上書きし、「おくった」に賞金全額が出る（battle_strings.c の B_TXT_BUFF1）。gStringVar2 は触られない
	ConvertIntToDecimalStringN(gStringVar3, put, STR_CONV_MODE_LEFT_ALIGN, 7);
	ConvertIntToDecimalStringN(gStringVar2, GetSavings(), STR_CONV_MODE_LEFT_ALIGN, 7);
	gBattlescriptCurrInstr = next + 6;
	BattleScriptPushCursor();
	gBattlescriptCurrInstr = BattleScript_HackMomSavingsSent;
}

//戦闘命令 0x5D（getmoneyreward）
void HackMomSavings_GetMoneyReward(void)
{
	u32 before, prize, put;

	if (!(VarGet(VAR_MOM_SETTINGS) & SETTING_ON))
	{
		((void (*)(void)) (0x080250A0 | 1))(); //ためないときは素のまま
		return;
	}

	//★ 2026-10-20 ユーザー報告「送る金額を 100% にしているのに 500 円しか送っていない」: 前は「素の処理の前後の所持金の差」を賞金とみなしていたが、
	//  素の AddMoney は上限（9,999,999）で頭打ちにするので、所持金が上限の近くだと差が小さくなり（賞金 5,000 円・所持金 9,999,499 なら 500 円）、
	//  100% でも毎回その少ない額しか送れなかった（送ると所持金が before に戻るので、次の戦いも同じ差になる）。
	//  いったん所持金を 0 にして素の処理を呼べば、素の AddMoney が足した額＝賞金そのもの（賞金の計算が 9,999,999 までに止めているので頭打ちされない）。
	//  そのあと所持金を「前の額 ＋ 賞金 − 送る額」（AddMoney の上限で止まる）に作り直す
	before = GetMoney(&gSaveBlock1->money);
	if (before > 0)
		RemoveMoney(&gSaveBlock1->money, before);

	((void (*)(void)) (0x080250A0 | 1))(); //素の getmoneyreward（賞金を渡す・文の数字を用意する）

	prize = GetMoney(&gSaveBlock1->money);
	put = prize * GetPercent() / 100;
	if (prize > 0)
		RemoveMoney(&gSaveBlock1->money, prize);
	AddMoney(&gSaveBlock1->money, before + (prize - put));   //上限をこえる分は AddMoney が切る（素の挙動と同じ）
	if (put > 0)
	{
		//★ 2026-10-08 ユーザー「タメルでもたまりにくいみたい」→「お母さんが上乗せ」: ためた分と同じだけお母さんが足す
		SetSavings(GetSavings() + put * 2);
		TryMomPurchase();
		ShowSentMessage(put);
	}
}

//★ 2026-10-08 ユーザー「野生の戦いでも貯まる」: 野生に勝ったとき（CFRU の src/end_battle.c の EndOfBattleThings・patches/0224）、
//  たおした相手のレベル × WILD_YEN_PER_LEVEL 円をお母さんがためる（おこづかいは減らない・割合は関係しない・上乗せは無し）
void HackMomSavings_WildWin(void)
{
	u32 add = 0, i;

	if (!(VarGet(VAR_MOM_SETTINGS) & SETTING_ON))
		return;
	if (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK | BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_SAFARI | BATTLE_TYPE_POKE_DUDE | BATTLE_TYPE_GHOST))
		return;
	if (gBattleOutcome != B_OUTCOME_WON)
		return;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
			add += GetMonData(&gEnemyParty[i], MON_DATA_LEVEL, NULL) * WILD_YEN_PER_LEVEL;
	}
	if (add > 0)
	{
		SetSavings(GetSavings() + add);
		TryMomPurchase();
	}
}

//フィールドの毎コマ: 電話
void HackMomSavings_FieldTick(void)
{
	if (!(VarGet(VAR_MOM_STEP) & CALL_PENDING))
		return;
	if (ScriptContext2_IsEnabled())
		return;
	if (gPlayerAvatar->tileTransitionState != T_NOT_MOVING || gPlayerAvatar->runningState != NOT_MOVING)
		return;
	//★ 2026-10-10 ユーザー「お母さんからの電話は家にいるときは来ないように」: 自分の家（マサラタウンの 1F・2F・群 4 の 0／1）では鳴らさない（出たら鳴る）
	if (gSaveBlock1->location.mapGroup == 4 && (u8) gSaveBlock1->location.mapNum <= 1)
		return;

	{
		u16 step = VarGet(VAR_MOM_STEP);
		u16 boughtNow = (step & CALL_COUNT_MASK) >> CALL_COUNT_SHIFT;   //この回に買った数（0 は前の形のセーブ＝家待ちの数で代える）
		u16 count = VarGet(VAR_MOM_PENDING_COUNT);
		u8 last;

		Var8004 = (step & CALL_TO_PC) ? 1 : 0;   //★ 2026-10-11 1 = 全部パソコンへ入れた（文を変える）・0 = 家で渡すものがある
		VarSet(VAR_MOM_STEP, step & ~(CALL_PENDING | CALL_TO_PC | CALL_COUNT_MASK));
		//電話: 「ちょきんで [BUFFER1]を かっておいたわ」＝最後に買ったもの（2 こ以上なら「〜など」）
		last = ((step & 0xFF) + ARRAY_COUNT(sMomPurchases) - 1) % ARRAY_COUNT(sMomPurchases);
		if (count == 0 && VarGet(VAR_MOM_PENDING) != ITEM_NONE)
			CopyItemName(HackLuxrayLove_ShopItem(VarGet(VAR_MOM_PENDING)), gStringVar1);
		else
			CopyItemName(HackLuxrayLove_ShopItem(sMomPurchases[last].item), gStringVar1);
		if (boughtNow == 0)
			boughtNow = count + (VarGet(VAR_MOM_PENDING) != ITEM_NONE);
		if (boughtNow > 1)
			StringAppend(gStringVar1, gText_MomSav_Etc);
	}
	ScriptContext1_SetupScript(EventScript_MomSavingsCall);
}

//以下はスクリプトから（callasm）

//★ 2026-10-10 家で全部まとめて渡す（バッグに入れるだけ・入らなかった分は次に）。
//  LASTRESULT 0 = 渡すものが無い / 1 = 全部渡した / 2 = バッグがいっぱいで残った。gStringVar1 = 渡した数
void HackMomSavings_GiveAll(void)
{
	u16 count = VarGet(VAR_MOM_PENDING_COUNT);
	u8 next = (VarGet(VAR_MOM_STEP) & 0xFF) % ARRAY_COUNT(sMomPurchases);
	u8 start = (next + ARRAY_COUNT(sMomPurchases) - (count % ARRAY_COUNT(sMomPurchases))) % ARRAY_COUNT(sMomPurchases);
	u16 given = 0;
	u16 legacy = VarGet(VAR_MOM_PENDING);   //前の形（1 つだけの道具）

	if (count == 0 && legacy == ITEM_NONE)
	{
		gSpecialVar_LastResult = 0;
		return;
	}
	gSpecialVar_LastResult = 1;
	if (legacy != ITEM_NONE)
	{
		if (AddBagItem(HackLuxrayLove_ShopItem(legacy), 1))
		{
			VarSet(VAR_MOM_PENDING, ITEM_NONE);
			++given;
		}
		else
			gSpecialVar_LastResult = 2;
	}
	while (count > 0 && gSpecialVar_LastResult == 1)
	{
		if (!AddBagItem(HackLuxrayLove_ShopItem(sMomPurchases[start].item), 1))
		{
			gSpecialVar_LastResult = 2;
			break;
		}
		start = (start + 1) % ARRAY_COUNT(sMomPurchases);
		--count;
		++given;
	}
	VarSet(VAR_MOM_PENDING_COUNT, count);
	if (count == 0 && VarGet(VAR_MOM_PENDING) == ITEM_NONE)
		VarSet(VAR_MOM_STEP, VarGet(VAR_MOM_STEP) & ~CALL_PENDING);   //★ 2026-10-10 家で全部受け取ったら、まだ鳴っていない電話は取り消す
	ConvertIntToDecimalStringN(gStringVar1, given, STR_CONV_MODE_LEFT_ALIGN, 3);
	if (given == 0)
		gSpecialVar_LastResult = 3;   //1 つも入らなかった
}

//LASTRESULT = 家で渡す道具（0 = なし）。0x8000 にも入れる（obtainitem 0x8000）
void HackMomSavings_TakePending(void)
{
	Var8000 = HackLuxrayLove_ShopItem(VarGet(VAR_MOM_PENDING));   //しばり「ぜんぶレントラー」のあいだは リルの あいじょう（0 のまま）
	gSpecialVar_LastResult = Var8000;
}

//渡し終えた（obtainitem のあと）
void HackMomSavings_ClearPending(void)
{
	VarSet(VAR_MOM_PENDING, ITEM_NONE);
	TryMomPurchase(); //貯金がまだ足りていれば次を買う（電話はまたかかる）
}

//gStringVar1 = 貯金・gStringVar2 = 割合。LASTRESULT = ためているか
void HackMomSavings_BufferStatus(void)
{
	ConvertIntToDecimalStringN(gStringVar1, GetSavings(), STR_CONV_MODE_LEFT_ALIGN, 7);
	ConvertIntToDecimalStringN(gStringVar2, GetPercent(), STR_CONV_MODE_LEFT_ALIGN, 3);
	gSpecialVar_LastResult = (VarGet(VAR_MOM_SETTINGS) & SETTING_ON) != 0;
}

void HackMomSavings_ToggleOn(void)
{
	u16 s = VarGet(VAR_MOM_SETTINGS);
	if ((s & 0xFF) == 0)
		s |= DEFAULT_PERCENT;
	VarSet(VAR_MOM_SETTINGS, s ^ SETTING_ON);
	gSpecialVar_LastResult = (VarGet(VAR_MOM_SETTINGS) & SETTING_ON) != 0;
}

//LASTRESULT = かいものを しているか（1 = する）。台本のメニューの文字の選び方に使う
void HackMomSavings_ShopStatus(void)
{
	gSpecialVar_LastResult = (VarGet(VAR_MOM_SETTINGS) & SETTING_SHOP_OFF) == 0;
}

//かいものの する／しない を入れ替える。LASTRESULT = 入れ替えたあと かいものを しているか。
//「する」に戻したときは、そのとき貯金で買える分をすぐ買う（電話は家を出てから・TryMomPurchase 経由）
void HackMomSavings_ToggleShop(void)
{
	VarSet(VAR_MOM_SETTINGS, VarGet(VAR_MOM_SETTINGS) ^ SETTING_SHOP_OFF);
	gSpecialVar_LastResult = (VarGet(VAR_MOM_SETTINGS) & SETTING_SHOP_OFF) == 0;
	if (gSpecialVar_LastResult)
	{
		u16 r = gSpecialVar_LastResult;
		TryMomPurchase();
		gSpecialVar_LastResult = r;
	}
}

//LASTRESULT（0〜6）→ 10・20・30・40・50・75・100%（2026-10-08 ユーザー「割合を 100% まで」）
void HackMomSavings_SetPercent(void)
{
	static const u8 sPercents[] = {10, 20, 30, 40, 50, 75, 100};
	u16 s = VarGet(VAR_MOM_SETTINGS) & (SETTING_ON | SETTING_SHOP_OFF | SETTING_NEXT_MASK);   //割合を変えても かいものの するしない・次に買える日は残す
	u8 i = gSpecialVar_LastResult < ARRAY_COUNT(sPercents) ? gSpecialVar_LastResult : 1;
	VarSet(VAR_MOM_SETTINGS, s | sPercents[i]);
	ConvertIntToDecimalStringN(gStringVar2, GetPercent(), STR_CONV_MODE_LEFT_ALIGN, 3);
}

//できるだけひきだす（おこづかいの上限まで）。gStringVar1 = ひきだした額・LASTRESULT = ひきだした額が 0 でない
void HackMomSavings_Withdraw(void)
{
	u32 savings = GetSavings();
	u32 money = GetMoney(&gSaveBlock1->money);
	u32 room = (money < PLAYER_MONEY_MAX) ? PLAYER_MONEY_MAX - money : 0;
	u32 take = (savings < room) ? savings : room;

	if (take > 0)
	{
		AddMoney(&gSaveBlock1->money, take);
		SetSavings(savings - take);
	}
	ConvertIntToDecimalStringN(gStringVar1, take, STR_CONV_MODE_LEFT_ALIGN, 7);
	gSpecialVar_LastResult = take != 0;
}

/* ---- あずける（2026-10-16 ユーザー「あずけるを足す・電話と家の両方」・数は お店の個数／コイン売りの画面と同じ操作）---- */
// 金額（円）を数字で入れる: 上下 ±100・左右 ±1,000・L＋左右 ±10,000・R＋左右 ±100,000（A 決定・B やめる）。
// コイン売り（hack_coin_clerk.c）の 1・10・100・1000 はコインの枚数の刻みなので、円は 100 倍した刻みにした。
// 最小は 100 円（持っているお金が 100 円に満たないときは持っているお金だけ）・最大は min(手持ち, 貯金の上限 − 今の貯金)。
#define DEP_STEP_UNIT 100
#define DEP_WIN_LEFT 12
#define DEP_WIN_WIDTH 18
#define tDepWin data[0]
#define tDepNLo data[1]
#define tDepMaxLo data[3]

extern const u8 gText_MomSav_DepYen[], gText_MomSav_DepSavings[];

static u32 DepGet32(s16 *lo)
{
	return (u16) lo[0] | ((u32) (u16) lo[1] << 16);
}

static void DepPut32(s16 *lo, u32 v)
{
	lo[0] = v & 0xFFFF;
	lo[1] = v >> 16;
}

static u32 DepMax(void)
{
	u32 money = GetMoney(&gSaveBlock1->money);
	u32 savings = GetSavings();
	u32 room = (savings < SAVINGS_MAX) ? SAVINGS_MAX - savings : 0;

	return (money < room) ? money : room;
}

//上下 ±100・左右 ±1,000・L＋左右 ±10,000・R＋左右 ±100,000。min〜max に収める（max が 100 に満たないときは max に固定）
u32 HackMomSavings_DepositStep(u32 n, u32 max, u16 keys)
{
	u32 big = (keys & L_BUTTON) ? 10000 : (keys & R_BUTTON) ? 100000 : 1000;
	u32 lo = (max < DEP_STEP_UNIT) ? max : DEP_STEP_UNIT;

	if (keys & DPAD_UP)
		n = (n + DEP_STEP_UNIT > max) ? max : n + DEP_STEP_UNIT;
	if (keys & DPAD_DOWN)
		n = (n > lo + DEP_STEP_UNIT) ? n - DEP_STEP_UNIT : lo;
	if (keys & DPAD_RIGHT)
		n = (n + big > max) ? max : n + big;
	if (keys & DPAD_LEFT)
		n = (n > lo + big) ? n - big : lo;
	return n;
}

static void DepDraw(u8 window, u32 n)
{
	u8 buf[20];

	FillWindowPixelBuffer(window, PIXEL_FILL(1));
	ConvertIntToDecimalStringN(buf, n, STR_CONV_MODE_RIGHT_ALIGN, 7);
	StringAppend(buf, gText_MomSav_DepYen);
	AddTextPrinterParameterized(window, 2, buf, 8, 0, 0xFF, NULL);
	StringCopy(buf, gText_MomSav_DepSavings);                       //「ちょきん」＋ あずけたあとの額
	ConvertIntToDecimalStringN(buf + StringLength(buf), GetSavings() + n, STR_CONV_MODE_RIGHT_ALIGN, 7);
	StringAppend(buf, gText_MomSav_DepYen);
	AddTextPrinterParameterized(window, 2, buf, 8, 16, 0xFF, NULL);
	PutWindowTilemap(window);
	CopyWindowToVram(window, COPYWIN_BOTH);
}

static void Task_MomDeposit(u8 taskId)
{
	s16 *data = gTasks[taskId].data;
	u32 n = DepGet32(&tDepNLo);
	u32 next = HackMomSavings_DepositStep(n, DepGet32(&tDepMaxLo),
		(gMain.newAndRepeatedKeys & (DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT)) | (gMain.heldKeysRaw & (L_BUTTON | R_BUTTON)));
	bool8 decided = (gMain.newKeys & A_BUTTON) != 0;

	if (next != n)
	{
		PlaySE(SE_SELECT);
		DepPut32(&tDepNLo, next);
		DepDraw(tDepWin, next);
		return;
	}
	if (!decided && !(gMain.newKeys & B_BUTTON))
		return;
	PlaySE(SE_SELECT);
	if (decided)
	{
		Var8004 = n & 0xFFFF;
		Var8005 = n >> 16;
		gSpecialVar_LastResult = 1;
	}
	else
		gSpecialVar_LastResult = 0;
	ClearStdWindowAndFrameToTransparent(tDepWin, TRUE);
	RemoveWindow(tDepWin);
	DestroyTask(taskId);
	EnableBothScriptContexts();
}

//callasm: Var8004/Var8005 = 預けられる最大の額。LASTRESULT 0 = 預けられる／1 = 手持ちが 0 円／2 = 貯金がいっぱい（上限）
void HackMomSavings_DepositMax(void)
{
	u32 max = DepMax();
	u32 savings = GetSavings();

	Var8004 = max & 0xFFFF;
	Var8005 = max >> 16;
	if (max != 0)
		gSpecialVar_LastResult = 0;
	else
		gSpecialVar_LastResult = (savings >= SAVINGS_MAX) ? 2 : 1;
}

//callasm → waitstate: 金額を入れる画面（Var8004/Var8005 = 最大）。LASTRESULT 1 = 決めた（Var8004/Var8005 に額）／ 0 = B
void HackMomSavings_DepositOpen(void)
{
	struct WindowTemplate win;
	u32 max = Var8004 | ((u32) Var8005 << 16);
	u32 first = (max < DEP_STEP_UNIT) ? max : DEP_STEP_UNIT;
	u8 windowId, taskId;
	s16 *data;

	win.bg = 0;
	win.tilemapLeft = DEP_WIN_LEFT;
	win.tilemapTop = 1;
	win.width = DEP_WIN_WIDTH;
	win.height = 4;
	win.paletteNum = 15;
	win.baseBlock = 0x038;
	windowId = AddWindow(&win);
	DrawStdWindowFrame(windowId, FALSE);
	taskId = CreateTask(Task_MomDeposit, 80);
	data = gTasks[taskId].data;
	tDepWin = windowId;
	DepPut32(&tDepNLo, first);
	DepPut32(&tDepMaxLo, max);
	DepDraw(windowId, first);
}

//callasm: Var8004/Var8005 の額を 所持金 → 貯金 へ。gStringVar1 = 預けた額・gStringVar2 = 預けたあとの貯金。LASTRESULT 1 = 預けた
void HackMomSavings_Deposit(void)
{
	u32 amount = Var8004 | ((u32) Var8005 << 16);
	u32 max = DepMax();

	if (amount > max)
		amount = max;           //どんな画面の経路でも、手持ちと上限をこえて預けない
	if (amount != 0)
	{
		RemoveMoney(&gSaveBlock1->money, amount);
		SetSavings(GetSavings() + amount);
	}
	ConvertIntToDecimalStringN(gStringVar1, amount, STR_CONV_MODE_LEFT_ALIGN, 7);
	ConvertIntToDecimalStringN(gStringVar2, GetSavings(), STR_CONV_MODE_LEFT_ALIGN, 7);
	gSpecialVar_LastResult = amount != 0;
}
