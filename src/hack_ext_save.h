#pragma once

#include "hack_ext_sector_map.h"   //区画の割り振りの定数（生成物・正本は tools/save_layout/ext_sector_map.json）

//追加のセーブ領域（src/hack_ext_save.c・直した mGBA だけ）。区画 n は 0〜EXT_SECTOR_COUNT-1
bool8 HackExtSave_Available(void);
u8 HackExtSave_Check(void);                 //0 = 使えない・1 = 使える・2 = 決められない（本体の比べる相手がまだ無い）
bool8 HackExtSave_BackHalfVisible(void);    //64MB の後ろ半分（PKSPICS1）が見えるか
bool8 HackExtSave_BaseHeadErased(u8 sector); //本体の区画（0〜31）の頭がまだ消えたまま（0xFF）か
bool8 HackExtSave_LegacySlot2Present(void);    //旧配置（6MB）のぼうけんのしょ 2 に中身が残っているか（ぼうけんのしょ 1 の側で呼ぶ・使えない版は偽）
bool8 HackExtSave_LayoutMarkValid(void);      //形の合う配置版の印があるか
u8 HackExtSave_LayoutVersion(void);          //配置版の印を読む（無い・壊れ・使えない = 1）。EXT_LAYOUT_VERSION_CAP より大きければ知らない配置
bool8 HackExtSave_ReadSector(u16 n, void* dst);
bool8 HackExtSave_ReadSectorChecked(u16 n, void* dst);   //★ 改造の検出つき（HackSaveCrypt_IsTampered で見る）
bool8 HackExtSave_Read(u16 n, u32 off, void* dst, u32 size);
bool8 HackExtSave_ReadKnownAvail(u16 n, u32 off, void* dst, u32 size);   //★ patches/0740 使えると分かっているときの読み（バンクの確かめを飛ばす）
bool8 HackExtSave_WriteSector(u16 n, const void* src);

//後ろ半分（0x0A000000〜）の読み口（src/hack_split_pics.c）。src から len バイトを dst へ。読めなければ FALSE
bool8 HackExt_Read(const void* src, void* dst, u32 len);
//上位 64MB のバンク（2・3・128MB の ROM・窓の切り替え）の bank の off から len バイトを RAM へ（src/hack_split_pics.c）。IME を止めた中で 窓を切り替え→印を確かめ→写し→窓を 1 に戻す。入れ子・割り込みの中からは呼ばない。印が違えば FALSE
bool8 HackExt_ReadW(u8 bank, u32 off, void* dst, u32 len);
