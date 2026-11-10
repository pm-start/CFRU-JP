#pragma once
#include "../include/global.h"

struct Pokemon;

/*
 * 個体の印（ヌシ・デルタ種・ダーク）のランダム付与の入口（段 A2・FEAT-shadow-a2・patches/0960・設計 work/tmp/audit/NUSHI_DESIGN.md 2 章・SHADOW_S1_PLAN.md N2/N4）。
 * 判定は「作る瞬間に 1 回」。1 回の乱数（万分率）を帯に分け、ダーク→ヌシ→デルタの順で排他に付ける（重ならない）。値: ダーク＝255（心のゲージ満タン）・ヌシ＝0・デルタ＝0（タイプの決め方は段 B）。
 *
 * 入口は 2 か所だけ（検算 verify_shadow_roll.py が呼び元を数える）:
 *   野生   : src/wild_encounter.c CreateWildMon の終わり（草・水・岩・釣り・ずかんナビ・群れ・ダブルの 2 匹目）。台本の野生（CreateScriptedWildMon）・レイド・幽霊戦は
 *            CreateWildMon を通らない（CreateWildMonNoKind／NoSwap を直接）＝付かない。
 *   トレーナー: src/build_pokemon.c CreateNPCTrainerParty の手持ちループ（呼び元 4 か所は全部ここを通る）。
 * 除外: 通信・フロンティア・施設・サファリ・ダミー練習・味方側・主要トレーナー（ジムリーダー・四天王・チャンピオン・ライバル・主人公・博士）・台本の固定戦・もらうポケモン・卵・ミラクル交換。
 * ダークは悪の組織（アクア団・マグマ団・ロケット団・ギンガ団）のトレーナーの手持ちだけ。組織ごとに「ストーリーで最初に戦うしたっぱ」の先頭 1 匹は必ずダーク（状態を持たない表）。
 * 再戦のたびに引き直す（保存しない）。
 */

//確率（万分率）。1 か所にまとめた定数表（変えたら版更新）
#define HACK_KIND_DENOM 10000

struct HackKindBands
{
	u16 dark;
	u16 nushi;
	u16 delta;
};

enum   //帯の表（場面）
{
	HACK_KIND_SCENE_WILD,       //野生
	HACK_KIND_SCENE_TRAINER,    //一般のトレーナーの手持ち（1 匹ごと）
	HACK_KIND_SCENE_GRUNT,      //悪の組織のしたっぱ
	HACK_KIND_SCENE_BOSS,       //悪の組織の幹部・ボス
	HACK_KIND_SCENE_COUNT,
};

enum   //トレーナーの区分
{
	HACK_KIND_TIER_EXCLUDED,    //主要トレーナー（印を付けない）
	HACK_KIND_TIER_NORMAL,
	HACK_KIND_TIER_GRUNT,
	HACK_KIND_TIER_BOSS,
};

enum   //悪の組織
{
	HACK_KIND_ORG_NONE,
	HACK_KIND_ORG_AQUA,
	HACK_KIND_ORG_MAGMA,
	HACK_KIND_ORG_ROCKET,
	HACK_KIND_ORG_GALACTIC,
};

#define HACK_KIND_DARK_FULL 255   //ダークの値＝心のゲージの割合（満タン）

const struct HackKindBands* HackKind_Bands(u8 scene);                //場面の帯の表（定数・デバッグ版の確率増しは build_rom.py --debug-menu --kind-boost のビルド時の定義だけ）
u8 HackKind_Pick(const struct HackKindBands* b, u16 r);              //r（0〜9999）→ HACK_KIND_*（帯の境目はここ 1 か所）
u8 HackKind_TrainerTier(u8 trainerClass, const u8* trainerName);     //HACK_KIND_TIER_*
u8 HackKind_TrainerOrg(u8 trainerClass, const u8* trainerName);      //HACK_KIND_ORG_*
bool8 HackKind_IsForcedDark(u16 trainerId);                          //組織ごとに最初に戦うしたっぱ（表）

void HackKind_RollWild(struct Pokemon* mon);                         //野生 1 匹を作った直後に
void HackKind_RollTrainer(struct Pokemon* mon, u8 side, u8 trainerClass, const u8* trainerName, u16 trainerId, u32 partyIndex);
