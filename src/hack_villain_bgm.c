#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/event_data.h"
#include "../include/m4a.h"
#include "../include/constants/trainer_classes.h"
#include "../include/new/frontier.h"
#include "../include/new/ram_locs_battle.h"

/*
 * 悪の組織の戦闘 BGM を BW 音源のアレンジに（FEAT-villain-bgm-bw・2026-10-28・Redmine #205・patches/0936）。
 * 曲は tools/bgm/villain_bgm.py が作る（ホワイトのプラズマ団の戦闘曲の楽器へ配役し直したギンガ団・アカギ・ロケット団、ホワイトのゲーチス戦の曲そのもの）。
 * 後ろ半分（0x0A000000〜）の曲として組み込み、曲の表の枠 545〜549 を GetMUS_ForBattle（src/battle_start_turn_start.c）の頭から呼ぶ（hack_oak_bgm.c と同じ形）。0 を返せば今までどおりの曲。
 * - ギンガだんしたっぱ／かんぶ／ボス・ロケットだん（クラス 0x55・したっぱもかんぶも）・ヤマブキのサカキ（クラス 0x53 のトレーナー 348／349＝ロケットアジトとシルフ。トキワジムの 350 は対象外）。
 * - 通信・フロンティア（施設のトレーナー）は今のまま。二人組みの相手は 2 人目も見る。
 * - 後ろ半分が見えない ROM（SuperCard の 32MB・--rom32・古い mGBA）は枠が前半の無音の見出しのままなので、ここで 0 を返して今の曲に戻す
 *   （判定は src/hack_oak_bgm.c と同じ: HackSplit_Native かつ 枠の見出しが 0x0A000000〜0x0BFFFFFF）。
 * 枠の番号は tools/bgm/verify_villain_bgm.py が曲の表（song_table.s）・目録 villain_songs.json と同じことを確かめる。
 */
bool8 HackSplit_Native(void); //src/hack_split_pics.c（0x0A000000 に後ろ半分が見える 64MB の ROM か）

#define HACK_VILLAIN_SONG_GALACTIC_GRUNT 545
#define HACK_VILLAIN_SONG_GALACTIC_CMDR  546
#define HACK_VILLAIN_SONG_CYRUS          547
#define HACK_VILLAIN_SONG_ROCKET         548
#define HACK_VILLAIN_SONG_GHETSIS        549
#define HACK_VILLAIN_SAKAKI_ROCKET_HIDEOUT 348
#define HACK_VILLAIN_SAKAKI_SILPH          349

static u16 VillainSongOf(u16 trainerId)
{
	switch (gTrainers[trainerId].trainerClass)
	{
		case CLASS_GALACTIC_GRUNT:
			return HACK_VILLAIN_SONG_GALACTIC_GRUNT;
		case CLASS_GALACTIC_ADMIN:
			return HACK_VILLAIN_SONG_GALACTIC_CMDR;
		case CLASS_GALACTIC_BOSS:
			return HACK_VILLAIN_SONG_CYRUS;
		case CLASS_TEAM_ROCKET:
			return HACK_VILLAIN_SONG_ROCKET;
		case CLASS_BOSS:
			if (trainerId == HACK_VILLAIN_SAKAKI_ROCKET_HIDEOUT || trainerId == HACK_VILLAIN_SAKAKI_SILPH)
				return HACK_VILLAIN_SONG_GHETSIS;
			break;
	}
	return 0;
}

u16 HackVillain_BattleMusic(void)
{
	u16 song;
	u32 header;

	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)))
		return 0;
	if (IsFrontierTrainerId(gTrainerBattleOpponent_A))
		return 0;
	song = VillainSongOf(gTrainerBattleOpponent_A);
	if (song == 0 && (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) && !IsFrontierTrainerId(SECOND_OPPONENT))
		song = VillainSongOf(SECOND_OPPONENT);
	if (song == 0)
		return 0;
	if (!HackSplit_Native())
		return 0;
	header = (u32) gSongTable[song].header;
	if (header < 0x0A000000 || header >= 0x0C000000)
		return 0;
	return song;
}
