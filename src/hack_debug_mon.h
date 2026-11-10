#pragma once
#include "../include/global.h"

/* デバッグモン（デバッグ用の種族 2 匹・見た目はレントラー・tools/custom_mon/debug_mon・patches/0422）
   SPECIES_DEBUGMON_MAX（種族値が全部 255）・SPECIES_DEBUGMON_MIN（全部 1）。種族番号は配る版でも同じ（セーブの互換）だが、手に入る道は
   デバッグメニュー付きの版（HACK_DEBUG_MENU）のデバッグメニューだけ。タイプ・特性の切り替えは性格値（personality）に入れる（src/hack_debug_mon.c） */
#define HACK_IS_DEBUG_MON(sp) ((sp) == SPECIES_DEBUGMON_MAX || (sp) == SPECIES_DEBUGMON_MIN)

//GetMonType／戦闘に出るときのタイプ。デバッグモンで切り替えがあればそのタイプ・無ければ base。配る版は base をそのまま返すだけ
u8 HackDebugMon_Type(const struct Pokemon* mon, u16 species, u8 slot, u8 base);
//GetMonAbility の頭。デバッグモンで特性を選んであればその特性・無ければ ABILITY_NONE。配る版は ABILITY_NONE
u16 HackDebugMon_Ability(const struct Pokemon* mon, u16 species);

#ifdef HACK_DEBUG_MENU
//デバッグメニューの手持ち編集（欄 0 つよさ＝0 全部255・1 全部1／1 タイプ1／2 タイプ2（番号は HackDebugMon_TypeOfIndex の添字）／3 特性＝0 標準・ほかは特性の番号）
bool8 HackDebugMon_IsMon(const struct Pokemon* mon);
s32 HackDebugMon_EditGet(struct Pokemon* mon, u8 field);
void HackDebugMon_EditSet(struct Pokemon* mon, u8 field, s32 value);
u8 HackDebugMon_TypeCount(void);
u8 HackDebugMon_TypeOfIndex(s32 index);
#endif
