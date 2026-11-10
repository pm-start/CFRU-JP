#include "defines.h"
#include "../include/window.h"
#include "Tables/bw_card_pic.h"
#include "hack_player_mei_pics.h" //★ FEAT-player-mei-2

/*
    トレーナーカードの主人公を「縮めない全身」にする（#20・2026-09-23）

    ユーザー「全身にすると潰れる？ VS と同じ方法は無理？」。

    ★ カードの絵はスプライトではなく**窓 2 番（72x80 画素）に貼る絵**
      （pokefirered の CreateTrainerCardSprite → BlitBitmapRectToWindow）。
      素は 64x64 を (13, 4) に貼るだけなので、窓の下 12 画素は空いている。
      BW の前向きの絵（トウヤ 35x71・トウコ 40x74）は縮めなければ 64x64 に入らないが、
      **窓の 80 画素なら全身入る**。だから VS のように 2 枚に分ける必要は無い。

    ## やり方

    素の `CreateTrainerCardTrainerPicSprite`（0810CE08）の中の
    `BL CreateTrainerCardSprite`（0810CE3C）に 8 バイトのフックを置き（assembly/bwplayer/bw_card_hook.s）、
    この関数を呼ぶ。この関数は**素の CreateTrainerCardSprite（0810CB98）をそのまま呼んで**
    今までどおり縮めた全身版を貼り、パレットも読ませてから、
    主人公（前向きの絵 135 / 136 番）のときだけ**縮めない 64x80 を同じ窓に貼り直す**。
    ★ パレットは同じもの（BW の前向きの絵の 16 色）なので、素に読ませたままでよい。
    ★ 64x80 は素の 64x64 の範囲（y 4〜67）を覆うので、先に消さなくてよい。
      窓の右端（72）を越える所は BlitBitmapRectToWindow が切る（素も同じ）。

    ★ 関数ごと差し替える（functionrewrites）のは**使えない**。引数 6 個の踏み台は 76 バイトで、
      すぐ後ろの PlayerGenderToFrontTrainerPicId（0810CE4C・呼び元 4 か所）まで潰す（CLAUDE.md 6.22）。
    ★ 殿堂入り・図鑑は別の関数で描くので、縮めた全身版のまま。
*/

#define BwCard_CreateTrainerCardSprite \
    ((u16 (*)(u16, u32, u32, bool8, u16, u16, u8, u8, bool8))(0x0810CB98 + 1))

u16 BwCreateTrainerCardSprite(u16 species, u32 otId, u32 personality, bool8 isFrontPic,
                              u16 destX, u16 destY, u8 paletteSlot, u8 windowId, bool8 isTrainer)
{
    const u8 *tall = NULL;
    bool8 mei = FALSE;
    u16 ret = BwCard_CreateTrainerCardSprite(species, otId, personality, isFrontPic,
                                             destX, destY, paletteSlot, windowId, isTrainer);

    if (ret == 0 && isTrainer && isFrontPic)
    {
        if (species == BW_CARD_PIC_MALE_ID)
            tall = gBwCardPicMale;
        else if (species == BW_CARD_PIC_FEMALE_ID)
            tall = gBwCardPicFemale;
        else if (species == TRAINER_PIC_MEI) //★ FEAT-player-mei-2: メイは立ち絵 64x96 を高さ 78 に縮めた 64x80
        {
            tall = gBwMeiCardPic;
            mei = TRUE;
        }
    }
    if (tall != NULL)
        BlitBitmapRectToWindow(windowId, tall, 0, 0, BW_CARD_PIC_W, BW_CARD_PIC_H,
                               destX, 0, BW_CARD_PIC_W, BW_CARD_PIC_H);
    /* ★ メイの立ち絵は正面（205 番の 64x64）とパレットが別。素が読ませた正面のパレットを立ち絵のものに替える（BG パレット paletteSlot） */
    if (mei && tall != NULL)
        LoadPalette(gBwMeiTallPal, paletteSlot * 16, 32);
    return ret;
}
