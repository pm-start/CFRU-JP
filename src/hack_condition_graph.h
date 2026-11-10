#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 3（patches/0978）: コンディションのゲージ・星・ポケモンの絵の出入り・選択の玉（src/hack_condition_graph.c）。
//  原文 pokeemerald include/menu_specialized.h の定数と宣言（ポケナビのコンディション画面の部品）。ポケナビそのものの画面は入れない。
//  使う側は defines.h の後に読む。
enum {
	TAG_CONDITION_MON = 100,
	TAG_CONDITION_BALL,
	TAG_CONDITION_CANCEL,
	TAG_CONDITION_BALL_PLACEHOLDER,
	TAG_CONDITION_SPARKLE,
};

enum {
	CONDITION_ICON_SELECTED,
	CONDITION_ICON_UNSELECTED,
};

#define MAX_CONDITION_SPARKLES 10
#define MAX_CONDITION 255
#define MAX_SHEEN 255
#define MON_PIC_SIZE 0x800                 //64x64 の 4bpp

// 星の数（原文のマクロは末尾に ; が付いている書き方なのでここは普通の式にした）。1 匹ごとに最初の 1 つは必ず出る
#define GET_NUM_CONDITION_SPARKLES(sheen) (((sheen) != MAX_SHEEN) ? (sheen) / ((u32)MAX_SHEEN / (MAX_CONDITION_SPARKLES - 1) + 1) : MAX_CONDITION_SPARKLES - 1)

#define CONDITION_GRAPH_TOP_Y  56
#define CONDITION_GRAPH_BOTTOM_Y 121
#define CONDITION_GRAPH_HEIGHT  (CONDITION_GRAPH_BOTTOM_Y - CONDITION_GRAPH_TOP_Y + 1)
#define CONDITION_GRAPH_CENTER_X 155
#define CONDITION_GRAPH_CENTER_Y ((CONDITION_GRAPH_BOTTOM_Y + CONDITION_GRAPH_TOP_Y) / 2 + 3)
#define CONDITION_GRAPH_UPDATE_STEPS 10
#define CONDITION_GRAPH_LOAD_MAX 4

// フレーバーやコンテストの値と同じ物の別の並び
enum { CONDITION_COOL, CONDITION_TOUGH, CONDITION_SMART, CONDITION_CUTE, CONDITION_BEAUTY, CONDITION_COUNT };
// もう 1 つの並び（グラフの頂点の順）
enum { GRAPH_COOL, GRAPH_BEAUTY, GRAPH_CUTE, GRAPH_SMART, GRAPH_TOUGH };

struct ConditionGraph
{
	u8 conditions[CONDITION_GRAPH_LOAD_MAX][CONDITION_COUNT];
	struct UCoords16 savedPositions[CONDITION_GRAPH_LOAD_MAX][CONDITION_COUNT];
	struct UCoords16 newPositions[CONDITION_GRAPH_UPDATE_STEPS][CONDITION_COUNT];
	struct UCoords16 curPositions[CONDITION_COUNT];
	u16 scanlineRight[CONDITION_GRAPH_HEIGHT][2];
	u16 scanlineLeft[CONDITION_GRAPH_HEIGHT][2];
	u16 bottom;
	u16 updateCounter;
	bool8 needsDraw;
	u8 scanlineResetState;
};

void ConditionGraph_Init(struct ConditionGraph* graph);
void ConditionGraph_InitWindow(u8 bg);
void ConditionGraph_InitResetScanline(struct ConditionGraph* graph);
bool8 ConditionGraph_ResetScanline(struct ConditionGraph* graph);
void ConditionGraph_Draw(struct ConditionGraph* graph);
bool8 ConditionGraph_TryUpdate(struct ConditionGraph* graph);
void ConditionGraph_Update(struct ConditionGraph* graph);
void ConditionGraph_CalcPositions(u8* conditions, struct UCoords16* positions);
void ConditionGraph_SetNewPositions(struct ConditionGraph* graph, struct UCoords16* old, struct UCoords16* new);

bool8 ConditionMenu_UpdateMonEnter(struct ConditionGraph* graph, s16* x);
bool8 ConditionMenu_UpdateMonExit(struct ConditionGraph* graph, s16* x);
bool8 MoveConditionMonOnscreen(s16* x);
bool8 MoveConditionMonOffscreen(s16* x);
//原文の GetConditionMenuMon*（箱も扱う物）の手持ち専用版。numMons は選択の数（やめる込み）・partyId が最後（やめる）なら空にする
void GetConditionMenuMonName(u8* nameDst, u16 monId, u16 partyId, u16 numMons);
void GetConditionMenuMonConditions(struct ConditionGraph* graph, u8* numSparkles, u16 monId, u16 partyId, u16 id, u16 numMons);
void GetConditionMenuMonGfx(void* tilesDst, void* palDst, u16 monId, u16 partyId, u16 numMons);
void LoadConditionMonPicTemplate(struct SpriteSheet* sheet, struct SpriteTemplate* template, struct SpritePalette* pal);
void LoadConditionSelectionIcons(struct SpriteSheet* sheets, struct SpriteTemplate* template, struct SpritePalette* pals);

void LoadConditionSparkle(struct SpriteSheet* sheet, struct SpritePalette* pal);
void ResetConditionSparkleSprites(struct Sprite** sprites);
void CreateConditionSparkleSprites(struct Sprite** sprites, u8 monSpriteId, u8 _count);
void DestroyConditionSparkleSprites(struct Sprite** sprites);
void FreeConditionSparkles(struct Sprite** sprites);
