#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/field_player_avatar.h"
#include "../include/main.h"
#include "../include/palette.h"
#include "../include/random.h"
#include "../include/sprite.h"
#include "../include/new/Vanilla_functions_battle.h"
#include "../include/safari_zone.h"
#include "../include/string_util.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/constants/flags.h"
#include "map_region.h"
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"
#include "hack_safari_pokeblock.h"

/*
 * ホウエンのサファリゾーンのポロック（2026-11-09 FEAT-pokeblock 段 6・patches/0978）。
 * 原文: 参考/pokeemerald/src/safari_zone.c（おき場 10 台・100 歩・|dx|+|dy| <= 5）・src/pokeblock.c（PokeblockAction_UseInBattle／UseOnPokeblockFeeder）・
 *       src/battle_util.c（HandleAction_ThrowPokeblock・sPkblToEscapeFactor）・src/wild_encounter.c（PickWildMonNature の 80%）。
 * 原文との差（ユーザー決定・Commander 決定 2026-11-09）:
 *   - 置き場は EWRAM の固定番地（原文は static 配列＝書き換え可能な static は ROM に置かれる 6.13・ヒープは地図の読み込みで消える）。
 *   - 逃げ係数が 0 になる原文の不具合（BUGFIX 側の <=）は直す側。
 *   - 歩数の制限なしの決定（SafariZoneTakeStep の return FALSE）は変えない。おき場の歩数だけ減らす。
 *   - 通信・TV（ファンクラブ）は無い（pokeblockThrows は数えない）。
 *   - 投げる演出は原文 B_ANIM_POKEBLOCK_THROW（General_PokeblockThrow）の写し（段 6b）。台本は FR の「エサを投げる」と同じ命令列＝絵とパレットだけ日本語版エメラルド ROM の物
 *     （src/hack_pokeblock_throw_gfx.c・生成）。圧縮の無い生のまま LoadSpriteSheet／LoadSpritePalette（札は ANIM_TAG_POKEBLOCK＝0x281D・FR の SAFARI_BAIT と同じ数・同時には使わない）。
 *   - おき場の距離は台ごとに独立（原文の GetPokeblockFeederWithinRange は x・y を台ごとに上書きして引くので、同じ地図に 2 台以上あると 2 台目からの距離が累積する＝原文の癖。
 *     Commander 決定 2026-11-09 で直す側）。
 *   - 戦闘のメニュー「いしころ」は素の FR のまま（原文は「ちかづく」だが、その動きの移植はこの段の外）。
 * カントーのサファリは素のまま（おき場は 0 台・ホウエンでなければ素の機械語へ戻る・PickWildMonNature は Random を呼ばない）。
 */

#define sFeeders ((struct HackFeeder*) HACK_FEEDERS_ADDR)
typedef char HackFeeder_Size[sizeof(struct HackFeeder) == 16 ? 1 : -1];
typedef char HackFeeder_Fits[HACK_NUM_FEEDERS * 16 == 0xA0 ? 1 : -1];   //0x0203F048 + 0xA0 = 0x0203F0E8（空きの終わり）

//素の FR（JP）の番地（BPRJ.ld に無いもの・tools/rom/verify_safari_pokeblock.py が逆アセンブルで確かめる）
#define Sfr_BattlePutTextOnWindow ((void (*)(const u8*, u8))(0x080D980C + 1))
#define Sfr_SafariHandleChooseAction ((void (*)(void))(0x080DEC60 + 1))
#define Sfr_SafariBufferExecCompleted ((void (*)(void))(0x080DE8B0 + 1))
#define Sfr_EmitTwoReturnValues ((void (*)(u8, u8, u16))(0x0800E1FC + 1))
#define Sfr_ThrowBait ((void (*)(void))(0x08016330 + 1))              //素の HandleAction_ThrowBait（アクション表 0x0820CAB0 の [6]）
#define Sfr_BattleMainCB2 ((MainCallback) (0x080109C0 + 1))
#define SFR_SAFARI_MENU_TEXT ((const u8*) 0x083C46D5)                 //素の gText_SafariZoneMenu
#define SFR_ACTION_SAFARI_BAIT 6
#define gBattlerControllerFuncsArr ((void (**)(void)) 0x03005020)
#define gBattlerInMenuIdVar (*(u8*) 0x02023F64)

extern const u8 gText_HoennSafariMenu[];
extern const u8 HoennSafari_BattleScript_ThrowPokeblock[];

static bool8 InHoennSafari(void)
{
	return GetCurrentMapRegion() == MAP_REGION_HOENN;
}

//---------------------------------------------------------------- おき場

void HackSafari_ClearFeeders(void)
{
	u32 i;
	u8* p = (u8*) HACK_FEEDERS_ADDR;

	for (i = 0; i < HACK_NUM_FEEDERS * sizeof(struct HackFeeder); i++)
		p[i] = 0;
}

//callasm: 出口（assembly/hoenn_safari/safari_exit.s）。カントーでも安全（0 のまま 0 にするだけ）
void HackSafari_Native_ClearFeeders(void)
{
	HackSafari_ClearFeeders();
}

void HackSafari_DecrementFeeders(void)
{
	u8 i;

	for (i = 0; i < HACK_NUM_FEEDERS; i++)
	{
		if (sFeeders[i].stepCounter != 0)
		{
			sFeeders[i].stepCounter--;
			if (sFeeders[i].stepCounter == 0)
			{
				u32 k;
				u8* p = (u8*) &sFeeders[i];

				for (k = 0; k < sizeof(struct HackFeeder); k++)
					p[k] = 0;
			}
		}
	}
}

//原文 GetPokeblockFeederWithinRange。★ 原文は x・y を台ごとに上書きして引く（同じ地図に 2 台以上あると 2 台目からの距離が累積する）が、台ごとに引き直す（Commander 決定 2026-11-09・原文との差）
struct Pokeblock* HackSafari_GetActivePokeblock(void)
{
	s16 x, y, dx, dy;
	u16 i;

	PlayerGetDestCoords(&x, &y);
	for (i = 0; i < HACK_NUM_FEEDERS; i++)
	{
		if (gSaveBlock1->location.mapNum == sFeeders[i].mapNum)
		{
			dx = x - sFeeders[i].x;
			dy = y - sFeeders[i].y;
			if (dx < 0)
				dx *= -1;
			if (dy < 0)
				dy *= -1;
			if ((dx + dy) <= HACK_FEEDER_RANGE)
				return &sFeeders[i].pokeblock;
		}
	}
	return NULL;
}

bool8 HackSafari_ActivateFeeder(const struct Pokeblock* pb)
{
	s16 x, y;
	u8 i;

	for (i = 0; i < HACK_NUM_FEEDERS; i++)
	{
		if (sFeeders[i].mapNum == 0 && sFeeders[i].x == 0 && sFeeders[i].y == 0)
		{
			GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
			sFeeders[i].mapNum = gSaveBlock1->location.mapNum;
			sFeeders[i].pokeblock = *pb;
			sFeeders[i].stepCounter = HACK_FEEDER_STEPS;
			sFeeders[i].x = x;
			sFeeders[i].y = y;
			return TRUE;
		}
	}
	return FALSE;
}

//specialvar GetPokeblockFeederInFront（台本 HoennSafari_EventScript_PokeblockFeeder が callnative で呼ぶ）: LASTRESULT = 台の番号（無ければ 0xFFFF）・あれば gStringVar1 = ポロックの名前
void HackSafari_Native_FeederInFront(void)
{
	s16 x, y;
	u16 i;

	GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
	for (i = 0; i < HACK_NUM_FEEDERS; i++)
	{
		if (gSaveBlock1->location.mapNum == sFeeders[i].mapNum && sFeeders[i].x == x && sFeeders[i].y == y)
		{
			gSpecialVar_LastResult = i;
			StringCopy(gStringVar1, gPokeblockNames[sFeeders[i].pokeblock.color]);
			return;
		}
	}
	gSpecialVar_LastResult = 0xFFFF;
}

//special OpenPokeblockCaseOnFeeder: ケースを開く（戻りは台本の続き＝台本は waitstate で待つ）。置いたら LASTRESULT = 元の番号・やめたら 0xFFFF（ケースの画面が書く）
void HackSafari_Native_OpenCaseOnFeeder(void)
{
	OpenPokeblockCase(PBLOCK_CASE_FEEDER, PBC_RET_SCRIPT);
}

//---------------------------------------------------------------- 出会い

//原文 PickWildMonNature の先頭の枝。0xFF = この枝は使わない（素の動き）。ホウエン以外は Random() も呼ばない
u8 HackSafari_PickNature(void)
{
	if (GetSafariZoneFlag() && InHoennSafari() && Random() % 100 < HACK_FEEDER_NATURE_PERCENT)
	{
		struct Pokeblock* pb = HackSafari_GetActivePokeblock();

		if (pb != NULL)
		{
			u8 natures[NUM_NATURES];
			u8 i, j;

			for (i = 0; i < NUM_NATURES; i++)
				natures[i] = i;
			for (i = 0; i < NUM_NATURES - 1; i++)
			{
				for (j = i + 1; j < NUM_NATURES; j++)
				{
					if (Random() & 1)
					{
						u8 temp = natures[i];

						natures[i] = natures[j];
						natures[j] = temp;
					}
				}
			}
			for (i = 0; i < NUM_NATURES; i++)
			{
				if (PokeblockGetGain(natures[i], pb) > 0)
					return natures[i];
			}
		}
	}
	return 0xFF;
}

//---------------------------------------------------------------- 戦闘

u8 HackSafari_BattleReaction(u8 nature, const struct Pokeblock* pb)
{
	s16 gain = PokeblockGetGain(nature, pb);

	if (gain == 0)
		return HACK_PKBL_REACT_CURIOUS;
	if (gain > 0)
		return HACK_PKBL_REACT_ENTHRALLED;
	return HACK_PKBL_REACT_IGNORED;
}

//メニューの文: ホウエンは「ボール ポロック／いしころ にげる」（原文の日本語版 ROM の「ボール ポロック」・素の FR の「エサだま」の所だけ）。hooks の HackSafari_MenuHook（0x080DEC78）から
void HackSafari_PutMenuText(void)
{
	Sfr_BattlePutTextOnWindow(InHoennSafari() ? gText_HoennSafariMenu : SFR_SAFARI_MENU_TEXT, 2);
}

static void SafariWaitForCase(void);

static void SafariOpenCase(void)
{
	if (!gPaletteFade->active)
	{
		gBattlerControllerFuncsArr[gActiveBattler] = SafariWaitForCase;
		FreeAllWindowBuffers();
		gSpecialVar_ItemId = 0;
		OpenPokeblockCase(PBLOCK_CASE_BATTLE, PBC_RET_BATTLE);
	}
}

//原文 CompleteWhenChosePokeblock。戦闘の画面に戻るまで待つ（callback1 は動き続けるので callback2 を見る）。やめたら（0）行動のメニューへ戻る
static void SafariWaitForCase(void)
{
	if (gMain.callback2 == Sfr_BattleMainCB2 && !gPaletteFade->active)
	{
		if (gSpecialVar_ItemId == 0)
			Sfr_SafariHandleChooseAction();
		else
		{
			Sfr_EmitTwoReturnValues(1, SFR_ACTION_SAFARI_BAIT, gSpecialVar_ItemId);
			Sfr_SafariBufferExecCompleted();
		}
	}
}

//hooks の HackSafari_ActionHook（0x080DE5AC）から: 行動のメニューでカーソル 1（素は「エサだま」）に A。ホウエンならケースを開く（TRUE）・それ以外は素の動き（FALSE）
u8 HackSafari_TryOpenCase(void)
{
	if (!InHoennSafari())
		return FALSE;
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, 0);
	gBattlerControllerFuncsArr[gActiveBattler] = SafariOpenCase;
	gBattlerInMenuIdVar = gActiveBattler;
	return TRUE;
}

//原文 sPkblToEscapeFactor（行＝何回目か 0〜4・列＝気になる／夢中／無視）
static const u8 sPkblToEscapeFactor[][3] =
{
	{0, 0, 0},
	{3, 5, 0},
	{2, 3, 0},
	{1, 2, 0},
	{1, 1, 0},
};

//アクション表（0x0820CAB0）の [6] の語を routinepointers でこれに差し替える。引数が 0 なら素の HandleAction_ThrowBait（カントー・ホウエンの「エサだま」ではない道）
void HackSafari_HandleActionBait(void)
{
	u8 bank = gBanksByTurnOrder[gCurrentTurnActionNumber];
	u16 v = gBattleBufferB[bank][2] | (gBattleBufferB[bank][3] << 8);
	u8 code, color, factor;

	if (v == 0)
	{
		Sfr_ThrowBait();
		return;
	}
	gBankAttacker = bank;
	gBattle_BG0_X = 0;
	gBattle_BG0_Y = 0;
	code = v & 0xFF;
	color = v >> 8;
	if (code < HACK_PKBL_REACT_CURIOUS || code > HACK_PKBL_REACT_IGNORED)
		code = HACK_PKBL_REACT_IGNORED;
	if (color == PBLOCK_CLR_NONE || color > PBLOCK_CLR_GOLD)
		color = PBLOCK_CLR_RED;
	StringCopy(gBattleTextBuff1, gPokeblockNames[color]);
	gBattleCommunication[MULTISTRING_CHOOSER] = code - 1;

	if (gNewBS->hackSafariPkblCounter < 3)
		gNewBS->hackSafariPkblCounter++;
	if (gBattleStruct->safariEscapeFactor > 1)
	{
		factor = sPkblToEscapeFactor[gNewBS->hackSafariPkblCounter][code - 1];
		if (gBattleStruct->safariEscapeFactor <= factor)    //★ 原文の #ifdef BUGFIX 側（<=）。BUGFIX 無しの原文は < で、同じ値だと逃げ係数が 0 になる（ポロックの不具合）＝ユーザー決定: 直す側
			gBattleStruct->safariEscapeFactor = 1;
		else
			gBattleStruct->safariEscapeFactor -= factor;
	}
	gBattlescriptCurrInstr = HoennSafari_BattleScript_ThrowPokeblock;
	gCurrentActionFuncId = ACTION_RUN_BATTLESCRIPT;
}

//---------------------------------------------------------------- 投げる演出（原文 AnimTask_LoadPokeblockGfx）
//戦闘アニメ gBattleAnims_General の最後の台本（assembly/data/special_battle_anim_table.s の HACK_ANIM_POKEBLOCK_THROW）から launchtask で呼ぶ。
//原文は gBattleAnimPicTable／PaletteTable の ANIM_TAG_POKEBLOCK の圧縮データをヒープへ読む。こちらは同じ中身の生データ（日本語版エメラルド ROM から生成）を読む。
//読んだ絵は台本の最後の AnimTask_FreeBaitGfx（FR の同じ札 0x281D を解放）が解放する（原文の FreePokeblockGfx と同じ札）。
extern const u8 gHackPokeblockThrowTiles[];
extern const u8 gHackPokeblockThrowPal[];

void AnimTask_HackPokeblockLoadGfx(u8 taskId)
{
	struct SpriteSheet sheet;
	struct SpritePalette pal;

	sheet.data = gHackPokeblockThrowTiles;
	sheet.size = 0x80;
	sheet.tag = HACK_ANIM_TAG_POKEBLOCK;
	pal.data = (const u16*) gHackPokeblockThrowPal;
	pal.tag = HACK_ANIM_TAG_POKEBLOCK;
	LoadSpriteSheet(&sheet);
	LoadSpritePalette(&pal);
	DestroyAnimVisualTask(taskId);
}
