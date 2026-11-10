#pragma once

//セーブのかき混ぜ（src/hack_save_crypt.c）。フラッシュから読むときはこれを通す（素の ReadFlash を直に呼ばない）
u8 HackSaveCrypt_KeyAt(u32 i);
void HackSaveCrypt_ReadFlash(u16 sector, u32 off, void* dst, u32 n);
void HackSaveCrypt_ReadFlashChecked(u16 sector, void* dst);   //★ 区画全体を読み、MAC が合わなければ dst の +0xFF0 に印（HackSaveCrypt_IsTampered）
bool8 HackSaveCrypt_IsTampered(const void* sec);
bool8 HackSaveCrypt_IsUnsigned(const void* sec);
bool8 HackSaveCrypt_IsLegacyData(u16 sector, u32 fillerId);
bool8 HackSaveCrypt_SlotLegacy(u16 first);   //★ 本体の組に 署名のない前の版の区画があるか   //★ 署名あり・T だけ・id が fillerId でない区画か（棚の古い写しの掃除）   //★ 検算つきの読みで 署名のない区画だった（棚の書き直しの対象）
bool8 HackSaveCrypt_NewerKeyPresent(void);   //★ 2026-10-21 本体の組に この ROM が読めない鍵の区画が 1 つでもあれば TRUE
