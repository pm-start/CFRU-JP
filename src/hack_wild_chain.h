#pragma once

/* 野生のむれ（#110・むずかしい／ももアイス（表は hack_wild_chain.c）・src/hack_wild_chain.c・patches/0353）
 * 状態は特別な旗（EWRAM の sSpecialFlags・セーブしない）だけに持つ。6.13 書き換える static は使えない・ヒープは戦いの始まりで作り直される */
#define HACK_FLAG_WILD_CHAIN_ELIGIBLE   0x4002   /* 今から始める戦いは むれ の対象（普通の野生・DoStandardWildBattle が読んで消す）*/
#define HACK_FLAG_WILD_CHAIN_START_NEXT 0x4003   /* 次の戦いを野外の最初の空いたコマで始める */
#define HACK_FLAG_WILD_CHAIN_CNT0       0x4004   /* 続けて出た回数（0〜3）の下位ビット */
#define HACK_FLAG_WILD_CHAIN_CNT1       0x4005   /* 同・上位ビット */
#define HACK_FLAG_WILD_CHAIN_FORCE      0x4006   /* 試験用: 立っていれば 抽選がいつも当たる（ゲームの中では誰も立てない）*/

void HackWildChain_MarkEligible(void);           /* wild_encounter.c の StandardWildEncounter（草むら・なみのり）から */
u8 HackWildChain_Count(void);                    /* battle_strings.c の出だしの文から */
void HackWildChain_CB2_EndWildBattle(void);      /* DoStandardWildBattle が savedCallback に置く */
void HackWildChain_FieldTick(void);              /* overworld_speedup.c から毎コマ */
void HackWildChain_Reset(void);                  /* つづきから */

bool8 HackWildChain_GenerateNext(bool8 water);
bool8 HoennPike_InWildRoom(void);   /* src/hoenn_pike.c */
