#pragma once
#include "../include/global.h"

/*
    しばり 縛り F（6 本・0.63.0・docs/research/2026-11-03_challenge-f.md・旗の台帳 docs/research/flag-ledger.md）。
    せっていの「しばり」フォルダの 仮の行（値をもつ・ふういんルーレット／ぜんぶレントラーと同じ作り）。既定は全部 OFF。保存枠（filler_B20）・MENUITEM／order は使わない。

      さかさバトル   全部の戦いで タイプ相性が逆（戦闘に入るとき FLAG_INVERSE を立てる・戦闘が終わると end_battle.c の gEndBattleFlagClearTable が下ろす）  … 担当 A（この枠）
      しばりボーナス ON の しばり 1 つにつき トレーナー戦の賞金 +10% ／ バッジを取るたびに ON の数に応じた道具                                … 担当 A（この枠）
      めかくしバトル 相手の名前・Lv・HP バーを ？？？                                                                                          … 担当 B
      いっぱつしょうぶ ボールが外れたら 野生が逃げる                                                                                        … 担当 C
      ローテーション 戦いごとに 最初に出す子を 1→2→3… と回す（番号は旗の 3 ビット）                                                          … 担当 C
      わざガチャ     戦いが始まるたび 技が 全部の技から ランダム 4 つ                                                                          … 担当 C

    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: モンスター採掘・おめんの印（0xFE9）の次 → **0xFEA〜0xFFF（ここ）** → 次の空きは 0x1000 より上は台本の割り当て窓（build_hoenn_scripts.HOENN_FLAG_BASE）なので 使わない。
    変数・セーブの欄は増やさない（旗だけ）。旧版のセーブは旗 0＝OFF で読める・新しい版のセーブを旧版で開いても旗が立っているだけで壊れない。
*/
#define HACK_FLAG_CHF_INVERSE    0xFEA   /* さかさバトル: 全部の戦いが さかさバトル（タイプ相性が逆） */
#define HACK_FLAG_CHF_BONUS      0xFEB   /* しばりボーナス: ON の しばり 1 つにつき トレーナー戦の賞金 +10%・バッジを取るたびに ON の数に応じた道具（OFF にしたことがあると もらえない） */
#define HACK_FLAG_CHF_BLIND      0xFEC   /* めかくしバトル: 相手の名前・レベル・HP バーが ？？？ */
#define HACK_FLAG_CHF_ONESHOT    0xFED   /* いっぱつしょうぶ: ボールが外れたら 野生が逃げる（マスターボールは必ず捕まる・台本の野生は対象外） */
#define HACK_FLAG_CHF_ROTATION   0xFEE   /* ローテーション: 戦いごとに 最初に出る子を 1→2→3… と回す（手持ちの並びは動かさない） */
#define HACK_FLAG_CHF_GACHA      0xFEF   /* わざガチャ: 戦いが始まるたび 自分の技が 全部の技から ランダム 4 つ（その戦いだけ） */
#define HACK_FLAG_CHF_ROT_BIT0   0xFF0   /* ローテーション: 次に最初に出す子の番号（0〜5）の 3 ビット。bit0 */
#define HACK_FLAG_CHF_ROT_BIT1   0xFF1   /* bit1 */
#define HACK_FLAG_CHF_ROT_BIT2   0xFF2   /* bit2 */
#define HACK_FLAG_CHF_BONUS_CNT0 0xFF3   /* しばりボーナス: ごほうびを払ったバッジの数（0〜63）の 6 ビット。bit0（bit5 は 0xFF8）。数えるのは GetAllBadgeCount（カントー・ホウエン・ジョウト・シンオウの合計） */
#define HACK_FLAG_CHF_BONUS_CNT5 0xFF8
#define HACK_FLAG_CHF_SPARE0     0xFF9   /* 予備（担当 B／C が 戦いの最中の印などで要るとき使う・使ったら台帳 docs/research/flag-ledger.md に書く）〜 0xFFF */
#define HACK_FLAG_CHF_SPARE_LAST 0xFFF

#define HACK_CHF_COUNT           6       /* せっていの行になる 縛り F の数（最後のバッジの数え方・Syakakiri の「全部」） */
#define HACK_CHF_BONUS_PERCENT   10      /* しばりボーナス: ON の しばり 1 つにつき トレーナー戦の賞金に足す % */

bool8 HackChallengeF_On(void);                  /* 縛り F の どれかの旗が立っているか（AnyChallengeOn） */
u8 HackChallengeF_CountOn(void);                /* 立っている旗の数（0〜6・最後のバッジの数え方） */
bool8 HackChallengeF_Inverse(void);             /* さかさバトルの旗 */
bool8 HackChallengeF_Blind(void);               /* めかくしバトルの旗（担当 B が読む） */
bool8 HackChallengeF_Oneshot(void);             /* いっぱつしょうぶの旗（担当 C が読む） */
bool8 HackChallengeF_Rotation(void);            /* ローテーションの旗（担当 C が読む） */
bool8 HackChallengeF_Gacha(void);               /* わざガチャの旗（担当 C が読む） */
u8 HackChallengeF_RotationGet(void);            /* ローテーションの番号（旗の 3 ビット・0〜7）。担当 C が読み書きする入口 */
void HackChallengeF_RotationSet(u8 index);

void HackChallengeF_OnBattleStart(void);        /* challenge_c_hooks.s の HackChallengeC_BattleStartHook（CreateBattleStartTask の入口）から: さかさバトルの FLAG_INVERSE を立てる */
u32 HackChallengeF_PrizeBonus(u32 money);       /* multi.c の PrizeForOneTrainer の最後: しばりボーナスの賞金（ON の数 × 10% を足す・上限は呼ぶ側で止める） */
u8 HackChallengeF_BonusCount(void);             /* しばりボーナスで数える「ほかに ON の しばり」の数（ボーナス自身は数えない・BROKEN なら 0） */
void HackChallengeF_BonusSyncPaid(void);        /* 払った数を今のバッジの数にそろえる（ボーナスを ON にする ほぞんの直前・0.63.1） */
void HackChallengeF_FieldTick(void);            /* hack_challenge.c の HackChallenge_FieldTick から（16 コマごと）: 新しいバッジを見つけたら台本を始める */
void HackChallengeF_BonusGiveA(void);           /* callasm（台本 EventScript_ChallengeBonus）: ふしぎなアメ。LASTRESULT 1 = 渡した（gStringVar1 = 道具の名前・gStringVar2 = 数） */
void HackChallengeF_BonusGiveB(void);           /* callasm: ポイントアップ／ポイントマックス。LASTRESULT 1 = 渡した */
