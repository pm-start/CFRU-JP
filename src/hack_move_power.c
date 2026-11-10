#include "defines.h"
#include "../include/string_util.h"
/*
* 技の威力の表示（#126・patches/0582・2026-10-17）。技の表の 1 行は 16 バイトになり、威力が 255 をこえる技は bigPower に本当の威力が入る。
* 素の ROM の画面（わざマシンケース 08133CCE・素の要約画面 08137258・素のわざ画面 080E6338）は 1 バイト目の power（255 で頭打ち）を
* 3 桁で書いていたので、ここ（assembly/hack_move_power/move_power_hooks.s の hooks）から呼ぶ。
* dest に威力を書く（ConvertIntToDecimalStringN と同じ・3 桁・mode は素の呼び方のまま）。
*/
u8* HackPrintMovePower(u8* dest, const struct BattleMove* row, u8 mode)
{
	u16 power = row->bigPower ? row->bigPower : row->power;
	return ConvertIntToDecimalStringN(dest, power, mode, 3);
}