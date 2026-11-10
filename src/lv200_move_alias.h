#ifndef GUARD_LV200_MOVE_ALIAS_H
#define GUARD_LV200_MOVE_ALIAS_H

#include "../include/global.h"

// 比較・switch判定専用。技データ、名前、PP、タイプの参照には元のmoveを使う。
// 名前は Lv101〜200（0593）のまま、Lv1〜100（0600）の新技も同じ関数で扱う。
u16 Lv200MoveAlias(u16 move);

#endif
