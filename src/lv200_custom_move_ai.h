#ifndef GUARD_LV200_CUSTOM_MOVE_AI_H
#define GUARD_LV200_CUSTOM_MOVE_AI_H

#include "../include/global.h"

// AI評価専用。独自効果技は近い既存技へ、その他は通常のLv200 aliasへ寄せる。
u16 Lv200MoveAIBase(u16 move);

// AI用「bank が baseMove（本家の技）と同じ扱いの技を持っているか」。MoveInMoveset(MOVE_X, bank) の代わりに使う（FIX-ai-original-moves）。
// 各技を Lv200MoveAIBase と EffectiveMove（HackEffectiveMove）で解決して比べるので、新技・Lv200技も本家の技として数える。実技の完全一致が要る所（gLockedMoves 等）は MoveInMoveset のまま。
bool8 AIMoveInMoveset(u16 baseMove, u8 bank);

#endif
