// このファイルは tools/frontier/build_frontier_brains.py が生成しました。手で編集しないこと。
// フロンティアブレーンの手持ち（pokeemerald src/frontier_util.c sFrontierBrainsMons）。src/hoenn_frontier.c が読む
#pragma once

#include "../include/constants/items.h"
#include "../include/constants/moves.h"
#include "../include/constants/species.h"
#include "../include/pokemon.h"

struct HoennFrontierBrainMon
{
	u16 species;
	u16 item;
	u8 fixedIV;
	u8 nature;
	u8 evs[6];	// HP・攻撃・防御・素早さ・特攻・特防
	u16 moves[4];
	u8 ability;	// FRONTIER_ABILITY_*（0 隠れ・1・2）
	u8 teraType;	// 0 なし・1〜 タイプ + 1
};

#define HOENN_FRONTIER_BRAIN_COUNT 14

static const struct HoennFrontierBrainMon sHoennFrontierBrainMons[HOENN_FRONTIER_BRAIN_COUNT][3] =
{
	[0] = {	// TRAINER_ANABEL の銀
		{65, 539, 31, 10, {4, 0, 0, 252, 252, 0}, {94, 247, 374, 347}, 0, 15},	// ALAKAZAM（Smogon の型）
		{244, 186, 31, 3, {4, 252, 0, 252, 0, 0}, {221, 245, 429, 382}, 0, 11},	// ENTEI（Smogon の型）
		{408, 191, 31, 10, {4, 0, 0, 252, 252, 0}, {369, 502, 347, 105}, 1, 15},	// LATIOS（Smogon の型）
	},
	[1] = {	// TRAINER_ANABEL の金
		{243, 678, 31, 10, {4, 0, 0, 252, 252, 0}, {2246, 459, 359, 443}, 0, 14},	// RAIKOU（Smogon の型）
		{408, 572, 31, 10, {4, 0, 0, 252, 252, 0}, {369, 502, 359, 105}, 1, 17},	// LATIOS（Smogon の型）
		{1380, 200, 31, 3, {4, 252, 0, 252, 0, 0}, {14, 458, 945, 382}, 1, 18},	// KINGAMBIT（Smogon の型）
	},
	[2] = {	// TRAINER_SPENSER の銀
		{245, 200, 31, 10, {252, 0, 4, 252, 0, 0}, {347, 459, 58, 164}, 1, 18},	// SUICUNE（Smogon の型）
		{59, 186, 31, 3, {4, 252, 0, 252, 0, 0}, {434, 245, 423, 365}, 1, 1},	// ARCANINE（Smogon の型）
		{366, 678, 31, 13, {0, 252, 0, 252, 4, 0}, {216, 89, 228, 126}, 1, 1},	// SLAKING（Smogon の型）
	},
	[3] = {	// TRAINER_SPENSER の金
		{245, 200, 31, 15, {252, 0, 0, 0, 252, 4}, {347, 56, 58, 164}, 1, 12},	// SUICUNE（Smogon の型）
		{1380, 141, 31, 3, {4, 252, 0, 252, 0, 0}, {14, 458, 945, 382}, 1, 18},	// KINGAMBIT（Smogon の型）
		{149, 788, 31, 3, {0, 252, 4, 252, 0, 0}, {349, 245, 89, 7}, 0, 1},	// DRAGONITE（Smogon の型）
	},
	[4] = {	// TRAINER_GRETA の銀
		{214, 551, 31, 3, {4, 252, 0, 252, 0, 0}, {14, 365, 42, 350}, 2, 2},	// HERACROSS（Smogon の型）
		{809, 142, 31, 3, {4, 252, 0, 252, 0, 0}, {14, 365, 440, 395}, 0, 2},	// HAWLUCHA（Smogon の型）
		{94, 678, 31, 10, {4, 0, 0, 252, 252, 0}, {386, 247, 188, 374}, 1, 8},	// GENGAR（Smogon の型）
	},
	[5] = {	// TRAINER_GRETA の金
		{94, 541, 31, 10, {4, 0, 0, 252, 252, 0}, {1736, 531, 374, 261}, 1, 8},	// GENGAR（Smogon の型）
		{1381, 725, 31, 13, {4, 252, 0, 252, 0, 0}, {753, 943, 229, 365}, 1, 9},	// GREAT_TUSK（Smogon の型）
		{307, 186, 31, 3, {4, 252, 0, 252, 0, 0}, {183, 331, 317, 365}, 0, 2},	// BRELOOM（Smogon の型）
	},
	[6] = {	// TRAINER_LUCY の銀
		{329, 142, 31, 15, {252, 0, 4, 0, 252, 0}, {459, 58, 105, 370}, 1, 24},	// MILOTIC（Smogon の型）
		{799, 787, 31, 15, {4, 0, 0, 252, 252, 0}, {531, 369, 374, 459}, 0, 4},	// DRAGALGE（Smogon の型）
		{379, 678, 31, 4, {8, 56, 0, 220, 224, 0}, {531, 202, 89, 458}, 1, 4},	// SEVIPER（Smogon の型）
	},
	[7] = {	// TRAINER_LUCY の金
		{130, 544, 31, 13, {4, 252, 0, 252, 0, 0}, {349, 127, 242, 89}, 1, 12},	// GYARADOS（Smogon の型）
		{329, 200, 31, 15, {252, 0, 4, 0, 252, 0}, {56, 58, 105, 2045}, 1, 12},	// MILOTIC（Smogon の型）
		{1179, 696, 31, 10, {4, 0, 0, 252, 252, 0}, {369, 247, 53, 442}, 2, 8},	// DRAGAPULT（Smogon の型）
	},
	[8] = {	// TRAINER_NOLAND の銀
		{SPECIES_METANG, ITEM_SITRUS_BERRY, 31, NATURE_BRAVE, {0, 252, 252, 0, 6, 0}, {MOVE_LIGHTSCREEN, MOVE_PSYCHIC, MOVE_REFLECT, MOVE_METALCLAW}, 1, 0},
		{SPECIES_SKARMORY, ITEM_SITRUS_BERRY, 31, NATURE_IMPISH, {252, 0, 0, 0, 6, 252}, {MOVE_TOXIC, MOVE_AERIALACE, MOVE_PROTECT, MOVE_STEELWING}, 1, 0},
		{SPECIES_AGGRON, ITEM_SITRUS_BERRY, 31, NATURE_ADAMANT, {0, 252, 0, 0, 252, 6}, {MOVE_THUNDERBOLT, MOVE_PROTECT, MOVE_SOLARBEAM, MOVE_DRAGONCLAW}, 1, 0},
	},
	[9] = {	// TRAINER_NOLAND の金
		{SPECIES_METANG, ITEM_SITRUS_BERRY, 31, NATURE_BRAVE, {0, 252, 252, 0, 6, 0}, {MOVE_LIGHTSCREEN, MOVE_PSYCHIC, MOVE_REFLECT, MOVE_METALCLAW}, 1, 0},
		{SPECIES_SKARMORY, ITEM_SITRUS_BERRY, 31, NATURE_IMPISH, {252, 0, 0, 0, 6, 252}, {MOVE_TOXIC, MOVE_AERIALACE, MOVE_PROTECT, MOVE_STEELWING}, 1, 0},
		{SPECIES_AGGRON, ITEM_SITRUS_BERRY, 31, NATURE_ADAMANT, {0, 252, 0, 0, 252, 6}, {MOVE_THUNDERBOLT, MOVE_PROTECT, MOVE_SOLARBEAM, MOVE_DRAGONCLAW}, 1, 0},
	},
	[10] = {	// TRAINER_TUCKER の銀
		{285, 556, 31, 3, {4, 252, 0, 252, 0, 0}, {127, 89, 728, 468}, 1, 12},	// SWAMPERT（Smogon の型）
		{397, 725, 31, 10, {4, 0, 0, 252, 252, 0}, {369, 412, 89, 395}, 1, 17},	// SALAMENCE（Smogon の型）
		{6, 141, 31, 10, {4, 0, 0, 252, 252, 0}, {311, 76, 731, 395}, 1, 11},	// CHARIZARD（Smogon の型）
	},
	[11] = {	// TRAINER_TUCKER の金
		{400, 570, 31, 3, {4, 252, 0, 252, 0, 0}, {1836, 89, 8, 363}, 1, 9},	// METAGROSS（Smogon の型）
		{285, 200, 31, 3, {4, 252, 0, 252, 0, 0}, {127, 89, 728, 468}, 1, 12},	// SWAMPERT（Smogon の型）
		{407, 710, 31, 10, {252, 0, 4, 252, 0, 0}, {347, 502, 359, 105}, 1, 4},	// LATIAS（Smogon の型）
	},
	[12] = {	// TRAINER_BRANDON の銀
		{1186, 725, 31, 10, {4, 0, 0, 252, 252, 0}, {443, 85, 721, 229}, 1, 16},	// REGIELEKI（Smogon の型）
		{1187, 695, 31, 10, {0, 0, 0, 252, 252, 4}, {740, 369, 372, 370}, 1, 17},	// REGIDRAGO（Smogon の型）
		{538, 803, 31, 15, {4, 0, 0, 252, 252, 0}, {494, 449, 372, 468}, 1, 11},	// HEATRAN（Smogon の型）
	},
	[13] = {	// TRAINER_BRANDON の金
		{145, 725, 31, 10, {4, 0, 0, 252, 252, 0}, {412, 443, 257, 395}, 0, 9},	// ZAPDOS（Smogon の型）
		{146, 142, 31, 10, {4, 0, 0, 252, 252, 0}, {1942, 412, 395, 442}, 0, 24},	// MOLTRES（Smogon の型）
		{1221, 200, 31, 10, {4, 0, 0, 252, 252, 0}, {735, 412, 347, 105}, 1, 15},	// ARTICUNO_G（Smogon の型）
	},
};
