#include "defines.h"
#include "defines_battle.h"
#include "../include/new/battle_strings.h"
#include "../include/new/battle_util.h"
#include "../include/m4a.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/malloc.h"
#include "../include/trainer_pokemon_sprites.h"
#include "../include/new/ram_locs.h"
#include "hack_kind.h" //[hack] patches/0956 個体の印の窓口（段 A1）
#include "hack_aura.h"
#include "hack_shadow_screens.h" //[hack] patches/0966 段 H
#include "../include/battle_anim.h"
#include "../include/constants/moves.h"
#include "../include/decompress.h"
#include "../include/trig.h"
#include "../include/gpu_regs.h"

/*
[hack] patches/0956 FEAT-shadow-c（段 C オーラ）: キット（koro 氏・使用可・CREDITS.md）shadow_aura_generic.c の戦闘部分の移植。
 - 出す条件は「判定の窓口」HackKind_AuraKind(bank)（この中・1 か所）。印の種類（ヌシ・デルタ・ダーク）は個体が持つ（hack_kind.h・段 A1）。
 - 絵は 1 セット（graphics/Hack_Aura）。パレットだけ種類ごと: ダーク＝紫・ヌシ＝金・デルタ＝青緑（KindMix の行列）。
 - 登場の文は種類ごとに 3 本（仮の文・strings/hack_aura.string）。
 - つよさをみる画面のオーラ（キットの BanishPokemon／DelPokemon の hook）は移植していない（本流は BW 版・段 H で作る）。
 - リバース（赤紫）はダーク本体（段 E）の物＝IsBattlerReverse は FALSE。
 - 元のコメントは英語のまま残す（キットの原文）。
*/
/*
shadow_aura_generic.c - the shared Dark aura (config.h SHADOW_AURA_GENERIC)

Any Dark individual (Shadow_IsBattlerDark / Shadow_IsMonDark: the individual, not the species), of any species, gets
the same purple aura. The body picture is never touched (the species' own picture). Pictures: graphics/
Shadow_Aura_Generic (grit, 64x64 4bpp, one 8-colour palette): dark_aura_back_layer + dark_aura_front_layer (a foe's
front picture: one behind the body, one over it, same place and transform) and dark_aura_backsprite (the player's
back picture: behind, static). Sounds: assembly/shadow_aura_generic/se_dark_aura_searcher.s / se_dark_aura_appear.s,
played by their song header (MPlayStart), no song table entry.

- Fit (measured once per aura from the body picture, its RAM source or OBJ VRAM): the opaque box of the body's 64x64
  picture. Scale s = (body width + 22) / 58 (58 = the opaque width of the unsplit front aura and of the back aura),
  clamped to 0.85-1.7; the aura's opaque centre column on the body's centre column, the aura's opaque bottom
  (row 61 of the aura picture) 2 px below the body's bottom. Drawn with an own OAM matrix (double size, 128x128 area)
  shared by both layers: 1/s, then the parent's own matrix when the parent is affine (the catch shrink, move
  animations): the whole aura is transformed with the body about the body's centre. The aura has no motion of its own.
- Each frame the aura copies from its parent: position, coordinate offset, priority, OBJ mode, mosaic, visibility
  (hidden with the body, shown while a move animation draws the body on a BG). Subpriority: back layer parent + 1,
  front layer parent - 1. Destroyed when the parent sprite / Pokemon changes, the battler is transformed or behind a
  substitute, or the Pokemon is not Dark any more; tiles, palette and matrix are freed with the last user.
- Palettes: one per battler position (tag 0x5B00 + position), both layers on it. Reverse Mode (the player's Pokemon)
  shows the same shades purple-red. A fade of the parent's palette alone (catch, hit flash) is copied onto the aura's
  palette (MirrorFade), so body and aura change and vanish together.
- Foe faint (FR SeqEnemyPokeKizetsuMain: the body sinks behind a fixed line): own copies of both sheets (RAM buffer
  gNewBS->hackAuraRevealBuf[pos / 2], tags 0x5CFC-0x5CFF) lose the texel rows below that line.
- Resources missing (tile tag slot, tiles, palette, matrix, sprite): no aura (the body alone), no retry until the
  battler's picture changes.
- Reveal (foe front only; the player's back aura is static from the start). The first time a foe party slot shows its
  aura in a battle (gNewBS->hackAuraRevealed) it is hidden until both sides are out: FR(J) ServerMain (0x08011C34)
  is replaced by HackAura_ServerMain. At the start of a frame with no command pending (gBattleExecBuffer = 0), the
  intro past the player's send-out, every player battler shown with its healthbox, and the foe's battler settled,
  the sequence starts and that frame's server and clients are skipped until it is over:
    1. the searcher sound (SE2, player volume 0x200) and the Lock-On on every foe of the sequence, together
    2. wait for both (the searcher's end, the Lock-On's frame 62)
    3. the aura (flash and rising light) and the aura sound (SE1, normal volume) start on the same frame
    4. wait for the aura sound (and the reveal's 128 frames)
    5. gText_ShadowAuraFound printed by the player's controller, kept MOJI_WAIT (64) frames, then the action menu
       (only the server is held). Link battles: no message (the command would go over the link).
  The battle music is at 0xC0 from 1 to 4 and back to 0x100 after it; the reverb the songs' headers switch off is
  given back. Foes that start together share one searcher, one aura sound and one message.
- Lock-On: FR's own Lock-On picture (ANIM_TAG_LOCK_ON); at the battler's coordinate 4 frames, moved to the picture's
  centroid in 8 frames, the corners closing from 32 px to 8 px in 17 frames, held, blinking every 3 frames from
  frame 44, gone at 62 (the last one frees its tiles and palette). Never red, no sound of its own.
- Reveal (frame f from the aura sound's start; palette writes only, the pictures stay still): the front layers' bright
  shades 5-7 are split into 3 height bands (slots 5-13, tools/make_aura_flow_layers.py). A wave of light climbs the
  bands from f 0: w = max(0, cos(2 pi (band / 3 - f / 24)))^2 * 0.55 * amp, slot = base * (1 - w) + (210, 150, 255) * w,
  amp 1 up to f 84, then ((104 - f) / 20)^2, 0 (still, base palette) from f 104. Over it, f 0-23, every colour from
  (232, 200, 255) to the wave colour, 1 - (1 - f / 24)^3.
- Summary screen (FR(J) p_status2.c BanishPokemon 0x0813A41C / DelPokemon 0x0813A458 hooked): both front layers,
  static, fitted the same way, on an own palette slot (purple, purple-red in Reverse Mode) and an own matrix.
*/

#ifdef HACK_AURA

#ifndef SHADOW_AURA_GENERIC_SE_SEARCH
#define SHADOW_AURA_GENERIC_SE_SEARCH 1
#endif
#ifndef SHADOW_AURA_GENERIC_SE_APPEAR
#define SHADOW_AURA_GENERIC_SE_APPEAR 1
#endif

extern const u8 dark_aura_back_layerTiles[];  //graphics/Shadow_Aura_Generic (grit)
extern const u8 dark_aura_front_layerTiles[];
extern const u8 dark_aura_backspriteTiles[];
extern const u16 dark_aura_back_layerPal[];   //the front layers: colours 1-4, then slots 5-13 = shades 5, 6, 7 per band
extern const u16 dark_aura_backspritePal[];   //the back picture: colours 1-7

#if SHADOW_AURA_GENERIC_SE_SEARCH
extern const struct SongHeader se_dark_aura_searcher; //assembly/shadow_aura_generic/se_dark_aura_searcher.s
#define SEARCHER_SONG (&se_dark_aura_searcher)
#else
#define SEARCHER_SONG ((const struct SongHeader*) NULL)
#endif
#if SHADOW_AURA_GENERIC_SE_APPEAR
extern const struct SongHeader se_dark_aura_appear;
#define APPEAR_SONG (&se_dark_aura_appear)
#else
#define APPEAR_SONG ((const struct SongHeader*) NULL)
#endif

void __attribute__((long_call)) MainFightWait2Comm(void);
void __attribute__((long_call)) MainFightWait4Comm(void);
void __attribute__((long_call)) SpriteCallbackDummy3(struct Sprite* sprite); //FR battle_main: the wild foe after its intro
extern const struct CompressedSpriteSheet gBattleAnimPicTable[];
extern const struct CompressedSpritePalette gBattleAnimPaletteTable[];
#define SeqEnemyPokeKizetsuMain ((void (*)(struct Sprite*)) (0x08011804 | 1)) //FR(J) a_enemy.c: the foe's faint slide (data[3] = tile rows still shown)
#define OAM_MATRICES ((struct OamMatrix*) 0x02021B48) //FR(J) gOamMatrices (SetOamMatrix 0x08006F38)

//FR(J) server.c intro steps up to the player's send-out (gBattleMainFunc values): no reveal before both sides are out.
static const u32 sIntroBeforePlayerOut[] =
{
	0x08011C11, //ServerInit
	0x080127FD, //ServerPokeParaGet
	0x08012871, //ServerEncountEffect
	0x080128C1, //ServerResWait
	0x08012B69, //ServerEncountMessage
	0x08012D09, //ServerBallGaugeSet
	0x08012D41, //ServerBallGaugeMSet
	0x08012D95, //ServerPokemonEAppearMsg
	0x08012DC9, //ServerTrainerReturnE
	0x08012E59, //ServerTrainerReturnEWait
	0x08012EE5, //ServerPokemonEAppear
	0x08012F05, //ServerPokemonMAppearMsg
	0x08012F49, //ServerTrainerReturnM
};

//Tags (below the gauge tags 0x5D00)
#define TILE_TAG_BACK_LAYER 0x5B00   //foe: behind the body
#define TILE_TAG_FRONT_LAYER 0x5B01  //foe: over the body
#define TILE_TAG_BACKSPRITE 0x5B02   //player: behind the back picture
#define AURA_PAL_TAG(pos) (0x5B00 + (pos))
#define OWN_TILE_TAG(pos, front) (0x5CFC + ((front) ? 2 : 0) + ((pos) >> 1)) //foe positions 1 / 3: own copies while appearing / fainting
#define AURA_TILES_SIZE (64 * 64 / 2)
#define FRONT_TILES_SIZE (32 * 64 / 2)  //the front layer sprite: the right half of its picture
#define OWN_BUFFER_SIZE (AURA_TILES_SIZE + FRONT_TILES_SIZE) //gNewBS->hackAuraRevealBuf: back sheet, then front sheet

//The aura pictures' opaque box (all three: columns 4-61, bottom row 60; the front pair measured from both layers
//together before the split, the back picture on its own) and the fit rule.
#define AURA_BOX_X0 4
#define AURA_BOX_X1 62  //exclusive
#define AURA_BOX_Y1 61  //exclusive
#define FIT_EXTRA_WIDTH 22
#define FIT_BELOW 2
#define FIT_SCALE_MIN 218 //0.85 in 1/256
#define FIT_SCALE_MAX 435 //1.7

#define AURA_SEQUENCE_MAX_FRAMES 600 //a sound that never reports its end does not hold the battle longer
#define AURA_SEARCHER_VOLUME 0x200   //SE2 player volume for the searcher (0x100 = normal)
#define AURA_CUE_BGM_VOLUME 0xC0     //the battle music while the two sounds play
#define MOJI_WAIT 0x40               //FR(J) waza_seq.s: frames MESSAGE_WAIT keeps a battle message
#define STRINGID_STRING_LOADER 0x184 //CFRU BufferStringBattle: the text gBattleStringLoader points to
#define PLANB_FLASH_FRAMES 24
#define PLANB_END 104                //f from which the reveal is still (base palette)
#define PLANB_STOP 20                //frames over which the wave fades out
#define PLANB_FRAMES (PLANB_FLASH_FRAMES + PLANB_END)

extern const u8 gText_HackAuraFoundDark[], gText_HackAuraFoundNushi[], gText_HackAuraFoundDelta[]; //strings/hack_aura.string（仮の文）

//Manager task data (index = battler position)
#define tAuraSprite(pos) data[0 + (pos)] //back-layer (main) aura sprite id or MAX_SPRITES
#define tKey(pos) data[4 + (pos)]        //the parent sprite and Pokemon the aura (or its failure) belongs to
#define tAttached data[8]                //counters for the tests
#define tFallbacks data[9]
#define tLastFailure data[10]            //AURA_FAIL_*
#define tMessage data[11]                //MESSAGE_*
#define tFailed data[12]                 //bit pos: the aura could not be made for tKey(pos)
#define tSound data[13]                  //SOUND_*: what the sequence changed
#define tReverb data[14]                 //the reverb before the sequence
#define tMagic data[15]
#define AURA_MAGIC 0x5A0B
#define SOUND_BGM_LOWERED 0x1
#define SOUND_REVERB_SAVED 0x2

//Aura sprite data (the main sprite: the back layer of a foe, the player's back aura)
#define sPosParent data[0] //position (low byte), parent sprite (high byte)
#define sCx data[1]        //aura centre at rest, from the body picture's centre, in 1/16 px
#define sFrontMatrix data[2] //front-layer sprite (low byte, MAX_SPRITES none), OAM matrix (high byte)
#define sCy data[3]
#define sReveal data[4]
#define sTimer data[5]
#define sFlags data[6]     //AURA_FLAG_* (low byte), first cleared texel row while fainting (high byte, 64 = none)
#define sScale data[7]     //s in 1/256
#define AURA_FLAG_RED      0x01
#define AURA_FLAG_MIRRORED 0x02
#define AURA_FLAG_FAINT    0x04 //own sheet copies (FAINT_TILE_TAG) lose rows with the body
#define AURA_FLAG_MEASURED 0x08
#define AURA_FLAG_BACK     0x10 //the player's back aura (one layer)
#define AURA_FLAG_PLANB    0x20 //the palette shows the reveal's colours
#define AURA_FLAG_DISSOLVE 0x40 //own sheets (OWN_TILE_TAG) being filled dot by dot
#define HAS_FLAG(sprite, f) (((sprite)->sFlags & (f)) != 0)
#define SET_FLAG(sprite, f, on) ((sprite)->sFlags = (on) ? ((sprite)->sFlags | (f)) : ((sprite)->sFlags & ~(f)))
#define POS(sprite) ((u8) ((sprite)->sPosParent & 0xFF))
#define PARENT(sprite) ((u8) (((u16) (sprite)->sPosParent) >> 8))
#define FRONT(sprite) ((u8) ((sprite)->sFrontMatrix & 0xFF))
#define MATRIX(sprite) ((u8) (((u16) (sprite)->sFrontMatrix) >> 8))
#define FAINT_CUT(sprite) ((u8) (((u16) (sprite)->sFlags) >> 8))
#define SET_FAINT_CUT(sprite, v) ((sprite)->sFlags = ((sprite)->sFlags & 0xFF) | ((v) << 8))

//Front-layer sprite data
#define sLayerMain data[0] //the main sprite id

//NONE: static. WAIT: hidden until HackAura_ServerMain starts the sequence. SEARCH: hidden, the searcher and the
//Lock-On. APPEAR: the reveal (flash, rising light) and the aura sound. SEARCH / APPEAR hold the battle.
enum {REVEAL_NONE, REVEAL_WAIT, REVEAL_SEARCH, REVEAL_APPEAR};
enum {MESSAGE_NONE, MESSAGE_PENDING, MESSAGE_PRINTING, MESSAGE_WAIT};
enum {HOLD_NONE, HOLD_SERVER, HOLD_ALL};
enum {AURA_FAIL_TAG_SLOT = 1, AURA_FAIL_TILES, AURA_FAIL_PALETTE, AURA_FAIL_SPRITE, AURA_FAIL_MATRIX};

static void Task_ShadowAura(u8 taskId);
static void SpriteCB_ShadowAura(struct Sprite* sprite);
static void SpriteCB_ShadowAuraFront(struct Sprite* sprite);

static const struct OamData sAuraOam =
{
	.affineMode = ST_OAM_AFFINE_OFF, //set to double size once the sprite has its own matrix (CreateSprite allocates none)
	.objMode = ST_OAM_OBJ_NORMAL,
	.shape = SPRITE_SHAPE(64x64),
	.size = SPRITE_SIZE(64x64),
	.priority = 2,
};

static const struct OamData sFrontOam = //the front layer: the right half (columns 32-63) of its 64x64 picture
{
	.affineMode = ST_OAM_AFFINE_OFF,
	.objMode = ST_OAM_OBJ_NORMAL,
	.shape = SPRITE_SHAPE(32x64),
	.size = SPRITE_SIZE(32x64),
	.priority = 2,
};

#define AURA_TEMPLATE(tag, oamData, cb) {.tileTag = (tag), .paletteTag = 0xFFFF, .oam = &(oamData), .anims = gDummySpriteAnimTable, \
	.images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = (cb)}
static const struct SpriteTemplate sBackLayerTemplate = AURA_TEMPLATE(TILE_TAG_BACK_LAYER, sAuraOam, SpriteCB_ShadowAura);
static const struct SpriteTemplate sFrontLayerTemplate = AURA_TEMPLATE(TILE_TAG_FRONT_LAYER, sFrontOam, SpriteCB_ShadowAuraFront);
static const struct SpriteTemplate sBackspriteTemplate = AURA_TEMPLATE(TILE_TAG_BACKSPRITE, sAuraOam, SpriteCB_ShadowAura);

//---- fit ------------------------------------------------------------------------------------------------------------

//The opaque box of a 64x64 4bpp picture (1D tiles): FALSE if it has no opaque pixel.
static bool8 PictureBox(const u8* tiles, u8* x0, u8* x1, u8* y1)
{
	u32 x, y, tx, minX = 64, maxX = 0, maxY = 0;
	bool8 any = FALSE;

	for (y = 0; y < 64; ++y)
	{
		for (tx = 0; tx < 8; ++tx)
		{
			u32 word = *(const u32*) (tiles + ((y / 8) * 8 + tx) * TILE_SIZE_4BPP + (y % 8) * 4);
			if (word == 0)
				continue;
			for (x = 0; x < 8; ++x)
			{
				if ((word >> (x * 4)) & 0xF)
				{
					u32 px = tx * 8 + x;
					if (px < minX)
						minX = px;
					if (px > maxX)
						maxX = px;
				}
			}
			maxY = y;
			any = TRUE;
		}
	}
	if (!any)
		return FALSE;
	*x0 = minX;
	*x1 = maxX + 1;
	*y1 = maxY + 1;
	return TRUE;
}

//The picture a sprite shows: its frame source (RAM / ROM) when it copies frames, else its tiles in OBJ VRAM.
static const u8* PictureOf(struct Sprite* parent)
{
	if (!parent->usingSheet && parent->images != NULL)
	{
		u32 src = (u32) parent->images[0].data;
		if ((src >= 0x02000000 && src < 0x02040000) || (src >= 0x08000000 && src < 0x0A000000))
			return (const u8*) src;
	}
	return (const u8*) (OBJ_VRAM0 + parent->oam.tileNum * TILE_SIZE_4BPP);
}

//Scale and rest centre (from the body picture's centre, 1/16 px) of an aura on this body. FALSE: nothing shown yet.
static bool8 MeasureFit(struct Sprite* parent, s16* cx, s16* cy, s16* scale)
{
	u8 x0, x1, y1;
	s32 s;

	if (!PictureBox(PictureOf(parent), &x0, &x1, &y1))
		return FALSE;
	if (parent->oam.affineMode == ST_OAM_AFFINE_OFF && (parent->oam.matrixNum & 8)) //h-flipped body
	{
		u8 t = x0;
		x0 = 64 - x1;
		x1 = 64 - t;
	}
	s = ((x1 - x0 + FIT_EXTRA_WIDTH) * 256 + (AURA_BOX_X1 - AURA_BOX_X0) / 2) / (AURA_BOX_X1 - AURA_BOX_X0);
	if (s < FIT_SCALE_MIN)
		s = FIT_SCALE_MIN;
	else if (s > FIT_SCALE_MAX)
		s = FIT_SCALE_MAX;
	*scale = s;
	*cx = (x0 + x1) * 8 - s * (AURA_BOX_X0 + AURA_BOX_X1 - 64) * 8 / 256 - 32 * 16;
	*cy = (y1 + FIT_BELOW) * 16 - s * (AURA_BOX_Y1 - 32) * 16 / 256 - 32 * 16;
	return TRUE;
}

static s32 RoundQ(s32 v, u32 shift)
{
	return (v + (1 << (shift - 1))) >> shift; //arithmetic shift: rounds half up for negatives too
}

//Places an aura (and its front layer) on its parent: rest centre (cx, cy) in 1/16 px, scale in 1/256, then the
//parent's own matrix if it is affine. FALSE if the parent's matrix is degenerate (nothing to show).
//The front layer is a 32x64 sprite holding the right half (columns 32-63) of its 64x64 picture: its centre is texel
//(48, 32) of the picture, 16 texels right of the back layer's (16 * s px before the parent's transform). Its double-size
//area is 64 px wide: 138 OBJ cycles a scanline instead of 266 (the GBA draws at most 1210 a scanline; three 128-wide
//auras, the battlers and CFRU's ability pop-up went over it, and sprites were dropped on those lines).
static bool8 PlaceAura(struct Sprite* aura, struct Sprite* front, struct Sprite* parent, s32 cx, s32 cy, s32 scale, u8 matrix)
{
	s32 pcx, pcy, dx = cx, dy = cy, fdx = cx + scale, fdy = cy, a, half; //1/16 px
	s32 ta, tb, tc, td;

	half = (parent->oam.affineMode & ST_OAM_AFFINE_DOUBLE_MASK) ? 64 : 32;
	pcx = parent->pos1.x + parent->pos2.x + parent->centerToCornerVecX + half;
	pcy = parent->pos1.y + parent->pos2.y + parent->centerToCornerVecY + half;
	a = (65536 + scale / 2) / scale; //1/s in 1/256

	if (parent->oam.affineMode & ST_OAM_AFFINE_ON_MASK)
	{
		const struct OamMatrix* p = &OAM_MATRICES[parent->oam.matrixNum & 0x1F];
		s32 det = (p->a * p->d - p->b * p->c) >> 8; //1/256
		s32 n1, n2;

		if (det == 0)
			return FALSE;
		n1 = (p->d * dx - p->b * dy) / det; //P^-1 d (32-bit only)
		n2 = (-p->c * dx + p->a * dy) / det;
		dx = n1;
		dy = n2;
		n1 = (p->d * fdx - p->b * fdy) / det;
		n2 = (-p->c * fdx + p->a * fdy) / det;
		fdx = n1;
		fdy = n2;
		ta = (a * p->a) >> 8;
		tb = (a * p->b) >> 8;
		tc = (a * p->c) >> 8;
		td = (a * p->d) >> 8;
	}
	else
	{
		ta = a;
		tb = 0;
		tc = 0;
		td = a;
	}

	SetOamMatrix(matrix, ta, tb, tc, td);
	aura->pos1.x = pcx + RoundQ(dx, 4);
	aura->pos1.y = pcy + RoundQ(dy, 4);
	aura->pos2.x = 0;
	aura->pos2.y = 0;
	aura->centerToCornerVecX = -64;
	aura->centerToCornerVecY = -64;
	aura->coordOffsetEnabled = parent->coordOffsetEnabled;
	aura->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
	aura->oam.matrixNum = matrix;
	aura->oam.priority = parent->oam.priority;
	aura->oam.objMode = parent->oam.objMode;
	aura->oam.mosaic = parent->oam.mosaic;
	if (front != NULL)
	{
		front->pos1.x = pcx + RoundQ(fdx, 4);
		front->pos1.y = pcy + RoundQ(fdy, 4);
		front->pos2.x = 0;
		front->pos2.y = 0;
		front->centerToCornerVecX = -32;
		front->centerToCornerVecY = -64;
		front->coordOffsetEnabled = aura->coordOffsetEnabled;
		front->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
		front->oam.matrixNum = matrix;
		front->oam.priority = aura->oam.priority;
		front->oam.objMode = aura->oam.objMode;
		front->oam.mosaic = aura->oam.mosaic;
		front->oam.paletteNum = aura->oam.paletteNum;
	}
	return TRUE;
}

//---- palettes -------------------------------------------------------------------------------------------------------

//---- kinds ----------------------------------------------------------------------------------------------------------
//[hack] patches/0956 絵は 1 セット（紫）・種類ごとにパレットの色だけ変える: 3x3 の行列（1/8 単位）で r,g,b を組み替える。
//ダーク＝そのまま（紫）・ヌシ＝金（R'=B・G'=1.125R・B'=G）・デルタ＝青緑（R'=G・G'=1.25R・B'=0.875B）。
static const u8 sKindMatrix[4][3][3] =
{
	[HACK_KIND_NONE]  = {{8, 0, 0}, {0, 8, 0}, {0, 0, 8}},
	[HACK_KIND_NUSHI] = {{0, 0, 8}, {9, 0, 0}, {0, 8, 0}},
	[HACK_KIND_DELTA] = {{0, 8, 0}, {10, 0, 0}, {0, 0, 7}},
	[HACK_KIND_DARK]  = {{8, 0, 0}, {0, 8, 0}, {0, 0, 8}},
};

//v[3]（0〜max）を種類の行列で組み替える
static void KindMix(u8 kind, const s32* in, s32* out, s32 max)
{
	u32 o, i;

	for (o = 0; o < 3; ++o)
	{
		s32 sum = 0;
		for (i = 0; i < 3; ++i)
			sum += (s32) sKindMatrix[kind & 3][o][i] * in[i];
		sum >>= 3;
		out[o] = (sum > max) ? max : sum;
	}
}

static u16 KindColour(u8 kind, u16 c)
{
	s32 in[3], out[3];

	in[0] = c & 0x1F;
	in[1] = (c >> 5) & 0x1F;
	in[2] = (c >> 10) & 0x1F;
	KindMix(kind, in, out, 31);
	return (u16) ((c & 0x8000) | out[0] | (out[1] << 5) | (out[2] << 10));
}

//---- palettes -------------------------------------------------------------------------------------------------------

//The aura colours (the front layers' palette, or the back picture's) in the kind's colours, or in Reverse Mode the same
//shades purple-red (red and blue swapped; Dark only).
static void AuraColours(u8 kind, bool8 red, bool8 back, u16* out)
{
	const u16* src = back ? dark_aura_backspritePal : dark_aura_back_layerPal;
	u32 i;

	for (i = 0; i < 16; ++i)
	{
		u16 c = src[i];
		out[i] = red ? (u16) ((c & 0x83E0) | ((c & 0x1F) << 10) | ((c >> 10) & 0x1F)) : KindColour(kind, c);
	}
}

static bool8 PaletteFaded(u8 slot, u8 count)
{
	u32 i;
	const u16* unfaded = &gPlttBufferUnfaded[0x100 + slot * 16];
	const u16* faded = &gPlttBufferFaded[0x100 + slot * 16];

	for (i = 1; i <= count; ++i)
	{
		if (unfaded[i] != faded[i])
			return TRUE;
	}
	return FALSE;
}

static s32 Channel(u16 colour, u32 ch)
{
	return (colour >> (ch * 5)) & 0x1F;
}

//The parent's palette faded on its own: the same blend on the aura's palette (FR's blends are a line with the same
//slope in every channel: the slope from the parent's channel with the widest spread, each channel's offset from its
//colours 1-15).
static void MirrorFade(u8 parentSlot, u8 auraSlot)
{
	const u16* pu = &gPlttBufferUnfaded[0x100 + parentSlot * 16];
	const u16* pf = &gPlttBufferFaded[0x100 + parentSlot * 16];
	const u16* au = &gPlttBufferUnfaded[0x100 + auraSlot * 16];
	u16* af = &gPlttBufferFaded[0x100 + auraSlot * 16];
	s32 slope = 256, spread = 0, offset[3];
	u32 ch, i;

	for (ch = 0; ch < 3; ++ch)
	{
		u32 lo = 1, hi = 1;
		for (i = 2; i < 16; ++i)
		{
			if (Channel(pu[i], ch) < Channel(pu[lo], ch))
				lo = i;
			if (Channel(pu[i], ch) > Channel(pu[hi], ch))
				hi = i;
		}
		if (Channel(pu[hi], ch) - Channel(pu[lo], ch) > spread)
		{
			spread = Channel(pu[hi], ch) - Channel(pu[lo], ch);
			slope = (Channel(pf[hi], ch) - Channel(pf[lo], ch)) * 256 / spread;
		}
	}

	for (ch = 0; ch < 3; ++ch)
	{
		s32 sum = 0;
		for (i = 1; i < 16; ++i)
			sum += Channel(pf[i], ch) * 256 - slope * Channel(pu[i], ch);
		offset[ch] = sum / 15;
	}

	for (i = 1; i < 16; ++i)
	{
		u16 c = 0;
		for (ch = 0; ch < 3; ++ch)
		{
			s32 v = (offset[ch] + slope * Channel(au[i], ch) + 128) >> 8;
			c |= ((v < 0) ? 0 : (v > 31) ? 31 : v) << (ch * 5);
		}
		af[i] = c;
	}
}

static bool8 SlotInScreenFade(u8 slot)
{
	return gPaletteFade->active && (gPaletteFade_selectedPalettes & (1 << (16 + slot)));
}

//A fade that also reaches the aura's palette (screen fades) is left to the fade engine; a fade of the parent alone is
//copied (MirrorFade); once it is over the aura's colours come back.
static void UpdateFade(struct Sprite* sprite, struct Sprite* parent)
{
	u8 slot = sprite->oam.paletteNum;

	if (SlotInScreenFade(slot))
		SET_FLAG(sprite, AURA_FLAG_MIRRORED, FALSE);
	else if (PaletteFaded(parent->oam.paletteNum, 15))
	{
		MirrorFade(parent->oam.paletteNum, slot);
		SET_FLAG(sprite, AURA_FLAG_MIRRORED, TRUE);
	}
	else if (HAS_FLAG(sprite, AURA_FLAG_MIRRORED))
	{
		CpuCopy16(&gPlttBufferUnfaded[0x100 + slot * 16], &gPlttBufferFaded[0x100 + slot * 16], 32);
		SET_FLAG(sprite, AURA_FLAG_MIRRORED, FALSE);
	}
}

//New colours for the aura's own slot: both buffers, or only the unfaded one while a screen fade owns the faded one.
static void SetAuraColours(struct Sprite* sprite, const u16* colours)
{
	u8 slot = sprite->oam.paletteNum;

	if (SlotInScreenFade(slot))
		CpuCopy16(colours, &gPlttBufferUnfaded[0x100 + slot * 16], 32);
	else
		LoadPalette(colours, 0x100 + slot * 16, 32);
	SET_FLAG(sprite, AURA_FLAG_MIRRORED, TRUE); //a fade of the parent (if any) is copied again over the new colours
}

static void UpdateRed(struct Sprite* sprite, bool8 red, u8 kind)
{
	u16 colours[16];

	if (HAS_FLAG(sprite, AURA_FLAG_RED) == red)
		return;
	AuraColours(kind, red, HAS_FLAG(sprite, AURA_FLAG_BACK), colours);
	SetAuraColours(sprite, colours);
	SET_FLAG(sprite, AURA_FLAG_RED, red);
}

//[hack] リバース（赤紫）はダーク本体（段 E）の物。段 C では常に FALSE（E で窓口をつなぐ）
static bool8 IsBattlerReverse(u8 bank)
{
	(void) bank;
	return FALSE;
}

//判定の窓口（1 か所）: この戦闘の bank のポケモンの印の種類（HACK_KIND_*）。印は個体が持つ（段 A1・hack_kind.h）
u8 HackKind_AuraKind(u8 bank)
{
	struct Pokemon* mon;

	if (bank >= gBattlersCount)
		return HACK_KIND_NONE;
	mon = GetBankPartyData(bank);
	return (mon == NULL) ? HACK_KIND_NONE : HackKind_Get((const struct BoxPokemon*) mon);
}

static u8 KindOfPos(u8 pos)
{
	return HackKind_AuraKind(GetBattlerAtPosition(pos));
}

//---- the reveal: flash and rising light ------------------------------------------------------------------------------

//The front layers' bright shades (5, 6, 7) are split into 3 height bands (tools/make_aura_flow_layers.py): colour
//5 + (shade - 5) * 3 + band, band = ((63 - y) / 3) % 3 from the bottom, so palette slots 5-13 (each the base colour of
//its shade). A wave of light climbs the bands (palette writes only, the picture never moves): for frame f from the
//aura sound's start, w = max(0, cos(2 pi (band / 3 - f / 24)))^2 * 0.55 * amp, slot = base * (1 - w) + (210, 150, 255) * w,
//amp 1 up to f 84, then ((104 - f) / 20)^2, 0 from f 104. The flash (f 0-23) is laid over it: every colour from
//(232, 200, 255) to the wave colour, 1 - (1 - f / 24)^3.
static const u8 sFlashColour[3] = {232, 200, 255}; //ダーク（紫）の値・ほかの種類は KindMix で組み替える
static const u8 sGlowColour[3] = {210, 150, 255};
#define FLOW_PERIOD 24   //frames for the wave to climb one set of 3 bands
#define FLOW_STRENGTH 141 //0.55 in 1/256
#define FLOW_SLOT(shade, band) (5 + ((shade) - 5) * 3 + (band))

static s32 Mix8(s32 from, s32 to, s32 t) //t in 1/256: from -> to
{
	return (from * (256 - t) + to * t) >> 8;
}

//The reveal's palette at frame f (f < PLANB_FRAMES), from the normal colours.
static void PlanBColours(u8 kind, s32 f, u16* out)
{
	s32 amp, rgb[16][3], flash[3], glow[3], in[3];
	u32 i, ch, shade, band;

	for (ch = 0; ch < 3; ++ch)
		in[ch] = sFlashColour[ch];
	KindMix(kind, in, flash, 255);
	for (ch = 0; ch < 3; ++ch)
		in[ch] = sGlowColour[ch];
	KindMix(kind, in, glow, 255);
	AuraColours(kind, FALSE, FALSE, out);
	for (i = 0; i < 16; ++i)
		for (ch = 0; ch < 3; ++ch)
			rgb[i][ch] = Channel(out[i], ch) << 3; //the palette's 8-bit values (multiples of 8)

	amp = (f < PLANB_END - PLANB_STOP) ? 256 : (f < PLANB_END) ? ((PLANB_END - f) * (PLANB_END - f) * 256) / (PLANB_STOP * PLANB_STOP) : 0;
	if (amp > 0)
	{
		for (band = 0; band < 3; ++band)
		{
			s32 phase = ((s32) band * (FLOW_PERIOD / 3) - f) % FLOW_PERIOD;
			s32 cosine, w;
			if (phase < 0)
				phase += FLOW_PERIOD;
			cosine = gSineTable[((phase * 256 / FLOW_PERIOD) + 64) & 0xFF]; //cos(2 pi (band / 3 - f / 24)) in 1/256
			if (cosine < 0)
				cosine = 0;
			w = (cosine * cosine / 256) * FLOW_STRENGTH / 256 * amp / 256;
			for (shade = 5; shade <= 7; ++shade)
				for (ch = 0; ch < 3; ++ch)
					rgb[FLOW_SLOT(shade, band)][ch] = Mix8(rgb[FLOW_SLOT(shade, band)][ch], glow[ch], w);
		}
	}
	if (f < PLANB_FLASH_FRAMES)
	{
		s32 u = PLANB_FLASH_FRAMES - f;
		s32 e = 256 - (256 * u * u * u) / (PLANB_FLASH_FRAMES * PLANB_FLASH_FRAMES * PLANB_FLASH_FRAMES); //1 - (1 - t)^3
		for (i = 1; i < 16; ++i)
			for (ch = 0; ch < 3; ++ch)
				rgb[i][ch] = Mix8(flash[ch], rgb[i][ch], e);
	}
	for (i = 1; i < 16; ++i)
	{
		u16 c = 0;
		for (ch = 0; ch < 3; ++ch)
			c |= ((rgb[i][ch] >> 3) & 0x1F) << (ch * 5);
		out[i] = c;
	}
}

//---- battle aura ----------------------------------------------------------------------------------------------------

static u8 FindManager(void)
{
	u8 taskId = FindTaskIdByFunc(Task_ShadowAura);
	return (taskId < NUM_TASKS && gTasks[taskId].tMagic == AURA_MAGIC) ? taskId : NUM_TASKS;
}

static u8 FindOrCreateManager(void)
{
	u8 taskId = FindManager();
	u32 i;

	if (taskId < NUM_TASKS)
		return taskId;
	taskId = CreateTask(Task_ShadowAura, 0xFF); //runs after the battle's own tasks
	if (taskId >= NUM_TASKS)
		return NUM_TASKS;
	for (i = 0; i < MAX_BATTLERS_COUNT; ++i)
	{
		gTasks[taskId].tAuraSprite(i) = MAX_SPRITES;
		gTasks[taskId].tKey(i) = -1;
	}
	gTasks[taskId].tMagic = AURA_MAGIC; //last: the task is only used once this is set
	return taskId;
}

static bool8 IsAuraSprite(u8 id, u8 pos)
{
	return id < MAX_SPRITES && gSprites[id].inUse && gSprites[id].callback == SpriteCB_ShadowAura && POS(&gSprites[id]) == pos;
}

static bool8 InBattleScreen(void)
{
	MainCallback cb2 = gMain.callback2;
	return cb2 == BattleMainCB2 || cb2 == MainFightWait2Comm || cb2 == MainFightWait4Comm;
}

//The battler sprite of a position (FR: created from gMultiuseSpriteTemplate, palette = battler).
static u8 ParentSprite(u8 pos)
{
	u8 bank = GetBattlerAtPosition(pos);
	u8 id;

	if (bank >= gBattlersCount)
		return MAX_SPRITES;
	id = gBattlerSpriteIds[bank];
	if (id >= MAX_SPRITES || !gSprites[id].inUse || gSprites[id].template != gMultiuseSpriteTemplate
	|| gSprites[id].oam.paletteNum != bank)
		return MAX_SPRITES;
	return id;
}

//Whether the battler sprite shows the Pokemon's own picture: its frames come from the position's picture buffer
//(gMonSpritesGfxPtr->sprites[pos]). A trainer's back picture in the same sprite slot (the intro, the throw, Safari,
//the old man's tutorial, a multi partner) has its frames in ROM. (A foe trainer's front picture is decoded into the
//buffer too: the foe's aura stays hidden until its reveal, which needs the Pokemon out, and is measured again then.)
static bool8 ShowsPokemonPicture(u8 pos, u8 parent)
{
	struct Sprite* sprite = &gSprites[parent];

	return gMonSpritesGfxPtr != NULL && !sprite->usingSheet && sprite->images != NULL
		&& sprite->images[0].data == gMonSpritesGfxPtr->sprites[pos];
}

//Which aura this position should have now: its key (parent sprite, party slot, species), or -1 for none.
static s16 WantedKey(u8 pos, u8 parent)
{
	u8 bank = GetBattlerAtPosition(pos);

	if (parent >= MAX_SPRITES || gBattleSpritesDataPtr == NULL || !ShowsPokemonPicture(pos, parent))
		return -1;
	if ((gBattleMons[bank].status2 & STATUS2_TRANSFORMED) || gBattleSpritesDataPtr->bankData[bank].behindSubstitute
	|| HackKind_AuraKind(bank) == HACK_KIND_NONE)
		return -1;
	return (s16) ((parent & 0x3F) | ((gBattlerPartyIndexes[bank] & 7) << 6) | ((gBattleMons[bank].species & 0x3F) << 9));
}

static bool8 AuraUsesTiles(u16 tileTag)
{
	u32 i;

	for (i = 0; i < MAX_SPRITES; ++i)
	{
		struct Sprite* s = &gSprites[i];
		if (s->inUse && (s->callback == SpriteCB_ShadowAura || s->callback == SpriteCB_ShadowAuraFront)
		&& s->template->tileTag == tileTag)
			return TRUE;
	}
	return FALSE;
}

static void FreeUnusedAuraResources(u8 pos)
{
	static const u16 sTags[] = {TILE_TAG_BACK_LAYER, TILE_TAG_FRONT_LAYER, TILE_TAG_BACKSPRITE};
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sTags); ++i)
	{
		if (!AuraUsesTiles(sTags[i]))
			FreeSpriteTilesByTag(sTags[i]);
	}
	FreeSpritePaletteByTag(AURA_PAL_TAG(pos)); //only this position's aura uses it
}

static void FreeFaintBuffer(u8 pos)
{
	if (gNewBS != NULL && (pos & 1) && gNewBS->hackAuraRevealBuf[pos >> 1] != NULL)
	{
		Free(gNewBS->hackAuraRevealBuf[pos >> 1]);
		gNewBS->hackAuraRevealBuf[pos >> 1] = NULL;
	}
}

static void DestroyAura(struct Sprite* sprite)
{
	u8 pos = POS(sprite), front = FRONT(sprite);

	if (HAS_FLAG(sprite, AURA_FLAG_FAINT | AURA_FLAG_DISSOLVE))
	{
		FreeSpriteTilesByTag(OWN_TILE_TAG(pos, FALSE));
		FreeSpriteTilesByTag(OWN_TILE_TAG(pos, TRUE));
	}
	FreeFaintBuffer(pos);
	if (front < MAX_SPRITES && gSprites[front].inUse && gSprites[front].callback == SpriteCB_ShadowAuraFront)
		DestroySprite(&gSprites[front]);
	if (MATRIX(sprite) < 32)
		FreeOamMatrix(MATRIX(sprite));
	DestroySprite(sprite);
	FreeUnusedAuraResources(pos);
}

static bool8 LoadSheet(const u8* tiles, u32 size, u16 tag)
{
	struct SpriteSheet sheet = {tiles, size, tag};

	if (GetSpriteTileStartByTag(tag) != 0xFFFF)
		return TRUE;
	if (GetSpriteTileStartByTag(0xFFFF) == 0xFFFF) //no free tag slot: LoadSpriteSheet would keep the tiles untagged
		return FALSE;
	LoadSpriteSheet(&sheet);
	return GetSpriteTileStartByTag(tag) != 0xFFFF;
}

static bool8 LoadAuraSheet(const u8* tiles, u16 tag)
{
	return LoadSheet(tiles, AURA_TILES_SIZE, tag);
}

//The right half (columns 32-63) of a 64x64 4bpp picture as a 32x64 sheet (1D tiles: 4 tiles a row).
static void CopyFrontHalf(const u8* picture, u8* dest)
{
	u32 row;

	for (row = 0; row < 8; ++row)
		CpuCopy32(picture + (row * 8 + 4) * TILE_SIZE_4BPP, dest + row * 4 * TILE_SIZE_4BPP, 4 * TILE_SIZE_4BPP);
}

//The front layer's 32x64 sheet from its 64x64 picture: tiles allocated with the first 32 tiles, then the right half
//written over them (OBJ VRAM may be written at any time).
static bool8 LoadFrontSheet(const u8* picture, u16 tag)
{
	u16 start;

	if (GetSpriteTileStartByTag(tag) != 0xFFFF)
		return TRUE;
	if (!LoadSheet(picture, FRONT_TILES_SIZE, tag))
		return FALSE;
	start = GetSpriteTileStartByTag(tag);
	CopyFrontHalf(picture, (u8*) (OBJ_VRAM0 + start * TILE_SIZE_4BPP));
	return TRUE;
}

static u8 CreateAura(struct Task* task, u8 pos, u8 parent)
{
	u8 bank = GetBattlerAtPosition(pos);
	bool8 back = SIDE(bank) == B_SIDE_PLAYER;
	bool8 red = IsBattlerReverse(bank);
	u16 colours[16];
	u8 id, front = MAX_SPRITES, palSlot, matrix;

	if (back ? !LoadAuraSheet(dark_aura_backspriteTiles, TILE_TAG_BACKSPRITE)
	         : (!LoadAuraSheet(dark_aura_back_layerTiles, TILE_TAG_BACK_LAYER) || !LoadFrontSheet(dark_aura_front_layerTiles, TILE_TAG_FRONT_LAYER)))
	{
		task->tLastFailure = (GetSpriteTileStartByTag(0xFFFF) == 0xFFFF) ? AURA_FAIL_TAG_SLOT : AURA_FAIL_TILES;
		FreeUnusedAuraResources(pos);
		return MAX_SPRITES;
	}

	AuraColours(HackKind_AuraKind(bank), red, back, colours);
	palSlot = IndexOfSpritePaletteTag(AURA_PAL_TAG(pos));
	if (palSlot != 0xFF) //kept by an aura of this position that is gone: its colours again
		LoadPalette(colours, 0x100 + palSlot * 16, sizeof(colours));
	else
	{
		struct SpritePalette pal = {colours, AURA_PAL_TAG(pos)};
		palSlot = LoadSpritePalette(&pal);
		if (palSlot == 0xFF)
		{
			task->tLastFailure = AURA_FAIL_PALETTE;
			FreeUnusedAuraResources(pos);
			return MAX_SPRITES;
		}
	}

	matrix = AllocOamMatrix();
	if (matrix >= 32)
	{
		task->tLastFailure = AURA_FAIL_MATRIX;
		FreeUnusedAuraResources(pos);
		return MAX_SPRITES;
	}

	id = CreateSpriteAtEnd(back ? &sBackspriteTemplate : &sBackLayerTemplate, gSprites[parent].pos1.x, gSprites[parent].pos1.y, gSprites[parent].subpriority);
	if (id < MAX_SPRITES && !back)
	{
		front = CreateSpriteAtEnd(&sFrontLayerTemplate, gSprites[parent].pos1.x, gSprites[parent].pos1.y, gSprites[parent].subpriority);
		if (front >= MAX_SPRITES)
		{
			DestroySprite(&gSprites[id]);
			id = MAX_SPRITES;
		}
	}
	if (id >= MAX_SPRITES)
	{
		task->tLastFailure = AURA_FAIL_SPRITE;
		FreeOamMatrix(matrix);
		FreeUnusedAuraResources(pos);
		return MAX_SPRITES;
	}

	gSprites[id].oam.paletteNum = palSlot;
	gSprites[id].sPosParent = pos | (parent << 8);
	gSprites[id].sFrontMatrix = front | (matrix << 8);
	gSprites[id].sFlags = (back ? AURA_FLAG_BACK : 0) | (red ? AURA_FLAG_RED : 0) | AURA_FLAG_MIRRORED | (64 << 8);
	gSprites[id].sTimer = 0;
	gSprites[id].sReveal = (!back && gNewBS != NULL
		&& !(gNewBS->hackAuraRevealed & (1 << gBattlerPartyIndexes[bank]))) ? REVEAL_WAIT : REVEAL_NONE;
	gSprites[id].affineAnimPaused = TRUE;
	gSprites[id].animPaused = TRUE;
	gSprites[id].invisible = TRUE;
	if (front < MAX_SPRITES)
	{
		gSprites[front].oam.paletteNum = palSlot;
		gSprites[front].sLayerMain = id;
		gSprites[front].affineAnimPaused = TRUE;
		gSprites[front].animPaused = TRUE;
		gSprites[front].invisible = TRUE;
	}
	++task->tAttached;
	return id;
}

static void UpdatePosition(struct Task* task, u8 pos)
{
	u8 parent = ParentSprite(pos);
	s16 key = WantedKey(pos, parent);
	u8 id = task->tAuraSprite(pos);

	if (key != task->tKey(pos))
	{
		task->tFailed &= ~(1 << pos);
		task->tKey(pos) = key;
	}

	if (IsAuraSprite(id, pos))
	{
		if (key == -1 || PARENT(&gSprites[id]) != parent)
		{
			DestroyAura(&gSprites[id]);
			id = MAX_SPRITES;
		}
	}
	else
		id = MAX_SPRITES;

	if (id >= MAX_SPRITES && key != -1 && !(task->tFailed & (1 << pos)))
	{
		id = CreateAura(task, pos, parent);
		if (id >= MAX_SPRITES)
		{
			task->tFailed |= 1 << pos; //no retry until the picture changes
			++task->tFallbacks;
		}
		else
		{
			task->tAuraSprite(pos) = id;
			SpriteCB_ShadowAura(&gSprites[id]);
		}
	}
	task->tAuraSprite(pos) = id;
}

static void Task_ShadowAura(u8 taskId)
{
	struct Task* task = &gTasks[taskId];
	u32 pos;

	if (!gMain.inBattle) //the battle is over (normally the tasks are reset before this)
	{
		for (pos = 0; pos < MAX_BATTLERS_COUNT; ++pos)
		{
			if (IsAuraSprite(task->tAuraSprite(pos), pos))
				DestroyAura(&gSprites[task->tAuraSprite(pos)]);
		}
		task->tMagic = 0;
		DestroyTask(taskId);
		return;
	}

	if (!InBattleScreen()) //bag, party, summary: their own sprites; the battle screen is rebuilt afterwards
		return;

	for (pos = 0; pos < MAX_BATTLERS_COUNT; ++pos)
		UpdatePosition(task, pos);
}

//Own copies of both sheets (foe positions 1 / 3): gNewBS->hackAuraRevealBuf[pos / 2] (the back sheet, then the
//front sheet) and tiles OWN_TILE_TAG, used while the aura appears (dissolve) and while the foe faints.
static u8* OwnBuffer(u8 pos)
{
	return (gNewBS != NULL && (pos & 1)) ? gNewBS->hackAuraRevealBuf[pos >> 1] : NULL;
}

static void FreeOwnSheets(struct Sprite* sprite)
{
	u8 pos = POS(sprite);

	FreeSpriteTilesByTag(OWN_TILE_TAG(pos, FALSE));
	FreeSpriteTilesByTag(OWN_TILE_TAG(pos, TRUE));
	FreeFaintBuffer(pos);
	SET_FLAG(sprite, AURA_FLAG_FAINT, FALSE);
	SET_FLAG(sprite, AURA_FLAG_DISSOLVE, FALSE);
}

//A buffer and own tiles, empty or holding both pictures (the front one's right half), shown by both layers. FALSE:
//nothing changed (no RAM, tag slot or tiles).
static bool8 UseOwnSheets(struct Sprite* sprite, struct Sprite* front, bool8 empty)
{
	u8 pos = POS(sprite);
	u8* buffer;

	if (gNewBS == NULL || !(pos & 1) || front == NULL || gNewBS->hackAuraRevealBuf[pos >> 1] != NULL)
		return FALSE;
	buffer = Calloc(OWN_BUFFER_SIZE);
	if (buffer == NULL)
		return FALSE;
	if (!empty)
	{
		CpuCopy32(dark_aura_back_layerTiles, buffer, AURA_TILES_SIZE);
		CopyFrontHalf(dark_aura_front_layerTiles, buffer + AURA_TILES_SIZE);
	}
	if (!LoadSheet(buffer, AURA_TILES_SIZE, OWN_TILE_TAG(pos, FALSE))
	|| !LoadSheet(buffer + AURA_TILES_SIZE, FRONT_TILES_SIZE, OWN_TILE_TAG(pos, TRUE)))
	{
		FreeSpriteTilesByTag(OWN_TILE_TAG(pos, FALSE));
		Free(buffer);
		return FALSE;
	}
	gNewBS->hackAuraRevealBuf[pos >> 1] = buffer;
	sprite->oam.tileNum = GetSpriteTileStartByTag(OWN_TILE_TAG(pos, FALSE));
	front->oam.tileNum = GetSpriteTileStartByTag(OWN_TILE_TAG(pos, TRUE));
	return TRUE;
}

//Back to the shared sheets (the same pictures), own copies freed.
static void UseSharedSheets(struct Sprite* sprite, struct Sprite* front)
{
	u16 back = GetSpriteTileStartByTag(TILE_TAG_BACK_LAYER), fr = GetSpriteTileStartByTag(TILE_TAG_FRONT_LAYER);

	if (back != 0xFFFF)
		sprite->oam.tileNum = back;
	if (front != NULL && fr != 0xFFFF)
		front->oam.tileNum = fr;
	FreeOwnSheets(sprite);
}

//The rows from `from` down cleared (a sheet of `tilesPerRow` tiles a row).
static void ClearRowsFrom(u8* sheet, u32 from, u32 tilesPerRow)
{
	u32 v, tx;

	for (v = from; v < 64; ++v)
	{
		for (tx = 0; tx < tilesPerRow; ++tx)
			*(u32*) (sheet + ((v / 8) * tilesPerRow + tx) * TILE_SIZE_4BPP + (v % 8) * 4) = 0;
	}
}

//The foe's faint (FR SeqEnemyPokeKizetsuMain): every few frames the body goes 8 px down and loses its lowest shown
//tile row, so it sinks behind a fixed line. The own copies (copied whole at VBlank) lose the texel rows at and below
//that line.
static void FollowFaint(struct Sprite* sprite, struct Sprite* parent, struct Sprite* front)
{
	u8 pos = POS(sprite);
	u8* buffer;
	s32 shown = parent->data[3], cut, line;
	u16 startBack, startFront;

	if (!HAS_FLAG(sprite, AURA_FLAG_FAINT))
	{
		if (parent->pos2.y <= 0) //not moved yet: the shared sheets are the same picture
			return;
		if (HAS_FLAG(sprite, AURA_FLAG_DISSOLVE))
			UseSharedSheets(sprite, front);
		if (!UseOwnSheets(sprite, front, FALSE))
		{
			sprite->invisible = TRUE; //no room for the copy (not expected): hidden
			return;
		}
		SET_FLAG(sprite, AURA_FLAG_FAINT, TRUE); //DestroyAura frees both
		SET_FAINT_CUT(sprite, 64);
	}

	buffer = OwnBuffer(pos);
	startBack = GetSpriteTileStartByTag(OWN_TILE_TAG(pos, FALSE));
	startFront = GetSpriteTileStartByTag(OWN_TILE_TAG(pos, TRUE));
	if (buffer == NULL || startBack == 0xFFFF || startFront == 0xFFFF || front == NULL)
	{
		sprite->invisible = TRUE;
		return;
	}
	if (shown < 0)
		shown = 0;
	//the body's visible bottom from the body's centre: 8 * shown - 32 (px); the aura texel at that line
	line = ((8 * shown - 32) * 16 - sprite->sCy) * 16; //from the aura's centre, in texels * scale
	cut = 32 + ((line >= 0) ? line / sprite->sScale : -((-line + sprite->sScale - 1) / sprite->sScale));
	if (cut < 0)
		cut = 0;
	else if (cut > 64)
		cut = 64;
	if ((u32) cut < FAINT_CUT(sprite))
	{
		ClearRowsFrom(buffer, cut, 8);
		ClearRowsFrom(buffer + AURA_TILES_SIZE, cut, 4);
		RequestSpriteCopy(buffer, (u8*) (OBJ_VRAM0 + startBack * 32), AURA_TILES_SIZE);
		RequestSpriteCopy(buffer + AURA_TILES_SIZE, (u8*) (OBJ_VRAM0 + startFront * 32), FRONT_TILES_SIZE);
		SET_FAINT_CUT(sprite, cut);
	}
}

//---- dissolve (the front layers appear dot by dot, rising from the feet) ----------------------------------------------

//Each opaque pixel of the two front layers appears on its own frame of the reveal: its height above the pictures'
//lowest row spread over 18 frames, plus a fixed noise of 0-5 frames, so the pictures rise from the feet with a
//scattered, dotted edge. The writes per frame come from a table made from the pictures (src/shadow_aura_generic_dissolve.c,
//tools/make_aura_dissolve_table.py): a few dozen 32-bit ORs a frame. Every pixel is in by frame DISSOLVE_LAST; the copies
//land at the next VBlank, then the shared sheets take over.
#define DISSOLVE_LAST 22
#define DISSOLVE_PREPARE_FRAME 8 //frame of the searcher on which the empty own sheets are made (+ 4 for the right foe)
extern const u16 gHackAuraDissolveFirst[DISSOLVE_LAST + 2];
extern const u16 gHackAuraDissolveOffset[];
extern const u32 gHackAuraDissolveBits[];

static void DissolveFrame(u8* buffer, s32 f)
{
	u32 i;

	for (i = gHackAuraDissolveFirst[f]; i < gHackAuraDissolveFirst[f + 1]; ++i)
		*(u32*) (buffer + gHackAuraDissolveOffset[i]) |= gHackAuraDissolveBits[i];
}

//Tile rows of the own buffer that differ from VRAM copied at VBlank (also a copy dropped by a full queue). TRUE when
//VRAM equals the buffer.
static bool8 SyncOwnSheets(u8 pos)
{
	u8* buffer = OwnBuffer(pos);
	u16 startBack = GetSpriteTileStartByTag(OWN_TILE_TAG(pos, FALSE)), startFront = GetSpriteTileStartByTag(OWN_TILE_TAG(pos, TRUE));
	bool8 same = TRUE;
	u32 layer, row, i;

	if (buffer == NULL || startBack == 0xFFFF || startFront == 0xFFFF)
		return TRUE;
	for (layer = 0; layer < 2; ++layer)
	{
		u32 words = (layer == 0 ? 8 : 4) * TILE_SIZE_4BPP / 4; //a tile row
		const u32* b = (const u32*) (buffer + (layer == 0 ? 0 : AURA_TILES_SIZE));
		const u32* v = (const u32*) (OBJ_VRAM0 + (layer == 0 ? startBack : startFront) * TILE_SIZE_4BPP);
		for (row = 0; row < 8; ++row)
		{
			for (i = 0; i < words; ++i)
			{
				if (b[row * words + i] != v[row * words + i])
					break;
			}
			if (i < words)
			{
				RequestSpriteCopy((const u8*) &b[row * words], (u8*) &v[row * words], words * 4);
				same = FALSE;
			}
		}
	}
	return same;
}

//The reveal's start: empty own sheets (no dots yet). Without them the finished pictures are shown at once.
static void StartDissolve(struct Sprite* sprite, struct Sprite* front)
{
	if (UseOwnSheets(sprite, front, TRUE))
		SET_FLAG(sprite, AURA_FLAG_DISSOLVE, TRUE);
}

//Frame f of the reveal: the dots due now, then the copies; the shared sheets once everything is in.
static void UpdateDissolve(struct Sprite* sprite, struct Sprite* front, s32 f)
{
	u8* buffer = OwnBuffer(POS(sprite));

	if (!HAS_FLAG(sprite, AURA_FLAG_DISSOLVE) || buffer == NULL)
		return;
	if (f <= DISSOLVE_LAST)
	{
		DissolveFrame(buffer, f);
	}
	if (SyncOwnSheets(POS(sprite)) && f > DISSOLVE_LAST)
		UseSharedSheets(sprite, front);
}

//---- sounds ---------------------------------------------------------------------------------------------------------

static bool8 SongPlaying(struct MusicPlayerInfo* player, const struct SongHeader* song)
{
	return song != NULL && player->songHeader == song && (player->status & MUSICPLAYER_STATUS_TRACK)
		&& !(player->status & MUSICPLAYER_STATUS_PAUSE);
}

static void StartSequenceSound(struct Task* task)
{
	if (!SHADOW_AURA_GENERIC_SE_SEARCH && !SHADOW_AURA_GENERIC_SE_APPEAR)
		return;
	if (!(task->tSound & SOUND_REVERB_SAVED))
	{
		task->tReverb = SOUND_INFO_PTR->reverb; //the songs' headers (reverb byte 0x80) switch the reverb off
		task->tSound |= SOUND_REVERB_SAVED;
	}
	m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, AURA_CUE_BGM_VOLUME);
	task->tSound |= SOUND_BGM_LOWERED;
	if (SHADOW_AURA_GENERIC_SE_SEARCH)
	{
		MPlayStart(&gMPlayInfo_SE2, SEARCHER_SONG);
		m4aMPlayVolumeControl(&gMPlayInfo_SE2, 0xFFFF, AURA_SEARCHER_VOLUME);
	}
}

//The searcher louder than its channel volumes allow: its player's volume (kept while it plays).
static void BoostSearcher(void)
{
	if (SongPlaying(&gMPlayInfo_SE2, SEARCHER_SONG) && gMPlayInfo_SE2.tracks[0].volX != AURA_SEARCHER_VOLUME / 4)
		m4aMPlayVolumeControl(&gMPlayInfo_SE2, 0xFFFF, AURA_SEARCHER_VOLUME);
}

static void KeepBgmLowered(void)
{
	u8 taskId = FindManager();

	if (taskId < NUM_TASKS && (gTasks[taskId].tSound & SOUND_BGM_LOWERED)
	&& gMPlayInfo_BGM.tracks != NULL && gMPlayInfo_BGM.tracks[0].volX != AURA_CUE_BGM_VOLUME / 4)
		m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, AURA_CUE_BGM_VOLUME);
}

static void RestoreSound(struct Task* task)
{
	if (task->tSound & SOUND_BGM_LOWERED)
	{
		m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x100);
		task->tSound &= ~SOUND_BGM_LOWERED;
	}
	if (task->tSound & SOUND_REVERB_SAVED)
	{
		m4aSoundMode(SOUND_MODE_REVERB_SET | (u8) task->tReverb);
		if (SOUND_INFO_PTR->reverb == (u8) task->tReverb) //m4aSoundMode does nothing while the driver is busy: retried
			task->tSound &= ~SOUND_REVERB_SAVED;
	}
}

//The battle's end (end_battle.c): the battle music volume, the reverb and the faint copies given back if a sequence
//was cut short.
void HackAura_RestoreMixer(void)
{
	u8 taskId = FindManager();

	if (taskId < NUM_TASKS)
		RestoreSound(&gTasks[taskId]);
	FreeFaintBuffer(B_POSITION_OPPONENT_LEFT);
	FreeFaintBuffer(B_POSITION_OPPONENT_RIGHT);
}

//---- Lock-On --------------------------------------------------------------------------------------------------------

#define LOCKON_TEMPLATE_SIGHT ((const struct SpriteTemplate*) 0x083AAA5C)  //FR(J) a_normal.c RockOnHeader, 32x32
#define LOCKON_TEMPLATE_CORNER ((const struct SpriteTemplate*) 0x083AAA74) //RockOn2Header, 16x16, tiles 16-19
#define LOCKON_START_OFFSET 32 //corner centres from the target's centre at the start
#define LOCKON_END_OFFSET 8    //and at the end (FR(J) ROCKON_OUTOFFS_X / _Y)
#define LOCKON_APPEAR_FRAMES 4 //shown where it appears
#define LOCKON_MOVE_FRAMES 8   //then moved to the picture's middle
#define LOCKON_SHRINK_FRAMES 17
#define LOCKON_BLINK_START 44
#define LOCKON_BLINK_RATE 3    //FR(J) ROCKON_TENMETU
#define LOCKON_FRAMES 62
#define sLockCorner data[0]    //0 sight, 1-4 corners (FR(J) RockOn2Init: up-left, down-left, up-right, down-right)
#define sLockTimer data[1]
#define sLockX data[2]         //where it appears (the battler's coordinate)
#define sLockY data[3]
#define sLockToX data[4]       //the centroid of the picture's opaque pixels
#define sLockToY data[5]

static void SpriteCB_DetectLockOn(struct Sprite* sprite);

static u8 CountDetectLockOns(void)
{
	u32 i, n = 0;

	for (i = 0; i < MAX_SPRITES; ++i)
	{
		if (gSprites[i].inUse && gSprites[i].callback == SpriteCB_DetectLockOn)
			++n;
	}
	return n;
}

static void PlaceDetectLockOn(struct Sprite* sprite)
{
	s32 t = sprite->sLockTimer, d, m, x, y;
	static const s8 sSigns[5][2] = {{0, 0}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

	m = t - LOCKON_APPEAR_FRAMES; //moving
	if (m < 0)
		m = 0;
	else if (m > LOCKON_MOVE_FRAMES)
		m = LOCKON_MOVE_FRAMES;
	x = sprite->sLockX + (sprite->sLockToX - sprite->sLockX) * m / LOCKON_MOVE_FRAMES;
	y = sprite->sLockY + (sprite->sLockToY - sprite->sLockY) * m / LOCKON_MOVE_FRAMES;

	t -= LOCKON_APPEAR_FRAMES + LOCKON_MOVE_FRAMES; //shrinking
	if (t < 0)
		t = 0;
	else if (t > LOCKON_SHRINK_FRAMES)
		t = LOCKON_SHRINK_FRAMES;
	d = LOCKON_START_OFFSET - (LOCKON_START_OFFSET - LOCKON_END_OFFSET) * t / LOCKON_SHRINK_FRAMES;
	sprite->pos1.x = x + sSigns[sprite->sLockCorner][0] * d;
	sprite->pos1.y = y + sSigns[sprite->sLockCorner][1] * d;
}

//The screen position of the centroid of a battler sprite's opaque pixels (its picture), FALSE if it shows none.
static bool8 PictureCentroid(struct Sprite* parent, s16* outX, s16* outY)
{
	const u8* tiles = PictureOf(parent);
	u32 sumX = 0, sumY = 0, count = 0, x, y;
	bool8 hFlip = parent->oam.affineMode == ST_OAM_AFFINE_OFF && (parent->oam.matrixNum & 8);

	for (y = 0; y < 64; ++y)
	{
		for (x = 0; x < 64; ++x)
		{
			u8 b = tiles[((y / 8) * 8 + x / 8) * TILE_SIZE_4BPP + (y % 8) * 4 + (x % 8) / 2];
			if (((x & 1) ? b >> 4 : b & 0xF) == 0)
				continue;
			sumX += hFlip ? 63 - x : x;
			sumY += y;
			++count;
		}
	}
	if (count == 0)
		return FALSE;
	*outX = parent->pos1.x + parent->pos2.x + parent->centerToCornerVecX + (2 * sumX + count) / (2 * count);
	*outY = parent->pos1.y + parent->pos2.y + parent->centerToCornerVecY + (2 * sumY + count) / (2 * count);
	return TRUE;
}

static void SpriteCB_DetectLockOn(struct Sprite* sprite)
{
	++sprite->sLockTimer;
	PlaceDetectLockOn(sprite);
	if (sprite->sLockTimer >= LOCKON_BLINK_START && (sprite->sLockTimer - LOCKON_BLINK_START) % LOCKON_BLINK_RATE == 0)
		sprite->invisible ^= TRUE;
	if (sprite->sLockTimer < LOCKON_FRAMES)
		return;
	DestroySprite(sprite);
	if (CountDetectLockOns() == 0)
	{
		FreeSpriteTilesByTag(ANIM_TAG_LOCK_ON);
		FreeSpritePaletteByTag(ANIM_TAG_LOCK_ON);
	}
}

//At the sequence's start, with the searcher: the reticles on every foe position in pending (bits).
static void StartDetectLockOns(u8 pending)
{
	u32 pos, corner;
	bool8 loaded = FALSE;

	for (pos = B_POSITION_OPPONENT_LEFT; pos < MAX_BATTLERS_COUNT; pos += 2)
	{
		u8 parent = ParentSprite(pos), target;
		s16 x, y, toX, toY;

		if (!(pending & (1 << pos)) || parent >= MAX_SPRITES || gSprites[parent].invisible)
			continue; //nothing to aim at
		if (KindOfPos(pos) != HACK_KIND_DARK)
			continue; //[hack] patches/0956 2026-11-08 ユーザー: ロックオン（緑の照準）はダークだけ（原作ではスナッチマシンがダークを見つけた合図）。ヌシ・デルタはオーラと登場の文だけ
		if (!loaded)
		{
			const u32 index = ANIM_TAG_LOCK_ON - ANIM_SPRITES_START;
			if (GetSpriteTileStartByTag(ANIM_TAG_LOCK_ON) == 0xFFFF)
				LoadCompressedSpriteSheetUsingHeap(&gBattleAnimPicTable[index]);
			if (IndexOfSpritePaletteTag(ANIM_TAG_LOCK_ON) == 0xFF)
				LoadCompressedSpritePaletteUsingHeap(&gBattleAnimPaletteTable[index]);
			if (GetSpriteTileStartByTag(ANIM_TAG_LOCK_ON) == 0xFFFF || IndexOfSpritePaletteTag(ANIM_TAG_LOCK_ON) == 0xFF)
				break; //no room: no reticle (the sequence goes on with the searcher)
			loaded = TRUE;
		}
		target = GetBattlerAtPosition(pos);
		x = GetBattlerSpriteCoord(target, BATTLER_COORD_X);
		y = GetBattlerSpriteCoord(target, BATTLER_COORD_Y);
		if (!PictureCentroid(&gSprites[parent], &toX, &toY))
		{
			toX = x;
			toY = y;
		}
		for (corner = 0; corner < 5; ++corner)
		{
			struct SpriteTemplate template = *(corner == 0 ? LOCKON_TEMPLATE_SIGHT : LOCKON_TEMPLATE_CORNER);
			u8 id;

			template.callback = SpriteCB_DetectLockOn;
			id = CreateSprite(&template, x, y, 1); //in front of the battlers and the aura
			if (id >= MAX_SPRITES)
				continue;
			gSprites[id].sLockCorner = corner;
			gSprites[id].sLockTimer = 0;
			gSprites[id].sLockX = x;
			gSprites[id].sLockY = y;
			gSprites[id].sLockToX = toX;
			gSprites[id].sLockToY = toY;
			if (corner != 0)
			{
				static const u8 sFlips[5] = {0, 0, 16, 8, 24}; //RockOn2Init: OAM matrixNum bits = h / v flip
				gSprites[id].oam.tileNum += 16;
				gSprites[id].oam.matrixNum = sFlips[corner];
			}
			PlaceDetectLockOn(&gSprites[id]);
		}
	}
	if (loaded && CountDetectLockOns() == 0)
	{
		FreeSpriteTilesByTag(ANIM_TAG_LOCK_ON);
		FreeSpritePaletteByTag(ANIM_TAG_LOCK_ON);
	}
}

//---- reveal sequence ------------------------------------------------------------------------------------------------

//Whether another foe aura of this sequence is past the searcher (it started the aura sound for both).
static bool8 OtherAuraShown(struct Sprite* sprite)
{
	struct Task* task = &gTasks[FindManager()]; //SpriteCB_ShadowAura checked the manager
	u32 pos;

	for (pos = B_POSITION_OPPONENT_LEFT; pos < MAX_BATTLERS_COUNT; pos += 2)
	{
		u8 id = task->tAuraSprite(pos);
		if (pos != POS(sprite) && IsAuraSprite(id, pos) && gSprites[id].sReveal >= REVEAL_APPEAR)
			return TRUE;
	}
	return FALSE;
}

//The reveal's state for this frame: hidden or not, and its palette.
static void UpdateReveal(struct Sprite* sprite)
{
	struct Sprite* front = (FRONT(sprite) < MAX_SPRITES) ? &gSprites[FRONT(sprite)] : NULL;
	u16 colours[16];
	s32 f;

	if (sprite->sReveal >= REVEAL_SEARCH)
		KeepBgmLowered();
	switch (sprite->sReveal) {
		case REVEAL_WAIT:
			sprite->invisible = TRUE;
			return;
		case REVEAL_SEARCH:
			++sprite->sTimer;
			if (sprite->sTimer == DISSOLVE_PREPARE_FRAME + (POS(sprite) >> 1) * 4 && !HAS_FLAG(sprite, AURA_FLAG_DISSOLVE))
				StartDissolve(sprite, front); //the empty own sheets made while the searcher plays (hidden), not on the reveal's
				                              //busy first frame; two foes on different frames
			if (!OtherAuraShown(sprite)) //the first of the sequence waits for the searcher and the Lock-On, then starts the aura sound
			{
				bool8 searching = (sprite->sTimer < 2) ? SHADOW_AURA_GENERIC_SE_SEARCH != 0 : SongPlaying(&gMPlayInfo_SE2, SEARCHER_SONG);
				if (sprite->sTimer < AURA_SEQUENCE_MAX_FRAMES && (CountDetectLockOns() != 0 || searching))
				{
					BoostSearcher();
					sprite->invisible = TRUE;
					return;
				}
				if (SHADOW_AURA_GENERIC_SE_APPEAR)
					MPlayStart(&gMPlayInfo_SE1, APPEAR_SONG);
			}
			sprite->sReveal = REVEAL_APPEAR;
			sprite->sTimer = 0;
			if (!HAS_FLAG(sprite, AURA_FLAG_DISSOLVE))
				StartDissolve(sprite, front); //not made yet (a short searcher): now
			__attribute__((fallthrough)); //the reveal's first frame on this frame
		case REVEAL_APPEAR:
			f = sprite->sTimer++;
			UpdateDissolve(sprite, front, f);
			if (f >= PLANB_FRAMES)
			{
				bool8 playing = (f < PLANB_FRAMES + 2 && SHADOW_AURA_GENERIC_SE_APPEAR != 0) || SongPlaying(&gMPlayInfo_SE1, APPEAR_SONG);
				if (!playing || f >= AURA_SEQUENCE_MAX_FRAMES)
					sprite->sReveal = REVEAL_NONE;
				if (HAS_FLAG(sprite, AURA_FLAG_PLANB))
				{
					AuraColours(KindOfPos(POS(sprite)), FALSE, FALSE, colours);
					SetAuraColours(sprite, colours);
					SET_FLAG(sprite, AURA_FLAG_PLANB, FALSE);
				}
				return;
			}
			PlanBColours(KindOfPos(POS(sprite)), f, colours);
			SetAuraColours(sprite, colours);
			SET_FLAG(sprite, AURA_FLAG_PLANB, TRUE); //the pictures stay still (no transform of their own)
			return;
	}
}

//Whether a battler sprite is settled: FR's dummy callbacks after the intro / ball, healthbox shown, palette not faded.
static bool8 BattlerSettled(u8 pos, u8 parentId)
{
	u8 healthbox = gHealthboxSpriteIds[GetBattlerAtPosition(pos)];
	struct Sprite* parent = &gSprites[parentId];

	return (parent->callback == SpriteCallbackDummy || parent->callback == SpriteCallbackDummy3)
		&& healthbox < MAX_SPRITES && gSprites[healthbox].inUse && !gSprites[healthbox].invisible
		&& !PaletteFaded(parent->oam.paletteNum, 15);
}

static bool8 ReadyToReveal(struct Sprite* sprite)
{
	u8 pos = POS(sprite), parent = PARENT(sprite);

	return ParentSprite(pos) == parent && !gSprites[parent].invisible
		&& gBattleMons[GetBattlerAtPosition(pos)].hp != 0 && BattlerSettled(pos, parent)
		&& HAS_FLAG(sprite, AURA_FLAG_MEASURED);
}

//Whether every player battler on the field is out. Absent and fainted slots are not waited for; a Pokemon in the
//air / underground is invisible but out; Safari and the old man's tutorial send none.
static bool8 PlayerSideOut(void)
{
	u32 pos;

	if (gBattleTypeFlags & (BATTLE_TYPE_SAFARI | BATTLE_TYPE_OLD_MAN))
		return TRUE;
	for (pos = B_POSITION_PLAYER_LEFT; pos < MAX_BATTLERS_COUNT; pos += 2)
	{
		u8 bank = GetBattlerAtPosition(pos);
		u8 parent;

		if (bank >= gBattlersCount || (gAbsentBattlerFlags & (1 << bank)) || gBattleMons[bank].hp == 0)
			continue;
		parent = ParentSprite(pos);
		if (parent >= MAX_SPRITES || (gSprites[parent].invisible && !(gStatuses3[bank] & STATUS3_SEMI_INVULNERABLE))
		|| !BattlerSettled(pos, parent))
			return FALSE;
	}
	return TRUE;
}

static bool8 BeforePlayerOut(void)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sIntroBeforePlayerOut); ++i)
	{
		if ((u32) gBattleMainFunc == sIntroBeforePlayerOut[i])
			return TRUE;
	}
	return FALSE;
}

//gText_ShadowAuraFound to the local player's controller, as PrepareStringBattle(0x184, bank) but leaving the server's
//gActiveBattler and gBattleStringLoader as they were.
//登場の文: 相手側で印のある最初の子の種類の文（同時に出た相手は 1 回の文を共有する）。仮の文（strings/hack_aura.string）
static const u8* FoundMessage(void)
{
	u8 pos;

	for (pos = B_POSITION_OPPONENT_LEFT; pos <= B_POSITION_OPPONENT_RIGHT; ++pos)
	{
		switch (KindOfPos(pos)) {
			case HACK_KIND_NUSHI: return gText_HackAuraFoundNushi;
			case HACK_KIND_DELTA: return gText_HackAuraFoundDelta;
			case HACK_KIND_DARK: return gText_HackAuraFoundDark;
		}
	}
	return gText_HackAuraFoundDark;
}

static void SendFoundMessage(void)
{
	u8 active = gActiveBattler;
	const u8* loader = gBattleStringLoader;

	gBattleStringLoader = FoundMessage();
	gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
	EmitPrintString(0, STRINGID_STRING_LOADER);
	MarkBufferBankForExecution(gActiveBattler);
	gActiveBattler = active;
	gBattleStringLoader = loader;
}

static u8 UpdateMessage(struct Task* task)
{
	switch (task->tMessage) {
		case MESSAGE_PENDING:
			if (gBattleTypeFlags & BATTLE_TYPE_LINK) //the command would go over the link
				break;
			if (gBattleExecBuffer != 0)
				return HOLD_SERVER;
			SendFoundMessage();
			task->tMessage = MESSAGE_PRINTING;
			return HOLD_SERVER;
		case MESSAGE_PRINTING:
			if (gBattleExecBuffer == 0) //the controller has printed it
				task->tMessage = MESSAGE_WAIT;
			return HOLD_SERVER;
		default:
			if (++task->tMessage < MESSAGE_WAIT + MOJI_WAIT)
				return HOLD_SERVER;
			break;
	}
	task->tMessage = MESSAGE_NONE;
	return HOLD_NONE;
}

static u8 HoldBattle(void)
{
	u8 taskId = FindOrCreateManager(); //the aura manager lives as long as the battle (any battle: the Dark foe may come later)
	struct Task* task;
	bool8 start = FALSE;
	u8 lockOn = 0;
	u32 pos;

	if (taskId >= NUM_TASKS)
		return HOLD_NONE;
	task = &gTasks[taskId];
	if (!InBattleScreen())
	{
		RestoreSound(task); //no sequence can run
		return HOLD_NONE;
	}

	for (pos = B_POSITION_OPPONENT_LEFT; pos < MAX_BATTLERS_COUNT; pos += 2)
	{
		u8 id = task->tAuraSprite(pos);
		if (IsAuraSprite(id, pos) && gSprites[id].sReveal >= REVEAL_SEARCH)
			return HOLD_ALL;
	}
	RestoreSound(task); //no aura in its sequence

	if (task->tMessage != MESSAGE_NONE && UpdateMessage(task) != HOLD_NONE)
		return HOLD_SERVER;

	if (gBattleExecBuffer != 0 || BeforePlayerOut() || !PlayerSideOut())
		return HOLD_NONE;

	for (pos = B_POSITION_OPPONENT_LEFT; pos < MAX_BATTLERS_COUNT; pos += 2)
	{
		u8 id = task->tAuraSprite(pos);
		if (IsAuraSprite(id, pos) && gSprites[id].sReveal == REVEAL_WAIT && ReadyToReveal(&gSprites[id]))
		{
			gNewBS->hackAuraRevealed |= 1 << gBattlerPartyIndexes[GetBattlerAtPosition(pos)]; //never again this battle
			lockOn |= 1 << pos;
			gSprites[id].sReveal = REVEAL_SEARCH;
			gSprites[id].sTimer = 0;
			SET_FLAG(&gSprites[id], AURA_FLAG_MEASURED, FALSE); //measured again from the Pokemon now out
			start = TRUE;
		}
	}
	if (!start)
		return HOLD_NONE;

	task->tMessage = MESSAGE_PENDING;
	StartSequenceSound(task);
	StartDetectLockOns(lockOn); //the reticles start with the searcher
	return HOLD_ALL;
}

//FR(J) ServerMain 0x08011C34 (hooks): ServerSequence, then every client. A foe aura's sequence skips both; its message
//skips only ServerSequence, so the controller printing it runs.
void HackAura_ServerMain(void)
{
	u8 hold;

	hold = HoldBattle();

	if (hold != HOLD_ALL)
	{
		if (hold == HOLD_NONE)
			gBattleMainFunc();
		for (gActiveBattler = 0; gActiveBattler < gBattlersCount; ++gActiveBattler)
			gBattlerControllerFuncs[gActiveBattler]();
	}
}

//FR(J) waza_eff.c AutoPokeTailingSeq: while a move animation shows a battler on a BG its sprite is invisible and this
//task (data[0] = the sprite) moves the BG copy with it.
#define MON_BG_TASK ((TaskFunc) (0x08072918 | 1))
//The priority of the BG that shows this battler sprite while a move animation draws it there (AutoPokeTailingSeq:
//data[5] FALSE = BG1, TRUE = BG2), or 0xFF. Its sprite is invisible meanwhile; the aura stays, but at the BG's
//priority an OBJ is drawn over the BG copy: the layers behind the body go one priority back (otherwise, e.g. during
//the Sand Stream animation after the ability pop-up, the aura would cover the body's middle rows).
static u8 ShownAsBgPriority(u8 spriteId)
{
	u32 i;

	for (i = 0; i < NUM_TASKS; ++i)
	{
		if (gTasks[i].isActive && gTasks[i].func == MON_BG_TASK && gTasks[i].data[0] == spriteId)
			return GetGpuReg(gTasks[i].data[5] ? REG_OFFSET_BG2CNT : REG_OFFSET_BG1CNT) & 3;
	}
	return 0xFF;
}

//The front layer: set up by its main sprite's callback (which runs before it); gone with it.
static void SpriteCB_ShadowAuraFront(struct Sprite* sprite)
{
	u8 mainId = sprite->sLayerMain;

	if (mainId >= MAX_SPRITES || !gSprites[mainId].inUse || gSprites[mainId].callback != SpriteCB_ShadowAura
	|| FRONT(&gSprites[mainId]) != (u8) (sprite - gSprites))
		DestroySprite(sprite);
}

static void SpriteCB_ShadowAura(struct Sprite* sprite)
{
	u8 self = sprite - gSprites;
	u8 pos = POS(sprite);
	u8 taskId = FindManager();
	struct Sprite* parent;
	struct Sprite* front;
	bool8 hidden;
	u8 bgPriority;

	if (taskId >= NUM_TASKS || gTasks[taskId].tAuraSprite(pos) != self) //not owned any more (the task was reset)
	{
		DestroyAura(sprite);
		return;
	}

	front = (FRONT(sprite) < MAX_SPRITES) ? &gSprites[FRONT(sprite)] : NULL;
	if (!InBattleScreen() || ParentSprite(pos) != PARENT(sprite))
	{
		sprite->invisible = TRUE; //the task destroys it on its next run
		if (front != NULL)
			front->invisible = TRUE;
		return;
	}
	parent = &gSprites[PARENT(sprite)];

	if (!HAS_FLAG(sprite, AURA_FLAG_MEASURED))
	{
		s16 cx, cy, scale;
		if (MeasureFit(parent, &cx, &cy, &scale))
		{
			sprite->sCx = cx;
			sprite->sCy = cy;
			sprite->sScale = scale;
			SET_FLAG(sprite, AURA_FLAG_MEASURED, TRUE);
		}
	}

	bgPriority = ShownAsBgPriority(PARENT(sprite));
	hidden = (parent->invisible && bgPriority == 0xFF) || parent->oam.objMode == ST_OAM_OBJ_WINDOW
		|| !HAS_FLAG(sprite, AURA_FLAG_MEASURED);
	sprite->invisible = hidden;
	UpdateRed(sprite, IsBattlerReverse(GetBattlerAtPosition(pos)), HackKind_AuraKind(GetBattlerAtPosition(pos)));

	if (sprite->sReveal != REVEAL_NONE)
		UpdateReveal(sprite);

	if (HAS_FLAG(sprite, AURA_FLAG_MEASURED)
	&& !PlaceAura(sprite, front, parent, sprite->sCx, sprite->sCy, sprite->sScale, MATRIX(sprite)))
		sprite->invisible = TRUE;
	sprite->subpriority = (parent->subpriority < 0xFF) ? parent->subpriority + 1 : 0xFF;
	if (bgPriority != 0xFF) //the body is drawn on a BG: the back layer behind that BG, the front layer over it
	{
		if (bgPriority >= 3)
			sprite->invisible = TRUE;
		else
			sprite->oam.priority = bgPriority + 1;
		if (front != NULL)
			front->oam.priority = bgPriority;
	}

	if (!HAS_FLAG(sprite, AURA_FLAG_BACK) && HAS_FLAG(sprite, AURA_FLAG_MEASURED)
	&& (parent->callback == SeqEnemyPokeKizetsuMain || HAS_FLAG(sprite, AURA_FLAG_FAINT)))
		FollowFaint(sprite, parent, front);
	UpdateFade(sprite, parent);

	if (front != NULL)
	{
		front->invisible = sprite->invisible;
		front->subpriority = (parent->subpriority > 0) ? parent->subpriority - 1 : 0;
	}
}


//---- つよさをみる（BW 版）のオーラ -----------------------------------------------------------------------------------
//[hack] patches/0966 FEAT-shadow-h（段 H）: つよさをみる画面のポケモンの絵の後ろ（前）にオーラを出す。絵・パレットの作り方・
//位置合わせ（MeasureFit・PlaceAura）は戦闘と同じ物を使う。戦闘用の task・登場の演出・音は使わない（絵に毎フレーム付いてくるだけ）。
//呼ぶのは src/bw_summary_screen.c（絵を作ったあとと解放の前）。状態はスプライトだけが持つ（書き込める static は使えない）。
#define SUMMARY_TAG_BACK 0x5B10
#define SUMMARY_TAG_FRONT 0x5B11
#define SUMMARY_PAL_TAG 0x5B12
#define SUMMARY_AURA_PRIORITY 1 //文字 BG0 と絵（どちらも 0）の 1 つ後ろ

static void SpriteCB_SummaryAura(struct Sprite* sprite);
static void SpriteCB_SummaryAuraFront(struct Sprite* sprite);

static const struct SpriteTemplate sSummaryBackTemplate = AURA_TEMPLATE(SUMMARY_TAG_BACK, sAuraOam, SpriteCB_SummaryAura);
static const struct SpriteTemplate sSummaryFrontTemplate = AURA_TEMPLATE(SUMMARY_TAG_FRONT, sFrontOam, SpriteCB_SummaryAuraFront);

static void SpriteCB_SummaryAuraFront(struct Sprite* sprite)
{
	(void) sprite; //前の層は本体の callback が動かす
}

static u8 FindSummaryAura(void)
{
	u32 i;

	for (i = 0; i < MAX_SPRITES; ++i)
	{
		if (gSprites[i].inUse && gSprites[i].callback == SpriteCB_SummaryAura)
			return i;
	}
	return MAX_SPRITES;
}

static void SpriteCB_SummaryAura(struct Sprite* sprite)
{
	struct Sprite* parent = &gSprites[PARENT(sprite)];
	struct Sprite* front = (FRONT(sprite) < MAX_SPRITES) ? &gSprites[FRONT(sprite)] : NULL;

	if (!parent->inUse)
	{
		sprite->invisible = TRUE;
		if (front != NULL)
			front->invisible = TRUE;
		return;
	}
	if (!HAS_FLAG(sprite, AURA_FLAG_MEASURED))
	{
		s16 cx, cy, scale;
		if (MeasureFit(parent, &cx, &cy, &scale))
		{
			sprite->sCx = cx;
			sprite->sCy = cy;
			sprite->sScale = scale;
			SET_FLAG(sprite, AURA_FLAG_MEASURED, TRUE);
		}
	}
	sprite->invisible = parent->invisible || !HAS_FLAG(sprite, AURA_FLAG_MEASURED);
	if (HAS_FLAG(sprite, AURA_FLAG_MEASURED) && !PlaceAura(sprite, front, parent, sprite->sCx, sprite->sCy, sprite->sScale, MATRIX(sprite)))
		sprite->invisible = TRUE;
	//[hack] patches/0983 BUG-summary-aura-layer（ユーザー 2026-11-09）: 2 層とも絵と文字（BG0・priority 0）の後ろへ。BG3（技の枠・1）・BG1（格子・2）・BG2（背景・3）より前
	//（OBJ は同じ priority の BG に勝つ）。絵は priority 0（実機で測定）。PlaceAura が毎フレーム絵の priority をコピーするのでその後で上書きする（戦闘の SpriteCB_ShadowAura には影響しない）
	sprite->oam.priority = SUMMARY_AURA_PRIORITY;
	sprite->subpriority = (parent->subpriority < 0xFF) ? parent->subpriority + 1 : 0xFF;
	if (front != NULL)
	{
		front->invisible = sprite->invisible;
		front->oam.priority = SUMMARY_AURA_PRIORITY;
		front->subpriority = (parent->subpriority > 0) ? parent->subpriority - 1 : 0;
	}
}

//つよさをみるの絵（parentId）にオーラを付ける。kind は HACK_KIND_*（NONE なら何もしない）。前のオーラは消す
void HackAura_SummaryAttach(u8 parentId, u8 kind)
{
	u16 colours[16];
	struct SpritePalette pal = {colours, SUMMARY_PAL_TAG};
	u8 id, front = MAX_SPRITES, palSlot, matrix;

	HackAura_SummaryDetach();
	if (kind == HACK_KIND_NONE || parentId >= MAX_SPRITES || !gSprites[parentId].inUse)
		return;
	if (!LoadAuraSheet(dark_aura_back_layerTiles, SUMMARY_TAG_BACK) || !LoadFrontSheet(dark_aura_front_layerTiles, SUMMARY_TAG_FRONT))
		goto fail;
	AuraColours(kind, FALSE, FALSE, colours);
	palSlot = IndexOfSpritePaletteTag(SUMMARY_PAL_TAG);
	if (palSlot != 0xFF)
		LoadPalette(colours, 0x100 + palSlot * 16, sizeof(colours));
	else if ((palSlot = HackScreens_LoadPalette(&pal, gSprites[parentId].oam.paletteNum)) == 0xFF)   //絵の枠は避ける（絵の色を上書きしない）
		goto fail;
	matrix = AllocOamMatrix();
	if (matrix >= 32)
		goto fail;
	id = CreateSprite(&sSummaryBackTemplate, gSprites[parentId].pos1.x, gSprites[parentId].pos1.y, gSprites[parentId].subpriority);
	if (id < MAX_SPRITES)
	{
		front = CreateSprite(&sSummaryFrontTemplate, gSprites[parentId].pos1.x, gSprites[parentId].pos1.y, gSprites[parentId].subpriority);
		if (front >= MAX_SPRITES)
		{
			DestroySprite(&gSprites[id]);
			id = MAX_SPRITES;
		}
	}
	if (id >= MAX_SPRITES)
	{
		FreeOamMatrix(matrix);
		goto fail;
	}
	gSprites[id].oam.paletteNum = palSlot;
	gSprites[id].sPosParent = parentId << 8;
	gSprites[id].sFrontMatrix = front | (matrix << 8);
	gSprites[id].sFlags = 0;
	gSprites[id].invisible = TRUE;
	gSprites[id].affineAnimPaused = TRUE;
	gSprites[id].animPaused = TRUE;
	gSprites[front].oam.paletteNum = palSlot;
	gSprites[front].invisible = TRUE;
	gSprites[front].affineAnimPaused = TRUE;
	gSprites[front].animPaused = TRUE;
	return;
fail:
	HackAura_SummaryDetach();
}

void HackAura_SummaryDetach(void)
{
	u8 id = FindSummaryAura();

	if (id < MAX_SPRITES)
	{
		struct Sprite* sprite = &gSprites[id];

		if (FRONT(sprite) < MAX_SPRITES && gSprites[FRONT(sprite)].inUse && gSprites[FRONT(sprite)].callback == SpriteCB_SummaryAuraFront)
			DestroySprite(&gSprites[FRONT(sprite)]);
		if (MATRIX(sprite) < 32)
			FreeOamMatrix(MATRIX(sprite));
		DestroySprite(sprite);
	}
	FreeSpriteTilesByTag(SUMMARY_TAG_BACK);
	FreeSpriteTilesByTag(SUMMARY_TAG_FRONT);
	FreeSpritePaletteByTag(SUMMARY_PAL_TAG);
}

#endif
