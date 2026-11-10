#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokemon_icon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/pokemon_storage_system_internal.h"
#include "../include/string_util.h"
#include "../include/text.h"
#include "../include/constants/flags.h"
#include "../include/constants/vars.h"

#include "../include/new/build_pokemon.h"
#include "../include/new/form_change.h"
#include "../include/new/frontier.h"
#include "../include/new/item.h"
#include "../include/new/pokemon_storage_system.h"
#include "hack_mon_id.h" //[hack] patches/0259
#include "hack_ribbon.h" //[hack] patches/0977 リボンの表（箱の縮めた形はリボンを捨てるので、鍵つきの別の表で持つ）
#include "hack_kind.h" //[hack] patches/0958 個体の印（ヌシ・デルタ種・ダーク）
#include "shadow_relive.h" //[hack] patches/0965 段 G リライブボックス
#include "hack_box_shelf.h" //[hack] patches/0196 ボックスの仮想化
#include "hack_ramshift.h" //[hack] patches/0622 PokemonStorage の位置は版ごと（HACK_STORAGE_BASE）
#include "hack_stat_access.h"   //[hack] patches/0761 上の桁の表（窓口）
/*
pokemon_storage_system.c
	handles pokemon storage expansion and related functions
*/

enum
{
	WALLPAPER_FOREST,
	WALLPAPER_CITY,
	WALLPAPER_DESERT,
	WALLPAPER_SAVANNA,
	WALLPAPER_CRAG,
	WALLPAPER_VOLCANO,
	WALLPAPER_SNOW,
	WALLPAPER_CAVE,
	WALLPAPER_BEACH,
	WALLPAPER_SEAFLOOR,
	WALLPAPER_RIVER,
	WALLPAPER_SKY,
	WALLPAPER_STARS,
	WALLPAPER_POKECENTER,
	WALLPAPER_TILES,
	WALLPAPER_SIMPLE,
	WALLPAPER_SPARE, //[hack] patches/0242 使わない番号（メニューの文 38 番は CFRU の Select）
	WALLPAPER_PEACH, //[hack] patches/0242 もも（src/hack_box_wallpaper.c）
	WALLPAPER_ORI, //[hack] FEAT-box-wallpaper-ori-pokes オリポケーズ（18）
	WALLPAPER_COUNT
};

typedef u8 BoxNameT[9];

#define ORIGINAL_BOX_NAME_RAM ((BoxNameT*) (HACK_STORAGE_BASE + 0x8344))
#define ORIGINAL_BOX_WALLPAPER_RAM ((u8*) (HACK_STORAGE_BASE + 0x83C2))

//#define sPSSData ((struct PokemonStorageSystemData*) *((u32*) 0x20397B0))

#define gDisposableBoxMon ((struct BoxPokemon*) 0x2022B24) //Reuse Battle Buffer A

#define TABLE_BOXES_COUNT 50   //[hack] FEAT-box100 patches/0913: 表は 1〜50 箱だけ・51〜100 箱（箱 id 50〜99）は BOX2_NAMES_OFF からの計算（GetBoxNamePtr・GetBoxWallpaper・SetBoxWallpaper）

BoxNameT* const sPokemonBoxNamePtrs[TABLE_BOXES_COUNT] =
{
	ORIGINAL_BOX_NAME_RAM,		//Box 1
	ORIGINAL_BOX_NAME_RAM + 1,	//Box 2
	ORIGINAL_BOX_NAME_RAM + 2,	//Box 3
	ORIGINAL_BOX_NAME_RAM + 3,	//Box 4
	ORIGINAL_BOX_NAME_RAM + 4,	//Box 5
	ORIGINAL_BOX_NAME_RAM + 5,	//Box 6
	ORIGINAL_BOX_NAME_RAM + 6,	//Box 7
	ORIGINAL_BOX_NAME_RAM + 7,	//Box 8
	ORIGINAL_BOX_NAME_RAM + 8,	//Box 9
	ORIGINAL_BOX_NAME_RAM + 9,	//Box 10
	ORIGINAL_BOX_NAME_RAM + 10,	//Box 11
	ORIGINAL_BOX_NAME_RAM + 11,	//Box 12
	ORIGINAL_BOX_NAME_RAM + 12,	//Box 13
	ORIGINAL_BOX_NAME_RAM + 13,	//Box 14
	ORIGINAL_BOX_NAME_RAM - 1,	//Box 15
	ORIGINAL_BOX_NAME_RAM - 2,	//Box 16
	ORIGINAL_BOX_NAME_RAM - 3,	//Box 17
	ORIGINAL_BOX_NAME_RAM - 4,	//Box 18
	ORIGINAL_BOX_NAME_RAM - 5,	//Box 19
	ORIGINAL_BOX_NAME_RAM - 6,	//Box 20
	ORIGINAL_BOX_NAME_RAM - 7,	//Box 21
	ORIGINAL_BOX_NAME_RAM - 8,	//Box 22
	ORIGINAL_BOX_NAME_RAM - 9,	//Box 23
	ORIGINAL_BOX_NAME_RAM - 10,	//Box 24
	ORIGINAL_BOX_NAME_RAM - 11,	//Box 25
	//[hack] patches/0196 26〜39 箱（PokemonStorage + 0x8134 の空き・棚のページ 0 に入る）
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 0,	//Box 26
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 1,	//Box 27
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 2,	//Box 28
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 3,	//Box 29
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 4,	//Box 30
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 5,	//Box 31
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 6,	//Box 32
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 7,	//Box 33
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 8,	//Box 34
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 9,	//Box 35
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 10,	//Box 36
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 11,	//Box 37
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 12,	//Box 38
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8134) + 13,	//Box 39
	//[hack] patches/0232 40〜50 箱（PokemonStorage + 0x8260・追加のセーブ領域のページ 4 に入る）
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 0,	//Box 40
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 1,	//Box 41
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 2,	//Box 42
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 3,	//Box 43
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 4,	//Box 44
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 5,	//Box 45
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 6,	//Box 46
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 7,	//Box 47
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 8,	//Box 48
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 9,	//Box 49
	(BoxNameT*) (HACK_STORAGE_BASE + 0x8260) + 10,	//Box 50
};

//Declaring this as "static" makes it not compile properly for some reason.
u8* const sPokemonBoxWallpaperPtrs[TABLE_BOXES_COUNT] =
{
	ORIGINAL_BOX_WALLPAPER_RAM,							//Box 1
	ORIGINAL_BOX_WALLPAPER_RAM + 1,						//Box 2
	ORIGINAL_BOX_WALLPAPER_RAM + 2,						//Box 3
	ORIGINAL_BOX_WALLPAPER_RAM + 3,						//Box 4
	ORIGINAL_BOX_WALLPAPER_RAM + 4,						//Box 5
	ORIGINAL_BOX_WALLPAPER_RAM + 5,						//Box 6
	ORIGINAL_BOX_WALLPAPER_RAM + 6,						//Box 7
	ORIGINAL_BOX_WALLPAPER_RAM + 7,						//Box 8
	ORIGINAL_BOX_WALLPAPER_RAM + 8,						//Box 9
	ORIGINAL_BOX_WALLPAPER_RAM + 9,						//Box 10
	ORIGINAL_BOX_WALLPAPER_RAM + 10,					//Box 11
	ORIGINAL_BOX_WALLPAPER_RAM + 11,					//Box 12
	ORIGINAL_BOX_WALLPAPER_RAM + 12,					//Box 13
	ORIGINAL_BOX_WALLPAPER_RAM + 13,					//Box 14
	(u8*) (HACK_STORAGE_BASE + 0x8128),		//Box 15
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 1,	//Box 16
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 2,	//Box 17
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 3,	//Box 18
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 4,	//Box 19
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 5,	//Box 20
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 6,	//Box 21
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 7,	//Box 22
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 8,	//Box 23
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 9,	//Box 24
	(u8*) (HACK_STORAGE_BASE + 0x8128) + 10,	//Box 25
	//[hack] patches/0196 26〜39 箱（PokemonStorage + 0x81B2）
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 0,	//Box 26
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 1,	//Box 27
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 2,	//Box 28
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 3,	//Box 29
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 4,	//Box 30
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 5,	//Box 31
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 6,	//Box 32
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 7,	//Box 33
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 8,	//Box 34
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 9,	//Box 35
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 10,	//Box 36
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 11,	//Box 37
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 12,	//Box 38
	(u8*) (HACK_STORAGE_BASE + 0x81B2) + 13,	//Box 39
	//[hack] patches/0232 40〜50 箱（PokemonStorage + 0x82C3）
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 0,	//Box 40
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 1,	//Box 41
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 2,	//Box 42
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 3,	//Box 43
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 4,	//Box 44
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 5,	//Box 45
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 6,	//Box 46
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 7,	//Box 47
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 8,	//Box 48
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 9,	//Box 49
	(u8*) (HACK_STORAGE_BASE + 0x82C3) + 10,	//Box 50
};

#define gTempTeamBackup ((struct CompressedPokemon*) 0x203E118)

//This file's functions:
void CreateBoxMonFromCompressedMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);
#define HACK_MARK_OFF 0x10   //[hack] patches/0286 hyperTraining（手持ち・縮めた形とも 0x10）
#define HACK_MARK_BIT 0x80
void CreateCompressedMonFromBoxMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);
struct CompressedPokemon* GetCompressedMonPtr(u8 boxId, u8 boxPosition);

//[hack] patches/0196 ボックスの仮想化: 読むときは 1 匹の写し（棚の箱が入れ物に無ければフラッシュから直に）・書くときは入れ物（汚れの印）
static void ReadMonAt(u8 boxId, u8 boxPosition, struct CompressedPokemon* out)
{
	//[hack] patches/0430 L 案: 20〜25 箱の固定の場所は無くなり、全 50 箱が棚（BoxShelf_*）
	BoxShelf_ReadMon(boxId, boxPosition, out);
}

static struct CompressedPokemon* MonPtrForWrite(u8 boxId, u8 boxPosition)
{
	return &BoxShelf_BoxMons(boxId, TRUE)[boxPosition]; //呼び元が boxId < TOTAL_BOXES_COUNT（= 50 = 全部棚）を見ている
}

//[hack] patches/0680 X 案: 箱の努力値・個体値は 16 ビット（下位 = 63 バイトの中の u8／5 ビット・上位 = 後ろの evHi／ivHi）。
//  手持ち・80 バイト形には入らない上位は、箱から出すとき 255／31 に頭打ち（CreateBoxMonFromCompressedMon）。今の上限（252／31）では上位は常に 0。
u16 HackBox_CompEv(const struct CompressedPokemon* c, u8 i)
{
	return ((const u8*) &c->hpEv)[i] | ((u16) c->evHi[i] << 8);
}

static u16 CompIvHi(const struct CompressedPokemon* c, u8 i)
{
	u32 bit = 11 * i, o = bit >> 3;
	u32 v = c->ivHi[o] | ((u32) c->ivHi[o + 1] << 8) | ((u32) c->ivHi[o + 2] << 16);	//i = 5 でも o + 2 = 8（9 バイトの中）
	return (v >> (bit & 7)) & 0x7FF;
}

u16 HackBox_CompIv(const struct CompressedPokemon* c, u8 i)
{
	return ((c->ivs >> (5 * i)) & 31) | (CompIvHi(c, i) << 5);
}

static void CompSetEv(struct CompressedPokemon* c, u8 i, u16 v)
{
	((u8*) &c->hpEv)[i] = v;
	c->evHi[i] = v >> 8;
}

static void CompSetIv(struct CompressedPokemon* c, u8 i, u16 v)
{
	u32 bit = 11 * i, o = bit >> 3, sh = bit & 7;
	u32 cur = c->ivHi[o] | ((u32) c->ivHi[o + 1] << 8) | ((u32) c->ivHi[o + 2] << 16);

	cur = (cur & ~(0x7FFu << sh)) | ((u32) (v >> 5) << sh);
	c->ivHi[o] = cur;
	c->ivHi[o + 1] = cur >> 8;
	c->ivHi[o + 2] = cur >> 16;
	c->ivs = (c->ivs & ~(31u << (5 * i))) | ((u32) (v & 31) << (5 * i));
}

u32 GetBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		if ((request >= MON_DATA_HP_EV && request <= MON_DATA_SPDEF_EV) || (request >= MON_DATA_HP_IV && request <= MON_DATA_SPDEF_IV))
		{
			//[hack] patches/0680 箱の中の値は 16 ビットで返す（80 バイト形に直すと 255／31 で頭打ちになるので、直に読む）
			struct CompressedPokemon comp;
			ReadMonAt(boxId, boxPosition, &comp);
			return (request <= MON_DATA_SPDEF_EV) ? HackBox_CompEv(&comp, request - MON_DATA_HP_EV) : HackBox_CompIv(&comp, request - MON_DATA_HP_IV);
		}
		if (request == MON_DATA_SPECIES)
		{
			//Save time since checking if a slot is empty is very common
			struct CompressedPokemon comp;
			ReadMonAt(boxId, boxPosition, &comp);
			return comp.substruct0.species;
		}
		else
		{
			struct BoxPokemon mon;
			struct CompressedPokemon comp;
			ReadMonAt(boxId, boxPosition, &comp);
			CreateBoxMonFromCompressedMon(&mon, &comp);
			return GetBoxMonData(&mon, request, NULL);
		}
	}
	else
		return 0;
}

void SetBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request, const void* value)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct Pokemon mon; //Not Box Mon just in case stats are modified by hold item form change and it corrupts data
		struct CompressedPokemon* comp = MonPtrForWrite(boxId, boxPosition);
		CreateBoxMonFromCompressedMon((struct BoxPokemon*) &mon, comp); //Create temporary mon
		SetBoxMonData((struct BoxPokemon*) &mon, request, value);
		CreateCompressedMonFromBoxMon((struct BoxPokemon*) &mon, comp); //Copy new data back
	}
}

void GetBoxMonNickAt(u8 boxId, u8 boxPosition, u8* dst)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct BoxPokemon mon;
		struct CompressedPokemon comp;
		ReadMonAt(boxId, boxPosition, &comp);
		CreateBoxMonFromCompressedMon(&mon, &comp);
		GetBoxMonData(&mon, MON_DATA_NICKNAME, dst);
	}
	else
		*dst = EOS;
}

void SetBoxMonNickAt(u8 boxId, u8 boxPosition, const u8* nick)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct BoxPokemon mon;
		struct CompressedPokemon* comp = MonPtrForWrite(boxId, boxPosition);
		CreateBoxMonFromCompressedMon(&mon, comp);
		SetBoxMonData(&mon, MON_DATA_NICKNAME, nick);
		CreateCompressedMonFromBoxMon(&mon, comp); //Copy new data back
	}
}

u32 GetAndCopyBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request, void* dst)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct BoxPokemon mon;
		struct CompressedPokemon comp;
		ReadMonAt(boxId, boxPosition, &comp);
		CreateBoxMonFromCompressedMon(&mon, &comp);
		return GetBoxMonData(&mon, request, dst);
	}
	else
		return 0;
}

void SetBoxMonAt(u8 boxId, u8 boxPosition, struct BoxPokemon* src)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		#ifdef BOX_PRESET_TEAMS
		if (boxId != BOX_PRESET_TEAMS) //Don't revert in Battle Box
		#endif
			HoopaShayminPCRevertCheck((struct Pokemon*) src, FALSE); //Don't recalc stats for a box mon

		struct CompressedPokemon mon;
		CreateCompressedMonFromBoxMon(src, &mon);
		*MonPtrForWrite(boxId, boxPosition) = mon;
		HackRibbon_StoreFromMon(src); //[hack] patches/0977 預けるとき手持ちのリボンを表へ（ID はいま付いた）
	}
}

void CopyBoxMonAt(u8 boxId, u8 boxPosition, struct CompressedPokemon* dst)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
		ReadMonAt(boxId, boxPosition, dst);
}

void CreateBoxMonAt(u8 boxId, u8 boxPosition, u16 species, u16 level, u8 fixedIV, u8 hasFixedPersonality, u32 personality, u8 otIDType, u32 otID)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct BoxPokemon boxMon;
		CreateBoxMon(&boxMon,
					 species,
					 level,
					 fixedIV,
					 hasFixedPersonality, personality,
					 otIDType, otID);

		struct CompressedPokemon compMon;
		CreateCompressedMonFromBoxMon(&boxMon, &compMon);
		*MonPtrForWrite(boxId, boxPosition) = compMon;
	}
}

void ZeroBoxMonAt(u8 boxId, u8 boxPosition)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		//[hack] patches/0196 もう空なら書かない（はじめからの全部の箱を空にする所で、棚の箱を汚さない）
		struct CompressedPokemon comp;
		const u8* b = (const u8*) &comp;
		u32 i;
		ReadMonAt(boxId, boxPosition, &comp);
		for (i = 0; i < sizeof(comp) && b[i] == 0; ++i);
		if (i < sizeof(comp))
			memset(MonPtrForWrite(boxId, boxPosition), 0, sizeof(struct CompressedPokemon));
	}
}

void BoxMonAtToMon(u8 boxId, u8 boxPosition, struct Pokemon* dst)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct BoxPokemon mon;
		struct CompressedPokemon comp;
		ReadMonAt(boxId, boxPosition, &comp);
		CreateBoxMonFromCompressedMon(&mon, &comp);
		BoxMonToMon(&mon, dst);
		HackRibbon_RestoreToMon(dst); //[hack] patches/0977 引き出すとき表のリボンを戻す
	}
}

void CompressedMonToMon(struct CompressedPokemon* compMon, struct Pokemon* dst)
{
	struct BoxPokemon mon;
	CreateBoxMonFromCompressedMon(&mon, compMon);
	BoxMonToMon(&mon, dst);
	HackRibbon_RestoreToMon(dst); //[hack] patches/0977
}

struct BoxPokemon* GetBoxedMonPtr(u8 boxId, u8 boxPosition)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		struct CompressedPokemon compMon;
		ReadMonAt(boxId, boxPosition, &compMon);
		CreateBoxMonFromCompressedMon(gDisposableBoxMon, &compMon); //Use a static location in memory
		return gDisposableBoxMon;
	}
	else
		return NULL;
}

struct CompressedPokemon* GetCompressedMonPtr(u8 boxId, u8 boxPosition)
{
	if (boxId < TOTAL_BOXES_COUNT && boxPosition < IN_BOX_COUNT)
	{
		//[hack] patches/0196 ポインタを渡す（ステータス画面がこの先を 58 バイトずつ足して読む）→ 棚の箱は入れ物に読み込む（今の箱は追い出さない）
		struct CompressedPokemon* box = BoxShelf_BoxMons(boxId, FALSE);
		return (box != NULL) ? &box[boxPosition] : NULL;
	}
	else
		return NULL;
}

void CreateBoxMonFromCompressedMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon)
{
	u32 moveHi;
	Memset(boxMon, 0, sizeof(struct BoxPokemon)); //Clear garbage data

	Memcpy(boxMon, compMon, 0x1C); //Personality to markings
	Memcpy(&boxMon->substruct0, &compMon->substruct0, sizeof(struct CompressedPokemonSubstruct0));
	Memcpy(&boxMon->substruct2, &compMon->hpEv, NUM_STATS); //Copy EVs
	Memcpy(&boxMon->substruct3, &compMon->pokerus, 8); //Copy some of substruct misc
	//[hack] patches/0680 X 案: 上位（evHi／ivHi）が 0 でなければ 255／31 に頭打ち（80 バイト形は u8／5 ビット・箱の中の 16 ビットの値は残る）
	//[hack] patches/0761 頭打ちした上の桁は「上の桁の表」（性格値が鍵・hack_stat_pool.c）に入れて持ち越す（入らなければ頭打ちのまま＝今までと同じ）
	{
		u16 wideEv[NUM_STATS], wideIv[NUM_STATS];
		bool8 anyWide = FALSE, anySat = FALSE;

		for (int s = 0; s < NUM_STATS; ++s)
		{
			wideEv[s] = HackBox_CompEv(compMon, s);
			wideIv[s] = HackBox_CompIv(compMon, s);
			if (wideEv[s] > STATPOOL_EV_WIDTH || wideIv[s] > STATPOOL_IV_WIDTH)
				anyWide = TRUE;
			if (wideEv[s] >= STATPOOL_EV_WIDTH || wideIv[s] >= STATPOOL_IV_WIDTH)
				anySat = TRUE;
			if (compMon->evHi[s] != 0)
				((u8*) &boxMon->substruct2)[s] = 255;
			if (wideIv[s] > STATPOOL_IV_WIDTH)
			{
				u32 w;
				Memcpy(&w, (u8*) &boxMon->substruct3 + 4, 4);
				w |= 31u << (5 * s);
				Memcpy((u8*) &boxMon->substruct3 + 4, &w, 4);
			}
		}
		if (anyWide || (anySat && HackStatPool_NotEmpty()))     //表が空で頭打ちも無ければ何もしない（熱い道・セーブも変わらない）
			HackStatPool_Store(compMon->personality, (u16) compMon->otid, wideEv, wideIv);
	}

	boxMon->substruct1.moves[0] = compMon->move1;
	boxMon->substruct1.moves[1] = compMon->move2;
	boxMon->substruct1.moves[2] = compMon->move3;
	boxMon->substruct1.moves[3] = compMon->move4;
	//[hack] patches/0583 W 案: 末尾 3 バイトから技の bit10〜15 を戻す（箱でも u16 を欠けさせない）
	moveHi = compMon->moveHi[0] | ((u32) compMon->moveHi[1] << 8) | ((u32) compMon->moveHi[2] << 16);

	//[hack] patches/0430 L 案: ext を 80 バイト形のヘッダの空き（unknown・+0x1E）へ（bit0〜8 = 出会った場所の上位 8 ビット・出会ったレベルの 8 ビット目・bit13〜15 = 出会ったレベルの 9〜11 ビット目）。
	//  技の 11 ビット目（ext の bit9〜12）は 80 バイト形の技（u16）の bit10 へ。ext = 0 の古い子はこれまでと同じ
boxMon->unknown = compMon->ext & 0xE1FF;
	HackKind_UnpackFromComp(compMon, boxMon); //[hack] patches/0958 印（種類 2 ビット＋値 8 ビット）を ext の bit9〜12・ivHi[8] の上 6 ビットから戻す（種類 0 なら何もしない＝今までと同じ）
for (int k = 0; k < MAX_MON_MOVES; ++k)
	boxMon->substruct1.moves[k] |= ((moveHi >> (6 * k)) & 0x3F) << 10;

	for (int i = 0; i < MAX_MON_MOVES; ++i) //Give PP
		boxMon->substruct1.pp[i] = CalculatePPWithBonus(boxMon->substruct1.moves[i], boxMon->substruct0.ppBonuses, i);

	//[hack] patches/0259 #70 ポケモン自身の表 ID・裏 ID: 縮めた形の空きビット 32 本 → コンディションの 4 バイト（src/hack_mon_id.c）
	HackMonId_ClearSpare(boxMon);
	HackMonId_Set(boxMon, HackMonId_UnpackComp(compMon));

	//[hack] patches/0286 うんめいの印（obedient・クチバの特別なフカマル・お母さんのピカチュウ）: 縮めた形には欄が無く、預けると消えていた。
	//  縮めるとき すごいとっくんの欄（0x10・使うのは bit0〜5）の bit7 に入れたものを戻す
	if (((u8*) boxMon)[HACK_MARK_OFF] & HACK_MARK_BIT)
	{
		((u8*) boxMon)[HACK_MARK_OFF] &= ~HACK_MARK_BIT;
		boxMon->substruct3.obedient = TRUE;
	}
}

void CreateCompressedMonFromBoxMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon)
{
	u32 moveHi = 0;
	u16 oldEv[NUM_STATS], oldIv[NUM_STATS];
	bool8 keepWide = FALSE;

	//[hack] patches/0680 X 案: 書き先に同じ子（性格値・親 ID・種族が同じ）が入っていて、箱の中の 16 ビットの値が 255／31 を超えていたら、
	//  80 バイト形の側で変わらなかった欄（頭打ちした値のまま）は箱の 16 ビットの値を残す（わざの入れ替え・ニックネームなど別の欄を書き直しても失わない）
	if (compMon->substruct0.species != SPECIES_NONE && compMon->personality == boxMon->personality && compMon->otid == boxMon->otId
	 && compMon->substruct0.species == boxMon->substruct0.species)
	{
		for (int s = 0; s < NUM_STATS; ++s)
		{
			oldEv[s] = HackBox_CompEv(compMon, s);
			oldIv[s] = HackBox_CompIv(compMon, s);
			if (oldEv[s] > 255 || oldIv[s] > 31)
				keepWide = TRUE;
		}
	}
	Memset(compMon, 0, sizeof(struct CompressedPokemon)); //Clear garbage data

	Memcpy(compMon, boxMon, 0x1C); //Personality to markings
	Memcpy(&compMon->substruct0, &boxMon->substruct0, sizeof(struct CompressedPokemonSubstruct0));
	Memcpy(&compMon->hpEv, &boxMon->substruct2, NUM_STATS); //Copy EVs
	Memcpy(&compMon->pokerus, &boxMon->substruct3, 8); //Copy some of substruct misc

	compMon->move1 = boxMon->substruct1.moves[0];
	compMon->move2 = boxMon->substruct1.moves[1];
	compMon->move3 = boxMon->substruct1.moves[2];
	compMon->move4 = boxMon->substruct1.moves[3];

	//[hack] patches/0430 L 案: 上の逆（ext = unknown の bit0〜8 + 技 1〜4 の bit10 を bit9〜12 へ・bit13〜15 は出会ったレベルの 9〜11 ビット目をそのまま写す）
compMon->ext = boxMon->unknown & 0xE1FF;
	HackKind_PackToComp(boxMon, compMon); //[hack] patches/0958 印を ext の bit9〜12・ivHi[8] の上 6 ビットへ（種類 0 なら何も立てない）
for (int k = 0; k < MAX_MON_MOVES; ++k)
	moveHi |= ((u32) boxMon->substruct1.moves[k] >> 10) << (6 * k);
compMon->moveHi[0] = moveHi;
compMon->moveHi[1] = moveHi >> 8;
compMon->moveHi[2] = moveHi >> 16;

	//[hack] patches/0259 #70 ポケモン自身の表 ID・裏 ID: まだ無ければ今の色違いのまま付けて、空きビット 32 本へ（src/hack_mon_id.c）
	HackMonId_Ensure(boxMon);
	HackMonId_PackBall(compMon); //[hack] patches/0393 ボールの 6 本目のビット（ボール 32〜37）を +0x13 の bit0 へ（PackComp が上の 3 ビットを潰す前に）
	HackMonId_PackComp(compMon, HackMonId_Get(boxMon));

	//[hack] patches/0286 うんめいの印を すごいとっくんの欄の bit7 へ（上の CreateBoxMonFromCompressedMon で戻す）
	((u8*) compMon)[HACK_MARK_OFF] &= ~HACK_MARK_BIT;
	if (boxMon->substruct3.obedient)
		((u8*) compMon)[HACK_MARK_OFF] |= HACK_MARK_BIT;

	//[hack] patches/0761 影が入れ物の最大ちょうどの欄は「上の桁の表」の値で 16 ビットに戻す（表が空なら引かない）
	if (HackStatPool_NotEmpty())
	{
		for (int s = 0; s < NUM_STATS; ++s)
		{
			u16 e = (((u8*) &compMon->hpEv)[s] == STATPOOL_EV_WIDTH) ? HackStatPool_ResolveEv(boxMon->personality, (u16) boxMon->otId, s, STATPOOL_EV_WIDTH) : 0;
			u16 i = (((compMon->ivs >> (5 * s)) & STATPOOL_IV_WIDTH) == STATPOOL_IV_WIDTH) ? HackStatPool_ResolveIv(boxMon->personality, (u16) boxMon->otId, s, STATPOOL_IV_WIDTH) : 0;

			if (e > STATPOOL_EV_WIDTH)
				CompSetEv(compMon, s, e);
			if (i > STATPOOL_IV_WIDTH)
				CompSetIv(compMon, s, i);
		}
	}
	//[hack] patches/0680 X 案: 80 バイト形から写した下位（u8／5 ビット）のまま = 上位は 0。頭打ちした値のままの欄は元の 16 ビットの値に戻す
	if (keepWide)
	{
		for (int s = 0; s < NUM_STATS; ++s)
		{
			if (oldEv[s] > 255 && HackBox_CompEv(compMon, s) == 255)
				CompSetEv(compMon, s, oldEv[s]);
			if (oldIv[s] > 31 && HackBox_CompIv(compMon, s) == 31)
				CompSetIv(compMon, s, oldIv[s]);
		}
	}
}

u8* GetBoxNamePtr(u8 boxId)
{
	if (boxId == RELIVE_LEGACY_BOX_ID)
	{
		//[hack] patches/0975 案 B: 0.65.0 は箱 26 の名前を「リライブボックス」に固定していた（遊ぶ人が付けた名前は上書きされて戻せない）。
		//  今の名前が その 9 バイトと完全に同じときだけ「ボックス26」に戻す（それ以外の名前には触らない）
		static const u8 sOld[9] = {0x78, 0x77, 0x52, 0x98, 0x9A, 0xA0, 0x58, 0x5D, 0xFF};   //リライブボックス
		static const u8 sNew[9] = {0x9A, 0xA0, 0x58, 0x5D, 0xA3, 0xA7, 0xFF, 0xFF, 0xFF};   //ボックス26
		u8* name = (u8*) sPokemonBoxNamePtrs[boxId];
		u32 i;

		for (i = 0; i < 9 && name[i] == sOld[i]; ++i);
		if (i == 9)
			for (i = 0; i < 9; ++i)
				name[i] = sNew[i];
		return name;
	}
	if (boxId >= TABLE_BOXES_COUNT && boxId < TOTAL_BOXES_COUNT)
	{
		u8* name = (u8*) HACK_STORAGE_BASE + BOX2_NAMES_OFF + 9 * (boxId - TABLE_BOXES_COUNT);
		//素の ResetPokemonStorageSystem（はじめから）は箱の番号を 2 桁で書くので、100 箱目だけ「ボックス?0」になる → 「ボックス100」に直す
		if (boxId == TOTAL_BOXES_COUNT - 1 && name[4] == 0xAC && name[5] == 0xA1 && name[6] == 0xFF)
		{
			name[4] = 0xA2; //1
			name[5] = 0xA1; //0
			name[6] = 0xA1; //0
			name[7] = 0xFF;
		}
		return name;
	}
	else if (boxId < TABLE_BOXES_COUNT)
		return (u8*) sPokemonBoxNamePtrs[boxId];
	else
		return NULL;
}

u8 GetBoxWallpaper(u8 boxId)
{
	if (boxId >= TABLE_BOXES_COUNT && boxId < TOTAL_BOXES_COUNT)
		return *((u8*) HACK_STORAGE_BASE + BOX2_NAMES_OFF + 9 * (TOTAL_BOXES_COUNT - TABLE_BOXES_COUNT) + (boxId - TABLE_BOXES_COUNT));
	else if (boxId < TABLE_BOXES_COUNT)
		return *sPokemonBoxWallpaperPtrs[boxId];
	else
		return 0;
}

void SetBoxWallpaper(u8 boxId, u8 wallpaperId)
{
	if (boxId >= TABLE_BOXES_COUNT && boxId < TOTAL_BOXES_COUNT && wallpaperId < WALLPAPER_COUNT)
		*((u8*) HACK_STORAGE_BASE + BOX2_NAMES_OFF + 9 * (TOTAL_BOXES_COUNT - TABLE_BOXES_COUNT) + (boxId - TABLE_BOXES_COUNT)) = wallpaperId;
	else if (boxId < TABLE_BOXES_COUNT && wallpaperId < WALLPAPER_COUNT)
		*(sPokemonBoxWallpaperPtrs[boxId]) = wallpaperId;
}

s8 sub_8091130(u8 boxId)
{
	u8 currentBox = StorageGetCurrentBox();
	u16 distance;

	u8 visible = BoxShelf_VisibleBoxes(); //[hack] FEAT-box100-unlock 51〜100 箱目が出るまでは 50 箱で数える

	if (boxId >= visible || currentBox >= visible)
		return 1;
	distance = boxId >= currentBox ? boxId - currentBox
		: boxId + visible - currentBox;

	return (distance < visible / 2) ? 1 : -1;
}

s16 sub_808B884(struct CompressedPokemon *boxMons, u8 currIndex, u8 maxIndex, u8 arg3)
{
	s16 i;
	s16 adder = -1;
	if (boxMons == NULL || maxIndex >= IN_BOX_COUNT || arg3 > 3
	|| (currIndex > maxIndex && currIndex != 0xFF))
		return -1;

	if (arg3 < 2)
		adder = 1;

	if (arg3 == 1 || arg3 == 3)
	{
		for (i = (s8)currIndex + adder; i >= 0 && i <= maxIndex; i += adder)
		{
			if (boxMons[i].substruct0.species != SPECIES_NONE)
				return i;
		}
	}
	else
	{
		for (i = (s8) currIndex + adder; i >= 0 && i <= maxIndex; i += adder)
		{
			struct BoxPokemon boxMon;
			CreateBoxMonFromCompressedMon(&boxMon, &boxMons[i]);
			if (boxMon.substruct0.species != SPECIES_NONE
			&& !GetBoxMonData(&boxMon, MON_DATA_IS_EGG, NULL))
				return i;
		}
	}

	return -1;
}

u8 SendMonToPC(struct Pokemon* mon)
{
	s32 boxNo, boxPos;

	//[hack] patches/0975 案 B リライブボックス: ダークの子は追加のセーブ領域の専用の箱だけ（使えない・満杯・書けないときは送れない）。PC のふつうの箱には入れない
	if (Relive_IsDark((struct BoxPokemon*) mon))
	{
		struct CompressedPokemon comp;
		u8 slot;

		CreateCompressedMonFromBoxMon((struct BoxPokemon*) mon, &comp);
		if (!Relive_PutComp(&comp, &slot))
			return MON_CANT_GIVE;
		HackRibbon_StoreFromMon((struct BoxPokemon*) mon); //[hack] patches/0977
		gSpecialVar_MonBoxId = 0;   //文の箱の名前は catching.c が専用の名前にする（PC の箱ではない）
		gSpecialVar_MonBoxPos = slot;
		return MON_GIVEN_TO_PC;   //「送る箱」の記録（VAR_PC_BOX_TO_SEND_MON）は動かさない＝ふつうの子の送り先はそのまま
	}

	SetPCBoxToSendMon(VarGet(VAR_PC_BOX_TO_SEND_MON));

	boxNo = StorageGetCurrentBox();

	do
	{
		//[hack] patches/0196 次のレポートに収まらない箱は「いっぱい」と同じ・patches/0732 空きは箱ごとにまとめて探す（1 匹ずつ読むと遅い）
		boxPos = BoxShelf_CanTouchBox(boxNo) ? BoxShelf_FindEmptyPos(boxNo) : -1;
		if (boxPos >= 0)
		{
			CreateCompressedMonFromBoxMon((struct BoxPokemon*) mon, MonPtrForWrite(boxNo, boxPos));
			gSpecialVar_MonBoxId = boxNo;
			gSpecialVar_MonBoxPos = boxPos;
			if (GetPCBoxToSendMon() != boxNo)
				FlagClear(FLAG_SHOWN_BOX_WAS_FULL_MESSAGE);
			VarSet(VAR_PC_BOX_TO_SEND_MON, boxNo);
			return MON_GIVEN_TO_PC;
		}

		boxNo++;
		if (boxNo >= BoxShelf_VisibleBoxes()) //[hack] FEAT-box100-unlock 1〜50 箱目が満杯になるまで 51〜100 箱目へは送らない
			boxNo = 0;

	} while (boxNo != StorageGetCurrentBox());

	return MON_CANT_GIVE;
}


bool8 SendMonToBoxPos(struct Pokemon* mon, u8 boxNo, u8 boxPos)
{
	if (boxNo >= TOTAL_BOXES_COUNT || boxPos >= IN_BOX_COUNT)
		return TRUE;
	if (!Relive_BoxAccepts(boxNo, (struct BoxPokemon*) mon))
		return TRUE;   //[hack] patches/0975 案 B: ダークは PC のどの箱にも入れない（台本の「箱へ送る」も同じ）

	struct CompressedPokemon* checkingMon = MonPtrForWrite(boxNo, boxPos); //[hack] patches/0196
	CreateCompressedMonFromBoxMon((struct BoxPokemon*) mon, checkingMon);
	HackRibbon_StoreFromMon((struct BoxPokemon*) mon); //[hack] patches/0977
	gSpecialVar_MonBoxId = boxNo;
	gSpecialVar_MonBoxPos = boxPos;
	if (GetPCBoxToSendMon() != boxNo)
		FlagClear(FLAG_SHOWN_BOX_WAS_FULL_MESSAGE);
	VarSet(VAR_PC_BOX_TO_SEND_MON, boxNo);
	return FALSE;

}

u32 SummaryScreenBoxMonMultiplier(u8 index)
{
	return index * sizeof(struct CompressedPokemon);
}

void BackupPartyToTempTeam(u8 firstId, u8 numPokes)
{
	if (firstId >= PARTY_SIZE || numPokes > PARTY_SIZE - firstId)
		return;
	struct CompressedPokemon* backup = gTempTeamBackup;

	for (int i = 0; i < numPokes; ++i)
		CreateCompressedMonFromBoxMon((struct BoxPokemon*) &gPlayerParty[firstId + i], &backup[firstId + i]);
}

void RestorePartyFromTempTeam(u8 firstId, u8 numPokes)
{
	if (firstId >= PARTY_SIZE || numPokes > PARTY_SIZE - firstId)
		return;
	struct CompressedPokemon* backup = gTempTeamBackup;

	for (int i = 0; i < numPokes; ++i)
	{
		Memset(&gPlayerParty[firstId + i], 0, sizeof(struct Pokemon)); //Wipe old data - mainly for wiping struct data that's not included in Box mon *IMPORTANT
		CreateBoxMonFromCompressedMon((struct BoxPokemon*) &gPlayerParty[firstId + i], &backup[firstId + i]);
		CalculateMonStats(&gPlayerParty[firstId + i]);
	}
}

u16 __attribute__((long_call)) PSS_TryLoadMonIconTiles(u16 species);
void __attribute__((long_call)) LoadCursorMonSprite(void);
void __attribute__((long_call)) RefreshCursorMonData(void);
void PlaceBoxMonIcon(u8 boxId, u8 position)
{
    if (boxId >= TOTAL_BOXES_COUNT) //Party mon
    {
        gPSSData->partySprites[position] = gPSSData->movingMonSprite;
        gPSSData->partySprites[position]->oam.priority = 1;
        gPSSData->partySprites[position]->subpriority = 12;
    }
    else
    {
		u16 species = GetBoxMonDataAt(boxId, position, MON_DATA_SPECIES2);
	
		#if (defined SPECIES_HOOPA || defined SPECIES_SHAYMIN)
		//Try an instant sprite change for post placing Hoopa-Unbound or Shaymin-Sky in the PC
		if (species == SPECIES_HOOPA || species == SPECIES_SHAYMIN)
		{
			u16 tileNum = PSS_TryLoadMonIconTiles(species); //Gets the tile number of the mon icon
			if (tileNum != 0xFFFF)
			{
				//Update mon Icon
				gPSSData->movingMonSprite->oam.tileNum = tileNum;
				gPSSData->movingMonSprite->oam.paletteNum = IndexOfSpritePaletteTag(0xDAC0 + GetMonIconPaletteIndexFromSpecies(species));
			}

			//Update front sprite
			u32 otId = GetBoxMonDataAt(boxId, position, MON_DATA_OT_ID);
			gPSSData->cursorMonSpecies = species;
			gPSSData->cursorMonPalette = GetMonSpritePalFromSpeciesAndPersonality(gPSSData->cursorMonSpecies, otId, gPSSData->cursorMonPersonality);
			RefreshCursorMonData();
		}
		#endif

        gPSSData->boxMonsSprites[position] = gPSSData->movingMonSprite;
        gPSSData->boxMonsSprites[position]->oam.priority = 2;
        gPSSData->boxMonsSprites[position]->subpriority = 19 - (position % IN_BOX_ROWS);
    }

    gPSSData->movingMonSprite->callback = SpriteCallbackDummy;
    gPSSData->movingMonSprite = NULL;
}

void FixItemNameInPokemonStorageSystem(void)
{
	const u8* name = ItemId_GetName(gPSSData->cursorMonItem);
	u8 length = StringLength(name);

	StringCopyPadded(gPSSData->cursorMonTexts[3], name, CHAR_SPACE, 14);
	if (length >= 14) //Too long to look nice
	{
		gPSSData->cursorMonTexts[3][12] = CHAR_ELLIPSIS; //End with trailing ...
		gPSSData->cursorMonTexts[3][13] = EOS;
	}
}
