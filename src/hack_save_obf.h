#pragma once

/*
    セーブ難読化 段階 3（版ごとの鍵）＋ 改造の検出（区画ごとの MAC）の中核。docs/research/2026-10-21_save-obfuscation.md

    ★ ROM のものを呼ばない（フラッシュの読み書きはマクロ OBF_FLASH_* で外から渡す）＝ホストの clang でそのまま組んで
      Python／JS と突き合わせられる（tools/save_editor/verify_save_obf.py）。書き換え可能な static は持たない（6.13）。
    ★ 使う側（src/hack_save_crypt.c）が u8／u16／u32 と OBF_FLASH_READ／OBF_FLASH_PROGRAM／OBF_FLASH_VERIFY を先に定義してから読む。

    区画（4KB）の持ち方（フラッシュの上の姿）:
      [0x000,0xFF0) 中身（難読化の対象）   [0xFF0,0xFF4) 区画の印（鍵番号 8 ビット・鍵の確かめ 8 ビット・MAC 16 ビット）
      [0xFF4,0x1000) 足跡（id・checksum・署名・counter）。難読化しない
    外側にこれまでの「位置の鍵 XOR」(T・Obf_TKey) を区画 4KB 全体にかける（前の版の ROM が読む姿を保つため）。
      書く: 中身 ─MAC（平文の中身から）→ V（版の鍵・加算）→ 印を書く ─T→ フラッシュ     読む: フラッシュ ─T→ 印を見る ─V⁻¹→ 中身 → （検算 checksum が合えば）MAC

    V は 4 バイトの語ごとの加算（w' = w + k）。k は「検算の和を変えない」ように作る（素の CalculateSaveChecksum は u32 の語の和なので、
    k の和が 0 なら 前の版の ROM でも checksum が合う＝読み込みに成功して 版の変数 0x50EF の確認（「ふるい／あたらしい バージョン」の止め画面）まで届く）。
      r(j) = Mix(seed, nonce, j)   nonce = 足跡の counter
      checksum の範囲（span）の語 i: k = r(i) − r(i+1)（最後の語は r(最後) − r(0)）＝ 和は 0
      それより後ろの語: k = r(0x400 + i)

    MAC（改造の検出）: HalfSipHash-2-4（32 ビットの語で動く SipHash・出力 32 ビット）。鍵 64 ビット = (版の seed, 台帳の mac_secret)。
      入力 = 平文の中身 0xFF0 バイト ＋ 語 (id | 鍵番号 << 16) ＋ 語 counter（1022 語＝4088 バイト）。出力の上位 16 ビットを印に入れる。
      検算の順序: 先に checksum（平文の中身の u32 の和）を見て、合わない区画は「壊れた区画」（電源断・書き損じ）として MAC を見ない。
      checksum が合ったのに MAC が合わない区画だけが「改造」。印が 0（前の版・まだ署名していない）の区画は「署名なし」＝改造としない。
*/

#define OBF_SECTOR 0x1000
#define OBF_PAYLOAD 0xFF0
#define OBF_OFF_TAG 0xFF0
#define OBF_OFF_ID 0xFF4
#define OBF_OFF_CHK 0xFF6
#define OBF_OFF_SIG 0xFF8
#define OBF_OFF_CTR 0xFFC
#define OBF_FILE_SIG 0x08012025u
#define OBF_T_SEED 0x504B5354u
#define OBF_UNSIGNED_MARK 0x47534E55u   /* 検算つきの読みで 署名のない区画（前の版が書いた T だけ／T も無い）だった区画の +0xFF0（棚のページを書き直す印・普段そこは 0） */
/* H1（古い ROM を確実に止める）: 本体の組 id 2 の 語 0x104（SaveBlock1 + 0x1400 = 棚の対応表の magic）の V の鍵を定数にして、平文の 'SHL3' が保存の上では 'SHEL' になるようにする。
   0.50.0〜0.57.x の ROM は 'SHEL' を「古い形の棚」と読んで止める画面を出す（BoxShelf_OnLoad の oldFormat）。差し引きは 最後の語の鍵で合わせる＝和は 0 のまま */
#define OBF_H1_ID 2
#define OBF_H1_WORD 0x104u
#define OBF_H1_K 0x18F90000u    /* 'SHEL'(0x4C454853) − 'SHL3'(0x334C4853) */
#define OBF_H1_K_X 0x17F90000u  /* 'SHEL' − 'SHL4'(0x344C4853)。箱の形式 X（78 バイト・patches/0680）の棚も 古い ROM から見て 'SHEL'（止まる印）にする。鍵の行が 0.59.2 以降のものだけ（X を書くのは 0.59.2 以降の ROM） */
#define OBF_H1_X_FROM 0x3B02u
#define OBF_H1_KOF(ver) (((ver) >= OBF_H1_X_FROM) ? OBF_H1_K_X : OBF_H1_K)   /* 鍵の行の版 → 語 0x104 の鍵 */
#define OBF_TAMPER_MARK 0x504D4154u     /* 検算つきの読みで MAC が合わなかった区画の +0xFF0（呼んだ側が見る・印は普段 0 に見える） */

struct HackSaveKey
{
	u16 version;     /* hack_save_version.h と同じ数え方（更新 * 256 + バグ修正）。表の位置（1 始まり）が区画の印の鍵番号 */
	u16 pad;
	u32 seed;
};

#ifndef HACK_SAVE_KEYS_EXTERNAL     /* ホストの試験は自前の表（sHackSaveKeys・HACK_SAVE_KEY_COUNT・HACK_SAVE_MAC_SECRET）を先に定義してこれを立てる */
#include "hack_save_keys.h"         /* 生成物（tools/build/build_save_keys.py） */
#endif
#define OBF_MAC_KEY1 HACK_SAVE_MAC_SECRET     /* MAC の鍵の後ろ半分（全版共通・台帳の mac_secret）*/

/* ---- 外側 T（前の版から変わらない位置の鍵） ---- */
static u8 Obf_TKey(u32 i)
{
	u32 x = i * 0x9E3779B1u + OBF_T_SEED;
	x ^= x >> 15;
	x *= 0x2C1B3C6Du;
	x ^= x >> 12;
	x *= 0x297A2D39u;
	x ^= x >> 15;
	return (u8) (x >> 24);
}

static void Obf_TXor(u8* p, u32 off, u32 n)
{
	u32 i;
	for (i = 0; i < n; ++i)
		p[i] ^= Obf_TKey(off + i);
}

/* ---- 内側 V（版の鍵） ---- */
static u32 Obf_Mix(u32 seed, u32 nonce, u32 j)
{
	u32 x = seed ^ (nonce * 0x85EBCA6Bu);
	x += j * 0x9E3779B1u;
	x ^= x >> 16;
	x *= 0x7FEB352Du;
	x ^= x >> 15;
	x *= 0x846CA68Bu;
	x ^= x >> 16;
	return x;
}

/* 鍵の確かめ（8 ビット）: 鍵を取り違えた区画・足跡（id・checksum・counter）が化けた区画を「改造」と数えないための印。
   V は和を保つので、nonce（counter）が化けても checksum は合ってしまう＝足跡ごと確かめる（電源断で足跡の終わりだけ書けなかった区画の守り）*/
static u32 Obf_KeyCheck(u32 seed, u32 nonce, u32 id, u32 chk)
{
	return Obf_Mix(seed, nonce, 0xA17F0001u ^ id ^ (chk << 16)) >> 24;
}

static u32 Obf_Ld32(const u8* p)
{
	return (u32) p[0] | ((u32) p[1] << 8) | ((u32) p[2] << 16) | ((u32) p[3] << 24);
}

static void Obf_St32(u8* p, u32 v)
{
	p[0] = (u8) v;
	p[1] = (u8) (v >> 8);
	p[2] = (u8) (v >> 16);
	p[3] = (u8) (v >> 24);
}

/* 素の gSaveSectionOffsets（src/save.c）の checksum の範囲。id 0 = SaveBlock2、4 = SaveBlock1 の最後。棚のページ（id 0x40〜）・追加の区画は 0xFF0 */
static u32 Obf_SpanWords(u32 id)
{
	if (id == 0)
		return 0xF24 / 4;
	if (id == 4)
		return 0xD98 / 4;
	return OBF_PAYLOAD / 4;
}

/* 区画の中の語 first..first+n−1 に V（dir = +1）か V⁻¹（dir = −1）をかける。buf は語 first の位置（4 の倍数で並ぶ必要は無い） */
static void Obf_Words(u8* buf, u32 first, u32 n, u32 seed, u32 h1k, u32 nonce, u32 span, u32 id, int dir)
{
	u32 i, rc, rn, k;

	if (n == 0)
		return;
	rc = (first < span) ? Obf_Mix(seed, nonce, first) : 0;
	for (i = 0; i < n; ++i)
	{
		u32 w = first + i;
		if (w < span)
		{
			rn = Obf_Mix(seed, nonce, (w + 1 < span) ? w + 1 : 0);
			k = rc - rn;
			rc = rn;
			if (id == OBF_H1_ID)
			{
				if (w == OBF_H1_WORD)
					k = h1k;
				else if (w == span - 1)    /* H1 で変えた分を 最後の語で合わせる（和を 0 に保つ） */
					k = k - h1k + (Obf_Mix(seed, nonce, OBF_H1_WORD) - Obf_Mix(seed, nonce, OBF_H1_WORD + 1));
			}
		}
		else
			k = Obf_Mix(seed, nonce, 0x400u + w);
		Obf_St32(buf + i * 4, dir > 0 ? Obf_Ld32(buf + i * 4) + k : Obf_Ld32(buf + i * 4) - k);
	}
}

/* 区画 4KB 全体（足跡の id・counter を読む）。tag は触らない */
static void Obf_Sector(u8* sec, u32 seed, u32 h1k, int dir)
{
	u32 id = sec[OBF_OFF_ID] | ((u32) sec[OBF_OFF_ID + 1] << 8);
	u32 nonce = Obf_Ld32(sec + OBF_OFF_CTR);
	Obf_Words(sec, 0, OBF_PAYLOAD / 4, seed, h1k, nonce, Obf_SpanWords(id), id, dir);
}

/* ---- MAC: HalfSipHash-2-4（出力 32 ビット）。語を 1 つずつ食わせる ---- */
#define OBF_ROTL(x, b) (((x) << (b)) | ((x) >> (32 - (b))))
#define OBF_SIPROUND do { \
	v0 += v1; v1 = OBF_ROTL(v1, 5); v1 ^= v0; v0 = OBF_ROTL(v0, 16); \
	v2 += v3; v3 = OBF_ROTL(v3, 8); v3 ^= v2; \
	v0 += v3; v3 = OBF_ROTL(v3, 7); v3 ^= v0; \
	v2 += v1; v1 = OBF_ROTL(v1, 13); v1 ^= v2; v2 = OBF_ROTL(v2, 16); } while (0)

/* 平文の区画 sec（足跡の id・counter 入り）の MAC（32 ビット）。idx1 = 鍵番号（1 始まり）。入力は 1020 語 + 2 語 = 4088 バイト */
static u32 Obf_Mac(const u8* sec, u32 seed, u32 idx1)
{
	u32 v0 = seed, v1 = OBF_MAC_KEY1, v2 = 0x6c796765u ^ seed, v3 = 0x74656462u ^ OBF_MAC_KEY1;
	u32 b = (u32) ((OBF_PAYLOAD + 8) & 0xFF) << 24;
	u32 i, m;
	u32 id = sec[OBF_OFF_ID] | ((u32) sec[OBF_OFF_ID + 1] << 8);

	for (i = 0; i < OBF_PAYLOAD / 4 + 2; ++i)
	{
		if (i < OBF_PAYLOAD / 4)
			m = Obf_Ld32(sec + i * 4);
		else if (i == OBF_PAYLOAD / 4)
			m = id | (idx1 << 16);
		else
			m = Obf_Ld32(sec + OBF_OFF_CTR);
		v3 ^= m;
		OBF_SIPROUND;
		OBF_SIPROUND;
		v0 ^= m;
	}
	v3 ^= b;
	OBF_SIPROUND;
	OBF_SIPROUND;
	v0 ^= b;
	v2 ^= 0xff;
	OBF_SIPROUND;
	OBF_SIPROUND;
	OBF_SIPROUND;
	OBF_SIPROUND;
	return v1 ^ v3;
}

/* 印の 32 ビット = 鍵番号(8) | 鍵の確かめ(8) << 8 | MAC(16) << 16 */
static u32 Obf_MakeTag(u32 idx1, u32 seed, u32 nonce, u32 id, u32 chk, u32 mac32)
{
	return idx1 | (Obf_KeyCheck(seed, nonce, id, chk) << 8) | ((mac32 >> 16) << 16);
}

/* 平文の中身の u32 の和を折り畳んだ checksum（素の CalculateSaveChecksum と同じ）*/
static u32 Obf_Checksum(const u8* sec, u32 bytes)
{
	u32 s = 0, i;
	for (i = 0; i + 4 <= bytes; i += 4)
		s += Obf_Ld32(sec + i);
	return ((s >> 16) + s) & 0xFFFFu;
}

enum { OBF_PLAIN = 0, OBF_LEGACY = 1, OBF_V = 2, OBF_UNREADABLE = 3 };

/*
   区画の足跡（16 バイト・+0xFF0〜）を T で戻したもの tail から、この区画の姿を決める。
     OBF_PLAIN       署名がそのまま読めた（T も V も無い・前の前の版）
     OBF_LEGACY      T だけ（前の版・または署名が無い区画＝消えたまま・試験用）＝署名なし
     OBF_V           T ＋ 版の鍵（*key に入る・*idx1 に鍵番号）
     OBF_UNREADABLE  署名はあるのに印が合わない（この ROM が鍵を持たない新しい版・同じ版で鍵が違う試験ビルド・印の化け）
*/
static int Obf_Classify(const u8* rawTail, u8* tail, const struct HackSaveKey** key, u32* idx1)
{
	u32 i, tag;

	if (Obf_Ld32(rawTail + 8) == OBF_FILE_SIG)
		return OBF_PLAIN;
	for (i = 0; i < 16; ++i)
		tail[i] = rawTail[i] ^ Obf_TKey(OBF_OFF_TAG + i);
	if (Obf_Ld32(tail + 8) != OBF_FILE_SIG)
		return OBF_LEGACY;
	tag = Obf_Ld32(tail);
	if (tag == 0)
		return OBF_LEGACY;
	*idx1 = tag & 0xFFu;
	if (*idx1 == 0 || *idx1 > HACK_SAVE_KEY_COUNT)
		return OBF_UNREADABLE;
	*key = &sHackSaveKeys[*idx1 - 1];
	if (((tag >> 8) & 0xFFu) != Obf_KeyCheck((*key)->seed, Obf_Ld32(tail + 12), tail[4] | ((u32) tail[5] << 8), tail[6] | ((u32) tail[7] << 8)))
		return OBF_UNREADABLE;
	return OBF_V;
}

/*
   区画 sector の off から n バイトを戻して dst へ（HackSaveCrypt_ReadFlash の中身）。
   verify があり、区画全体（off 0・n 4096）を V の姿で読んだときだけ MAC を確かめ、checksum が合っているのに MAC が合わなければ
   dst の +0xFF0 に OBF_TAMPER_MARK を置く（普段そこは 0）。呼んだ側が見る。何も止めない・何も書き換えない。
*/
static void Obf_ReadEx(u16 sector, u32 off, void* dstv, u32 n, int verify)
{
	u8* dst = (u8*) dstv;
	u8 raw[16], tail[16];
	const struct HackSaveKey* key = 0;
	u32 idx1 = 0;
	int mode;
	u32 i;

	OBF_FLASH_READ(sector, OBF_OFF_TAG, raw, 16);
	mode = Obf_Classify(raw, tail, &key, &idx1);
	if (mode == OBF_PLAIN)
	{
		OBF_FLASH_READ(sector, off, dst, n);
		if (verify && off == 0 && n == OBF_SECTOR)
			Obf_St32(dst + OBF_OFF_TAG, OBF_UNSIGNED_MARK);
		return;
	}
	if (mode != OBF_V)
	{
		OBF_FLASH_READ(sector, off, dst, n);
		Obf_TXor(dst, off, n);
		if (mode == OBF_LEGACY && verify && off == 0 && n == OBF_SECTOR && Obf_Ld32(dst + OBF_OFF_SIG) == OBF_FILE_SIG)
			Obf_St32(dst + OBF_OFF_TAG, OBF_UNSIGNED_MARK);
		if (mode == OBF_UNREADABLE && off <= OBF_OFF_SIG && off + n >= OBF_OFF_SIG + 4)
		{
			for (i = 0; i < 4; ++i)       /* 署名を潰して 呼んだ側に「無い区画」と見せる（鍵が無いものを平文として読ませない） */
				dst[OBF_OFF_SIG - off + i] = 0;
		}
		return;
	}
	{
		u32 nonce = Obf_Ld32(tail + 12), id = tail[4] | ((u32) tail[5] << 8), span = Obf_SpanWords(id);
		u32 pos = off & ~3u, end = off + n;       /* 4 バイトずつ・32 語ずつ読む */
		while (pos < end)
		{
			u8 tmp[128];
			u32 len = end - pos, a, b;
			if (len > sizeof(tmp))
				len = sizeof(tmp);
			len = (len + 3) & ~3u;
			OBF_FLASH_READ(sector, pos, tmp, len);
			Obf_TXor(tmp, pos, len);
			if (pos < OBF_PAYLOAD)
			{
				u32 pe = pos + len;
				if (pe > OBF_PAYLOAD)
					pe = OBF_PAYLOAD;
				Obf_Words(tmp, pos / 4, (pe - pos) / 4, key->seed, OBF_H1_KOF(key->version), nonce, span, id, -1);
			}
			if (pos <= OBF_OFF_TAG && pos + len > OBF_OFF_TAG)           /* 印は呼んだ側に見せない（0 に） */
			{
				for (i = 0; i < 4; ++i)
					tmp[OBF_OFF_TAG - pos + i] = 0;
			}
			a = (pos > off) ? pos : off;
			b = (pos + len < end) ? pos + len : end;
			for (i = a; i < b; ++i)
				dst[i - off] = tmp[i - pos];
			pos += len;
		}
		if (verify && off == 0 && n == OBF_SECTOR)
		{
			u32 stored = Obf_Ld32(tail), chk = tail[6] | ((u32) tail[7] << 8);
			if (Obf_Checksum(dst, span * 4) == chk                         /* 壊れた区画（電源断・書き損じ）は MAC を見ない */
			 && (Obf_Mac(dst, key->seed, idx1) >> 16) != (stored >> 16))
				Obf_St32(dst + OBF_OFF_TAG, OBF_TAMPER_MARK);
		}
	}
}

static void Obf_Read(u16 sector, u32 off, void* dstv, u32 n)
{
	Obf_ReadEx(sector, off, dstv, n, 0);
}

static int Obf_IsTampered(const u8* sec)
{
	return Obf_Ld32(sec + OBF_OFF_TAG) == OBF_TAMPER_MARK;
}

static int Obf_IsUnsigned(const u8* sec)
{
	return Obf_Ld32(sec + OBF_OFF_TAG) == OBF_UNSIGNED_MARK;
}

/* 区画 sector へ src（4KB・足跡が入っている）を書く（HackSaveCrypt_ProgramFlashSectorAndVerify の中身）。src は書いたあと元に戻す。0 なら書けた */
static u32 Obf_Write(u16 sector, u8* src)
{
	u32 result = 0, tries;
	u8 saved[4];
	int vee = 0;
	const struct HackSaveKey* cur = &sHackSaveKeys[HACK_SAVE_KEY_COUNT - 1];
	u32 nonce = Obf_Ld32(src + OBF_OFF_CTR);

	if (Obf_Ld32(src + OBF_OFF_SIG) == OBF_FILE_SIG)
		vee = 1;       /* 署名のある区画だけ（棚のページ・本体の組・追加の区画）。署名の無いもの（試験用）は T だけ */
	if (vee)
	{
		u32 i, mac = Obf_Mac(src, cur->seed, HACK_SAVE_KEY_COUNT);     /* 平文から（V の前）*/
		for (i = 0; i < 4; ++i)
			saved[i] = src[OBF_OFF_TAG + i];
		Obf_Sector(src, cur->seed, OBF_H1_KOF(cur->version), +1);
		Obf_St32(src + OBF_OFF_TAG, Obf_MakeTag(HACK_SAVE_KEY_COUNT, cur->seed, nonce, src[OBF_OFF_ID] | ((u32) src[OBF_OFF_ID + 1] << 8), src[OBF_OFF_CHK] | ((u32) src[OBF_OFF_CHK + 1] << 8), mac));
	}
	Obf_TXor(src, 0, OBF_SECTOR);
	for (tries = 0; tries < 3; ++tries)
	{
		result = OBF_FLASH_PROGRAM(sector, src);
		if (result != 0)
			continue;
		result = OBF_FLASH_VERIFY(sector, src);
		if (result == 0)
			break;
	}
	Obf_TXor(src, 0, OBF_SECTOR);
	if (vee)
	{
		u32 i;
		Obf_Sector(src, cur->seed, OBF_H1_KOF(cur->version), -1);
		for (i = 0; i < 4; ++i)
			src[OBF_OFF_TAG + i] = saved[i];
	}
	return result;
}

/* 署名のある 前の版の区画（T だけ・印 0・署名あり）で、id が fillerId でないもの（＝箱などの中身を持つ古い写し）か。棚の「だれも指していない古い写し」を消す対象を決める */
static int Obf_IsLegacyData(u16 sector, u32 fillerId)
{
	u8 raw[16], tail[16];
	u32 i;

	OBF_FLASH_READ(sector, OBF_OFF_TAG, raw, 16);
	if (Obf_Ld32(raw + 8) == OBF_FILE_SIG)
		return (raw[4] | ((u32) raw[5] << 8)) != fillerId;     /* T も無い前の前の版 */
	for (i = 0; i < 16; ++i)
		tail[i] = raw[i] ^ Obf_TKey(OBF_OFF_TAG + i);
	return Obf_Ld32(tail + 8) == OBF_FILE_SIG && Obf_Ld32(tail) == 0 && (u32) (tail[4] | ((u32) tail[5] << 8)) != fillerId;
}

/* 本体の組（first から 5 区画）に 署名のない前の版の区画が 1 つでもあるか（前の版の最後の写し＝控えの組を 署名つきで書き直すかの判断） */
static int Obf_SlotLegacy(u16 first)
{
	u16 i;
	for (i = 0; i < 5; ++i)
	{
		if (Obf_IsLegacyData(first + i, 0xFFFFu))
			return 1;
	}
	return 0;
}

/*
   メニューの確かめ: 本体の組（区画 0〜9・今のぼうけんのしょ）に、この ROM が読めない鍵の区画が 1 つでもあるか。
   あれば TRUE（新しい版のレポートがある＝古い写しを読んで進めない・「あたらしい バージョン」の止め画面を出す）
*/
static int Obf_NewerKeyPresent(void)
{
	u16 s;
	for (s = 0; s < 10; ++s)
	{
		u8 raw[16], tail[16];
		const struct HackSaveKey* key = 0;
		u32 idx1 = 0;
		OBF_FLASH_READ(s, OBF_OFF_TAG, raw, 16);
		if (Obf_Classify(raw, tail, &key, &idx1) == OBF_UNREADABLE)
			return 1;
	}
	return 0;
}
