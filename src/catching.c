#include "defines.h"
#include "hack_base_stats_wide.h" //[hack] patches/0678 種族値の u16 の窓口
#include "hack_dex_male.h" //[hack] patches/0945 図鑑の絵は♂（ふつう）
#include "defines_battle.h"
#include "../include/battle_anim.h"
#include "../include/bg.h"
#include "../include/event_data.h"
#include "../include/field_player_avatar.h"
#include "../include/field_specials.h"
#include "../include/item.h"
#include "../include/overworld.h"
#include "../include/random.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/constants/game_stat.h"
#include "../include/constants/items.h"
#include "../include/constants/songs.h"

#include "../include/new/battle_util.h"
#include "../include/new/catching.h"
#include "../include/new/dns.h"
#include "../include/new/dynamax.h"
#include "../include/new/end_battle_battle_scripts.h"
#include "../include/new/form_change.h"
#include "../include/new/util.h"
#include "../include/new/mega.h"
#include "../include/new/pokemon_storage_system.h"
#include "../include/new/terastal.h"
#include "hack_challenge.h" //[hack] patches/0510 しばり
#include "hack_snatch_ball.h" //[hack] patches/0961 スナッチボール
#include "hack_delta.h" //[hack] patches/0981 手持ちにヌシは 1 匹だけ
#include "shadow_relive.h" //[hack] リライブボックス（ダークを捕まえたときの送り先の箱の名前）
/*
catching.c
	handles the catch probability logic, expands pokeballs, etc.
*/

#define gOpenPokeballGfx (u8*) 0x8C022E8

#define BattleScript_TutorialThrow 		((u8*) 0x81BDA44)
#define BattleScript_ShakeBallThrow 	((u8*) 0x81BDA4F)
#define BattleScript_TrainerBallBlock 	((u8*) 0x81BDA7D)
#define BattleScript_DodgedBall 		((u8*) 0x81BDA8D)
extern const u8 BattleScript_RaidMonEscapeBall[];
extern const u8 BattleScript_OneShotEscapeBall[]; //[hack] patches/0951（仮）assembly/hack_challenge/challenge_f_blind_scripts.s
#include "hack_challenge_f_blind.h" //[hack] patches/0951（仮）しばり「いっぱつしょうぶ」
extern bool8 HackLegendRaid_Active(void); //[hack] patches/0181
u32 HackDifficulty_CatchOdds(u32 odds); //[hack] patches/0326 なんいどが低いほど捕まえやすい（src/hack_difficulty.c）
#include "hack_mash_catch.h" //[hack] patches/0423 A 連打の補正（隠し機能）
#ifdef HACK_DEBUG_MENU
#include "hack_debug_catch_info.h" //[hack] patches/0425 デバッグ版だけ: 戦闘中の捕獲情報
#endif

#define sBallCatchBonuses ((u8*) 0x820D29A)

extern const struct CompressedSpriteSheet gBallSpriteSheets[];
extern const struct CompressedSpritePalette gBallSpritePalettes[];
extern const struct SpriteTemplate gBallSpriteTemplates[];

#ifdef CAPTURE_EXPERIENCE
	extern u8 BattleScript_SuccessBallThrow[];
#else
	#define BattleScript_SuccessBallThrow ((u8*) 0x81BD9FE)
#endif

extern const u8 gText_Relive_BoxName[]; //[hack] patches/0975 strings/hack_shadow_relive.string
extern const u8 gText_CantAimAtTwoTargets[];
extern const u8 gText_CantAimAtSemiInvulnerableTarget[];
extern const u8 gText_CantUseAMoveAndABall[];
extern const u8 gText_CantCatchPokemonYet[];
extern const u8 gText_CantCatchPokemonRightNow[];

extern species_t gUltraBeastList[];

//This file's functions:
static u8 GetCatchingBattler(void);
static bool8 CriticalCapture(u32 odds);
static u32 CatchOddsAfterModifiers(u32 odds, u8 ballType, u8 bankAtk, u8 bankDef); //[hack] patches/0425
static u32 CatchShakeThreshold(u32 odds); //[hack] patches/0425
static void SnatchBallThrow(u8 ballType); //[hack] patches/0961

void atkEF_handleballthrow(void)
{
	if (gBattleExecBuffer) return;

	gActiveBattler = gBankAttacker;
	gBankTarget = GetCatchingBattler();

	u8 ballType = ItemId_GetType(gLastUsedItem);
	HackMashCatch_Finish(); //[hack] patches/0423 A 連打を数え終わる（数えていたのはターンの始めからのタスク・待ちは足さない）
	gNewBS->threwBall = TRUE;
	if (ballType != BALL_TYPE_QUICK_BALL //Useless to offer to player after initial use
	|| gBattleResults.battleTurnCounter > 0) //Unless the player explictly chose it after the first turn
		gLastUsedBall = gLastUsedItem;

	if (gNewBS->isTrainerBattle)
	{
		#ifdef FLAG_CATCH_TRAINERS_POKEMON
			//[hack] patches/0961 スナッチボール: 相手のダークの個体に投げたら 旗の有無によらず 野生と同じ捕獲の判定（GetBaseBallCatchOdds が クイック・ハイパー・タイマーの いちばん高い倍率）。
			//捕まえたら戦闘は続く（台本 BattleScript_SnatchCaught）・外れたら普通のボールと同じ（揺れて出てきて 相手のダークは戦闘を続ける・BattleScript_ShakeBallThrow）。
			//ダークがもういなければ 投げない（ボールは返す）。旗（0x905・デバッグ用）が立っているときのダーク以外は今までどおり（普通のボールで全員捕まえる・戦闘は終わる）
			if (HackSnatch_IsSnatchBall(gLastUsedItem) && HackSnatch_FindDarkFoe() == HACK_SNATCH_NO_FOE && !FlagGet(FLAG_CATCH_TRAINERS_POKEMON))
			{
				gBattlescriptCurrInstr = BattleScript_SnatchNoTarget;
			}
			else if (HackSnatch_IsSnatchBall(gLastUsedItem) && HackSnatch_FindDarkFoe() != HACK_SNATCH_NO_FOE)
			{
				SnatchBallThrow(ballType); //[hack] patches/0961 野生と同じ捕獲の判定（下の関数）
			}
			else if (FlagGet(FLAG_CATCH_TRAINERS_POKEMON))
			{
				EmitBallThrowAnim(0, 4);
				MarkBufferBankForExecution(gActiveBattler);
				gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
				if (ballType != BALL_TYPE_PARK_BALL)
					SetMonData(GetBankPartyData(gBankTarget), MON_DATA_POKEBALL, &ballType);
				if (CalculatePlayerPartyCount() == 6)
					gBattleCommunication[MULTISTRING_CHOOSER] = 0;
				else
					gBattleCommunication[MULTISTRING_CHOOSER] = 1;
			}
			else
			{
				EmitBallThrowAnim(0, 5);
				MarkBufferBankForExecution(gActiveBattler);
				gBattlescriptCurrInstr = BattleScript_TrainerBallBlock;
			}
		#else
			EmitBallThrowAnim(0, 5);
			MarkBufferBankForExecution(gActiveBattler);
			gBattlescriptCurrInstr = BattleScript_TrainerBallBlock;
		#endif
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_GHOST
	#ifdef FLAG_NO_CATCHING
	|| FlagGet(FLAG_NO_CATCHING)
	#endif
	#ifdef FLAG_NO_CATCHING_AND_RUNNING
	|| FlagGet(FLAG_NO_CATCHING_AND_RUNNING)
	#endif
	)
	{
		EmitBallThrowAnim(0, 6);
		MarkBufferBankForExecution(gActiveBattler);
		gBattlescriptCurrInstr = BattleScript_DodgedBall;
	}
	else if (gBattleTypeFlags & (BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_POKE_DUDE))
	{
		EmitBallThrowAnim(0, 4);
		MarkBufferBankForExecution(gActiveBattler);
		gBattlescriptCurrInstr = BattleScript_TutorialThrow;
	}
	else
	{
		u32 odds = GetBaseBallCatchOdds(ballType, gBankAttacker, gBankTarget);
		HackMashCatch_Clear(); //[hack] patches/0423 連打の回数は ここで使い終わり

		odds = CatchOddsAfterModifiers(odds, ballType, gBankAttacker, gBankTarget); //[hack] patches/0425 野生のダブル・状態異常・レイド・なんいど・伝説レイドの補正を関数に分けた（元の順のまま・デバッグの表示と式を共有）

		if (ballType == BALL_TYPE_MASTER_BALL) //元は上の補正の途中（サファリボール以外）にあった
			gBattleResults.usedMasterBall = 1;

		if (odds >= 0xFF) //Pokemon is Caught
		{
			EmitBallThrowAnim(0, 4);
			MarkBufferBankForExecution(gActiveBattler);
			gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;

			if (ballType != BALL_TYPE_PARK_BALL || IsRaidBattle())
				SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBankTarget]], MON_DATA_POKEBALL, &ballType);

			if (CalculatePlayerPartyCount() == 6)
				gBattleCommunication[MULTISTRING_CHOOSER] = 0;
			else
				gBattleCommunication[MULTISTRING_CHOOSER] = 1;
		}
		else //Pokemon may be caught, calculate shakes
		{
			u8 shakes, maxShakes;

			if (CriticalCapture(odds))
			{
				maxShakes = 2;  //Critical capture doesn't gauarantee capture
			}
			else
			{
				maxShakes = 4;
			}

			if (ballType == BALL_TYPE_MASTER_BALL
			||	ballType == BALL_TYPE_PARK_BALL
			#ifdef FLAG_ALWAYS_CATCHABLE
			|| FlagGet(FLAG_ALWAYS_CATCHABLE)
			#endif
			)
				shakes = maxShakes;
			else
			{
				#ifdef BUTTON_PRESS_INCREASES_CATCH_ODDS
				if (JOY_HELD(A_BUTTON)
				|| JOY_REPT(A_BUTTON)
				|| (JOY_HELD(B_BUTTON) && JOY_HELD(DPAD_DOWN))
				|| (JOY_REPT(B_BUTTON) && JOY_REPT(DPAD_DOWN)))
					odds += (Sqrt(odds * 5)); //Bigger gains for lower numbers
				#endif

				odds = CatchShakeThreshold(odds); //[hack] patches/0425 式を関数に分けた（デバッグの表示と共有）
				for (shakes = 0; shakes < maxShakes && Random() < odds; ++shakes) ;
			}

			EmitBallThrowAnim(0, shakes);
			MarkBufferBankForExecution(gActiveBattler);

			if (!gNewBS->firstFailedPokeBallStored)
			{
				gNewBS->firstFailedPokeBallStored = TRUE; //Only once per battle
				gNewBS->failedThrownPokeBall = gLastUsedItem;
			}

			if (shakes >= maxShakes)
			{
				if (gNewBS->criticalCapture)
					gNewBS->criticalCaptureSuccess = TRUE;

				gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
				if (ballType != BALL_TYPE_PARK_BALL || IsRaidBattle())
					SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBankTarget]], MON_DATA_POKEBALL, &ballType);

				if (CalculatePlayerPartyCount() == 6)
					gBattleCommunication[MULTISTRING_CHOOSER] = 0;
				else
					gBattleCommunication[MULTISTRING_CHOOSER] = 1;
			}
			else if (IsRaidBattle())
			{
				gBattleCommunication[MULTISTRING_CHOOSER] = shakes;
				gBattlescriptCurrInstr = BattleScript_RaidMonEscapeBall;
			}
			else if (HackChallengeF_OneShotApplies()) //[hack] patches/0951（仮）しばり「いっぱつしょうぶ」: 外れたふつうの野生は その場で逃げて戦いが終わる
			{
				gBattleCommunication[MULTISTRING_CHOOSER] = shakes;
				gBattlescriptCurrInstr = BattleScript_OneShotEscapeBall;
			}
			else //Rip
			{
				gBattleCommunication[MULTISTRING_CHOOSER] = shakes;
				gBattlescriptCurrInstr = BattleScript_ShakeBallThrow;
			}
		}
	}
}

//[hack] patches/0425 atkEF_handleballthrow の中にあった「ボールの補正のあとにかける補正」を関数に分けた（順・式は元のまま）。
//  デバッグの捕獲情報（HackDebugCatchInfo_Calc）も同じ関数を通す＝式を二重に書かない（CLAUDE.md 6.16）。副作用（マスターボールの記録）は呼び元に残した。
static u32 CatchOddsAfterModifiers(u32 odds, u8 ballType, unusedArg u8 bankAtk, u8 bankDef)
{
	#ifndef NO_HARDER_WILD_DOUBLES
	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) && IS_DOUBLE_BATTLE)
	{
		u16 PokesCaught = GetNationalPokedexCount(FLAG_GET_CAUGHT);
		if (PokesCaught < 30)
			odds = (odds * 30) / 100;
		else if (PokesCaught <= 150)
			odds = (odds * 50) / 100;
		else if (PokesCaught <= 300)
			odds = (odds * 70) / 100;
		else if (PokesCaught <= 450)
			odds = (odds * 80) / 100;
		else if (PokesCaught <= 600)
			odds = (odds * 80) / 100;
	}
	#endif

	if (gBattleMons[bankDef].status1 & (STATUS_SLEEP | STATUS_FREEZE))
		odds = (odds * 25) / 10;
	if (gBattleMons[bankDef].status1 & (STATUS_PSN_ANY | STATUS_BURN | STATUS_PARALYSIS))
		odds = (odds * 15) / 10;

	if (IsRaidBattle()) //Dynamax Raid Pokemon can be caught easier
		odds *= 4;

	if (ballType != BALL_TYPE_SAFARI_BALL)
	{
		//Difficulty Modifier - from SwSh
		#ifdef SWSH_CATCHING_DIFFICULTY_MODIFIER
		if (!FlagGet(FLAG_SYS_GAME_CLEAR) && BM_LEVEL(gBattleMons[bankAtk]) < BM_LEVEL(gBattleMons[bankDef]))
			odds /= 10;
		#endif

		//This was used for the TV shows in Ruby, but seems kind of pointless in FR.
		//Commenting it out also prevents errors from using poke balls with large indices.
		//else if (gBattleResults.usedBalls[ballType - BALL_TYPE_ULTRA_BALL] < 0xFF)
		//		gBattleResults.usedBalls[ballType - BALL_TYPE_ULTRA_BALL]++;
	}

	odds = HackDifficulty_CatchOdds(odds); //[hack] patches/0326 なんいど: りるアイス 3 倍・やさしい 2 倍・ほどほど 1.5 倍・むずかしい 0.7 倍・ももアイス 0.5 倍（サファリも・0xFF で頭打ち・マスターボールは下の分岐で必ず捕まる）

	//[hack] patches/0181: 伝説レイド（src/hack_legend_raid.c）は勝てば必ず捕まえられる（2026-10-06 ユーザー「捕まえられなかった」）
	if (HackLegendRaid_Active())
		odds = 0xFF;

	return odds;
}

//1 回の揺れが続く確率（0〜0xFFFF のしきい値・Random() がこれより小さければ続く）
static u32 CatchShakeThreshold(u32 odds)
{
	return udivsi(0xFFFF0, Sqrt(Sqrt(udivsi(0xFF0000, odds))));
}

//[hack] patches/0961 スナッチボール: 相手のダークの個体への投げ。野生と同じ判定（GetBaseBallCatchOdds が クイック・ハイパー・タイマーの いちばん高い倍率・補正も共通）。
//  捕まえたら戦闘は続く（BattleScript_SnatchCaught）・外れたら普通のボールと同じ（BattleScript_ShakeBallThrow・相手のダークは戦闘を続ける）。急所捕獲は付けない（揺れは 4 回）
static void SnatchBallThrow(u8 ballType)
{
	u32 odds;
	u8 shakes = 4;

	gBankTarget = HackSnatch_FindDarkFoe();
	odds = CatchOddsAfterModifiers(GetBaseBallCatchOdds(ballType, gBankAttacker, gBankTarget), ballType, gBankAttacker, gBankTarget);
	if (odds < 0xFF)
	{
		#ifdef BUTTON_PRESS_INCREASES_CATCH_ODDS
		if (JOY_HELD(A_BUTTON) || JOY_REPT(A_BUTTON) || (JOY_HELD(B_BUTTON) && JOY_HELD(DPAD_DOWN)) || (JOY_REPT(B_BUTTON) && JOY_REPT(DPAD_DOWN)))
			odds += (Sqrt(odds * 5));
		#endif
		odds = CatchShakeThreshold(odds);
		for (shakes = 0; shakes < 4 && Random() < odds; ++shakes) ;
	}
	EmitBallThrowAnim(0, shakes);
	MarkBufferBankForExecution(gActiveBattler);
	if (shakes >= 4)
	{
		gNewBS->snatchActive = TRUE;
		gBattlescriptCurrInstr = BattleScript_SnatchCaught;
		SetMonData(GetBankPartyData(gBankTarget), MON_DATA_POKEBALL, &ballType);
	}
	else
	{
		gBattleCommunication[MULTISTRING_CHOOSER] = shakes;
		gBattlescriptCurrInstr = BattleScript_ShakeBallThrow;
	}
}


#ifdef HACK_DEBUG_MENU
//[hack] patches/0425 デバッグ版だけ: 戦闘中の捕獲情報（src/hack_debug_catch_info.c が画面に出す）。上の関数と GetBaseBallCatchOdds を通す＝捕獲の式と同じ。
//  捕まる確率は「ほかくの命令（atkEF_handleballthrow）が odds から揺れ 4 回を続ける確率」: odds >= 0xFF・マスター／パークは 100%、
//  それ以外は しきい値 p の 4 乗（CriticalCapture は乱数で決まるので入れない）。単位は 0.1%。
static u16 DebugCatchPermille(u32 odds, u8 ballType)
{
	u32 p, p2, p4;

	if (ballType == BALL_TYPE_MASTER_BALL || ballType == BALL_TYPE_PARK_BALL || odds >= 0xFF)
		return 1000;
	if (odds == 0)
		return 0;

	p = CatchShakeThreshold(odds);
	if (p >= 0x10000)
		return 1000;
	p2 = (p * p) >> 16;
	p4 = (p2 * p2) >> 16;
	return (p4 * 1000) >> 16;
}

static u16 DebugCatchOneBall(u16 item, u8 bankAtk, u8 bankDef, u8 *outMultiplier, u16 *outMashPermille)
{
	u8 ballType = ItemId_GetType(item);
	u32 odds;
	u16 normal;

	gNewBS->hackMashCount = 0;
	odds = GetBaseBallCatchOdds(ballType, bankAtk, bankDef);
	*outMultiplier = gNewBS->hackCatchInfoLastMult;
	normal = DebugCatchPermille(CatchOddsAfterModifiers(odds, ballType, bankAtk, bankDef), ballType);

	*outMashPermille = normal;
	if (HackMashCatch_AppliesToBall(ballType)) //連打が効く場面・ボールだけ 10 回押した場合を出す
	{
		gNewBS->hackMashCount = 10;
		odds = GetBaseBallCatchOdds(ballType, bankAtk, bankDef);
		*outMashPermille = DebugCatchPermille(CatchOddsAfterModifiers(odds, ballType, bankAtk, bankDef), ballType);
		gNewBS->hackMashCount = 0;
	}
	return normal;
}

void HackDebugCatchInfo_Calc(struct HackDebugCatchInfo *out)
{
	u8 bankAtk, bankDef, mashSaved, i;
	u16 species, slotCount;
	struct ItemSlot *slots;

	for (i = 0; i < sizeof(*out); ++i)
		((u8 *) out)[i] = 0;
	if (gNewBS == NULL)
	{
		out->state = DEBUG_CATCH_NONE;
		return;
	}
	if (gNewBS->isTrainerBattle)
	{
		out->state = DEBUG_CATCH_TRAINER;
		return;
	}
	if ((gBattleTypeFlags & (BATTLE_TYPE_GHOST | BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_POKE_DUDE | BATTLE_TYPE_LINK))
	#ifdef FLAG_NO_CATCHING
	|| FlagGet(FLAG_NO_CATCHING)
	#endif
	#ifdef FLAG_NO_CATCHING_AND_RUNNING
	|| FlagGet(FLAG_NO_CATCHING_AND_RUNNING)
	#endif
	)
	{
		out->state = DEBUG_CATCH_CANT;
		return;
	}

	bankAtk = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
	bankDef = GetCatchingBattler();
	species = GetMonData(GetBankPartyData(bankDef), MON_DATA_SPECIES, NULL);
	out->state = DEBUG_CATCH_OK;
	out->catchRate = gBaseStats[species].catchRate;
	out->hpPercent = gBattleMons[bankDef].maxHP ? (gBattleMons[bankDef].hp * 100) / gBattleMons[bankDef].maxHP : 0;
	out->otherMult10 = CatchOddsAfterModifiers(10, BALL_TYPE_ULTRA_BALL, bankAtk, bankDef); //ボール・HP 以外の補正（状態異常・ダブル・レイド・なんいど）。odds 10 を通した結果がそのまま 0.1 倍単位の倍率

	mashSaved = gNewBS->hackMashCount;
	//gBagPockets は BPRJ.ld に番地が無く組めない（6.29）＝achievements.c と同じく番地 0x20397D8 で読む（ポインタ＋4 バイトの数が 5 組）
	slots = ((struct BagPocket*) 0x20397D8)[POCKET_POKE_BALLS - 1].itemSlots;
	slotCount = ((struct BagPocket*) 0x20397D8)[POCKET_POKE_BALLS - 1].capacity;
	for (i = 0; i < slotCount && out->count < DEBUG_CATCH_ROWS; ++i)
	{
		struct DebugCatchRow *row = &out->rows[out->count];

		if (slots[i].itemId == ITEM_NONE || slots[i].quantity == 0)
			continue;
		row->item = slots[i].itemId;
		row->permille = DebugCatchOneBall(slots[i].itemId, bankAtk, bankDef, &row->multiplier, &row->mashPermille);
		++out->count;
	}
	gNewBS->hackMashCount = mashSaved;
}
#endif

//[hack] patches/0961 スナッチボール: クイックボール・タイマーボールの倍率（10 で 1 倍）。元の case の式を関数に出しただけ（スナッチボールの「合わせた性能」が同じ式を呼ぶ）
static u8 QuickBallMultiplier(void)
{
	return gBattleResults.battleTurnCounter == 0 ? 50 : 10;
}

static u8 TimerBallMultiplier(void)
{
	return MathMin(40, 10 + gBattleResults.battleTurnCounter * 3);
}

u32 GetBaseBallCatchOdds(u8 ballType, u8 bankAtk, u8 bankDef)
{
	u8 catchRate;
	u8 ballMultiplier = 0;
	u16 atkSpecies = gBattleMons[bankAtk].species;
	u16 defSpecies = GetMonData(GetBankPartyData(bankDef), MON_DATA_SPECIES, NULL); //Can't use gBattleMons because of things like Transform
	u16 atkLevel = BM_LEVEL(gBattleMons[bankAtk]);
	u16 defLevel = BM_LEVEL(gBattleMons[bankDef]);

	if (ballType == BALL_TYPE_SAFARI_BALL)
	{
		//本家は 被捕獲度 × 100 ÷ 1275 で 0〜20 の係数に潰してから
		//ここで戻すため、整数除算の切り捨てで**実際の捕獲率が下がる**。
		//  例: ラッキー 30 → 係数 2 → 25 ／ ケンタロス 45 → 係数 3 → 38
		//係数そのものは えさ(÷2)・いし(×2) を表すので、
		//**元の被捕獲度に「係数 ÷ 初期係数」の倍率だけを掛ける**ことで
		//本家と同じ挙動のまま精度の欠落だけを取り除く。
		//（patches/0016）
		u8 baseCatchRate = gBaseStats[defSpecies].catchRate;
		u16 initialFactor = (baseCatchRate * 100) / 1275; //本家の初期係数
		u16 scaled;

		if (initialFactor == 0)
			initialFactor = 1; //0 除算よけ。被捕獲度 12 以下がここに来る

		scaled = (baseCatchRate * gBattleStruct->safariCatchFactor)
		       / initialFactor;
		catchRate = (scaled > 255) ? 255 : scaled;
	}
	else
		catchRate = gBaseStats[GetMonData(GetBankPartyData(bankDef), MON_DATA_SPECIES, NULL)].catchRate; //Uses party data b/c Transform update Gen 5+

	if (ballType >= BALL_TYPE_NET_BALL)
	{
		switch (ballType) {
			case BALL_TYPE_NET_BALL:
				if (IsOfType(bankDef, TYPE_WATER) || IsOfType(bankDef, TYPE_BUG))
					ballMultiplier = 35;
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_DIVE_BALL:
				if (GetCurrentMapType() == MAP_TYPE_UNDERWATER
				|| gFishingByte || TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
					ballMultiplier = 35;
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_NEST_BALL:
				if (BM_LEVEL(gBattleMons[bankDef]) < 31)
					ballMultiplier = 41 - defLevel; //At lowest 11
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_REPEAT_BALL:
				if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(defSpecies), FLAG_GET_CAUGHT))
					ballMultiplier = 35;
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_TIMER_BALL:
				ballMultiplier = TimerBallMultiplier();
				break;

			case BALL_TYPE_LUXURY_BALL:
			case BALL_TYPE_PREMIER_BALL:
			case BALL_TYPE_CHERISH_BALL:
			case BALL_TYPE_FRIEND_BALL:
			case BALL_TYPE_HEAL_BALL:
				ballMultiplier = 10;
				break;

			case BALL_TYPE_LEVEL_BALL:
				if (atkLevel <= defLevel)
					ballMultiplier = 10;
				else if (atkLevel > defLevel && atkLevel < defLevel * 2)
					ballMultiplier = 20;
				else if (atkLevel >= defLevel * 2 && atkLevel < defLevel * 4)
					ballMultiplier = 40;
				else
					ballMultiplier = 80;
				break;

			case BALL_TYPE_LURE_BALL:
				if (gFishingByte)
					ballMultiplier = 40;
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_MOON_BALL: ;
				switch (defSpecies) {
					default:
						ballMultiplier = 10;
						break;
					#ifdef SPECIES_NIDORAN_F
					case SPECIES_NIDORAN_F:
					#endif
					#ifdef SPECIES_NIDORINA
					case SPECIES_NIDORINA:
					#endif
					#ifdef SPECIES_NIDOQUEEN
					case SPECIES_NIDOQUEEN:
					#endif
					#ifdef SPECIES_NIDORAN_M
					case SPECIES_NIDORAN_M:
					#endif
					#ifdef SPECIES_NIDORINO
					case SPECIES_NIDORINO:
					#endif
					#ifdef SPECIES_NIDOKING
					case SPECIES_NIDOKING:
					#endif
					#ifdef SPECIES_CLEFFA
					case SPECIES_CLEFFA:
					#endif
					#ifdef SPECIES_CLEFAIRY
					case SPECIES_CLEFAIRY:
					#endif
					#ifdef SPECIES_CLEFABLE
					case SPECIES_CLEFABLE:
					#endif
					#ifdef SPECIES_IGGLYBUFF
					case SPECIES_IGGLYBUFF:
					#endif
					#ifdef SPECIES_JIGGLYPUFF
					case SPECIES_JIGGLYPUFF:
					#endif
					#ifdef SPECIES_WIGGLYTUFF
					case SPECIES_WIGGLYTUFF:
					#endif
					#ifdef SPECIES_SKITTY
					case SPECIES_SKITTY:
					#endif
					#ifdef SPECIES_DELCATTY
					case SPECIES_DELCATTY:
					#endif
					#ifdef SPECIES_MUNNA
					case SPECIES_MUNNA:
					#endif
					#ifdef SPECIES_MUSHARNA
					case SPECIES_MUSHARNA:
					#endif
						ballMultiplier = 40;
						break;
				}

				break;

			case BALL_TYPE_LOVE_BALL:
				ballMultiplier = 10;
				if (SpeciesToNationalPokedexNum(atkSpecies) == SpeciesToNationalPokedexNum(defSpecies))
				{
					u8 atkGender = GetGenderFromSpeciesAndPersonality(atkSpecies, gBattleMons[bankAtk].personality);
					u8 defGender = GetGenderFromSpeciesAndPersonality(defSpecies, gBattleMons[bankDef].personality);

					if (atkGender != 0xFF && defGender != 0xFF && atkGender != defGender)
						ballMultiplier = 80;
				}
				break;

			//Heavy Ball modifies the catch rate itself, not the multiplier
			case BALL_TYPE_HEAVY_BALL:	;
				ballMultiplier = 10;
				//Apparently abilities or Float Stone have no affect here
				u16 defWeight = GetPokedexHeightWeight(SpeciesToNationalPokedexNum(defSpecies), PKDX_GET_WEIGHT);
				u8 oldCatchRate;

				if (defWeight < 1000)
				{
					oldCatchRate = catchRate;
					catchRate -= 20;
					if (catchRate > oldCatchRate)
						catchRate = 1; //New in USUM
				}
				else if (defWeight >= 2000 && defWeight < 3000)
					catchRate += 20;
				else if (defWeight > 3000)
					catchRate += 30;
				break;

			case BALL_TYPE_FAST_BALL:
				if (HackBaseStat(defSpecies, STAT_SPEED) >= 100)
					ballMultiplier = 40;
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_SPORT_BALL:
				ballMultiplier = 10; //★ Champions（patches/0071）: 捕獲率はモンスターボールと同じ
				break;

			case BALL_TYPE_DUSK_BALL:
				if (GetCurrentMapType() == MAP_TYPE_UNDERGROUND)
					ballMultiplier = DUSK_BALL_MULTIPLIER;
				#ifdef TIME_ENABLED
				else if (IsNightTime())
					ballMultiplier = DUSK_BALL_MULTIPLIER;
				#endif
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_QUICK_BALL:
				ballMultiplier = QuickBallMultiplier();
				break;

			case BALL_TYPE_SNATCH_BALL: //[hack] patches/0961 スナッチボール: クイック（1 ターン目に強い）・ハイパー（いつも 2 倍）・タイマー（ターンが進むほど強い）を合わせた性能＝3 つのうち いちばん高い倍率
				ballMultiplier = MathMax(QuickBallMultiplier(), MathMax(sBallCatchBonuses[BALL_TYPE_ULTRA_BALL - BALL_TYPE_ULTRA_BALL], TimerBallMultiplier()));
				break;

			case BALL_TYPE_DREAM_BALL:
				if (gBattleMons[bankDef].status1 & STATUS1_SLEEP || ABILITY(bankDef) == ABILITY_COMATOSE)
					ballMultiplier = 40;
				else
					ballMultiplier = 10;
				break;

			case BALL_TYPE_BEAST_BALL:
				if (CheckTableForSpecies(defSpecies, gUltraBeastList))
					ballMultiplier = 50;
				else
					ballMultiplier = 1;
				break;

			case BALL_TYPE_START_BALL:
				ballMultiplier = 13; //[hack] patches/0393 STARTボール: 1.3 倍（倍率は 10 で 1 倍・odds = catchRate * 13 / 10 ...）
				break;
// WATASHI_BALLS_BEGIN catch

			case BALL_TYPE_MOON_PETAL_BALL:
			case BALL_TYPE_MEMORY_VEIL_BALL:
			case BALL_TYPE_TEAR_WISTERIA_BALL:
			case BALL_TYPE_SWEET_HEART_BALL:
			case BALL_TYPE_SILENT_HALO_BALL:
			case BALL_TYPE_GAME_PULSE_BALL:
			case BALL_TYPE_WINTER_BLOSSOM_BALL:
			case BALL_TYPE_THORN_HEART_BALL:
			case BALL_TYPE_FOX_SPIRIT_BALL:
			case BALL_TYPE_BERRY_BLOOM_BALL:
				ballMultiplier = 20; //[hack] patches/0393 わたしたち 10 人のボール: ハイパーボールと同じ 2 倍
				break;

// WATASHI_BALLS_END catch
		}
	}
	else
		ballMultiplier = sBallCatchBonuses[ballType - BALL_TYPE_ULTRA_BALL];

	#ifdef VAR_CATCH_RATE_BONUS
	if (VarGet(VAR_CATCH_RATE_BONUS) > 0)
		ballMultiplier += (ballMultiplier * VarGet(VAR_CATCH_RATE_BONUS)) / 100; //Percent increase - Eg. Var is set to 10, and multiplier is 5x, then final multiplier will be 5.5 (55)
	#endif

	u32 mashBonus100 = HackMashCatch_Bonus100(); //[hack] patches/0423 A 連打: 補正値に足す（1/100 単位・回数 0 なら 0）
	if (CheckTableForSpecies(defSpecies, gUltraBeastList) && ballType != BALL_TYPE_BEAST_BALL)
	{
		ballMultiplier = 1; //All balls except for Beast Ball have a hard time catching Ultra Beasts
		mashBonus100 = 0;
	}

	#ifdef HACK_DEBUG_MENU
	if (gNewBS != NULL)
		gNewBS->hackCatchInfoLastMult = ballMultiplier; //[hack] patches/0425 デバッグの捕獲情報が補正値を読む
	#endif

	u32 odds = (((catchRate * (ballMultiplier * 10 + mashBonus100)) / 100) * (gBattleMons[bankDef].maxHP * 3 - gBattleMons[bankDef].hp * 2)) / (3 * gBattleMons[bankDef].maxHP); //[hack] patches/0423 元は (catchRate * ballMultiplier) / 10（回数 0 で同じ値）
	return odds;
}

static u8 GetCatchingBattler(void)
{
	u8 battler;

	if (IS_DOUBLE_BATTLE && GetBankPartyData(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT))->hp > 0)
		battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
	else
		battler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

	if (battler >= gBattlersCount || SIDE(battler) == B_SIDE_PLAYER)
		battler = B_POSITION_OPPONENT_LEFT;

	return battler;
}

#ifdef CRITICAL_CAPTURE
struct CriticalCaptureOdds
{
	u16 numCaught; //Less than or equal to this number
	u16 oddsMultiplier; //x10
};

static const struct CriticalCaptureOdds sCriticalCaptureSpeciesCounts[] =
{
	{30,      0}, //x0.0
	{150,     5}, //x0.5
	{300,    10}, //x1.0
	{450,    15}, //x1.5
	{600,    20}, //x2.0
	{0xFFFF, 25}, //x2.5 default
};
#endif

static bool8 CriticalCapture(unusedArg u32 odds)
{
	if (IsRaidBattle())
		return FALSE; //Critical Captures can't happen in Raids

	#ifdef CRITICAL_CAPTURE
	//Adjust Original Catch Odds - X
	u8 baseOdds = MathMin(odds, 255);

	//Pokedex Modifier - P
	u8 pokedexMultiplier = 0;
	u16 speciesCaught = GetNationalPokedexCount(FLAG_GET_CAUGHT);

	for (u32 i = 0; i < NELEMS(sCriticalCaptureSpeciesCounts); ++i)
	{
		if (speciesCaught <= sCriticalCaptureSpeciesCounts[i].numCaught)
		{
			pokedexMultiplier = sCriticalCaptureSpeciesCounts[i].oddsMultiplier;
			break;
		}
	}

	//Catching Charm Modifier - Ch
	u8 catchingCharmModifier = 1;

	#ifdef ITEM_CATCHING_CHARM
	if (CheckBagHasItem(ITEM_CATCHING_CHARM, 1))
		catchingCharmModifier = 2;
	#endif

	//Final Calculation
	odds = ((baseOdds * pokedexMultiplier * catchingCharmModifier) / 10) / 6;

	gNewBS->criticalCaptureSuccess = FALSE;
	if (Random() % 0x100 < odds) //0 to 255 inclusive
		return gNewBS->criticalCapture = TRUE;
	#endif

	return gNewBS->criticalCapture = FALSE;
}

bool8 IsCriticalCapture(void)
{
	return gNewBS->criticalCapture;
}

bool8 IsCriticalCaptureSuccess(void)
{
	return gNewBS->criticalCaptureSuccess;
}

static bool8 GetFreeSlotInPartyForMon(void)
{
	u32 i = 0;

	while (i < PARTY_SIZE && gPlayerParty[i].species != SPECIES_NONE)
		++i;

	if (i >= PARTY_SIZE
	|| (gMain.inBattle && gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)) //Always send Pokemon in multi battles to the PC because of overwritten team
		return PARTY_SIZE;

	return i;
}

bool8 HackChallengeLuxray_OnObtain(struct Pokemon* mon, bool8 markDex); //[hack] しばり『ぜんぶレントラー』src/hack_challenge_luxray.c

u8 GiveMonToPlayer(struct Pokemon* mon) //Hook in
{
	TryFormRevert(mon);
	TryRevertMega(mon);
	TryRevertGigantamax(mon);
	TryRevertTerastalForm(mon);
	HackChallengeLuxray_OnObtain(mon, TRUE); //[hack] 手に入れた子は ぜんぶレントラー♀（旗 OFF・施設・ダミー戦・タマゴは何もしない）。捕獲は atkF1 で済み済み＝二度目は何もしない

	SetMonData(mon, MON_DATA_OT_NAME, gSaveBlock2->playerName);
	SetMonData(mon, MON_DATA_OT_GENDER, &gSaveBlock2->playerGender);
	SetMonData(mon, MON_DATA_OT_ID, gSaveBlock2->playerTrainerId);

	u8 freeSlot = GetFreeSlotInPartyForMon();
	if (freeSlot >= PARTY_SIZE || HackNushi_PartyHasOther(gPlayerParty, mon, PARTY_SIZE)) //Can't add mon（[hack] patches/0981 ヌシの 2 匹目は手持ちが空いていてもボックスへ）
	{
		TryRevertOriginFormes(mon, TRUE);
		return SendMonToPC(mon);
	}

	CopyMon(&gPlayerParty[freeSlot], mon, sizeof(struct Pokemon));
	gPlayerPartyCount = freeSlot + 1;
	return MON_GIVEN_TO_PARTY;
}

void ApplyBallSpecialEffect(void)
{
	struct Pokemon* mon = LoadTargetPartyData();
	u8 ballType = ItemId_GetType(gLastUsedItem);

	if (ballType == BALL_TYPE_HEAL_BALL)
		HealMon(mon);
	else if (ballType == BALL_TYPE_FRIEND_BALL)
		mon->friendship = 200;
	#ifdef UNBOUND
	else if (ballType == BALL_TYPE_DREAM_BALL)
		mon->hiddenAbility = TRUE;
	#endif
}

void atkF0_givecaughtmon(void)
{
	struct Pokemon* mon = LoadTargetPartyData();

	if (HackChallenge_IsLentrantBattle()) //[hack] patches/0510 ？？？（しばり）は捕まえられない（ここへ来る道は無いはずだが、来ても手持ちに入れない）
	{
		gBattlescriptCurrInstr += 1;
		return;
	}

	if (IsRaidBattle() && FlagGet(FLAG_BATTLE_FACILITY) && gNewBS->dynamaxData.backupRaidMonItem != ITEM_NONE)
	{
		SetMonData(mon, MON_DATA_HELD_ITEM, &gNewBS->dynamaxData.backupRaidMonItem);
		gNewBS->dynamaxData.backupRaidMonItem = ITEM_NONE;
	}

	if (GiveMonToPlayer(mon) != MON_GIVEN_TO_PARTY)
	{
		if (Relive_IsDark((struct BoxPokemon*) mon)) //[hack] ダークは「送る箱」の記録を動かさず専用のリライブボックスへ入る（SendMonToPC・patches/0975）＝文の箱の名前は専用の名前にする
		{
			gBattleCommunication[MULTISTRING_CHOOSER] = 0;
			StringCopy(gStringVar1, gText_Relive_BoxName); //[hack] patches/0975 案 B: 専用の箱は PC の箱ではない＝箱の名前は専用の名前
			GetMonData(mon, MON_DATA_NICKNAME, gStringVar2);
		}
		else if (!ShouldShowBoxWasFullMessage())
		{
			gBattleCommunication[MULTISTRING_CHOOSER] = 0;
			StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON)));
			GetMonData(mon, MON_DATA_NICKNAME, gStringVar2);
		}
		else
		{
			StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON))); //Box the mon was sent to
			GetMonData(mon, MON_DATA_NICKNAME, gStringVar2);
			StringCopy(gStringVar3, GetBoxNamePtr(GetPCBoxToSendMon())); //Box the mon was going to be sent to
			gBattleCommunication[MULTISTRING_CHOOSER] = 2;
		}

		if (FlagGet(FLAG_SYS_NOT_SOMEONES_PC))
			++gBattleCommunication[MULTISTRING_CHOOSER];
	}

	if (!IsTimeInVarInFuture(VAR_SWARM_DAILY_EVENT))
		IncrementGameStat(GAME_STAT_CAUGHT_TODAY);
	else
		SetGameStat(GAME_STAT_CAUGHT_TODAY, 0);

	gBattleResults.caughtMonSpecies = SPECIES(gBankTarget);
	GetMonData(mon, MON_DATA_NICKNAME, gBattleResults.caughtMonNick);
	++gBattlescriptCurrInstr;
}

void atkF1_trysetcaughtmondexflags(void)
{
	struct Pokemon* mon = LoadTargetPartyData();

	TryRevertMega(mon); //Megas aren't set in the habitat table
	TryRevertGigantamax(mon); //Gigantamaxes aren't set in the habitat table
	TryRevertTerastalForm(mon);
	TryFormRevert(mon);
	HackChallengeLuxray_OnObtain(mon, FALSE); //[hack] 図鑑の「つかまえた」の判定より前にレントラーにする（図鑑の画面もレントラー・もとの種族は「見た」だけ）

	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);

	if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
	{
		gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
	}
	else
	{
		HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT, personality);
		gBattlescriptCurrInstr += 5;
	}
}

void CreateCapturedMonDexPic(u16 species)
{
	HackDexMale_CreateMonPic(species, gBattleMons[gBankTarget].otId, //[hack] patches/0945 図鑑の画面は性別差分を使わない
									gBattleMons[gBankTarget].personality,
									TRUE, 120, 64, 0, 0xFFFF);
}

u8 ItemIdToBallId(u16 ballItem)
{
	if (ballItem == 0xFFFF)
		return BALL_TYPE_DYNAMAX_BALL;

	return ItemId_GetType(ballItem);
}

void PlayerHandleSuccessBallThrowAnim(void)
{
	u8 animNum = (gNewBS->criticalCapture) ? B_ANIM_CRITICAL_CAPTURE_THROW : B_ANIM_BALL_THROW;
	gBattleSpritesDataPtr->animationData->ballThrowCaseId = BALL_3_SHAKES_SUCCESS;

	gDoingBattleAnim = TRUE;
	InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, GetCatchingBattler(), animNum);
	gBattlerControllerFuncs[gActiveBattler] = CompleteOnSpecialAnimDone;
}

void PlayerHandleBallThrowAnim(void)
{
	u8 animNum = (gNewBS->criticalCapture) ? B_ANIM_CRITICAL_CAPTURE_THROW : B_ANIM_BALL_THROW;
	u8 ballThrowCaseId = gBattleBufferA[gActiveBattler][1];

	gBattleSpritesDataPtr->animationData->ballThrowCaseId = ballThrowCaseId;
	gDoingBattleAnim = TRUE;
	InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, GetCatchingBattler(), animNum);
	gBattlerControllerFuncs[gActiveBattler] = CompleteOnSpecialAnimDone;
}

void AnimTask_LoadBallGfx(u8 taskId)
{
	u8 ballId = ItemIdToBallId(gLastUsedItem);
	LoadBallGfx(ballId);
	DestroyAnimVisualTask(taskId);
}

void __attribute__((long_call)) FreeBallGfx(u8 ballId);
void AnimTask_FreeBallGfx(u8 taskId)
{
	u8 ballId = ItemIdToBallId(gLastUsedItem);
	FreeBallGfx(ballId);
	DestroyAnimVisualTask(taskId);
}

void AnimTask_ThrowBall(u8 taskId)
{
	u8 ballId;
	u8 spriteId;

	ballId = ItemIdToBallId(gLastUsedItem);
	spriteId = CreateSprite(&gBallSpriteTemplates[ballId], 32, 80, 29);
	gSprites[spriteId].data[0] = 34;

	gBattleAnimTarget = GetCatchingBattler();

	gSprites[spriteId].data[1] = GetBattlerSpriteCoord(gBattleAnimTarget, 0);
	gSprites[spriteId].data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, 1) - 16;
	gSprites[spriteId].callback = (void*) 0x80F0821;
	gBattleSpritesDataPtr->animationData->wildMonInvisible = gSprites[gBattlerSpriteIds[gBattleAnimTarget]].invisible;
	gTasks[taskId].data[0] = spriteId;
	gTasks[taskId].func = (void*) 0x80F05F9;
}

void LoadBallGfx(u8 ballId)
{
//	u16 var;
	if (GetSpriteTileStartByTag(gBallSpriteSheets[ballId].tag) == 0xFFFF)
	{
		LoadCompressedSpriteSheetUsingHeap(&gBallSpriteSheets[ballId]);
		LoadCompressedSpritePaletteUsingHeap(&gBallSpritePalettes[ballId]);
	}
	/*switch (ballId) {
		case BALL_TYPE_MASTER_BALL:
		case BALL_TYPE_ULTRA_BALL:
		case BALL_TYPE_GREAT_BALL:
		case BALL_TYPE_POKE_BALL:
		case BALL_TYPE_SAFARI_BALL:
		case BALL_TYPE_NET_BALL:
		case BALL_TYPE_NEST_BALL:
		case BALL_TYPE_REPEAT_BALL:
		case BALL_TYPE_TIMER_BALL:
			var = GetSpriteTileStartByTag(gBallSpriteSheets[ballId].tag);
			LZDecompressVram(gOpenPokeballGfx, (void*)(VRAM + 0x10100 + var * 32));
			break;
	}*/
}

u16 GetBattlerPokeballItemId(u8 bank)
{
	u8 ballId;
	
	if (IsDynamaxed(bank))
		ballId = BALL_TYPE_DYNAMAX_BALL;
	else
		ballId = GetMonData(GetIllusionPartyData(bank), MON_DATA_POKEBALL, 0);

	return ballId;
}

static bool8 CantCatchBecauseRaid(void)
{
	return IsRaidBattle() && !RAID_BATTLE_END;
}

bool8 CantCatchBecauseFlag(void)
{
	return FlagGet(FLAG_NO_CATCHING) || FlagGet(FLAG_NO_CATCHING_AND_RUNNING);
}

static bool8 CantCatchBecauseTwoAlive(void)
{
	return BATTLER_ALIVE(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT))
		&& BATTLER_ALIVE(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT));
}

static bool8 CantCatchBecauseSemiInvulnerable(u8 pos)
{
	return BATTLER_SEMI_INVULNERABLE(GetBattlerAtPosition(pos)) != 0;
}

static bool8 CantCatchBecauseSemiInvulnerableDoubles(void)
{
	return (BATTLER_ALIVE(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)) && CantCatchBecauseSemiInvulnerable(B_POSITION_OPPONENT_LEFT))
		|| (BATTLER_ALIVE(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)) && CantCatchBecauseSemiInvulnerable(B_POSITION_OPPONENT_RIGHT));
}

static bool8 CantCatchBecauseSecondMon(void)
{
	return BATTLER_ALIVE(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT))
		&& gChosenActionByBank[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)] != ACTION_USE_ITEM
		&& gChosenActionByBank[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)] != ACTION_NOTHING_FAINTED;
}

bool8 DoubleWildPokeBallItemUseFix(u8 taskId)
{
	bool8 effect = FALSE;
	u16 ballItem = gSpecialVar_ItemId;
	const u8* snatchRefusal = HackSnatch_GetBagRefusal(ballItem); //[hack] patches/0961 スナッチボールの断り（ダークに普通のボール・野生やダークでない個体にスナッチ）

	if (snatchRefusal != NULL)
	{
		DisplayItemMessageInBag(taskId, 2, snatchRefusal, Task_ReturnToBagFromContextMenu);
		effect = TRUE;
	}
	else if (CantCatchBecauseRaid())
	{
		DisplayItemMessageInBag(taskId, 2, gText_CantCatchPokemonYet, Task_ReturnToBagFromContextMenu);
		effect = TRUE;
	}
	else if (CantCatchBecauseFlag())
	{
		DisplayItemMessageInBag(taskId, 2, gText_CantCatchPokemonRightNow, Task_ReturnToBagFromContextMenu);
		effect = TRUE;
	}
	else if (HackSnatch_BagSkipsDoubleChecks(ballItem)) //[hack] patches/0961 トレーナー戦でダークへ投げるスナッチは「2 ひきいてねらえない」「技とボールは同時に出せない」を通さない
	{
		u8 foe = HackSnatch_FindDarkFoe();

		if (BATTLER_SEMI_INVULNERABLE(foe) != 0)
		{
			DisplayItemMessageInBag(taskId, 2, gText_CantAimAtSemiInvulnerableTarget, Task_ReturnToBagFromContextMenu);
			effect = TRUE;
		}
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		if (CantCatchBecauseTwoAlive())
		{
			DisplayItemMessageInBag(taskId, 2, gText_CantAimAtTwoTargets, Task_ReturnToBagFromContextMenu);
			effect = TRUE;
		}
		else if (CantCatchBecauseSemiInvulnerableDoubles())
		{
			DisplayItemMessageInBag(taskId, 2, gText_CantAimAtSemiInvulnerableTarget, Task_ReturnToBagFromContextMenu);
			effect = TRUE;
		}
		else if (CantCatchBecauseSecondMon())
		{
			DisplayItemMessageInBag(taskId, 2, gText_CantUseAMoveAndABall, Task_ReturnToBagFromContextMenu);
			effect = TRUE;
		}
	}
	else if (CantCatchBecauseSemiInvulnerable(B_POSITION_OPPONENT_LEFT))
	{
		DisplayItemMessageInBag(taskId, 2, gText_CantAimAtSemiInvulnerableTarget, Task_ReturnToBagFromContextMenu);
		effect = TRUE;
	}

	return effect;
}

bool8 CantCatchPokemonRightNow(void)
{
	if (CantCatchBecauseRaid() || CantCatchBecauseFlag())
		return TRUE;
	
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	{
		if (CantCatchBecauseTwoAlive()
		|| CantCatchBecauseSemiInvulnerableDoubles()
		|| (IS_DOUBLE_BATTLE
		 && BATTLER_ALIVE(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT))
		 && gActiveBattler == GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT))) //Only Pokemon on left can throw ball
			return TRUE;
	}
	else if (CantCatchBecauseSemiInvulnerable(B_POSITION_OPPONENT_LEFT))
		return TRUE;

	return FALSE;
}

struct Pokemon* LoadTargetPartyData(void)
{
	return GetBankPartyData(gBankTarget);
}

u16 LoadTargetPokedexSpecies(void)
{
	u16 species = GetMonData(LoadTargetPartyData(), MON_DATA_SPECIES, NULL);

	#ifdef  SPECIES_MINIOR_SHIELD
	if (IsMinior(species))
		return SPECIES_MINIOR_SHIELD;
	#endif

	#if (defined NATIONAL_POKEDEX_PIKACHU && defined SPECIES_PIKACHU)
	if (SpeciesToNationalPokedexNum(species) == NATIONAL_POKEDEX_PIKACHU)
		return SPECIES_PIKACHU;
	#endif

	return species;
}

#define BattleScript_BallThrow (const u8*) 0x81BD9D0
#define BattleScript_SafariBallThrow (const u8*) 0x81BD9F8
bool8 TryGetPokeBallBattleScript(void)
{
	if (GetPocketByItemId(gLastUsedItem) == POCKET_POKE_BALLS)
	{
		u8 ballType = ItemId_GetType(gLastUsedItem);
		if (ballType == BALL_TYPE_SAFARI_BALL)
			gBattlescriptCurrInstr = BattleScript_SafariBallThrow;
		else
			gBattlescriptCurrInstr = BattleScript_BallThrow;

		return TRUE;
	}

	return FALSE;
}

//はじめの話（モモカ博士）のフカマルを出す／しまうボールだけ STARTボールにする（2026-10-15 patches/0404・ユーザー「タイトルのはじめからのところ」）。
//この関数は交換の演出（CreatePokeballSpriteToReleaseMon／CreateTradePokeballSprite の素の呼び元 0x080519AC・0x08052E08・0x08050DAC・0x0805219C）とも共有なので、
//はじめの話のタスク（Task_OakSpeech_ReleaseNidoranFFromPokeBall 0x08130200／ReturnNidoranFToPokeBall 0x08130404・呼ぶ時点でまだ func が入っている）が動いているときだけ START。
//状態は持たない（6.13）。交換は今までどおりモンスターボール。
#define OAK_SPEECH_TASK_RELEASE ((TaskFunc) (0x08130200 | 1))
#define OAK_SPEECH_TASK_RETURN ((TaskFunc) (0x08130404 | 1))
const struct SpriteTemplate* OakIntroPokeballFix(void)
{
	u8 ballId = BALL_TYPE_POKE_BALL;

	if (FuncIsActiveTask(OAK_SPEECH_TASK_RELEASE) || FuncIsActiveTask(OAK_SPEECH_TASK_RETURN))
		ballId = BALL_TYPE_START_BALL;

	LoadCompressedSpriteSheetUsingHeap(&gBallSpriteSheets[ballId]);
	LoadCompressedSpritePaletteUsingHeap(&gBallSpritePalettes[ballId]);
	return &gBallSpriteTemplates[ballId];
}
