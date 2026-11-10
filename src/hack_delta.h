#pragma once
#include "../include/global.h"

/*
 * デルタ種・ヌシの中身（段 B・FEAT-shadow-b・patches/0962・設計 work/tmp/audit/SHADOW_S0_MEASURE.md 6 章・NUSHI_DESIGN.md 3 章）。
 *
 * デルタ種（印の種類 HACK_KIND_DELTA）: タイプが変わる。ずかんの種族のタイプは変えない（個体のタイプだけ）。
 *   値（HackKind_Value）: 0 = 自動（種族から表で引く＝進化しても系統のタイプのまま）・1 以上 = タイプ番号 + 1 を明示（デバッグ・検算用の上書き・単タイプ）
 *   種族ごとの表: gHackDeltaTypes（生成物・tools/build/gen_delta_types.py・案 H）
 * ヌシ（HACK_KIND_NUSHI）: 能力値の計算で種族値を 11/10 倍（patches/0980・0962 では 13/10）（HP も・切り捨て・255 では頭打ちにしない＝種族値の入れ物は u16）。
 */

extern const u8 gHackDeltaTypes[][2];

//種族ごとのデルタのタイプ（slot 0／1。単タイプは同じ値）。範囲外・SPECIES_NONE は TYPE_NORMAL
u8 HackDelta_SpeciesType(u16 species, u8 slot);

//個体のタイプの窓口: 印がデルタなら delta のタイプ・そうでなければ base をそのまま返す（デバッグモンの口と同じ形・mon は struct Pokemon）
u8 HackDelta_Type(const struct Pokemon* mon, u16 species, u8 slot, u8 base);

//印を付ける口（段 A2 の付与の入口から 1 行で呼ぶ）: 種類 DELTA・値 0（自動）。typeOverride が 0xFF 以外ならそのタイプ（単タイプ）を明示
void HackDelta_Assign(struct BoxPokemon* mon, u8 typeOverride);

//ひこうタイプ扱い（段 B の追加・ユーザー決定 2026-11-08）: 元の種族のタイプ（baseType1／baseType2）にひこうがあるデルタ種か。
//  使うのは「地面に着いているか」と「じめん技の無効」だけ（弱点・耐性・タイプ一致は個体のデルタのタイプのまま）。印がデルタでなければ FALSE（ひこうそのものを持つ個体は今までの IsOfType／タイプ表が見る）
bool8 HackDelta_CoversFlying(const struct Pokemon* mon, u8 baseType1, u8 baseType2);

//同じことを手持ちの子の種族で（src/hack_delta_battle.c・戦闘の外の計算・AI の入れ替え予測用）
bool8 HackDelta_MonFlyingCover(const struct Pokemon* mon, u16 species);

//同じことを戦闘中の子で（src/hack_delta_battle.c）。テラスタルした子・へんしんしている子・はねやすめ中の子は FALSE（ひこうタイプと同じ条件）
bool8 HackDelta_BankFlyingCover(u8 bank);

//ヌシの種族値の倍率（倍率の本体はここの 2 定数だけ・patches/0980 で 13/10→11/10）
#define HACK_NUSHI_MUL 11
#define HACK_NUSHI_DIV 10
//印がヌシなら base * MUL / DIV・そうでなければ base のまま
u32 HackNushi_Base(const struct Pokemon* mon, u32 base);

//手持ちにヌシが 1 匹だけ（patches/0981）: mon がヌシで party（exceptSlot 以外）にもヌシがいれば TRUE。exceptSlot=PARTY_SIZE で全部を見る
bool8 HackNushi_PartyHasOther(const struct Pokemon* party, const struct Pokemon* mon, u8 exceptSlot);
//手持ち（exceptSlot 以外）にヌシがいれば真（patches/0981）
bool8 HackNushi_PartyHasNushi(const struct Pokemon* party, u8 exceptSlot);
