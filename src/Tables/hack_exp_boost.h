#ifndef GUARD_HACK_EXP_BOOST_H
#define GUARD_HACK_EXP_BOOST_H

/* 戦闘経験値だけを滑らかに補正する。保存経験値表と既存のレベルは変えない。 */
#define HACK_EXP_BOOST_START_LEVEL 80
#define HACK_EXP_BOOST_PERCENT 100
#define HACK_EXP_BOOST_SLOPE 14

static inline u32 HackExpBoostPercent(u32 level)
{
	if (level < HACK_EXP_BOOST_START_LEVEL)
		return 100;
	return HACK_EXP_BOOST_PERCENT
	     + (level - HACK_EXP_BOOST_START_LEVEL) * HACK_EXP_BOOST_SLOPE;
}

#endif
