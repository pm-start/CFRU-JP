#pragma once

#include "../include/global.h"

/*
 * 主人公の 3 つ目（メイ）の印（FEAT-player-mei・案 K）。
 *  - playerGender（SaveBlock2 +0x08）は 0/1 のまま。メイの土台は 1（女）＝手を入れ損ねた所は女として動く。
 *  - メイの印は SaveBlock2 +0x90 の 1 バイト。+0x28〜0x8F は図鑑の owned/seen（0x34 バイト x2）で空きではない。
 *    +0x90〜0x97 は素の FRLG の filler_90（使われない 8 バイト）。global.h（上流）は触らず、オフセットで読み書きする。
 *  - 古いセーブは 0（＝メイでない）。ゲームの中で「性別」を 3 択で聞くときは GetPlayerGender3()。
 */
#define HACK_PLAYER_KIND_OFFSET 0x90
#define PLAYER_KIND_NORMAL 0
#define PLAYER_KIND_MEI 1

#define PLAYER_GENDER3_MALE 0
#define PLAYER_GENDER3_FEMALE 1
#define PLAYER_GENDER3_MEI 2

u8 GetPlayerGender3(void);                 //0 男 / 1 女 / 2 メイ
void HackPlayerKind_Set(u8 kind);          //PLAYER_KIND_*
