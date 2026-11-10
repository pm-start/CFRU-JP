#include "defines.h"
#include "../include/event_data.h"
#include "../include/text.h"
#include "../include/constants/flags.h"

#include "Tables/hoenn_script_flags.h"
#include "Tables/johto_flags.h"
#include "Tables/sinnoh_flags.h"

/*
badge_count.c
    持っているバッジの数（2026-09-29 ユーザー「レポート時のバッジが８のままカウントが増えない」）

    素 FRLG はレポートの画面（SaveStatToString の SAVE_STAT_BADGES・080F9114）と
    「つづきから」の画面（PrintBadgeCount・0800C9E4）で、カントーの旗 0x820〜0x827 の 8 つだけを数える。
    ホウエンのバッジは別の旗（HOENN_FLAG_BADGE1〜8・移植したジムのスクリプトが立てる）なので増えなかった。
    → カントー＋ホウエン＋ジョウト＋シンオウの合計（最大 32）。
    フックは assembly/hoenn/badges_hook.s・hooks（080F9114 / 0800C9E8）。
*/

/* FLAG_BADGE01_GET（0x820）・CHAR_0（0xA1）・EOS は CFRU のヘッダのもの。
   ★ 「こ」は素のレポート画面と同じ 0x0A（text.h の CHAR_KO は 0x5A で別の字）*/
#define CHAR_HIRAGANA_KO 0x0A

/* 地方ごとのバッジの旗（添字は map_region.h の MAP_REGION_*: 0 カントー・1 ホウエン・2 ジョウト・3 シンオウ）。カントーは FLAG_BADGE01_GET + i */
static const u16 sHoennBadgeFlags[8] =
{
	HOENN_FLAG_BADGE1, HOENN_FLAG_BADGE2, HOENN_FLAG_BADGE3, HOENN_FLAG_BADGE4,
	HOENN_FLAG_BADGE5, HOENN_FLAG_BADGE6, HOENN_FLAG_BADGE7, HOENN_FLAG_BADGE8,
};
static const u16 sJohtoBadgeFlags[8] =
{
	JOHTO_FLAG_BADGE1, JOHTO_FLAG_BADGE2, JOHTO_FLAG_BADGE3, JOHTO_FLAG_BADGE4,
	JOHTO_FLAG_BADGE5, JOHTO_FLAG_BADGE6, JOHTO_FLAG_BADGE7, JOHTO_FLAG_BADGE8,
};
static const u16 sSinnohBadgeFlags[8] =
{
	SINNOH_FLAG_BADGE1, SINNOH_FLAG_BADGE2, SINNOH_FLAG_BADGE3, SINNOH_FLAG_BADGE4,
	SINNOH_FLAG_BADGE5, SINNOH_FLAG_BADGE6, SINNOH_FLAG_BADGE7, SINNOH_FLAG_BADGE8,
};

/* その地方のバッジの数 0〜8（region は MAP_REGION_KANTO／HOENN／JOHTO／SINNOH・それ以外は 0）。バッジのレベル制限（src/hack_level_cap.c）も使う */
u8 GetRegionBadgeCount(u8 region)
{
	u8 n = 0;
	u32 i;

	for (i = 0; i < 8; ++i)
	{
		switch (region)
		{
		case 0:
			n += FlagGet(FLAG_BADGE01_GET + i) != 0;
			break;
		case 1:
			n += FlagGet(sHoennBadgeFlags[i]) != 0;
			break;
		case 2:
			n += FlagGet(sJohtoBadgeFlags[i]) != 0;		/* 2026-09-30 ジョウトのジム（tools/johto/build_johto_gyms.py）*/
			break;
		case 3:
			n += FlagGet(sSinnohBadgeFlags[i]) != 0;	/* 2026-10-01 シンオウのジム（tools/sinnoh/build_sinnoh_gyms.py）*/
			break;
		}
	}
	return n;
}

/* [hack] patches/0620 その地方で持っているバッジの「いちばん大きいジム番号」1〜8（持っていなければ 0）。バッジのレベル制限の表の行はこの数で引く
   （数ではなく最大の番号＝ ジムを順番どおりに回らなくても、倒した最後のジムのリーダーの + 10 が上限になる。ユーザー 2026-10-20「持ってるジムバッジの持ってる最大値でいいよ」）。
   旗の並びは GetRegionBadgeCount と同じ（バッジ i+1 ＝ ジム i+1） */
u8 GetRegionBadgeHighest(u8 region)
{
	u32 i;

	for (i = 8; i > 0; --i)
	{
		switch (region)
		{
		case 0:
			if (FlagGet(FLAG_BADGE01_GET + i - 1))
				return i;
			break;
		case 1:
			if (FlagGet(sHoennBadgeFlags[i - 1]))
				return i;
			break;
		case 2:
			if (FlagGet(sJohtoBadgeFlags[i - 1]))
				return i;
			break;
		case 3:
			if (FlagGet(sSinnohBadgeFlags[i - 1]))
				return i;
			break;
		}
	}
	return 0;
}

u8 GetAllBadgeCount(void)
{
	u8 n = 0;
	u32 r;

	for (r = 0; r < 4; ++r)
		n += GetRegionBadgeCount(r);
	return n;
}
/* レポートの画面: 「16こ」の形で dest に書く（素は 1 桁しか書けない）*/
void SaveStatBadgesToString(u8* dest)
{
	u8 n = GetAllBadgeCount();

	if (n >= 10)
		*dest++ = CHAR_0 + n / 10;
	*dest++ = CHAR_0 + n % 10;
	*dest++ = CHAR_HIRAGANA_KO;
	*dest = EOS;
}
