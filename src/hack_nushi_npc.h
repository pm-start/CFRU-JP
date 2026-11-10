#pragma once
#include "../include/global.h"

//ヌシにしてくれる人（2026-11-09・FEAT-nushi-npc・patches/0982）。スクリプトは assembly/hack_nushi_npc/・文は strings/hack_nushi_npc.string
//callasm HackNushiNpc_Check（0x8004 = 手持ちの番号）→ LASTRESULT: 0 できる・1 たまご・2 レベルが足りない・3 もうヌシ・4 ダーク・5 デルタ・6 手持ちに別のヌシ・7 選びなおし（番号が手持ちの外）
//callasm HackNushiNpc_Apply（0x8004）: ヌシの印を付けて能力を計算し直し、旗 HACK_FLAG_NUSHI_NPC を立てる
enum
{
	NUSHI_NPC_OK = 0,
	NUSHI_NPC_EGG,
	NUSHI_NPC_LOW_LEVEL,
	NUSHI_NPC_ALREADY,
	NUSHI_NPC_DARK,
	NUSHI_NPC_DELTA,
	NUSHI_NPC_OTHER,
	NUSHI_NPC_BAD_SLOT,
	NUSHI_NPC_RESULT_COUNT,
};

void HackNushiNpc_Check(void);
void HackNushiNpc_Apply(void);
