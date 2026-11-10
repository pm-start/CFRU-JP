#include "defines.h"
#include "defines_battle.h"
#include "hack_watashitachi.h"

/*
 * カントーのジムリーダーの 1 回目の戦いでもダイマックスを使えるように（2026-10-03 ユーザー「1回目からで」）。
 * 素の FRLG のジムのスクリプトは FLAG_DYNAMAX_BATTLE（0x918）を立てないので、戦いを始めるときに番号で見る
 * （src/overworld.c・patches/0144）。再戦（927〜934・assembly/kanto/kanto_leader_rematch.s）は今までどおりスクリプトが旗を立てる。
 * ダイマックスバンドは最初にお母さんからもらう（assembly/kanto/kanto_home_scripts.s）。
 */

//pokefirered include/constants/opponents.h（番号は素の FRLG と同じ・tools/kanto/build_kanto_trainers.py も同じ番号を置き直す）
static const u16 sKantoGymLeaders[] =
{
	414, //TRAINER_LEADER_BROCK
	415, //TRAINER_LEADER_MISTY
	416, //TRAINER_LEADER_LT_SURGE
	417, //TRAINER_LEADER_ERIKA
	418, //TRAINER_LEADER_KOGA
	419, //TRAINER_LEADER_BLAINE
	420, //TRAINER_LEADER_SABRINA
	350, //TRAINER_LEADER_GIOVANNI
};

bool8 HackKanto_IsGymLeaderBattle(void)
{
	u32 i;

	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
		return FALSE;
	for (i = 0; i < ARRAY_COUNT(sKantoGymLeaders); ++i)
	{
		if (gTrainerBattleOpponent_A == sKantoGymLeaders[i])
			return TRUE;
	}
	return FALSE;
}

/*
 * 全地方のチャンピオン戦でもダイマックスを使えるように（2026-10-15 ユーザー確定・patches/0400）。四天王は今までどおり使えない。
 * 番号は各地方の trainers 表の添字（出典 docs/research/2026-10-15_champion-dynamax.md）。どれも手持ちの道具にダイマックスバンド（347）を持つ。
 */
static const u16 sChampions[] =
{
	438, //カントー TRAINER_CHAMPION_FIRST_SQUIRTLE（src/Tables/kanto_trainers.h）
	439, //カントー TRAINER_CHAMPION_FIRST_BULBASAUR
	440, //カントー TRAINER_CHAMPION_FIRST_CHARMANDER
	739, //カントー TRAINER_CHAMPION_REMATCH_SQUIRTLE
	740, //カントー TRAINER_CHAMPION_REMATCH_BULBASAUR
	741, //カントー TRAINER_CHAMPION_REMATCH_CHARMANDER
	813, //ホウエン TRAINER_WALLACE（ミクリ・src/Tables/hoenn_trainers.h）
	818, //ホウエン TRAINER_WALLACE_2
	916, //ジョウト TRAINER_CHAMPION_LANCE（ワタル・src/Tables/johto_trainers.h）
	926, //シンオウ TRAINER_CHAMPION_CYNTHIA（シロナ・src/Tables/sinnoh_trainers.h）
};

bool8 HackDynamax_IsChampionBattle(void)
{
	u32 i;

	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
		return FALSE;
	for (i = 0; i < ARRAY_COUNT(sChampions); ++i)
	{
		if (gTrainerBattleOpponent_A == sChampions[i])
			return TRUE;
	}
	return FALSE;
}

/*
 * 隠しトレーナー「わたしたち」10 人（940〜949）との戦いは、相手だけダイマックスできる（2026-10-15 ユーザー確定・patches/0402）。
 * 相手は手持ちの道具のダイマックスバンド（347・tools/rom/build_hoenn_boss_sets.py が boss に付ける）で使える。
 * 主人公がわ（パートナー含む）は止める＝DynamaxEnabled・CanDynamax・CanGigantamax・技の画面の START・トリガーの表示。
 */
bool8 HackDynamax_IsWatashitachiBattle(void)
{
	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
		return FALSE;
	return gTrainerBattleOpponent_A >= WATASHI_TRAINER_FIRST && gTrainerBattleOpponent_A < WATASHI_TRAINER_FIRST + WATASHI_COUNT;
}

bool8 HackDynamax_PlayerBlocked(u8 bank)
{
	return SIDE(bank) == B_SIDE_PLAYER && HackDynamax_IsWatashitachiBattle();
}
