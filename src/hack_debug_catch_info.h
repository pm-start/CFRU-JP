#pragma once

//デバッグ版だけ: 戦闘中の捕獲情報の窓（src/hack_debug_catch_info.c・計算は src/catching.c の HackDebugCatchInfo_Calc・patches/0425）
//  配る版には入らない（-DHACK_DEBUG_MENU のときだけ）。

#define DEBUG_CATCH_ROWS 4      //窓に出すボールの数（バッグの ボール の並び順の先頭から）

enum
{
	DEBUG_CATCH_NONE,       //戦闘の準備前
	DEBUG_CATCH_OK,         //野生（捕まえられる）
	DEBUG_CATCH_TRAINER,    //トレーナー戦
	DEBUG_CATCH_CANT,       //ゴースト・チュートリアル・通信・捕獲禁止の旗
};

struct DebugCatchRow
{
	u16 item;
	u16 permille;           //そのまま投げたときの捕まる確率（0.1% 単位・0〜1000）
	u16 mashPermille;       //A 連打 10 回の場合（連打が効かないボール・場面は permille と同じ）
	u8 multiplier;          //ボールの補正値（10 が 1 倍）
};

struct HackDebugCatchInfo
{
	u8 state;
	u8 catchRate;           //相手の種族の捕獲率
	u8 hpPercent;           //相手の HP の割合（%）
	u8 count;               //rows の数
	u16 otherMult10;        //状態異常・ダブル・レイド・なんいどなど、ボールと HP 以外の補正（10 が 1 倍）
	struct DebugCatchRow rows[DEBUG_CATCH_ROWS];
};

void HackDebugCatchInfo_Calc(struct HackDebugCatchInfo *out);   //src/catching.c
void HackDebugCatchInfo_Tick(void);                             //毎コマ（src/hack_battle_speed.c の BattleMainCB2 から）
