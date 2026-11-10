#include "defines.h"
#include "defines_battle.h"
#include "hack_delta.h" //[hack] patches/0962
#include "../include/battle.h"
#include "../include/battle_string_ids.h"
#include "../include/battle_message.h"
#include "../include/battle_script_commands.h"
#include "../include/bg.h"
#include "../include/evolution_scene.h"
#include "../include/gpu_regs.h"
#include "../include/m4a.h"
#include "../include/malloc.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/pokemon.h"
#include "../include/pokemon_summary_screen.h"
#include "../include/scanline_effect.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/window.h"
#include "../include/gba/macro.h"
#include "../include/constants/pokemon.h"
#include "../include/constants/songs.h"

#include "../include/new/battle_util.h"
#include "../include/new/dynamax.h"
#include "../include/new/terastal.h"
#include "../include/new/frontier.h"
#include "../include/new/evolution.h"
#include "../include/new/item.h"
#include "../include/new/learn_move.h"
#include "../include/new/Vanilla_functions.h"
#include "../include/new/Vanilla_functions_battle.h"
#include "hack_chf_rotgacha.h" //[hack] 縛り F わざガチャ（0.63.1: 進化のあとも ガチャの技のまま）
#include "shadow_battle.h" //[hack] patches/0963 段 E

/*
 * 戦闘中の進化（Redmine #91 段階 1・patches/0860・FEAT-midbattle-evo-s1）
 *
 * 元: Shiny-Miner/CFRU-expansion の src/mid_battle_evo.c（Andre Freitas・Shiny Miner・grilokapu。
 *     commit 8f25edc6 〜 69437cc1）。LICENSE ファイルは無く README は「作者を書けば使ってよい」の趣旨。CREDITS.md に出典を書いた。
 *     ここは日本語版（BPRJ）の番地に直し、この改造の事情に合わせた写し（差の一覧は docs/research/2026-10-26_mid-battle-evo-stage1.md）。
 *
 * 動き: レベルが上がった子（gLeveledUpInBattle）を、ターンの終わり（BattleTurnPassed の末尾が書く gBattleMainFunc）で
 *       自前の進化の場面に送り、終わったら ReshowBattleScreenAfterMenu で戦闘の画面を作り直して続ける。
 *       フックは routinepointers の「PlayerTryEvolution 08013564」（素の BattleTurnPassed が最後に gBattleMainFunc へ入れる語）。
 *
 * この改造での差（expansion の写しからの）:
 *   (1) 素の日本語版の StringCopy10 は 5 文字で止まる → CFRU-JP の bytereplacement が進化の場面で使う 6 文字版（0x0804320C）を呼ぶ
 *   (2) 持ち物で進化する種族の持ち物の消費（ItemEvolutionRemoval）と旗の後始末を足した
 *   (3) 絵は DPE の HackMoemon_EvoFrontSheet（萌えもん・メス姿）を橋（gHackEvoFrontBridge）経由で呼ぶ。メス姿の自前の表は持たない
 *   (4) 進化の場面を許すのは「野生・通常のトレーナー戦・シングル」だけ（許可リスト）。通信・施設・ダブル・パートナー・レイドなどは戦闘後に回す
 *   (5) 進化後も登場時の特性は出し直さない（STATUS3_SWITCH_IN_ABILITY_DONE を下ろさない・ユーザー決定 2026-10-25 質問 5）
 */

#ifndef BG_PLTT_ID
#define BG_PLTT_ID(n) ((n) * 16)
#endif
#ifndef OBJ_PLTT_ID
#define OBJ_PLTT_ID(n) (256 + (n) * 16)
#endif
#ifndef PLTT_SIZE_4BPP
#define PLTT_SIZE_4BPP 0x20
#endif

/* ---- 素の日本語版の番地（docs: work/tmp/midbattle_evo_design/stage0.md 1 章・検算 tools/rom/verify_mid_battle_evo.py） ---- */
#define MBE_FN(ret, addr, args) ((ret (*) args) ((addr) | 1))
#define MbeLoadBattleTextboxAndBackground MBE_FN(void, 0x0800EDD4, (void))   /* battle.h の宣言は long_call でなく、素の関数を bl すると ARM 用の veneer（blx＝GBA に無い命令）になり未定義命令で暴走した */
#define MbeInitBattleBgsVideo            MBE_FN(void, 0x0800ED00, (void))
#define MbeReshowBattleScreenAfterMenu   ((void (*)(void)) (0x08076EFC | 1))
#define MbeStringCopyNickname            MBE_FN(u8*, 0x0804320C, (u8*, const u8*))  /* 6 文字版（素の StringCopy10 08008870 は 5 文字） */
#define MbeLoadEvoSparkleSpriteAndPal    MBE_FN(void, 0x080F6DDC, (void))
#define MbeVBlankCB_EvolutionScene       ((void (*)(void)) (0x080D116C | 1))
#define MbeM4aMPlayAllStop               MBE_FN(void, 0x081C11E4, (void))
#define MbeHelpSystemDisable             MBE_FN(void, 0x0812BC7C, (void))
#define MbeHelpSystemEnable              MBE_FN(void, 0x0812BC88, (void))
#define MbeStopBgAnimation               MBE_FN(void, 0x080D15C4, (void))
#define MbeStartBgAnimation              MBE_FN(void, 0x080D1490, (bool8))
#define MbeSparkles_SpiralUpward         MBE_FN(u8, 0x080F6DF8, (u16))
#define MbeSparkles_ArcDown              MBE_FN(u8, 0x080F6F18, (void))
#define MbeSparkles_CircleInward         MBE_FN(u8, 0x080F6FCC, (void))
#define MbeSparkles_SprayAndFlash        MBE_FN(u8, 0x080F70A8, (u16))
#define MbeCycleEvolutionMonSprite       MBE_FN(u8, 0x080F730C, (u8, u8))
#define MbeRestoreBgAfterAnim            MBE_FN(void, 0x080D1604, (void))
#define MbeEvolutionRenameMon            MBE_FN(void, 0x08042C5C, (struct Pokemon*, u16, u16))  /* DPE の bytereplacement で種族名の表を ×8 に直してある */
#define MbePlayBattleBGM                 MBE_FN(void, 0x080436BC, (void))
#define MbeBattleCreateYesNoCursor       MBE_FN(void, 0x0802D4B4, (void))  /* BPRJ.ld の BattleCreateYesNoCursorAt（gBattleCommunication[1] を読む版） */
#define MbeBattleDestroyYesNoCursor      MBE_FN(void, 0x0802D4F4, (void))
#define MbeBufferMoveToLearnIntoBattleTextBuff2 MBE_FN(void, 0x08026054, (void))
#define MbeDecompressPicFromTable        MBE_FN(void, 0x0800E640, (const struct CompressedSpriteSheet*, void*, s32))
#define MbeSetMultiuseSpriteTemplateToPokemon MBE_FN(void, 0x0803EF40, (u16, u8))
/* 2026-10-26 段 1b: 0x08134F1C は hooks の先頭（BwShowSelectMovePokemonSummaryScreen 08134F1C 4）で r4 を壊す（ldr r4／bx r4）。
   C の呼び元が r4 に taskId を持つと戻りで壊れ、わざ忘れの画面から戻ると暴走（ソフトリセット）した。番地ではなく C の関数を直に呼ぶ（src/bw_summary_screen.c） */
void BwShowSelectMovePokemonSummaryScreen(struct Pokemon* party, u8 cursorPos, u8 lastIdx, MainCallback savedCallback, u16 newMove);
#define MbeShowSelectMovePokemonSummaryScreen BwShowSelectMovePokemonSummaryScreen
#define MbeIsCryFinished                 MBE_FN(bool8, 0x08071944, (void))
#define MbeStopMapMusic                  MBE_FN(void, 0x08071238, (void))
#define sEvoStructPtr                    (*((struct MbeEvoInfo**) 0x02039998))
#define MbeReservedSpritePaletteCount    (*((u8*) 0x03003E98))

/* 文字列（素の進化の場面の同じ文・Task_EvolutionScene 080CF9F8 が読む番地） */
#define MBE_STR_INTRO      ((u8*) 0x083C463E)   /* ……おや！？ {VAR02}の ようすが……！ */
#define MBE_STR_EVOLVED    ((u8*) 0x083C4651)   /* おめでとう！ {VAR02}は {VAR03}に しんかした！ */
#define MBE_STR_STOPPED    ((u8*) 0x083C466A)   /* あれ……？ {VAR02}の へんかが とまった！ */
#define MBE_STR_ELLIPSIS   ((u8*) 0x083C4680)   /* ……？ */
#define MBE_STR_YESNO      ((u8*) 0x083C4720)   /* はい／いいえ */

/* 絵の橋: DPE の HackMoemon_EvoFrontSheet(species, personality) の番地（tools/build/build_rom.py の patch_evo_front_bridge が DPE を組んだあとに書く・0 なら表を直に引く） */
extern const u32 gHackEvoFrontBridge[1];
#define gMonFrontPicTable ((const struct CompressedSpriteSheet*) *((u32*) 0x8000128))

#define B_WIN_MSG                 0
#define B_WIN_YESNO              14
#define MBE_MUS_LEVEL_UP          257
#define MBE_MUS_EVOLVED           259
#define MBE_MUS_EVOLUTION_INTRO   263
#define MBE_MUS_EVOLUTION         264
#define MBE_BATTLESTRINGS_START   STRINGID_TRAINER1LOSETEXT

#define sEvoCursorPos           gBattleCommunication[1]
#define sEvoGraphicsTaskId      gBattleCommunication[2]
#define tEvoStopped             data[8]
#define TASK_BIT_CAN_STOP       (1 << 0)
#define TASK_BIT_LEARN_MOVE     (1 << 7)

enum {
    EVOSTATE_FADE_IN,
    EVOSTATE_INTRO_MSG,
    EVOSTATE_INTRO_MON_ANIM,
    EVOSTATE_INTRO_SOUND,
    EVOSTATE_START_MUSIC,
    EVOSTATE_START_BG_AND_SPARKLE_SPIRAL,
    EVOSTATE_SPARKLE_ARC,
    EVOSTATE_CYCLE_MON_SPRITE,
    EVOSTATE_WAIT_CYCLE_MON_SPRITE,
    EVOSTATE_SPARKLE_CIRCLE,
    EVOSTATE_SPARKLE_SPRAY,
    EVOSTATE_EVO_SOUND,
    EVOSTATE_RESTORE_SCREEN,
    EVOSTATE_EVO_MON_ANIM,
    EVOSTATE_SET_MON_EVOLVED,
    EVOSTATE_TRY_LEARN_MOVE,
    EVOSTATE_END,
    EVOSTATE_CANCEL,
    EVOSTATE_CANCEL_MON_ANIM,
    EVOSTATE_CANCEL_MSG,
    EVOSTATE_LEARNED_MOVE,
    EVOSTATE_TRY_LEARN_ANOTHER_MOVE,
    EVOSTATE_REPLACE_MOVE,
};

enum {
    MVSTATE_INTRO_MSG_1,
    MVSTATE_INTRO_MSG_2,
    MVSTATE_INTRO_MSG_3,
    MVSTATE_PRINT_YES_NO,
    MVSTATE_HANDLE_YES_NO,
    MVSTATE_SHOW_MOVE_SELECT,
    MVSTATE_HANDLE_MOVE_SELECT,
    MVSTATE_FORGET_MSG_1,
    MVSTATE_FORGET_MSG_2,
    MVSTATE_LEARNED_MOVE,
    MVSTATE_ASK_CANCEL,
    MVSTATE_CANCEL,
    MVSTATE_RETRY_AFTER_HM,
};

struct MbeEvoInfo
{
    u8 preEvoSpriteId;
    u8 postEvoSpriteId;
    u8 evoTaskId;
    u8 delayTimer;
    u16 savedPalette[48];
};

void PlayerTryEvolution(void);
void HandleTurnActionSelectionState(void);   /* BPRJ.ld 0x08013860 */
void CreateShedinja(u16 preEvoSpecies, struct Pokemon* mon);   /* src/evolution.c */
static void WaitForEvolutionThenTryAnother(void);
static void CB2_SetUpReshowBattleScreenAfterEvolution(void);
static void Mbe_Task_EvolutionScene(u8 taskId);
static void UpdateEvolvedBattleMon(struct Pokemon *mon);
static void UpdateEvolvedBattleMoves(struct Pokemon *mon);
static void CB2_MidBattleEvolutionLoadGraphics(void);
static void Mbe_EvolutionScene(struct Pokemon* mon, u16 postEvoSpecies, bool8 canStopEvo, u8 partyId);

#define tState              data[0]
#define tPreEvoSpecies      data[1]
#define tPostEvoSpecies     data[2]
#define tCanStop            data[3]
#define tBits               data[3]
#define tLearnsFirstMove    data[4]
#define tLearnMoveState     data[6]
#define tLearnMoveYesState  data[7]
#define tLearnMoveNoState   data[8]
#define tEvoWasStopped      data[9]
#define tPartyId            data[10]

static void MbeClearEvoItemFlag(void)
{
#ifdef EVO_HOLD_ITEM_REMOVAL
    FlagClear(FLAG_REMOVE_EVO_ITEM);   /* GetEvolutionTargetSpecies が立てた旗。進化しなかったときに残すと次の進化の持ち物を誤って消す */
#endif
}

/* 戦闘中の進化の場面を出してよい戦闘か（許可リスト・段階 1）。
   野生・通常のトレーナー戦・シングルだけ。通信・安全地帯・ポケモンじいさん・フロンティア・タワー・e-Reader・BENJAMIN・
   ダブル・パートナー・レイド・縛りの追加施設（FLAG_BATTLE_FACILITY＝ホウエンのアリーナ・パレス・ファクトリー・ロジックアリーナほか）は全部外す。 */
static bool8 MbeBattleAllowsMidEvolution(void)
{
    const u32 allowed = BATTLE_TYPE_TRAINER | BATTLE_TYPE_IS_MASTER;

    if (gBattleTypeFlags & ~allowed)
        return FALSE;
    if (FlagGet(FLAG_BATTLE_FACILITY))
        return FALSE;
    return TRUE;
}

static void EvoDummyFunc(void)
{
}

static void CB2_SetUpReshowBattleScreenAfterEvolution(void)
{
	/* BUG-battle-evo-expshare-hang（Redmine #204）: 進化の場面の BG アニメのタスク（0x080D127D・0x080D1379）が場面のあとも残り、毎フレーム gBattle_BG1/BG2 の X/Y を書き換える。
	   学習装置の控えのレベルアップ箱（drawlvlupbox 状態 2・0x08025CB8）は BG2_X を +8 して 0x200 ちょうどで終わるので、永遠に終わらず進行不能になった。
	   場面の入口（Mbe_EvolutionScene）で ResetTasks 済みなので、ここで生きているタスクは進化の場面のものだけ＝全部消してよい */
	ResetTasks();
    gBattleTerrain = gNewBS->midBattleEvolution.savedTerrain;
    memcpy(gBattleCommunication, gNewBS->midBattleEvolution.savedCommunication, sizeof(gBattleCommunication));
    gNewBS->midBattleEvolution.active = FALSE;
    SetMainCallback2(MbeReshowBattleScreenAfterMenu);
}

#define tSpeciesToEvolveInto data[0]
#define tPartyToEvolve       data[1]

static void Task_BeginBattleEvolutionScene(u8 taskId)
{
    if (!gPaletteFade->active)
    {
        u8 partyId = gTasks[taskId].tPartyToEvolve;
        u16 species = gTasks[taskId].tSpeciesToEvolveInto;

        gCB2_AfterEvolution = CB2_SetUpReshowBattleScreenAfterEvolution;
        DestroyTask(taskId);
        Mbe_EvolutionScene(&gPlayerParty[partyId], species, TRUE, partyId);
    }
}

void PlayerTryEvolution(void)
{
    u8 position;

    if (gNewBS == NULL || !gMain.inBattle || gBattleOutcome != 0
     || !MbeBattleAllowsMidEvolution())
    {
        gBattleMainFunc = HandleTurnActionSelectionState;
        return;
    }

    /* 台本・画面・パレットが動いているあいだは場面を作り直さない */
    if (gBattleExecBuffer || gPaletteFade->active)
        return;

    for (position = B_POSITION_PLAYER_LEFT; position <= B_POSITION_PLAYER_RIGHT; position += BIT_FLANK)
    {
        u8 bank, partyId, taskId;
        u16 species;
        struct Pokemon *mon;

        if (position == B_POSITION_PLAYER_RIGHT && !IS_DOUBLE_BATTLE)
            break;
        bank = GetBattlerAtPosition(position);
        if (bank >= gBattlersCount || !IsBattlerAlive(bank))
            continue;
        partyId = gBattlerPartyIndexes[bank];
        if (partyId >= PARTY_SIZE
         || (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && partyId >= PARTY_SIZE / 2)
         || !(gLeveledUpInBattle & gBitTable[partyId])
         || (gNewBS->midBattleEvolution.cancelledParty & gBitTable[partyId]))
            continue;

        mon = &gPlayerParty[partyId];
        /* 戦闘中だけの姿は後回し（ビットは残して戦闘後に回す） */
        if (IS_TRANSFORMED(bank) || IsDynamaxed(bank) || IsTerastal(bank)
         || SPECIES(bank) != mon->species || gMonSpritesGfxPtr == NULL)
            continue;
        species = GetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, ITEM_NONE);
        if (species == SPECIES_NONE || species >= NUM_SPECIES)
        {
            MbeClearEvoItemFlag();
            continue;
        }
        taskId = CreateTask(Task_BeginBattleEvolutionScene, 0);
        if (taskId >= NUM_TASKS)
        {
            MbeClearEvoItemFlag();
            return;
        }

        gLeveledUpInBattle &= ~gBitTable[partyId];
        gNewBS->midBattleEvolution.battler = bank;
        gNewBS->midBattleEvolution.savedTerrain = gBattleTerrain;
        memcpy(gNewBS->midBattleEvolution.savedCommunication, gBattleCommunication, sizeof(gBattleCommunication));
        memcpy(gNewBS->midBattleEvolution.originalMoves, mon->moves, sizeof(mon->moves));
        gNewBS->midBattleEvolution.active = TRUE;
        /* 再計算は今の HP から始める（戦闘のダメージを全回復させない） */
        SetMonData(mon, MON_DATA_HP, &gBattleMons[bank].hp);
        SetMonData(mon, MON_DATA_STATUS, &gBattleMons[bank].status1);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        gBattleMainFunc = WaitForEvolutionThenTryAnother;
        gTasks[taskId].tSpeciesToEvolveInto = species;
        gTasks[taskId].tPartyToEvolve = partyId;
        return;
    }

    gBattleMainFunc = HandleTurnActionSelectionState;
}

static void WaitForEvolutionThenTryAnother(void)
{
    if (!gNewBS->midBattleEvolution.active
     && gMain.callback2 == BattleMainCB2 && !gPaletteFade->active)
        gBattleMainFunc = PlayerTryEvolution;
}

/* 絵の表の 1 件（8 バイト {絵, 大きさ, 札}）。橋があれば DPE の HackMoemon_EvoFrontSheet（萌えもん・メス姿）、無ければ表を直に引く */
static const struct CompressedSpriteSheet* MbeFrontSheet(u16 species, u32 personality)
{
    if (gHackEvoFrontBridge[0] != 0)
    {
        const struct CompressedSpriteSheet* (*fn)(u16, u32) = (const struct CompressedSpriteSheet* (*)(u16, u32)) gHackEvoFrontBridge[0];
        return fn(species, personality);
    }
    return &gMonFrontPicTable[species];
}

static void Mbe_EvolutionScene(struct Pokemon* mon, u16 postEvoSpecies, bool8 canStopEvo, u8 partyId)
{
    u8 name[20];
    u16 currSpecies;
    u32 trainerId, personality;
    const struct CompressedSpritePalette* pokePal;
    u8 id;

    sEvoStructPtr = AllocZeroed(sizeof(struct MbeEvoInfo));
    if (sEvoStructPtr == NULL)
    {
        /* 確保に失敗したら戦闘後に回す（無限に繰り返さない） */
        MbeClearEvoItemFlag();
        gLeveledUpInBattle |= gBitTable[partyId];
        gNewBS->midBattleEvolution.cancelledParty |= gBitTable[partyId];
        SetMainCallback2(gCB2_AfterEvolution);
        return;
    }
    FreeAllWindowBuffers();
    SetHBlankCallback(NULL);
    SetVBlankCallback(NULL);
    CpuFill32(0, (void *)(VRAM), VRAM_SIZE);

    SetGpuReg(REG_OFFSET_MOSAIC, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WIN1H, 0);
    SetGpuReg(REG_OFFSET_WIN1V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);

    ResetPaletteFade();

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    gBattle_BG2_X = 0;
    gBattle_BG2_Y = 0;
    gBattle_BG3_X = 256;
    gBattle_BG3_Y = 0;

    gBattleTerrain = BATTLE_TERRAIN_PLAIN;

    MbeInitBattleBgsVideo();
    MbeLoadBattleTextboxAndBackground();
    ResetSpriteData();
    ScanlineEffect_Stop();
    ResetTasks();
    FreeAllSpritePalettes();

    MbeReservedSpritePaletteCount = 4;

    GetMonData(mon, MON_DATA_NICKNAME, name);
    MbeStringCopyNickname(gStringVar1, name);
    StringCopy(gStringVar2, gSpeciesNames[postEvoSpecies]);

    /* 進化前の絵（素の進化の場面の DPE のフック 1 と同じ引き方） */
    currSpecies = GetMonData(mon, MON_DATA_SPECIES, NULL);
    trainerId = GetMonData(mon, MON_DATA_OT_ID, NULL);
    personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);

    MbeDecompressPicFromTable(MbeFrontSheet(currSpecies, personality),
                              gMonSpritesGfxPtr->sprites[B_POSITION_OPPONENT_LEFT],
                              currSpecies);
    pokePal = GetMonSpritePalStructFromOtIdPersonality(currSpecies, trainerId, personality);
    LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(1), PLTT_SIZE_4BPP);

    MbeSetMultiuseSpriteTemplateToPokemon(currSpecies, B_POSITION_OPPONENT_LEFT);
    gMultiuseSpriteTemplate->affineAnims = gDummySpriteAffineAnimTable;
    sEvoStructPtr->preEvoSpriteId = id = CreateSprite(gMultiuseSpriteTemplate, 120, 64, 30);

    gSprites[id].callback = SpriteCallbackDummy;
    gSprites[id].oam.paletteNum = 1;
    gSprites[id].invisible = TRUE;

    /* 進化後の絵（DPE のフック 2） */
    MbeDecompressPicFromTable(MbeFrontSheet(postEvoSpecies, personality),
                              gMonSpritesGfxPtr->sprites[B_POSITION_OPPONENT_RIGHT],
                              postEvoSpecies);
    pokePal = GetMonSpritePalStructFromOtIdPersonality(postEvoSpecies, trainerId, personality);
    LoadCompressedPalette(pokePal->data, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);

    MbeSetMultiuseSpriteTemplateToPokemon(postEvoSpecies, B_POSITION_OPPONENT_RIGHT);
    gMultiuseSpriteTemplate->affineAnims = gDummySpriteAffineAnimTable;
    sEvoStructPtr->postEvoSpriteId = id = CreateSprite(gMultiuseSpriteTemplate, 120, 64, 30);
    gSprites[id].callback = SpriteCallbackDummy;
    gSprites[id].oam.paletteNum = 2;
    gSprites[id].invisible = TRUE;

    MbeLoadEvoSparkleSpriteAndPal();

    sEvoStructPtr->evoTaskId = id = CreateTask(Mbe_Task_EvolutionScene, 0);
    gTasks[id].tState = 0;
    gTasks[id].tPreEvoSpecies = currSpecies;
    gTasks[id].tPostEvoSpecies = postEvoSpecies;
    gTasks[id].tCanStop = canStopEvo;
    gTasks[id].tLearnsFirstMove = TRUE;
    gTasks[id].tEvoWasStopped = FALSE;
    gTasks[id].tPartyId = partyId;

    memcpy(&sEvoStructPtr->savedPalette, &gPlttBufferUnfaded[BG_PLTT_ID(2)], sizeof(sEvoStructPtr->savedPalette));

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_1D_MAP);

    SetHBlankCallback(EvoDummyFunc);
    SetVBlankCallback(MbeVBlankCB_EvolutionScene);
    MbeM4aMPlayAllStop();
    MbeHelpSystemDisable();
    SetMainCallback2(CB2_EvolutionSceneUpdate);
}

static void Mbe_Task_EvolutionScene(u8 taskId)
{
    u32 var;
    struct Pokemon* mon = &gPlayerParty[gTasks[taskId].tPartyId];

    /* B ボタンで止める */
    if (gMain.heldKeys & B_BUTTON
        && gTasks[taskId].tState == EVOSTATE_WAIT_CYCLE_MON_SPRITE
        && gTasks[sEvoGraphicsTaskId].isActive
        && gTasks[taskId].tBits & TASK_BIT_CAN_STOP)
    {
        gTasks[taskId].tState = EVOSTATE_CANCEL;
        gNewBS->midBattleEvolution.cancelledParty |= gBitTable[gTasks[taskId].tPartyId];
        gLeveledUpInBattle |= gBitTable[gTasks[taskId].tPartyId];   /* 戦闘後の進化でもう一度聞く（開始時に下ろしたビットを戻す） */
        gTasks[sEvoGraphicsTaskId].tEvoStopped = TRUE;
        MbeStopBgAnimation();
        return;
    }

    switch (gTasks[taskId].tState)
    {
    case EVOSTATE_FADE_IN:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        gSprites[sEvoStructPtr->preEvoSpriteId].invisible = FALSE;
        gTasks[taskId].tState++;
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        break;
    case EVOSTATE_INTRO_MSG:
        if (!gPaletteFade->active)
        {
            StringExpandPlaceholders(gStringVar4, MBE_STR_INTRO);
            BattlePutTextOnWindow(gStringVar4, B_WIN_MSG);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_INTRO_MON_ANIM:
        if (!IsTextPrinterActive(0))
        {
            PlayCry1(gTasks[taskId].tPreEvoSpecies, 0);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_INTRO_SOUND:
        if (MbeIsCryFinished())
        {
            PlaySE(MBE_MUS_EVOLUTION_INTRO);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_START_MUSIC:
        if (!IsSEPlaying())
        {
            PlayNewMapMusic(MBE_MUS_EVOLUTION);
            gTasks[taskId].tState++;
            BeginNormalPaletteFade(0x1C, 4, 0, 0x10, RGB_BLACK);
        }
        break;
    case EVOSTATE_START_BG_AND_SPARKLE_SPIRAL:
        if (!gPaletteFade->active)
        {
            MbeStartBgAnimation(FALSE);
            sEvoGraphicsTaskId = MbeSparkles_SpiralUpward(17);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_SPARKLE_ARC:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            gTasks[taskId].tState++;
            sEvoStructPtr->delayTimer = 1;
            sEvoGraphicsTaskId = MbeSparkles_ArcDown();
        }
        break;
    case EVOSTATE_CYCLE_MON_SPRITE:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            sEvoGraphicsTaskId = MbeCycleEvolutionMonSprite(sEvoStructPtr->preEvoSpriteId, sEvoStructPtr->postEvoSpriteId);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_WAIT_CYCLE_MON_SPRITE:
        if (--sEvoStructPtr->delayTimer == 0)
        {
            sEvoStructPtr->delayTimer = 3;
            if (!gTasks[sEvoGraphicsTaskId].isActive)
                gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_SPARKLE_CIRCLE:
        sEvoGraphicsTaskId = MbeSparkles_CircleInward();
        gTasks[taskId].tState++;
        break;
    case EVOSTATE_SPARKLE_SPRAY:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            sEvoGraphicsTaskId = MbeSparkles_SprayAndFlash(gTasks[taskId].tPostEvoSpecies);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_EVO_SOUND:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            PlaySE(SE_EXP);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_RESTORE_SCREEN:
        if (IsSEPlaying())
        {
            MbeM4aMPlayAllStop();
            memcpy(&gPlttBufferUnfaded[BG_PLTT_ID(2)], sEvoStructPtr->savedPalette, sizeof(sEvoStructPtr->savedPalette));
            MbeRestoreBgAfterAnim();
            BeginNormalPaletteFade(0x1C, 0, 0x10, 0, RGB_BLACK);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_EVO_MON_ANIM:
        if (!gPaletteFade->active)
        {
            PlayCry1(gTasks[taskId].tPostEvoSpecies, 0);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_SET_MON_EVOLVED:
        if (MbeIsCryFinished())
        {
            StringExpandPlaceholders(gStringVar4, MBE_STR_EVOLVED);
            BattlePutTextOnWindow(gStringVar4, B_WIN_MSG);
            PlayBGM(MBE_MUS_EVOLVED);
            gTasks[taskId].tState++;
            SetMonData(mon, MON_DATA_SPECIES, (void *)(&gTasks[taskId].tPostEvoSpecies));
            CalculateMonStats(mon);
            MbeEvolutionRenameMon(mon, gTasks[taskId].tPreEvoSpecies, gTasks[taskId].tPostEvoSpecies);
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(gTasks[taskId].tPostEvoSpecies), FLAG_SET_SEEN);
            GetSetPokedexFlag(SpeciesToNationalPokedexNum(gTasks[taskId].tPostEvoSpecies), FLAG_SET_CAUGHT);
            IncrementGameStat(GAME_STAT_EVOLVED_POKEMON);
            {
                u16 itemBefore = mon->item;
                ItemEvolutionRemoval(mon);
                if (itemBefore != ITEM_NONE && mon->item == ITEM_NONE)
                {
                    u8 pid = gTasks[taskId].tPartyId;

                    gBattleMons[gNewBS->midBattleEvolution.battler].item = ITEM_NONE;   /* 戦闘側の持ち物も空に */
                    if (pid < PARTY_SIZE)
                        gNewBS->itemBackup[pid] = ITEM_NONE;   /* トレーナー戦の終わりの RestoreNonConsumableItems（end_battle.c）が戦闘前の持ち物を戻すので、消費した物は控えからも消す（段 1b・mGBA でオーバルストーンが戻ったのを確認） */
                }
            }   /* 持ち物で進化する種族の持ち物を消し、旗を下ろす（素の進化の場面の RemoveEvoItem と同じ・hooks:495） */
            UpdateEvolvedBattleMon(mon);
        }
        break;
    case EVOSTATE_TRY_LEARN_MOVE:
        if (!IsTextPrinterActive(0))
        {
            MbeHelpSystemEnable();
            var = gTasks[taskId].tEvoWasStopped ? MOVE_NONE
                : MonTryLearningNewMoveAfterEvolution(mon, gTasks[taskId].tLearnsFirstMove);
            if (var != MOVE_NONE && !gTasks[taskId].tEvoWasStopped)
            {
                u8 text[20];

                if (!(gTasks[taskId].tBits & TASK_BIT_LEARN_MOVE))
                {
                    MbeStopMapMusic();
                    MbePlayBattleBGM();
                }
                gTasks[taskId].tBits |= TASK_BIT_LEARN_MOVE;
                gTasks[taskId].tLearnsFirstMove = FALSE;
                gTasks[taskId].tLearnMoveState = MVSTATE_INTRO_MSG_1;
                GetMonData(mon, MON_DATA_NICKNAME, text);
                MbeStringCopyNickname(gBattleTextBuff1, text);

                if (var == MON_HAS_MAX_MOVES)
                    gTasks[taskId].tState = EVOSTATE_REPLACE_MOVE;
                else if (var == MON_ALREADY_KNOWS_MOVE)
                    break;
                else
                {
                    gTasks[taskId].tState = EVOSTATE_LEARNED_MOVE;
                }
            }
            else // 覚える技が無い／進化を止めた
            {
                BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                gTasks[taskId].tState++;
            }
        }
        break;
    case EVOSTATE_END:
        if (!gPaletteFade->active)
        {
            if (!(gTasks[taskId].tBits & TASK_BIT_LEARN_MOVE))
            {
                MbeStopMapMusic();
                if (gMain.inBattle && gBattleOutcome == 0)
                    MbePlayBattleBGM(); // 戦闘が続くなら戦闘の曲に戻す
                else
                    Overworld_PlaySpecialMapMusic();
            }
            if (!gTasks[taskId].tEvoWasStopped)
            {
                UpdateEvolvedBattleMoves(mon);
                CreateShedinja(gTasks[taskId].tPreEvoSpecies, mon);
            }

            DestroyTask(taskId);
            FREE_AND_SET_NULL(sEvoStructPtr);
            FreeAllWindowBuffers();
            SetMainCallback2(gCB2_AfterEvolution);
        }
        break;
    case EVOSTATE_CANCEL:
        if (!gTasks[sEvoGraphicsTaskId].isActive)
        {
            MbeClearEvoItemFlag();   /* 止めたので持ち物は減らさない */
            MbeM4aMPlayAllStop();
            BeginNormalPaletteFade(0x6001C, 0, 0x10, 0, RGB_WHITE);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_CANCEL_MON_ANIM:
        if (!gPaletteFade->active)
        {
            PlayCry1(gTasks[taskId].tPreEvoSpecies, 0);
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_CANCEL_MSG:
        if (MbeIsCryFinished())
        {
            if (gTasks[taskId].tEvoWasStopped)
                StringExpandPlaceholders(gStringVar4, MBE_STR_ELLIPSIS);
            else
                StringExpandPlaceholders(gStringVar4, MBE_STR_STOPPED);

            BattlePutTextOnWindow(gStringVar4, B_WIN_MSG);
            gTasks[taskId].tEvoWasStopped = TRUE;
            gTasks[taskId].tState = EVOSTATE_TRY_LEARN_MOVE;
        }
        break;
    case EVOSTATE_LEARNED_MOVE:
        if (!IsTextPrinterActive(0) && !IsSEPlaying())
        {
            MbeBufferMoveToLearnIntoBattleTextBuff2();
            PlayFanfare(MBE_MUS_LEVEL_UP);
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_PKMNLEARNEDMOVE - MBE_BATTLESTRINGS_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tLearnsFirstMove = 0x40; // 待ちの数え（使い回し）
            gTasks[taskId].tState++;
        }
        break;
    case EVOSTATE_TRY_LEARN_ANOTHER_MOVE:
        if (!IsTextPrinterActive(0) && !IsSEPlaying() && --gTasks[taskId].tLearnsFirstMove == 0)
            gTasks[taskId].tState = EVOSTATE_TRY_LEARN_MOVE;
        break;
    case EVOSTATE_REPLACE_MOVE:
        switch (gTasks[taskId].tLearnMoveState)
        {
        case MVSTATE_INTRO_MSG_1:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                MbeBufferMoveToLearnIntoBattleTextBuff2();
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE1 - MBE_BATTLESTRINGS_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_INTRO_MSG_2:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE2 - MBE_BATTLESTRINGS_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_INTRO_MSG_3:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_TRYTOLEARNMOVE3 - MBE_BATTLESTRINGS_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveYesState = MVSTATE_SHOW_MOVE_SELECT;
                gTasks[taskId].tLearnMoveNoState = MVSTATE_ASK_CANCEL;
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_PRINT_YES_NO:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                HandleBattleWindow(23, 8, 29, 13, 0);
                BattlePutTextOnWindow(MBE_STR_YESNO, B_WIN_YESNO);
                gTasks[taskId].tLearnMoveState++;
                sEvoCursorPos = 0;
                MbeBattleCreateYesNoCursor();
            }
            break;
        case MVSTATE_HANDLE_YES_NO:
            // 「忘れさせる？」と「覚えるのをやめる？」の両方で使う。次の状態は tLearnMoveYesState / tLearnMoveNoState
            if (JOY_NEW(DPAD_UP) && sEvoCursorPos != 0)
            {
                PlaySE(SE_SELECT);
                MbeBattleDestroyYesNoCursor();
                sEvoCursorPos = 0;
                MbeBattleCreateYesNoCursor();
            }
            if (JOY_NEW(DPAD_DOWN) && sEvoCursorPos == 0)
            {
                PlaySE(SE_SELECT);
                MbeBattleDestroyYesNoCursor();
                sEvoCursorPos = 1;
                MbeBattleCreateYesNoCursor();
            }
            if (JOY_NEW(A_BUTTON))
            {
                HandleBattleWindow(0x17, 8, 0x1D, 0xD, WINDOW_CLEAR);
                PlaySE(SE_SELECT);

                if (sEvoCursorPos != 0)
                {
                    gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveNoState;
                }
                else
                {
                    gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveYesState;
                    if (gTasks[taskId].tLearnMoveState == MVSTATE_SHOW_MOVE_SELECT)
                        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                }
            }
            if (JOY_NEW(B_BUTTON))
            {
                HandleBattleWindow(0x17, 8, 0x1D, 0xD, WINDOW_CLEAR);
                PlaySE(SE_SELECT);
                gTasks[taskId].tLearnMoveState = gTasks[taskId].tLearnMoveNoState;
            }
            break;
        case MVSTATE_SHOW_MOVE_SELECT:
            if (!gPaletteFade->active)
            {
                FreeAllWindowBuffers();
                MbeShowSelectMovePokemonSummaryScreen(gPlayerParty, gTasks[taskId].tPartyId,
                            gPlayerPartyCount - 1, CB2_MidBattleEvolutionLoadGraphics,
                            gMoveToLearn);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_HANDLE_MOVE_SELECT:
            if (!gPaletteFade->active && gMain.callback2 == CB2_EvolutionSceneUpdate)
            {
                var = GetMoveSlotToReplace();
                if (var == MAX_MON_MOVES)
                {
                    gTasks[taskId].tLearnMoveState = MVSTATE_ASK_CANCEL;
                }
                else
                {
                    u16 move = GetMonData(mon, var + MON_DATA_MOVE1, NULL);
                    if (CheckIsHmMove(move))
                    {
                        BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_HMMOVESCANTBEFORGOTTEN - MBE_BATTLESTRINGS_START]);
                        BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                        gTasks[taskId].tLearnMoveState = MVSTATE_RETRY_AFTER_HM;
                    }
                    else
                    {
                        PREPARE_MOVE_BUFFER(gBattleTextBuff2, move)
                        RemoveMonPPBonus(mon, var);
                        SetMonMoveSlot(mon, gMoveToLearn, var);
                        gTasks[taskId].tLearnMoveState++;
                    }
                }
            }
            break;
        case MVSTATE_FORGET_MSG_1:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_123POOF - MBE_BATTLESTRINGS_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tLearnMoveState++;
            break;
        case MVSTATE_FORGET_MSG_2:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_PKMNFORGOTMOVE - MBE_BATTLESTRINGS_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tLearnMoveState++;
            }
            break;
        case MVSTATE_LEARNED_MOVE:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
            {
                BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_ANDELLIPSIS - MBE_BATTLESTRINGS_START]);
                BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
                gTasks[taskId].tState = EVOSTATE_LEARNED_MOVE;
            }
            break;
        case MVSTATE_ASK_CANCEL:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_STOPLEARNINGMOVE - MBE_BATTLESTRINGS_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tLearnMoveYesState = MVSTATE_CANCEL;
            gTasks[taskId].tLearnMoveNoState = MVSTATE_INTRO_MSG_1;
            gTasks[taskId].tLearnMoveState = MVSTATE_PRINT_YES_NO;
            break;
        case MVSTATE_CANCEL:
            BattleStringExpandPlaceholdersToDisplayedString(gBattleStringsTable[STRINGID_DIDNOTLEARNMOVE - MBE_BATTLESTRINGS_START]);
            BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
            gTasks[taskId].tState = EVOSTATE_TRY_LEARN_MOVE;
            break;
        case MVSTATE_RETRY_AFTER_HM:
            if (!IsTextPrinterActive(0) && !IsSEPlaying())
                gTasks[taskId].tLearnMoveState = MVSTATE_SHOW_MOVE_SELECT;
            break;
        }
        break;
    }
}

/* 進化で変わる所だけ更新する。入れ替え・復活ではない: 消費した持ち物・状態異常・能力ランク・みがわり・タイマーは保つ */
static void UpdateEvolvedBattleMon(struct Pokemon *mon)
{
    u8 bank = gNewBS->midBattleEvolution.battler;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);

    gBattleMons[bank].species = species;
    RELOAD_BATTLE_STATS(bank, mon);
    if (gBattleTypeFlags & BATTLE_TYPE_CAMOMONS)
        UpdateTypesForCamomons(bank);
    else
    {
        gBattleMons[bank].type1 = HackDelta_Type(mon, species, 0, gBaseStats[species].type1); //[hack] patches/0962 デルタ種
        gBattleMons[bank].type2 = HackDelta_Type(mon, species, 1, gBaseStats[species].type2);
    }
    gBattleMons[bank].type3 = TYPE_BLANK;
    *GetAbilityLocation(bank) = GetMonAbility(mon);
    ClearBattlerAbilityHistory(bank);
    GetMonData(mon, MON_DATA_NICKNAME, gBattleMons[bank].nickname);
    gStatuses3[bank] &= ~STATUS3_ILLUSION;   /* 登場時の特性の済み印（STATUS3_SWITCH_IN_ABILITY_DONE）は下ろさない＝いかく等を出し直さない */
    gNewBS->secondAbilitySpecies[bank] = species;   /* 2 つ目の特性は今の種族で再評価される。登場時の分は出し直さない（src/second_ability.c） */
    ClearTemporarySpeciesSpriteData(bank, TRUE);
}

static void UpdateEvolvedBattleMoves(struct Pokemon *mon)
{
    u8 bank = gNewBS->midBattleEvolution.battler;
    u8 i;

    ShadowBattle_OnSwitchOut(bank);   /* [hack] patches/0963 段 E ダーク技の組み立て（ダークの子は進化しないので通常は何もしない・念のため） */
    HackChFGacha_OnSwitchOut(bank);   /* わざガチャ中: 今の PP をガチャの控えへ（下で戦闘の技を手持ちの技にそろえるので その前に）。ガチャ中でなければ何もしない */
    for (i = 0; i < MAX_MON_MOVES; ++i)
    {
        /* ものまねで借りた技は消さない。変わっていない枠の PP は戻さない */
        if (mon->moves[i] != gNewBS->midBattleEvolution.originalMoves[i])
        {
            gBattleMons[bank].moves[i] = mon->moves[i];
            gBattleMons[bank].pp[i] = mon->pp[i];
            gBattleMons[bank].ppBonuses = (gBattleMons[bank].ppBonuses & ~(3 << (i * 2)))
                                      | (mon->ppBonuses & (3 << (i * 2)));
            gDisableStructs[bank].mimickedMoves &= ~gBitTable[i];
        }
    }
    ShadowBattle_OnEvolved(bank);   /* [hack] patches/0963 段 E（進化のあとは status2 が正しい＝へんしん中は除く口） */
    HackChFGacha_OnSwitchIn(bank);   /* わざガチャ中: 戦闘の技を ガチャの技に戻す（手持ちの技は本物のまま・戦闘のあとの戻しは今までどおり）。ガチャ中でなければ何もしない */
    if (gBattleTypeFlags & BATTLE_TYPE_CAMOMONS)
        UpdateTypesForCamomons(bank);
}

/* わざ忘れの画面（BW 風サマリー）から戻ったあとの絵の読み直し。素の読み直しが DPE のフック 3 でメス姿・萌えもんの絵を選ぶ */
static void CB2_MidBattleEvolutionLoadGraphics(void)
{
    CB2_EvolutionSceneLoadGraphics();
}
