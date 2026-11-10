#include "defines.h"
#include "../include/event_data.h"
#include "../include/malloc.h"
#include "hack_ext_save.h"

/*
 * 追加のセーブ領域 2.875MB（2026-10-08 ユーザー「1mで」→ 2026-10-09「先に 6MB に広げてから 2 つ目のセーブを作る」）。
 * docs/research/2026-10-08_save-1mb.md / 2026-10-09_save-6mb-two-slots.md
 *
 * - 直した mGBA（tools/mgba/save1m.patch）はフラッシュを 6MB（64KB × 96 バンク）まで持つ。ゲームがバンク 2 以上に
 *   切り替えたときだけ .sav を 6MB に広げる（前の 1MB の .sav は中身を残して広げる）。区画（4KB）の番号はバンク × 16 + バンクの中の番号＝
 *   本体の 128KB は 0〜31、追加は 32〜767（バンク 2〜47）。バンク 48〜95 は 2 つ目のセーブ（同じ並びを 48 バンク後ろへ）に取ってある。
 * - ★ 実機の 128KB のフラッシュにはバンク 0・1 しか無い（推測: バンクの番号の下 1 ビットしか見ない）→ 実機で 32 番以降に書くと本体のセーブを
 *   上書きして壊す。使ってよいのは「64MB の ROM の後ろ半分が見える」とき（＝直した mGBA で _split64.gba を遊んでいる）だけ。
 *   後ろ半分の頭には PKSPICS1（tools/build/build_rom.py の write_split_outputs）。SuperCard（0x0A000000 は ROM の写し）・素の mGBA・実機では見えない。
 * - 書くのは素の ProgramFlashSectorAndVerify（081C2C50・区画の番号は u16）。素の TryWriteSector（080DA9C0）は区画の番号が u8 で
 *   256 番から書けず、壊れた区画の印（1 << 番号）も 32 番からは外れるので通さない。書き込みの関数は区画の番号を gFlash->sector.count（32）で
 *   はねるので、書く間だけ gFlash（03007470 の指す FlashType）を count = 768 の写しに差し替える。読むのは素の ReadFlash（081C2A54・
 *   区画の番号 >> 4 でバンクを切り替える・count は見ない）。
 * ★ 2026-10-23 B2（配置版 2）: 追加の区画は 2016 個（区画 32〜2047＝バンク 2〜127）・ぼうけんのしょ 2 は +128 バンク・探りは 2・4・8・16・32・47・64・127。
 *   16MB 版の mGBA（B1）だけで使える（6MB 版の mGBA は 96 以上のバンクを丸めるので 127 の探りで「使えない」になる）。以下の 736／47／6MB は旧配置の説明。
 *   配置版の印（区画 2）は初めて追加の領域に書くときに、旧配置のスロット 2 に中身が無ければ書く（StampLayoutMark）。
 * - 使っている区画: 0 = 6MB 版の mGBA かの確かめ（src/hack_save_slot.c）・1／735 = HackExtSave_DebugTest・8〜25 = ボックスの 33〜50 箱・26〜55 = 箱のページの写し（src/hack_box_shelf.c）・
 *   64 = フロンティアのレベル49・100 の記録（src/hoenn_frontier_rec.c・2026-10-20・ヘッダ付きで 2KB まで伸ばせる）。
 *   ★ 2026-10-23 区画の割り振りの正本は tools/save_layout/ext_sector_map.json（検算 tools/save_layout/verify_ext_sector_map.py）。区画を足すときは先に表へ載せる（56〜59 = 卵ボックス・65〜72 = プラチナの記録は予約）。使う側は HackExtSave_Available() を見てから読み書きし、使えない版では
 *   入れたものが無い前提で動くこと。
 */
//EXT_FIRST_SECTOR（32）・EXT_SECTOR_COUNT（736 = 区画 32〜767＝バンク 2〜47）・EXT_SECTOR_SIZE（0x1000）は hack_ext_sector_map.h（生成物・正本 tools/save_layout/ext_sector_map.json）
#define gFlashPtr (*(const u8**) 0x03007470)  //const struct FlashType *gFlash（romSize +0・sector.size +4・shift +8・count +0xA）
#define FLASHTYPE_COPY_SIZE 0x20
#define ProgramFlashSectorAndVerifyVanilla ((u32 (*)(u16, u8*)) (0x081C2C50 | 1))   //0 なら書けた
#include "hack_save_crypt.h"

#define ReadFlashVanilla ((void (*)(u16, u32, void*, u32)) (0x081C2A54 | 1))   //かき混ぜたまま読む（比べるだけ）
#define BANK_PROBE_SIZE 16

static bool8 SameBytes16(const u8* a, const u8* b)
{
	u32 i;
	for (i = 0; i < BANK_PROBE_SIZE; ++i)
		if (a[i] != b[i])
			return FALSE;
	return TRUE;
}

/* ★ 2026-10-11 ユーザーのセーブが壊れた（docs/research/2026-10-11_save-shelf-page-corruption.md）:
   64MB の ROM が見えても、mGBA がバンク 2 以上の切り替えを無視する版（.sav が 128KB のまま）だと、区画 32 への書き込みが
   「今のバンクの区画 0」（物理 0 か 16）に落ちて本体を壊す（src/hack_save_slot.c の確かめ用の書き込みで棚のページ 15 が消えた）。
   書く前に「読むだけ」でバンクの切り替えが効くかを見る: 区画 0（バンク 0）→ 区画 32（バンク 2）の頭が同じなら
   切り替えが無視された（か 1 ビットに丸められた）か、どちらも消えたまま＝使わない。区画 16（バンク 1）→ 区画 32 も同じ。
   はじめから直後（全部 0xFF）は決められない（HackExtSave_Check が 2）・最初のレポートのあとは下の s = 10 の組が必ず埋まるので使える（2026-10-16） */
//ref（本体のバンク 0 か 1 の区画）を読んだ直後に test（高いバンクの区画）を読んで、違うものが読めるか。
//  切り替えを無視する版は「今のバンク＝ref のバンク」の同じ所が読めるので同じになる・1 ビットに丸める版も 0 か 1 のどちらかと同じになる
static bool8 ProbeDiffers(u16 refSector, u16 testSector)
{
	u8 a[BANK_PROBE_SIZE], c[BANK_PROBE_SIZE];

	ReadFlashVanilla(refSector, 0, a, BANK_PROBE_SIZE);
	ReadFlashVanilla(testSector, 0, c, BANK_PROBE_SIZE);
	return !SameBytes16(a, c);
}

//★ 5e の指摘 2026-10-11: 前の 1MB 版の mGBA（バンク 0〜15 は本物・16 以上は無視して今のバンクに書く）はバンク 2 が本物なので、
//  一番低いバンク（2）だけでなく使う一番高いバンク（追加の区画の末尾＝バンク 47・ぼうけんのしょ 2 なら +48 の hooks で 95）も見る。
//  区画の番号は今のスロット基準（スロット 2 のときは SwitchFlashBank の hooks が +48 する）
//★ 2026-10-16（docs/research/2026-10-16_adventure-log2-detect.md）: 比べる区画の「バンクの中の番号 s」を 0 だけでなく 0〜15 のどれでもよくした。
//  s 番について、バンク 0 の s 番・バンク 1 の s 番（＝本体の 128KB の中・どの mGBA でも本物）が両方とも消えていない（頭 16 バイトが全部 0xFF でない）なら、
//  高いバンク b の s 番（区画 b×16+s）の頭がその両方と違うことを b = 2・4・8・16・32・47 で確かめる。
//  切り替えを無視する版・下 1 ビットしか見ない版・2 のべき乗で割った余りに回す版は、どれかの b で必ず「バンク 0 か 1 の s 番」と同じものを読む（ref を読んだ直後に test を読む）。
//  s の範囲を広げただけで、「両方と違う」「読むだけ」「書く道を増やさない」は前のまま。本体の側は src/hack_box_shelf.c の BoxShelf_Commit が
//  最初のレポートで 10 番・26 番（s = 10）を必ず消えていない状態にする。
#define PROBE_BANK_COUNT EXT_PROBE_BANK_COUNT   //★ 2026-10-23 B2: 一覧は割り振り表（ext_sector_map.json の probe_banks）= 2・4・8・16・32・47・64・127（127 は 6MB＝96 バンクの mGBA が 96 以上を下 1 ビットに丸めるのを見分ける）
#define PROBE_NO 0
#define PROBE_YES 1
#define PROBE_UNKNOWN 2

static bool8 HeadErased(u16 sector)
{
	u8 a[BANK_PROBE_SIZE];
	u32 i;

	ReadFlashVanilla(sector, 0, a, BANK_PROBE_SIZE);
	for (i = 0; i < BANK_PROBE_SIZE; ++i)
		if (a[i] != 0xFF)
			return FALSE;
	return TRUE;
}

bool8 HackExtSave_BackHalfVisible(void)
{
	const u32* head = (const u32*) 0x0A000000;

	return head[0] == 0x50534B50 && head[1] == 0x31534349;   //'PKSPICS1'（64MB の後ろ半分が見える）
}

//本体のバンク 0・1 の区画（0〜31）の頭 16 バイトが全部 0xFF（まだ書いていない）か。バンクの切り替えは使わない
bool8 HackExtSave_BaseHeadErased(u8 sector)
{
	return sector < 32 && HeadErased(sector);
}

//0 = 使えない（切り替えが効かない）・1 = 使える・2 = 決められない（比べる相手の本体の区画がまだ無い＝最初のレポートの前）
u8 HackExtSave_Check(void)
{
	static const u8 banks[PROBE_BANK_COUNT] = EXT_PROBE_BANK_LIST;
	bool8 candidate = FALSE;
	u32 k, i;

	if (!HackExtSave_BackHalfVisible())
		return PROBE_NO;
	for (k = 0; k < 16; ++k)
	{
		u16 s = (k + 10) & 15;   //10〜15（棚）→ 0〜9

		if (HeadErased(s) || HeadErased(16 + s))
			continue;
		candidate = TRUE;
		for (i = 0; i < PROBE_BANK_COUNT; ++i)
		{
			u16 t = banks[i] * 16 + s;

			if (!ProbeDiffers(s, t) || !ProbeDiffers(16 + s, t))
				break;
		}
		if (i == PROBE_BANK_COUNT)
			return PROBE_YES;
	}
	return candidate ? PROBE_NO : PROBE_UNKNOWN;
}

bool8 HackExtSave_Available(void)
{
	return HackExtSave_Check() == PROBE_YES;
}

bool8 HackExtSave_ReadSector(u16 n, void* dst)
{
	if (n >= EXT_SECTOR_COUNT || dst == NULL || !HackExtSave_Available())
		return FALSE;
	HackSaveCrypt_ReadFlash(EXT_FIRST_SECTOR + n, 0, dst, EXT_SECTOR_SIZE);   //★ 2026-10-09 かき混ぜを戻して読む（src/hack_save_crypt.c）
	return TRUE;
}

bool8 HackExtSave_ReadSectorChecked(u16 n, void* dst)
{
	if (n >= EXT_SECTOR_COUNT || dst == NULL || !HackExtSave_Available())
		return FALSE;
	HackSaveCrypt_ReadFlashChecked(EXT_FIRST_SECTOR + n, dst);
	return TRUE;
}

//★ 2026-10-24 patches/0740: 呼び元が「追加の領域は使える」と覚えている（HackExtSave_Available が TRUE と分かっている）ときの読み。
//  HackExtSave_Read と同じ範囲の確かめのあと、バンクの確かめ（約 34 回のフラッシュ読み）を飛ばして読む。使えるかは呼び元が責任を持つ
bool8 HackExtSave_ReadKnownAvail(u16 n, u32 off, void* dst, u32 size)
{
	if (n >= EXT_SECTOR_COUNT || dst == NULL || off > EXT_SECTOR_SIZE || size > EXT_SECTOR_SIZE - off)
		return FALSE;
	HackSaveCrypt_ReadFlash(EXT_FIRST_SECTOR + n, off, dst, size);
	return TRUE;
}

//区画 n の off から size バイト（ボックスの 40〜50 箱・src/hack_box_shelf.c）
bool8 HackExtSave_Read(u16 n, u32 off, void* dst, u32 size)
{
	if (n >= EXT_SECTOR_COUNT || dst == NULL || off > EXT_SECTOR_SIZE
	 || size > EXT_SECTOR_SIZE - off || !HackExtSave_Available())
		return FALSE;
	HackSaveCrypt_ReadFlash(EXT_FIRST_SECTOR + n, off, dst, size);
	return TRUE;
}

//★ 2026-10-23 FEAT-save16-A2 配置版の印（追加の区画 EXT_LAYOUT_MARK_SECTOR の先頭 16 バイト・割り振りの正本 tools/save_layout/ext_sector_map.json）:
//  'S' 'L' 'V' 0x00・u16 版・u16 版の補数（版 ^ 0xFFFF）。印が無い・壊れている・追加の領域が使えない = 配置版 1（今までの配置）。
//  読むだけ（書くのは B2 以降の新しい配置のとき・今の配置の挙動は変えない）。返す版が EXT_LAYOUT_VERSION_CAP より大きければ、このゲームが知らない配置
u8 HackExtSave_LayoutVersion(void)
{
	u8 mark[EXT_LAYOUT_MARK_SIZE];
	u16 version, check;

	if (!HackExtSave_Read(EXT_LAYOUT_MARK_SECTOR, 0, mark, EXT_LAYOUT_MARK_SIZE))
		return EXT_LAYOUT_DEFAULT_VERSION;
	if (mark[0] != 'S' || mark[1] != 'L' || mark[2] != 'V' || mark[3] != 0)
		return EXT_LAYOUT_DEFAULT_VERSION;
	version = mark[4] | (mark[5] << 8);
	check = mark[6] | (mark[7] << 8);
	if (version < 1 || version > 255 || check != (u16) (version ^ 0xFFFF))
		return EXT_LAYOUT_DEFAULT_VERSION;
	return (u8) version;
}

//有効な印（形が合う）があるか（版 1 の印と「印が無い」を分ける）
bool8 HackExtSave_LayoutMarkValid(void)
{
	u8 mark[EXT_LAYOUT_MARK_SIZE];
	u16 version;

	if (!HackExtSave_Read(EXT_LAYOUT_MARK_SECTOR, 0, mark, EXT_LAYOUT_MARK_SIZE))
		return FALSE;
	version = mark[4] | (mark[5] << 8);
	return mark[0] == 'S' && mark[1] == 'L' && mark[2] == 'V' && mark[3] == 0 && version >= 1 && version <= 255
	    && (u16) (mark[6] | (mark[7] << 8)) == (u16) (version ^ 0xFFFF);
}

//★ 2026-10-23 B2 旧配置（配置版 1）のスロット 2 の本体（フラッシュ区画 EXT_LEGACY_SLOT2_FLASH_FIRST〜+COUNT-1）に中身があるか。
//  あるセーブは未移行（移行ツール B3 の入力）＝配置版の印を書かない。ぼうけんのしょ 2 の側で呼ぶと +SLOT2_BANK_OFFSET のバンクを読む（空）
static bool8 LegacySlot2Present(void)
{
	u16 s;

	for (s = EXT_LEGACY_SLOT2_FLASH_FIRST; s < EXT_LEGACY_SLOT2_FLASH_FIRST + EXT_LEGACY_SLOT2_FLASH_COUNT; ++s)
		if (!HeadErased(s))
			return TRUE;
	return FALSE;
}

//★ 2026-10-23 B3: ぼうけんのしょ 1 の側（SetSlot(0)）で呼ぶ。旧配置（6MB）のぼうけんのしょ 2 に中身が残っているか（＝セーブの変換が要る）。追加の領域が使えないときは FALSE
//  （使えない版の mGBA は 48 バンクを丸めて本体と同じ中身が読めてしまい、誤って「ある」と答えるため）
bool8 HackExtSave_LegacySlot2Present(void)
{
	return HackExtSave_Available() && LegacySlot2Present();
}

static bool8 WriteSectorRaw(u16 n, const void* src);

//印が無く（読めない・壊れ）、旧配置のスロット 2 にも中身が無いときだけ、印（配置版 EXT_LAYOUT_VERSION_CAP）を書く。書けなくても本来の書き込みは続ける
static void StampLayoutMark(void)
{
	u8* sector;
	u32 i;

	if (HackExtSave_LayoutMarkValid() || LegacySlot2Present())
		return;
	sector = Malloc(EXT_SECTOR_SIZE);
	if (sector == NULL)
		return;
	for (i = 0; i < EXT_SECTOR_SIZE; ++i)
		sector[i] = 0xFF;
	sector[0] = 'S';
	sector[1] = 'L';
	sector[2] = 'V';
	sector[3] = 0;
	sector[4] = EXT_LAYOUT_VERSION_CAP;
	sector[5] = 0;
	sector[6] = (u8) (EXT_LAYOUT_VERSION_CAP ^ 0xFF);
	sector[7] = 0xFF;
	WriteSectorRaw(EXT_LAYOUT_MARK_SECTOR, sector);
	Free(sector);
}

bool8 HackExtSave_WriteSector(u16 n, const void* src)
{
	if (n != EXT_LAYOUT_MARK_SECTOR && n < EXT_SECTOR_COUNT && HackExtSave_Available())
		StampLayoutMark();
	return WriteSectorRaw(n, src);
}

static bool8 WriteSectorRaw(u16 n, const void* src)
{
	const u8* old = gFlashPtr;
	u8* copy;
	u32 status;
	u32 i;

	if (n >= EXT_SECTOR_COUNT || src == NULL || EXT_FIRST_SECTOR + n == EXT_RTC_GUARD_FLASH_SECTOR
	 || !HackExtSave_Available() || old == NULL || *(const u32*) old != 0x20000)
		return FALSE;   //1MB（128KB）のフラッシュのときだけ・フラッシュの区画 EXT_RTC_GUARD_FLASH_SECTOR（6MB の位置・古い mGBA の RTC 末尾）の頭は書かない
	copy = Malloc(FLASHTYPE_COPY_SIZE);
	if (copy == NULL)
		return FALSE;
	for (i = 0; i < FLASHTYPE_COPY_SIZE; ++i)
		copy[i] = old[i];
	*(u16*) (copy + 0xA) = EXT_FIRST_SECTOR + EXT_SECTOR_COUNT;
	gFlashPtr = copy;
	status = ProgramFlashSectorAndVerifyVanilla(EXT_FIRST_SECTOR + n, (u8*) src);
	gFlashPtr = old;
	Free(copy);
	return status == 0;
}

#ifndef HACK_DEMO_BUILD
//確かめ用（callasm）: 追加の区画 1 と 735 に模様を書いて読み戻す → 0x8004 = 1 なら書けて読めた・0 は使えない版・2 は食い違い・3 は入れ物が取れない
void HackExtSave_DebugTest(void)
{
	u32* buf = Malloc(EXT_SECTOR_SIZE);
	u32 i, k;
	u16 sectors[2] = {EXT_DEBUG_SECTOR_A, EXT_DEBUG_SECTOR_B};   //0 は src/hack_save_slot.c の確かめ用

	Var8004 = 0;
	if (buf == NULL)
	{
		Var8004 = 3;
		return;
	}
	if (!HackExtSave_Available())
	{
		Free(buf);
		return;
	}
	Var8004 = 1;
	Var8005 = HackExtSave_LayoutVersion();   //確かめ用（デバッグ版の callasm）: 配置版の印の読み（印が無ければ 1）
	for (k = 0; k < 2; ++k)
	{
		for (i = 0; i < EXT_SECTOR_SIZE / 4; ++i)
			buf[i] = 0x45585400 + sectors[k] * 0x10000 + i;   //'EXT'
		if (!HackExtSave_WriteSector(sectors[k], buf))
			Var8004 = 2;
		for (i = 0; i < EXT_SECTOR_SIZE / 4; ++i)
			buf[i] = 0;
		HackExtSave_ReadSector(sectors[k], buf);
		for (i = 0; i < EXT_SECTOR_SIZE / 4; ++i)
			if (buf[i] != 0x45585400 + sectors[k] * 0x10000 + i)
				Var8004 = 2;
	}
	Free(buf);
}
#endif
