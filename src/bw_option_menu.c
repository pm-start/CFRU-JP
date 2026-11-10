#include "defines.h"
#include "../include/new/hack_random_weather.h"
#include "../include/bg.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/palette.h"
#include "../include/random.h"     /* Random（ランダムしばり・#122） */
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/text_window.h"
#include "../include/window.h"
#include "../include/gba/macro.h"
#include "../include/gba/m4a_internal.h"
#include "../include/new/dns.h"
#include "../include/event_data.h"
#include "Tables/hoenn_script_flags.h"   /* HOENN_VAR_OVERWORLD_SPEED */
#include "hack_battle_speed.h"
#include "../include/constants/pokemon.h"   /* TYPE_NAME_LENGTH（2026-10-11） */
#include "hack_difficulty.h"
#include "hack_network_trade.h"   /* HACK_FLAG_NET_TRADE_BUTTON（2026-10-12） */
#include "hack_challenge.h"       /* HACK_FLAG_CH_*（しばり・2026-10-16 #118） */
#include "hack_challenge_b.h"     /* HACK_FLAG_CHB_*（しばり B・0.50.3 patches/0540） */
#include "hack_challenge_c.h"     /* HACK_FLAG_CHC_*（縛り C・0.50.3 patches/0550） */
#include "hack_fuuin.h"          /* HACK_FLAG_CHD_FUUIN（ふういんルーレット・#123・patches/0560） */
#include "hack_challenge_luxray.h"   /* HACK_FLAG_CHE_LUXRAY（ぜんぶレントラー・patches/0609） */
#include "hack_challenge_f.h"   /* HACK_FLAG_CHF_*（縛り F 6 本・0.63.0） */

/*
    BW 風オプション画面（せってい）— #19 の追加分

    元: https://github.com/Shiny-Miner/FR-OptionMenu（作者 Compumaxx・訳 Shiny-Miner）。
    `参考/github/FR-OptionMenu/src/option_menu.c` を BPRJ + CFRU に合わせて書き直した。
    経緯: docs/research/2026-09-19_bw-option-menu.md

    ## 素 FRLG との関係

    元のパッチは、素のスタートメニューが「せってい」で飛ぶ先のリテラル 1 語
    （BPRE 0806F534 = CB2_OptionsMenuFromStartMenu）を自分の CB2 に差し替えるだけ。
    こちらは BW 風スタートメニュー（src/bw_start_menu.c）から**直接この CB2 を呼ぶ**ので
    ROM には 1 バイトも触らない。★ タイトル画面（つづきから／せってい）の「せってい」は
    素の画面のまま（元のパッチも同じ）。
    戻り先は gMain.savedCallback（スタートメニューが CB2_ReturnToFieldWithOpenMenu を入れる）。

    ## 中身
      - 項目は 6 つ（はなしのはやさ／せんとうアニメ／せんとうルール／おと／
        いどうそくど（元はボタンモード・下）／わく）。読み書きするのは gSaveBlock2 の素の欄（いどうそくどだけ変数）
      - 見た目: 暗い画面（bg1 のタイルマップ 6 枚を選択中の行で切り替える）、
        上に「せってい」と「A ほぞん / B もどる」、下の枠に説明（項目ごと）。
        枠は「わく」の選択で差し替わる（素の GetWindowFrameTilesPal）
      - 文字の色は先頭に FC 01 色 / FC 02 00 を付けて決める（元の {COLOR n}{HIGHLIGHT 0} と同じ）。
        選んでいる選択肢 1（白）、選んでいない選択肢と説明 5（灰）、題 2

    ## CFRU との関係
      - 元の `sArrowPressed`（書き込み可能な static）はタスクの data[8] に置き換えた（6.13）
      - gPaletteFade は CFRU ではポインタ
      - フォントは 2（日本語の標準。CFRU の時計と同じ）。選択肢の x は日本語の幅に合わせて動かした

    ## いどうそくど（2026-10-01）
      ユーザー「その倍速を設定に組み込んでほしい」「ボタンモード潰していいよ」。
      5 行目の「ボタンモード」を**フィールドの倍速（1〜4 ばい）**に差し替えた（行の数と背景の絵はそのまま）。
      値は変数 HOENN_VAR_OVERWORLD_SPEED（倍速の処理は src/overworld_speedup.c）。
      ★ gSaveBlock2 のボタンモードはもう書かない（セーブに入っている値のまま・タイトル画面の素の「せってい」では変えられる）

    ## たいせんそくど・スクロール（2026-10-09）
      ユーザー「後設定スクロールできない？」→ 増やす項目「たいせんそくど」（戦闘の 1〜4 ばい・src/hack_battle_speed.c）。
      画面に出る行は 6 つのまま（背景の絵 6 枚＝選んでいる行の位置）。7 行目からは上下でスクロールする（TD_TOP＝いちばん上の項目）。
      項目を足すときは MENUITEM_* と sItemNames・sDescriptions・TD_* を同じ並びで足す（TD_TEXTSPEED + 項目の番号が値の置き場）。
*/

void __attribute__((long_call)) ScanlineEffect_Stop(void);      /* BPRJ.ld 08087A48 */

/* ---- 絵（src/bw_option_menu_gfx.c・生成物）---- */
extern const u8 gBwOptTiles[];
extern const u8 gBwOptMap0[];
extern const u8 gBwOptMap1[];
extern const u8 gBwOptMap2[];
extern const u8 gBwOptMap3[];
extern const u8 gBwOptMap4[];
extern const u8 gBwOptMap5[];
extern const u8 gBwOptTextPal[];

/* ---- 文字列（strings/bw_option_menu.string）---- */
extern const u8 gText_BwOpt_Title[];
extern const u8 gText_BwOpt_Save[];
extern const u8 gText_BwOpt_Back[];
extern const u8 gText_BwOpt_TextSpeed[];
extern const u8 gText_BwOpt_BattleScene[];
extern const u8 gText_BwOpt_BattleStyle[];
extern const u8 gText_BwOpt_Sound[];
extern const u8 gText_BwOpt_FieldSpeed[];
extern const u8 gText_BwOpt_Difficulty[];
extern const u8 gText_BwOpt_BattleSpeed[];
extern const u8 gText_BwOpt_Moemon[];
extern const u8 gText_BwOpt_MoemonOn[];
extern const u8 gText_BwOpt_MoemonOff[];
extern const u8 gText_BwOpt_Habitat[];
extern const u8 gText_BwOpt_HabitatOrig[];
extern const u8 gText_BwOpt_WonderTrade[];
extern const u8 gText_BwOpt_WonderType[];       /* 2026-10-11 */
extern const u8 gText_BwOpt_WonderTypeAll[];
extern const u8 gText_BwOpt_DescWonderType[];
extern const u8 gText_BwOpt_NetTradeButton[], gText_BwOpt_NetTradeUse[], gText_BwOpt_NetTradeNotUse[], gText_BwOpt_DescNetTradeButton[];   /* 2026-10-12 */
extern const u8 gText_BwOpt_Weather[], gText_BwOpt_WeatherRandom[], gText_BwOpt_WeatherFixed[], gText_BwOpt_DescWeather[];   /* 2026-10-12 #106 */
extern const u8 gText_BwOpt_Nickname[], gText_BwOpt_NicknameAsk[], gText_BwOpt_NicknameNoAsk[], gText_BwOpt_DescNickname[];   /* 2026-10-15 ニックネーム（patches/0405） */
extern const u8 gText_BwOpt_Affection[], gText_BwOpt_AffectionOn[], gText_BwOpt_AffectionOff[], gText_BwOpt_DescAffection[];   /* 2026-10-16 なかよし（patches/0418） */
/* ★ 2026-10-16 #118 せっていの フォルダ・しばり（patches/0510 の台帳・docs/research/2026-10-16_settings-folders-challenges.md） */
extern const u8 gText_BwOpt_FolderBasic[], gText_BwOpt_FolderOriginal[], gText_BwOpt_FolderChallenge[];
extern const u8 gText_BwOpt_DescFolderBasic[], gText_BwOpt_DescFolderOriginal[], gText_BwOpt_DescFolderChallenge[];
extern const u8 gText_BwOpt_Open[], gText_BwOpt_Quit[];
extern const u8 gText_BwOpt_ChNoItems[], gText_BwOpt_ChNoHeld[], gText_BwOpt_ChCenterFee[], gText_BwOpt_ChPcFee[], gText_BwOpt_ChLentrant[];
extern const u8 gText_BwOpt_ChDeath[], gText_BwOpt_ChFirstOnly[], gText_BwOpt_ChNickname[];   /* 縛り A（0.50.3・patches/0530） */
extern const u8 gText_BwOpt_ChbLevelCap[], gText_BwOpt_ChbNoSwitch[], gText_BwOpt_ChbNoWildExp[], gText_BwOpt_ChbDoubleDmg[], gText_BwOpt_ChbNoFly[];   /* 0.50.3 しばり B（strings/hack_challenge_b.string） */
extern const u8 gText_BwOpt_DescChbLevelCap[], gText_BwOpt_DescChbNoSwitch[], gText_BwOpt_DescChbNoWildExp[], gText_BwOpt_DescChbDoubleDmg[], gText_BwOpt_DescChbNoFly[];
extern const u8 gText_BwOpt_ChOn[], gText_BwOpt_ChOff[];
extern const u8 gText_BwOpt_DescChNoItems[], gText_BwOpt_DescChNoHeld[], gText_BwOpt_DescChCenterFee[], gText_BwOpt_DescChPcFee[], gText_BwOpt_DescChLentrant[];
extern const u8 gText_BwOpt_DescChDeath[], gText_BwOpt_DescChFirstOnly[], gText_BwOpt_DescChNickname[];
extern const u8 gText_BwOpt_ChCenterBan[], gText_BwOpt_ChShopBan[], gText_BwOpt_ChNoRevive[], gText_BwOpt_ChMonoType[], gText_BwOpt_ChRandom[];   /* 縛り C（0.50.3・patches/0550） */
extern const u8 gText_BwOpt_DescChCenterBan[], gText_BwOpt_DescChShopBan[], gText_BwOpt_DescChNoRevive[], gText_BwOpt_DescChMonoType[], gText_BwOpt_DescChRandom[];
/* ランダムしばり（#122・2026-10-17）: しばりフォルダの先頭に出す「ボタンの行」。MENUITEM／TD／保存の並び（order）には入れない＝値を持たず、セーブの形も変わらない。
   行の番号は 0xFD（MENUITEM_COUNT より大きい仮の番号・sItemNames／data[TD_TEXTSPEED + item] を引いてはいけない＝生成器の画面だけが扱う） */
#define BWOPT_ITEM_RANDOM 0xFD
/* ふういんルーレット（#123）: 値をもつ『仮の行』（0xFC）。ON/OFF は task data の TD_CH_FUUIN（ヒープ上＝保存しない）と旗 0xFC5 だけで、MENUITEM／order／保存の並びには入れない（保存枠 34/34 を使わない） */
#define BWOPT_ITEM_FUUIN 0xFC
extern const u8 gText_BwOpt_ChFuuin[], gText_BwOpt_DescChFuuin[];
/* ぜんぶレントラー（patches/0609）: ふういんルーレットと同じ『値をもつ仮の行』（0xF7）。値の欄は TD_CH_LUXRAY（ヒープ上＝保存しない）と旗 0xFC7 だけ・MENUITEM／order／保存の並びには入れない */
#define BWOPT_ITEM_LUXRAY 0xF7
extern const u8 gText_BwOpt_ChLuxray[], gText_BwOpt_DescChLuxray[];
/* 縛り F 6 本（0.63.0・docs/research/2026-11-03_challenge-f.md）: ぜんぶレントラーと同じ『値をもつ仮の行』（0xF4〜0xEF）。値の欄は TD_CHF_*（ヒープ上＝保存しない）と旗 0xFEA〜0xFEF だけ・MENUITEM／order／保存の並びには入れない（保存枠 34/34 を使わない） */
#define BWOPT_ITEM_INVERSE 0xF4
#define BWOPT_ITEM_BONUS 0xF3
#define BWOPT_ITEM_BLIND 0xF2
#define BWOPT_ITEM_ONESHOT 0xF1
#define BWOPT_ITEM_ROTATION 0xF0
#define BWOPT_ITEM_GACHA 0xEF

/* きのみブレンダー（FEAT-pokeblock 段 4c・patches/0978）: ミニゲームを あそぶ／スキップ。値をもつ『仮の行』（0xEE・ベーシックの いちばん下）。値の欄は TD_PB_SKIP（ヒープ上＝保存しない）と旗 HACK_FLAG_PB_BLENDER_SKIP（0x138E・立っていればスキップ）だけ・MENUITEM／order／保存の並びには入れない（保存枠 34/34 を使わない）。旗は SetChallengeFlag ではなく FlagSet／FlagClear（しばりではない＝『しばりを はずした印』を立てない） */
#define BWOPT_ITEM_PBSKIP 0xEE
extern const u8 gText_BwOpt_PbSkip[], gText_BwOpt_DescPbSkip[], gText_BwOpt_PbPlay[], gText_BwOpt_PbSkipOn[];
extern const u8 gText_BwOpt_ChInverse[], gText_BwOpt_DescChInverse[], gText_BwOpt_ChBonus[], gText_BwOpt_DescChBonus[], gText_BwOpt_DescChBonusBroken[], gText_BwOpt_ChBlind[], gText_BwOpt_DescChBlind[], gText_BwOpt_ChOneshot[], gText_BwOpt_DescChOneshot[], gText_BwOpt_ChRotation[], gText_BwOpt_DescChRotation[], gText_BwOpt_ChGacha[], gText_BwOpt_DescChGacha[];
/* Syakakiri（patches/0610）: しばりフォルダの いちばん上の『ボタンの行』（0xF6・ランダムしばりの上）。押す（A→はい）と ぜんぶの しばりを ON にする。値は持たない・保存しない＝保存枠も行の上限も使わない */
#define BWOPT_ITEM_SYAKAKIRI 0xF6
extern const u8 gText_BwOpt_ChSyakakiri[], gText_BwOpt_DescChSyakakiri[], gText_BwOpt_SyakakiriAsk[], gText_BwOpt_SyakakiriDone[];
/* 緊急脱出（patches/0752・TODO #144）: どくじきのうフォルダの いちばん上の『ボタンの行』（0xF5・値を持たない・保存しない＝保存枠も行の上限も使わない）。
   スタートメニューから開いた せってい（フィールドの上）のときだけ 行を出す（HackEscape_FieldAvailable）。A→はい で せっていを閉じ、フィールドに戻ってから最寄りのポケセンへ（HackEscape_WarpNow・src/hack_emergency_escape.c） */
#define BWOPT_ITEM_ESCAPE 0xF5
#define BWOPT_CONFIRM_ESCAPE 5      /* state->confirm の値: 緊急脱出の確かめ */
extern const u8 gText_BwOpt_ChEscape[], gText_BwOpt_DescEscape[], gText_BwOpt_EscapeAsk[];
bool8 HackEscape_FieldAvailable(void);   /* src/hack_emergency_escape.c */
void HackEscape_WarpNow(void);
extern const u8 gText_BwOpt_ChRandomBtn[], gText_BwOpt_ChRandomHint[], gText_BwOpt_DescChRandomBtn[], gText_BwOpt_RandomAsk[], gText_BwOpt_RandomDone[];
/* バッジリセット（#125・2026-10-17）: 4 つ目のフォルダ「バッジリセット」の中の 仮の行 2 つ（ホウエン 0xFB・シンオウ 0xFA）。値を持たない＝MENUITEM／TD／sItemFolder／保存の並びに入れない
   （保存枠 34/34 を使わない）。A で 2 回の確かめ（BWOPT_CONFIRM_BADGE1／2）→ はい で HackBadgeReset_Run（src/hack_badge_reset.c）。ほぞん（Task_Save）とは別に その場で旗を下ろす */
#define BWOPT_ITEM_BADGE_HOENN 0xFB
#define BWOPT_ITEM_BADGE_SINNOH 0xFA
#define BWOPT_CONFIRM_RANDOM 1      /* state->confirm の値: ランダムしばりの確かめ（#122） */
#define BWOPT_CONFIRM_BADGE1 2      /* バッジリセットの 1 回目「ほんとうに よろしいですね？」 */
#define BWOPT_CONFIRM_BADGE2 3      /* 2 回目「いいんですか？」 */
#define BWOPT_CONFIRM_SYAKAKIRI 4   /* Syakakiri の確かめ（patches/0610） */
extern const u8 gText_BwOpt_FolderBadge[], gText_BwOpt_DescFolderBadge[], gText_BwOpt_BadgeHoenn[], gText_BwOpt_BadgeSinnoh[], gText_BwOpt_DescBadge[];
extern const u8 gText_BwOpt_BadgeRegionHoenn[], gText_BwOpt_BadgeRegionSinnoh[], gText_BwOpt_BadgeHint[], gText_BwOpt_BadgeAsk1[], gText_BwOpt_BadgeAsk2[];
extern const u8 gText_BwOpt_BadgeDoneHoenn[], gText_BwOpt_BadgeDoneSinnoh[], gText_BwOpt_BadgeInGym[];
void HackBadgeReset_Run(u8 region);   /* src/hack_badge_reset.c（0 ホウエン・1 シンオウ） */
bool8 HackBadgeReset_InGym(void);     /* ホウエン・シンオウのジムの地図の中か（中ではリセットしない・A で「ジムの なかでは リセット できません」） */
#define HACK_FLAG_AFFECTION_OFF 0xF8F /* src/hack_affection.h と同じ（5e の台帳）。立っている＝なつき度の戦いの効果を止める */
#define HACK_FLAG_WEATHER_FIXED 0xF16   /* src/hack_random_weather.c と同じ（5e の台帳） */
#define WONDER_TRADE_TYPE_COUNT 18
#define HACK_VAR_WONDER_TYPE 0x50DC
extern const u8 gWonderTradeTypeOrder[WONDER_TRADE_TYPE_COUNT];   /* src/hack_wonder_trade.c */
extern const u8 gTypeNames[][TYPE_NAME_LENGTH + 1];   /* 1 つ 6 バイト（src/move_menu.c と同じ宣言） */
extern const u8 gText_BwOpt_Medium[];
extern const u8 gText_BwOpt_Easy[];
extern const u8 gText_BwOpt_Hard[];
extern const u8 gText_BwOpt_MomoIce[];
extern const u8 gText_BwOpt_RiruIce[];    /* 2026-10-11 りるアイス（patches/0321） */
extern const u8 gText_BwOpt_Shakariki[];  /* 2026-10-18 しゃかりき（ももアイスより むずかしい） */
extern const u8 gText_BwOpt_Slow[];
extern const u8 gText_BwOpt_Mid[];
extern const u8 gText_BwOpt_Fast[];
extern const u8 gText_BwOpt_On[];
extern const u8 gText_BwOpt_Off[];
extern const u8 gText_BwOpt_Shift[];
extern const u8 gText_BwOpt_Set[];
extern const u8 gText_BwOpt_Mono[];
extern const u8 gText_BwOpt_Stereo[];
extern const u8 gText_BwOpt_Speed1[];
extern const u8 gText_BwOpt_Speed2[];
extern const u8 gText_BwOpt_Speed3[];
extern const u8 gText_BwOpt_Speed4[];
extern const u8 gText_BwOpt_DescTextSpeed[];
extern const u8 gText_BwOpt_DescBattleScene[];
extern const u8 gText_BwOpt_DescBattleStyle[];
extern const u8 gText_BwOpt_DescSound[];
extern const u8 gText_BwOpt_DescFieldSpeed[];
extern const u8 gText_BwOpt_DescDifficulty[];
extern const u8 gText_BwOpt_DescMomoIceWarn[];   /* ★ 2026-10-12 ももアイスの注意 */
extern const u8 gText_BwOpt_DescBattleSpeed[];
extern const u8 gText_BwOpt_DescMoemon[];
extern const u8 gText_BwOpt_DescHabitat[];
extern const u8 gText_BwOpt_DescWonderTrade[];
extern const u8 gText_BwOpt_DescSave[];
extern const u8 gText_BwOpt_DescCancel[];

#define BWOPT_FONT 2
#define BWOPT_FRAME_COUNT 10             /* 素 FRLG の「わく」の数（タイプ 1〜10） */
#define BWOPT_SPEED_COUNT 4              /* いどうそくど 1〜4 ばい */
#define BWOPT_COLOR_TITLE 2
#define BWOPT_COLOR_ON 1                 /* 選んでいる選択肢（白） */
#define BWOPT_COLOR_OFF 5                /* 選んでいない選択肢・説明（灰） */
#define BWOPT_FRAME_TILE 0x1A2           /* 枠の見本のタイル（bg2・charBase 0）*/
#define BWOPT_FRAME_PAL 14
#define BWOPT_VISIBLE_ROWS 6             /* 画面に出る行（背景の絵の数） */

enum
{
    TD_MENUSELECTION,
    TD_TEXTSPEED,
    TD_BATTLESCENE,
    TD_BATTLESTYLE,
    TD_SOUND,
    TD_FIELDSPEED,                       /* 元はボタンモード */
    TD_DIFFICULTY,                       /* 元はわく（2026-10-03） */
    TD_BATTLESPEED,                      /* 2026-10-09 */
#ifndef HACK_NO_MOEMON
    TD_MOEMON,                           /* 2026-10-10 Moémon の絵（もとは たいせつなもの Moemonボタン） */
#endif
    TD_HABITAT,                          /* 2026-10-10 せいそくち（もとは たいせつなもの せいそくスイッチ） */
    TD_WONDERTRADE,                      /* 2026-10-10 ミラクルこうかんの場面 */
    TD_WONDERTYPE,                       /* 2026-10-11 ミラクルこうかんで来るポケモンの 1 つ目のタイプ */
    TD_NETTRADE,                         /* 2026-10-12 ネット交換をフィールドの A＋Select で開くか */
#if HACK_RANDOM_WEATHER_ENABLED
    TD_WEATHER,                          /* 2026-10-12 #106 てんき ランダム／きまり */
#endif
    TD_NICKNAME,                         /* 2026-10-15 ニックネーム きかれる／きかれない */
    TD_AFFECTION,                        /* 2026-10-16 なかよし こうか あり／こうか なし */
    TD_CH_NOITEMS,                       /* 2026-10-16 #118 しばり（5 つ・どれも 0 ＝ OFF・旗は hack_challenge.h） */
    TD_CH_NOHELD,
    TD_CH_CENTERFEE,
    TD_CH_PCFEE,
    TD_CH_LENTRANT,
    TD_CH_DEATH,                         /* 縛り A（0.50.3・patches/0530）: ひんしでおわかれ／はじめの1ぴきだけ／ニックネームひっす */
    TD_CH_FIRSTONLY,
    TD_CH_NICKNAME,
    TD_CHB_LEVELCAP,                     /* 0.50.3 しばり B（5 つ・どれも 0 ＝ OFF・旗は hack_challenge_b.h） */
    TD_CHB_NOSWITCH,
    TD_CHB_NOWILDEXP,
    TD_CHB_DOUBLEDMG,
    TD_CHB_NOFLY,
    TD_CH_CENTERBAN,                     /* 縛り C（0.50.3・patches/0550）: ポケセンきんし／おみせきんし／ひんしどうぐきんし／モノタイプ／ランダム */
    TD_CH_SHOPBAN,
    TD_CH_NOREVIVE,
    TD_CH_MONOTYPE,
    TD_CH_RANDOM,
    TD_CH_FUUIN,                         /* ふういんルーレット（#123）: 仮の行 BWOPT_ITEM_FUUIN の値（0 OFF／1 ON・旗 0xFC5） */
    TD_CH_LUXRAY,                        /* ぜんぶレントラー（patches/0609）: 仮の行 BWOPT_ITEM_LUXRAY の値（0 OFF／1 ON・旗 0xFC7）。MENUITEM の列の外＝TD_CH_FUUIN の次・TD_TIMER の手前 */
    TD_CHF_INVERSE,                            /* 縛り F（0.63.0）: 仮の行 BWOPT_ITEM_INVERSE の値（0 OFF／1 ON・旗 HACK_FLAG_CHF_INVERSE） */
    TD_CHF_BONUS,                            /* 縛り F（0.63.0）: 仮の行 BWOPT_ITEM_BONUS の値（0 OFF／1 ON・旗 HACK_FLAG_CHF_BONUS） */
    TD_CHF_BLIND,                            /* 縛り F（0.63.0）: 仮の行 BWOPT_ITEM_BLIND の値（0 OFF／1 ON・旗 HACK_FLAG_CHF_BLIND） */
    TD_CHF_ONESHOT,                            /* 縛り F（0.63.0）: 仮の行 BWOPT_ITEM_ONESHOT の値（0 OFF／1 ON・旗 HACK_FLAG_CHF_ONESHOT） */
    TD_CHF_ROTATION,                            /* 縛り F（0.63.0）: 仮の行 BWOPT_ITEM_ROTATION の値（0 OFF／1 ON・旗 HACK_FLAG_CHF_ROTATION） */
    TD_CHF_GACHA,                            /* 縛り F（0.63.0）: 仮の行 BWOPT_ITEM_GACHA の値（0 OFF／1 ON・旗 HACK_FLAG_CHF_GACHA） */
    TD_PB_SKIP,                              /* きのみブレンダー（段 4c）: 仮の行 BWOPT_ITEM_PBSKIP の値（0 あそぶ／1 スキップ・旗 HACK_FLAG_PB_BLENDER_SKIP 0x138E） */
    TD_TIMER,
    TD_TOP,                              /* スクロール: いちばん上に出ている項目 */
};
_Static_assert(TD_TOP < 16, "bw_option_menu: task data は 16 個まで（gTasks[].data[16]）・行を足すときは欄を空ける（2026-10-12）。2026-10-16 から行が増えて 16 を超える＝原本だけでは組めない（組むのは settings-plus／Fly の生成コピー＝設定値をヒープへ置き、この行を消す・patches/0418）");

enum
{
    MENUITEM_TEXTSPEED,
    MENUITEM_BATTLESCENE,
    MENUITEM_BATTLESTYLE,
    MENUITEM_SOUND,
    MENUITEM_FIELDSPEED,                 /* 元はボタンモード */
    MENUITEM_DIFFICULTY,                 /* 元はわく（2026-10-03・なんいど ふつう／やさしい） */
    MENUITEM_BATTLESPEED,                /* 2026-10-09 たいせんそくど */
#ifndef HACK_NO_MOEMON
    MENUITEM_MOEMON,                     /* 2026-10-10 だす／ださない＝旗 HACK_FLAG_MOEMON_ON（オミット版には無い） */
#endif
    MENUITEM_HABITAT,                    /* 2026-10-10 いれかえ／もとのまま＝旗 HACK_FLAG_HABITAT_ORIGINAL */
    MENUITEM_WONDERTRADE,                /* 2026-10-10 みる／みない＝旗 HACK_FLAG_WONDER_SKIP（src/hack_wonder_trade.c） */
    MENUITEM_WONDERTYPE,                 /* 2026-10-11 ぜんぶ／タイプ 18 個＝予約変数 HACK_VAR_WONDER_TYPE（0 ぜんぶ・1〜18 は gWonderTradeTypeOrder の番号 + 1） */
    MENUITEM_NETTRADE,                   /* 2026-10-12 ネットこうかんボタン つかう／つかわない＝予約フラグ HACK_FLAG_NET_TRADE_BUTTON（src/hack_network_trade.c・最初は つかわない） */
#if HACK_RANDOM_WEATHER_ENABLED
    MENUITEM_WEATHER,                    /* 2026-10-12 #106 てんき ランダム／きまり＝予約フラグ HACK_FLAG_WEATHER_FIXED 0xF16 が立っていれば きまり（src/hack_random_weather.c・最初は ランダム） */
#endif
    MENUITEM_NICKNAME,                   /* 2026-10-15 ニックネーム きかれる／きかれない＝旗 FLAG_DONT_OFFER_NICKNAMES_BATTLE 0xF8E が立っていれば きかれない（patches/0405・最初は きかれる＝古いセーブも） */
    MENUITEM_AFFECTION,                  /* 2026-10-16 なかよし こうか あり／こうか なし＝予約フラグ HACK_FLAG_AFFECTION_OFF 0xF8F が立っていれば こうか なし（patches/0418・最初は こうか あり＝古いセーブも） */
    MENUITEM_CH_NOITEMS,                 /* 2026-10-16 #118 しばり: どうぐきんし／もちものきんし／ポケセンりょうきん／PCりょうきん／ポケセンの？？？（ON/OFF・旗 HACK_FLAG_CH_*・最初は全部 OFF＝古いセーブも） */
    MENUITEM_CH_NOHELD,
    MENUITEM_CH_CENTERFEE,
    MENUITEM_CH_PCFEE,
    MENUITEM_CH_LENTRANT,
    MENUITEM_CH_DEATH,                   /* 縛り A（0.50.3・patches/0530）: ひんしでおわかれ／はじめの1ぴきだけ／ニックネームひっす（旗は hack_challenge.h） */
    MENUITEM_CH_FIRSTONLY,
    MENUITEM_CH_NICKNAME,
    MENUITEM_CHB_LEVELCAP,               /* 0.50.3 しばり B: レベルキャップ／こうたいきんし／やせいけいけんなし／ダメージ1.5ばい（旧 2ばい）／そらをとぶきんし（ON/OFF・旗 HACK_FLAG_CHB_*・最初は全部 OFF） */
    MENUITEM_CHB_NOSWITCH,
    MENUITEM_CHB_NOWILDEXP,
    MENUITEM_CHB_DOUBLEDMG,
    MENUITEM_CHB_NOFLY,
    MENUITEM_CH_CENTERBAN,               /* 縛り C（0.50.3・patches/0550）: 旗は hack_challenge_c.h */
    MENUITEM_CH_SHOPBAN,
    MENUITEM_CH_NOREVIVE,
    MENUITEM_CH_MONOTYPE,
    MENUITEM_CH_RANDOM,
    MENUITEM_COUNT,
};

/* ★ 2026-10-16 #118 フォルダ。せっていを開くと まずフォルダの一覧（きほん／どくじきのう／しばり）。A で開く・フォルダの中は今までの行（A ほぞん・B フォルダの一覧へ）。
   行→フォルダの表は sItemFolder（MENUITEM の並びと同じ・ひこうえんしゅつは Fly の写しが後ろに足す）。
   ★ この原本は行が 16 を超えて単独では組めない（_Static_assert）。フォルダの画面・並べ替えは tools/settings_plus/prepare_ui_patch.py が写しに足す（理由: docs/research/2026-10-16_settings-folders-challenges.md） */
enum
{
    BWOPT_FOLDER_BASIC,                  /* きほん: 素の せってい（はなしのはやさ・せんとうアニメ・せんとうルール・おと） */
    BWOPT_FOLDER_ORIGINAL,               /* どくじきのう: このハックで足した設定（標準に無いもの全部） */
    BWOPT_FOLDER_CHALLENGE,              /* しばり */
    BWOPT_FOLDER_BADGE,                  /* バッジリセット（#125・行は仮の 2 つだけ＝sItemFolder に出てこない） */
    BWOPT_FOLDER_COUNT,
};

enum
{
    DESC_SAVE = MENUITEM_COUNT,
    DESC_CANCEL,
};

enum
{
    WIN_TEXT,
    WIN_FRAME,
};

static const u8 *const sItemNames[MENUITEM_COUNT] =
{
    gText_BwOpt_TextSpeed, gText_BwOpt_BattleScene, gText_BwOpt_BattleStyle,
    gText_BwOpt_Sound, gText_BwOpt_FieldSpeed, gText_BwOpt_Difficulty, gText_BwOpt_BattleSpeed,
#ifndef HACK_NO_MOEMON
    gText_BwOpt_Moemon,
#endif
    gText_BwOpt_Habitat,
    gText_BwOpt_WonderTrade,
    gText_BwOpt_WonderType,
    gText_BwOpt_NetTradeButton,
#if HACK_RANDOM_WEATHER_ENABLED
    gText_BwOpt_Weather,
#endif
    gText_BwOpt_Nickname,
    gText_BwOpt_Affection,
    gText_BwOpt_ChNoItems, gText_BwOpt_ChNoHeld, gText_BwOpt_ChCenterFee, gText_BwOpt_ChPcFee, gText_BwOpt_ChLentrant,
    gText_BwOpt_ChDeath, gText_BwOpt_ChFirstOnly, gText_BwOpt_ChNickname,
    gText_BwOpt_ChbLevelCap, gText_BwOpt_ChbNoSwitch, gText_BwOpt_ChbNoWildExp, gText_BwOpt_ChbDoubleDmg, gText_BwOpt_ChbNoFly,
    gText_BwOpt_ChCenterBan, gText_BwOpt_ChShopBan, gText_BwOpt_ChNoRevive, gText_BwOpt_ChMonoType, gText_BwOpt_ChRandom,
};

/* 行（MENUITEM の並び）→ フォルダ。ユーザー 2026-10-16「せっていの独自機能は『独自機能』のフォルダに・縛りは『縛り』で分けて」。
   標準（素の FR／BW 風の せってい）は 4 つだけ。Fly の ひこうえんしゅつ は Fly の写し（prepare_option_menu.py）が最後に BWOPT_FOLDER_ORIGINAL を足す */
static const u8 sItemFolder[MENUITEM_COUNT] =
{
    BWOPT_FOLDER_BASIC, BWOPT_FOLDER_BASIC, BWOPT_FOLDER_BASIC,         /* はなしのはやさ・せんとうアニメ・せんとうルール */
    BWOPT_FOLDER_BASIC,                                                 /* おと */
    BWOPT_FOLDER_ORIGINAL, BWOPT_FOLDER_ORIGINAL, BWOPT_FOLDER_ORIGINAL, /* いどうそくど・なんいど・たいせんそくど */
#ifndef HACK_NO_MOEMON
    BWOPT_FOLDER_ORIGINAL,                                              /* Moemon */
#endif
    BWOPT_FOLDER_ORIGINAL, BWOPT_FOLDER_ORIGINAL, BWOPT_FOLDER_ORIGINAL, BWOPT_FOLDER_ORIGINAL,   /* せいそくち・ミラクルこうかん・こうかんタイプ・ネットボタン */
#if HACK_RANDOM_WEATHER_ENABLED
    BWOPT_FOLDER_ORIGINAL,                                              /* てんき */
#endif
    BWOPT_FOLDER_ORIGINAL, BWOPT_FOLDER_ORIGINAL,                       /* ニックネーム・なかよし */
    BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE,   /* しばり 5 つ */
    BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE,   /* 縛り A 3 つ（ひんしでおわかれ・はじめの1ぴきだけ・ニックネームひっす） */
    BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE,   /* しばり B 5 つ（0.50.3） */
    BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE, BWOPT_FOLDER_CHALLENGE,   /* 縛り C 5 つ（ポケセンきんし・おみせきんし・ひんしどうぐきんし・モノタイプ・ランダム） */
};

static const u8 *const sFolderNames[BWOPT_FOLDER_COUNT] =
{
    gText_BwOpt_FolderBasic, gText_BwOpt_FolderOriginal, gText_BwOpt_FolderChallenge, gText_BwOpt_FolderBadge,
};

static const u8 *const sFolderDescriptions[BWOPT_FOLDER_COUNT] =
{
    gText_BwOpt_DescFolderBasic, gText_BwOpt_DescFolderOriginal, gText_BwOpt_DescFolderChallenge, gText_BwOpt_DescFolderBadge,
};

/* 縛り F 6 本（0.63.0）の仮の行（0xEF〜0xF4 の連番）: 行の番号 → 値の欄（TD_CHF_*）・行の名前。MENUITEM／sItemNames／sItemFolder には入れない（ふういん／ぜんぶレントラーと同じ） */
#define BWOPT_ITEM_CHF_FIRST BWOPT_ITEM_GACHA
#define BWOPT_ITEM_CHF_LAST BWOPT_ITEM_INVERSE
#define BWOPT_IS_CHF(item) ((item) >= BWOPT_ITEM_CHF_FIRST && (item) <= BWOPT_ITEM_CHF_LAST)
static const u8 sChfTd[] =
{
    TD_CHF_GACHA, TD_CHF_ROTATION, TD_CHF_ONESHOT, TD_CHF_BLIND, TD_CHF_BONUS, TD_CHF_INVERSE,   /* 行の番号 0xEF から 0xF4 の順 */
};
static const u8 *const sChfNames[] =
{
    gText_BwOpt_ChGacha, gText_BwOpt_ChRotation, gText_BwOpt_ChOneshot, gText_BwOpt_ChBlind, gText_BwOpt_ChBonus, gText_BwOpt_ChInverse,
};

static const u8 *const sDescriptions[] =
{
    gText_BwOpt_DescTextSpeed, gText_BwOpt_DescBattleScene, gText_BwOpt_DescBattleStyle,
    gText_BwOpt_DescSound, gText_BwOpt_DescFieldSpeed, gText_BwOpt_DescDifficulty, gText_BwOpt_DescBattleSpeed,
#ifndef HACK_NO_MOEMON
    gText_BwOpt_DescMoemon,
#endif
    gText_BwOpt_DescHabitat,
    gText_BwOpt_DescWonderTrade,
    gText_BwOpt_DescWonderType,
    gText_BwOpt_DescNetTradeButton,
#if HACK_RANDOM_WEATHER_ENABLED
    gText_BwOpt_DescWeather,
#endif
    gText_BwOpt_DescNickname,
    gText_BwOpt_DescAffection,
    gText_BwOpt_DescChNoItems, gText_BwOpt_DescChNoHeld, gText_BwOpt_DescChCenterFee, gText_BwOpt_DescChPcFee, gText_BwOpt_DescChLentrant,
    gText_BwOpt_DescChDeath, gText_BwOpt_DescChFirstOnly, gText_BwOpt_DescChNickname,
    gText_BwOpt_DescChbLevelCap, gText_BwOpt_DescChbNoSwitch, gText_BwOpt_DescChbNoWildExp, gText_BwOpt_DescChbDoubleDmg, gText_BwOpt_DescChbNoFly,
    gText_BwOpt_DescChCenterBan, gText_BwOpt_DescChShopBan, gText_BwOpt_DescChNoRevive, gText_BwOpt_DescChMonoType, gText_BwOpt_DescChRandom,
    gText_BwOpt_DescSave, gText_BwOpt_DescCancel,
};

static const u8 *const sMaps[BWOPT_VISIBLE_ROWS] =   /* 選んでいる行の位置（項目 - TD_TOP）で選ぶ */
{
    gBwOptMap0, gBwOptMap1, gBwOptMap2, gBwOptMap3, gBwOptMap4, gBwOptMap5,
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TEXT] = {.bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = 30, .height = 20, .paletteNum = 5, .baseBlock = 0},
    [WIN_FRAME] = {.bg = 2, .tilemapLeft = 0, .tilemapTop = 14, .width = 30, .height = 6, .paletteNum = BWOPT_FRAME_PAL, .baseBlock = 0},
    DUMMY_WIN_TEMPLATE,
};

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 1, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 0, .mapBaseIndex = 7, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
    {.bg = 2, .charBaseIndex = 0, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
};

static void Task_FadeIn(u8 taskId);
static void Task_ProcessInput(u8 taskId);
static void Task_Save(u8 taskId);
static void Task_Cancel(u8 taskId);
static void Task_FadeOut(u8 taskId);
static void DrawChoicesFor(u8 item, u8 selection, u8 top);

/* ---- 文字 ---- */

/* 先頭に色の指定を付けて描く。元の {COLOR n}{HIGHLIGHT 0} と同じ並び */
static void PrintColored(const u8 *text, u8 x, u8 y, u8 color)
{
    u8 dst[96];
    u8 *p = dst;
    u16 i;

    *p++ = 0xFC; *p++ = 0x01; *p++ = color;
    *p++ = 0xFC; *p++ = 0x02; *p++ = 0;
    for (i = 0; text[i] != EOS && i < 80; ++i)
        *p++ = text[i];
    *p = EOS;
    AddTextPrinterParameterized(WIN_TEXT, BWOPT_FONT, dst, x, y, TEXT_SPEED_FF, NULL);
}

static void DrawChoice(const u8 *text, u8 x, u8 y, bool8 on)
{
    PrintColored(text, x, y + 1, on ? BWOPT_COLOR_ON : BWOPT_COLOR_OFF);
}

static void ClearDescription(void)
{
    FillWindowPixelRect(WIN_TEXT, 0, 8, 120, 224, 32);
}

static void ShowDescription(u8 which)
{
    ClearDescription();
    PrintColored(which == BWOPT_ITEM_RANDOM ? gText_BwOpt_DescChRandomBtn : which == BWOPT_ITEM_FUUIN ? gText_BwOpt_DescChFuuin
        : which == BWOPT_ITEM_LUXRAY ? gText_BwOpt_DescChLuxray : which == BWOPT_ITEM_SYAKAKIRI ? gText_BwOpt_DescChSyakakiri
        : which == BWOPT_ITEM_INVERSE ? gText_BwOpt_DescChInverse
        : which == BWOPT_ITEM_BONUS ? (FlagGet(HACK_FLAG_CH_BROKEN) ? gText_BwOpt_DescChBonusBroken : gText_BwOpt_DescChBonus)   /* 縛りを OFF にしたことがあるセーブは ごほうびが もらえない（BonusCount が 0）ので 説明でも知らせる（2026-11-03） */
        : which == BWOPT_ITEM_BLIND ? gText_BwOpt_DescChBlind
        : which == BWOPT_ITEM_ONESHOT ? gText_BwOpt_DescChOneshot
        : which == BWOPT_ITEM_ROTATION ? gText_BwOpt_DescChRotation
        : which == BWOPT_ITEM_GACHA ? gText_BwOpt_DescChGacha
        : which == BWOPT_ITEM_PBSKIP ? gText_BwOpt_DescPbSkip
        : which == BWOPT_ITEM_ESCAPE ? gText_BwOpt_DescEscape
        : (which == BWOPT_ITEM_BADGE_HOENN || which == BWOPT_ITEM_BADGE_SINNOH) ? gText_BwOpt_DescBadge : sDescriptions[which], 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

/* ---- ランダムしばり（#122・2026-10-17）----
   しばりフォルダの「ランダムしばり」を A→はい で、今の しばりを ぜんぶ OFF にして 1〜5 個を ON にする。
   画面の値（task data）だけを書き換える＝旗を立てる処理は通常の ほぞん（Task_Save の SetChallengeFlag）がそのまま通す
   （だから ON→OFF になったものには「OFF にした印」HACK_FLAG_CH_BROKEN が 手で OFF にしたときと同じに立ち、
   ニックネームひっす・モノタイプ・はじめの1ぴきだけ などの ON のあとの処理も 手で ON にしたときと同じ道）。 */
static const u8 sRandomChallengeTd[] =
{
    TD_CH_NOITEMS, TD_CH_NOHELD, TD_CH_CENTERFEE, TD_CH_PCFEE, TD_CH_LENTRANT,
    TD_CH_DEATH, TD_CH_FIRSTONLY, TD_CH_NICKNAME,
    TD_CHB_LEVELCAP, TD_CHB_NOSWITCH, TD_CHB_NOWILDEXP, TD_CHB_DOUBLEDMG, TD_CHB_NOFLY,
    TD_CH_CENTERBAN, TD_CH_SHOPBAN, TD_CH_NOREVIVE, TD_CH_MONOTYPE, TD_CH_RANDOM,
    TD_CH_FUUIN,   /* 仮の行（#123） */
    TD_CH_LUXRAY,  /* 仮の行（patches/0609）。ほかの しばりと ぶつかる組は無い（モノタイプは レントラー〈でんき〉に合わせる＝hack_challenge_c.c の MonoRecord） */
    TD_CHF_INVERSE,   /* 縛り F（0.63.0）仮の行 */
    TD_CHF_BONUS,     /* 縛り F（0.63.0）仮の行 */
    TD_CHF_BLIND,     /* 縛り F（0.63.0）仮の行 */
    TD_CHF_ONESHOT,   /* 縛り F（0.63.0）仮の行 */
    TD_CHF_ROTATION,  /* 縛り F（0.63.0）仮の行 */
    TD_CHF_GACHA,     /* 縛り F（0.63.0）仮の行 */
};

/* 同時に選ばない組（互いに打ち消す）。ポケセンりょうきん（入るたびに料金）は ポケセンきんし（回復できない）と意味が合わない */
static const u8 sRandomChallengeExclusive[][2] =
{
    {TD_CH_CENTERFEE, TD_CH_CENTERBAN},
};

static bool8 BwOptRandomConflict(const s16 *data, u8 td)
{
    u8 i;

    for (i = 0; i < NELEMS(sRandomChallengeExclusive); ++i)
    {
        if ((td == sRandomChallengeExclusive[i][0] && data[sRandomChallengeExclusive[i][1]] == 1)
         || (td == sRandomChallengeExclusive[i][1] && data[sRandomChallengeExclusive[i][0]] == 1))
            return TRUE;
    }
    return FALSE;
}

/* ぜんぶ OFF → 1〜5 個をランダムに ON。選んだ数を返す。除外した組は選び直す（候補は引いたら外す＝同じものを 2 度引かない） */
static u8 BwOptRandomChallenges(s16 *data)
{
    u8 pool[NELEMS(sRandomChallengeTd)];
    u8 n = NELEMS(sRandomChallengeTd);
    u8 want, picked = 0, i;

    for (i = 0; i < n; ++i)
    {
        data[sRandomChallengeTd[i]] = 0;
        pool[i] = sRandomChallengeTd[i];
    }
    want = 1 + (Random() % 5);
    while (picked < want && n > 0)
    {
        u8 k = Random() % n;
        u8 td = pool[k];

        pool[k] = pool[--n];
        if (BwOptRandomConflict(data, td))
            continue;
        data[td] = 1;
        ++picked;
    }
    return picked;
}

/* ---- Syakakiri（patches/0610・ユーザー 2026-10-18「Syakakiriって選択肢を一番上に追加してそれで全部の縛りをONにしてあげて」）----
   しばりフォルダの いちばん上の「Syakakiri」を A→はい で、しばりを 全部 ON にする（ぜんぶレントラーも ふくむ）。ランダムしばりと同じ 画面の値（task data）だけを書き換える＝
   旗は 通常の ほぞん（Task_Save の SetChallengeFlag）がそのまま立てる。対象の表は ランダムしばりと同じ（sRandomChallengeTd＝しばりの全部）。
   ★ ポケセンりょうきん × ポケセンきんし も 両方 ON（ランダムしばりは 組み合わせないが、全部 ON の「全部」を その通りに＝最後のバッジの『すべての しばり』も 出せる。両方 ON は 料金を取られて 回復もできない いちばんきびしい形）。
   ここで 全部を 1 にするので OFF→ON だけ（「OFF にした印」HACK_FLAG_CH_BROKEN は 立たない）。数は返さない */
static void BwOptSyakakiri(s16 *data)
{
    u8 i;

    for (i = 0; i < NELEMS(sRandomChallengeTd); ++i)
        data[sRandomChallengeTd[i]] = 1;
}

static void BwOptShowSyakakiriAsk(void)
{
    ClearDescription();
    PrintColored(gText_BwOpt_SyakakiriAsk, 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

static void BwOptShowSyakakiriDone(void)
{
    ClearDescription();
    PrintColored(gText_BwOpt_SyakakiriDone, 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

/* 結果の短い文: 「3こ えらんだよ！ …」（数字は 1 けた＝1〜5） */
static void BwOptShowRandomDone(u8 picked)
{
    u8 buf[48];
    u8 i = 0, j;

    buf[i++] = 0xA1 + picked;   /* 数字の文字コード（0 = 0xA1） */
    for (j = 0; gText_BwOpt_RandomDone[j] != EOS && i < sizeof(buf) - 1; ++j)
        buf[i++] = gText_BwOpt_RandomDone[j];
    buf[i] = EOS;
    ClearDescription();
    PrintColored(buf, 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

static void BwOptShowRandomAsk(void)
{
    ClearDescription();
    PrintColored(gText_BwOpt_RandomAsk, 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

/* ---- バッジリセット（#125・2026-10-17）----
   説明の欄（2 行）に出す。1 行目: 地方（「ホウエンの バッジ」）と右に「Aはい Bいいえ」・2 行目: 確かめの問い（1 回目「ほんとうに よろしいですね？」・2 回目「いいんですか？」）。
   2 行に収めるため（1 行 220 画素・問い 153 画素＋「Aはい Bいいえ」80 画素は並ばない）、案内は 1 行目の右はし（x140）。
   終わったら 2 行の結果（「ホウエンの バッジを リセットしました」＋「ジムに また ちょうせんできます」）。 */
static void BwOptShowBadgeAsk(u8 item, u8 step)
{
    ClearDescription();
    PrintColored(item == BWOPT_ITEM_BADGE_HOENN ? gText_BwOpt_BadgeRegionHoenn : gText_BwOpt_BadgeRegionSinnoh, 12, 120, BWOPT_COLOR_OFF);
    PrintColored(gText_BwOpt_BadgeHint, 140, 120, BWOPT_COLOR_OFF);
    PrintColored(step == 1 ? gText_BwOpt_BadgeAsk1 : gText_BwOpt_BadgeAsk2, 12, 136, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

/* 緊急脱出の確かめ（説明の欄の 2 行）: 1 行目 行の名前（x12）と右に「Aはい Bいいえ」（x140）・2 行目 問い */
static void BwOptShowEscapeAsk(void)
{
    ClearDescription();
    PrintColored(gText_BwOpt_ChEscape, 12, 120, BWOPT_COLOR_OFF);
    PrintColored(gText_BwOpt_BadgeHint, 140, 120, BWOPT_COLOR_OFF);
    PrintColored(gText_BwOpt_EscapeAsk, 12, 136, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

static void BwOptShowBadgeInGym(void)
{
    ClearDescription();
    PrintColored(gText_BwOpt_BadgeInGym, 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

static void BwOptShowBadgeDone(u8 item)
{
    ClearDescription();
    PrintColored(item == BWOPT_ITEM_BADGE_HOENN ? gText_BwOpt_BadgeDoneHoenn : gText_BwOpt_BadgeDoneSinnoh, 12, 120, BWOPT_COLOR_OFF);
    CopyWindowToVram(WIN_TEXT, 2);
}

/* ★ 2026-10-12 ユーザー「ももアイスにしたとき注意の文を」: なんいど の行で ももアイス（画面の並びの 6 つ目＝DrawDifficultyChoice の sNames の 5）を
   選んでいる間だけ、下の説明の欄に注意の文（gText_BwOpt_DescMomoIceWarn）を出す。ほかの行・ほかの なんいど は今までの説明 */
#define BWOPT_DIFFICULTY_SEL_MOMO_ICE 5
static void ShowDescriptionFor(u8 which, s16 *data)
{
    if (which == MENUITEM_DIFFICULTY && data[TD_DIFFICULTY] >= BWOPT_DIFFICULTY_SEL_MOMO_ICE)   /* 5 ももアイス・6 しゃかりき */
    {
        ClearDescription();
        PrintColored(gText_BwOpt_DescMomoIceWarn, 12, 120, BWOPT_COLOR_OFF);
        CopyWindowToVram(WIN_TEXT, 2);
        return;
    }
    ShowDescription(which);
}

static void DrawFramePreview(void)
{
    /*                          bg  tile                 x   y   w   h  pal */
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 0,  0, 14,  1,  1, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 1,  1, 14, 28,  1, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 2, 29, 14,  1,  1, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 3,  0, 15,  1,  4, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 4,  1, 15, 28,  4, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 5, 29, 15,  1,  4, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 6,  0, 19,  1,  1, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 7,  1, 19, 28,  1, BWOPT_FRAME_PAL);
    FillBgTilemapBufferRect(2, BWOPT_FRAME_TILE + 8, 29, 19,  1,  1, BWOPT_FRAME_PAL);
    CopyBgTilemapBufferToVram(2);
}

static void LoadFrameGfx(u8 frameType)
{
	if (frameType >= BWOPT_FRAME_COUNT)
		frameType = 0;
    const struct TilesPal *frame = GetWindowFrameTilesPal(frameType);

    LoadBgTiles(2, frame->tiles, 0x120, BWOPT_FRAME_TILE);
    LoadPalette(frame->pal, BWOPT_FRAME_PAL * 16, 0x20);
}

static void DrawTexts(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_TEXT, PIXEL_FILL(0));
    PrintColored(gText_BwOpt_Title, 8, 1, BWOPT_COLOR_TITLE);
    /* ★ A / B のボタンの絵は**背景の絵に焼き込まれている**（格子の x タイル 16 = 画面 x128〜135 と
       x タイル 23 = 画面 x184〜191）。文字で描くと 2 つ出る（実機 2026-09-20）。言葉だけをその右へ置く */
    PrintColored(gText_BwOpt_Save, 138, 1, BWOPT_COLOR_TITLE);
    PrintColored(gText_BwOpt_Back, 194, 1, BWOPT_COLOR_TITLE);
    ShowDescription(0);
    for (i = 0; i < MENUITEM_COUNT && i < BWOPT_VISIBLE_ROWS; ++i)
        PrintColored(sItemNames[i], 8, (i * 16) + 17, BWOPT_COLOR_ON);
    CopyWindowToVram(WIN_TEXT, 2);
}

/* スクロールしたとき: 行の場所を消して、出ている項目の名前と選択肢を描き直す */
static void DrawRows(const s16 *data)
{
    u8 top = data[TD_TOP];
    u8 i;

    FillWindowPixelRect(WIN_TEXT, 0, 0, 16, 240, BWOPT_VISIBLE_ROWS * 16);
    for (i = top; i < MENUITEM_COUNT && i < top + BWOPT_VISIBLE_ROWS; ++i)
    {
        PrintColored(sItemNames[i], 8, ((i - top) * 16) + 17, BWOPT_COLOR_ON);
        DrawChoicesFor(i, data[TD_TEXTSPEED + i], top);
    }
    CopyWindowToVram(WIN_TEXT, 2);
}

/* ---- 選択肢 ---- */

static void DrawChoices3(const u8 *a, const u8 *b, const u8 *c, u8 x1, u8 x2, u8 x3, u8 y, u8 sel)
{
    DrawChoice(a, x1, y, sel == 0);
    DrawChoice(b, x2, y, sel == 1);
    DrawChoice(c, x3, y, sel == 2);
}

static void DrawChoices2(const u8 *a, const u8 *b, u8 x1, u8 x2, u8 y, u8 sel)
{
    DrawChoice(a, x1, y, sel == 0);
    DrawChoice(b, x2, y, sel == 1);
}

/* やさしい・ほどほど・ふつう・むずかしい・ももアイス。保存値の対応は hack_difficulty.h。 */

/* 横に並ばない選択肢は今の値だけを出す（左右で切り替え） */
static void DrawValueChoice(const u8 *text, u8 y)
{
    FillWindowPixelRect(WIN_TEXT, 0, 104, y, 120, 16);
    DrawChoice(text, 150, y, TRUE);
}

/* 2026-10-11 こうかんタイプ: 0 ぜんぶ・1〜18 はタイプの名前（gTypeNames） */
static void DrawWonderTypeChoice(u8 selection, u8 y)
{
    DrawValueChoice((selection >= 1 && selection <= WONDER_TRADE_TYPE_COUNT) ? gTypeNames[gWonderTradeTypeOrder[selection - 1]] : gText_BwOpt_WonderTypeAll, y);
}

static void DrawDifficultyChoice(u8 selection, u8 y)
{
    static const u8 *const sNames[DIFFICULTY_COUNT] =
    {
        gText_BwOpt_RiruIce, gText_BwOpt_Easy, gText_BwOpt_Medium, gText_BwOpt_Mid, gText_BwOpt_Hard, gText_BwOpt_MomoIce, gText_BwOpt_Shakariki,
    };

    DrawValueChoice(sNames[selection < DIFFICULTY_COUNT ? selection : 3], y);   /* 3 = ふつう（HackDifficulty_SelectionFromValue） */
}

static void DrawSpeedChoice(u8 selection, u8 y)
{
    static const u8 *const sSpeeds[BWOPT_SPEED_COUNT] =
    {
        gText_BwOpt_Speed1, gText_BwOpt_Speed2, gText_BwOpt_Speed3, gText_BwOpt_Speed4,
    };

    DrawValueChoice(sSpeeds[selection < BWOPT_SPEED_COUNT ? selection : 0], y);
}

/* ★ 日本語の幅に合わせた x。3 文字（約 36 画素）が重ならないよう 40 ずつ空ける。y は画面の行（項目 - top）で決める */
static void DrawChoicesFor(u8 item, u8 selection, u8 top)
{
    u8 y;

    if (item < top || item >= top + BWOPT_VISIBLE_ROWS)
        return;
    y = (item - top + 1) * 16;
    switch (item)
    {
    case MENUITEM_TEXTSPEED:
        DrawChoices3(gText_BwOpt_Slow, gText_BwOpt_Mid, gText_BwOpt_Fast, 104, 148, 192, y, selection);
        break;
    case MENUITEM_BATTLESCENE:
        DrawChoices2(gText_BwOpt_On, gText_BwOpt_Off, 112, 162, y, selection);
        break;
    case MENUITEM_BATTLESTYLE:
        DrawChoices2(gText_BwOpt_Shift, gText_BwOpt_Set, 112, 172, y, selection);
        break;
    case MENUITEM_SOUND:
        DrawChoices2(gText_BwOpt_Mono, gText_BwOpt_Stereo, 112, 172, y, selection);
        break;
    case MENUITEM_FIELDSPEED:
    case MENUITEM_BATTLESPEED:
        DrawSpeedChoice(selection, y);
        break;
    case MENUITEM_DIFFICULTY:
        DrawDifficultyChoice(selection, y);
        break;
#ifndef HACK_NO_MOEMON
    case MENUITEM_MOEMON:
        DrawChoices2(gText_BwOpt_MoemonOn, gText_BwOpt_MoemonOff, 112, 162, y, selection);
        break;
#endif
    case MENUITEM_HABITAT:
        DrawChoices2(gText_BwOpt_Shift, gText_BwOpt_HabitatOrig, 112, 172, y, selection);
        break;
    case MENUITEM_WONDERTRADE:
        DrawChoices2(gText_BwOpt_On, gText_BwOpt_Off, 112, 162, y, selection);
        break;
    case MENUITEM_WONDERTYPE:
        DrawWonderTypeChoice(selection, y);
        break;
    case MENUITEM_NETTRADE:
        DrawChoices2(gText_BwOpt_NetTradeUse, gText_BwOpt_NetTradeNotUse, 112, 162, y, selection);
        break;
#if HACK_RANDOM_WEATHER_ENABLED
    case MENUITEM_WEATHER:
        DrawChoices2(gText_BwOpt_WeatherRandom, gText_BwOpt_WeatherFixed, 112, 172, y, selection);
        break;
#endif
    case MENUITEM_NICKNAME:
        DrawChoices2(gText_BwOpt_NicknameAsk, gText_BwOpt_NicknameNoAsk, 112, 162, y, selection);
        break;
    case MENUITEM_AFFECTION:
        DrawChoices2(gText_BwOpt_AffectionOn, gText_BwOpt_AffectionOff, 112, 172, y, selection);
        break;
    case MENUITEM_CH_NOITEMS:
    case MENUITEM_CH_NOHELD:
    case MENUITEM_CH_CENTERFEE:
    case MENUITEM_CH_PCFEE:
    case MENUITEM_CH_LENTRANT:
    case MENUITEM_CH_DEATH:
    case MENUITEM_CH_FIRSTONLY:
    case MENUITEM_CH_NICKNAME:
    case MENUITEM_CHB_LEVELCAP:
    case MENUITEM_CHB_NOSWITCH:
    case MENUITEM_CHB_NOWILDEXP:
    case MENUITEM_CHB_DOUBLEDMG:
    case MENUITEM_CHB_NOFLY:
    case MENUITEM_CH_CENTERBAN:
    case MENUITEM_CH_SHOPBAN:
    case MENUITEM_CH_NOREVIVE:
    case MENUITEM_CH_MONOTYPE:
    case MENUITEM_CH_RANDOM:
        DrawChoices2(gText_BwOpt_ChOff, gText_BwOpt_ChOn, 112, 162, y, selection);   /* 0 OFF / 1 ON（旗が立っている＝ ON） */
        break;
    case BWOPT_ITEM_FUUIN:   /* ふういんルーレット（仮の行・#123）: MENUITEM の列の外 */
    case BWOPT_ITEM_LUXRAY:  /* ぜんぶレントラー（仮の行・patches/0609） */
    case BWOPT_ITEM_INVERSE:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_BONUS:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_BLIND:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_ONESHOT:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_ROTATION:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_GACHA:   /* 縛り F（仮の行・0.63.0） */
        DrawChoices2(gText_BwOpt_ChOff, gText_BwOpt_ChOn, 112, 162, y, selection);
        break;
    case BWOPT_ITEM_PBSKIP:   /* きのみブレンダーの ミニゲーム（仮の行・段 4c）: 0 あそぶ／1 スキップ */
        DrawChoices2(gText_BwOpt_PbPlay, gText_BwOpt_PbSkipOn, 112, 162, y, selection);
        break;
    }
    CopyWindowToVram(WIN_TEXT, 2);
}

static u8 CycleInput(u8 selection, u8 count, s16 *arrow)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        selection = (selection + 1 < count) ? selection + 1 : 0;
        *arrow = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        selection = (selection != 0) ? selection - 1 : count - 1;
        *arrow = TRUE;
    }
    return selection;
}

static u8 ProcessInputFor(u8 item, u8 selection, s16 *arrow)
{
    u8 prev = selection;

    switch (item)
    {
    case MENUITEM_TEXTSPEED:
        selection = CycleInput(selection, 3, arrow);
        break;
    case MENUITEM_BATTLESCENE:
    case MENUITEM_BATTLESTYLE:
        selection = CycleInput(selection, 2, arrow);
        break;
    case MENUITEM_SOUND:
        selection = CycleInput(selection, 2, arrow);
        if (selection != prev)
            SetPokemonCryStereo(selection);
        break;
    case MENUITEM_FIELDSPEED:
    case MENUITEM_BATTLESPEED:
        selection = CycleInput(selection, BWOPT_SPEED_COUNT, arrow);
        break;
    case MENUITEM_DIFFICULTY:
        selection = CycleInput(selection, DIFFICULTY_COUNT, arrow);
        break;
#ifndef HACK_NO_MOEMON
    case MENUITEM_MOEMON:
#endif
    case MENUITEM_HABITAT:
    case MENUITEM_WONDERTRADE:
    case MENUITEM_NETTRADE:
#if HACK_RANDOM_WEATHER_ENABLED
    case MENUITEM_WEATHER:
#endif
    case MENUITEM_NICKNAME:
    case MENUITEM_AFFECTION:
    case MENUITEM_CH_NOITEMS:
    case MENUITEM_CH_NOHELD:
    case MENUITEM_CH_CENTERFEE:
    case MENUITEM_CH_PCFEE:
    case MENUITEM_CH_LENTRANT:
    case MENUITEM_CH_DEATH:
    case MENUITEM_CH_FIRSTONLY:
    case MENUITEM_CH_NICKNAME:
    case MENUITEM_CHB_LEVELCAP:
    case MENUITEM_CHB_NOSWITCH:
    case MENUITEM_CHB_NOWILDEXP:
    case MENUITEM_CHB_DOUBLEDMG:
    case MENUITEM_CHB_NOFLY:
    case MENUITEM_CH_CENTERBAN:
    case MENUITEM_CH_SHOPBAN:
    case MENUITEM_CH_NOREVIVE:
    case MENUITEM_CH_MONOTYPE:
    case MENUITEM_CH_RANDOM:
        selection = CycleInput(selection, 2, arrow);
        break;
    case BWOPT_ITEM_FUUIN:   /* ふういんルーレット（仮の行・#123） */
    case BWOPT_ITEM_LUXRAY:  /* ぜんぶレントラー（仮の行・patches/0609） */
    case BWOPT_ITEM_INVERSE:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_BONUS:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_BLIND:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_ONESHOT:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_ROTATION:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_GACHA:   /* 縛り F（仮の行・0.63.0） */
    case BWOPT_ITEM_PBSKIP:   /* きのみブレンダー（仮の行・段 4c） */
        selection = CycleInput(selection, 2, arrow);
        break;
    case MENUITEM_WONDERTYPE:
        selection = CycleInput(selection, WONDER_TRADE_TYPE_COUNT + 1, arrow);
        break;
    }
    return selection;
}

/* ---- 画面の流れ ---- */

static void MainCB2_BwOptionMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_BwOptionMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_BwOptionMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        SetHBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, (void *)OAM, OAM_SIZE);
        DmaClear16(3, (void *)PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
        ChangeBgX(0, 0, 0); ChangeBgY(0, 0, 0);
        ChangeBgX(1, 0, 0); ChangeBgY(1, 0, 0);
        ChangeBgX(2, 0, 0); ChangeBgY(2, 0, 0);
        ChangeBgX(3, 0, 0); ChangeBgY(3, 0, 0);
        if (!InitWindows(sWindowTemplates))
        {
            FreeAllWindowBuffers();
            SetMainCallback2(gMain.savedCallback);
            return;
        }
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        gMain.state++;
        break;
    case 2:
        ResetSpriteData();
        ResetPaletteFade();
        FreeAllSpritePalettes();
        ResetTasks();
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 3:
        LoadFrameGfx(gSaveBlock2->optionsWindowFrameType);
        gMain.state++;
        break;
    case 4:
        LZ77UnCompVram(gBwOptTiles, (void *)VRAM);
        LZ77UnCompVram(gBwOptMap0, (void *)(VRAM + 0x3800));
        LoadPalette(gBwOptTextPal, 0x50, 0xE0);
        gMain.state++;
        break;
    case 5:
        PutWindowTilemap(WIN_TEXT);
        DrawTexts();
        PutWindowTilemap(WIN_FRAME);
        DrawFramePreview();
        gMain.state++;
        break;
    case 6:
    {
        u8 taskId = CreateTask(Task_FadeIn, 0);
        s16 *data = gTasks[taskId].data;
        u8 i;

        data[TD_MENUSELECTION] = 0;
        data[TD_TEXTSPEED] = gSaveBlock2->optionsTextSpeed;
        if (data[TD_TEXTSPEED] > OPTIONS_TEXT_SPEED_FAST)
            data[TD_TEXTSPEED] = OPTIONS_TEXT_SPEED_MID;
        data[TD_BATTLESCENE] = gSaveBlock2->optionsBattleSceneOff;
        data[TD_BATTLESTYLE] = gSaveBlock2->optionsBattleStyle;
        data[TD_SOUND] = gSaveBlock2->optionsSound;
        data[TD_FIELDSPEED] = VarGet(HOENN_VAR_OVERWORLD_SPEED);
        if (data[TD_FIELDSPEED] >= BWOPT_SPEED_COUNT)
            data[TD_FIELDSPEED] = 0;
        data[TD_DIFFICULTY] = HackDifficulty_SelectionFromValue(VarGet(HOENN_VAR_DIFFICULTY));
        data[TD_BATTLESPEED] = HackBattleSpeed_Get();
#ifndef HACK_NO_MOEMON
        data[TD_MOEMON] = FlagGet(HACK_FLAG_MOEMON_ON) ? 0 : 1;          /* 0 だす / 1 ださない */
#endif
        data[TD_HABITAT] = FlagGet(HACK_FLAG_HABITAT_ORIGINAL) ? 1 : 0;  /* 0 いれかえ / 1 もとのまま */
        data[TD_WONDERTRADE] = FlagGet(HACK_FLAG_WONDER_SKIP) ? 1 : 0;    /* 0 みる / 1 みない */
        data[TD_WONDERTYPE] = VarGet(HACK_VAR_WONDER_TYPE) <= WONDER_TRADE_TYPE_COUNT ? VarGet(HACK_VAR_WONDER_TYPE) : 0;   /* 0 ぜんぶ */
        data[TD_NETTRADE] = FlagGet(HACK_FLAG_NET_TRADE_BUTTON) ? 0 : 1;   /* 0 つかう / 1 つかわない */
#if HACK_RANDOM_WEATHER_ENABLED
        data[TD_WEATHER] = FlagGet(HACK_FLAG_WEATHER_FIXED) ? 1 : 0;       /* 0 ランダム / 1 きまり */
#endif
        data[TD_NICKNAME] = FlagGet(FLAG_DONT_OFFER_NICKNAMES_BATTLE) ? 1 : 0;   /* 0 きかれる / 1 きかれない */
        data[TD_AFFECTION] = FlagGet(HACK_FLAG_AFFECTION_OFF) ? 1 : 0;           /* 0 こうか あり / 1 こうか なし */
        data[TD_CH_CENTERBAN] = FlagGet(HACK_FLAG_CHC_CENTERBAN) ? 1 : 0;      /* 縛り C（patches/0550・ui_integration.patch の文脈の手前に置く） */
        data[TD_CH_SHOPBAN] = FlagGet(HACK_FLAG_CHC_SHOPBAN) ? 1 : 0;
        data[TD_CH_NOREVIVE] = FlagGet(HACK_FLAG_CHC_NOREVIVE) ? 1 : 0;
        data[TD_CH_MONOTYPE] = FlagGet(HACK_FLAG_CHC_MONOTYPE) ? 1 : 0;
        data[TD_CH_RANDOM] = FlagGet(HACK_FLAG_CHC_RANDOM) ? 1 : 0;
        data[TD_CH_NOITEMS] = FlagGet(HACK_FLAG_CH_NOITEMS) ? 1 : 0;           /* 0 OFF / 1 ON（しばり・既定は全部 OFF） */
        data[TD_CH_NOHELD] = FlagGet(HACK_FLAG_CH_NOHELD) ? 1 : 0;
        data[TD_CH_CENTERFEE] = FlagGet(HACK_FLAG_CH_CENTERFEE) ? 1 : 0;
        data[TD_CH_PCFEE] = FlagGet(HACK_FLAG_CH_PCFEE) ? 1 : 0;
        data[TD_CH_LENTRANT] = FlagGet(HACK_FLAG_CH_LENTRANT) ? 1 : 0;
        data[TD_CH_DEATH] = FlagGet(HACK_FLAG_CH_DEATH) ? 1 : 0;               /* 縛り A（patches/0530） */
        data[TD_CH_FIRSTONLY] = FlagGet(HACK_FLAG_CH_FIRSTONLY) ? 1 : 0;
        data[TD_CH_NICKNAME] = FlagGet(HACK_FLAG_CH_NICKNAME) ? 1 : 0;
        data[TD_CHB_LEVELCAP] = FlagGet(HACK_FLAG_CHB_LEVELCAP) ? 1 : 0;       /* 0.50.3 しばり B（既定は全部 OFF） */
        data[TD_CHB_NOSWITCH] = FlagGet(HACK_FLAG_CHB_NOSWITCH) ? 1 : 0;
        data[TD_CHB_NOWILDEXP] = FlagGet(HACK_FLAG_CHB_NOWILDEXP) ? 1 : 0;
        data[TD_CHB_DOUBLEDMG] = FlagGet(HACK_FLAG_CHB_DOUBLEDMG) ? 1 : 0;
        data[TD_CHB_NOFLY] = FlagGet(HACK_FLAG_CHB_NOFLY) ? 1 : 0;
        data[TD_CH_FUUIN] = FlagGet(HACK_FLAG_CHD_FUUIN) ? 1 : 0;       /* ふういんルーレット（#123・patches/0560・既定は OFF） */
        data[TD_CH_LUXRAY] = FlagGet(HACK_FLAG_CHE_LUXRAY) ? 1 : 0;      /* ぜんぶレントラー（patches/0609・既定は OFF） */
        data[TD_CHF_INVERSE] = FlagGet(HACK_FLAG_CHF_INVERSE) ? 1 : 0;   /* 縛り F（0.63.0・既定は OFF） */
        data[TD_CHF_BONUS] = FlagGet(HACK_FLAG_CHF_BONUS) ? 1 : 0;   /* 縛り F（0.63.0・既定は OFF） */
        data[TD_CHF_BLIND] = FlagGet(HACK_FLAG_CHF_BLIND) ? 1 : 0;   /* 縛り F（0.63.0・既定は OFF） */
        data[TD_CHF_ONESHOT] = FlagGet(HACK_FLAG_CHF_ONESHOT) ? 1 : 0;   /* 縛り F（0.63.0・既定は OFF） */
        data[TD_CHF_ROTATION] = FlagGet(HACK_FLAG_CHF_ROTATION) ? 1 : 0;   /* 縛り F（0.63.0・既定は OFF） */
        data[TD_CHF_GACHA] = FlagGet(HACK_FLAG_CHF_GACHA) ? 1 : 0;   /* 縛り F（0.63.0・既定は OFF） */
        data[TD_PB_SKIP] = FlagGet(HACK_FLAG_PB_BLENDER_SKIP) ? 1 : 0;   /* きのみブレンダー（段 4c・既定は あそぶ） */
        data[TD_TOP] = 0;
        for (i = 0; i < MENUITEM_COUNT; ++i)
            DrawChoicesFor(i, data[TD_TEXTSPEED + i], 0);
        CopyWindowToVram(WIN_TEXT, 3);
        gMain.state++;
        break;
    }
    case 7:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_BwOptionMenu);
        SetMainCallback2(MainCB2_BwOptionMenu);
        return;
    }
}

static void Task_FadeIn(u8 taskId)
{
    if (!gPaletteFade->active)
        gTasks[taskId].func = Task_ProcessInput;
}

static void Task_ProcessInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u8 item = data[TD_MENUSELECTION];

    if (JOY_NEW(A_BUTTON))
    {
        data[TD_TIMER] = 20;
        gTasks[taskId].func = Task_Save;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        data[TD_TIMER] = 20;
        gTasks[taskId].func = Task_Cancel;
    }
    else if (JOY_NEW(DPAD_UP) || JOY_NEW(DPAD_DOWN))
    {
        if (JOY_NEW(DPAD_UP))
            item = (item > 0) ? item - 1 : MENUITEM_COUNT - 1;
        else
            item = (item + 1 < MENUITEM_COUNT) ? item + 1 : 0;
        data[TD_MENUSELECTION] = item;
        if (item < data[TD_TOP] || item >= data[TD_TOP] + BWOPT_VISIBLE_ROWS)
        {
            data[TD_TOP] = (item < data[TD_TOP]) ? item : item - BWOPT_VISIBLE_ROWS + 1;
            DrawRows(data);
        }
        LZ77UnCompVram(sMaps[item - data[TD_TOP]], (void *)(VRAM + 0x3800));
        ShowDescriptionFor(item, data);
    }
    else
    {
        u8 prev = data[TD_TEXTSPEED + item];
        s16 arrow = FALSE;   //★ 2026-10-12 元の TD_ARROW_PRESSED（このコマの中でしか使わない）→ 手元の変数に＝task data の欄を 1 つ空けた（#106 の「てんき」の行・5e）

        data[TD_TEXTSPEED + item] = ProcessInputFor(item, prev, &arrow);
        if (data[TD_TEXTSPEED + item] != prev)
        {
            DrawChoicesFor(item, data[TD_TEXTSPEED + item], data[TD_TOP]);
            if (item == MENUITEM_DIFFICULTY)
                ShowDescriptionFor(item, data);    /* ももアイスに合わせた／外したとき注意の文を出し入れ */
        }
        if (arrow)
            CopyWindowToVram(WIN_TEXT, 2);
    }
}

/* しばり（#118）: ON なら旗を立てる。ON だったものを OFF にした瞬間は「しばりをはずした印」HACK_FLAG_CH_BROKEN を立てる（下りない＝最後のバッジの特別なメッセージが出なくなる） */
static void SetChallengeFlag(u16 flag, s16 value)
{
    if (value == 1)
    {
        if (flag == HACK_FLAG_CHF_BONUS && !FlagGet(flag))
            HackChallengeF_BonusSyncPaid();   /* しばりボーナスを ON にする直前に 払った数を今のバッジにそろえる（OFF のあいだは毎コマの確かめを止めた・0.63.1） */
        FlagSet(flag);
    }
    else
    {
        if (FlagGet(flag))
            FlagSet(HACK_FLAG_CH_BROKEN);
        FlagClear(flag);
    }
}

static void Task_Save(u8 taskId)
{

    s16 *data = gTasks[taskId].data;

    if (data[TD_TIMER] == 20)
        ShowDescription(DESC_SAVE);
    if (data[TD_TIMER])
    {
        data[TD_TIMER]--;
        return;
    }
    gSaveBlock2->optionsTextSpeed = data[TD_TEXTSPEED];
    gSaveBlock2->optionsBattleSceneOff = data[TD_BATTLESCENE];
    gSaveBlock2->optionsBattleStyle = data[TD_BATTLESTYLE];
    gSaveBlock2->optionsSound = data[TD_SOUND];
    VarSet(HOENN_VAR_OVERWORLD_SPEED, data[TD_FIELDSPEED]);
    HackBattleSpeed_Set(data[TD_BATTLESPEED]);
#ifndef HACK_NO_MOEMON
    if (data[TD_MOEMON] == 0)
        FlagSet(HACK_FLAG_MOEMON_ON);
    else
        FlagClear(HACK_FLAG_MOEMON_ON);
#endif
    if (data[TD_HABITAT] == 1)
        FlagSet(HACK_FLAG_HABITAT_ORIGINAL);
    else
        FlagClear(HACK_FLAG_HABITAT_ORIGINAL);
    if (data[TD_WONDERTRADE] == 1)
        FlagSet(HACK_FLAG_WONDER_SKIP);
    else
        FlagClear(HACK_FLAG_WONDER_SKIP);
    VarSet(HACK_VAR_WONDER_TYPE, data[TD_WONDERTYPE]);
    if (data[TD_NETTRADE] == 0)
        FlagSet(HACK_FLAG_NET_TRADE_BUTTON);
    else
        FlagClear(HACK_FLAG_NET_TRADE_BUTTON);
#if HACK_RANDOM_WEATHER_ENABLED
    if (data[TD_WEATHER] == 1)
        FlagSet(HACK_FLAG_WEATHER_FIXED);
    else
        FlagClear(HACK_FLAG_WEATHER_FIXED);
#endif
    if (data[TD_NICKNAME] == 1)
        FlagSet(FLAG_DONT_OFFER_NICKNAMES_BATTLE);
    else
        FlagClear(FLAG_DONT_OFFER_NICKNAMES_BATTLE);
    if (data[TD_AFFECTION] == 1)
        FlagSet(HACK_FLAG_AFFECTION_OFF);
    else
        FlagClear(HACK_FLAG_AFFECTION_OFF);
    SetChallengeFlag(HACK_FLAG_CH_NOITEMS, data[TD_CH_NOITEMS]);
    SetChallengeFlag(HACK_FLAG_CH_NOHELD, data[TD_CH_NOHELD]);
    SetChallengeFlag(HACK_FLAG_CH_CENTERFEE, data[TD_CH_CENTERFEE]);
    SetChallengeFlag(HACK_FLAG_CH_PCFEE, data[TD_CH_PCFEE]);
    SetChallengeFlag(HACK_FLAG_CH_LENTRANT, data[TD_CH_LENTRANT]);
    SetChallengeFlag(HACK_FLAG_CH_DEATH, data[TD_CH_DEATH]);
    SetChallengeFlag(HACK_FLAG_CH_FIRSTONLY, data[TD_CH_FIRSTONLY]);
    SetChallengeFlag(HACK_FLAG_CH_NICKNAME, data[TD_CH_NICKNAME]);
    SetChallengeFlag(HACK_FLAG_CHB_LEVELCAP, data[TD_CHB_LEVELCAP]);
    SetChallengeFlag(HACK_FLAG_CHB_NOSWITCH, data[TD_CHB_NOSWITCH]);
    SetChallengeFlag(HACK_FLAG_CHB_NOWILDEXP, data[TD_CHB_NOWILDEXP]);
    SetChallengeFlag(HACK_FLAG_CHB_DOUBLEDMG, data[TD_CHB_DOUBLEDMG]);
    SetChallengeFlag(HACK_FLAG_CHB_NOFLY, data[TD_CHB_NOFLY]);
    SetChallengeFlag(HACK_FLAG_CHC_CENTERBAN, data[TD_CH_CENTERBAN]);   /* 縛り C（patches/0550） */
    SetChallengeFlag(HACK_FLAG_CHC_SHOPBAN, data[TD_CH_SHOPBAN]);
    SetChallengeFlag(HACK_FLAG_CHC_NOREVIVE, data[TD_CH_NOREVIVE]);
    SetChallengeFlag(HACK_FLAG_CHC_MONOTYPE, data[TD_CH_MONOTYPE]);
    SetChallengeFlag(HACK_FLAG_CHC_RANDOM, data[TD_CH_RANDOM]);
    SetChallengeFlag(HACK_FLAG_CHD_FUUIN, data[TD_CH_FUUIN]);   /* ふういんルーレット（#123） */
    SetChallengeFlag(HACK_FLAG_CHE_LUXRAY, data[TD_CH_LUXRAY]);   /* ぜんぶレントラー（patches/0609） */
    SetChallengeFlag(HACK_FLAG_CHF_INVERSE, data[TD_CHF_INVERSE]);   /* 縛り F（0.63.0） */
    SetChallengeFlag(HACK_FLAG_CHF_BONUS, data[TD_CHF_BONUS]);   /* 縛り F（0.63.0） */
    SetChallengeFlag(HACK_FLAG_CHF_BLIND, data[TD_CHF_BLIND]);   /* 縛り F（0.63.0） */
    SetChallengeFlag(HACK_FLAG_CHF_ONESHOT, data[TD_CHF_ONESHOT]);   /* 縛り F（0.63.0） */
    SetChallengeFlag(HACK_FLAG_CHF_ROTATION, data[TD_CHF_ROTATION]);   /* 縛り F（0.63.0） */
    SetChallengeFlag(HACK_FLAG_CHF_GACHA, data[TD_CHF_GACHA]);   /* 縛り F（0.63.0） */
    /* きのみブレンダー（段 4c）: しばりではない＝SetChallengeFlag は使わない（『しばりを はずした印』を立てない） */
    if (data[TD_PB_SKIP])
        FlagSet(HACK_FLAG_PB_BLENDER_SKIP);
    else
        FlagClear(HACK_FLAG_PB_BLENDER_SKIP);
    VarSet(HOENN_VAR_DIFFICULTY, HackDifficulty_ValueFromSelection(data[TD_DIFFICULTY]));   /* ★ わくはもう変えない（セーブの値のまま・枠の見本もその値） */
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_FadeOut;
}

static void Task_Cancel(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (data[TD_TIMER] == 20)
        ShowDescription(DESC_CANCEL);
    if (data[TD_TIMER])
    {
        data[TD_TIMER]--;
        return;
    }
    SetPokemonCryStereo(gSaveBlock2->optionsSound);
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_FadeOut;
}

static void Task_FadeOut(u8 taskId)
{
    if (!gPaletteFade->active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}
