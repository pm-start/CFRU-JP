#include "defines.h"
#include "../include/event_data.h"
#include "hack_box_shelf.h"
#include "hack_johto.h"

//ジョウトが開いているか（hack_johto.h）。栓（HACK_JOHTO_PLUG）が 1 の間は常に偽
bool8 HackJohto_Open(void)
{
#if HACK_JOHTO_PLUG
	return FALSE;
#else
	return VarGet(HACK_SINNOH_STORY_VAR) >= HACK_SINNOH_STORY_END && BoxShelf_ExtAvailable()  /* 覚えてある判定（HackExtSave_Available は 1 回約 34 回のフラッシュ読み＝熱い道では使わない・6.112） */;
#endif
}
