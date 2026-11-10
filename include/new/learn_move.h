#pragma once

#include "../global.h"

/**
 * \file learn_move.h
 * \brief Contains functions relating to Pokemon learning new moves. Also contains
 *		  functions for Evolution moves and the Move Relearner.
 */

 //Exported Structs
struct MovePair
{
	u16 move;
	u16 num;
};

//Exported Functions
void GiveBoxMonInitialMoveset(struct BoxPokemon* boxMon);
u16 MonTryLearningNewMoveAfterEvolution(struct Pokemon* mon, bool8 firstMove);
u16 GetLevelUpMovesBySpecies(u16 species, u16* moves); //[hack] patches/0598 戻り値 u8 → u16（技が 255 をこえる種族に備える）
u16 GetLevelUpMovePairsBySpecies(u16 species, struct MovePair* moves);
u16 BuildLearnableMoveset(struct Pokemon* mon, u16* moves);
u16 BuildTMMoveset(struct Pokemon* mon, struct MovePair* moves);
u16 BuildTutorMoveset(struct Pokemon* mon, struct MovePair* moves);
u16 RandomizeMove(u16 move);

//Functions Hooked In
u16 MonTryLearningNewMove(struct Pokemon* mon, bool8 firstMove);
u8 GetMoveRelearnerMoves(struct Pokemon* mon, u16* moves);
u8 GetNumberOfRelearnableMoves(struct Pokemon* mon);
u16 GiveMoveToBoxMon(struct BoxPokemon *boxMon, u16 move);
void SetMonMoveSlot(struct Pokemon* mon, u16 move, u8 slot);
void InitLearnMoveFix(void);
bool8 CheckMoveRelearnerMoveLimit(u8 counter);
u16 GetMoveIdFromRelearnerStruct(u8 index);
void InitMoveRelearnerMoveIDs(void);
u8 GetRelearnableMoves(struct Pokemon* mon);
const u8* CopyMoveReminderMoveName(u8 cursor);
bool16 InitMoveRelearnerWindows(void);

//Exported Constants
#define MAX_LEARNABLE_MOVES 40
#define MAX_LEVEL_UP_MOVES 512 // 一時配列用（Malloc／Calloc で確保する＝スタックに置かない）。画面の40件枠とは別。0.53.0 の最大 204 件・0.53.2 で約 410 件の見込み（patches/0598）。
