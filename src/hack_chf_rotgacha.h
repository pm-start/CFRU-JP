#pragma once
#include "../include/global.h"

/*
    縛り F のうち ローテーション（案 12）と わざガチャ（案 15）。docs/research/2026-11-03_challenge-f-rotation-gacha.md
    旗は担当 A の src/hack_challenge_f.h（HACK_FLAG_CHF_ROTATION・HACK_FLAG_CHF_ROT_BIT0〜BIT2・HACK_FLAG_CHF_GACHA）。ここでは旗の番地を決めない。
    置き場: ローテーションの番号 = 旗の 3 ビット（セーブの形は変えない）／わざガチャの控え = gNewBS の末尾 hackChfGacha（戦闘のあいだだけ・セーブに入らない）。
*/

#define HACK_CHF_ROT_SLOTS        6   /* 手持ちの数（番号 0〜5） */

/* わざガチャの技の山から外す範囲（技番号。FIRST_Z_MOVE〜LAST_G_MAX_MOVE は連続している）。1 = Lv1〜100 の新技（MOVE_LV100_*）も外す。
   0.63.2: ユーザー「技ランダム 100以上の技も入れていいよ」で 0（新技も山に入れる・山は約 5,010 個） */
#ifndef HACK_CHF_GACHA_EXCLUDE_LV100
#define HACK_CHF_GACHA_EXCLUDE_LV100 0
#endif

bool8 HackChF_BattleAllowed(void);                 /* ローテーション・ガチャの対象の戦いか（通信・施設・ダミー・？？？戦などを除く） */
void HackChFRotation_OnPartyIndexes(void);         /* multi.c の MultiInitPokemonOrder の最後（素の「最初に出す子」を決めた直後・戦闘の初期化で 1 回） */
void HackChFGacha_OnBattleStart(void);             /* battle_start_turn_start.c の BattleBeginFirstTurn（BTSTART_BACKUP_PARTY_ITEMS）: 自分の側の技を入れ替える */
void HackChFGacha_OnSwitchOut(u8 bank);            /* switching.c の atk4D_switchindataupdate（gBattleMons を入れ替える前）: 出ていく子の PP を控える */
void HackChFGacha_OnSwitchIn(u8 bank);             /* 同（gBattleMons を入れ替えたあと）: 出てきた子の技を入れ替える */
void HackChFGacha_OnBattleEnd(void);               /* end_battle.c の EndOfBattleThings の頭: 手持ちの技・PP を元に戻す */
bool8 HackChFGacha_Active(void);                   /* この戦いはガチャ中か（技進化の回数・熟練度を数えない） */
