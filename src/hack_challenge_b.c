#include "defines.h"
#include "defines_battle.h"
#include "../include/battle_string_ids.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/string_util.h" //[hack] patches/0613 ConvertIntToDecimalStringN・StringExpandPlaceholders
#include "../include/text.h" //gStringVar1・gStringVar4
#include "../include/sound.h"
#include "../include/constants/songs.h"
#include "../include/new/battle_strings.h"
#include "../include/new/battle_util.h"
#include "../include/new/exp.h"
#include "../include/new/rom_locs.h"
#include "hack_challenge_b.h"
#include "hack_dummy_battle.h" //[hack] patches/0602 ポケセン 3F のダミー

/*
    しばり B（#118・0.50.3）。docs/research/2026-10-16_challenges-b.md

      レベルキャップ     自分のポケモンは HackLevelCap_FromBadges(GetAllBadgeCount())（src/hack_level_cap.c・4 個ごとの区切り 20〜180）をこえて レベルが上がらない。
                         ★ 0.50.6 から 縛りが OFF でも「今いる地方のバッジによる上限」（HackLevelCap_FromPlace・#124）がいつも効く。縛りが ON なら 2 つの低いほう。
                         下の HackChallengeB_LevelCap／AtLevelCap／ExpBlocked／ClampCandyExp／DaycareExp の名前は縛り B の時のまま（呼ぶ側の upstream パッチ 0540 を変えない）。
                         戦闘の経験値（exp.c）・ふしぎなアメ（party_menu.c）・けいけんアメ（exp_candy.c）・そだてや（daycare.c）の 4 か所。上限の子も 努力値は入る
      こうたいきんし     戦闘のメニューで「いれかえ」を選んでも入れ替わらない（move_menu.c）・バトルスタイルを「かえない」に（battle_start_turn_start.c）。
                         ひんしのあとの入れ替え・とんぼがえり／バトンタッチ／ほえる などの技・だっしゅつボタンなどの道具で強制されるときは そのまま
      やせいけいけんなし  野生（トレーナー戦でない）のポケモンを倒しても けいけんちも努力値も入らない（exp.c・ユーザー 2026-10-16）
      ダメージ1.5ばい    相手側の攻撃が自分の側の子に与えるダメージ（atk05_damagecalc を通るもの）を 1.5 倍（0.63.0 で 2 倍から・damage + damage/2＝切り捨て・1 は 1 のまま）。きあいのタスキ・こらえる は 1.5 倍のあとで効く。
                         1.5 倍にしないもの: 固定ダメージの技・どく／やけど／天気／まきびし などの毎ターンのダメージ・反動・こんらんの自傷・みらいよち／はめつのねがい
      そらをとぶきんし    SetUpFieldMove_Fly（party_menu.c）が FALSE ＝ 「そらをとぶ」も「ゆうらん」（Fly Reloaded は同じ関数を呼ぶ）も「ここでは つかえない」

    どれも通信（BATTLE_TYPE_LINK）・バトルフロンティア・バトル施設は対象外（戦闘の中の しばり）。
*/

u8 GetAllBadgeCount(void);                 //src/badge_count.c（全地方の合計）
u8 HackLevelCap_FromBadges(u8 badges);     //src/hack_level_cap.c
u8 HackLevelCap_FromPlace(void);           //src/hack_level_cap.c（今いる地方のバッジによる上限・0 ＝ なし）

extern u8 String_ChB_LevelCapped[];
extern u8 String_ChB_NoWildExp[];
extern const u8 gText_ChB_NoSwitch[];

/* ---- 戦闘の中のしばりが効く戦いか ---- */

static bool8 InCountedBattle(void)
{
	return !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)) && !FlagGet(FLAG_BATTLE_FACILITY) && !HackDummy_Active(); //[hack] patches/0602 ポケセン 3F のダミーの練習は 縛り B の対象外（こうたいきんし・ダメージ2ばい・レベルキャップの経験値止め）
}

/* ---- レベルキャップ ---- */

//自分のポケモンのレベルの上限（0 ＝ 上限なし）。**経験値・レベルが上がる道を塞ぐのは この関数の値だけ**（戦闘・ふしぎなアメ・けいけんアメ・そだてや）。
//  ① いつも効く: 今いる地方のバッジによる上限（HackLevelCap_FromPlace・#124・0.50.6）  ② レベルキャップの縛りが ON: 全地方のバッジの合計の上限  → 両方あれば低いほう
u8 HackChallengeB_LevelCap(void)
{
	u8 cap = HackLevelCap_FromPlace();

	if (FlagGet(HACK_FLAG_CHB_LEVELCAP))
	{
		u8 total = HackLevelCap_FromBadges(GetAllBadgeCount());

		if (total < 1)
			total = 1;
		if (cap == 0 || total < cap)
			cap = total;
	}
	return cap;
}

bool8 HackChallengeB_AtLevelCap(struct Pokemon* mon)
{
	u8 cap = HackChallengeB_LevelCap();

	return cap != 0 && GetMonData(mon, MON_DATA_LEVEL, NULL) >= cap;
}

//[hack] patches/0613 手持ちの画面（ふしぎなアメ・けいけんアメ）の上限の一言。数字は 止める判定と同じ HackChallengeB_LevelCap。上限なし（0）のときは 数字を出さず ふつうの「こうかがない」を返す（呼び元は AtLevelCap のときだけ呼ぶので 通常は来ない）
extern const u8 gText_ChB_CandyCapped[];
#define gText_WontHaveEffect ((const u8*) 0x83DDE0B)   /* 素 FRLG の「こうかがない」（party_menu.c などと同じ・BPRJ.ld に名前は無い） */
const u8* HackChallengeB_CandyCappedText(void)
{
	u8 cap = HackChallengeB_LevelCap();

	if (cap == 0)
		return gText_WontHaveEffect;
	ConvertIntToDecimalStringN(gStringVar1, cap, STR_CONV_MODE_LEFT_ALIGN, 3);
	StringExpandPlaceholders(gStringVar4, gText_ChB_CandyCapped);
	return gStringVar4;
}

//けいけんアメ・そだてやなど: 経験値を「上限のレベルになったちょうどの経験値」でとめる（それ以上は足さない）
static u32 CapExp(u16 species)
{
	u8 cap = HackChallengeB_LevelCap();

	if (cap == 0 || cap > MAX_LEVEL)
		return 0xFFFFFFFF;
	return gExperienceTables[gBaseStats[species].growthRate][cap];
}

u32 HackChallengeB_ClampCandyExp(u16 species, u32 exp)
{
	u32 max = CapExp(species);

	return exp > max ? max : exp;
}

u32 HackChallengeB_DaycareExp(u16 species, u32 originalExp, u32 newExp)
{
	u32 max = CapExp(species);

	if (originalExp >= max)
		return originalExp;   //もう上限（すでに上なら何も足さない）
	return newExp > max ? max : newExp;
}

/* ---- 戦闘の経験値（exp.c の GetExp_CheckCurrentMonDeserving から）---- */

static u8 PlayerFieldBank(u8 partyId)   //自分の子が場に出ていればその戦闘位置・出ていなければ 0xFF
{
	u8 bank = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);

	if (gBattlerPartyIndexes[bank] == partyId && BATTLER_ALIVE(bank))
		return bank;
	if (IS_DOUBLE_BATTLE)
	{
		bank = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
		if (gBattlerPartyIndexes[bank] == partyId && BATTLER_ALIVE(bank))
			return bank;
	}
	return 0xFF;
}

//経験値を入れない子なら 1（上限・努力値だけ入れて次の子へ）か 2（やせいけいけんなし・努力値も入れない）。入れる子は 0。participant ＝ 戦いに出た子としてのループ（学習装置の分は一言を出さない）。
//場に出ている子に 1 回だけ一言（戦闘のメッセージ 0x184＝gBattleStringLoader の文・学習装置の文と同じ道）
u8 HackChallengeB_ExpBlocked(u8 partyId, bool8 participant)
{
	u8 reason = 0;
	u8 bank;
	bool8 wild = FlagGet(HACK_FLAG_CHB_NOWILDEXP) && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER);

	if (!InCountedBattle())
		return 0;
	if (HackChallengeB_AtLevelCap(&gPlayerParty[partyId]))
		reason = 1;
	else if (wild)
		reason = 2;
	if (reason == 0)
		return 0;

	bank = PlayerFieldBank(partyId);
	if (participant && bank != 0xFF)
	{
		if (reason == 1)
		{
			PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, bank, partyId)
			PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff2, 3, HackChallengeB_LevelCap()) //[hack] patches/0613 上限の数字（止める判定 AtLevelCap と同じ HackChallengeB_LevelCap・reason 1 のときは 0 でない）
			gBattleStringLoader = String_ChB_LevelCapped;
		}
		else
			gBattleStringLoader = String_ChB_NoWildExp;
		PrepareStringBattle(0x184, 0);
	}
	return wild ? 2 : 1;   //2 ＝ 野生（経験値も努力値も入れない）・1 ＝ 上限（努力値だけ入れる）
}

/* ---- こうたいきんし ---- */

bool8 HackChallengeB_SwitchBlocked(void)
{
	return FlagGet(HACK_FLAG_CHB_NOSWITCH) && InCountedBattle();
}

//move_menu.c の HandleInputChooseAction（「いれかえ」を選んだとき）: 入れ替えずに 音と一言（質問の窓に出す・次の操作でふつうの画面に戻る）
void HackChallengeB_ShowSwitchBlocked(void)
{
	PlaySE(SE_ERROR);
	BattleStringExpandPlaceholdersToDisplayedString(gText_ChB_NoSwitch);
	BattlePutTextOnWindow(gDisplayedStringBattle, 1);
}

/* ---- ダメージ1.5ばい（旗の名前 DOUBLEDMG は旧名のまま・番号 0xFB8 も同じ）---- */

s32 HackChallengeB_ModifyDamage(u8 bankDef, s32 damage)
{
	if (damage <= 0 || !FlagGet(HACK_FLAG_CHB_DOUBLEDMG) || !InCountedBattle())
		return damage;
	if (SIDE(bankDef) != B_SIDE_PLAYER || SIDE(gBankAttacker) != B_SIDE_OPPONENT)
		return damage;
	if (damage > 0x3FFFFFFF)
		return 0x7FFFFFFF;
	return damage + damage / 2;   //1.5 倍（切り捨て）。最大 0x3FFFFFFF のとき 0x5FFFFFFE で収まる
}

/* ---- そらをとぶきんし ---- */

bool8 HackChallengeB_FlyBlocked(void)
{
	return FlagGet(HACK_FLAG_CHB_NOFLY) != 0;
}

/* ---- 最後のバッジのメッセージ ---- */

static const u16 sFlagsB[HACK_CHB_COUNT] =
{
	HACK_FLAG_CHB_LEVELCAP, HACK_FLAG_CHB_NOSWITCH, HACK_FLAG_CHB_NOWILDEXP, HACK_FLAG_CHB_DOUBLEDMG, HACK_FLAG_CHB_NOFLY,
};

bool8 HackChallengeB_AnyOn(void)
{
	return HackChallengeB_CountOn() != 0;
}

u8 HackChallengeB_CountOn(void)
{
	u8 n = 0;
	u32 i;

	for (i = 0; i < HACK_CHB_COUNT; ++i)
	{
		if (FlagGet(sFlagsB[i]))
			++n;
	}
	return n;
}
