#pragma once

#include "bw_hikari_battle.h"   /* 生成物: tools/bwplayer/build_hikari_battle.py */

/*
 * ヒカリ（相方）の戦闘の後ろ姿の番号（FEAT-heroine-76-s3）。
 *  TRAINER_BACK_PIC_HIKARI  後ろ姿の表（パレット・アニメ・座標・SpriteFrameImage・スプライトの雛形の 5 表）の 7 番。
 *                           素は 0〜5・メイが 6。UNBOUND のとき 7 は Poke Kid なので使えない（この改造は UNBOUND ではない）。
 *                           相方の後ろ姿は VAR_PARTNER_BACKSPRITE（gRaidPartners[].backSpriteId）で決まる。
 */
#ifdef UNBOUND
#error "TRAINER_BACK_PIC_HIKARI (7) は UNBOUND の Poke Kid と重なる"
#endif
#define TRAINER_BACK_PIC_HIKARI 7
