#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 6（patches/0978）: ホウエンのサファリゾーンのポロックおき場・出会い・戦闘の「ポロックをなげる」（src/hack_safari_pokeblock.c）。
//  原文: 参考/pokeemerald/src/safari_zone.c・src/pokeblock.c（UseInBattle／UseOnPokeblockFeeder）・src/battle_util.c（HandleAction_ThrowPokeblock）・src/wild_encounter.c（PickWildMonNature）。
//  使う側は defines.h（struct Pokeblock）の後に読む。

//床の印（おき場）。素の FR の全タイルセットで 0xEB〜0xEF は 0 マス（tools/rom/verify_hoenn_behaviors.py が見る）・build_emerald_tileset.EXTRA_BEHAVIOR_MAP の 0x87（EM MB_POKEBLOCK_FEEDER）の寄せ先。0xEB＝引き戸・0xEC＝森の入口・0xED＝水の南矢印が使用中
#define HOENN_MB_POKEBLOCK_FEEDER 0xEE

//おき場の置き場（EWRAM の固定番地・10 台×16 バイト＝160 バイト・0x0203F048〜0x0203F0E7）。ヒープは地図の読み込みで作り直される（docs/research/2026-11-03_perf-region-2.md）のでサファリの地図をまたぐ状態には使えない。
//  空き 0x0203E274〜0x0203F0E8 の末尾にちょうど収まる（RAM 台帳 docs/research/flag-ledger.md・tools/rom/verify_safari_pokeblock.py が番地の重なりを見る）
#define HACK_FEEDERS_ADDR 0x0203F048
#define HACK_NUM_FEEDERS 10
#define HACK_FEEDER_STEPS 100          //原文 SafariZoneActivatePokeblockFeeder の stepCounter
#define HACK_FEEDER_RANGE 5            //原文 GetPokeblockFeederWithinRange（|dx|+|dy| <= 5）
#define HACK_FEEDER_NATURE_PERCENT 80  //原文 PickWildMonNature（Random() % 100 < 80）

struct HackFeeder                       //原文 struct PokeblockFeeder と同じ並び（0x10 バイト）
{
	s16 x;
	s16 y;
	s8 mapNum;
	u8 stepCounter;
	u8 pad[2];
	struct Pokeblock pokeblock;         //+8・7 バイト
	u8 pad2;
};

//戦闘のアクション 6（FR の「エサだま」）に入れる引数。TwoReturnValues の 16 ビット＝色 << 8 | 反応（1 気になる・2 夢中・3 無視）。0 なら素の動き（カントーは常に 0）
#define HACK_PKBL_REACT_CURIOUS 1
#define HACK_PKBL_REACT_ENTHRALLED 2
#define HACK_PKBL_REACT_IGNORED 3

//ポロックをなげる演出（原文 B_ANIM_POKEBLOCK_THROW・段 6b）。gBattleAnims_General（assembly/data/special_battle_anim_table.s）の最後の台本 HACK_ANIM_POKEBLOCK_THROW の番号＝
//  素の FR（0x00〜0x3B）＋CFRU の追加（〜0x52 ANIM_AI_ITEM_HEAL）の次の 0x53。この番号は assembly/hoenn_safari/safari_pokeblock_battle.s の .equ PKBL_THROW_ANIM と 2 か所（表の足し方で動く値なので、
//  検算は値ではなく「表の最後の項目がこの台本」で見る）。カントーのエサ投げは素の FR の番号 4 のまま
#define HACK_PKBL_THROW_ANIM 0x53
//  絵・パレットの札（原文 ANIM_TAG_POKEBLOCK＝ANIM_SPRITES_START + 269＝FR の ANIM_TAG_SAFARI_BAIT と同じ数・同時には使わない・新しい札は作らない）
#define HACK_ANIM_TAG_POKEBLOCK 0x281D

void HackSafari_DecrementFeeders(void);                       //1 歩ごと（SafariZoneTakeStep・歩数制限なしの決定は変えない）
struct Pokeblock* HackSafari_GetActivePokeblock(void);        //原文 SafariZoneGetActivePokeblock（|dx|+|dy| <= 5 のおき場・無ければ NULL）
bool8 HackSafari_ActivateFeeder(const struct Pokeblock* pb);  //原文 SafariZoneActivatePokeblockFeeder（空きが無ければ何もしない・FALSE）
void HackSafari_ClearFeeders(void);                           //入口・出口で全部消す（callnative からも呼ぶ）
u8 HackSafari_BattleReaction(u8 nature, const struct Pokeblock* pb);   //原文 UseInBattle の 1／2／3
