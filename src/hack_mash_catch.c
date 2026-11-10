#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/new/catching.h"
#include "hack_mash_catch.h"

/*
    A ボタン連打で捕獲率が上がる（隠し機能・patches/0423・2026-10-16 ユーザー「隠し機能として A ボタン連打で捕獲率上がらない？」
    →「1.5 倍まで・ボールの補正値に足す」→「待ち時間を足さない」→「ビーストボールは対象外」）。
    説明・攻略メモ・パッチノートには書かない。資料: docs/research/2026-10-16_mash-catch.md

    - 数える時間（待ちは足さない）: ボールを使うと決まったターンの始め（SetActionsAndBanksTurnOrder・src/battle_start_turn_start.c）から、
      捕獲の命令 atkEF_handleballthrow が呼ばれる瞬間まで。ゲームの流れはそのまま（「〜は ボールを つかった！」の文が出る間）。
      数えるのは小さなタスク（Task_MashCatchCount・毎コマ動く）。捕獲の命令は文が出ている間は呼ばれない（gBattleExecBuffer）ので、
      命令の中では数えられないため。
    - 文の早送りを止める: 戦闘の文の窓は A／B を押すと文字の送りが速くなる（canABSpeedUpPrint・decomp の BattlePutTextOnWindow）。
      連打で文が短くなると数える時間が減るので、数えている間は毎コマこの旗を FALSE にして、押しても文の進み方が変わらないようにする。
      旗は次の文が出るとき TRUE に戻る（文を出す関数がそのつど書く）ので、あとに残らない。
    - 反映: 結果を決める前。ボールの補正値（GetBaseBallCatchOdds の ballMultiplier・10 が 1 倍）に
      1 回につき +0.05（1/100 単位で +5）・10 回で頭打ち＝最大 +0.5（モンスターボール 1.0 → 1.5・ハイパーボール 2.0 → 2.5）。
      端数は出ない（1/100 単位）。回数 0 のときは本来の式と同じ値。
    - 効かない場面: トレーナー戦・通信・サファリ・チュートリアル・ゴースト・捕獲禁止の旗・必ず捕まる旗・マスターボール・パークボール・
      ビーストボール・伝説レイド。ウルトラビーストにビーストボール以外を投げたときの 0.1 倍は変えない（足さない）。
    - 状態は gNewBS（戦闘のあいだだけ Calloc された入れ物）の末尾の 2 バイトと、タスクの data[0]（経過コマ）。static は使わない（CLAUDE.md 6.13）。
*/

#define MASH_CATCH_MAX_PRESSES     10
#define MASH_CATCH_BONUS_PER_PRESS 5   //1/100 単位（10 回で 50 ＝ ballMultiplier の +5 ＝ +0.5）
#define MASH_CATCH_TASK_MAX_FRAMES 300 //投げずに終わったターンで数え続けない（5 秒）

extern bool8 HackLegendRaid_Active(void);

static bool8 MashCatchApplies(u8 ballType)
{
	if (gNewBS == NULL || gNewBS->isTrainerBattle)
		return FALSE;

	if (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK | BATTLE_TYPE_SAFARI | BATTLE_TYPE_GHOST
						  | BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_POKE_DUDE))
		return FALSE;

	if (ballType == BALL_TYPE_MASTER_BALL || ballType == BALL_TYPE_PARK_BALL || ballType == BALL_TYPE_SAFARI_BALL
	 || ballType == BALL_TYPE_BEAST_BALL)
		return FALSE;

	#ifdef FLAG_NO_CATCHING
	if (FlagGet(FLAG_NO_CATCHING))
		return FALSE;
	#endif
	#ifdef FLAG_NO_CATCHING_AND_RUNNING
	if (FlagGet(FLAG_NO_CATCHING_AND_RUNNING))
		return FALSE;
	#endif
	#ifdef FLAG_ALWAYS_CATCHABLE
	if (FlagGet(FLAG_ALWAYS_CATCHABLE))
		return FALSE;
	#endif

	if (HackLegendRaid_Active())
		return FALSE;

	return TRUE;
}

static void Task_MashCatchCount(u8 taskId)
{
	if (gNewBS == NULL || !gNewBS->hackMashActive)
	{
		DestroyTask(taskId); //捕獲の命令が呼ばれた（Finish）・戦闘が終わった
		return;
	}

	if (++gTasks[taskId].data[0] > MASH_CATCH_TASK_MAX_FRAMES)
	{
		gNewBS->hackMashActive = FALSE; //投げずに終わった
		gNewBS->hackMashCount = 0;
		DestroyTask(taskId);
		return;
	}

	if (JOY_NEW(A_BUTTON) && gNewBS->hackMashCount < MASH_CATCH_MAX_PRESSES)
		++gNewBS->hackMashCount;

	gTextFlags.canABSpeedUpPrint = FALSE; //A／B で文が速く進まない（数える時間を縮めない）
}

void HackMashCatch_Begin(u16 item)
{
	if (!MashCatchApplies(ItemId_GetType(item)))
		return;

	gNewBS->hackMashCount = 0;
	if (gNewBS->hackMashActive)
		return; //タスクはもう動いている（数だけ 0 から）

	gNewBS->hackMashActive = TRUE;
	CreateTask(Task_MashCatchCount, 0);
}

void HackMashCatch_Finish(void)
{
	if (gNewBS == NULL || !gNewBS->hackMashActive)
		return;

	if (JOY_NEW(A_BUTTON) && gNewBS->hackMashCount < MASH_CATCH_MAX_PRESSES)
		++gNewBS->hackMashCount; //この命令が呼ばれたコマの押下（タスクはこのあと動くので）

	gNewBS->hackMashActive = FALSE; //タスクは次のコマで自分を消す
}

u16 HackMashCatch_Bonus100(void)
{
	if (gNewBS == NULL)
		return 0;

	return gNewBS->hackMashCount * MASH_CATCH_BONUS_PER_PRESS; //回数は 10 で頭打ち（上の 2 か所）
}

void HackMashCatch_Clear(void)
{
	if (gNewBS != NULL)
		gNewBS->hackMashCount = 0;
}

#ifdef HACK_DEBUG_MENU
bool8 HackMashCatch_AppliesToBall(u8 ballType) //[hack] patches/0425 デバッグの捕獲情報が「連打が効くか」を聞く（効く場面の判定は上の MashCatchApplies と同じ）
{
	return MashCatchApplies(ballType);
}
#endif
