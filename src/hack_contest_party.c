#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/field_screen_effect.h"
#include "../include/party_menu.h"
#include "../include/sound.h"
#include "../include/constants/songs.h"
#include "hack_contest.h"

/*
 * コンテスト移植 S5b（FEAT-contest・patches/0985）: 出すポケモンを選ぶパーティ画面（原文 pokeemerald party_menu.c の PARTY_MENU_TYPE_CONTEST）。
 * 素の FR(J) の手持ち画面には menuType=CONTEST（2）の行表示もキャンセルの確認も無い（choosecontestmon も scrcmd.c の stub）ので次を足す:
 *  - 各行に「できる／できない」（原文 DisplayPartyPokemonDataForContest・資格は TryEnterContestMon と同じ判定・卵／ひんし／ランクの資格）
 *  - B のキャンセルで「さんかを やめますか？」（原文 DisplayCancelChooseMonYesNo の CONTEST 枝＝素の FR の CancelParticipationPrompt を呼ぶ）
 *  - 閉じたあと Var8004＝手持ちの番号（やめたら 0xFF）・台本の続きへ（原文 CB2_ChooseContestMon）
 * 部門・ランクは台本が Var8005／Var8006 に入れて開く（TryEnterContestMon と同じ読み口）。
 * hook: 素の DisplayPartyPokemonData（0811FA3C・static・呼び元 BL 3）を HackContest_DisplayPartyPokemonData に置き換える（hooks）。menuType が CONTEST 以外は
 *       素の関数と同じ内容（素 pokefirered src/party_menu.c:779 DisplayPartyPokemonData を 1 行ずつ写した・HP の 3 関数は素の ROM の番地）。
 *       キャンセルは既存の ChooseFaintedMonCancelHook（8120578・assembly/hooks/general_hooks.s）に CONTEST の枝を足して HackContest_PartyCancel へ。
 */

#define PARTY_MENU_TYPE_CONTEST_ 2
#define PARTYBOX_DESC_ABLE_ 5      /* 素の FR の sDescriptionStringTable（pokefirered include/constants/party_menu.h・原文 PARTYBOX_DESC_ABLE は 6 でエメラルドは 1 つずれる） */
#define PARTYBOX_DESC_NOT_ABLE_ 6

struct HackPartyMenuBoxInfoRects
{
	void (*blitFunc)(u8 windowId, u8 x, u8 y, u8 width, u8 height, bool8 isEgg);
	u8 dimensions[24];
	u8 descTextLeft;
	u8 descTextTop;
	u8 descTextWidth;
	u8 descTextHeight;
};

struct HackPartyMenuBox
{
	const struct HackPartyMenuBoxInfoRects* infoRects;
	const u8* spriteCoords;
	u8 windowId;
	u8 monSpriteId;
	u8 itemSpriteId;
	u8 pokeballSpriteId;
	u8 statusSpriteId;
};

extern struct HackPartyMenuBox* sPartyMenuBoxes;
void __attribute__((long_call)) DisplayPartyPokemonNickname(struct Pokemon* mon, struct HackPartyMenuBox* ptr, u8 c);
void __attribute__((long_call)) DisplayPartyPokemonLevelCheck(struct Pokemon*, struct HackPartyMenuBox*, u8);
void __attribute__((long_call)) DisplayPartyPokemonGenderNidoranCheck(struct Pokemon*, struct HackPartyMenuBox*, u8);

//素の ROM の番地（素の FR(J)・BL の飛び先を DisplayPartyPokemonData の機械語から読んだ）。tools/rom/verify_contest_script.py が素の ROM と突き合わせる
#define DisplayPartyPokemonHPCheck_ ((void (*)(struct Pokemon*, struct HackPartyMenuBox*, u8))0x08122171)
#define DisplayPartyPokemonMaxHPCheck_ ((void (*)(struct Pokemon*, struct HackPartyMenuBox*, u8))0x08122209)
#define DisplayPartyPokemonHPBarCheck_ ((void (*)(struct Pokemon*, struct HackPartyMenuBox*))0x081222BD)
#define CancelParticipationPrompt_ ((void (*)(u8))0x08121249)

void HackContest_DisplayPartyPokemonData(u8 slot)
{
	struct Pokemon* mon = &gPlayerParty[slot];
	struct HackPartyMenuBox* box = &sPartyMenuBoxes[slot];

	if (gPartyMenu.menuType == PARTY_MENU_TYPE_CONTEST_)
	{
		u8 e = HackContest_GetEligibility(slot, Var8005, Var8006);

		DisplayPartyPokemonSelectData(slot, (e == HACK_CAN_ENTER_CONTEST_EQUAL_RANK || e == HACK_CAN_ENTER_CONTEST_HIGH_RANK) ? PARTYBOX_DESC_ABLE_ : PARTYBOX_DESC_NOT_ABLE_);
		return;
	}
	if (GetMonData(mon, MON_DATA_IS_EGG, NULL))
	{
		box->infoRects->blitFunc(box->windowId, 0, 0, 0, 0, TRUE);
		DisplayPartyPokemonNickname(mon, box, 0);
	}
	else
	{
		box->infoRects->blitFunc(box->windowId, 0, 0, 0, 0, FALSE);
		DisplayPartyPokemonNickname(mon, box, 0);
		DisplayPartyPokemonLevelCheck(mon, box, 0);
		DisplayPartyPokemonGenderNidoranCheck(mon, box, 0);
		DisplayPartyPokemonHPCheck_(mon, box, 0);
		DisplayPartyPokemonMaxHPCheck_(mon, box, 0);
		DisplayPartyPokemonHPBarCheck_(mon, box);
	}
}

//B（キャンセル）: 原文は「さんかを やめますか？」のはい／いいえ。はいで Var8004＝キャンセル（素の Task_HandleCancelParticipationYesNoInput）
void HackContest_PartyCancel(u8 taskId)
{
	PlaySE(SE_SELECT);
	CancelParticipationPrompt_(taskId);
}

//原文 CB2_ChooseContestMon: 選んだ手持ちの番号（やめた／枠の外は PARTY_NOTHING_CHOSEN=0xFF）を Var8004 に入れて台本の続きへ
static void CB2_ChooseContestMon(void)
{
	if (Var8004 >= PARTY_SIZE)
		Var8004 = 0xFF;
	SetMainCallback2(CB2_ReturnToFieldContinueScript);
}

//台本の `choosecontestmon` の代わり（fadescreen 1 のあと・直後に waitstate）。原文 Task_ChooseContestMon: 手持ち画面（CONTEST・選んだら閉じる）
void HoennNative_ChooseContestMon(void)
{
	CleanupOverworldWindowsAndTilemaps();
	InitPartyMenu(PARTY_MENU_TYPE_CONTEST_, PARTY_LAYOUT_SINGLE, PARTY_ACTION_CHOOSE_AND_CLOSE, FALSE, PARTY_MSG_CHOOSE_MON, Task_HandleChooseMonInput, CB2_ChooseContestMon);
}

//台本の `special DoContestHallWarp`（原文: 暗転して会場の地図へ。setwarp は台本が済ませている）。素の DoWarp（暗転→地図の読み込み→明転）
void HoennNative_DoContestHallWarp(void)
{
	DoWarp();
}
