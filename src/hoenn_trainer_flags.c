#include "defines.h"
#include "defines_battle.h"        /* gTrainerBattleOpponent_A */
#include "../include/event_data.h"

/*
hoenn_trainer_flags.c
    トレーナーフラグの置き場を広げる（やりたいこと #2・ホウエンのジム）。

    ★ 詳細は docs/research/2026-09-21_trainer-flag-space.md

    ## なぜ要るか

    素 FRLG のトレーナーフラグは **`0x500 + トレーナー番号`**。
    `0x800` からはシステムフラグ（`SYS_FLAGS`）なので、
    **767 番が上限**。素 FRLG が 742 番まで使っているので
    **空きは 25 枠しかない**。ホウエンの 8 ジムだけで 65 人要る。

    ## どう広げたか

        番号 0〜767    … 0x500 + 番号   （素のまま。カントーは一切変わらない）
        番号 768〜1791 … 0x1400 + (番号 - 768) ＝ 番号 + 0x1100（0x1400〜0x17FF）
        番号 1792〜2047 … 0x1900 + (番号 - 1792)（0x1900〜0x19FF・★ patches/0986 FEAT-johto-j0: ジョウトの旗の窓＝SaveBlock1 +0x3A94〜。それまでの式だと 0x1800〜0x18FF でレイド・実績の旗とぶつかった）
        番号 2048 以上 … 範囲の外（0x3FFF＝Var8000 へ逃がされる・★ FEAT-johto-p1: 0x1B00〜0x1CFF がジョウトの旗の窓 2 になったので印を 0x3FFF に移した）

    行き先の `0x1400`〜`0x18FF` は CFRU の**拡張フラグ**の一部
    （`src/save.c` の `GetExpandedFlagPointer` が 0x900〜0x18FF を見る）。
    0x18FF が拡張フラグの終わりなので **2047 番が新しい上限**。

    ★ ホウエンのイベント用フラグは `0x1000`〜`0x13FF`
      （`tools/rom/build_hoenn_scripts.py` の `HOENN_FLAG_BASE`）。
      **こことぶつからない**ように `0x1400` から始めてある。

    ★★ **素の番号（0〜767）はわざと動かしていない。**
      カントーの 743 人ぶんのフラグはセーブの同じ場所に残るので、
      素 FRLG のスクリプト・VS シーカー・ずかんナビの挙動は変わらない。
      「全部を拡張フラグへ移す」ほうが式は 1 本で済むが、
      素の 743 人を巻き込むぶん壊す範囲が大きい。

    ## 触る所

    トレーナーフラグを作っているのは、ゲーム全体で **9 箇所しか無い**
    （`参考/pokefirered` を `TRAINER_FLAGS_START` で検索して確認。
    VS シーカーもずかんナビも下の 3 関数を呼んでいるだけ）。

      ROM（`functionrewrites` で下の 4 つに差し替える）
        0807F7D8  GetTrainerAFlag        呼び元 3
        0807FB44  HasTrainerBeenFought   呼び元 6
        0807FB5C  SetTrainerFlag         呼び元 1
        0807FB70  ClearTrainerFlag       呼び元 1

      CFRU の C（`src/overworld.c` の 5 箇所。patches/0038）
        CheckTrainerSpotting / GetTrainerFlagFromScriptPointer /
        SetTrainerFlags / HasOneTrainerBeenDefeated

    ★ 呼び元の数は ROM の BL を数えた実測で、decomp の数と一致している。
*/

/* 素 FRLG の枠はここまで（`MAX_TRAINERS_COUNT`）*/
#define HOENN_TRAINER_ID_SPLIT   768
/* 768 番以降の行き先。拡張フラグ 0x1400〜0x18FF */
#define HOENN_TRAINER_FLAG_BASE  0x1400
/* ★ patches/0986（FEAT-johto-j0）: 番号 1792〜2047 は 0x1900〜0x19FF（ジョウトの旗の窓の前半＝SaveBlock1 +0x3A94〜）。768〜1791 は 0x1400〜0x17FF のまま
   （今ある番号は最大 1746・2032〜2041 のピラミッド・ヒルは別の旗＝既存セーブの旗は動かない） */
#define HOENN_TRAINER_ID_JOHTO   1792
#define HOENN_TRAINER_ID_LAST    2047
#define HOENN_TRAINER_FLAG_JOHTO 0x1900
#define HOENN_TRAINER_FLAG_INVALID 0x3FFF

u16 HoennTrainerFlag(u16 trainerId);
u16 HoennGetTrainerAFlag(void);
bool8 HoennHasTrainerBeenFought(u16 trainerId);
void HoennSetTrainerFlag(u16 trainerId);
void HoennClearTrainerFlag(u16 trainerId);

u16 HoennPyramid_TrainerFlag(u16 trainerId); /* src/hoenn_pyramid.c */
u16 HoennHill_TrainerFlag(u16 trainerId);    /* src/hoenn_trainer_hill.c */

u16 HoennTrainerFlag(u16 trainerId)
{
	u16 pyramidFlag = HoennPyramid_TrainerFlag(trainerId); //★ 2026-10-03 バトルピラミッドのトレーナー（0x7F0〜0x7F7）は一時フラグ 0x11〜0x18

	if (pyramidFlag != 0)
		return pyramidFlag;
	pyramidFlag = HoennHill_TrainerFlag(trainerId); //★ 2026-10-03 トレーナーヒルの 2 人（0x7F8・0x7F9）は旗 0x12F0 + 階 × 2 + 何人目
	if (pyramidFlag != 0)
		return pyramidFlag;
	if (trainerId < HOENN_TRAINER_ID_SPLIT)
		return FLAG_TRAINER_FLAG_START + trainerId;

	if (trainerId < HOENN_TRAINER_ID_JOHTO)
		return HOENN_TRAINER_FLAG_BASE + (trainerId - HOENN_TRAINER_ID_SPLIT);
	if (trainerId <= HOENN_TRAINER_ID_LAST)
		return HOENN_TRAINER_FLAG_JOHTO + (trainerId - HOENN_TRAINER_ID_JOHTO);   //★ patches/0986（FEAT-johto-j0）: 1792〜2047 は 0x1900〜0x19FF（SaveBlock1 +0x3A94〜・src/hack_johto_save.h）。0x1800〜0x186C はレイド・実績の旗なので使わない
	return HOENN_TRAINER_FLAG_INVALID;   //2048 以上は範囲の外（0x3FFF＝Var8000 へ逃がされる）
}

/* 素 0807F7D8 GetTrainerAFlag */
u16 HoennGetTrainerAFlag(void)
{
	return HoennTrainerFlag(gTrainerBattleOpponent_A);
}

/* 素 0807FB44 HasTrainerBeenFought */
bool8 HoennHasTrainerBeenFought(u16 trainerId)
{
	return FlagGet(HoennTrainerFlag(trainerId));
}

/* 素 0807FB5C SetTrainerFlag */
void HoennSetTrainerFlag(u16 trainerId)
{
	FlagSet(HoennTrainerFlag(trainerId));
}

/* 素 0807FB70 ClearTrainerFlag */
void HoennClearTrainerFlag(u16 trainerId)
{
	FlagClear(HoennTrainerFlag(trainerId));
}
