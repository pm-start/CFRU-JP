#include "defines.h"
#include "../include/event_data.h"
#include "../include/field_screen_effect.h"
#include "../include/load_save.h"
#include "../include/overworld.h"
#include "../include/safari_zone.h"
#include "../include/new/follow_me.h"
#include "hack_challenge_c.h"
#include "hack_dummy_battle.h"
#include "hack_emergency_escape.h"
#include "map_region.h"
#include "region_fly.h"

/*
 * 緊急脱出（patches/0750〜0753・TODO #144・ユーザー 2026-10-24「緊急脱出機能つけておいて。最寄りのポケセンに強制転送。」）。
 * 詳細は docs/research/2026-10-24_emergency-escape.md・CLAUDE.md 6.112。
 *
 * ■ 行き先（HackEscape_Decide）— 全滅したときの戻り先と同じ決め方を使い回す（お金の罰金・手持ちの回復は無し）
 *   1. ポケセンの縛り（りょうきん・？？？・きんし）が ON → マサラタウンの家（全滅・バージョンアップのワープと同じ・hack_challenge_c.c）。回復はしない
 *   2. 最後の回復地点（gSaveBlock1->lastHealLocation）が あり、かつ 今いる地方と同じ地方 → そこのポケセンの中（看護師の 2 つ下）
 *        カントー・ななしま: 素の表 20 件（080C0E20 の索引 → 戻り先の表 083B59E4 ＋ 素の x・y の例外）
 *        ホウエン・ジョウト・シンオウ: src/region_fly.c の表（RegionWhiteoutRespawn＝全滅の戻りと同じ）
 *   3. 回復地点が無い（はじめ直後）・別の地方の回復地点しか無い → 今いる地方の最初の町のポケセン
 *        カントー・ななしま: マサラタウンの家 ／ ホウエン: コトキタウン ／ ジョウト・キタカミ: ヨシノシティ（ワカバタウンにポケセンは無い）／ シンオウ: マサゴタウン（フタバタウンにポケセンは無い）
 * ■ 後始末（HackEscape_Cleanup）
 *   ・施設の挑戦（バトルフロンティアなど）をやめた扱い: 元の手持ち・バッグを戻し、連勝を消し、旗と状態を片付ける（src/hoenn_tower.c HoennFacility_EscapeCleanup）
 *   ・全滅のあとと同じ: 素の Overworld_ResetStateAfterWhitingOut（自転車・なみのり・サイクリングロード・サファリ・フラッシュ・怪力・連れ歩き…）・サファリゾーンの旗・
 *     四天王の倒した印（EventScript_ResetEliteFourEnd）・トレーナータワーの挑戦中の印
 *   ・ポケセン 3F のダミー戦の旗 0xFC6
 *   ・一時フラグ・一時変数・dynamic warp は 地図を読み込み直すと戻る／触らない（全滅も触らない）
 * ■ 旗・変数・保存の欄は増やさない。確かめの「はい」を押すまで何も書き換えない。
 */

extern void HoennFacility_EscapeCleanup(void);   //src/hoenn_tower.c

//素の ROM（JP）。ホストの検算は ESCAPE_ROM_* を差し替えて動かす
#ifndef ESCAPE_ROM_HEAL_INDEX
#define ESCAPE_ROM_HEAL_INDEX ((u16 (*)(u16, u16)) (0x080C0E20 | 1))          //GetHealLocationIndexFromMapGroupAndNum（素の 20 件・0 = 無い）
#endif
#ifndef ESCAPE_ROM_RESPAWN_TABLE
#define ESCAPE_ROM_RESPAWN_TABLE ((const u16*) 0x083B59E4)                      //sWhiteoutRespawnHealCenterMapIdxs（[索引 − 1] × {群, 番号} の u16 2 つ）
#endif
#ifndef ESCAPE_ROM_RESET_STATE
#define ESCAPE_ROM_RESET_STATE ((void (*)(void)) (0x08054698 | 1))             //Overworld_ResetStateAfterWhitingOut
#endif
#ifndef ESCAPE_ROM_RESET_E4
#define ESCAPE_ROM_RESET_E4() ((void (*)(const u8*)) (0x08069408 | 1))((const u8*) 0x08193E19)   //RunScriptImmediately(EventScript_ResetEliteFourEnd)
#endif
#ifndef ESCAPE_ROM_RESET_SAFARI
#define ESCAPE_ROM_RESET_SAFARI() ResetSafariZoneFlag()
#endif
#ifndef ESCAPE_LINK_ACTIVE
#define ESCAPE_LINK_ACTIVE (*(u8*) 0x03003FA4)                                  //gReceivedRemoteLinkPlayers
#endif

#define ESCAPE_KANTO_HOME_GROUP 4      //マサラタウンの主人公の家 1F（素の全滅の索引 1 番・x 8 y 5）
#define ESCAPE_KANTO_HOME_NUM 0

//素の戻り先の x・y の例外（080C0F2C〜0F9A）: マサラの家 (8,5)・セキエイ (13,12)・ナナシマ 1 の島 (5,4)・トレーナータワーのロビー (4,11)・それ以外 (7,4)
static void KantoRespawnXY(u16 group, u16 num, s16 *x, s16 *y)
{
	if (group == 4 && num == 0)
	{
		*x = 8;
		*y = 5;
	}
	else if (group == 13 && num == 0)
	{
		*x = 13;
		*y = 12;
	}
	else if (group == 32 && num == 0)
	{
		*x = 5;
		*y = 4;
	}
	else if (group == 2 && num == 10)
	{
		*x = 4;
		*y = 11;
	}
	else
	{
		*x = 7;
		*y = 4;
	}
}

static void SetHome(struct WarpData *out)
{
	out->mapGroup = ESCAPE_KANTO_HOME_GROUP;
	out->mapNum = ESCAPE_KANTO_HOME_NUM;
	out->warpId = -1;
	out->x = HACK_CHC_HOME_X;
	out->y = HACK_CHC_HOME_Y;
}

//カントー・ななしま: 素の表で「最後の回復地点」のポケセンを引く（無ければ FALSE）
static bool8 KantoRespawn(const struct WarpData *heal, struct WarpData *out)
{
	u16 idx = ESCAPE_ROM_HEAL_INDEX((u16) heal->mapGroup, (u16) heal->mapNum);
	const u16 *entry;

	if (idx == 0)
		return FALSE;
	entry = ESCAPE_ROM_RESPAWN_TABLE + (idx - 1) * 2;
	out->mapGroup = (s8) entry[0];
	out->mapNum = (s8) entry[1];
	out->warpId = -1;
	KantoRespawnXY(entry[0], entry[1], &out->x, &out->y);
	return TRUE;
}

//今いる地方の最初の町（ポケセンのある最初の町）のポケセン。{群, 番号} は src/region_fly.c の表の ポケセンの地図（検算が表の名前と突き合わせる）
//ホウエン: コトキタウン / ジョウト（キタカミも）: ヨシノシティ（CHERRYGROVE）/ シンオウ: マサゴタウン（SANDGEM）
#define ESCAPE_FIRST_HOENN_GROUP 45
#define ESCAPE_FIRST_HOENN_NUM 2
#define ESCAPE_FIRST_JOHTO_GROUP 78
#define ESCAPE_FIRST_JOHTO_NUM 52
#define ESCAPE_FIRST_SINNOH_GROUP 77
#define ESCAPE_FIRST_SINNOH_NUM 71

static bool8 FirstTownCenter(u8 region, struct WarpData *out)
{
	u8 group, num;
	u32 i;

	if (region == MAP_REGION_HOENN)
	{
		group = ESCAPE_FIRST_HOENN_GROUP;
		num = ESCAPE_FIRST_HOENN_NUM;
	}
	else if (region == MAP_REGION_JOHTO || region == MAP_REGION_KITAKAMI)
	{
		group = ESCAPE_FIRST_JOHTO_GROUP;
		num = ESCAPE_FIRST_JOHTO_NUM;
	}
	else if (region == MAP_REGION_SINNOH)
	{
		group = ESCAPE_FIRST_SINNOH_GROUP;
		num = ESCAPE_FIRST_SINNOH_NUM;
	}
	else
		return FALSE;
	for (i = 0; i < gRegionPokeCenterCount[0]; i++)
	{
		const struct RegionPokeCenter *p = &gRegionPokeCenters[i];

		if (p->pcGroup != group || p->pcNum != num)
			continue;
		out->mapGroup = (s8) p->pcGroup;
		out->mapNum = (s8) p->pcNum;
		out->warpId = -1;
		out->x = p->respX;
		out->y = p->respY;
		return TRUE;
	}
	return FALSE;
}

u8 RegionWhiteoutRespawn(struct WarpData *warp);   //src/region_fly.c（全滅の戻り先・0 = 決めた）

u8 HackEscape_Decide(struct WarpData *out, u8 regionNow, const struct WarpData *heal, bool8 homeOnly)
{
	if (homeOnly)
	{
		SetHome(out);
		return 2;
	}
	if (!(heal->mapGroup == 0 && heal->mapNum == 0 && heal->x == 0 && heal->y == 0)
	 && GetMapRegionOf((u8) heal->mapGroup, (u8) heal->mapNum) == regionNow)
	{
		if (regionNow == MAP_REGION_KANTO)
		{
			if (KantoRespawn(heal, out))
				return 0;
		}
		else if (RegionWhiteoutRespawn(out) == 0)
			return 0;
	}
	if (FirstTownCenter(regionNow, out))
		return 1;
	SetHome(out);   //カントー・ななしま（マサラの家＝全滅の「見つからない」と同じ）・表に見つからない
	return 2;
}

bool8 HackEscape_FieldAvailable(void)
{
	if (ESCAPE_LINK_ACTIVE)
		return FALSE;                                      //通信中
	if (gMain.savedCallback != CB2_ReturnToFieldWithOpenMenu)
		return FALSE;                                      //スタートメニューから開いた せってい（フィールドの上）だけ。タイトルのせっていは出さない
	if (gMain.inBattle)
		return FALSE;                                      //戦闘中
	return TRUE;
}

/*
 * live = 地図とプレイヤーが読み込まれている（フィールドの上）。タイトルでは地図もプレイヤーの物体も無いので、
 * 素の Overworld_ResetStateAfterWhitingOut の中の連れ歩きの片付け（物体を触る）だけ外す（inProgress を一時的に 0 にする）。
 */
void HackEscape_Cleanup(bool8 live)
{
	u8 follower = gFollowerState.inProgress;

	HoennFacility_EscapeCleanup();                         //施設の挑戦をやめた扱い（元の手持ち・バッグ・連勝・状態）
	ESCAPE_ROM_RESET_SAFARI();                             //サファリゾーンの旗（全滅の CB2_WhiteOut と同じ）
	ESCAPE_ROM_RESET_E4();                                 //四天王の倒した印（全滅の DoWhiteOut の頭と同じ）
	if (!live)
		gFollowerState.inProgress = FALSE;
	ESCAPE_ROM_RESET_STATE();                              //自転車・なみのり・サファリ・フラッシュ・怪力…（連れ歩きは live のときだけ）
	if (!live)
		gFollowerState.inProgress = follower;
	if (VarGet(VAR_MAP_SCENE_TRAINER_TOWER) == 1)
		VarSet(VAR_MAP_SCENE_TRAINER_TOWER, 0);            //トレーナータワーの挑戦中の印
	FlagClear(HACK_FLAG_DUMMY_BATTLE);                     //ポケセン 3F のダミー戦の旗（戦闘のあとに残っていると 戦闘の終わりの処理が変わる）
}

//タイトルの「つづきから」の はい。セーブは RAM に読まれていて、まだ地図には入っていない（src/bw_main_menu.c）
void HackEscape_OnContinue(void)
{
	struct WarpData dest;

	HackEscape_Cleanup(FALSE);
	HackEscape_Decide(&dest, GetCurrentMapRegion(), &gSaveBlock1->lastHealLocation, HackChallengeC_CenterBound());
	SetContinueGameWarp(dest.mapGroup, dest.mapNum, -1, (s8) dest.x, (s8) dest.y);   //素の continueGameWarp（HackSaveVersion_OnContinue・HoennTower_SaveGame と同じ道）
	SetContinueGameWarpStatus();
}

//せっていの はい のあと、フィールドに戻ってから（src/bw_option_menu.c）
void HackEscape_WarpNow(void)
{
	struct WarpData dest;

	HackEscape_Cleanup(TRUE);
	HackEscape_Decide(&dest, GetCurrentMapRegion(), &gSaveBlock1->lastHealLocation, HackChallengeC_CenterBound());
	SetWarpDestination(dest.mapGroup, dest.mapNum, -1, (s8) dest.x, (s8) dest.y);
	DoWarp();
}
