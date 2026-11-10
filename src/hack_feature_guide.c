#include "defines.h"
#include "../include/bg.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/constants/songs.h"
#include "../include/new/dns.h"

void __attribute__((long_call)) ScanlineEffect_Stop(void);

extern const u8 gText_FeatureGuide_Title[];
extern const u8 gText_FeatureGuide_Help[];
extern const u8 gText_FeatureGuide_PhoneTitle[], gText_FeatureGuide_PhoneBody[];
extern const u8 gText_FeatureGuide_AchievementsTitle[], gText_FeatureGuide_AchievementsBody[];
extern const u8 gText_FeatureGuide_DexNavTitle[], gText_FeatureGuide_DexNavBody[];
extern const u8 gText_FeatureGuide_FollowerTitle[], gText_FeatureGuide_FollowerBody[];
extern const u8 gText_FeatureGuide_SettingsTitle[], gText_FeatureGuide_SettingsBody[];
extern const u8 gText_FeatureGuide_ChallengeTitle[], gText_FeatureGuide_ChallengeBody[];
extern const u8 gText_FeatureGuide_SavingsTitle[], gText_FeatureGuide_SavingsBody[];
extern const u8 gText_FeatureGuide_BoxLinkTitle[], gText_FeatureGuide_BoxLinkBody[];
extern const u8 gText_FeatureGuide_FlightTitle[], gText_FeatureGuide_FlightBody[];
extern const u8 gText_FeatureGuide_FreeFlightTitle[], gText_FeatureGuide_FreeFlightBody[];
extern const u8 gText_FeatureGuide_BallsTitle[], gText_FeatureGuide_BallsBody[];
#define FEATURE_GUIDE_EXTRA_TEXT(name) \
    extern const u8 gText_FeatureGuide_##name##Title[], gText_FeatureGuide_##name##Body[]
FEATURE_GUIDE_EXTRA_TEXT(FieldSpeed);
FEATURE_GUIDE_EXTRA_TEXT(Difficulty);
FEATURE_GUIDE_EXTRA_TEXT(BattleSpeed);
FEATURE_GUIDE_EXTRA_TEXT(Moemon);
FEATURE_GUIDE_EXTRA_TEXT(Habitat);
FEATURE_GUIDE_EXTRA_TEXT(WonderSettings);
FEATURE_GUIDE_EXTRA_TEXT(NetButton);
FEATURE_GUIDE_EXTRA_TEXT(Weather);
FEATURE_GUIDE_EXTRA_TEXT(Nickname);
FEATURE_GUIDE_EXTRA_TEXT(Affection);
FEATURE_GUIDE_EXTRA_TEXT(TrainingPouch);
FEATURE_GUIDE_EXTRA_TEXT(TrainingTools);
FEATURE_GUIDE_EXTRA_TEXT(TrainingHelpers);
FEATURE_GUIDE_EXTRA_TEXT(SaveSlots);
FEATURE_GUIDE_EXTRA_TEXT(Boxes);
FEATURE_GUIDE_EXTRA_TEXT(NetworkTrade);
FEATURE_GUIDE_EXTRA_TEXT(Gifts);
FEATURE_GUIDE_EXTRA_TEXT(WonderTrade);
FEATURE_GUIDE_EXTRA_TEXT(WildChain);
FEATURE_GUIDE_EXTRA_TEXT(LegendRaid);
FEATURE_GUIDE_EXTRA_TEXT(SpecialEncounters);
FEATURE_GUIDE_EXTRA_TEXT(StoryPartner);
FEATURE_GUIDE_EXTRA_TEXT(SpecialItems);
FEATURE_GUIDE_EXTRA_TEXT(HiddenGifts);
FEATURE_GUIDE_EXTRA_TEXT(HiddenTrainers);
FEATURE_GUIDE_EXTRA_TEXT(RegionalTools);
FEATURE_GUIDE_EXTRA_TEXT(GameCorner);
FEATURE_GUIDE_EXTRA_TEXT(Dynamax);
FEATURE_GUIDE_EXTRA_TEXT(StartSupport);
FEATURE_GUIDE_EXTRA_TEXT(MomPikachu);
FEATURE_GUIDE_EXTRA_TEXT(Momoka);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeItems);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeFees);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeIntruder);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeA);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeB1);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeB2);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeC1);
FEATURE_GUIDE_EXTRA_TEXT(ChallengeC2);
#undef FEATURE_GUIDE_EXTRA_TEXT

struct FeatureGuidePage
{
    const u8 *title;
    const u8 *body;
};

static const struct FeatureGuidePage sPages[] =
{
    {gText_FeatureGuide_PhoneTitle, gText_FeatureGuide_PhoneBody},
    {gText_FeatureGuide_AchievementsTitle, gText_FeatureGuide_AchievementsBody},
    {gText_FeatureGuide_DexNavTitle, gText_FeatureGuide_DexNavBody},
    {gText_FeatureGuide_FollowerTitle, gText_FeatureGuide_FollowerBody},
    {gText_FeatureGuide_SettingsTitle, gText_FeatureGuide_SettingsBody},
    {gText_FeatureGuide_ChallengeTitle, gText_FeatureGuide_ChallengeBody},
    {gText_FeatureGuide_SavingsTitle, gText_FeatureGuide_SavingsBody},
    {gText_FeatureGuide_BoxLinkTitle, gText_FeatureGuide_BoxLinkBody},
    {gText_FeatureGuide_FlightTitle, gText_FeatureGuide_FlightBody},
    {gText_FeatureGuide_FreeFlightTitle, gText_FeatureGuide_FreeFlightBody},
    {gText_FeatureGuide_BallsTitle, gText_FeatureGuide_BallsBody},
#define FEATURE_GUIDE_EXTRA_PAGE(name) {gText_FeatureGuide_##name##Title, gText_FeatureGuide_##name##Body}
    FEATURE_GUIDE_EXTRA_PAGE(FieldSpeed),
    FEATURE_GUIDE_EXTRA_PAGE(Difficulty),
    FEATURE_GUIDE_EXTRA_PAGE(BattleSpeed),
    FEATURE_GUIDE_EXTRA_PAGE(Moemon),
    FEATURE_GUIDE_EXTRA_PAGE(Habitat),
    FEATURE_GUIDE_EXTRA_PAGE(WonderSettings),
    FEATURE_GUIDE_EXTRA_PAGE(NetButton),
    FEATURE_GUIDE_EXTRA_PAGE(Weather),
    FEATURE_GUIDE_EXTRA_PAGE(Nickname),
    FEATURE_GUIDE_EXTRA_PAGE(Affection),
    FEATURE_GUIDE_EXTRA_PAGE(TrainingPouch),
    FEATURE_GUIDE_EXTRA_PAGE(TrainingTools),
    FEATURE_GUIDE_EXTRA_PAGE(TrainingHelpers),
    FEATURE_GUIDE_EXTRA_PAGE(SaveSlots),
    FEATURE_GUIDE_EXTRA_PAGE(Boxes),
    FEATURE_GUIDE_EXTRA_PAGE(NetworkTrade),
    FEATURE_GUIDE_EXTRA_PAGE(Gifts),
    FEATURE_GUIDE_EXTRA_PAGE(WonderTrade),
    FEATURE_GUIDE_EXTRA_PAGE(WildChain),
    FEATURE_GUIDE_EXTRA_PAGE(LegendRaid),
    FEATURE_GUIDE_EXTRA_PAGE(SpecialEncounters),
    FEATURE_GUIDE_EXTRA_PAGE(StoryPartner),
    FEATURE_GUIDE_EXTRA_PAGE(SpecialItems),
    FEATURE_GUIDE_EXTRA_PAGE(HiddenGifts),
    FEATURE_GUIDE_EXTRA_PAGE(HiddenTrainers),
    FEATURE_GUIDE_EXTRA_PAGE(RegionalTools),
    FEATURE_GUIDE_EXTRA_PAGE(GameCorner),
    FEATURE_GUIDE_EXTRA_PAGE(Dynamax),
    FEATURE_GUIDE_EXTRA_PAGE(StartSupport),
    FEATURE_GUIDE_EXTRA_PAGE(MomPikachu),
    FEATURE_GUIDE_EXTRA_PAGE(Momoka),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeItems),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeFees),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeIntruder),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeA),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeB1),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeB2),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeC1),
    FEATURE_GUIDE_EXTRA_PAGE(ChallengeC2),
#undef FEATURE_GUIDE_EXTRA_PAGE
};

enum { TD_PAGE, TD_EXITING };

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
    {.bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = 30, .height = 20, .paletteNum = 15, .baseBlock = 1},
    DUMMY_WIN_TEMPLATE,
};

static const u16 sPalette[16] = {0, 0x7FFF, 0x1084, 0x7FE0, 0x03FF, 0x0842, 0x18C6, 0x3180, 0x6739, 0x0421};

static void Print(const u8 *text, u8 x, u8 y, u8 color, u8 bg)
{
    u8 colors[3] = {bg, color, 2};
    AddTextPrinterParameterized3(0, 2, x, y, colors, 0, text);
}

static void Rect(u8 color, u16 x, u16 y, u16 width, u16 height)
{
    FillWindowPixelRect(0, color, x, y, width, height);
}

static void DrawPage(u8 page)
{
    u8 number[6];

    FillWindowPixelBuffer(0, 0x55);
    Rect(6, 0, 0, 240, 24);
    Rect(3, 0, 23, 240, 1);
    Rect(6, 0, 138, 240, 22);
    Rect(8, 0, 138, 240, 1);
    Print(gText_FeatureGuide_Title, 8, 5, 1, 6);
    Print(sPages[page].title, 12, 34, 4, 5);
    Print(sPages[page].body, 12, 55, 1, 5);
    ConvertIntToDecimalStringN(number, page + 1, STR_CONV_MODE_RIGHT_ALIGN, 2);
    Print(number, 205, 5, 8, 6);
    Print(gText_FeatureGuide_Help, 8, 143, 1, 6);
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_BOTH);
}

static void MainCB2_FeatureGuide(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_FeatureGuide(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void Task_FeatureGuide(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u16 keys;

    if (gPaletteFade->active)
        return;
    if (data[TD_EXITING])
    {
        SetVBlankCallback(NULL);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        ResetSpriteData();
        FreeAllSpritePalettes();
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
        return;
    }
    keys = gMain.newKeys;
    if (keys & B_BUTTON)
    {
        data[TD_EXITING] = TRUE;
        PlaySE(SE_PC_OFF);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    }
    else if (keys & (DPAD_UP | DPAD_LEFT))
    {
        data[TD_PAGE] = (data[TD_PAGE] != 0) ? data[TD_PAGE] - 1 : NELEMS(sPages) - 1;
        PlaySE(SE_SELECT);
        DrawPage(data[TD_PAGE]);
    }
    else if (keys & (DPAD_DOWN | DPAD_RIGHT))
    {
        data[TD_PAGE] = (data[TD_PAGE] + 1 < (s16)NELEMS(sPages)) ? data[TD_PAGE] + 1 : 0;
        PlaySE(SE_SELECT);
        DrawPage(data[TD_PAGE]);
    }
}

void CB2_HackFeatureGuide(void)
{
    u8 taskId;

    SetVBlankCallback(NULL);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadPalette(sPalette, 15 * 16, sizeof(sPalette));
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    ShowBg(0);
    taskId = CreateTask(Task_FeatureGuide, 0);
    DrawPage(gTasks[taskId].data[TD_PAGE]);
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
    SetVBlankCallback(VBlankCB_FeatureGuide);
    SetMainCallback2(MainCB2_FeatureGuide);
}
