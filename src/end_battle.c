#include "defines.h"
#include "defines_battle.h"
#include "hack_xram_battle.h"
#include "hack_aura.h" //[hack] patches/0956 オーラの後始末
#include "lv200_custom_moves.h"
#include "../include/event_data.h"
#include "../include/field_weather.h"
#include "../include/item_menu.h"
#include "../include/item_use.h"
#include "../include/load_save.h"
#include "../include/malloc.h"
#include "../include/random.h"
#include "../include/constants/maps.h"
#include "../include/constants/items.h" //[hack] patches/0617 ITEM_RIRU_LOVE
#include "../include/constants/songs.h"
#include "../include/constants/trainer_classes.h"

#include "../include/new/battle_start_turn_start.h"
#include "../include/new/battle_util.h"
#include "../include/new/catching.h"
#include "../include/new/dynamax.h"
#include "../include/new/end_battle.h"
#include "../include/new/end_battle_battle_scripts.h"
#include "../include/new/evolution.h"
#include "../include/new/form_change.h"
#include "../include/new/frontier.h"
#include "../include/new/util.h"
#include "../include/new/mega.h"
#include "../include/new/multi.h"
#include "../include/new/terastal.h"
#include "hack_affection.h" //[hack] patches/0240 なつき度（src/hack_affection.c）
#include "hack_challenge.h" //[hack] patches/0510 しばり
#include "hack_dummy_battle.h" //[hack] patches/0602 ポケセン 3F のダミー
#include "hack_chf_rotgacha.h" //[hack] 縛り F
#include "shadow_battle.h" //[hack] patches/0963 段 E ダーク技の組み立て
#include "hack_oak_phone.h"
#include "hack_item_restore.h" //[hack] 使いきった持ち物を戦いのあと戻す
/*
end_battle.c
	handles all battle termination logic and data resetting/saving
*/

const u16 gEndBattleFlagClearTable[] =
{
#ifdef FLAG_CATCH_TRAINERS_POKEMON
	FLAG_CATCH_TRAINERS_POKEMON,
#endif
#ifdef FLAG_ACTIVATE_TUTORIAL
	FLAG_ACTIVATE_TUTORIAL,
#endif
#ifdef FLAG_INVERSE
	FLAG_INVERSE,
#endif
#ifdef FLAG_SKY_BATTLE
	FLAG_SKY_BATTLE,
#endif
#ifdef FLAG_NO_CATCHING
	FLAG_NO_CATCHING,
#endif
#ifdef FLAG_NO_RUNNING
	FLAG_NO_RUNNING,
#endif
#ifdef FLAG_NO_CATCHING_AND_RUNNING
	FLAG_NO_CATCHING_AND_RUNNING,
#endif
#ifdef FLAG_ALWAYS_CATCHABLE
	FLAG_ALWAYS_CATCHABLE,
#endif
#ifdef FLAG_WILD_CUSTOM_MOVES
	FLAG_WILD_CUSTOM_MOVES,
#endif
#ifdef FLAG_SMART_WILD
	FLAG_SMART_WILD,
#endif
#ifdef FLAG_SCALE_WILD_BOSS_LEVEL
	FLAG_SCALE_WILD_BOSS_LEVEL,
#endif
#ifdef FLAG_DOUBLE_WILD_BATTLE
	FLAG_DOUBLE_WILD_BATTLE,
#endif
#ifdef FLAG_SHINY_CREATION
	FLAG_SHINY_CREATION,
#endif
#ifdef FLAG_CAMOMONS_BATTLE
	FLAG_CAMOMONS_BATTLE,
#endif
#ifdef FLAG_PRESET_FRONTIER_OPPONENT_TEAM
	FLAG_PRESET_FRONTIER_OPPONENT_TEAM,
#endif
#ifdef FLAG_PRESET_RANDOM_TEAM
	FLAG_PRESET_RANDOM_TEAM,
#endif
#ifdef FLAG_TWO_WILD_POKEMON_ATTACKED_STRING
	FLAG_TWO_WILD_POKEMON_ATTACKED_STRING,
#endif
#ifdef FLAG_WORST_NIGHTMARE_STRING
	FLAG_WORST_NIGHTMARE_STRING,
#endif
#ifdef FLAG_LEGENDARY_APPEARED_STRING
	FLAG_LEGENDARY_APPEARED_STRING,
#endif
#ifdef FLAG_SINGLE_TRAINER_MON_TOTEM_BOOST
	FLAG_SINGLE_TRAINER_MON_TOTEM_BOOST,
#endif
#ifdef FLAG_BENJAMIN_BUTTERFREE_BATTLE
	FLAG_BENJAMIN_BUTTERFREE_BATTLE,
#endif
#ifdef FLAG_DYNAMAX_BATTLE
	FLAG_DYNAMAX_BATTLE,
#endif
#ifdef FLAG_RAID_BATTLE
	FLAG_RAID_BATTLE,
#endif
#ifdef FLAG_RAID_BATTLE_NO_FORCE_END
	FLAG_RAID_BATTLE_NO_FORCE_END,
#endif
#ifdef FLAG_KEEP_CONSUMABLE_ITEMS
	FLAG_KEEP_CONSUMABLE_ITEMS,
#endif
#ifdef FLAG_HIDDEN_ABILITY
	FLAG_HIDDEN_ABILITY,
#endif
#ifdef FLAG_GIGANTAMAXABLE
	FLAG_GIGANTAMAXABLE,
#endif
#ifdef FLAG_CUSTOM_TERA_TYPE
	FLAG_CUSTOM_TERA_TYPE,
#endif
#ifdef FLAG_TEMP_DISABLE_RANDOMIZER
	FLAG_TEMP_DISABLE_RANDOMIZER,
#endif
#ifdef FLAG_AI_CONTROL_BATTLE
	FLAG_AI_CONTROL_BATTLE,
#endif
#ifdef FLAG_USE_HALF_PARTNER_TEAM
	FLAG_USE_HALF_PARTNER_TEAM,
#endif
#ifdef FLAG_BATTLE_YOURSELF
	FLAG_BATTLE_YOURSELF,
#endif
#ifdef FLAG_DISPLAY_OPPOSING_FOR_WILD
	FLAG_DISPLAY_OPPOSING_FOR_WILD,
#endif

	FLAG_TAG_BATTLE,
	FLAG_TWO_OPPONENTS,
	HACK_FLAG_OAK_PIC,
};

//This file's functions:
static void TryRestoreEnemyTeam(void);
static void NaturalCureHeal(void);
static void RestoreNonConsumableItems(void);
static void RevertDynamax(void);
static void RecalcAllStats(void);
static void BringBackTheDead(void);
static void EndPartnerBattlePartyRestore(void);
static void EndSkyBattlePartyRestore(void);
static void EndBattleFlagClear(void);
static void HealPokemonInFrontier(void);

void HandleEndTurn_BattleWon(void)
{
	gCurrentActionFuncId = 0;

	if (gBattleTypeFlags & BATTLE_TYPE_LINK)
	{
		//gSpecialVar_Result = gBattleOutcome;
		gBattleTextBuff1[0] = gBattleOutcome;
		gBankAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
		gBattlescriptCurrInstr = BattleScript_LinkBattleWonOrLost;
		gBattleOutcome &= ~(B_OUTCOME_LINK_BATTLE_RAN);
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER
		  && gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_TOWER))
	{
		BattleStopLowHpSound();
		gBattlescriptCurrInstr = BattleScript_Victory;

		PlayBGM(BGM_VICTORY_TRAINER_BATTLE);
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && !(gBattleTypeFlags & BATTLE_TYPE_LINK))
	{
		BattleStopLowHpSound();
		gBattlescriptCurrInstr = BattleScript_Victory;

		u16 id = gTrainerBattleOpponent_A;
		u8 specialMus = FALSE;
		u8 loop = FALSE;

		if (IsFrontierTrainerId(id))
		{
			PlayBGM(BGM_VICTORY_TRAINER_BATTLE);
			goto SKIP_MUSIC_SELECTION;
		}

	VICTORY_MUSIC_SELECTION:
		switch (gTrainers[id].trainerClass) {
		#ifndef UNBOUND //Change this part
			case CLASS_LEADER:
			case CLASS_ELITE_4:
			case CLASS_CHAMPION:
				PlayBGM(BGM_VICTORY_SPECIAL);
				specialMus = TRUE;
				break;
			default:
				PlayBGM(BGM_VICTORY_TRAINER_BATTLE);
				break;

		#else //For Pokemon Unbound
			case CLASS_CHAMPION:
				PlayBGM(BGM_VICTORY_CHAMPION);
				specialMus = TRUE;
				break;
			case CLASS_ELITE_4:
				PlayBGM(BGM_VICTORY_ELITE_4);
				specialMus = TRUE;
				break;
			case CLASS_LEADER:
			case CLASS_FRONTIER_BRAIN:
				PlayBGM(BGM_VICTORY_GYM);
				specialMus = TRUE;
				break;
			case CLASS_SHADOW_ADMIN: //0x30
			case CLASS_BOSS: //0x53
			case CLASS_SHADOW: //0x55
			#ifndef DEBUG_UNBOUND_MUSIC
				PlayBGM(BGM_VICTORY_PLASMA);
				specialMus = TRUE;
				break;
			#endif
			case CLASS_LOR_LEADER: //0x2
			case CLASS_LOR_ADMIN: //0x2E
			case CLASS_LOR: //0x2F
				PlayBGM(BGM_VICTORY_GALACTIC);
				specialMus = TRUE;
				break;
			default:
				PlayBGM(BGM_VICTORY_TRAINER_BATTLE);
				break;
		#endif
		}

	SKIP_MUSIC_SELECTION:
		if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && !specialMus && !loop)
		{
			id = VarGet(VAR_SECOND_OPPONENT);
			loop = TRUE;
			goto VICTORY_MUSIC_SELECTION;
		}
	}
	else
	{
		gBattlescriptCurrInstr = BattleScript_PayDayMoneyAndPickUpItems;
	}

	gSpecialVar_LastResult = 0;
	gBattleMainFunc = HandleEndTurn_FinishBattle;
}

void HandleEndTurn_BattleLost(void)
{
	gCurrentActionFuncId = 0;
	if (HackDummy_Active()) //[hack] patches/0602 ポケセン 3F のダミー: 全滅しても「にげた」で終える（ぜんめつの文・所持金を半分にする処理・ワープが走らない）
	{
		gBattleOutcome = B_OUTCOME_RAN;
		HandleEndTurn_RanFromBattle();
		return;
	}
	HackChallenge_OnBattleLost(); //[hack] patches/0510 ぜんめつで戻されたときはポケセンに入ったと数えない（しばり）

	if (gBattleTypeFlags & BATTLE_TYPE_LINK)
	{
		if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
		{
			if (gBattleOutcome & B_OUTCOME_LINK_BATTLE_RAN)
			{
				gBattlescriptCurrInstr = BattleScript_PrintPlayerForfeitedLinkBattle;
				gBattleOutcome &= ~(B_OUTCOME_LINK_BATTLE_RAN);
			}
			else
			{
				gBattlescriptCurrInstr = BattleScript_LostMultiBattleTower;
				gBattleOutcome &= ~(B_OUTCOME_LINK_BATTLE_RAN);
			}
		}
		else
		{
			gBattleTextBuff1[0] = gBattleOutcome;
			gBankAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
			gBattlescriptCurrInstr = BattleScript_LinkBattleWonOrLost;
			gBattleOutcome &= ~(B_OUTCOME_LINK_BATTLE_RAN);
			EndOfBattleThings();
		}
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
	{
		if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
			gBattlescriptCurrInstr = BattleScript_LostMultiBattleTower;
		else
			gBattlescriptCurrInstr = BattleScript_LostBattleTower;

		gBattleOutcome &= ~(B_OUTCOME_RAN);
		EndOfBattleThings();
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && sTrainerBattleMode == TRAINER_BATTLE_OAK_TUTORIAL)
	{
		if (sTrainerBattleOakTutorialHelper & 1)
			gBattleCommunication[MULTISTRING_CHOOSER] = 1;
		else
			gBattleCommunication[MULTISTRING_CHOOSER] = 2;

		gBankAttacker = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
		gBattlescriptCurrInstr = BattleScript_LocalBattleLost;
	}
	else if (IsRaidBattle() && FlagGet(FLAG_BATTLE_FACILITY))
	{
		gBattlescriptCurrInstr = BattleScript_LoseFrontierRaidBattle;
		EndOfBattleThings();
	}
	else
	{
		gBattleCommunication[MULTISTRING_CHOOSER] = 0;
		gBattlescriptCurrInstr = BattleScript_LocalBattleLost;
	}

	gSpecialVar_LastResult = 1;
	gBattleMainFunc = HandleEndTurn_FinishBattle;
}

void HandleEndTurn_RanFromBattle(void)
{
	gCurrentActionFuncId = 0;

	if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
	{
		gBattlescriptCurrInstr = BattleScript_PrintPlayerForfeited;
		gBattleOutcome = B_OUTCOME_LOST;
	}
	else if (RAID_BATTLE_END)
	{
		gActiveBattler = BANK_RAID_BOSS;
		EmitSpriteInvisibility(0, TRUE);
		MarkBufferBankForExecution(gActiveBattler);

		gBattlescriptCurrInstr = BattleScript_RaidMonRanAway;
		gBattleOutcome = B_OUTCOME_WON;
	}
	else
	{
		switch (gProtectStructs[gBankAttacker].fleeFlag) {
			case 1:
				gBattlescriptCurrInstr = BattleScript_SmokeBallEscape; //0x81D8901
				break;
			case 2:
				gBattleScripting.bank = gBankAttacker;
				gBattlescriptCurrInstr = BattleScript_RanAwayUsingMonAbility; //0x81D890F
				break;
			default:
				gBattlescriptCurrInstr = BattleScript_GotAwaySafely; //0x81D8916
				break;
		}
	}

	gBattleMainFunc = HandleEndTurn_FinishBattle;
}

#define STATE_BEFORE_ACTION_CHOSEN 0

bool8 HandleRunActionFrontier(void)
{
	if (gBattleBufferB[gActiveBattler][1] == ACTION_RUN)
	{
		if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
		{
			if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER)
			{
				BattleScriptExecute(BattleScript_AskIfWantsToForfeitMatch);
				gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
				return TRUE;
			}
		}
		else if (!CantCatchBecauseFlag() //Wild battle where mon can be caught
		&& !IsRunningFromBattleImpossible()) //Can run right now
		{
			if ((GetMonData(&gEnemyParty[0], MON_DATA_HP, 0) != 0 && IsMonShiny(&gEnemyParty[0]))
			|| (IS_DOUBLE_BATTLE && GetMonData(&gEnemyParty[1], MON_DATA_HP, 0) != 0 && IsMonShiny(&gEnemyParty[1]))
			#ifdef FLAG_LEGENDARY_APPEARED_STRING
			|| FlagGet(FLAG_LEGENDARY_APPEARED_STRING)
			#endif
			)
			{
				BattleScriptExecute(BattleScript_ConfirmRunAway);
				gBattleCommunication[gActiveBattler] = STATE_BEFORE_ACTION_CHOSEN;
				return TRUE;
			}
		}
	}

	return FALSE;
}

#define ABILITY_PREVENTING_ESCAPE 2

bool8 AreAllKindsOfRunningPrevented(void)
{
	if (IsRaidBattle() && !RAID_BATTLE_END)
		return TRUE;
	#ifdef FLAG_NO_RUNNING
	else if (FlagGet(FLAG_NO_RUNNING))
		return TRUE;
	#endif
	#ifdef FLAG_NO_CATCHING_AND_RUNNING
	else if (FlagGet(FLAG_NO_CATCHING_AND_RUNNING))
		return TRUE;
	#endif

	return FALSE;
}

u8 IsRunningFromBattleImpossible(void)
{
	u8 itemEffect;
	u8 side;
	int i;

	itemEffect = ITEM_EFFECT(gActiveBattler);
	gStringBank = gActiveBattler;
	gBattleCommunication[MULTISTRING_CHOOSER] = 0;

	if (HackDummy_Active() && SIDE(gActiveBattler) == B_SIDE_PLAYER) //[hack] patches/0602 ポケセン 3F のダミーからは いつでも にげられる（にげる／R・特性やわざで とじこめられていても）
		return FALSE;
	else if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER))
		return FALSE;
	else if (RAID_BATTLE_END)
		return FALSE;
	else if (AreAllKindsOfRunningPrevented())
		return TRUE;
	else if (itemEffect == ITEM_EFFECT_CAN_ALWAYS_RUN)
		return FALSE;
	else if (gBattleMons[gActiveBattler].ability == ABILITY_RUNAWAY)
		return FALSE;
	else if (IsOfType(gActiveBattler, TYPE_GHOST))
		return FALSE;

	side = SIDE(gActiveBattler);

	for (i = 0; i < gBattlersCount; i++)
	{
		if (side != SIDE(i))
		{
			u16 ability = ABILITY(i);
			if (IsTrappedByAbility(gActiveBattler, ability))
			{
				gBattleScripting.bank = i;
				gLastUsedAbility = ABILITY(i);
				gBattleCommunication[MULTISTRING_CHOOSER] = ABILITY_PREVENTING_ESCAPE;
				return ABILITY_PREVENTING_ESCAPE;
			}
		}
	}

	if ((gBattleMons[gActiveBattler].status2 & (STATUS2_ESCAPE_PREVENTION | STATUS2_WRAPPED))
	|| (gStatuses3[gActiveBattler] & STATUS3_SKY_DROP_TARGET)
	|| !Lv200CustomMove_CanSwitch(gActiveBattler, TRUE)) //[hack] FIX-moves-P6 しんけんしょうぶ・まよいごのさそい（A のタイマー）でも にげられない（ゴーストは上で FALSE を返す）
	{
		gBattleCommunication[MULTISTRING_CHOOSER] = 0;
		return TRUE;
	}

	if (!gNewBS->TeleportBit)
	{
		if (gStatuses3[gActiveBattler] & STATUS3_ROOTED)
		{
			gBattleCommunication[MULTISTRING_CHOOSER] = 0;
			return 1;
		}
	}
	else
		gNewBS->TeleportBit = FALSE;

	if (IsFairyLockActive())
	{
		gBattleCommunication[MULTISTRING_CHOOSER] = 0;
		return TRUE;
	}

	if (gBattleTypeFlags & BATTLE_TYPE_OAK_TUTORIAL)
	{
		gBattleCommunication[MULTISTRING_CHOOSER] = 1;
		return TRUE;
	}

	return 0;
}

bool8 TryRunFromBattle(u8 bank)
{
	bool8 effect = FALSE;
	u8 itemEffect;
	u8 speedVar;

	itemEffect = ITEM_EFFECT(bank);
	gStringBank = bank;

	if (HackDummy_Active() && SIDE(bank) == B_SIDE_PLAYER) //[hack] patches/0602 ポケセン 3F のダミーからは かならず にげきれる
	{
		++effect;
	}
	else if (AreAllKindsOfRunningPrevented())
	{
		return FALSE;
	}
	else if (IsRaidBattle() && RAID_BATTLE_END)
	{
		++effect; //So you can always run at the end of a Raid
	}
	else if (IsOfType(bank, TYPE_GHOST))
	{
		++effect;
	}
	else if (itemEffect == ITEM_EFFECT_CAN_ALWAYS_RUN)
	{
		gLastUsedItem = ITEM(bank);
		gProtectStructs[bank].fleeFlag = 1;
		++effect;
	}
	else if (ABILITY(bank) == ABILITY_RUNAWAY)
	{
		gLastUsedAbility = ABILITY_RUNAWAY;
		gProtectStructs[bank].fleeFlag = 2;
		++effect;
	}
	#ifndef NO_GHOST_BATTLES
	else if ((gBattleTypeFlags & (BATTLE_TYPE_SCRIPTED_WILD_1 | BATTLE_TYPE_GHOST)) == BATTLE_TYPE_GHOST)
	{
		if (SIDE(bank) == B_SIDE_PLAYER)
			++effect;
	}
	#endif
	else if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER) && gBattleTypeFlags & BATTLE_TYPE_TRAINER)
	{
		++effect;
	}
	#ifdef QUICK_CLAW_PROCING_ALWAYS_ALLOWS_FLEEING
	else if (ITEM_EFFECT(bank) == ITEM_EFFECT_QUICK_CLAW && QuickClawActivatesThisTurn(bank))
	{
		++effect;
	}
	#endif
	else
	{
		if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
		{
		SINGLE_FLEE_CALC:
			if (gBattleMons[bank].speed < gBattleMons[BATTLE_OPPOSITE(bank)].speed)
			{
				speedVar = udivsi((gBattleMons[bank].speed * 128), (gBattleMons[BATTLE_OPPOSITE(bank)].speed)) + (gBattleStruct->runTries * 30);
				if (speedVar > (Random() & 0xFF))
					++effect;
			}
			else // same speed or faster
			{
				++effect;
			}
		}
		else //Wild Double
		{
			if (gBattleMons[BATTLE_OPPOSITE(bank)].hp)
				goto SINGLE_FLEE_CALC;

			if (gBattleMons[bank].speed < gBattleMons[PARTNER(BATTLE_OPPOSITE(bank))].speed)
			{
				speedVar = udivsi((gBattleMons[bank].speed * 128), (gBattleMons[PARTNER(BATTLE_OPPOSITE(bank))].speed)) + (gBattleStruct->runTries * 30);
				if (speedVar > (Random() & 0xFF))
					++effect;
			}
			else // same speed or faster
			{
				++effect;
			}
		}

		gBattleStruct->runTries++;
	}

	if (effect)
	{
		gCurrentTurnActionNumber = gBattlersCount;
		gBattleOutcome = B_OUTCOME_RAN;
	}

	return effect;
}

void BattleUseFunc_PokeDoll(u8 taskId)
{
	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
	&& !AreAllKindsOfRunningPrevented())
	{
		RemoveUsedItem();
		DisplayItemMessageInBag(taskId, 2, gStringVar4, ItemMenu_StartFadeToExitCallback);
	}
	else
		PrintNotTheTimeToUseThat(taskId, 0);
}

void HackMomSavings_WildWin(void); //[hack] patches/0224
void HackGymTier_OnBattleEnd(void); //[hack] patches/0470
void HackBadgeReplay_OnBattleEnd(void); //[hack] patches/0594 バッジリセット後のストーリー再戦

void HackBadgeSquash_Restore(void); //[hack] patches/0580 上限をこえた子の戻し（src/hack_battle_squash.c）
void HackRocketBall_Restore(void); //[hack] patches/0914 ロケットボール: 奪われた子を戻す（src/hack_rocket_ball.c）
void HackChallengeC_MonoRestore(void); //[hack] patches/0550 縛り C モノタイプ（src/hack_challenge_c.c）
void HackMoveEvo_OnBattleEnd(void); //[hack] patches/0878 技進化の使用回数を表へ書く（src/hack_move_evo.c）
void HackHeroine_AfterBattle(void); //[hack] patches/0875 #76 連れ歩き中のハルカ・ヒカリが戦闘のあと手持ちを回復（src/hack_heroine.c）

void EndOfBattleThings(void)
{
	if (gNewBS != NULL) //Hasn't been cleared yet
	{
		ShadowBattle_OnBattleEnd(); //0963
		HackChFGacha_OnBattleEnd(); //[hack] 縛り F わざガチャ: 手持ちの技が元のままの枠の PP・ポイントアップを戻す（全部の戻しの最初）
		HackMoveEvo_OnBattleEnd(); //[hack] patches/0878 技進化の使用回数: 変わった分だけ表へ（gNewBS を捨てる前・勝ち／負け／逃げ／捕獲の全部がここを通る）
		HackRocketBall_Restore(); //[hack] patches/0914 奪われた子を控えのまま戻す（ひんしでお別れ・BringBackTheDead・上限の戻しより前）
		Lv200CustomMove_OnBattleEnd();
		HackChallengeC_MonoRestore(); //[hack] patches/0550 出せないため ひんしにした子の HP を戻す（ひんしでお別れ＝縛り A の「外す」より前）
		TryRestoreEnemyTeam();
		NaturalCureHeal();
		RestoreNonConsumableItems();
		RevertDynamax();
		FormsRevert(gPlayerParty);
		MegaRevert(gPlayerParty);
		GigantamaxRevert(gPlayerParty);
		TerastalFormRevert(gPlayerParty);
		UpdateBurmy();
		EndPartnerBattlePartyRestore();
		EndSkyBattlePartyRestore();
		RecalcAllStats();
		HackBadgeSquash_Restore(); //[hack] patches/0580 戦闘のあいだだけ上限のレベルにした子を元のレベル・経験値に戻す（HP は割合・印が無ければ何もしない）
		HackDummy_Rescue(); //[hack] patches/0602 ポケセン 3F のダミー: 自分の側が全滅していたら全回復（BringBackTheDead の前）
		BringBackTheDead();
		if (!HackDummy_Active()) //[hack] patches/0602 ポケセン 3F のダミーの戦いは 貯金・再戦の段階・なつき度・ひんしでお別れ の どれも動かさない
		{
		HackMomSavings_WildWin(); //[hack] patches/0224 野生に勝ったらお母さんの貯金（src/hack_mom_savings.c）
		HackGymTier_OnBattleEnd(); //[hack] patches/0470 ジムリーダーの再戦に勝ったら段階 +1（src/hack_gym_tiers.c）
	HackBadgeReplay_OnBattleEnd(); //[hack] patches/0594 勝った専用再戦をこの一周では選べなくする
		HackAffection_BattleEnd(); //[hack] patches/0240 勝ったら技を出した子を 30% で なつき度 +2（src/hack_affection.c）
		HackChallenge_BuryFainted(); //[hack] patches/0530 しばり「ひんしでお別れ」: ひんしの子を手持ちから外す（全員ならゲームオーバー）
		HackHeroine_AfterBattle(); //[hack] patches/0875 #76 連れ歩き中のハルカ・ヒカリが回復（ひんしでお別れの外し方より後＝外した子は起きない・全滅・通信・施設は除く）
		}
		EndBattleFlagClear();
		HealPokemonInFrontier();
		gTerrainType = 0; //Reset now b/c normal reset is after BG is loaded
		CalculatePlayerPartyCount(); //Party size can change after multi battle is over

		#ifdef UNBOUND
		u8 weather = GetCurrentWeather();
		if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_SANDS)
		{
			if (weather != WEATHER_NONE
			&& weather != WEATHER_CLOUDS
			&& weather != WEATHER_SUNNY)
				SetSav1Weather(WEATHER_RAIN_LIGHT); //Reset weather in Battle Sands
		}
		else if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_CIRCUS)
		{
			SetSav1Weather(WEATHER_NONE);
		}
		#endif
	}
}

static void TryRestoreEnemyTeam(void)
{
	#ifdef FLAG_BACKUP_ENEMY_TEAM
	if (FlagGet(FLAG_BACKUP_ENEMY_TEAM))
	{
		Memcpy(gEnemyParty, gNewBS->foePartyBackup, sizeof(struct Pokemon) * PARTY_SIZE);
		Free(gNewBS->foePartyBackup);
	}
	#endif
}

static void NaturalCureHeal(void)
{
	for (int bank = 0; bank < gBattlersCount; ++bank)
	{
		if (ABILITY(bank) == ABILITY_NATURALCURE && gBattleMons[bank].status1)
			GetBankPartyData(bank)->condition = 0;
	}
}

extern bool8 HoennPike_KeepDamageAfterBattle(void); //src/hoenn_pike.c（patches/0099）

static void RestoreNonConsumableItems(void)
{
	u16 none = ITEM_NONE;
	u16* items = gNewBS->itemBackup;
	bool8 frontierRestore = (gBattleTypeFlags & BATTLE_TYPE_FRONTIER) && !HoennPike_KeepDamageAfterBattle(); //★ バトルチューブは使った持ち物を戻さない
	#ifdef FLAG_KEEP_CONSUMABLE_ITEMS
	bool8 keepConsumables = FlagGet(FLAG_KEEP_CONSUMABLE_ITEMS);
	#else
	bool8 keepConsumables = FALSE;
	#endif

	if (gBattleTypeFlags & BATTLE_TYPE_TRAINER || IsRaidBattle() || keepConsumables)
	{
		for (int i = 0; i < PARTY_SIZE; ++i)
		{
			if (items[i] == ITEM_NONE && gPlayerParty[i].item == ITEM_RIRU_LOVE) //[hack] patches/0617 戦闘中に捕まえた子（レイド）が持つ リルの あいじょうを「持ち物なし」に戻さない
				continue;
			if (frontierRestore
			||  keepConsumables
			||  items[i] == ITEM_NONE
			||  !IsConsumable(items[i]))
			{
				SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &items[i]);
			}
			else if (gPlayerParty[i].item != items[i] //The player consumed their item, and then picked up another one
			&& IsConsumable(items[i]))
			{
				SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &none);
			}
		}
	}

	HackItemRestore_Apply(); //[hack] 使いきった持ち物（きのみ以外）を野生戦でもトレーナー戦でも戻す（通信・施設・チューブは除く・src/hack_item_restore.c）
}

static void RevertDynamax(void)
{
	for (int i = 0; i < gBattlersCount; ++i)
		EndBattleDynamaxRevert(i);
}

static void RecalcAllStats(void)
{
	for (int i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
		if (species != SPECIES_NONE && species != SPECIES_EGG)
			CalculateMonStats(&gPlayerParty[i]); //Only recalc Pokemon that need to be recalced
	}
}

static void BringBackTheDead(void)
{
	//Used after Multi Battles that you lost, but your partner won
	if (ViableMonCount(gPlayerParty) == 0)
	{
		for (int i = 0; i < PARTY_SIZE; ++i)
		{
			u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
			if (species != SPECIES_NONE && species != SPECIES_EGG)
			{
				gPlayerParty[i].hp = 1;
				break;
			}
		}
	}
}

static void EndPartnerBattlePartyRestore(void)
{
	int i;
	u8 counter = 0;
	struct Pokemon* backup = ExtensionState.partyBackup;

	if (ExtensionState.partyBackup != NULL)
	{
		if (gSelectedOrderFromParty[0] == 0)
		{ 	//Special 0x2F was not used
			Memcpy(&gPlayerParty[3], ExtensionState.partyBackup, sizeof(struct Pokemon) * 3);
		}
		else
		{ 	//Special 0x2F was used
			struct Pokemon foughtMons[3];
			u8 newLeveledUpInBattle = 0;
			Memcpy(foughtMons, gPlayerParty, sizeof(struct Pokemon) * 3);
			Memset(gPlayerParty, 0x0, sizeof(struct Pokemon) * PARTY_SIZE);

			for (i = 0; i < 3; ++i)
			{
				if (gSelectedOrderFromParty[i] != 0) //Pokemon was chosen for battle
				{
					u8 correctSlotId = gSelectedOrderFromParty[i] - 1;
					Memcpy(&gPlayerParty[correctSlotId], &foughtMons[i], sizeof(struct Pokemon));
					if (gLeveledUpInBattle & gBitTable[i])
						newLeveledUpInBattle |= gBitTable[correctSlotId]; //Move to correct index
				}
			}

			//Pokemon that leveled up may move around after battle.
			//This is necessary so proper evolutions can happen after battle.
			gLeveledUpInBattle = newLeveledUpInBattle;

			for (i = 0; i < PARTY_SIZE; ++i)
			{
				if (!FlagGet(FLAG_BATTLE_FACILITY))
					gSelectedOrderFromParty[i] = 0; //Reset for next battle

				if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
					Memcpy(&gPlayerParty[i], &backup[counter++], sizeof(struct Pokemon));
			}
		}

		Free(ExtensionState.partyBackup);
	}
}

static void EndSkyBattlePartyRestoreWithTeam(struct Pokemon* tempTeam)
{
	u32 i;
	u8 counter = 0;
	struct Pokemon* backup = ExtensionState.skyBattlePartyBackup;

	if (backup != NULL)
	{
		u8 newLeveledUpInBattle = 0;
		bool8 slotFilled[PARTY_SIZE] = {FALSE};

		for (i = 0; i < PARTY_SIZE; ++i)
		{
			if (gSelectedOrderFromParty[i] != 0)
			{
				u8 correctSlotId = gSelectedOrderFromParty[i] - 1;
				tempTeam[correctSlotId] = gPlayerParty[i];
				slotFilled[correctSlotId] = TRUE;
				gSelectedOrderFromParty[i] = 0;

				if (gLeveledUpInBattle & gBitTable[i])
					newLeveledUpInBattle |= gBitTable[correctSlotId]; //Move to correct index
			}
		}

		gLeveledUpInBattle = newLeveledUpInBattle; //Same logic as in above function

		for (i = 0; i < PARTY_SIZE; ++i)
		{
			if (!slotFilled[i])
			{
				tempTeam[i] = backup[counter++];
				slotFilled[i] = TRUE;
			}
		}

		Memcpy(gPlayerParty, tempTeam, sizeof(struct Pokemon) * PARTY_SIZE);
		Free(backup);
	}
}

/* 対応環境の通常経路へ、大きな配列のスタック枠を持ち込まない。 */
static void __attribute__((noinline)) EndSkyBattlePartyRestoreLegacy(void)
{
	struct Pokemon tempTeam[PARTY_SIZE] = {0};
	EndSkyBattlePartyRestoreWithTeam(tempTeam);
}

static void EndSkyBattlePartyRestore(void)
{
	struct Pokemon* tempTeam;
	if (ExtensionState.skyBattlePartyBackup == NULL)
		return;
	tempTeam = HackXramBattle_TempTeam();
	if (tempTeam != NULL)
		EndSkyBattlePartyRestoreWithTeam(tempTeam);
	else
		EndSkyBattlePartyRestoreLegacy();
}

static void EndBattleFlagClear(void)
{
	u32 i;

	HackPhone_OakOnBattleEnd();
	for (i = 0; i < ARRAY_COUNT(gEndBattleFlagClearTable); ++i)
		FlagClear(gEndBattleFlagClearTable[i]);

	#ifdef VAR_STATUS_INDUCER
	u16 inducer = VarGet(VAR_STATUS_INDUCER);
	if (inducer & 0xFF00) //Temporary status inducer
	{
		u8 status = inducer & 0xFF;
		u8 amount = ((inducer & 0xFF00) >> 8) - 1; //Subtract num battles by 1

		if (amount == 0) //Time's up
			VarSet(VAR_STATUS_INDUCER, 0);
		else
			VarSet(VAR_STATUS_INDUCER, status | (amount << 8));
	}
	#endif

	//Handle Sirfetch'd Evolution
	gScored3CritsInBattle = 0;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (gNewBS->criticalHitsThisBattle[i] >= 3)
		{
			gScored3CritsInBattle |= gBitTable[i];
			if (EvolvesViaScoring3Crits(&gPlayerParty[i])
			&& !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_SAFARI | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_BENJAMIN_BUTTERFREE | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER)))
				gLeveledUpInBattle |= gBitTable[i];
		}
	}

	//Reset Totem Vars
	VarSet(VAR_TOTEM + 0, 0);	//Bank B_POSITION_PLAYER_LEFT's Stat
	VarSet(VAR_TOTEM + 1, 0);	//Bank B_POSITION_OPPONENT_LEFT's Stat
	VarSet(VAR_TOTEM + 2, 0);	//Bank B_POSITION_PLAYER_RIGHT's Stat
	VarSet(VAR_TOTEM + 3, 0);	//Bank B_POSITION_OPPONENT_RIGHT's Stat

	VarSet(VAR_TERRAIN, 0);
	VarSet(VAR_BATTLE_FACILITY_TRAINER1_NAME, 0xFFFF);
	VarSet(VAR_BATTLE_FACILITY_TRAINER2_NAME, 0xFFFF);
	#ifdef VAR_BATTLE_TRANSITION_LOGO
	VarSet(VAR_BATTLE_TRANSITION_LOGO, 0);
	#endif
	gFishingByte = FALSE;
	HackAura_RestoreMixer(); //[hack] patches/0956 オーラの曲の音量・リバーブ・写しを戻す（gNewBS を解放する前）
	HackXramBattle_Free(gNewBS);
	gNewBS = NULL;

	//Handle DexNav Chain
	if (gDexNavStartedBattle
	&& (gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_CAUGHT))
	{
		if (gCurrentDexNavChain == 0)
			gCurrentDexNavChain = 2; //Next battle would be battle 2
		else if (gCurrentDexNavChain < 100)
			++gCurrentDexNavChain;
		//Hack (patches/0170, 2026-10-04 user): stay at 100 instead of restarting from 1, so the 100-chain level and shiny bonuses keep applying
	}
	else
		gCurrentDexNavChain = 0;
	gDexNavStartedBattle = FALSE;

	u16 backup = gTrainerBattleOpponent_B;
	Memset(&ExtensionState, 0x0, sizeof(struct BattleExtensionState));
	gTrainerBattleOpponent_B = backup;
	gBattleCircusFlags = 0;
	Memset(gStatuses3, 0, sizeof(gStatuses3));
}

static void HealPokemonInFrontier(void)
{
	//This helps with the issue of multi battle teams in the Battle Tower are not healed
	if (gBattleTypeFlags & BATTLE_TYPE_FRONTIER && !HoennPike_KeepDamageAfterBattle()) //★ バトルチューブは回復しない（patches/0099）
	{
		for (int i = 0; i < PARTY_SIZE; ++i)
		{
			struct Pokemon* mon = &gPlayerParty[i];

			if (GetMonData(mon, MON_DATA_SPECIES, NULL) != SPECIES_NONE)
				HealMon(mon);
		}
	}
}

bool8 IsConsumable(u16 item)
{
	u8 effect = gItems[SanitizeItemId(item)].holdEffect;

	for (u32 i = 0; gConsumableItemEffects[i] != 0xFF; ++i)
	{
		if (effect == gConsumableItemEffects[i])
			return TRUE;
	}

	return FALSE;
}
