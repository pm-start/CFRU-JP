#pragma once

#include "../include/global.h"

/* Preserve existing held items; roll 0..99 selects difficulty recovery berries. */
u16 HackDifficulty_RecoveryItem(u16 currentItem, u16 level, u8 roll, u16 difficulty);
u32 HackDifficulty_BuyPrice(u32 price, u16 difficulty);
bool8 HackDifficulty_BagLimitReached(u16 difficulty, u8 usedCount);
bool8 HackDifficulty_ForceSetStyle(u16 difficulty);
bool8 HackDifficulty_ItemBlocked(u16 item);                      //しゃかりき: キズぐすり以外の回復の道具は つかえない（バッグ・きのみぶくろ・patches/0616）
bool8 HackDifficulty_HeldRecoveryNullified(u8 bank, u8 effect);  //しゃかりき: 自分の側の もたせた回復のきのみ系は こうかなし（battle_util.c の GetBankItemEffect・item_battle_effects.c）
bool8 HackDifficulty_HeldRegenBlocked(u8 bank, u8 effect);       //しゃかりき: 自分の側の ももアイス系・くりーぱーアイスの 毎ターンの回復だけ止める

// Existing save values retain their meaning. Menu order is easy to hardest.
enum HackDifficulty
{
    DIFFICULTY_NORMAL,
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
    DIFFICULTY_MOMO_ICE,
    DIFFICULTY_RIRU_ICE,     //5 りるアイス（やさしいより やさしい・2026-10-11・patches/0321）
    DIFFICULTY_SHAKARIKI,    //6 しゃかりき（ももアイスより むずかしい・2026-10-18・番号は足すだけ＝セーブの値 HOENN_VAR_DIFFICULTY の意味を変えない）
    DIFFICULTY_COUNT,
};

//ももアイスの仲間（ももアイス・しゃかりき）か。ももアイスの中身を引き継ぐ判定は全部これを通す
static inline bool8 HackDifficulty_IsMomoClass(u16 difficulty)
{
    return difficulty == DIFFICULTY_MOMO_ICE || difficulty == DIFFICULTY_SHAKARIKI;
}

enum HackAIRank
{
    AI_RANK_BASIC = 0,
    AI_RANK_SEMI_SMART = 2,
    AI_RANK_STANDARD = 4,
    AI_RANK_ADVANCED = 8,
    AI_RANK_EXPERT = 14,
    AI_RANK_MAX = 24,
};

struct AIRankContext
{
    u16 difficulty;
    u16 level; //[hack] patches/0679
    bool8 importantTrainer;
    bool8 lowHp;
    bool8 statused;
    bool8 prolongedBattle;
    bool8 boostedFoe;
    bool8 wild;
};

u8 HackAI_GetRank(const struct AIRankContext* context);
u8 HackAI_GetBattlerRank(u8 bank);
u8 HackAI_GetTrainerRank(u8 bank);
u8 HackAI_GetWildRank(u8 bank);
u32 HackAI_TrainerFlags(u32 flags, u8 bank);
u32 HackAI_WildFlags(u32 flags, u8 bank);
void HackDifficulty_AdjustFrontierMon(struct Pokemon* mon, bool8 forPlayer, bool8 isBrain);   //ももアイス: 施設の相手も Lv+3（src/build_pokemon.c の CreateFrontierMon から・2026-10-11）
u32 HackDifficulty_CatchOdds(u32 odds);          //捕獲率の補正（なんいどが低いほど捕まえやすい・src/catching.c から・2026-10-11）
u32 HackDifficulty_BuyPrice(u32 price, u16 difficulty);   //お店の買い値（りるアイスは半額・src/mega_shop.c から・2026-10-11・f8 の liru さん 3 回目と同じ形）
void HackDifficulty_WildMon(struct Pokemon* mon); //りるアイス: 野生は 2V 保証（src/wild_encounter.c の CreateWildMon から）
bool8 HackDifficulty_IsStoryTrainerId(u16 id);    //物語の大事な相手か（クラスで決める・patches/0435）
bool8 HackAI_MobNoSwitch(u8 bank);                //モブの AI は自分から入れ替えない（りるアイス〜ふつう・src/Battle_AI/ai_master.c の ShouldSwitch から・patches/0435）
bool8 HackAI_MoveBanned(u8 bank, u16 move);       //まもる系 3 回・能力アップ系 2 回・同じ攻撃技は続けて 2 回まで（src/Battle_AI/ai_master.c の FindBestLegalMoves から・patches/0436）
void HackAI_NoteMoveChosen(u8 bank, u16 move);    //相手が技を選んだ（src/battle_controller_opponent.c から・patches/0436）

static inline u8 HackDifficulty_SelectionFromValue(u16 value)
{
    switch (value)
    {
    case DIFFICULTY_RIRU_ICE: return 0;   //画面の並び: りるアイス → やさしい → ほどほど → ふつう → むずかしい → ももアイス
    case DIFFICULTY_EASY: return 1;
    case DIFFICULTY_MEDIUM: return 2;
    case DIFFICULTY_HARD: return 4;
    case DIFFICULTY_MOMO_ICE: return 5;
    case DIFFICULTY_SHAKARIKI: return 6;   //しゃかりき（ももアイスの次）
    default: return 3;
    }
}

static inline u8 HackDifficulty_ValueFromSelection(u16 selection)
{
    switch (selection)
    {
    case 0: return DIFFICULTY_RIRU_ICE;
    case 1: return DIFFICULTY_EASY;
    case 2: return DIFFICULTY_MEDIUM;
    case 4: return DIFFICULTY_HARD;
    case 5: return DIFFICULTY_MOMO_ICE;
    case 6: return DIFFICULTY_SHAKARIKI;
    default: return DIFFICULTY_NORMAL;
    }
}
