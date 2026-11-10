#include "defines.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/constants/items.h"

/*
 * ひでんマシンは 1 個まで（patches/0392・2026-10-15・docs/research/2026-10-15_hm-no-duplicate.md）。
 * ユーザーのセーブで同じひでんマシンが 2 個になっていた（地方ごとの台本で別々にもらった推測）。
 * もう持っているひでんマシンは、渡す台本（std の ObtainItem と FindItem のひろう側・assembly/overworld_scripts/system_scripts.s）で
 * 数を増やさず「もう もっている！」とだけ出す。giveitem／obtainitem／finditem は全部ここを通る。
 */

//callasm: Var8004（渡す道具）がひでんマシン（HM01〜HM08）で、もう 1 個以上持っているなら LASTRESULT = 1、それ以外は 0
void HackHmOnce_CheckOwned(void)
{
	u16 item = Var8004;

	gSpecialVar_LastResult = (item >= ITEM_HM01_CUT && item <= ITEM_HM08_ROCK_CLIMB && CheckBagHasItem(item, 1) > 0);
}
