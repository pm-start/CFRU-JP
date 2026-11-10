#include "../defines.h"
#include "../../include/new/catching.h"
#include "../../include/constants/items.h"
/*
ball_graphics_tables.c
	set up the structures for expanded pokeballs

tables to edit:
	gBallSpriteSheets
	gBallSpritePalettes
	gBallSpriteTemplates
	gBallParticleSpritesheets
	gBallParticlePalettes
	gBallOpenParticleAnimNums
	gBallOpenParticleAnimationFuncs
	gBallOpenMonFadePal
	gBallParticleSpriteTemplates
*/

#define POKEBALL_COUNT NUM_BALLS

void __attribute__((long_call)) SpriteCB_TestBallThrow(struct Sprite *sprite);
void __attribute__((long_call)) PokeBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) GreatBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) SafariBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) UltraBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) MasterBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) DiveBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) RepeatBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) TimerBallOpenParticleAnimation(u8 taskId);
void __attribute__((long_call)) PremierBallOpenParticleAnimation(u8 taskId);

/*#define gInterfaceGfx_PokeBall (const u8*) 0x8D01724
#define gInterfaceGfx_GreatBall (const u8*) 0x8D01800
#define gInterfaceGfx_SafariBall (const u8*) 0x8D018E0
#define gInterfaceGfx_UltraBall (const u8*) 0x8D019C8
#define gInterfaceGfx_MasterBall (const u8*) 0x8D01A9C
#define gInterfaceGfx_NetBall (const u8*) 0x8D01B80
#define gInterfaceGfx_DiveBall (const u8*) 0x8D01C60
#define gInterfaceGfx_NestBall (const u8*) 0x8D01D9C
#define gInterfaceGfx_RepeatBall (const u8*) 0x8D01E84
#define gInterfaceGfx_TimerBall (const u8*) 0x8D01F68
#define gInterfaceGfx_LuxuryBall (const u8*) 0x8D02084
#define gInterfaceGfx_PremierBall (const u8*) 0x8D021D8*/
extern const u8 gInterfaceGfx_MasterBallTiles[];
extern const u8 gInterfaceGfx_UltraBallTiles[];
extern const u8 gInterfaceGfx_GreatBallTiles[];
extern const u8 gInterfaceGfx_PokeBallTiles[];
extern const u8 gInterfaceGfx_SafariBallTiles[];
extern const u8 gInterfaceGfx_NetBallTiles[];
extern const u8 gInterfaceGfx_DiveBallTiles[];
extern const u8 gInterfaceGfx_NestBallTiles[];
extern const u8 gInterfaceGfx_RepeatBallTiles[];
extern const u8 gInterfaceGfx_TimerBallTiles[];
extern const u8 gInterfaceGfx_LuxuryBallTiles[];
extern const u8 gInterfaceGfx_PremierBallTiles[];
extern const u8 gInterfaceGfx_DuskBallTiles[];
extern const u8 gInterfaceGfx_HealBallTiles[];
extern const u8 gInterfaceGfx_QuickBallTiles[];
extern const u8 gInterfaceGfx_CherishBallTiles[];
extern const u8 gInterfaceGfx_ParkBallTiles[];
extern const u8 gInterfaceGfx_FastBallTiles[];
extern const u8 gInterfaceGfx_LevelBallTiles[];
extern const u8 gInterfaceGfx_LureBallTiles[];
extern const u8 gInterfaceGfx_HeavyBallTiles[];
extern const u8 gInterfaceGfx_LoveBallTiles[];
extern const u8 gInterfaceGfx_FriendBallTiles[];
extern const u8 gInterfaceGfx_MoonBallTiles[];
extern const u8 gInterfaceGfx_SportBallTiles[];
extern const u8 gInterfaceGfx_BeastBallTiles[];
extern const u8 gInterfaceGfx_DreamBallTiles[];
extern const u8 gInterfaceGfx_DynamaxBallTiles[];
extern const u8 gInterfaceGfx_StartBallTiles[]; //[hack] patches/0393 STARTボール（tools/balls/build_start_ball.py）
// WATASHI_BALLS_BEGIN ext_tiles
extern const u8 gInterfaceGfx_MoonPetalBallTiles[];
extern const u8 gInterfaceGfx_MemoryVeilBallTiles[];
extern const u8 gInterfaceGfx_TearWisteriaBallTiles[];
extern const u8 gInterfaceGfx_SweetHeartBallTiles[];
extern const u8 gInterfaceGfx_SilentHaloBallTiles[];
extern const u8 gInterfaceGfx_GamePulseBallTiles[];
extern const u8 gInterfaceGfx_WinterBlossomBallTiles[];
extern const u8 gInterfaceGfx_ThornHeartBallTiles[];
extern const u8 gInterfaceGfx_FoxSpiritBallTiles[];
extern const u8 gInterfaceGfx_BerryBloomBallTiles[];
// WATASHI_BALLS_END ext_tiles
extern const u8 gInterfaceGfx_RocketBallTiles[]; //[hack] patches/0914 ロケットボール
extern const u8 gInterfaceGfx_SnatchBallTiles[]; //[hack] patches/0961 スナッチボール（tools/balls/build_snatch_ball.py）

/*#define gInterfacePal_PokeBall (const u8*) 0x8D017E0
#define gInterfacePal_GreatBall (const u8*) 0x8D018BC
#define gInterfacePal_SafariBall (const u8*) 0x8D019A4
#define gInterfacePal_UltraBall (const u8*) 0x8D01A7C
#define gInterfacePal_MasterBall (const u8*) 0x8D01B5C
#define gInterfacePal_NetBall (const u8*) 0x8D01C40
#define gInterfacePal_DiveBall (const u8*) 0x8D01D74
#define gInterfacePal_NestBall (const u8*) 0x8D01E60
#define gInterfacePal_RepeatBall (const u8*) 0x8D01F44
#define gInterfacePal_TimerBall (const u8*) 0x8D02060
#define gInterfacePal_LuxuryBall (const u8*) 0x8D021B0
#define gInterfacePal_PremierBall (const u8*) 0x8D022C8*/
extern const u8 gInterfaceGfx_MasterBallPal[];
extern const u8 gInterfaceGfx_UltraBallPal[];
extern const u8 gInterfaceGfx_GreatBallPal[];
extern const u8 gInterfaceGfx_PokeBallPal[];
extern const u8 gInterfaceGfx_SafariBallPal[];
extern const u8 gInterfaceGfx_NetBallPal[];
extern const u8 gInterfaceGfx_DiveBallPal[];
extern const u8 gInterfaceGfx_NestBallPal[];
extern const u8 gInterfaceGfx_RepeatBallPal[];
extern const u8 gInterfaceGfx_TimerBallPal[];
extern const u8 gInterfaceGfx_LuxuryBallPal[];
extern const u8 gInterfaceGfx_PremierBallPal[];
extern const u8 gInterfaceGfx_DuskBallPal[];
extern const u8 gInterfaceGfx_HealBallPal[];
extern const u8 gInterfaceGfx_QuickBallPal[];
extern const u8 gInterfaceGfx_CherishBallPal[];
extern const u8 gInterfaceGfx_ParkBallPal[];
extern const u8 gInterfaceGfx_FastBallPal[];
extern const u8 gInterfaceGfx_LevelBallPal[];
extern const u8 gInterfaceGfx_LureBallPal[];
extern const u8 gInterfaceGfx_HeavyBallPal[];
extern const u8 gInterfaceGfx_LoveBallPal[];
extern const u8 gInterfaceGfx_FriendBallPal[];
extern const u8 gInterfaceGfx_MoonBallPal[];
extern const u8 gInterfaceGfx_SportBallPal[];
extern const u8 gInterfaceGfx_BeastBallPal[];
extern const u8 gInterfaceGfx_DreamBallPal[];
extern const u8 gInterfaceGfx_DynamaxBallPal[];
extern const u8 gInterfaceGfx_StartBallPal[];
// WATASHI_BALLS_BEGIN ext_pal
extern const u8 gInterfaceGfx_MoonPetalBallPal[];
extern const u8 gInterfaceGfx_MemoryVeilBallPal[];
extern const u8 gInterfaceGfx_TearWisteriaBallPal[];
extern const u8 gInterfaceGfx_SweetHeartBallPal[];
extern const u8 gInterfaceGfx_SilentHaloBallPal[];
extern const u8 gInterfaceGfx_GamePulseBallPal[];
extern const u8 gInterfaceGfx_WinterBlossomBallPal[];
extern const u8 gInterfaceGfx_ThornHeartBallPal[];
extern const u8 gInterfaceGfx_FoxSpiritBallPal[];
extern const u8 gInterfaceGfx_BerryBloomBallPal[];
// WATASHI_BALLS_END ext_pal
extern const u8 gInterfaceGfx_RocketBallPal[];
extern const u8 gInterfaceGfx_SnatchBallPal[];

#define sBallOamData (const struct OamData*) 0x821D0A8
#define sBallAnimSequences (const union AnimCmd* const*) 0x821D0F0
#define sBallAffineAnimSequences (const union AffineAnimCmd* const*) 0x821D15C

#define gBattleAnimSpriteSheet_Particles (const u8*) 0x8C1A608
extern const u8 gBattleAnimSpriteSheet_ParticlesDuskBallTiles[];
extern const u8 gBattleAnimSpriteSheet_ParticlesHealBallTiles[];
extern const u8 gBattleAnimSpriteSheet_ParticlesQuickCherishBallTiles[];
extern const u8 gBattleAnimSpriteSheet_ParticlesDynamaxBallTiles[];

#define gBattleAnimSpritePalette_136 (const u8*) 0x8C1A6DC
extern const u8 gBattleAnimSpriteSheet_ParticlesDuskBallPal[];
extern const u8 gBattleAnimSpriteSheet_ParticlesHealBallPal[];
extern const u8 gBattleAnimSpriteSheet_ParticlesQuickCherishBallPal[];
extern const u8 gBattleAnimSpriteSheet_ParticlesDynamaxBallPal[];

#define gOamData_AffineOff_ObjNormal_8x8 (const struct OamData*) 0x83709F8
#define sAnims_BallParticles (const union AnimCmd* const*) 0x83D2228

#define BALL_OPEN_WHITE_STARS 0 //Custom for Heal Ball
#define BALL_OPEN_RAINBOW_CIRCLES 0 //Custom for Cherish Ball
#define BALL_OPEN_YELLOW_CIRCLES 2 //Custom for Quick Ball
#define BALL_OPEN_PURPLE_CIRCLES 2 //Custom for Dusk Ball

enum BallOpenParticles
{
	BALL_OPEN_STICKS,
	BALL_OPEN_YELLOW_STARS,
	BALL_OPEN_BLUE_BUBBLES,
	BALL_OPEN_HEARTS,
	BALL_OPEN_GREEN_STARS,
	BALL_OPEN_GREEN_BUBBLES,
};

enum BallTags
{
	GFX_TAG_POKEBALL = 55000, //0xD6D8
	GFX_TAG_GREATBALL,
	GFX_TAG_SAFARIBALL,
	GFX_TAG_ULTRABALL,
	GFX_TAG_MASTERBALL,
	GFX_TAG_NETBALL,
	GFX_TAG_DIVEBALL,
	GFX_TAG_NESTBALL,
	GFX_TAG_REPEATBALL,
	GFX_TAG_TIMERBALL,
	GFX_TAG_LUXURYBALL,
	GFX_TAG_PREMIERBALL,
	GFX_TAG_DUSKBALL = 0xFF00, //65280
	GFX_TAG_HEALBALL,
	GFX_TAG_QUICKBALL,
	GFX_TAG_CHERISHBALL,
	GFX_TAG_PARKBALL,
	GFX_TAG_FASTBALL,
	GFX_TAG_LEVELBALL,
	GFX_TAG_LUREBALL,
	GFX_TAG_HEAVYBALL,
	GFX_TAG_LOVEBALL,
	GFX_TAG_FRIENDBALL,
	GFX_TAG_MOONBALL,
	GFX_TAG_SPORTBALL,
	GFX_TAG_BEASTBALL,
	GFX_TAG_DREAMBALL,
	GFX_TAG_DYNAMAXBALL,
	GFX_TAG_STARTBALL, //[hack] patches/0393（0xFF10）
// WATASHI_BALLS_BEGIN tag
	GFX_TAG_MOON_PETAL_BALL = 0xFF11,
	GFX_TAG_MEMORY_VEIL_BALL = 0xFF12,
	GFX_TAG_TEAR_WISTERIA_BALL = 0xFF13,
	GFX_TAG_SWEET_HEART_BALL = 0xFF14,
	GFX_TAG_SILENT_HALO_BALL = 0xFF15,
	GFX_TAG_GAME_PULSE_BALL = 0xFF16,
	GFX_TAG_WINTER_BLOSSOM_BALL = 0xFF17,
	GFX_TAG_THORN_HEART_BALL = 0xFF18,
	GFX_TAG_FOX_SPIRIT_BALL = 0xFF19,
	GFX_TAG_BERRY_BLOOM_BALL = 0xFF1A,
// WATASHI_BALLS_END tag
	GFX_TAG_ROCKET_BALL = 0xFF1B, //[hack] patches/0914
	GFX_TAG_SNATCH_BALL = 0xFF1C, //[hack] patches/0961
};

enum BallOpenParticleTags
{
	TAG_BALL_OPEN_DUSK = 0xFE00,
	TAG_BALL_OPEN_HEAL,
	TAG_BALL_OPEN_QUICK,
	TAG_BALL_OPEN_CHERISH,
	TAG_BALL_OPEN_PARK,
	TAG_BALL_OPEN_FAST,
	TAG_BALL_OPEN_LEVEL,
	TAG_BALL_OPEN_LURE,
	TAG_BALL_OPEN_HEAVY,
	TAG_BALL_OPEN_LOVE,
	TAG_BALL_OPEN_FRIEND,
	TAG_BALL_OPEN_MOON,
	TAG_BALL_OPEN_SPORT,
	TAG_BALL_OPEN_BEAST,
	TAG_BALL_OPEN_DREAM,
	TAG_BALL_OPEN_DYNAMAX,
	TAG_BALL_OPEN_START, //[hack] patches/0393（0xFE10）
// WATASHI_BALLS_BEGIN ptag
	TAG_BALL_OPEN_MOON_PETAL = 0xFE11,
	TAG_BALL_OPEN_MEMORY_VEIL = 0xFE12,
	TAG_BALL_OPEN_TEAR_WISTERIA = 0xFE13,
	TAG_BALL_OPEN_SWEET_HEART = 0xFE14,
	TAG_BALL_OPEN_SILENT_HALO = 0xFE15,
	TAG_BALL_OPEN_GAME_PULSE = 0xFE16,
	TAG_BALL_OPEN_WINTER_BLOSSOM = 0xFE17,
	TAG_BALL_OPEN_THORN_HEART = 0xFE18,
	TAG_BALL_OPEN_FOX_SPIRIT = 0xFE19,
	TAG_BALL_OPEN_BERRY_BLOOM = 0xFE1A,
// WATASHI_BALLS_END ptag
	TAG_BALL_OPEN_ROCKET = 0xFE1B, //[hack] patches/0914
	TAG_BALL_OPEN_SNATCH = 0xFE1C, //[hack] patches/0961
};

const struct CompressedSpriteSheet gBallSpriteSheets[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] = 	{gInterfaceGfx_PokeBallTiles,		(16 * 48) / 2, GFX_TAG_POKEBALL},
	[BALL_TYPE_GREAT_BALL] = 	{gInterfaceGfx_GreatBallTiles,   	(16 * 48) / 2, GFX_TAG_GREATBALL},
	[BALL_TYPE_SAFARI_BALL] = 	{gInterfaceGfx_SafariBallTiles,  	(16 * 48) / 2, GFX_TAG_SAFARIBALL},
	[BALL_TYPE_ULTRA_BALL] = 	{gInterfaceGfx_UltraBallTiles,   	(16 * 48) / 2, GFX_TAG_ULTRABALL},
	[BALL_TYPE_MASTER_BALL] = 	{gInterfaceGfx_MasterBallTiles,  	(16 * 48) / 2, GFX_TAG_MASTERBALL},
	[BALL_TYPE_NET_BALL] = 		{gInterfaceGfx_NetBallTiles,	 	(16 * 48) / 2, GFX_TAG_NETBALL},
	[BALL_TYPE_DIVE_BALL] = 	{gInterfaceGfx_DiveBallTiles,		(16 * 48) / 2, GFX_TAG_DIVEBALL},
	[BALL_TYPE_NEST_BALL] = 	{gInterfaceGfx_NestBallTiles,		(16 * 48) / 2, GFX_TAG_NESTBALL},
	[BALL_TYPE_REPEAT_BALL] = 	{gInterfaceGfx_RepeatBallTiles,  	(16 * 48) / 2, GFX_TAG_REPEATBALL},
	[BALL_TYPE_TIMER_BALL] = 	{gInterfaceGfx_TimerBallTiles,   	(16 * 48) / 2, GFX_TAG_TIMERBALL},
	[BALL_TYPE_LUXURY_BALL] = 	{gInterfaceGfx_LuxuryBallTiles,  	(16 * 48) / 2, GFX_TAG_LUXURYBALL},
	[BALL_TYPE_PREMIER_BALL] = 	{gInterfaceGfx_PremierBallTiles, 	(16 * 48) / 2, GFX_TAG_PREMIERBALL},
	[BALL_TYPE_DUSK_BALL] = 	{gInterfaceGfx_DuskBallTiles, 		(16 * 48) / 2, GFX_TAG_DUSKBALL},
	[BALL_TYPE_HEAL_BALL] = 	{gInterfaceGfx_HealBallTiles, 		(16 * 48) / 2, GFX_TAG_HEALBALL},
	[BALL_TYPE_QUICK_BALL] = 	{gInterfaceGfx_QuickBallTiles, 		(16 * 48) / 2, GFX_TAG_QUICKBALL},
	[BALL_TYPE_CHERISH_BALL] = 	{gInterfaceGfx_CherishBallTiles, 	(16 * 48) / 2, GFX_TAG_CHERISHBALL},
	[BALL_TYPE_PARK_BALL] = 	{gInterfaceGfx_ParkBallTiles, 		(16 * 48) / 2, GFX_TAG_PARKBALL},
	[BALL_TYPE_FAST_BALL] = 	{gInterfaceGfx_FastBallTiles, 		(16 * 48) / 2, GFX_TAG_FASTBALL},
	[BALL_TYPE_LEVEL_BALL] = 	{gInterfaceGfx_LevelBallTiles, 		(16 * 48) / 2, GFX_TAG_LEVELBALL},
	[BALL_TYPE_LURE_BALL] = 	{gInterfaceGfx_LureBallTiles, 		(16 * 48) / 2, GFX_TAG_LUREBALL},
	[BALL_TYPE_HEAVY_BALL] = 	{gInterfaceGfx_HeavyBallTiles, 		(16 * 48) / 2, GFX_TAG_HEAVYBALL},
	[BALL_TYPE_LOVE_BALL] = 	{gInterfaceGfx_LoveBallTiles, 		(16 * 48) / 2, GFX_TAG_LOVEBALL},
	[BALL_TYPE_FRIEND_BALL] = 	{gInterfaceGfx_FriendBallTiles, 	(16 * 48) / 2, GFX_TAG_FRIENDBALL},
	[BALL_TYPE_MOON_BALL] = 	{gInterfaceGfx_MoonBallTiles, 		(16 * 48) / 2, GFX_TAG_MOONBALL},
	[BALL_TYPE_SPORT_BALL] = 	{gInterfaceGfx_SportBallTiles, 		(16 * 48) / 2, GFX_TAG_SPORTBALL},
	[BALL_TYPE_BEAST_BALL] = 	{gInterfaceGfx_BeastBallTiles, 		(16 * 48) / 2, GFX_TAG_BEASTBALL},
	[BALL_TYPE_DREAM_BALL] = 	{gInterfaceGfx_DreamBallTiles, 		(16 * 48) / 2, GFX_TAG_DREAMBALL},
	[BALL_TYPE_START_BALL] = 	{gInterfaceGfx_StartBallTiles, 		(16 * 48) / 2, GFX_TAG_STARTBALL},
// WATASHI_BALLS_BEGIN sheets
	[BALL_TYPE_MOON_PETAL_BALL] = 	{gInterfaceGfx_MoonPetalBallTiles, 		(16 * 48) / 2, GFX_TAG_MOON_PETAL_BALL},
	[BALL_TYPE_MEMORY_VEIL_BALL] = 	{gInterfaceGfx_MemoryVeilBallTiles, 		(16 * 48) / 2, GFX_TAG_MEMORY_VEIL_BALL},
	[BALL_TYPE_TEAR_WISTERIA_BALL] = 	{gInterfaceGfx_TearWisteriaBallTiles, 		(16 * 48) / 2, GFX_TAG_TEAR_WISTERIA_BALL},
	[BALL_TYPE_SWEET_HEART_BALL] = 	{gInterfaceGfx_SweetHeartBallTiles, 		(16 * 48) / 2, GFX_TAG_SWEET_HEART_BALL},
	[BALL_TYPE_SILENT_HALO_BALL] = 	{gInterfaceGfx_SilentHaloBallTiles, 		(16 * 48) / 2, GFX_TAG_SILENT_HALO_BALL},
	[BALL_TYPE_GAME_PULSE_BALL] = 	{gInterfaceGfx_GamePulseBallTiles, 		(16 * 48) / 2, GFX_TAG_GAME_PULSE_BALL},
	[BALL_TYPE_WINTER_BLOSSOM_BALL] = 	{gInterfaceGfx_WinterBlossomBallTiles, 		(16 * 48) / 2, GFX_TAG_WINTER_BLOSSOM_BALL},
	[BALL_TYPE_THORN_HEART_BALL] = 	{gInterfaceGfx_ThornHeartBallTiles, 		(16 * 48) / 2, GFX_TAG_THORN_HEART_BALL},
	[BALL_TYPE_FOX_SPIRIT_BALL] = 	{gInterfaceGfx_FoxSpiritBallTiles, 		(16 * 48) / 2, GFX_TAG_FOX_SPIRIT_BALL},
	[BALL_TYPE_BERRY_BLOOM_BALL] = 	{gInterfaceGfx_BerryBloomBallTiles, 		(16 * 48) / 2, GFX_TAG_BERRY_BLOOM_BALL},
// WATASHI_BALLS_END sheets
	[BALL_TYPE_ROCKET_BALL] = 	{gInterfaceGfx_RocketBallTiles, 		(16 * 48) / 2, GFX_TAG_ROCKET_BALL}, //[hack] patches/0914
	[BALL_TYPE_SNATCH_BALL] = 	{gInterfaceGfx_SnatchBallTiles, 		(16 * 48) / 2, GFX_TAG_SNATCH_BALL}, //[hack] patches/0961
	[POKEBALL_COUNT] =		 	{gInterfaceGfx_DynamaxBallTiles, 	(16 * 48) / 2, GFX_TAG_DYNAMAXBALL},
};

const struct CompressedSpritePalette gBallSpritePalettes[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] = 	{gInterfaceGfx_PokeBallPal,		GFX_TAG_POKEBALL},
	[BALL_TYPE_GREAT_BALL] = 	{gInterfaceGfx_GreatBallPal,   	GFX_TAG_GREATBALL},
	[BALL_TYPE_SAFARI_BALL] = 	{gInterfaceGfx_SafariBallPal,  	GFX_TAG_SAFARIBALL},
	[BALL_TYPE_ULTRA_BALL] = 	{gInterfaceGfx_UltraBallPal,   	GFX_TAG_ULTRABALL},
	[BALL_TYPE_MASTER_BALL] = 	{gInterfaceGfx_MasterBallPal,  	GFX_TAG_MASTERBALL},
	[BALL_TYPE_NET_BALL] = 		{gInterfaceGfx_NetBallPal,	 	GFX_TAG_NETBALL},
	[BALL_TYPE_DIVE_BALL] = 	{gInterfaceGfx_DiveBallPal,		GFX_TAG_DIVEBALL},
	[BALL_TYPE_NEST_BALL] = 	{gInterfaceGfx_NestBallPal,		GFX_TAG_NESTBALL},
	[BALL_TYPE_REPEAT_BALL] = 	{gInterfaceGfx_RepeatBallPal,  	GFX_TAG_REPEATBALL},
	[BALL_TYPE_TIMER_BALL] = 	{gInterfaceGfx_TimerBallPal,   	GFX_TAG_TIMERBALL},
	[BALL_TYPE_LUXURY_BALL] = 	{gInterfaceGfx_LuxuryBallPal,  	GFX_TAG_LUXURYBALL},
	[BALL_TYPE_PREMIER_BALL] = 	{gInterfaceGfx_PremierBallPal, 	GFX_TAG_PREMIERBALL},
	[BALL_TYPE_DUSK_BALL] = 	{gInterfaceGfx_DuskBallPal, 	GFX_TAG_DUSKBALL},
	[BALL_TYPE_HEAL_BALL] = 	{gInterfaceGfx_HealBallPal, 	GFX_TAG_HEALBALL},
	[BALL_TYPE_QUICK_BALL] = 	{gInterfaceGfx_QuickBallPal, 	GFX_TAG_QUICKBALL},
	[BALL_TYPE_CHERISH_BALL] = 	{gInterfaceGfx_CherishBallPal, 	GFX_TAG_CHERISHBALL},
	[BALL_TYPE_PARK_BALL] = 	{gInterfaceGfx_ParkBallPal, 	GFX_TAG_PARKBALL},
	[BALL_TYPE_FAST_BALL] = 	{gInterfaceGfx_FastBallPal, 	GFX_TAG_FASTBALL},
	[BALL_TYPE_LEVEL_BALL] = 	{gInterfaceGfx_LevelBallPal, 	GFX_TAG_LEVELBALL},
	[BALL_TYPE_LURE_BALL] = 	{gInterfaceGfx_LureBallPal, 	GFX_TAG_LUREBALL},
	[BALL_TYPE_HEAVY_BALL] = 	{gInterfaceGfx_HeavyBallPal, 	GFX_TAG_HEAVYBALL},
	[BALL_TYPE_LOVE_BALL] = 	{gInterfaceGfx_LoveBallPal, 	GFX_TAG_LOVEBALL},
	[BALL_TYPE_FRIEND_BALL] = 	{gInterfaceGfx_FriendBallPal, 	GFX_TAG_FRIENDBALL},
	[BALL_TYPE_MOON_BALL] = 	{gInterfaceGfx_MoonBallPal, 	GFX_TAG_MOONBALL},
	[BALL_TYPE_SPORT_BALL] = 	{gInterfaceGfx_SportBallPal, 	GFX_TAG_SPORTBALL},
	[BALL_TYPE_BEAST_BALL] = 	{gInterfaceGfx_BeastBallPal, 	GFX_TAG_BEASTBALL},
	[BALL_TYPE_DREAM_BALL] = 	{gInterfaceGfx_DreamBallPal, 	GFX_TAG_DREAMBALL},
	[BALL_TYPE_START_BALL] = 	{gInterfaceGfx_StartBallPal, 	GFX_TAG_STARTBALL},
// WATASHI_BALLS_BEGIN pals
	[BALL_TYPE_MOON_PETAL_BALL] = 	{gInterfaceGfx_MoonPetalBallPal, 	GFX_TAG_MOON_PETAL_BALL},
	[BALL_TYPE_MEMORY_VEIL_BALL] = 	{gInterfaceGfx_MemoryVeilBallPal, 	GFX_TAG_MEMORY_VEIL_BALL},
	[BALL_TYPE_TEAR_WISTERIA_BALL] = 	{gInterfaceGfx_TearWisteriaBallPal, 	GFX_TAG_TEAR_WISTERIA_BALL},
	[BALL_TYPE_SWEET_HEART_BALL] = 	{gInterfaceGfx_SweetHeartBallPal, 	GFX_TAG_SWEET_HEART_BALL},
	[BALL_TYPE_SILENT_HALO_BALL] = 	{gInterfaceGfx_SilentHaloBallPal, 	GFX_TAG_SILENT_HALO_BALL},
	[BALL_TYPE_GAME_PULSE_BALL] = 	{gInterfaceGfx_GamePulseBallPal, 	GFX_TAG_GAME_PULSE_BALL},
	[BALL_TYPE_WINTER_BLOSSOM_BALL] = 	{gInterfaceGfx_WinterBlossomBallPal, 	GFX_TAG_WINTER_BLOSSOM_BALL},
	[BALL_TYPE_THORN_HEART_BALL] = 	{gInterfaceGfx_ThornHeartBallPal, 	GFX_TAG_THORN_HEART_BALL},
	[BALL_TYPE_FOX_SPIRIT_BALL] = 	{gInterfaceGfx_FoxSpiritBallPal, 	GFX_TAG_FOX_SPIRIT_BALL},
	[BALL_TYPE_BERRY_BLOOM_BALL] = 	{gInterfaceGfx_BerryBloomBallPal, 	GFX_TAG_BERRY_BLOOM_BALL},
// WATASHI_BALLS_END pals
	[BALL_TYPE_ROCKET_BALL] = 	{gInterfaceGfx_RocketBallPal, 	GFX_TAG_ROCKET_BALL},
	[BALL_TYPE_SNATCH_BALL] = 	{gInterfaceGfx_SnatchBallPal, 	GFX_TAG_SNATCH_BALL},
	[POKEBALL_COUNT] =		 	{gInterfaceGfx_DynamaxBallPal,	GFX_TAG_DYNAMAXBALL},
};

const struct SpriteTemplate gBallSpriteTemplates[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] =
	{
		.tileTag = GFX_TAG_POKEBALL,
		.paletteTag = GFX_TAG_POKEBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_GREAT_BALL] =
	{
		.tileTag = GFX_TAG_GREATBALL,
		.paletteTag = GFX_TAG_GREATBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_SAFARI_BALL] =
	{
		.tileTag = GFX_TAG_SAFARIBALL,
		.paletteTag = GFX_TAG_SAFARIBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_ULTRA_BALL] =
	{
		.tileTag = GFX_TAG_ULTRABALL,
		.paletteTag = GFX_TAG_ULTRABALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_MASTER_BALL] =
	{
		.tileTag = GFX_TAG_MASTERBALL,
		.paletteTag = GFX_TAG_MASTERBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_NET_BALL] =
	{
		.tileTag = GFX_TAG_NETBALL,
		.paletteTag = GFX_TAG_NETBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_DIVE_BALL] =
	{
		.tileTag = GFX_TAG_DIVEBALL,
		.paletteTag = GFX_TAG_DIVEBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_NEST_BALL] =
	{
		.tileTag = GFX_TAG_NESTBALL,
		.paletteTag = GFX_TAG_NESTBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_REPEAT_BALL] =
	{
		.tileTag = GFX_TAG_REPEATBALL,
		.paletteTag = GFX_TAG_REPEATBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_TIMER_BALL] =
	{
		.tileTag = GFX_TAG_TIMERBALL,
		.paletteTag = GFX_TAG_TIMERBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_LUXURY_BALL] =
	{
		.tileTag = GFX_TAG_LUXURYBALL,
		.paletteTag = GFX_TAG_LUXURYBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_PREMIER_BALL] =
	{
		.tileTag = GFX_TAG_PREMIERBALL,
		.paletteTag = GFX_TAG_PREMIERBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_DUSK_BALL] =
	{
		.tileTag = GFX_TAG_DUSKBALL,
		.paletteTag = GFX_TAG_DUSKBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_HEAL_BALL] =
	{
		.tileTag = GFX_TAG_HEALBALL,
		.paletteTag = GFX_TAG_HEALBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_QUICK_BALL] =
	{
		.tileTag = GFX_TAG_QUICKBALL,
		.paletteTag = GFX_TAG_QUICKBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_CHERISH_BALL] =
	{
		.tileTag = GFX_TAG_CHERISHBALL,
		.paletteTag = GFX_TAG_CHERISHBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_PARK_BALL] =
	{
		.tileTag = GFX_TAG_PARKBALL,
		.paletteTag = GFX_TAG_PARKBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_FAST_BALL] =
	{
		.tileTag = GFX_TAG_FASTBALL,
		.paletteTag = GFX_TAG_FASTBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_LEVEL_BALL] =
	{
		.tileTag = GFX_TAG_LEVELBALL,
		.paletteTag = GFX_TAG_LEVELBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_LURE_BALL] =
	{
		.tileTag = GFX_TAG_LUREBALL,
		.paletteTag = GFX_TAG_LUREBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_HEAVY_BALL] =
	{
		.tileTag = GFX_TAG_HEAVYBALL,
		.paletteTag = GFX_TAG_HEAVYBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_LOVE_BALL] =
	{
		.tileTag = GFX_TAG_LOVEBALL,
		.paletteTag = GFX_TAG_LOVEBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_FRIEND_BALL] =
	{
		.tileTag = GFX_TAG_FRIENDBALL,
		.paletteTag = GFX_TAG_FRIENDBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_MOON_BALL] =
	{
		.tileTag = GFX_TAG_MOONBALL,
		.paletteTag = GFX_TAG_MOONBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_SPORT_BALL] =
	{
		.tileTag = GFX_TAG_SPORTBALL,
		.paletteTag = GFX_TAG_SPORTBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_BEAST_BALL] =
	{
		.tileTag = GFX_TAG_BEASTBALL,
		.paletteTag = GFX_TAG_BEASTBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_DREAM_BALL] =
	{
		.tileTag = GFX_TAG_DREAMBALL,
		.paletteTag = GFX_TAG_DREAMBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_START_BALL] =
	{
		.tileTag = GFX_TAG_STARTBALL,
		.paletteTag = GFX_TAG_STARTBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
// WATASHI_BALLS_BEGIN templates
	[BALL_TYPE_MOON_PETAL_BALL] =
	{
		.tileTag = GFX_TAG_MOON_PETAL_BALL,
		.paletteTag = GFX_TAG_MOON_PETAL_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_MEMORY_VEIL_BALL] =
	{
		.tileTag = GFX_TAG_MEMORY_VEIL_BALL,
		.paletteTag = GFX_TAG_MEMORY_VEIL_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_TEAR_WISTERIA_BALL] =
	{
		.tileTag = GFX_TAG_TEAR_WISTERIA_BALL,
		.paletteTag = GFX_TAG_TEAR_WISTERIA_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_SWEET_HEART_BALL] =
	{
		.tileTag = GFX_TAG_SWEET_HEART_BALL,
		.paletteTag = GFX_TAG_SWEET_HEART_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_SILENT_HALO_BALL] =
	{
		.tileTag = GFX_TAG_SILENT_HALO_BALL,
		.paletteTag = GFX_TAG_SILENT_HALO_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_GAME_PULSE_BALL] =
	{
		.tileTag = GFX_TAG_GAME_PULSE_BALL,
		.paletteTag = GFX_TAG_GAME_PULSE_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_WINTER_BLOSSOM_BALL] =
	{
		.tileTag = GFX_TAG_WINTER_BLOSSOM_BALL,
		.paletteTag = GFX_TAG_WINTER_BLOSSOM_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_THORN_HEART_BALL] =
	{
		.tileTag = GFX_TAG_THORN_HEART_BALL,
		.paletteTag = GFX_TAG_THORN_HEART_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_FOX_SPIRIT_BALL] =
	{
		.tileTag = GFX_TAG_FOX_SPIRIT_BALL,
		.paletteTag = GFX_TAG_FOX_SPIRIT_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_BERRY_BLOOM_BALL] =
	{
		.tileTag = GFX_TAG_BERRY_BLOOM_BALL,
		.paletteTag = GFX_TAG_BERRY_BLOOM_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
// WATASHI_BALLS_END templates
	[BALL_TYPE_ROCKET_BALL] =
	{
		.tileTag = GFX_TAG_ROCKET_BALL,
		.paletteTag = GFX_TAG_ROCKET_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[BALL_TYPE_SNATCH_BALL] = //[hack] patches/0961
	{
		.tileTag = GFX_TAG_SNATCH_BALL,
		.paletteTag = GFX_TAG_SNATCH_BALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
	[POKEBALL_COUNT] =
	{
		.tileTag = GFX_TAG_DYNAMAXBALL,
		.paletteTag = GFX_TAG_DYNAMAXBALL,
		.oam = sBallOamData,
		.anims = sBallAnimSequences,
		.images = NULL,
		.affineAnims = sBallAffineAnimSequences,
		.callback = SpriteCB_TestBallThrow,
	},
};

const struct CompressedSpriteSheet gBallParticleSpritesheets[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55020},
	[BALL_TYPE_GREAT_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55021},
	[BALL_TYPE_SAFARI_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55022},
	[BALL_TYPE_ULTRA_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55023},
	[BALL_TYPE_MASTER_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55024},
	[BALL_TYPE_NET_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55025},
	[BALL_TYPE_DIVE_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55026},
	[BALL_TYPE_NEST_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55027},
	[BALL_TYPE_REPEAT_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55028},
	[BALL_TYPE_TIMER_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55029},
	[BALL_TYPE_LUXURY_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55030},
	[BALL_TYPE_PREMIER_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, 55031},
	[BALL_TYPE_DUSK_BALL] =		{gBattleAnimSpriteSheet_ParticlesDuskBallTiles, 			(8 * 64) / 2, TAG_BALL_OPEN_DUSK},
	[BALL_TYPE_HEAL_BALL] =		{gBattleAnimSpriteSheet_ParticlesHealBallTiles, 			(8 * 64) / 2, TAG_BALL_OPEN_HEAL},
	[BALL_TYPE_QUICK_BALL] =	{gBattleAnimSpriteSheet_ParticlesQuickCherishBallTiles, 	(8 * 64) / 2, TAG_BALL_OPEN_QUICK},
	[BALL_TYPE_CHERISH_BALL] =	{gBattleAnimSpriteSheet_ParticlesQuickCherishBallTiles, 	(8 * 64) / 2, TAG_BALL_OPEN_CHERISH},
	[BALL_TYPE_PARK_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_PARK},
	[BALL_TYPE_FAST_BALL] =		{gBattleAnimSpriteSheet_ParticlesQuickCherishBallTiles, 	(8 * 64) / 2, TAG_BALL_OPEN_FAST},
	[BALL_TYPE_LEVEL_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_LEVEL},
	[BALL_TYPE_LURE_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_LURE},
	[BALL_TYPE_HEAVY_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_HEAVY},
	[BALL_TYPE_LOVE_BALL] =		{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_LOVE},
	[BALL_TYPE_FRIEND_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_FRIEND},
	[BALL_TYPE_MOON_BALL] =		{gBattleAnimSpriteSheet_ParticlesHealBallTiles, 			(8 * 64) / 2, TAG_BALL_OPEN_MOON},
	[BALL_TYPE_SPORT_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_SPORT},
	[BALL_TYPE_BEAST_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_BEAST},
	[BALL_TYPE_DREAM_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_DREAM},
	[BALL_TYPE_START_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_START},
// WATASHI_BALLS_BEGIN psheets
	[BALL_TYPE_MOON_PETAL_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_MOON_PETAL},
	[BALL_TYPE_MEMORY_VEIL_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_MEMORY_VEIL},
	[BALL_TYPE_TEAR_WISTERIA_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_TEAR_WISTERIA},
	[BALL_TYPE_SWEET_HEART_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_SWEET_HEART},
	[BALL_TYPE_SILENT_HALO_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_SILENT_HALO},
	[BALL_TYPE_GAME_PULSE_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_GAME_PULSE},
	[BALL_TYPE_WINTER_BLOSSOM_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_WINTER_BLOSSOM},
	[BALL_TYPE_THORN_HEART_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_THORN_HEART},
	[BALL_TYPE_FOX_SPIRIT_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_FOX_SPIRIT},
	[BALL_TYPE_BERRY_BLOOM_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_BERRY_BLOOM},
// WATASHI_BALLS_END psheets
	[BALL_TYPE_ROCKET_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_ROCKET},
	[BALL_TYPE_SNATCH_BALL] =	{gBattleAnimSpriteSheet_Particles, 							(8 * 64) / 2, TAG_BALL_OPEN_SNATCH}, //[hack] patches/0961
	[POKEBALL_COUNT] =			{gBattleAnimSpriteSheet_ParticlesDynamaxBallTiles, 			(8 * 64) / 2, TAG_BALL_OPEN_DYNAMAX},
};

const struct CompressedSpritePalette gBallParticlePalettes[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] = 	{gBattleAnimSpritePalette_136, 							55020},
	[BALL_TYPE_GREAT_BALL] = 	{gBattleAnimSpritePalette_136, 							55021},
	[BALL_TYPE_SAFARI_BALL] = 	{gBattleAnimSpritePalette_136, 							55022},
	[BALL_TYPE_ULTRA_BALL] = 	{gBattleAnimSpritePalette_136, 							55023},
	[BALL_TYPE_MASTER_BALL] = 	{gBattleAnimSpritePalette_136, 							55024},
	[BALL_TYPE_NET_BALL] = 		{gBattleAnimSpritePalette_136, 							55025},
	[BALL_TYPE_DIVE_BALL] = 	{gBattleAnimSpritePalette_136, 							55026},
	[BALL_TYPE_NEST_BALL] = 	{gBattleAnimSpritePalette_136, 							55027},
	[BALL_TYPE_REPEAT_BALL] = 	{gBattleAnimSpritePalette_136, 							55028},
	[BALL_TYPE_TIMER_BALL] = 	{gBattleAnimSpritePalette_136, 							55029},
	[BALL_TYPE_LUXURY_BALL] = 	{gBattleAnimSpritePalette_136, 							55030},
	[BALL_TYPE_PREMIER_BALL] = 	{gBattleAnimSpritePalette_136, 							55031},
	[BALL_TYPE_DUSK_BALL] = 	{gBattleAnimSpriteSheet_ParticlesDuskBallPal, 			TAG_BALL_OPEN_DUSK},
	[BALL_TYPE_HEAL_BALL] = 	{gBattleAnimSpriteSheet_ParticlesHealBallPal, 			TAG_BALL_OPEN_HEAL},
	[BALL_TYPE_QUICK_BALL] = 	{gBattleAnimSpriteSheet_ParticlesQuickCherishBallPal, 	TAG_BALL_OPEN_QUICK},
	[BALL_TYPE_CHERISH_BALL] = 	{gBattleAnimSpriteSheet_ParticlesQuickCherishBallPal, 	TAG_BALL_OPEN_CHERISH},
	[BALL_TYPE_PARK_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_PARK},
	[BALL_TYPE_FAST_BALL] = 	{gBattleAnimSpriteSheet_ParticlesQuickCherishBallPal, 	TAG_BALL_OPEN_FAST},
	[BALL_TYPE_LEVEL_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_LEVEL},
	[BALL_TYPE_LURE_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_LURE},
	[BALL_TYPE_HEAVY_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_HEAVY},
	[BALL_TYPE_LOVE_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_LOVE},
	[BALL_TYPE_FRIEND_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_FRIEND},
	[BALL_TYPE_MOON_BALL] = 	{gBattleAnimSpriteSheet_ParticlesHealBallPal, 			TAG_BALL_OPEN_MOON},
	[BALL_TYPE_SPORT_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_SPORT},
	[BALL_TYPE_BEAST_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_BEAST},
	[BALL_TYPE_DREAM_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_DREAM},
	[BALL_TYPE_START_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_START},
// WATASHI_BALLS_BEGIN ppals
	[BALL_TYPE_MOON_PETAL_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_MOON_PETAL},
	[BALL_TYPE_MEMORY_VEIL_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_MEMORY_VEIL},
	[BALL_TYPE_TEAR_WISTERIA_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_TEAR_WISTERIA},
	[BALL_TYPE_SWEET_HEART_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_SWEET_HEART},
	[BALL_TYPE_SILENT_HALO_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_SILENT_HALO},
	[BALL_TYPE_GAME_PULSE_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_GAME_PULSE},
	[BALL_TYPE_WINTER_BLOSSOM_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_WINTER_BLOSSOM},
	[BALL_TYPE_THORN_HEART_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_THORN_HEART},
	[BALL_TYPE_FOX_SPIRIT_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_FOX_SPIRIT},
	[BALL_TYPE_BERRY_BLOOM_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_BERRY_BLOOM},
// WATASHI_BALLS_END ppals
	[BALL_TYPE_ROCKET_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_ROCKET},
	[BALL_TYPE_SNATCH_BALL] = 	{gBattleAnimSpritePalette_136, 							TAG_BALL_OPEN_SNATCH}, //[hack] patches/0961
	[POKEBALL_COUNT] =			{gBattleAnimSpriteSheet_ParticlesDynamaxBallPal,		TAG_BALL_OPEN_DYNAMAX},
};

const u8 gBallOpenParticleAnimNums[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] = 	BALL_OPEN_STICKS, 			//Poke Ball
	[BALL_TYPE_GREAT_BALL] = 	BALL_OPEN_STICKS, 			//Great Ball
	[BALL_TYPE_SAFARI_BALL] = 	BALL_OPEN_STICKS, 			//Safari Ball
	[BALL_TYPE_ULTRA_BALL] = 	BALL_OPEN_GREEN_BUBBLES, 	//Ultra Ball
	[BALL_TYPE_MASTER_BALL] = 	BALL_OPEN_YELLOW_STARS, 	//Master Ball
	[BALL_TYPE_NET_BALL] = 		BALL_OPEN_BLUE_BUBBLES, 	//Net Ball
	[BALL_TYPE_DIVE_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//Dive Ball
	[BALL_TYPE_NEST_BALL] = 	BALL_OPEN_HEARTS, 			//Nest Ball
	[BALL_TYPE_REPEAT_BALL] = 	BALL_OPEN_GREEN_BUBBLES, 	//Repeat Ball
	[BALL_TYPE_TIMER_BALL] = 	BALL_OPEN_GREEN_BUBBLES, 	//Timer Ball
	[BALL_TYPE_LUXURY_BALL] = 	BALL_OPEN_GREEN_STARS, 		//Luxury Ball
	[BALL_TYPE_PREMIER_BALL] = 	BALL_OPEN_GREEN_STARS, 		//Premier Ball
	[BALL_TYPE_DUSK_BALL] = 	BALL_OPEN_PURPLE_CIRCLES, 	//Dusk Ball
	[BALL_TYPE_HEAL_BALL] = 	BALL_OPEN_WHITE_STARS, 		//Heal Ball
	[BALL_TYPE_QUICK_BALL] = 	BALL_OPEN_YELLOW_CIRCLES, 	//Quick Ball
	[BALL_TYPE_CHERISH_BALL] = 	BALL_OPEN_RAINBOW_CIRCLES, 	//Cherish Ball
	[BALL_TYPE_PARK_BALL] = 	BALL_OPEN_YELLOW_STARS, 	//Park Ball
	[BALL_TYPE_FAST_BALL] = 	BALL_OPEN_YELLOW_CIRCLES, 	//Fast Ball
	[BALL_TYPE_LEVEL_BALL] = 	BALL_OPEN_STICKS, 			//Level Ball
	[BALL_TYPE_LURE_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//Lure Ball
	[BALL_TYPE_HEAVY_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//Heavy Ball
	[BALL_TYPE_LOVE_BALL] = 	BALL_OPEN_HEARTS, 			//Love Ball
	[BALL_TYPE_FRIEND_BALL] = 	BALL_OPEN_HEARTS, 			//Friend Ball
	[BALL_TYPE_MOON_BALL] = 	BALL_OPEN_WHITE_STARS, 		//Moon Ball
	[BALL_TYPE_SPORT_BALL] = 	BALL_OPEN_STICKS, 			//Sport Ball
	[BALL_TYPE_BEAST_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//Beast Ball
	[BALL_TYPE_DREAM_BALL] = 	BALL_OPEN_HEARTS, 			//Dream Ball
	[BALL_TYPE_START_BALL] = 	BALL_OPEN_YELLOW_STARS, 	//START Ball（金の星）
// WATASHI_BALLS_BEGIN anims
	[BALL_TYPE_MOON_PETAL_BALL] = 	BALL_OPEN_HEARTS, 	//ムーンペタルボール
	[BALL_TYPE_MEMORY_VEIL_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//メモリーヴェールボール
	[BALL_TYPE_TEAR_WISTERIA_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//ティアウィステリアボール
	[BALL_TYPE_SWEET_HEART_BALL] = 	BALL_OPEN_HEARTS, 	//スイートハートボール
	[BALL_TYPE_SILENT_HALO_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//サイレントヘイローボール
	[BALL_TYPE_GAME_PULSE_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//ゲームパルスボール
	[BALL_TYPE_WINTER_BLOSSOM_BALL] = 	BALL_OPEN_BLUE_BUBBLES, 	//ウィンターブロッサムボール
	[BALL_TYPE_THORN_HEART_BALL] = 	BALL_OPEN_HEARTS, 	//ソーンハートボール
	[BALL_TYPE_FOX_SPIRIT_BALL] = 	BALL_OPEN_YELLOW_STARS, 	//フォックススピリットボール
	[BALL_TYPE_BERRY_BLOOM_BALL] = 	BALL_OPEN_HEARTS, 	//ベリーブルームボール
// WATASHI_BALLS_END anims
	[BALL_TYPE_ROCKET_BALL] = 	BALL_OPEN_STICKS, 	//ロケットボール
	[BALL_TYPE_SNATCH_BALL] = 	BALL_OPEN_STICKS, 	//スナッチボール [hack] patches/0961
	[POKEBALL_COUNT] =			BALL_OPEN_RAINBOW_CIRCLES,	//Dynamax Ball
};

const TaskFunc gBallOpenParticleAnimationFuncs[POKEBALL_COUNT + 1] =
{
	[BALL_TYPE_POKE_BALL] = 	PokeBallOpenParticleAnimation, //Poke Ball
	[BALL_TYPE_GREAT_BALL] = 	GreatBallOpenParticleAnimation, //Great Ball
	[BALL_TYPE_SAFARI_BALL] = 	SafariBallOpenParticleAnimation, //Safari Ball
	[BALL_TYPE_ULTRA_BALL] = 	UltraBallOpenParticleAnimation, //Ultra Ball
	[BALL_TYPE_MASTER_BALL] = 	MasterBallOpenParticleAnimation, //Master Ball
	[BALL_TYPE_NET_BALL] = 		SafariBallOpenParticleAnimation, //Net Ball
	[BALL_TYPE_DIVE_BALL] = 	DiveBallOpenParticleAnimation, //Dive Ball
	[BALL_TYPE_NEST_BALL] = 	UltraBallOpenParticleAnimation, //Nest Ball
	[BALL_TYPE_REPEAT_BALL] = 	RepeatBallOpenParticleAnimation, //Repeat Ball
	[BALL_TYPE_TIMER_BALL] = 	TimerBallOpenParticleAnimation, //Timer Ball
	[BALL_TYPE_LUXURY_BALL] = 	GreatBallOpenParticleAnimation, //Luxury Ball
	[BALL_TYPE_PREMIER_BALL] = 	PremierBallOpenParticleAnimation, //Premier Ball
	[BALL_TYPE_DUSK_BALL] = 	GreatBallOpenParticleAnimation, //Dusk Ball
	[BALL_TYPE_HEAL_BALL] = 	SafariBallOpenParticleAnimation, //Heal Ball
	[BALL_TYPE_QUICK_BALL] = 	UltraBallOpenParticleAnimation, //Quick Ball
	[BALL_TYPE_CHERISH_BALL] = 	DiveBallOpenParticleAnimation, //Cherish Ball
	[BALL_TYPE_PARK_BALL] = 	PokeBallOpenParticleAnimation, //Park Ball
	[BALL_TYPE_FAST_BALL] = 	TimerBallOpenParticleAnimation, //Fast Ball
	[BALL_TYPE_LEVEL_BALL] = 	SafariBallOpenParticleAnimation, //Level Ball
	[BALL_TYPE_LURE_BALL] = 	SafariBallOpenParticleAnimation, //Lure Ball
	[BALL_TYPE_HEAVY_BALL] = 	RepeatBallOpenParticleAnimation, //Heavy Ball
	[BALL_TYPE_LOVE_BALL] = 	GreatBallOpenParticleAnimation, //Love Ball
	[BALL_TYPE_FRIEND_BALL] = 	PokeBallOpenParticleAnimation, //Friend Ball
	[BALL_TYPE_MOON_BALL] = 	DiveBallOpenParticleAnimation, //Moon Ball
	[BALL_TYPE_SPORT_BALL] = 	PokeBallOpenParticleAnimation, //Sport Ball
	[BALL_TYPE_BEAST_BALL] = 	MasterBallOpenParticleAnimation, //Beast Ball
	[BALL_TYPE_DREAM_BALL] = 	RepeatBallOpenParticleAnimation, //Dream Ball
	[BALL_TYPE_START_BALL] = 	MasterBallOpenParticleAnimation, //START Ball
// WATASHI_BALLS_BEGIN funcs
	[BALL_TYPE_MOON_PETAL_BALL] = 	RepeatBallOpenParticleAnimation, //ムーンペタルボール
	[BALL_TYPE_MEMORY_VEIL_BALL] = 	MasterBallOpenParticleAnimation, //メモリーヴェールボール
	[BALL_TYPE_TEAR_WISTERIA_BALL] = 	MasterBallOpenParticleAnimation, //ティアウィステリアボール
	[BALL_TYPE_SWEET_HEART_BALL] = 	RepeatBallOpenParticleAnimation, //スイートハートボール
	[BALL_TYPE_SILENT_HALO_BALL] = 	MasterBallOpenParticleAnimation, //サイレントヘイローボール
	[BALL_TYPE_GAME_PULSE_BALL] = 	MasterBallOpenParticleAnimation, //ゲームパルスボール
	[BALL_TYPE_WINTER_BLOSSOM_BALL] = 	MasterBallOpenParticleAnimation, //ウィンターブロッサムボール
	[BALL_TYPE_THORN_HEART_BALL] = 	RepeatBallOpenParticleAnimation, //ソーンハートボール
	[BALL_TYPE_FOX_SPIRIT_BALL] = 	MasterBallOpenParticleAnimation, //フォックススピリットボール
	[BALL_TYPE_BERRY_BLOOM_BALL] = 	RepeatBallOpenParticleAnimation, //ベリーブルームボール
// WATASHI_BALLS_END funcs
	[BALL_TYPE_ROCKET_BALL] = 	PokeBallOpenParticleAnimation, //ロケットボール
	[BALL_TYPE_SNATCH_BALL] = 	PokeBallOpenParticleAnimation, //スナッチボール [hack] patches/0961
	[POKEBALL_COUNT] =			MasterBallOpenParticleAnimation, //Dynamax Ball
};

const u16 gBallOpenMonFadePal[] = //gUnknown_085E5310 in Emerald
{
	[BALL_TYPE_POKE_BALL] = 	RGB(31, 22, 30), //Poke Ball - Pink
	[BALL_TYPE_GREAT_BALL] = 	RGB(16, 23, 30), //Great Ball - Dull Light Blue
	[BALL_TYPE_SAFARI_BALL] = 	RGB(23, 30, 20), //Safari Ball - Light Green
	[BALL_TYPE_ULTRA_BALL] = 	RGB(31, 31, 15), //Ultra Ball - Yellow
	[BALL_TYPE_MASTER_BALL] = 	RGB(23, 20, 28), //Master Ball - Violet
	[BALL_TYPE_NET_BALL] =		RGB(21, 31, 25), //Net Ball - Turquoise
	[BALL_TYPE_DIVE_BALL] = 	RGB(12, 25, 30), //Dive Ball - Deep Sky Blue
	[BALL_TYPE_NEST_BALL] = 	RGB(30, 27, 10), //Nest Ball - Gold
	[BALL_TYPE_REPEAT_BALL] = 	RGB(31, 24, 16), //Repeat Ball - Light Orange
	[BALL_TYPE_TIMER_BALL] = 	RGB(29, 30, 30), //Timer Ball - Light Gray
	[BALL_TYPE_LUXURY_BALL] = 	RGB(31, 17, 10), //Luxury Ball - Orange
	[BALL_TYPE_PREMIER_BALL] = 	RGB(31, 9, 10), //Premier Ball - Red
	[BALL_TYPE_DUSK_BALL] = 	RGB(6, 0, 14), //Dusk Ball - Deep Purple
	[BALL_TYPE_HEAL_BALL] = 	RGB(31, 25, 31), //Heal Ball - Pink
	[BALL_TYPE_QUICK_BALL] = 	RGB(14, 20, 27), //Quick Ball - Dull Blue
	[BALL_TYPE_CHERISH_BALL] = 	RGB(31, 9, 10), //Cherish Ball - Red
	[BALL_TYPE_PARK_BALL] = 	RGB(27, 25, 5), //Park Ball - Gold
	[BALL_TYPE_FAST_BALL] = 	RGB(31, 25, 4), //Fast Ball - Light Orange
	[BALL_TYPE_LEVEL_BALL] = 	RGB(29, 23, 16), //Level Ball - Light Brown
	[BALL_TYPE_LURE_BALL] = 	RGB(12, 25, 7), //Lure Ball - Dull Green
	[BALL_TYPE_HEAVY_BALL] = 	RGB(12, 14, 15), //Heavy Ball - Darkish Gray
	[BALL_TYPE_LOVE_BALL] = 	RGB(26, 16, 20), //Love Ball - Pink
	[BALL_TYPE_FRIEND_BALL] = 	RGB(15, 22, 11), //Friend Ball - Dull Green
	[BALL_TYPE_MOON_BALL] = 	RGB(9, 9, 11), //Moon Ball - Dark Gray
	[BALL_TYPE_SPORT_BALL] = 	RGB(30, 17, 17), //Sport Ball - Pinkish Red
	[BALL_TYPE_BEAST_BALL] = 	RGB(7, 12, 31), //Beast Ball - Royal Blue
	[BALL_TYPE_DREAM_BALL] = 	RGB(31, 12, 20), //Dream Ball - Deep Pink
	[BALL_TYPE_START_BALL] = 	RGB(30, 22, 6), //START Ball - Gold
// WATASHI_BALLS_BEGIN fade
	[BALL_TYPE_MOON_PETAL_BALL] = 	RGB(25, 20, 31), //ムーンペタルボール
	[BALL_TYPE_MEMORY_VEIL_BALL] = 	RGB(22, 18, 31), //メモリーヴェールボール
	[BALL_TYPE_TEAR_WISTERIA_BALL] = 	RGB(14, 18, 31), //ティアウィステリアボール
	[BALL_TYPE_SWEET_HEART_BALL] = 	RGB(31, 22, 27), //スイートハートボール
	[BALL_TYPE_SILENT_HALO_BALL] = 	RGB(20, 19, 31), //サイレントヘイローボール
	[BALL_TYPE_GAME_PULSE_BALL] = 	RGB(10, 26, 31), //ゲームパルスボール
	[BALL_TYPE_WINTER_BLOSSOM_BALL] = 	RGB(25, 24, 31), //ウィンターブロッサムボール
	[BALL_TYPE_THORN_HEART_BALL] = 	RGB(26, 8, 16), //ソーンハートボール
	[BALL_TYPE_FOX_SPIRIT_BALL] = 	RGB(31, 18, 8), //フォックススピリットボール
	[BALL_TYPE_BERRY_BLOOM_BALL] = 	RGB(31, 14, 20), //ベリーブルームボール
// WATASHI_BALLS_END fade
	[BALL_TYPE_ROCKET_BALL] = 	RGB(31, 8, 8), //ロケットボール - 赤
	[BALL_TYPE_SNATCH_BALL] = 	RGB(20, 8, 28), //スナッチボール - 紫 [hack] patches/0961
	[POKEBALL_COUNT] =			RGB(27, 12, 23), //Dynamax Ball - Shiny Pink

	RGB(0, 0, 0), //No idea what these lower values are for
	RGB(1, 16, 0),
	RGB(3, 0, 1),
	RGB(1, 8, 0),
	RGB(0, 8, 0),
	RGB(3, 8, 1),
	RGB(6, 8, 1),
	RGB(4, 0, 0),
};

const struct SpriteTemplate gBallParticleSpriteTemplates[POKEBALL_COUNT + 1] =
{
	{ //GFX_TAG_MASTERBALL
		.tileTag = 55024,
		.paletteTag = 55024,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_ULTRABALL
		.tileTag = 55023,
		.paletteTag = 55023,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_GREATBALL
		.tileTag = 55021,
		.paletteTag = 55021,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_POKEBALL
		.tileTag = 55020,
		.paletteTag = 55020,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_SAFARIBALL
		.tileTag = 55022,
		.paletteTag = 55022,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_NETBALL
		.tileTag = 55025,
		.paletteTag = 55025,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_DIVEBALL
		.tileTag = 55026,
		.paletteTag = 55026,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_NESTBALL
		.tileTag = 55027,
		.paletteTag = 55027,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_REPEATBALL
		.tileTag = 55028,
		.paletteTag = 55028,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_TIMERBALL
		.tileTag = 55029,
		.paletteTag = 55029,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_LUXURYBALL
		.tileTag = 55030,
		.paletteTag = 55030,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_PREMIERBALL
		.tileTag = 55031,
		.paletteTag = 55031,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_DUSKBALL
		.tileTag = TAG_BALL_OPEN_DUSK,
		.paletteTag = TAG_BALL_OPEN_DUSK,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_HEALBALL
		.tileTag = TAG_BALL_OPEN_HEAL,
		.paletteTag = TAG_BALL_OPEN_HEAL,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_QUICKBALL
		.tileTag = TAG_BALL_OPEN_QUICK,
		.paletteTag = TAG_BALL_OPEN_QUICK,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_CHERISHBALL
		.tileTag = TAG_BALL_OPEN_CHERISH,
		.paletteTag = TAG_BALL_OPEN_CHERISH,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_PARKBALL
		.tileTag = TAG_BALL_OPEN_PARK,
		.paletteTag = TAG_BALL_OPEN_PARK,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_FASTBALL
		.tileTag = TAG_BALL_OPEN_FAST,
		.paletteTag = TAG_BALL_OPEN_FAST,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_LEVELBALL
		.tileTag = TAG_BALL_OPEN_LEVEL,
		.paletteTag = TAG_BALL_OPEN_LEVEL,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_LUREBALL
		.tileTag = TAG_BALL_OPEN_LURE,
		.paletteTag = TAG_BALL_OPEN_LURE,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_HEAVYBALL
		.tileTag = TAG_BALL_OPEN_HEAVY,
		.paletteTag = TAG_BALL_OPEN_HEAVY,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_LOVEBALL
		.tileTag = TAG_BALL_OPEN_LOVE,
		.paletteTag = TAG_BALL_OPEN_LOVE,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_FRIENDBALL
		.tileTag = TAG_BALL_OPEN_FRIEND,
		.paletteTag = TAG_BALL_OPEN_FRIEND,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_MOONBALL
		.tileTag = TAG_BALL_OPEN_MOON,
		.paletteTag = TAG_BALL_OPEN_MOON,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_SPORTBALL
		.tileTag = TAG_BALL_OPEN_SPORT,
		.paletteTag = TAG_BALL_OPEN_SPORT,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_BEASTBALL
		.tileTag = TAG_BALL_OPEN_BEAST,
		.paletteTag = TAG_BALL_OPEN_BEAST,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_DREAMBALL
		.tileTag = TAG_BALL_OPEN_DREAM,
		.paletteTag = TAG_BALL_OPEN_DREAM,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_STARTBALL
		.tileTag = TAG_BALL_OPEN_START,
		.paletteTag = TAG_BALL_OPEN_START,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
// WATASHI_BALLS_BEGIN ptemplates
	{ //GFX_TAG_MOON_PETAL_BALL
		.tileTag = TAG_BALL_OPEN_MOON_PETAL,
		.paletteTag = TAG_BALL_OPEN_MOON_PETAL,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_MEMORY_VEIL_BALL
		.tileTag = TAG_BALL_OPEN_MEMORY_VEIL,
		.paletteTag = TAG_BALL_OPEN_MEMORY_VEIL,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_TEAR_WISTERIA_BALL
		.tileTag = TAG_BALL_OPEN_TEAR_WISTERIA,
		.paletteTag = TAG_BALL_OPEN_TEAR_WISTERIA,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_SWEET_HEART_BALL
		.tileTag = TAG_BALL_OPEN_SWEET_HEART,
		.paletteTag = TAG_BALL_OPEN_SWEET_HEART,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_SILENT_HALO_BALL
		.tileTag = TAG_BALL_OPEN_SILENT_HALO,
		.paletteTag = TAG_BALL_OPEN_SILENT_HALO,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_GAME_PULSE_BALL
		.tileTag = TAG_BALL_OPEN_GAME_PULSE,
		.paletteTag = TAG_BALL_OPEN_GAME_PULSE,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_WINTER_BLOSSOM_BALL
		.tileTag = TAG_BALL_OPEN_WINTER_BLOSSOM,
		.paletteTag = TAG_BALL_OPEN_WINTER_BLOSSOM,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_THORN_HEART_BALL
		.tileTag = TAG_BALL_OPEN_THORN_HEART,
		.paletteTag = TAG_BALL_OPEN_THORN_HEART,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_FOX_SPIRIT_BALL
		.tileTag = TAG_BALL_OPEN_FOX_SPIRIT,
		.paletteTag = TAG_BALL_OPEN_FOX_SPIRIT,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_BERRY_BLOOM_BALL
		.tileTag = TAG_BALL_OPEN_BERRY_BLOOM,
		.paletteTag = TAG_BALL_OPEN_BERRY_BLOOM,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
// WATASHI_BALLS_END ptemplates
	{ //GFX_TAG_ROCKET_BALL
		.tileTag = TAG_BALL_OPEN_ROCKET,
		.paletteTag = TAG_BALL_OPEN_ROCKET,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_SNATCH_BALL [hack] patches/0961
		.tileTag = TAG_BALL_OPEN_SNATCH,
		.paletteTag = TAG_BALL_OPEN_SNATCH,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
	{ //GFX_TAG_DYNAMAXBALL
		.tileTag = TAG_BALL_OPEN_DYNAMAX,
		.paletteTag = TAG_BALL_OPEN_DYNAMAX,
		.oam = gOamData_AffineOff_ObjNormal_8x8,
		.anims = sAnims_BallParticles,
		.images = NULL,
		.affineAnims = gDummySpriteAffineAnimTable,
		.callback = SpriteCallbackDummy,
	},
};
