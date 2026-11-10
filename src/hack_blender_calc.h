#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 4a（patches/0978）: きのみブレンダーの「計算と結果」（src/hack_blender_calc.c・原文 berry_blender.c の
//  CalculatePokeblock／CalculatePokeblockColor／AreBlenderBerriesSame／SetOpponentsBerryData／ConvertItemToBlenderBerry／TryUpdateBerryBlenderRecord）。
//  ミニゲームの画面（段 4b）とスキップ（段 4c）の両方がここを通る。画面を持たない・書き換え可能な static を持たない（6.13）。
//  使う側は defines.h（struct Pokeblock＝global.h）と hack_pokeblock.h の後に読む。

#define BLENDER_MAX_PLAYERS 4              //原文の MAX_LINK_PLAYERS
#define BLENDER_FLAVORS 6                  //からい・しぶい・あまい・にがい・すっぱい（FLAVOR_*）＋なめらかさ（feel）
#define BLENDER_NPC_BERRIES 5              //NPC が使うきのみは Cheri〜Aspear の 5 種（原文 NUM_NPC_BERRIES）

struct BlenderBerry                        //原文の struct BlenderBerry から名前を除いた（名前は道具の名前の表から画面側が引く）
{
	u16 itemId;
	u8 flavors[BLENDER_FLAVORS];
};

//きのみの道具番号（ITEM_CHERI_BERRY〜ITEM_ENIGMA_BERRY の 43 個）かどうか。CFRU が足した戦闘用のきのみ（オッカなど）は味の表が無い＝FALSE
bool8 HackBlender_IsBlendableBerry(u16 itemId);
//berry に道具の味を入れる。FALSE = 表に無いきのみ（berry は変えない）
bool8 HackBlender_ConvertItem(struct BlenderBerry* berry, u16 itemId);
//NPC の分のきのみを berries[1..players-1] に入れる（berries[0] はプレイヤーで、先に入れておく）。master = 名人（NPC が 1 人で名人のきのみを使う）
void HackBlender_SetOpponentBerries(u16 playerItemId, u8 players, bool8 master, struct BlenderBerry* berries);
//ポロックを作る。flavors は 6 つ（原文 CalculatePokeblock の出力・画面側が使う）。players は 2〜4
void HackBlender_Calculate(const struct BlenderBerry* berries, struct Pokeblock* pokeblock, u8 players, u8* flavors, u16 maxRPM);

//スキップ（ミニゲームを飛ばす）のときの「腕前ふつう」の最高の回転数。players は 2〜4。master＝名人の台（2 人・NPC が必ず Best）のときは名人の値（15748）。値は tools/pokeblock/sim_blender_npc.py の平均（根拠は patches/0978）
u16 HackBlender_SkipMaxRPM(u8 players, bool8 master);

//ブレンダーの記録（1・2・3 人用の最高の回転数）を、大きいときだけ更新する（原文 TryUpdateBerryBlenderRecord）。players は 2〜4
void HackBlender_TryUpdateRecord(u8 players, u16 maxRPM);

//できたポロックをケースに入れて、使ったきのみを 1 個外す。FALSE = 入れられない（ケースがいっぱい・書けない）／きのみを外せない＝何も変えない
//  原文は RemoveBagItem のあとに AddPokeblock（空きは台本が先に確かめる）。ここは「ポロックを失ったのにきのみが減らない／きのみだけ減る」が起きない順（足す→外す→失敗なら戻す）
bool8 HackBlender_StoreAndConsume(const struct Pokeblock* pokeblock, u16 playerItemId);
//ケースに空きがあるか（満杯・使えない版は FALSE）
bool8 HackBlender_CaseHasRoom(void);
