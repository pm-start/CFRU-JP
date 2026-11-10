#pragma once
#include "../include/global.h"

/*
    しばり B（#118・0.50.3・せってい の「しばり」フォルダの 5 つ・既定は全部 OFF）。docs/research/2026-10-16_challenges-b.md
    （しばり A＝ひんしでお別れ／最初の 1 匹だけ／ニックネーム必須 は別の枠・patches/0530）

    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: しばり（FA4〜FAC）→ しばり A（その次の 8 個）→ **FB5〜FB9 番（ここ・5 つ）** → 次の空き 0xFBA（FBA〜FBC も B の予備）。
    変数・一時変数は増やさない。
*/
#define HACK_FLAG_CHB_LEVELCAP   0xFB5   /* レベルキャップ: 自分のポケモンは バッジの数のレベルの上限（HackLevelCap_FromBadges）をこえて レベルが上がらない */
#define HACK_FLAG_CHB_NOSWITCH   0xFB6   /* こうたいきんし: 戦闘中に じぶんから いれかえられない（ひんし・技・道具で強制されるときは可） */
#define HACK_FLAG_CHB_NOWILDEXP  0xFB7   /* やせいけいけんなし: 野生のポケモンと戦っても けいけんちも努力値も入らない */
#define HACK_FLAG_CHB_DOUBLEDMG  0xFB8   /* ダメージ1.5ばい（旧 2ばい）: 相手の攻撃（damagecalc を通る攻撃）で自分のポケモンが受けるダメージが 1.5 倍 */
#define HACK_FLAG_CHB_NOFLY      0xFB9   /* そらをとぶきんし: 「そらをとぶ」と「ゆうらん」がフィールドで使えない */

#define HACK_CHB_COUNT           5       /* しばり B の数（最後のバッジのメッセージの数え方） */

u8 HackChallengeB_LevelCap(void);                              /* 自分のポケモンのレベルの上限（0 ＝ なし）。今いる地方のバッジによる上限（いつも・#124）と、レベルキャップの縛りが ON なら全地方の合計の上限の低いほう */
bool8 HackChallengeB_AtLevelCap(struct Pokemon* mon);          /* レベルが上限（上の関数）に届いている（exp.c の IsAffectedByHardLevelCap・party_menu.c のふしぎなアメ）。縛りが OFF でも効く */
const u8* HackChallengeB_CandyCappedText(void);               /* 手持ちの画面の上限の一言（「いまの バッジの じょうげんは Lv○○だから あがらない」・数字は LevelCap と同じ・上限なしなら ふつうのこうかがない）[hack] patches/0613 */
u8 HackChallengeB_ExpBlocked(u8 partyId, bool8 participant);   /* 戦闘の経験値を入れない: 0 入れる／1 上限（努力値は入る）／2 野生（努力値も入らない）。場に出ている子には一言（exp.c） */
u32 HackChallengeB_ClampCandyExp(u16 species, u32 exp);        /* けいけんアメの経験値を上限のレベルの経験値でとめる（OFF なら そのまま・exp_candy.c） */
u32 HackChallengeB_DaycareExp(u16 species, u32 originalExp, u32 newExp);   /* そだてやの経験値（daycare.c） */
bool8 HackChallengeB_SwitchBlocked(void);                      /* 戦闘で自分から いれかえられないか（通信・フロンティア・バトル施設は除く） */
void HackChallengeB_ShowSwitchBlocked(void);                   /* 戦闘のメニューで「いれかえ」を選んだとき（move_menu.c）: 音と一言 */
s32 HackChallengeB_ModifyDamage(u8 bankDef, s32 damage);       /* ダメージ1.5ばい（damage_calc.c の atk05_damagecalc） */
bool8 HackChallengeB_FlyBlocked(void);                         /* そらをとぶ・ゆうらん禁止（party_menu.c の SetUpFieldMove_Fly） */

/* 最後のバッジのメッセージ（hack_challenge.c の FinaleTick／HackChallenge_FinaleCount・台本 HackScr_ChallengeB_FinaleList） */
bool8 HackChallengeB_AnyOn(void);
u8 HackChallengeB_CountOn(void);
