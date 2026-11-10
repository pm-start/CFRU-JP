#ifndef HACK_CONTEST_ADDR_H
#define HACK_CONTEST_ADDR_H

#include "defines.h"

/* コンテスト移植（FEAT-contest・patches/0985・S1）: 素の FR(J)（BPRJ）の関数の番地の直書き（BPRJ.ld に無い物・別名のある物も同じ形で並べた）。
 * 求め方: 素の US（pokefirered.map）の関数の機械語（BL を除く）を素の JP から探して照合（tools/contest/addr_expected.tsv に番地・一致度の下限）。
 * 検算 tools/rom/verify_contest_addr.py が「素の JP のこの番地の先頭の機械語が素の US と合う・BL の飛び先が（番地の分かる物同士で）同じ名前を指す」を見る。
 * 型の引数の構造体は void* で簡略（呼ぶ側で必要なら include）。呼び方: ContestFr_DoMoveAnim(move);
 * 使わない（宣言しない）物: ShowEasyChatScreen（エメラルドでは TV のインタビュー用＝範囲外）・ConvertInternationalString／StripExtCtrlCodes
 * （JP は中身が違い一致度 0.2 未満＝使うなら自作）。 */

#define ContestFr_CopyToBgTilemapBufferRect ((void (*)(u8, const void*, u8, u8, u8, u8)) (0x08002124 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_ScriptContext_Enable ((void (*)(void)) (0x080693F4 | 1)) /* 一致度 1.00・確かさ 高・BPRJ.ld の別名 EnableBothScriptContexts */
#define ContestFr_RequestDma3Copy ((s16 (*)(const void*, void*, u16, u8)) (0x08000EAC | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_RequestDma3Fill ((s16 (*)(s32, void*, u16, u8)) (0x08000F44 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_AddTextPrinter ((bool16 (*)(void*, u8, void (*)(void*, u16))) (0x08002CF0 | 1)) /* 一致度 0.36・確かさ 高 */
#define ContestFr_StringCopy_PlayerName ((u8* (*)(u8*, const u8*)) (0x080088D0 | 1)) /* 一致度 0.96・確かさ 高 */
#define ContestFr_AllocateMonSpritesGfx ((void (*)(void)) (0x08035048 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_PlaySE12WithPanning ((void (*)(u16, s8)) (0x08071A98 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_FreeMonSpritesGfx ((void (*)(void)) (0x08035120 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_WriteSequenceToBgTilemapBuffer ((void (*)(u8, u16, u8, u8, u8, u8, u8, s16)) (0x08002590 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_TransferPlttBuffer ((void (*)(void)) (0x0806FC18 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_UnlockPlayerFieldControls ((void (*)(void)) (0x0806920C | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_SetMultiuseSpriteTemplateToPokemon ((void (*)(u16, u8)) (0x0803EF40 | 1)) /* 一致度 0.94・確かさ 高 */
#define ContestFr_CopyEasyChatWord ((u8* (*)(u8*, u16)) (0x080BEB0C | 1)) /* 一致度 0.95・確かさ 高 */
#define ContestFr_StringGet_Nickname ((u8* (*)(u8*)) (0x080088A4 | 1)) /* 一致度 0.95・確かさ 高・BPRJ.ld の別名 StringGetEnd10 */
#define ContestFr_CopySpriteTiles ((void (*)(u8, u8, u8*, u16*, u8*)) (0x08044588 | 1)) /* 一致度 0.99・確かさ 高 */
#define ContestFr_ClearBattleMonForms ((void (*)(void)) (0x08043608 | 1)) /* 一致度 0.47・確かさ 中 */
#define ContestFr_DoMoveAnim ((void (*)(u16)) (0x08071D40 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_m4aMPlayPitchControl ((void (*)(void*, u16, s16)) (0x081C2104 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_ClearBattleAnimationVars ((void (*)(void)) (0x08071C64 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_CreateInvisibleSpriteWithCallback ((u8 (*)(void (*)(struct Sprite*))) (0x080444BC | 1)) /* 一致度 1.00・確かさ 高・BPRJ.ld の別名 CreateHiddenSpriteWithCallback */
#define ContestFr_SetContinueGameWarpStatusToDynamicWarp ((void (*)(void)) (0x0804B978 | 1)) /* 一致度 1.00・確かさ 中 */
#define ContestFr_GetMonIconPtr ((const u8* (*)(u16, u32, u32)) (0x08096A80 | 1)) /* 一致度 1.00・確かさ 中・BPRJ.ld の別名 LoadPartyIconTiles */
#define ContestFr_PlayCry_Normal ((void (*)(u16, s8)) (0x08071594 | 1)) /* 一致度 1.00・確かさ 高・BPRJ.ld の別名 PlayCry1 */
#define ContestFr_SetDynamicWarp ((void (*)(s32, s8, s8, s8)) (0x08054CA8 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_GetObjectEventIdByLocalIdAndMap ((u8 (*)(u8, u8, u8)) (0x0805D820 | 1)) /* 一致度 1.00・確かさ 中・BPRJ.ld の別名 GetEventObjectIdByLocalIdAndMap */
#define ContestFr_GetBgTilemapBuffer ((void* (*)(u8)) (0x08002008 | 1)) /* 一致度 1.00・確かさ 高 */
#define ContestFr_SeedRng ((void (*)(u16)) (0x080444AC | 1)) /* 一致度 1.00・確かさ 中・BPRJ.ld の別名 SeedRng2 */
#define ContestFr_InitKeys ((void (*)(void)) (0x080005C0 | 1)) /* 一致度 0.95・確かさ 高 */
#define ContestFr_StringCopy_Nickname ((u8* (*)(u8*, const u8*)) (0x08008870 | 1)) /* 一致度 0.96・確かさ 高・BPRJ.ld の別名 StringCopy10 */

/* 固定 RAM の語（素の FR(J) の gBattleAnimBgTileBuffer／gBattleAnimBgTilemapBuffer・FEAT-contest S2a）: 素の US の gBattleBufferA（0x02022BC4）の 0xA0 手前から換算。
 * 素の JP の 0x08074A64／9C／D8・0x0802D92C／0x0802DA1C の literal pool にこの番地が並ぶ。CFRU の ld・ram_locs はこの 8 バイトを使わない（verify_contest_addr.py の E が見る）。 */
#define ContestFr_gBattleAnimBgTileBuffer (*(void* volatile*) 0x02022B18)
#define ContestFr_gBattleAnimBgTilemapBuffer (*(void* volatile*) 0x02022B1C)

#endif
