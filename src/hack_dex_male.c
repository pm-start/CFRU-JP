#include "defines.h"
#include "../include/sprite.h"
#include "hack_dex_male.h"

/*
 * 図鑑の絵は ♂（ふつう）の絵（patches/0945・2026-11-03・ユーザー決定）。
 * 0.63.0 の ♀ピカチュウなど性別差分（DPE の TryGetFemaleGenderedSpecies）は、図鑑が性格値 0 で絵を呼ぶと 0 が ♀ と判定され、
 * 図鑑の絵がいつも ♀ になる。図鑑の絵を作る呼び口で旗 HACK_DEX_MALE_FLAG を立て、DPE 側が差し替えを飛ばす。
 *   ・素の DexScreen の絵（081070C4 の bl CreateMonPicSprite_HandleDeoxys）: hooks HackDexMale_PicHook 081070C0（assembly/hack_dex/dex_hooks.s）
 *   ・捕まえたときの図鑑の画面: src/catching.c の CreateCapturedMonDexPic
 * 絵（と色）は CreateMonPicSprite_HandleDeoxys の中で作り終わる（戻ったら旗を 0 に）。戦闘・ステータス・ボックスなどは旗を立てないので今のまま。
 */
u16 HackDexMale_CreateMonPic(u16 species, u32 otId, u32 personality, u8 flags, s16 x, s16 y, u8 paletteSlot, u16 paletteTag)
{
	u16 spriteId;

	HACK_DEX_MALE_FLAG = 1;
	spriteId = CreateMonPicSprite_HandleDeoxys(species, otId, personality, flags, x, y, paletteSlot, paletteTag);
	HACK_DEX_MALE_FLAG = 0;
	return spriteId;
}
