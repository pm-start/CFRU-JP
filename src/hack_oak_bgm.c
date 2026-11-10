#include "defines.h"
#include "../include/battle.h"
#include "../include/event_data.h"
#include "../include/m4a.h"
#include "../include/new/ram_locs_battle.h"
#include "hack_oak_phone.h"

/*
 * オーキド戦（電話から始まる平均 Lv170 の戦い）の戦闘 BGM（FEAT-oak-bgm・2026-10-25・Redmine #118・patches/0820）。
 * ユーザーがもらった曲（BW 風・3 分 42 秒）を、後ろ半分（0x0A000000〜）の PCM の曲として組み込み（tools/bgm/back_pcm.py の key x_oak_bw・
 * 曲の表の枠 HACK_OAK_BGM_SONG）、GetMUS_ForBattle（src/battle_start_turn_start.c）の頭から呼ぶ。0 を返せば今までどおりの曲。
 * - 目印は戦闘の絵と同じ旗 HACK_FLAG_OAK_PIC（HackPhone_OakPrepareBattle が立て、戦いの終わりに消す）。研究所の 1 戦（トレーナー 1 を共有）は旗が立たない＝今の曲のまま。
 * - 後ろ半分が見えない ROM（SuperCard の 32MB・--rom32・古い mGBA）は枠が前半の無音の見出しのままなので、ここで 0 を返して今の曲に戻す
 *   （無音の戦闘にしない）。判定は src/music_player.c の MusicPlayer_BackSongsPlayable と同じ規則（HackSplit_Native かつ 枠の見出しが 0x0A000000〜0x0BFFFFFF）。
 * - ホウエンの地図で受けた戦いでも新しい曲（HackHoenn_BattleMusic より前に呼ぶ）。
 * 枠の番号は tools/bgm/verify_back_pcm.py が曲の表（song_table.s）の x_oak_bw の番号と同じことを確かめる。
 */
bool8 HackSplit_Native(void); //src/hack_split_pics.c（0x0A000000 に後ろ半分が見える 64MB の ROM か）

u16 HackOak_BattleMusic(void)
{
	u32 header;

	if (!FlagGet(HACK_FLAG_OAK_PIC) || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
		return 0;
	if (!HackSplit_Native())
		return 0;
	header = (u32) gSongTable[HACK_OAK_BGM_SONG].header;
	if (header < 0x0A000000 || header >= 0x0C000000)
		return 0;
	return HACK_OAK_BGM_SONG;
}
