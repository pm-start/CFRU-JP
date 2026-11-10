#pragma once

#include "../global.h"

/**
 * \file pokemon_storage_system.h
 * \brief Contains functions relating to viewing or modifying data in the PC. It also
 *		  helps expand the total number of available boxes past the original 14. It does
 *		  this mainly be compressing the Pokemon data structure down to 65 bytes. Contest
 *		  stats, ribbons, unused bytes, and fateful encounter are all removed in the
 *		  processed. Thus, these stats can no longer be used for anything permanent, but
 *		  can be used as long as the player doesn't deposit their Pokemon (temporary stats).
 */


struct __attribute__((packed)) CompressedPokemonSubstruct0
{
    u16 species;
    u16 heldItem;
    u32 experience;
    u8 ppBonuses;
    u8 friendship;
	u8 pokeball;
	//u8 unknown; //Removed to save space for box mon
};

struct __attribute__((packed)) CompressedPokemon
{
	u32 personality;
	u32 otid;
	u8 nickname[9];
	u8 teratype;
	u8 language;
	u8 sanity;
	u8 otname[7];
	u8 markings;

//Substructure Growth
    struct CompressedPokemonSubstruct0 substruct0;
	u32 move1 : 10;
	u32 move2 : 10;
	u32 move3 : 10;
	u16 move4 : 10;

//Substructure Condition
    u8 hpEv;
    u8 atkEv;
    u8 defEv;
    u8 spdEv;
    u8 spAtkEv;
    u8 spDefEv;

//Substructure Misc
 /* 0x00 */ u8 pokerus;
 /* 0x01 */	u8 metLocation;
 /* 0x02 */ u16 metInfo; //Met level, met game, OT gender
 /* 0x04 */ u32 ivs;

//[hack] patches/0430 L 案（docs/research/2026-10-16_mon-data-l-format.md）: 末尾に ext（出会った場所の上位 8 ビット・出会ったレベルの 8 ビット目・技 1〜4 の 11 ビット目・bit13〜15 = 出会ったレベルの 9〜11 ビット目）。
//  保存・写し・入れ替えで失わないための欄（値の読み書きは GetBoxMonData/SetBoxMonData 側）。58 バイトまでの並びは 1 バイトも動かさない
 u16 ext;
 //[hack] patches/0583 W 案: 技 1〜4 の bit10〜15 を 6 ビットずつ詰める（little endian の 24 ビット）。
 // ext の bit9〜12 は旧 L 案の技 bit10 なので W 案では常に 0。先頭 60 バイトは動かさない。
 u8 moveHi[3];

//[hack] patches/0680 X 案（箱の努力値・個体値を 16 ビットに・stat16 WP4）: 先頭 63 バイトは W 案と 1 バイトも違わない（#70 の ID の入れ物・ボールの 6 本目も同じ位置）。
// 後ろに 15 バイト足す。努力値 = hpEv〜spDefEv（下位 8 ビット）| evHi[i] << 8（0〜65535）。
// 個体値 = ivs の 5 ビット × 6（下位）| ivHi の 11 ビット × 6（little endian・66 ビット・上の 6 ビットは予約で 0）<< 5（0〜65535）。
// 手持ち・80 バイト形は太らせない（SaveBlock1 がぴったり）ので、箱から出すとき 255／31 に頭打ちする（今の上限 252／31 では起きない）。
 u8 evHi[6];
 u8 ivHi[9];
}; //SIZE = 0x4E / 78 bytes


//Exported Functions
u32 GetBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request);
void SetBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request, const void* value);
void GetBoxMonNickAt(u8 boxId, u8 boxPosition, u8* dst);
void SetBoxMonNickAt(u8 boxId, u8 boxPosition, const u8* nick);
u32 GetAndCopyBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request, void* dst);
void SetBoxMonAt(u8 boxId, u8 boxPosition, struct BoxPokemon* src);
void CreateBoxMonAt(u8 boxId, u8 boxPosition, u16 species, u16 level, u8 fixedIV, u8 hasFixedPersonality, u32 personality, u8 otIDType, u32 otID);
void ZeroBoxMonAt(u8 boxId, u8 boxPosition);
void BoxMonAtToMon(u8 boxId, u8 boxPosition, struct Pokemon *dst);
struct BoxPokemon* GetBoxedMonPtr(u8 boxId, u8 boxPosition);
u8 SendMonToBoxPos(struct Pokemon* mon, u8 boxNo, u8 boxPos);
void BackupPartyToTempTeam(u8 firstId, u8 numPokes);
void RestorePartyFromTempTeam(u8 firstId, u8 numPokes);

//Functions Hooked In
u8* GetBoxNamePtr(u8 boxId);
u8 GetBoxWallpaper(u8 boxId);
void SetBoxWallpaper(u8 boxId, u8 wallpaperId);
s8 sub_8091130(u8 boxId);
u8 SendMonToPC(struct Pokemon* mon);
