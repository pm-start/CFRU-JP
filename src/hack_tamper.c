#include "defines.h"
#include "../include/event_data.h"
#include "../include/load_save.h"
#include "hack_tamper.h"

u16 HackSaveVersion_Current(void);   //src/hack_save_version.c

//つづきから で MAC が合わない区画を見つけたとき（src/save.c の HandleLoadSector・src/hack_box_shelf.c の BoxShelf_OnLoad の後ろ）に呼ぶ。
//版とプレイ時間は最初の 1 回だけ残す（再署名しても・何度見つけても 最初の記録のまま）。理由の bit は貯める
void HackTamper_Mark(u16 reasons)
{
	if (!FlagGet(HACK_FLAG_TAMPERED))
	{
		FlagSet(HACK_FLAG_TAMPERED);
		VarSet(HACK_VAR_TAMPER_VERSION, HackSaveVersion_Current());
		VarSet(HACK_VAR_TAMPER_HOURS, gSaveBlock2->playTimeHours);
	}
	VarSet(HACK_VAR_TAMPER_REASON, VarGet(HACK_VAR_TAMPER_REASON) | reasons);   //理由は後から別のものが見つかっても足す
}

u16 HackTamper_Reasons(void)
{
	return VarGet(HACK_VAR_TAMPER_REASON);
}

bool8 HackTamper_IsMarked(void)
{
	return FlagGet(HACK_FLAG_TAMPERED);
}
