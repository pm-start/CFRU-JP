#include "defines.h"
#include "hack_orcaiser.h" //[hack] patches/0952 HACK_IS_ORCAISER_LINE
#include "hack_bugfix_decide.h"
#include "defines_battle.h"
#include "../include/data2.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/pokemon.h"
#include "../include/pokedex.h"
#include "../include/script.h"
#include "../include/constants/items.h"
#include "../include/constants/pokemon.h"
#include "../include/new/achievements.h"
#include "hack_challenge.h"
#include "hack_challenge_c.h"
#include "hack_dummy_battle.h" //[hack] patches/0602 ポケセン 3F のダミー
#include "hack_challenge_luxray.h" //[hack] patches/0609 ぜんぶレントラー（モノタイプの基準のタイプ）
#include "hack_mon_id.h"

/*
    しばり 縛り C（0.50.3・#121・patches/0550・資料 docs/research/2026-10-16_settings-folders-challenges.md 13 章）。5 つとも既定は OFF。
      ポケセンきんし        ポケセンの看護師の台本（素の Common_EventScript_PkmnCenterNurse 0x08193E46 を呼ぶ 20 か所を repoints で EventScript_ChallengeNurse に）が断る。
                            全滅のあとの回復・家のベッド・お母さん・PC は止めない。ポケセンに入ること自体は可
      おみせきんし          script コマンド 0x86 pokemart（表 0x08162CC4 の 0x08162EDC・routinepointers）を置き換え、ON のあいだ お店（買う・売る とも）を開かない
      ひんしのどうぐきんし  バッグの「つかう」（hack_challenge.c の 2 つの入口）で げんきのかけら・げんきのかたまり・ふっかつそう・せいなるはい を止める
      モノタイプ            最初の 1 匹（旗が ON のとき 手持ちの先頭の子）の 2 つのタイプのどちらかを 1 つでも持つ子だけ 戦闘に出せる。
                            出せない子は CreateBattleStartTask の入口（challenge_c_hooks.s）で HP を 0 にして（HP は覚えておく）戦闘へ入る＝素の戦闘が ひんしと同じに扱う
                            （先頭に選ばれない・「たたかう気がない」・経験値が入らない）。戦闘のあと EndOfBattleThings の頭で HP を戻す。
                            出せる子（HP 1 以上）が 1 匹もいないときは何もしない（詰み防止）
      ランダム              CreateWildMon（草むら・つり・いわくだき…）・ScriptGiveMon（もらう子・御三家）・CreateNPCTrainerParty の手持ちの種族を、
                            セーブの表 ID・裏 ID（SaveBlock2 の playerTrainerId）と元の種族と場所（地図／トレーナー番号と何匹目）で決まる 図鑑 1〜1039 の基本の姿に置き換える（同じ場所・同じ元なら 同じ結果）。
                            性格値には触らない（色違い・Moémon・6.5／#70 はそのまま）。伝説・幻（HackLegendRaid_IsLegend）・わたしたちの伝説・デバッグモンは 元でも先でも除く
    戻り先: ポケセンの縛り（りょうきん・？？？・きんし）が 1 つでも ON のとき、全滅（region_fly_hook.s の RegionWhiteoutHook → HackChallengeC_WhiteoutHome）とバージョンアップのワープ
            （hack_save_version.c → HackChallengeC_VersionHome）をマサラタウンの主人公の家 1F にし、手持ちを全回復（HealPlayerParty）する
*/

#define MONO(i) (HackChallengeC_MonoData() + (i))

bool8 HackLegendRaid_IsLegend(u16 species);   //src/hack_legend_raid.c
void HackRocketBall_FieldTick(void);           //src/hack_rocket_ball.c（patches/0914）
void HackBadgeSquash_FieldTick(void);          //src/hack_battle_squash.c
u16 NationalPokedexNumToSpecies(u16 nationalNum);

#define IS_WATASHI_LEGEND(sp) ((sp) >= SPECIES_TSUKIRIRU && (sp) <= SPECIES_ICHIGOBERRY)
#define IS_DEBUG_MON(sp) ((sp) == SPECIES_DEBUGMON_MAX || (sp) == SPECIES_DEBUGMON_MIN)

static u8* HackChallengeC_MonoData(void)
{
	return gSaveBlock2->filler_B20 + HACK_CHC_MONO_OFFSET;
}

/* ---- ON の数・戻り先 ---- */

bool8 HackChallengeC_CenterBound(void)
{
	return FlagGet(HACK_FLAG_CH_CENTERFEE) || FlagGet(HACK_FLAG_CH_LENTRANT) || FlagGet(HACK_FLAG_CHC_CENTERBAN);
}

bool8 HackChallengeC_AnyOn(void)
{
	return FlagGet(HACK_FLAG_CHC_CENTERBAN) || FlagGet(HACK_FLAG_CHC_SHOPBAN) || FlagGet(HACK_FLAG_CHC_NOREVIVE)
		|| FlagGet(HACK_FLAG_CHC_MONOTYPE) || FlagGet(HACK_FLAG_CHC_RANDOM);
}

u8 HackChallengeC_CountOn(void)
{
	return (FlagGet(HACK_FLAG_CHC_CENTERBAN) != 0) + (FlagGet(HACK_FLAG_CHC_SHOPBAN) != 0) + (FlagGet(HACK_FLAG_CHC_NOREVIVE) != 0)
		+ (FlagGet(HACK_FLAG_CHC_MONOTYPE) != 0) + (FlagGet(HACK_FLAG_CHC_RANDOM) != 0);
}

//region_fly_hook.s の RegionWhiteoutHook（素の SetWhiteoutRespawnWarpAndHealerNpc の中）から。1 を返すと 素の「戻り先の番号 1」＝マサラの家（母が回復する場面）で続ける。
//ここで先に手持ちを全回復する（ひんしでお別れ＝縛り A の「外す」は戦闘のあと EndOfBattleThings で済んでいる＝外してから回復）
u8 HackChallengeC_WhiteoutHome(void)
{
	if (!HackChallengeC_CenterBound())
		return 0;
	HealPlayerParty();
	return 1;
}

//hack_save_version.c の HackSaveVersion_OnContinue（バージョンアップの「最後のポケセンへのワープ」）から。ポケセンの縛りが ON のときは マサラの家へ・全回復
bool8 HackChallengeC_VersionHome(void)
{
	if (!HackChallengeC_CenterBound())
		return FALSE;
	HealPlayerParty();
	return TRUE;
}

/* ---- おみせきんし ---- */

extern const u8 EventScript_ChallengeShopBan[];
#define VanillaScrCmdPokemart ((bool8 (*)(struct ScriptContext*)) 0x0806BBA1)

bool8 HackChallengeC_ScrCmdPokemart(struct ScriptContext* ctx)
{
	if (FlagGet(HACK_FLAG_CHC_SHOPBAN))
	{
		ScriptReadWord(ctx);   //品物の表のポインタを読み捨てる（素の命令と同じだけ進める）
		ScriptCall(ctx, EventScript_ChallengeShopBan);   //断りの文を出して return → 素の台本の次の行へ（「またどうぞ」）
		return FALSE;
	}
	return VanillaScrCmdPokemart(ctx);
}

/* ---- ひんしのどうぐきんし ---- */

bool8 HackChallengeC_ReviveBlocked(u16 item)
{
	if (!FlagGet(HACK_FLAG_CHC_NOREVIVE))
		return FALSE;
	return item == ITEM_REVIVE || item == ITEM_MAX_REVIVE || item == ITEM_REVIVAL_HERB || item == ITEM_SACRED_ASH;
}

/* ---- モノタイプ ---- */

static bool8 MonoEligible(const struct Pokemon* mon, u8 t1, u8 t2)
{
	u16 species = GetMonData((struct Pokemon*) mon, MON_DATA_SPECIES2, NULL);
	u8 a, b;

	if (species == SPECIES_NONE || species == SPECIES_EGG || species >= NUM_SPECIES)
		return TRUE;   //空き・タマゴは数えない
	a = gBaseStats[species].type1;
	b = gBaseStats[species].type2;
	return a == t1 || a == t2 || b == t1 || b == t2;
}

static void MonoRecord(void)   //旗が ON で まだ記録していないとき、手持ちの先頭（タマゴでない子）のタイプを覚える
{
	u8* d = MONO(0);
	u32 i;

	if (!FlagGet(HACK_FLAG_CHC_MONOTYPE) || d[0] != 0)
		return;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);

		if (species == SPECIES_NONE)
			return;   //手持ちが空（まだ最初の 1 匹をもらっていない）
		if (species == SPECIES_EGG || species >= NUM_SPECIES)
			continue;
		d[0] = gBaseStats[species].type1 + 1;
		d[1] = gBaseStats[species].type2 + 1;
		return;
	}
}

static u32 ReadPid(const u8* p)
{
	return (u32) p[0] | ((u32) p[1] << 8) | ((u32) p[2] << 16) | ((u32) p[3] << 24);
}

static void WritePid(u8* p, u32 v)
{
	p[0] = v;
	p[1] = v >> 8;
	p[2] = v >> 16;
	p[3] = v >> 24;
}

//CreateBattleStartTask の入口（assembly/hack_challenge/challenge_c_hooks.s）から。通信・施設は除く
void HackChallengeC_OnBattleStart(void)
{
	if (HackDummy_Active()) //[hack] patches/0602 ポケセン 3F のダミーの戦いは モノタイプで出せない子を HP 0 にしない
		return;
	u8* d = MONO(0);
	u8 t1, t2, marked = 0;
	u32 i, eligible = 0;
	u16 zero = 0;

	if (!FlagGet(HACK_FLAG_CHC_MONOTYPE) || d[0] == 0 || d[2] != 0)
		return;
	if ((gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)) || FlagGet(FLAG_BATTLE_FACILITY))
		return;
	t1 = d[0] - 1;
	t2 = d[1] != 0 ? d[1] - 1 : t1;
	if (HackChallengeLuxray_On())   //[hack] patches/0609 ぜんぶレントラー: 最初の 1 匹はレントラー（でんき）＝基準のタイプはでんき。途中でON/OFFしても記録は書き換えない（OFF に戻せば元のタイプに戻る）。レントラーはみな出せる＝詰まない
		t1 = t2 = TYPE_ELECTRIC;
	for (i = 0; i < PARTY_SIZE; ++i)   //出せる子（HP 1 以上）が 1 匹もいないなら 何もしない
	{
		struct Pokemon* mon = &gPlayerParty[i];
		u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);

		if (species != SPECIES_NONE && species != SPECIES_EGG && GetMonData(mon, MON_DATA_HP, NULL) != 0 && MonoEligible(mon, t1, t2))
			++eligible;
	}
	if (eligible == 0)
		return;
	for (i = 0; i < PARTY_SIZE && marked < HACK_CHC_MONO_SLOTS; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];
		u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);
		u16 hp = GetMonData(mon, MON_DATA_HP, NULL);

		if (species == SPECIES_NONE || species == SPECIES_EGG || hp == 0 || MonoEligible(mon, t1, t2))
			continue;
		WritePid(d + 4 + 6 * marked, GetMonData(mon, MON_DATA_PERSONALITY, NULL));
		d[8 + 6 * marked] = hp;
		d[9 + 6 * marked] = hp >> 8;
		++marked;
		SetMonData(mon, MON_DATA_HP, &zero);
	}
	d[2] = marked;
}

//戦闘のあと（end_battle.c の EndOfBattleThings の頭＝ひんしでお別れ（縛り A）の「外す」より前）と フィールドの安全装置から。性格値で子を探す（戦闘中に並びが入れ替わるため）
void HackChallengeC_MonoRestore(void)
{
	u8* d = MONO(0);
	u32 i, j;

	if (d[2] == 0)
		return;
	for (i = 0; i < d[2] && i < HACK_CHC_MONO_SLOTS; ++i)
	{
		u32 pid = ReadPid(d + 4 + 6 * i);
		u16 hp = d[8 + 6 * i] | (d[9 + 6 * i] << 8);

		for (j = 0; j < PARTY_SIZE; ++j)
		{
			if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) != SPECIES_NONE
			&&  GetMonData(&gPlayerParty[j], MON_DATA_PERSONALITY, NULL) == pid)
			{
				SetMonData(&gPlayerParty[j], MON_DATA_HP, &hp);
				break;
			}
		}
		WritePid(d + 4 + 6 * i, 0);
	}
	d[2] = 0;
}

/* ---- ランダム ---- */

static u32 Mix(u32 x)
{
	x ^= x >> 16;
	x *= 0x045D9F3B;
	x ^= x >> 16;
	x *= 0x045D9F3B;
	x ^= x >> 16;
	return x;
}

static bool8 RandomOk(u16 species)
{
	return species != SPECIES_NONE && species != SPECIES_EGG && species < NUM_SPECIES && gBaseStats[species].baseHP != 0
		&& !IS_WATASHI_LEGEND(species) && !IS_DEBUG_MON(species) && !HackLegendRaid_IsLegend(species)
		&& !HACK_IS_FOSSIL_ONLY_MON(species); //[hack] patches/0952 コシャル系（入手は化石の復元だけ）はランダムの行き先にしない
}

//元の種族 original と場所の印 key で決まる 図鑑 1〜NATIONAL_DEX_COUNT の基本の姿（セーブの表 ID・裏 ID が種）。元が伝説などなら そのまま
static u16 RandomSpecies(u16 original, u32 key)
{
	u32 seed = T1_READ_32(gSaveBlock2->playerTrainerId);
	u32 i;

	if (ChallengeC_KeepSpecies(original, SPECIES_NONE, SPECIES_EGG, NUM_SPECIES,
		IS_WATASHI_LEGEND(original), IS_DEBUG_MON(original) || HACK_IS_FOSSIL_ONLY_MON(original), HackLegendRaid_IsLegend(original))) //[hack] patches/0952 元がコシャル系なら そのまま
		return original;   //伝説・幻・わたしたちの伝説・デバッグモン・空き・タマゴは変えない
	for (i = 0; i < 64; ++i)
	{
		u32 h = Mix(seed ^ Mix(key * 0x9E3779B1u + original * 0x85EBCA6Bu + i * 0xC2B2AE35u));
		u16 species = NationalPokedexNumToSpecies(1 + (h >> 4) % NATIONAL_DEX_COUNT);

		if (RandomOk(species))
			return species;
	}
	return original;
}

u16 HackChallengeC_RandomWild(u16 species)
{
	if (!FlagGet(HACK_FLAG_CHC_RANDOM) || HackChallenge_IsLentrantBattle() || FlagGet(FLAG_BATTLE_FACILITY))
		return species;
	return RandomSpecies(species, ((u32) (u8) gSaveBlock1->location.mapGroup << 8) | (u8) gSaveBlock1->location.mapNum);
}

u16 HackChallengeC_RandomGift(u16 species)
{
	if (!FlagGet(HACK_FLAG_CHC_RANDOM) || FlagGet(FLAG_BATTLE_FACILITY) || FlagGet(ACH_FLAG_STARTER_CTX))
		return species;   //じっせきの ほうしゅうは そのまま（中身が決まっている）
	return RandomSpecies(species, 0x7E570000u);
}

/*
 * ポケモンリーグ（四天王・チャンピオン）の戦いは、ランダムでも相手の手持ちを入れ替えない（2026-11-03 ユーザー「リーグ戦では無効にしておいて」）。
 * 見分け方はトレーナー番号の表（各地方の trainers 表の trainerClass が 四天王・チャンピオンの人を全部拾った一覧・出典 docs/research/2026-10-17_random-challenge.md 追記）。
 * 再戦・強化版の番号も入れる。ジムリーダー・ライバル・ブレーン・ふつうのトレーナーは入れない。検算 tools/rom/verify_random_league.py が表と trainers 表を突き合わせる。
 * 書き込める static は置かない（6.13）＝const の表だけ。
 */
static const u16 sLeagueTrainerRanges[][2] =
{
	{410, 413},   //カントー 四天王 カンナ・シバ・キクコ・ワタル
	{438, 440},   //カントー チャンピオン（ライバル）3 通り
	{735, 738},   //カントー 四天王の再戦
	{739, 741},   //カントー チャンピオンの再戦 3 通り
	{809, 812},   //ホウエン 四天王 カゲツ・フヨウ・プリム・ゲンジ
	{813, 813},   //ホウエン チャンピオン ミクリ
	{814, 817},   //ホウエン 四天王の強化版（ゲンジ・プリム・フヨウ・カゲツ）
	{818, 818},   //ホウエン ミクリの強化版
	{912, 915},   //ジョウト 四天王 イツキ・キョウ・シバ・カリン
	{916, 916},   //ジョウト チャンピオン ワタル
	{922, 925},   //シンオウ 四天王 リョウ・キクノ・オーバ・ゴヨウ
	{926, 926},   //シンオウ チャンピオン シロナ
};

bool8 HackChallengeC_IsLeagueTrainer(u16 trainerId)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sLeagueTrainerRanges); ++i)
	{
		if (trainerId >= sLeagueTrainerRanges[i][0] && trainerId <= sLeagueTrainerRanges[i][1])
			return TRUE;
	}
	return FALSE;
}

//build_pokemon.c の CreateNPCTrainerParty の手持ちを 1 匹作ったあと（HackClassPool_ReplaceMon の直後）。種族だけ入れ替え、レベル・性格値（色違い・Moémon）・親・ボール・なかよしは そのまま。
//技は CreateMon が レベルで覚える技を入れる（型・持ち物は外す＝モモカ博士・きんこんしき・ヒガン・わたしたち（935〜949）と 施設は除く）
void HackChallengeC_RandomTrainerMon(struct Pokemon* mon, u16 trainerId, u8 side, u8 index)
{
	u16 original, species;
	u16 level, otGender, ball, friendship, fixedIv, otName[PLAYER_NAME_LENGTH + 1];
	u32 otId, personality;

	if (!FlagGet(HACK_FLAG_CHC_RANDOM) || side != B_SIDE_OPPONENT || FlagGet(FLAG_BATTLE_FACILITY) || (trainerId >= 935 && trainerId <= 949) || HackChallengeC_IsLeagueTrainer(trainerId))
		return;
	original = GetMonData(mon, MON_DATA_SPECIES, NULL);
	species = RandomSpecies(original, 0x70000000u + ((u32) trainerId << 4) + index);
	if (species == original)
		return;
	level = GetMonData(mon, MON_DATA_LEVEL, NULL);
	otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
	personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
	otGender = GetMonData(mon, MON_DATA_OT_GENDER, NULL);
	ball = GetMonData(mon, MON_DATA_POKEBALL, NULL);
	friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
	fixedIv = GetMonData(mon, MON_DATA_HP_IV, NULL);
	GetMonData(mon, MON_DATA_OT_NAME, otName);

	CreateMon(mon, species, level, fixedIv, TRUE, personality, OT_ID_PRESET, otId);
	SetMonData(mon, MON_DATA_OT_NAME, otName);
	SetMonData(mon, MON_DATA_OT_GENDER, &otGender);
	SetMonData(mon, MON_DATA_POKEBALL, &ball);
	SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
}

/* ---- ？？？の いかり（0.50.3 ユーザー: ？？？にダメージを入れたら／？？？の攻撃が当たったら 手持ち全員ひんし＝負け）---- */

//kind 1 = こちらから ？？？に ダメージ（HP が減る・0 になる）・2 = ？？？の攻撃が こちらに当たった
void HackChallengeC_LentrantWrath(u8 kind)
{
	u8* d = MONO(0);
	u16 zero = 0;
	u32 i;

	if (FlagGet(HACK_FLAG_CHC_WRATH_MSG))
		return;   //もう始まっている
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
			SetMonData(&gPlayerParty[i], MON_DATA_HP, &zero);
	}
	for (i = 0; i < gBattlersCount; ++i)
	{
		if (SIDE(i) == B_SIDE_PLAYER)
			gBattleMons[i].hp = 0;
	}
	for (i = 0; i < HACK_CHC_MONO_SLOTS; ++i)
		WritePid(d + 4 + 6 * i, 0);
	d[2] = 0;   //モノタイプで ひんしにした子の HP も戻さない（全員ひんし）
	d[3] = kind;
	FlagSet(HACK_FLAG_CHC_WRATH_MSG);
	if (gBattleOutcome == 0)
		gBattleOutcome = B_OUTCOME_LOST;
}

//end_turn.c の HandleDynamaxOnTurnEnd の頭から（ターンの終わりに毎回）。battleTurnCounter はターンの終わりの「あと」で増える＝ +1 で数える
void HackChallengeC_TurnLimit(void)
{
	if (FlagGet(HACK_FLAG_CH_BATTLE) && !(gBattleTypeFlags & BATTLE_TYPE_LINK)
	&& gBattleResults.battleTurnCounter + 1 >= HACK_CHC_LENTRANT_TURNS)
		HackChallengeC_LentrantWrath(HACK_CHC_WRATH_TIME);
}

bool8 HackChallengeC_NoSwitch(u8 bank)
{
	return FlagGet(HACK_FLAG_CH_BATTLE) && SIDE(bank) == B_SIDE_PLAYER && !(gBattleTypeFlags & BATTLE_TYPE_LINK);
}

//battle_start_turn_start.c の BattleBeginFirstTurn（BTSTART_BACKUP_PARTY_ITEMS・戦闘の最初に 1 回）から。？？？との戦いだけ。
//戦闘の中の写し（gBattleMons）の技を「ひっかく」1 つにする（手持ちの本当の技は書き換えない）。PP の引き算は手持ちの 1 つ目の技の PP に書かれるが、
//？？？との戦いは かならず ぜんめつ＝回復で PP も元に戻る。Z 技・ダイマックス・テラスタル・メガシンカ・ウルトラバーストは「もう使った」ことにして出さない
void HackChallengeC_LentrantRules(void)
{
	u32 b, m;

	if (!FlagGet(HACK_FLAG_CH_BATTLE) || (gBattleTypeFlags & BATTLE_TYPE_LINK))
		return;
	for (b = 0; b < gBattlersCount; ++b)
	{
		if (SIDE(b) != B_SIDE_PLAYER)
			continue;
		for (m = 0; m < MAX_MON_MOVES; ++m)
		{
			gBattleMons[b].moves[m] = (m == 0) ? MOVE_SCRATCH : MOVE_NONE;
			gBattleMons[b].pp[m] = (m == 0) ? gBattleMoves[MOVE_SCRATCH].pp : 0;
		}
		gNewBS->megaData.done[b] = TRUE;
		gNewBS->ultraData.done[b] = TRUE;
		gNewBS->zMoveData.used[b] = TRUE;
		gNewBS->dynamaxData.used[b] = TRUE;
		gNewBS->terastalData.done[b] = TRUE;
	}
}

//callasm（台本 EventScript_ChallengeWrath）: LASTRESULT = いかりの種類
void HackChallengeC_WrathKind(void)
{
	gSpecialVar_LastResult = MONO(0)[3];
	MONO(0)[3] = 0;
}

/* ---- フィールドの毎コマ ---- */

extern const u8 EventScript_ChallengeWrath[];

void HackChallengeC_FieldTick(void)
{
	if ((gMain.vblankCounter2 & 0xF) != 0)
		return;
	HackRocketBall_FieldTick(); //patches/0914 安全装置: 奪われた子が戻っていなければ戻す
	HackBadgeSquash_FieldTick(); //#124 安全装置: 戦闘が終わって野外に戻っているのに 上限のレベルにした子が戻っていなければ戻す
	MonoRecord();
	//安全装置: 戦闘が始まらなかった／戻し損ねたときの HP の戻し（戦闘の前後は台本・戦闘の画面で ScriptContext2 が有効＝その間は動かない）
	if (MONO(0)[2] != 0 && gNewBS == NULL && !ScriptContext2_IsEnabled() && !gPaletteFade->active && gMain.callback2 == CB2_Overworld)
		HackChallengeC_MonoRestore();
	if (FlagGet(HACK_FLAG_CHC_WRATH_MSG))
	{
		if (gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld || gNewBS != NULL)
			return;
		FlagClear(HACK_FLAG_CHC_WRATH_MSG);
		ScriptContext1_SetupScript(EventScript_ChallengeWrath);
	}
}
