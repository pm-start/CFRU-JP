#pragma once
#include "../include/global.h"

/* 段 C オーラ（patches/0956・src/hack_aura.c） */
u8 HackKind_AuraKind(u8 bank);   //判定の窓口: この戦闘の bank のポケモンの印（HACK_KIND_*・hack_kind.h）
void HackAura_ServerMain(void);  //hooks: FR(J) ServerMain 0x08011C34 の置き換え
void HackAura_RestoreMixer(void); //戦闘の終わり（end_battle.c）: 曲の音量・リバーブ・ひんしの写しを戻す
/* 段 H（patches/0966）つよさをみる（BW 版）のオーラ: 絵のスプライトに付ける／外す */
void HackAura_SummaryAttach(u8 parentId, u8 kind);
void HackAura_SummaryDetach(void);
