#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/task.h"
#include "../include/sprite.h"
#include "../include/new/battle_anims.h"
#include "hack_contest.h"

/*
 * コンテスト移植 S0（FEAT-contest・2026-11-09）: 技のアニメの試作。デバッグ版だけ（HACK_DEBUG_MENU）。
 *
 * 目的: 素の FR＋CFRU の技のアニメ台本が、「相手がいない画面・自分のポケモン 1 匹」でも崩れず動くかを見る
 *   （エメラルドのコンテストの技のアニメ APPEALSTATE_MOVE_ANIM = DoMoveAnim(move) と同じ呼び方）。
 * 使い方（tools/test/contest_anim_proto_mgba.py）:
 *   野生戦のコマンド選択中に gBattleMainFunc を ContestProto_Main に書き換える →
 *   専用のタスクができる。タスクの data[7] に技の番号を書くと、その技のアニメを 1 回流す。
 *   data[3]: 0＝待機 1＝流している 2＝終わった 3＝1200 コマ待っても終わらない（固まり）・data[4]＝かかったコマ数・
 *   data[5]＝流している間の経過コマ・data[6]＝試した技の番号の控え
 * 条件: 攻撃側＝0 番（自分のポケモン）・相手の絵は消す（invisible）・自分向けの技は対象も 0 番、そのほかは 1 番（消した相手の位置）
 * 書き換え可能な static は使えない（6.13）＝状態はタスクの data[] だけ
 * S1: data[8] に 1＝HackContest_Open・2＝HackContest_Close を書くと 1 回実行（実行後に 0 へ戻る）。data[9]＝素の 0x0800108C（hooks の踏み台）を
 *   実際に呼んだ戻り値（毎コマ更新）・data[10]＝HackContest_Open／Close の最後の戻り値 +1（0＝未実行）
 */

#ifdef HACK_DEBUG_MENU

#define PROTO_TIMEOUT 1200
#define PROTO_MOVETURN (*((u8*) 0x02037E4A)) /* 素の FR の gAnimMoveTurn（BPRJ.ld に無い・sAnimMoveIndex 0x2037E4C の 2 つ前・pokefirered の EWRAM の並びから推定・実機で確かめる） */

static void Task_ContestProto(u8 taskId)
{
	struct Task* t = &gTasks[taskId];
	u16 move;

	if (t->data[8] == 1) {
		t->data[10] = (s16) HackContest_Open() + 1;
		t->data[8] = 0;
	} else if (t->data[8] == 2) {
		HackContest_Close();
		t->data[10] = 1;
		t->data[8] = 0;
	}
	t->data[9] = ((bool8 (*)(void)) (0x0800108C | 1))(); //hooks で踏み台に置き換えた素の入口（IsContest の 110 か所の呼び元と同じ道）

	switch (t->data[0]) {
	case 0: //待機
		if (t->data[7] != 0) {
			move = (u16) t->data[7];
			t->data[6] = move;
			t->data[7] = 0;
			t->data[3] = 1;
			t->data[4] = 0;
			t->data[5] = 0;
			gBankAttacker = 0;
			if (gBattleMoves[move].target & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_PARTNER))
				gBankTarget = 0;
			else
				gBankTarget = 1;
			PROTO_MOVETURN = 0;
			gSprites[gBattlerSpriteIds[1]].invisible = TRUE;
			DoMoveAnim(move);
			t->data[0] = 1;
		}
		break;
	case 1: //アニメが終わるまで 1 コマずつ進める（コンテストの APPEALSTATE_WAIT_MOVE_ANIM と同じ）
		t->data[5]++;
		gAnimScriptCallback();
		if (!gAnimScriptActive) {
			t->data[4] = t->data[5];
			t->data[3] = 2;
			t->data[0] = 0;
		} else if (t->data[5] > PROTO_TIMEOUT) {
			t->data[4] = t->data[5];
			t->data[3] = 3;
			t->data[0] = 2;
		}
		break;
	case 2: //固まり: 動かさない
		break;
	}
}

static void ContestProto_Idle(void)
{
}

void ContestProto_Main(void)
{
	u8 taskId = CreateTask(Task_ContestProto, 1);
	gTasks[taskId].data[0] = 0;
	gTasks[taskId].data[3] = 0;
	gTasks[taskId].data[7] = 0;
	gTasks[taskId].data[15] = 0x5A5A; //試験の道具が gTasks から探す目印
	gBattleMainFunc = ContestProto_Idle;
}

#endif
