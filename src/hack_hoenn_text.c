#include "defines.h"
#include "../include/event_data.h"
#include "../include/string_util.h"

/*
 * ホウエンの人の台本が使う「文の部品」を入れる special の代わり（2026-10-17・NPC 点検 fix/npc-audit-hs）。
 *
 * エメラルドの `special GetPlayerBigGuyGirlString`（pokeemerald src/field_specials.c）は、主人公が男なら gText_BigGuy・女なら
 * gText_BigGirl を gStringVar1 に入れる。素 FRLG に無いので Conv.value が 0 番の special にして台本ごと隠していた
 * （tools/rom/build_hoenn_scripts.py の SPECIAL_MISSING で callnative に差し替える・CLAUDE.md 6.42 のとおり名前は HoennNative_ で始める）。
 * 文字は日本語版エメラルドの ROM（0x085CB02A「おにいちゃん」・0x085CB031「おねえちゃん」）と同じバイト。
 */

void HoennNative_PlayerBigGuyGirl(void)
{
	static const u8 sBigGuy[] = {0x05, 0x16, 0x02, 0x11, 0x34, 0x2E, 0xFF};   //おにいちゃん
	static const u8 sBigGirl[] = {0x05, 0x18, 0x04, 0x11, 0x34, 0x2E, 0xFF};  //おねえちゃん

	if (gSaveBlock2->playerGender == 0)   //MALE
		StringCopy(gStringVar1, sBigGuy);
	else
		StringCopy(gStringVar1, sBigGirl);
}
