#pragma once
#include "../include/global.h"

struct ScriptContext;   /* include/script.h */
struct Pokemon;

/*
    しばり 縛り C（0.50.3・#121・patches/0550・docs/research/2026-10-16_settings-folders-challenges.md 13 章）: ポケセンきんし・おみせきんし・ひんしのどうぐきんし・モノタイプ・ランダム＋
    ポケセンの縛りがあるときの戻り先をマサラタウンの家に（全滅・バージョンアップのワープ）。
    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: 縛り A（hack_challenge.h）・縛り B（hack_challenge_b.h）の次・**縛り C ここ 0xFBD〜0xFC4**（使うのは 0xFBD〜0xFC2・0xFC3〜0xFC4 は予備）→ 次の空きは 0xFC5。
    変数は増やさない。モノタイプの置き場は SaveBlock2 の filler_B20 +0xD0〜0xF7（A は +0x100〜・せっていの並びは +0〜79）。
*/
#define HACK_FLAG_CHC_CENTERBAN  0xFBD   /* ポケセンきんし: ポケモンセンターの看護師は回復してくれない（入るのは可・家のベッドなどは可） */
#define HACK_FLAG_CHC_SHOPBAN    0xFBE   /* おみせきんし: お店（pokemart の命令を使う売り場すべて）で買えない・売れない（売るも止まる＝仮） */
#define HACK_FLAG_CHC_NOREVIVE   0xFBF   /* ひんしのどうぐきんし: げんきのかけら・げんきのかたまり・ふっかつそう・せいなるはい が つかえない */
#define HACK_FLAG_CHC_MONOTYPE   0xFC0   /* モノタイプ: 最初の 1 匹と 1 つでも同じタイプを持つポケモンしか 戦闘に出せない（出せない子は戦闘のあいだだけ ひんし あつかい） */
#define HACK_FLAG_CHC_RANDOM     0xFC1   /* ランダム: 野生（草むら・つり・いわくだき など）・もらうポケモン・トレーナーの手持ちの種族を 図鑑 1〜1039 の基本の姿から ランダムに（セーブごとに固定） */
#define HACK_FLAG_CHC_WRATH_MSG  0xFC2   /* ？？？の いかりに ふれた あとの メッセージを まだ出していない印（次の自由なコマで 1 回出して下ろす） */
#define HACK_CHC_FLAG_LAST       0xFC4   /* 縛り C の枠の終わり（予備 0xFC3〜0xFC4） */

#define HACK_CHC_COUNT           5       /* せっていの行になる縛り C の数（最後のバッジの数え方・合計） */

/* マサラタウンの主人公の家 1F（素 FRLG の全滅のあとの戻り先と同じ: 群 4・番号 0・x 8・y 5） */
#define HACK_CHC_HOME_GROUP      4
#define HACK_CHC_HOME_NUM        0
#define HACK_CHC_HOME_X          8
#define HACK_CHC_HOME_Y          5

/* モノタイプの置き場（SaveBlock2 filler_B20 +0xD0〜0xF7・40 バイト）: [0] 1 つ目のタイプ + 1・[1] 2 つ目のタイプ + 1（0 = まだ記録していない）・
   [2] 印の数（戦闘のあいだ ひんしにした子の数）・[3] 予備・[4 + 6n] 子の性格値（u32）・[8 + 6n] 戦う前の HP（u16） */
#define HACK_CHC_WRATH_TOUCH     1       /* いかりの種類: こちらから ？？？に ダメージ（HP が減る・0 になる） */
#define HACK_CHC_WRATH_HIT       2       /* ？？？の攻撃が こちらに当たった */
#define HACK_CHC_WRATH_TIME      3       /* ターン数の限り（ぬけにん・まもる連打・ねむる など 戦闘が終わらない道をふさぐ） */
#define HACK_CHC_LENTRANT_TURNS  1       /* ？？？との戦いは この数のターンの終わりで かならず ぜんめつで終わる（ユーザー 2026-10-16「1 ターンにしておくか、対策されたくないし」・定数はここ 1 か所・6 → 1） */

#define HACK_CHC_MONO_OFFSET     0xD0
#define HACK_CHC_MONO_SLOTS      6

bool8 HackChallengeC_CenterBound(void);                     /* ポケセンの縛り（りょうきん・？？？・きんし）が 1 つでも ON か（戻り先をマサラにする） */
bool8 HackChallengeC_AnyOn(void);                           /* 縛り C のどれかが ON か（最後のバッジの条件） */
u8 HackChallengeC_CountOn(void);                            /* ON の数（最後のバッジの「n この しばり」） */
bool8 HackChallengeC_ReviveBlocked(u16 item);               /* ひんしのどうぐきんし: この道具は つかえないか */
u8 HackChallengeC_WhiteoutHome(void);                       /* 全滅の戻り先（region_fly_hook.s）: 1 = マサラの家（手持ちを全回復した）・0 = 今までどおり */
bool8 HackChallengeC_VersionHome(void);                     /* バージョンアップのワープ（hack_save_version.c）: TRUE = マサラの家へワープさせた（全回復） */
bool8 HackChallengeC_ScrCmdPokemart(struct ScriptContext* ctx);   /* script コマンド 0x86 pokemart の置き換え（routinepointers） */
void HackChallengeC_OnBattleStart(void);                    /* CreateBattleStartTask の入口（challenge_c_hooks.s）: モノタイプ — 出せない子を ひんしにする */
void HackChallengeC_MonoRestore(void);                      /* 戦闘のあと（end_battle.c の EndOfBattleThings の頭）/ フィールドの安全装置: ひんしにした子の HP を戻す */
void HackChallengeC_FieldTick(void);                        /* フィールドの毎コマ（hack_challenge.c の FieldTick から）: モノタイプの記録・安全装置・？？？のあとのメッセージ */
u16 HackChallengeC_RandomWild(u16 species);                 /* wild_encounter.c の CreateWildMon の頭 */
u16 HackChallengeC_RandomGift(u16 species);                 /* build_pokemon.c の ScriptGiveMon の頭 */
void HackChallengeC_RandomTrainerMon(struct Pokemon* mon, u16 trainerId, u8 side, u8 index);   /* build_pokemon.c の CreateNPCTrainerParty（ClassPool の直後） */
bool8 HackChallengeC_IsLeagueTrainer(u16 trainerId);        /* 四天王・チャンピオン（再戦・強化版を含む）の番号か: ランダムでも相手の手持ちを入れ替えない */
void HackChallengeC_LentrantWrath(u8 kind);                 /* ？？？に ダメージ／HP が減った（1）・？？？の攻撃が当たった（2）→ 手持ち全員ひんし・戦闘は負け（hack_challenge.c から） */
void HackChallengeC_TurnLimit(void);                        /* end_turn.c の HandleDynamaxOnTurnEnd の頭: ？？？との戦いが 3 ターンたったら いかり（HACK_CHC_WRATH_TIME） */
bool8 HackChallengeC_NoSwitch(u8 bank);                    /* switching.c: ？？？戦の自分側は 自分からの入れ替えができない（PARTY_CANT_SWITCH） */
void HackChallengeC_LentrantRules(void);                  /* battle_start_turn_start.c の BattleBeginFirstTurn の頭（1 回）: ？？？戦は こちらの技を「ひっかく」だけに・Z／ダイマックス／テラス／メガを使えなくする */
void HackChallengeC_WrathKind(void);                        /* callasm（台本 EventScript_ChallengeWrath）: LASTRESULT = 1／2 */
