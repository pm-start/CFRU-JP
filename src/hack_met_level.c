#include "defines.h"
#include "../include/pokemon.h"
#include "hack_met_level.h"

/*
 * 出会った場所（16 ビット）・出会ったレベル（8 ビット）の読み書き（L 案・2026-10-16・仕様 docs/research/2026-10-16_mon-data-l-format.md 1.1・1.2）。
 * 80 バイト形（手持ち・育て屋・箱の取り出し）のヘッダの空き BoxPokemon.unknown（+0x1E・u16）を ext として使う:
 *   bit0〜7 = 出会った場所の上位 8 ビット / bit8 = 出会ったレベルの 8 ビット目 / bit9〜12 = 箱の縮めた形の技の 11 ビット目（80 バイト形では常に 0）/ bit13〜15 = 出会ったレベルの 9〜11 ビット目（0〜2047）。
 *   箱の縮めた形（60 バイト）の末尾の ext と同じ並び（縮める・戻すときは bit0〜8 をそのまま写す）。
 * 出会った場所の下位 8 ビット = substruct3.metLocation・出会ったレベルの下位 7 ビット = substruct3.metLevel（今までの位置のまま）。
 *
 * 前の持ち方（patches/0427）: hyperTraining の bit6 が立っていれば metLevel + 127（128〜254）。ext.bit8 と ext.bit13〜15 が 0 で bit6 が立っているときだけこれで読む（自己修復）。
 *   書くときは必ず新しい持ち方（ext.bit8）にして bit6 を消す。HackMetLevel_Normalize は箱へ縮める前に呼ぶと古い持ち方が混ざっていても直る。
 *
 * GetBoxMonData／SetBoxMonData の MON_DATA_MET_LOCATION(35)・MON_DATA_MET_LEVEL(36) の枝は assembly/hooks/pokemon_data_hooks.s で
 * ここの関数へ（repoints の跳び表 0803F5D0・0803F5D4・0803FCE8・0803FCEC）。
 *   Get: 場所は 16 ビット・レベルは 0〜2047 を返す。
 *   Set: **元が 8 ビット**（素の ROM の呼び元は u8 変数のアドレスを渡す）ので、1 バイトだけ読む。場所は 0〜255 で、上位 8 ビットは 0 にする。
 *        256 以上を書くときは HackMetLocation_Set／HackMetLevel_Set16 を直に呼ぶ（SetMonData に u16 を渡しても下位 8 ビットしか入らない）。
 */
#define MET_EXT_LOC_MASK 0x00FF
#define MET_EXT_LEVEL_BIT 0x0100
#define MET_EXT_LEVEL_HI_SHIFT 13      //bit13〜15 = 出会ったレベルの 9〜11 ビット目
#define MET_EXT_LEVEL_HI_MASK 0xE000
#define MET_LEVEL_HI_FLAG 0x40    //hyperTraining の bit6（前の持ち方）
#define MET_LEVEL_HI_BASE 127

u16 HackMetLocation_Get(const struct BoxPokemon* mon)
{
	return mon->substruct3.metLocation | ((mon->unknown & MET_EXT_LOC_MASK) << 8);
}

void HackMetLocation_Set(struct BoxPokemon* mon, u16 location)
{
	mon->substruct3.metLocation = location & 0xFF;
	mon->unknown = (mon->unknown & ~MET_EXT_LOC_MASK) | (location >> 8);
}

u16 HackMetLevel_Get(const struct BoxPokemon* mon)
{
	u16 level = mon->substruct3.metLevel;

	if (mon->unknown & MET_EXT_LEVEL_BIT)
		level |= 0x80;
	level |= ((mon->unknown & MET_EXT_LEVEL_HI_MASK) >> MET_EXT_LEVEL_HI_SHIFT) << 8;
	if (!(mon->unknown & (MET_EXT_LEVEL_BIT | MET_EXT_LEVEL_HI_MASK)) && (mon->hyperTraining & MET_LEVEL_HI_FLAG))
		level += MET_LEVEL_HI_BASE;
	return level;
}

void HackMetLevel_Set16(struct BoxPokemon* mon, u16 level)
{
	if (level > 2047)
		level = 2047;
	mon->substruct3.metLevel = level & 0x7F;
	mon->unknown = (mon->unknown & ~(MET_EXT_LEVEL_BIT | MET_EXT_LEVEL_HI_MASK))
		| ((level & 0x80) ? MET_EXT_LEVEL_BIT : 0)
		| (((level >> 8) & 7) << MET_EXT_LEVEL_HI_SHIFT);
	mon->hyperTraining &= ~MET_LEVEL_HI_FLAG;
}

void HackMetLevel_Set(struct BoxPokemon* mon, u16 level)
{
	HackMetLevel_Set16(mon, level);
}

void HackMetLevel_Normalize(struct BoxPokemon* mon)
{
	if (!(mon->unknown & (MET_EXT_LEVEL_BIT | MET_EXT_LEVEL_HI_MASK)) && (mon->hyperTraining & MET_LEVEL_HI_FLAG))
		HackMetLevel_Set16(mon, HackMetLevel_Get(mon));
}