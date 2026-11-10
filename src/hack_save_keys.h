/* 生成物（tools/build/build_save_keys.py・元は tools/build/save_keys.json）。手で直さない。追記専用の台帳 */
#pragma once
/* 版（u16・hack_save_version.h と同じ数え方）と鍵。版の古い順。最後が今の版（書くときの鍵）。この ROM より新しい版の鍵は持たない */
#define HACK_SAVE_KEY_COUNT 29
#define HACK_SAVE_MAC_SECRET 0xE4EE25B3U  /* 改造の検出の MAC の鍵の半分（全版共通・台帳の mac_secret）*/
static const struct HackSaveKey sHackSaveKeys[HACK_SAVE_KEY_COUNT] = {
	{0x3901, 0, 0xCF0A5E2EU},  /* v0.57.1 */
	{0x3B01, 0, 0x2B1DB2EBU},  /* v0.59.1 */
	{0x3B02, 0, 0x35BF48D2U},  /* v0.59.2 */
	{0x3B03, 0, 0x557D0A65U},  /* v0.59.3 */
	{0x3B04, 0, 0xFAF25C60U},  /* v0.59.4 */
	{0x3B05, 0, 0xEEA1554FU},  /* v0.59.5 */
	{0x3B06, 0, 0x446B6578U},  /* v0.59.6 */
	{0x3B07, 0, 0xBD6EA138U},  /* v0.59.7 */
	{0x3B08, 0, 0xDD815660U},  /* v0.59.8 */
	{0x3B09, 0, 0x62359629U},  /* v0.59.9 */
	{0x3B0A, 0, 0x5C965F4FU},  /* v0.59.10 */
	{0x3B0B, 0, 0x728BBA7EU},  /* v0.59.11 */
	{0x3C00, 0, 0xCE5DE03BU},  /* v0.60.0 */
	{0x3D00, 0, 0x6D8D2BC9U},  /* v0.61.0 */
	{0x3D01, 0, 0xEB4690AAU},  /* v0.61.1 */
	{0x3E00, 0, 0x4169CFE3U},  /* v0.62.0 */
	{0x3E01, 0, 0xD392B2A2U},  /* v0.62.1 */
	{0x3E02, 0, 0x4C47BD46U},  /* v0.62.2 */
	{0x3E03, 0, 0x6D117C67U},  /* v0.62.3 */
	{0x3E04, 0, 0x5192C6BEU},  /* v0.62.4 */
	{0x3E05, 0, 0xF1032CB1U},  /* v0.62.5 */
	{0x3E06, 0, 0xCFC21D87U},  /* v0.62.6 */
	{0x3F00, 0, 0xF3C8262AU},  /* v0.63.0 */
	{0x3F01, 0, 0x6DAE5AD3U},  /* v0.63.1 */
	{0x3F02, 0, 0xBAB5FE4FU},  /* v0.63.2 */
	{0x4000, 0, 0xBE0B7757U},  /* v0.64.0 */
	{0x4100, 0, 0x3809FC27U},  /* v0.65.0 */
	{0x4101, 0, 0x38E4D94CU},  /* v0.65.1 */
	{0x4200, 0, 0x28943BE2U},  /* v0.66.0 */
};
