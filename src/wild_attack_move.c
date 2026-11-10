#include "defines.h"
#include "defines_battle.h"
#include "../include/pokemon.h"
#include "../include/constants/moves.h"
#include "../include/new/learn_move.h"
#include "../include/new/wild_attack_move.h"

//[hack] patches/0376（2026-10-15）: 足す所は「最初の空き」。前は「最後の空き」で、技が 1〜2 つの子に
//  [はねる, 0, 0, たいあたり] の穴を作っていた（ヒンバス Lv5・捕まえると 4 つ目に使えない たいあたり）。
//  4 つとも変化技なら今までどおり 4 つ目を置き換える。検算 tools/rom/verify_wild_attack_move.py
void WildMon_EnsureAttackMove(struct Pokemon* mon)
{
    u8 slot = MAX_MON_MOVES;
    for (u8 i = 0; i < MAX_MON_MOVES; ++i)
    {
        u16 move = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
        if (move != MOVE_NONE && move < MOVES_COUNT && gBattleMoves[move].split != SPLIT_STATUS)
        {
            if (GetMonData(mon, MON_DATA_PP1 + i, NULL) == 0)
                SetMonMoveSlot(mon, move, i);
            return;
        }
        if (move == MOVE_NONE && slot == MAX_MON_MOVES)
            slot = i;
    }
    if (slot == MAX_MON_MOVES)
        slot = MAX_MON_MOVES - 1;
    SetMonMoveSlot(mon, MOVE_TACKLE, slot);
}
