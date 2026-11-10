#include "defines.h"
#include "../include/save.h"
#include "hack_save_crypt.h"

/*
    セーブのかき混ぜ（2026-10-09 ユーザー「ポケモンIDとか変えてほしい　PokeHexとかで連れて行かれないように」→
    「CFRUとも互換なくすように　1やな」＝セーブの中身を PokemonStart だけの鍵でかき混ぜる）。
    資料: docs/research/2026-10-09_save-crypt.md

    フラッシュに書く区画（4KB）を丸ごと、区画の中の位置で決まる鍵（HackSaveCrypt_KeyAt・tools/save_editor/savecrypt.py と同じ式）で
    XOR してから書き、読むときに戻す。本体の組・棚のページ・40〜50 箱・追加の区画・ぼうけんのしょ 2（バンク +48）すべてに効く。
      書く: 素の ProgramFlashSectorAndVerify（081C2C50・functionrewrites）をこれに。書く前に XOR、書き終えたら元に戻す（呼んだ側のバッファは変えない）
      読む: 素の DoReadFlashWholeSection（080DB178・functionrewrites）と、棚・追加の区画が直に呼んでいた ReadFlash を HackSaveCrypt_ReadFlash に
    前の版のセーブ（かき混ぜていない区画）は、区画の終わりの署名（+0xFF8 = 0x08012025）がそのまま読めるかで見分けてそのまま読む
    ＝引っ越しは要らない（次のレポートでかき混ぜて書く）。消えたままの区画（FF）は混ぜたものとして戻すので、署名は合わない（前と同じく「無い」）。
    かき混ぜないまま読むもの: 書き損じの画面の消去の確かめ（素の ReadFlash のまま）・ぼうけんのしょの 6MB 版の確かめ（hack_save_slot.c・読み比べるだけ）。
*/

#define SECTOR_SIZE 0x1000
#define ReadFlashVanilla ((void (*)(u16, u32, void*, u32)) (0x081C2A54 | 1))
#define VerifyFlashSectorVanilla ((u32 (*)(u16, u8*)) (0x081C2B20 | 1))
#define ProgramFlashSectorPtr (*(u16 (**)(u16, void*)) 0x0300746C)

/*
    ★ 2026-10-21 段階 3（セーブ難読化・版ごとの鍵）: 中身は src/hack_save_obf.h（ホストの clang でそのまま組んで検算する）。
      上の かき混ぜ(T) の内側に 版の鍵の加算(V) を足した。
      書く: 署名のある区画は 中身に V（この ROM の版の鍵）・+0xFF0 に印（版＋認証）→ T → 書く
      読む: T を戻して 印を見る → 印が無い＝前の版の姿（V 無し）・印がある＝その版の鍵で V を戻す・鍵が無い／印が化けた＝署名を潰して「無い区画」に
      checksum は V をかけても変わらない（鍵の和が 0）＝前の版の ROM でも区画は「正しい」と読め、版の変数の止め画面へ届く
    資料 docs/research/2026-10-21_save-obfuscation.md
*/
#define OBF_FLASH_READ(sector, off, dst, n) ReadFlashVanilla(sector, off, dst, n)
#define OBF_FLASH_PROGRAM(sector, src) ProgramFlashSectorPtr(sector, src)
#define OBF_FLASH_VERIFY(sector, src) VerifyFlashSectorVanilla(sector, src)
#include "hack_save_obf.h"

u8 HackSaveCrypt_KeyAt(u32 i)
{
	return Obf_TKey(i);
}

void HackSaveCrypt_ReadFlash(u16 sector, u32 off, void* dst, u32 n)
{
	Obf_Read(sector, off, dst, n);
}

//区画全体を 改造の検出つきで読む（src/save.c の HandleLoadSector・棚のページの検算が使う）。MAC が合わない区画は dst の +0xFF0 に印が付く（HackSaveCrypt_IsTampered）
void HackSaveCrypt_ReadFlashChecked(u16 sector, void* dst)
{
	Obf_ReadEx(sector, 0, dst, SECTOR_SIZE, 1);
}

bool8 HackSaveCrypt_IsTampered(const void* sec)
{
	return Obf_IsTampered((const u8*) sec);
}

//本体の組（first から 5 区画）に 署名のない前の版の区画があるか（src/save.c の SaveWriteToFlash が 控えの組も書き直すか決める）
bool8 HackSaveCrypt_SlotLegacy(u16 first)
{
	return Obf_SlotLegacy(first);
}

//だれも指していない前の版の区画（署名あり・T だけ・中身を持つ）か（棚の古い写しを消す対象・src/hack_box_shelf.c）
bool8 HackSaveCrypt_IsLegacyData(u16 sector, u32 fillerId)
{
	return Obf_IsLegacyData(sector, fillerId);
}

//検算つきの読みで 署名のない（前の版が書いた）区画だった
bool8 HackSaveCrypt_IsUnsigned(const void* sec)
{
	return Obf_IsUnsigned((const u8*) sec);
}

//素の DoReadFlashWholeSection（080DB178）の代わり
u8 HackSaveCrypt_DoReadFlashWholeSection(u8 sector, struct SaveSection* section)
{
	HackSaveCrypt_ReadFlash(sector, 0, section, SECTOR_SIZE);
	return TRUE;
}

//素の ProgramFlashSectorAndVerify（081C2C50）の代わり（3 回まで書いて確かめる・0 なら書けた）
u32 HackSaveCrypt_ProgramFlashSectorAndVerify(u16 sector, u8* src)
{
	return Obf_Write(sector, src);
}

//本体の組に この ROM が読めない鍵の区画があるか（src/bw_main_menu.c が「あたらしい バージョン」の止め画面に使う）
bool8 HackSaveCrypt_NewerKeyPresent(void)
{
	return Obf_NewerKeyPresent();
}
