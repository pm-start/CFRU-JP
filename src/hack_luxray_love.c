#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/party_menu.h"
#include "../include/pokemon.h"
#include "../include/sound.h"
#include "../include/constants/items.h"
#include "../include/constants/songs.h"
#include "hack_challenge_luxray.h"

/*
 * 「リルの あいじょう」（ITEM_RIRU_LOVE 0x366・2026-10-19・patches/0617）。
 * ユーザー「レントラー縛りの時のレントラーの手持ちはリルの愛情って効果のないアイテムで固定しといて」
 *            「レントラー縛りのとき買ったアイテムすべてモンスターボール以外はリルの愛情に上書き」。
 *
 *   道具: 効果なし（holdEffect 0・使えない・値段 0＝売れない）。絵はハートのウロコ。
 *   固定: しばり「ぜんぶレントラー」が ON のあいだ、リルの あいじょうを持つポケモンは その道具を はずせない・ほかの道具に替えられない。
 *         ・手持ちメニューの「もたせる」「もちものをとる」「もちものを うつす」（src/party_menu.c の gPartyMenuCursorOptions・CursorCb_MoveItem）
 *         ・バッグの「もたせる」で相手に選ぶ（IsSelectedMonNotEgg の置き換え＝functionrewrites 0812054C）
 *         ・戦闘のどろぼう・はたきおとす・トリック・すりかえ・なげつける など（battle_util.c の CanTransferItem）
 *       （FRLG のボックス画面には もちものを持たせる／とる操作が無い＝入口は上の 3 つで全部）
 *   持たせるとき: 手に入れた子を レントラーにする所（HackChallengeLuxray_OnObtain）で、もとの持ち物はバッグ（いっぱいならパソコン）へ移し、リルの あいじょうを持たせる。
 *       どちらにも入らないときは もとの持ち物のままにする（道具を失わない・その子は固定されない）。
 *   買った道具の置き換え: ON のあいだ、お店・お母さんの買い物・じっせきの ほうしゅうの道具は同じ数の リルの あいじょうになる（モンスターボールだけ そのまま）。
 *       ひろった・もらった道具・わざマシンは そのまま（置き換えは この 3 つだけ）。お店のお金は表どおり払う。
 */

#define SE_LOVE_ERROR SE_ERROR

//固定されている子か（しばり ON・リルの あいじょうを持つ・タマゴではない）
bool8 HackLuxrayLove_IsFixed(const struct Pokemon* mon)
{
	if (!HackChallengeLuxray_On())
		return FALSE;
	return GetMonData((struct Pokemon*) mon, MON_DATA_HELD_ITEM, NULL) == ITEM_RIRU_LOVE
	    && !GetMonData((struct Pokemon*) mon, MON_DATA_IS_EGG, NULL);
}

//手に入れた子に リルの あいじょうを持たせる（hack_challenge_luxray.c の OnObtain から・旗が ON のときだけ呼ばれる）
void HackLuxrayLove_Apply(struct Pokemon* mon)
{
	u16 held = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
	u16 love = ITEM_RIRU_LOVE;

	if (held == ITEM_RIRU_LOVE || GetMonData(mon, MON_DATA_IS_EGG, NULL))
		return;
	if (held != ITEM_NONE)
	{
		if (IsMail(held))   //手紙は持ち物ごと動かせない（もとのまま）
			return;
		if (!AddBagItem(held, 1) && !HackMomSavings_AddPcItem(held))
			return;   //バッグもパソコンもいっぱい: もとの持ち物のまま（失くさない）
	}
	SetMonData(mon, MON_DATA_HELD_ITEM, &love);
}

//お店・お母さんの買い物・じっせきの ほうしゅうで受け取る道具（ON のあいだ モンスターボール以外は リルの あいじょう）
u16 HackLuxrayLove_ShopItem(u16 item)
{
	if (item == ITEM_NONE || item == ITEM_POKE_BALL || item == ITEM_RIRU_LOVE || !HackChallengeLuxray_On())
		return item;
	return ITEM_RIRU_LOVE;
}

//functionrewrites: IsSelectedMonNotEgg 0812054C（素は「タマゴなら SE_ERROR を鳴らして FALSE」・引数 1・戻り値あり）。
//  バッグの「もたせる」（手持ちから相手を選ぶ）で 固定された子を選べなくする（タマゴと同じ音だけ）
bool8 HackLuxrayLove_IsSelectedMonNotEgg(u8* slotPtr)
{
	struct Pokemon* mon = &gPlayerParty[*slotPtr];

	if (GetMonData(mon, MON_DATA_IS_EGG, NULL) == TRUE)
	{
		PlaySE(SE_ERROR);
		return FALSE;
	}
	if ((gPartyMenu.action == PARTY_ACTION_GIVE_ITEM || gPartyMenu.action == PARTY_ACTION_GIVE_PC_ITEM) && HackLuxrayLove_IsFixed(mon))
	{
		PlaySE(SE_LOVE_ERROR);
		return FALSE;
	}
	return TRUE;
}
