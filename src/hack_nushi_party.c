#include "defines.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"   //StorageGetCurrentBox（long_call・これが無いと Thumb から ARM の thunk へ blx して ARM7 で落ちる）
#include "../include/new/pokemon_storage_system.h"
#include "hack_delta.h"
#include "hack_kind.h"
#include "hack_box_shelf.h"

/*
 * ヌシは手持ちに 1 匹だけ（PC の画面・patches/0981・2026-11-09 ユーザー）。
 * ボックスの画面（src/hack_box_shelf.c・原作 ROM の PSS）の中で、手持ちの枠へ入る入口を止める。素の ROM の地番は hack_box_shelf.c の同じ名前と同じ（FRLG 日本版）。
 *   BoxShelf_WithdrawCount … Task_WithdrawMon（0808D700 付近・「てもちに くわえる」）の CalculatePlayerPartyCount（0808D710）の呼びの置き換え（hooks・assembly/hack_box_shelf/box_shelf_hooks.s）
 *   HackNushi_PssPlaceRefused … 手持ちの枠へ「つかんだ子」をおく・入れかえる入力（hack_box_shelf.c の BoxShelf_PssHandleInput から）
 */

#define sCursorPosition (*(s8*) 0x0203976D)
#define gStorageState (**(u8**) 0x020396FC)       //gStorage->state
#define sMovingMon ((const struct Pokemon*) (*(u8**) 0x020396FC + 0x20A0))   //gPSSData->movingMon（つかんでいる子）

void BoxShelf_PrintNushiRefusal(void);   //src/hack_box_shelf.c

//つかんだ子がヌシで、手持ちの cursorPos の枠に入る（空きの枠へおく・入っている子と入れかえる）とき、ほかにヌシがいれば真。入れかえで出る子は数えない・空の枠は除外なし。
//つかんだ子は元の枠が空になっている（素の PurgeMonOrBoxMon）ので、手持ちからつかんだヌシを手持ちへ戻すのは通る
bool8 HackNushi_PssPlaceRefused(s8 cursorPos)
{
	u8 except;

	if (cursorPos < 0 || cursorPos >= PARTY_SIZE)
		return FALSE;
	except = gPlayerParty[cursorPos].species != SPECIES_NONE ? (u8) cursorPos : PARTY_SIZE;
	return HackNushi_PartyHasOther(gPlayerParty, sMovingMon, except);
}

//hooks: 0〜6 は手持ちの数（素と同じ）・0xFF は「ヌシは手持ちに 1 匹だけ」で止めた（文と state 1 はここで出す＝素の「てもちが いっぱい」と同じ戻り方）
u8 BoxShelf_WithdrawCount(void)
{
	struct CompressedPokemon comp;

	//取り出す子の印だけ見る（BoxMonAtToMon は能力値の計算まで走るので使わない・箱の縮めた形から印だけ読む）
	BoxShelf_ReadMon(StorageGetCurrentBox(), sCursorPosition, &comp);
	if (HackKind_CompGet(&comp) == HACK_KIND_NUSHI && HackNushi_PartyHasNushi(gPlayerParty, PARTY_SIZE))
	{
		BoxShelf_PrintNushiRefusal();
		gStorageState = 1;
		return 0xFF;
	}
	return CalculatePlayerPartyCount();
}

/*
 * 育て屋に「あずける」: ヌシは あずけられない（ユーザー決定 2026-11-09「ヌシA」・patches/0981）。
 * 育て屋の「どの子をあずけますか」は、どの地方の育て屋も同じ手持ちの画面（PARTY_MENU_TYPE_DAYCARE・special ChooseSendDaycareMon）を使い、
 * 「あずける」を選ぶと素の CursorCB_Store（0x08124B30・PlaySE / gSpecialVar_0x8004 = スロット / Task_ClosePartyMenu）が走る。これを丸ごと置き換える（hooks）。
 * ヌシのときは文を出して手持ちの画面へ戻る（台本は「あずける」が選ばれたあとから続く＝選び直しの流れは壊れない）。
 */
#include "../include/party_menu.h"
#include "../include/sound.h"
#include "../include/task.h"
#include "../include/event_data.h"
#include "../include/constants/songs.h"
#include "party_menu_internal.h"
#include "hack_kind.h"

void __attribute__((long_call)) Task_ClosePartyMenu(u8 taskId);
void __attribute__((long_call)) Task_ReturnToChooseMonAfterText(u8 taskId);
void __attribute__((long_call)) PartyMenuRemoveWindow(u8* ptr);
u8 __attribute__((long_call)) DisplayPartyMenuMessage(const u8* str, u8 keepOpen);
extern const u8 gText_Nushi_NoDaycare[];   //strings/hack_nushi_party.string

void HackNushi_CursorCB_Store(u8 taskId)
{
	if (HackKind_Get((const struct BoxPokemon*) &gPlayerParty[gPartyMenu.slotId]) == HACK_KIND_NUSHI)
	{
		PlaySE(SE_ERROR);
		PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[0]);
		PartyMenuRemoveWindow(&sPartyMenuInternal->windowId[1]);
		DisplayPartyMenuMessage(gText_Nushi_NoDaycare, 1);
		gTasks[taskId].func = Task_ReturnToChooseMonAfterText;
		return;
	}
	PlaySE(SE_SELECT);
	Var8004 = gPartyMenu.slotId;   //素の gSpecialVar_0x8004（0x02036FF4）
	Task_ClosePartyMenu(taskId);
}
