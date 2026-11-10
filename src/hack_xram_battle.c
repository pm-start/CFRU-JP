#include "defines.h"
#include "defines_battle.h"
#include "../include/malloc.h"
#include "hack_xram_battle.h"

/* 追加RAM末尾の専用枠。先頭の上限レベル退避と、末尾の検出語は避ける。
 * セーブ対象外。BIOSのCpuSetは使わず、バイト単位で初期化する。
 * 構造体全体を移すため、内部ポインタや既存の参照方法は変えない。 */
#define XRAM_BATTLE_BASE 0x01FF0000
#define XRAM_BATTLE_CAPACITY 0xF000
#define XRAM_BATTLE_TEMP_BASE (XRAM_BATTLE_BASE + XRAM_BATTLE_CAPACITY)
#define XRAM_BATTLE_PROBE 0x01FFFFFC

typedef char HackXramBattle_SizeFits[
    sizeof(struct NewBattleStruct) <= XRAM_BATTLE_CAPACITY ? 1 : -1];
typedef char HackXramBattle_TempTeamFits[
    sizeof(struct Pokemon) * PARTY_SIZE <= XRAM_BATTLE_PROBE - XRAM_BATTLE_TEMP_BASE ? 1 : -1];

extern bool8 HackXram_Available(void);

struct NewBattleStruct* HackXramBattle_Alloc(void)
{
    if (HackXram_Available())
    {
        volatile u8* bytes = (volatile u8*) XRAM_BATTLE_BASE;
        u32 i;
        for (i = 0; i < sizeof(struct NewBattleStruct); ++i)
            bytes[i] = 0;
        return (struct NewBattleStruct*) XRAM_BATTLE_BASE;
    }
    return Calloc(sizeof(struct NewBattleStruct));
}

void HackXramBattle_Free(struct NewBattleStruct* state)
{
    /* 追加RAMの固定枠を通常ヒープへFreeしてはいけない。
     * 中の別途確保された入れ物は、従来どおり呼出元で先に解放する。 */
    if (state != NULL && state != (struct NewBattleStruct*) XRAM_BATTLE_BASE)
        Free(state);
}

/* スカイバトル終了時だけの作業用。構造体本体の専用枠とは重ねない。
 * 通常環境ではNULLを返し、従来のスタック処理へ戻す。 */
void* HackXramBattle_TempTeam(void)
{
    if (HackXram_Available())
    {
        volatile u8* bytes = (volatile u8*) XRAM_BATTLE_TEMP_BASE;
        u32 i;
        for (i = 0; i < sizeof(struct Pokemon) * PARTY_SIZE; ++i)
            bytes[i] = 0;
        return (void*) bytes;
    }
    return NULL;
}
