#pragma once
#include "../include/global.h"

/* 出会った場所（16 ビット）・出会ったレベル（11 ビット・0〜2047）。80 バイト形のヘッダ +0x1E の ext を使う（L 案・2026-10-16・詳細は hack_met_level.c の頭） */
u16 HackMetLocation_Get(const struct BoxPokemon* mon);          //struct Pokemon も (struct BoxPokemon*) にして渡してよい（先頭の並びが同じ）
void HackMetLocation_Set(struct BoxPokemon* mon, u16 location);
u16 HackMetLevel_Get(const struct BoxPokemon* mon);
void HackMetLevel_Set(struct BoxPokemon* mon, u16 level); //SetMonData の 8 ビットの呼び元用（上位ビットは 0 になる）
void HackMetLevel_Set16(struct BoxPokemon* mon, u16 level); //256 以上を書くとき（2047 まで）
void HackMetLevel_Normalize(struct BoxPokemon* mon);            //古い持ち方（hyperTraining の bit6）を ext に直す（箱へ縮める前に）