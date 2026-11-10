#include "defines.h"
#include "../include/constants/moves.h"
#include "../include/constants/pokemon.h"
#include "shadow_gauge.h"

/*
 * ダークの心のゲージと技の並びの規則（段 E・src/shadow_gauge.h に仕様）。ROM の外の部品に頼らない純粋な計算だけ（検算がホストで動かす）。
 */

//XD の「性格ごとの 1 回の減り」（満タン 3000 を基準にした値・列＝戦闘に出す／よびかけ／256 歩／かおり）。性格番号は constants/pokemon.h の NATURE_* の順
static const u16 sNatureReductionXD[25][SHADOW_ACTION_COUNT] =
{
	{110, 300, 100,  90}, //HARDY
	{ 70, 330, 100, 130}, //LONELY
	{130, 270,  90,  80}, //BRAVE
	{110, 270, 110,  80}, //ADAMANT
	{120, 270, 110,  70}, //NAUGHTY
	{110, 270,  90, 100}, //BOLD
	{100, 360,  80, 120}, //DOCILE
	{ 90, 270, 110, 100}, //RELAXED
	{120, 300, 100,  80}, //IMPISH
	{100, 270,  90, 110}, //LAX
	{ 70, 330, 110, 120}, //TIMID
	{130, 300,  70, 100}, //HASTY
	{100, 330, 110,  90}, //SERIOUS
	{120, 300,  90,  90}, //JOLLY
	{100, 300, 120,  80}, //NAIVE
	{ 70, 300, 120, 110}, //MODEST
	{ 80, 270, 100, 120}, //MILD
	{100, 300, 100, 100}, //QUIET
	{ 80, 270,  90, 130}, //BASHFUL
	{ 90, 300,  90, 120}, //RASH
	{ 80, 300, 110, 110}, //CALM
	{ 70, 300, 100, 100}, //GENTLE
	{130, 240, 100,  90}, //SASSY
	{ 90, 300, 100, 110}, //CAREFUL
	{130, 270,  80,  90}, //QUIRKY
};

u8 Shadow_GaugeStage(u8 gauge)
{
	if (gauge == 0)
		return 0;

	return (gauge * SHADOW_GAUGE_STAGES + SHADOW_GAUGE_FULL - 1) / SHADOW_GAUGE_FULL;
}

u8 Shadow_BarsCleared(u8 gauge)
{
	return SHADOW_GAUGE_STAGES - Shadow_GaugeStage(gauge);
}

bool8 Shadow_OwnMoveOpen(u8 ownIndex, u8 gauge)
{
	u8 bars = Shadow_BarsCleared(gauge);

	if (ownIndex == 0)
		return bars >= 1;
	if (ownIndex == 1)
		return bars >= 3;
	return FALSE;
}

u8 Shadow_GaugeAmount(u8 nature, u8 action)
{
	u32 v;

	if (nature >= 25 || action >= SHADOW_ACTION_COUNT)
		return 0;

	v = (sNatureReductionXD[nature][action] * SHADOW_GAUGE_FULL + 1500) / 3000;
	return v < 1 ? 1 : v;
}

u8 Shadow_GaugeAfter(u8 gauge, u8 nature, u8 action)
{
	u8 amount = Shadow_GaugeAmount(nature, action);

	return amount >= gauge ? 0 : gauge - amount;
}

u8 Shadow_StepAdvance(u8 steps, bool8* fire)
{
	if (steps >= SHADOW_RELIVE_STEPS - 1)
	{
		*fire = TRUE;
		return 0;
	}
	*fire = FALSE;
	return steps + 1;
}

bool8 Shadow_IsOpen(u8 gauge)
{
	return gauge == 0;
}

static bool8 HasType(u8 type1, u8 type2, u8 a, u8 b, u8 c, u8 d)
{
	return type1 == a || type1 == b || type1 == c || type1 == d || type2 == a || type2 == b || type2 == c || type2 == d;
}

//3 つめの技（2026-11-08 ユーザー「どのポケモンにも割り当てられない 6 技 B」: 規則を広げて 18 技ぜんぶがどこかに付く形）
//  種族値合計 535 以上=ダークハーフ／480〜534=くさ・ドラゴン・フェアリーならダークリムーブ・それ以外ダークウェザー
//  420〜479=ゴースト・エスパー・どくならダークパニック／みず・ひこう・むしならダークミスト／かくとう・いわ・じめん・はがねならダークダウン／それ以外ダークホールド
//  420 未満は 3 つめなし
static u16 ThirdMove(u8 type1, u8 type2, u16 bst)
{
	if (bst >= 535)
		return MOVE_YAMINOHANBUN;
	if (bst >= 480)
		return HasType(type1, type2, TYPE_GRASS, TYPE_DRAGON, TYPE_FAIRY, TYPE_GRASS) ? MOVE_YAMINOUZU : MOVE_YAMINOSORA;
	if (bst >= 420)
	{
		if (HasType(type1, type2, TYPE_GHOST, TYPE_PSYCHIC, TYPE_POISON, TYPE_GHOST))
			return MOVE_YAMINOMADOWASHI;
		if (HasType(type1, type2, TYPE_WATER, TYPE_FLYING, TYPE_BUG, TYPE_WATER))
			return MOVE_YAMINOKIRI;
		if (HasType(type1, type2, TYPE_FIGHTING, TYPE_ROCK, TYPE_GROUND, TYPE_STEEL))
			return MOVE_YAMINOOMOSHI;
		return MOVE_YAMINOTOJIKOME;
	}
	return 0;
}

u8 Shadow_AutoMoves(u8 type1, u8 type2, u16 atk, u16 spatk, u16 bst, u16* out)
{
	u8 n = 0;
	u16 second, third;

	out[n++] = MOVE_YAMITACKLE; //1 つめはみんな同じ

	if (type1 == TYPE_FIRE || type2 == TYPE_FIRE)
		second = MOVE_YAMINOHONOO;
	else if (type1 == TYPE_ELECTRIC || type2 == TYPE_ELECTRIC)
		second = MOVE_YAMINOINAZUMA;
	else if (type1 == TYPE_ICE || type2 == TYPE_ICE)
		second = MOVE_YAMINOREIKI;
	else if (atk >= spatk)
		second = bst < 420 ? MOVE_YAMITOSSHIN : (bst < 500 ? MOVE_YAMIKUDAKI : (bst < 600 ? MOVE_YAMINOSHINKUU : MOVE_YAMITOTSUGEKI)); //600 以上は ダークエンド
	else
		second = bst < 420 ? MOVE_YAMINOHADOU : (bst < 500 ? MOVE_YAMINOTSUKIAGE : MOVE_YAMINOTATSUMAKI);
	out[n++] = second;

	third = ThirdMove(type1, type2, bst);
	if (third != 0)
		out[n++] = third;

	return n;
}

u8 Shadow_BuildMoveList(u8 gauge, const u16* dark, u8 darkCount, const u16* own, bool8 forSummary, u16* moves, u8* src)
{
	u8 len = 0, i, ownIndex = 0, real;

	for (i = 0; i < 4; ++i)
	{
		moves[i] = MOVE_NONE;
		src[i] = SHADOW_SRC_NONE;
	}

	for (i = 0; i < darkCount && i < SHADOW_DARK_MOVES_MAX && len < 4; ++i)
	{
		moves[len] = dark[i];
		src[len++] = SHADOW_SRC_DARK;
	}

	//開いた元の技（技欄の順）を続ける。出ない分は戦闘では並びに出さず、つよさの画面では「？？？」の行
	for (i = 0; i < 4 && len < 4; ++i)
	{
		if (own[i] == MOVE_NONE)
			continue;

		if (Shadow_OwnMoveOpen(ownIndex++, gauge))
		{
			moves[len] = own[i];
			src[len++] = i;
		}
	}

	real = len;

	if (forSummary)
	{
		ownIndex = 0;
		for (i = 0; i < 4 && len < 4; ++i)
		{
			if (own[i] == MOVE_NONE)
				continue;

			if (!Shadow_OwnMoveOpen(ownIndex++, gauge))
				src[len++] = SHADOW_SRC_SEALED; //技は MOVE_NONE のまま
		}
	}

	return real;
}
