#ifndef HACK_CONTEST_PAINTING_DATA_H
#define HACK_CONTEST_PAINTING_DATA_H

// 生成物: tools/contest/extract_jp_painting.py（手で直さない）
#include "defines.h"

extern const u8 gHackPaintFramePal[]; /* 日本語版 0x085886AC・raw 0x100（BG パレット 0〜7） */
extern const u8 gHackPaintTiles_Cool[]; /* 日本語版 0x085888AC・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintMap_Cool[]; /* 日本語版 0x0858D784・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintTiles_Beauty[]; /* 日本語版 0x08589930・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintMap_Beauty[]; /* 日本語版 0x0858DC90・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintTiles_Cute[]; /* 日本語版 0x0858A560・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintMap_Cute[]; /* 日本語版 0x0858E19C・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintTiles_Smart[]; /* 日本語版 0x0858B098・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintMap_Smart[]; /* 日本語版 0x0858E6A8・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintTiles_Tough[]; /* 日本語版 0x0858C050・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintMap_Tough[]; /* 日本語版 0x0858EBB4・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintTiles_Lobby[]; /* 日本語版 0x0858D180・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8 gHackPaintMap_Lobby[]; /* 日本語版 0x0858F0C0・RL（RLUnCompVram／RLUnCompWram に渡す） */
extern const u8* const gHackPaintFrameTiles[6]; /* 0〜4＝部門・5＝ロビー */
extern const u8* const gHackPaintFrameMap[6]; /* 0〜4＝部門・5＝ロビー */
extern const u8 gHackPaintCaptionHall[]; /* 日本語版 0x0824C1F5・文字列 */
extern const u8 gHackPaintBumon[]; /* 日本語版 0x0824C200・文字列 */
extern const u8 gHackPaintSpace[]; /* 日本語版 0x085C941F・文字列 */
extern const u8 gHackPaintCategory0[]; /* 日本語版 0x0824C22B・表 0x0858F5D0 の 0 番目 */
extern const u8 gHackPaintCategory1[]; /* 日本語版 0x0824C231・表 0x0858F5D0 の 1 番目 */
extern const u8 gHackPaintCategory2[]; /* 日本語版 0x0824C237・表 0x0858F5D0 の 2 番目 */
extern const u8 gHackPaintCategory3[]; /* 日本語版 0x0824C23C・表 0x0858F5D0 の 3 番目 */
extern const u8 gHackPaintCategory4[]; /* 日本語版 0x0824C241・表 0x0858F5D0 の 4 番目 */
extern const u8* const gHackPaintCategoryNames[5]; /* 日本語版の表 0x0858F5D0 */
extern const u8 gHackPaintRank0[]; /* 日本語版 0x0824C206・表 0x0858F5E4 の 0 番目 */
extern const u8 gHackPaintRank1[]; /* 日本語版 0x0824C20E・表 0x0858F5E4 の 1 番目 */
extern const u8 gHackPaintRank2[]; /* 日本語版 0x0824C216・表 0x0858F5E4 の 2 番目 */
extern const u8 gHackPaintRank3[]; /* 日本語版 0x0824C21E・表 0x0858F5E4 の 3 番目 */
extern const u8 gHackPaintRank4[]; /* 日本語版 0x0824C226・表 0x0858F5E4 の 4 番目 */
extern const u8* const gHackPaintRankNames[5]; /* 日本語版の表 0x0858F5E4 */
extern const u8 gHackPaintMuseum0[]; /* 日本語版 0x0824C247・表 0x0858F604 の 0 番目 */
extern const u8 gHackPaintMuseum1[]; /* 日本語版 0x0824C25A・表 0x0858F604 の 1 番目 */
extern const u8 gHackPaintMuseum2[]; /* 日本語版 0x0824C26A・表 0x0858F604 の 2 番目 */
extern const u8 gHackPaintMuseum3[]; /* 日本語版 0x0824C27E・表 0x0858F604 の 3 番目 */
extern const u8 gHackPaintMuseum4[]; /* 日本語版 0x0824C298・表 0x0858F604 の 4 番目 */
extern const u8 gHackPaintMuseum5[]; /* 日本語版 0x0824C2AE・表 0x0858F604 の 5 番目 */
extern const u8 gHackPaintMuseum6[]; /* 日本語版 0x0824C2C6・表 0x0858F604 の 6 番目 */
extern const u8 gHackPaintMuseum7[]; /* 日本語版 0x0824C2D8・表 0x0858F604 の 7 番目 */
extern const u8 gHackPaintMuseum8[]; /* 日本語版 0x0824C2EE・表 0x0858F604 の 8 番目 */
extern const u8 gHackPaintMuseum9[]; /* 日本語版 0x0824C308・表 0x0858F604 の 9 番目 */
extern const u8 gHackPaintMuseum10[]; /* 日本語版 0x0824C31F・表 0x0858F604 の 10 番目 */
extern const u8 gHackPaintMuseum11[]; /* 日本語版 0x0824C33D・表 0x0858F604 の 11 番目 */
extern const u8 gHackPaintMuseum12[]; /* 日本語版 0x0824C353・表 0x0858F604 の 12 番目 */
extern const u8 gHackPaintMuseum13[]; /* 日本語版 0x0824C365・表 0x0858F604 の 13 番目 */
extern const u8 gHackPaintMuseum14[]; /* 日本語版 0x0824C37D・表 0x0858F604 の 14 番目 */
extern const u8* const gHackPaintMuseumNames[15]; /* 日本語版の表 0x0858F604 */
extern const u8 gHackPaintMonOam[]; /* 日本語版 0x0858F640・OAM の 8 バイト */

/* 日本語版の BG の枠 0x0858F5F8（bg1・文字 1・地図 10・priority 1）と窓 0x0858F5FC */
#define HACK_PAINT_BG_TEMPLATE_BITS 0x000010A5
#define HACK_PAINT_WIN_BG 1
#define HACK_PAINT_WIN_LEFT 5
#define HACK_PAINT_WIN_TOP 14
#define HACK_PAINT_WIN_WIDTH 20
#define HACK_PAINT_WIN_HEIGHT 4
#define HACK_PAINT_WIN_PAL 15
#define HACK_PAINT_WIN_BASEBLOCK 1

extern const struct HackContestWinner gHackDefaultContestWinners[8];

#endif
