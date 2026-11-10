#pragma once
#include "../include/global.h"

/*
    縛り「ふういんルーレット」（#123・docs/research/2026-10-17_fuuin-roulette.md）。せっていの「しばり」フォルダの 1 つ（既定は OFF）。

    手持ちの子のレベルが 10 の倍数をまたぐたび（Lv10・20・30…に達したとき・0.63.0 から。前は 1 レベルごと）に 1 回、ポケモン／どうぐ／わざ のどれか 1 つをランダムで封印する（戦闘のレベルアップは戦闘が終わってからまとめて・
    戦闘の外は フィールドに戻ったとき 1 回ずつ）。封印は OFF にするまで続き、OFF にすると全部とける（記録も消す）。
      ポケモン  封印された子はボックスに送られ、手持ちに戻っていても フィールドで見つけ次第 ボックスへ戻す（手持ちが 1 匹のときは選ばない）
      どうぐ    道具の種類ごと。バッグの「つかう」が使えない（ボール・たいせつなもの・ひでんマシンは選ばない）
      わざ      技の種類ごと。自分の側の子は戦闘で出せない（フィールドで使う技〈gFieldMoves〉は選ばない・全部封印されれば わるあがき）
    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: 縛り C の枠（〜0xFC4）の次 → **0xFC5（ここ）** → 0xFC8〜0xFCE はフロンティアの金の前口上の印（build_hoenn_scripts.LEDGER_FLAGS）・次の空きは 0xFD0（0xFCF は hack_tamper.h＝セーブの改造ありの印）（0xFC6 は hack_dummy_battle.h＝ポケセン 3F のダミー・patches/0602／0xFC7 は hack_challenge_luxray.h＝ぜんぶレントラー・patches/0609）。変数は増やさない。

    記録の置き場（セーブの形は変えない）: SaveBlock2 の filler_B20 の空き 136 バイト（+0x50〜0xCF の 128 バイト ＋ +0xF8〜0xFF の 8 バイト）を
    1 本の論理領域（0〜135）として読み書きする（HackFuuin の FuRd／FuWr）。前の版は ここを書かない（予備）＝古い版で開いて保存しても そのまま戻る。
      [0] 印 0xA7（無い／違う値は「記録なし」）  [1] 封印した子の数（最大 8）  [2] 道具の数（最大 24）  [3] 技の数（最大 24）
      [4〜35] 子の性格値（u32）× 8  [36〜83] 道具（u16）× 24  [84〜131] 技（u16）× 24  [132] レベルアップの未処理の回数  [133〜135] 予備
*/
#define HACK_FLAG_CHD_FUUIN      0xFC5
#define HACK_FUUIN_LEVEL_STEP    10      /* 何レベルごとに 1 つ封印するか（oldLevel/10 と newLevel/10 の差＝状態を持たない） */
#define HACK_CHD_COUNT           1       /* せっていの行になる縛り D の数（最後のバッジの数え方） */

#define HACK_FUUIN_BASE_A        0x50    /* filler_B20 の +0x50〜0xCF（128 バイト） */
#define HACK_FUUIN_BASE_B        0xF8    /* +0xF8〜0xFF（8 バイト）・論理 128〜135 */
#define HACK_FUUIN_LEN           136
#define HACK_FUUIN_MAGIC         0xA7
#define HACK_FUUIN_O_MON         4
#define HACK_FUUIN_O_ITEM        36
#define HACK_FUUIN_O_MOVE        84
#define HACK_FUUIN_O_PENDING     132
#define HACK_FUUIN_MAX_MON       8
#define HACK_FUUIN_MAX_ITEM      24
#define HACK_FUUIN_MAX_MOVE      24

bool8 HackFuuin_On(void);
bool8 HackFuuin_ItemSealed(u16 item);                  /* item.c 経由（hack_challenge.c の「つかう」）: 封印された道具か */
bool8 HackFuuin_MoveSealed(u8 bank, u16 move);         /* battle_util.c の IsUnusableMove・move_menu.c: 自分の側の戦闘で封印された技か */
void HackFuuin_OnLevelsGained(u8 oldLevel, u8 newLevel); /* exp.c（戦闘）・party_menu.c／exp_candy.c（アメ）: レベルが oldLevel → newLevel になった（10 の倍数をまたいだ数だけ数える） */
void HackFuuin_FieldTick(void);                        /* hack_challenge.c の HackChallenge_FieldTick から */
void HackFuuin_Spin(void);                             /* callasm（台本 EventScript_FuuinSpin）: LASTRESULT 0 空振り／1 ポケモン／2 どうぐ／3 わざ */
