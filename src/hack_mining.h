#pragma once
/* 地下の化石ほり（ちかつうろ・FEAT-sinnoh-underground・patches/0872〜0874）。
 * 旗・変数の番号は tools/rom/build_hoenn_scripts.py の台帳（LEDGER_FLAGS・RESERVED_VARS）が決め、Tables/hoenn_script_flags.h が生成する:
 *   HACK_FLAG_MINING_EXPLAINED 0xFDE（説明を見せた印）・HACK_FLAG_MINING_WALL_BASE 0xFDF〜0xFE6（かべ 0〜7 を今日ほった印）・HACK_VAR_MINING_DAY 0x50B0（最後にほった日）
 * 地図の番号・かべの座標は tools/sinnoh/sinnoh_underground.py が生成する Tables/sinnoh_underground_ids.h（ここに直書きしない）。 */
#include "../include/global.h"
#include "Tables/hoenn_script_flags.h"
#include "Tables/sinnoh_underground_ids.h"

#define FLAG_EXPLAINED_MINING_RULES HACK_FLAG_MINING_EXPLAINED   /* em_mining.c が見る（最初にほったときの説明） */
#define HACK_MINING_WALL_MAX HACK_UG_WALL_COUNT

bool8 HackMining_IsDiggable(s16 x, s16 y);   /* 今の地図の(x,y)＝位置イベントの座標(+7 つき)のかべが今日まだほれるか */
void HackMining_MarkDug(void);               /* 目の前のかべを今日ほった印にする（ほり始めたときだけ呼ぶ） */
void FieldUseFunc_ExplorerKit(u8 taskId);    /* たんけんセット（ITEM_EXPLORER_KIT 0x370）をつかったとき */
