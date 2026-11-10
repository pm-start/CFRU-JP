#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/money.h"
#include "../include/pokemon.h"
#include "../include/script.h"
#include "../include/sound.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/constants/items.h"
#include "../include/constants/songs.h"
#include "../include/constants/species.h"
#include "../include/new/frontier.h"
#include "../include/new/follow_me.h"
#include "../include/event_object_movement.h"
#include "../include/field_player_avatar.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/constants/event_objects.h"
#include "../include/constants/trainer_classes.h"
#include "bw_follower_internal.h"       /* BwFollowerBannedHere ほか（#76 段 2） */
#include "../include/new/catching.h"      /* GiveMonToPlayer */
#include "../include/new/daycare.h"
#include "../include/new/util.h"           /* HealMon */
#include "hack_challenge_luxray.h"       /* HackMomSavings_AddPcItem */
#include "hack_oak_phone.h"             /* HACK_FLAG_OAK_PIC */
#include "Tables/hoenn_script_flags.h"   /* HACK_VAR_HEROINE_*・HACK_FLAG_PHONE_*（build_hoenn_scripts.RESERVED_VARS／LEDGER_FLAGS から生成） */
#include "Tables/hack_heroine_table.h"

/*
 * ハルカ・ヒカリの連れ歩き＋ダブルバトルの戦闘の入口（Redmine #76 段 0・FEAT-heroine-76-s0・patches/0865）。
 * 試作: 「連れ歩き中の人」を表す変数 HACK_VAR_HEROINE_FOLLOWER（0＝いない・1＝ハルカ）が 0 でなければ、野生の戦闘（草むら）と
 * トレーナー戦の始まりで、タグバトルの旗と相方の変数を毎回立てる。相方の手持ち・名前・後ろ姿・AI はレイドの助っ人（gRaidPartners の
 * 最後の 4 人のうちハルカ）をそのまま使う。
 * 旗 FLAG_TAG_BATTLE・FLAG_DOUBLE_WILD_BATTLE は戦闘が終わるたびに消える（end_battle.c の gEndBattleFlagClearTable）ので、立てっぱなしにせず毎回ここで立てる。
 * 人の絵の連れ歩き・電話・好感度・例外（水・ボス戦・施設）は段 1 以降。
 */


#define FLAG_HEROINE_TAG_BATTLE 0x908    //config.h の FLAG_TAG_BATTLE と同じ

static bool8 HeroineObjectPresent(void);

//[hack] patches/0920 #76: その人の地方の「ストーリーの終わり」まで好感度のしくみは何も起きない（h＝0 ハルカ＝ホウエン・1 ヒカリ＝シンオウ）。
//  ユーザー 10/27「B」: 殿堂入り（リーグ制覇）ではなくストーリーの終わり（ストーリーを飛ばしても殿堂入りはできるため）。
//  ハルカ＝ホウエンのストーリー終章（tools/rom/hoenn_story.py の STORY_FINAL_DONE＝10: ミナモでハルカに勝って相棒が決まった）を過ぎたら。値は 10 が最大。
//  ヒカリ＝シンオウのストーリーの終わり（tools/sinnoh/sinnoh_story.py の STORY_CH7_DONE＝10: やりのはしらでアカギに勝った）を過ぎたら。値 0x50C8 は 10 が最大（ホウエンと同じ形）。
//  （0.61.1 の「常に偽」を 0.62 のストーリー 6・7 章で差し替えた＝ FEAT-sinnoh-story-ch6-7b・patches/0937）。保存済みの旗・変数は読むだけで消さない。
#define HEROINE_HOENN_STORY_END 10
#include "hack_johto.h"   //[hack] patches/0986 シンオウのストーリーの変数・終わりの値はジョウトの開閉と共有（src/hack_johto.h）
#define HEROINE_SINNOH_STORY_VAR HACK_SINNOH_STORY_VAR
#define HEROINE_SINNOH_STORY_END HACK_SINNOH_STORY_END
bool8 HackHeroine_Unlocked(u8 h)
{
	if (h == 0)
		return VarGet(HOENN_VAR_STORY) >= HEROINE_HOENN_STORY_END;
	return VarGet(HEROINE_SINNOH_STORY_VAR) >= HEROINE_SINNOH_STORY_END;       //ヒカリ: シンオウのストーリー STORY_VAR ≥ 10（ここ1か所）
}

//連れ歩き中のハルカが「いま付いている」（人の連れ歩きが出ている）。待機中・水・施設では偽＝相方にならない
bool8 HackHeroine_Active(void)
{
	u8 who = VarGet(HACK_VAR_HEROINE_FOLLOWER) & 0xFF;

	return (who == 1 || who == 2) && HackHeroine_Unlocked(who - 1) && HeroineObjectPresent() && !FlagGet(FLAG_BATTLE_FACILITY);   //1 ハルカ・2 ヒカリ（段 3）
}

//相方の添字（gRaidPartners）: ハルカ＝末尾から 3 番目（最後の 4 人のうち 2 人目）・ヒカリ＝末尾から 5 番目（ユウキの前・patches/0870）
static u8 PartnerIndex(void)
{
	if ((VarGet(HACK_VAR_HEROINE_FOLLOWER) & 0xFF) == 2)
		return gNumRaidPartners - 5;
	return gNumRaidPartners - 4 + 1;
}

//戦闘の入口: タグバトルの旗と相方の変数を立てる（野生なら呼ぶ前後で DOUBLE_WILD も呼び元が立てる）
void HackHeroine_Prepare(void)
{
	u8 id = PartnerIndex();

	//[hack] BUG-raid-party-order: 選ぶ画面を使わない入口。施設の戦い（special 0x2F）の「選んだ順番」の残りを捨てる（残っていると先頭以外の子が最初に出る）
	Memset(gSelectedOrderFromParty, 0, PARTY_SIZE);

	FlagSet(FLAG_HEROINE_TAG_BATTLE);
	VarSet(VAR_PARTNER, RAID_BATTLE_MULTI_TRAINER_TID);
	VarSet(VAR_FACILITY_TRAINER_ID_PARTNER, id);
	VarSet(VAR_PARTNER_BACKSPRITE, gRaidPartners[id].backSpriteId);
}

//シンオウ 6 章（アジトの奥）のタッグ戦: 台本が callasm で呼ぶ。相方のヒカリ（gRaidPartners の末尾から 5 番目・手持ち 6 匹から 3 匹）の添字を置く。
//  相方の本体は trainerbattle 0xA が VAR_PARTNER（RAID_BATTLE_MULTI_TRAINER_TID）と後ろ姿を置き、build_pokemon.c が VAR_PARTNER で BuildRaidMultiParty を呼ぶ（FIX-galactic-ow-size-double）
void HackHeroine_StoryTagPartner(void)
{
	//[hack] BUG-raid-party-order: 選ぶ画面を使わない入口。施設の戦い（special 0x2F）の「選んだ順番」の残りを捨てる（残っていると先頭以外の子が最初に出る）
	Memset(gSelectedOrderFromParty, 0, PARTY_SIZE);
	VarSet(VAR_FACILITY_TRAINER_ID_PARTNER, gNumRaidPartners - 5);
}

/* ============================================================================================================
 * 段 1: 電話・好感度・ごほうび（Redmine #76・FEAT-heroine-76-s1・patches/0866）。連れ歩きはまだ無い（段 2）。
 *
 * ★ 持ち物（拡張変数 4 つ。0 から読めるので前のセーブでも壊れない）:
 *   AFFINITY  下位 8 ビット = ハルカ・上位 8 ビット = ヒカリ（好感度 0〜255）
 *   CLAIMED   人ごとに 8 ビット（ハルカ 0〜7・ヒカリ 8〜15）: ビット 0〜4 = 段 1〜5 を渡した印・ビット 5〜7 = 電話で知らせた最後の段（0〜5）
 *   SLOT      30 分の枠の番号（プレイ時間の時 × 2 ＋ 分が 30 以上なら 1）。枠が変わったら USED を 0 に戻す
 *   USED      ビット 0 = ハルカに電話した・ビット 1 = ヒカリに電話した・ビット 2〜8 = ハルカの貢ぎ（道具 × 11 ＋ お金・各 0〜10）・ビット 9〜15 = ヒカリ
 *   ※ 「話しかけ」は段 2 から（連れ歩き中）。そのときの印は別に変数をもらう。
 * ★ 電話帳への登録: 会った印（ハルカ＝ホウエン序章 0x12E8・ヒカリ＝シンオウ序章 0x12A0）が立ったら（好感度 0 から）src/hack_phone.c が登録する。
 * ★ 着信（ごほうび）: ハルカ・ヒカリは通常の抽選に入れない。段が上がったら次の 30 分の枠（IsCallMoment）に、専用の台本で鳴る。
 * ★ 台本（assembly/hack_phone/phone_heroine.s）は Var8003 に 0（ハルカ）か 1（ヒカリ）を置いてから callasm で呼ぶ。
 * ============================================================================================================ */

#define HEROINE_WHO_VAR 0x8003           /* 台本が置く: 0 = ハルカ・1 = ヒカリ */
#define HEROINE_MET_FLAG_HARUKA HOENN_FLAG_STORY_PROLOGUE   /* ホウエン序章（ミナモ）でハルカに会った（tools/rom/hoenn_story.py の FLAG_PROLOGUE_HIDE） */
#define HEROINE_MET_FLAG_HIKARI 0x12A0   /* シンオウ序章（キッサキ）でヒカリに会った（tools/sinnoh/sinnoh_story.py の FLAG_PROLOGUE_HIDE） */
#define HEROINE_PC_ITEM_MAX 9999         /* hack_mom_savings.c の PC_ITEM_MAX と同じ */

#define CL_CLAIM_MASK 0x1F
#define CL_NOTIFY_SHIFT 5
#define CL_NOTIFY_MASK 0x7

static u8 WhoIsCalling(void)
{
	return (VarGet(HEROINE_WHO_VAR) == 0) ? 0 : 1;
}

static u16 PhoneFlag(u8 h)
{
	return (h == 0) ? HACK_FLAG_PHONE_HARUKA : HACK_FLAG_PHONE_HIKARI;
}

bool8 HackHeroine_Met(u8 h)
{
	//[hack] 2026-10-27 BUG-mina-story-stuck: ホウエンは序章の印 0x12E8 が立たないままストーリーの変数だけ進んだセーブがある（序章を通らず 1 章へ進めた）。
	//  変数が 1 以上なら序章（ミナモでハルカに会う）は済んだ扱い＝ハルカには会っている（1 章の博物館でも会う）
	if (h == 0)
		return FlagGet(HEROINE_MET_FLAG_HARUKA) || VarGet(HOENN_VAR_STORY) >= 1;
	return FlagGet(HEROINE_MET_FLAG_HIKARI);
}

static u8 GetAffinity(u8 h)
{
	u16 v = VarGet(HACK_VAR_HEROINE_AFFINITY);

	return (h == 0) ? (v & 0xFF) : (v >> 8);
}

static void AddAffinity(u8 h, u32 gain)
{
	u32 v = GetAffinity(h) + gain;
	u16 a = VarGet(HACK_VAR_HEROINE_AFFINITY);

	if (!HackHeroine_Unlocked(h))
		return;                 //クリア前は上がらない（patches/0920）

	if (v > HEROINE_AFFINITY_MAX)
		v = HEROINE_AFFINITY_MAX;
	if (h == 0)
		a = (a & 0xFF00) | v;
	else
		a = (a & 0x00FF) | (v << 8);
	VarSet(HACK_VAR_HEROINE_AFFINITY, a);
}

static u8 StageOf(u8 affinity)
{
	u8 i, s = 0;

	for (i = 0; i < HEROINE_STAGES; ++i)
		if (affinity >= sHeroineThreshold[i])
			s = i + 1;
	return s;
}

static u8 GetStage(u8 h)
{
	return StageOf(GetAffinity(h));
}

static u8 GetClaimByte(u8 h)
{
	u16 v = VarGet(HACK_VAR_HEROINE_CLAIMED);

	return (h == 0) ? (v & 0xFF) : (v >> 8);
}

static void SetClaimByte(u8 h, u8 b)
{
	u16 v = VarGet(HACK_VAR_HEROINE_CLAIMED);

	if (h == 0)
		v = (v & 0xFF00) | b;
	else
		v = (v & 0x00FF) | ((u16) b << 8);
	VarSet(HACK_VAR_HEROINE_CLAIMED, v);
}

//枠が変わっていたら今の枠の使用を 0 に戻す
static void SlotSync(void)
{
	u16 slot = gSaveBlock2->playTimeHours * 2 + ((gSaveBlock2->playTimeMinutes >= 30) ? 1 : 0);

	if (VarGet(HACK_VAR_HEROINE_SLOT) != slot)
	{
		VarSet(HACK_VAR_HEROINE_SLOT, slot);
		VarSet(HACK_VAR_HEROINE_USED, 0);
		FlagClear(HACK_FLAG_HEROINE_TALK_HARUKA);       //話しかけの印も枠ごと（#76 段 2）
		FlagClear(HACK_FLAG_HEROINE_TALK_HIKARI);
	}
}

#define USED_PAIR_SHIFT(h) (((h) == 0) ? 2 : 9)

static void GetGiftPoints(u8 h, u8* item, u8* money)
{
	u16 pair = (VarGet(HACK_VAR_HEROINE_USED) >> USED_PAIR_SHIFT(h)) & 0x7F;
	u8 i;

	for (i = 0; i <= HEROINE_SLOT_CAP; ++i)
	{
		if (pair >= i * (HEROINE_SLOT_CAP + 1) && pair < (i + 1) * (HEROINE_SLOT_CAP + 1))
		{
			*item = i;
			*money = pair - i * (HEROINE_SLOT_CAP + 1);
			return;
		}
	}
	*item = HEROINE_SLOT_CAP;      //壊れた値は上限として扱う
	*money = HEROINE_SLOT_CAP;
}

static void SetGiftPoints(u8 h, u8 item, u8 money)
{
	u16 used = VarGet(HACK_VAR_HEROINE_USED);
	u8 sh = USED_PAIR_SHIFT(h);

	used = (used & ~(0x7F << sh)) | ((item * (HEROINE_SLOT_CAP + 1) + money) << sh);
	VarSet(HACK_VAR_HEROINE_USED, used);
}

//渡す必要のある（しきい値を越えていて、まだ渡していない）いちばん低い段 2〜5。無ければ 0
static u8 LowestPending(u8 h)
{
	u8 s, stage = GetStage(h), claim = GetClaimByte(h);

	for (s = 2; s <= stage && s <= HEROINE_STAGES; ++s)
		if (!(claim & (1 << (s - 1))))
			return s;
	return 0;
}

/* ---------------- 電話帳・着信（src/hack_phone.c から） ---------------- */

//着信を鳴らす人（段が上がったのに まだ知らせていない・登録済み）。無ければ 0xFF
u8 HackHeroine_PendingCall(void)
{
	u8 h;

	for (h = 0; h < HEROINE_COUNT; ++h)
	{
		u8 notified = (GetClaimByte(h) >> CL_NOTIFY_SHIFT) & CL_NOTIFY_MASK;

		if (FlagGet(PhoneFlag(h)) && HackHeroine_Unlocked(h) && GetStage(h) > notified)
			return h;
	}
	return 0xFF;
}

//callasm（着信の台本の頭）: Var8003 の人に知らせたことにして、種類を Var8007 に置く（1 = 親しくなった一言・2 = プレゼントがある）
void HackHeroine_CallBegin(void)
{
	u8 h = WhoIsCalling(), stage = GetStage(h);
	u8 b = GetClaimByte(h);

	Var8007 = (LowestPending(h) != 0) ? 2 : 1;
	b = (b & ~(CL_NOTIFY_MASK << CL_NOTIFY_SHIFT)) | (stage << CL_NOTIFY_SHIFT);
	SetClaimByte(h, b);
}

//callasm: 電話帳のメニューを出す前に。LASTRESULT 1 = 渡す物がある（「うけとる」を出す）
void HackHeroine_HasPending(void)
{
	gSpecialVar_LastResult = (LowestPending(WhoIsCalling()) != 0) ? 1 : 0;
}

//callasm（かける）: 30 分の枠に 1 回だけ好感度 +2。LASTRESULT = いまの段（0〜5・会話の変え方に使う）
void HackHeroine_Phone(void)
{
	u8 h = WhoIsCalling();

	SlotSync();
	if (!(VarGet(HACK_VAR_HEROINE_USED) & (1 << h)))
	{
		VarSet(HACK_VAR_HEROINE_USED, VarGet(HACK_VAR_HEROINE_USED) | (1 << h));
		AddAffinity(h, HEROINE_PHONE_GAIN);
	}
	gSpecialVar_LastResult = GetStage(h);
}

/* ---------------- あげる（道具） ---------------- */

static u8 ItemGain(u8 h, u16 item)
{
	u8 i;

	for (i = 0; i < 3; ++i)
		if (sHeroineFavorite[h][i] == item)
			return HEROINE_GIFT_ITEM_FAVORITE;
	if (GetPocketByItemId(item) == POCKET_TM_CASE)
		return HEROINE_GIFT_ITEM_GOOD;
	for (i = 0; i < HEROINE_STONE_COUNT; ++i)
		if (sHeroineStones[i] == item)
			return HEROINE_GIFT_ITEM_GOOD;
	return HEROINE_GIFT_ITEM_NORMAL;
}

//callasm: 選んだ道具（Var800E）を 1 つ渡す。LASTRESULT 5 = げんなま／1 = 渡した（gStringVar1 = 道具の名前）／2 = 渡せない道具（だいじなもの）／3 = いまは もう じゅうぶん（受け取らない）／4 = 選ばなかった
/* げんなま（隠しの裏技・別の枝 task/FEAT-gennama・patches/0868・道具 0x36F）。#76 の枝には道具が無いので、まとめの枝で道具の定義が入れば
   items.h の定義が使われ、無ければここの値（同じ 0x36F）になる（#ifndef＝まとめで二重定義にならない）。好感度 +60・30 分の枠の外・255 で頭打ち */
#ifndef ITEM_GENNAMA
#define ITEM_GENNAMA 0x036F
#endif
#define HEROINE_GENNAMA_GAIN 60

void HackHeroine_GiveItem(void)
{
	u8 h = WhoIsCalling(), itemPts, moneyPts, gain, rem;
	u16 item = gSpecialVar_ItemId;

	SlotSync();
	GetGiftPoints(h, &itemPts, &moneyPts);
	if (item == ITEM_NONE)           //バッグで何も選ばずに戻った（B）: 何も言わずに終わる
	{
		gSpecialVar_LastResult = 4;
		return;
	}
	if (GetPocketByItemId(item) == POCKET_KEY_ITEMS || !CheckBagHasItem(item, 1))
	{
		gSpecialVar_LastResult = 2;
		return;
	}
	if (item == ITEM_GENNAMA)        //好物の判定・30 分の枠より先（枠の外・1 個消費・専用の文）
	{
		RemoveBagItem(item, 1);
		AddAffinity(h, HEROINE_GENNAMA_GAIN);
		gSpecialVar_LastResult = 5;
		return;
	}
	if (itemPts >= HEROINE_SLOT_CAP)
	{
		gSpecialVar_LastResult = 3;
		return;
	}
	gain = ItemGain(h, item);
	rem = HEROINE_SLOT_CAP - itemPts;
	if (gain > rem)
		gain = rem;
	RemoveBagItem(item, 1);
	SetGiftPoints(h, itemPts + gain, moneyPts);
	AddAffinity(h, gain);
	CopyItemName(item, gStringVar1);
	gSpecialVar_LastResult = 1;
}

/* ---------------- あげる（お金）: 上下 ±1,000・左右 ±10,000・L 最小・R 最大・A 決定・B やめる ---------------- */

#define MONEY_WIN_LEFT 12
#define MONEY_WIN_WIDTH 14
#define tMoneyWin data[0]
#define tMoneyN data[1]
#define tMoneyMax data[3]

extern const u8 gText_MomSav_DepYen[];

static u32 Get32(s16* lo)
{
	return (u16) lo[0] | ((u32) (u16) lo[1] << 16);
}

static void Put32(s16* lo, u32 v)
{
	lo[0] = v & 0xFFFF;
	lo[1] = v >> 16;
}

//今の枠で渡せる最大の額（1,000 円単位・上限は残りの点に入る段まで）。0 = 渡せない
static u32 MoneyMax(u8 h)
{
	u8 itemPts, moneyPts, rem;
	u32 cap, allowed, units = 0;

	SlotSync();
	GetGiftPoints(h, &itemPts, &moneyPts);
	if (moneyPts >= HEROINE_SLOT_CAP)
		return 0;
	rem = HEROINE_SLOT_CAP - moneyPts;
	if (rem >= sHeroineMoneyPoints[0])
		cap = HEROINE_MONEY_MAX;
	else if (rem >= sHeroineMoneyPoints[1])
		cap = sHeroineMoneyTier[0] - HEROINE_MONEY_UNIT;
	else
		cap = sHeroineMoneyTier[1] - HEROINE_MONEY_UNIT;
	allowed = GetMoney(&gSaveBlock1->money);
	if (allowed > cap)
		allowed = cap;
	while ((units + 1) * HEROINE_MONEY_UNIT <= allowed)
		++units;
	return units * HEROINE_MONEY_UNIT;
}

static u8 MoneyPoints(u32 amount)
{
	u8 i;

	for (i = 0; i < 3; ++i)
		if (amount >= sHeroineMoneyTier[i])
			return sHeroineMoneyPoints[i];
	return 0;
}

//callasm: Var8004/Var8005 = 渡せる最大の額。LASTRESULT 0 = 渡せる／1 = お金が足りない（1,000 円に届かない）／2 = いまは もう じゅうぶん（受け取らない）
void HackHeroine_MoneyMax(void)
{
	u8 h = WhoIsCalling(), itemPts, moneyPts;
	u32 max = MoneyMax(h);

	GetGiftPoints(h, &itemPts, &moneyPts);
	Var8004 = max & 0xFFFF;
	Var8005 = max >> 16;
	if (max != 0)
		gSpecialVar_LastResult = 0;
	else
		gSpecialVar_LastResult = (moneyPts >= HEROINE_SLOT_CAP) ? 2 : 1;
}

static void MoneyDraw(u8 window, u32 n)
{
	u8 buf[20];

	FillWindowPixelBuffer(window, PIXEL_FILL(1));
	ConvertIntToDecimalStringN(buf, n, STR_CONV_MODE_RIGHT_ALIGN, 7);
	StringAppend(buf, gText_MomSav_DepYen);
	AddTextPrinterParameterized(window, 2, buf, 8, 0, 0xFF, NULL);
	PutWindowTilemap(window);
	CopyWindowToVram(window, COPYWIN_BOTH);
}

static void Task_HeroineMoney(u8 taskId)
{
	s16* data = gTasks[taskId].data;
	u32 n = Get32(&tMoneyN), max = Get32(&tMoneyMax), next = n;
	u16 keys = gMain.newAndRepeatedKeys;
	u32 lo = (max < HEROINE_MONEY_UNIT) ? max : HEROINE_MONEY_UNIT;
	bool8 decided = (gMain.newKeys & A_BUTTON) != 0;

	if (keys & DPAD_UP)
		next = (n + HEROINE_MONEY_UNIT > max) ? max : n + HEROINE_MONEY_UNIT;
	if (keys & DPAD_DOWN)
		next = (n > lo + HEROINE_MONEY_UNIT) ? n - HEROINE_MONEY_UNIT : lo;
	if (keys & DPAD_RIGHT)
		next = (n + 10 * HEROINE_MONEY_UNIT > max) ? max : n + 10 * HEROINE_MONEY_UNIT;
	if (keys & DPAD_LEFT)
		next = (n > lo + 10 * HEROINE_MONEY_UNIT) ? n - 10 * HEROINE_MONEY_UNIT : lo;
	if (gMain.newKeys & R_BUTTON)
		next = max;
	if (gMain.newKeys & L_BUTTON)
		next = lo;
	if (next != n)
	{
		PlaySE(SE_SELECT);
		Put32(&tMoneyN, next);
		MoneyDraw(tMoneyWin, next);
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
	ClearStdWindowAndFrameToTransparent(tMoneyWin, TRUE);
	RemoveWindow(tMoneyWin);
	DestroyTask(taskId);
	EnableBothScriptContexts();
}

//callasm → waitstate: 金額を入れる画面（Var8004/Var8005 = 最大）。LASTRESULT 1 = 決めた（Var8004/Var8005 に額）／0 = B
void HackHeroine_MoneyOpen(void)
{
	struct WindowTemplate win;
	u32 max = Var8004 | ((u32) Var8005 << 16);
	u32 first = (max < HEROINE_MONEY_UNIT) ? max : HEROINE_MONEY_UNIT;
	u8 windowId, taskId;
	s16* data;

	win.bg = 0;
	win.tilemapLeft = MONEY_WIN_LEFT;
	win.tilemapTop = 1;
	win.width = MONEY_WIN_WIDTH;
	win.height = 2;
	win.paletteNum = 15;
	win.baseBlock = 0x038;
	windowId = AddWindow(&win);
	DrawStdWindowFrame(windowId, FALSE);
	taskId = CreateTask(Task_HeroineMoney, 80);
	data = gTasks[taskId].data;
	tMoneyWin = windowId;
	Put32(&tMoneyN, first);
	Put32(&tMoneyMax, max);
	MoneyDraw(windowId, first);
}

//callasm: Var8004/Var8005 の額を渡す（手持ち・上限を越えて渡さない）。gStringVar1 = 渡した額・LASTRESULT 1 = 渡した
void HackHeroine_MoneyApply(void)
{
	u8 h = WhoIsCalling(), itemPts, moneyPts, pts, rem;
	u32 amount = Var8004 | ((u32) Var8005 << 16);
	u32 max = MoneyMax(h);

	if (amount > max)
		amount = max;
	pts = MoneyPoints(amount);
	if (amount == 0 || pts == 0)
	{
		gSpecialVar_LastResult = 0;
		return;
	}
	GetGiftPoints(h, &itemPts, &moneyPts);
	rem = HEROINE_SLOT_CAP - moneyPts;
	if (pts > rem)
		pts = rem;
	RemoveMoney(&gSaveBlock1->money, amount);
	SetGiftPoints(h, itemPts, moneyPts + pts);
	AddAffinity(h, pts);
	ConvertIntToDecimalStringN(gStringVar1, amount, STR_CONV_MODE_LEFT_ALIGN, 7);
	gSpecialVar_LastResult = 1;
}

/* ---------------- うけとる ---------------- */

static bool8 PcHasRoom(u16 item, u16 count)
{
	u32 i;
	struct ItemSlot* pc = gSaveBlock1->pcItems;

	for (i = 0; i < PC_ITEMS_COUNT; ++i)
		if (pc[i].itemId == item && pc[i].quantity + count <= HEROINE_PC_ITEM_MAX)
			return TRUE;
	for (i = 0; i < PC_ITEMS_COUNT; ++i)
		if (pc[i].itemId == ITEM_NONE || pc[i].quantity == 0)
			return TRUE;
	return FALSE;
}

static bool8 CanTake(u16 item, u8 count)
{
	return item == ITEM_NONE || CheckBagHasSpace(item, count) || PcHasRoom(item, count);
}

//バッグへ・入らなければパソコンへ（入る見込みは CanTake が先に確かめる）。パソコンへ送ったら Var8006 を 1 に
static void TakeItem(u16 item, u8 count)
{
	u8 i;

	if (item == ITEM_NONE)
		return;
	if (CheckBagHasSpace(item, count) && AddBagItem(item, count))
		return;
	for (i = 0; i < count; ++i)
		HackMomSavings_AddPcItem(item);
	Var8006 = 1;
}

//callasm: Var8003 の人の、渡す物があるいちばん低い段を渡す。Var8004 = 渡した段（2〜5）・Var8006 = 0 そのまま／1 = 道具がパソコンへ／2 = タマゴがボックスへ
//LASTRESULT 1 = 渡した／0 = 渡す物が無い／2 = バッグもパソコンも手持ちもボックスも入らない（受け取らない・次にもう一度）
void HackHeroine_Receive(void)
{
	u8 h = WhoIsCalling(), s = LowestPending(h);
	const struct HeroineReward* r;

	Var8006 = 0;
	if (s == 0)
	{
		gSpecialVar_LastResult = 0;
		return;
	}
	r = &sHeroineReward[h][s - 1];
	Var8004 = s;
	if (r->egg != 0)
	{
		struct Pokemon egg;
		u8 res;

		CreateEgg(&egg, r->egg);
		res = GiveMonToPlayer(&egg);
		if (res == MON_CANT_GIVE)
		{
			gSpecialVar_LastResult = 2;
			return;
		}
		if (res == MON_GIVEN_TO_PC)
			Var8006 = 2;
	}
	else
	{
		if (!CanTake(r->item1, r->count1) || !CanTake(r->item2, r->count2))
		{
			gSpecialVar_LastResult = 2;
			return;
		}
		TakeItem(r->item1, r->count1);
		TakeItem(r->item2, r->count2);
	}
	SetClaimByte(h, GetClaimByte(h) | (1 << (s - 1)));
	gSpecialVar_LastResult = 1;
}

/* ============================================================================================================
 * 段 2a: ハルカの連れ歩き（人が主人公の後ろを付いてくる）・電話から呼ぶ・話しかけ・水／自転車の自動待機
 *       （Redmine #76・FEAT-heroine-76-s2・patches/0867）
 *
 * ★ 連れ歩き中の人の変数 HACK_VAR_HEROINE_FOLLOWER（0x50C3）: 下位 8 ビット = 0 いない・1 ハルカ（2 ヒカリは段 3＝後ろ姿ができてから）・
 *   ビット 8 = 「ここで まってるね」をまだ言っていない（水・ダイビングで引っ込めたとき立てる）。
 * ★ 仕組み: CFRU の人の連れ歩き（src/follow_me.c）が土台。ポケモンの連れ歩き（src/bw_follower.c）は localId 0xFE の自前の枠を使うが、こちらは
 *   0xFD（assembly/bwfollow/bw_follower_hooks.s の BwFollowerTemplateHook が gHeroineFollowerTemplate を返す）。「map.id が 0xFE 以外＝人の連れ歩き」の
 *   既存の振り分けにそのまま乗る＝ポケモンの連れ歩きは出ず、ボールの演出とも混ざらない。変数が「付いている」を表し、毎コマの見張り（FieldTick）が
 *   付けられる所では付け・付けられない所（なみのり・自転車・ダイビング・施設・通信・殿堂入りの部屋）では引っ込める。
 * ★ 地図を作り直すとき（つづきから・ワープ）は src/bw_follower.c の BwFollowerUpdateFromParty が HackHeroine_BuildState で状態を組み直す
 *   （台本の番地を今の番地に入れ直す＝6.88）。
 * ============================================================================================================ */

#define HEROINE_GFX_HIKARI 0x07A1       /* シンオウのヒカリの歩く絵（tools/rom/build_hoenn_ow.py の SINNOH_HIKARI・patches/0870 で aveontrainer 氏の絵に差し替え） */
#define HEROINE_LOCAL_ID 0xFD           /* assembly/bwfollow/bw_follower_hooks.s の cmp r0, #0xFD と同じ（ポケモンの連れ歩きは 0xFE） */
#define FOLLOW_WHO_MASK 0x00FF
#define FOLLOW_MSG_PENDING 0x0100
#define HEAL_MSG_PENDING 0x0800          /* 戦闘のあとの回復の言葉をまだ言っていない（patches/0875・新しい変数は使わずビット 9〜11 を借りる） */
#define HEAL_TIER_SHIFT 9                /* ビット 9〜10 = 回復の段階 0〜3 */
#define HEAL_TIER_MASK 0x0600
#define HEROINE_FOLLOW_FLAGS (FOLLOWER_FLAG_CAN_BIKE | FOLLOWER_FLAG_CAN_LEAVE_ROUTE | FOLLOWER_FLAG_CAN_SURF \
                            | FOLLOWER_FLAG_CAN_WATERFALL | FOLLOWER_FLAG_CAN_DIVE | FOLLOWER_FLAG_CAN_ROCK_CLIMB)
#define HEROINE_FOLLOW_MIN_STAGE 3       /* 電話の「つれあるく」は好感度の段 3（120）から */
#define HEROINE_TALK_GAIN 3              /* 話しかけ +3（30 分の枠に 1 回） */

extern const u8 HackScr_HeroineTalk[], HackScr_HeroineWait[], HackScr_HeroineHeal[];
void FollowMe_HeroineSyncSurf(void);   //src/follow_me.c（patches/0923 #195）

const struct EventObjectTemplate gHeroineFollowerTemplate =
{
	.localId = HEROINE_LOCAL_ID,
	.graphicsIdLowerByte = EVENT_OBJ_GFX_MAY,     //実際の絵は CreateFollowerAvatar が gFollowerState.gfxId で上書きする
	.graphicsIdUpperByte = 0,
	.x = 0,
	.y = 0,
	.elevation = 3,
	.movementType = 0,                            //自分では動かない
	.movementRangeX = 0,
	.movementRangeY = 0,
	.trainerType = 0,
	.trainerRange_berryTreeId = 0,
	.script = HackScr_HeroineTalk,
	.flagId = 0,
};

static u8 FollowWho(void)
{
	u8 who = VarGet(HACK_VAR_HEROINE_FOLLOWER) & FOLLOW_WHO_MASK;

	if ((who == 1 || who == 2) && !HackHeroine_Unlocked(who - 1))
		return 0;       //クリア前は付いていないものとして扱う（変数の値は消さない・クリア後にそのまま使える・patches/0920）
	return who;
}

//連れ歩きの人の値（1 ハルカ・2 ヒカリ）か
static bool8 WhoValid(u8 who)
{
	return who == 1 || who == 2;
}

//その人の歩く絵（連れ歩きの物体の gfxId）
static u16 GfxOf(u8 who)
{
	return (who == 2) ? HEROINE_GFX_HIKARI : EVENT_OBJ_GFX_MAY;
}

static bool8 HeroineObjectPresent(void)
{
	return gFollowerState.inProgress && gFollowerState.map.id == HEROINE_LOCAL_ID;
}

bool8 HackHeroine_IsFollowing(void)
{
	return HeroineObjectPresent();
}

//付けられる場所か（地図: 施設・通信・殿堂入りの部屋・水中・フロンティア）
static bool8 PlaceOk(void)
{
	return !BwFollowerBannedHere() && !FlagGet(FLAG_BATTLE_FACILITY);
}

//主人公の状態が付けられるか。自転車は 2026-10-26（patches/0869）・なみのりは 2026-10-27（patches/0923・#195）から付いて来る（絵は gFollowerAlternateSprites）。
//水中（ダイビング）は PlaceOk の地図の種別で待機のまま
static bool8 AvatarOk(void)
{
	return TRUE;
}

//BwFollowerUpdateFromParty（地図を作り直すとき）から: 人の連れ歩きを組むか
bool8 HackHeroine_WantsFollow(void)
{
	return WhoValid(FollowWho()) && PlaceOk();
}

//連れ歩きの状態を組む（まだ物体は作らない＝InitPlayerAvatar の CreateFollowerAvatar が作る）
void HackHeroine_BuildState(void)
{
	u8 who = FollowWho();
	u16 gfx = GfxOf(who);

	if (HeroineObjectPresent() && gFollowerState.gfxId == gfx)
	{
		//同じ人が続いているだけ: ワープの途中で CFRU が立てた warpEnd・comeOutDoorStairs を消さない（ポケモンの連れ歩きと同じ約束）
		if (gFollowerState.map.number != (u8) gSaveBlock1->location.mapNum || gFollowerState.map.group != (u8) gSaveBlock1->location.mapGroup)
			gFollowerState.comeOutDoorStairs = FALSE;
		if (gFollowerState.warpEnd
		 || gFollowerState.map.number != (u8) gSaveBlock1->location.mapNum
		 || gFollowerState.map.group != (u8) gSaveBlock1->location.mapGroup)
			gFollowerState.delayedState = 0;
		gFollowerState.map.number = gSaveBlock1->location.mapNum;
		gFollowerState.map.group = gSaveBlock1->location.mapGroup;
		gFollowerState.flags = HEROINE_FOLLOW_FLAGS;
		gFollowerState.script = HackScr_HeroineTalk;   //6.88: 保存された台本の番地は組み直しで古くなる＝毎回今の番地に
		return;
	}
	gFollowerState.inProgress = TRUE;
	gFollowerState.gfxId = gfx;
	gFollowerState.map.id = HEROINE_LOCAL_ID;
	gFollowerState.map.number = gSaveBlock1->location.mapNum;
	gFollowerState.map.group = gSaveBlock1->location.mapGroup;
	gFollowerState.script = HackScr_HeroineTalk;
	gFollowerState.flag = 0;
	gFollowerState.flags = HEROINE_FOLLOW_FLAGS;
	gFollowerState.createSurfBlob = 0;
	gFollowerState.comeOutDoorStairs = FALSE;
	gFollowerState.warpEnd = TRUE;                //最初の一歩で出てくる（ドアの演出は通さない）
	gFollowerState.locked = 0;
	gFollowerState.delayedState = 0;
	gFollowerState.currentSprite = 0;
}

//今いる連れ歩き（ポケモンでも人でも）の物体を片付けて「いない」にする
static void RemoveCurrentFollower(void)
{
	if (gFollowerState.inProgress)
	{
		HideFollower();     //なみのりの台を作っていたらそれも片付く（CFRU）
		if (gFollowerState.objId != 0 && gFollowerState.objId < EVENT_OBJECTS_COUNT && gEventObjects[gFollowerState.objId].active
		 && !gEventObjects[gFollowerState.objId].isPlayer)
			RemoveEventObject(&gEventObjects[gFollowerState.objId]);
	}
	gFollowerState.inProgress = FALSE;
	gFollowerState.createSurfBlob = 0;
}

//見張りの付け方: ポケモンの連れ歩きがいたら片付けて、人の連れ歩きを作る（見えないまま主人公の足もと・最初の一歩で出てくる）
static void StartHeroineFollower(void)
{
	RemoveCurrentFollower();
	HackHeroine_BuildState();
	CreateFollowerAvatar();
}

//引っ込める（言葉は FieldTick が主人公が止まってから）
static void StopHeroineFollower(bool8 sayWait)
{
	bool8 wasVisible = FALSE;

	if (HeroineObjectPresent() && gFollowerState.objId < EVENT_OBJECTS_COUNT)
		wasVisible = !gEventObjects[gFollowerState.objId].invisible;
	RemoveCurrentFollower();
	if (sayWait && wasVisible)
		VarSet(HACK_VAR_HEROINE_FOLLOWER, VarGet(HACK_VAR_HEROINE_FOLLOWER) | FOLLOW_MSG_PENDING);
}

static bool8 FieldIdle(void)
{
	return gMain.callback2 == CB2_Overworld && !gPaletteFade->active && !ScriptContext2_IsEnabled()
	    && gPlayerAvatar->tileTransitionState == T_NOT_MOVING && gPlayerAvatar->runningState == NOT_MOVING
	    && !gPlayerAvatar->preventStep && !BwFollowerBallBusy() && !BwFollowerCutsceneFieldEffectActive();
}

//src/overworld_speedup.c が毎コマ呼ぶ（フィールドの間だけ）
void HackHeroine_FieldTick(void)
{
	u8 who = FollowWho();
	bool8 present = HeroineObjectPresent();
	bool8 canBe = WhoValid(who) && PlaceOk() && AvatarOk();

	if (who == 0)
	{
		if (present && FieldIdle())     //外部から変数が消えた（わかれた・デバッグ）: 引っ込めてポケモンの連れ歩きに戻す
		{
			StopHeroineFollower(FALSE);
			BwFollowerUpdateFromParty();
			CreateFollowerAvatar();
		}
		return;
	}
	if (!WhoValid(who))
	{
		VarSet(HACK_VAR_HEROINE_FOLLOWER, 0);   //知らない値（1 ハルカ・2 ヒカリ以外）は連れ歩かない
		return;
	}
	if (present && !canBe)
	{
		StopHeroineFollower(gMapHeader.mapType == MAP_TYPE_UNDERWATER);   //ダイビングで水中に入ったときだけ「ここで まってるね」
		return;
	}
	if (!present && canBe && FieldIdle())
	{
		StartHeroineFollower();
		return;
	}
	if (present && canBe && FieldIdle() && gFollowerState.gfxId != GfxOf(who))   //ハルカからヒカリへ（外から変数を書き換えた）: 作り直す
	{
		StartHeroineFollower();
		return;
	}
	//なみのり中なのに連れ歩きが歩く絵（つづきから・ダイビングの浮上・電話で呼んだ）: なみのりの絵と台に切り替える（patches/0923 #195）
	if (present && canBe && FieldIdle())
		FollowMe_HeroineSyncSurf();
	//そらをとぶ・デバッグのワープのあと: 見えないまま（invisible・warpEnd = 0）で残ることがある（2026-10-26 mGBA で確認）。
	//ポケモンの連れ歩きと同じ直し方（bw_follower.c「見えないまま置き去り」）: 止まっていれば一歩目で出す印（warpEnd）を立てる
	if (present && canBe && FieldIdle() && !gFollowerState.warpEnd && !gFollowerState.comeOutDoorStairs
	 && gFollowerState.objId < EVENT_OBJECTS_COUNT && gEventObjects[gFollowerState.objId].invisible)
	{
		gFollowerState.warpEnd = TRUE;
		return;
	}
	//戦闘のあとの回復の言葉（patches/0875）: 付いていて主人公が止まり、台本も動いていないとき
	if (present && canBe && (VarGet(HACK_VAR_HEROINE_FOLLOWER) & HEAL_MSG_PENDING) && FieldIdle() && gScriptEnv1.mode == 0)   //トレーナー戦のあとは、その台本（trainerbattle の続き）が終わってから＝動いている台本を上書きしない
	{
		ScriptContext2_Enable();
		ScriptContext1_SetupScript(HackScr_HeroineHeal);
		return;
	}
	if (!present && !canBe && (VarGet(HACK_VAR_HEROINE_FOLLOWER) & FOLLOW_MSG_PENDING) && FieldIdle())
	{
		VarSet(HACK_VAR_HEROINE_FOLLOWER, VarGet(HACK_VAR_HEROINE_FOLLOWER) & ~FOLLOW_MSG_PENDING);
		ScriptContext2_Enable();
		ScriptContext1_SetupScript(HackScr_HeroineWait);
	}
}

/* ---------------- 電話のメニュー（台本から） ---------------- */

//電話帳のメニューで「つれあるく」を出せるか: 0 = 出さない／1 = つれあるく／2 = かえってもらう
//ヒカリも段 3（好感度 120）から（h＝0 ハルカ・1 ヒカリ・連れ歩きの値は h + 1）。同時に連れ歩けるのは 1 人＝もう一人が付いている間は
//「つれあるく」を出したままにして、押すと「いま 〇〇と いっしょ」（FollowToggle が 3 を返す）
static u8 FollowMenuKind(u8 h)
{
	if (h > 1 || GetStage(h) < HEROINE_FOLLOW_MIN_STAGE)
		return 0;
	return (FollowWho() == h + 1) ? 2 : 1;
}

//callasm（メニューを出す前）: Var8007 = ビット 0 渡す物がある・ビット 1〜2 つれあるく項目の種類（0 なし／1 つれあるく／2 かえってもらう）
void HackHeroine_MenuBegin(void)
{
	u8 h = WhoIsCalling();

	Var8007 = ((LowestPending(h) != 0) ? 1 : 0) | (FollowMenuKind(h) << 1);
}

//callasm: 選んだ番号（LASTRESULT）を 動作に直す。0 かける／1 あげる／2 うけとる／3 つれあるく／かえってもらう／4 やめる
void HackHeroine_MenuMap(void)
{
	u16 sel = gSpecialVar_LastResult;
	u16 flags = Var8007;
	u8 a;
	u8 slot = 0;

	for (a = 0; a < 5; ++a)
	{
		bool8 present;

		switch (a) {
			case 2: present = (flags & 1) != 0; break;
			case 3: present = (flags & 6) != 0; break;
			default: present = TRUE; break;
		}
		if (!present)
			continue;
		if (slot == sel)
		{
			gSpecialVar_LastResult = a;
			return;
		}
		++slot;
	}
	gSpecialVar_LastResult = 4;     //B キーなど
}

//callasm: 呼ぶ／帰ってもらう。LASTRESULT 1 = 呼んだ／2 = かえってもらった／0 = いまは呼べない（水・自転車・施設）
void HackHeroine_FollowToggle(void)
{
	u8 h = WhoIsCalling();
	u8 kind = FollowMenuKind(h);

	if (kind == 2)
	{
		VarSet(HACK_VAR_HEROINE_FOLLOWER, 0);
		gSpecialVar_LastResult = 2;
		return;
	}
	if (kind == 1 && WhoValid(FollowWho()))   //もう一人が付いている（入れ替えはしない・ユーザー 10/26）
	{
		gSpecialVar_LastResult = 3;
		return;
	}
	if (kind == 1 && PlaceOk() && AvatarOk() && !TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_BIKE))   //呼ぶのは降りてから（出る絵が歩きのため）
	{
		VarSet(HACK_VAR_HEROINE_FOLLOWER, h + 1);
		gSpecialVar_LastResult = 1;
		return;
	}
	gSpecialVar_LastResult = 0;
}

/* ---------------- 話しかけ（連れ歩き中） ---------------- */

//callasm: 話しかけ +3（30 分の枠に 1 回・旗で覚える）。LASTRESULT = いまの段
void HackHeroine_TalkBegin(void)
{
	u8 h = WhoIsCalling();
	u16 flag = (h == 0) ? HACK_FLAG_HEROINE_TALK_HARUKA : HACK_FLAG_HEROINE_TALK_HIKARI;

	SlotSync();
	if (!FlagGet(flag))
	{
		FlagSet(flag);
		AddAffinity(h, HEROINE_TALK_GAIN);
	}
	gSpecialVar_LastResult = GetStage(h);
}

//callasm（連れ歩きの台本から）: LASTRESULT = 今付いている人が 1 ハルカ／2 ヒカリ（台本の分け方に使う）
void HackHeroine_WhoIsFollowing(void)
{
	gSpecialVar_LastResult = FollowWho();
}

//callasm（わかれる）: 連れ歩きをやめる。見張りがすぐ片付けるのではなく、ここで引っ込めてポケモンの連れ歩きを戻す
void HackHeroine_Dismiss(void)
{
	VarSet(HACK_VAR_HEROINE_FOLLOWER, 0);
	StopHeroineFollower(FALSE);
	BwFollowerUpdateFromParty();
	CreateFollowerAvatar();
}

/* ---------------- 戦闘: 相方として戦わない戦い（段 2b） ---------------- */

bool8 HackKanto_IsGymLeaderBattle(void);           //src/hack_kanto_dynamax.c
bool8 HackDynamax_IsChampionBattle(void);
bool8 HackDynamax_IsWatashitachiBattle(void);

//トレーナー戦の始まり（gTrainerBattleOpponent_A が決まったあと）で呼ぶ。FALSE ならハルカは相方にならず、ふつうのシングルの戦い（ハルカは立ったまま）
bool8 HackHeroine_BattleAllowed(void)
{
	u16 id = gTrainerBattleOpponent_A;
	u8 cls, i;

	if (FlagGet(FLAG_BATTLE_FACILITY) || FlagGet(FLAG_DYNAMAX_BATTLE) || FlagGet(HACK_FLAG_OAK_PIC))
		return FALSE;
	if (IsFrontierTrainerId(id))
		return FALSE;
	if ((id >= HEROINE_STORY_SINNOH_FIRST && id <= HEROINE_STORY_SINNOH_LAST) || (id >= HEROINE_STORY_HOENN_FIRST && id <= HEROINE_STORY_HOENN_LAST))
		return FALSE;
	if (HackKanto_IsGymLeaderBattle() || HackDynamax_IsChampionBattle() || HackDynamax_IsWatashitachiBattle())
		return FALSE;
	cls = gTrainers[id].trainerClass;
	if (cls >= NUM_TRAINER_CLASSES)
		return FALSE;                  //範囲外のクラスは出さない
	for (i = 0; i < HEROINE_DENIED_CLASS_COUNT; ++i)
		if (sHeroineDeniedClass[i] == cls)
			return FALSE;
	return TRUE;
}

/* ---------------- 戦闘のあとの回復（patches/0875・Redmine #76 追加） ---------------- */

//好感度から回復の段階（0〜3）。sHeroineHealFrom の中で好感度がその値以上の最も高い段階
static u8 HealTierOf(u8 affinity)
{
	u8 i, t = 0;

	for (i = 0; i < HEROINE_HEAL_TIERS; ++i)
		if (affinity >= sHeroineHealFrom[i])
			t = i;
	return t;
}

bool8 HackChallengeC_IsLeagueTrainer(u16 trainerId); //src/hack_challenge_c.c

//リーグ（四天王・チャンピオン）の戦いのあとか。リーグでは回復を 1/4 までにする（2026-11-03 ユーザー「リーグでの なつき度マックスの HP 回復効果を 1/4 まで」「PP MAX も同じように」）
static bool8 LeagueBattle(void)
{
	return (gBattleTypeFlags & BATTLE_TYPE_TRAINER) && HackChallengeC_IsLeagueTrainer(gTrainerBattleOpponent_A);
}

//PP を 1/4（技ごとに最大 PP の 1/4・最低 1）足す。リーグ用
static void HealPPQuarter(struct Pokemon* mon)
{
	u8 i, bonuses = GetMonData(mon, MON_DATA_PP_BONUSES, NULL);

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u16 move = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
		u8 pp = GetMonData(mon, MON_DATA_PP1 + i, NULL);
		u8 max, add;

		if (move == MOVE_NONE)
			continue;
		max = CalculatePPWithBonus(move, bonuses, i);
		if (pp >= max)
			continue;
		add = max / 4;
		if (add == 0)
			add = 1;
		pp = (pp + add > max) ? max : pp + add;
		SetMonData(mon, MON_DATA_PP1 + i, &pp);
	}
}

//手持ち 1 匹の回復（ひんし＝HP 0 は段階 3 だけ）。たまごは触らない
//league: リーグの戦いのあと。HP も PP も 1/4 まで（最低 1）・ひんしは起こさない・状態異常は段階 1 以上で今までどおり治す
static void HealOne(struct Pokemon* mon, u8 tier, bool8 league)
{
	u32 none = 0;
	u32 hp = GetMonData(mon, MON_DATA_HP, NULL);
	u32 max = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	u32 add;

	if (league)
	{
		if (hp == 0)
			return;
		add = max / 4;   //リーグはどの段階も 1/4
		if (add == 0)
			add = 1;
		if (hp < max)
		{
			hp += add;
			if (hp > max)
				hp = max;
			SetMonData(mon, MON_DATA_HP, &hp);
		}
		HealPPQuarter(mon);
		if (tier >= 1)
			SetMonData(mon, MON_DATA_STATUS, &none);
		return;
	}
	if (tier == 3)
	{
		HealMon(mon);                     //HP・PP・状態異常を全部（ひんしも起こす）
		return;
	}
	if (hp == 0)
		return;                           //ひんしは起こさない（ポケセンの役目）
	if (hp < max)
	{
		add = (tier == 0) ? max / 4 : (tier == 1) ? max / 2 : max;
		if (add == 0)
			add = 1;
		hp += add;
		if (hp > max)
			hp = max;
		SetMonData(mon, MON_DATA_HP, &hp);
	}
	if (tier >= 1)
		SetMonData(mon, MON_DATA_STATUS, &none);   //状態異常を治す
}

//end_battle.c の EndOfBattleThings から（勝ち・逃げた・捕まえたなど。全滅・引き分けと、通信・施設のあとは回復しない）
void HackHeroine_AfterBattle(void)
{
	u8 who = FollowWho();
	u8 outcome = gBattleOutcome & 0x7F;
	u8 tier, i;
	bool8 league;
	u16 species;

	if (!WhoValid(who) || !HeroineObjectPresent() || FlagGet(FLAG_BATTLE_FACILITY))
		return;
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_POKE_DUDE | BATTLE_TYPE_OLD_MAN))
		return;
	if (outcome == B_OUTCOME_LOST || outcome == B_OUTCOME_DREW)
		return;
	tier = HealTierOf(GetAffinity(who - 1));
	league = LeagueBattle();
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
		if (species == SPECIES_NONE || species == SPECIES_EGG)
			continue;
		HealOne(&gPlayerParty[i], tier, league);
	}
	if (league)
		tier = 0;                         //リーグは言葉も「すこし かいふく」（全部 かいふくと言わない）
	VarSet(HACK_VAR_HEROINE_FOLLOWER, (VarGet(HACK_VAR_HEROINE_FOLLOWER) & ~HEAL_TIER_MASK) | HEAL_MSG_PENDING | (tier << HEAL_TIER_SHIFT));
}

//callasm（HackScr_HeroineHeal）: LASTRESULT = (人 - 1) * 4 + 回復の段階。言った印を消す
void HackHeroine_HealPick(void)
{
	u16 v = VarGet(HACK_VAR_HEROINE_FOLLOWER);
	u8 who = v & FOLLOW_WHO_MASK;

	gSpecialVar_LastResult = ((who == 2) ? 4 : 0) + ((v & HEAL_TIER_MASK) >> HEAL_TIER_SHIFT);
	VarSet(HACK_VAR_HEROINE_FOLLOWER, v & ~(HEAL_MSG_PENDING | HEAL_TIER_MASK));
}
