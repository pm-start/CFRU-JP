#include "defines.h"
#include "hack_fly_forbid.h"
#include "../include/event_data.h"
#include "../include/script.h"

/* 禁止エリアの表（verify_fly_forbid.py が参考/pokefirered・参考/pokeemerald の SafariZone_* と突き合わせる）。
   カントー: FR の群 1 の 63〜71（中央・東・北・西・休憩所 4・隠れ家）。
   ホウエン: エメラルドの群 26 + 43 = 69 の 0〜3（北西・北・南西・南）・11（休憩所）・12・13（北東・南東）。 */
struct HackFlyForbidMap { u8 group; u8 num; };
#define HACK_FLY_FORBID_COUNT 16
static const struct HackFlyForbidMap sForbidMaps[HACK_FLY_FORBID_COUNT] =
{
	{1, 63}, {1, 64}, {1, 65}, {1, 66}, {1, 67}, {1, 68}, {1, 69}, {1, 70}, {1, 71},
	{69, 0}, {69, 1}, {69, 2}, {69, 3}, {69, 11}, {69, 12}, {69, 13},
};

extern const u8 HoennSafari_EventScript_Exit[];   //assembly/hoenn_safari/safari_exit.s（ホウエンなら 121 ばんどうろのゲート・それ以外は素のセキチクのゲートへ）

bool8 HackFly_InSafariMap(void)
{
	u8 i;
	//★ 2026-11-03 perf-region-2: 毎コマ呼ばれる（HackSafariGuardTick）。表の群は 1 と 69 だけ（verify_perf_region_2.py が表と突き合わせる）なので、
	//  ほかの群なら 16 件を見ずに帰る
	if (gSaveBlock1->location.mapGroup != 1 && gSaveBlock1->location.mapGroup != 69)
		return FALSE;
	for (i = 0; i < HACK_FLY_FORBID_COUNT; ++i)
	{
		if (gSaveBlock1->location.mapGroup == sForbidMaps[i].group && gSaveBlock1->location.mapNum == sForbidMaps[i].num)
			return TRUE;
	}
	return FALSE;
}

bool8 HackFly_AreaForbidden(void)
{
	return FlagGet(FLAG_SYS_SAFARI_MODE) != 0 || HackFly_InSafariMap();
}

/* すでに壊れたセーブ（ゆうらんでサファリの中に着地して旗が消えた）の直し: サファリの地図にいるのに旗が無く、台本も動いていないなら、
   サファリを出る台本（素の終わり方と同じ）を動かす。正規の終わり（special ExitSafariMode で旗を消してすぐ warp）も、
   入る側（EnterSafariMode で旗を立ててから warp）も、台本の最中（ScriptContext2 が有効）なので当たらない。
   書き換え可能な static は使えない（CLAUDE.md 6.13・ROM に置かれて書けない）ので、数えずに毎フレーム見る。 */
void HackSafariGuardTick(void)
{
	//★ 2026-11-03 perf-region-2: 先に地図で見る（旗は 1 回 約 160 サイクル・サファリの外なら地図の群の比べ 2 回で済む）。3 つとも読むだけの条件の「または」なので順序を替えても同じ
	if (!HackFly_InSafariMap() || FlagGet(FLAG_SYS_SAFARI_MODE) || ScriptContext2_IsEnabled())
		return;
	ScriptContext1_SetupScript(HoennSafari_EventScript_Exit);
}
