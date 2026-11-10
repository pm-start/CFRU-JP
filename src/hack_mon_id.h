#pragma once
#include "../include/global.h"

struct CompressedPokemon;   //0.63.1: HackMonId_CompIsMoemonHit の引数（中身は include/pokemon_storage_system 側・前の宣言だけ）

/* ポケモン自身の表 ID・裏 ID（#70・src/hack_mon_id.c） */
u32 HackMonId_Get(const struct BoxPokemon* mon);                 //表 ID | 裏 ID << 16（0 = まだ無い）
void HackMonId_Set(struct BoxPokemon* mon, u32 id);
void HackMonId_Ensure(struct BoxPokemon* mon);
void HackMonId_MakeMoemon(struct BoxPokemon* mon);
bool8 HackMonId_RollTenp(void);                                   //クチバのフカマル: 全員当たり（2026-10-28・元は 1/16）
u32 HackMonId_ToTenp(u32 id, bool8 shiny, u32 pers);                  //FIX-gible-tenp-old-saves: ID を Tenp の当たりに（色違いは保つ・もう当たりならそのまま）
void HackMonId_MakeClubGible(struct BoxPokemon* mon, bool8 tenp);   //色違い＋裏 ID を Tenp の当たり／はずれに
void HackMonId_MakeShiny(struct BoxPokemon* mon);                //性格値は触らず自身の ID を色違いの当たりに（ホウエンのミラクル交換・2026-10-11）               //今の色違いのまま裏 ID を Moémon の当たりに（2026-10-10）                   //無ければ今の色違いのまま付ける
#ifdef HACK_DEBUG_MENU   //patches/0417 デバッグメニュー付きの版だけ
void HackMonId_SetShiny(struct BoxPokemon* mon, bool8 on);       //デバッグメニューだけ（2026-10-12・性格値は触らない）
void HackMonId_SetMoemon(struct BoxPokemon* mon, bool8 on);
#endif
void HackMonId_SetParts(struct BoxPokemon* mon, bool8 shiny, bool8 moemon);   //patches/0429 色違い・Moémon を自身の ID だけで決め直す
bool8 HackMonId_IsShinyBox(const struct BoxPokemon* mon);
bool8 HackMonId_CompIsMoemonHit(const struct CompressedPokemon* comp);   //箱の縮めた形のまま同じ定義で（0.63.1 Moémon の世代の控え）
bool8 HackMonId_IsMoemonHit(const struct BoxPokemon* mon);         //第 2 段: 萌えもんの当たり（裏 ID・ON/OFF は見ない）
u32 HackMonId_MoemonKey(u32 personality);
//ボックスの縮めた 58 バイト（src/pokemon_storage_system.c が使う）
u32 HackMonId_UnpackComp(const void* comp);
void HackMonId_PackComp(void* comp, u32 id);
void HackMonId_MakeShinyMoemon(struct BoxPokemon* mon);   //patches/0393 わたしたちの手持ち: 色違い＋Moémon の当たりの ID
void HackMonId_PackBall(void* comp);   //patches/0393 ボール 32 以上の 6 本目のビット（PackComp の前に）
void HackMonId_ClearSpare(struct BoxPokemon* mon);
void HackMonId_FieldTick(void);
