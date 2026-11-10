#include "defines.h"
#include "../include/bg.h"
#include "../include/berry.h"
#include "../include/scanline_effect.h"
#include "../include/text_window.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/menu.h"
#include "../include/menu_helpers.h"
#include "../include/palette.h"
#include "../include/sound.h"
#include "../include/m4a.h"
#include "../include/sprite.h"
#include "../include/decompress.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/trig.h"
#include "../include/event_data.h"
#include "../include/new/Vanilla_functions.h"
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"
#include "hack_condition_graph.h"
#include "hack_mon_id.h"
#include "hack_pokeblock_feed.h"

/*
 * ポロックを食べさせる演出画面（2026-11-09 FEAT-pokeblock 段 3b・patches/0978）。
 * 原文: 参考/pokeemerald/src/pokeblock_feed.c（L29-L1193）。日本語版エメラルドの同じ画面（asm/・data/）と同じ動き・同じ文。
 * 原文との差:
 *   - 渡し物は原文のグローバル（gPokeblockMonId・gPokeblockGain・sPokeblock・sExitCallback・gSpecialVar_ItemId）ではなく struct PokeblockCarry（hack_pokeblock_feed.h）。
 *     書き換え可能な static は ROM に置かれる（6.13）ので、状態（sPokeblockFeed）も Alloc した入れ物＋ポインタの語 0x203E038（carry->feed）。
 *   - ポケモンの絵: 原文は gMonSpritesGfxPtr へ読んで gMultiuseSpriteTemplate で作るが、素の FR にその仕組みが無い。あげる画面（hack_condition_graph.c の GetConditionMenuMonGfx）と同じ道
 *     （戦闘と同じ絵の表＋色違い込みのパレット）で入れ物のメンバーへ読み、同じ 64x64 のスプライトを作る。CFRU 追加の種族・Moémon の絵もこの道を通る。
 *   - 向きの反転（原文 IsMonSpriteNotFlipped＝種族ごとの noFlip）: エメラルドの noFlip の 18 種族（gSpeciesInfo の noFlip = TRUE）だけ反転しない。
 *     CFRU が足した種族（番号 > 411）と、Moémon の絵を出している子は反転する（絵の向きが分からないので原文の「ほとんどは反転する」側に合わせた・ユーザー承認済みの設計 案 A）。
 *   - 食べた文のとき、ポロックの味の好き嫌い（gain）は あげる画面が食べさせる前に決めて carry に持つ（原文は文の直前に計算）。同じ値。
 *   - ポケモンの体のアフィン行列: 原文は動きの終わりに FreeOamMatrix で返すが、反転した絵ではその番号を他のスプライト（ケース）が拾うと向きが変わる恐れがあるので返さない
 *     （画面の終わりの ResetSpriteData で全部消える）。見た目は同じ。
 *   - 通信中の待ち（MenuHelpers_ShouldWaitForLinkRecv など）は省く（入口がフィールドのケースだけ）。
 *   - 文字: FONT_NORMAL→素の FR の 2。窓の枠・パレットはあげる画面と同じ素の FR の同等品。
 *   - 原文は演出の終わりで gMain.savedCallback へ戻るが、ここは CB2_PokeblockUseReturn（あげる画面を作り直す・hack_pokeblock_use.c）へ。
 */

#define PBF_FONT 2
#define PBF_ALL_PALETTES 0xFFFFFFFFu
#define BG_SCREEN_SIZE 0x800
#define TAG_POKEBLOCK_CASE 14800
#define TAG_POKEBLOCK 14801
#define MON_X 48
#define MON_Y 80
#define MON_FLIP_FLAG_MOEMON_ON 0x12E9     //Moémon の絵を出す旗（src/hack_moemon_switch.c の HACK_FLAG_MOEMON_ON）
#define PBF_NUM_NATURES 25

//素の FR（JP）の番地（BPRJ.ld に無いもの・あげる画面／ケースと同じ。tools/rom/verify_pokeblock_case.py が確かめる）
#define Pbf_SetVBlankHBlankCallbacksToNull ((void (*)(void))(0x080C0938 + 1))

enum {
	ANIMDATA_ROT_IDX,
	ANIMDATA_ROT_SPEED,
	ANIMDATA_SIN_AMPLITUDE,
	ANIMDATA_COS_AMPLITUDE,
	ANIMDATA_TIME,
	ANIMDATA_ROT_ACCEL,
	ANIMDATA_TARGET_X,
	ANIMDATA_TARGET_Y,
	ANIMDATA_APPR_TIME,
	ANIMDATA_IS_LAST,
	NUM_ANIMDATA
};

enum {
	AFFINE_NONE,
	AFFINE_TURN_UP,
	AFFINE_TURN_UP_AND_DOWN,
	AFFINE_TURN_DOWN,
	AFFINE_TURN_DOWN_SLOW,
	AFFINE_TURN_DOWN_SLIGHT,
	AFFINE_TURN_UP_HIGH,
	AFFINE_UNUSED_1,
	AFFINE_UNUSED_2,
	AFFINE_UNUSED_3,
	NUM_MON_AFFINES,
};

// せいかくごとの動き（sMonPokeblockAnims の先頭の番号）。原文のとおり
#define ANIM_HARDY   0
#define ANIM_LONELY  (ANIM_HARDY + 3)
#define ANIM_BRAVE   (ANIM_LONELY + 1)
#define ANIM_ADAMANT (ANIM_BRAVE + 1)
#define ANIM_NAUGHTY (ANIM_ADAMANT + 5)
#define ANIM_BOLD    (ANIM_NAUGHTY + 3)
#define ANIM_DOCILE  (ANIM_BOLD + 2)
#define ANIM_RELAXED (ANIM_DOCILE + 1)
#define ANIM_IMPISH  (ANIM_RELAXED + 2)
#define ANIM_LAX     (ANIM_IMPISH + 1)
#define ANIM_TIMID   (ANIM_LAX + 1)
#define ANIM_HASTY   (ANIM_TIMID + 5)
#define ANIM_SERIOUS (ANIM_HASTY + 2)
#define ANIM_JOLLY   (ANIM_SERIOUS + 1)
#define ANIM_NAIVE   (ANIM_JOLLY + 1)
#define ANIM_MODEST  (ANIM_NAIVE + 4)
#define ANIM_MILD    (ANIM_MODEST + 3)
#define ANIM_QUIET   (ANIM_MILD + 1)
#define ANIM_BASHFUL (ANIM_QUIET + 2)
#define ANIM_RASH    (ANIM_BASHFUL + 3)
#define ANIM_CALM    (ANIM_RASH + 3)
#define ANIM_GENTLE  (ANIM_CALM + 1)
#define ANIM_SASSY   (ANIM_GENTLE + 1)
#define ANIM_CAREFUL (ANIM_SASSY + 1)
#define ANIM_QUIRKY  (ANIM_CAREFUL + 5)

struct PokeblockFeed
{
	struct Sprite* monSpritePtr;
	u8 tilemapBuffer[BG_SCREEN_SIZE];
	s16 monAnimX[0x200];
	s16 monAnimY[0x200];
	u8 monGfx[MON_PIC_SIZE];       //原文は gMonSpritesGfxPtr 側。ここは入れ物の中
	u16 monPal[16];
	u8 animRunState;
	u8 animId;
	bool8 noMonFlip;               //TRUE = 絵がもともと右向き（反転しない）
	u16 species;
	u16 monAnimLength;
	u16 timer;
	u8 nature;
	u8 monSpriteId;
	u8 pokeblockCaseSpriteId;
	u8 pokeblockSpriteId;
	s16 animData[NUM_ANIMDATA];
	s16 monInitX;
	s16 monInitY;
	s16 maxAnimStageTime;
	s16 monX;
	s16 monY;
	s16 loadGfxState;
};

#define sCarry (*(struct PokeblockCarry**) PBF_STATE_PTR)
#define sPokeblockFeed ((struct PokeblockFeed*) sCarry->feed)

extern const u8 gPbfPokeblock_Gfx[], gPbfPokeblock_Pals[], gPbfFeedBg_Tilemap[], gPbfBuilding_Tiles[], gPbfFrontier_Pal[];
extern const u8 gText_Pbf_Ate[], gText_Pbf_HappilyAte[], gText_Pbf_DisdainfullyAte[];
#define gMonFrontPicTable ((const struct CompressedSpriteSheet*) *((u32*) 0x8000128))

void TransferPlttBuffer(void);                 //src/dns.c

static void HandleInitBackgrounds(void);
static void HandleInitWindows(void);
static void LaunchPokeblockFeedTask(void);
static void SetPokeblockSpritePal(void);
static void CalculateMonAnimLength(void);
static void DoPokeblockCaseThrowEffect(u8, bool8);
static void StartMonJumpForPokeblock(u8);
static void Task_PrintAtePokeblockMessage(u8);
static void Task_FadeOutPokeblockFeed(u8);
static void UpdateMonAnim(void);
static void SpriteCB_MonJumpForPokeblock(struct Sprite*);
static void CalculateMonAnimMovement(void);
static void CalculateMonAnimMovementEnd(void);
static bool8 InitMonAnimStage(void);
static bool8 DoMonAnimStep(void);
static bool8 LoadMonAndSceneGfx(struct Pokemon*);
static u8 CreatePokeblockSprite(void);
static u8 CreatePokeblockCaseSpriteForFeeding(void);
static u8 CreateMonSprite(struct Pokemon*);
static void SpriteCB_ThrownPokeblock(struct Sprite*);

//原文 Sin／Cos（素の FR の gSineTable は 256＋64 個で同じ形）
static s16 PbfSin(s16 index, s16 amplitude)
{
	return (amplitude * gSineTable[index]) >> 8;
}

static s16 PbfCos(s16 index, s16 amplitude)
{
	return (amplitude * gSineTable[index + 64]) >> 8;
}

// エメラルドの noFlip（gSpeciesInfo の noFlip = TRUE）の 18 種族＝絵がもともと右向き。原文の IsMonSpriteNotFlipped
static const u16 sNoFlipSpecies[] =
{
	SPECIES_POLIWAG, SPECIES_POLIWHIRL, SPECIES_POLIWRATH, SPECIES_KINGLER, SPECIES_ELECTABUZZ, SPECIES_CROCONAW,
	SPECIES_IGGLYBUFF, SPECIES_POLITOED, SPECIES_UNOWN, SPECIES_SNEASEL, SPECIES_TEDDIURSA, SPECIES_ELEKID,
	SPECIES_SPINDA, SPECIES_ROSELIA, SPECIES_ABSOL, SPECIES_SEVIPER, SPECIES_ZANGOOSE, SPECIES_DEOXYS,
};

static bool8 IsMonSpriteNotFlipped(struct Pokemon* mon, u16 species)
{
	u32 i;

	//Moémon の絵を出している子は反転する（絵の向きが違う・設計の案 A）
	if (FlagGet(MON_FLIP_FLAG_MOEMON_ON) && HackMonId_IsMoemonHit((struct BoxPokemon*) mon))
		return FALSE;
	for (i = 0; i < ARRAY_COUNT(sNoFlipSpecies); i++)
	{
		if (sNoFlipSpecies[i] == species)
			return TRUE;
	}
	return FALSE;                                   //CFRU が足した種族もここ（反転する）
}

static const u8 sNatureToMonPokeblockAnim[PBF_NUM_NATURES][2] =
{
	[0]  = { ANIM_HARDY,   AFFINE_NONE },                    //NATURE_HARDY
	[1]  = { ANIM_LONELY,  AFFINE_NONE },                    //LONELY
	[2]  = { ANIM_BRAVE,   AFFINE_TURN_UP },                 //BRAVE
	[3]  = { ANIM_ADAMANT, AFFINE_NONE },                    //ADAMANT
	[4]  = { ANIM_NAUGHTY, AFFINE_NONE },                    //NAUGHTY
	[5]  = { ANIM_BOLD,    AFFINE_NONE },                    //BOLD
	[6]  = { ANIM_DOCILE,  AFFINE_NONE },                    //DOCILE
	[7]  = { ANIM_RELAXED, AFFINE_TURN_UP_AND_DOWN },        //RELAXED
	[8]  = { ANIM_IMPISH,  AFFINE_NONE },                    //IMPISH
	[9]  = { ANIM_LAX,     AFFINE_NONE },                    //LAX
	[10] = { ANIM_TIMID,   AFFINE_NONE },                    //TIMID
	[11] = { ANIM_HASTY,   AFFINE_NONE },                    //HASTY
	[12] = { ANIM_SERIOUS, AFFINE_TURN_DOWN },               //SERIOUS
	[13] = { ANIM_JOLLY,   AFFINE_NONE },                    //JOLLY
	[14] = { ANIM_NAIVE,   AFFINE_NONE },                    //NAIVE
	[15] = { ANIM_MODEST,  AFFINE_TURN_DOWN_SLOW },          //MODEST
	[16] = { ANIM_MILD,    AFFINE_NONE },                    //MILD
	[17] = { ANIM_QUIET,   AFFINE_NONE },                    //QUIET
	[18] = { ANIM_BASHFUL, AFFINE_NONE },                    //BASHFUL
	[19] = { ANIM_RASH,    AFFINE_NONE },                    //RASH
	[20] = { ANIM_CALM,    AFFINE_NONE },                    //CALM
	[21] = { ANIM_GENTLE,  AFFINE_TURN_DOWN_SLIGHT },        //GENTLE
	[22] = { ANIM_SASSY,   AFFINE_TURN_UP_HIGH },            //SASSY
	[23] = { ANIM_CAREFUL, AFFINE_NONE },                    //CAREFUL
	[24] = { ANIM_QUIRKY,  AFFINE_NONE },                    //QUIRKY
};

// 構えているときの動き（せいかくごとに最大 8 段・原文のとおり）。各列の意味は原文のコメント
//  ROT_IDX（sin／cos 表の位置）・ROT_SPEED・SIN_AMPLITUDE（x の振れ）・COS_AMPLITUDE（y の振れ）・TIME（フレーム数）・ROT_ACCEL・TARGET_X・TARGET_Y・APPR_TIME・IS_LAST
static const s16 sMonPokeblockAnims[][NUM_ANIMDATA] =
{
	[ANIM_HARDY] =
	{   0,   4,   0,   8,  24,   0,   0,   0,  12,   FALSE},
	{   0,   4,   0,  16,  24,   0,   0,   0,  12,   FALSE},
	{   0,   4,   0,  32,  32,   0,   0,   0,  16,   TRUE},

	[ANIM_LONELY] =
	{   0,   3,   6,   0,  48,   0,   0,   0,  24,   TRUE},

	[ANIM_BRAVE] =
	{  64,  16, -24,   0,  32,   0,   0,   0,   0,   TRUE},

	[ANIM_ADAMANT] =
	{   0,   4,   8,   0,  16,   0,  -8,   0,   0,   FALSE},
	{   0,   0,   0,   0,  16,   0,   0,   0,   0,   FALSE},
	{   0,   4,   8,   0,  16,   0,  -8,   0,   0,   FALSE},
	{   0,   0,   0,   0,  16,   0,   0,   0,   0,   FALSE},
	{   0,   4, -16,   0,   4,   0,  16,   0,   0,   TRUE},

	[ANIM_NAUGHTY] =
	{   0,   3,   6,   0,  12,   0,   0,   0,   6,   FALSE},
	{   0,   3,  -6,   0,  12,   0,   0,   0,   6,   FALSE},
	{   0,  16,  16,   0,  45,   1,   0,   0,   0,   TRUE},

	[ANIM_BOLD] =
	{   0,  16,   0,  24,  32,   0,   0,   0,  16,   FALSE},
	{   0,  16,   0,  23,  32,   0,   0,   0,  16,   TRUE},

	[ANIM_DOCILE] =
	{   0,   0,   0,   0,  80,   0,   0,   0,   0,   TRUE},

	[ANIM_RELAXED] =
	{   0,   2,   8,   0,  32,   0,   0,   0,   0,   FALSE},
	{   0,   2,  -8,   0,  32,   0,   0,   0,   0,   TRUE},

	[ANIM_IMPISH] =
	{   0,  32,   2,   1,  48,   1,   0,   0,  24,   TRUE},

	[ANIM_LAX] =
	{   0,   2,  16,  16, 128,   0,   0,   0,   0,   TRUE},

	[ANIM_TIMID] =
	{   0,   2,  -8,   0,  48,   0, -24,   0,   0,   FALSE},
	{   0,   0,   0,   0,   8,   0,   0,   0,   0,   FALSE},
	{  64,  32,   2,   0,  36,   0,   0,   0,   0,   FALSE},
	{   0,   0,   0,   0,   8,   0,   0,   0,   0,   FALSE},
	{   0,   2,   8,   0,  48,   0,  24,   0,   0,   TRUE},

	[ANIM_HASTY] =
	{  64,  24,  16,   0,  32,   0,   0,   0,   0,   FALSE},
	{   0,  28,   2,   1,  32,   1,   0,   0,  16,   TRUE},

	[ANIM_SERIOUS] =
	{   0,   0,   0,   0,  32,   0,   0,   0,   0,   TRUE},

	[ANIM_JOLLY] =
	{  64,  16, -16,   2,  48,   0,   0,   0,  32,   TRUE},

	[ANIM_NAIVE] =
	{   0,  12,  -8,   4,  24,   0,   8,   0,  12,   FALSE},
	{   0,  12,   8,   8,  24,   0, -16,   0,  12,   FALSE},
	{   0,  12,  -8,  16,  24,   0,  16,   0,  12,   FALSE},
	{   0,  12,   8,  28,  24,   0,  -8,   0,  12,   TRUE},

	[ANIM_MODEST] =
	{   0,   0,   0,   0,   8,   0,   0,   0,   0,   FALSE},
	{  64,  16,  -4,   0,  32,   0,   0,   0,   0,   FALSE},
	{   0,   0,   0,   0,   8,   0,   0,   0,   0,   TRUE},

	[ANIM_MILD] =
	{ 128,   4,   0,   8,  64,   0,   0,   0,   0,   TRUE},

	[ANIM_QUIET] =
	{   0,   2,  16,   0,  48,   0,   0,   0,   0,   FALSE},
	{ 128,   2,  16,   0,  48,   0,   0,   0,   0,   TRUE},

	[ANIM_BASHFUL] =
	{   0,   2,  -4,   0,  48,   0, -48,   0,   0,   FALSE},
	{   0,   0,   0,   0,  80,   0,   0,   0,   0,   FALSE},
	{   0,   2,   8,   0,  24,   0,  48,   0,   0,   TRUE},

	[ANIM_RASH] =
	{  64,   4,  64,  58,  52,   0, -88,   0,   0,   FALSE},
	{   0,   0,   0,   0,  80,   0,   0,   0,   0,   FALSE},
	{   0,  24,  80,   0,  32,   0,  88,   0,   0,   TRUE},

	[ANIM_CALM] =
	{   0,   2,  16,   4,  64,   0,   0,   0,   0,   TRUE},

	[ANIM_GENTLE] =
	{   0,   0,   0,   0,  32,   0,   0,   0,   0,   TRUE},

	[ANIM_SASSY] =
	{   0,   0,   0,   0,  42,   0,   0,   0,   0,   TRUE},

	[ANIM_CAREFUL] =
	{   0,   4,   0,   8,  24,   0,   0,   0,  12,   FALSE},
	{   0,   0,   0,   0,  12,   0,   0,   0,   0,   FALSE},
	{   0,   4,   0,  12,  24,   0,   0,   0,  12,   FALSE},
	{   0,   0,   0,   0,  12,   0,   0,   0,   0,   FALSE},
	{   0,   4,   0,   4,  24,   0,   0,   0,  12,   TRUE},

	[ANIM_QUIRKY] =
	{   0,   4,  16,  12,  64,   0,   0,   0,   0,   FALSE},
	{   0,  -4,  16,  12,  64,   0,   0,   0,   0,   TRUE},
};

static const union AffineAnimCmd sAffineAnim_Mon_None[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnUp[] =
{
	AFFINEANIMCMD_FRAME(0, 0, 12, 1),
	AFFINEANIMCMD_FRAME(0, 0, 0, 30),
	AFFINEANIMCMD_FRAME(0, 0, -12, 1),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnUp_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0, 0, 12, 1),
	AFFINEANIMCMD_FRAME(0, 0, 0, 28),
	AFFINEANIMCMD_FRAME(0, 0, -4, 3),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnUpAndDown[] =
{
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 16),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 32),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 16),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnUpAndDown_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 16),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 32),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 16),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnDown[] =
{
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 8),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 16),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 8),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnDown_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 8),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 16),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 8),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnDownSlow[] =
{
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 8),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 32),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 8),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnDownSlow_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 8),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 32),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 8),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnDownSlight[] =
{
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 4),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 24),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 4),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnDownSlight_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -1, 4),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 24),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 4),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnUpHigh[] =
{
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 24),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 16),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -12, 2),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_Mon_TurnUpHigh_Flipped[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 1, 24),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 16),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -12, 2),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd* const sAffineAnims_Mon[] =
{
	// 反転しない絵の動き
	[AFFINE_NONE]             = sAffineAnim_Mon_None,
	[AFFINE_TURN_UP]          = sAffineAnim_Mon_TurnUp,
	[AFFINE_TURN_UP_AND_DOWN] = sAffineAnim_Mon_TurnUpAndDown,
	[AFFINE_TURN_DOWN]        = sAffineAnim_Mon_TurnDown,
	[AFFINE_TURN_DOWN_SLOW]   = sAffineAnim_Mon_TurnDownSlow,
	[AFFINE_TURN_DOWN_SLIGHT] = sAffineAnim_Mon_TurnDownSlight,
	[AFFINE_TURN_UP_HIGH]     = sAffineAnim_Mon_TurnUpHigh,
	[AFFINE_UNUSED_1]         = sAffineAnim_Mon_None,
	[AFFINE_UNUSED_2]         = sAffineAnim_Mon_None,
	[AFFINE_UNUSED_3]         = sAffineAnim_Mon_None,

	// 反転する絵の動き
	[AFFINE_NONE + NUM_MON_AFFINES]             = sAffineAnim_Mon_None,
	[AFFINE_TURN_UP + NUM_MON_AFFINES]          = sAffineAnim_Mon_TurnUp_Flipped,
	[AFFINE_TURN_UP_AND_DOWN + NUM_MON_AFFINES] = sAffineAnim_Mon_TurnUpAndDown_Flipped,
	[AFFINE_TURN_DOWN + NUM_MON_AFFINES]        = sAffineAnim_Mon_TurnDown_Flipped,
	[AFFINE_TURN_DOWN_SLOW + NUM_MON_AFFINES]   = sAffineAnim_Mon_TurnDownSlow_Flipped,
	[AFFINE_TURN_DOWN_SLIGHT + NUM_MON_AFFINES] = sAffineAnim_Mon_TurnDownSlight_Flipped,
	[AFFINE_TURN_UP_HIGH + NUM_MON_AFFINES]     = sAffineAnim_Mon_TurnUpHigh_Flipped,
	[AFFINE_UNUSED_1 + NUM_MON_AFFINES]         = sAffineAnim_Mon_None,
	[AFFINE_UNUSED_2 + NUM_MON_AFFINES]         = sAffineAnim_Mon_None,
	[AFFINE_UNUSED_3 + NUM_MON_AFFINES]         = sAffineAnim_Mon_None,

	sAffineAnim_Mon_None, // 原文の余り 1 つ
};

static const struct BgTemplate sBackgroundTemplates[] =
{
	{
		.bg = 0,
		.charBaseIndex = 0,
		.mapBaseIndex = 31,
		.screenSize = 0,
		.paletteMode = 0,
		.priority = 0,
		.baseTile = 0
	},
	{
		.bg = 1,
		.charBaseIndex = 2,
		.mapBaseIndex = 30,
		.screenSize = 0,
		.paletteMode = 0,
		.priority = 3,
		.baseTile = 0
	}
};

static const struct WindowTemplate sWindowTemplates[] =
{
	{
		.bg = 0,
		.tilemapLeft = 1,
		.tilemapTop = 15,
		.width = 28,
		.height = 4,
		.paletteNum = 15,
		.baseBlock = 0xA
	},
	DUMMY_WIN_TEMPLATE
};

static const union AffineAnimCmd sAffineAnim_Still[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd* const sSpriteAffineAnimTable_MonNoFlip[] =
{
	sAffineAnim_Still
};

static const union AffineAnimCmd sAffineAnim_PokeblockCase_ThrowFromVertical[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 8),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 16, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 16, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 16, 1),
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd sAffineAnim_PokeblockCase_ThrowFromHorizontal[] =
{
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, 0, 8),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -16, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -16, 1),
	AFFINEANIMCMD_FRAME(0x0, 0x0, -16, 1),
	AFFINEANIMCMD_FRAME(-0x100, 0x100, 0, 0),
	AFFINEANIMCMD_END
};

static const union AffineAnimCmd* const sAffineAnims_PokeblockCase_Still[] =
{
	sAffineAnim_Still
};

static const union AffineAnimCmd* const sAffineAnims_PokeblockCase_ThrowFromVertical[] =
{
	sAffineAnim_PokeblockCase_ThrowFromVertical
};

static const union AffineAnimCmd* const sAffineAnims_PokeblockCase_ThrowFromHorizontal[] =
{
	sAffineAnim_PokeblockCase_ThrowFromHorizontal
};

static const struct OamData sOamData_Pokeblock =
{
	.y = 0,
	.affineMode = ST_OAM_AFFINE_DOUBLE,
	.objMode = ST_OAM_OBJ_NORMAL,
	.mosaic = FALSE,
	.bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(8x8),
	.x = 0,
	.matrixNum = 0,
	.size = SPRITE_SIZE(8x8),
	.tileNum = 0,
	.priority = 1,
	.paletteNum = 0,
	.affineParam = 0,
};

static const union AnimCmd sAnim_Pokeblock[] =
{
	ANIMCMD_FRAME(0, 0),
	ANIMCMD_END
};

static const union AnimCmd* const sAnims_Pokeblock[] =
{
	sAnim_Pokeblock,
};

static const union AffineAnimCmd sAffineAnim_Pokeblock[] =
{
	AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
	AFFINEANIMCMD_FRAME(-8, -8, 0, 1),
	AFFINEANIMCMD_JUMP(1)
};

static const union AffineAnimCmd* const sAffineAnims_Pokeblock[] =
{
	sAffineAnim_Pokeblock
};

static const struct SpriteSheet sSpriteSheet_Pokeblock =
{
	gPbfPokeblock_Gfx, 0x20, TAG_POKEBLOCK
};

static const struct SpriteTemplate sSpriteTemplate_Pokeblock =
{
	.tileTag = TAG_POKEBLOCK,
	.paletteTag = TAG_POKEBLOCK,
	.oam = &sOamData_Pokeblock,
	.anims = sAnims_Pokeblock,
	.images = NULL,
	.affineAnims = sAffineAnims_Pokeblock,
	.callback = SpriteCB_ThrownPokeblock
};

static void CB2_PokeblockFeed(void)
{
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	DoScheduledBgTilemapCopiesToVram();
	UpdatePaletteFade();
}

static void VBlankCB_PokeblockFeed(void)
{
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

static bool8 LoadPokeblockFeedScene(void)
{
	switch (gMain.state)
	{
	case 0:
		sCarry->feed = AllocZeroed(sizeof(struct PokeblockFeed));
		Pbf_SetVBlankHBlankCallbacksToNull();
		clear_scheduled_bg_copies_to_vram();
		gMain.state++;
		break;
	case 1:
		ScanlineEffect_Stop();                   //あげる画面のゲージの HBlank 転送を止めておく（素の FR の画面の入り方・ケースと同じ）
		ResetPaletteFade();
		gPaletteFade->bufferTransferDisabled = TRUE;
		gMain.state++;
		break;
	case 2:
		ResetSpriteData();
		ResetTasks();
		gMain.state++;
		break;
	case 3:
		FreeAllSpritePalettes();
		gMain.state++;
		break;
	case 4:
		gMain.state++;                           //原文の AllocateMonSpritesGfx（絵の入れ物）は入れ物のメンバーなので要らない
		break;
	case 5:
		HandleInitBackgrounds();
		gMain.state++;
		break;
	case 6:
		HandleInitWindows();
		gMain.state++;
		break;
	case 7:
		if (LoadMonAndSceneGfx(&gPlayerParty[sCarry->partyId]))
			gMain.state++;
		break;
	case 8:
		sPokeblockFeed->pokeblockCaseSpriteId = CreatePokeblockCaseSpriteForFeeding();
		gMain.state++;
		break;
	case 9:
		sPokeblockFeed->monSpriteId = CreateMonSprite(&gPlayerParty[sCarry->partyId]);
		gMain.state++;
		break;
	case 10:
		DrawTextBorderOuter(0, 1, 14);           //原文 DrawStdFrameWithCustomTileAndPalette(0, TRUE, 1, 14)
		ScheduleBgCopyTilemapToVram(0);
		gMain.state++;
		break;
	case 11:
		LaunchPokeblockFeedTask();
		gMain.state++;
		break;
	case 12:
		BlendPalettes(PBF_ALL_PALETTES, 16, 0);
		gMain.state++;
		break;
	case 13:
		BeginNormalPaletteFade(PBF_ALL_PALETTES, 0, 16, 0, 0);
		gPaletteFade->bufferTransferDisabled = FALSE;
		gMain.state++;
		break;
	default:
		SetVBlankCallback(VBlankCB_PokeblockFeed);
		SetMainCallback2(CB2_PokeblockFeed);
		return TRUE;
	}
	return FALSE;
}

static void CB2_PokeblockFeedInit(void)
{
	while (1)
	{
		if (LoadPokeblockFeedScene() == TRUE)
			break;
	}
}

// あげる画面から（carry を語に置いたあと）。原文 PreparePokeblockFeedScene
void PokeblockFeed_Start(void)
{
	SetMainCallback2(CB2_PokeblockFeedInit);        //gMain.state は 0 に戻る
}

static void HandleInitBackgrounds(void)
{
	ResetVramOamAndBgCntRegs();
	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sBackgroundTemplates, ARRAY_COUNT(sBackgroundTemplates));
	SetBgTilemapBuffer(1, sPokeblockFeed->tilemapBuffer);
	ResetBgPositions();
	ScheduleBgCopyTilemapToVram(1);
	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
	ShowBg(0);
	ShowBg(1);
	SetGpuReg(REG_OFFSET_BLDCNT, 0);
}

static bool8 LoadMonAndSceneGfx(struct Pokemon* mon)
{
	u16 species;
	u32 personality, trainerId;

	switch (sPokeblockFeed->loadGfxState)
	{
	case 0:
		// ポケモンの絵（あげる画面と同じ道・戦闘と同じ絵の表）
		species = GetMonData(mon, MON_DATA_SPECIES2, NULL);
		personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
		HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species], sPokeblockFeed->monGfx, species, personality);
		sPokeblockFeed->loadGfxState++;
		break;
	case 1:
		// ポケモンのパレット（色違いも戦闘と同じ）
		species = GetMonData(mon, MON_DATA_SPECIES2, NULL);
		personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
		trainerId = GetMonData(mon, MON_DATA_OT_ID, NULL);
		LZ77UnCompWram(GetMonSpritePalFromSpeciesAndPersonality(species, trainerId, personality), sPokeblockFeed->monPal);
		sPokeblockFeed->loadGfxState++;
		break;
	case 2:
		PokeblockCase_LoadSpriteGfx();           //ケースの絵＋パレット（原文は 2 段・ここは 1 つ）
		sPokeblockFeed->loadGfxState++;
		break;
	case 3:
		LoadSpriteSheet(&sSpriteSheet_Pokeblock);
		sPokeblockFeed->loadGfxState++;
		break;
	case 4:
		SetPokeblockSpritePal();
		sPokeblockFeed->loadGfxState++;
		break;
	case 5:
		ResetTempTileDataBuffers();
		decompress_and_copy_tile_data_to_vram(1, gPbfBuilding_Tiles, 0, 0, 0);
		sPokeblockFeed->loadGfxState++;
		break;
	case 6:
		if (free_temp_tile_data_buffers_if_possible() != TRUE)
		{
			LZDecompressWram(gPbfFeedBg_Tilemap, sPokeblockFeed->tilemapBuffer);
			sPokeblockFeed->loadGfxState++;
		}
		break;
	case 7:
		LoadPalette(gPbfFrontier_Pal, 2 * 16, 3 * 32);
		sPokeblockFeed->loadGfxState = 0;
		return TRUE;
	}

	return FALSE;
}

static void HandleInitWindows(void)
{
	InitWindows(sWindowTemplates);
	DeactivateAllTextPrinters();
	TextWindow_SetStdFrame0_WithPal(0, 1, 14 * 16);              //原文 LoadUserWindowBorderGfx(0, 1, BG_PLTT_ID(14))
	Menu_LoadStdPalAt(15 * 16);                                  //原文 LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP)
	FillWindowPixelBuffer(0, 0);
	PutWindowTilemap(0);
	ScheduleBgCopyTilemapToVram(0);
}

// ポロックの色のパレット（原文は sPokeblocksPals[色 - 1]・色 1〜14）
static void SetPokeblockSpritePal(void)
{
	struct SpritePalette pal;
	u8 color = sCarry->pokeblock.color;

	if (color < 1 || color > 14)
		color = 1;
	pal.data = (const u16*) (gPbfPokeblock_Pals + (color - 1) * 32);
	pal.tag = TAG_POKEBLOCK;
	LoadSpritePalette(&pal);
}

// タスクのデータ
#define tState            data[0]
#define tHorizontalThrow  data[1]

#define STATE_START_THROW  255 // 動きが 255 を超えると投げが 2 回になりうる（原文のまま）
#define STATE_SPAWN_PBLOCK (STATE_START_THROW + 14)
#define STATE_START_JUMP   (STATE_SPAWN_PBLOCK + 12)
#define STATE_PRINT_MSG    (STATE_START_JUMP + 16)

static void Task_HandlePokeblockFeed(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		switch (gTasks[taskId].tState)
		{
		case 0:
			sPokeblockFeed->animRunState = 0;
			sPokeblockFeed->timer = 0;
			CalculateMonAnimLength();
			break;
		case STATE_START_THROW:
			DoPokeblockCaseThrowEffect(sPokeblockFeed->pokeblockCaseSpriteId, gTasks[taskId].tHorizontalThrow);
			break;
		case STATE_SPAWN_PBLOCK:
			sPokeblockFeed->pokeblockSpriteId = CreatePokeblockSprite();
			break;
		case STATE_START_JUMP:
			StartMonJumpForPokeblock(sPokeblockFeed->monSpriteId);
			break;
		case STATE_PRINT_MSG:
			gTasks[taskId].func = Task_PrintAtePokeblockMessage;
			return;
		}

		if (sPokeblockFeed->timer < sPokeblockFeed->monAnimLength)
			UpdateMonAnim();
		else if (sPokeblockFeed->timer == sPokeblockFeed->monAnimLength)
			gTasks[taskId].tState = STATE_START_THROW - 1;

		sPokeblockFeed->timer++;
		gTasks[taskId].tState++;
	}
}

static void LaunchPokeblockFeedTask(void)
{
	u8 taskId = CreateTask(Task_HandlePokeblockFeed, 0);
	gTasks[taskId].tState = 0;
	gTasks[taskId].tHorizontalThrow = TRUE;
}

static void Task_WaitForAtePokeblockMessage(u8 taskId)
{
	if (IsTextPrinterActive(0) != TRUE)
		gTasks[taskId].func = Task_FadeOutPokeblockFeed;
}

static void Task_PrintAtePokeblockMessage(u8 taskId)
{
	struct Pokemon* mon = &gPlayerParty[sCarry->partyId];

	GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
	PokeblockCopyName(&sCarry->pokeblock, gStringVar2);

	if (sCarry->gain == 0)
		StringExpandPlaceholders(gStringVar4, gText_Pbf_Ate);
	else if (sCarry->gain > 0)
		StringExpandPlaceholders(gStringVar4, gText_Pbf_HappilyAte);
	else
		StringExpandPlaceholders(gStringVar4, gText_Pbf_DisdainfullyAte);

	gTextFlags.canABSpeedUpPrint = TRUE;
	FillWindowPixelBuffer(0, 17);                                //あげる画面の文と同じ窓の作り（背景 17）
	AddTextPrinterParameterized(0, PBF_FONT, gStringVar4, 0, 1, GetPlayerTextSpeedDelay(), NULL);
	PutWindowTilemap(0);
	CopyWindowToVram(0, 3);
	gTasks[taskId].func = Task_WaitForAtePokeblockMessage;
}

static void Task_ExitPokeblockFeed(u8 taskId)
{
	if (!gPaletteFade->active)
	{
		ResetSpriteData();
		FreeAllSpritePalettes();
		m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x100);   //鳴き声で下げた曲の音量を戻す（原文 TRACKS_ALL）
		SetMainCallback2(CB2_PokeblockUseReturn);
		DestroyTask(taskId);
		FreeAllWindowBuffers();
		Free(sCarry->feed);
		sCarry->feed = NULL;
	}
}

static void Task_FadeOutPokeblockFeed(u8 taskId)
{
	BeginNormalPaletteFade(PBF_ALL_PALETTES, 0, 0, 16, 0);
	gTasks[taskId].func = Task_ExitPokeblockFeed;
}

#undef tState
#undef tHorizontalThrow

// ポロックとポケモンのスプライトのデータ
#define sSpeed   data[0]
#define sAccel   data[1]
#define sSpecies data[2]

static u8 CreateMonSprite(struct Pokemon* mon)
{
	struct SpriteSheet sheet;
	struct SpriteTemplate tmpl;
	struct SpritePalette pal;
	struct Sprite* sprite;
	u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);
	u8 spriteId;

	//原文の gMultiuseSpriteTemplate の代わり（あげる画面と同じ 64x64 のスプライトの型）
	LoadConditionMonPicTemplate(&sheet, &tmpl, &pal);
	sheet.data = sPokeblockFeed->monGfx;
	pal.data = sPokeblockFeed->monPal;
	LoadSpriteSheet(&sheet);
	LoadSpritePalette(&pal);
	spriteId = CreateSprite(&tmpl, MON_X, MON_Y, 2);
	sprite = &gSprites[spriteId];

	sPokeblockFeed->species = species;
	sPokeblockFeed->nature = GetNature(mon);
	sprite->sSpecies = species;
	sprite->callback = SpriteCallbackDummy;

	sPokeblockFeed->noMonFlip = TRUE;
	if (!IsMonSpriteNotFlipped(mon, species))
	{
		sprite->affineAnims = sSpriteAffineAnimTable_MonNoFlip;
		sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
		InitSpriteAffineAnim(sprite);                            //反転の行列を確実に作る（原文は作成時に済む）
		CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
		sPokeblockFeed->noMonFlip = FALSE;
	}

	return spriteId;
}

static void StartMonJumpForPokeblock(u8 spriteId)
{
	gSprites[spriteId].pos1.x = MON_X;
	gSprites[spriteId].pos1.y = MON_Y;
	gSprites[spriteId].sSpeed = -8;
	gSprites[spriteId].sAccel = 1;
	gSprites[spriteId].callback = SpriteCB_MonJumpForPokeblock;
}

static void SpriteCB_MonJumpForPokeblock(struct Sprite* sprite)
{
	sprite->pos1.x += 4;
	sprite->pos1.y += sprite->sSpeed;
	sprite->sSpeed += sprite->sAccel;

	// とびあがった頂点で鳴く
	if (sprite->sSpeed == 0)
		PlayCry1(sprite->sSpecies, 0);

	if (sprite->sSpeed == 9)
		sprite->callback = SpriteCallbackDummy;
}

static u8 CreatePokeblockCaseSpriteForFeeding(void)
{
	u8 spriteId = CreatePokeblockCaseSprite(188, 100, 2);
	gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
	gSprites[spriteId].affineAnims = sAffineAnims_PokeblockCase_Still;
	gSprites[spriteId].callback = SpriteCallbackDummy;
	InitSpriteAffineAnim(&gSprites[spriteId]);
	return spriteId;
}

static void DoPokeblockCaseThrowEffect(u8 spriteId, bool8 horizontalThrow)
{
	FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
	gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;

	if (!horizontalThrow)
		gSprites[spriteId].affineAnims = sAffineAnims_PokeblockCase_ThrowFromVertical; // 使われない（原文でも horizontalThrow は常に TRUE）
	else
		gSprites[spriteId].affineAnims = sAffineAnims_PokeblockCase_ThrowFromHorizontal;

	InitSpriteAffineAnim(&gSprites[spriteId]);
}

static u8 CreatePokeblockSprite(void)
{
	u8 spriteId = CreateSprite(&sSpriteTemplate_Pokeblock, 174, 84, 1);
	gSprites[spriteId].sSpeed = -12;
	gSprites[spriteId].sAccel = 1;
	return spriteId;
}

static void SpriteCB_ThrownPokeblock(struct Sprite* sprite)
{
	sprite->pos1.x -= 4;
	sprite->pos1.y += sprite->sSpeed;
	sprite->sSpeed += sprite->sAccel;
	if (sprite->sSpeed == 10)
		DestroySprite(sprite);
}

static void CalculateMonAnimLength(void)
{
	u8 animId, i;
	struct PokeblockFeed* pokeblockFeed;

	pokeblockFeed = sPokeblockFeed;
	pokeblockFeed->monAnimLength = 1;
	animId = sNatureToMonPokeblockAnim[pokeblockFeed->nature][0];

	// 動きの各段の時間を足す
	for (i = 0; i < 8; i++, animId++)
	{
		pokeblockFeed->monAnimLength += sMonPokeblockAnims[animId][ANIMDATA_TIME];
		if (sMonPokeblockAnims[animId][ANIMDATA_IS_LAST] == TRUE)
			break;
	}
}

static void UpdateMonAnim(void)
{
	struct PokeblockFeed* pokeblockFeed = sPokeblockFeed;

	switch (pokeblockFeed->animRunState)
	{
	case 0:
		pokeblockFeed->animId = sNatureToMonPokeblockAnim[pokeblockFeed->nature][0];
		pokeblockFeed->monSpritePtr = &gSprites[pokeblockFeed->monSpriteId];
		pokeblockFeed->animRunState = 10;
		break;
	case 1 ... 9:
		break;
	case 10:
		InitMonAnimStage();
		if (sNatureToMonPokeblockAnim[pokeblockFeed->nature][1] != AFFINE_NONE)
		{
			// 回る動きの用意
			if (!pokeblockFeed->noMonFlip)
				FreeSpriteOamMatrix(pokeblockFeed->monSpritePtr);   //作成時の反転の行列を返してから作り直す（原文は返さず 0 番を決め打ち）
			pokeblockFeed->monSpritePtr->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
			pokeblockFeed->monSpritePtr->affineAnims = sAffineAnims_Mon;
			InitSpriteAffineAnim(pokeblockFeed->monSpritePtr);
			CalcCenterToCornerVec(pokeblockFeed->monSpritePtr, pokeblockFeed->monSpritePtr->oam.shape, pokeblockFeed->monSpritePtr->oam.size, ST_OAM_AFFINE_DOUBLE);
		}
		pokeblockFeed->animRunState = 50;
		// fallthrough
	case 50:
		if (sNatureToMonPokeblockAnim[pokeblockFeed->nature][1] != AFFINE_NONE)
		{
			// 回る動きを始める
			if (!pokeblockFeed->noMonFlip) // 反転する絵
				StartSpriteAffineAnim(pokeblockFeed->monSpritePtr, sNatureToMonPokeblockAnim[pokeblockFeed->nature][1] + NUM_MON_AFFINES);
			else
				StartSpriteAffineAnim(pokeblockFeed->monSpritePtr, sNatureToMonPokeblockAnim[pokeblockFeed->nature][1]);
		}
		pokeblockFeed->animRunState = 60;
		break;
	case 60:
		if (DoMonAnimStep() == TRUE)
		{
			if (!pokeblockFeed->animData[ANIMDATA_IS_LAST])
			{
				// 次の段へ
				pokeblockFeed->animId++;
				InitMonAnimStage();
				pokeblockFeed->animRunState = 60;
			}
			else
			{
				// 動きの終わり。原文はここでアフィン行列を返すが返さない（ファイル先頭の差を参照）
				pokeblockFeed->animRunState = 70;
			}
		}
		break;
	case 70:
		pokeblockFeed->animId = 0;
		pokeblockFeed->animRunState = 0;
		break;
	case 71 ... 90:
		break;
	}
}

static bool8 InitMonAnimStage(void)
{
	struct PokeblockFeed* pokeblockFeed = sPokeblockFeed;
	u8 i;

	for (i = 0; i < NUM_ANIMDATA; i++)
		pokeblockFeed->animData[i] = sMonPokeblockAnims[pokeblockFeed->animId][i];

	if (pokeblockFeed->animData[ANIMDATA_TIME] == 0)
	{
		return TRUE;
	}
	else
	{
		pokeblockFeed->monInitX = PbfSin(pokeblockFeed->animData[ANIMDATA_ROT_IDX], pokeblockFeed->animData[ANIMDATA_SIN_AMPLITUDE]);
		pokeblockFeed->monInitY = PbfCos(pokeblockFeed->animData[ANIMDATA_ROT_IDX], pokeblockFeed->animData[ANIMDATA_COS_AMPLITUDE]);
		pokeblockFeed->maxAnimStageTime = pokeblockFeed->animData[ANIMDATA_TIME];
		pokeblockFeed->monX = pokeblockFeed->monSpritePtr->pos2.x;
		pokeblockFeed->monY = pokeblockFeed->monSpritePtr->pos2.y;

		// この段で動く位置を全部先に計算する（時間を数え下げるので、終わったら戻す）
		CalculateMonAnimMovement();
		pokeblockFeed->animData[ANIMDATA_TIME] = pokeblockFeed->maxAnimStageTime;
		CalculateMonAnimMovementEnd();
		pokeblockFeed->animData[ANIMDATA_TIME] = pokeblockFeed->maxAnimStageTime;
		return FALSE;
	}
}

static bool8 DoMonAnimStep(void)
{
	// ポケモンの位置を進める
	u16 time = sPokeblockFeed->maxAnimStageTime - sPokeblockFeed->animData[ANIMDATA_TIME];
	sPokeblockFeed->monSpritePtr->pos2.x = sPokeblockFeed->monAnimX[time];
	sPokeblockFeed->monSpritePtr->pos2.y = sPokeblockFeed->monAnimY[time];

	// この段の残り時間を数え下げる。終わったら TRUE
	if (--sPokeblockFeed->animData[ANIMDATA_TIME] == 0)
		return TRUE;
	else
		return FALSE;
}

static void CalculateMonAnimMovementEnd(void)
{
	struct PokeblockFeed* pokeblockFeed = sPokeblockFeed;
	u16 i;
	u16 approachTime = pokeblockFeed->animData[ANIMDATA_APPR_TIME];
	u16 time = pokeblockFeed->maxAnimStageTime - approachTime;
	s16 x = pokeblockFeed->monX + pokeblockFeed->animData[ANIMDATA_TARGET_X];
	s16 y = pokeblockFeed->monY + pokeblockFeed->animData[ANIMDATA_TARGET_Y];

	for (i = 0; i < time - 1; i++)
	{
		s16 xOffset = pokeblockFeed->monAnimX[approachTime + i] - x;
		s16 yOffset = pokeblockFeed->monAnimY[approachTime + i] - y;

		pokeblockFeed->monAnimX[approachTime + i] -= xOffset * (i + 1) / time;
		pokeblockFeed->monAnimY[approachTime + i] -= yOffset * (i + 1) / time;
	}

	pokeblockFeed->monAnimX[approachTime + time - 1] = x;
	pokeblockFeed->monAnimY[approachTime + time - 1] = y;
}

static void CalculateMonAnimMovement(void)
{
	struct PokeblockFeed* pokeblockFeed = sPokeblockFeed;
	bool8 negative = FALSE;
	s16 x = pokeblockFeed->monX - pokeblockFeed->monInitX;
	s16 y = pokeblockFeed->monY - pokeblockFeed->monInitY;

	while (1)
	{
		u16 amplitude;
		u16 time;
		u16 acceleration;

		acceleration = abs(pokeblockFeed->animData[ANIMDATA_ROT_ACCEL]);
		amplitude = acceleration + pokeblockFeed->animData[ANIMDATA_COS_AMPLITUDE];
		pokeblockFeed->animData[ANIMDATA_COS_AMPLITUDE] = amplitude;

		if (pokeblockFeed->animData[ANIMDATA_SIN_AMPLITUDE] < 0)
			negative = TRUE;

		time = pokeblockFeed->maxAnimStageTime - pokeblockFeed->animData[ANIMDATA_TIME];

		if (pokeblockFeed->animData[ANIMDATA_TIME] == 0)
			break;

		if (!negative)
		{
			pokeblockFeed->monAnimX[time] = PbfSin(pokeblockFeed->animData[ANIMDATA_ROT_IDX],
												   pokeblockFeed->animData[ANIMDATA_SIN_AMPLITUDE] + amplitude / 0x100) + x;
			pokeblockFeed->monAnimY[time] = PbfCos(pokeblockFeed->animData[ANIMDATA_ROT_IDX],
												   pokeblockFeed->animData[ANIMDATA_COS_AMPLITUDE] + amplitude / 0x100) + y;
		}
		else
		{
			pokeblockFeed->monAnimX[time] = PbfSin(pokeblockFeed->animData[ANIMDATA_ROT_IDX],
												   pokeblockFeed->animData[ANIMDATA_SIN_AMPLITUDE] - amplitude / 0x100) + x;
			pokeblockFeed->monAnimY[time] = PbfCos(pokeblockFeed->animData[ANIMDATA_ROT_IDX],
												   pokeblockFeed->animData[ANIMDATA_COS_AMPLITUDE] - amplitude / 0x100) + y;
		}

		pokeblockFeed->animData[ANIMDATA_ROT_IDX] += pokeblockFeed->animData[ANIMDATA_ROT_SPEED];
		pokeblockFeed->animData[ANIMDATA_ROT_IDX] &= 0xFF;
		pokeblockFeed->animData[ANIMDATA_TIME]--;
	}
}
