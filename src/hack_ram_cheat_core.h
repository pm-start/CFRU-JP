#pragma once

/*
    RAM チート検出の「中身の判定」（ROM を呼ばない＝ホストの clang でそのまま組める・tools/rom/verify_ram_cheat.py）。patches/0682
    docs/research/2026-10-21_ram-cheat-detect.md

    使う側（src/hack_ram_cheat.c）が先に HACK_RC_* の上限を定義してから取り込む。上限は **この改造の本当の値**（素の Lv100・EV 255 は使わない）。
      HACK_RC_NUM_SPECIES   種族は 1 〜 NUM_SPECIES-1      HACK_RC_MOVES_COUNT   技は 1 〜 MOVES_COUNT-1（16 ビットの全域ではなく実在する分）
      HACK_RC_ITEMS_COUNT   もちものは ITEMS_COUNT 未満    HACK_RC_MAX_LEVEL     レベルは 1 〜 MAX_LEVEL（200）
      HACK_RC_EV_CAP        努力値 1 つ（252）            HACK_RC_EV_TOTAL      努力値の合計（510）
    個体値は見ない（保存は 5 ビット＝0〜31 しか入らない。Lv100 超えのボーナスは保存せず計算で出す＝src/hack_iv_over100.h）。
    ★ このゲームのポケモンは暗号も checksum も持たない（平文・+0x1C は 0。mGBA で確認 2026-10-21）ので、部品の checksum は見ない。
      値は必ず素の GetMonData／GetBoxMonData（箱は CreateBoxMonFromCompressedMon で戻してから）で読み、生のバイトを直に読まない
      （縮めた形の持ち物・経験値の上位ビットは ID の入れ物・技は 16 ビットを 3 バイトに分けて持つ）。
    型は呼び元の u8／u16／u32／bool8 を使う（global.h）。
*/

#define HACK_RC_MONEY_MAX     9999999U    /* AddMoney の上限（bytereplacement 0809FCB4 = 0x0098967F） */
#define HACK_RC_COIN_MAX      99999U      /* MAX_COINS_DIGITS 5 */
#define HACK_RC_MIN_GAP       300U        /* 続けて数える 2 つの機会の最小の間隔（コマ） */
#define HACK_RC_MAGIC         0x31534352U /* 'RCS1' */

#ifndef HACK_RC_CHECK_LEVEL_EXP
#define HACK_RC_CHECK_LEVEL_EXP 1
#endif

/* 1 匹ぶんの値（手持ちは GetMonData から・箱は縮めた形から埋める）。skipStats = 1 ならレベル・HP・PP は見ない（箱・卵） */
struct HackRcMon
{
	u16 species;
	u16 item;
	u16 moves[4];
	u16 hp;
	u16 maxHP;
	u32 exp;
	u32 expMax;       /* gExperienceTables[成長型][MAX_LEVEL] */
	u16 level;      /* patches/0679 レベルは 16 ビット */
	u16 expLevel;      /* 経験値から出るレベル・0 = 見ない */
	u8 pp[4];
	u8 ppMax[4];
	u8 ev[6];
	u8 skipStats;
};

static inline u32 HackRc_Hash(u32 v, u32 a, u32 b)
{
	u32 x = (v ^ b) * a;       /* a は奇数（全単射） */

	x ^= x >> 15;
	x *= 0x2C1B3C6DU;
	x ^= x >> 12;
	return x + (b << 3) + 1U;
}

static inline u8 HackRc_CheckMon(const struct HackRcMon* m)
{
	u32 i, total = 0;

	if (m->species == 0 || m->species >= HACK_RC_NUM_SPECIES)
		return HRC_SPECIES;
	if (m->exp > m->expMax)
		return HRC_EXP;
	if (m->item >= HACK_RC_ITEMS_COUNT)
		return HRC_ITEM;
	for (i = 0; i < 4; ++i)
	{
		if (m->moves[i] >= HACK_RC_MOVES_COUNT)
			return HRC_MOVE;
	}
	for (i = 0; i < 6; ++i)
	{
		if (m->ev[i] > HACK_RC_EV_CAP)
			return HRC_EV;
		total += m->ev[i];
	}
	if (total > HACK_RC_EV_TOTAL)
		return HRC_EV_TOTAL;
	if (m->skipStats)
		return HRC_OK;
	if (m->level == 0 || m->level > HACK_RC_MAX_LEVEL)
		return HRC_LEVEL;
	#if HACK_RC_CHECK_LEVEL_EXP
	if (m->expLevel != 0 && m->expLevel != m->level)
		return HRC_LEVEL_EXP;
	#endif
	for (i = 0; i < 4; ++i)
	{
		if (m->moves[i] != 0 && m->pp[i] > m->ppMax[i])
			return HRC_PP;
	}
	if (m->maxHP == 0 || m->hp > m->maxHP)
		return HRC_HP;
	return HRC_OK;
}

/* 影（EWRAM の固定番地に置く 24 バイト）。魔法の値が無い間は影の確かめをしない */
struct HackRcShadow
{
	u32 magic;
	u32 moneyH;
	u32 coinsH;
	u32 lastFrame;
	u8 prevMask;
	u8 hasLast;
	u8 marked;
	u8 pad;
	u32 reserved;
};

/* 続けて 2 回（別の機会・同じ区分）見つかったら TRUE を返す（印を付ける合図）。印を付けたあとは何もしない。
   bypassGap = セーブの機会（間隔の制限をしない）。間隔が足りない機会は数えず・リセットもしない */
static inline u8 HackRc_Step(struct HackRcShadow* s, u8 mask, u32 now, u8 bypassGap)
{
	if (s->marked)
		return 0;
	if (!bypassGap && s->hasLast && (u32) (now - s->lastFrame) < HACK_RC_MIN_GAP)
		return 0;
	s->lastFrame = now;
	s->hasLast = 1;
	if (mask == 0)
	{
		s->prevMask = 0;
		return 0;
	}
	if (s->prevMask & mask)
	{
		s->marked = 1;
		return 1;
	}
	s->prevMask = mask;
	return 0;
}
