#pragma once
#include "../include/global.h"

struct Pokemon;
struct Sprite;
struct SpritePalette;

/*
 * 段 H（FEAT-shadow-h・patches/0966）: BW 版の画面（つよさをみる・戦闘の HP 枠）のダーク表示。設計 work/tmp/audit/SHADOW_S1_PLAN.md（H 段）。
 *   - つよさをみる: ポケモンの絵の後ろにオーラ（src/hack_aura.c の HackAura_Summary*）・ダークは経験値バーの代わりに心のゲージ（ハート 5 個）
 *   - 戦闘の HP 枠（src/bw_hpbar.c から Sync を呼ぶ）: ダークだけ紫のハート（味方のシングルは EXP バーの位置に 5 個・それ以外は名前の横に 1 個）
 *   - ヌシ・デルタは HP 枠に出さない（計画に案が無い）
 */

//心のゲージの段（印の値 8 ビット・255 満タン・5 段＝51 ずつ・端数は切り上げ＝0 でなければ 1 段は見える）。まとめの枝で HackGauge_Stage は段 E の Shadow_GaugeStage（src/shadow_gauge.c）を呼ぶ形に差し替えた
#define HACK_GAUGE_STAGES 5
u8 HackGauge_Stage(u8 value);
u8 HackGauge_StageOfMon(const struct Pokemon* mon); //ダークでなければ 0

//スプライトのパレットを読む。avoidSlot の枠は使わない（つよさをみるの絵は枠 12 に札なしで直に置かれる＝LoadSpritePalette が「空き」と見て上書きする。mGBA で絵の色が変わって見つかった）
#define HACK_SUMMARY_PIC_PAL_SLOT 12
u8 HackScreens_LoadPalette(const struct SpritePalette* pal, u8 avoidSlot);

//つよさをみる（src/bw_summary_screen.c から）
bool8 HackSummary_IsDark(const struct Pokemon* mon);  //ダークの子か（卵は除く）。つよさの画面が「EXP」「つぎのLv」の行を出さない判定にも使う
void HackSummary_GaugeLoad(void);                   //ハート 5 個を（見えない状態で）作る
void HackSummary_GaugeInv(const struct Pokemon* mon, u8 inv, struct Sprite** expBar, u8 expCount); //経験値バーの SetInvisExpBar の代わり
u8 HackSummary_GaugeWantedInv(void);                //今の「見えない」指定（なければ 1）
void HackSummary_GaugeFree(void);

//戦闘の HP 枠（src/bw_hpbar.c から）
void HackShadowHp_Sync(u8 healthboxId, u8 battlerId, bool8 playerSingles);
bool8 HackShadowHp_IsDark(u8 battlerId);            //この bank のポケモンがダークか（EXP バーを描かない判定にも使う）
