#include "defines.h"
#include "../include/bg.h"
#include "../include/decompress.h"
#include "../include/event_data.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/menu.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/text_window.h"
#include "../include/window.h"
#include "../include/constants/flags.h"
#include "../include/constants/songs.h"
#include "../include/gba/macro.h"
#include "../include/new/dns.h"

/*
    ポケナビ（2026-11-07・FEAT-pokenav・ユーザー「ポケナビ作り始めて 機能はあとから追加できるように」・設計 work/tmp/audit/PNAV_DESIGN.md）

    スタートメニューの「ポケナビ」（src/bw_start_menu.c の BWSM_POKENAV）から開く、機能の一覧の画面。
    エメラルドのポケナビ（LoopedTask＋19 個のサブ構造）は持ち込まず、BW 風スタートメニューと同じ作り
    （MainCB2＋Task＋Calloc した入れ物）で自作した。見た目（背景・枠・アイコンの置き方）は bw_start_menu.c と同じ絵を使う。

    ## 機能を足すとき（表に 1 行＋画面 1 つ）
      1. 画面（CB2）を書く。戻るときは SetMainCallback2(CB2_HackPokenavReturn)（カーソルを元に戻して開き直す）
      2. 下の sFeatures[] に 1 行足す（名前・説明・旗・アイコン・開く関数・条件）。アイコンが新しければ sIconSheets[]・sIconPals[]・
         sIconTemplates[] と PNAV_ICON_* にも足す
      3. strings/hack_pokenav.string に名前と説明を足す
      4. tools/pokenav/verify_pokenav.py の件数（機能の数・アイコンの数）を更新
    開く関数は、画面を閉じる処理（状態の解放）が済んだあとに呼ばれる。ここから SetMainCallback2 で子の画面へ渡す。
    子から戻る CB2 は CB2_HackPokenavReturn（Var8002＝カーソル位置）。ちずは素の InitRegionMapWithExitCB の戻り先に渡す。
    でんわは gMain.savedCallback に CB2_HackPokenavReturn を入れて渡す（src/hack_phone.c が見て、ポケナビへ戻る）。

    ## 状態を持たない
    書き込み可能な static は無い（6.13）。状態は Calloc した入れ物（EWRAM）。ポインタの語は bw_start_menu.c と同じ 0x203E038
    （スタートメニューは開く前に自分を片づける＝同時には使われない）。
    Var8000／8001 は触らない（スタートメニューへ戻るときのカーソルがそこにある）。ポケナビの戻りのカーソルは Var8002。

    ## 素の JP 版の番地（BPRJ.ld に無いもの。verify_pokenav.py が毎回確かめる）
      InitRegionMapWithExitCB 080C1110（pokefirered region_map.c:987・type は +0x4796・戻り先は +0x47BC）
      DecompressAndCopyTileDataToVram 080F78D0 / FreeTempTileDataBuffersIfPossible 080F7884
*/

#define Pnav_InitRegionMapWithExitCB ((void (*)(u8, MainCallback))(0x080C1110 + 1))
#define Pnav_DecompressAndCopyTileDataToVram \
    ((void (*)(u8, const void *, u32, u16, u8))(0x080F78D0 + 1))
#define Pnav_FreeTempTileDataBuffersIfPossible ((bool8 (*)(void))(0x080F7884 + 1))
#define PNAV_REGIONMAP_TYPE_NORMAL 0

void __attribute__((long_call)) ScanlineEffect_Stop(void);      /* BPRJ.ld 08087A48 */
extern void CB2_HackPhoneBook(void);    /* src/hack_phone.c */
extern void CB2_HackPokenavRibbon(void);    /* src/hack_pokenav_ribbon.c（FEAT-relive-ribbon 段 2） */
void CB2_HackPokenav(void);
void CB2_HackPokenavReturn(void);

/* ---- 絵（bw_start_menu.c と同じ物＋ポケナビ用の 1 枚）---- */
extern const u8 BwSmBgTiles[];
extern const u8 BwSmBgMap[];
extern const u16 BwSmBgPal[];
extern const u8 BwSmPanelTiles[];
extern const u16 BwSmPanelPal[];
extern const u8 BwSmExitTiles[];
extern const u16 BwSmExitPal[];
extern const u8 BwSmPhoneTiles[];
extern const u16 BwSmPhonePal[];
extern const u8 BwSmMapTiles[];
extern const u16 BwSmMapPal[];
extern const u8 BwSmRibbonTiles[];
extern const u16 BwSmRibbonPal[];

/* ---- 文字列（strings/hack_pokenav.string）---- */
extern const u8 gText_Pnav_Title[];
extern const u8 gText_Pnav_Map[];
extern const u8 gText_Pnav_MapDesc[];
extern const u8 gText_Pnav_Phone[];
extern const u8 gText_Pnav_PhoneDesc[];
extern const u8 gText_Pnav_Ribbon[];
extern const u8 gText_Pnav_RibbonDesc[];

#define PANEL_X 34
#define PANEL_Y 41
#define HSPACING 20
#define VSPACING 6
#define PNAV_FONT 2
#define PNAV_ON_SCREEN 6                 /* 2 列 x 3 段 */
#define PNAV_MAX_FEATURES 16             /* 機能表の上限（状態の配列の大きさ）。越えるなら増やす（verify_pokenav.py が見る） */
#define PNAV_STATE_PTR 0x203E038         /* bw_start_menu.c と同じ「大きな画面の状態」の語 */
#define PNAV_CURSOR_VAR 0x8002           /* 子の画面から戻るときのカーソル位置 */

enum
{
    GFXTAG_PNAV_PANEL = 0x7B40,          /* bw_start_menu.c（0x7B00〜）とぶつからない */
    GFXTAG_PNAV_EXIT,
    GFXTAG_PNAV_ICON0,                   /* 以降 PNAV_ICON_COUNT 個ぶんがアイコン */
};

enum
{
    PNAV_ICON_MAP,
    PNAV_ICON_PHONE,
    PNAV_ICON_RIBBON,
    PNAV_ICON_COUNT,
};

enum
{
    WIN_TOPBAR_TITLE,
    WIN_ITEMS,
    WIN_BOTTOMBAR,
    WIN_COUNT,
};

enum
{
    BG_TEXT,
    BG_1,
    BG_2,
    BG_BACKGROUND,
};

struct PnavState
{
    u8 cursor;              /* 一覧の中の番号（画面の枠ではない） */
    u8 scroll;              /* 画面の先頭の段 */
    u8 numItems;
    u8 numOnScreen;
    u8 items[PNAV_MAX_FEATURES];    /* 出す機能の番号（sFeatures の添字） */
    u8 onScreen[PNAV_ON_SCREEN];
    u8 iconSpriteIds[PNAV_ON_SCREEN];
    u8 *bgTilemapBuffer;
    u8 closing;             /* 0＝動いている・1＝スタートメニューへ戻る・2＝機能を開く */
};

#define sState (*((struct PnavState **)PNAV_STATE_PTR))

/* ================================================================ */
/* 機能表（ここに 1 行足すと一覧に出る）                              */
/* ================================================================ */

struct PokenavFeature
{
    const u8 *name;              /* ひらがな・カタカナ */
    const u8 *desc;              /* 下の帯に出す 1 行の説明 */
    u16 flag;                    /* 0 でなければ、立っているときだけ出す */
    u8 icon;                     /* PNAV_ICON_* */
    void (*open)(void);          /* 画面を閉じたあとに呼ぶ。子の画面へ SetMainCallback2 する */
    bool8 (*isAvailable)(void);  /* 旗以外の条件（NULL なら常に出す） */
};

static void Pnav_OpenMap(void);
static void Pnav_OpenPhone(void);
static void Pnav_OpenRibbon(void);

static const struct PokenavFeature sFeatures[] =
{
    {gText_Pnav_Map,   gText_Pnav_MapDesc,   0, PNAV_ICON_MAP,   Pnav_OpenMap,   NULL},
    {gText_Pnav_Phone, gText_Pnav_PhoneDesc, 0, PNAV_ICON_PHONE, Pnav_OpenPhone, NULL},
    {gText_Pnav_Ribbon, gText_Pnav_RibbonDesc, 0, PNAV_ICON_RIBBON, Pnav_OpenRibbon, NULL},
};
#define PNAV_FEATURE_COUNT NELEMS(sFeatures)

static const struct SpriteSheet sIconSheets[PNAV_ICON_COUNT] =
{
    [PNAV_ICON_MAP] = {BwSmMapTiles, 32 * 32, GFXTAG_PNAV_ICON0 + PNAV_ICON_MAP},
    [PNAV_ICON_PHONE] = {BwSmPhoneTiles, 32 * 32, GFXTAG_PNAV_ICON0 + PNAV_ICON_PHONE},
    [PNAV_ICON_RIBBON] = {BwSmRibbonTiles, 32 * 32, GFXTAG_PNAV_ICON0 + PNAV_ICON_RIBBON},
};

static const struct SpritePalette sIconPals[PNAV_ICON_COUNT] =
{
    [PNAV_ICON_MAP] = {BwSmMapPal, GFXTAG_PNAV_ICON0 + PNAV_ICON_MAP},
    [PNAV_ICON_PHONE] = {BwSmPhonePal, GFXTAG_PNAV_ICON0 + PNAV_ICON_PHONE},
    [PNAV_ICON_RIBBON] = {BwSmRibbonPal, GFXTAG_PNAV_ICON0 + PNAV_ICON_RIBBON},
};

/* ---- 開く関数 ---- */

static void Pnav_OpenMap(void)
{
    /* 素のタウンマップ（ホウエン・ジョウト・シンオウ・キタカミは hoenn_townmap.c のフックが InitRegionMapType の中で効く）。
       出口で CB2_HackPokenavReturn へ戻る（region_map.c:1330 の SetMainCallback2(savedCallback)） */
    Pnav_InitRegionMapWithExitCB(PNAV_REGIONMAP_TYPE_NORMAL, CB2_HackPokenavReturn);
}

static void Pnav_OpenPhone(void)
{
    /* 電話帳（src/hack_phone.c）。B で戻るとき、savedCallback がポケナビの戻りならそこへ。かけたときはフィールドで台本 */
    gMain.savedCallback = CB2_HackPokenavReturn;
    SetMainCallback2(CB2_HackPhoneBook);
}

static void Pnav_OpenRibbon(void)
{
    /* リボン画面（src/hack_pokenav_ribbon.c）。B で CB2_HackPokenavReturn へ戻る */
    SetMainCallback2(CB2_HackPokenavRibbon);
}

/* ================================================================ */

static const struct BgTemplate sBgTemplates[] =
{
    [BG_TEXT] = {.bg = BG_TEXT, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    [BG_1] = {.bg = BG_1, .charBaseIndex = 1, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
    [BG_2] = {.bg = BG_2, .charBaseIndex = 2, .mapBaseIndex = 29, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
    [BG_BACKGROUND] = {.bg = BG_BACKGROUND, .charBaseIndex = 3, .mapBaseIndex = 28, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TOPBAR_TITLE] = {.bg = BG_TEXT, .tilemapLeft = 0, .tilemapTop = 0, .width = 14, .height = 4, .paletteNum = 14, .baseBlock = 1},
    [WIN_ITEMS] = {.bg = BG_TEXT, .tilemapLeft = 0, .tilemapTop = 4, .width = 30, .height = 12, .paletteNum = 14, .baseBlock = 123},
    [WIN_BOTTOMBAR] = {.bg = BG_TEXT, .tilemapLeft = 0, .tilemapTop = 16, .width = 30, .height = 4, .paletteNum = 14, .baseBlock = 484},
    DUMMY_WIN_TEMPLATE,
};

static const struct TextColor sWhiteText = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GREY};

static const struct OamData sPanelOam = {.affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_BLEND, .shape = SPRITE_SHAPE(64x64), .size = SPRITE_SIZE(64x64), .priority = 1};
static const struct OamData sExitOam = {.affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .shape = SPRITE_SHAPE(16x16), .size = SPRITE_SIZE(16x16), .priority = 1};
static const struct OamData sIconOam = {.affineMode = ST_OAM_AFFINE_OFF, .objMode = ST_OAM_OBJ_NORMAL, .shape = SPRITE_SHAPE(32x32), .size = SPRITE_SIZE(32x32), .priority = 1};

static const union AnimCmd sAnimPanelUnselected1[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd sAnimPanelUnselected2[] = {ANIMCMD_FRAME(64, 0), ANIMCMD_END};
static const union AnimCmd sAnimPanelSelected1[] = {ANIMCMD_FRAME(128, 0), ANIMCMD_END};
static const union AnimCmd sAnimPanelSelected2[] = {ANIMCMD_FRAME(192, 0), ANIMCMD_JUMP(0)};
static const union AnimCmd *const sAnimTablePanel1[] = {sAnimPanelUnselected1, sAnimPanelSelected1};
static const union AnimCmd *const sAnimTablePanel2[] = {sAnimPanelUnselected2, sAnimPanelSelected2};
static const union AnimCmd sAnimExit[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd sAnimExitSelected[] = {ANIMCMD_FRAME(4, 0), ANIMCMD_END};
static const union AnimCmd *const sAnimTableExit[] = {sAnimExit, sAnimExitSelected};
static const union AnimCmd sAnimIcon[] = {ANIMCMD_FRAME(0, 0), ANIMCMD_END};
static const union AnimCmd sAnimIconSelected[] = {ANIMCMD_FRAME(16, 0), ANIMCMD_END};
static const union AnimCmd *const sAnimTableIcon[] = {sAnimIcon, sAnimIconSelected};

static void PanelCallback(struct Sprite *sprite);
static void IconCallback(struct Sprite *sprite);

static const struct SpriteSheet sPanelSheet = {BwSmPanelTiles, (64 * 64 * 4) / 2, GFXTAG_PNAV_PANEL};
static const struct SpritePalette sPanelPal = {BwSmPanelPal, GFXTAG_PNAV_PANEL};
static const struct SpriteTemplate sPanel1Template = {GFXTAG_PNAV_PANEL, GFXTAG_PNAV_PANEL, &sPanelOam, sAnimTablePanel1, NULL, gDummySpriteAffineAnimTable, PanelCallback};
static const struct SpriteTemplate sPanel2Template = {GFXTAG_PNAV_PANEL, GFXTAG_PNAV_PANEL, &sPanelOam, sAnimTablePanel2, NULL, gDummySpriteAffineAnimTable, PanelCallback};
static const struct SpriteSheet sExitSheet = {BwSmExitTiles, 16 * 16, GFXTAG_PNAV_EXIT};
static const struct SpritePalette sExitPal = {BwSmExitPal, GFXTAG_PNAV_EXIT};
static const struct SpriteTemplate sExitTemplate = {GFXTAG_PNAV_EXIT, GFXTAG_PNAV_EXIT, &sExitOam, sAnimTableExit, NULL, gDummySpriteAffineAnimTable, PanelCallback};

#define PNAV_ICON_TEMPLATE(i) {GFXTAG_PNAV_ICON0 + (i), GFXTAG_PNAV_ICON0 + (i), &sIconOam, sAnimTableIcon, NULL, gDummySpriteAffineAnimTable, IconCallback}
static const struct SpriteTemplate sIconTemplates[PNAV_ICON_COUNT] =
{
    [PNAV_ICON_MAP] = PNAV_ICON_TEMPLATE(PNAV_ICON_MAP),
    [PNAV_ICON_PHONE] = PNAV_ICON_TEMPLATE(PNAV_ICON_PHONE),
    [PNAV_ICON_RIBBON] = PNAV_ICON_TEMPLATE(PNAV_ICON_RIBBON),
};

static void Task_FadeIn(u8 taskId);
static void Task_WaitForKeyPress(u8 taskId);
static void Task_FadeOut(u8 taskId);

/* ---- 共通の準備（bw_start_menu.c と同じ） ---- */

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

static void VBlankCB_Pokenav(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2_Pokenav(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

/* ---- 機能の一覧の組み立て ---- */

static void ConfigureOnScreen(void)
{
    u8 i, n = 0;

    for (i = 0; i < PNAV_ON_SCREEN; ++i)
    {
        if (i + 2 * sState->scroll >= sState->numItems)
            break;
        sState->onScreen[i] = sState->items[i + sState->scroll * 2];
        ++n;
    }
    sState->numOnScreen = n;
}

static u8 CursorSlot(void)
{
    return sState->cursor - 2 * sState->scroll;
}

static void BuildItems(void)
{
    u8 i, n = 0;

    for (i = 0; i < PNAV_FEATURE_COUNT && n < PNAV_MAX_FEATURES; ++i)
    {
        if (sFeatures[i].flag != 0 && !FlagGet(sFeatures[i].flag))
            continue;
        if (sFeatures[i].isAvailable != NULL && !sFeatures[i].isAvailable())
            continue;
        sState->items[n++] = i;
    }
    sState->numItems = n;
    if (sState->cursor >= n)
        sState->cursor = 0;
    if (sState->scroll > sState->cursor / 2 || sState->cursor / 2 > sState->scroll + 2)
        sState->scroll = (sState->cursor / 2 > 2) ? sState->cursor / 2 - 2 : 0;
    ConfigureOnScreen();
}

/* ---- 描く ---- */

static void LoadBgGfx(void)
{
    Pnav_DecompressAndCopyTileDataToVram(BG_BACKGROUND, BwSmBgTiles, 0, 0, 0);
    LZDecompressWram(BwSmBgMap, sState->bgTilemapBuffer);
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

static void PrintDescription(void)
{
    CleanWindow(WIN_BOTTOMBAR);
    if (sState->numItems != 0)
        WindowPrint(WIN_BOTTOMBAR, PNAV_FONT, 2, 13, &sWhiteText, 0, sFeatures[sState->items[sState->cursor]].desc);
    CommitWindow(WIN_BOTTOMBAR);
}

static void PrintItemNames(void)
{
    u8 row, col, n = 0;

    for (row = 0; row < 3 && n < sState->numOnScreen; ++row)
    {
        for (col = 0; col < 2 && n < sState->numOnScreen; ++col)
        {
            u16 x = (PANEL_X + (HSPACING + 64 / 2 + 64) * col) + 8;
            u16 y = (PANEL_Y + (VSPACING + 32) * row) - 40;

            WindowPrint(WIN_ITEMS, PNAV_FONT, x, y, &sWhiteText, 0, sFeatures[sState->onScreen[n]].name);
            ++n;
        }
    }
}

static void DrawPanels(void)
{
    u8 row, col, n = 0, spriteId;

    LoadSpriteSheet(&sPanelSheet);
    LoadSpritePalette(&sPanelPal);
    for (row = 0; row < 3; ++row)
    {
        for (col = 0; col < 2; ++col)
        {
            u16 x = PANEL_X + (HSPACING + 64 / 2 + 62) * col;
            u16 y = PANEL_Y + (VSPACING + 32) * row;
            u8 id1 = CreateSprite(&sPanel1Template, x, y, 0);
            u8 id2 = CreateSprite(&sPanel2Template, x + 64, y, 0);

            gSprites[id1].data[0] = n;
            gSprites[id2].data[0] = n;
            ++n;
        }
    }
    LoadSpritePalette(&sExitPal);
    LoadSpriteSheet(&sExitSheet);
    spriteId = CreateSprite(&sExitTemplate, 240 - 16, 160 - 11, 0);
    gSprites[spriteId].data[0] = 0xFF;
}

static void DrawIcons(void)
{
    u8 row, col, n = 0;

    for (row = 0; row < 3 && n < sState->numOnScreen; ++row)
    {
        for (col = 0; col < 2 && n < sState->numOnScreen; ++col)
        {
            u16 x = (PANEL_X - 11) + (HSPACING - 2 + 64 / 2 + 64) * col;
            u16 y = (PANEL_Y - 3) + (VSPACING + 32) * row;
            u8 icon = sFeatures[sState->onScreen[n]].icon;
            u8 spriteId;

            LoadSpriteSheet(&sIconSheets[icon]);
            LoadSpritePalette(&sIconPals[icon]);
            spriteId = CreateSprite(&sIconTemplates[icon], x, y, 0);
            gSprites[spriteId].data[0] = n;
            gSprites[spriteId].data[1] = 3;
            gSprites[spriteId].data[2] = y;
            gSprites[spriteId].data[3] = -1;
            sState->iconSpriteIds[n] = spriteId;
            ++n;
        }
    }
}

static void InitGui(void)
{
    u8 i;

    for (i = 0; i < WIN_COUNT; ++i)
        CleanWindow(i);
    DrawPanels();
    DrawIcons();
    PrintItemNames();
    WindowPrint(WIN_TOPBAR_TITLE, PNAV_FONT, 3, 0, &sWhiteText, 0, gText_Pnav_Title);
    PrintDescription();
    for (i = 0; i < WIN_COUNT; ++i)
        CommitWindow(i);
}

static void RefreshOptions(void)
{
    u8 i;

    CleanWindow(WIN_ITEMS);
    for (i = 0; i < sState->numOnScreen; ++i)
        DestroySpriteAndFreeResources(&gSprites[sState->iconSpriteIds[i]]);
    ConfigureOnScreen();
    DrawIcons();
    PrintItemNames();
    CommitWindow(WIN_ITEMS);
}

/* ---- スプライトの動き ---- */

static void PanelCallback(struct Sprite *sprite)
{
    if (sprite->data[0] >= sState->numOnScreen && sprite->data[0] != 0xFF)
        sprite->invisible = TRUE;
    else
        sprite->invisible = FALSE;
    StartSpriteAnimIfDifferent(sprite, sprite->data[0] == CursorSlot() ? 1 : 0);
}

static void IconCallback(struct Sprite *sprite)
{
    if (sprite->data[0] == CursorSlot())
    {
        StartSpriteAnim(sprite, 1);
        if (sprite->data[4] % 5 == 0)
        {
            if (sprite->data[1] == 0)
                sprite->data[3] = 1;
            else if (sprite->data[1] == 3)
                sprite->data[3] = -1;
            sprite->data[1] += sprite->data[3];
            sprite->pos1.y = sprite->data[2] + sprite->data[1];
        }
        sprite->data[4]++;
    }
    else
    {
        sprite->pos1.y = sprite->data[2];
        StartSpriteAnim(sprite, 0);
    }
}

/* ---- 画面の流れ ---- */

static void CB2_Pokenav(void)
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
        sState->bgTilemapBuffer = Malloc(0x1000);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
        SetBgTilemapBuffer(BG_BACKGROUND, sState->bgTilemapBuffer);
        gMain.state++;
        break;
    case 3:
        BuildItems();
        LoadBgGfx();
        gMain.state++;
        break;
    case 4:
        if (!Pnav_FreeTempTileDataBuffersIfPossible())
        {
            ShowBg(BG_TEXT);
            ShowBg(BG_BACKGROUND);
            CopyBgTilemapBufferToVram(BG_BACKGROUND);
            gMain.state++;
        }
        break;
    case 5:
        Free(sState->bgTilemapBuffer);
        sState->bgTilemapBuffer = NULL;
        InitWindows(sWindowTemplates);
        DeactivateAllTextPrinters();
        gMain.state++;
        break;
    case 6:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    case 7:
        SetVBlankCallback(VBlankCB_Pokenav);
        InitGui();
        CreateTask(Task_FadeIn, 0);
        SetMainCallback2(MainCB2_Pokenav);
        gMain.state = 0;
        break;
    }
}

static void Task_FadeIn(u8 taskId)
{
    if (!gPaletteFade->active)
        gTasks[taskId].func = Task_WaitForKeyPress;
}

static void MoveCursor(u8 newCursor)
{
    u8 oldScroll = sState->scroll;
    u8 row = newCursor / 2;

    PlaySE(SE_SELECT);
    sState->cursor = newCursor;
    if (row < sState->scroll)
        sState->scroll = row;
    else if (row > sState->scroll + 2)
        sState->scroll = row - 2;
    if (sState->scroll != oldScroll)
        RefreshOptions();
    PrintDescription();
}

static void Task_WaitForKeyPress(u8 taskId)
{
    u8 cursor = sState->cursor;

    if (JOY_NEW(B_BUTTON | START_BUTTON))
    {
        sState->closing = 1;
        PlaySE(SE_PC_OFF);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_FadeOut;
    }
    else if (JOY_NEW(A_BUTTON) && sState->numItems != 0)
    {
        sState->closing = 2;
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_FadeOut;
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (cursor + 1 < sState->numItems && cursor % 2 == 0)
            MoveCursor(cursor + 1);
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        if (cursor % 2 == 1)
            MoveCursor(cursor - 1);
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (cursor >= 2)
            MoveCursor(cursor - 2);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (cursor + 2 < sState->numItems)
            MoveCursor(cursor + 2);
    }
}

static void FreeAndClose(u8 taskId)
{
    Free(sState);
    sState = NULL;
    FreeAllWindowBuffers();
    BGMVolumeMax_EnableHelpSystemReduction();
    DestroyTask(taskId);
}

static void Task_FadeOut(u8 taskId)
{
    if (!gPaletteFade->active)
    {
        void (*open)(void) = NULL;

        if (sState->closing == 2)
        {
            VarSet(PNAV_CURSOR_VAR, sState->cursor);
            open = sFeatures[sState->items[sState->cursor]].open;
        }
        FreeAndClose(taskId);       /* ★ 先に自分を片づける（スタートメニューや子の画面が同じ語を使う） */
        if (open != NULL)
            open();
        else
            SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);   /* スタートメニューへ戻る（Var8000／8001 のカーソルで開き直す） */
    }
}

/* ---- 入口 ---- */

/* スタートメニューの「ポケナビ」から（最初から） */
void CB2_HackPokenav(void)
{
    gMain.savedCallback = NULL;
    sState = Calloc(sizeof(struct PnavState));
    SetMainCallback2(CB2_Pokenav);
}

/* 子の画面（ちず・でんわ…）から戻る。カーソルを Var8002 から戻して開き直す */
void CB2_HackPokenavReturn(void)
{
    gMain.savedCallback = NULL;
    sState = Calloc(sizeof(struct PnavState));
    sState->cursor = VarGet(PNAV_CURSOR_VAR);
    SetMainCallback2(CB2_Pokenav);
}
