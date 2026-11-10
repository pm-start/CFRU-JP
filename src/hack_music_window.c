#include "defines.h"
#include "../include/malloc.h"
#include "../include/m4a.h"

/*
 * SuperCard の「曲の窓」（2026-10-20・docs/research/2026-10-20_supercard-music-window.md・ユーザー承認）。
 *
 * SuperCard は 64MB の後ろ半分が見えない（0x0A000000〜は 32MB の鏡）ので、BGM（BW 風アレンジ 106 曲・64MB では後ろ半分から直に鳴る）は
 * 前半の末尾の「窓」（0x09E00000・448KB・tools/build/rom_regions.py MUSIC_WINDOW）へ、鳴らす直前に SD の PKSMUS.BIN から写してから鳴らす:
 *   1. m4aSongNumStart／OrChange／OrContinue（functionrewrites）が、曲の番号（曲の表の枠）で下の gHackMusTable を引く
 *   2. 載っていて、SuperCard（0x0A000000 が 32MB の鏡）なら: 窓の札（頭の 16 バイト）が その曲の塊のものか見る。同じならそのまま鳴らす
 *   3. 違えば、窓の曲を鳴らしているプレーヤーを止める → 札を消す → SD から 4KB ずつ読んで窓へ書く（中核の op 2・SDRAM を書き込み可にして 16 ビット）→ 札を書く
 *   4. 窓の中の見出し（e->header）で MPlayStart（素の関数と同じ分岐）
 *   5. 失敗（SD が無い・読めない・書けた確認が合わない）は **曲の表の元の見出し** で鳴らす。フェーズ B（旧曲を前半から消した版）では、表の枠は
 *      bw_arrange.write_stubs の 8 バイトの「無音の見出し」（トラック 0・優先度 255）＝**無音**（前の曲も止まる）。ゲームは止まらない・固まらない
 *   m4aSongNumStop／Continue（081C117C／081C11B0）も包む: 窓から鳴っている曲は info の見出しが窓の中の見出しなので、素の「表の見出しと同じか」では
 *   止まらない／続かない。曲の表を番号で引く関数は この 5 つだけ（素 ROM の 081C10D8 の語を読むのは 5 か所＝repoints の TABLE_REFS と同じ）
 * 64MB の直した mGBA（後ろ半分が見える）では何もしない（元の関数と同じ動き）。SuperCard でない 32MB（素の mGBA）は SD が開けず 5 に落ちる。
 *
 * gHackMusTable は build_rom.py が ROM に書く（枠 → SD の区画・大きさ・窓の中の見出し・札の値 stamp）。PKSMUS.BIN（tools/bgm/mus_window.py）と同じビルドのもの。
 * 窓は IME=0 の長い処理を 4KB ずつに区切る（SD 4KB ≒ 約 10 ミリ秒・VBlank をまたがない）。曲の切り替えは 1 曲 約 0.2〜1.1 秒の停止（同期・推測）。
 * 状態は窓の札（SDRAM）にだけ持つ（6.13・RAM に書き換える変数を置かない）。
 *
 * ★ HACK_DEBUG_MENU の版だけ: 試験用に、SD の代わりに XRAM（0x01100000・直した mGBA）の写し（"PKSMUSX1"・tools/bgm/mus_window.py --emu-image）から読める。
 *   窓への書き込みだけは SuperCard の SDRAM と同じ 0x09FFFFFE のモード（A55A・A55A・値・値）を真似る mGBA（tools/mgba/scwindow.patch）が要る。
 */

#define MUS_WIN_BASE   0x09E00000
#define MUS_WIN_SIZE   0x70000
#define MUS_WIN_TAG    16
#define MUS_CHUNK      4096
#define MUS_FILE_83    "PKSMUS  BIN"
#define MUS_FILE_LONG  "PKSMUS.BIN"
#define MUS_TABLE_MAGIC 0x534D4B48          //"HKMS"（ROM に書く前の印・build_rom.py が count を入れる）
#define MUS_TAG_MAGIC   0x574D5350          //"PSMW"
#define HACKMUS_MAX    160

struct HackMusEntry
{
	u16 slot;        //曲の表の番号
	u16 pad;
	u32 sector;      //PKSMUS.BIN の中の、その塊の頭（見出しの区画）
	u32 size;        //塊の中身の大きさ（バイト）
	u32 header;      //窓の中の曲の見出しの番地
};

struct HackMusTable
{
	u32 magic;
	u32 stamp;       //PKSMUS.BIN の札（塊の中身の SHA-1 の先頭 4 バイト）
	u32 count;       //枠の数（0 = この機能は無し）
	u32 reserved;
	struct HackMusEntry e[HACKMUS_MAX];
};

//tools/build/build_rom.py（write_music_window）が SuperCard の ROM にだけ書く。ここでは初期値（count 0）。
//★ volatile: ビルドのあとで中身が書き換わるので、コンパイラが「count は 0」と決めつけて関数ごと畳まないように（2026-10-20 逆アセンブルで畳まれていたのを確認）
const volatile struct HackMusTable gHackMusTable = { .magic = MUS_TABLE_MAGIC };

bool8 HackSplit_Native(void);                                                  //src/hack_split_pics.c
void *HackScsd_FileOpen(const char *name83, const char *longName);             //src/hack_scsd.c
bool8 HackScsd_FileRead(void *fs, u32 sector, u32 n, void *out);
void HackScsd_FileClose(void *fs);
bool8 HackScsd_WriteRom(u32 dst, const void *src, u32 n);
bool8 HackXram_Available(void);                                                //src/hack_xram.c

#ifdef HACK_DEBUG_MENU
//---- 試験用: XRAM の写しから読む（写しの形は tools/bgm/mus_window.py の emu_image）----
#define EMU_BASE 0x01100000   //0x01000100〜の hack_battle_squash・0x01FF0000〜の hack_xram_battle と重ならない（写しは 0xEF0000 バイトまで）

static bool8 Mus_EmuActive(void)
{
	const vu8 *p = (const vu8 *) EMU_BASE;

	return HackXram_Available() && p[0] == 'P' && p[1] == 'K' && p[2] == 'S' && p[3] == 'M' && p[4] == 'U' && p[5] == 'S' && p[6] == 'X' && p[7] == '1';
}

static bool8 Mus_EmuRead(u32 sector, u32 n, u8 *out)
{
	const vu32 *h = (const vu32 *) EMU_BASE;
	u32 count = h[2], i;

	for (i = 0; i < count && i < 40; i++)
	{
		u32 sec = h[3 + i * 3], secs = h[4 + i * 3], pos = h[5 + i * 3];

		if (sector >= sec && sector + n <= sec + secs)
		{
			const vu32 *src = (const vu32 *) (EMU_BASE + pos + (sector - sec) * 512);
			u32 *dst = (u32 *) out, k;

			for (k = 0; k < n * 128; k++)
				dst[k] = src[k];
			return TRUE;
		}
	}
	return FALSE;
}
#endif

static void *Mus_Open(void)
{
#ifdef HACK_DEBUG_MENU
	if (Mus_EmuActive())
		return (void *) 1;
#endif
	return HackScsd_FileOpen(MUS_FILE_83, MUS_FILE_LONG);
}

static bool8 Mus_Read(void *fs, u32 sector, u32 n, u8 *out)
{
#ifdef HACK_DEBUG_MENU
	if (fs == (void *) 1)
		return Mus_EmuRead(sector, n, out);
#endif
	return HackScsd_FileRead(fs, sector, n, out);
}

static void Mus_Close(void *fs)
{
#ifdef HACK_DEBUG_MENU
	if (fs == (void *) 1)
		return;
#endif
	HackScsd_FileClose(fs);
}

//曲の表の番号 slot の枠を out に写す（無ければ FALSE）
static bool8 Mus_Find(u16 slot, struct HackMusEntry *out)
{
	u32 i, n = gHackMusTable.count;

	if (gHackMusTable.magic != MUS_TABLE_MAGIC || n == 0 || n > HACKMUS_MAX)
		return FALSE;
	for (i = 0; i < n; i++)
	{
		if (gHackMusTable.e[i].slot == slot)
		{
			out->slot = slot;
			out->pad = 0;
			out->sector = gHackMusTable.e[i].sector;
			out->size = gHackMusTable.e[i].size;
			out->header = gHackMusTable.e[i].header;
			return TRUE;
		}
	}
	return FALSE;
}

static bool8 Mus_IsLoaded(const struct HackMusEntry *e)
{
	const vu32 *t = (const vu32 *) MUS_WIN_BASE;

	return t[0] == MUS_TAG_MAGIC && t[1] == gHackMusTable.stamp && t[2] == e->sector && t[3] == e->size;
}

static void Mus_StopWindowPlayers(void)
{
	struct MusicPlayerInfo *infos[4];
	u32 i;

	infos[0] = &gMPlayInfo_BGM;
	infos[1] = &gMPlayInfo_SE1;
	infos[2] = &gMPlayInfo_SE2;
	infos[3] = &gMPlayInfo_SE3;
	for (i = 0; i < 4; i++)
	{
		u32 h = (u32) infos[i]->songHeader;

		if (h >= MUS_WIN_BASE && h < MUS_WIN_BASE + MUS_WIN_SIZE)
		{
			m4aMPlayStop(infos[i]);
			//窓の中身はこれから別の曲に書き換わる。見出しの番地が次の曲と偶然同じでも「同じ曲が止まっているだけ」と取り違えて
			//（OrContinue の m4aMPlayContinue）書き換わったトラックを走らせないよう、前の見出しを捨てる（フェーズ B）
			infos[i]->songHeader = NULL;
		}
	}
}

//SD の PKSMUS.BIN から塊を読んで窓へ写す。TRUE = 窓にその曲が入った（もう入っていた場合も）
static bool8 Mus_Load(const struct HackMusEntry *e)
{
	void *fs;
	u32 *buf;
	u32 off;
	bool8 ok = FALSE;
	u32 tag[4];

	if (Mus_IsLoaded(e))
		return TRUE;
	if (e->size == 0 || e->size > MUS_WIN_SIZE - MUS_WIN_TAG || (e->size & 1))
		return FALSE;
	buf = Malloc(MUS_CHUNK);
	if (buf == NULL)
		return FALSE;
	fs = Mus_Open();
	if (fs == NULL)
	{
		Free(buf);
		return FALSE;
	}
	//塊の頭の 1 区画（"PKSMUSB1"・stamp・区画・大きさ・見出し）が ROM の表と合うか（古い SD のファイルを使わない）
	if (!Mus_Read(fs, e->sector, 1, (u8 *) buf)
	 || buf[0] != 0x4D534B50 || buf[1] != 0x31425355        //"PKSM" "USB1"
	 || buf[2] != gHackMusTable.stamp || buf[3] != e->sector || buf[4] != e->size || buf[5] != e->header)
		goto DONE;
	Mus_StopWindowPlayers();
	tag[0] = tag[1] = tag[2] = tag[3] = 0;
	if (!HackScsd_WriteRom(MUS_WIN_BASE, tag, MUS_WIN_TAG))   //途中で失敗したら札は無効のまま
		goto DONE;
	for (off = 0; off < e->size; off += MUS_CHUNK)
	{
		u32 n = e->size - off;

		if (n > MUS_CHUNK)
			n = MUS_CHUNK;
		if (!Mus_Read(fs, e->sector + 1 + off / 512, (n + 511) / 512, (u8 *) buf)
		 || !HackScsd_WriteRom(MUS_WIN_BASE + MUS_WIN_TAG + off, buf, n))
			goto DONE;
	}
	tag[0] = MUS_TAG_MAGIC;
	tag[1] = gHackMusTable.stamp;
	tag[2] = e->sector;
	tag[3] = e->size;
	ok = HackScsd_WriteRom(MUS_WIN_BASE, tag, MUS_WIN_TAG);
DONE:
	Mus_Close(fs);
	Free(buf);
	return ok;
}

//曲の表の番号 n と元の見出し orig → 鳴らす見出し（窓の曲なら窓の中の見出し・それ以外と失敗は orig）
static const struct SongHeader *Mus_Resolve(u16 n, const struct SongHeader *orig)
{
	struct HackMusEntry e;

	if (gHackMusTable.count == 0 || HackSplit_Native())
		return orig;
	if (!Mus_Find(n, &e) || !Mus_Load(&e))
		return orig;
	return (const struct SongHeader *) e.header;
}

//枠 n の曲が今 info で（窓の中から）鳴っているか。窓の札がその曲のもので、info の見出しが窓の中のその曲の見出しなら TRUE（読むだけ・SD には触れない）
static bool8 Mus_PlayingFromWindow(u16 n, const struct MusicPlayerInfo *info)
{
	struct HackMusEntry e;

	if (gHackMusTable.count == 0 || HackSplit_Native())
		return FALSE;
	return Mus_Find(n, &e) && Mus_IsLoaded(&e) && (u32) info->songHeader == e.header;
}

//---- 素の m4aSongNumStart／OrChange／OrContinue（081C10B0／081C10DC／081C1128）と同じ分岐に「窓の曲なら見出しを差し替える」を足したもの ----

void HackMus_SongNumStart(u16 n)
{
	const struct Song *song = &gSongTable[n];
	const struct MusicPlayer *mplay = &gMPlayTable[song->ms];

	MPlayStart(mplay->info, Mus_Resolve(n, song->header));
}

void HackMus_SongNumStartOrChange(u16 n)
{
	const struct Song *song = &gSongTable[n];
	struct MusicPlayerInfo *info = gMPlayTable[song->ms].info;
	const struct SongHeader *header = Mus_Resolve(n, song->header);

	if (info->songHeader != header)
		MPlayStart(info, header);
	else if (!(info->status & MUSICPLAYER_STATUS_TRACK) || (info->status & MUSICPLAYER_STATUS_PAUSE))
		MPlayStart(info, header);
}

void HackMus_SongNumStartOrContinue(u16 n)
{
	const struct Song *song = &gSongTable[n];
	struct MusicPlayerInfo *info = gMPlayTable[song->ms].info;
	const struct SongHeader *header = Mus_Resolve(n, song->header);

	if (info->songHeader != header)
		MPlayStart(info, header);
	else if (!(info->status & MUSICPLAYER_STATUS_TRACK))
		MPlayStart(info, header);
	else if (info->status & MUSICPLAYER_STATUS_PAUSE)
		m4aMPlayContinue(info);
}

//---- 素の m4aSongNumStop／Continue（081C117C／081C11B0）: 曲の表の見出しと info の見出しが同じなら止める／続ける ----
//SuperCard で窓から鳴っている曲は、info の見出しが窓の中の見出し（曲の表の見出しは無音の見出し）なので、素の比べ方では止まらない・続かない。窓の曲も数える。

void HackMus_SongNumStop(u16 n)
{
	const struct Song *song = &gSongTable[n];
	struct MusicPlayerInfo *info = gMPlayTable[song->ms].info;

	if (info->songHeader == song->header || Mus_PlayingFromWindow(n, info))
		m4aMPlayStop(info);
}

void HackMus_SongNumContinue(u16 n)
{
	const struct Song *song = &gSongTable[n];
	struct MusicPlayerInfo *info = gMPlayTable[song->ms].info;

	if (info->songHeader == song->header || Mus_PlayingFromWindow(n, info))
		m4aMPlayContinue(info);
}