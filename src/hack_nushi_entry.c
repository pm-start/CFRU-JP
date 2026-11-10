/* 生成物: tools/fishing/gen_nushi.py（元は tools/fishing/nushi_src/nushi_entry.c・直に編集しない）。
 * 由来: liru 氏 pm-start/fishing-rework@aad8bcf（詳細は tools/fishing/README.md）。 */
#include "defines.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/field_player_avatar.h"
#include "hack_nushi_host.h"

/* 素の FR の関数（BPRJ.ld に番地が無い・CLAUDE.md 6.29 の流儀で番地を直に書く。どちらも実 ROM の入口の命令を確かめてある）。 */
#define ItemId_GetSecondaryId ((u8 (*)(u16)) (0x0809A4A4 | 1))

/* [PokemonStart] liru 氏 pm-start/fishing-rework@aad8bcf の integration/cfru/nushi_entry.c から、
 * 診断用 (FISHING_DEMO) の入口を除き、ItemId_GetSecondaryId の宣言を番地の直書きに変えた。
 *
 * Replace ItemUseOnFieldCB_Rod, not StartFishing itself: the original routine
 * remains callable and the CFRU encounter hook is left intact.
 * 差し替えは functionrewrites の「NfCfruRodCallback 080A26E4 1 0」。 */
void NfCfruRodCallback(u8 taskId){
 u8 rod=ItemId_GetSecondaryId(VarGet(0x800e));
 if(!NfUiOpen(rod,1,0))NfHostUnlock(0);
 DestroyTask(taskId);
}

/* Event ABI: VAR_0x8000=rod quality (0..2), VAR_RESULT=started. The event must
 * branch on failure BEFORE waitstate, otherwise allocation failure can hang.
 * [PokemonStart] 今回はイベントから呼ばない（台本は入れない）。 */
void NfCfruBeginScript(void){u16 rod=VarGet(0x8000);VarSet(0x800d,rod<=2?NfUiOpen(rod,1,1):0);}
