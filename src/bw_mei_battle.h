/* 生成物。手で編集しないこと: tools/bwplayer/build_mei_battle.py が作る。
   メイ（FEAT-player-mei-2）の絵の宣言と、手に持つボールの位置。
   ボールの位置は 64x64 の後ろ姿の絵の (11,18) からスプライトの中心 (32,32) へのずれ。 */
#pragma once

#define BW_HELD_BALL_MEI_DX (-21)
#define BW_HELD_BALL_MEI_DY (-14)

#define BW_MEI_CARD_W 64
#define BW_MEI_CARD_H 80

extern const u8 gBwMeiBack[];            /* 0x800 x 5 コマ（非圧縮） */
extern const u8 gBwMeiBackPal[];         /* LZ77 */
extern const u8 gBwMeiTallPal[];         /* 32 バイト（非圧縮・LoadPalette 用） */
extern const u8 gBwMeiCardPic[];         /* 64x80 */
extern const u8 gBwMeiHofTop[];          /* 64x64 = 0x800 */
extern const u8 gBwMeiHofBottom[];       /* 64x32 = 0x400 */
extern const union AnimCmd* const gBwMeiBackAnims[2];
