#ifndef HACK_DEX_MALE_H
#define HACK_DEX_MALE_H

#include "defines.h"

/* patches/0945 図鑑の絵は性別差分を使わない（♂／ふつうの絵）。図鑑の呼び口（DexScreen の絵・捕まえたときの図鑑の画面）だけが、
 * 絵を作る間 1 を立てる。DPE の TryGetFemaleGenderedSpecies が見る（同じ番地を src/updated_code.c にも書く）。
 * EWRAM 0x0203EFA0（1 バイト・空き 0x0203E274〜0x0203F0E8 の中。BGM 0x0203E300〜E800・RAM チートの影 0x0203EF80〜98・
 * 連れ歩きの覚え（bw_follower_catalog.c・EFA0 の 32 バイト後ろから）の間）。立てるのは作る間だけ・必ず 0 に戻す。 */
#define HACK_DEX_MALE_FLAG (*(volatile u8*) 0x0203EFA0)

u16 HackDexMale_CreateMonPic(u16 species, u32 otId, u32 personality, u8 flags, s16 x, s16 y, u8 paletteSlot, u16 paletteTag);

#endif
