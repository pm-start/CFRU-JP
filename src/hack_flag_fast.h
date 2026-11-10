#pragma once

/*
 * 展開した旗（0x900〜0x18FF）を FlagGet 抜きで読む（2026-11-03・perf-region-2・docs/research/2026-11-03_perf-region-2.md）。
 * 毎コマ呼ばれる所で FlagGet は 1 回 約 160 サイクル（FlagGet 94 + GetFlagPointer 64・拡張の旗のフック経由）。
 * 素の FlagGet（0x0806DEC4）は GetFlagPointer(id)（0x0806DDB4 → ExpandedFlagsHook → src/save.c の GetExpandedFlagPointer）の指す 1 バイトの
 * ビット (id & 7) を見るだけ。0x900〜0x18FF はそこが gExpandedFlags[(id - 0x900) / 8]（include/new/ram_locs.h の 0x0203B0E8）。
 * 同じ値になることは tools/rom/verify_perf_region_2.py が、save.c と ram_locs.h の式から数えて確かめる。
 * ★ id はコンパイル時の定数だけ（範囲の外は組めない）。読むだけ（書くのは今までどおり FlagSet／FlagClear）
 */
#ifndef HACK_XFLAG_BASE   /* 検算（ホスト）だけが別の入れ物に差し替える */
#define HACK_XFLAG_BASE ((const vu8*) 0x0203B0E8)
#endif
#define HackXFlagGet(id) \
	((void) sizeof(char[((id) >= 0x900 && (id) < 0x1900) ? 1 : -1]), \
	 (u32) ((HACK_XFLAG_BASE[((id) - 0x900) >> 3] >> ((id) & 7)) & 1))

/*
 * ★ 2026-11-03 perf-region-3: 実行時に決まる id 用（実績のように id が配列の番号で変わる所）。
 *   HackXFlagGetRt(id)  … id が 0x900〜0x18FF の中だと呼ぶ側が保証するとき（_Static_assert で範囲を確かめてから使う）。式は HackXFlagGet と同じ
 *   HackFlagGetAny(id)  … 中なら直に読み、外なら素の FlagGet（どの id でも FlagGet と同じ値）
 */
#define HackXFlagGetRt(id) 	((u32) ((HACK_XFLAG_BASE[((u32) (id) - 0x900) >> 3] >> ((id) & 7)) & 1))
static inline u32 HackFlagGetAny(u16 id)
{
	if (id >= 0x900 && id < 0x1900)
		return HackXFlagGetRt(id);
	return FlagGet(id) != 0;
}
