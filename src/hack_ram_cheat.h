#pragma once
#include "../include/global.h"

/* RAM チート検出（patches/0682・docs/research/2026-10-21_ram-cheat-detect.md）。印は src/hack_tamper.h の HackTamper_Mark（理由 RAM_CHEAT）を共用 */

/* 確かめの区分（mask のビット） */
#define HACK_RC_MONEY_RANGE   0x01
#define HACK_RC_MONEY_SHADOW  0x02
#define HACK_RC_COIN_RANGE    0x04
#define HACK_RC_COIN_SHADOW   0x08
#define HACK_RC_PARTY         0x10
#define HACK_RC_BOX           0x20

/* 手持ち・箱の 1 匹のあり得ない値の理由（0 = 問題なし） */
enum
{
	HRC_OK = 0,
	HRC_LEVEL,        /* 卵以外で 0 または MAX_LEVEL をこえる */
	HRC_EXP,          /* 経験値が MAX_LEVEL のぶんをこえる */
	HRC_LEVEL_EXP,    /* レベルが経験値から出るレベルと違う（危険「中」・HACK_RC_CHECK_LEVEL_EXP で切れる） */
	HRC_SPECIES,
	HRC_MOVE,
	HRC_PP,
	HRC_EV,
	HRC_EV_TOTAL,
	HRC_ITEM,
	HRC_HP,
};


struct HackRamCheatReport
{
	u8 mask;           /* HACK_RC_* の区分のビット（0 = 問題なし） */
	u8 partySlot;      /* 手持ちで最初に見つかった番号（0xFF = なし） */
	u8 partyReason;    /* HRC_* */
	u8 boxId;          /* 箱で最初に見つかった箱（0xFF = なし） */
	u8 boxPos;
	u8 boxReason;
	u8 shadowValid;    /* 影が有効だったか（0 なら影の確かめは省いた） */
};

void HackRamCheat_SetMoney(u32* moneyPtr, u32 newValue);   /* 素の SetMoney 0809FC50 の置き換え（functionrewrites） */
void HackRamCheat_NoteCoins(u32 numCoins);                  /* scripting.c の SetCoins から */
void HackRamCheat_Resync(void);                             /* つづきから／はじめから: 今の値から影を作り直す */
void HackRamCheat_OnMapLoad(void);                          /* 自動の入口（通常版だけ中身がある・デバッグ版は空） */
void HackRamCheat_OnBattleStart(void);
void HackRamCheat_OnSave(void);
struct Pokemon;
struct CompressedPokemon;
u8 HackRamCheat_CheckPartyMon(struct Pokemon* mon);           /* 1 匹の確かめ(HRC_*)。検算が素の ROM で本物の形の子を通す */
u8 HackRamCheat_CheckCompMon(struct CompressedPokemon* c);
void HackRamCheat_RunNow(struct HackRamCheatReport* r);     /* 全部の確かめを 1 回・状態を変えず・印を付けない（デバッグメニュー用） */
