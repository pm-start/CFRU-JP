#include "defines.h"
#include "defines_battle.h"
#include "hack_kind.h"
#include "hack_snatch_ball.h"
#include "shadow_relive.h"   //[hack] patches/0965 段 G
extern const u8 gText_Relive_BoxFull[];
#include "../include/battle_interface.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/m4a.h"
#include "../include/sound.h"
#include "../include/task.h"
#include "../include/constants/items.h"
#include "../include/constants/songs.h"

#include "../include/new/battle_util.h"
#include "../include/new/catching.h"

/*
 * hack_snatch_ball.c（段 F・patches/0961）。判定の型は src/hack_rocket_ball.c（組織のトレーナー側の「奪う」）に合わせてあるが、こちらは主人公が奪う側で道具がある。
 * 「組織のトレーナーのダーク」かどうかは個体の印（HackKind_Get == HACK_KIND_DARK）だけで見る＝印は組織のトレーナーの手持ちにしか付かない（段 A2）。
 */

extern const u8 gText_CantCatchPokemonRightNow[]; //strings/general_battle_strings.string

//Whether the player may throw this item at all in this battle (bag check) is decided in HackSnatch_GetBagRefusal.

bool8 HackSnatch_IsSnatchBall(u16 item)
{
	return item == ITEM_SNATCH_BALL;
}

static bool8 IsDarkFoe(u8 bank)
{
	return bank < gBattlersCount
		&& SIDE(bank) == B_SIDE_OPPONENT
		&& BATTLER_ALIVE(bank)
		&& !(gAbsentBattlerFlags & gBitTable[bank])
		&& HackKind_Get((const struct BoxPokemon*) GetBankPartyData(bank)) == HACK_KIND_DARK;
}

u8 HackSnatch_FindDarkFoe(void)
{
	u8 right = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
	u8 left = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

	if (IS_DOUBLE_BATTLE && IsDarkFoe(right))
		return right;
	if (IsDarkFoe(left))
		return left;
	return HACK_SNATCH_NO_FOE;
}

static bool8 FlagOpen(void)
{
	#ifdef FLAG_CATCH_TRAINERS_POKEMON
	return FlagGet(FLAG_CATCH_TRAINERS_POKEMON);
	#else
	return FALSE;
	#endif
}

//バッグの「つかう」（戦闘中・ボール）で断る文。NULL なら通す。
// 普通のボール: トレーナー戦で旗が無く、場にダークの個体がいるなら断る（ダークでない個体は今までの動きのまま＝投げてからトレーナーが弾く）
// スナッチボール: トレーナー戦で（ダークの個体がいる・または旗が立っている）以外は断る（野生・ダークでない個体）
const u8* HackSnatch_GetBagRefusal(u16 item)
{
	bool8 trainer = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) != 0;
	bool8 dark = HackSnatch_FindDarkFoe() != HACK_SNATCH_NO_FOE;

	if (HackSnatch_IsSnatchBall(item))
	{
		if (!trainer || (!dark && !FlagOpen()))
			return gText_Snatch_OnlyDark;
		if (dark && CalculatePlayerPartyCount() >= PARTY_SIZE && !Relive_HasRoom())
			return gText_Relive_BoxFull;   //[hack] patches/0965 段 G／0975 案 B: 捕まえたダークの行き先（手持ち満員なら専用のリライブボックス）が無い（追加の領域なし・満杯・最初のレポートの前）
		return NULL;
	}
	if (trainer && !FlagOpen() && dark)
		return gText_Snatch_NeedSnatchBall;
	return NULL;
}

bool8 HackSnatch_BagSkipsDoubleChecks(u16 item)
{
	return HackSnatch_IsSnatchBall(item)
		&& (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
		&& HackSnatch_FindDarkFoe() != HACK_SNATCH_NO_FOE;
}

//callasm at the start of BattleScript_SnatchCaught
void HackSnatch_Start(void)
{
	gNewBS->snatchPartyCount = CalculatePlayerPartyCount();
}

//callasm after catchpoke: the "sent to the PC" message only when it went to the PC (vanilla shows it for a full party)
void HackSnatch_SkipPcMessageIfInParty(void)
{
	if (CalculatePlayerPartyCount() > gNewBS->snatchPartyCount)
		gBattlescriptCurrInstr = BattleScript_SnatchCaught_Remove - 5;
}

//callasm after catchpoke (the player has the Pokemon now): out of the Trainer's usable party like a knocked out one
void HackSnatch_RemoveCaughtMon(void)
{
	u8 bank = gBankTarget;
	u16 hp = 0;
	u32 i;

	SetMonData(GetBankPartyData(bank), MON_DATA_HP, &hp);
	gBattleMons[bank].hp = 0;
	gHitMarker |= HITMARKER_FAINTED(bank);

	for (i = gCurrentTurnActionNumber + 1; i < gBattlersCount; ++i) //its action later in this turn is dropped
	{
		if (gBanksByTurnOrder[i] == bank)
			gActionsByTurnOrder[i] = ACTION_FINISHED;
	}

	SetHealthboxSpriteInvisible(gHealthboxSpriteIds[bank]);
	gNewBS->snatchActive = FALSE;
}

//callasm of BattleScript_SnatchNoTarget: no ball was thrown (the target is gone), the one taken from the bag goes back
void HackSnatch_ReturnBall(void)
{
	AddBagItem(gLastUsedItem, 1);
}

//----- 捕まえたジングル（ボールの演出の 95 フレーム目の hook）-----
//原版はここで全部の曲を止めて「捕まえた」ジングルを鳴らし、あとの文の PLAY_BGM で捕獲の曲にする。トレーナー戦は続くので、
//戦闘 BGM は止めるだけにして、ジングルが終わったら続きから鳴らす（文は 0x10B でなく gText_Snatch_Gotcha＝BGM の命令なし）。

#define CAUGHT_JINGLE_MIN_FRAMES 220 //BPRJ a_fight.c: MB_GET_SE_WAIT = MB_GET_WAIT2 + 220
#define CAUGHT_JINGLE_MAX_FRAMES 300 //never leave a stopped BGM waiting forever

static bool8 TryResumeBattleBgm(void)
{
	const struct Song* jingle = &gSongTable[SE_CAUGHT_POKEMON];
	struct MusicPlayerInfo* player = gMPlayTable[jingle->ms].info;

	if (!gMain.inBattle || gNewBS == NULL || gNewBS->snatchJingleWait == 0)
		return TRUE;

	m4aMPlayStop(&gMPlayInfo_BGM);
	if (gNewBS->snatchJingleWait++ <= CAUGHT_JINGLE_MIN_FRAMES
	|| (player->songHeader == jingle->header && (player->status & MUSICPLAYER_STATUS_TRACK) != 0
		&& !(player->status & MUSICPLAYER_STATUS_PAUSE) && gNewBS->snatchJingleWait <= CAUGHT_JINGLE_MAX_FRAMES))
		return FALSE;

	gNewBS->snatchJingleWait = 0;
	m4aMPlayContinue(&gMPlayInfo_BGM);
	return TRUE;
}

static void Task_ResumeBattleBgmAfterCaughtJingle(u8 taskId)
{
	if (TryResumeBattleBgm())
		DestroyTask(taskId);
}

void HackSnatch_PlayCaughtJingle(void)
{
	m4aMPlayAllStop();
	if (gNewBS != NULL && gNewBS->snatchActive)
		gNewBS->snatchJingleWait = 1;

	PlaySE(SE_CAUGHT_POKEMON); //0x13F

	if (gNewBS != NULL && gNewBS->snatchActive && !FuncIsActiveTask(Task_ResumeBattleBgmAfterCaughtJingle))
		CreateTask(Task_ResumeBattleBgmAfterCaughtJingle, 255);
}

//callasm of BattleScript_SnatchCaught: hold capture processing behind the jingle
void HackSnatch_WaitCaughtJingle(void)
{
	if (gNewBS->snatchJingleWait != 0 && !FuncIsActiveTask(Task_ResumeBattleBgmAfterCaughtJingle))
		TryResumeBattleBgm();
	if (gNewBS->snatchJingleWait != 0)
		gBattlescriptCurrInstr -= 5; //This callasm again next frame
}
