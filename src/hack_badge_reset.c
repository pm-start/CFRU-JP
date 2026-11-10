#include "defines.h"
#include "../include/event_data.h"
#include "Tables/hoenn_script_flags.h"
#include "Tables/sinnoh_flags.h"
#include "Tables/hack_badge_reset.h"
#include "hack_badge_replay.h"

/*
 * せってい「バッジリセット」（#125・2026-10-17 ユーザー「ホウエン／シンオウのバッジをリセット・ジムを最初からやり直せる」）。
 *
 *  - 消すのは その地方の バッジ 8 つの旗・（ホウエンだけ）ジムを倒した印 8 つ・ジムリーダーとジムトレーナーの「倒した印」（トレーナーの旗）。
 *    ＝ジムは 初めて来たときと同じ状態（リーダーが「はじめての戦い」に戻る・ホウエンのジムの扉・キンセツのしかけ・ハクタイの花時計は
 *      トレーナーの旗から毎回計算するので 地図に入り直せば最初の形）。
 *  - 消さないもの: カントー・ジョウト／もらった わざマシン・どうぐ（ジムのわざマシンは この ROM では もう渡さない＝旗だけ立てて何も渡さない・
 *    assembly/hoenn/scripts.s の Give*）／殿堂入りの印そのもの（ホウエン HOENN_VAR_LEAGUE_WINS・FLAG_IS_CHAMPION・シンオウ SINNOH_FLAG_LEAGUE_HOF・HACK_FLAG_SINNOH_CLEARED。代わりに下の『殿堂入りを数えない印』を立てる）／
 *    四天王・チャンピオンのトレーナーの旗／ジムリーダーの再戦の段階・状態（HACK_VAR_GYM_TIER_*・#117）／ストーリーの変数・旗。
 *  - 表は src/Tables/hack_badge_reset.h（tools/rom/build_badge_reset.py が scripts.s・sinnoh_gyms.s から作る）。旗は FlagClear・トレーナーは
 *    HoennClearTrainerFlag（src/hoenn_trainer_flags.c・768 番以降は拡張旗）。新しい旗・変数・セーブの置き場は使わない（旗を下ろすだけ）。
 *  - 呼び出し: せってい（src/bw_option_menu.c）のバッジリセットフォルダで、確かめを 2 回「はい」にしたとき
 */

void HoennClearTrainerFlag(u16 trainerId);   //src/hoenn_trainer_flags.c

/*
 *  ★ 殿堂入り済みの地方でリセットしたとき（ユーザー 2026-10-17）: 殿堂入りの印そのものは消さず（#117 の再戦の条件・リーグ制覇の回数はそのまま）、
 *    『バッジリセットのあと』の旗（HACK_FLAG_BADGE_RESET_HOENN／SINNOH＝0x138C／0x138D・0x138C〜0x13BC の空き）を立てる。
 *    #124 の上限（src/hack_level_cap.c の BadgeCapOfRow）はこの旗があると その地方の殿堂入りを数えない＝バッジの数の上限に戻る。
 *    その地方のリーグをもう一度クリアしたら下ろす（src/hack_gym_tiers.c の HackGymTier_OnLeagueClear）。
 */
//region は map_region.h の MAP_REGION_*（1 ホウエン・3 シンオウ）。それ以外は FALSE
bool8 HackBadgeReset_AfterHof(u8 region)
{
	if (region == 1)
		return FlagGet(HACK_FLAG_BADGE_RESET_HOENN) != 0;
	if (region == 3)
		return FlagGet(HACK_FLAG_BADGE_RESET_SINNOH) != 0;
	return FALSE;
}


#define BADGE_RESET_HOENN 0

static const u16 sHoennFlags[BADGE_RESET_HOENN_FLAG_COUNT] = {BADGE_RESET_HOENN_FLAGS};
static const u16 sHoennTrainers[BADGE_RESET_HOENN_TRAINER_COUNT] = {BADGE_RESET_HOENN_TRAINERS};
static const u16 sSinnohFlags[BADGE_RESET_SINNOH_FLAG_COUNT] = {BADGE_RESET_SINNOH_FLAGS};
static const u16 sSinnohTrainers[BADGE_RESET_SINNOH_TRAINER_COUNT] = {BADGE_RESET_SINNOH_TRAINERS};
static const u16 sHoennOrdinary[BADGE_RESET_HOENN_ORDINARY_COUNT] = {BADGE_RESET_HOENN_ORDINARY};
static const u16 sSinnohOrdinary[BADGE_RESET_SINNOH_ORDINARY_COUNT] = {BADGE_RESET_SINNOH_ORDINARY};
static const u8 sGymMaps[BADGE_RESET_GYM_MAP_COUNT][2] = {BADGE_RESET_GYM_MAPS};

//ホウエン・シンオウのジムの地図の中か（ジムの中では リセットしない＝地図を読み直すまで扉などの絵が前のままになるため）
bool8 HackBadgeReset_InGym(void)
{
	u32 i;

	for (i = 0; i < BADGE_RESET_GYM_MAP_COUNT; ++i)
		if ((u8) gSaveBlock1->location.mapGroup == sGymMaps[i][0] && (u8) gSaveBlock1->location.mapNum == sGymMaps[i][1])
			return TRUE;
	return FALSE;
}

//region: 0 ホウエン・それ以外（1）はシンオウ
void HackBadgeReset_Run(u8 region)
{
	const u16* flags;
	const u16* trainers;
	const u16* ordinary;
	u32 flagCount, trainerCount, ordinaryCount, i;

	if (region == BADGE_RESET_HOENN)
	{
		flags = sHoennFlags;
		flagCount = BADGE_RESET_HOENN_FLAG_COUNT;
		trainers = sHoennTrainers;
		trainerCount = BADGE_RESET_HOENN_TRAINER_COUNT;
		ordinary = sHoennOrdinary;
		ordinaryCount = BADGE_RESET_HOENN_ORDINARY_COUNT;
	}
	else
	{
		flags = sSinnohFlags;
		flagCount = BADGE_RESET_SINNOH_FLAG_COUNT;
		trainers = sSinnohTrainers;
		trainerCount = BADGE_RESET_SINNOH_TRAINER_COUNT;
		ordinary = sSinnohOrdinary;
		ordinaryCount = BADGE_RESET_SINNOH_ORDINARY_COUNT;
	}
	FlagSet(region == BADGE_RESET_HOENN ? HACK_FLAG_BADGE_RESET_HOENN : HACK_FLAG_BADGE_RESET_SINNOH);   //殿堂入りを数えない印（#124 の上限が戻る・もう一度リーグをクリアしたら下りる）
	HackBadgeReplay_Reset(region);
	for (i = 0; i < flagCount; ++i)
		FlagClear(flags[i]);
	for (i = 0; i < trainerCount; ++i)
		HoennClearTrainerFlag(trainers[i]);
	for (i = 0; i < ordinaryCount; ++i)
		HoennClearTrainerFlag(ordinary[i]);
}
