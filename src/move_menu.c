#include "hack_move_alias.h"
#include "defines.h"
#include "defines_battle.h"
#include "hack_difficulty.h"
#include "hack_challenge.h" //[hack] patches/0510 しばり
#include "hack_snatch_ball.h" //[hack] patches/0961 スナッチボール
#include "hack_challenge_b.h" //[hack] patches/0540 しばり B（こうたいきんし）
#include "hack_fuuin.h" //[hack] patches/0560 縛り ふういんルーレット（封印された技を出せない）
#include "Tables/hoenn_script_flags.h"
#include "../include/menu.h"
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/window.h"
#include "../include/constants/songs.h"

#include "../include/new/accuracy_calc.h"
#include "../include/new/ai_util.h"
#include "../include/new/battle_indicators.h"
#include "../include/new/battle_util.h"
#include "../include/new/catching.h"
#include "../include/new/damage_calc.h"
#include "../include/new/dynamax.h"
#include "../include/new/general_bs_commands.h"
#include "../include/new/item.h"
#include "../include/new/frontier.h"
#include "../include/new/mega.h"
#include "../include/new/move_menu.h"
#include "../include/new/move_menu_battle_scripts.h"
#include "../include/new/move_tables.h"
#include "../include/new/multi.h"
#include "../include/new/set_z_effect.h"
#include "../include/new/terastal.h"
#include "../include/new/text.h"
#include "../include/new/util.h"
#include "../include/new/z_move_effects.h"
#include "shadow_moves.h" //[hack] patches/0963 段 E

u8 HackDelta_Type(const struct Pokemon* mon, u16 species, u8 slot, u8 base); //[hack] patches/0962 デルタ種のタイプ（src/hack_delta.c）
u8 HackDragonBoulder_Type2(const struct Pokemon* mon, u16 species, u8 type2); //[hack] patches/0207 りゅうのいわお（src/hack_dragon_boulder.c・特別なガブリアスZ は 2 つ目のタイプが いわ）

bool8 HoennPyramid_BagUsableInBattle(void); //[hack] patches/0131 src/hoenn_pyramid.c

/*
move_menu.c
	functions for displaying move data and z moves in the battle menu

tables:
	sTypeIconPositions
	sTypeIconPicTable

*/

//TODO: Make The Z-Move Names change colour (look in SetPpNumbersPaletteInMoveSelection)
//Fix Heal Block + Pollen Puff on partner link battle

extern const u8 gMoveNames[][MOVE_NAME_LENGTH + 1];
extern const u8 gTypeNames[][TYPE_NAME_LENGTH + 1];
extern const u8 gDynamaxMovePowers[MOVES_COUNT];
extern const u8 sTargetIdentities[];
extern const u16 gUserInterfaceGfx_TypeHighlightingPal[];
extern const u8 gMoveEffectsThatIgnoreWeaknessResistance[];
extern const u8 CamomonsTypeIconsTiles[];
extern const u8 CamomonsTypeIcons2Tiles[];
extern const u16 CamomonsTypeIconsPal[];
extern const u16 CamomonsTypeIcons2Pal[];
extern const u8 PSSIconsTiles[];

//This file's functions:
static bool8 TriggerMegaEvolution(void);
static void MoveNameToDisplayedStringBattle(u8 moveIndex);
void MoveSelectionDisplayMoveNames(void);
static void MoveSelectionDisplayMoveType(void);
static bool8 MoveSelectionDisplayZMove(void);
static bool8 MoveSelectionDisplayMaxMove(void);
static void ZMoveSelectionDisplayPpNumber(void);
static void ZMoveSelectionDisplayPower(void);
static void MaxMoveSelectionDisplayPower(void);
static void MoveSelectionDisplayDetails(void);
static void ReloadMoveNamesIfNecessary(void);
static void CloseZMoveDetails(void);
static void CloseMaxMoveDetails(void);
static void HighlightPossibleTargets(void);
static void LoadShadowColourForGreyedOutBagText(void);
static void TryLoadTypeIcons(void);
static void SpriteCB_CamomonsTypeIcon(struct Sprite* sprite);
static void TriggerTerastal(void);
u8 HackMomoIce_MegaCount(u8 bank); //[hack] patches/0183 src/mega.c（0192 で START からも使うので上へ）
static bool8 HackMomoIce_CanViewMaxMove(void); //[hack] patches/0192
bool8 HackDynamax_PlayerBlocked(u8 bank); //[hack] patches/0402 src/hack_kanto_dynamax.c

static const struct Coords16 sTypeIconPositions[][/*IS_SINGLE_BATTLE*/2] =
{
#ifndef UNBOUND //MODIFY THIS
	[B_POSITION_PLAYER_LEFT] =
	{
		[TRUE] = {221, 86}, 	//Single Battle
		[FALSE] = {144, 70},	//Double Battle
	},
	[B_POSITION_OPPONENT_LEFT] =
	{
		[TRUE] = {20, 26}, 		//Single Battle
		[FALSE] = {97, 14},		//Double Battle
	},
	[B_POSITION_PLAYER_RIGHT] =
	{
		[FALSE] = {156, 96},	//Double Battle
	},
	[B_POSITION_OPPONENT_RIGHT] =
	{
		[FALSE] = {85, 39},		//Double Battle
	},
#else //For Pokemon Unbound
	[B_POSITION_PLAYER_LEFT] =
	{
		[TRUE] = {225, 86}, 	//Single Battle
		[FALSE] = {142, 70},	//Double Battle
	},
	[B_POSITION_OPPONENT_LEFT] =
	{
		[TRUE] = {18, 26}, 		//Single Battle
		[FALSE] = {99, 15},		//Double Battle
	},
	[B_POSITION_PLAYER_RIGHT] =
	{
		[FALSE] = {154, 96},	//Double Battle
	},
	[B_POSITION_OPPONENT_RIGHT] =
	{
		[FALSE] = {87, 40},		//Double Battle
	},
#endif
};

static const struct OamData sTypeIconOAM =
{
	.affineMode = ST_OAM_AFFINE_OFF,
	.objMode = ST_OAM_OBJ_NORMAL,
	.shape = SPRITE_SHAPE(8x16),
	.size = SPRITE_SIZE(8x16),
	.priority = 1, //Same level as health bar
};

#define type_icon_frame(ptr, frame) {.data = (u8 *)ptr + (1 * 2 * frame * 32), .size = 1 * 2 * 32}
//[hack] patches/0275: 上流は長さ指定なし＝TYPE_FAIRY + 1（24）で、タイプの数（TYPE_STELLAR まで 25）より 1 つ短かった。
//ステラの絵は無いので、使わない番号と同じ「？？？」の絵にする（リル 0x19 は #110 で 26 コマ目に絵を足した）
static const struct SpriteFrameImage sTypeIconPicTable[NUMBER_OF_MON_TYPES] =
{
	[TYPE_NORMAL] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_NORMAL),
	[TYPE_FIGHTING] =	type_icon_frame(CamomonsTypeIconsTiles, TYPE_FIGHTING),
	[TYPE_FLYING] =		type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_FLYING),
	[TYPE_POISON] =		type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_POISON),
	[TYPE_GROUND] =		type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_GROUND),
	[TYPE_ROCK] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_ROCK),
	[TYPE_BUG] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_BUG),
	[TYPE_GHOST] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_GHOST),
	[TYPE_STEEL] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_STEEL),
	[TYPE_MYSTERY] =	type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[TYPE_FIRE] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_FIRE),
	[TYPE_WATER] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_WATER),
	[TYPE_GRASS] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_GRASS),
	[TYPE_ELECTRIC] =	type_icon_frame(CamomonsTypeIconsTiles, TYPE_ELECTRIC),
	[TYPE_PSYCHIC] =	type_icon_frame(CamomonsTypeIconsTiles, TYPE_PSYCHIC),
	[TYPE_ICE] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_ICE),
	[TYPE_DRAGON] =		type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_DRAGON),
	[TYPE_DARK] =		type_icon_frame(CamomonsTypeIconsTiles, TYPE_DARK),
	[0x12] =			type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[TYPE_ROOSTLESS] = 	type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[TYPE_BLANK] = 		type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[0x15] = 			type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[0x16] = 			type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[TYPE_FAIRY] = 		type_icon_frame(CamomonsTypeIconsTiles, TYPE_FAIRY),
	[TYPE_STELLAR] = 	type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_MYSTERY),
	[TYPE_RIL] = 		type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_RIL), //[hack] patches/0408 #110: リル（CamomonsTypeIcons2.bmp の 26 コマ目・パレット 6 番が藤色）
	[TYPE_SHADOW] = 	type_icon_frame(CamomonsTypeIcons2Tiles, TYPE_SHADOW), //[hack] patches/0955 段 D: ダーク（27 コマ目・パレット 7 番が暗い紫）
};

static struct SpriteTemplate sTypeIconSpriteTemplate =
{
	.tileTag = 0xFFFF,
	.paletteTag = TYPE_ICON_TAG,
	.oam = &sTypeIconOAM,
	.anims = gDummySpriteAnimTable,
	.images = sTypeIconPicTable,
	.affineAnims = gDummySpriteAffineAnimTable,
	.callback = SpriteCB_CamomonsTypeIcon,
};

static struct SpriteTemplate sTypeIconSpriteTemplate2 =
{
	.tileTag = 0xFFFF,
	.paletteTag = TYPE_ICON_TAG_2,
	.oam = &sTypeIconOAM,
	.anims = gDummySpriteAnimTable,
	.images = sTypeIconPicTable,
	.affineAnims = gDummySpriteAffineAnimTable,
	.callback = SpriteCB_CamomonsTypeIcon,
};

static const struct SpritePalette sTypeIconPalTemplate =
{
	.data = CamomonsTypeIconsPal,
	.tag = TYPE_ICON_TAG,
};

static const struct SpritePalette sTypeIconPalTemplate2 =
{
	.data = CamomonsTypeIcons2Pal,
	.tag = TYPE_ICON_TAG_2,
};

void InitMoveSelectionsVarsAndStrings(void)
{
	LoadMenuElementsPalette(6 * 0x10, 1); //For move type and split

	TryLoadMegaTriggers();
	TryLoadZTrigger();
	TryLoadDynamaxTrigger();
	TryLoadTerastalTrigger();
	TryLoadTypeIcons();
	MoveSelectionDisplayMoveNames();
	gMultiUsePlayerCursor = 0xFF;
	MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
	MoveSelectionDisplayPpString();
	MoveSelectionDisplayPpNumber();
	MoveSelectionDisplayMoveType();
	MoveSelectionDisplayMoveEffectiveness();
}

void HandleInputChooseMove(void)
{
	bool32 canSelectTarget = FALSE;
	struct ChooseMoveStruct* moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	gNewBS->zMoveData.used[gActiveBattler] = moveInfo->zMoveUsed; //So the link opponent's previous Z-selection can't be used
	gNewBS->megaData.done[gActiveBattler] = moveInfo->megaDone; //So the link opponent's previous Z-selection can't be used
	gNewBS->ultraData.done[gActiveBattler] = moveInfo->ultraDone;
	gNewBS->dynamaxData.used[gActiveBattler] = moveInfo->dynamaxDone;
	gNewBS->terastalData.done[gActiveBattler] = moveInfo->terastalDone;

	if (moveInfo->zMoveUsed)
		gNewBS->zMoveData.toBeUsed[gActiveBattler] = FALSE;

	if (moveInfo->megaDone)
		gNewBS->megaData.chosen[gActiveBattler] = FALSE;

	if (moveInfo->ultraDone)
		gNewBS->ultraData.chosen[gActiveBattler] = FALSE;

	if (moveInfo->dynamaxDone)
		gNewBS->dynamaxData.toBeUsed[gActiveBattler] = FALSE;

	if (moveInfo->terastalDone)
		gNewBS->terastalData.chosen[gActiveBattler] = FALSE;

	gNewBS->zMoveData.partyIndex[SIDE(gActiveBattler)] = moveInfo->zPartyIndex;
	gNewBS->dynamaxData.partyIndex[SIDE(gActiveBattler)] = moveInfo->dynamaxPartyIndex;
	gNewBS->terastalData.partyIndex[SIDE(gActiveBattler)] = moveInfo->terastalPartyIndex;

	HighlightPossibleTargets();

	if (gMain.newKeys & A_BUTTON)
	{
		u8 moveTarget;
		u16 chosenMove = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];

		PlaySE(SE_SELECT);
		if (MoveIs(chosenMove, MOVE_CURSE))
		{
			if (moveInfo->monType1 != TYPE_GHOST && moveInfo->monType2 != TYPE_GHOST && moveInfo->monType3 != TYPE_GHOST)
				moveTarget = MOVE_TARGET_USER;
			else
				moveTarget = MOVE_TARGET_SELECTED;
		}
		else if (MoveIs(chosenMove, MOVE_ACUPRESSURE) && !(IS_DOUBLE_BATTLE))
			moveTarget = MOVE_TARGET_USER; //Only can target yourself in singles
		else
			moveTarget = GetBaseMoveTarget(chosenMove, gActiveBattler);

		if (gNewBS->zMoveData.viewing && moveInfo->moveSplit[gMoveSelectionCursor[gActiveBattler]] != SPLIT_STATUS) //Status moves keep original targets
			moveTarget = gBattleMoves[moveInfo->possibleZMoves[gMoveSelectionCursor[gActiveBattler]]].target;

		if (gNewBS->dynamaxData.viewing || moveInfo->dynamaxed)
			moveTarget = gBattleMoves[moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]]].target;

		if (moveTarget & MOVE_TARGET_USER)
			gMultiUsePlayerCursor = gActiveBattler;
		else
			gMultiUsePlayerCursor = GetBattlerAtPosition((GetBattlerPosition(gActiveBattler) & BIT_SIDE) ^ BIT_SIDE);

		if (gBattleBufferA[gActiveBattler][1]) // double battle
		{
			if (!(moveTarget & (MOVE_TARGET_RANDOM | MOVE_TARGET_BOTH | MOVE_TARGET_DEPENDS | MOVE_TARGET_FOES_AND_ALLY | MOVE_TARGET_OPPONENTS_FIELD | MOVE_TARGET_USER)))
			{
				if (!(gNewBS->dynamaxData.viewing || moveInfo->dynamaxed) //Not a max move
				|| CountAliveMonsInBattle(BATTLE_ALIVE_DEF_SIDE, gActiveBattler, FOE(gActiveBattler)) > 1) //At least two targets
					canSelectTarget++; // either selected or user
			}

			if (moveInfo->currentPp[gMoveSelectionCursor[gActiveBattler]] == 0)
			{
				canSelectTarget = FALSE;
			}
			else if (!(moveTarget & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_PARTNER)) && CountAliveMons(BATTLE_ALIVE_EXCEPT_ACTIVE) <= 1)
			{
				gMultiUsePlayerCursor = GetDefaultMoveTarget(gActiveBattler); //0x803F6A0
				canSelectTarget = FALSE;
			}
		}

		ResetPaletteFadeControl();
		BeginNormalPaletteFade(0xF0000, 0, 0, 0, 0x7FFF);

		if (!canSelectTarget)
		{
			CloseZMoveDetails();
			CloseMaxMoveDetails();

			if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
			{
				//Will be set by emit, but needed in multi battles so the partner chooses the best move for the job
				gBattleStruct->chosenMovePositions[gActiveBattler] = gMoveSelectionCursor[gActiveBattler];
				gBattleStruct->moveTarget[gActiveBattler] =gMultiUsePlayerCursor;
				gChosenMovesByBanks[gActiveBattler] = chosenMove;

				//Wipe doubles killing score
				TryRemovePartnerDoublesKillingScoreComplete(gActiveBattler, gMultiUsePlayerCursor, chosenMove, moveTarget, FALSE);
			}

			EmitMoveChosen(1, gMoveSelectionCursor[gActiveBattler], gMultiUsePlayerCursor, gNewBS->megaData.chosen[gActiveBattler], gNewBS->ultraData.chosen[gActiveBattler], gNewBS->zMoveData.toBeUsed[gActiveBattler], gNewBS->dynamaxData.toBeUsed[gActiveBattler], gNewBS->terastalData.chosen[gActiveBattler]);
			PlayerBufferExecCompleted();
		}
		else
		{
			gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseTarget;

			if (moveTarget & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_PARTNER))
				gMultiUsePlayerCursor = gActiveBattler;
			else if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)])
				gMultiUsePlayerCursor = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
			else
				gMultiUsePlayerCursor = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

			gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
			MoveSelectionDisplayMoveEffectiveness();
		}
	}
	else if (gMain.newKeys & B_BUTTON)
	{
		gNewBS->zMoveData.viewing = FALSE;
		gNewBS->zMoveData.viewingDetails = FALSE;
		gNewBS->dynamaxData.viewing = FALSE;
		gMoveSelectionCursor[gActiveBattler] = 0;
		PlaySE(SE_SELECT);
		EmitTwoReturnValues(1, 10, 0xFFFF);
		PlayerBufferExecCompleted();
		ResetPaletteFadeControl();
		BeginNormalPaletteFade(0xF0000, 0, 0, 0, 0x7FFF);
	}
	else if (gMain.newKeys & DPAD_LEFT)
	{
		if (gMoveSelectionCursor[gActiveBattler] & 1)
		{
			ReloadMoveNamesIfNecessary();
			MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]); //0x8030A14
			gMoveSelectionCursor[gActiveBattler] ^= 1;
			PlaySE(SE_SELECT);
			MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0); //0x80309CC
			MoveSelectionDisplayPpNumber(); //0x80309CC
			MoveSelectionDisplayMoveType();
			MoveSelectionDisplayMoveEffectiveness();
			BeginNormalPaletteFade(0xF0000, 0, 0, 0, 0x7FFF);
		}
	}
	else if (gMain.newKeys & DPAD_RIGHT)
	{
		if (!(gMoveSelectionCursor[gActiveBattler] & 1)
		 && (gMoveSelectionCursor[gActiveBattler] ^ 1) < gNumberOfMovesToChoose)
		{
			ReloadMoveNamesIfNecessary();
			MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
			gMoveSelectionCursor[gActiveBattler] ^= 1;
			PlaySE(SE_SELECT);
			MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
			MoveSelectionDisplayPpNumber();
			MoveSelectionDisplayMoveType();
			MoveSelectionDisplayMoveEffectiveness();
			BeginNormalPaletteFade(0xF0000, 0, 0, 0, 0x7FFF);
		}
	}
	else if (gMain.newKeys & DPAD_UP)
	{
		if (gMoveSelectionCursor[gActiveBattler] & 2)
		{
			ReloadMoveNamesIfNecessary();
			MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
			gMoveSelectionCursor[gActiveBattler] ^= 2;
			PlaySE(SE_SELECT);
			MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
			MoveSelectionDisplayPpNumber();
			MoveSelectionDisplayMoveType();
			MoveSelectionDisplayMoveEffectiveness();
			BeginNormalPaletteFade(0xF0000, 0, 0, 0, 0x7FFF);
		}
	}
	else if (gMain.newKeys & DPAD_DOWN)
	{
		if (!(gMoveSelectionCursor[gActiveBattler] & 2)
		 && (gMoveSelectionCursor[gActiveBattler] ^ 2) < gNumberOfMovesToChoose)
		{
			ReloadMoveNamesIfNecessary();
			MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
			gMoveSelectionCursor[gActiveBattler] ^= 2;
			PlaySE(SE_SELECT);
			MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
			MoveSelectionDisplayPpNumber();
			MoveSelectionDisplayMoveType();
			MoveSelectionDisplayMoveEffectiveness();
			BeginNormalPaletteFade(0xF0000, 0, 0, 0, 0x7FFF);
		}
	}
	else if (gMain.newKeys & SELECT_BUTTON)
	{
		if (gNumberOfMovesToChoose > 1 && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_CAMOMONS))
		&& gDisableStructs[gActiveBattler].encoreTimer == 0 //Breaks the Encore otherwise
		&& !gNewBS->dynamaxData.viewing
		&& !gNewBS->zMoveData.viewing
		&& !gNewBS->zMoveData.viewingDetails
		&& !Shadow_IsBattlerDark(gActiveBattler)) //[hack] patches/0963 段 E ダークの子の技の並びは組み立てた物なので 並べ替えない
		{
			MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 29);

			if (gMoveSelectionCursor[gActiveBattler] != 0)
				gMultiUsePlayerCursor = 0;
			else
				gMultiUsePlayerCursor = gMoveSelectionCursor[gActiveBattler] + 1;

			MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 27);
			BattlePutTextOnWindow(gText_BattleSwitchWhich, 0xB); //0x80D87BC
			gBattlerControllerFuncs[gActiveBattler] = HandleMoveSwitching; //0x802EF58
		}
	}
	else if (gMain.newKeys & START_BUTTON)
	{
		if (!MoveSelectionDisplayZMove()) //Only one is allowed at a time
		{
			//[hack] patches/0192: ももアイスは START でダイマックス（メガシンカは R・2026-10-06 ユーザー「ももアイスがメガストーン扱いになっている
			//メガ進化できるダイマックスできる両方対応」）。ダイマックスを見るときはメガとテラスタルの選択を外す
			if (HackMomoIce_MegaCount(gActiveBattler) != 0 && (gBattleTypeFlags & BATTLE_TYPE_DYNAMAX) && !HackDynamax_PlayerBlocked(gActiveBattler) //[hack] patches/0402
			&& HackMomoIce_CanViewMaxMove())
			{
				gNewBS->megaData.chosen[gActiveBattler] = 0;
				gNewBS->terastalData.chosen[gActiveBattler] = FALSE;
				MoveSelectionDisplayMoveType(); //メガのタイプの表示を技のタイプに戻す
				MoveSelectionDisplayMaxMove();
			}
			else if (!TriggerMegaEvolution() && gBattleTypeFlags & BATTLE_TYPE_DYNAMAX && !HackDynamax_PlayerBlocked(gActiveBattler)) //[hack] patches/0402
				MoveSelectionDisplayMaxMove();
		}
	}
	else if (gMain.newKeys & R_BUTTON)
	{
		if (!MoveSelectionDisplayZMove())
		{
			if (!TriggerMegaEvolution())
				TriggerTerastal();
		}
	}
	else if (gMain.newKeys & L_BUTTON)
	{
		if (!gNewBS->zMoveData.viewing && !gNewBS->dynamaxData.viewing)
			MoveSelectionDisplayDetails();
	}
}


//[hack] patches/0192: ダイマックスの技を見られるか（今のカーソルの技にダイマックス技がある）
static bool8 HackMomoIce_CanViewMaxMove(void)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	return moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]] != MOVE_NONE;
}

//[hack] patches/0183: ももアイスで選んだメガのタイプを、右の枠に技の枠と同じタイプのアイコン（32x12）で出す（メガの名前は元と同じなので X／Y が見分けられない）
//[hack] patches/0930: 右の枠は 効き目の言葉（1 画素上げる）・PP の行（2 画素上げる）・アイコン 1〜2 つ。2 つ並べて重ならずに収まる（アイコンは y=22 と 34・枠の内側は y=45 まで）
static void MsDrawPills(u8 win, u8 x, u8 y0, u8 type0, u8 y1, u8 type1);
static void MsDrawPanelBase(u8 win);
static void MsDrawPpRow(u8 win, u8 y, bool8 zView);
static void MsEffectivenessImpl(u8 dy);
static void HackMomoIce_ShowMegaTypes(void)
{
	const struct Evolution* evo = CanMegaEvolve(gActiveBattler, FALSE);
	u8 type1, type2;

	if (evo == NULL)
		return;

	type1 = HackDelta_Type(GetBankPartyData(gActiveBattler), evo->targetSpecies, 0, gBaseStats[evo->targetSpecies].type1); //[hack] patches/0962 デルタ種
	type2 = HackDelta_Type(GetBankPartyData(gActiveBattler), evo->targetSpecies, 1, HackDragonBoulder_Type2(GetBankPartyData(gActiveBattler), evo->targetSpecies, gBaseStats[evo->targetSpecies].type2)); //[hack] patches/0207 りゅうのいわお・patches/0962 デルタ種
	MsDrawPanelBase(7);
	MsDrawPpRow(7, 10, FALSE);
	MsEffectivenessImpl(1);
	if (type2 != type1)
		MsDrawPills(7, 16, 22, type1, 34, type2);
	else
		MsDrawPills(7, 16, 28, type1, 0, 0xFF);
	CopyWindowToVram(7, COPYWIN_BOTH);
	PutWindowTilemap(7);
}

static bool8 TriggerMegaEvolution(void)
{
	#ifndef MEGA_EVOLUTION_FEATURE
		return FALSE;
	#else
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	if (!moveInfo->canMegaEvolve)
		return FALSE;

	if (moveInfo->megaVariance == MEGA_VARIANT_ULTRA_BURST) //Ultra Burst
	{
		if (gNewBS->ultraData.chosen[gActiveBattler])
		{ // Turn Off
			PlaySE(3);
			gNewBS->ultraData.chosen[gActiveBattler] = FALSE;
			return TRUE;
		}
		else
		{ // Turn On
			PlaySE(2);
			gNewBS->ultraData.chosen[gActiveBattler] = TRUE;
			return TRUE;
		}
	}
	else //Regular Megas
	{
		u8 momoCount = HackMomoIce_MegaCount(gActiveBattler); //[hack] patches/0183
		if (momoCount != 0)
		{
			u8 next;

			//[hack] patches/0192: R は なし → メガ 1 → （メガ 2）→ テラスタル → なし。ダイマックスを見ていたら閉じる
			if (gNewBS->dynamaxData.viewing)
				MoveSelectionDisplayMaxMove();
			if (gNewBS->terastalData.chosen[gActiveBattler])
			{
				TriggerTerastal(); //テラスタル → なし（外す音と技のタイプの枠を戻す）
				return TRUE;
			}
			next = gNewBS->megaData.chosen[gActiveBattler] + 1; //0 → 1 → … → 種類の数 → テラスタル
			if (next > momoCount)
			{
				gNewBS->megaData.chosen[gActiveBattler] = 0;
				MoveSelectionDisplayMoveType();
				if (moveInfo->canterastal)
					return FALSE; //呼び元が TriggerTerastal でテラスタルを選ぶ
				PlaySE(3);
				return TRUE;
			}
			gNewBS->megaData.chosen[gActiveBattler] = next;
			if (next != 0)
			{
				PlaySE(2);
				HackMomoIce_ShowMegaTypes();
			}
			else
			{
				PlaySE(3);
				MoveSelectionDisplayMoveType();
			}
			return TRUE;
		}

		if (gNewBS->megaData.chosen[gActiveBattler])
		{
			// Turn Off
			PlaySE(3);
			gNewBS->megaData.chosen[gActiveBattler] = 0;
			return TRUE;
		}
		else
		{
			// Turn On
			PlaySE(2);
			gNewBS->megaData.chosen[gActiveBattler] = 1;
			return TRUE;
		}
	}
	return FALSE;
	#endif
}

void TriggerTerastal(void)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	if (moveInfo->canterastal)
	{
		if (gNewBS->terastalData.chosen[gActiveBattler])
		{ // Turn Off
			PlaySE(3);
			gNewBS->terastalData.chosen[gActiveBattler] = FALSE;
			MoveSelectionDisplayMoveType();
			MoveSelectionDisplayMoveEffectiveness();
		}
		else
		{ // Turn On
			if (gNewBS->dynamaxData.viewing)
				ReloadMoveNamesIfNecessary();

			PlaySE(2);
			gNewBS->terastalData.chosen[gActiveBattler] = TRUE;
			MoveSelectionDisplayMoveType();
			MoveSelectionDisplayMoveEffectiveness();
		}
	}
}

//This function sends useful data over Link Cable for the move menu to use
void EmitChooseMove(u8 bufferId, bool8 isDoubleBattle, bool8 NoPpNumber, struct ChooseMoveStruct *movePpData)
{
	u32 i, j;
	const struct Evolution* evolutions;

	struct ChooseMoveStruct* tempMoveStruct = Calloc(sizeof(struct ChooseMoveStruct)); //Make space for new expanded data
	Memcpy(tempMoveStruct, movePpData, sizeof(struct ChooseMoveStructOld)); //Copy the old data
	tempMoveStruct->monType1 = gBattleMons[gActiveBattler].type1;
	tempMoveStruct->monType2 = gBattleMons[gActiveBattler].type2;
	tempMoveStruct->monType3 = gBattleMons[gActiveBattler].type3;
	tempMoveStruct->monTeraType = GetTeraType(gActiveBattler);
	tempMoveStruct->ability = ABILITY(gActiveBattler);
	tempMoveStruct->atkIsGrounded = CheckGrounding(gActiveBattler);

	//Fix Transformed Move PP
	if (IS_TRANSFORMED(gActiveBattler))
	{
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			if (tempMoveStruct->maxPp[i] > 5)
				tempMoveStruct->maxPp[i] = 5;
		}
	}

	#ifdef DYNAMAX_FEATURE
	tempMoveStruct->dynamaxed = IsDynamaxed(gActiveBattler);
	tempMoveStruct->dynamaxDone = gNewBS->dynamaxData.used[gActiveBattler];
	tempMoveStruct->dynamaxPartyIndex = gNewBS->dynamaxData.partyIndex[SIDE(gActiveBattler)];
	if ((!gNewBS->dynamaxData.used[gActiveBattler] || IsDynamaxed(gActiveBattler))
	&& !(IS_DOUBLE_BATTLE && BATTLER_ALIVE(PARTNER(gActiveBattler)) && gNewBS->dynamaxData.toBeUsed[PARTNER(gActiveBattler)]) //Partner didn't already select it
	&& DynamaxEnabled(gActiveBattler)
	&& MonCanDynamax(GetBankPartyData(gActiveBattler))
	&& !BATTLER_SEMI_INVULNERABLE(gActiveBattler)
	&& !DoesTerastalUsageStopMegaEvolution(gActiveBattler))
	{
		for (i = 0; i < MAX_MON_MOVES; ++i)
			tempMoveStruct->possibleMaxMoves[i] = GetMaxMove(gActiveBattler, i);
	}
	#endif


	#ifdef TERASTAL_FEATURE
	tempMoveStruct->terastalDone = gNewBS->terastalData.done[gActiveBattler];
	tempMoveStruct->terastalPartyIndex = gNewBS->terastalData.partyIndex[SIDE(gActiveBattler)];
	if (!gNewBS->terastalData.done[gActiveBattler]
	&& !(IS_DOUBLE_BATTLE && BATTLER_ALIVE(PARTNER(gActiveBattler)) && gNewBS->terastalData.chosen[PARTNER(gActiveBattler)]))
		{
			if (!BATTLER_SEMI_INVULNERABLE(gActiveBattler)
			&& TerastalEnabled(gActiveBattler)
			&& MonCanTerastal(GetBankPartyData(gActiveBattler))
			&& !DoesDynamaxUsageStopMegaEvolution(gActiveBattler))
			{
				tempMoveStruct->canterastal = TRUE;
			}
		}
	#endif


	tempMoveStruct->megaDone = gNewBS->megaData.done[gActiveBattler];
	tempMoveStruct->ultraDone = gNewBS->ultraData.done[gActiveBattler];
	if (!IS_TRANSFORMED(gActiveBattler) && !tempMoveStruct->dynamaxed)
	{
		if (!gNewBS->megaData.done[gActiveBattler])
		{
			evolutions = CanMegaEvolve(gActiveBattler, FALSE);
			if (evolutions != NULL)
			{
				if (!BankMegaEvolved(gActiveBattler, FALSE)
				&& MegaEvolutionEnabled(gActiveBattler)
				&& !BATTLER_SEMI_INVULNERABLE(gActiveBattler) //No Mega Evolving while not on screen
				&& !DoesZMoveUsageStopMegaEvolution(gActiveBattler) //No Mega Evolving if you've used a Z-Move (*cough* *cough* Rayquaza)
				&& !DoesDynamaxUsageStopMegaEvolution(gActiveBattler)
				&& !DoesTerastalUsageStopMegaEvolution(gActiveBattler))
				{
					tempMoveStruct->canMegaEvolve = TRUE;
					tempMoveStruct->megaVariance = evolutions->unknown;
				}
			}
		}

		if (!gNewBS->ultraData.done[gActiveBattler])
		{
			evolutions = CanMegaEvolve(gActiveBattler, TRUE); //Check Ultra Burst
			if (evolutions != NULL)
			{
				if (!BankMegaEvolved(gActiveBattler, TRUE) //Check Ultra Burst
				&& !BATTLER_SEMI_INVULNERABLE(gActiveBattler) //No Ultra Bursting while not on screen
				&& !DoesDynamaxUsageStopMegaEvolution(gActiveBattler)
				&& !DoesTerastalUsageStopMegaEvolution(gActiveBattler))
				{
					tempMoveStruct->canMegaEvolve = TRUE;
					tempMoveStruct->megaVariance = evolutions->unknown;
				}
			}
		}
	}


	tempMoveStruct->zMoveUsed = gNewBS->zMoveData.used[gActiveBattler];
	tempMoveStruct->zPartyIndex = gNewBS->zMoveData.partyIndex[SIDE(gActiveBattler)];
	if (!gNewBS->zMoveData.used[gActiveBattler] && !IsMegaZMoveBannedBattle() && !IsMega(gActiveBattler) && !IsBluePrimal(gActiveBattler) && !IsRedPrimal(gActiveBattler))
	{
		u8 limitations = CheckMoveLimitations(gActiveBattler, 0, MOVE_LIMITATION_PP);

		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			if (!(limitations & gBitTable[i])) //Don't display a Z-Move if the base move has no PP
				tempMoveStruct->possibleZMoves[i] = CanUseZMove(gActiveBattler, i, 0);
		}
	}

	gBattleScripting.dmgMultiplier = 1;
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u8 foe = (IS_DOUBLE_BATTLE && !BATTLER_ALIVE(FOE(gActiveBattler))) ? PARTNER(FOE(gActiveBattler)) : FOE(gActiveBattler);
		u16 originalMove = gBattleMons[gActiveBattler].moves[i];
		u16 move = (tempMoveStruct->dynamaxed) ? tempMoveStruct->possibleMaxMoves[i] : originalMove;

		tempMoveStruct->moveTypes[i] = GetMoveTypeSpecial(gActiveBattler, move);

		if (IS_DOUBLE_BATTLE && CountAliveMonsInBattle(BATTLE_ALIVE_DEF_SIDE, gActiveBattler, foe) >= 2) //Because target can vary, display only attacker's modifiers
		{
			tempMoveStruct->movePowers[i] = CalcVisualBasePower(gActiveBattler, gActiveBattler, move, TRUE);
			tempMoveStruct->moveAcc[i] = VisualAccuracyCalc_NoTarget(move, gActiveBattler);
			tempMoveStruct->moveSplit[i] = CalcMoveSplit(gActiveBattler, move, gActiveBattler);
			tempMoveStruct->makesContact[i] = CheckContact(move, gActiveBattler, gActiveBattler);

			if (tempMoveStruct->possibleMaxMoves[i] != MOVE_NONE)
			{
				gNewBS->ai.zMoveHelper = originalMove;
				tempMoveStruct->maxMovePowers[i] = CalcVisualBasePower(gActiveBattler, gActiveBattler, tempMoveStruct->possibleMaxMoves[i], TRUE);
				gNewBS->ai.zMoveHelper = MOVE_NONE;
			}

			for (j = 0; j < gBattlersCount; ++j)
			{
				u8 foePosition = GetBattlerPosition(j);

				if (j == gActiveBattler || j == PARTNER(gActiveBattler))
				{
					tempMoveStruct->moveResults[foePosition][i] = tempMoveStruct->zMoveResults[foePosition][i] = 0;
					continue;
				}

				u8 moveResult = VisualTypeCalc(move, gActiveBattler, j);
				tempMoveStruct->moveResults[foePosition][i] = tempMoveStruct->zMoveResults[foePosition][i] = moveResult;

				//All Z-Moves have the same type as their base move except for these two moves
				if ((move == MOVE_FREEZEDRY || MOVE_SECOND_TYPE(move) != 0xFF) //[hack] patches/0630 (#142)
				&& tempMoveStruct->possibleZMoves[i] != MOVE_NONE)
					tempMoveStruct->zMoveResults[foePosition][i] =  VisualTypeCalc(tempMoveStruct->zMoveResults[foePosition][i], gActiveBattler, j);
			}
		}
		else //Single Battle or single target
		{
			u8 foePosition = GetBattlerPosition(foe);
			tempMoveStruct->movePowers[i] = CalcVisualBasePower(gActiveBattler, foe, move, FALSE);
			tempMoveStruct->moveAcc[i] = VisualAccuracyCalc(move, gActiveBattler, foe);
			tempMoveStruct->moveSplit[i] = CalcMoveSplit(gActiveBattler, move, foe);
			tempMoveStruct->makesContact[i] = CheckContact(move, gActiveBattler, foe);

			if (tempMoveStruct->possibleMaxMoves[i] != MOVE_NONE)
			{
				gNewBS->ai.zMoveHelper = originalMove;
				tempMoveStruct->maxMovePowers[i] = CalcVisualBasePower(gActiveBattler, foe, tempMoveStruct->possibleMaxMoves[i], FALSE);
				gNewBS->ai.zMoveHelper = MOVE_NONE;
			}

			u8 moveResult = VisualTypeCalc(move, gActiveBattler, foe);
			tempMoveStruct->moveResults[foePosition][i] = tempMoveStruct->zMoveResults[foePosition][i] = moveResult;

			//All Z-Moves have the same type as their base move except for these two moves
			if ((move == MOVE_FREEZEDRY || MOVE_SECOND_TYPE(move) != 0xFF) //[hack] patches/0630 (#142)
			&& tempMoveStruct->possibleZMoves[i] != MOVE_NONE)
				tempMoveStruct->zMoveResults[foePosition][i] = VisualTypeCalc(tempMoveStruct->zMoveResults[foePosition][i], gActiveBattler, foe);
		}
	}

	gBattleBuffersTransferData[0] = CONTROLLER_CHOOSEMOVE;
	gBattleBuffersTransferData[1] = isDoubleBattle;
	gBattleBuffersTransferData[2] = NoPpNumber;
	gBattleBuffersTransferData[3] = 0;
	for (i = 0; i < sizeof(*tempMoveStruct); ++i)
		gBattleBuffersTransferData[4 + i] = ((u8*)(tempMoveStruct))[i];
	PrepareBufferDataTransfer(bufferId, gBattleBuffersTransferData, sizeof(*tempMoveStruct) + 4);

	Free(tempMoveStruct);
}

void EmitMoveChosen(u8 bufferId, u8 chosenMoveIndex, u8 target, u8 megaState, u8 ultraState, u8 zMoveState, u8 dynamaxState, u8 terastalState)
{
	gBattleBuffersTransferData[0] = CONTROLLER_TWORETURNVALUES;
	gBattleBuffersTransferData[1] = ACTION_RUN_BATTLESCRIPT;
	gBattleBuffersTransferData[2] = chosenMoveIndex;
	gBattleBuffersTransferData[3] = target;
	gBattleBuffersTransferData[4] = megaState;
	gBattleBuffersTransferData[5] = ultraState;
	gBattleBuffersTransferData[6] = zMoveState;
	gBattleBuffersTransferData[7] = dynamaxState;
	gBattleBuffersTransferData[8] = terastalState;
	PrepareBufferDataTransfer(bufferId, gBattleBuffersTransferData, 9);
}

const u8 sText_StabMoveInterfaceType[] = {0xFC, 0x05, 0x05, 0xFC, 0x04, 0x09, 0x0E, 0x08, 0xFF};

static void MoveNameToDisplayedStringBattle(u8 moveIndex)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);

	if (moveInfo->dynamaxed && moveInfo->moves[moveIndex] != MOVE_NONE)
	{
		if (IsGMaxMove(moveInfo->possibleMaxMoves[moveIndex]))
		{
			gDisplayedStringBattle[0] = PC_KI; //Short for Max
			gDisplayedStringBattle[1] = PC_XYO;
			gDisplayedStringBattle[2] = PC_DA;
			gDisplayedStringBattle[3] = PC_II;
			StringCopy(gDisplayedStringBattle + 4, gMoveNames[moveInfo->possibleMaxMoves[moveIndex]]);
		}	
		else
		{
			gDisplayedStringBattle[0] = PC_DA; //Short for G-Max
			gDisplayedStringBattle[1] = PC_II;
			StringCopy(gDisplayedStringBattle + 2, gMoveNames[moveInfo->possibleMaxMoves[moveIndex]]);
		}	
	}
	else
		StringCopy(gDisplayedStringBattle, gMoveNames[moveInfo->moves[moveIndex]]);
}

//[hack] patches/0930 (FEAT-move-select-ui・Redmine #201): 戦闘の技を選ぶ画面を「技ごとのタイプ色の枠＋タイプの札＋ぶつり／とくしゅ／へんかの絵＋技名、右の枠に PP・PW（威力）・HIT（命中）」の形にする。
//  窓（bytereplacement の 0x0820520C の表）: 3〜6＝技の枠 4 つ（11x3 タイル・パレット 10〜13・技のタイプ色に毎回塗る）・7／11＝右の枠（8x6・パレット 14）・8〜10＝使わない（1x1）。
//  BG のパレット 10〜15 は戦闘中だれも使っていない（mGBA で 0 を確認）。カーソルは枠の線（選ぶ=濃い赤＋白・入れ替えでつかんだ技=だいだい・相手側=白＋赤）。
//  窓 3〜6 の文字の色番号は上流の表のまま（13＝文字・14＝地・15＝かげ）なので、Z 技・ダイマックス技・詳細（L）の文章も枠の色の上にそのまま出る。
#define MS_FONT 0 //技名・札・数字の小さい文字
#define MS_CELL_W 88
#define MS_CELL_H 24
#define MS_PANEL_W 64
#define MS_PANEL_H 48
#define MS_PANEL_WIN 7
#define MS_PANEL_PAL 14

//技の枠の地の色（2026-10-28 ユーザー: タイプはアイコンで分かるので背景色はつけない＝右の枠と同じ窓の地）
#define MS_CELL_GROUND RGB(30, 28, 24)

//右の枠のパレット（0 透明・1 白・2 枠・3 PP 少ない・4 PP 0・6/7 STAB・8/9 相性・5/10/11/12 札（タイプのアイコン）の絵の色・13 文字・14 地・15 かげ）
static const u16 sMsPanelPal[16] =
{
	0, RGB(31, 31, 31), RGB(18, 7, 6), RGB(30, 17, 0), RGB(27, 4, 4), 0, RGB(27, 27, 27), RGB(4, 4, 4),
	RGB(26, 26, 25), RGB(5, 5, 5), 0, RGB(22, 22, 22), RGB(8, 8, 8), RGB(5, 5, 6), RGB(30, 28, 24), RGB(21, 20, 19),
};

//ぶつり／とくしゅ／へんかの絵（PSSIcons・色番号 1 3 10 13 15）を技の枠のパレット番号 2 3 4 5 1 に付け替える
static const u8 sMsSplitMap[16] = {14, 2, 14, 3, 14, 14, 14, 14, 14, 14, 4, 14, 14, 5, 14, 1};

//右の枠は 4 行（効き目の言葉・PP・いりょく・めいちゅう）。2026-10-28 ユーザー決定（FEAT-move-select-panel-a・A 案）
//  文字の箱は透明（段の間隔が文字の高さより狭く、前の段の文字や枠の線を消さないため）
#define MS_ROW0_Y 2 //効き目の言葉の段
#define MS_ROW1_Y 12
#define MS_ROW2_Y 23
#define MS_ROW3_Y 33
#define MS_PANEL_RIGHT 62
#define MS_TXT_BG 0
#define MS_LS 0xFF //字間 -1（めいちゅう と 100 が重ならないように）
#define MS_LABEL_X 3
#define MS_COL_BLACK 13 //黒い文字・縁取りの色（右の枠のパレット 13）

static const u8 sMsTextPW[] = {0x02, 0x28, 0x36, 0x08, 0xFF};        //いりょく
static const u8 sMsTextHIT[] = {0x22, 0x02, 0x11, 0x35, 0x03, 0xFF}; //めいちゅう
static const u8 sMsTextPP[] = {0xCA, 0xCA, 0xFF};                    //PP
static const u8 sMsTextSE[] = {0x46, 0x12, 0x39, 0x2E, 0xFF};        //ばつぐん
static const u8 sMsTextNVE[] = {0x02, 0x1F, 0x1B, 0x14, 0x12, 0xFF}; //いまひとつ
static const u8 sMsTextNO[] = {0x0A, 0x03, 0x06, 0x15, 0x0C, 0xFF};  //こうかなし
static const u8 sMsTextNORMAL[] = {0x1C, 0x12, 0x03, 0xFF};          //ふつう

#define SUPER_EFFECTIVE_COLOURS 0
#define NOT_VERY_EFFECTIVE_COLOURS 4
#define NO_EFFECT_COLOURS 8
#define REGULAR_COLOURS 12
//[hack] patches/0930: 2タイプの技（patches/0630）の 2 つ目は「効果でそのタイプも持つ」だけなので、技を選ぶ画面は元の技のタイプ（1 つ目）だけを出す
//  （2026-10-27 ユーザー決定。patches/0641 の交互表示は取りやめ。battle.h の hackDualKey／Phase／Timer は欄の位置を保つため残す・使わない）
#define SUPER_EFFECTIVE_COLOURS 0
#define NOT_VERY_EFFECTIVE_COLOURS 4
#define NO_EFFECT_COLOURS 8
#define REGULAR_COLOURS 12

static u8 MsSlotType(const struct ChooseMoveStruct* moveInfo, u8 slot)
{
	u16 move = moveInfo->moves[slot];

	#ifdef DISPLAY_REAL_MOVE_TYPE_ON_MENU
	if ((move == MOVE_TERABLAST || move == MOVE_TERASTARSTORM) && gNewBS->terastalData.chosen[gActiveBattler])
		return moveInfo->monTeraType;
	return moveInfo->moveTypes[slot];
	#else
	if (!moveInfo->dynamaxed)
		return gBattleMoves[move].type;
	return gBattleMoves[moveInfo->possibleMaxMoves[slot]].type;
	#endif
}

static void MsLoadCellPalette(u8 slot, u8 type, bool8 empty)
{
	u16 pal[16];
	u16 bg = empty ? RGB(9, 9, 10) : MS_CELL_GROUND; //type は使わない（呼び元を変えないため残す）
	(void)type;
	u32 r = bg & 31, g = (bg >> 5) & 31, b = (bg >> 10) & 31;
	bool8 dark = (r * 3 + g * 6 + b) < 135;
	u32 i;

	for (i = 0; i < 16; ++i)
		pal[i] = 0;
	pal[1] = RGB(31, 31, 31);
	pal[2] = RGB(29, 6, 0);
	pal[3] = RGB(31, 24, 6);
	pal[4] = RGB(27, 27, 24);
	pal[5] = RGB(10, 15, 17);
	pal[6] = RGB(18, 7, 6);
	pal[7] = RGB(31, 20, 0);
	pal[11] = RGB(22, 22, 22);
	pal[12] = RGB(8, 8, 8);
	pal[14] = bg;
	if (dark)
	{
		pal[13] = RGB(31, 31, 31);
		pal[15] = RGB(6, 6, 7);
	}
	else
	{
		pal[13] = RGB(3, 3, 4);
		pal[15] = RGB((r + 31) / 2, (g + 31) / 2, (b + 31) / 2);
	}
	LoadPalette(pal, (10 + slot) * 16, 32);
}

//タイプの札: 今のゲームにあるタイプアイコン（Type_Icons・32x12）をそのまま使う。絵の色の番号は技の枠のパレット（地が 14・文字が 13）に合わせて付け替える
//  0（四隅）→14（枠の地）・15（白）→1・14（暗い縁）→12・残りの色（絵ごとに 2 色まで）→extraA／extraB（技の枠は 8・9／右の枠は 10・5）
struct MsInfoIcon { u8 width; u8 height; u16 offset; };
extern const struct MsInfoIcon gMoveMenuInfoIcons[]; //assembly/data/type_tables.s
extern const u8 Type_IconsTiles[]; //graphics/Other/Type_Icons
static const u16 sMsIconColour[16] = //Type_Icons.bmp のパレット
{
	RGB(15, 19, 16), RGB(30, 10, 6), RGB(20, 10, 7), RGB(31, 24, 6), RGB(27, 28, 6), RGB(15, 25, 10), RGB(19, 27, 27), RGB(13, 18, 30),
	RGB(20, 8, 20), RGB(30, 22, 30), RGB(24, 24, 24), RGB(23, 20, 7), RGB(31, 11, 17), RGB(10, 15, 17), RGB(8, 8, 8), RGB(31, 31, 31),
};

//札（タイプのアイコン）を 1 つか 2 つ描く（type1 が 0xFF なら 1 つ）。2 つのとき色の番号の割り当てを共有する（右の枠は 1 つのパレットなので、別々に割り当てると 2 つ目が 1 つ目の色を上書きする）
//  技の枠: 絵の色 2 つを 8・9・暗い縁を 12 / 右の枠: 絵の色を 10・5・11・12（最大 4 色）・暗い縁は 13（黒い文字の色）
static void MsDrawPills(u8 win, u8 x, u8 y0, u8 type0, u8 y1, u8 type1)
{
	u8 buf[8 * 32];
	u8 map[16];
	u8 palBase = (win == MS_PANEL_WIN) ? MS_PANEL_PAL * 16 : (10 + (win - 3)) * 16;
	u8 pool[4] = {8, 9, 0, 0};
	u8 npool = 2;
	u8 dark = 12;
	u8 nextra = 0;
	u32 tx, ty, i, k;

	if (win == MS_PANEL_WIN)
	{
		pool[0] = 10;
		pool[1] = 5;
		pool[2] = 11;
		pool[3] = 12;
		npool = 4;
		dark = MS_COL_BLACK;
	}
	for (i = 0; i < 16; ++i)
		map[i] = 0xFF;
	map[0] = 14;
	map[15] = 1;
	map[14] = dark;
	for (k = 0; k < 2; ++k)
	{
		u8 type = k ? type1 : type0;
		const struct MsInfoIcon* icon;

		if (type == 0xFF)
			break;
		if (type > TYPE_SHADOW || type == TYPE_STELLAR)
			type = TYPE_MYSTERY;
		icon = &gMoveMenuInfoIcons[type + 1];
		for (ty = 0; ty < 2; ++ty)
		{
			for (tx = 0; tx < 4; ++tx)
			{
				const u8* src = Type_IconsTiles + (icon->offset + ty * 16 + tx) * 32;
				u8* dst = buf + (ty * 4 + tx) * 32;

				for (i = 0; i < 32; ++i)
				{
					u8 n, j;

					for (j = 0; j < 2; ++j)
					{
						n = j ? (src[i] >> 4) : (src[i] & 0xF);
						if (map[n] == 0xFF && nextra < npool)
						{
							map[n] = pool[nextra];
							LoadPalette(&sMsIconColour[n], palBase + pool[nextra], 2);
							++nextra;
						}
						if (map[n] == 0xFF)
							map[n] = 1;
					}
					dst[i] = map[src[i] & 0xF] | (map[src[i] >> 4] << 4);
				}
			}
		}
		BlitBitmapToWindow(win, buf, x, k ? y1 : y0, 32, 12);
	}
	if (dark == 12)
		LoadPalette(&sMsIconColour[14], palBase + 12, 2);
}

static void MsDrawSplit(u8 win, u8 x, u8 y, u8 split)
{
	u8 buf[24 * 8];
	const u8* src = PSSIconsTiles + 24 * 8 * split;
	u32 i;

	for (i = 0; i < sizeof(buf); ++i)
		buf[i] = sMsSplitMap[src[i] & 0xF] | (sMsSplitMap[src[i] >> 4] << 4);
	BlitBitmapToWindow(win, buf, x, y, 24, 12);
}

//Z 技・ダイマックス技を見ている間は技名が窓 3（22 タイル）に出るので、1 つ目の枠のカーソルは 2 マスぶん（その行の横いっぱい）にする
static u8 MsCellWidth(u8 slot)
{
	if (slot == 0 && (gNewBS->zMoveData.viewing || gNewBS->dynamaxData.viewing))
		return MS_CELL_W * 2;
	return MS_CELL_W;
}

static void MsDrawBorder(u8 slot, u8 outer, u8 inner)
{
	u8 win = 3 + slot;
	u8 w = MsCellWidth(slot);

	FillWindowPixelRect(win, outer, 0, 0, w, 1);
	FillWindowPixelRect(win, outer, 0, MS_CELL_H - 1, w, 1);
	FillWindowPixelRect(win, outer, 0, 0, 1, MS_CELL_H);
	FillWindowPixelRect(win, outer, w - 1, 0, 1, MS_CELL_H);
	FillWindowPixelRect(win, inner, 1, 1, w - 2, 1);
	FillWindowPixelRect(win, inner, 1, MS_CELL_H - 2, w - 2, 1);
	FillWindowPixelRect(win, inner, 1, 1, 1, MS_CELL_H - 2);
	FillWindowPixelRect(win, inner, w - 2, 1, 1, MS_CELL_H - 2);
}

//2x2 の 4 つの枠の間の十字線（各枠の中心側の縁 1 画素ずつ＝全体で 2 画素の線）
static void MsDrawCross(u8 slot)
{
	u8 win = 3 + slot;

	if (slot & 1)
		FillWindowPixelRect(win, 6, 0, 0, 1, MS_CELL_H);
	else
		FillWindowPixelRect(win, 6, MS_CELL_W - 1, 0, 1, MS_CELL_H);
	if (slot & 2)
		FillWindowPixelRect(win, 6, 0, 0, MS_CELL_W, 1);
	else
		FillWindowPixelRect(win, 6, 0, MS_CELL_H - 1, MS_CELL_W, 1);
}

//[hack] patches/0930 選んでいる技の枠は濃い青で塗り、文字を白にする（2026-10-29 ユーザー決定「案2」・FEAT-move-select-cursor）。ほかの枠は今まで通り
#define MS_SELECT_BG RGB(12, 5, 4) //濃いえんじ（2026-10-29 ユーザー決定「えんじかな」・枠の茶色い線に合わせた）

static void MsApplyCellBg(u8 slot, bool8 selected)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u16 base = (10 + slot) * 16;
	u16 bg = (moveInfo->moves[slot] == MOVE_NONE) ? RGB(9, 9, 10) : MS_CELL_GROUND;
	u16 text = RGB(3, 3, 4);
	u16 shadow = RGB(30, 29, 27);

	if (selected && moveInfo->moves[slot] != MOVE_NONE)
	{
		bg = MS_SELECT_BG;
		text = RGB(31, 31, 31);
		shadow = RGB(5, 2, 2);
	}
	else if (moveInfo->moves[slot] == MOVE_NONE)
	{
		text = RGB(31, 31, 31);
		shadow = RGB(6, 6, 7);
	}
	LoadPalette(&bg, base + 14, 2);
	LoadPalette(&text, base + 13, 2);
	LoadPalette(&shadow, base + 15, 2);
}

//mode: 0 なし・1 選んでいる・2 入れ替えでつかんだ技・3 入れ替え先
static void MsApplyBorder(u8 slot)
{
	MsApplyCellBg(slot, gNewBS->hackMsMode[slot] == 1);
	switch (gNewBS->hackMsMode[slot]) {
		case 1: MsDrawBorder(slot, 14, 14); MsDrawCross(slot); break;
		case 2: MsDrawBorder(slot, 7, 7); break;
		case 3: MsDrawBorder(slot, 1, 6); break;
		default: MsDrawBorder(slot, 14, 14); MsDrawCross(slot); break;
	}
	CopyWindowToVram(3 + slot, COPYWIN_BOTH);
	//窓 3・5 は Z 技などの長い文のため 22 タイルの幅（上流と同じに隣の枠へ重ねる）。ふだんは自分の枠（11 タイル）だけ地図に置く
	if (MsCellWidth(slot) > MS_CELL_W)
		PutWindowTilemap(3 + slot); //2 マスぶんのカーソル（窓 3 全幅）
	else if (slot == 0 || slot == 2)
		PutWindowRectTilemap(3 + slot, 0, 0, 11, 3);
	else
		PutWindowTilemap(3 + slot);
}

static void MsDrawCell(u8 slot)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u8 win = 3 + slot;
	u8 col[3] = {14, 13, 15};

	if (moveInfo->moves[slot] == MOVE_NONE)
	{
		MsLoadCellPalette(slot, 0, TRUE);
		FillWindowPixelBuffer(win, PIXEL_FILL(14));
	}
	else
	{
		u8 first = MsSlotType(moveInfo, slot);

		MsLoadCellPalette(slot, first, FALSE);
		FillWindowPixelBuffer(win, PIXEL_FILL(14));
		MsDrawPills(win, 6, 0, first, 0, 0xFF);
		MsDrawSplit(win, 44, 0, moveInfo->moveSplit[slot]);
		MoveNameToDisplayedStringBattle(slot);
		AddTextPrinterParameterized4(win, MS_FONT, 4, 11, 0, 0, col, -1, gDisplayedStringBattle);
	}
	MsApplyBorder(slot);
}

void MoveSelectionDisplayMoveNames(void)
{
    s32 i;
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
    gNumberOfMovesToChoose = 0;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        gNewBS->hackMsMode[i] = 0;
        MsDrawCell(i);
        if (moveInfo->moves[i] != MOVE_NONE)
            gNumberOfMovesToChoose++;
    }
}

//カーソルは枠の線だけを描き直す（中の絵・文字には触らない＝Z 技・L の詳細の文章の上でも使える）
void MoveSelectionCreateCursorAt(u8 cursorPosition, u8 baseTileNum)
{
	u8 slot = cursorPosition & 3;

	gNewBS->hackMsMode[slot] = (baseTileNum == 29) ? 2 : (baseTileNum == 27) ? 3 : 1;
	MsApplyBorder(slot);
}

void MoveSelectionDestroyCursorAt(u8 cursorPosition)
{
	u8 slot = cursorPosition & 3;

	gNewBS->hackMsMode[slot] = 0;
	MsApplyBorder(slot);
}

//技の枠の札・分類の絵を全部描き直す（テラスタル・2 タイプの入れ替え・メガの切り替えの後）
static void MoveSelectionDisplayMoveType(void)
{
	u8 slot;

	for (slot = 0; slot < MAX_MON_MOVES; ++slot)
		MsDrawCell(slot);
	for (slot = 0; slot < MAX_MON_MOVES; ++slot)
		MsApplyBorder(slot);
}

//右の枠の地と縁。パレットも入れ直す
static void MsDrawPanelBase(u8 win)
{
	LoadPalette(sMsPanelPal, 14 * 16, 32);
	FillWindowPixelBuffer(win, PIXEL_FILL(0));
	FillWindowPixelRect(win, 2, 1, 0, MS_PANEL_W - 2, MS_PANEL_H);
	FillWindowPixelRect(win, 2, 0, 1, MS_PANEL_W, MS_PANEL_H - 2);
	FillWindowPixelRect(win, 14, 2, 2, MS_PANEL_W - 4, MS_PANEL_H - 4);
}

static void MsPrintRight(u8 win, u8 y, const u8* str, u8 fg)
{
	u8 col[3] = {MS_TXT_BG, fg, 15};
	s32 w = GetStringWidth(MS_FONT, str, (s8)MS_LS);

	AddTextPrinterParameterized4(win, MS_FONT, MS_PANEL_RIGHT - w, y, MS_LS, 0, col, -1, str);
}

static void MsPrintLeft(u8 win, u8 y, const u8* str)
{
	u8 col[3] = {MS_TXT_BG, 13, 15};

	AddTextPrinterParameterized4(win, MS_FONT, MS_LABEL_X, y, MS_LS, 0, col, -1, str);
}

//右の枠の PP の行（技の枠の右の枠の上の段・メガの札のときは y を 2 画素上げて呼ぶ）
static void MsDrawPpRow(u8 win, u8 y, bool8 zView)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u8 slot = gMoveSelectionCursor[gActiveBattler];
	u8 buf[16];
	u8* p;
	u8 ppColour = 13;

	if (zView)
	{
		p = ConvertIntToDecimalStringN(buf, 1, STR_CONV_MODE_LEFT_ALIGN, 2);
		*p++ = CHAR_SLASH;
		p = ConvertIntToDecimalStringN(p, 1, STR_CONV_MODE_LEFT_ALIGN, 2);
	}
	else if (gBattleBufferA[gActiveBattler][2] == TRUE) //PP の数を出さない戦い
	{
		StringCopy(buf, gText_NoMiss);
	}
	else
	{
		u8 cur = moveInfo->currentPp[slot], max = moveInfo->maxPp[slot];

		p = ConvertIntToDecimalStringN(buf, cur, STR_CONV_MODE_LEFT_ALIGN, 2);
		*p++ = CHAR_SLASH;
		ConvertIntToDecimalStringN(p, max, STR_CONV_MODE_LEFT_ALIGN, 2);
		if (cur == 0)
			ppColour = 4;
		else if (cur * 4 <= max)
			ppColour = 3;
	}
	MsPrintRight(win, y, buf, ppColour);
	MsPrintLeft(win, y, sMsTextPP);
}

//[hack] patches/0930: ダイマックス技・Z 技を見ている間の右の枠の威力。戻り値 0 は「ー」（へんか技）
//  ダイマックス技: EmitChooseMove が戦闘と同じ計算（CalcVisualBasePower→gDynamaxMovePowers）で入れた maxMovePowers。Z 技: 下の Z 技の窓と同じ値
static u16 MsPanelPower(struct ChooseMoveStruct *moveInfo, u8 slot, bool8 zView)
{
	if (moveInfo->moveSplit[slot] == SPLIT_STATUS)
		return 0;
	if (zView)
	{
		u16 zmove = moveInfo->possibleZMoves[slot];

		if (zmove >= MOVE_CATASTROPIKA)
			return MOVE_POWER(zmove); //[hack] patches/0582 (#126)
		return gBattleMoves[moveInfo->moves[slot]].z_move_power;
	}
	if (moveInfo->dynamaxed || gNewBS->dynamaxData.viewing)
		return moveInfo->maxMovePowers[slot];
	return moveInfo->movePowers[slot];
}

//右の枠: 上の段 PP・中の段 PW（威力）・下の段 HIT（命中）。威力・命中が無い技は「ー」
//  ダイマックス技・Z 技を見ている間はその技の値（命中は必中なので「ー」・PP は元の技のまま）
static void MsDrawPanel(bool8 zView)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u8 slot = gMoveSelectionCursor[gActiveBattler];
	u8 buf[16];
	u16 power, acc;
	bool8 maxView = moveInfo->dynamaxed || gNewBS->dynamaxData.viewing;

	MsDrawPanelBase(MS_PANEL_WIN);
	MsDrawPpRow(MS_PANEL_WIN, MS_ROW1_Y, zView);

	if (moveInfo->moves[slot] != MOVE_NONE)
	{
		//威力
		power = MsPanelPower(moveInfo, slot, zView);
		MsPrintLeft(MS_PANEL_WIN, MS_ROW2_Y, sMsTextPW);
		if (power == 0)
			StringCopy(buf, gText_NoMiss);
		else
			ConvertIntToDecimalStringN(buf, power, STR_CONV_MODE_LEFT_ALIGN, 3);
		MsPrintRight(MS_PANEL_WIN, MS_ROW2_Y, buf, 13);

		//命中
		acc = moveInfo->moveAcc[slot];
		MsPrintLeft(MS_PANEL_WIN, MS_ROW3_Y, sMsTextHIT);
		if (zView || maxView || acc == 0xFFFF || acc == 0)
			StringCopy(buf, gText_NoMiss);
		else
			ConvertIntToDecimalStringN(buf, acc, STR_CONV_MODE_LEFT_ALIGN, 3);
		MsPrintRight(MS_PANEL_WIN, MS_ROW3_Y, buf, 13);
	}
	CopyWindowToVram(MS_PANEL_WIN, COPYWIN_BOTH);
	PutWindowTilemap(MS_PANEL_WIN);
}

void MoveSelectionDisplayPpNumber(void)
{
	MsDrawPanel(FALSE);
}

//効き目の言葉の文字の色（熟練度などで数字の色を変えるときもここのような関数に分ける）。fg は右の枠のパレットの番号
//  ばつぐん・いまひとつ・こうかなしは相性の色（パレット 9）を黒で縁取る（読みやすくする）・ふつうは黒い文字（縁取りなし）
static u8 MsEffFgColour(u8 effKind)
{
	return (effKind == 0) ? MS_COL_BLACK : 9;
}

static bool8 MsEffOutlined(u8 effKind)
{
	return effKind != 0;
}

//黒の縁取りの文字: 黒を上下左右の 4 方向（2026-10-28 ユーザー決定）にずらして描いてから、色の文字を重ねる
static void MsPrintOutlined(u8 win, u8 font, u8 x, u8 y, const u8* str, u8 fg)
{
	u8 black[3] = {0, MS_COL_BLACK, MS_COL_BLACK};
	u8 mainCol[3] = {0, 0, 0};
	s32 dx, dy;

	mainCol[1] = fg;
	for (dy = -1; dy <= 1; ++dy)
		for (dx = -1; dx <= 1; ++dx)
			if ((dx == 0) != (dy == 0)) //上下左右の 4 方向だけ
				AddTextPrinterParameterized4(win, font, x + dx, y + dy, 0, 0, black, -1, str);
	AddTextPrinterParameterized4(win, font, x, y, 0, 0, mainCol, -1, str);
}

//効き目を言葉で出す。effKind: 0 ふつう・1 ばつぐん・2 いまひとつ・3 こうかなし・0xFF 出さない（へんか技・相手が決まらない）。STAB の印（F9 04）は別に出す
static void MsDrawEffText(u8 effKind, bool8 stab, u8 dy)
{
	const u8* word = (effKind == 1) ? sMsTextSE : (effKind == 2) ? sMsTextNVE : (effKind == 3) ? sMsTextNO : (effKind == 0) ? sMsTextNORMAL : NULL;
	u8 col[3] = {0, 9, 8};

	//枠の線（上・左右の 2 画素）には触らない（2026-10-28 直し: 以前は y=1 から塗って上の線が欠けていた）
	FillWindowPixelRect(MS_PANEL_WIN, 14, 2, 2, MS_PANEL_W - 4, 11);
	if (word != NULL)
	{
		if (MsEffOutlined(effKind))
			MsPrintOutlined(MS_PANEL_WIN, MS_FONT, MS_LABEL_X + 1, MS_ROW0_Y - dy, word, MsEffFgColour(effKind));
		else
		{
			col[1] = MsEffFgColour(effKind);
			col[2] = 0;
			AddTextPrinterParameterized4(MS_PANEL_WIN, MS_FONT, MS_LABEL_X + 1, MS_ROW0_Y - dy, 0, 0, col, -1, word);
		}
	}
	if (stab)
	{
		col[1] = 9;
		col[2] = 8;
		AddTextPrinterParameterized4(MS_PANEL_WIN, 1, 46, 0, 0, 0, col, -1, gText_BattleUI_STAB);
	}
}

//相性（▲▼＊）と STAB の印は右の枠の左上に出す（上の段の PP の左）
//dy: 効き目の言葉を上げる画素数（メガの札のときだけ 1）
static void MsEffectivenessImpl(u8 dy)
{
	u8 *txtPtr, moveType, split;
	const u8* string;
	bool8 stab = FALSE;
	u8 effKind = 0xFF; //0 ふつう・1 ばつぐん・2 いまひとつ・3 こうかなし・0xFF 出さない
	u8 stabPalIndex = MS_PANEL_PAL * 0x10 + 6;
	u8 palIndex = stabPalIndex + 2;
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u16 move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
	//[hack] patches/0930: ダイマックス技を見ている間（まだ ダイマックス していない）は、効き目も STAB もダイマックス技で見る
	bool8 maxView = gNewBS->dynamaxData.viewing && !moveInfo->dynamaxed
		&& moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]] != MOVE_NONE;
	u16 maxMove = moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]];

	#ifdef DISPLAY_REAL_MOVE_TYPE_ON_MENU
	if (maxView)
		moveType = GetMoveTypeSpecial(gActiveBattler, maxMove);
	else if ((move == MOVE_TERABLAST || move == MOVE_TERASTARSTORM)
	&& gNewBS->terastalData.chosen[gActiveBattler])
		moveType = moveInfo->monTeraType;
	else
		moveType = moveInfo->moveTypes[gMoveSelectionCursor[gActiveBattler]];
	#else
	if (!moveInfo->dynamaxed)
		moveType = gBattleMoves[move].type;
	else
		moveType = gBattleMoves[moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]]].type;
	#endif

	//Update Palette Fading for Effectiveness
	split = moveInfo->moveSplit[gMoveSelectionCursor[gActiveBattler]];
	#ifdef DISPLAY_EFFECTIVENESS_ON_MENU
	if (!(gBattleTypeFlags & BATTLE_TYPE_LINK) && !IS_GHOST_BATTLE) //Don't use this feature in link battles or battles against Ghosts
	{
		u8 foePosition;
		u8 moveResult = 0;
		bool8 known = TRUE; //相手が決まらない（2 体以上の相手）ときは効き目を出さない
		const u16* palPtr = gUserInterfaceGfx_TypeHighlightingPal;
		stab = split != SPLIT_STATUS
			&& (moveType == moveInfo->monType1
			 || moveType == moveInfo->monType2
			 || moveType == moveInfo->monType3
			 || (moveType == moveInfo->monTeraType && TeraTypeActive(gActiveBattler)))
			&& !CheckTableForMoveEffect(maxView ? maxMove : move, gMoveEffectsThatIgnoreWeaknessResistance); //These moves can't have STAB

		if (IS_SINGLE_BATTLE)
		{
			foePosition = GetBattlerPosition(FOE(gActiveBattler));
		}
		else if (gBattlerControllerFuncs[gActiveBattler] == HandleInputChooseTarget)
		{
			foePosition = GetBattlerPosition(gMultiUsePlayerCursor);
		}
		else if (CountAliveMonsInBattle(BATTLE_ALIVE_DEF_SIDE, gActiveBattler, FOE(gActiveBattler)) <= 1) //Only 1 enemy left
		{
			u8 bankDef = FOE(gActiveBattler);
			if (!BATTLER_ALIVE(bankDef))
				bankDef = PARTNER(bankDef);

			foePosition = GetBattlerPosition(bankDef);
		}
		else
		{
				known = FALSE;
				goto SKIP_LOAD_MOVE_RESULT;
			}

		if (maxView)
			moveResult = VisualTypeCalc(maxMove, gActiveBattler, GetBattlerAtPosition(foePosition)); //戦闘と同じ計算をダイマックス技で
		else if (gNewBS->zMoveData.viewing)
			moveResult = moveInfo->zMoveResults[foePosition][gMoveSelectionCursor[gActiveBattler]];
		else
			moveResult = moveInfo->moveResults[foePosition][gMoveSelectionCursor[gActiveBattler]];

		SKIP_LOAD_MOVE_RESULT:
		if (moveResult & MOVE_RESULT_NO_EFFECT) //Comes first because overrides all others
		{
			effKind = 3;
			gPlttBufferUnfaded[palIndex + 0] = palPtr[NO_EFFECT_COLOURS + 0]; //No STAB for moves with no effect
			gPlttBufferUnfaded[palIndex + 1] = palPtr[NO_EFFECT_COLOURS + 1];
			string = gText_BattleUI_NoEffect;
			stab = FALSE; //No STAB on a move that does no damage
		}
		else if (moveResult & MOVE_RESULT_SUPER_EFFECTIVE)
		{
			effKind = 1;
			gPlttBufferUnfaded[palIndex + 0] = palPtr[SUPER_EFFECTIVE_COLOURS + 0];
			gPlttBufferUnfaded[palIndex + 1] = palPtr[SUPER_EFFECTIVE_COLOURS + 1];
			string = gText_BattleUI_SuperEffective;
		}
		else if (moveResult & MOVE_RESULT_NOT_VERY_EFFECTIVE)
		{
			effKind = 2;
			gPlttBufferUnfaded[palIndex + 0] = palPtr[NOT_VERY_EFFECTIVE_COLOURS + 0];
			gPlttBufferUnfaded[palIndex + 1] = palPtr[NOT_VERY_EFFECTIVE_COLOURS + 1];
			string = gText_BattleUI_NotVeryEffective;
		}
		else //Nothing special about move
		{
			gPlttBufferUnfaded[palIndex + 0] = palPtr[REGULAR_COLOURS + 0];
			gPlttBufferUnfaded[palIndex + 1] = palPtr[REGULAR_COLOURS + 1];
			if (known && split != SPLIT_STATUS)
				effKind = 0; //ふつう（へんか技・相手が決まらないときは出さない）
			string = StringNull;
		}
	}
	else
		string = StringNull;
	#else
		string = gText_PP;
	#endif

	txtPtr = gDisplayedStringBattle;
	*txtPtr++ = EXT_CTRL_CODE_BEGIN;
	*txtPtr++ = 6;
	*txtPtr++ = 1;
	#ifdef DISPLAY_EFFECTIVENESS_ON_MENU
	txtPtr = StringCopy(txtPtr, sText_StabMoveInterfaceType);
	#endif
	txtPtr = StringCopy(txtPtr, string);

	if (stab)
	{
		gPlttBufferUnfaded[stabPalIndex + 0] = RGB(27, 27, 27); //Copy over PP colours so it's unaffected by low PP
		gPlttBufferUnfaded[stabPalIndex + 1] = RGB(4, 4, 4);
		StringCopy(txtPtr, gText_BattleUI_STAB);
	}

	MsDrawEffText(effKind, stab, dy);
	CopyWindowToVram(MS_PANEL_WIN, COPYWIN_BOTH);
	PutWindowTilemap(MS_PANEL_WIN);
	CpuCopy16(&gPlttBufferUnfaded[stabPalIndex], &gPlttBufferFaded[stabPalIndex], sizeof(u16) * 4);
}

void MoveSelectionDisplayMoveEffectiveness(void)
{
	MsEffectivenessImpl(0);
}

static bool8 MoveSelectionDisplayZMove(void)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u16 zmove = moveInfo->possibleZMoves[gMoveSelectionCursor[gActiveBattler]];
	u8 i;

	if (gNewBS->zMoveData.viewing)
	{
		ReloadMoveNamesIfNecessary();
		PlaySE(3); //Turn Off
		return TRUE;
	}

	if (moveInfo->dynamaxed || IsTerastal(gActiveBattler))
		return FALSE;

	if (zmove != MOVE_NONE)
	{
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			MoveSelectionDestroyCursorAt(i);
			StringCopy(gDisplayedStringBattle, StringNull);
			BattlePutTextOnWindow(gDisplayedStringBattle, i + 3);
		}

		if (zmove == 0xFFFF)
		{
			u8 zEffect = gBattleMoves[moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]].z_move_effect;

			if (zEffect == Z_EFFECT_CURSE)
			{
				if (moveInfo->monType1 == TYPE_GHOST || moveInfo->monType2 == TYPE_GHOST || moveInfo->monType3 == TYPE_GHOST)
					zEffect = Z_EFFECT_RECOVER_HP;
				else
					zEffect = Z_EFFECT_ATK_UP_1;
			}

			gDisplayedStringBattle[0] = EOS;
			switch (zEffect) {
				case Z_EFFECT_RESET_STATS:
					StringCopy(gDisplayedStringBattle, gText_ResetStats);
					break;

				case Z_EFFECT_ALL_STATS_UP_1:
					StringCopy(gDisplayedStringBattle, gText_StatsPlus);
					break;

				case Z_EFFECT_BOOST_CRITS:
					StringCopy(gDisplayedStringBattle, gText_CritHitsPlus);
					break;

				case Z_EFFECT_FOLLOW_ME:
					StringCopy(gDisplayedStringBattle, gText_FollowMe);
					break;

				case Z_EFFECT_RECOVER_HP:
					StringCopy(gDisplayedStringBattle, gText_RecoverHP);
					break;

				case Z_EFFECT_RESTORE_REPLACEMENT_HP:
					StringCopy(gDisplayedStringBattle, gText_HealAllyHP);
					break;

				case Z_EFFECT_ATK_UP_1:
				case Z_EFFECT_DEF_UP_1:
				case Z_EFFECT_SPD_UP_1:
				case Z_EFFECT_SPATK_UP_1:
				case Z_EFFECT_SPDEF_UP_1:
				case Z_EFFECT_ACC_UP_1:
				case Z_EFFECT_EVSN_UP_1:
					PREPARE_STAT_BUFFER(gBattleTextBuff1, zEffect - Z_EFFECT_ATK_UP_1 + 1);
					ExpandBattleTextBuffPlaceholders(gBattleTextBuff1, gDisplayedStringBattle);
					StringAppend(gDisplayedStringBattle, gText_ZMoveRaise1);
					break;

				case Z_EFFECT_ATK_UP_2:
				case Z_EFFECT_DEF_UP_2:
				case Z_EFFECT_SPD_UP_2:
				case Z_EFFECT_SPATK_UP_2:
				case Z_EFFECT_SPDEF_UP_2:
				case Z_EFFECT_ACC_UP_2:
				case Z_EFFECT_EVSN_UP_2:
					PREPARE_STAT_BUFFER(gBattleTextBuff1, zEffect - Z_EFFECT_ATK_UP_2 + 1);
					ExpandBattleTextBuffPlaceholders(gBattleTextBuff1, gDisplayedStringBattle);
					StringAppend(gDisplayedStringBattle, gText_ZMoveRaise2);
					break;

				case Z_EFFECT_ATK_UP_3:
				case Z_EFFECT_DEF_UP_3:
				case Z_EFFECT_SPD_UP_3:
				case Z_EFFECT_SPATK_UP_3:
				case Z_EFFECT_SPDEF_UP_3:
				case Z_EFFECT_ACC_UP_3:
				case Z_EFFECT_EVSN_UP_3:
					PREPARE_STAT_BUFFER(gBattleTextBuff1, zEffect - Z_EFFECT_ATK_UP_3 + 1);
					ExpandBattleTextBuffPlaceholders(gBattleTextBuff1, gDisplayedStringBattle);
					StringAppend(gDisplayedStringBattle, gText_ZMoveRaise3);
					break;
			}
			BattlePutTextOnWindow(gDisplayedStringBattle, 3 + 2); //Slot of Move 3

			gDisplayedStringBattle[0] = 0xD4;
			StringCopy(gDisplayedStringBattle + 1, gMoveNames[moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]]);
		}
		else
		{
			ZMoveSelectionDisplayPower();
			StringCopy(gDisplayedStringBattle, GetZMoveName(zmove));
		}
		BattlePutTextOnWindow(gDisplayedStringBattle, 3);

		gNewBS->zMoveData.viewing = TRUE;
		ZMoveSelectionDisplayPpNumber();
		MoveSelectionDisplayMoveEffectiveness();
		MoveSelectionCreateCursorAt(0, 0);
		PlaySE(2); //Turn On
		return TRUE;
	}

	return FALSE;
}

static bool8 MoveSelectionDisplayMaxMove(void)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u16 maxMove = moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]];
	u8 i;

	if (gNewBS->dynamaxData.viewing)
	{
		ReloadMoveNamesIfNecessary();

		if (!moveInfo->dynamaxed) //Don't play sound if already Dynamaxed
			PlaySE(3); //Turn Off
		return TRUE;
	}

	if (maxMove != MOVE_NONE)
	{
		for (i = 0; i < MAX_MON_MOVES; ++i)
		{
			MoveSelectionDestroyCursorAt(i);
			StringCopy(gDisplayedStringBattle, StringNull);
			BattlePutTextOnWindow(gDisplayedStringBattle, i + 3);
		}

		u8 maxEffect = gBattleMoves[maxMove].z_move_effect;

		gDisplayedStringBattle[0] = EOS;
		switch (maxEffect) {
			case MAX_EFFECT_NONE:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveProtect);
				break;

			case MAX_EFFECT_RAISE_TEAM_ATTACK:
			case MAX_EFFECT_RAISE_TEAM_DEFENSE:
			case MAX_EFFECT_RAISE_TEAM_SPEED:
			case MAX_EFFECT_RAISE_TEAM_SP_ATK:
			case MAX_EFFECT_RAISE_TEAM_SP_DEF:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveRaiseTeamStat);
				PREPARE_STAT_BUFFER(gBattleTextBuff1, maxEffect - MAX_EFFECT_RAISE_TEAM_ATTACK + 1);
				ExpandBattleTextBuffPlaceholders(gBattleTextBuff1, gDisplayedStringBattle + 5);
				StringAppend(gDisplayedStringBattle, gText_Raise);
				break;

			case MAX_EFFECT_LOWER_ATTACK:
			case MAX_EFFECT_LOWER_DEFENSE:
			case MAX_EFFECT_LOWER_SPEED:
			case MAX_EFFECT_LOWER_SP_ATK:
			case MAX_EFFECT_LOWER_SP_DEF:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveFoes);
				PREPARE_STAT_BUFFER(gBattleTextBuff1, maxEffect - MAX_EFFECT_LOWER_ATTACK + 1);
				ExpandBattleTextBuffPlaceholders(gBattleTextBuff1, gDisplayedStringBattle + 5);
				StringAppend(gDisplayedStringBattle, gText_Lower);
				break;

			case MAX_EFFECT_SUN:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveWeather);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSunny);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_RAIN:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveWeather);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveRainy);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_SANDSTORM:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveWeather);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSandstorm);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_HAIL:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveWeather);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveHail);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;

			case MAX_EFFECT_ELECTRIC_TERRAIN:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveElectric);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveTerrain);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_GRASSY_TERRAIN:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveGrassy);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveTerrain);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_MISTY_TERRAIN:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveMisty);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveTerrain);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_PSYCHIC_TERRAIN:
				StringCopy(gDisplayedStringBattle, gText_MaxMovePsychic);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveTerrain);
				StringAppend(gDisplayedStringBattle, gText_MaxMoveSet);
				break;
			case MAX_EFFECT_VINE_LASH:
			case MAX_EFFECT_WILDFIRE:
			case MAX_EFFECT_CANNONADE:
			case MAX_EFFECT_VOLCAITH_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveLastingDamage);
				break;
			case MAX_EFFECT_EFFECT_SPORE_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveEffectSpore);
				break;
			case MAX_EFFECT_PARALYZE_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveParalyzeFoes);
				break;
			case MAX_EFFECT_CONFUSE_FOES_PAY_DAY:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveConfusePayDay);
				break;
			case MAX_EFFECT_CRIT_PLUS:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveCritPlus);
				break;
			case MAX_EFFECT_MEAN_LOOK:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveMeanLook);
				break;
			case MAX_EFFECT_AURORA_VEIL:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveAuroraVeil);
				break;
			case MAX_EFFECT_INFATUATE_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveInfatuate);
				break;
			case MAX_EFFECT_RECYCLE_BERRIES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveRecycleBerries);
				break;
			case MAX_EFFECT_POISON_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMovePoisonFoes);
				break;
			case MAX_EFFECT_STEALTH_ROCK:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveStealthRock);
				break;
			case MAX_EFFECT_DEFOG:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveRemoveScreens);
				break;
			case MAX_EFFECT_POISON_PARALYZE_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMovePoisonParalyzeFoes);
				break;
			case MAX_EFFECT_HEAL_TEAM:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveHealTeam);
				break;
			case MAX_EFFECT_SPITE:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveSpite);
				break;
			case MAX_EFFECT_GRAVITY:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveGravity);
				break;
			case MAX_EFFECT_FIRE_SPIN_FOES:
			case MAX_EFFECT_SANDBLAST_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveTrapDamage);
				break;
			case MAX_EFFECT_YAWN_FOE:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveYawn);
				break;
			case MAX_EFFECT_LOWER_EVASIVENESS_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveLowerEvasiveness);
				break;
			case MAX_EFFECT_AROMATHERAPY:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveAromatherapy);
				break;
			case MAX_EFFECT_CONFUSE_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveConfuseFoes);
				break;
			case MAX_EFFECT_STEELSURGE:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveSteelsurge);
				break;
			case MAX_EFFECT_TORMENT_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveTormentFoes);
				break;
			case MAX_EFFECT_LOWER_SPEED_2_FOES:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveLowerSpeed2);
				break;
			case MAX_EFFECT_IGNORE_ABILITY:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveIgnoreAbility);
				break;
			case MAX_EFFECT_BYPASS_PROTECT:
				StringCopy(gDisplayedStringBattle, gText_MaxMoveBypassProtect);
				break;
		}

		BattlePutTextOnWindow(gDisplayedStringBattle, 3 + 2); //Slot of Move 3

		if (IsGMaxMove(maxMove))
		{
			gDisplayedStringBattle[0] = PC_KI;
			gDisplayedStringBattle[1] = PC_XYO;
			gDisplayedStringBattle[2] = PC_DA;
			gDisplayedStringBattle[3] = PC_II;
			StringCopy(gDisplayedStringBattle + 4, gMoveNames[maxMove]);
		}
		else
		{
			gDisplayedStringBattle[0] = PC_DA;
			gDisplayedStringBattle[1] = PC_II;
			StringCopy(gDisplayedStringBattle + 2, gMoveNames[maxMove]);
		}

		BattlePutTextOnWindow(gDisplayedStringBattle, 3);
		MaxMoveSelectionDisplayPower();
		gNewBS->dynamaxData.viewing = TRUE; //[hack] patches/0930: カーソルの幅（2 マス）が見ている状態を読むので、先に立てる
		MoveSelectionCreateCursorAt(0, 0);
		gNewBS->terastalData.chosen[gActiveBattler] = FALSE;
		MoveSelectionDisplayPpNumber(); //[hack] patches/0930: 右の枠もダイマックス技の値で描き直す
		MoveSelectionDisplayMoveEffectiveness();

		if (!moveInfo->dynamaxed) //Don't play sound if already Dynamaxed
			PlaySE(2); //Turn On
		return TRUE;
	}

	return FALSE;
}

static void ZMoveSelectionDisplayPpNumber(void)
{
	MsDrawPanel(TRUE); //[hack] patches/0930: 右の枠に 1/1
}

static void ZMoveSelectionDisplayPower(void)
{
	u8 *txtPtr;
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u16 move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
	u16 power = gBattleMoves[move].z_move_power;

	u16 zmove = moveInfo->possibleZMoves[gMoveSelectionCursor[gActiveBattler]];

	if (zmove >= MOVE_CATASTROPIKA)
		power = MOVE_POWER(zmove); //[hack] patches/0582 (#126)

	if (moveInfo->moveSplit[gMoveSelectionCursor[gActiveBattler]] != SPLIT_STATUS)
	{
		txtPtr = StringCopy(gDisplayedStringBattle, gText_Power);
		ConvertIntToDecimalStringN(txtPtr, power, STR_CONV_MODE_LEFT_ALIGN, 3);
		BattlePutTextOnWindow(gDisplayedStringBattle, 3 + 2); //Slot of Move 3
	/*
		txtPtr = StringCopy(gDisplayedStringBattle, gText_Power);
		txtPtr[0] = EXT_CTRL_CODE_BEGIN;
		txtPtr++;
		txtPtr[0] = 6;
		txtPtr++;
		txtPtr[0] = 1;
		txtPtr++;
		ConvertIntToDecimalStringN(txtPtr, power, STR_CONV_MODE_LEFT_ALIGN, 3);
		BattlePutTextOnWindow(gDisplayedStringBattle, 8); //Where Move Type goes
	*/
	}
}

static void MaxMoveSelectionDisplayPower(void)
{
	#ifdef DYNAMAX_FEATURE
	u8 *txtPtr;
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);

	#ifdef DISPLAY_REAL_POWER_ON_MENU
		u16 power = moveInfo->maxMovePowers[gMoveSelectionCursor[gActiveBattler]];
	#else
		u16 power = gDynamaxMovePowers[move];
	#endif

	if (moveInfo->moveSplit[gMoveSelectionCursor[gActiveBattler]] != SPLIT_STATUS)
	{
		txtPtr = StringCopy(gDisplayedStringBattle, gText_Power);
		ConvertIntToDecimalStringN(txtPtr, power, STR_CONV_MODE_LEFT_ALIGN, 3);
		BattlePutTextOnWindow(gDisplayedStringBattle, 3 + 1); //Slot of Move 2
	}
	#endif
}

static void MoveSelectionDisplayDetails(void)
{
	u8 *txtPtr;
	const u8* string;
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);

	if (gNewBS->zMoveData.viewingDetails)
	{
		ReloadMoveNamesIfNecessary();
		return;
	}

	for (int i = 0; i < MAX_MON_MOVES; ++i)
	{
		MoveSelectionDestroyCursorAt(i);
		StringCopy(gDisplayedStringBattle, StringNull);
		BattlePutTextOnWindow(gDisplayedStringBattle, i + 3);
	}

//Diaplay Move Name
	MoveNameToDisplayedStringBattle(gMoveSelectionCursor[gActiveBattler]);
	BattlePutTextOnWindow(gDisplayedStringBattle, 3);

//Display Move Contact
	if (moveInfo->makesContact[gMoveSelectionCursor[gActiveBattler]])
		string = gText_Contact;
	else
		string = gText_NoContact;

	BattlePutTextOnWindow(string, 3 + 1); //Slot of Move 2

//Display Move Power
	if (!moveInfo->dynamaxed && gBattleMoves[moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]].effect == EFFECT_0HKO)
	{
		StringCopy(gDisplayedStringBattle, gText_OHKO);
	}
	else
	{
		u16 power;
		txtPtr = StringCopy(gDisplayedStringBattle, gText_Power);
		#ifdef DISPLAY_REAL_POWER_ON_MENU
			if (moveInfo->dynamaxed)
				power = moveInfo->maxMovePowers[gMoveSelectionCursor[gActiveBattler]];
			else
				power = moveInfo->movePowers[gMoveSelectionCursor[gActiveBattler]];
		#else
			if (moveInfo->dynamaxed)
				power = gDynamaxMovePowers[moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]];
			else
				power = MOVE_POWER(moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]); //[hack] patches/0582 (#126)
		#endif
		ConvertIntToDecimalStringN(txtPtr, power, STR_CONV_MODE_LEFT_ALIGN, 3);
	}
	BattlePutTextOnWindow(gDisplayedStringBattle, 3 + 2); //Slot of Move 3

//Display Move Accuracy
	txtPtr = StringCopy(gDisplayedStringBattle, gText_Acc);
	#ifdef DISPLAY_REAL_ACCURACY_ON_MENU
		if (moveInfo->dynamaxed || moveInfo->moveAcc[gMoveSelectionCursor[gActiveBattler]] == 0xFFFF)
			StringCopy(txtPtr, gText_NoMiss);
		else
			ConvertIntToDecimalStringN(txtPtr, moveInfo->moveAcc[gMoveSelectionCursor[gActiveBattler]], STR_CONV_MODE_LEFT_ALIGN, 3);
	#else
		if (moveInfo->dynamaxed || gBattleMoves[moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]].accuracy == 0)
			StringCopy(txtPtr, gText_NoMiss);
		else
			ConvertIntToDecimalStringN(txtPtr, gBattleMoves[moveInfo->moves[gMoveSelectionCursor[gActiveBattler]]].accuracy, STR_CONV_MODE_LEFT_ALIGN, 3);
	#endif
	BattlePutTextOnWindow(gDisplayedStringBattle, 3 + 3); //Slot of Move 4

	MoveSelectionCreateCursorAt(0, 0);
	gNewBS->zMoveData.viewingDetails = TRUE;
}

static void ReloadMoveNamesIfNecessary(void)
{
	if (gNewBS->zMoveData.viewing || gNewBS->dynamaxData.viewing)
	{
		gNewBS->zMoveData.viewing = FALSE;
		gNewBS->dynamaxData.viewing = FALSE;
		MoveSelectionDestroyCursorAt(0);
		MoveSelectionDisplayMoveNames();
		MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
		MoveSelectionDisplayPpNumber();
		MoveSelectionDisplayMoveType();
		MoveSelectionDisplayMoveEffectiveness();
	}
	else if (gNewBS->zMoveData.viewingDetails)
	{
		gNewBS->zMoveData.viewingDetails = FALSE;
		MoveSelectionDestroyCursorAt(0);
		MoveSelectionDisplayMoveNames();
		MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
		MoveSelectionDisplayPpNumber();
		MoveSelectionDisplayMoveType();
		MoveSelectionDisplayMoveEffectiveness();
	}
}

static void CloseZMoveDetails(void)
{
	if (gNewBS->zMoveData.viewing)
	{
		gNewBS->zMoveData.toBeUsed[gActiveBattler] = TRUE;
		gNewBS->zMoveData.viewing = FALSE;
	}

	gNewBS->zMoveData.viewingDetails = FALSE;
}

static void CloseMaxMoveDetails(void)
{
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);

	if (gNewBS->dynamaxData.viewing)
	{
		if (!moveInfo->dynamaxed)
			gNewBS->dynamaxData.toBeUsed[gActiveBattler] = TRUE; //Only set if not already dynamaxed

		gNewBS->dynamaxData.viewing = FALSE;
	}
}

void HandleMoveSwitchingUpdate(void)
{
	u8 perMovePPBonuses[4];
	struct ChooseMoveStruct moveStruct;
	u8 totalPPBonuses;
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);
	u32 i, j;
	u8 slot1 = gMoveSelectionCursor[gActiveBattler];
	u8 slot2 = gMultiUsePlayerCursor;

	//Swap Moves
	i = moveInfo->moves[slot1];
	moveInfo->moves[slot1] = moveInfo->moves[slot2];
	moveInfo->moves[slot2] = i;

	//Swap PP
	i = moveInfo->currentPp[slot1];
	moveInfo->currentPp[slot1] = moveInfo->currentPp[slot2];
	moveInfo->currentPp[slot2] = i;

	i = moveInfo->maxPp[slot1];
	moveInfo->maxPp[slot1] = moveInfo->maxPp[slot2];
	moveInfo->maxPp[slot2] = i;

	//Swap Move Types Details
	i = moveInfo->moveTypes[slot1];
	moveInfo->moveTypes[slot1] = moveInfo->moveTypes[slot2];
	moveInfo->moveTypes[slot2] = i;

	//Swap Move Powers Details
	i = moveInfo->movePowers[slot1];
	moveInfo->movePowers[slot1] = moveInfo->movePowers[slot2];
	moveInfo->movePowers[slot2] = i;

	i = moveInfo->maxMovePowers[slot1];
	moveInfo->maxMovePowers[slot1] = moveInfo->maxMovePowers[slot2];
	moveInfo->maxMovePowers[slot2] = i;

	//Swap Move Accuracies Details
	i = moveInfo->moveAcc[slot1];
	moveInfo->moveAcc[slot1] = moveInfo->moveAcc[slot2];
	moveInfo->moveAcc[slot2] = i;

	//Swap Move Split Details
	i = moveInfo->moveSplit[slot1];
	moveInfo->moveSplit[slot1] = moveInfo->moveSplit[slot2];
	moveInfo->moveSplit[slot2] = i;

	//Swap Move Contact Details
	i = moveInfo->makesContact[slot1];
	moveInfo->makesContact[slot1] = moveInfo->makesContact[slot2];
	moveInfo->makesContact[slot2] = i;

	//Swap Effectiveness Details
	for (j = 0; j < gBattlersCount; ++j) //Swap for effectiveness on each bank
	{
		i = moveInfo->moveResults[j][slot1];
		moveInfo->moveResults[j][slot1] = moveInfo->moveResults[j][slot2];
		moveInfo->moveResults[j][slot2] = i;

		i = moveInfo->zMoveResults[j][slot1];
		moveInfo->zMoveResults[j][slot1] = moveInfo->zMoveResults[j][slot2];
		moveInfo->zMoveResults[j][slot2] = i;
	}

	//Swap Possible Z-Moves
	i = moveInfo->possibleZMoves[slot1];
	moveInfo->possibleZMoves[slot1] = moveInfo->possibleZMoves[slot2];
	moveInfo->possibleZMoves[slot2] = i;

	//Swap Possible Max-Moves
	i = moveInfo->possibleMaxMoves[slot1];
	moveInfo->possibleMaxMoves[slot1] = moveInfo->possibleMaxMoves[slot2];
	moveInfo->possibleMaxMoves[slot2] = i;

	if (gDisableStructs[gActiveBattler].mimickedMoves & gBitTable[slot1])
	{
		gDisableStructs[gActiveBattler].mimickedMoves &= (~gBitTable[slot1]);
		gDisableStructs[gActiveBattler].mimickedMoves |= gBitTable[slot2];
	}

	MoveSelectionDisplayMoveNames();

	for (i = 0; i < MAX_MON_MOVES; i++)
		perMovePPBonuses[i] = (gBattleMons[gActiveBattler].ppBonuses & (3 << (i * 2))) >> (i * 2);

	totalPPBonuses = perMovePPBonuses[slot1];
	perMovePPBonuses[slot1] = perMovePPBonuses[slot2];
	perMovePPBonuses[slot2] = totalPPBonuses;

	totalPPBonuses = 0;
	for (i = 0; i < MAX_MON_MOVES; i++)
		totalPPBonuses |= perMovePPBonuses[i] << (i * 2);

	gBattleMons[gActiveBattler].ppBonuses = totalPPBonuses;

	for (i = 0; i < MAX_MON_MOVES; i++)
	{
		gBattleMons[gActiveBattler].moves[i] = moveInfo->moves[i];
		gBattleMons[gActiveBattler].pp[i] = moveInfo->currentPp[i];
	}

	if (!(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED))
	{
		for (i = 0; i < MAX_MON_MOVES; i++)
		{
			moveStruct.moves[i] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + i, 0);
			moveStruct.currentPp[i] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP1 + i, 0);
		}

		totalPPBonuses = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP_BONUSES, 0);
		for (i = 0; i < MAX_MON_MOVES; i++)
			perMovePPBonuses[i] = (totalPPBonuses & (3 << (i * 2))) >> (i * 2);

		i = moveStruct.moves[slot1];
		moveStruct.moves[slot1] = moveStruct.moves[slot2];
		moveStruct.moves[slot2] = i;

		i = moveStruct.currentPp[slot1];
		moveStruct.currentPp[slot1] = moveStruct.currentPp[slot2];
		moveStruct.currentPp[slot2] = i;

		totalPPBonuses = perMovePPBonuses[slot1];
		perMovePPBonuses[slot1] = perMovePPBonuses[slot2];
		perMovePPBonuses[slot2] = totalPPBonuses;

		totalPPBonuses = 0;
		for (i = 0; i < MAX_MON_MOVES; i++)
			totalPPBonuses |= perMovePPBonuses[i] << (i * 2);

		for (i = 0; i < MAX_MON_MOVES; i++)
		{
			SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + i, &moveStruct.moves[i]);
			SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP1 + i, &moveStruct.currentPp[i]);
		}

		SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP_BONUSES, &totalPPBonuses);
	}
}

void HandleInputChooseTarget(void)
{
	int i;
	u8 identities[4];
	memcpy(identities, sTargetIdentities, 4 /*NELEMS(sTargetIdentities)*/); //sTargetIdentities = 0x8250980
	struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct*)(&gBattleBufferA[gActiveBattler][4]);

	DoBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX, 15, 1); //0x8012138

	if (gBattlersCount != 0)
	{
		for (i = 0; i < gBattlersCount; ++i)
		{
			if (i != gMultiUsePlayerCursor)
				EndBounceEffect(i, BOUNCE_HEALTHBOX); //0x8012258
		}
	}

	if (JOY_NEW(A_BUTTON))
	{
		PlaySE(SE_SELECT);
		gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;
		if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
			TryRemovePartnerDoublesKillingScore(gActiveBattler, gMultiUsePlayerCursor, moveInfo->moves[gMoveSelectionCursor[gActiveBattler]], FALSE);
		EmitMoveChosen(1, gMoveSelectionCursor[gActiveBattler], gMultiUsePlayerCursor, gNewBS->megaData.chosen[gActiveBattler], gNewBS->ultraData.chosen[gActiveBattler], gNewBS->zMoveData.toBeUsed[gActiveBattler], gNewBS->dynamaxData.toBeUsed[gActiveBattler], gNewBS->terastalData.chosen[gActiveBattler]);
		CloseZMoveDetails();
		CloseMaxMoveDetails();
		EndBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX);
		PlayerBufferExecCompleted();
	}
	else if (JOY_NEW(B_BUTTON))
	{
		PlaySE(SE_SELECT);
		gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;
		gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
		DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
		DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);
		EndBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX);

		ReloadMoveNamesIfNecessary();
		TryLoadMegaTriggers();
		TryLoadZTrigger();
		TryLoadDynamaxTrigger();
		TryLoadTerastalTrigger();
		MoveSelectionDisplayMoveEffectiveness();
	}
	else if (JOY_NEW(DPAD_LEFT | DPAD_UP))
	{
		PlaySE(SE_SELECT);
		gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;

		do
		{
			u8 currSelIdentity = GetBattlerPosition(gMultiUsePlayerCursor);

			u16 move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
			switch (EffectiveMove(move)) {
				case MOVE_ACUPRESSURE:
					if (!(gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor ^ BIT_FLANK]))
					{
						gMultiUsePlayerCursor ^= BIT_FLANK;
					}
					gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget; //sub_8039AD8 in Emerald
					return;

				case MOVE_POLLENPUFF:
					if (IsHealBlocked(gActiveBattler) //Affected by Heal Block can't target partner
					&& !(gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor ^ 2]))
					{
						gMultiUsePlayerCursor ^= BIT_FLANK;
						gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget; //sub_8039AD8 in Emerald
						return;
					}
			}

			for (i = 0; i < MAX_BATTLERS_COUNT; i++)
			{
				if (currSelIdentity == identities[i])
					break;
			}
			do
			{
				if (--i < 0)
					i = MAX_BATTLERS_COUNT - 1; // UB: array out of range
				gMultiUsePlayerCursor = GetBattlerAtPosition(identities[i]);
			} while (gMultiUsePlayerCursor == gBattlersCount);

			i = 0;
			move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
			switch (GetBattlerPosition(gMultiUsePlayerCursor)) {
				case B_POSITION_PLAYER_LEFT:
				case B_POSITION_PLAYER_RIGHT:
					if (gActiveBattler != gMultiUsePlayerCursor)
						i++;
					else if (GetBaseMoveTarget(move, gActiveBattler) & MOVE_TARGET_USER_OR_PARTNER)
						i++;

					if ((moveInfo->dynamaxed || gNewBS->dynamaxData.viewing || move == MOVE_DOODLE) && gMultiUsePlayerCursor == PARTNER(gActiveBattler))
						i = FALSE; //Can't use Max Move on partner

					break;
				case B_POSITION_OPPONENT_LEFT:
				case B_POSITION_OPPONENT_RIGHT:
					i++;
					break;
			}

			if (gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor])
				i = 0;

		} while (i == 0);

		gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
		MoveSelectionDisplayMoveEffectiveness();
	}
	else if (JOY_NEW(DPAD_RIGHT | DPAD_DOWN))
	{
		PlaySE(SE_SELECT);
		gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;

		do
		{
			u8 currSelIdentity = GetBattlerPosition(gMultiUsePlayerCursor);

			u16 move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
			switch (EffectiveMove(move)) {
				case MOVE_ACUPRESSURE:
					if (!(gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor ^ BIT_FLANK]))
					{
						gMultiUsePlayerCursor ^= BIT_FLANK;
					}
					gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
					return;

				case MOVE_POLLENPUFF:
					if (IsHealBlocked(gActiveBattler) //Affected by Heal Block can't target partner
					&& !(gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor ^ 2]))
					{
						gMultiUsePlayerCursor ^= BIT_FLANK;
						gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
						return;
					}
			}

			for (i = 0; i < MAX_BATTLERS_COUNT; i++)
			{
				if (currSelIdentity == identities[i])
					break;
			}
			do
			{
				if (++i > 3)
					i = 0;
				gMultiUsePlayerCursor = GetBattlerAtPosition(identities[i]);
			} while (gMultiUsePlayerCursor == gBattlersCount);

			i = 0;
			move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
			switch (GetBattlerPosition(gMultiUsePlayerCursor)) {
				case B_POSITION_PLAYER_LEFT:
				case B_POSITION_PLAYER_RIGHT:
					if (gActiveBattler != gMultiUsePlayerCursor)
						i++;
					else if (GetBaseMoveTarget(move, gActiveBattler) & MOVE_TARGET_USER_OR_PARTNER)
						i++;

					if ((moveInfo->dynamaxed || gNewBS->dynamaxData.viewing || move == MOVE_DOODLE) && gMultiUsePlayerCursor == PARTNER(gActiveBattler))
						i = FALSE; //Can't use Max Move on partner
					break;
				case B_POSITION_OPPONENT_LEFT:
				case B_POSITION_OPPONENT_RIGHT:
					i++;
					break;
			}

			if (gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor])
				i = 0;

		} while (i == 0);

		gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget; //sub_8039AD8 in Emerald
		MoveSelectionDisplayMoveEffectiveness();
	}
}

static void HighlightPossibleTargets(void)
{
	u32 bitMask = 0;
	u8 startY = 0;

	if (IS_DOUBLE_BATTLE)
	{
		u8 moveTarget;
		struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleBufferA[gActiveBattler][4]);
		u16 chosenMove = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];

		if (MoveIs(chosenMove, MOVE_CURSE))
		{
			if (TeraTypeActive(gActiveBattler))
			{
				if (moveInfo->monTeraType != TYPE_GHOST)
					moveTarget = MOVE_TARGET_USER;
				else
					moveTarget = MOVE_TARGET_SELECTED;
			}
			else if (moveInfo->monType1 != TYPE_GHOST&& moveInfo->monType2 != TYPE_GHOST && moveInfo->monType3 != TYPE_GHOST)
				moveTarget = MOVE_TARGET_USER;
			else
				moveTarget = MOVE_TARGET_SELECTED;
		}
		else
			moveTarget = GetBaseMoveTarget(chosenMove, gActiveBattler);

		if (gNewBS->zMoveData.viewing && moveInfo->moveSplit[gMoveSelectionCursor[gActiveBattler]] != SPLIT_STATUS) //Status moves keep original targets
			moveTarget = gBattleMoves[moveInfo->possibleZMoves[gMoveSelectionCursor[gActiveBattler]]].target;

		if (gNewBS->dynamaxData.viewing || moveInfo->dynamaxed)
			moveTarget = gBattleMoves[moveInfo->possibleMaxMoves[gMoveSelectionCursor[gActiveBattler]]].target;

		switch (moveTarget)
		{
			case MOVE_TARGET_SELECTED:
			case MOVE_TARGET_DEPENDS:
			case MOVE_TARGET_USER_OR_PARTNER:
			case MOVE_TARGET_RANDOM:
				bitMask = 0xF0000;
				startY = 0;
				break;
			case MOVE_TARGET_BOTH:
			case MOVE_TARGET_OPPONENTS_FIELD:
				bitMask = (gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)] 
						 | gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)]) << 16; 
				startY = 8;
				break;
			case MOVE_TARGET_USER:
				switch (EffectiveMove(chosenMove))
				{
					//Moves that affect the whole field
					case MOVE_HAZE:
					case MOVE_SANDSTORM:
					case MOVE_PERISHSONG:
					case MOVE_RAINDANCE:
					case MOVE_SUNNYDAY:
					case MOVE_HAIL:
					case MOVE_MUDSPORT:
					case MOVE_WATERSPORT:
					case MOVE_FLOWERSHIELD:
					case MOVE_ROTOTILLER:
					case MOVE_ELECTRICTERRAIN:
					case MOVE_GRASSYTERRAIN:
					case MOVE_MISTYTERRAIN:
					case MOVE_PSYCHICTERRAIN:
					case MOVE_TRICKROOM:
					case MOVE_MAGICROOM:
					case MOVE_WONDERROOM:
					case MOVE_GRAVITY:
					case MOVE_IONDELUGE:
					case MOVE_FAIRYLOCK:
					case MOVE_TEATIME:
					case MOVE_COURTCHANGE:
						bitMask = 0xF0000;
						break;

					//Moves that affect the user's side
					case MOVE_HOWL:
					case MOVE_SAFEGUARD:
					case MOVE_REFLECT:
					case MOVE_LIGHTSCREEN:
					case MOVE_MIST:
					case MOVE_AURORAVEIL:
					case MOVE_HEALBELL:
					case MOVE_AROMATHERAPY:
					case MOVE_TAILWIND:
					case MOVE_LUCKYCHANT:
					case MOVE_CRAFTYSHIELD:
					case MOVE_MATBLOCK:
					case MOVE_QUICKGUARD:
					case MOVE_WIDEGUARD:
					case MOVE_GEARUP:
					case MOVE_MAGNETICFLUX:
					case MOVE_LIFEDEW:
					case MOVE_JUNGLEHEALING:
						bitMask = (gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)] 
								 | gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)]) << 16; 
						break;

					//Moves the affect the user's partner
					case MOVE_HELPINGHAND:
					case MOVE_AROMATICMIST:
					case MOVE_COACHING:
						bitMask = (gBitTable[GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK)]) << 16;
						break;
					default:
						bitMask = (gBitTable[gActiveBattler]) << 16;
						break;
				}
				startY = 8;
				break;
			case MOVE_TARGET_FOES_AND_ALLY:
				bitMask = (gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)] 
						 | gBitTable[GetBattlerAtPosition(GetBattlerPosition(gActiveBattler) ^ BIT_FLANK)] 
						 | gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)]) << 16;
				startY = 8;
				break;
		}

		BeginNormalPaletteFade(bitMask, 8, startY, 0, RGB_WHITE);
	}
}

u8 TrySetCantSelectMoveBattleScript(void)
{
	u8 limitations, holdEffect, isAnyMaxMove;
	u16 move, *choicedMove, ability;

	limitations = 0; isAnyMaxMove = FALSE;
	ability = ABILITY(gActiveBattler);
	holdEffect = ITEM_EFFECT(gActiveBattler);
	move = gBattleMons[gActiveBattler].moves[gBattleBufferB[gActiveBattler][2]];
	choicedMove = &gBattleStruct->choicedMove[gActiveBattler];

	gStringBank = gActiveBattler;
	gBattleScripting.bank = gActiveBattler;
	gCurrentMove = move;

	if (RAID_BATTLE_END) //Fix bug where partner would be forced to use a move with 0 PP and cause the game to crash
		return FALSE;

	if (gNewBS->NoMoreMovingThisTurn & gBitTable[gActiveBattler]) //Fix a bug where partner is forced to use a move they can't in wild doubles
		return FALSE;

	if (IsDynamaxed(gActiveBattler) || gNewBS->dynamaxData.toBeUsed[gActiveBattler])
		move = gCurrentMove = GetMaxMoveByMove(gActiveBattler, move);

	if (IsAnyMaxMove(move))
		isAnyMaxMove = TRUE; //Save time later

	if (gDisableStructs[gActiveBattler].encoredMove
	&& gDisableStructs[gActiveBattler].encoredMove != move
	&& !gNewBS->zMoveData.toBeUsed[gActiveBattler] //Can still use other Z-Moves even if encored
	&& !isAnyMaxMove)
	{
		gCurrentMove = gDisableStructs[gActiveBattler].encoredMove;
		gSelectionBattleScripts[gActiveBattler] = BattleScript_MustSelectEncoredMove;
		++limitations;
	}
	else if (gDisableStructs[gActiveBattler].disabledMove == move && move != MOVE_NONE)
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingDisabledMove;
		++limitations;
	}
	else if (IsTormented(gActiveBattler) && !isAnyMaxMove && move == gLastUsedMoves[gActiveBattler] && move != MOVE_STRUGGLE)
	{
		CancelMultiTurnMoves(gActiveBattler);
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingTormentedMove;
		++limitations;
	}
	else if (IsTaunted(gActiveBattler) != 0 && SPLIT(move) == SPLIT_STATUS)
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveTaunt;
		limitations++;
	}
	else if (IsImprisoned(gActiveBattler, move) && !isAnyMaxMove)
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingImprisionedMove;
		++limitations;
	}
	else if (!isAnyMaxMove && ability == ABILITY_GORILLATACTICS && *choicedMove != 0 && *choicedMove != 0xFFFF && *choicedMove != move)
	{
		gCurrentMove = *choicedMove;
		gLastUsedAbility = ability;
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveChoiceAbility;
		++limitations;
	}
	else if (!isAnyMaxMove && holdEffect == ITEM_EFFECT_CHOICE_BAND && *choicedMove != 0 && *choicedMove != 0xFFFF && *choicedMove != move)
	{
		gCurrentMove = *choicedMove;
		gLastUsedItem = ITEM(gActiveBattler);
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveChoiceItem;
		++limitations;
	}
	else if (holdEffect == ITEM_EFFECT_ASSAULT_VEST && IsMoveBannedByAssaultVest(move))
	{
		gLastUsedItem = ITEM(gActiveBattler);
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedMoveAssaultVest;
		++limitations;
	}
	#ifdef FLAG_SKY_BATTLE
	else if (!gNewBS->zMoveData.toBeUsed[gActiveBattler] //Can still use status Z-Moves even during Sky Battle
	&& FlagGet(FLAG_SKY_BATTLE) && CheckTableForMove(move, gSkyBattleBannedMoves))
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedSkyBattle;
		++limitations;
	}
	#endif
	else if (!gNewBS->zMoveData.toBeUsed[gActiveBattler]
	&& IsGravityActive() && CheckTableForMove(move, gGravityBannedMoves))
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedGravity;
		++limitations;
	}
	else if (!gNewBS->zMoveData.toBeUsed[gActiveBattler]
	&& CantUseSoundMoves(gActiveBattler) && CheckSoundMove(move))
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedThroatChop;
		++limitations;
	}
	else if (!gNewBS->zMoveData.toBeUsed[gActiveBattler]
	&& IsHealBlocked(gActiveBattler) && CheckHealingMove(move))
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedHealBlock;
		++limitations;
	}
	else if (HackFuuin_MoveSealed(gActiveBattler, move)) //[hack] patches/0560 ふういんルーレット: 封印された技は選べない（全部封印なら 素の わるあがき）
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingSealedMove;
		++limitations;
	}
	else if (gBattleMons[gActiveBattler].pp[gBattleBufferB[gActiveBattler][2]] == 0)
	{
		gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingMoveWithNoPP;
		++limitations;
	}
	else if(move == MOVE_GIGATONHAMMER && gLastPrintedMoves[gActiveBattler] == MOVE_GIGATONHAMMER)
    {
        gCurrentMove = MOVE_GIGATONHAMMER;
        gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedGigatonHammer;
        ++limitations;
    }
	else if(move == MOVE_BLOODMOON && gLastPrintedMoves[gActiveBattler] == MOVE_BLOODMOON)
    {
        gCurrentMove = MOVE_BLOODMOON;
        gSelectionBattleScripts[gActiveBattler] = BattleScript_SelectingNotAllowedGigatonHammer;
        ++limitations;
    }
	
	if (limitations != 0)
	{
		gNewBS->dynamaxData.toBeUsed[gActiveBattler] = FALSE;
		gNewBS->zMoveData.toBeUsed[gActiveBattler] = FALSE;
	}

	return limitations;
}

void PlayerHandleChooseAction(void)
{
	int i;
	u16 itemId = gBattleBufferA[gActiveBattler][2] | (gBattleBufferA[gActiveBattler][3] << 8);
	bool8 raidBattleEnd = RAID_BATTLE_END;

	gNewBS->zMoveData.toBeUsed[gActiveBattler] = FALSE;

	if (IS_DOUBLE_BATTLE)
	{
		if (raidBattleEnd && gActiveBattler == GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT))
		{
			if ((gBattleBufferA[gActiveBattler][1] == ACTION_USE_ITEM && GetPocketByItemId(itemId) == POCKET_POKEBALLS)
			|| gBattleBufferA[gActiveBattler][1] == ACTION_RUN)
			{
				gNewBS->NoMoreMovingThisTurn |= gBitTable[gActiveBattler];
				EmitTwoReturnValues(1, ACTION_USE_MOVE, 0);
				PlayerBufferExecCompleted();
				return;
			}
			else
			{
				EmitTwoReturnValues(1, ACTION_CANCEL_PARTNER, 0);
				PlayerBufferExecCompleted();
				return;
			}
		}
		else
		{
			//Running or using balls cancels the second mon's attack
			if (!IsBagDisabled()
			&&  gBattleBufferA[gActiveBattler][1] == ACTION_USE_ITEM //If mon 1 used a ball, then
			&&  GetPocketByItemId(itemId) == POCKET_POKEBALLS		 //mon 2 doesn't get to do anything.
			&&  !HackSnatch_IsSnatchBall(itemId))					 //[hack] patches/0961 ただしスナッチボールは戦闘が続くので 相方は普通に行動する
			{
				gNewBS->NoMoreMovingThisTurn |= gBitTable[gActiveBattler];

				if (raidBattleEnd) //Raid mon can't move if it's dead
					gNewBS->NoMoreMovingThisTurn |= gBitTable[BANK_RAID_BOSS];

				EmitTwoReturnValues(1, ACTION_USE_MOVE, 0);
				PlayerBufferExecCompleted();
				return;
			}
			else if (GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT) == gActiveBattler
			&& gBattleBufferA[gActiveBattler][1] == ACTION_RUN)
			{
				gNewBS->NoMoreMovingThisTurn |= gBitTable[gActiveBattler];
				EmitTwoReturnValues(1, ACTION_USE_MOVE, 0);
				PlayerBufferExecCompleted();
				return;
			}
		}
	}

	gBattlerControllerFuncs[gActiveBattler] = HandleChooseActionAfterDma3;

	if (raidBattleEnd) //Time to catch Raid opponent
		BattlePutTextOnWindow(gText_BattleMenuRaidEnd, 2);
	else if ((IS_DOUBLE_BATTLE)
	&& GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT
	&& !(gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)])
	&& !(gNewBS->commandingDondozo & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)])
	&& !(gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER))
	&& gBattleBufferA[gActiveBattler][1] != ACTION_USE_ITEM) //Mon 1 didn't use item
	{
		if (IsBagDisabled()) //Grey out bag text to indicate no point in pressing it
		{
			BattlePutTextOnWindow(gText_BattleMenu2NoItems, 2);
			LoadShadowColourForGreyedOutBagText();
		}
		else
			BattlePutTextOnWindow(gText_BattleMenu2, 2);
	}
	else
	{
		if (IsBagDisabled())
		{
			BattlePutTextOnWindow(gText_BattleMenuNoItems, 2);
			LoadShadowColourForGreyedOutBagText();
		}
		else
			BattlePutTextOnWindow(gText_BattleMenu, 2);
	}

	for (i = 0; i < MAX_MON_MOVES; i++)
		ActionSelectionDestroyCursorAt(i);

	ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);

	if (raidBattleEnd)
		BattleStringExpandPlaceholdersToDisplayedString(gText_CaptureRaidMon);
	else
		BattleStringExpandPlaceholdersToDisplayedString(gText_WhatWillPkmnDo);
	BattlePutTextOnWindow(gDisplayedStringBattle, 1);

	#ifdef LAST_USED_BALL_TRIGGER
	TryLoadLastUsedBallTrigger();
	#endif
}

static void LoadShadowColourForGreyedOutBagText(void)
{
	gPlttBufferUnfaded[5 * 0x10 + 11] = RGB(28, 28, 27); //Grey shadow colour
	CpuCopy16(&gPlttBufferUnfaded[5 * 0x10 + 11], &gPlttBufferFaded[5 * 0x10 + 11], sizeof(u16));
}

void HandleInputChooseAction(void)
{
	bool8 choose = (gMain.newKeys & A_BUTTON) != 0;

	DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
	DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);

	//[hack] patches/0199 2026-10-07 ユーザー「たたかう にげる の4つが表示されているときにショートカットでにげられるようにしてほしい」「R ですぐ逃げる」:
	//  R でカーソルを「にげる」へ移して、A で「にげる」を選んだのと同じに（トレーナー戦の にげられない・ダブルの 2 匹目・レイドの終わりも A と同じ扱い）
	if (gMain.newKeys & R_BUTTON)
	{
		u8 run = RAID_BATTLE_END ? 1 : 3;

		if (gActionSelectionCursor[gActiveBattler] != run)
		{
			ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
			gActionSelectionCursor[gActiveBattler] = run;
			ActionSelectionCreateCursorAt(run, 0);
		}
		choose = TRUE;
	}

	if (choose)
	{
		PlaySE(SE_SELECT);

		if (RAID_BATTLE_END)
		{
			switch (gActionSelectionCursor[gActiveBattler]) {
				case 0:
					EmitTwoReturnValues(1, ACTION_USE_ITEM, 0);
					break;
				case 1:
					EmitTwoReturnValues(1, ACTION_RUN, 0);
					break;
			}
		}
		else
		{
			switch (gActionSelectionCursor[gActiveBattler]) {
				case 0:
					EmitTwoReturnValues(1, ACTION_USE_MOVE, 0);
					break;
				case 1:
					gNewBS->megaData.chosen[gActiveBattler] = FALSE;
					gNewBS->ultraData.chosen[gActiveBattler] = FALSE;
					gNewBS->terastalData.chosen[gActiveBattler] = FALSE;
					EmitTwoReturnValues(1, ACTION_USE_ITEM, 0);
					break;
				case 2:
					gNewBS->megaData.chosen[gActiveBattler] = FALSE;
					gNewBS->ultraData.chosen[gActiveBattler] = FALSE;
					gNewBS->terastalData.chosen[gActiveBattler] = FALSE;
					if (HackChallengeB_SwitchBlocked()) //[hack] patches/0540 しばり B こうたいきんし: 入れ替えない（音と一言・もう一度えらべる）
						{
							HackChallengeB_ShowSwitchBlocked();
							return;
						}
						EmitTwoReturnValues(1, ACTION_SWITCH, 0);
					break;
				case 3:
					gNewBS->megaData.chosen[gActiveBattler] = FALSE;
					gNewBS->ultraData.chosen[gActiveBattler] = FALSE;
					gNewBS->terastalData.chosen[gActiveBattler] = FALSE;

					if ((IS_DOUBLE_BATTLE)
					&& GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT
					&& !(gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)])
					&& !(gNewBS->commandingDondozo & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)])
					&& !(gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER))
					&& gBattleBufferA[gActiveBattler][1] != ACTION_USE_ITEM) //Mon 1 didn't use item
						goto CANCEL_PARTNER;

					EmitTwoReturnValues(1, ACTION_RUN, 0);
					break;
			}
		}

		PlayerBufferExecCompleted();
	}
	else if (gMain.newKeys & DPAD_LEFT)
	{
		if (gActionSelectionCursor[gActiveBattler] & 1) // if is B_ACTION_USE_ITEM or B_ACTION_RUN
		{
			PlaySE(SE_SELECT);
			ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
			gActionSelectionCursor[gActiveBattler] ^= 1;
			ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
		}
	}
	else if (gMain.newKeys & DPAD_RIGHT)
	{
		if (!(gActionSelectionCursor[gActiveBattler] & 1)) // if is B_ACTION_USE_MOVE or B_ACTION_SWITCH
		{
			PlaySE(SE_SELECT);
			ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
			gActionSelectionCursor[gActiveBattler] ^= 1;
			ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
		}
	}
	else if (gMain.newKeys & DPAD_UP)
	{
		if (!RAID_BATTLE_END && gActionSelectionCursor[gActiveBattler] & 2) // if is B_ACTION_SWITCH or B_ACTION_RUN
		{
			PlaySE(SE_SELECT);
			ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
			gActionSelectionCursor[gActiveBattler] ^= 2;
			ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
		}
	}
	else if (gMain.newKeys & DPAD_DOWN)
	{
		if (!RAID_BATTLE_END && !(gActionSelectionCursor[gActiveBattler] & 2)) // if is B_ACTION_USE_MOVE or B_ACTION_USE_ITEM
		{
			PlaySE(SE_SELECT);
			ActionSelectionDestroyCursorAt(gActionSelectionCursor[gActiveBattler]);
			gActionSelectionCursor[gActiveBattler] ^= 2;
			ActionSelectionCreateCursorAt(gActionSelectionCursor[gActiveBattler], 0);
		}
	}
	else if (gMain.newKeys & B_BUTTON)
	{
		if ((IS_DOUBLE_BATTLE)
		 && GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT
		 && !(gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)])
		 && !(gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)))
		{
		CANCEL_PARTNER:
			if (gBattleBufferA[gActiveBattler][1] == ACTION_USE_ITEM)
				return;

			PlaySE(SE_SELECT);
			gNewBS->dynamaxData.toBeUsed[PARTNER(gActiveBattler)] = FALSE; //Cancel's Dynamax if partner selected it
			gNewBS->terastalData.chosen[PARTNER(gActiveBattler)] = FALSE;
			EmitTwoReturnValues(1, ACTION_CANCEL_PARTNER, 0);
			PlayerBufferExecCompleted();
		}
	}
	else if (gMain.newKeys & START_BUTTON)
	{
		SwapHpBarsWithHpText();
	}
	else if (gMain.newKeys & L_BUTTON)
	{
		#ifdef LAST_USED_BALL_TRIGGER
		if (!CantLoadLastBallTrigger()) //Can use last ball
		{
			if (IsPlayerPartyAndPokemonStorageFull())
				PlaySE(SE_ERROR);
			else
			{
				PlaySE(SE_SELECT);
				gSpecialVar_ItemId = GetLastUsedBall();
				RemoveBagItem(gSpecialVar_ItemId, 1);
				gNewBS->usedLastBall = TRUE;
				gNewBS->megaData.chosen[gActiveBattler] = FALSE;
				gNewBS->ultraData.chosen[gActiveBattler] = FALSE;
				gNewBS->terastalData.chosen[gActiveBattler] = FALSE;
				EmitTwoReturnValues(1, ACTION_USE_ITEM, 0);
				PlayerBufferExecCompleted();
			}
			
			return; //The Team Preview trigger check is unimportant
		}
		#endif
	}
}

bool8 CheckCantMoveThisTurn(void)
{
	if (gNewBS->NoMoreMovingThisTurn & gBitTable[gActiveBattler])
	{
		EmitMoveChosen(1, 0, gMultiUsePlayerCursor, FALSE, FALSE, FALSE, FALSE, FALSE);
		return TRUE;
	}
	return FALSE;
}

bool8 IsBagDisabled(void)
{
	if (HackChallenge_BagClosedInBattle()) //[hack] patches/0510 アイテムきんし: トレーナー戦はバッグごと閉じる（野生はボールのために開く）
		return TRUE;
	if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER))   //[hack] 2026-10-11 野生戦でも数える（ボールは数に入れない＝battle_start_turn_start.c）
	&& !FlagGet(FLAG_BATTLE_FACILITY)
	&& HackDifficulty_BagLimitReached(VarGet(HOENN_VAR_DIFFICULTY), gNewBS->playerItemUsedCount))
		return TRUE;
	#ifdef VAR_GAME_DIFFICULTY
	u8 itemRestrictions = 0;
	u8 difficulty = VarGet(VAR_GAME_DIFFICULTY);

	#ifdef VAR_ITEM_RESTRICTIONS
	itemRestrictions = VarGet(VAR_ITEM_RESTRICTIONS);
	#endif

	if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
	{
		//Determine based on player-set restrictions
		if (itemRestrictions >= OPTIONS_ITEM_RESTRICTIONS_NO_ITEMS)
			return TRUE;
		else if (itemRestrictions == OPTIONS_ITEM_RESTRICTIONS_4_ITEMS
		&& gNewBS->playerItemUsedCount >= 4)
			return TRUE;

		//Determine based on difficulty set restrictions
		if (difficulty == OPTIONS_HARD_DIFFICULTY)
		{
			if (gNewBS->playerItemUsedCount >= 4) //Max four items can be used
				return TRUE;
		}
		else if (difficulty >= OPTIONS_EXPERT_DIFFICULTY) //No items in battles for Insane players
			return TRUE;
	}
	else
	{
		if (!IsRaidBattle()
		&& CantCatchBecauseFlag() //Can't be caught
		&& (difficulty >= OPTIONS_EXPERT_DIFFICULTY //No items in battles for Insane players
		 || itemRestrictions >= OPTIONS_ITEM_RESTRICTIONS_NO_ITEMS))
			return TRUE;
	}
	#endif

	return FlagGet(FLAG_DISABLE_BAG) || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_EREADER_TRAINER))
	    || ((gBattleTypeFlags & BATTLE_TYPE_FRONTIER) && !HoennPyramid_BagUsableInBattle()); //[hack] patches/0131 ピラミッドの中はバトルバッグを使える
}

static void TryLoadTypeIcons(void)
{
	#ifndef HEALTHBAR_TYPE_ICONS
	if (((gBattleTypeFlags & BATTLE_TYPE_CAMOMONS)
	#ifdef FLAG_HEALTHBAR_TYPE_ICONS
	|| FlagGet(FLAG_HEALTHBAR_TYPE_ICONS)
	#endif
	)
	&&
	#else
	if (
	#endif
	!(gBattleTypeFlags & BATTLE_TYPE_LINK)
	&& IndexOfSpritePaletteTag(TYPE_ICON_TAG) == 0xFF)
	{
		LoadSpritePalette(&sTypeIconPalTemplate);
		LoadSpritePalette(&sTypeIconPalTemplate2);
		for (u8 position = 0; position < gBattlersCount; ++position)
		{
			if (!BATTLER_ALIVE(GetBattlerAtPosition(position)))
				continue;

			for (u8 typeNum = 0; typeNum < 2; ++typeNum) //Load each type
			{
				u8 spriteId;
				s16 x = sTypeIconPositions[position][IS_SINGLE_BATTLE].x;
				s16 y = sTypeIconPositions[position][IS_SINGLE_BATTLE].y + (11 * typeNum); //2nd type is 13px below

				u8* type1Ptr;
				if (gStatuses3[GetBattlerAtPosition(position)] & STATUS3_ILLUSION && !(gBattleTypeFlags & BATTLE_TYPE_CAMOMONS))
					type1Ptr = &gBaseStats[GetIllusionPartyData(GetBattlerAtPosition(position))->species].type1;
				else
					type1Ptr = &gBattleMons[GetBattlerAtPosition(position)].type1;

				u8 type = *(type1Ptr + typeNum);

				switch (type) { //Certain types have a different palette
					case TYPE_FLYING:
					case TYPE_POISON:
					case TYPE_GROUND:
					case TYPE_DRAGON:
					case TYPE_MYSTERY:
					case TYPE_ROOSTLESS:
					case TYPE_BLANK:
					case TYPE_RIL: //[hack] patches/0408 #110: リルの札は CamomonsTypeIcons2（パレット 2）の絵
					case TYPE_SHADOW: //[hack] patches/0955 段 D: ダークの札も CamomonsTypeIcons2
						spriteId = CreateSpriteAtEnd(&sTypeIconSpriteTemplate2, x, y, 0xFF);
						break;
					default:
						spriteId = CreateSpriteAtEnd(&sTypeIconSpriteTemplate, x, y, 0xFF);
				}

				if (spriteId != MAX_SPRITES)
				{
					struct Sprite* sprite = &gSprites[spriteId];
					sprite->data[0] = position;
					sprite->data[1] = gActiveBattler;
					sprite->data[3] = y; //Save original y-value for bouncing

					if (IS_SINGLE_BATTLE)
					{
						if (SIDE(GetBattlerAtPosition(position)) == B_SIDE_PLAYER)
							SetSpriteOamFlipBits(sprite, TRUE, FALSE); //Flip horizontally
					}
					else //Double Battle
					{
						if (SIDE(GetBattlerAtPosition(position)) == B_SIDE_OPPONENT)
							SetSpriteOamFlipBits(sprite, TRUE, FALSE); //Flip horizontally
					}

					RequestSpriteFrameImageCopy(type, sprite->oam.tileNum, sprite->images);
				}
			}
		}
	}
}

static void SpriteCB_CamomonsTypeIcon(struct Sprite* sprite)
{
	u8 position = sprite->data[0];
	u8 bank = sprite->data[1];

	if (sprite->data[2] == 10)
	{
		FreeSpritePaletteByTag(TYPE_ICON_TAG);
		DestroySprite(sprite);
		return;
	}

	//Type icons should prepare to destroy themselves if the Player is not choosing an action
	if (gBattlerControllerFuncs[bank] != (void*) (0x08032404 | 1) //PlayerHandleChooseMove
	&&  gBattlerControllerFuncs[bank] != (void*) (0x0802E1EC | 1) //HandleInputChooseMove
	&&  gBattlerControllerFuncs[bank] != (void*) (0x080323C0 | 1) //HandleChooseMoveAfterDma3
	&&  gBattlerControllerFuncs[bank] != HandleInputChooseMove
	&&  gBattlerControllerFuncs[bank] != HandleInputChooseTarget
	&&  gBattlerControllerFuncs[bank] != HandleMoveSwitching
	&&  gBattlerControllerFuncs[bank] != HandleInputChooseMove)
	{
		if (IS_SINGLE_BATTLE)
		{
			switch (position) {
				case B_POSITION_PLAYER_LEFT:
					sprite->pos1.x -= 1;
					break;
				case B_POSITION_OPPONENT_LEFT:
					sprite->pos1.x += 1;
					break;
			}
		}
		else //Double Battle
		{
			switch (position) {
				case B_POSITION_PLAYER_LEFT:
				case B_POSITION_PLAYER_RIGHT:
					sprite->pos1.x += 1;
					break;
				case B_POSITION_OPPONENT_LEFT:
				case B_POSITION_OPPONENT_RIGHT:
					sprite->pos1.x -= 1;
					break;
			}
		}

		++sprite->data[2];
		return;
	}

	if (IS_SINGLE_BATTLE)
	{
		switch (position) {
			case B_POSITION_PLAYER_LEFT:
				if (sprite->pos1.x < sTypeIconPositions[position][TRUE].x + 10)
					sprite->pos1.x += 1;
				break;
			case B_POSITION_OPPONENT_LEFT:
				if (sprite->pos1.x > sTypeIconPositions[position][TRUE].x - 10)
					sprite->pos1.x -= 1;
				break;
		}
	}
	else //Double Battle
	{
		switch (position) {
			case B_POSITION_PLAYER_LEFT:
			case B_POSITION_PLAYER_RIGHT:
				if (sprite->pos1.x > sTypeIconPositions[position][FALSE].x - 10)
					sprite->pos1.x -= 1;
				break;
			case B_POSITION_OPPONENT_LEFT:
			case B_POSITION_OPPONENT_RIGHT:
				if (sprite->pos1.x < sTypeIconPositions[position][FALSE].x + 10)
					sprite->pos1.x += 1;
				break;
		}
	}

	//Deal with bouncing player healthbox
	s16 originalY = sprite->data[3];
	struct Sprite* healthbox = &gSprites[gHealthboxSpriteIds[GetBattlerAtPosition(position)]];
	sprite->pos1.y = originalY + healthbox->pos2.y;
}
