#ifndef GUARD_HACK_MOVE_ALIAS_H
#define GUARD_HACK_MOVE_ALIAS_H

#include "lv200_move_alias.h"

// 新技（Lv101〜200・Lv1〜100・伝説の技）を「効果元の技」として扱うための入口（自作・リファクタ R1）。
// 新技は効果元の既存技の行を写し、戦闘中の「あの技か」の判定は効果元に寄せて行う。
// gCurrentMove／move を MOVE_X と直接 == / != / switch で比べると新技だけが素通りする
// （0.53.5 jumpifmove・0.54.1 あやしいねむり ほか）。「この技は MOVE_X か」は必ず MoveIs、
// 「この技の効果元」は EffectiveMove を通す。表そのもの（唯一の正）は生成物 src/lv200_move_alias.c。
//   MoveIs(move, MOVE_X)     … move の効果元が MOVE_X か（新技は効果元で比べ、独自効果の技と本物の技は自分自身で比べる）
//   !MoveIs(move, MOVE_X)    … そうでない
//   EffectiveMove(move)      … 効果元の技の番号。switch の式・表の引き・別の関数への引数に使う
// 比べるのは効果の判定だけ。技データ（gBattleMoves）・名前・PP・タイプ・アニメ・習得の判定は元の move を使う。
// 検算: tools/rom/verify_move_desc_effect.py（5・7）が、このヘッダを通さない直接の比べを NG にする。
// AI の評価だけは別の入口 Lv200MoveAIBase（src/lv200_custom_move_ai.h・独自効果は近い既存技へ）。

// 技進化の新技（0x1410〜）のうち、効果元の技の特別処理を受け継ぐ物（ドラゴンジャブ等）は HackEffectiveMove が先に拾う（src/hack_move_evo_alias.c・生成物・FEAT-move-evo-original-moves）。それ以外は Lv200MoveAlias の表。
u16 HackEffectiveMove(u16 move);
#define EffectiveMove(move) HackEffectiveMove(move)
#define MoveIs(move, base) (HackEffectiveMove(move) == (base))

#endif // GUARD_HACK_MOVE_ALIAS_H
