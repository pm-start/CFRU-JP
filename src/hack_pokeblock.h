#pragma once

//★ 2026-11-09 FEAT-pokeblock（patches/0978）: ポロックケース（40 個）＝追加のセーブ領域の入れ物（src/hack_pokeblock.c）。画面・ブレンダー・あげる画面は別のファイル。
//  置き場は割り振り表（tools/save_layout/ext_sector_map.json）の pokeblock＝追加の区画 POKEBLOCK_SECTOR・+1 の交互 2 区画。
//  どちらが正か・数は SaveBlock1 +0x3A34 の 6 バイト、きのみブレンダーの記録（エメラルドの berryBlenderRecords）は +0x3A3A の 6 バイト。
//  追加のセーブ領域が使えない版（素の mGBA・実機・SuperCard）ではケースは空で、ポロックは増やせない（ユーザー決定 2026-11-09）。記録だけは SaveBlock1 なので使える。
//  使う側は defines.h（struct Pokeblock＝global.h・7 バイト）の後にこのヘッダを読む。
#define POKEBLOCK_CASE_SLOTS 40            //エメラルドの POKEBLOCKS_COUNT

#define POKEBLOCK_OK 0
#define POKEBLOCK_NO_EXT 1                 //追加のセーブ領域が使えない版
#define POKEBLOCK_NEED_REPORT 2            //使えるかをまだ決められない（最初のレポートの前）
#define POKEBLOCK_BAD_DATA 3               //中身が読めない（壊れている）

struct PokeblockImage                      //追加の区画 1 つに書く（先頭から・RAM の入れ物にもこの形で読む）。書き換え可能な static は使えない（6.13）ので入れ物は呼ぶ側が AllocZeroed する
{
	u32 magic;
	u16 version;
	u16 sum;                               //先頭 16 バイトより後ろの和
	u32 spare[2];
	struct Pokeblock blocks[POKEBLOCK_CASE_SLOTS];   //1 個 7 バイト（color が 0 なら空き）
};

u8 Pokeblock_Usable(void);                                    //POKEBLOCK_OK／NO_EXT／NEED_REPORT
u8 Pokeblock_Count(void);                                     //入っている数（読むだけ・SaveBlock1 を書き換えない）
u32 Pokeblock_ImageSize(void);
bool8 Pokeblock_Load(struct PokeblockImage* image);           //今の中身を入れ物へ。FALSE = 読めない（入れ物は空にする）。使えない版・数 0 は空で TRUE
bool8 Pokeblock_Commit(struct PokeblockImage* image);         //入れ物の今の中身を書く（交互 2 区画）。FALSE = 書けない（状態は変えない）
bool8 Pokeblock_Add(struct PokeblockImage* image, const struct Pokeblock* pb, u8* slotOut);   //空きに足して書く。FALSE = 満杯／書けない（入れ物は元に戻す）
bool8 Pokeblock_Remove(struct PokeblockImage* image, u8 slot);                               //外して書く。FALSE = 空き／書けない（入れ物は元に戻す）
u8 Pokeblock_ImageCount(const struct PokeblockImage* image);                                 //入れ物の中の数
void Pokeblock_OnLoad(void);                                  //つづきから（src/save.c）
void Pokeblock_AfterSave(bool8 ok);                           //レポートのあと（src/save.c）

//きのみブレンダーの記録（1・2・3 人用の最高の回転数・エメラルドの berryBlenderRecords[3]・SaveBlock1 +0x3A3A）
#define POKEBLOCK_RECORD_COUNT 3
u16 Pokeblock_GetRecord(u8 i);
void Pokeblock_SetRecord(u8 i, u16 v);                        //大きいときだけ更新するかは呼ぶ側が決める（ここは書くだけ）

//ケースの画面の前回の位置（原文 sSavedPokeblockData の置き換え・SaveBlock1 +0x3A40 の 4 バイト＝row・scroll・戻り先の種類・予備・FEAT-pokeblock 段 2）
void Pokeblock_GetCasePos(u8* row, u8* scroll);
void Pokeblock_SetCasePos(u8 row, u8 scroll);
u8 Pokeblock_GetCaseReturn(void);                             //戻り先の種類（hack_pokeblock_case.h の PBC_RET_*）・SaveBlock1 +0x3A42（段 3）
void Pokeblock_SetCaseReturn(u8 kind);
