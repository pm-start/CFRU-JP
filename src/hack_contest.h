#ifndef HACK_CONTEST_H
#define HACK_CONTEST_H

#include "defines.h"

/* コンテスト移植（FEAT-contest・patches/0985・S1）の土台。
 *
 * 固定 RAM の語 HACK_CONTEST_PTR（EWRAM 0x0203F040・4 バイト）= コンテストの状態構造体へのポインタ。
 *   NULL でなければ「コンテスト中」（素の FR の IsContest（0x08072F2C・常に 0）の 110 か所の呼び元は
 *   踏み台 HackContest_IsContest（hooks で素の 0x0800108C を置き換え）に付け替えてあり、この語を見て 0／1 を返す）。
 * 置き場: 空き 0x0203E274〜0x0203F0E8 の中（BGM 0x0203E300〜E800・RAM チートの影 0x0203EF80〜9F・EFA0・EFC0〜DF・計測 0x0203F800〜の外）。
 *   ROM の中でこの番地を指す語は src/hack_contest.c だけ（tools/rom/verify_contest_core.py が見る）。ほかの機能で使わない。
 * 寿命: Open／Close はコンテスト用の CB2（会場の画面）の入口と出口だけで呼ぶ。ヒープは地図の読み込みで初期化される（6.13）ので、
 *   フィールドの地図で Open したまま次へ進まない。念のため はじめから・つづきから・地図の読み込みの入口で
 *   HackContest_Reset（Free せずに語だけ NULL）を呼ぶ＝ここで残りがあってもコンテスト外の戦闘の技のアニメは今までどおり。 */
#define HACK_CONTEST_PTR (*(struct HackContestState* volatile*) 0x0203F040)

#define HACK_CONTEST_MAGIC 0x43544E31 /* 'CTN1' */

#define HACK_CONTEST_CONTESTANTS 4
#define HACK_CONTEST_BG_TILEMAP_SIZE 0x1000

/* 出場者 1 人分（S2a・日本語版の gContestMons[i] の必要な所だけ。0x40 バイトの原文の並びは使わない）。 */
struct HackContestMon
{
	u16 species;
	u32 otId;
	u32 personality;
	u8 nickname[POKEMON_NAME_LENGTH + 1];
	u8 trainerName[PLAYER_NAME_LENGTH + 1];
	u16 moves[4];             /* S2b: 技（0＝無し） */
	u8 stats[5];              /* S2b: コンディション 6 値（かっこよさ・うつくしさ・かわいさ・かしこさ・たくましさの順）＋ */
	u8 sheen;                 /* 　　　　　　　　　　　　　　　　　　　　　　　　　つや。debug は固定値の仮（本物は hack_condition.c の読み口） */
	u32 aiFlags;              /* S3: 原文 ContestPokemon.aiFlags（どの台本を回すか・1<<0〜6 だけが本物・残りは AI_Nothing） */
};

/* S2b: 出場者ごとの状態（原文 ContestantStatus の、技を選ぶ画面・順番の札・箱の並びが読む所だけ。ターンの処理は次の担当で足す） */
struct HackContestStatus
{
	u16 prevMove;
	u16 currMove;
	u8 hasJudgesAttention;
	u8 repeatedMove;
	u8 nervous;
	u8 noMoreTurns;
	u8 numTurnsSkipped;
	u8 turnOrderMod;          /* 0＝変わらない 1＝番号つき 2＝ばらばら */
	u8 nextTurnOrder;         /* 0xFF＝指定なし（CONTESTANT_NONE） */
	u8 pad;
	/* ここから S2c（原文 ContestantStatus の残り・ターンの処理が読み書きする所。効果 48 の中身（S3）が増やす所はここに足す） */
	s16 baseAppeal;
	s16 appeal;
	s16 pointTotal;
	s8 condition;
	u8 moveCategory;          /* 0〜4（5＝無効） */
	u8 jam;
	u8 jamReduction;
	u8 jamSafetyCount;
	u8 effectStringId;        /* 0xFF＝無し（CONTEST_STRING_NONE） */
	u8 effectStringId2;
	u8 conditionMod;          /* 0 変化なし 1 上がった 2 下がった */
	u8 turnOrderModAction;    /* 1 先頭 2 最後 3 ばらばら */
	u8 moveRepeatCount;
	u8 repeatJam;
	u8 comboAppealBonus;
	u8 completedCombo;
	u8 usedComboMove;
	u8 judgesAttentionWasRemoved;
	u8 turnSkipped;
	u8 exploded;
	u8 resistant;
	u8 immune;
	u8 overrideCategoryExcitementMod;
	u8 appealTripleCondition;
	u8 ranking;
	u8 attentionLevel;
	u8 contestantAnimTarget;
	u8 unnervedPokes;         /* eContestAppealResults.unnervedPokes[i] */
	/* ここから S3（効果 48・AI が読み書きする所） */
	u8 moreEasilyStartled;
	u8 usedRepeatableMove;
	u8 completedComboFlag;    /* 原文 completedComboFlag（AI が読む・立てるだけ） */
	u8 pad4;
};

/* S3: 原文 contest.h の CONTEST_STRING_*（sAppealResultTexts の添字・hack_contest_main.c の表と同じ並び） */
enum
{
	HCS_MORE_CONSCIOUS, HCS_NO_APPEAL, HCS_SETTLE_DOWN, HCS_OBLIVIOUS_TO_OTHERS, HCS_LESS_AWARE, HCS_STOPPED_CARING,
	HCS_STARTLE_ATTEMPT, HCS_DAZZLE_ATTEMPT, HCS_JUDGE_LOOK_AWAY2, HCS_UNNERVE_ATTEMPT, HCS_NERVOUS, HCS_UNNERVE_WAITING,
	HCS_TAUNT_WELL, HCS_REGAINED_FORM, HCS_JAM_WELL, HCS_HUSTLE_STANDOUT, HCS_WORK_HARD_UNNOTICED, HCS_WORK_BEFORE,
	HCS_APPEAL_NOT_WELL, HCS_WORK_PRECEDING, HCS_APPEAL_NOT_WELL2, HCS_APPEAL_NOT_SHOWN_WELL, HCS_APPEAL_SLIGHTLY_WELL,
	HCS_APPEAL_PRETTY_WELL, HCS_APPEAL_EXCELLENTLY, HCS_APPEAL_DUD, HCS_APPEAL_NOT_VERY_WELL, HCS_APPEAL_SLIGHTLY_WELL2,
	HCS_APPEAL_PRETTY_WELL2, HCS_APPEAL_VERY_WELL, HCS_APPEAL_EXCELLENTLY2, HCS_SAME_TYPE_GOOD, HCS_DIFF_TYPE_GOOD,
	HCS_STOOD_OUT_AS_MUCH, HCS_NOT_AS_WELL, HCS_CONDITION_ROSE, HCS_HOT_STATUS, HCS_MOVE_UP_LINE, HCS_MOVE_BACK_LINE,
	HCS_SCRAMBLE_ORDER, HCS_JUDGE_EXPECTANTLY2, HCS_WENT_OVER_WELL, HCS_WENT_OVER_VERY_WELL, HCS_APPEAL_COMBO_EXCELLENTLY,
	HCS_AVERT_GAZE, HCS_AVOID_SEEING, HCS_NOT_FAZED, HCS_LITTLE_DISTRACTED, HCS_ATTEMPT_STARTLE, HCS_LOOKED_DOWN,
	HCS_TURNED_BACK, HCS_UTTER_CRY, HCS_LEAPT_UP, HCS_TRIPPED_OVER, HCS_MESSED_UP2, HCS_FAILED_TARGET_NERVOUS,
	HCS_FAILED_ANYONE_NERVOUS, HCS_IGNORED, HCS_NO_CONDITION_IMPROVE, HCS_BAD_CONDITION_WEAK_APPEAL, HCS_UNAFFECTED,
	HCS_ATTRACTED_ATTENTION,
	HCS_COUNT /* 62 */
};

/* S2c: 技のアニメに渡す種族（原文 ContestMoveAnimData）。FR の技のアニメは読まないので、DoMoveAnim の前に FR の読む場所へ書き写す（案 b） */
struct HackContestMoveAnim
{
	u16 species;
	u16 targetSpecies;
	u32 personality;
	u32 targetPersonality;
	u32 otId;
	u8 contestant;
	u8 hasTargetAnim;
	u8 pad[2];
};

/* 技のコンテスト用の設定の読み口（S2b。まとめ 0.66.0 で本物の gContestMoves／gContestEffects＝src/Tables/contest_moves.c につないだ）。 */
struct HackContestMoveInfo
{
	u8 effect;    /* 0〜47 */
	u8 category;  /* 0 かっこよさ 1 うつくしさ 2 かわいさ 3 かしこさ 4 たくましさ */
	u8 appeal;    /* 0xFF＝無し */
	u8 jam;       /* 0xFF＝無し */
};
#define HACK_CONTEST_EFFECT_COUNT 48
#define HACK_CONTEST_EFFECT_REPETITION_NOT_BORING 3
extern const u8* const gHackContestEffectDescriptions[HACK_CONTEST_EFFECT_COUNT];
void HackContest_GetMove(u16 move, struct HackContestMoveInfo* out);
bool8 HackContest_AreMovesCombo(u16 prev, u16 move, const u16* ownMoves); /* 原文 AreMovesContestCombo（ownMoves は使わない） */
u8 HackContest_GetEffectAppeal(u8 effect);
u8 HackContest_GetEffectType(u8 effect);                          /* S3: 原文 gContestEffects[].effectType（AI が読む） */
bool8 HackContest_IsComboStarter(u16 move, const u16* ownMoves);  /* S3: 原文 gContestMoves[].comboStarterId != 0（ownMoves は使わない） */
u8 HackContest_GetEffectJam(u8 effect);

/* S3: 原文 ContestAIInfo（NPC が技を選ぶ台本の状態）。stack は台本の中の位置（blob 先頭からの相対ではなく、実際のポインタ） */
struct HackContestAI
{
	u8 aiState;               /* 0 SETTING_UP 1 PROCESSING 2 FINISHED 3 DO_NOT_PROCESS */
	u8 nextMoveIndex;
	u8 moveScores[4];
	u8 aiAction;
	u8 currentAIFlag;
	u8 stackSize;
	u8 contestantId;
	u16 nextMove;
	s16 scriptResult;
	s16 vars[3];
	u32 aiFlags;
	const u8* scriptPtr;      /* gAIScriptPtr */
	const u8* stack[8];
};

struct HackContestState;
/* S3: 効果 48・AI の入口（hack_contest_effect.c・hack_contest_ai.c） */
void HackContest_RunEffect(struct HackContestState* s, u8 contestant, u8 effect);
bool8 HackContest_IsMonsTurnDisabled(struct HackContestState* s, u8 contestant);
void HackContest_AIReset(struct HackContestState* s, u8 contestant);
u8 HackContest_AIGetActionToUse(struct HackContestState* s);
s8 HackContest_GetMoveExcitement(struct HackContestState* s, u16 move);

#define HACK_CONTEST_AI_COMMON 0x00000FFF /* 原文 CONTEST_AI_COMMON＝bit0〜11（台本 7 本＋ダミー 5 本） */

/* S2a で本物にした中身（原文 contest.c の gContestResources の contestBgTilemaps／boxBlinkTiles／animBgTileBuffer・eContestTempSave と、
 * 画面の進行に要る小さい変数）。static は 1 つも使わず（6.13）、全部この中。ヒープに 1 つの塊（約 37KB）で取る。
 * 技を選ぶ画面・審査・メーターの変数は S2b 以降でここに足す。 */
struct HackContestState
{
	u32 magic;
	u8 phase;                 /* 0＝開いたばかり 1＝画面を作っている 2＝幕が上がる 3＝待ち（debug はここで B を待つ） */
	u8 setupState;            /* SetupContestGraphics の段（eContest.contestSetupState） */
	u8 mainTaskId;
	u8 judgeSpeechBubbleSpriteId;
	u8 bgCopyFlags;           /* sContestBgCopyFlags */
	u8 linkFlags;             /* gLinkContestFlags（通信は未対応＝0） */
	u8 playerIndex;           /* gContestPlayerMonIndex */
	u8 judgeSpriteId;
	u8 monSpriteId;           /* debug で出した出場ポケモンの絵（0xFF＝無い） */
	u8 turnOrder[HACK_CONTEST_CONTESTANTS]; /* gContestantTurnOrder */
	u8 pad[3];
	struct HackContestMon mons[HACK_CONTEST_CONTESTANTS];
	u16 bgTilemap[4][HACK_CONTEST_BG_TILEMAP_SIZE / 2]; /* contestBgTilemaps[0..3]（BG0〜3 の tilemap buffer） */
	u8 boxBlink1[0x800];      /* boxBlinkTiles1（S2b 以降） */
	u8 boxBlink2[0x800];      /* boxBlinkTiles2 */
	u8 animBg[0x2000];        /* animBgTileBuffer（gBattleAnimBgTileBuffer の指す先） */
	u8 audienceGfx[0x2000];   /* eUnzippedContestAudience_Gfx（展開した観客の絵の控え） */
	u16 cachedWindowPalettes[16][16]; /* eContestTempSave.cachedWindowPalettes */
	/* ここから S2b */
	u16 cachedPlttUnfaded[0x200];     /* eContestTempSave.cachedPlttBufferUnfaded（技を選ぶ画面を閉じるとき戻す） */
	u16 cachedPlttFaded[0x200];       /* eContestTempSave.cachedPlttBufferFaded */
	s16 round1Points[HACK_CONTEST_CONTESTANTS]; /* gContestMonRound1Points */
	struct HackContestStatus status[HACK_CONTEST_CONTESTANTS]; /* eContestantStatus（一部） */
	u8 nextTurnSpriteId[HACK_CONTEST_CONTESTANTS];
	u8 category;              /* コンテストの部門（gSpecialVar_ContestCategory）。debug は 0 */
	u8 appealNumber;          /* eContest.appealNumber（何回目のアピールか・0〜4） */
	s8 playerMoveChoice;      /* eContest.playerMoveChoice */
	u8 pad2;
	/* ここから S2c（eContest・eContestGfxState・eContestExcitement） */
	u8 currentContestant;
	u8 turnNumber;
	s8 applauseLevel;
	u8 applauseMeterSpriteId;
	u8 applauseMeterIsMoving;
	u8 sliderHeartsAnimating;
	u8 isShowingApplauseMeter;
	u8 animatingAudience;
	u8 waitForAudienceBlend;
	u8 waitForJudgeSpeechBubble;
	u8 judgeAttentionTaskId;
	u8 moveAnimTurnCount;
	u8 prevTurnOrder[HACK_CONTEST_CONTESTANTS];
	u8 sliderHeartSpriteId[HACK_CONTEST_CONTESTANTS];
	u8 updatingAppealHearts[HACK_CONTEST_CONTESTANTS];
	u8 boxBlinking[HACK_CONTEST_CONTESTANTS];
	u8 sliderUpdating[HACK_CONTEST_CONTESTANTS];
	s8 moveExcitement;        /* eContestExcitement */
	u8 excitementAppealBonus;
	u8 excitementFrozen;
	u8 excitementFreezer;
	struct HackContestMoveAnim moveAnim;
	u8 battleSpritesAllocated;
	u8 pad3[3];
	/* ここから S3（原文 eContestAppealResults の jam 系・eContest.moveHistory／excitementHistory・eContestAI） */
	s16 jam;
	s16 jam2;
	u8 jamQueue[5];
	u8 pad5[3];
	u16 moveHistory[5][HACK_CONTEST_CONTESTANTS];
	s8 excitementHistory[5][HACK_CONTEST_CONTESTANTS];
	struct HackContestAI ai;
};

/* S4: コンテストの結果の記録（HackContestKeep）。固定 RAM 0x0203E800〜0x0203E8FF（256 バイト・Commander の割り当て 2026-11-09）。
 * ヒープの外＝地図の読み込みをまたいで残る（原文はフィールドへ戻ったあとの台本が ShowContestResults・GetContestWinnerId 等で
 * gContestMons／gContestFinalStandings／gContestMonRound?Points を読む。それをここに置く）。BGM のトラック 0x0203E300〜0x0203E800 の次・
 * ジョウトの変数の写し 0x0203EA00〜 の手前。書くのは hack_contest_result.c の HackContest_FinalizeScores（5 ラウンドの終わり）と S5 の受付だけ。 */
#define HACK_CONTEST_KEEP_ADDR 0x0203E800
#define HACK_CONTEST_KEEP_MAGIC 0x31504B43 /* 'CKP1' */
struct HackContestResults; /* hack_contest_resultscreen.c */
struct HackContestKeepMon
{
	u16 species;
	u16 pad;
	u32 otId;
	u32 personality;
	u8 nickname[POKEMON_NAME_LENGTH + 1];
	u8 trainerName[PLAYER_NAME_LENGTH + 1];
	u8 pad2[1];
};
struct HackContestKeep
{
	u32 magic;               /* HACK_CONTEST_KEEP_MAGIC なら中身が有効 */
	u8 playerIndex;          /* gContestPlayerMonIndex */
	u8 category;             /* 0 かっこよさ〜4 たくましさ（gSpecialVar_ContestCategory） */
	u8 rank;                 /* 0 ノーマル 1 スーパー 2 ハイパー 3 マスター（gSpecialVar_ContestRank・S5 の受付が書く） */
	u8 isLink;               /* 通信は未対応＝常に 0 */
	u8 finalStandings[HACK_CONTEST_CONTESTANTS]; /* gContestFinalStandings（0＝優勝） */
	s16 round1Points[HACK_CONTEST_CONTESTANTS];  /* gContestMonRound1Points */
	s16 round2Points[HACK_CONTEST_CONTESTANTS];  /* gContestMonRound2Points（アピールの合計 × 2） */
	s16 totalPoints[HACK_CONTEST_CONTESTANTS];   /* gContestMonTotalPoints */
	struct HackContestKeepMon mons[HACK_CONTEST_CONTESTANTS];
	u8 resultsReturn;        /* S4b: 1＝結果発表のあと台本の続きへ戻る（ShowContestResults が立てる）・0＝野外へ（debug の入口） */
	u8 fromScript;           /* S5a: 1＝台本の startcontest（StartHackContest）で入った＝画面を出るとき台本の続きへ戻る */
	u8 pad3[2];
	u8 partyIndex;           /* S5a: 出すポケモンの手持ちの番号（原文 gContestMonPartyIndex） */
	u8 opp[3];               /* S5a: NPC の出場者 3 人の gHackContestOpponents の番号（受付で決める・会場の画面の始めに表から作り直す） */
	struct HackContestResults* screen; /* S4b: 結果発表の画面の作業領域（ヒープ・画面を出るとき Free して NULL・HackContest_Reset でも NULL） */
};
_Static_assert(sizeof(struct HackContestKeep) <= 256, "HackContestKeep は 256 バイト以下（0x0203E800〜0x0203E8FF）");
#define HACK_CONTEST_KEEP (*(struct HackContestKeep*) HACK_CONTEST_KEEP_ADDR)

/* S5a: 出場者の表（日本語版 ROM の gContestOpponents・tools/contest/extract_jp_opponents.py が生成）と、受付の special */
struct HackContestOpponent
{
	u16 species;
	u8 nickname[11];
	u8 trainerName[8];
	u16 gfx;                 /* この ROM の NPC の絵の番号（エメラルドの絵を台本の生成器と同じ規則で変換済み） */
	u32 aiFlags;
	u8 rankPool;             /* bit0〜1 whichRank・bit2 Cool・3 Beauty・4 Cute・5 Smart・6 Tough の AI の枠 */
	u16 moves[4];
	u8 stats[6];             /* cool beauty cute smart tough sheen */
	u8 highestRank;
	u8 gameCleared;
	u32 personality;
	u32 otId;
};
#define HACK_CONTEST_OPPONENT_COUNT 96
extern const struct HackContestOpponent gHackContestOpponents[HACK_CONTEST_OPPONENT_COUNT];
extern const u8 gHackContestPostgameFilter[HACK_CONTEST_OPPONENT_COUNT];
#define HACK_CONTEST_FILTER_NONE 0
#define HACK_CONTEST_FILTER_NO_POSTGAME 1
#define HACK_CONTEST_FILTER_ONLY_POSTGAME 2
/* 原文 constants/contest.h: TryEnterContestMon の VAR_RESULT */
#define HACK_CANT_ENTER_CONTEST 0
#define HACK_CAN_ENTER_CONTEST_EQUAL_RANK 1
#define HACK_CAN_ENTER_CONTEST_HIGH_RANK 2
#define HACK_CANT_ENTER_CONTEST_EGG 3
#define HACK_CANT_ENTER_CONTEST_FAINTED 4
u8 HackContest_GetEligibility(u8 partyIndex, u8 category, u8 rank); /* S5b: 資格（TryEnterContestMon と手持ち画面の「できる／できない」が共用） */
void HackContest_BuildMonFromOpponent(struct HackContestMon* out, u8 index);
void HackContest_BuildMonFromParty(struct HackContestMon* out, u8 partyIndex);
void HackContest_GetPartyCondition(u8 partyIndex, u8* out6); /* S5a: コンディション 6 値の読み口（今は素の MON_DATA_COOL〜SHEEN・まとめで HackCondition_Get に差し替える） */
void GetContestWinnerId(void);
void GetContestPlayerId(void);
void BufferContestWinnerTrainerName(void);
void BufferContestWinnerMonName(void);
void BufferContestantTrainerName(void);
void BufferContestantMonNickname(void);
void BufferContestantMonSpecies(void);
void GetContestMonCondition(void);
u16 HasMonWonThisContestBefore(void);
void GiveMonContestRibbon(void);
void TryEnterContestMon(void);
void SetContestTrainerGfxIds(void);
void ShowContestEntryMonPic(void);
void HideContestEntryMonPic(void);
void BufferContestTrainerAndMonNames(void);
void GenerateContestRand(void);
void ShouldReadyContestArtist(void);
void StartHackContest(void);

/* S4: 5 ラウンドの終わりの最終点・順位（原文 contest.c:2666 CalculateFinalScores／3551〜3633）と Keep への書き込み */
void HackContest_FinalizeScores(struct HackContestState* s);
/* S4: リボンを渡す口（1 か所・今は仮。FEAT-ribbons が差し替える）と、部門のリボンのランクを読む口（仮＝0） */
void HackContest_AwardRibbon(u8 partyIndex, u8 category, u8 rank);
u8 HackContest_GetCategoryRibbonRank(u8 partyIndex, u8 category);
/* S6: 勝者の記録（追加の区画 93〜94・hack_contest_winner.c）。原文 contest.c:5522〜5636・contest_painting.c・contest_util.c:2362〜2392 */
struct HackContestWinner /* 原文 struct ContestWinner（日本語版も同じ並び・32 バイト）。species は +8・category +10・monName +11・trainerName +22・rank +30 */
{
	u32 personality;
	u32 trainerId;
	u16 species;
	u8 category;                       /* ホールの絵は部門 0〜4・美術館の絵と絵描きの絵は 字幕番号（部門×3＋0〜2） */
	u8 monName[POKEMON_NAME_LENGTH + 1];
	u8 trainerName[PLAYER_NAME_LENGTH + 1];
	u8 rank;
};
#define HACK_WINNER_COUNT 13        /* 原文 NUM_CONTEST_WINNERS: ホール 8（絵は 6 枚）＋美術館 5 */
#define HACK_WINNER_HALL_COUNT 8    /* MUSEUM_CONTEST_WINNERS_START */
#define HACK_WINNER_ARTIST 0        /* 台本の showcontestpainting の 0＝絵描きの一時の絵（保存しない）。1〜13＝記録の N 番 */
#define HACK_PAINTING_CAPTIONS 3    /* 原文 NUM_PAINTING_CAPTIONS */
#define HACK_SAVE_FOR_MUSEUM 0xFF   /* 原文 CONTEST_SAVE_FOR_MUSEUM */
#define HACK_SAVE_FOR_ARTIST 0xFE   /* 原文 CONTEST_SAVE_FOR_ARTIST */
/* 絵描き用の一時の優勝者（保存しない・原文 gCurContestWinner）。固定 RAM 0x0203E900〜0x0203E91F（Keep の次・ジョウトの変数の写し 0x0203EA00 の手前） */
#define HACK_ARTIST_WINNER_ADDR 0x0203E900
#define HACK_ARTIST_WINNER (*(struct HackContestWinner*) HACK_ARTIST_WINNER_ADDR)
_Static_assert(sizeof(struct HackContestWinner) == 32, "HackContestWinner は 32 バイト");
bool8 SaveContestWinner(u8 rank);                         /* 原文 contest.c:5522。HACK_SAVE_FOR_ARTIST／HACK_SAVE_FOR_MUSEUM／ランク 0〜3 */
u8 HackContest_WinnerSaveIdx(u8 rank);                    /* 原文 GetContestWinnerSaveIdx（shift なし）。絵描き・美術館は部門の枠 8〜12 */
bool8 HackContest_WinnersUsable(void);                    /* 追加のセーブ領域が使えて記録できるか（使えない環境は記録しない・案 A） */
void HackContest_LoadWinners(struct HackContestWinner* out13);   /* 今の記録 13 件（記録が無い・使えない・壊れ＝ホールは初期の 8 件・美術館は空） */
u8 CountPlayerMuseumPaintings(void);                      /* 原文 contest_util.c:2381 */
void SaveMuseumContestPainting(void);                     /* 原文 contest_util.c:2362 */
bool8 GiveMonArtistRibbon(void);                          /* 原文 contest_util.c:2549（リボンを付ける口は仮＝HackContest_AwardArtistRibbon） */
bool8 HackContest_AwardArtistRibbon(u8 partyIndex);       /* 仮の口 1 か所（FEAT-relive-ribbon のまとめが差し替える）。付けたら TRUE（今は付けない＝FALSE・debug 版だけ TRUE） */
u8 HackContest_HasArtistRibbon(u8 partyIndex);            /* 仮の口（常に 0・debug 版は印） */
void HackContest_SaveWinner(void);                        /* 結果発表の始めに呼ぶ（原文 contest_util.c:662〜666）。優勝者を記録＋絵描き用の一時の優勝者を作る */
/* S8: ミナモのポケモンセンターのおねえさんの状態（0x40 バイト）。優勝者のブロックの後ろ（version 2・hack_contest_winner.c）に置く。実装は hack_lilycove_lady.c */
struct HackLadySave
{
	u8 magic;               /* 0x4C＝中身あり（0 のブロック・version 1 は「無し」＝読んだ側が初期化する） */
	u8 id;                  /* 0 クイズ・1 おねがい・2 コンテスト（TID % 6 >> 1） */
	u8 favorState;          /* LILYCOVE_LADY_STATE_READY／COMPLETED／PRIZE */
	u8 favorLiked;
	u8 favorNumGiven;
	u8 favorId;             /* 0〜5（すべすべ…ちくちく） */
	u16 favorItem;
	u16 favorBest;
	u8 contestGiven;        /* 原文 givenPokeblock */
	u8 contestGood;         /* 原文 numGoodPokeblocksGiven */
	u8 contestOther;        /* 原文 numOtherPokeblocksGiven */
	u8 contestCategory;     /* 0〜4（かっこよさ…たくましさ） */
	u8 quizState;           /* クイズ（S10）: LADY_STATE_READY／COMPLETED／PRIZE（バッグがいっぱいで賞品を渡せなかった） */
	u8 quizQ;               /* 今の問題（0〜15）・S10 で追加（今までの保存は 0＝問題 0 から始まる） */
	u8 quizPrev;            /* 前の問題（次の問題を前と変えるため・16＝まだ無い）。今までの保存は 0 */
	u8 quizSel;             /* 選んだ選択肢（0〜2・0xFF＝まだ）。保存はしない（台本の 1 回の間だけ使う） */
	u8 spare[0x40 - 18];     /* 先頭 18 バイト（0〜17）の残り */
};
_Static_assert(sizeof(struct HackLadySave) == 0x40, "HackLadySave は 0x40 バイト");
/* S10: クイズの 16 問（日本語版エメラルドの ROM から・tools/contest/extract_jp_quiz.py が src/hack_lilycove_quiz_data.c を作る） */
#define HACK_QUIZ_COUNT 16
struct HackQuiz
{
	const u8* question;      /* 問題文（会話の窓の本文・改行つき） */
	const u8* choice[3];     /* 3 つの選択肢 */
	u8 answer;               /* 答えの位置（0〜2） */
	u16 prize;               /* 賞品（CFRU の道具の番号） */
};
extern const struct HackQuiz gHackQuiz[HACK_QUIZ_COUNT];
extern const u8 gHackQuizMenuWidth;                       /* 選択肢の窓の幅（タイル） */
extern const u8 gHackQuizLadyName[];                      /* おねえさんの呼び名（日本語版 0x085D3D90） */
bool8 HackContest_LoadLady(struct HackLadySave* out);     /* 区画の version 2 から。無い・使えない・壊れ＝FALSE（out は触らない） */
bool8 HackContest_CommitLady(const struct HackLadySave* lady);   /* 今の優勝者の記録と一緒に書く。使えない環境は FALSE（記録しない） */
void HackLady_Invalidate(void);                           /* つづきから: RAM の写しを捨てる（Winner_OnLoad が呼ぶ） */
void Winner_OnLoad(void);                                 /* つづきから（src/save.c）。読んだ sel を正に */
void Winner_AfterSave(bool8 ok);                          /* レポートのあと（src/save.c） */
/* 絵の画面（hack_contest_painting.c）。台本の showcontestpainting の読み替え（HoennNative_ShowContestPainting・Var8004＝0 絵描き／1〜13 記録） */
void ShowContestPainting(void);
/* S4b: 結果発表の画面（hack_contest_resultscreen.c・原文 contest_util.c の結果の部分）。ShowContestResults は special（台本から・呼び出しは S5）。
 * HackContest_ResultsStart は画面の入口の CB2（Keep が有効なときだけ画面を出す）。debug 版は HackContest_DebugResults（結果だけの入口）も持つ */
void ShowContestResults(void);
void HackContest_ResultsStart(void);
#ifdef HACK_DEBUG_MENU
void HackContest_DebugResults(void);
#endif
void HackContest_DetermineStandings(const s16* round1, const s16* total, u8* standingsOut);

bool8 HackContest_IsContest(void);
u8 HackContest_GetBattleBgPaletteNum(void); /* S3: 素の GetBattleBgPaletteNum の呼び元 BL 2 か所の付け替え先（hooks の踏み台 0x08059830） */
bool8 HackContest_Open(void);   /* Malloc＋0 埋め＋語に代入。すでに開いていれば何もせず 0。確保できたら 1 */
void HackContest_Close(void);   /* Free＋語を NULL に。開いていなければ何もしない */
void HackContest_Reset(void);   /* 語を NULL に戻すだけ（Free しない・ヒープが消えたあとでも安全） */

struct BattleAnimBgData;
void HackContest_GetBattleAnimBg1Data(struct BattleAnimBgData* d);                  /* S2d: 素の GetBattleAnimBg1Data（0x08074A44）の置き換え（hooks） */
void HackContest_GetBattleAnimBgData(struct BattleAnimBgData* d, u32 bgId);         /* S2d: 素の GetBattleAnimBgData（0x08074A6C） */
void HackContest_GetBattleAnimBgDataByPriorityRank(struct BattleAnimBgData* d, u8 unused); /* S2d: 素の GetBattleAnimBgDataByPriorityRank（0x08074AA4） */
u8 HackContest_GetBattlerSpriteBGPriority(u8 battlerId); /* S2d: 素の GetBattlerSpriteBGPriority（0x08076028）の置き換え（hooks） */
void HackContest_LoadMoveBg(u16 bgId);              /* S2d: 素の LoadMoveBg（0x080730C8）の置き換え（hooks） */
void HackContest_LoadDefaultBg(void);                /* S2d: 素の LoadDefaultBg（0x08073110）の置き換え（hooks） */
void HackContest_LoadContestBgAfterMoveAnim(void);   /* S2d: 技のアニメが書き換えた会場の BG・パレットを戻す（hack_contest_main.c） */

void HackContest_CB2_Start(void); /* 原文 CB2_StartContest（会場の画面を作る・S2a はここまで）。Open 済み（または debug 入口）で呼ぶ */
#ifdef HACK_DEBUG_MENU
void HackContest_DebugEnter(void); /* debug 版だけ: 出場者 4 人を仮に作って Open → HackContest_CB2_Start。gMain.callback2 に書いて入る（tools/test/contest_screen_mgba.py） */
#endif

#endif
