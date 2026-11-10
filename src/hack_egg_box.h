#pragma once

//★ 2026-10-23 FEAT-save16-B4（Redmine #38）: 卵専用ボックス（1 つ・30 匹・箱の中で孵化）の入れ物（src/hack_egg_box.c・画面は src/hack_egg_box_screen.c）。
//  置き場は割り振り表（tools/save_layout/ext_sector_map.json）の egg_box＝追加の区画 EGG_BOX_SECTOR・+1 の交互 2 区画（mGBA の追加の領域が使えるときだけ）。
#define EGGBOX_SLOTS 30

#define EGGBOX_OK 0
#define EGGBOX_NO_EXT 1       //追加のセーブ領域が使えない版（素の mGBA・実機・SuperCard）
#define EGGBOX_NEED_REPORT 2  //使えるかをまだ決められない（最初のレポートの前）
#define EGGBOX_BAD_DATA 3     //中身が読めない（壊れている）

struct CompressedPokemon;

u8 EggBox_Usable(void);                                   //EGGBOX_OK／NO_EXT／NEED_REPORT
u8 EggBox_Count(void);                                    //入っている数（卵と、かえった子の合計）
u32 EggBox_Clock(void);                                   //卵の時計（サイクル）
bool8 EggBox_IsHatched(u32 hatchAt);                      //孵る時刻に時計が届いたか
bool8 EggBox_Load(void* image);                           //今の中身を RAM の入れ物へ（image は EggBox_ImageSize() バイト）。FALSE = 読めない
u32 EggBox_ImageSize(void);
u32 EggBox_SlotHatchAt(const void* image, u8 slot);       //0 = 空
const struct CompressedPokemon* EggBox_SlotMon(const void* image, u8 slot);
bool8 EggBox_Put(void* image, const struct CompressedPokemon* mon, u8 cyclesLeft, u8* slotOut);   //入れ物に足して書く。FALSE = 満杯／書けない（入れ物は元に戻す）
bool8 EggBox_Take(void* image, u8 slot);                  //入れ物から外して書く。FALSE = 空き／書けない（入れ物は元に戻す）
bool8 EggBox_OnStep(void);                                //1 歩ごと（overworld.c）。1 サイクルが終わって新しくかえった子がいれば TRUE（図鑑・孵化数は済ませる）
void EggBox_OnLoad(void);                                 //つづきから（src/save.c）
void EggBox_AfterSave(bool8 ok);                          //レポートのあと（src/save.c）

//画面（src/hack_egg_box_screen.c・べんりきのう）
void CB2_HackEggBox(void);
