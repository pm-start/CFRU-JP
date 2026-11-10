#pragma once
#include "../include/global.h"
#include "../include/overworld.h"

/*
    ポケモンセンター 3F の「れんしゅうの あいて」（ダミー）。patches/0602・#138・docs/research/2026-10-17_pc-3f-dummy.md

    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: ふういんルーレット（hack_fuuin.h・0xFC5）の次 → **0xFC6（ここ）** → ぜんぶレントラー（hack_challenge_luxray.h・0xFC7）→ 金の前口上の印 0xFC8〜0xFCE（build_hoenn_scripts.LEDGER_FLAGS）→ 次の空きは 0xFD0（0xFCF は hack_tamper.h＝セーブの改造ありの印）。
    変数は増やさない（選んだ種類・レベルの組・何番目は一時変数 0x4001／0x4002／0x4003＝地図を読み込むたびに 0 に戻る・ポケセンの料金の 0x400E／0x400F とは別）。
*/
#define HACK_FLAG_DUMMY_BATTLE   0xFC6   /* ダミーとの戦いの最中の印（戦闘が終わったあと HackDummy_CB2_EndBattle が下ろす。end_battle.c の gEndBattleFlagClearTable には入れない＝戦闘の終わりの処理が見るため） */
#define HACK_DUMMY_VAR_KIND      0x4001  /* 台本が選ばせた「ダミーの種類」（0 ヌケニン・1 ハガネール・2 ハピナス・3 ツボツボ） */
#define HACK_DUMMY_VAR_GROUP     0x4002  /* 台本が選ばせた「レベルの組」（0〜3 ＝ 10〜50・60〜100・110〜150・160〜200） */
#define HACK_DUMMY_VAR_STEP      0x4003  /* 台本が選ばせた「組の中の何番目」（0〜4）。レベル ＝ 10 + 50 × 組 + 10 × 何番目（10〜200） */
#define HACK_DUMMY_KIND_COUNT    4
#define HACK_DUMMY_MIN_LEVEL     10
#define HACK_DUMMY_MAX_LEVEL     200

bool8 HackDummy_Active(void);                  /* 今の戦いがダミーとの戦いか（野生戦だけ・旗が立っている間） */
bool8 HackDummy_ForceUsable(u8 bank);          /* ダミー側の技を「全部使える」に（CheckMoveLimitations の頭から）。わるあがきを出させない */
void HackDummy_KeepSplash(u8 bank);            /* ダミーの 1 番目の技（はねる）の PP を毎ターン満タンに（OpponentHandleChooseMove から） */
void HackDummy_Rescue(void);                   /* 自分の側が全滅していたら手持ちを全回復（EndOfBattleThings から・BringBackTheDead の前） */
struct HackPc3fSave
{
	u8 on;
	u8 flags;                        /* 元の specialSaveWarpFlags（bit0＝つづきからはワープ） */
	u16 objMask;                      /* eventObjects のうち 3F の番号を行き先の番号に替えた欄（bit i） */
	struct Coords16 pos;
	struct WarpData location;
	struct WarpData continueGameWarp;
	struct WarpData dynamicWarp;
	struct WarpData escapeWarp;
	struct WarpData lastHealLocation;
	u16 mapLayoutId;
};
#define HACK_PC3F_GROUP 79
#define HACK_PC3F_NUM   10
void HackPc3f_SaveBegin(struct HackPc3fSave* b);       /* セーブを書く直前（save.c の SaveWriteToFlash）: 3F にいるなら 書く場所を 入ってきた階段（動的ワープ・無ければ最後に回復した場所）にすり替える */
void HackPc3f_SaveEnd(const struct HackPc3fSave* b);   /* 書き終えたら元に戻す（今のゲームは 3F のまま）*/

void HackDummy_Start(void);                    /* callasm: HACK_DUMMY_VAR_KIND／LEVEL のダミーとの戦いを始める（台本は直後に waitstate） */
