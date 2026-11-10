#include "defines.h"
#include "hack_mining.h"
#include "../include/event_data.h"
#include "../include/global.fieldmap.h"
#include "../include/field_control_avatar.h"
#include "../include/event_object_movement.h"
#include "../include/overworld.h"
#include "../include/item_use.h"
#include "../include/script.h"
#include "../include/task.h"
#include "../include/field_player_avatar.h"

/* 地下の化石ほり（#111・2026-10-12）。Rボタンの走査は使わない（IsValidMiningSpot は HackMining_IsDiggable を見る）。
 * かべの番号 = sMiningWalls[] の添字（0〜7・全部の地下の地図で通し番号＝旗を地図ごとに分けない）。表に無いかべは いつでもほれる・印も付けない。
 * 日替わり: 拡張変数 HACK_VAR_MINING_DAY に最後にほった日（HackWild_DaySlot()/4 の下位16ビット）を持ち、変わっていたら印 8 本を消す。 */

/* ちかの ひろば（tools/sinnoh/sinnoh_underground.py のかべ・北のかべ y = 1・番号 0〜7）。座標は生成ヘッダ（Tables/sinnoh_underground_ids.h）から。
 * 並びが かべの番号＝旗 HACK_FLAG_MINING_WALL_BASE + 番号（変えると今日ほった印がずれる）。tools/rom/verify_mining.py が地図の中身と突き合わせる */
static const struct { u16 x; u16 y; } sMiningWalls[HACK_UG_WALL_COUNT] = HACK_UG_WALLS_INIT;

u32 HackWild_DaySlot(void);   //src/wild_shuffle.c

static void ResetIfNewDay(void)
{
	u16 today = (u16)(HackWild_DaySlot() / 4u);
	if (VarGet(HACK_VAR_MINING_DAY) != today)
	{
		u32 i;
		for (i = 0; i < HACK_MINING_WALL_MAX; ++i)
			FlagClear(HACK_FLAG_MINING_WALL_BASE + i);
		VarSet(HACK_VAR_MINING_DAY, today);
	}
}

static s32 HackMining_WallIndex(s16 x, s16 y)
{
	u32 i;
	if (gSaveBlock1->location.mapGroup != HACK_UG_GROUP || gSaveBlock1->location.mapNum != HACK_UG_NUM)
		return -1;
	for (i = 0; i < HACK_UG_WALL_COUNT; ++i)
	{
		if (sMiningWalls[i].x == x - 7 && sMiningWalls[i].y == y - 7)
			return i;
	}
	return -1;
}

bool8 HackMining_IsDiggable(s16 x, s16 y)
{
	s32 idx;
	ResetIfNewDay();
	idx = HackMining_WallIndex(x, y);
	if (idx < 0)
		return TRUE;
	return !FlagGet(HACK_FLAG_MINING_WALL_BASE + idx);
}

void HackMining_MarkDug(void)
{
	struct MapPosition pos;
	s16 x, y;
	s32 idx;

	GetPlayerPosition(&pos);
	x = pos.x;
	y = pos.y;
	MoveCoords(GetPlayerFacing(), &x, &y);
	ResetIfNewDay();
	idx = HackMining_WallIndex(x, y);
	if (idx >= 0)
		FlagSet(HACK_FLAG_MINING_WALL_BASE + idx);
}

/* ---- たんけんセット（ITEM_EXPLORER_KIT 0x370・たいせつなもの・#111 段階 2・2026-10-12）----
 * 野外（町・都市・道路・海の道路）でつかうと「もぐりますか？」→ 今の場所を動的ワープに覚えて ちかの ひろば（HACK_UG_GROUP・HACK_UG_NUM）へ。
 * ひろばの出口は動的ワープ (0x7F, 0x7F)＝覚えた場所へ戻る。つかえない所: 屋内（バトル施設の中も）・洞窟・ひろば自身・なみのり中・サファリゾーン */
extern const u8 gText_ExplorerKit_CantUseHere[];
extern const u8 HackScr_ExplorerKit_Enter[];

#define ExplorerKitDisplayCannotUse ((void (*)(u8, u8, u8, const u8 *))(0x080A2398 + 1))   //素の PrintNotTheTimeToUseThat と同じ道（hack_box_link.c と同じ）

static bool8 ExplorerKitAllowedHere(void)
{
	u8 type = gMapHeader.mapType;

	if (type != MAP_TYPE_TOWN && type != MAP_TYPE_CITY && type != MAP_TYPE_ROUTE && type != MAP_TYPE_6)
		return FALSE;
	if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
		return FALSE;
	/* 挑戦中の旗（FLAG_BATTLE_FACILITY 0x930・ヤーティ挑戦中 0xFD9）は見ない（0.65.1・BUG-explore-kit-flag）: フロンティアの施設の地図は屋内（mapType 8）で上の判定が断る。
	 * 旗は施設を途中で出たり ロジックアリーナの「やすむ」（0xFD9 は再開用の印で外へ出られる）で残り、残ると屋外どこでも断っていた。サファリゾーンは野外（道路扱い）なので旗で断る */
	if (FlagGet(FLAG_SYS_SAFARI_MODE))
		return FALSE;
	return TRUE;
}

static void Task_ExplorerKitField(u8 taskId)
{
	ScriptContext1_SetupScript(HackScr_ExplorerKit_Enter);
	DestroyTask(taskId);
}

void FieldUseFunc_ExplorerKit(u8 taskId)
{
	if (!ExplorerKitAllowedHere())
	{
		ExplorerKitDisplayCannotUse(taskId, gTasks[taskId].data[3], 4, gText_ExplorerKit_CantUseHere);
		return;
	}
	sItemUseOnFieldCB = Task_ExplorerKitField;
	SetUpItemUseOnFieldCallback(taskId);
}

/* callasm: 今の場所（地図・座標）を動的ワープに覚える（素の SetDynamicWarp と同じ＝座標は gSaveBlock1->pos・ワープ番号 -1）。ひろばの出口が使う */
void HackExplorerKit_SetDynamicWarp(void)
{
	gSaveBlock1->dynamicWarp.mapGroup = gSaveBlock1->location.mapGroup;
	gSaveBlock1->dynamicWarp.mapNum = gSaveBlock1->location.mapNum;
	gSaveBlock1->dynamicWarp.warpId = -1;
	gSaveBlock1->dynamicWarp.x = gSaveBlock1->pos.x;
	gSaveBlock1->dynamicWarp.y = gSaveBlock1->pos.y;
}
