/*
 * ハルカ・ヒカリの好感度の表（Redmine #76 段 1・patches/0866・手書き・src/hack_heroine.c が読む）。
 * 数字はすべて仮（ユーザーが調整する）。tools/rom/verify_heroine_s1.py が件数・実在・昇順を全部確かめる。
 * 人の並びは 0 = ハルカ・1 = ヒカリ。段は 1〜5（表の添字は 段 - 1）。
 */
#ifndef HACK_HEROINE_TABLE_H
#define HACK_HEROINE_TABLE_H

#define HEROINE_COUNT 2
#define HEROINE_STAGES 5
#define HEROINE_AFFINITY_MAX 255
#define HEROINE_PHONE_GAIN 2         /* 電話（「かける」）1 回（30 分の枠ごとに 1 回） */
#define HEROINE_SLOT_CAP 10          /* 30 分の枠ごとの貢ぎの上限（道具・お金それぞれ） */
#define HEROINE_GIFT_ITEM_NORMAL 1   /* ふつうの道具 */
#define HEROINE_GIFT_ITEM_GOOD 3     /* わざマシン・進化の石 */
#define HEROINE_GIFT_ITEM_FAVORITE 6 /* 好物 */
#define HEROINE_MONEY_MAX 100000     /* 1 回の最大 */
#define HEROINE_MONEY_UNIT 1000

/* 段のしきい値（好感度がこの値以上でその段） */
static const u8 sHeroineThreshold[HEROINE_STAGES] = {30, 70, 120, 180, 240};

/* お金の貢ぎ: この額以上でこの点（大きい順に見る）。1,000 円 +1・10,000 円 +4・100,000 円 +8 */
static const u32 sHeroineMoneyTier[3] = {100000, 10000, 1000};
static const u8 sHeroineMoneyPoints[3] = {8, 4, 1};

/* ごほうび（段 2〜5 は「うけとる」で渡す。段 1 は一言の着信だけで物は無し）。egg があれば道具の代わりにタマゴ */
struct HeroineReward
{
	u16 item1;
	u8 count1;
	u16 item2;
	u8 count2;
	u16 egg;
};

static const struct HeroineReward sHeroineReward[HEROINE_COUNT][HEROINE_STAGES] =
{
	{	//ハルカ
		{0, 0, 0, 0, 0},
		{ITEM_HYPER_POTION, 5, ITEM_ULTRA_BALL, 5, 0},
		{ITEM_RARE_CANDY, 3, ITEM_STARDUST, 1, 0},
		{0, 0, 0, 0, SPECIES_FEEBAS},
		{ITEM_RARE_CANDY, 10, 0, 0, 0},
	},
	{	//ヒカリ
		{0, 0, 0, 0, 0},
		{ITEM_HYPER_POTION, 5, ITEM_ULTRA_BALL, 5, 0},
		{ITEM_RARE_CANDY, 3, ITEM_BRIGHT_POWDER, 1, 0},
		{0, 0, 0, 0, SPECIES_PIPLUP},
		{ITEM_RARE_CANDY, 10, 0, 0, 0},
	},
};

/* 好物（貢ぐと +6） */
static const u16 sHeroineFavorite[HEROINE_COUNT][3] =
{
	{ITEM_STARDUST, ITEM_FRESH_WATER, ITEM_PECHA_BERRY},      /* ハルカ: ほしのすな・おいしいみず・モモンのみ */
	{ITEM_BRIGHT_POWDER, ITEM_LEMONADE, ITEM_LUM_BERRY},      /* ヒカリ: ひかりのこな・ミックスオレ・ラムのみ */
};

/* 進化の石（貢ぐと +3。わざマシンはポケットで見る） */
static const u16 sHeroineStones[] =
{
	ITEM_SUN_STONE, ITEM_MOON_STONE, ITEM_FIRE_STONE, ITEM_THUNDER_STONE, ITEM_WATER_STONE,
	ITEM_LEAF_STONE, ITEM_SHINY_STONE, ITEM_DUSK_STONE, ITEM_DAWN_STONE, ITEM_ICE_STONE,
};
#define HEROINE_STONE_COUNT 10

/*
 * 連れ歩き中のハルカが「相方として戦わない」戦い（段 2b・patches/0867）。トレーナー戦の始まり（overworld.c の BattleSetup_StartTrainerBattle）で
 * HackHeroine_BattleAllowed が見る。相方が付かないだけで、ハルカは そのまま立っている（待機）。
 *  ・クラス（下の表）: ライバル・ブレーン・各団のボス／幹部／下っ端・ジムリーダー・四天王・チャンピオン・博士・ロジックアリーナのロンジャ ほか＝ストーリーに関わる人
 *  ・ストーリーのトレーナーの番号: シンオウのストーリー 950〜963（tools/sinnoh/sinnoh_gym_ids.json）・ホウエンのストーリー 1329〜1354（tools/rom/hoenn_scripts.json）
 *  ・わたしたち 10 人（HackDynamax_IsWatashitachiBattle）・ジムリーダー（HackKanto_IsGymLeaderBattle）・チャンピオン（HackDynamax_IsChampionBattle）・
 *    ダイマックスの戦い（FLAG_DYNAMAX_BATTLE）・オーキドの電話戦（HACK_FLAG_OAK_PIC）・施設（FLAG_BATTLE_FACILITY・IsFrontierTrainerId）・相手が 2 人の戦い
 * ★ 全クラス（tools/rom/verify_heroine_s2.py が include/constants/trainer_classes.h の 111 件を「出さない」「出す」に全部分類して、この表と突き合わせる）。
 *   新しいクラスを足すと検算が NG になる＝そのとき分類し直す。表に無いクラス（255 など範囲外）は「出さない」。
 */
#define HEROINE_STORY_SINNOH_FIRST 950
#define HEROINE_STORY_SINNOH_LAST 963
#define HEROINE_STORY_HOENN_FIRST 1329
#define HEROINE_STORY_HOENN_LAST 1354

static const u8 sHeroineDeniedClass[] =
{
	CLASS_PKMN_TRAINER_1, CLASS_PKMN_TRAINER_2, CLASS_PKMN_TRAINER_3,
	CLASS_AQUA_LEADER, CLASS_TEAM_AQUA, CLASS_AQUA_ADMIN,
	CLASS_MAGMA_LEADER, CLASS_TEAM_MAGMA, CLASS_MAGMA_ADMIN,
	CLASS_ELITE_FOUR_RS, CLASS_LEADER_RS, CLASS_CHAMPION_RS,
	CLASS_RIVAL, CLASS_RIVAL_2, CLASS_BOSS, CLASS_LEADER, CLASS_TEAM_ROCKET, CLASS_ELITE_4, CLASS_CHAMPION,
	CLASS_PKMN_PROF, CLASS_PLAYER,
	CLASS_GALACTIC_GRUNT, CLASS_GALACTIC_ADMIN, CLASS_GALACTIC_BOSS, CLASS_YARTIE,
};
#define HEROINE_DENIED_CLASS_COUNT 25

/* 戦闘のあとの回復（patches/0875）: 連れ歩き中の戦闘のあと、好感度がこの値以上の最も高い段階の回復をする。
 * 0 = HP 1/4・1 = HP 1/2 + 状態異常・2 = HP 全部 + 状態異常・3 = HP・状態異常・PP 全部 + ひんしも起こす（好感度が上限＝マックスのときだけ）。
 * 連れ歩けるのは好感度 120（段 3）から＝120 以上 180 未満は 0・180 以上 240 未満は 1・240 以上 上限未満は 2・上限は 3。数字は仮（ユーザーが調整する）。 */
#define HEROINE_HEAL_TIERS 4
static const u8 sHeroineHealFrom[HEROINE_HEAL_TIERS] = {0, 180, 240, HEROINE_AFFINITY_MAX};

#endif
