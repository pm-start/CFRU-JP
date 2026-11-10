#include "../config.h"


static const struct BattleTowerSpread sRaidPartnerSpread_Catherine_Rank12[] =
{
	{
		.species = SPECIES_SHELLOS_EAST,
		.nature = NATURE_BOLD,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.spdEv = 4,
		.ability = FRONTIER_ABILITY_2, //ABILITY_STORMDRAIN
		.item = ITEM_EVIOLITE,
		.moves =
		{
			MOVE_SCALD,
			MOVE_CLEARSMOG,
			MOVE_MUDBOMB,
			MOVE_RECOVER,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_ALOMOMOLA,
		.nature = NATURE_IMPISH,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_HEALER
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_AQUAJET,
			MOVE_MIRRORCOAT,
			MOVE_WISH,
			MOVE_HEALPULSE,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_WARTORTLE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_RAINDISH
		.item = ITEM_IAPAPA_BERRY,
		.moves =
		{
			MOVE_AQUATAIL,
			MOVE_ICEPUNCH,
			MOVE_POWERUPPUNCH,
			MOVE_RAINDANCE,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Catherine_Rank3[] =
{
	{
		.species = SPECIES_GASTRODON_EAST,
		.nature = NATURE_BOLD,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.spAtkEv = 4,
		.ability = FRONTIER_ABILITY_2, //ABILITY_STORMDRAIN
		.item = ITEM_WIKI_BERRY,
		.moves =
		{
			MOVE_SCALD,
			MOVE_MUDBOMB,
			MOVE_CLEARSMOG,
			MOVE_RECOVER,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_ALOMOMOLA,
		.nature = NATURE_IMPISH,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_HEALER
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_AQUAJET,
			MOVE_MIRRORCOAT,
			MOVE_WISH,
			MOVE_HEALPULSE,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_BLASTOISE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_RAINDISH
		.item = ITEM_DAMP_ROCK,
		.moves =
		{
			MOVE_AQUATAIL,
			MOVE_ICEPUNCH,
			MOVE_POWERUPPUNCH,
			MOVE_RAINDANCE,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Catherine_Rank456[] =
{
	{
		.species = SPECIES_GASTRODON_EAST,
		.nature = NATURE_BOLD,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.spAtkEv = 4,
		.ability = FRONTIER_ABILITY_2, //ABILITY_STORMDRAIN
		.item = ITEM_WIKI_BERRY,
		.moves =
		{
			MOVE_SCALD,
			MOVE_EARTHPOWER,
			MOVE_CLEARSMOG,
			MOVE_RECOVER,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_ALOMOMOLA,
		.nature = NATURE_IMPISH,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_HEALER
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_AQUAJET,
			MOVE_MIRRORCOAT,
			MOVE_WISH,
			MOVE_HEALPULSE,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_BLASTOISE,
		.nature = NATURE_MODEST,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_RAINDISH
		.item = ITEM_BLASTOISINITE,
		.moves =
		{
			MOVE_WATERPULSE,
			MOVE_AURASPHERE,
			MOVE_DARKPULSE,
			MOVE_FLASHCANNON,
		},
		.ball = BALL_TYPE_DIVE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Ginger_Rank123[] =
{
	{
		.species = SPECIES_VIVILLON,
		.nature = NATURE_TIMID,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 4,
		.spdEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_FRIENDGUARD
		.item = ITEM_BINDING_BAND,
		.moves =
		{
			MOVE_INFESTATION,
			MOVE_ELECTROWEB,
			MOVE_LIGHTSCREEN,
			MOVE_PROTECT,
		},
		.ball = BALL_TYPE_NET_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_CLEFAIRY,
		.nature = NATURE_CALM,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_FRIENDGUARD
		.item = ITEM_EVIOLITE,
		.moves =
		{
			MOVE_MOONBLAST,
			MOVE_HELPINGHAND,
			MOVE_FOLLOWME,
			MOVE_LIFEDEW,
		},
		.ball = BALL_TYPE_MOON_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_AUDINO,
		.nature = NATURE_CALM,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_HEALER
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_DAZZLINGGLEAM,
			MOVE_FLAMETHROWER,
			MOVE_HEALPULSE,
			MOVE_WISH,
		},
		.ball = BALL_TYPE_PREMIER_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Ginger_Rank456[] =
{
	{
		.species = SPECIES_VIVILLON,
		.nature = NATURE_TIMID,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 4,
		.spdEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_FRIENDGUARD
		.item = ITEM_BINDING_BAND,
		.moves =
		{
			MOVE_INFESTATION,
			MOVE_ELECTROWEB,
			MOVE_LIGHTSCREEN,
			MOVE_PROTECT,
		},
		.ball = BALL_TYPE_NET_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_CLEFABLE,
		.nature = NATURE_CALM,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_2, //ABILITY_MAGICGUARD
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_MOONBLAST,
			MOVE_FOLLOWME,
			MOVE_MAGICCOAT,
			MOVE_LIFEDEW,
		},
		.ball = BALL_TYPE_MOON_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_AUDINO,
		.nature = NATURE_CALM,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_HEALER
		.item = ITEM_AUDINITE,
		.moves =
		{
			MOVE_DAZZLINGGLEAM,
			MOVE_HELPINGHAND,
			MOVE_HEALPULSE,
			MOVE_WISH,
		},
		.ball = BALL_TYPE_PREMIER_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Abimbola_Rank23[] =
{
	{
		.species = SPECIES_FOONGUS,
		.nature = NATURE_CALM,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_REGENERATOR
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_GIGADRAIN,
			MOVE_VENOSHOCK,
			MOVE_CLEARSMOG,
			MOVE_TOXIC,
		},
		.ball = BALL_TYPE_GREAT_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_AXEW,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_2, //ABILITY_MOLDBREAKER
		.item = ITEM_EXPERT_BELT,
		.moves =
		{
			MOVE_DRAGONCLAW,
			MOVE_AQUATAIL,
			MOVE_POISONJAB,
			MOVE_XSCISSOR,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_ONIX,
		.nature = NATURE_CAREFUL,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.atkEv = 252,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_2, //ABILITY_STURDY
		.item = ITEM_EVIOLITE,
		.moves =
		{
			MOVE_ROCKSLIDE,
			MOVE_STOMPINGTANTRUM,
			MOVE_IRONHEAD,
			MOVE_BIND,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Abimbola_Rank456[] =
{
	{
		.species = SPECIES_AMOONGUSS,
		.nature = NATURE_CALM,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_REGENERATOR
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_GIGADRAIN,
			MOVE_SLUDGEBOMB,
			MOVE_RAGEPOWDER,
			MOVE_SPORE,
		},
		.ball = BALL_TYPE_GREAT_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_HAXORUS,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.atkEv = 252,
		.defEv = 252,
		.spdEv = 4,
		.ability = FRONTIER_ABILITY_2, //ABILITY_MOLDBREAKER
		.item = ITEM_DRAGONIUM_Z,
		.moves =
		{
			MOVE_DRAGONCLAW,
			MOVE_AQUATAIL,
			MOVE_POISONJAB,
			MOVE_SWORDSDANCE,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_STEELIX,
		.nature = NATURE_BRAVE,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 0,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_2, //ABILITY_STURDY
		.item = ITEM_STEELIXITE,
		.moves =
		{
			MOVE_GYROBALL,
			MOVE_STOMPINGTANTRUM,
			MOVE_AQUATAIL,
			MOVE_FIREFANG,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Mahina_Rank456[] =
{
	{
		.species = SPECIES_GYARADOS,
		.nature = NATURE_CAREFUL,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_INTIMIDATE
		.item = ITEM_WATERIUM_Z,
		.moves =
		{
			MOVE_AQUATAIL,
			MOVE_CRUNCH,
			MOVE_IRONHEAD,
			MOVE_POWERWHIP,
		},
		.ball = BALL_TYPE_LURE_BALL,
		.shiny = TRUE,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_VOLCARONA,
		.nature = NATURE_MODEST,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.defEv = 4,
		.spAtkEv = 252,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_FLAMEBODY
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_BUGBUZZ,
			MOVE_FIERYDANCE,
			MOVE_LIGHTSCREEN,
			MOVE_ROOST,
		},
		.ball = BALL_TYPE_LUXURY_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_VENUSAUR,
		.nature = NATURE_BOLD,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.defEv = 252,
		.spAtkEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_CHLOROPHYLL
		.item = ITEM_VENUSAURITE,
		.moves =
		{
			MOVE_GRASSPLEDGE,
			MOVE_SLUDGEBOMB,
			MOVE_WEATHERBALL,
			MOVE_LEECHSEED,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Alford_Rank456[] =
{
	{
		.species = SPECIES_EMPOLEON,
		.nature = NATURE_BOLD,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_TORRENT
		.item = ITEM_LEFTOVERS,
		.moves =
		{
			MOVE_WATERPLEDGE,
			MOVE_FLASHCANNON,
			MOVE_ICEBEAM,
			MOVE_SWAGGER,
		},
		.ball = BALL_TYPE_CHERISH_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_AEGISLASH,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 0,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_STANCECHANGE
		.item = ITEM_FOCUS_SASH,
		.moves =
		{
			MOVE_IRONHEAD,
			MOVE_SHADOWCLAW,
			MOVE_SACREDSWORD,
			MOVE_KINGSSHIELD,
		},
		.ball = BALL_TYPE_CHERISH_BALL,
		.shiny = TRUE,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_GALLADE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spDefEv = 252,
		.spdEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_JUSTIFIED
		.item = ITEM_GALLADITE,
		.moves =
		{
			MOVE_PSYCHOCUT,
			MOVE_DRAINPUNCH,
			MOVE_POISONJAB,
			MOVE_POWERUPPUNCH,
		},
		.ball = BALL_TYPE_CHERISH_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Rival_Rank56[] =
{
	{
		.species = SPECIES_GARCHOMP,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_ROUGHSKIN
		.item = ITEM_GARCHOMPITE,
		.moves =
		{
			MOVE_DRAGONCLAW,
			MOVE_STOMPINGTANTRUM,
			MOVE_POISONJAB,
			MOVE_CRUNCH,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_MAMOSWINE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 196,
		.atkEv = 252,
		.spdEv = 60,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_THICKFAT
		.item = ITEM_FOCUS_SASH,
		.moves =
		{
			MOVE_ICICLESPEAR,
			MOVE_STOMPINGTANTRUM,
			MOVE_STONEEDGE,
			MOVE_ICESHARD,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_GRANBULL,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_INTIMIDATE
		.item = ITEM_ASSAULT_VEST,
		.moves =
		{
			MOVE_PLAYROUGH,
			MOVE_POWERUPPUNCH,
			MOVE_ICEPUNCH,
			MOVE_THUNDERPUNCH,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_TYRANITAR,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_SANDSTREAM
		.item = ITEM_TYRANITARITE,
		.moves =
		{
			MOVE_STONEEDGE,
			MOVE_CRUNCH,
			MOVE_POWERUPPUNCH,
			MOVE_FIREPUNCH,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_METAGROSS,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.atkEv = 252,
		.spDefEv = 252,
		.spdEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_CLEARBODY
		.item = ITEM_METAGROSSITE,
		.moves =
		{
			MOVE_IRONHEAD,
			MOVE_ZENHEADBUTT,
			MOVE_POWERUPPUNCH,
			MOVE_ICEPUNCH,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Marlon_Rank56[] =
{
	{
		.species = SPECIES_MACHAMP,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 0,
		.hpEv = 252,
		.atkEv = 4,
		.spDefEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_GUTS
		.item = ITEM_SITRUS_BERRY,
		.moves =
		{
			MOVE_BRICKBREAK,
			MOVE_ICEPUNCH,
			MOVE_BULLETPUNCH,
			MOVE_WIDEGUARD,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
		.gigantamax = TRUE,
	},
	{
		.species = SPECIES_KROOKODILE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_INTIMIDATE
		.item = ITEM_ASSAULT_VEST,
		.moves =
		{
			MOVE_STOMPINGTANTRUM,
			MOVE_DARKESTLARIAT,
			MOVE_AQUATAIL,
			MOVE_THUNDERFANG,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_ZAPDOS,
		.nature = NATURE_MODEST,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_STATIC
		.item = ITEM_ELECTRIUM_Z,
		.moves =
		{
			MOVE_THUNDERBOLT,
			MOVE_HEATWAVE,
			MOVE_OMINOUSWIND,
			MOVE_ROOST,
		},
		.ball = BALL_TYPE_DUSK_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Jax_Rank6[] =
{
	{
		.species = SPECIES_GOLURK,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_IRONFIST
		.item = ITEM_GROUNDIUM_Z,
		.moves =
		{
			MOVE_HIGHHORSEPOWER,
			MOVE_PHANTOMFORCE,
			MOVE_POWERUPPUNCH,
			MOVE_GRAVITY,
		},
		.ball = BALL_TYPE_CHERISH_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_MAGNEZONE,
		.nature = NATURE_MODEST,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.spAtkEv = 252,
		.ability = FRONTIER_ABILITY_2, //ABILITY_STURDY
		.item = ITEM_AIR_BALLOON,
		.moves =
		{
			MOVE_THUNDERBOLT,
			MOVE_FLASHCANNON,
			MOVE_SIGNALBEAM,
			MOVE_MIRRORCOAT,
		},
		.ball = BALL_TYPE_CHERISH_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_SALAMENCE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.atkEv = 252,
		.defEv = 252,
		.spdEv = 4,
		.ability = FRONTIER_ABILITY_1, //ABILITY_INTIMIDATE
		.item = ITEM_SALAMENCITE,
		.moves =
		{
			MOVE_DRAGONCLAW,
			MOVE_DOUBLEEDGE,
			MOVE_AQUATAIL,
			MOVE_FIREFANG,
		},
		.ball = BALL_TYPE_CHERISH_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_Red_Rank6[] =
{
	{
		.species = SPECIES_PIKACHU_CAP_ORIGINAL,
		.nature = NATURE_MODEST,
		.hpIv = 31,
		.atkIv = 0,
		.defIv = 31,
		.spAtkIv = 31,
		.spDefIv = 31,
		.spdIv = 31,
		.spAtkEv = 252,
		.spDefEv = 4,
		.spdEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_LIGHTNINGROD
		.item = ITEM_PIKASHUNIUM_Z,
		.moves =
		{
			MOVE_THUNDERBOLT,
			MOVE_GRASSKNOT,
			MOVE_VOLTSWITCH,
			MOVE_REFLECT,
		},
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{
		.species = SPECIES_SNORLAX,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //ABILITY_GLUTTONY
		.item = ITEM_AGUAV_BERRY,
		.moves =
		{
			MOVE_BODYSLAM,
			MOVE_CRUNCH,
			MOVE_HIGHHORSEPOWER,
			MOVE_RECYCLE,
		},
		.ball = BALL_TYPE_MASTER_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
		.gigantamax = TRUE,
	},
	{
		.species = SPECIES_MEWTWO,
		.nature = NATURE_ADAMANT,
		.hpIv = 31,
		.atkIv = 31,
		.defIv = 31,
		.spAtkIv = 0,
		.spDefIv = 31,
		.spdIv = 31,
		.hpEv = 252,
		.atkEv = 252,
		.ability = FRONTIER_ABILITY_1, //ABILITY_PRESSURE
		.item = ITEM_MEWTWONITE_X,
		.moves =
		{
			MOVE_ZENHEADBUTT,
			MOVE_POWERUPPUNCH,
			MOVE_POISONJAB,
			MOVE_RECOVER,
		},
		.ball = BALL_TYPE_MASTER_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

extern const u8 sTrainerName_Catherine[];
extern const u8 sTrainerName_Ginger[];
extern const u8 sTrainerName_Abimbola[];
extern const u8 sTrainerName_Mahina[];
extern const u8 sTrainerName_Alford[];
extern const u8 sTrainerName_Marlon[];
extern const u8 sTrainerName_Jax[];
extern const u8 sTrainerName_Red[];
extern const u8 sTrainerName_HackHikari[];
extern const u8 sTrainerName_HackBrendan[];
extern const u8 sTrainerName_HackMay[];
extern const u8 sTrainerName_HackHilbert[];
extern const u8 sTrainerName_HackHilda[];

//RAIDPARTNER BEGIN (tools/raid/build_raid_partners.py が書く・ここを手で直さない)
//[hack] patches/0180・FEAT-raid-partner: 伝説レイドの助っ人（主人公たち・src/hack_legend_raid.c）。★6 だけ。Smogon の型・メガ／Z を 1 人 1 匹以上。レベルは主人公の手持ちの最大（src/hack_legend_raid.c の HackRaidPartner_Level）
static const struct BattleTowerSpread sRaidPartnerSpread_HackBrendan_Rank6[] =
{
	{ //Swampert メガ（第7世代 ou・Rain Sweeper・出典 data.pkmn.cc）
		.species = SPECIES_SWAMPERT,
		.nature = NATURE_JOLLY,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 4, .atkEv = 244, .spdEv = 252, .spDefEv = 8,
		.ability = FRONTIER_ABILITY_HIDDEN, //Damp
		.item = ITEM_SWAMPERTITE, //Swampertite
		.moves =
		{
			MOVE_WATERFALL, //Waterfall
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_ICEPUNCH, //Ice Punch
			MOVE_STEALTHROCK, //Stealth Rock
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Metagross 通常（第9世代 uu・Choice Band・出典 data.pkmn.cc）
		.species = SPECIES_METAGROSS,
		.nature = NATURE_ADAMANT,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.atkEv = 252, .spdEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_CHOICE_BAND, //Choice Band
		.moves =
		{
			MOVE_HEAVYSLAM, //Heavy Slam
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_KNOCKOFF, //Knock Off
			MOVE_BULLETPUNCH, //Bullet Punch
		},
		.teraType = 9, //0 なし・タイプ + 1 (Steel)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Salamence Z（第7世代 ou・Flyinium Z・出典 data.pkmn.cc）
		.species = SPECIES_SALAMENCE,
		.nature = NATURE_JOLLY,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.atkEv = 252, .spdEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //Moxie
		.item = ITEM_FLYINIUM_Z, //Flyinium Z
		.moves =
		{
			MOVE_DRAGONDANCE, //Dragon Dance
			MOVE_FLY, //Fly
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_SUBSTITUTE, //Substitute
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_HackMay_Rank6[] =
{
	{ //Blaziken メガ（第7世代 ubers・Swords Dance・出典 data.pkmn.cc）
		.species = SPECIES_BLAZIKEN,
		.nature = NATURE_ADAMANT,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.atkEv = 252, .defEv = 4, .spdEv = 252,
		.ability = FRONTIER_ABILITY_HIDDEN, //Speed Boost
		.item = ITEM_BLAZIKENITE, //Blazikenite
		.moves =
		{
			MOVE_SWORDSDANCE, //Swords Dance
			MOVE_FLAREBLITZ, //Flare Blitz
			MOVE_LOWKICK, //Low Kick
			MOVE_STONEEDGE, //Stone Edge
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Gardevoir 通常（第9世代 uu・Choice Scarf・出典 data.pkmn.cc）
		.species = SPECIES_GARDEVOIR,
		.nature = NATURE_MODEST,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.defEv = 4, .spdEv = 252, .spAtkEv = 252,
		.ability = FRONTIER_ABILITY_2, //Trace
		.item = ITEM_CHOICE_SCARF, //Choice Scarf
		.moves =
		{
			MOVE_MOONBLAST, //Moonblast
			MOVE_MYSTICALFIRE, //Mystical Fire
			MOVE_PSYCHICNOISE, //Psychic Noise
			MOVE_HEALINGWISH, //Healing Wish
		},
		.teraType = 24, //0 なし・タイプ + 1 (Fairy)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Milotic 通常（第9世代 pu・Physically Defensive・出典 data.pkmn.cc）
		.species = SPECIES_MILOTIC,
		.nature = NATURE_BOLD,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 252, .defEv = 248, .spdEv = 8,
		.ability = FRONTIER_ABILITY_1, //Marvel Scale
		.item = ITEM_HEAVY_DUTY_BOOTS, //Heavy-Duty Boots
		.moves =
		{
			MOVE_RECOVER, //Recover
			MOVE_SCALD, //Scald
			MOVE_FLIPTURN, //Flip Turn
			MOVE_ALLURINGVOICE, //Alluring Voice
		},
		.teraType = 24, //0 なし・タイプ + 1 (Fairy)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Sceptile メガ（第7世代 uu・All-out Attacker・出典 data.pkmn.cc）
		.species = SPECIES_SCEPTILE,
		.nature = NATURE_TIMID,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.spdEv = 252, .spAtkEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_SCEPTILITE, //Sceptilite
		.moves =
		{
			MOVE_LEAFSTORM, //Leaf Storm
			MOVE_DRAGONPULSE, //Dragon Pulse
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_FOCUSBLAST, //Focus Blast
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Flygon Z（第9世代 nationaldexuu・Defensive Utility・出典 data.pkmn.cc）
		.species = SPECIES_FLYGON,
		.nature = NATURE_CAREFUL,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 248, .defEv = 120, .spdEv = 20, .spDefEv = 120,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_GROUNDIUM_Z, //Groundium Z
		.moves =
		{
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_DEFOG, //Defog
			MOVE_TOXIC, //Toxic
			MOVE_ROOST, //Roost
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Ludicolo 通常（第9世代 ru・Rain Sweeper・出典 data.pkmn.cc）
		.species = SPECIES_LUDICOLO,
		.nature = NATURE_MODEST,
		.hpIv = 31, .atkIv = 0, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 4, .spdEv = 252, .spAtkEv = 252,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_LIFE_ORB, //Life Orb
		.moves =
		{
			MOVE_WEATHERBALL, //Weather Ball
			MOVE_GIGADRAIN, //Giga Drain
			MOVE_ICEBEAM, //Ice Beam
			MOVE_FOCUSBLAST, //Focus Blast
		},
		.teraType = 2, //0 なし・タイプ + 1 (Fighting)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_HackHilbert_Rank6[] =
{
	{ //Excadrill Z（第7世代 ou・Sand Rush・出典 data.pkmn.cc）
		.species = SPECIES_EXCADRILL,
		.nature = NATURE_ADAMANT,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.atkEv = 252, .spdEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_STEELIUM_Z, //Steelium Z
		.moves =
		{
			MOVE_SWORDSDANCE, //Swords Dance
			MOVE_IRONHEAD, //Iron Head
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_RAPIDSPIN, //Rapid Spin
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Volcarona 通常（第9世代 ubersuu・Quiver Dance・出典 data.pkmn.cc）
		.species = SPECIES_VOLCARONA,
		.nature = NATURE_TIMID,
		.hpIv = 31, .atkIv = 0, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 252, .defEv = 132, .spdEv = 124,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_HEAVY_DUTY_BOOTS, //Heavy-Duty Boots
		.moves =
		{
			MOVE_FIERYDANCE, //Fiery Dance
			MOVE_GIGADRAIN, //Giga Drain
			MOVE_QUIVERDANCE, //Quiver Dance
			MOVE_MORNINGSUN, //Morning Sun
		},
		.teraType = 13, //0 なし・タイプ + 1 (Grass)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Hydreigon 通常（第9世代 ou・Choice Specs・出典 data.pkmn.cc）
		.species = SPECIES_HYDREIGON,
		.nature = NATURE_TIMID,
		.hpIv = 31, .atkIv = 0, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.spdEv = 252, .spAtkEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_CHOICE_SPECS, //Choice Specs
		.moves =
		{
			MOVE_DARKPULSE, //Dark Pulse
			MOVE_DRACOMETEOR, //Draco Meteor
			MOVE_FLASHCANNON, //Flash Cannon
			MOVE_EARTHPOWER, //Earth Power
		},
		.teraType = 9, //0 なし・タイプ + 1 (Steel)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

static const struct BattleTowerSpread sRaidPartnerSpread_HackHilda_Rank6[] =
{
	{ //Serperior 通常（第9世代 ou・Choice Scarf・出典 data.pkmn.cc）
		.species = SPECIES_SERPERIOR,
		.nature = NATURE_MODEST,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.spdEv = 252, .spAtkEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //Contrary
		.item = ITEM_CHOICE_SCARF, //Choice Scarf
		.moves =
		{
			MOVE_LEAFSTORM, //Leaf Storm
			MOVE_KNOCKOFF, //Knock Off
			MOVE_GIGADRAIN, //Giga Drain
			MOVE_GLARE, //Glare
		},
		.teraType = 13, //0 なし・タイプ + 1 (Grass)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Haxorus Z（第7世代 uu・Swords Dance・出典 data.pkmn.cc）
		.species = SPECIES_HAXORUS,
		.nature = NATURE_JOLLY,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.atkEv = 252, .spdEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_2, //Mold Breaker
		.item = ITEM_DRAGONIUM_Z, //Dragonium Z
		.moves =
		{
			MOVE_SWORDSDANCE, //Swords Dance
			MOVE_OUTRAGE, //Outrage
			MOVE_EARTHQUAKE, //Earthquake
			MOVE_POISONJAB, //Poison Jab
		},
		.teraType = 0, //0 なし・タイプ + 1 (-)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Chandelure 通常（第9世代 nu・Choice Specs・出典 data.pkmn.cc）
		.species = SPECIES_CHANDELURE,
		.nature = NATURE_TIMID,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.spdEv = 252, .spAtkEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_1, //(型に無し)
		.item = ITEM_CHOICE_SPECS, //Choice Specs
		.moves =
		{
			MOVE_FIREBLAST, //Fire Blast
			MOVE_SHADOWBALL, //Shadow Ball
			MOVE_ENERGYBALL, //Energy Ball
			MOVE_TRICK, //Trick
		},
		.teraType = 13, //0 なし・タイプ + 1 (Grass)
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

//RAIDPARTNER END

//HIKARIPARTNER BEGIN (tools/raid/build_hikari_partner.py が書く・ここを手で直さない)
#include "../hack_heroine_back_pic.h" //TRAINER_BACK_PIC_HIKARI（ヒカリの後ろ姿・7）
//[hack] patches/0870・FEAT-heroine-76-s3: ヒカリ（相方）の手持ち 6 匹（tools/raid/build_hikari_partner.py）。ユーザー決定の種族・持ち物・特性・技。努力値・性格は標準の振り方
static const struct BattleTowerSpread sRaidPartnerSpread_HackHikari_Rank6[] =
{
	{ //Empoleon（とつげきチョッキ・かちき）
		.species = SPECIES_EMPOLEON,
		.nature = NATURE_MODEST,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 252, .spAtkEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //COMPETITIVE
		.item = ITEM_ASSAULT_VEST,
		.moves =
		{
			MOVE_HYDROPUMP, //ハイドロポンプ
			MOVE_FLASHCANNON, //ラスターカノン
			MOVE_ICEBEAM, //れいとうビーム
			MOVE_GRASSKNOT, //くさむすび
		},
		.teraType = 0, //0 なし
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Togekiss（フェアリーZ・てんのめぐみ）
		.species = SPECIES_TOGEKISS,
		.nature = NATURE_TIMID,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 4, .spdEv = 252, .spAtkEv = 252,
		.ability = FRONTIER_ABILITY_2, //SERENEGRACE
		.item = ITEM_FAIRIUM_Z,
		.moves =
		{
			MOVE_AIRSLASH, //エアスラッシュ
			MOVE_DAZZLINGGLEAM, //マジカルシャイン
			MOVE_AURASPHERE, //はどうだん
			MOVE_HELPINGHAND, //てだすけ
		},
		.teraType = 0, //0 なし
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Lopunny（ミミロップナイト・じゅうなん）
		.species = SPECIES_LOPUNNY,
		.nature = NATURE_JOLLY,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.atkEv = 252, .spdEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //LIMBER
		.item = ITEM_LOPUNNITE,
		.moves =
		{
			MOVE_FAKEOUT, //ねこだまし
			MOVE_HIGHJUMPKICK, //とびひざげり
			MOVE_RETURN, //おんがえし
			MOVE_ICEPUNCH, //れいとうパンチ
		},
		.teraType = 0, //0 なし
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Pachirisu（でんきだま・ちくでん）
		.species = SPECIES_PACHIRISU,
		.nature = NATURE_BOLD,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 252, .defEv = 252, .spDefEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //VOLTABSORB
		.item = ITEM_LIGHT_BALL,
		.moves =
		{
			MOVE_NUZZLE, //ほっぺすりすり
			MOVE_FOLLOWME, //このゆびとまれ
			MOVE_SUPERFANG, //いかりのまえば
			MOVE_PROTECT, //まもる
		},
		.teraType = 0, //0 なし
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Ambipom（いのちのたま・テクニシャン）
		.species = SPECIES_AMBIPOM,
		.nature = NATURE_JOLLY,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 4, .atkEv = 252, .spdEv = 252,
		.ability = FRONTIER_ABILITY_1, //TECHNICIAN
		.item = ITEM_LIFE_ORB,
		.moves =
		{
			MOVE_DOUBLEHIT, //ダブルアタック
			MOVE_UTURN, //とんぼがえり
			MOVE_KNOCKOFF, //はたきおとす
			MOVE_FAKEOUT, //ねこだまし
		},
		.teraType = 0, //0 なし
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
	{ //Mamoswine（コオリZ・あついしぼう）
		.species = SPECIES_MAMOSWINE,
		.nature = NATURE_ADAMANT,
		.hpIv = 31, .atkIv = 31, .defIv = 31, .spAtkIv = 31, .spDefIv = 31, .spdIv = 31,
		.hpEv = 252, .atkEv = 252, .spdEv = 4,
		.ability = FRONTIER_ABILITY_HIDDEN, //THICKFAT
		.item = ITEM_ICIUM_Z,
		.moves =
		{
			MOVE_ICICLECRASH, //つららおとし
			MOVE_ICESHARD, //こおりのつぶて
			MOVE_ROCKSLIDE, //いわなだれ
			MOVE_KNOCKOFF, //はたきおとす
		},
		.teraType = 0, //0 なし
		.ball = BALL_TYPE_POKE_BALL,
		.forSingles = TRUE,
		.forDoubles = TRUE,
		.modifyMovesDoubles = FALSE,
	},
};

//HIKARIPARTNER END

const struct MultiRaidTrainer gRaidPartners[] =
{
	{
		.owNum = EVENT_OBJ_GFX_MAY,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_MAY,
		.gender = FEMALE,
		.otId = 0xCD2F27AB,
		.name = sTrainerName_Catherine,
		.spreads =
		{
			[ONE_STAR_RAID ... TWO_STAR_RAID] =	 sRaidPartnerSpread_Catherine_Rank12,
			[THREE_STAR_RAID] =				 	 sRaidPartnerSpread_Catherine_Rank3,
			[FOUR_STAR_RAID ... SIX_STAR_RAID] =	 sRaidPartnerSpread_Catherine_Rank456,
		},
		.spreadSizes =
		{
			[ONE_STAR_RAID ... TWO_STAR_RAID] =   NELEMS(sRaidPartnerSpread_Catherine_Rank12),
			[THREE_STAR_RAID] = 				 NELEMS(sRaidPartnerSpread_Catherine_Rank3),
			[FOUR_STAR_RAID ... SIX_STAR_RAID] =  NELEMS(sRaidPartnerSpread_Catherine_Rank456),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_LEAF_NORMAL,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_LEAF,
		.gender = FEMALE,
		.otId = 0xAD5C417B,
		.name = sTrainerName_Ginger,
		.spreads =
		{
			[ONE_STAR_RAID ... THREE_STAR_RAID] = sRaidPartnerSpread_Ginger_Rank123,
			[FOUR_STAR_RAID ... SIX_STAR_RAID] =  sRaidPartnerSpread_Ginger_Rank456,
		},
		.spreadSizes =
		{
			[ONE_STAR_RAID ... THREE_STAR_RAID] =  NELEMS(sRaidPartnerSpread_Ginger_Rank123),
			[FOUR_STAR_RAID ... SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Ginger_Rank456),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_BRENDAN,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_BRENDAN,
		.gender = MALE,
		.otId = 0x2EC3AEBC,
		.name = sTrainerName_Abimbola,
		.spreads =
		{
			[TWO_STAR_RAID ... THREE_STAR_RAID] = sRaidPartnerSpread_Abimbola_Rank23,
			[FOUR_STAR_RAID ... SIX_STAR_RAID] =  sRaidPartnerSpread_Abimbola_Rank456,
		},
		.spreadSizes =
		{
			[TWO_STAR_RAID ... THREE_STAR_RAID] =  NELEMS(sRaidPartnerSpread_Abimbola_Rank23),
			[FOUR_STAR_RAID ... SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Abimbola_Rank456),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_MIDDLE_AGED_MAN,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_POKE_DUDE,
		.gender = MALE,
		.otId = 0x87116209,
		.name = sTrainerName_Mahina,
		.spreads =
		{
			[FOUR_STAR_RAID ... SIX_STAR_RAID] =  sRaidPartnerSpread_Mahina_Rank456,
		},
		.spreadSizes =
		{
			[FOUR_STAR_RAID ... SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Mahina_Rank456),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_OLD_MAN,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_OLD_MAN,
		.gender = MALE,
		.otId = 0x61AC8B90,
		.name = sTrainerName_Alford,
		.spreads =
		{
			[FOUR_STAR_RAID ... SIX_STAR_RAID] =  sRaidPartnerSpread_Alford_Rank456,
		},
		.spreadSizes =
		{
			[FOUR_STAR_RAID ... SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Alford_Rank456),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_RIVAL,
		.trainerClass = CLASS_RIVAL,
		.backSpriteId = TRAINER_BACK_PIC_BRENDAN,
		.gender = MALE,
		.otId = 0xECBAA1AD,
		.name = NULL, //Predefined ingame
		.spreads =
		{
			[FIVE_STAR_RAID ... SIX_STAR_RAID] =  sRaidPartnerSpread_Rival_Rank56,
		},
		.spreadSizes =
		{
			[FIVE_STAR_RAID ... SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Rival_Rank56),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_FISHERMAN,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_POKE_DUDE,
		.gender = MALE,
		.otId = 0x4156010,
		.name = sTrainerName_Marlon,
		.spreads =
		{
			[FIVE_STAR_RAID ... SIX_STAR_RAID] =  sRaidPartnerSpread_Marlon_Rank56,
		},
		.spreadSizes =
		{
			[FIVE_STAR_RAID ... SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Marlon_Rank56),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_MR_FUJI,
		.trainerClass = CLASS_PKMN_TRAINER_2,//CLASS_CHAMPION,
		.backSpriteId = TRAINER_BACK_PIC_OLD_MAN,
		.gender = MALE,
		.otId = 0x95746426,
		.name = sTrainerName_Jax,
		.spreads =
		{
			[SIX_STAR_RAID] =  sRaidPartnerSpread_Jax_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Jax_Rank6),
		},
	},
	{
		.owNum = EVENT_OBJ_GFX_RED_NORMAL,
		.trainerClass = CLASS_PKMN_TRAINER_2,
		.backSpriteId = TRAINER_BACK_PIC_RED,
		.gender = MALE,
		.otId = 0x95746426,
		.name = sTrainerName_Red,
		.spreads =
		{
			[SIX_STAR_RAID] =  sRaidPartnerSpread_Red_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_Red_Rank6),
		},
	},
	//HIKARIENTRY BEGIN (tools/raid/build_hikari_partner.py が書く・ここを手で直さない)
	{ //[hack] patches/0870: ヒカリ（相方・#76 段 3）。ユウキの前＝末尾から 5 番目（末尾の 4 人の並びは src/hack_legend_raid.c が数える）
		.owNum = 0x07A1, //シンオウのヒカリの歩く絵（OW 0x07A1）
		.trainerClass = CLASS_PKMN_TRAINER_3, //「ポケモントレーナー」
		.backSpriteId = TRAINER_BACK_PIC_HIKARI,
		.gender = FEMALE,
		.otId = 0x5E1A0E07,
		.name = sTrainerName_HackHikari,
		.spreads =
		{
			[SIX_STAR_RAID] = sRaidPartnerSpread_HackHikari_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_HackHikari_Rank6),
		},
	},
	//HIKARIENTRY END
	{ //[hack] patches/0180: 伝説レイドの助っ人 ユウキ（最後の 4 人の並びは src/hack_legend_raid.c が数える）
		.owNum = EVENT_OBJ_GFX_BRENDAN,
		.trainerClass = CLASS_PKMN_TRAINER_3, //「ポケモントレーナー」（クラス 1 はフロンティアブレーンに書き換えた・patches/0088）
		.backSpriteId = TRAINER_BACK_PIC_BRENDAN,
		.gender = MALE,
		.otId = 0x5E1A0E03,
		.name = sTrainerName_HackBrendan,
		.spreads =
		{
			[SIX_STAR_RAID] = sRaidPartnerSpread_HackBrendan_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_HackBrendan_Rank6),
		},
	},
	{ //[hack] patches/0180: 伝説レイドの助っ人 ハルカ（最後の 4 人の並びは src/hack_legend_raid.c が数える）
		.owNum = EVENT_OBJ_GFX_MAY,
		.trainerClass = CLASS_PKMN_TRAINER_3, //「ポケモントレーナー」（クラス 1 はフロンティアブレーンに書き換えた・patches/0088）
		.backSpriteId = TRAINER_BACK_PIC_MAY,
		.gender = FEMALE,
		.otId = 0x5E1A0E04,
		.name = sTrainerName_HackMay,
		.spreads =
		{
			[SIX_STAR_RAID] = sRaidPartnerSpread_HackMay_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_HackMay_Rank6),
		},
	},
	{ //[hack] patches/0180: 伝説レイドの助っ人 トウヤ（最後の 4 人の並びは src/hack_legend_raid.c が数える）
		.owNum = EVENT_OBJ_GFX_RED_NORMAL,
		.trainerClass = CLASS_PKMN_TRAINER_3, //「ポケモントレーナー」（クラス 1 はフロンティアブレーンに書き換えた・patches/0088）
		.backSpriteId = TRAINER_BACK_PIC_RED,
		.gender = MALE,
		.otId = 0x5E1A0E05,
		.name = sTrainerName_HackHilbert,
		.spreads =
		{
			[SIX_STAR_RAID] = sRaidPartnerSpread_HackHilbert_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_HackHilbert_Rank6),
		},
	},
	{ //[hack] patches/0180: 伝説レイドの助っ人 トウコ（最後の 4 人の並びは src/hack_legend_raid.c が数える）
		.owNum = EVENT_OBJ_GFX_LEAF_NORMAL,
		.trainerClass = CLASS_PKMN_TRAINER_3, //「ポケモントレーナー」（クラス 1 はフロンティアブレーンに書き換えた・patches/0088）
		.backSpriteId = TRAINER_BACK_PIC_LEAF,
		.gender = FEMALE,
		.otId = 0x5E1A0E06,
		.name = sTrainerName_HackHilda,
		.spreads =
		{
			[SIX_STAR_RAID] = sRaidPartnerSpread_HackHilda_Rank6,
		},
		.spreadSizes =
		{
			[SIX_STAR_RAID] = NELEMS(sRaidPartnerSpread_HackHilda_Rank6),
		},
	},
};




const u8 gNumRaidPartners = NELEMS(gRaidPartners);
