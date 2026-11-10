#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 3b（patches/0978）: ポロックを食べさせる演出画面（src/hack_pokeblock_feed.c・原文 pokeblock_feed.c）。
//  使う側は defines.h の後・hack_condition_graph.h（CONDITION_COUNT）と hack_pokeblock.h の後に読む。
//  あげる画面（hack_pokeblock_use.c）→ 暗転 → 演出 → あげる画面を作り直して結果のゲージ、の「画面をまたぐ持ち物」を 1 つの入れ物にまとめる。
//  原文は gPokeblockMonId・gPokeblockGain・sPokeblock・sExitCallback の 4 つのグローバルで渡すが、書き換え可能な static は ROM に置かれる（6.13）ので
//  Alloc した入れ物 1 つ＋ポインタの語 0x203E038（ポロックケース・あげる画面と共用＝大きな画面は同時に開かない）に持つ。
//  あげる画面が開いている間は語は UsePokeblockMenu、演出の間は PokeblockCarry を指す（演出中は他の画面は開かない）。

#define PBF_STATE_PTR 0x203E038

struct PokeblockCarry
{
	struct Pokeblock pokeblock;                  //食べさせたポロックの写し（色の絵・名前・文の選びに使う）
	MainCallback exitCallback;                   //あげる画面を閉じたあとの戻り先（ケースを開き直す）
	void* feed;                                  //演出画面の入れ物（struct PokeblockFeed・hack_pokeblock_feed.c の中だけで使う）
	s16 gain;                                    //ポロックの味とせいかくの相性の合計（> 0 好き・< 0 きらい・原文 gPokeblockGain）
	u8 partyId;                                  //食べた子の gPlayerParty の番号（原文 gPokeblockMonId）
	u8 selection;                                //あげる画面で選んでいた番号（たまご抜き・戻ったとき同じ子にカーソルを置く）
	u8 before[CONDITION_COUNT];                  //食べる前のコンディション 5 つ（あげる画面のゲージの並び）
	u8 after[CONDITION_COUNT];                   //食べたあと
	u8 enhancements[CONDITION_COUNT];            //after - before（符号つきで読む）
	u8 sheenBefore;                              //食べる前のけづや（星の数）
};

//演出画面を始める。carry を語（PBF_STATE_PTR）に置いてから呼ぶ。終わったら CB2_PokeblockUseReturn（hack_pokeblock_use.c）へ
void PokeblockFeed_Start(void);
void CB2_PokeblockUseReturn(void);   //演出のあと、あげる画面を作り直して結果を見せる（hack_pokeblock_use.c）
