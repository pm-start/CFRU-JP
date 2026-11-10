#pragma once
#include "../include/global.h"

/*
    しばり（#118・せってい の「しばり」フォルダ・既定は全部 OFF）。docs/research/2026-10-16_settings-folders-challenges.md

    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: わたしたちの伝説（F90 番から 20 個）の次 → **FA4〜FAC 番（ここ）** → 次の空き 0xFAD。
    変数は増やさない（入るたびの印は一時変数 0x400F）。
*/
#define HACK_FLAG_CH_NOITEMS     0xFA4   /* どうぐきんし: 戦闘で どうぐ（ボール以外）・フィールドで かいふくの どうぐ が つかえない */
#define HACK_FLAG_CH_NOHELD      0xFA5   /* もちものきんし: 自分のポケモンの もちものは 戦闘で こうか なし（姿を変えるものは のぞく） */
#define HACK_FLAG_CH_CENTERFEE   0xFA6   /* ポケセンりょうきん: 入るたびに 100 円 */
#define HACK_FLAG_CH_PCFEE       0xFA7   /* PC りょうきん: パソコン・ボックスリンクを つかうたびに 500 円 */
#define HACK_FLAG_CH_LENTRANT    0xFA8   /* ポケセンの ？？？: ポケセンに入るたびに ？？？（レントラー）と戦う */
#define HACK_FLAG_CH_BATTLE      0xFA9   /* ？？？との戦いの最中の印（戦闘が終わる所で下ろす・end_battle.c の gEndBattleFlagClearTable） */
#define HACK_FLAG_CH_WHITEOUT    0xFAA   /* 全滅して戻された印（次にポケセンの地図に入ったとき 1 回だけ「入った」と数えない） */
#define HACK_FLAG_CH_BROKEN      0xFAB   /* しばりを 1 つでも OFF にしたことがある印（せっていで ON→OFF にした瞬間に立つ・下りない）＝最後のバッジの特別なメッセージが出なくなる */
#define HACK_FLAG_CH_FINALE      0xFAC   /* 最後のバッジの特別なメッセージを出した印（1 回だけ） */

/* 縛り A（0.50.3・patches/0530）: 0xFAD〜0xFB4 は縛り A（縛り B はその次・hack_challenge_b.h） */
#define HACK_FLAG_CH_DEATH       0xFAD   /* ひんしでお別れ: 戦闘のあと ひんしの ポケモンは 手持ちから おわかれ（もどらない）。全員いなくなったらゲームオーバー（さいごのレポートから） */
#define HACK_FLAG_CH_FIRSTONLY   0xFAE   /* 最初の 1 匹だけ: 地図ごとに 最初に出会った野生 1 匹しか つかまえられない（色違いは例外） */
#define HACK_FLAG_CH_NICKNAME    0xFAF   /* ニックネームひっす: 手持ちの ポケモンが ニックネームなし（種族名のまま）なら つけるまで 画面を出す */
#define HACK_FLAG_CH_GAMEOVER    0xFB0   /* ゲームオーバーの最中の印（ソフトリセットで ロードし直すので セーブには残らない） */
#define HACK_FLAG_CH_DEATH_MSG   0xFB1   /* ひんしで おわかれした あとの メッセージを まだ出していない印（次の自由なコマで 1 回出して下ろす） */

#define HACK_CH_FIRST_OFFSET     0x100   /* SaveBlock2 の filler_B20 の中の「出会った地図」の置き場（+0x100〜0x3F7・0xA5 の印 1・個数 2（下位・上位）・(群,番号) × 最大 378） */
#define HACK_CH_FIRST_MAGIC      0xA5
#define HACK_CH_FIRST_MAX        378

#define HACK_CH_CENTER_FEE       100
#define HACK_CH_PC_FEE           500
#define HACK_CH_TEMP_VAR         0x400F  /* ポケセンに入ったあと処理を済ませた印（一時変数＝地図を読み込むたびに 0 に戻る） */
/* 最後のバッジの控えは一時変数に置かない（0x400E は白い岩の回数・アリーナのターン）。タスク Task_ChallengeBadgeSeen */

#define HACK_CH_LENTRANT_LEVEL   HACK_MAX_LEVEL     /* 中身のレベル（Lv200＝MAX_LEVEL・[hack] patches/0677 stat_caps.json から） */
#define HACK_CH_LENTRANT_SHOWN_LEVEL 9999  /* 見かけだけ（戦闘の枠・BW 風 HP バーの表示） */
#define HACK_CH_LENTRANT_DEX_ID  669     /* 表 ID（5 けた表示なら 00669） */

bool8 HackChallenge_NoItems(void);
bool8 HackChallenge_NoHeld(void);
bool8 HackChallenge_IsLentrantBattle(void);                 /* ？？？との戦いの最中か（HACK_FLAG_CH_BATTLE） */
bool8 HackChallenge_IsLentrantBank(u8 bank);                /* その戦闘位置が ？？？（相手側で戦いの最中）か */
bool8 HackChallenge_HeldItemNullified(u8 bank, u8 effect);  /* 自分の側の もちものを こうか なしにするか（battle_util.c の GetBankItemEffect） */
u16 HackChallenge_ShownLevel(struct Pokemon* mon, u16 level);   /* 戦闘の枠に出すレベル（？？？だけ 9999） */
void HackChallenge_KeepLentrantAlive(u8 bank);              /* ？？？の HP が 0 になっていたら 1 に戻す（tryfaintmon の頭） */
void HackChallenge_AdjustDamage(u8 bank);                   /* ？？？へのダメージを 0 に（atk0C_datahpupdate の頭） */
void HackChallenge_OnBattleLost(void);                      /* 負けた（全滅へ向かう）とき: 戻された印を立てる（end_battle.c） */

/* 台本から（callasm）。LASTRESULT: 0 何もない / 1 りょうきんを払った / 2 払った＋スイッチ / 3 払えない（罰金） / 4 スイッチだけ */
void HackChallenge_CenterCheck(void);
void HackChallenge_PcCheck(void);                           /* LASTRESULT: 0 しばりなし / 1 払った / 2 払えない */
void HackChallenge_StartLentrantBattle(void);               /* ？？？との戦いを始める（台本は直後に waitstate） */
void HackChallenge_ItemMenuUseInBattle(u8 taskId);          /* item.c の「つかう」（戦闘）: ボール以外は つかえない */
void HackChallenge_ItemMenuUseOutOfBattle(u8 taskId);       /* item.c の「つかう」（フィールド）: かいふくの どうぐは つかえない */
bool8 HackChallenge_BagClosedInBattle(void);                /* IsBagDisabled: トレーナー戦はバッグごと閉じる */
void HackChallenge_FixLentrant(void);                       /* setwildbattle で作った gEnemyParty[0] を ？？？に仕上げる */
void HackChallenge_FinaleCount(void);                      /* callasm: gStringVar1 = ON の しばりの数・LASTRESULT 1 = 全部 ON */
u8 HackChallenge_CountAllOn(void);                         /* いま ON の しばりの数（全部の縛り・縛り F のボーナス自身も 1 本）。しばりボーナスの賞金・道具の数え方 */
/* 縛り A（patches/0530） */
void HackChallenge_BuryFainted(void);                       /* end_battle.c の EndOfBattleThings の最後: ひんしでお別れ（手持ちから外す・全員なら ゲームオーバーへ） */
void HackChallenge_OnWildBattleStart(void);                 /* wild_encounter.c の DoStandardWildBattle: 最初の 1 匹だけ（使い切りの地図は 旗 0x902 で捕まえられない） */
void HackChallenge_NickPrepare(void);                       /* callasm: ニックネームなしの手持ちの番号を Var8004 に（無ければ LASTRESULT 0）・名前入力へ渡す準備 */
void HackChallenge_GameOverReset(void);                     /* callasm: ソフトリセット（タイトルへ・最後のレポートから） */
void HackChallenge_FieldTick(void);                       /* フィールドの毎コマ（overworld_speedup.c）・ポケセンに入った直後に台本を始める */
