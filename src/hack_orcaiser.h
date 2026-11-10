#pragma once

/*
 * ★ 2026-11-02 コシャル・オルカイザー・ゲンシオルカイザー（Redmine #230・patches/0952・tools/custom_mon/orcaiser）
 * 入手は「きょがのカセキ（ちかつうろの化石ほり）を グレンの研究所で よみがえらせる」だけ。
 * 野生・トレーナー・ガチャ・ミラクル交換・チャレンジ C・ネット交換・シリアルコードには出さない（この印で除外する）。
 */
#define HACK_IS_ORCAISER_LINE(sp) ((sp) >= SPECIES_KOSHAL && (sp) <= SPECIES_GENSHIORCAISER)
/*
 * ★ 2026-11-08 カセキメラ 4 種（パチニクス・ダンクルオ・ステゴラドン・プレチルド＝やん之助 氏・patches/0968・tools/custom_mon/kasekimera）
 * 入手は「剣盾のかけら（とり・さかな・りゅう・くびなが／ちかつうろの化石ほり）1 つを グレンの研究所で よみがえらせる」だけ（2 つ組のパッチラゴン等より聞く順があと）。
 * 野生・トレーナー・ガチャ・ミラクル交換・チャレンジ C・ネット交換・シリアルコードには出さない（HACK_IS_ORCAISER_LINE と同じ除外）。
 */
#define HACK_IS_KASEKIMERA_LINE(sp) ((sp) >= SPECIES_PACHINIKUSU && (sp) <= SPECIES_PURECHIRUDO)
#define HACK_IS_FOSSIL_ONLY_MON(sp) (HACK_IS_ORCAISER_LINE(sp) || HACK_IS_KASEKIMERA_LINE(sp))

void HackFossil_FindNext(void);
void HackFossil_Revive(void);
void HackFossil_FindPair(void);
void HackFossil_RevivePair(void);
void HackFossil_FindKase(void);
void HackFossil_ReviveKase(void);
