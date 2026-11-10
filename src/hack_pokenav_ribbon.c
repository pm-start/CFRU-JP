#include "defines.h"
#include "../include/bg.h"
#include "../include/decompress.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/party_menu.h"
#include "../include/pokemon.h"
#include "../include/pokemon_icon.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/text_window.h"
#include "../include/window.h"
#include "../include/constants/songs.h"
#include "../include/gba/macro.h"
#include "hack_ribbon.h"

/*
    ポケナビの「リボン」画面（2026-11-09・FEAT-relive-ribbon 段 2・patches/0977・ユーザー決定「ポケナビにリボン画面」）

    エメラルドのポケナビのリボン（src/pokenav_ribbons_summary.c）に寄せた作り。エメラルドは LoopedTask＋BG のタイルで描くが、ここは
    hack_pokenav.c と同じ（MainCB2＋Task＋Calloc した入れ物）で、リボンの絵はスプライトで置く（絵は参考/pokeemerald/graphics/pokenav/ribbons/ を
    tools/pokenav/build_ribbon_gfx.py が src/hack_pokenav_ribbon_gfx.h に変換した物）。

    ## 操作（エメラルドの原文 pokenav_ribbons_summary.c と、ユーザーの決定「手持ちを 1 匹ずつ ←→」の合成）
      ポケモンの選択: ←→ でポケモンを切り替える（手持ちだけ・卵は飛ばす）・A でリボンの選択に入る（リボンが 0 ならそのまま）・B でポケナビへ戻る
      リボンの選択:   十字キー 4 方向でカーソルを動かす（エメラルドの TrySelectRibbonUp／Down／Left／Right と同じ動き）・B でポケモンの選択へ
                      カーソルの下のリボンの大きい絵と説明 2 行を出す
    ## リボンの数え方（エメラルドと同じ）
      コンテスト 5 種は 3 ビットの段（0〜4）で、段の数だけ ノーマル→スーパー→ハイパー→マスター の順に並べる。そのあとに チャンピオン・ウイニング・ビクトリー・
      アーティスト・がんばり、ギフト 7 種（別の行）。表示の順はエメラルドの sRibbonData と同じ（チャンピオン→コンテスト 5 種→ウイニング→ビクトリー→アーティスト→がんばり）。
    ## 状態を持たない
      書き込み可能な static は無い（6.13）。状態は Calloc した入れ物（EWRAM）で、語は hack_pokenav.c と同じ 0x203E038（ポケナビの画面は開く前に自分を片づける）。
      Var8000／8001 は触らない。戻りは CB2_HackPokenavReturn（Var8002 のカーソルで一覧を開き直す）。
    ## 配布リボンの説明
      ナショナル以外の配布リボン 6 種の説明は全部同じ文「とくべつな イベントで／もらった きねんの リボン」（ユーザー決定 2026-11-09「リボン説明おなじで」・sRibInfo[] の Gift 行）。
*/

/* 素の JP 版の番地（hack_pokenav.c と同じ・verify_pokenav.py が確かめる） */
#define Rib_DecompressAndCopyTileDataToVram     ((void (*)(u8, const void *, u32, u16, u8))(0x080F78D0 + 1))
#define Rib_FreeTempTileDataBuffersIfPossible ((bool8 (*)(void))(0x080F7884 + 1))
void __attribute__((long_call)) ScanlineEffect_Stop(void);      /* BPRJ.ld 08087A48 */

extern void CB2_HackPokenavReturn(void);
void CB2_HackPokenavRibbon(void);

extern const u8 BwSmBgTiles[];
extern const u8 BwSmBgMap[];
extern const u16 BwSmBgPal[];

#include "hack_pokenav_ribbon_gfx.h"

extern const u8 gText_Pnav_Ribbon[];
extern const u8 gText_PnavRib_CountHead[];
extern const u8 gText_PnavRib_CountTail[];
extern const u8 gText_PnavRib_Help[];
extern const u8 gText_PnavRib_None[];
extern const u8 gText_PnavRib_Champion1[];
extern const u8 gText_PnavRib_Champion2[];
extern const u8 gText_PnavRib_Cool[];
extern const u8 gText_PnavRib_Beauty[];
extern const u8 gText_PnavRib_Cute[];
extern const u8 gText_PnavRib_Smart[];
extern const u8 gText_PnavRib_Tough[];
extern const u8 gText_PnavRib_RankNormal[];
extern const u8 gText_PnavRib_RankSuper[];
extern const u8 gText_PnavRib_RankHyper[];
extern const u8 gText_PnavRib_RankMaster[];
extern const u8 gText_PnavRib_Winning1[];
extern const u8 gText_PnavRib_Victory1[];
extern const u8 gText_PnavRib_Won2[];
extern const u8 gText_PnavRib_Artist1[];
extern const u8 gText_PnavRib_Artist2[];
extern const u8 gText_PnavRib_Effort1[];
extern const u8 gText_PnavRib_Effort2[];
extern const u8 gText_PnavRib_Gift1[];
extern const u8 gText_PnavRib_Gift2[];
extern const u8 gText_PnavRib_National1[];
extern const u8 gText_PnavRib_National2[];

#define RIB_STATE_PTR 0x203E038          /* hack_pokenav.c と同じ「大きな画面の状態」の語 */
#define RIB_FONT 2
#define RIB_COUNT 32                     /* リボンの種類（エメラルドの ribbonId 0〜31） */
#define RIB_NORMAL_COUNT 25              /* 0〜24（チャンピオン・コンテスト 20・ウイニング・ビクトリー・アーティスト・がんばり） */
#define RIB_PER_ROW 9
#define RIB_GIFT_ROW 3                   /* エメラルドの GIFT_RIBBON_ROW（= 1 + 25 / 9）。通常は 3 行まで・ギフトは 4 行目 */
#define RIB_GRID_X 84
#define RIB_GRID_Y 22
#define RIB_STEP 17
#define RIB_MON_X 36
#define RIB_MON_Y 32
#define RIB_BIG_X 36
#define RIB_BIG_Y 104
#define RIB_TAG_NONE 0xFFFF

enum
{
    RIBGFX_CHAMPION,
    RIBGFX_CONTEST_NORMAL,
    RIBGFX_CONTEST_SUPER,
    RIBGFX_CONTEST_HYPER,
    RIBGFX_CONTEST_MASTER,
    RIBGFX_WINNING,
    RIBGFX_VICTORY,
    RIBGFX_ARTIST,
    RIBGFX_EFFORT,
    RIBGFX_GIFT_1,
    RIBGFX_GIFT_2,
    RIBGFX_GIFT_3,
    RIBGFX_COUNT,
};

enum
{
    GFXTAG_RIB_SMALL = 0x7B80,           /* hack_pokenav.c（0x7B40〜）・bw_start_menu.c（0x7B00〜）とぶつからない */
    GFXTAG_RIB_BIG,
    GFXTAG_RIB_CURSOR,
    PALTAG_RIB_1,                        /* 以降 5 つ（エメラルドの PALTAG_RIBBON_ICONS_1〜5） */
    PALTAG_RIB_2,
    PALTAG_RIB_3,
    PALTAG_RIB_4,
    PALTAG_RIB_5,
    PALTAG_RIB_CURSOR,
};

enum
{
    WIN_TITLE,
    WIN_INFO,
    WIN_DESC,
    WIN_COUNT,
};

enum
{
    BG_TEXT,
    BG_1,
    BG_2,
    BG_BACKGROUND,
};

/* ---- リボンの表（名前は出さない。説明 2 行・絵・色。仮の文は 1 か所＝この表で直す）---- */
struct RibInfo
{
    const u8 *line1;
    const u8 *line2;
    u8 gfx;                      /* RIBGFX_* */
    u8 pal;                      /* 0〜4（icons1〜5.pal） */
};

#define RIB_CONTEST(name, pal)                                                                   \
    {gText_PnavRib_##name, gText_PnavRib_RankNormal, RIBGFX_CONTEST_NORMAL, pal},                 \
    {gText_PnavRib_##name, gText_PnavRib_RankSuper, RIBGFX_CONTEST_SUPER, pal},                   \
    {gText_PnavRib_##name, gText_PnavRib_RankHyper, RIBGFX_CONTEST_HYPER, pal},                   \
    {gText_PnavRib_##name, gText_PnavRib_RankMaster, RIBGFX_CONTEST_MASTER, pal}

static const struct RibInfo sRibInfo[RIB_COUNT] =
{
    /* 0 チャンピオン */ {gText_PnavRib_Champion1, gText_PnavRib_Champion2, RIBGFX_CHAMPION, 0},
    /* 1〜4 クール   */ RIB_CONTEST(Cool, 0),
    /* 5〜8 ビューティ */ RIB_CONTEST(Beauty, 1),
    /* 9〜12 キュート */ RIB_CONTEST(Cute, 2),
    /* 13〜16 スマート */ RIB_CONTEST(Smart, 3),
    /* 17〜20 タフ   */ RIB_CONTEST(Tough, 4),
    /* 21 ウイニング */ {gText_PnavRib_Winning1, gText_PnavRib_Won2, RIBGFX_WINNING, 0},
    /* 22 ビクトリー */ {gText_PnavRib_Victory1, gText_PnavRib_Won2, RIBGFX_VICTORY, 0},
    /* 23 アーティスト */ {gText_PnavRib_Artist1, gText_PnavRib_Artist2, RIBGFX_ARTIST, 1},
    /* 24 がんばり   */ {gText_PnavRib_Effort1, gText_PnavRib_Effort2, RIBGFX_EFFORT, 2},
    /* 25 マリン     */ {gText_PnavRib_Gift1, gText_PnavRib_Gift2, RIBGFX_GIFT_1, 1},     /* 配布リボンの共通の文（ユーザー決定 2026-11-09「リボン説明おなじで」） */
    /* 26 ランド     */ {gText_PnavRib_Gift1, gText_PnavRib_Gift2, RIBGFX_GIFT_1, 3},     /* 配布リボンの共通の文（ユーザー決定 2026-11-09「リボン説明おなじで」） */
    /* 27 スカイ     */ {gText_PnavRib_Gift1, gText_PnavRib_Gift2, RIBGFX_GIFT_1, 4},     /* 配布リボンの共通の文（ユーザー決定 2026-11-09「リボン説明おなじで」） */
    /* 28 カントリー */ {gText_PnavRib_Gift1, gText_PnavRib_Gift2, RIBGFX_GIFT_2, 3},     /* 配布リボンの共通の文（ユーザー決定 2026-11-09「リボン説明おなじで」） */
    /* 29 ナショナル */ {gText_PnavRib_National1, gText_PnavRib_National2, RIBGFX_GIFT_2, 4},   /* リライブで付く（確定の文） */
    /* 30 アース     */ {gText_PnavRib_Gift1, gText_PnavRib_Gift2, RIBGFX_GIFT_3, 0},     /* 配布リボンの共通の文（ユーザー決定 2026-11-09「リボン説明おなじで」） */
    /* 31 ワールド   */ {gText_PnavRib_Gift1, gText_PnavRib_Gift2, RIBGFX_GIFT_3, 1},     /* 配布リボンの共通の文（ユーザー決定 2026-11-09「リボン説明おなじで」） */
};

/* ---- 80 バイト形のリボンのビット（HackRibbon_Bits・RIBBON_MASK の 27 ビット）から ribbonId の並びを作る ---- */
/* ビット: 0〜14 = クール・ビューティ・キュート・スマート・タフ（各 3 ビット・段 0〜4）／15 チャンピオン／16 ウイニング／17 ビクトリー／18 アーティスト／19 がんばり／20〜26 ギフト 1〜7 */
#define RIB_BIT_CHAMPION 15
#define RIB_BIT_WINNING 16
#define RIB_BIT_GIFT 20

static u8 BuildRibbonIds(u32 bits, u8 *ids, u8 *numNormal)
{
    u8 n = 0, i, j, lv;

    if (bits & (1u << RIB_BIT_CHAMPION))
        ids[n++] = 0;
    for (i = 0; i < 5; ++i)
    {
        lv = (bits >> (i * 3)) & 7;
        if (lv > 4)
            lv = 4;
        for (j = 0; j < lv; ++j)
            ids[n++] = 1 + i * 4 + j;
    }
    for (i = 0; i < 4; ++i)
        if (bits & (1u << (RIB_BIT_WINNING + i)))
            ids[n++] = 21 + i;
    *numNormal = n;
    for (i = 0; i < 7; ++i)
        if (bits & (1u << (RIB_BIT_GIFT + i)))
            ids[n++] = RIB_NORMAL_COUNT + i;
    return n;
}

struct RibUi
{
    u8 mode;                     /* 0＝ポケモンの選択・1＝リボンの選択 */
    u8 closing;
    u8 numMons;
    u8 monIdx;                   /* mons[] の添字 */
    u8 mons[PARTY_SIZE];         /* 手持ちの番号（卵を除く） */
    u8 numNormal;
    u8 numRibbons;               /* normal ＋ gift */
    u8 sel;                      /* ids[] の添字 */
    u8 ids[RIB_COUNT];
    u8 anyRibbon;                /* 手持ちの誰かがリボンを持っているか */
    u8 monSprite;
    u8 cursorSprite;
    u8 bigSprite;
    u8 iconSprites[RIB_COUNT];
    u8 *bgTilemapBuffer;
};

#define sRib (*((struct RibUi **)RIB_STATE_PTR))

static const struct BgTemplate sBgTemplates[] =
{
    [BG_TEXT] = {.bg = BG_TEXT, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    [BG_1] = {.bg = BG_1, .charBaseIndex = 1, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
    [BG_2] = {.bg = BG_2, .charBaseIndex = 2, .mapBaseIndex = 29, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
    [BG_BACKGROUND] = {.bg = BG_BACKGROUND, .charBaseIndex = 3, .mapBaseIndex = 28, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TITLE] = {.bg = BG_TEXT, .tilemapLeft = 0, .tilemapTop = 0, .width = 14, .height = 4, .paletteNum = 14, .baseBlock = 1},
    [WIN_INFO] = {.bg = BG_TEXT, .tilemapLeft = 1, .tilemapTop = 6, .width = 10, .height = 4, .paletteNum = 14, .baseBlock = 60},
    [WIN_DESC] = {.bg = BG_TEXT, .tilemapLeft = 0, .tilemapTop = 16, .width = 30, .height = 4, .paletteNum = 14, .baseBlock = 123},
    DUMMY_WIN_TEMPLATE,
};

static const struct TextColor sWhiteText = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GREY};

static const struct OamData sSmallOam = {.affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .shape = SPRITE_SHAPE(16x16), .size = SPRITE_SIZE(16x16), .priority = 1};
static const struct OamData sBigOam = {.affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .shape = SPRITE_SHAPE(32x32), .size = SPRITE_SIZE(32x32), .priority = 1};
static const struct OamData sCursorOam = {.affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .shape = SPRITE_SHAPE(16x16), .size = SPRITE_SIZE(16x16), .priority = 0};

#define SA(n) static const union AnimCmd sSmallAnim##n[] = {ANIMCMD_FRAME((n) * 4, 0), ANIMCMD_END};
#define BA(n) static const union AnimCmd sBigAnim##n[] = {ANIMCMD_FRAME((n) * 16, 0), ANIMCMD_END};
SA(0) SA(1) SA(2) SA(3) SA(4) SA(5) SA(6) SA(7) SA(8) SA(9) SA(10) SA(11)
BA(0) BA(1) BA(2) BA(3) BA(4) BA(5) BA(6) BA(7) BA(8) BA(9) BA(10) BA(11)
static const union AnimCmd *const sSmallAnimTable[RIBGFX_COUNT] =
{
    sSmallAnim0, sSmallAnim1, sSmallAnim2, sSmallAnim3, sSmallAnim4, sSmallAnim5,
    sSmallAnim6, sSmallAnim7, sSmallAnim8, sSmallAnim9, sSmallAnim10, sSmallAnim11,
};
static const union AnimCmd *const sBigAnimTable[RIBGFX_COUNT] =
{
    sBigAnim0, sBigAnim1, sBigAnim2, sBigAnim3, sBigAnim4, sBigAnim5,
    sBigAnim6, sBigAnim7, sBigAnim8, sBigAnim9, sBigAnim10, sBigAnim11,
};
static const union AnimCmd sCursorAnim[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd *const sCursorAnimTable[] = {sCursorAnim};

static const struct SpriteSheet sSmallSheet = {sRibGfxSmall, sizeof(sRibGfxSmall), GFXTAG_RIB_SMALL};
static const struct SpriteSheet sBigSheet = {sRibGfxBig, sizeof(sRibGfxBig), GFXTAG_RIB_BIG};
static const struct SpriteSheet sCursorSheet = {sRibGfxCursor, sizeof(sRibGfxCursor), GFXTAG_RIB_CURSOR};
static const struct SpritePalette sCursorPal = {sRibGfxCursorPal, PALTAG_RIB_CURSOR};
static const struct SpriteTemplate sSmallTemplate = {GFXTAG_RIB_SMALL, RIB_TAG_NONE, &sSmallOam, sSmallAnimTable, NULL, gDummySpriteAffineAnimTable, SpriteCallbackDummy};
static const struct SpriteTemplate sBigTemplate = {GFXTAG_RIB_BIG, RIB_TAG_NONE, &sBigOam, sBigAnimTable, NULL, gDummySpriteAffineAnimTable, SpriteCallbackDummy};
static const struct SpriteTemplate sCursorTemplate = {GFXTAG_RIB_CURSOR, PALTAG_RIB_CURSOR, &sCursorOam, sCursorAnimTable, NULL, gDummySpriteAffineAnimTable, SpriteCallbackDummy};

static void Task_FadeIn(u8 taskId);
static void Task_Input(u8 taskId);
static void Task_FadeOut(u8 taskId);

/* ---- 共通の準備（hack_pokenav.c と同じ）---- */

static void ClearVramOamPlttRegs(void)
{
    DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
}

static void ClearTasksAndGraphicalStructs(void)
{
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetTempTileDataBuffers();
    ResetPaletteFade();
    FreeAllSpritePalettes();
}

static void VBlankCB_Ribbon(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2_Ribbon(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

/* ---- 手持ちとリボン ---- */

static void BuildMonList(void)
{
    u8 i;

    sRib->numMons = 0;
    sRib->anyRibbon = 0;
    for (i = 0; i < PARTY_SIZE; ++i)
    {
        struct Pokemon *mon = &gPlayerParty[i];

        if (GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG, NULL))
            continue;
        sRib->mons[sRib->numMons++] = i;
        if (HackRibbon_Bits((const struct BoxPokemon *) mon) != 0)
            sRib->anyRibbon = 1;
    }
}

static struct Pokemon *CurMon(void)
{
    return &gPlayerParty[sRib->mons[sRib->monIdx]];
}

static void LoadCurRibbons(void)
{
    sRib->numRibbons = BuildRibbonIds(HackRibbon_Bits((const struct BoxPokemon *) CurMon()), sRib->ids, &sRib->numNormal);
    sRib->sel = 0;
}

/* ---- 描く ---- */

static void LoadBgGfx(void)
{
    Rib_DecompressAndCopyTileDataToVram(BG_BACKGROUND, BwSmBgTiles, 0, 0, 0);
    LZDecompressWram(BwSmBgMap, sRib->bgTilemapBuffer);
    LoadPalette(BwSmBgPal, 0, 0x20);
    LoadPalette(stdpal_get(0), 14 * 16, 0x20);      /* 文字の色（白・濃い灰） */
}

static void CleanWindow(u8 windowId)
{
    FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
}

static void CommitWindow(u8 windowId)
{
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_BOTH);
}

static void IconPos(u8 pos, s16 *x, s16 *y)
{
    u8 col, row;

    if (pos < sRib->numNormal)
    {
        col = pos % RIB_PER_ROW;
        row = pos / RIB_PER_ROW;
    }
    else
    {
        col = pos - sRib->numNormal;
        row = RIB_GIFT_ROW;
    }
    *x = RIB_GRID_X + col * RIB_STEP + 8;
    *y = RIB_GRID_Y + row * RIB_STEP + 8;
}

static void PrintInfo(void)
{
    u8 *p;
    u8 total = sRib->numRibbons;

    CleanWindow(WIN_INFO);
    GetMonNickname(CurMon(), gStringVar1);
    WindowPrint(WIN_INFO, RIB_FONT, 0, 0, &sWhiteText, 0, gStringVar1);
    p = StringCopy(gStringVar4, gText_PnavRib_CountHead);
    *p++ = CHAR_SPACE;
    p = ConvertIntToDecimalStringN(p, total, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringCopy(p, gText_PnavRib_CountTail);
    WindowPrint(WIN_INFO, RIB_FONT, 0, 16, &sWhiteText, 0, gStringVar4);
    CommitWindow(WIN_INFO);
}

static void PrintDesc(void)
{
    CleanWindow(WIN_DESC);
    if (sRib->mode == 1 && sRib->numRibbons != 0)
    {
        const struct RibInfo *info = &sRibInfo[sRib->ids[sRib->sel]];

        WindowPrint(WIN_DESC, RIB_FONT, 40, 2, &sWhiteText, 0, info->line1);
        WindowPrint(WIN_DESC, RIB_FONT, 40, 16, &sWhiteText, 0, info->line2);
    }
    else if (sRib->anyRibbon)
    {
        WindowPrint(WIN_DESC, RIB_FONT, 8, 8, &sWhiteText, 0, gText_PnavRib_Help);
    }
    else
    {
        WindowPrint(WIN_DESC, RIB_FONT, 8, 8, &sWhiteText, 0, gText_PnavRib_None);
    }
    CommitWindow(WIN_DESC);
}

static void ShowMonIcon(void)
{
    struct Pokemon *mon = CurMon();
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);

    if (sRib->monSprite != 0xFF)
        DestroyMonIcon(&gSprites[sRib->monSprite]);
    LoadMonIconPalette(species);
    sRib->monSprite = CreateMonIcon(species, SpriteCallbackDummy, RIB_MON_X, RIB_MON_Y, 0, GetMonData(mon, MON_DATA_PERSONALITY, NULL), FALSE);
}

static void DestroyGrid(void)
{
    u8 i;

    for (i = 0; i < RIB_COUNT; ++i)
        if (sRib->iconSprites[i] != 0xFF)
        {
            DestroySprite(&gSprites[sRib->iconSprites[i]]);
            sRib->iconSprites[i] = 0xFF;
        }
}

static void DrawGrid(void)
{
    u8 i;

    DestroyGrid();
    for (i = 0; i < sRib->numRibbons; ++i)
    {
        const struct RibInfo *info = &sRibInfo[sRib->ids[i]];
        s16 x, y;
        u8 id;

        IconPos(i, &x, &y);
        id = CreateSprite(&sSmallTemplate, x, y, 2);
        gSprites[id].oam.paletteNum = IndexOfSpritePaletteTag(PALTAG_RIB_1 + info->pal);
        StartSpriteAnim(&gSprites[id], info->gfx);
        sRib->iconSprites[i] = id;
    }
}

static void UpdateCursorAndBig(void)
{
    struct Sprite *cursor = &gSprites[sRib->cursorSprite];
    struct Sprite *big = &gSprites[sRib->bigSprite];

    if (sRib->mode == 1 && sRib->numRibbons != 0)
    {
        const struct RibInfo *info = &sRibInfo[sRib->ids[sRib->sel]];
        s16 x, y;

        IconPos(sRib->sel, &x, &y);
        cursor->pos1.x = x;
        cursor->pos1.y = y;
        cursor->invisible = FALSE;
        big->oam.paletteNum = IndexOfSpritePaletteTag(PALTAG_RIB_1 + info->pal);
        StartSpriteAnim(big, info->gfx);
        big->invisible = FALSE;
    }
    else
    {
        cursor->invisible = TRUE;
        big->invisible = TRUE;
    }
}

static void RefreshMon(void)
{
    LoadCurRibbons();
    ShowMonIcon();
    DrawGrid();
    PrintInfo();
    PrintDesc();
    UpdateCursorAndBig();
}

static void InitGui(void)
{
    u8 i;

    LoadSpriteSheet(&sSmallSheet);
    LoadSpriteSheet(&sBigSheet);
    LoadSpriteSheet(&sCursorSheet);
    for (i = 0; i < 5; ++i)
    {
        struct SpritePalette pal;

        pal.data = sRibGfxPal[i];
        pal.tag = PALTAG_RIB_1 + i;
        LoadSpritePalette(&pal);
    }
    LoadSpritePalette(&sCursorPal);
    for (i = 0; i < RIB_COUNT; ++i)
        sRib->iconSprites[i] = 0xFF;
    sRib->monSprite = 0xFF;
    sRib->cursorSprite = CreateSprite(&sCursorTemplate, 0, 0, 0);
    sRib->bigSprite = CreateSprite(&sBigTemplate, RIB_BIG_X, RIB_BIG_Y, 1);
    gSprites[sRib->cursorSprite].invisible = TRUE;
    gSprites[sRib->bigSprite].invisible = TRUE;
    for (i = 0; i < WIN_COUNT; ++i)
        CleanWindow(i);
    WindowPrint(WIN_TITLE, RIB_FONT, 3, 0, &sWhiteText, 0, gText_Pnav_Ribbon);
    CommitWindow(WIN_TITLE);
    RefreshMon();
}

/* ---- 画面の流れ ---- */

static void CB2_Ribbon(void)
{
    switch (gMain.state)
    {
    case 0:
    default:
        SetBGMVolume_SuppressHelpSystemReduction(160);
        SetVBlankCallback(NULL);
        ClearVramOamPlttRegs();
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON);
        gMain.state++;
        break;
    case 1:
        ClearTasksAndGraphicalStructs();
        gMain.state++;
        break;
    case 2:
        sRib->bgTilemapBuffer = Malloc(0x1000);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
        SetBgTilemapBuffer(BG_BACKGROUND, sRib->bgTilemapBuffer);
        gMain.state++;
        break;
    case 3:
        BuildMonList();
        LoadBgGfx();
        gMain.state++;
        break;
    case 4:
        if (!Rib_FreeTempTileDataBuffersIfPossible())
        {
            ShowBg(BG_TEXT);
            ShowBg(BG_BACKGROUND);
            CopyBgTilemapBufferToVram(BG_BACKGROUND);
            gMain.state++;
        }
        break;
    case 5:
        Free(sRib->bgTilemapBuffer);
        sRib->bgTilemapBuffer = NULL;
        InitWindows(sWindowTemplates);
        DeactivateAllTextPrinters();
        gMain.state++;
        break;
    case 6:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    case 7:
        SetVBlankCallback(VBlankCB_Ribbon);
        InitGui();
        CreateTask(Task_FadeIn, 0);
        SetMainCallback2(MainCB2_Ribbon);
        gMain.state = 0;
        break;
    }
}

static void Task_FadeIn(u8 taskId)
{
    if (!gPaletteFade->active)
        gTasks[taskId].func = Task_Input;
}

static void MoveMon(s8 dir)
{
    s16 n = (s16) sRib->monIdx + dir;

    if (n < 0 || n >= sRib->numMons)
        return;
    sRib->monIdx = n;
    PlaySE(SE_SELECT);
    RefreshMon();
}

static void MoveSel(u8 newSel)
{
    PlaySE(SE_SELECT);
    sRib->sel = newSel;
    PrintDesc();
    UpdateCursorAndBig();
}

/* エメラルドの TrySelectRibbonUp／Down／Left／Right と同じ動き（pokenav_ribbons_summary.c:268〜372） */
static bool8 TryUp(void)
{
    u8 sel = sRib->sel, n = sRib->numNormal;

    if (sel < n)
    {
        if (sel < RIB_PER_ROW)
            return FALSE;
        MoveSel(sel - RIB_PER_ROW);
        return TRUE;
    }
    if (n != 0)
    {
        u8 to = ((n - 1) / RIB_PER_ROW) * RIB_PER_ROW + (sel - n);

        if (to >= n)
            to = n - 1;
        MoveSel(to);
        return TRUE;
    }
    return FALSE;
}

static bool8 TryDown(void)
{
    u8 sel = sRib->sel, n = sRib->numNormal, gifts = sRib->numRibbons - sRib->numNormal;

    if (sel >= n)
        return FALSE;
    if (sel / RIB_PER_ROW < (n - 1) / RIB_PER_ROW)
    {
        u8 to = sel + RIB_PER_ROW;

        if (to >= n)
            to = n - 1;
        MoveSel(to);
        return TRUE;
    }
    if (gifts != 0)
    {
        u8 col = sel % RIB_PER_ROW;

        if (col >= gifts)
            col = gifts - 1;
        MoveSel(n + col);
        return TRUE;
    }
    return FALSE;
}

static bool8 TryLeft(void)
{
    u8 sel = sRib->sel;

    if (sel < sRib->numNormal ? (sel % RIB_PER_ROW) == 0 : sel == sRib->numNormal)
        return FALSE;
    MoveSel(sel - 1);
    return TRUE;
}

static bool8 TryRight(void)
{
    u8 sel = sRib->sel;

    if (sel < sRib->numNormal)
    {
        if (sel % RIB_PER_ROW >= RIB_PER_ROW - 1 || sel >= sRib->numNormal - 1)
            return FALSE;
    }
    else if (sel >= sRib->numRibbons - 1)
        return FALSE;
    MoveSel(sel + 1);
    return TRUE;
}

static void Task_Input(u8 taskId)
{
    if (sRib->mode == 0)
    {
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_PC_OFF);
            sRib->closing = 1;
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_FadeOut;
        }
        else if (JOY_NEW(A_BUTTON) && sRib->numRibbons != 0)
        {
            PlaySE(SE_SELECT);
            sRib->mode = 1;
            sRib->sel = 0;
            PrintDesc();
            UpdateCursorAndBig();
        }
        else if (JOY_NEW_AND_REPEATED(DPAD_LEFT))
            MoveMon(-1);
        else if (JOY_NEW_AND_REPEATED(DPAD_RIGHT))
            MoveMon(1);
    }
    else
    {
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            sRib->mode = 0;
            PrintDesc();
            UpdateCursorAndBig();
        }
        else if (JOY_NEW_AND_REPEATED(DPAD_UP))
            TryUp();
        else if (JOY_NEW_AND_REPEATED(DPAD_DOWN))
            TryDown();
        else if (JOY_NEW_AND_REPEATED(DPAD_LEFT))
            TryLeft();
        else if (JOY_NEW_AND_REPEATED(DPAD_RIGHT))
            TryRight();
    }
}

static void Task_FadeOut(u8 taskId)
{
    if (!gPaletteFade->active)
    {
        Free(sRib);
        sRib = NULL;
        FreeAllWindowBuffers();
        BGMVolumeMax_EnableHelpSystemReduction();
        DestroyTask(taskId);
        SetMainCallback2(CB2_HackPokenavReturn);
    }
}

/* ---- 入口（ポケナビの一覧から）---- */

void CB2_HackPokenavRibbon(void)
{
    gMain.savedCallback = NULL;
    sRib = Calloc(sizeof(struct RibUi));
    SetMainCallback2(CB2_Ribbon);
}
