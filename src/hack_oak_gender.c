#include "defines.h"
#include "../include/menu.h"
#include "../include/palette.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/new_menu_helpers.h"
#include "hack_player_kind.h"

/*
 * はじめの話の主人公選択を 3 択に（FEAT-player-mei）。素（pokefirered src/oak_speech.c・日本語版）の
 *   Task_OakSpeech_ShowGenderOptions 0813069C / Task_OakSpeech_HandleGenderInput 081307B8
 * を丸ごと置き換える（hooks・r1）。3 つ目を選ぶと playerGender=1（女）＋メイの印。
 * 立ち絵: LoadTrainerPic の女の枝（08131A44）を HackOakFemalePicHook に向け、メイの印のときはメイの絵に。
 * 名前の候補: PrintNameChoiceOptions／GetDefaultName の表の選びを HackOak_PlayerNameTable／HackOak_RivalNameTable に。
 * 文言は strings/hack_player_mei.string の 3 本だけ（差し替えはここ）。
 */
extern const u8 gText_PlayerMei_Choice3[];
extern const u8 gText_PlayerMei_Name[];
extern const u8 gText_PlayerMei_Rival[];
extern const u8 gBwMeiOak[];
extern const u8 gBwMeiOakPal[];

#define gText_Boy ((const u8*) 0x083DD4C6)
#define gText_Girl ((const u8*) 0x083DD4CC)
#define Menu_ProcessInputNoWrapAround ((s8 (*)(void)) (0x08110538 | 1))
#define sOakSpeechResourcesPtr (*(u8**) 0x0203B07C)    //textColor は +0x1C
#define OAK_CLEAR_GENDER_WINDOWS ((TaskFunc) 0x08130805)

#define tMenuWindowId data[13]

/* baseBlock: 素は 372〜407（36 タイル）。372〜540 付近は同じ画面の文の窓の字のタイルなので、6 行・10 幅（60 タイル。「おとこのこ？」が入る幅）は手前の空き 312〜371 に置く（mGBA で VRAM を見て決めた） */
static const struct WindowTemplate sGenderWindow3 =
{
	.bg = 0, .tilemapLeft = 18, .tilemapTop = 9, .width = 10, .height = 6, .paletteNum = 15, .baseBlock = 312,
};

static void SetColors(void)
{
	u8* c = sOakSpeechResourcesPtr + 0x1C;
	c[0] = 1;
	c[1] = 2;
	c[2] = 3;
}

void HackOak_HandleGenderInput(u8 taskId);

void HackOak_ShowGenderOptions(u8 taskId)
{
	u8 win;
	if (IsTextPrinterActive(0))
		return;
	win = AddWindow(&sGenderWindow3);
	gTasks[taskId].tMenuWindowId = win;
	PutWindowTilemap(win);
	DrawStdFrameWithCustomTileAndPalette(win, TRUE, GetStdWindowBaseTileNum(), 14);
	FillWindowPixelBuffer(win, 0x11);
	SetColors();
	AddTextPrinterParameterized3(win, 2, 12, 1, sOakSpeechResourcesPtr + 0x1C, 0, gText_Boy);
	SetColors();
	AddTextPrinterParameterized3(win, 2, 12, 17, sOakSpeechResourcesPtr + 0x1C, 0, gText_Girl);
	SetColors();
	AddTextPrinterParameterized3(win, 2, 12, 33, sOakSpeechResourcesPtr + 0x1C, 0, gText_PlayerMei_Choice3);
	Menu_InitCursor(win, 2, 2, 1, GetFontAttribute(2, 1) + 4, 3, 0);
	CopyWindowToVram(win, 3);
	gTasks[taskId].func = HackOak_HandleGenderInput;
}

void HackOak_HandleGenderInput(u8 taskId)
{
	s8 input = Menu_ProcessInputNoWrapAround();
	switch (input)
	{
		case 0:
			gSaveBlock2->playerGender = MALE;
			HackPlayerKind_Set(PLAYER_KIND_NORMAL);
			break;
		case 1:
			gSaveBlock2->playerGender = FEMALE;
			HackPlayerKind_Set(PLAYER_KIND_NORMAL);
			break;
		case 2:
			gSaveBlock2->playerGender = FEMALE;		//メイの土台は女
			HackPlayerKind_Set(PLAYER_KIND_MEI);
			break;
		default:
			return;
	}
	gTasks[taskId].func = OAK_CLEAR_GENDER_WINDOWS;
}

/* LoadTrainerPic の女の枝（08131A44〜）の代わり。パレットを読み、絵の本体の番地を返す（呼び元の 08131A68 が展開する） */
const void* HackOak_LoadFemalePicPal(void)
{
	if (GetPlayerGender3() == PLAYER_GENDER3_MEI)
	{
		LoadPalette(gBwMeiOakPal, 0x40, 0x40);
		return gBwMeiOak;
	}
	LoadPalette(*(const void* const*) 0x08131A54, 0x40, 0x40);	//素のリテラル語（BW の女の立ち絵に repoints 済み）
	return *(const void* const*) 0x08131A58;
}

/* 名前の候補。先頭が既定。メイの主人公の名前の既定は「メイ」（ユーザー 2026-10-19）＝名前を決めずに進めたときの乱数（GetDefaultName は
 * 19 件から選ぶ）でも必ずメイになるよう 19 件ぜんぶメイ */
static const u8* const sMeiPlayerNames[19] =
{
	gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name,
	gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name,
	gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name,
	gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name, gText_PlayerMei_Name,
};

/* ライバル: 先頭＝ヒュウ（BW2 のライバル・Commander 判断）。2〜4 件目は素の候補（グリーン・ゲイリー・カズ）の文字列 */
static const u8* const sMeiRivalNames[4] =
{
	gText_PlayerMei_Rival,
	(const u8*) 0x081A9E58, (const u8*) 0x081A9E78, (const u8*) 0x081A9E80,
};

const u8* const* HackOak_PlayerNameTable(void)
{
	if (gSaveBlock2->playerGender == MALE)
		return (const u8* const*) 0x084253A0;
	if (GetPlayerGender3() == PLAYER_GENDER3_MEI)
		return sMeiPlayerNames;
	return (const u8* const*) 0x084253EC;
}

const u8* const* HackOak_RivalNameTable(void)
{
	if (GetPlayerGender3() == PLAYER_GENDER3_MEI)
		return sMeiRivalNames;
	return (const u8* const*) 0x08425438;
}

/* PrintNameChoiceOptions のループから毎回呼ばれる。素の表（男 084253A0・女 084253EC・ライバル 08425438）を受け取り、
 * メイのときだけ女の表・ライバルの表をメイ用に替える。ほかはそのまま返す */
const u8* const* HackOak_FixNameTable(const u8* const* table)
{
	if (GetPlayerGender3() == PLAYER_GENDER3_MEI)
	{
		if (table == (const u8* const*) 0x084253EC)
			return sMeiPlayerNames;
		if (table == (const u8* const*) 0x08425438)
			return sMeiRivalNames;
	}
	return table;
}
