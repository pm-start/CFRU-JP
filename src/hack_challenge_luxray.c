#include "defines.h"
#include "defines_battle.h"
#include "../include/data2.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/pokemon.h"
#include "../include/pokedex.h"
#include "../include/string_util.h"
#include "../include/constants/species.h"
#include "../include/new/learn_move.h"
#include "../include/new/rom_locs.h"
#include "../include/new/terastal.h"
#include "hack_challenge.h"
#include "hack_challenge_luxray.h"
#include "hack_dummy_battle.h"
#include "hack_mon_id.h"

/*
 * しばり「ぜんぶレントラー」（#140 仮・2026-10-18・docs/research/2026-10-18_challenge-luxray-syakakiri.md）。
 * ユーザー「しばり新しいの追加、捕まえたポケモンすべてレントラー♀ moemon 色違いに強制的にする」「技はレベルに合わせて」。
 *
 * 手に入れた子を 1 か所（HackChallengeLuxray_OnObtain）で レントラー♀・色違い・Moémon にする。入口（どれも 旗が OFF なら何も変えない）:
 *   - 捕獲:    atkF1_trysetcaughtmondexflags（図鑑の「つかまえた」の判定より前＝レントラーの図鑑の画面が出る）・atkF0 の GiveMonToPlayer
 *   - もらう:  GiveMonToPlayer（ScriptGiveMon・ジラーチ・ウルガモス・シリアル・ナナミ・化石・ガチャ…全部これを通る）・SetMonPokedexFlags（先に図鑑を付ける贈り物）
 *   - タマゴ:  GiveMonToPlayer では 変えない（タマゴのままレントラーにすると かえる前に種族が変わる）→ かえるとき（CreateHatchedMon）に変える
 *   - 交換:    素の TradeMons の中の UpdatePokedexForReceivedMon（functionrewrites 08050070）＝ゲーム内の交換・ミラクル交換の場面・通信交換。
 *              ミラクル交換で場面をとばす道（WonderTrade_TrySkipScene）とネット交換（hack_network_trade.c の Apply）は 手持ちへ直に入れるので それぞれ呼ぶ
 *   - 進化で増える: ヌケニン（CreateShedinja）
 *   バトル施設／フロンティアの借り物（FLAG_BATTLE_FACILITY・BATTLE_TYPE_FRONTIER）・ポケセン 3F のダミー戦・？？？・通信対戦は 変えない。相手のポケモンは 触らない
 *
 * 変えるもの: 種族 → レントラー（経験値はレントラーの表の「そのレベル」＝レベルは変わらない）・技 → 今のレベルまでに覚える技の最後の 4 つ（新しく作ったのと同じ）・
 *   ♀（きょせいどうぐと同じ性格値の入れ替え）・色違い＋Moémon（自身の ID だけ・性格値は作り直さない＝6.5）。ニックネームが種族名のままなら 新しい種族名。
 *   特性: 特性の番号（abilityNum／かくれ）はそのまま＝レントラーの同じ番号の特性（かくれ特性が無ければ 1 つ目か 2 つ目）。
 *   変えないもの: 性格・個体値・努力値・なつき度・ボール・親／出会いの記録・ニックネーム（つけた名前）・状態異常・HP の割合（満タンは満タン）
 *   もたせた道具: 「リルの あいじょう」（効果なし・はずせない）に入れ替える。もとの道具はバッグへ（いっぱいならパソコンへ・どちらも無理なら もとのまま＝失くさない）（patches/0617・src/hack_luxray_love.c）
 * 図鑑: もとの種族は「見た」・レントラーは「つかまえた」。
 */

bool8 HackPartner_IsMon(const struct Pokemon* mon);   //src/hack_story_partner.c（ホウエンの相棒は性格値で見分ける＝入れ替えない）

bool8 HackChallengeLuxray_On(void)
{
	return FlagGet(HACK_FLAG_CHE_LUXRAY) != 0;
}

//効く場面か。旗・バトル施設・ダミー戦・？？？・通信/フロンティアの戦闘の最中ではない
static bool8 Active(void)
{
	if (!FlagGet(HACK_FLAG_CHE_LUXRAY))
		return FALSE;
	if (FlagGet(FLAG_BATTLE_FACILITY) || HackDummy_Active() || HackChallenge_IsLentrantBattle())
		return FALSE;
	if (gMain.inBattle && (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)))
		return FALSE;
	return TRUE;
}

//もう ぜんぶ済みの子か（レントラー・♀・色違い・Moémon）＝二度目は何も変えない（技を入れ直さない）
static bool8 AlreadyForced(struct Pokemon* mon)
{
	return GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_LUXRAY
		&& GetMonGender(mon) == MON_FEMALE
		&& HackMonId_IsShinyBox((struct BoxPokemon*) mon)
		&& HackMonId_IsMoemonHit((struct BoxPokemon*) mon);
}

static void MarkLuxrayDex(struct Pokemon* mon)
{
	u16 dex = SpeciesToNationalPokedexNum(SPECIES_LUXRAY);
	u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);

	HandleSetPokedexFlag(dex, FLAG_SET_SEEN, personality);
	HandleSetPokedexFlag(dex, FLAG_SET_CAUGHT, personality);
}

bool8 HackChallengeLuxray_OnObtain(struct Pokemon* mon, bool8 markDex)
{
	u8 nick[POKEMON_NAME_LENGTH + 1];
	u8 name[POKEMON_NAME_LENGTH + 1];
	u16 species, oldSpecies, oldTera, newHp, maxHp;
	u32 level, exp, i;
	bool8 fullHp, defaultName;

	if (!Active() || GetMonData(mon, MON_DATA_IS_EGG, NULL))
		return FALSE;
	species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	if (species == SPECIES_NONE || species == SPECIES_EGG || HackPartner_IsMon(mon))
		return FALSE;
	if (AlreadyForced(mon))
	{
		HackLuxrayLove_Apply(mon);   //すでにレントラー♀・色違い・Moémon の子（最初のポケモンなど）も リルの あいじょうを持たせる（patches/0617）
		if (markDex)
			MarkLuxrayDex(mon);
		return FALSE;
	}

	oldSpecies = species;
	level = GetMonData(mon, MON_DATA_LEVEL, NULL);
	maxHp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	newHp = GetMonData(mon, MON_DATA_HP, NULL);
	fullHp = maxHp != 0 && newHp >= maxHp;
	GetMonData(mon, MON_DATA_NICKNAME, nick);
	GetSpeciesName(name, species);
	defaultName = StringCompare(nick, name) == 0;
	oldTera = mon->teratype;

	if (species != SPECIES_LUXRAY)   //もとの種族は「見た」
		HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_SEEN, GetMonData(mon, MON_DATA_PERSONALITY, NULL));

	//種族とレベル: 経験値はレントラーの表の そのレベルの値（成長の型が違っても レベルは変わらない）
	species = SPECIES_LUXRAY;
	SetMonData(mon, MON_DATA_SPECIES, &species);
	exp = gExperienceTables[gBaseStats[SPECIES_LUXRAY].growthRate][level];
	SetMonData(mon, MON_DATA_EXP, &exp);
	if (defaultName)
	{
		GetSpeciesName(name, SPECIES_LUXRAY);
		SetMonData(mon, MON_DATA_NICKNAME, name);
	}
	if (oldTera == GetSpeciesTeraType(oldSpecies))   //もとの種族のままの（変えていない）テラスタイプなら レントラーのもの。選んで変えた子はそのまま
		mon->teratype = GetSpeciesTeraType(SPECIES_LUXRAY);
	mon->gigantamax = FALSE;

	//技: 今のレベルまでに覚える技の最後の 4 つ（GiveBoxMonInitialMoveset は経験値から レベルを出す＝上で入れた値）
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		mon->moves[i] = MOVE_NONE;
		mon->pp[i] = 0;
	}
	mon->ppBonuses = 0;
	GiveBoxMonInitialMoveset((struct BoxPokemon*) mon);

	HackLuxray_ApplyIdentity(mon);   //♀・色違い・Moémon（最初のポケモンと共有）
	CalculateMonStats(mon);
	if (fullHp)
		newHp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	else
	{
		maxHp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
		if (newHp > maxHp)
			newHp = maxHp;
	}
	SetMonData(mon, MON_DATA_HP, &newHp);
	HackLuxrayLove_Apply(mon);   //もとの持ち物はバッグ（いっぱいならパソコン）へ・リルの あいじょうを持たせる（patches/0617・src/hack_luxray_love.c）
	if (markDex)
		MarkLuxrayDex(mon);
	return TRUE;
}

//functionrewrites: UpdatePokedexForReceivedMon 08050070（素の TradeMons が 入れ替えたあとに呼ぶ。素の関数は タマゴでなければ 見た・つかまえた を付けるだけ）
//・ふだん（旗 OFF）は素と同じ: タマゴでなければ その子の種族を 見た・つかまえた
//・ON のときは 先に レントラーにする（図鑑はレントラーが「つかまえた」・もとの種族は「見た」）
void HackLuxray_UpdatePokedexForReceivedMon(u8 partyIdx)
{
	struct Pokemon* mon = &gPlayerParty[partyIdx];
	u16 dex;

	HackChallengeLuxray_OnObtain(mon, FALSE);
	if (GetMonData(mon, MON_DATA_IS_EGG, NULL))
		return;
	dex = SpeciesToNationalPokedexNum(GetMonData(mon, MON_DATA_SPECIES, NULL));
	GetSetPokedexFlag(dex, FLAG_SET_SEEN);
	GetSetPokedexFlag(dex, FLAG_SET_CAUGHT);
}
