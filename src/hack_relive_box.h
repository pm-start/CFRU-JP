#pragma once

//★ 2026-11-09 FEAT-relive-box-ext（patches/0975・案 B）: リライブボックス＝追加のセーブ領域の専用の箱（ダークの子だけ・30 匹）の入れ物（src/hack_relive_box.c・画面は src/hack_relive_box_screen.c）。
//  PC の箱 26（0.65.0 の専用）はふつうの箱に戻した。置き場は割り振り表（tools/save_layout/ext_sector_map.json）の relive_box＝追加の区画 RELIVE_BOX_SECTOR・+1 の交互 2 区画。
//  追加のセーブ領域が使えない版（素の mGBA・実機・SuperCard）では使えない（ユーザー決定）。
#define RELIVE_BOX_SLOTS 30

#define RELIVE_BOX_OK 0
#define RELIVE_BOX_NO_EXT 1       //追加のセーブ領域が使えない版
#define RELIVE_BOX_NEED_REPORT 2  //使えるかをまだ決められない（最初のレポートの前）
#define RELIVE_BOX_BAD_DATA 3     //中身が読めない（壊れている）

struct CompressedPokemon;

u8 ReliveBox_Usable(void);                                   //RELIVE_BOX_OK／NO_EXT／NEED_REPORT
u8 ReliveBox_Count(void);                                    //入っている数（読むだけ・SaveBlock1 を書き換えない）
u32 ReliveBox_ImageSize(void);
bool8 ReliveBox_Load(void* image);                           //今の中身を入れ物へ（image は ReliveBox_ImageSize() バイト）。FALSE = 読めない（入れ物は空にする）
bool8 ReliveBox_SlotUsed(const void* image, u8 slot);
struct CompressedPokemon* ReliveBox_SlotMon(void* image, u8 slot);
bool8 ReliveBox_Put(void* image, const struct CompressedPokemon* mon, u8* slotOut);   //入れ物に足して書く。FALSE = 満杯／書けない（入れ物は元に戻す）
bool8 ReliveBox_Take(void* image, u8 slot);                  //入れ物から外して書く。FALSE = 空き／書けない（入れ物は元に戻す）
bool8 ReliveBox_Commit(void* image);                         //入れ物（image）の今の中身を書く（ゲージの書き換え用）
void* ReliveBox_ScratchImage(void);                          //gSaveDataBuffer（歩数・移行など画面の外で使う入れ物）
void ReliveBox_OnLoad(void);                                 //つづきから（src/save.c）
void ReliveBox_AfterSave(bool8 ok);                          //レポートのあと（src/save.c）

//画面（src/hack_relive_box_screen.c・べんりきのう）
void CB2_HackReliveBox(void);
