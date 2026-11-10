#pragma once

#include "defines.h"
#include "defines_battle.h"
#include "../include/new/lv200_custom_moves_state.h"

/* 新規技(Lv200)の共通部品: 能力変化・HP・状態異常の治療を、素の流れ(アニメ・文・HP バー・
 * 手持ちへ送る・ひんし)で出す。使い方: 技の Apply/フックが下の関数で効果を積む →
 * cmd49 の MOVE_END で Lv200Common_GetMoveEndScript が 1 件ずつ台本を返す。 */
enum
{
	LV200_COMMON_KIND_NONE,
	LV200_COMMON_KIND_STAT_SELF,
	LV200_COMMON_KIND_STAT_FOE,
	LV200_COMMON_KIND_HEAL,
	LV200_COMMON_KIND_HURT,
	LV200_COMMON_KIND_CURE_STATUS,
	LV200_COMMON_KIND_CURE_CONFUSION,
	LV200_COMMON_KIND_MESSAGE,
	LV200_COMMON_KIND_INFLICT,
	LV200_COMMON_KIND_WEATHER,
	LV200_COMMON_KIND_INFATUATE,
	LV200_COMMON_KIND_COUNT,
};

void Lv200Common_Reset(void);
void Lv200Common_Clear(void);
void Lv200Common_OnSwitchOut(u8 bank);
const u8 *Lv200Common_GetMoveEndScript(void);

/* 能力変化: bank の stat(STAT_STAGE_*)を delta 段階。source は使い手(自分に掛けるなら bank と同じ)。
 * 実際の増減はスクリプトの statbuffchange(あまのじゃく・たんじゅん・クリアボディ・ミラーアーマー・+6)に任せる。 */
void Lv200Common_QueueStat(u8 bank, u8 source, u8 stat, s8 delta, bool8 quiet);
/* HP: 戦闘中の値は即時に動かし(従来どおり)、画面の HP バー・手持ち・ひんしは MOVE_END に流す。実際の増減量を返す。 */
u32 Lv200Common_HealBank(u8 bank, u32 amount);
u32 Lv200Common_HurtBank(u8 bank, u32 amount);
/* 状態異常・混乱の治療: 戦闘中の値と手持ちへの送りを即時に、文は MOVE_END(キュー版)または台本側(callasm 版)で。 */
void Lv200Common_CureStatusBank(u8 bank);
void Lv200Common_CureConfusionBank(u8 bank);
void Lv200Common_QueueCureStatus(u8 bank);
void Lv200Common_QueueCureConfusion(u8 bank);

/* 文だけを出す（場・状態の知らせ。借りる素の文か承認済みの文を渡す）。bank は文の [ATTACKER]／[TARGET] に使う個体。 */
void Lv200Common_QueueMessage(u8 bank, const u8 *text);
/* 2 者の文（[ATTACKER]と[TARGET]）: bank が [TARGET]・source が [ATTACKER]。 */
void Lv200Common_QueueMessageFrom(u8 bank, u8 source, const u8 *text);
/* 状態異常・こんらん・ひるみを素の追加効果の流れ(seteffectsecondary)で付ける。
 * effect は MOVE_EFFECT_*（MOVE_EFFECT_POISON 等）。タイプ・特性・しんぴのまもり・みがわり・テレインの無効判定と文・手持ちへ送るのは素が行う。
 * poisonIgnoresSteel=TRUE のときだけ、はがねタイプにも毒が通る（ひふくぶんかい）。 */
void Lv200Common_QueueInflict(u8 bank, u8 source, u8 effect, bool8 poisonIgnoresSteel);
/* メロメロにする（MOVE_EFFECT が無いので専用の種類）。可否は付ける時に素の判定で見る:
 * 生きている・すでにメロメロでない・どんかん・アロマベール・性別なしでない・性別の組み合わせ(sameGender=FALSE は素どおり「違う」・TRUE は「同じ」)。
 * 通ったときだけ素のメロメロの文を出す（効かないときは何も出さない）。みがわりへの当たりは呼ぶ側が見る。 */
void Lv200Common_QueueInfatuate(u8 bank, u8 source, bool8 sameGender);
bool8 Lv200Common_CanInfatuate(u8 bank, u8 source, bool8 sameGender);
/* 毒の無効判定(CanBePoisoned)が参照する。キューの付与を処理している間だけ TRUE。 */
bool8 Lv200Common_PoisonIgnoresSteel(void);
void Lv200Common_ClearPoisonBypass(void);

/* 天気を変える（素の SetSunnyWeather／SetRainyWeather／SetSandstormWeather／SetHailWeather と同じ判定・持続）。
 * 変わったときだけ TRUE を返し、MOVE_END で素の天気開始文と天気に反応する特性・フォームの処理を出す。変わらないときは何も出さない。 */
enum { LV200_WEATHER_SUN, LV200_WEATHER_RAIN, LV200_WEATHER_SAND, LV200_WEATHER_HAIL };
bool8 Lv200Common_QueueWeather(u8 bank, u8 kind);
/* 浮いている相手を撃ち落とす（先に CheckGrounding で確かめてから）。 */
void Lv200Common_QueueBringDown(u8 bank, u8 source);
