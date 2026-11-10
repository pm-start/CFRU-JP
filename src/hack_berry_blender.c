#include "defines.h"
#include "../include/bg.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/menu.h"
#include "../include/palette.h"
#include "../include/random.h"
#include "../include/sound.h"
#include "../include/m4a.h"
#include "../include/sprite.h"
#include "../include/decompress.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/text_window.h"
#include "../include/window.h"
#include "../include/trig.h"
#include "../include/event_data.h"
#include "../include/item_menu.h"
#include "../include/overworld.h"
#include "../include/field_message_box.h"
#include "../include/scanline_effect.h"
#include "../include/international_string_util.h"
#include "../include/constants/songs.h"
#include "../include/constants/items.h"
#include "../include/new/Vanilla_functions.h"
#include "Tables/hoenn_script_flags.h"          //HACK_FLAG_PB_BLENDER_SKIP（段 4c）
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"
#include "hack_blender_calc.h"
#include "hack_berry_blender.h"

/*
 * きのみブレンダーの画面（2026-11-09 FEAT-pokeblock 段 4b1・patches/0978）。
 * 原文: 参考/pokeemerald/src/berry_blender.c（画面・矢印・投げ入れ・カウントダウン・入力・進行バー・RPM＝L957-L1806・L2084-L2240・L3097-L3447）。日本語版エメラルドの同じ画面と同じ動き。
 * 通信（段 7）は持ち込まない。
 * 原文との差:
 *   - 渡し物・状態は原文のグローバル（sBerryBlender・gSpecialVar_0x8004・gSendCmd／gRecvCmds・gLinkPlayers）ではなく struct BerryBlender（Alloc した入れ物）＋ポインタの語 0x203E038。
 *     書き換え可能な static は ROM に置かれる（6.13）。通信用の gSendCmd／gRecvCmds は「得点の知らせ」pendingScore[4] に置き換えた（原文の local 側は同じ値を自分に渡しているだけ）。
 *   - 名人かどうかは台本の旗ではなく引数 master（HackBlender_Start）。相手の名前は日本語版の 6 本（strings/hack_pokeblock.string）。
 *   - きのみを選ぶ画面: ChooseBerryForMachine → GoToBerryMenu(BERRY_POUCH_CHOOSE_ITEM)（素の FR の BERRYPOUCH_FROMBERRYCRUSH＝B で抜けられない・A で即決と同じ動き）。
 *     この改造で足したきのみ（味の表が無い）を選んだら SE_FAILURE を鳴らして袋を開き直す（文は出さない）。
 *   - 素の FR に無い関数: SetBgAffine は BIOS の BgAffineSet（swi 0x0E）を直に呼んで BG2 のレジスタへ書く（原文 SetBgAffine と同じ値）。
 *     m4aMPlayPitchControl／m4aMPlayTempoControl は原文（m4a.c）と同じ中身をここに持つ。きのみの絵は ArrangeBerryGfx の並べ替え済みを LoadSpriteSheet（タグはプレイヤーごと）で読む。
 *   - 原文のバグ（UpdateOpponentScores の gRecvCmds[BLENDER_COMM_SCORE][i]）は直さず原文どおり（BUGFIX 無しの側）。
 *   - 4b2: NPC の入力・結果・ランキング・ファンファーレ・もう1回を移植。文字の位置・窓は日本語版 ROM の値（US と違う）。通信と GAME_STAT／TV／毎日のブレンド（FR に無い）は持ち込まない。
 *     原文との差（Commander 承認）: ポロックを入れられないとき「ポロックケースが いっぱいの ようです」・きのみ無しの判定は混ぜられる 43 種を CheckBagHasItem で。
 */

#define BBL_FONT 2
#define BBL_ALL_PALETTES 0xFFFFFFFFu
#define BG_SCREEN_SIZE 0x800
#define BBL_MAX_PLAYERS 4
#define BBL_NAME_LEN 12

//素の FR（JP）の番地（BPRJ.ld に無いもの・hack_pokeblock_feed.c と同じ）
#define Bbl_SetVBlankHBlankCallbacksToNull ((void (*)(void))(0x080C0938 + 1))
void TransferPlttBuffer(void);                 //src/dns.c
void __attribute__((long_call)) UnsetBgTilemapBuffer(u8 bg);   //bg.h ではコメントの中・BPRJ.ld にはある

//効果音（素の FR の値・エメラルドとは違う。参考/pokefirered/include/constants/songs.h と同じ内部名を突き合わせた）
#define BBL_SE_PIN 21
#define BBL_SE_SUCCESS 25
#define BBL_SE_FAILURE 26
#define BBL_SE_ICE_STAIRS 34
#define BBL_SE_FALL 37
#define BBL_SE_TRUCK_DOOR 46
#define BBL_SE_BERRY_BLENDER 47
#define BBL_SE_BALL_BOUNCE_1 49
#define BBL_SE_BALL_TRAY_EXIT 109

enum { SCORE_BEST, SCORE_GOOD, SCORE_MISS, NUM_SCORE_TYPES };
enum { PROXIMITY_MISS, PROXIMITY_GOOD, PROXIMITY_BEST };
enum { SCOREANIM_GOOD, SCOREANIM_MISS, SCOREANIM_BEST_FLASH, SCOREANIM_BEST_STATIC };
enum { BLENDER_MISTER, BLENDER_LADDIE, BLENDER_LASSIE, BLENDER_MASTER, BLENDER_DUDE, BLENDER_MISS };

#define LINKCMD_BLENDER_SCORE_MISS 0x2345
#define LINKCMD_BLENDER_SCORE_BEST 0x4523
#define LINKCMD_BLENDER_SCORE_GOOD 0x5432

#define NO_PLAYER 0xFF
#define MAX_PROGRESS_BAR 1000
#define MAX_ARROW_POS 0x10000
#define MIN_ARROW_SPEED 0x80
#define ARROW_FALL_ROTATION 0x5800

#define PROGRESS_BAR_FILLED_TOP    0x80E9
#define PROGRESS_BAR_FILLED_BOTTOM 0x80F9
#define PROGRESS_BAR_EMPTY_TOP     0x80E1
#define PROGRESS_BAR_EMPTY_BOTTOM  0x80F1
#define RPM_DIGIT 0x8072

#define GFXTAG_COUNTDOWN_NUMBERS 12345
#define GFXTAG_START             12346
#define GFXTAG_PARTICLES         23456
#define GFXTAG_PLAYER_ARROW      46545
#define GFXTAG_SCORE_SYMBOLS     48888
#define PALTAG_PLAYER_ARROW      12312
#define PALTAG_MISC              46546
#define GFXTAG_BERRY_PIC         0xB0A0      //＋プレイヤーの番号（4 本）
#define PALTAG_BERRY_PIC         0xB0B0

enum { WIN_MSG = BBL_MAX_PLAYERS, WIN_RESULTS };

struct BerryBlender
{
	u8 mainState;
	u8 loadGfxState;
	u16 arrowPos;
	s16 speed;
	u16 maxRPM;
	u8 playerArrowSpriteIds[BBL_MAX_PLAYERS];
	u8 playerArrowSpriteIds2[BBL_MAX_PLAYERS];
	u8 gameEndState;
	u8 slowdownTimer;
	u16 chosenItemId[BBL_MAX_PLAYERS];
	u8 numPlayers;
	u16 arrowIdToPlayerId[BBL_MAX_PLAYERS];
	u16 playerIdToArrowId[BBL_MAX_PLAYERS];
	u32 gameFrameTime;
	s32 framesToWait;
	u8 playerToThrowBerry;
	u16 progressBarValue;
	u16 maxProgressBarValue;
	u16 centerScale;
	s16 bg_X;
	s16 bg_Y;
	u8 opponentTaskIds[BBL_MAX_PLAYERS - 1];       //4b2（NPC の入力のタスク）
	u8 perfectOpponents;                           //原文はデバッグ専用（配る版では常に FALSE）
	u16 scores[BBL_MAX_PLAYERS][NUM_SCORE_TYPES];
	struct BgAffineSrcData bgAffineSrc;
	u16 savedMusic;
	struct BlenderBerry blendedBerries[BBL_MAX_PLAYERS];
	u8 tilemapBuffers[2][BG_SCREEN_SIZE];
	s16 textState;
	void* tilesBuffer;
	//ここから原文に無い入れ物
	u16 pendingScore[BBL_MAX_PLAYERS];             //得点の知らせ（原文の gRecvCmds[i][BLENDER_COMM_SCORE]・0 = なし）
	u8 opponents;                                  //NPC の数（原文 gSpecialVar_0x8004）
	bool8 master;                                  //名人
	MainCallback exitCallback;
	u8 names[BBL_MAX_PLAYERS][BBL_NAME_LEN];       //名前（原文 gLinkPlayers[].name）
	struct SpriteTemplate berryTemplate[BBL_MAX_PLAYERS];
	u8 berryTiles[0x800];                          //並べ替えたきのみの絵（LoadSpriteSheet がすぐ VRAM へ写す）
	//4b2（結果・ランキング・もう1回）
	u8 scoreIconIds[NUM_SCORE_TYPES];
	u8 playerPlaces[BBL_MAX_PLAYERS];
	u8 yesNoAnswer;
	u8 playAgainState;
	bool8 skipped;                                 //段 4c: ミニゲームを飛ばして結果へ来た（せっていの行・旗 HACK_FLAG_PB_BLENDER_SKIP）＝得点の絵が無い・速さの記録は更新しない
	bool8 caseFull;                                //できたポロックを入れられなかった（原文に無い・Q1）
	u8 stringVar[0x80];
};

#define sBerryBlender (*((struct BerryBlender**) BBL_STATE_PTR))

extern const u8 gBbl_Center_Gfx[], gBbl_Center_Pal[], gBbl_Center_Map[], gBbl_Outer_Gfx[], gBbl_Outer_Map[], gBbl_Outer_Pal[];
extern const u8 gBbl_Arrow_Gfx[], gBbl_Arrow_Pal[], gBbl_Particles_Gfx[], gBbl_ScoreSymbols_Gfx[], gBbl_Countdown_Gfx[], gBbl_Start_Gfx[], gBbl_Misc_Pal[];
extern const u8 gBbl_BerryPics[], gBbl_BerryPals[];
extern const u8 gText_Bbl_Start[], gText_Bbl_NameMister[], gText_Bbl_NameLaddie[], gText_Bbl_NameLassie[], gText_Bbl_NameMaster[], gText_Bbl_NameDude[], gText_Bbl_NameMiss[];
extern const u8 gText_Bbl_WasMade[], gText_Bbl_NewLine[], gText_Bbl_NewParagraph[], gText_Bbl_TheLevelIs[], gText_Bbl_TheFeelIs[], gText_Bbl_Dot2[], gText_Bbl_Dot[];
extern const u8 gText_Bbl_Ranking[], gText_Bbl_Results[], gText_Bbl_Time[], gText_Bbl_Min[], gText_Bbl_Sec[], gText_Bbl_MaxSpeed[], gText_Bbl_RPM[];
extern const u8 gText_Bbl_AnotherBerry[], gText_Bbl_RunOutOfBerries[], gText_Bbl_CaseFull[];
extern const u8 gText_Bbl_RecordTitle[], gText_Bbl_RecordPlayers[];

//日本語版エメラルド ROM の表（0x30FC29〜: 結果画面の行の y の表 3 本）。検算が ROM と突き合わせる
#define BBL_TEXT_SKIP_DRAW 0xFF
#define BBL_SE_SELECT 5
#define BBL_MUS_LEVEL_UP 257                    //素の FR の MUS_FANFA1（エメラルドの MUS_LEVEL_UP と同じ内部名）
static const u8 sResultsRowBase[BBL_MAX_PLAYERS + 1] = {6, 6, 6, 6, 5};      //0x30FC29（人数で引く・0〜1 は使わない）
static const u8 sResultsRowStep[BBL_MAX_PLAYERS + 1] = {3, 3, 3, 2, 2};      //0x30FC2E
static const u8 sRankingRowStep[BBL_MAX_PLAYERS + 1] = {3, 3, 3, 3, 2};      //0x30FC33
static const struct WindowTemplate sYesNoWindowTemplate =                     //日本語版 0x30F8A8（US は baseBlock 0xCC・日本語版は 0xB8）
{
	.bg = 0, .tilemapLeft = 21, .tilemapTop = 9, .width = 5, .height = 4, .paletteNum = 14, .baseBlock = 0xB8
};
void __attribute__((long_call)) CreateYesNoMenu(const struct WindowTemplate* window, u8 fontId, u8 left, u8 top, u16 baseTileNum, u8 paletteNum, u8 initialCursorPos);   //BPRJ.ld

static void SetBgPos(void);
static void SpriteCB_PlayerArrow(struct Sprite*);
static void SpriteCB_ScoreSymbol(struct Sprite*);
static void SpriteCB_CountdownNumber(struct Sprite*);
static void SpriteCB_Start(struct Sprite*);
static void SpriteCB_ScoreSymbolBest(struct Sprite*);
static void CB2_LoadBerryBlender(void);
static void UpdateBlenderCenter(void);
static void StartBlender(void);
static void CB2_StartBlenderLocal(void);
static void CB2_PlayBlender(void);
static void CB2_EndBlenderGame(void);
static void SortScores(void);
static void CB2_CheckPlayAgainLocal(void);
static void Task_HandleOpponent1(u8 taskId);
static void Task_HandleOpponent2(u8 taskId);
static void Task_HandleOpponent3(u8 taskId);
static void Task_HandleBerryMaster(u8 taskId);
static void DrawBlenderCenter(struct BgAffineSrcData*);
static bool8 UpdateBlenderLandScreenShake(void);
static void CreateParticleSprites(void);
static void UpdateProgressBar(u16 value, u16 limit);
static void TryUpdateProgressBar(u16 current, u16 limit);
static void UpdateRPM(u16 speed);

static const struct BgTemplate sBgTemplates[3] =
{
	{ .bg = 0, .charBaseIndex = 3, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0 },
	{ .bg = 1, .charBaseIndex = 2, .mapBaseIndex = 12, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0 },
	{ .bg = 2, .charBaseIndex = 0, .mapBaseIndex = 8,  .screenSize = 1, .paletteMode = 1, .priority = 0, .baseTile = 0 },
};

static const struct WindowTemplate sWindowTemplates[] =
{
	{ .bg = 0, .tilemapLeft = 1,  .tilemapTop = 6,  .width = 7,  .height = 2, .paletteNum = 14, .baseBlock = 0x28 },   // Player 1
	{ .bg = 0, .tilemapLeft = 22, .tilemapTop = 6,  .width = 7,  .height = 2, .paletteNum = 14, .baseBlock = 0x36 },   // Player 2
	{ .bg = 0, .tilemapLeft = 1,  .tilemapTop = 12, .width = 7,  .height = 2, .paletteNum = 14, .baseBlock = 0x44 },   // Player 3
	{ .bg = 0, .tilemapLeft = 22, .tilemapTop = 12, .width = 7,  .height = 2, .paletteNum = 14, .baseBlock = 0x52 },   // Player 4
	[WIN_MSG] = { .bg = 0, .tilemapLeft = 4, .tilemapTop = 15, .width = 22, .height = 4, .paletteNum = 14, .baseBlock = 0x60 },
	[WIN_RESULTS] = { .bg = 0, .tilemapLeft = 5, .tilemapTop = 3, .width = 21, .height = 14, .paletteNum = 14, .baseBlock = 0x60 },
	DUMMY_WIN_TEMPLATE
};

static const s8 sPlayerArrowQuadrant[BBL_MAX_PLAYERS][2] =
{
	{-1, -1}, { 1, -1}, {-1,  1}, { 1,  1}
};

static const u8 sPlayerArrowPos[BBL_MAX_PLAYERS][2] =
{
	{ 72,  32}, {168,  32}, { 72, 128}, {168, 128}
};

static const u8 sPlayerIdMap[BBL_MAX_PLAYERS - 1][BBL_MAX_PLAYERS] =
{
	{NO_PLAYER, 0, 1, NO_PLAYER}, // 2 Players
	{NO_PLAYER, 0, 1,         2}, // 3 Players
	{        0, 1, 2,         3}  // 4 Players
};

// 矢印の位置: 0x0000 が上・0x4000 が左・0x8000 が下・0xC000 が右（原文のコメントのとおり）
static const u16 sArrowStartPos[] =
{
	0,
	MAX_ARROW_POS / 4 * 3,
	MAX_ARROW_POS / 4,
	MAX_ARROW_POS / 4 * 2
};
static const u8 sArrowStartPosIds[BBL_MAX_PLAYERS - 1] = {1, 1, 0};
static const u8 sArrowHitRangeStart[BBL_MAX_PLAYERS] = {32, 224, 96, 160};

static const struct OamData sOam_PlayerArrow =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(32x32), .x = 0, .matrixNum = 0, .size = SPRITE_SIZE(32x32), .tileNum = 0, .priority = 1, .paletteNum = 0, .affineParam = 0,
};

static const union AnimCmd sAnim_PlayerArrow_TopLeft[] = { ANIMCMD_FRAME(16, 5, .vFlip = TRUE, .hFlip = TRUE), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_TopRight[] = { ANIMCMD_FRAME(16, 5, .vFlip = TRUE), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_BottomLeft[] = { ANIMCMD_FRAME(16, 5, .hFlip = TRUE), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_BottomRight[] = { ANIMCMD_FRAME(16, 5, 0, 0), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_TopLeft_Flash[] =
{
	ANIMCMD_FRAME(48, 2, .vFlip = TRUE, .hFlip = TRUE), ANIMCMD_FRAME(32, 5, .vFlip = TRUE, .hFlip = TRUE),
	ANIMCMD_FRAME(48, 3, .vFlip = TRUE, .hFlip = TRUE), ANIMCMD_FRAME(16, 5, .vFlip = TRUE, .hFlip = TRUE), ANIMCMD_END
};
static const union AnimCmd sAnim_PlayerArrow_TopRight_Flash[] =
{
	ANIMCMD_FRAME(48, 2, .vFlip = TRUE), ANIMCMD_FRAME(32, 5, .vFlip = TRUE), ANIMCMD_FRAME(48, 3, .vFlip = TRUE), ANIMCMD_FRAME(16, 5, .vFlip = TRUE), ANIMCMD_END
};
static const union AnimCmd sAnim_PlayerArrow_BottomLeft_Flash[] =
{
	ANIMCMD_FRAME(48, 2, .hFlip = TRUE), ANIMCMD_FRAME(32, 5, .hFlip = TRUE), ANIMCMD_FRAME(48, 3, .hFlip = TRUE), ANIMCMD_FRAME(16, 5, .hFlip = TRUE), ANIMCMD_END
};
static const union AnimCmd sAnim_PlayerArrow_BottomRight_Flash[] =
{
	ANIMCMD_FRAME(48, 2, 0, 0), ANIMCMD_FRAME(32, 5, 0, 0), ANIMCMD_FRAME(48, 3, 0, 0), ANIMCMD_FRAME(16, 5, 0, 0), ANIMCMD_END
};
static const union AnimCmd sAnim_PlayerArrow_TopLeft_Off[] = { ANIMCMD_FRAME(0, 5, .vFlip = TRUE, .hFlip = TRUE), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_TopRight_Off[] = { ANIMCMD_FRAME(0, 5, .vFlip = TRUE), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_BottomLeft_Off[] = { ANIMCMD_FRAME(0, 5, .hFlip = TRUE), ANIMCMD_END };
static const union AnimCmd sAnim_PlayerArrow_BottomRight_Off[] = { ANIMCMD_FRAME(0, 5, 0, 0), ANIMCMD_END };

static const union AnimCmd* const sAnims_PlayerArrow[] =
{
	sAnim_PlayerArrow_TopLeft, sAnim_PlayerArrow_TopRight, sAnim_PlayerArrow_BottomLeft, sAnim_PlayerArrow_BottomRight,
	sAnim_PlayerArrow_TopLeft_Flash, sAnim_PlayerArrow_TopRight_Flash, sAnim_PlayerArrow_BottomLeft_Flash, sAnim_PlayerArrow_BottomRight_Flash,
	sAnim_PlayerArrow_TopLeft_Off, sAnim_PlayerArrow_TopRight_Off, sAnim_PlayerArrow_BottomLeft_Off, sAnim_PlayerArrow_BottomRight_Off
};

static const struct SpriteSheet sSpriteSheet_PlayerArrow = { gBbl_Arrow_Gfx, 0x800, GFXTAG_PLAYER_ARROW };
static const struct SpritePalette sSpritePal_BlenderMisc = { (const u16*) gBbl_Misc_Pal, PALTAG_MISC };
static const struct SpritePalette sSpritePal_PlayerArrow = { (const u16*) gBbl_Arrow_Pal, PALTAG_PLAYER_ARROW };

static const struct SpriteTemplate sSpriteTemplate_PlayerArrow =
{
	.tileTag = GFXTAG_PLAYER_ARROW, .paletteTag = PALTAG_PLAYER_ARROW, .oam = &sOam_PlayerArrow, .anims = sAnims_PlayerArrow,
	.images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCB_PlayerArrow
};

static const struct OamData sOam_ScoreSymbols =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(16x16), .x = 0, .matrixNum = 0, .size = SPRITE_SIZE(16x16), .tileNum = 0, .priority = 0, .paletteNum = 0, .affineParam = 0,
};
static const union AnimCmd sAnim_ScoreSymbols_Good[] = { ANIMCMD_FRAME(0, 20), ANIMCMD_END };
static const union AnimCmd sAnim_ScoreSymbols_Miss[] = { ANIMCMD_FRAME(4, 20, 1, 0), ANIMCMD_END };
static const union AnimCmd sAnim_ScoreSymbols_BestFlash[] =
{
	ANIMCMD_FRAME(8, 4), ANIMCMD_FRAME(12, 4), ANIMCMD_FRAME(8, 4), ANIMCMD_FRAME(12, 4), ANIMCMD_FRAME(8, 4), ANIMCMD_END
};
static const union AnimCmd sAnim_ScoreSymbols_BestStatic[] = { ANIMCMD_FRAME(8, 4), ANIMCMD_END };
static const union AnimCmd* const sAnims_ScoreSymbols[] =
{
	[SCOREANIM_GOOD] = sAnim_ScoreSymbols_Good, [SCOREANIM_MISS] = sAnim_ScoreSymbols_Miss,
	[SCOREANIM_BEST_FLASH] = sAnim_ScoreSymbols_BestFlash, [SCOREANIM_BEST_STATIC] = sAnim_ScoreSymbols_BestStatic,
};
static const struct SpriteSheet sSpriteSheet_ScoreSymbols = { gBbl_ScoreSymbols_Gfx, 0x200, GFXTAG_SCORE_SYMBOLS };
static const struct SpriteTemplate sSpriteTemplate_ScoreSymbols =
{
	.tileTag = GFXTAG_SCORE_SYMBOLS, .paletteTag = PALTAG_MISC, .oam = &sOam_ScoreSymbols, .anims = sAnims_ScoreSymbols,
	.images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCB_ScoreSymbol
};

static const struct OamData sOam_Particles =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(8x8), .x = 0, .matrixNum = 0, .size = SPRITE_SIZE(8x8), .tileNum = 0, .priority = 1, .paletteNum = 0, .affineParam = 0,
};
static const union AnimCmd sAnim_SparkleCrossToX[] =
{
	ANIMCMD_FRAME(0, 3), ANIMCMD_FRAME(1, 4), ANIMCMD_FRAME(3, 5), ANIMCMD_FRAME(1, 4), ANIMCMD_FRAME(0, 3), ANIMCMD_END
};
static const union AnimCmd sAnim_SparkleXToCross[] =
{
	ANIMCMD_FRAME(0, 3), ANIMCMD_FRAME(2, 4), ANIMCMD_FRAME(4, 5), ANIMCMD_FRAME(2, 4), ANIMCMD_FRAME(0, 3), ANIMCMD_END
};
static const union AnimCmd sAnim_SparkleFull[] =
{
	ANIMCMD_FRAME(0, 2), ANIMCMD_FRAME(1, 2), ANIMCMD_FRAME(2, 2), ANIMCMD_FRAME(4, 4), ANIMCMD_FRAME(3, 3), ANIMCMD_FRAME(2, 2), ANIMCMD_FRAME(1, 2), ANIMCMD_FRAME(0, 2), ANIMCMD_END
};
static const union AnimCmd sAnim_GreenArrow[] = { ANIMCMD_FRAME(5, 5, 1, 1), ANIMCMD_END };
static const union AnimCmd sAnim_GreenDot[] = { ANIMCMD_FRAME(6, 5, 1, 1), ANIMCMD_END };
static const union AnimCmd* const sAnims_Particles[] =
{
	sAnim_SparkleCrossToX, sAnim_SparkleXToCross, sAnim_SparkleFull, sAnim_GreenArrow, sAnim_GreenDot,
};
static const struct SpriteSheet sSpriteSheet_Particles = { gBbl_Particles_Gfx, 0xE0, GFXTAG_PARTICLES };
static const struct SpriteTemplate sSpriteTemplate_Particles =
{
	.tileTag = GFXTAG_PARTICLES, .paletteTag = PALTAG_MISC, .oam = &sOam_Particles, .anims = sAnims_Particles,
	.images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};

static const struct OamData sOam_CountdownNumbers =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(32x32), .x = 0, .matrixNum = 0, .size = SPRITE_SIZE(32x32), .tileNum = 0, .priority = 1, .paletteNum = 0, .affineParam = 0,
};
static const union AnimCmd sAnim_CountdownNumbers_3[] = { ANIMCMD_FRAME(32, 30), ANIMCMD_END };
static const union AnimCmd sAnim_CountdownNumbers_2[] = { ANIMCMD_FRAME(16, 30), ANIMCMD_END };
static const union AnimCmd sAnim_CountdownNumbers_1[] = { ANIMCMD_FRAME(0, 30), ANIMCMD_END };
static const union AnimCmd* const sAnims_CountdownNumbers[] =
{
	sAnim_CountdownNumbers_3, sAnim_CountdownNumbers_2, sAnim_CountdownNumbers_1,
};
static const struct SpriteSheet sSpriteSheet_CountdownNumbers = { gBbl_Countdown_Gfx, 0x600, GFXTAG_COUNTDOWN_NUMBERS };
static const struct SpriteTemplate sSpriteTemplate_CountdownNumbers =
{
	.tileTag = GFXTAG_COUNTDOWN_NUMBERS, .paletteTag = PALTAG_MISC, .oam = &sOam_CountdownNumbers, .anims = sAnims_CountdownNumbers,
	.images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCB_CountdownNumber
};

static const struct OamData sOam_Start =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(64x32), .x = 0, .matrixNum = 0, .size = SPRITE_SIZE(64x32), .tileNum = 0, .priority = 1, .paletteNum = 0, .affineParam = 0,
};
static const union AnimCmd sAnim_Start[] = { ANIMCMD_FRAME(0, 30), ANIMCMD_END };
static const union AnimCmd* const sAnims_Start[] = { sAnim_Start };
static const struct SpriteSheet sSpriteSheet_Start = { gBbl_Start_Gfx, 0x400, GFXTAG_START };
static const struct SpriteTemplate sSpriteTemplate_Start =
{
	.tileTag = GFXTAG_START, .paletteTag = PALTAG_MISC, .oam = &sOam_Start, .anims = sAnims_Start,
	.images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCB_Start
};

// きのみ（原文 item_menu_icons.c の sBerryPicRotatingSpriteTemplate・64x64 の 2 倍アフィン）
static const struct OamData sOam_BerryPicRotating =
{
	.y = 0, .affineMode = ST_OAM_AFFINE_DOUBLE, .objMode = ST_OAM_OBJ_NORMAL, .mosaic = FALSE, .bpp = ST_OAM_4BPP,
	.shape = SPRITE_SHAPE(64x64), .x = 0, .matrixNum = 0, .size = SPRITE_SIZE(64x64), .tileNum = 0, .priority = 0, .paletteNum = 7, .affineParam = 0
};
static const union AnimCmd sAnim_BerryPic[] = { ANIMCMD_FRAME(0, 0), ANIMCMD_END };
static const union AnimCmd* const sBerryPicSpriteAnimTable[] = { sAnim_BerryPic };
static const union AffineAnimCmd sSpriteAffineAnim_BerryPicRotation1[] =
{
	AFFINEANIMCMD_FRAME(-1, -1, 253, 96), AFFINEANIMCMD_FRAME(0, 0, 0, 16), AFFINEANIMCMD_FRAME(-2, -2, 255, 64), AFFINEANIMCMD_FRAME(-8, 0, 0, 16),
	AFFINEANIMCMD_FRAME(0, -8, 0, 16), AFFINEANIMCMD_FRAME(256, 256, 0, 0), AFFINEANIMCMD_JUMP(0)
};
static const union AffineAnimCmd sSpriteAffineAnim_BerryPicRotation2[] =
{
	AFFINEANIMCMD_FRAME(-1, -1, 3, 96), AFFINEANIMCMD_FRAME(0, 0, 0, 16), AFFINEANIMCMD_FRAME(-2, -2, 1, 64), AFFINEANIMCMD_FRAME(-8, 0, 0, 16),
	AFFINEANIMCMD_FRAME(0, -8, 0, 16), AFFINEANIMCMD_FRAME(256, 256, 0, 0), AFFINEANIMCMD_JUMP(0)
};
static const union AffineAnimCmd* const sBerryPicRotatingAnimCmds[] = { sSpriteAffineAnim_BerryPicRotation1, sSpriteAffineAnim_BerryPicRotation2 };

// きのみを投げ入れるときの動き: x, y, 弾む速さ, x の速さ, y の速さ
static const s16 sBerrySpriteData[][5] =
{
	{-10,  20,  10,   2,   1},
	{250,  20,  10,  -2,   1},
	{-10, 140,  10,   2,  -1},
	{250, 140,  10,  -2,  -1},
};

// 人数ごとの速さの割り方（0 人＝通信は段 7）
static const u8 sNumPlayersToSpeedDivisor[] = {1, 1, 2, 3, 4};
static const TaskFunc sLocalOpponentTasks[BBL_MAX_PLAYERS - 1] = { Task_HandleOpponent1, Task_HandleOpponent2, Task_HandleOpponent3 };

//--- m4a（原文 m4a.c と同じ中身・素の FR のこれらは BPRJ.ld に無い）
static void BblTempoControl(struct MusicPlayerInfo* mplayInfo, u16 tempo)
{
	if (mplayInfo->ident != ID_NUMBER)
		return;
	mplayInfo->ident++;
	mplayInfo->tempoU = tempo;
	mplayInfo->tempoI = (mplayInfo->tempoD * mplayInfo->tempoU) >> 8;
	mplayInfo->ident = ID_NUMBER;
}

static void BblPitchControl(struct MusicPlayerInfo* mplayInfo, u16 trackBits, s16 pitch)
{
	s32 i;
	struct MusicPlayerTrack* track;

	if (mplayInfo->ident != ID_NUMBER)
		return;
	mplayInfo->ident++;
	i = mplayInfo->trackCount;
	track = mplayInfo->tracks;
	while (i > 0)
	{
		if (trackBits & 1)
		{
			track->keyM = pitch >> 8;
			track->pitM = pitch;
			track->flags |= MPT_FLG_PITCHG;
		}
		trackBits >>= 1;
		track++;
		i--;
	}
	mplayInfo->ident = ID_NUMBER;
}

//--- BG2 のアフィン（原文 SetBgAffine：BIOS の BgAffineSet の結果を BG2PA〜BG2Y へ）
static void BblSetBg2Affine(u32 texX, u32 texY, s16 scrX, s16 scrY, s16 sx, s16 sy, u16 alpha)
{
	struct BgAffineSrcData src;
	struct BgAffineDstData dst;
	register struct BgAffineSrcData* r0 asm("r0") = &src;
	register struct BgAffineDstData* r1 asm("r1") = &dst;
	register s32 r2 asm("r2") = 1;

	src.texX = texX;
	src.texY = texY;
	src.scrX = scrX;
	src.scrY = scrY;
	src.sx = sx;
	src.sy = sy;
	src.alpha = alpha;
	asm volatile("swi 0x0E" : "+r"(r0), "+r"(r1), "+r"(r2) : : "r3", "memory");
	*((volatile u16*) 0x04000020) = dst.pa;
	*((volatile u16*) 0x04000022) = dst.pb;
	*((volatile u16*) 0x04000024) = dst.pc;
	*((volatile u16*) 0x04000026) = dst.pd;
	*((volatile u32*) 0x04000028) = dst.dx;
	*((volatile u32*) 0x0400002C) = dst.dy;
}

static void UpdateHitPitch(void)
{
	BblPitchControl(&gMPlayInfo_SE2, 0xFFFF, 2 * (sBerryBlender->speed - MIN_ARROW_SPEED));
}

static void VBlankCB_BerryBlender(void)
{
	SetBgPos();
	BblSetBg2Affine(sBerryBlender->bgAffineSrc.texX, sBerryBlender->bgAffineSrc.texY, sBerryBlender->bgAffineSrc.scrX, sBerryBlender->bgAffineSrc.scrY,
					sBerryBlender->bgAffineSrc.sx, sBerryBlender->bgAffineSrc.sy, sBerryBlender->bgAffineSrc.alpha);
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

static u32 BblDecompressedSize(const u8* lz)
{
	return (*((const u32*) lz)) >> 8;
}

static bool8 LoadBerryBlenderGfx(void)
{
	switch (sBerryBlender->loadGfxState)
	{
	case 0:
		sBerryBlender->tilesBuffer = AllocZeroed(BblDecompressedSize(gBbl_Center_Gfx) + 100);
		LZDecompressWram(gBbl_Center_Gfx, sBerryBlender->tilesBuffer);
		sBerryBlender->loadGfxState++;
		break;
	case 1:
		CopyToBgTilemapBuffer(2, gBbl_Center_Map, 0x400, 0);
		CopyBgTilemapBufferToVram(2);
		LoadPalette(gBbl_Center_Pal, 0, 32);                      //原文は 8 * PLTT_SIZE_4BPP＝256 バイト読むが、中央の絵は色番号 0〜15 しか使わない（後ろは配置の中身が入るだけ）
		sBerryBlender->loadGfxState++;
		break;
	case 2:
		LoadBgTiles(2, sBerryBlender->tilesBuffer, BblDecompressedSize(gBbl_Center_Gfx), 0);
		sBerryBlender->loadGfxState++;
		break;
	case 3:
		LZDecompressWram(gBbl_Outer_Gfx, sBerryBlender->tilesBuffer);
		sBerryBlender->loadGfxState++;
		break;
	case 4:
		LoadBgTiles(1, sBerryBlender->tilesBuffer, BblDecompressedSize(gBbl_Outer_Gfx), 0);
		sBerryBlender->loadGfxState++;
		break;
	case 5:
		LZDecompressWram(gBbl_Outer_Map, sBerryBlender->tilesBuffer);
		sBerryBlender->loadGfxState++;
		break;
	case 6:
		CopyToBgTilemapBuffer(1, sBerryBlender->tilesBuffer, BblDecompressedSize(gBbl_Outer_Map), 0);
		CopyBgTilemapBufferToVram(1);
		sBerryBlender->loadGfxState++;
		break;
	case 7:
		LoadPalette(gBbl_Outer_Pal, 8 * 16, 32);
		sBerryBlender->loadGfxState++;
		break;
	case 8:
		LoadSpriteSheet(&sSpriteSheet_PlayerArrow);
		LoadSpriteSheet(&sSpriteSheet_Particles);
		LoadSpriteSheet(&sSpriteSheet_ScoreSymbols);
		sBerryBlender->loadGfxState++;
		break;
	case 9:
		LoadSpriteSheet(&sSpriteSheet_CountdownNumbers);
		LoadSpriteSheet(&sSpriteSheet_Start);
		LoadSpritePalette(&sSpritePal_PlayerArrow);
		LoadSpritePalette(&sSpritePal_BlenderMisc);
		Free(sBerryBlender->tilesBuffer);
		sBerryBlender->tilesBuffer = NULL;
		sBerryBlender->loadGfxState = 0;
		return TRUE;
	}
	return FALSE;
}

static void DrawBlenderBg(void)
{
	FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 30, 20);
	CopyBgTilemapBufferToVram(0);
	ShowBg(0);
	ShowBg(1);
	SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
	ChangeBgX(0, 0, 0);
	ChangeBgY(0, 0, 0);
	ChangeBgX(1, 0, 0);
	ChangeBgY(1, 0, 0);
}

static void InitBerryBlenderWindows(void)
{
	s32 i;

	InitWindows(sWindowTemplates);
	DeactivateAllTextPrinters();
	// 名前の窓 4 つとメッセージの窓だけ初期化（結果の窓は含めない）
	for (i = 0; i < WIN_RESULTS; i++)
		FillWindowPixelBuffer(i, PIXEL_FILL(0));
	FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 30, 20);
	Menu_LoadStdPalAt(14 * 16);
}

static void Blender_AddTextPrinter(u8 windowId, const u8* string, u8 x, u8 y, s32 speed, s32 caseId)
{
	u8 txtColor[3];

	switch (caseId)
	{
	case 1:
		txtColor[0] = TEXT_COLOR_TRANSPARENT;
		txtColor[1] = TEXT_COLOR_DARK_GREY;
		txtColor[2] = TEXT_COLOR_LIGHT_GREY;
		break;
	case 2:
		txtColor[0] = TEXT_COLOR_TRANSPARENT;
		txtColor[1] = TEXT_COLOR_RED;
		txtColor[2] = TEXT_COLOR_LIGHT_RED;
		break;
	default:
		txtColor[0] = TEXT_COLOR_WHITE;
		txtColor[1] = TEXT_COLOR_DARK_GREY;
		txtColor[2] = TEXT_COLOR_LIGHT_GREY;
		break;
	}

	if (caseId != 3)
		FillWindowPixelBuffer(windowId, PIXEL_FILL(txtColor[0]));
	AddTextPrinterParameterized4(windowId, BBL_FONT, x, y, 0, 1, txtColor, speed, string);
}

static bool32 PrintMessage(s16* textState, const u8* string, s32 textSpeed)
{
	switch (*textState)
	{
	case 0:
		DrawStdFrameWithCustomTileAndPalette(WIN_MSG, FALSE, 0x14, 0xF);
		Blender_AddTextPrinter(WIN_MSG, string, 1, 1, textSpeed, 0);
		PutWindowTilemap(WIN_MSG);
		CopyWindowToVram(WIN_MSG, COPYWIN_BOTH);
		(*textState)++;
		break;
	case 1:
		if (!IsTextPrinterActive(WIN_MSG))
		{
			*textState = 0;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

//名前（原文 InitLocalPlayers の gLinkPlayers[].name）
static void SetPlayerName(u8 i, const u8* name)
{
	StringCopy(sBerryBlender->names[i], name);
}

static void InitLocalPlayers(u8 opponentsNum)
{
	StringCopy(sBerryBlender->names[0], gSaveBlock2->playerName);
	switch (opponentsNum)
	{
	case 1:
		sBerryBlender->numPlayers = 2;
		SetPlayerName(1, sBerryBlender->master ? gText_Bbl_NameMaster : gText_Bbl_NameMister);
		break;
	case 2:
		sBerryBlender->numPlayers = 3;
		SetPlayerName(1, sBerryBlender->master ? gText_Bbl_NameMaster : gText_Bbl_NameDude);
		SetPlayerName(2, sBerryBlender->master ? gText_Bbl_NameMaster : gText_Bbl_NameLassie);
		break;
	case 3:
		sBerryBlender->numPlayers = 4;
		SetPlayerName(1, sBerryBlender->master ? gText_Bbl_NameMaster : gText_Bbl_NameMiss);
		SetPlayerName(2, sBerryBlender->master ? gText_Bbl_NameMaster : gText_Bbl_NameLaddie);
		SetPlayerName(3, sBerryBlender->master ? gText_Bbl_NameMaster : gText_Bbl_NameLassie);
		break;
	}
}

void HackBlender_Start(u8 opponents, bool8 master, MainCallback exitCallback)
{
	if (opponents < 1 || opponents > 3)
		opponents = 1;
	sBerryBlender = AllocZeroed(sizeof(struct BerryBlender));
	sBerryBlender->opponents = opponents;
	sBerryBlender->master = master;   //段 5: 相手が全員名人（原文は 1 人だけ・ミナモのロビーの 3 人用の台が 3 人とも名人＝ユーザー決定 2026-11-09）
	sBerryBlender->exitCallback = exitCallback;
	InitLocalPlayers(opponents);
	sBerryBlender->mainState = 0;
	sBerryBlender->gameEndState = 0;
	SetMainCallback2(CB2_LoadBerryBlender);
}

static void FreeBlender(void)
{
	if (sBerryBlender != NULL)
	{
		Free(sBerryBlender);
		sBerryBlender = NULL;
	}
}

// ブレンダーの画面を出して、きのみを選ぶように言う（原文 CB2_LoadBerryBlender）
static void CB2_LoadBerryBlender(void)
{
	s32 i;

	switch (sBerryBlender->mainState)
	{
	case 0:
		Bbl_SetVBlankHBlankCallbacksToNull();
		clear_scheduled_bg_copies_to_vram();
		ScanlineEffect_Stop();
		ResetPaletteFade();
		SetGpuReg(REG_OFFSET_DISPCNT, 0);
		ResetSpriteData();
		FreeAllSpritePalettes();
		SetVBlankCallback(NULL);
		ResetBgsAndClearDma3BusyFlags(0);
		InitBgsFromTemplates(1, sBgTemplates, ARRAY_COUNT(sBgTemplates));
		SetBgTilemapBuffer(1, sBerryBlender->tilemapBuffers[0]);
		SetBgTilemapBuffer(2, sBerryBlender->tilemapBuffers[1]);
		TextWindow_SetStdFrame0_WithPal(0, 1, 13 * 16);             //原文 LoadUserWindowBorderGfx(0, 1, BG_PLTT_ID(13))
		TextWindow_SetUserSelectedFrame(0, 0x14, 15 * 16);          //原文 LoadMessageBoxGfx(0, 0x14, BG_PLTT_ID(15))
		InitBerryBlenderWindows();
		sBerryBlender->mainState++;
		sBerryBlender->maxProgressBarValue = 0;
		sBerryBlender->progressBarValue = 0;
		sBerryBlender->centerScale = 80;
		sBerryBlender->bg_X = 0;
		sBerryBlender->bg_Y = 0;
		sBerryBlender->loadGfxState = 0;
		UpdateBlenderCenter();
		break;
	case 1:
		if (LoadBerryBlenderGfx())
		{
			for (i = 0; i < BBL_MAX_PLAYERS; i++)
			{
				sBerryBlender->playerArrowSpriteIds[i] = CreateSprite(&sSpriteTemplate_PlayerArrow, sPlayerArrowPos[i][0], sPlayerArrowPos[i][1], 1);
				StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds[i]], i + 8);
			}
			SetVBlankCallback(VBlankCB_BerryBlender);
			sBerryBlender->mainState++;
		}
		break;
	case 2:
		BeginNormalPaletteFade(BBL_ALL_PALETTES, 0, 0x10, 0, 0);
		UpdateBlenderCenter();
		sBerryBlender->mainState++;
		break;
	case 3:
		DrawBlenderBg();
		if (!gPaletteFade->active)
			sBerryBlender->mainState++;
		break;
	case 4:
		if (PrintMessage(&sBerryBlender->textState, gText_Bbl_Start, GetPlayerTextSpeedDelay()))
			sBerryBlender->mainState++;
		break;
	case 5:
		BeginNormalPaletteFade(BBL_ALL_PALETTES, 0, 0, 0x10, 0);
		sBerryBlender->mainState++;
		break;
	case 6:
		if (!gPaletteFade->active)
		{
			// きのみ袋へ（戻り先は StartBlender）
			FreeAllWindowBuffers();
			UnsetBgTilemapBuffer(2);
			UnsetBgTilemapBuffer(1);
			SetVBlankCallback(NULL);
			GoToBerryMenu(BERRY_POUCH_CHOOSE_ITEM, StartBlender, 0);
			sBerryBlender->mainState = 0;
		}
		break;
	}
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
}

// きのみを投げ入れる（原文 SpriteCB_Berry）
#define sTargetY       data[0]
#define sX             data[1]
#define sY             data[2]
#define sBounceSpeed   data[3]
#define sYUpSpeed      data[4]
#define sBounces       data[5]
#define sXSpeed        data[6]
#define sYDownSpeed    data[7]

static void SpriteCB_Berry(struct Sprite* sprite)
{
	sprite->sX += sprite->sXSpeed;
	sprite->sY -= sprite->sYUpSpeed;
	sprite->sY += sprite->sYDownSpeed;
	sprite->sTargetY += sprite->sYDownSpeed;
	sprite->sYUpSpeed--;
	if (sprite->sTargetY < sprite->sY)
	{
		sprite->sBounceSpeed = sprite->sYUpSpeed = sprite->sBounceSpeed - 1;
		if (++sprite->sBounces > 3)
			DestroySprite(sprite);
		else
			PlaySE(BBL_SE_BALL_TRAY_EXIT);
	}
	sprite->pos1.x = sprite->sX;
	sprite->pos1.y = sprite->sY;
}

static void SetBerrySpriteData(struct Sprite* sprite, s16 x, s16 y, s16 bounceSpeed, s16 xSpeed, s16 ySpeed)
{
	sprite->sTargetY = y;
	sprite->sX = x;
	sprite->sY = y;
	sprite->sBounceSpeed = bounceSpeed;
	sprite->sYUpSpeed = 10;
	sprite->sBounces = 0;
	sprite->sXSpeed = xSpeed;
	sprite->sYDownSpeed = ySpeed;
	sprite->callback = SpriteCB_Berry;
}

#undef sTargetY
#undef sX
#undef sY
#undef sBounceSpeed
#undef sYUpSpeed
#undef sBounces
#undef sXSpeed
#undef sYDownSpeed

// きのみの絵の並べ替え（原文 ArrangeBerryGfx: 6x6 タイルを 8x8 タイルの枠の中央に・上と左に 1 タイルの余白）
static void ArrangeBerryGfx(const u8* src, u8* dest)
{
	u32 i, j, k;

	for (k = 0; k < 0x800; k++)
		dest[k] = 0;
	dest += 0x100;
	for (i = 0; i < 6; i++)
	{
		dest += 0x20;
		for (j = 0; j < 6; j++)
		{
			for (k = 0; k < 0x20; k++)
				dest[k] = src[k];
			dest += 0x20;
			src += 0x20;
		}
		dest += 0x20;
	}
}

// 原文 CreateSpinningBerrySprite（タグだけプレイヤーごと）。slot = 矢印の番号（0〜3）
static u8 CreateSpinningBerrySprite(u8 berryId, u8 x, u8 y, bool8 startAffine, u8 slot)
{
	struct SpriteSheet sheet;
	struct SpritePalette pal;
	struct SpriteTemplate* t = &sBerryBlender->berryTemplate[slot];
	u8 spriteId;

	ArrangeBerryGfx(gBbl_BerryPics + berryId * (36 * 32), sBerryBlender->berryTiles);
	sheet.data = sBerryBlender->berryTiles;
	sheet.size = 0x800;
	sheet.tag = GFXTAG_BERRY_PIC + slot;
	LoadSpriteSheet(&sheet);
	FreeSpritePaletteByTag(PALTAG_BERRY_PIC + slot);
	pal.data = (const u16*) (gBbl_BerryPals + berryId * 32);
	pal.tag = PALTAG_BERRY_PIC + slot;
	LoadSpritePalette(&pal);
	t->tileTag = GFXTAG_BERRY_PIC + slot;
	t->paletteTag = PALTAG_BERRY_PIC + slot;
	t->oam = &sOam_BerryPicRotating;
	t->anims = sBerryPicSpriteAnimTable;
	t->images = NULL;
	t->affineAnims = sBerryPicRotatingAnimCmds;
	t->callback = SpriteCallbackDummy;
	spriteId = CreateSprite(t, x, y, 0);
	if (startAffine == TRUE)
		StartSpriteAffineAnim(&gSprites[spriteId], 1);
	return spriteId;
}

static void CreateBerrySprite(u16 itemId, u8 playerId)
{
	u8 spriteId = CreateSpinningBerrySprite(itemId - ITEM_CHERI_BERRY, 0, 80, playerId & 1, playerId);

	SetBerrySpriteData(&gSprites[spriteId], sBerrySpriteData[playerId][0], sBerrySpriteData[playerId][1], sBerrySpriteData[playerId][2],
					   sBerrySpriteData[playerId][3], sBerrySpriteData[playerId][4]);
}

static void SetPlayerBerryData(u8 playerId, u16 itemId)
{
	sBerryBlender->chosenItemId[playerId] = itemId;
	HackBlender_ConvertItem(&sBerryBlender->blendedBerries[playerId], itemId);
}

static void InitBlenderBgs(void)
{
	SetGpuReg(REG_OFFSET_DISPCNT, 0);
	ResetSpriteData();
	FreeAllSpritePalettes();
	ResetTasks();
	SetVBlankCallback(VBlankCB_BerryBlender);
	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(1, sBgTemplates, ARRAY_COUNT(sBgTemplates));
	SetBgTilemapBuffer(1, sBerryBlender->tilemapBuffers[0]);
	SetBgTilemapBuffer(2, sBerryBlender->tilemapBuffers[1]);
	TextWindow_SetStdFrame0_WithPal(0, 1, 13 * 16);
	TextWindow_SetUserSelectedFrame(0, 0x14, 15 * 16);
	InitBerryBlenderWindows();
	sBerryBlender->speed = 0;
	sBerryBlender->arrowPos = 0;
	sBerryBlender->maxRPM = 0;
	sBerryBlender->bg_X = 0;
	sBerryBlender->bg_Y = 0;
}

static u8 GetArrowProximity(u16 arrowPos, u8 playerId)
{
	u32 pos = (arrowPos / 256) + 24;
	u8 arrowId = sBerryBlender->playerIdToArrowId[playerId];
	u32 hitRangeStart = sArrowHitRangeStart[arrowId];

	if (pos >= hitRangeStart && pos < hitRangeStart + 48)
	{
		if (pos >= hitRangeStart + 20 && pos < hitRangeStart + 28)
			return PROXIMITY_BEST;
		else
			return PROXIMITY_GOOD;
	}
	return PROXIMITY_MISS;
}

static void SetPlayerIdMaps(void)
{
	s32 i, j;

	for (i = 0; i < BBL_MAX_PLAYERS; i++)
	{
		sBerryBlender->playerIdToArrowId[i] = NO_PLAYER;
		sBerryBlender->arrowIdToPlayerId[i] = sPlayerIdMap[sBerryBlender->numPlayers - 2][i];
	}
	for (j = 0; j < BBL_MAX_PLAYERS; j++)
	{
		for (i = 0; i < BBL_MAX_PLAYERS; i++)
		{
			if (sBerryBlender->arrowIdToPlayerId[i] == j)
				sBerryBlender->playerIdToArrowId[j] = i;
		}
	}
}

static void PrintPlayerNames(void)
{
	s32 i, xPos;
	u32 playerId = 0;
	u8 text[BBL_NAME_LEN];

	for (i = 0; i < BBL_MAX_PLAYERS; i++)
	{
		if (sBerryBlender->arrowIdToPlayerId[i] != NO_PLAYER)
		{
			sBerryBlender->playerArrowSpriteIds[sBerryBlender->arrowIdToPlayerId[i]] = sBerryBlender->playerArrowSpriteIds2[i];
			StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds[sBerryBlender->arrowIdToPlayerId[i]]], i);
			text[0] = EOS;
			StringCopy(text, sBerryBlender->names[sBerryBlender->arrowIdToPlayerId[i]]);
			xPos = GetStringCenterAlignXOffset(BBL_FONT, text, 0x38);
			if (playerId == sBerryBlender->arrowIdToPlayerId[i])
				Blender_AddTextPrinter(i, text, xPos, 1, 0, 2);      // 自分の名前は赤
			else
				Blender_AddTextPrinter(i, text, xPos, 1, 0, 1);
			PutWindowTilemap(i);
			CopyWindowToVram(i, COPYWIN_BOTH);
		}
	}
}

//段 4c: せっていでスキップにしてあるとき、きのみを投げ入れる前（画面の準備と きのみの選びが済んだ所）で ミニゲームを飛ばして結果へ行く。
//  腕前は「ふつう」＝人数ごとの決めた最高回転数（HackBlender_SkipMaxRPM・名人の台は名人の値）。NPC の数・きのみ袋での選び・ポロックの計算は同じ。
//  得点は 0 のまま（ランキングは出さない）・速さの記録は更新しない・時間は 00分00秒。
static void SkipBlenderGame(void)
{
	SetPlayerIdMaps();
	sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]];
	sBerryBlender->centerScale = 256;
	DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
	ShowBg(2);
	SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG2_ON);
	SetGpuRegBits(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2));
	PrintPlayerNames();
	sBerryBlender->skipped = TRUE;
	SortScores();                                      //得点が全員 0 でも並びの表（playerPlaces）は要る（同点は番号の順＝プレイヤーが 1 位）
	sBerryBlender->speed = 0;
	sBerryBlender->gameFrameTime = 0;
	sBerryBlender->maxRPM = HackBlender_SkipMaxRPM(sBerryBlender->numPlayers, sBerryBlender->master);
	sBerryBlender->savedMusic = GetCurrentMapMusic();   //ファンファーレのあとに戻す曲（ブレンダーの曲 BGM_CYCLING は流さない）
	sBerryBlender->mainState = 0;
	sBerryBlender->gameEndState = 6;                   //結果の表示から（ランキングは飛ばす）
	SetMainCallback2(CB2_EndBlenderGame);
}

static void CB2_StartBlenderLocal(void)
{
	s32 i, j;

	switch (sBerryBlender->mainState)
	{
	case 0:
		InitBlenderBgs();
		SetPlayerBerryData(0, gSpecialVar_ItemId);
		HackBlender_SetOpponentBerries(gSpecialVar_ItemId, sBerryBlender->numPlayers, sBerryBlender->master, sBerryBlender->blendedBerries);
		for (i = 1; i < sBerryBlender->numPlayers; i++)
			sBerryBlender->chosenItemId[i] = sBerryBlender->blendedBerries[i].itemId;
		for (i = 0; i < BBL_MAX_PLAYERS; i++)
		{
			sBerryBlender->pendingScore[i] = 0;
			for (j = 0; j < NUM_SCORE_TYPES; j++)
				sBerryBlender->scores[i][j] = 0;
		}
		sBerryBlender->loadGfxState = 0;
		sBerryBlender->mainState++;
		break;
	case 1:
		if (LoadBerryBlenderGfx())
		{
			sBerryBlender->mainState++;
			UpdateBlenderCenter();
		}
		break;
	case 2:
		for (i = 0; i < BBL_MAX_PLAYERS; i++)
		{
			sBerryBlender->playerArrowSpriteIds2[i] = CreateSprite(&sSpriteTemplate_PlayerArrow, sPlayerArrowPos[i][0], sPlayerArrowPos[i][1], 1);
			StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds2[i]], i + 8);
		}
		sBerryBlender->mainState++;
		break;
	case 3:
		BeginNormalPaletteFade(BBL_ALL_PALETTES, 0, 0x10, 0, 0);
		sBerryBlender->mainState++;
		sBerryBlender->framesToWait = 0;
		break;
	case 4:
		if (++sBerryBlender->framesToWait == 2)
			DrawBlenderBg();
		if (!gPaletteFade->active)
			sBerryBlender->mainState = 8;
		break;
	case 8:
		if (FlagGet(HACK_FLAG_PB_BLENDER_SKIP))
		{
			SkipBlenderGame();
			break;
		}
		sBerryBlender->mainState = 11;
		sBerryBlender->playerToThrowBerry = 0;
		break;
	case 11:
		for (i = 0; i < BBL_MAX_PLAYERS; i++)
		{
			// 1 人ずつきのみを投げ入れる
			u32 playerId = sPlayerIdMap[sBerryBlender->numPlayers - 2][i];

			if (sBerryBlender->playerToThrowBerry == playerId)
			{
				CreateBerrySprite(sBerryBlender->chosenItemId[sBerryBlender->playerToThrowBerry], i);
				break;
			}
		}
		sBerryBlender->framesToWait = 0;
		sBerryBlender->mainState++;
		sBerryBlender->playerToThrowBerry++;
		break;
	case 12:
		if (++sBerryBlender->framesToWait > 60)
		{
			if (sBerryBlender->playerToThrowBerry >= sBerryBlender->numPlayers)
			{
				// 全員投げ終わった
				sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]] - ARROW_FALL_ROTATION;
				sBerryBlender->mainState++;
			}
			else
			{
				sBerryBlender->mainState--;
			}
			sBerryBlender->framesToWait = 0;
		}
		break;
	case 13:
		sBerryBlender->mainState++;
		SetPlayerIdMaps();
		PlaySE(BBL_SE_FALL);
		DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
		ShowBg(2);
		break;
	case 14:
		SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_BG2_ON);
		sBerryBlender->arrowPos += 0x200;
		sBerryBlender->centerScale += 4;
		if (sBerryBlender->centerScale > 255)
		{
			sBerryBlender->mainState++;
			sBerryBlender->centerScale = 256;
			sBerryBlender->arrowPos = sArrowStartPos[sArrowStartPosIds[sBerryBlender->numPlayers - 2]];
			SetGpuRegBits(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2));
			sBerryBlender->framesToWait = 0;
			PlaySE(BBL_SE_TRUCK_DOOR);
			PrintPlayerNames();
		}
		DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
		break;
	case 15:
		if (UpdateBlenderLandScreenShake())
			sBerryBlender->mainState++;
		DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
		break;
	case 16:
		CreateSprite(&sSpriteTemplate_CountdownNumbers, 120, -16, 3);
		sBerryBlender->mainState++;
		break;
	case 17:
		// カウントダウンを待つ（SpriteCB_Start が進める）
		break;
	case 18:
	case 19:
	case 20:
		sBerryBlender->mainState++;
		break;
	case 21:
		sBerryBlender->speed = MIN_ARROW_SPEED;
		sBerryBlender->gameFrameTime = 0;
		sBerryBlender->perfectOpponents = FALSE;
		sBerryBlender->slowdownTimer = 0;
		SetMainCallback2(CB2_PlayBlender);
		for (i = 0; i < sBerryBlender->opponents; i++)
		{
			if (sBerryBlender->master)
			{
				sBerryBlender->opponentTaskIds[i] = CreateTask(Task_HandleBerryMaster, 10 + i);
				gTasks[sBerryBlender->opponentTaskIds[i]].data[1] = i + 1;   //名人の相手の番号（1〜3・段 5 で相手ごとに 1 つ）
			}
			else
				sBerryBlender->opponentTaskIds[i] = CreateTask(sLocalOpponentTasks[i], 10 + i);
		}
		if (GetCurrentMapMusic() != BGM_CYCLING)
			sBerryBlender->savedMusic = GetCurrentMapMusic();
		PlayBGM(BGM_CYCLING);
		PlaySE(BBL_SE_BERRY_BLENDER);
		UpdateHitPitch();
		break;
	}
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
}

static void StartBlender(void)
{
	s32 i;

	SetGpuReg(REG_OFFSET_DISPCNT, 0);
	if (!HackBlender_IsBlendableBerry(gSpecialVar_ItemId))
	{
		// この改造で足したきのみ（味の表が無い）は混ぜられない＝袋を開き直す（文なし）
		PlaySE(BBL_SE_FAILURE);
		GoToBerryMenu(BERRY_POUCH_CHOOSE_ITEM, StartBlender, 0);
		return;
	}
	sBerryBlender->mainState = 0;
	for (i = 0; i < BBL_MAX_PLAYERS; i++)
		sBerryBlender->chosenItemId[i] = ITEM_NONE;
	SetMainCallback2(CB2_StartBlenderLocal);
}

// 得点（原文 CreateScoreSymbolSprite）
static void CreateScoreSymbolSprite(u16 cmd, u8 arrowId)
{
	u8 spriteId;

	spriteId = CreateSprite(&sSpriteTemplate_ScoreSymbols,
							sPlayerArrowPos[arrowId][0] - (10 * sPlayerArrowQuadrant[arrowId][0]),
							sPlayerArrowPos[arrowId][1] - (10 * sPlayerArrowQuadrant[arrowId][1]),
							1);
	if (cmd == LINKCMD_BLENDER_SCORE_BEST)
	{
		StartSpriteAnim(&gSprites[spriteId], SCOREANIM_BEST_FLASH);
		gSprites[spriteId].callback = SpriteCB_ScoreSymbolBest;
		PlaySE(BBL_SE_ICE_STAIRS);
	}
	else if (cmd == LINKCMD_BLENDER_SCORE_GOOD)
	{
		StartSpriteAnim(&gSprites[spriteId], SCOREANIM_GOOD);
		PlaySE(BBL_SE_SUCCESS);
	}
	else if (cmd == LINKCMD_BLENDER_SCORE_MISS)
	{
		StartSpriteAnim(&gSprites[spriteId], SCOREANIM_MISS);
		PlaySE(BBL_SE_FAILURE);
	}
	CreateParticleSprites();
}

// 原文 ShakeBgCoordForHit・RestoreBgCoord
static void ShakeBgCoordForHit(s16* coord, u16 speed)
{
	if (*coord == 0)
		*coord = (Random() % speed) - (speed / 2);
}

static void RestoreBgCoord(s16* coord)
{
	if (*coord < 0)
		(*coord)++;
	if (*coord > 0)
		(*coord)--;
}

static void RestoreBgCoords(void)
{
	RestoreBgCoord(&sBerryBlender->bg_X);
	RestoreBgCoord(&sBerryBlender->bg_Y);
}

static void UpdateSpeedFromHit(u16 cmd)
{
	UpdateHitPitch();
	switch (cmd)
	{
	case LINKCMD_BLENDER_SCORE_BEST:
		if (sBerryBlender->speed < 1500)
		{
			sBerryBlender->speed += (384 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
		}
		else
		{
			sBerryBlender->speed += (128 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
			ShakeBgCoordForHit(&sBerryBlender->bg_X, (sBerryBlender->speed / 100) - 10);
			ShakeBgCoordForHit(&sBerryBlender->bg_Y, (sBerryBlender->speed / 100) - 10);
		}
		break;
	case LINKCMD_BLENDER_SCORE_GOOD:
		if (sBerryBlender->speed < 1500)
			sBerryBlender->speed += (256 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
		break;
	case LINKCMD_BLENDER_SCORE_MISS:
		sBerryBlender->speed -= (256 / sNumPlayersToSpeedDivisor[sBerryBlender->numPlayers]);
		if (sBerryBlender->speed < MIN_ARROW_SPEED)
			sBerryBlender->speed = MIN_ARROW_SPEED;
		break;
	}
}

// 原文 UpdateOpponentScores の「自分の画面だけで遊ぶ」側（gRecvCmds[i][SCORE] → pendingScore[i]）
static void UpdateOpponentScores(void)
{
	s32 i;

	for (i = 0; i < sBerryBlender->numPlayers; i++)
	{
		if (sBerryBlender->pendingScore[i] != 0)
		{
			u32 arrowId = sBerryBlender->playerIdToArrowId[i];
			u16 score = sBerryBlender->pendingScore[i];

			if (score == LINKCMD_BLENDER_SCORE_BEST)
			{
				UpdateSpeedFromHit(LINKCMD_BLENDER_SCORE_BEST);
				sBerryBlender->progressBarValue += (sBerryBlender->speed / 55);
				if (sBerryBlender->progressBarValue >= MAX_PROGRESS_BAR)
					sBerryBlender->progressBarValue = MAX_PROGRESS_BAR;
				CreateScoreSymbolSprite(LINKCMD_BLENDER_SCORE_BEST, arrowId);
				sBerryBlender->scores[i][SCORE_BEST]++;
			}
			else if (score == LINKCMD_BLENDER_SCORE_GOOD)
			{
				UpdateSpeedFromHit(LINKCMD_BLENDER_SCORE_GOOD);
				sBerryBlender->progressBarValue += (sBerryBlender->speed / 70);
				CreateScoreSymbolSprite(LINKCMD_BLENDER_SCORE_GOOD, arrowId);
				sBerryBlender->scores[i][SCORE_GOOD]++;
			}
			else if (score == LINKCMD_BLENDER_SCORE_MISS)
			{
				CreateScoreSymbolSprite(LINKCMD_BLENDER_SCORE_MISS, arrowId);
				UpdateSpeedFromHit(LINKCMD_BLENDER_SCORE_MISS);
				if (sBerryBlender->scores[i][SCORE_MISS] < 999)
					sBerryBlender->scores[i][SCORE_MISS]++;
			}

			// 原文のバグ（gRecvCmds[BLENDER_COMM_SCORE][i]）を再現: ミスは誰でもテンポを更新するが、Best／Good は i == 2（3 人目）のときだけ
			if (score == LINKCMD_BLENDER_SCORE_MISS
			 || (i == 2 && (score == LINKCMD_BLENDER_SCORE_BEST || score == LINKCMD_BLENDER_SCORE_GOOD)))
			{
				if (sBerryBlender->speed > 1500)
					BblTempoControl(&gMPlayInfo_BGM, ((sBerryBlender->speed - 750) / 20) + 256);
				else
					BblTempoControl(&gMPlayInfo_BGM, 256);
			}
		}
	}
	for (i = 0; i < sBerryBlender->numPlayers; i++)
		sBerryBlender->pendingScore[i] = 0;
}

static void HandlePlayerInput(void)
{
	u8 arrowId;
	bool8 pressedA = FALSE;
	u8 playerId = 0;                        //原文: 通信のときだけ GetMultiplayerId()

	arrowId = sBerryBlender->playerIdToArrowId[playerId];

	if (sBerryBlender->gameEndState == 0)
	{
		if (gSaveBlock2->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A && JOY_NEW(A_BUTTON))
		{
			if ((gMain.heldKeysRaw & (A_BUTTON | L_BUTTON)) != (A_BUTTON | L_BUTTON))
				pressedA = TRUE;
		}
		else if (JOY_NEW(A_BUTTON))
		{
			pressedA = TRUE;
		}

		if (pressedA)
		{
			u8 proximity;

			StartSpriteAnim(&gSprites[sBerryBlender->playerArrowSpriteIds[sBerryBlender->arrowIdToPlayerId[arrowId]]], arrowId + 4);
			proximity = GetArrowProximity(sBerryBlender->arrowPos, playerId);

			if (proximity == PROXIMITY_BEST)
				sBerryBlender->pendingScore[0] = LINKCMD_BLENDER_SCORE_BEST;
			else if (proximity == PROXIMITY_GOOD)
				sBerryBlender->pendingScore[0] = LINKCMD_BLENDER_SCORE_GOOD;
			else
				sBerryBlender->pendingScore[0] = LINKCMD_BLENDER_SCORE_MISS;
		}
	}
	if (++sBerryBlender->slowdownTimer > 5)
	{
		if (sBerryBlender->speed > MIN_ARROW_SPEED)
			sBerryBlender->speed--;
		sBerryBlender->slowdownTimer = 0;
	}
}

static void CB2_PlayBlender(void)
{
	UpdateBlenderCenter();

	if (sBerryBlender->gameFrameTime < (99 * 60 * 60) + (59 * 60))
		sBerryBlender->gameFrameTime++;

	HandlePlayerInput();
	UpdateOpponentScores();
	TryUpdateProgressBar(sBerryBlender->progressBarValue, MAX_PROGRESS_BAR);
	UpdateRPM(sBerryBlender->speed);
	RestoreBgCoords();
	if (sBerryBlender->gameEndState == 0 && sBerryBlender->maxProgressBarValue >= MAX_PROGRESS_BAR)
	{
		sBerryBlender->progressBarValue = MAX_PROGRESS_BAR;
		sBerryBlender->gameEndState = 1;
		SetMainCallback2(CB2_EndBlenderGame);
	}

	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
}

static void DrawBlenderCenter(struct BgAffineSrcData* dest)
{
	struct BgAffineSrcData affineSrc;

	affineSrc.texX = (240 / 2) << 8;
	affineSrc.texY = (160 / 2) << 8;
	affineSrc.scrX = 240 / 2 - sBerryBlender->bg_X;
	affineSrc.scrY = 160 / 2 - sBerryBlender->bg_Y;
	affineSrc.sx = sBerryBlender->centerScale;
	affineSrc.sy = sBerryBlender->centerScale;
	affineSrc.alpha = sBerryBlender->arrowPos;

	*dest = affineSrc;
}

static void UpdateBlenderCenter(void)
{
	sBerryBlender->arrowPos += sBerryBlender->speed;
	DrawBlenderCenter(&sBerryBlender->bgAffineSrc);
}

static void SetBgPos(void)
{
	SetGpuReg(REG_OFFSET_BG1HOFS, sBerryBlender->bg_X);
	SetGpuReg(REG_OFFSET_BG1VOFS, sBerryBlender->bg_Y);
	SetGpuReg(REG_OFFSET_BG0HOFS, sBerryBlender->bg_X);
	SetGpuReg(REG_OFFSET_BG0VOFS, sBerryBlender->bg_Y);
}

static void SpriteCB_Particle(struct Sprite* sprite)
{
	sprite->data[2] += sprite->data[0];
	sprite->data[3] += sprite->data[1];
	sprite->pos2.x = sprite->data[2] / 8;
	sprite->pos2.y = sprite->data[3] / 8;

	if (sprite->animEnded)
		DestroySprite(sprite);
}

static void CreateParticleSprites(void)
{
	s32 limit = (Random() % 2) + 1;
	s32 i;

	for (i = 0; i < limit; i++)
	{
		u16 rand;
		s32 x, y;
		u8 spriteId;

		rand = sBerryBlender->arrowPos + (Random() % 20);

		x = gSineTable[(rand & 0xFF) + 64] / 4;
		y = gSineTable[(rand & 0xFF)] / 4;

		spriteId = CreateSprite(&sSpriteTemplate_Particles, x + 120, y + 80, 1);
		gSprites[spriteId].data[0] = 16 - (Random() % 32);
		gSprites[spriteId].data[1] = 16 - (Random() % 32);

		gSprites[spriteId].callback = SpriteCB_Particle;
	}
}

static void SpriteCB_ScoreSymbol(struct Sprite* sprite)
{
	sprite->data[0]++;
	sprite->pos2.y = -(sprite->data[0] / 3);

	if (sprite->animEnded)
		DestroySprite(sprite);
}

static void SpriteCB_ScoreSymbolBest(struct Sprite* sprite)
{
	sprite->data[0]++;
	sprite->pos2.y = -(sprite->data[0] * 2);

	if (sprite->pos2.y < -12)
		sprite->pos2.y = -12;
	if (sprite->animEnded)
		DestroySprite(sprite);
}

#define sState  data[0]
#define sYPos   data[1]
#define sDelay  data[2]
#define sAnimId data[3]

static void SpriteCB_CountdownNumber(struct Sprite* sprite)
{
	switch (sprite->sState)
	{
	case 0:
		sprite->sYPos += 8;
		if (sprite->sYPos > 160 / 2 + 8)
		{
			sprite->sYPos = 160 / 2 + 8;
			sprite->sState++;
			PlaySE(BBL_SE_BALL_BOUNCE_1);
		}
		break;
	case 1:
		if (++sprite->sDelay > 20)
		{
			sprite->sState++;
			sprite->sDelay = 0;
		}
		break;
	case 2:
		sprite->sYPos += 4;
		if (sprite->sYPos > 160 + 16)
		{
			if (++sprite->sAnimId == 3)
			{
				DestroySprite(sprite);
				CreateSprite(&sSpriteTemplate_Start, 120, -20, 2);
			}
			else
			{
				sprite->sState = 0;
				sprite->sYPos = -16;
				StartSpriteAnim(sprite, sprite->sAnimId);
			}
		}
		break;
	}

	sprite->pos2.y = sprite->sYPos;
}

#undef sState
#undef sYPos
#undef sDelay
#undef sAnimId

static void SpriteCB_Start(struct Sprite* sprite)
{
	switch (sprite->data[0])
	{
	case 0:
		sprite->data[1] += 8;
		if (sprite->data[1] > 92)
		{
			sprite->data[1] = 92;
			sprite->data[0]++;
			PlaySE(BBL_SE_PIN);
		}
		break;
	case 1:
		sprite->data[2] += 1;
		if (sprite->data[2] > 20)
			sprite->data[0]++;
		break;
	case 2:
		sprite->data[1] += 4;
		if (sprite->data[1] > 160 + 16)
		{
			sBerryBlender->mainState++;
			DestroySprite(sprite);
		}
		break;
	}

	sprite->pos2.y = sprite->data[1];
}

static void TryUpdateProgressBar(u16 current, u16 limit)
{
	// 進行バーは上がるときだけ動く
	if (sBerryBlender->maxProgressBarValue < current)
	{
		sBerryBlender->maxProgressBarValue += 2;
		UpdateProgressBar(sBerryBlender->maxProgressBarValue, limit);
	}
}

static void UpdateProgressBar(u16 value, u16 limit)
{
	s32 amountFilled, maxFilledSegment, subSegmentsFilled, i;
	u16* vram;

	vram = (u16*) (BG_SCREEN_ADDR(12));
	amountFilled = (value * 64) / limit;
	maxFilledSegment = amountFilled / 8;

	for (i = 0; i < maxFilledSegment; i++)
	{
		vram[11 + i] = PROGRESS_BAR_FILLED_TOP;
		vram[43 + i] = PROGRESS_BAR_FILLED_BOTTOM;
	}

	subSegmentsFilled = amountFilled % 8;
	if (subSegmentsFilled != 0)
	{
		vram[11 + i] = subSegmentsFilled + PROGRESS_BAR_EMPTY_TOP;
		vram[43 + i] = subSegmentsFilled + PROGRESS_BAR_EMPTY_BOTTOM;
		i++;
	}

	for (; i < 8; i++)
	{
		vram[11 + i] = PROGRESS_BAR_EMPTY_TOP;
		vram[43 + i] = PROGRESS_BAR_EMPTY_BOTTOM;
	}
}

static u32 ArrowSpeedToRPM(u16 speed)
{
	return 60 * 60 * 100 * speed / MAX_ARROW_POS;
}

static void UpdateRPM(u16 speed)
{
	u8 i;
	u8 digits[5];

	u32 currentRPM = ArrowSpeedToRPM(speed);
	if (sBerryBlender->maxRPM < currentRPM)
		sBerryBlender->maxRPM = currentRPM;

	for (i = 0; i < 5; i++)
	{
		digits[i] = currentRPM % 10;
		currentRPM /= 10;
	}
	*((u16*) (BG_SCREEN_ADDR(12) + 0x458)) = digits[4] + RPM_DIGIT;
	*((u16*) (BG_SCREEN_ADDR(12) + 0x45A)) = digits[3] + RPM_DIGIT;
	*((u16*) (BG_SCREEN_ADDR(12) + 0x45C)) = digits[2] + RPM_DIGIT;
	*((u16*) (BG_SCREEN_ADDR(12) + 0x460)) = digits[1] + RPM_DIGIT;
	*((u16*) (BG_SCREEN_ADDR(12) + 0x462)) = digits[0] + RPM_DIGIT;
}

static void BlenderLandShakeBgCoord(s16* coord, u16 timer)
{
	s32 strength;

	if (timer < 10)
		strength = 16;
	else
		strength = 8;

	if (*coord == 0)
	{
		*coord = (Random() % strength) - (strength / 2);
	}
	else
	{
		if (*coord < 0)
			(*coord)++;
		if (*coord > 0)
			(*coord)--;
	}
}

// ブレンダーが落ちてきたときの画面の揺れ
static bool8 UpdateBlenderLandScreenShake(void)
{
	if (sBerryBlender->framesToWait == 0)
	{
		sBerryBlender->bg_X = 0;
		sBerryBlender->bg_Y = 0;
	}

	sBerryBlender->framesToWait++;
	BlenderLandShakeBgCoord(&sBerryBlender->bg_X, sBerryBlender->framesToWait);
	BlenderLandShakeBgCoord(&sBerryBlender->bg_Y, sBerryBlender->framesToWait);

	if (sBerryBlender->framesToWait == 20)
	{
		sBerryBlender->bg_X = 0;
		sBerryBlender->bg_Y = 0;
		return TRUE;
	}

	return FALSE;
}

static void SpriteCB_PlayerArrow(struct Sprite* sprite)
{
	sprite->pos2.x = -(sBerryBlender->bg_X);
	sprite->pos2.y = -(sBerryBlender->bg_Y);
}

//---------------------------------------------------------------- 4b2: NPC の入力（原文 Task_HandleOpponent1〜3・Task_HandleBerryMaster・Task_OpponentMiss）
// 原文は gRecvCmds[i][BLENDER_COMM_SCORE] に入れる。ここは得点の知らせ pendingScore[i]（UpdateOpponentScores が読む）
#define tTimer    data[0]
#define tDelay    data[1]
#define tPlayerId data[2]

static void Task_OpponentMiss(u8 taskId)
{
	if (++gTasks[taskId].tTimer > gTasks[taskId].tDelay)
	{
		sBerryBlender->pendingScore[gTasks[taskId].tPlayerId] = LINKCMD_BLENDER_SCORE_MISS;
		DestroyTask(taskId);
	}
}

static void CreateOpponentMissTask(u8 playerId, u8 delay)
{
	u8 taskId = CreateTask(Task_OpponentMiss, 80);

	gTasks[taskId].tDelay = delay;
	gTasks[taskId].tPlayerId = playerId;
}

#undef tTimer
#undef tDelay
#undef tPlayerId

#define tDidInput data[0]

static void Task_HandleOpponent1(u8 taskId)
{
	if (GetArrowProximity(sBerryBlender->arrowPos, 1) == PROXIMITY_BEST)
	{
		if (!gTasks[taskId].tDidInput)
		{
			if (!sBerryBlender->perfectOpponents)
			{
				u8 rand = Random() / 655;

				if (sBerryBlender->speed < 500)
				{
					// 原文のバグ（BUGFIX 無しの側）を再現: 低速では必ず Good（Best は出ない）
					sBerryBlender->pendingScore[1] = LINKCMD_BLENDER_SCORE_GOOD;
				}
				else if (sBerryBlender->speed < 1500)
				{
					if (rand > 80)
					{
						sBerryBlender->pendingScore[1] = LINKCMD_BLENDER_SCORE_BEST;
					}
					else
					{
						u8 value = rand - 21;

						if (value < 60)
							sBerryBlender->pendingScore[1] = LINKCMD_BLENDER_SCORE_GOOD;
						else if (rand < 10)
							CreateOpponentMissTask(1, 5);
					}
				}
				else if (rand <= 90)
				{
					u8 value = rand - 71;

					if (value < 20)
						sBerryBlender->pendingScore[1] = LINKCMD_BLENDER_SCORE_GOOD;
					else if (rand < 30)
						CreateOpponentMissTask(1, 5);
				}
				else
				{
					sBerryBlender->pendingScore[1] = LINKCMD_BLENDER_SCORE_BEST;
				}
			}
			else
			{
				sBerryBlender->pendingScore[1] = LINKCMD_BLENDER_SCORE_BEST;
			}

			gTasks[taskId].tDidInput = TRUE;
		}
	}
	else
	{
		gTasks[taskId].tDidInput = FALSE;
	}
}

static void Task_HandleOpponent2(u8 taskId)
{
	u32 var1 = (sBerryBlender->arrowPos + 0x1800) & 0xFFFF;
	u8 arrowId = sBerryBlender->playerIdToArrowId[2];

	if ((var1 >> 8) > sArrowHitRangeStart[arrowId] + 20 && (var1 >> 8) < sArrowHitRangeStart[arrowId] + 40)
	{
		if (!gTasks[taskId].tDidInput)
		{
			if (!sBerryBlender->perfectOpponents)
			{
				u8 rand = Random() / 655;

				if (sBerryBlender->speed < 500)
				{
					if (rand > 66)
						sBerryBlender->pendingScore[2] = LINKCMD_BLENDER_SCORE_BEST;
					else
						sBerryBlender->pendingScore[2] = LINKCMD_BLENDER_SCORE_GOOD;
				}
				else
				{
					if (rand > 65)
						sBerryBlender->pendingScore[2] = LINKCMD_BLENDER_SCORE_BEST;
					if (rand > 40 && rand <= 65)
						sBerryBlender->pendingScore[2] = LINKCMD_BLENDER_SCORE_GOOD;
					if (rand < 10)
						CreateOpponentMissTask(2, 5);
				}

				gTasks[taskId].tDidInput = TRUE;
			}
			else
			{
				sBerryBlender->pendingScore[2] = LINKCMD_BLENDER_SCORE_BEST;
				gTasks[taskId].tDidInput = TRUE;
			}
		}
	}
	else
	{
		gTasks[taskId].tDidInput = FALSE;
	}
}

static void Task_HandleOpponent3(u8 taskId)
{
	u32 var1 = (sBerryBlender->arrowPos + 0x1800) & 0xFFFF;
	u8 arrowId = sBerryBlender->playerIdToArrowId[3];

	if ((var1 >> 8) > sArrowHitRangeStart[arrowId] + 20 && (var1 >> 8) < sArrowHitRangeStart[arrowId] + 40)
	{
		if (gTasks[taskId].data[0] == 0)
		{
			if (!sBerryBlender->perfectOpponents)
			{
				u8 rand = (Random() / 655);

				if (sBerryBlender->speed < 500)
				{
					if (rand > 88)
						sBerryBlender->pendingScore[3] = LINKCMD_BLENDER_SCORE_BEST;
					else
						sBerryBlender->pendingScore[3] = LINKCMD_BLENDER_SCORE_GOOD;
				}
				else
				{
					if (rand > 60)
						sBerryBlender->pendingScore[3] = LINKCMD_BLENDER_SCORE_BEST;
					else if (rand > 55 && rand <= 60)
						sBerryBlender->pendingScore[3] = LINKCMD_BLENDER_SCORE_GOOD;
					if (rand < 5)
						CreateOpponentMissTask(3, 5);
				}
				gTasks[taskId].tDidInput = TRUE;
			}
			else
			{
				sBerryBlender->pendingScore[3] = LINKCMD_BLENDER_SCORE_BEST;
				gTasks[taskId].tDidInput = TRUE;
			}
		}
	}
	else
	{
		gTasks[taskId].tDidInput = FALSE;
	}
}

static void Task_HandleBerryMaster(u8 taskId)
{
	u8 p = gTasks[taskId].data[1];   //相手の番号（原文は 1 固定・段 5 で 3 人でも動かすため data[1] に持つ）

	if (GetArrowProximity(sBerryBlender->arrowPos, p) == PROXIMITY_BEST)
	{
		if (!gTasks[taskId].tDidInput)
		{
			sBerryBlender->pendingScore[p] = LINKCMD_BLENDER_SCORE_BEST;
			gTasks[taskId].tDidInput = TRUE;
		}
	}
	else
	{
		gTasks[taskId].tDidInput = FALSE;
	}
}

#undef tDidInput

//---------------------------------------------------------------- 4b2: 結果・ランキング（原文 PrintBlendingResults／PrintBlendingRanking／SortScores／PrintMadePokeblockString・位置は日本語版 asm の値）
static void SortBasedOnPoints(u8* places, u8 playersNum, u32* scores)
{
	s32 i, j;

	for (i = 0; i < playersNum; i++)
	{
		for (j = 0; j < playersNum; j++)
		{
			if (scores[places[i]] > scores[places[j]])
			{
				u8 temp;
				SWAP(places[i], places[j], temp);
			}
		}
	}
}

static void SortScores(void)
{
	u8 i;
	u8 places[BBL_MAX_PLAYERS];
	u32 points[BBL_MAX_PLAYERS];

	for (i = 0; i < sBerryBlender->numPlayers; i++)
		places[i] = i;
	for (i = 0; i < sBerryBlender->numPlayers; i++)
	{
		points[i] = 1000000 * sBerryBlender->scores[i][SCORE_BEST];
		points[i] += 1000 * sBerryBlender->scores[i][SCORE_GOOD];
		points[i] += 1000 - sBerryBlender->scores[i][SCORE_MISS];
	}
	SortBasedOnPoints(places, sBerryBlender->numPlayers, points);
	for (i = 0; i < sBerryBlender->numPlayers; i++)
		sBerryBlender->playerPlaces[i] = places[i];
}

//行の y（日本語版の ((i * 段 + 基) & 0x1F) * 8）
static u8 ResultsRowY(u8 i)
{
	return (((i * sResultsRowStep[sBerryBlender->numPlayers]) + sResultsRowBase[sBerryBlender->numPlayers] - 3) & 0x1F) << 3;
}

static u8 RankingRowY(u8 i)
{
	return (((i * sRankingRowStep[sBerryBlender->numPlayers]) + 5) & 0x1F) << 3;
}

static void PrintMadePokeblockString(struct Pokeblock* pokeblock, u8* dst)
{
	u8 text[12];

	dst[0] = EOS;
	StringCopy(dst, gPokeblockNames[pokeblock->color]);
	StringAppend(dst, gText_Bbl_WasMade);
	StringAppend(dst, gText_Bbl_NewLine);
	StringAppend(dst, gText_Bbl_TheLevelIs);
	ConvertIntToDecimalStringN(text, GetHighestPokeblocksFlavorLevel(pokeblock), STR_CONV_MODE_LEFT_ALIGN, 3);
	StringAppend(dst, text);
	StringAppend(dst, gText_Bbl_TheFeelIs);
	ConvertIntToDecimalStringN(text, GetPokeblocksFeel(pokeblock), STR_CONV_MODE_LEFT_ALIGN, 3);
	StringAppend(dst, text);
	StringAppend(dst, gText_Bbl_Dot2);
	StringAppend(dst, gText_Bbl_NewParagraph);
}

static void Task_PlayPokeblockFanfare(u8 taskId)
{
	if (gTasks[taskId].data[0] == 0)
	{
		PlayFanfare(BBL_MUS_LEVEL_UP);
		gTasks[taskId].data[0]++;
	}
	if (IsFanfareTaskInactive())
	{
		PlayBGM(sBerryBlender->savedMusic);
		DestroyTask(taskId);
	}
}

static void PutPlayerWindowsBack(void)
{
	u8 i;

	for (i = 0; i < BBL_MAX_PLAYERS; i++)
	{
		if (sBerryBlender->arrowIdToPlayerId[i] != NO_PLAYER)
		{
			PutWindowTilemap(i);
			CopyWindowToVram(i, COPYWIN_BOTH);
		}
	}
}

static bool8 PrintBlendingResults(void)
{
	u16 i;
	s32 yPos;
	struct Pokeblock pokeblock;
	u8 flavors[BLENDER_FLAVORS];
	u8 text[40];

	switch (sBerryBlender->mainState)
	{
	case 0:
		sBerryBlender->mainState++;
		sBerryBlender->framesToWait = 17;
		break;
	case 1:
		sBerryBlender->framesToWait -= 10;
		if (sBerryBlender->framesToWait < 0)
		{
			sBerryBlender->framesToWait = 0;
			sBerryBlender->mainState++;
		}
		break;
	case 2:
		if (++sBerryBlender->framesToWait > 20)
		{
			if (!sBerryBlender->skipped)   //スキップのときは得点の絵を出していない（scoreIconIds は 0＝0 番のスプライトを壊さない）
				for (i = 0; i < NUM_SCORE_TYPES; i++)
					DestroySprite(&gSprites[sBerryBlender->scoreIconIds[i]]);

			sBerryBlender->framesToWait = 0;
			sBerryBlender->mainState++;
		}
		break;
	case 3:
		{
			u16 minutes, seconds;
			u8* txtPtr;

			if (sBerryBlender->skipped)   //スキップのときはランキングの画面を通っていない＝結果の窓の枠と並びをここで出す
			{
				DrawStdFrameWithCustomTileAndPalette(WIN_RESULTS, FALSE, 1, 0xD);
				PutWindowTilemap(WIN_RESULTS);
			}
			Blender_AddTextPrinter(WIN_RESULTS, gText_Bbl_Results, 0x38, 1, 0, 0);

			for (i = 0; i < sBerryBlender->numPlayers; i++)
			{
				u8 place = sBerryBlender->playerPlaces[i];

				yPos = ResultsRowY(i);
				ConvertIntToDecimalStringN(sBerryBlender->stringVar, i + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
				StringAppend(sBerryBlender->stringVar, gText_Bbl_Dot);
				StringAppend(sBerryBlender->stringVar, sBerryBlender->names[place]);
				Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0x10, yPos, 0, 3);

				CopyItemName(sBerryBlender->blendedBerries[place].itemId, sBerryBlender->stringVar);   //道具の名前は「〜のみ」まで入っている（日本語版の「きのみ名」＋「のみ」と同じ文字）
				Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0x60, yPos, 0, 3);
			}

			Blender_AddTextPrinter(WIN_RESULTS, gText_Bbl_MaxSpeed, 8, 0x50, 0, 3);
			ConvertIntToDecimalStringN(sBerryBlender->stringVar, sBerryBlender->maxRPM / 100, STR_CONV_MODE_RIGHT_ALIGN, 3);
			StringAppend(sBerryBlender->stringVar, gText_Bbl_Dot);
			ConvertIntToDecimalStringN(text, sBerryBlender->maxRPM % 100, STR_CONV_MODE_LEADING_ZEROS, 2);
			StringAppend(sBerryBlender->stringVar, text);
			StringAppend(sBerryBlender->stringVar, gText_Bbl_RPM);
			Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0x48, 0x50, 0, 3);

			Blender_AddTextPrinter(WIN_RESULTS, gText_Bbl_Time, 8, 0x60, 0, 3);
			seconds = (sBerryBlender->gameFrameTime / 60) % 60;
			minutes = (sBerryBlender->gameFrameTime / (60 * 60));
			ConvertIntToDecimalStringN(sBerryBlender->stringVar, minutes, STR_CONV_MODE_LEADING_ZEROS, 2);
			txtPtr = StringAppend(sBerryBlender->stringVar, gText_Bbl_Min);
			ConvertIntToDecimalStringN(txtPtr, seconds, STR_CONV_MODE_LEADING_ZEROS, 2);
			StringAppend(sBerryBlender->stringVar, gText_Bbl_Sec);
			Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0x48, 0x60, 0, 3);

			sBerryBlender->framesToWait = 0;
			sBerryBlender->mainState++;
			CopyWindowToVram(WIN_RESULTS, sBerryBlender->skipped ? COPYWIN_BOTH : COPYWIN_GFX);
		}
		break;
	case 4:
		if (JOY_NEW(A_BUTTON))
			sBerryBlender->mainState++;
		break;
	case 5:
		ClearStdWindowAndFrameToTransparent(WIN_RESULTS, TRUE);
		PutPlayerWindowsBack();

		HackBlender_Calculate(sBerryBlender->blendedBerries, &pokeblock, sBerryBlender->numPlayers, flavors, sBerryBlender->maxRPM);
		// 足す→きのみを外す→外せなければ戻す（原文は RemoveBagItem → AddPokeblock・ケースが満杯だとポロックだけ消える）
		if (HackBlender_StoreAndConsume(&pokeblock, sBerryBlender->blendedBerries[0].itemId))
		{
			PrintMadePokeblockString(&pokeblock, sBerryBlender->stringVar);
			CreateTask(Task_PlayPokeblockFanfare, 6);
		}
		else
		{
			// 入れられなかった（原文に無い・Q1）: ポロックケースがいっぱいの文・ファンファーレなし・きのみはそのまま
			sBerryBlender->caseFull = TRUE;
			StringCopy(sBerryBlender->stringVar, gText_Bbl_CaseFull);
			PlayBGM(sBerryBlender->savedMusic);
		}

		sBerryBlender->textState = 0;
		sBerryBlender->mainState++;
		break;
	case 6:
		if (PrintMessage(&sBerryBlender->textState, sBerryBlender->stringVar, GetPlayerTextSpeedDelay()))
		{
			if (!sBerryBlender->skipped)   //スキップの速さは記録しない（ユーザー決定）
				HackBlender_TryUpdateRecord(sBerryBlender->numPlayers, sBerryBlender->maxRPM);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static bool8 PrintBlendingRanking(void)
{
	u16 i;
	s32 yPos;

	switch (sBerryBlender->mainState)
	{
	case 0:
		sBerryBlender->mainState++;
		sBerryBlender->framesToWait = 255;
		break;
	case 1:
		sBerryBlender->framesToWait -= 10;
		if (sBerryBlender->framesToWait < 0)
		{
			sBerryBlender->framesToWait = 0;
			sBerryBlender->mainState++;
		}
		break;
	case 2:
		if (++sBerryBlender->framesToWait > 20)
		{
			sBerryBlender->framesToWait = 0;
			sBerryBlender->mainState++;
		}
		break;
	case 3:
		DrawStdFrameWithCustomTileAndPalette(WIN_RESULTS, FALSE, 1, 0xD);
		Blender_AddTextPrinter(WIN_RESULTS, gText_Bbl_Ranking, 0x40, 0, 0, 0);

		sBerryBlender->scoreIconIds[SCORE_BEST] = CreateSprite(&sSpriteTemplate_ScoreSymbols, 128, 52, 0);
		StartSpriteAnim(&gSprites[sBerryBlender->scoreIconIds[SCORE_BEST]], SCOREANIM_BEST_STATIC);
		gSprites[sBerryBlender->scoreIconIds[SCORE_BEST]].callback = SpriteCallbackDummy;

		sBerryBlender->scoreIconIds[SCORE_GOOD] = CreateSprite(&sSpriteTemplate_ScoreSymbols, 160, 52, 0);
		gSprites[sBerryBlender->scoreIconIds[SCORE_GOOD]].callback = SpriteCallbackDummy;

		sBerryBlender->scoreIconIds[SCORE_MISS] = CreateSprite(&sSpriteTemplate_ScoreSymbols, 192, 52, 0);
		StartSpriteAnim(&gSprites[sBerryBlender->scoreIconIds[SCORE_MISS]], SCOREANIM_MISS);
		gSprites[sBerryBlender->scoreIconIds[SCORE_MISS]].callback = SpriteCallbackDummy;

		SortScores();

		for (i = 0; i < sBerryBlender->numPlayers; i++)
		{
			u8 place = sBerryBlender->playerPlaces[i];
			u8 j;

			yPos = RankingRowY(i);
			ConvertIntToDecimalStringN(sBerryBlender->stringVar, i + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
			StringAppend(sBerryBlender->stringVar, gText_Bbl_Dot);
			StringAppend(sBerryBlender->stringVar, sBerryBlender->names[place]);
			Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 0, yPos, 0, 3);

			for (j = 0; j < NUM_SCORE_TYPES; j++)
			{
				ConvertIntToDecimalStringN(sBerryBlender->stringVar, sBerryBlender->scores[place][j], STR_CONV_MODE_RIGHT_ALIGN, 3);
				Blender_AddTextPrinter(WIN_RESULTS, sBerryBlender->stringVar, 72 + 32 * j, yPos, 0, 3);
			}
		}

		PutWindowTilemap(WIN_RESULTS);
		CopyWindowToVram(WIN_RESULTS, COPYWIN_BOTH);

		sBerryBlender->framesToWait = 0;
		sBerryBlender->mainState++;
		break;
	case 4:
		if (++sBerryBlender->framesToWait > 20)
			sBerryBlender->mainState++;
		break;
	case 5:
		if (JOY_NEW(A_BUTTON))
		{
			PlaySE(BBL_SE_SELECT);
			sBerryBlender->mainState++;
		}
		break;
	case 6:
		sBerryBlender->mainState = 0;
		return TRUE;
	}

	return FALSE;
}

//混ぜられるきのみ（43 種）が 1 個でもあるか。原文は IsBagPocketNonEmpty(きのみ袋)＝この改造は CFRU の追加きのみがあると「ある」と出て詰むので置き換え（Commander 承認 Q2）
static bool8 HasBlendableBerry(void)
{
	u16 id;

	for (id = ITEM_CHERI_BERRY; id <= ITEM_ENIGMA_BERRY; id++)
	{
		if (HackBlender_IsBlendableBerry(id) && CheckBagHasItem(id, 1))
			return TRUE;
	}
	return FALSE;
}

enum { PLAY_AGAIN_YES, PLAY_AGAIN_NO, CANT_PLAY_NO_BERRIES, CANT_PLAY_NO_PKBLCK_SPACE };

//ゲームの終わり（原文 CB2_EndBlenderGame のローカル側・通信の state 3・4・13 は持ち込まない）
static void CB2_EndBlenderGame(void)
{
	u8 i;

	if (sBerryBlender->gameEndState < 3)
		UpdateBlenderCenter();

	switch (sBerryBlender->gameEndState)
	{
	case 1:
		BblTempoControl(&gMPlayInfo_BGM, 256);
		for (i = 0; i < sBerryBlender->opponents; i++)
			DestroyTask(sBerryBlender->opponentTaskIds[i]);
		sBerryBlender->gameEndState++;
		break;
	case 2:
		sBerryBlender->speed -= 32;
		if (sBerryBlender->speed <= 0)
		{
			sBerryBlender->speed = 0;
			sBerryBlender->gameEndState = 5;
			sBerryBlender->mainState = 0;
			m4aMPlayStop(&gMPlayInfo_SE2);
		}
		UpdateHitPitch();
		break;
	case 5:
		if (PrintBlendingRanking())
			sBerryBlender->gameEndState++;
		break;
	case 6:
		if (PrintBlendingResults())
			sBerryBlender->gameEndState++;
		break;
	case 7:
		if (PrintMessage(&sBerryBlender->textState, gText_Bbl_AnotherBerry, GetPlayerTextSpeedDelay()))
			sBerryBlender->gameEndState = 9;
		break;
	case 9:
		sBerryBlender->yesNoAnswer = 0;
		CreateYesNoMenu(&sYesNoWindowTemplate, BBL_FONT, 0, 0, 1, 0xD, 0);
		sBerryBlender->gameEndState++;
		break;
	case 10:
		switch (Menu_ProcessInputNoWrapClearOnChoose())
		{
		case 1:
		case MENU_B_PRESSED:
			sBerryBlender->yesNoAnswer = 1;
			sBerryBlender->gameEndState++;
			PutPlayerWindowsBack();
			break;
		case 0:
			sBerryBlender->yesNoAnswer = 0;
			sBerryBlender->gameEndState++;
			PutPlayerWindowsBack();
			break;
		}
		break;
	case 11:
		if (sBerryBlender->yesNoAnswer == 0)
		{
			if (!HasBlendableBerry())
				sBerryBlender->playAgainState = CANT_PLAY_NO_BERRIES;
			else if (!HackBlender_CaseHasRoom())
				sBerryBlender->playAgainState = CANT_PLAY_NO_PKBLCK_SPACE;
			else
				sBerryBlender->playAgainState = PLAY_AGAIN_YES;
		}
		else
		{
			sBerryBlender->playAgainState = PLAY_AGAIN_NO;
		}
		sBerryBlender->gameEndState++;
		break;
	case 12:
		SetMainCallback2(CB2_CheckPlayAgainLocal);
		sBerryBlender->gameEndState = 0;
		sBerryBlender->mainState = 0;
		break;
	}

	RestoreBgCoords();
	UpdateRPM(sBerryBlender->speed);
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
}

//もう 1 回（原文 CB2_CheckPlayAgainLocal）。「はい」なら最初（きのみ袋）へ・「いいえ」と続けられないときは戻り先へ
static void CB2_CheckPlayAgainLocal(void)
{
	switch (sBerryBlender->gameEndState)
	{
	case 0:
		if (sBerryBlender->playAgainState == PLAY_AGAIN_YES || sBerryBlender->playAgainState == PLAY_AGAIN_NO)
			sBerryBlender->gameEndState = 9;
		if (sBerryBlender->playAgainState == CANT_PLAY_NO_BERRIES)
			sBerryBlender->gameEndState = 2;
		if (sBerryBlender->playAgainState == CANT_PLAY_NO_PKBLCK_SPACE)
			sBerryBlender->gameEndState = 1;
		break;
	case 1:
		sBerryBlender->gameEndState = 3;
		sBerryBlender->textState = 0;
		StringCopy(sBerryBlender->stringVar, gText_Bbl_CaseFull);
		break;
	case 2:
		sBerryBlender->gameEndState++;
		sBerryBlender->textState = 0;
		StringCopy(sBerryBlender->stringVar, gText_Bbl_RunOutOfBerries);
		break;
	case 3:
		if (PrintMessage(&sBerryBlender->textState, sBerryBlender->stringVar, GetPlayerTextSpeedDelay()))
			sBerryBlender->gameEndState = 9;
		break;
	case 9:
		BeginFastPaletteFade(3);
		sBerryBlender->gameEndState++;
		break;
	case 10:
		if (!gPaletteFade->active)
		{
			u8 again = (sBerryBlender->playAgainState == PLAY_AGAIN_YES);
			u8 opponents = sBerryBlender->opponents;
			bool8 master = sBerryBlender->master;
			MainCallback cb = sBerryBlender->exitCallback;

			BblTempoControl(&gMPlayInfo_BGM, 0x100);
			SetVBlankCallback(NULL);
			FreeAllWindowBuffers();
			UnsetBgTilemapBuffer(2);
			UnsetBgTilemapBuffer(1);
			FreeBlender();
			if (again)
				HackBlender_Start(opponents, master, cb);
			else
				SetMainCallback2(cb);
			return;
		}
		break;
	}

	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	RunTextPrinters();
	UpdatePaletteFade();
}

//---------------------------------------------------------------- ミナモのロビーの台（段 5・scripts は tools/rom/hoenn_lobby_people.py の SPECIAL_MISSING が callnative で呼ぶ）
// 原文の special DoBerryBlending／PlayerHasBerries／GetFirstFreePokeblockSlot／ShowBerryBlenderRecordWindow／RemoveRecordsWindow（pokeemerald src/berry_blender.c・src/pokeblock.c・src/field_specials.c）。
// 答えは VAR_RESULT（LASTRESULT）。通信の台は持ち込まない（段 7）。

//special DoBerryBlending: Var8004 = 相手の数（1〜3）。名人は 3 人用の台だけ（相手 3 人が全員名人・ユーザー決定 2026-11-09・1 人用は エキスパート＝名人の入れ替えは起きない）。
//台本が fadescreen してから呼ぶ＝暗転を待つ必要は無い（原文と同じ）
void HoennNative_BerryBlend(void)
{
	u8 opp = Var8004;

	if (opp < 1 || opp > 3)
		return;
	HackBlender_Start(opp, opp == 3, CB2_ReturnToField);
}

//specialvar PlayerHasBerries: 混ぜられるきのみが 1 個でもあるか
void HoennNative_PlayerHasBerries(void)
{
	gSpecialVar_LastResult = HasBlendableBerry();
}

//specialvar GetFirstFreePokeblockSlot: 原文は空きの番号（満杯なら 0xFFFF）。台本は 0xFFFF かどうかだけ見る。
//使えない版・レポート前・壊れは 0xFFFE（台本が案内の文へ分岐）・空きがあれば 0
void HoennNative_PokeblockFreeSlot(void)
{
	if (Pbc_GetRefusalText() != NULL)
		gSpecialVar_LastResult = 0xFFFE;
	else if (Pokeblock_Count() >= POKEBLOCK_CASE_SLOTS)
		gSpecialVar_LastResult = 0xFFFF;
	else
		gSpecialVar_LastResult = 0;
}

//使えない版の案内（ポロックケースを開こうとしたときと同じ文・同じ判定）。台本が waitmessage／closemessage で閉じる
void HoennNative_PokeblockRefuse(void)
{
	const u8* text = Pbc_GetRefusalText();

	if (text != NULL)
		ShowFieldMessage(text);
}

//記録の窓（原文 sBlenderRecordWindowTemplate は US の値＝bg0・left6・top4・w18・h11。日本語版 ROM は w16・h12（テンプレート 0x0830FC60・ShowBerryBlenderRecordWindow 0x080837C8 が読む））。★ 幅だけ US と同じ 18 にした（mGBA 2026-11-09: 素の FR の字は日本語版エメラルドより幅広く、幅 16 だと題の 2 行目「さいこうそくど ランキング！」の「グ！」が欠けた）。高さ 12・位置・文字の座標は日本語版の値。
//日本語版は文字を左寄せの x=8 に置く（原文の中央寄せではない）・記録は x=0x30 から「999.99rpm」を右寄せ 3 桁＋「,」＋2 桁（原文と同じ組み立て）。窓の番号は Var8005（VAR_0x8005）に持つ（書き換え可能な static は ROM に置かれる＝6.13）
static const struct WindowTemplate sBlenderRecordWindowTemplate =
{
	.bg = 0, .tilemapLeft = 6, .tilemapTop = 4, .width = 18, .height = 12, .paletteNum = 15, .baseBlock = 8
};

void HoennNative_ShowBlenderRecords(void)
{
	u8 text[32];
	u8 id;
	u8 i;

	id = AddWindow(&sBlenderRecordWindowTemplate);
	Var8005 = id;
	DrawStdWindowFrame(id, FALSE);
	FillWindowPixelBuffer(id, PIXEL_FILL(1));
	AddTextPrinterParameterized(id, BBL_FONT, gText_Bbl_RecordTitle, 8, 2, 0, NULL);
	AddTextPrinterParameterized(id, BBL_FONT, gText_Bbl_RecordPlayers, 8, 42, 0, NULL);
	for (i = 0; i < 3; i++)
	{
		u16 record = Pokeblock_GetRecord(i);
		u8* ptr = ConvertIntToDecimalStringN(text, record / 100, STR_CONV_MODE_RIGHT_ALIGN, 3);

		ptr = StringAppend(ptr, gText_Bbl_Dot);
		ptr = ConvertIntToDecimalStringN(ptr, record % 100, STR_CONV_MODE_LEADING_ZEROS, 2);
		StringAppend(ptr, gText_Bbl_RPM);
		AddTextPrinterParameterized(id, BBL_FONT, text, 0x30, 42 + i * 16, 0, NULL);
	}
	PutWindowTilemap(id);
	CopyWindowToVram(id, COPYWIN_BOTH);
}

void HoennNative_RemoveRecords(void)
{
	u8 id = Var8005;

	ClearStdWindowAndFrameToTransparent(id, FALSE);
	CopyWindowToVram(id, COPYWIN_GFX);
	RemoveWindow(id);
}
