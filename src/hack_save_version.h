/* 生成物（tools/build/build_save_version.py・元は tools/patch/make_demo_bundle.py の VERSION と version_resets.json）。手で直さない */
#pragma once
#define HACK_SAVE_VERSION 0x4200  /* PokemonStart v0.66.0 */
#define HACK_SAVE_MIN_VERSION 0x2402  /* これより古いレポートは つづきから を出さない（v0.36.2）*/
#define HACK_VERSION_RESET_COUNT 8
#define HACK_VERSION_COND_COUNT 2
#define HACK_VERSION_FUNC_COUNT 0
#ifdef HACK_SAVE_VERSION_TABLE
/* 版（u16）・種類（0 フラグを下ろす・1 立てる・2 変数に値・3 関数を呼ぶ）・番号・値［・行番号・条件番号（when／call のある行だけ）］。版の古い順。最後は番兵 */
static const struct HackVersionReset sHackVersionResets[HACK_VERSION_RESET_COUNT + 1] = {
	{0x2601, 2, 0x5140, 0x0000},  /* v0.38.1 */
	{0x2601, 2, 0x5141, 0x0000},  /* v0.38.1 */
	{0x2601, 2, 0x5142, 0x0000},  /* v0.38.1 */
	{0x2601, 2, 0x5143, 0x0000},  /* v0.38.1 */
	{0x2601, 2, 0x5144, 0x0000},  /* v0.38.1 */
	{0x2601, 2, 0x5145, 0x0000},  /* v0.38.1 */
	{0x2601, 2, 0x5146, 0x0000},  /* v0.38.1 */
	{0x3204, 1, 0x13BD, 0x0000, 1, 1},  /* v0.50.4 */
	{0, 0, 0, 0},
};
/* when の条件の項（条件番号・種類 0 旗が立っている／1 立っていない／2 変数の比較・番号・比較 0 == 1 != 2 >= 3 < 4 > 5 <=・値）。同じ条件番号は全部 AND */
static const struct HackVersionCond sHackVersionConds[HACK_VERSION_COND_COUNT + 1] = {
	{1, 0, 0x138A, 0, 0x0000},
	{1, 1, 0x13BD, 0, 0x0000},
	{0, 0, 0, 0, 0},
};
/* call で呼ぶ関数（HackVersionFix_・void(void)） */
static void (* const sHackVersionFuncs[HACK_VERSION_FUNC_COUNT + 1])(void) = {
	0,
};
#endif
