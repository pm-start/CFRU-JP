#pragma once

/*
 * ★ 2026-11-08 ダーク技 18 個（段 D・patches/0955・FEAT-shadow-d・tools/shadow/dark_tail.py）。
 * 技は MOVE_YAMITACKLE 〜 MOVE_YAMINOHANBUN（0x1447〜0x1458・FIRST_SHADOW_MOVE〜LAST_SHADOW_MOVE の連続）。タイプは TYPE_SHADOW（画面名「ダーク」）。
 * 相性: ダークでない相手 ×2・ダーク状態の相手 ×0.5（相性表 gTypeEffectiveness は通さない）・タイプ一致なし・PP を使わない。
 * 特殊 4（ダークエンド＝反動／ダークウェザー＝ダークの空／ダークリムーブ＝かべ消し／ダークハーフ）は EFFECT_HACK_CUSTOM（252）から Shadow_GetScript の台本へ。
 * 「ダーク状態の個体か」の窓口は Shadow_IsMonDark 1 か所（段 E で個体の印 HackKind_Get(mon) == HACK_KIND_DARK に差し替え済み）。
 */
#include "defines.h"

bool8 IsShadowMove(u16 move);
bool8 Shadow_IsMonDark(struct Pokemon* mon);
bool8 Shadow_IsBattlerDark(u8 bank);
u32 Shadow_MoveMultiplier(bool8 targetIsDark);
u32 Shadow_EndRecoil(u8 bank);
const u8* Shadow_GetScript(u16 move);                 //EFFECT_HACK_CUSTOM の入口（lv200_custom_moves.c）。ダーク技でなければ NULL
bool8 Shadow_EndTurnShadowSky(void);                  //ET_Weather: ダークの空は 5 ターン
bool8 Shadow_EndTurnShadowSkyDamage(u8 bank);         //ET_Weather_Damage_Health_Abilities: ダークでない子が最大 HP の 1/16

//ダークの空（gBattleWeather の 1 ビット・constants/battle.h の WEATHER_SHADOW_SKY）
void Shadow_TrySetShadowSky(void);
void Shadow_RemoveScreensBothSides(void);
void Shadow_HalfNext(void);
