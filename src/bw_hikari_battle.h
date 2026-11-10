/* 生成物。手で編集しないこと: tools/bwplayer/build_hikari_battle.py が作る。
   ヒカリ（FEAT-heroine-76-s3）の後ろ姿の宣言と、手に持つボールの位置。
   ボールの位置は 64x64 の後ろ姿の絵の (10,53) からスプライトの中心 (32,32) へのずれ。 */
#pragma once

#define BW_HELD_BALL_HIKARI_DX (-22)
#define BW_HELD_BALL_HIKARI_DY (21)

extern const u8 gBwHikariBack[];            /* 0x800 x 5 コマ（非圧縮） */
extern const u8 gBwHikariBackPal[];         /* LZ77 */
extern const union AnimCmd* const gBwHikariBackAnims[2];
