#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 3（patches/0978）: ポロックをポケモンにあげる画面（src/hack_pokeblock_use.c）。使う側は defines.h の後に読む。
void ChooseMonToGivePokeblock(MainCallback callback);   //ケースの「つかう」から（選んだポロックは gSpecialVar_ItemId の番号・ケースの並べ直しのあと）。callback は閉じたあとの戻り先（ケースを開き直す）
void HackPokeblock_AddFriendship(struct Pokemon* mon, s16 gain);   //好きな味（gain > 0）のポロックを食べたときのなつき度（追加の仕様）
