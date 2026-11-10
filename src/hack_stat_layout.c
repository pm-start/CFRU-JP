#include "defines.h"
#include "../include/new/pokemon_storage_system.h"
#include "hack_stat_caps.h"

/*
 * セーブの形・素の機械語が決め打ちで読む配置の守り（[hack] patches/0677・設計 docs/research/2026-10-23_stat-level-16bit.md WP1〜3）
 *
 * 努力値・個体値・種族値・レベルの入れ物を 16 ビットにする作業（WP1〜3）は、手持ち・80 バイト形・戦闘の構造体の大きさと位置を変えない。
 * ここが崩れたら（誰かが構造体を太らせたら）コンパイルが止まる。機械語は生成されない（typedef だけ）。
 */

#define HACK_OFFSETOF(type, field) ((u32) __builtin_offsetof(type, field))

//手持ち 100 バイト・80 バイト形（SaveBlock1 がぴったりなので太らせられない）
typedef char StatLayoutPokemonSize[(sizeof(struct Pokemon) == 100) ? 1 : -1];
typedef char StatLayoutBoxPokemonSize[(sizeof(struct BoxPokemon) == 80) ? 1 : -1];
typedef char StatLayoutPokemonEv[(HACK_OFFSETOF(struct Pokemon, hpEv) == 0x38 && HACK_OFFSETOF(struct Pokemon, spDefEv) == 0x3D) ? 1 : -1];
typedef char StatLayoutPokemonLevel[(HACK_OFFSETOF(struct Pokemon, level) == 0x54 && HACK_OFFSETOF(struct Pokemon, pokerus_timer) == 0x55) ? 1 : -1];
typedef char StatLayoutPokemonStats[(HACK_OFFSETOF(struct Pokemon, hp) == 0x56 && HACK_OFFSETOF(struct Pokemon, spDefense) == 0x62) ? 1 : -1];

//戦闘の構造体（素の機械語が × 0x58 と +0x2A を決め打ちで読む）
typedef char StatLayoutBattleMonSize[(sizeof(struct BattlePokemon) == 0x58) ? 1 : -1];
typedef char StatLayoutBattleMonLevel[(HACK_OFFSETOF(struct BattlePokemon, levelLo) == 0x2A && HACK_OFFSETOF(struct BattlePokemon, friendship) == 0x2B) ? 1 : -1];
typedef char StatLayoutBattleMonPad3A[(HACK_OFFSETOF(struct BattlePokemon, ability) == 0x38 && HACK_OFFSETOF(struct BattlePokemon, pad3A) == 0x3A && HACK_OFFSETOF(struct BattlePokemon, ppBonuses) == 0x3B) ? 1 : -1];   //+0x30〜+0x3A は素の nickname[11]（素の機械語が丸ごと書く）＝ability（CFRU が後から上書き）と pad3A 以外の欄を置かない（patches/0796）

//箱の保存形 X（78 バイト・[hack] patches/0680）: 先頭 63 バイトは W と同じ並び・後ろに evHi[6]・ivHi[9]
typedef char StatLayoutCompressedSize[(sizeof(struct CompressedPokemon) == 78) ? 1 : -1];
typedef char StatLayoutCompressedWTail[(HACK_OFFSETOF(struct CompressedPokemon, moveHi) == 0x3C && HACK_OFFSETOF(struct CompressedPokemon, evHi) == 0x3F && HACK_OFFSETOF(struct CompressedPokemon, ivHi) == 0x45
	&& HACK_OFFSETOF(struct CompressedPokemon, hpEv) == 0x2C && HACK_OFFSETOF(struct CompressedPokemon, ivs) == 0x36 && HACK_OFFSETOF(struct CompressedPokemon, ext) == 0x3A) ? 1 : -1];
typedef char StatLayoutTempTeamBackup[(6 * sizeof(struct CompressedPokemon) <= 0x0203E300 - 0x0203E118) ? 1 : -1];   //gTempTeamBackup（0x0203E118〜）が BGM のトラック（0x0203E300〜）の手前に収まる

//個体の印（[hack] patches/0958・src/hack_kind.c が生の位置で読み書きする）: 80 バイト形の unknown（+0x1E）・substruct0.unknown（+0x2B）・箱の縮めた形の ext（+0x3A）・ivHi[8]（+0x4D）
typedef char StatLayoutKindBox[(HACK_OFFSETOF(struct BoxPokemon, unknown) == 0x1E && HACK_OFFSETOF(struct BoxPokemon, substruct0) + 11 == 0x2B && sizeof(((struct BoxPokemon*) 0)->substruct0) == 12) ? 1 : -1];
typedef char StatLayoutKindComp[(HACK_OFFSETOF(struct CompressedPokemon, ext) == 0x3A && HACK_OFFSETOF(struct CompressedPokemon, ivHi) + 8 == 0x4D && sizeof(struct CompressedPokemon) == 0x4E) ? 1 : -1];

//種族値の表の行（素の機械語が lsl #5 で 19 か所使う）
typedef char StatLayoutBaseStatsSize[(sizeof(struct BaseStats) == 0x20) ? 1 : -1];

//種族値の並列表 gBaseStatsWide の並び（HP・こうげき・ぼうぎょ・すばやさ・とくこう・とくぼう）は gBaseStats の先頭 6 バイトと同じ（[hack] patches/0678）
typedef char StatLayoutBaseStatsOrder[(HACK_OFFSETOF(struct BaseStats, baseHP) == 0 && HACK_OFFSETOF(struct BaseStats, baseAttack) == 1 && HACK_OFFSETOF(struct BaseStats, baseDefense) == 2
	&& HACK_OFFSETOF(struct BaseStats, baseSpeed) == 3 && HACK_OFFSETOF(struct BaseStats, baseSpAttack) == 4 && HACK_OFFSETOF(struct BaseStats, baseSpDefense) == 5) ? 1 : -1];
