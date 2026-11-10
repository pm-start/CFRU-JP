#pragma once
#include "../include/global.h"

/*
 * スナッチボール（段 F・FEAT-shadow-f・patches/0961）。組織のトレーナーのダークの個体にだけ使えるボール。
 *  - 道具 ITEM_SNATCH_BALL（0x372）・ボール型 BALL_TYPE_SNATCH_BALL（39）。店では売らない（入手は段 G のほこらとイベント）。
 *  - 投げると必ず捕まえられ、戦闘は続く（相手の残りと戦う）。手持ちが満員なら箱へ。
 *  - 普通のボールはダークの個体に使えない（バッグの画面で断る）。スナッチボールはダークでない個体・野生には使えない（バッグで断る）。
 *  - 旗 FLAG_CATCH_TRAINERS_POKEMON（0x905・CFRU 標準）は残す＝デバッグ用: 立っているとき、ダーク以外のトレーナーのポケモンは今までどおり
 *    普通のボールで捕まえられる（戦闘は終わる）。ダークにスナッチボールを投げたときは旗の有無によらず続行の台本。
 *  - 台本とジングルの組み立ては共有キット（work/tmp/share_kit_20261107）の trainer_catch を本流の形に直したもの。
 */

#define HACK_SNATCH_NO_FOE 0xFF

bool8 HackSnatch_IsSnatchBall(u16 item);
u8 HackSnatch_FindDarkFoe(void);                      //場にいる相手のうち、ダークの個体（右を先に）。いなければ HACK_SNATCH_NO_FOE
const u8* HackSnatch_GetBagRefusal(u16 item);         //バッグの「つかう」で断る文。NULL なら断らない（戦闘中・ボールのとき）
bool8 HackSnatch_BagSkipsDoubleChecks(u16 item);      //トレーナー戦でダークへ投げるスナッチ＝野生ダブル用の「ねらえない」判定を通さない

//戦闘台本から（callasm）
void HackSnatch_Start(void);
void HackSnatch_WaitCaughtJingle(void);
void HackSnatch_SkipPcMessageIfInParty(void);
void HackSnatch_RemoveCaughtMon(void);
void HackSnatch_ReturnBall(void);

//ボールの演出の 95 フレーム目の hook（assembly/hack_snatch_ball/snatch_ball_hooks.s）
void HackSnatch_PlayCaughtJingle(void);

extern const u8 BattleScript_SnatchCaught[];
extern const u8 BattleScript_SnatchCaught_Remove[];
extern const u8 BattleScript_SnatchNoTarget[];

extern const u8 gText_Snatch_NeedSnatchBall[];
extern const u8 gText_Snatch_OnlyDark[];
extern const u8 gText_Snatch_Gotcha[];
