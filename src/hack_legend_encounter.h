#pragma once

/* わたしたちの伝説 10 匹の出会い（段 C・2026-10-16・src/hack_legend_encounter.c）。番号は 5e の台帳（2026-10-16）*/

#define LEGEND_COUNT            10
#define LEGEND_FLAG_DONE_FIRST  0xF90   /* 捕まえた／倒した印 0xF90〜0xF99（セーブに残る） */
#define LEGEND_FLAG_HIDE_FIRST  0xF9A   /* 人の表の「隠す旗」0xF9A〜0xFA3（立っていれば出ない）。HackLegend_FieldTick が保つ */
#define LEGEND_LEVEL            180
#define LEGEND_BADGES_NEEDED    16      /* カントー 8 ＋ ホウエン 8 */

u8 HackLegend_BadgeCount(void);         /* カントー 8 ＋ ホウエン 8 のうち持っているバッジの数 */
void HackLegend_FieldTick(void);
void HackLegend_OnHallOfFame(void);     /* 殿堂入りの記録が終わったとき: 捕まえた／倒した印を全部下ろす（hoenn_league.c） */
void HackLegend_FixWild(void);          /* callasm: gEnemyParty[0] の個体値 31・努力値 252 を 2 つ */
void HackLegend_CanTalk(void);          /* callasm: Var8004 番目に話しかけてよいか → LASTRESULT（16 バッジ＆未捕獲） */
