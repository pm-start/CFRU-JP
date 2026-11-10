#include "defines.h"
#include "../include/malloc.h"
#include "../include/decompress.h"
#include "../include/sprite.h"

/*
 * ROM の後ろ半分（0x0A000000〜）に置いたデータを読む（2026-10-07・docs/research/2026-10-07_rom-64mb.md）。
 * ユーザー「今のポケモンの画像全部そっちの領域において」「正面と後ろ姿だけ」「新しいmGBAとSuperCardに対応」
 * → 2026-10-11「64MB の後ろ半分を使い始めよう」: CFRU のデータ（地図のタイルセットの絵・縮めたマス目の LZ・連れ歩きのシート・
 *   Moémon の連れ歩き）も同じ道へ（tools/build/split_back.py・docs/research/2026-10-11_back-half-general.md）。
 *
 *   build_rom.py --split-pics で組むと、後ろ半分のかたまり（見出し "PKSPICS1"）は CFRU の塊（build/split_cfru.bin）＋ DPE の
 *   正面・後ろ姿の絵。各塊は [u32 長さ][データ] で、表やラベルはデータの頭（0x0A000000 + 位置）を指す。
 *   - 直した mGBA（tools/mgba・64MB の ROM）: 0x0A000000 に見出しが見える → そのまま読める（Native）
 *   - SuperCard: 0x0A000000 は ROM の頭の写し → SD の PKSPICS.BIN の同じ位置から、長さぶんの区画を読む
 *   - どちらも無い（素の mGBA・ほかのカートリッジ）: 読めない（絵が出ない）
 *   LZ77 の展開の入口（assembly/fcomp/lz_dispatch.s）は、元が 0x0A000000〜0x0BFFFFFF ならここ（HackSplitPics_Decompress）へ来る。
 *   ★ 5e の指摘 2026-10-11: 素の処理には、入口より前に「展開後の大きさ」を元のポインタから直に読んで Alloc するものがある
 *     （MallocAndDecompress 080F7AF8＝地図のタイルセット・LoadCompressedSpriteSheetUsingHeap 0800E9E8＝Moémon の連れ歩き・ボール）。
 *     SuperCard では 0x0A… は 32MB の鏡なのでゴミを読む → functionrewrites でこの 2 つを置き換え、後ろ半分なら先に写しを取ってから
 *     素と同じ処理をする（下の HackSplit_MallocAndDecompress・HackSplit_LoadCompressedSpriteSheetUsingHeap）。
 *   入口を通らずに直に読むもの（連れ歩きのシート＝src/bw_follower_sheet.c）は HackSplit_Fetch で RAM の写しをもらう。
 *   ヒープが無いとき（SuperCard は 7KB しか無いことがある）は gDecompressionBuffer（0x0201C000・0x4000）を借りる。
 *   ふつうのビルドでは 0x0A000000〜 を指すデータは無いので、ここには来ない。
 */

#define SPLIT_BASE 0x0A000000
#define SPLIT_FILE_83   "PKSPICS BIN"
#define SPLIT_FILE_LONG "PKSPICS.BIN"
#define SPLIT_MAX_READ  0x8000      //1 回に読む上限（ヒープは 0x1AE00・連れ歩きの 1 シートは 3KB・タイルセットは 6KB・マス目は 4.2KB まで）
#define FALLBACK_BUF      ((u8 *) 0x0201C000)   //gDecompressionBuffer（絵の展開の一時置き場＝地図の読み込み中は空いている）
#define FALLBACK_BUF_SIZE 0x4000

//---- 上位 64MB（バンク 2・3＝W・2026-10-23 FEAT-rom128-C2・docs/research/2026-10-23_rom128-c2.md）----
//128MB の ROM は 32MB のバンク 4 つ。バンク 0＝前半（0x08000000〜・いつも見える）・窓（0x0A000000〜0x0BFFFFFF）に見えるバンクを書き込みで選ぶ
//（1＝既定＝今の後ろ半分 B・2・3＝W）。W を指す番地は仮想の 0x10000000 + w（w = (バンク - 2) * 32MB + バンクの中の位置・バスには出ない
//＝直に読むとゴミ・必ず入口 HackExt_Read／HackSplit_Fetch… を通る）。各バンクの先頭 8 バイトは印 "PKSWBNK2"／"PKSWBNK3"。
//窓を切り替える書き込み: 0x0B7FFF00 へ 16 ビットを 4 回（0xB128・0x5A0E・n・~n）。窓の中のコードは実行しない・割り込みが読む物（曲など）は B に置く。
#define EXT_W_BASE        0x10000000
#define EXT_BANK_SIZE     0x02000000
#define EXT_WIN_BASE      0x0A000000
#define EXT_WIN_REG       ((vu16 *) 0x0B7FFF00)
#define EXT_REG_IME       (*(vu16 *) 0x04000208)
#define EXT_READW_CHUNK   0x1000    //1 回で IME を止めて写す量（4KB。窓を戻してから次を写す＝割り込みを止める時間を短く）

void *HackScsd_FileOpen(const char *name83, const char *longName);
bool8 HackScsd_FileRead(void *fs, u32 sector, u32 n, void *out);
void HackScsd_FileClose(void *fs);

bool8 HackSplit_Native(void)
{
	const u8 *p = (const u8 *) SPLIT_BASE;

	return p[0] == 'P' && p[1] == 'K' && p[2] == 'S' && p[3] == 'P' && p[4] == 'I' && p[5] == 'C' && p[6] == 'S' && p[7] == '1';
}

//★ 2026-10-23 FIX-rom128-oldmgba-notice: 128MB の ROM（-DHACK_ROM128）を、バンク 2・3 の窓が無い mGBA（古い mGBA）で開いたか。
//  窓 2 の先頭の印 "PKSWBNK2" が読めない場合だけ TRUE。ただし SuperCard の ROM（_split.gba）は同じコードで窓が無いのが正常なので除く
//  （SuperCard の ROM だけ build_rom.py が曲の窓の表 gHackMusTable の count を書く＝hack_music_window.c と同じ見分け方）。
//  古い mGBA は 128MB の ROM の 0x0A000000 に後ろ半分を出さず前半の写しが見える（Native は偽）ので、Native では見分けられない。
//  64MB の ROM（HACK_ROM128 なし）・新しい mGBA では FALSE。ぼうけんのしょを選ぶ画面（hack_save_slot.c）が最初に 1 回だけ呼び、TRUE なら案内で止める
extern const volatile u32 gHackMusTable[];   //src/hack_music_window.c（[0] = magic・[1] = stamp・[2] = count）
bool8 HackExt_ReadW(u8 bank, u32 off, void *dst, u32 len);

bool8 HackSplit_OldMgbaRom128(void)
{
#ifdef HACK_ROM128
	u32 head[1];

	return gHackMusTable[2] == 0 && !HackExt_ReadW(2, 0, head, 4);
#else
	return FALSE;
#endif
}

//B（0x0A000000〜0x0BFFFFFF）か
static bool8 IsB(const void *src)
{
	return ((u32) src >> 25) == (SPLIT_BASE >> 25);
}

//W（仮想 0x10000000〜0x13FFFFFF）か
static bool8 IsW(const void *src)
{
	return ((u32) src >> 26) == (EXT_W_BASE >> 26);
}

//後ろ（B か W）にある物か。lz_dispatch.s の判定と同じ
bool8 HackSplit_IsBack(const void *src)
{
	return IsB(src) || IsW(src);
}

//src をそのまま読んでよいか（直した mGBA の B。W は窓を切り替えないと読めない＝いつも偽）
bool8 HackSplit_Direct(const void *src)
{
	return IsB(src) && HackSplit_Native();
}

static void ExtWinSelect(u16 n)
{
	vu16 *reg = EXT_WIN_REG;

	*reg = 0xB128;
	*reg = 0x5A0E;
	*reg = n;
	*reg = (u16) ~n;
}

//★ 窓の切り替え口（1 本）: バンク bank（2・3）の off から len バイトを dst（RAM）へ写す。IME を止めた中で「窓を n に→印を確かめる→写す→窓を 1 に」を
//4KB ごとに繰り返す（入れ子にしない・割り込みの中からは呼ばない）。印が違えば FALSE（64MB／古い mGBA は書き込みが無視され、窓は 1 のまま＝別の印が見える）。窓は必ず 1 に戻る
bool8 HackExt_ReadW(u8 bank, u32 off, void *dst, u32 len)
{
	u8 *d = dst;

	if (bank < 2 || bank > 3 || dst == NULL || len == 0 || off >= EXT_BANK_SIZE || len > EXT_BANK_SIZE - off)
		return FALSE;
	while (len != 0)
	{
		u32 n = len > EXT_READW_CHUNK ? EXT_READW_CHUNK : len;
		vu8 *head = (vu8 *) EXT_WIN_BASE;
		vu8 *w = (vu8 *) (EXT_WIN_BASE + off);
		u16 ime = EXT_REG_IME;
		bool8 ok;
		u32 i;

		EXT_REG_IME = 0;
		ExtWinSelect(bank);
		ok = head[0] == 'P' && head[1] == 'K' && head[2] == 'S' && head[3] == 'W'
		  && head[4] == 'B' && head[5] == 'N' && head[6] == 'K' && head[7] == (u8) ('0' + bank);
		if (ok)
		{
			if (((off | (u32) d | n) & 3) == 0)
			{
				vu32 *ws = (vu32 *) w;
				u32 *dw = (u32 *) d;

				for (i = 0; i < n / 4; ++i)
					dw[i] = ws[i];
			}
			else
				for (i = 0; i < n; ++i)
					d[i] = w[i];
		}
		ExtWinSelect(1);
		EXT_REG_IME = ime;
		if (!ok)
			return FALSE;
		d += n;
		off += n;
		len -= n;
	}
	return TRUE;
}

//W の番地 src を バンク・バンクの中の位置に分ける
static void WSplit(const void *src, u8 *bank, u32 *off)
{
	u32 w = (u32) src - EXT_W_BASE;

	*bank = 2 + (w >> 25);
	*off = w & (EXT_BANK_SIZE - 1);
}

//---- 後ろ半分の読み口（2026-10-23 FEAT-save16-A1: 読み方を ここの 3 つの小さな関数と HackExt_Read に 1 本化。
//  SD のファイルを読むのは ExtSdLen／ExtSdRange だけ。W（バンク 2・3）は 窓の HackExt_ReadW・窓が無いとき（SuperCard）は SD の PKSW2.BIN／PKSW3.BIN。
//  Native（直した mGBA）の B は 0x0A… を直に読める＝SD に触らない）----

//塊（off・4 バイト前に長さ）の長さを SD から読む。借り物の入れ物の末尾の 1 区画を使う（本体の読みより前に使い終わる）。→ 長さ（0 = 読めない）
static u32 ExtSdLen(void *fs, u32 off)
{
	u8 *tmp = FALLBACK_BUF + FALLBACK_BUF_SIZE - 512;
	u32 lenOff = off - 4;

	if (!HackScsd_FileRead(fs, lenOff / 512, 1, tmp))
		return 0;
	return tmp[lenOff % 512] | (tmp[lenOff % 512 + 1] << 8) | (tmp[lenOff % 512 + 2] << 16) | (tmp[lenOff % 512 + 3] << 24);
}

//SD の off から len バイトを含む区画を dst（cap バイト以上・区画の頭から入る）へ読む。中身は dst + off % 512
static bool8 ExtSdRange(void *fs, u32 off, u32 len, u8 *dst, u32 cap)
{
	u32 nsec = (off % 512 + len + 511) / 512;

	return nsec * 512 <= cap && HackScsd_FileRead(fs, off / 512, nsec, dst);
}

//src（B か W）の SD のファイルを開く。*off = そのファイルの中の位置（B: 0x0A000000 からの差・W: バンクの中の位置）
static void *ExtSdOpen(const void *src, u32 *off)
{
	u8 bank;

	if (IsW(src))
	{
		WSplit(src, &bank, off);
		return bank == 2 ? HackScsd_FileOpen("PKSW2   BIN", "PKSW2.BIN") : HackScsd_FileOpen("PKSW3   BIN", "PKSW3.BIN");
	}
	*off = (u32) src - SPLIT_BASE;
	return HackScsd_FileOpen(SPLIT_FILE_83, SPLIT_FILE_LONG);
}

//src の塊（4 バイト前に長さ）を窓から読んで 長さを返す（0 = 読めない）
static u32 ExtWLen(const void *src)
{
	u8 bank, b[4];
	u32 off;

	WSplit(src, &bank, &off);
	if (off < 4 || !HackExt_ReadW(bank, off - 4, b, 4))
		return 0;
	return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
}

//src（後ろ半分・窓で直に読めない物）の len バイト（0 なら 4 バイト前の長さ）を RAM の入れ物に読む。入れ物は Malloc・だめなら借り物（*owned = FALSE）。
//→ 中身の頭（NULL = 読めない）・*base = 入れ物の頭（owned なら Free する）・*outLen = 長さ
//allowFallback: 借り物の入れ物を使ってよいか（★ 5e の指摘: 展開先が gDecompressionBuffer の呼び元＝素の LoadCompressedSpriteSheet などでは、
//入力と出力が同じ入れ物で重なる → 使わない。Malloc がだめなら読まない）
static u8 *ReadEntry(const void *src, u32 len, u8 **base, bool8 *owned, u32 *outLen, bool8 allowFallback)
{
	void *fs;
	u8 *buf = NULL;
	u32 off, nsec;

	*owned = FALSE;
	*outLen = 0;
	*base = NULL;
	if (IsW(src))
	{
		//窓から（128MB の mGBA）。印が見えなければ下の SD へ
		u8 bank;
		u32 woff, wlen = len != 0 ? len : ExtWLen(src);

		if (wlen != 0 && wlen <= SPLIT_MAX_READ)
		{
			WSplit(src, &bank, &woff);
			buf = Malloc(wlen);
			if (buf != NULL)
				*owned = TRUE;
			else if (allowFallback && wlen <= FALLBACK_BUF_SIZE)
				buf = FALLBACK_BUF;
			if (buf != NULL)
			{
				if (HackExt_ReadW(bank, woff, buf, wlen))
				{
					*base = buf;
					*outLen = wlen;
					return buf;
				}
				if (*owned)
					Free(buf);
				*owned = FALSE;
				buf = NULL;
			}
		}
	}
	fs = ExtSdOpen(src, &off);
	if (fs == NULL)
		return NULL;
	if (len == 0)
		len = ExtSdLen(fs, off);
	if (len == 0 || len > SPLIT_MAX_READ)
		goto DONE;
	nsec = (off % 512 + len + 511) / 512;
	buf = Malloc(nsec * 512);
	if (buf != NULL)
		*owned = TRUE;
	else if (allowFallback && nsec * 512 <= FALLBACK_BUF_SIZE)
		buf = FALLBACK_BUF;
	else
		goto DONE;
	if (!ExtSdRange(fs, off, len, buf, nsec * 512))
	{
		if (*owned)
			Free(buf);
		*owned = FALSE;
		buf = NULL;
		goto DONE;
	}
	*base = buf;
	*outLen = len;
	buf += off % 512;
DONE:
	HackScsd_FileClose(fs);
	return buf;
}

//★ 後ろ半分の読み口（1 本）: src（0x0A… の B か 仮想 0x10… の W）から len バイトを dst にコピーする。直した mGBA の B は直に・W は窓の切り替え（HackExt_ReadW）・
//SuperCard は SD の PKSPICS.BIN／PKSW2.BIN／PKSW3.BIN から。読めなければ FALSE（dst は変えない）。src が後ろ半分でなければ FALSE。len は SPLIT_MAX_READ 以下。
//ヒープが取れないときは借り物の入れ物を使う（dst が借り物の入れ物 gDecompressionBuffer と重なる呼び元は使わない）
bool8 HackExt_Read(const void *src, void *dst, u32 len)
{
	u8 *buf, *base, *d = dst;
	bool8 owned;
	u32 got, i;

	if (!HackSplit_IsBack(src) || dst == NULL || len == 0 || len > SPLIT_MAX_READ)
		return FALSE;
	if (HackSplit_Direct(src))
	{
		const u8 *s = src;

		for (i = 0; i < len; ++i)
			d[i] = s[i];
		return TRUE;
	}
	if (IsW(src))
	{
		u8 bank;
		u32 woff;

		WSplit(src, &bank, &woff);
		if (HackExt_ReadW(bank, woff, dst, len))
			return TRUE;
	}
	buf = ReadEntry(src, len, &base, &owned, &got, TRUE);
	if (buf == NULL)
		return FALSE;
	for (i = 0; i < len; ++i)
		d[i] = buf[i];
	if (owned)
		Free(base);
	return TRUE;
}

//後ろ半分の src から長さ len の写しを RAM にもらう（len 0 なら 4 バイト前の長さ）。直した mGBA の B なら src そのもの（*alloc は NULL）。
//W・SD から読んだときは *alloc に Malloc した入れ物（呼んだ側が Free する・借り物なら NULL）
const u8 *HackSplit_Fetch(const void *src, u32 len, void **alloc)
{
	u8 *buf, *base;
	bool8 owned;
	u32 got;

	*alloc = NULL;
	if (!HackSplit_IsBack(src) || HackSplit_Direct(src))
		return src;
	buf = ReadEntry(src, len, &base, &owned, &got, TRUE);        //呼んだ側の展開先は Alloc した先か VMap（借り物と重ならない）
	if (buf == NULL)
		return NULL;
	if (owned)
		*alloc = base;
	return buf;
}

//後ろ半分の塊（4 バイト前に長さ）を丸ごと RAM にもらう。長さを知らない読み手用（Deflate のマス目など）。直した mGBA の B なら src そのもの
const u8 *HackSplit_FetchEntry(const void *src, void **alloc)
{
	return HackSplit_Fetch(src, 0, alloc);
}

//後ろ半分の塊を、呼んだ側の入れ物 dst（cap バイト・SD は区画の頭から読むので 長さ + 1023 まで要る）に読む。ヒープを使わない読み手用
//（1e の Deflate のマス目＝VMap の末尾へ）。→ 長さ（0 = 読めない）・*out は中身の頭（直した mGBA の B なら src そのもの）
u32 HackSplit_FetchEntryTo(const void *src, void *dst, u32 cap, const u8 **out)
{
	u32 off;
	u32 len;
	void *fs;

	*out = src;
	if (!HackSplit_IsBack(src))
		return 0;
	if (HackSplit_Direct(src))
		return *(const u32 *) ((u32) src - 4);
	if (IsW(src))
	{
		u8 bank;

		len = ExtWLen(src);
		WSplit(src, &bank, &off);
		if (len != 0 && len <= cap && HackExt_ReadW(bank, off, dst, len))
		{
			*out = dst;
			return len;
		}
	}
	fs = ExtSdOpen(src, &off);
	if (fs == NULL)
		return 0;
	len = ExtSdLen(fs, off);
	if (len == 0 || !ExtSdRange(fs, off, len, dst, cap))
		len = 0;
	HackScsd_FileClose(fs);
	if (len)
		*out = (const u8 *) dst + off % 512;
	return len;
}

//lz_dispatch.s から: 1 = そのまま読める（入口のふつうの道で展開する）・0 = ここで済ませた（窓・SD から読んで展開した・読めなかった）
u32 HackSplitPics_Decompress(const void *src, void *dest, u32 vram)
{
	u32 len;
	u8 *buf, *base;
	bool8 owned;

	if (HackSplit_Direct(src))
		return 1;
	//展開先が借り物の入れ物（gDecompressionBuffer）と重なる呼び元（素の LoadCompressedSpriteSheet など）では借りない
	buf = ReadEntry(src, 0, &base, &owned, &len, !(!vram && (u32) dest >= (u32) FALLBACK_BUF && (u32) dest < (u32) FALLBACK_BUF + FALLBACK_BUF_SIZE));
	if (buf == NULL)
		return 0;
	//RAM の写しを展開（入口に戻るが、元が RAM なのでここには来ない。頭が 0x46／0x47 の F もそこで振り分けられる）
	if (vram)
		LZ77UnCompVram(buf, dest);
	else
		LZ77UnCompWram(buf, dest);
	if (owned)
		Free(base);
	return 0;
}

//---- 素の処理の置き換え（functionrewrites）: 展開前に大きさを直に読むものは、後ろ半分なら先に写しを取る ----

//MallocAndDecompress 080F7AF8（pokefirered new_menu_helpers.c）: 地図のタイルセット（DecompressAndCopyTileDataToVram2）などが通る
void *HackSplit_MallocAndDecompress(const void *src, u32 *size)
{
	void *alloc;
	const u8 *s = HackSplit_FetchEntry(src, &alloc);
	void *ptr = NULL;

	if (s == NULL)
	{
		*size = 0;
		return NULL;
	}
	*size = s[1] | (s[2] << 8) | (s[3] << 16);
	ptr = Malloc(*size);
	if (ptr)
		LZ77UnCompWram(s, ptr);
	if (alloc)
		Free(alloc);
	return ptr;
}

//★ 2026-10-31 BUG-raid-max-hail（Redmine #222）: ヒープに need バイト入る空きがあるか、Alloc を呼ぶ前に見る。
//  素の Alloc（0800295C 付近）は、空きが無いと AGBAssert(..., 1)（未定義命令 0xEFFF）を通る。その後の動きが不定で、戻り値が NULL にならず
//  （0x02022B25・0x02000000 などを観測）呼び元がそこへ展開して戦闘用 RAM を潰す／PC が BIOS（0x1C4）で止まる。レイドでダイアイスを撃たれたときが再現
//  （ICE_ROCK 10240 バイトに対し、最大の空きが 0x2544 だった）。attack_anim_table.s の「足りないときは ICE_ROCK_SINGLE」の分岐へ入れるよう、
//  足りないなら Alloc せず「読まない」（TRUE）で返す。ブロックの形は素の malloc.c と同じ（見出し 16 バイト: s16 flag・s16 magic 0xA3A3・u32 size・prev・next）。
//  壊れた見出し（magic 違い・範囲外・つながりが戻らない）に当たったら「空きなし」に倒す。回数の上限つき
#define HACKHEAP_START_PTR   (*(u32 **) 0x03000A38)	//Malloc 08002B9C が読む gHeapStart
#define HACKHEAP_LO          0x02000000
#define HACKHEAP_HI          0x02040000
#define HACKHEAP_MAGIC       0xA3A3
#define HACKHEAP_MAX_BLOCKS  1024

static bool8 HackHeap_HasFree(u32 need)
{
	u32 *head = HACKHEAP_START_PTR;
	u32 *blk = head;
	u32 i;

	if ((u32) head < HACKHEAP_LO || (u32) head >= HACKHEAP_HI || ((u32) head & 3))
		return FALSE;
	if (need & 3)
		need = (need & ~3u) + 4;	//Alloc と同じ 4 バイト丸め

	for (i = 0; i < HACKHEAP_MAX_BLOCKS; ++i)
	{
		u16 flag = *(u16 *) blk;
		u16 magic = *((u16 *) blk + 1);
		u32 size = blk[1];
		u32 *next = (u32 *) blk[3];

		if (magic != HACKHEAP_MAGIC)
			return FALSE;
		if (flag == 0 && size >= need)
			return TRUE;
		if (next == head)
			return FALSE;
		if ((u32) next < HACKHEAP_LO || (u32) next >= HACKHEAP_HI || ((u32) next & 3) || next <= blk)
			return FALSE;
		blk = next;
	}
	return FALSE;
}

//LoadCompressedSpriteSheetUsingHeap 0800E9E8（pokefirered decompress.c）: Moémon の連れ歩き・ボールのシートが通る。TRUE = 失敗（素と同じ）
bool8 HackSplit_LoadCompressedSpriteSheetUsingHeap(const struct CompressedSpriteSheet *src)
{
	struct SpriteSheet dest;
	void *buffer;
	void *alloc;
	const u8 *s = HackSplit_FetchEntry(src->data, &alloc);

	if (s == NULL)
		return TRUE;
	if (!HackHeap_HasFree((*(const u32 *) s) >> 8))	//空きが足りない: Alloc に入ると壊れる（上の説明）ので読まない
	{
		if (alloc)
			Free(alloc);
		return TRUE;
	}
	buffer = AllocZeroed((*(const u32 *) s) >> 8);
	if (!buffer)
	{
		if (alloc)
			Free(alloc);
		return TRUE;
	}
	LZ77UnCompWram(s, buffer);
	dest.data = buffer;
	dest.size = src->size;
	dest.tag = src->tag;
	LoadSpriteSheet(&dest);
	Free(buffer);
	if (alloc)
		Free(alloc);
	return FALSE;
}

//LoadCompressedSpritePaletteUsingHeap 0800EA2C（pokefirered decompress.c）: 2026-10-20 P-A DPE の色（正面・色違い・Moémon の LZ77）を後ろ半分へ出したので、
//これも 展開前に「展開後の大きさ」を src->data から直に読む（SuperCard は 0x0A… が鏡でゴミを読む）。TRUE = 失敗（素と同じ）
bool8 HackSplit_LoadCompressedSpritePaletteUsingHeap(const struct CompressedSpritePalette *src)
{
	struct SpritePalette dest;
	void *buffer;
	void *alloc;
	const u8 *s = HackSplit_FetchEntry(src->data, &alloc);

	if (s == NULL)
		return TRUE;
	buffer = AllocZeroed((*(const u32 *) s) >> 8);
	if (!buffer)
	{
		if (alloc)
			Free(alloc);
		return TRUE;
	}
	LZ77UnCompWram(s, buffer);
	dest.data = buffer;
	dest.tag = src->tag;
	LoadSpritePalette(&dest);
	Free(buffer);
	if (alloc)
		Free(alloc);
	return FALSE;
}
