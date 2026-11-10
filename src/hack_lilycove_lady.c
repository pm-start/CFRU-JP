#include "defines.h"
#include "../include/event_data.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/palette.h"
#include "../include/gba/io_reg.h"
#include "../include/constants/items.h"
#include "../include/constants/species.h"
#include "hack_contest.h"
#include "hack_pokeblock.h"
#include "hack_pokeblock_case.h"

/*
 * コンテスト移植 S8（FEAT-contest・patches/0985）: ミナモのポケモンセンター 1 階のおねえさん（原文 参考/pokeemerald/src/lilycove_lady.c・
 * data/scripts/lilycove_lady.inc・src/data/lilycove_lady.h）。3 種（TID % 6 >> 1 で決まる）のうち おねがい（Favor）と コンテスト（Contest）を原文どおり。
 * クイズ（S10・ユーザー決定 2026-11-10）は日本語版の 16 問を「3 つの候補から選ぶ」形で（自分でクイズを作る枝・他のプレイヤーの問題は外した）。通信・レコード混ぜ・TV は範囲外。
 *
 * ★ 状態（0x40 バイト）は優勝者の記録のブロックの後ろ（version 2・hack_contest_winner.c）。変えるたびに今の優勝者の記録ごと書き直す。
 *   追加のセーブ領域が使えない環境は RAM の写し（固定番地 LADY_RAM）だけ＝保存・つづきからで初期化される（仮・ユーザー確認待ち）。
 * ★ 初期化の乱数は TID から作る疑似乱数（原文は Random()）＝まだ書いていない間に読み直しても同じ値になる（書いたあと・おねがいの完了の再初期化は Random()）。
 * ★ レコード混ぜが無いので、おねがい・コンテストは原文どおり 1 回きり（ResetLilycoveLadyForRecordMix に当たる物は呼ばない）。
 * ★ 台本の special は callnative HoennNative_X（hoenn_contest_scripts.py の SPECIAL_MISSING）。specialvar の値は原文と同じ変数（VAR_RESULT＝Var800D・VAR_0x8004＝Var8004）へ書く。
 */

#define LADY_QUIZ 0
#define LADY_FAVOR 1
#define LADY_CONTEST 2
#define LADY_STATE_READY 0
#define LADY_STATE_COMPLETED 1
#define LADY_STATE_PRIZE 2
#define LADY_GIFT_THRESHOLD 5
#define LADY_MAGIC 0x4C

extern const u8* const gHackLadyFavor[];
extern const u8* const gHackLadyMonName[];
extern const u8* const gHackLadyCategory[];
extern void sp0B1_ChooseItemFromBag(void);
extern void sp025_AddTextByPointer(void);     //CFRU の自由な選択肢に文字を足す（src/scripting.c・Var8006＝位置・gLoadPointer＝文字）
#define QUIZ_NONE 0xFF
#define QUIZ_AUTHOR_LADY 2

//原文 sFavorLadyAcceptedItems_*（src/data/lilycove_lady.h:301〜421）。ITEM_TM_HYPER_BEAM は CFRU に無い（クイズの賞品だけ＝今は使わない）
static const u16 sAccepted0[] = {ITEM_REPEL, ITEM_SUPER_REPEL, ITEM_MAX_REPEL, ITEM_ANTIDOTE, ITEM_PARALYZE_HEAL, ITEM_BURN_HEAL, ITEM_BELUE_BERRY, ITEM_AWAKENING, ITEM_ICE_HEAL, ITEM_REVIVE, ITEM_MAX_REVIVE, ITEM_ENERGY_POWDER, 0};
static const u16 sAccepted1[] = {ITEM_FLUFFY_TAIL, ITEM_PEARL, ITEM_BIG_PEARL, ITEM_HARD_STONE, ITEM_SMOKE_BALL, ITEM_SHOAL_SHELL, ITEM_TINY_MUSHROOM, ITEM_BIG_MUSHROOM, ITEM_PECHA_BERRY, ITEM_ASPEAR_BERRY, ITEM_ORAN_BERRY, ITEM_GREPA_BERRY, ITEM_MAGOST_BERRY, ITEM_WATMEL_BERRY, ITEM_POKE_BALL, ITEM_ULTRA_BALL, 0};
static const u16 sAccepted2[] = {ITEM_REVIVAL_HERB, ITEM_POTION, ITEM_FRESH_WATER, ITEM_SODA_POP, ITEM_LEMONADE, ITEM_HARD_STONE, ITEM_LIGHT_BALL, ITEM_LAVA_COOKIE, ITEM_CHESTO_BERRY, ITEM_NANAB_BERRY, ITEM_WEPEAR_BERRY, ITEM_KELPSY_BERRY, ITEM_NOMEL_BERRY, ITEM_DURIN_BERRY, 0};
static const u16 sAccepted3[] = {ITEM_HEAL_POWDER, ITEM_X_SPEED, ITEM_X_ATTACK, ITEM_X_DEFEND, ITEM_BLUE_FLUTE, ITEM_YELLOW_FLUTE, ITEM_RED_FLUTE, ITEM_BLACK_FLUTE, ITEM_WHITE_FLUTE, ITEM_NUGGET, ITEM_SUN_STONE, ITEM_STARDUST, ITEM_STAR_PIECE, ITEM_PEARL, ITEM_BIG_PEARL, ITEM_TWISTED_SPOON, ITEM_SILVER_POWDER, ITEM_BRIGHT_POWDER, ITEM_LUXURY_BALL, ITEM_PREMIER_BALL, 0};
static const u16 sAccepted4[] = {ITEM_ENERGY_ROOT, ITEM_FULL_RESTORE, ITEM_MAX_POTION, ITEM_DIRE_HIT, ITEM_X_ACCURACY, ITEM_GUARD_SPEC, ITEM_WATMEL_BERRY, ITEM_LEFTOVERS, ITEM_TINY_MUSHROOM, ITEM_HEART_SCALE, 0};
static const u16 sAccepted5[] = {ITEM_QUICK_CLAW, ITEM_POISON_BARB, ITEM_SHARP_BEAK, ITEM_DRAGON_FANG, ITEM_TAMATO_BERRY, ITEM_DURIN_BERRY, ITEM_PETAYA_BERRY, ITEM_SALAC_BERRY, ITEM_STARDUST, ITEM_STAR_PIECE, 0};
static const u16* const sAcceptedLists[] = {sAccepted0, sAccepted1, sAccepted2, sAccepted3, sAccepted4, sAccepted5};
const u16 gHackLadyAcceptedCounts[] = {12, 16, 14, 20, 10, 10};                       //検算が原文の件数と突き合わせる
static const u16 sFavorPrizes[] = {ITEM_LUXURY_BALL, ITEM_NUGGET, ITEM_PROTEIN, ITEM_HEART_SCALE, ITEM_RARE_CANDY, ITEM_PP_MAX};   //原文 sFavorLadyPrizes
static const u16 sContestMonSpecies[] = {SPECIES_ZIGZAGOON, SPECIES_SKITTY, SPECIES_POOCHYENA, SPECIES_KECLEON, SPECIES_PIKACHU};   //原文 sContestLadyMonSpecies

//★ このビルドには .bss が無い（static の変数は ROM に置かれ、書けない）＝RAM の写しは固定番地（EWRAM の空き 0x0203E930〜0x0203E977・台帳 docs の S8 の行・Johto の写し 0x0203EA00 の手前）
struct LadyRam
{
	struct HackLadySave save;    //今の状態の写し
	u8 tid[4];                   //この写しを作ったときの TID（変わったら読み直す）
	u8 ok;                       //1＝写しは有効
	u8 pad[3];
};
#define HACK_LADY_RAM_ADDR 0x0203E930
#define LADY_RAM ((struct LadyRam*) HACK_LADY_RAM_ADDR)
_Static_assert(sizeof(struct LadyRam) == 0x48, "LadyRam is 0x48 bytes");

static u16 SRand(u32* seed)
{
	*seed = *seed * 1103515245u + 12345u;
	return (*seed >> 16) & 0x7FFF;
}

static u8 NumAccepted(u8 favorId)
{
	const u16* l = sAcceptedLists[favorId];
	u8 n = 0;

	while (l[n] != 0)
		++n;
	return n;
}

//原文 FavorLadyPickFavorAndBestItem（rand は seeded か Random()）
static void PickFavor(struct HackLadySave* l, u32* seed)       //seed が NULL なら Random()
{
	u8 n;

	l->favorId = (seed ? SRand(seed) : Random()) % 6;
	n = NumAccepted(l->favorId);
	l->favorBest = sAcceptedLists[l->favorId][(seed ? SRand(seed) : Random()) % n];
}

//原文 InitLilycoveFavorLady
static void InitFavor(struct HackLadySave* l, u32* seed)
{
	l->favorState = LADY_STATE_READY;
	l->favorLiked = 0;
	l->favorNumGiven = 0;
	l->favorItem = 0;
	PickFavor(l, seed);
}

//原文 InitLilycoveLady（new_game.c:199）。この改造は「状態が無いとき」に初めて読んだ所で作る
static void InitAll(struct HackLadySave* l)
{
	u16 id = (gSaveBlock2->playerTrainerId[1] << 8) | gSaveBlock2->playerTrainerId[0];
	u32 seed = (gSaveBlock2->playerTrainerId[3] << 24) | (gSaveBlock2->playerTrainerId[2] << 16) | (gSaveBlock2->playerTrainerId[1] << 8) | gSaveBlock2->playerTrainerId[0];

	seed ^= 0x4C41445Au;
	{
		u32 i;
		u8* p = (u8*) l;

		for (i = 0; i < sizeof(*l); ++i)
			p[i] = 0;
	}
	l->magic = LADY_MAGIC;
	id %= 6;
	id >>= 1;
	l->id = id;
	InitFavor(l, &seed);
	l->contestGiven = 0;
	l->contestGood = 0;
	l->contestOther = 0;
	l->contestCategory = SRand(&seed) % 5;                    //原文 Random() % CONTEST_CATEGORIES_COUNT
	l->quizState = LADY_STATE_READY;                          //S10: 原文 InitLilycoveQuizLady（QuizLadyPickQuestion）。乱数は上の続き（上の値は変えない）
	l->quizQ = SRand(&seed) % HACK_QUIZ_COUNT;
	l->quizPrev = HACK_QUIZ_COUNT;
	l->quizSel = QUIZ_NONE;
}

static void Invalidate(void)
{
	LADY_RAM->ok = 0;
}

void HackLady_Invalidate(void)
{
	Invalidate();
}

static struct HackLadySave* Lady(void)
{
	struct LadyRam* r = LADY_RAM;
	u32 i;

	if (r->ok == 1)
	{
		for (i = 0; i < 4; ++i)
			if (r->tid[i] != gSaveBlock2->playerTrainerId[i])
				break;
		if (i == 4)
			return &r->save;
	}
	if (!HackContest_LoadLady(&r->save) || r->save.id > 2 || r->save.favorId > 5 || r->save.contestCategory > 4)
		InitAll(&r->save);
	for (i = 0; i < 4; ++i)
		r->tid[i] = gSaveBlock2->playerTrainerId[i];
	r->ok = 1;
	return &r->save;
}

//変えたあとに呼ぶ。使えない環境は RAM の写しだけ（仮）
static void Save(void)
{
	HackContest_CommitLady(&LADY_RAM->save);
}

//---- 窓口（原文の special）------------------------------------------------

//special Script_GetLilycoveLadyId
void HoennNative_GetLilycoveLadyId(void)
{
	Var800D = Lady()->id;
}

//specialvar VAR_RESULT, GetFavorLadyState
void HoennNative_GetFavorLadyState(void)
{
	struct HackLadySave* l = Lady();

	Var800D = (l->favorState == LADY_STATE_PRIZE) ? LADY_STATE_PRIZE : (l->favorState == LADY_STATE_COMPLETED) ? LADY_STATE_COMPLETED : LADY_STATE_READY;
}

//special BufferFavorLadyRequest（gStringVar1＝すべすべ…）
void HoennNative_BufferFavorLadyRequest(void)
{
	StringCopy(gStringVar1, gHackLadyFavor[Lady()->favorId]);
}

//specialvar VAR_RESULT, HasAnotherPlayerGivenFavorLadyItem: 原文は他のプレイヤー（レコード混ぜ）が渡した名前があるか＝混ぜが無いので常に FALSE
void HoennNative_HasAnotherPlayerGivenFavorLadyItem(void)
{
	Var800D = FALSE;
}

//special BufferFavorLadyItemName（gStringVar2）
void HoennNative_BufferFavorLadyItemName(void)
{
	CopyItemName(Lady()->favorItem, gStringVar2);
}

//special BufferFavorLadyPlayerName（gStringVar3）: 原文は覚えた渡し主の名前。混ぜが無いので自分の名前
void HoennNative_BufferFavorLadyPlayerName(void)
{
	StringCopy(gStringVar3, gSaveBlock2->playerName);
}

//specialvar VAR_RESULT, DidFavorLadyLikeItem
void HoennNative_DidFavorLadyLikeItem(void)
{
	Var800D = Lady()->favorLiked ? TRUE : FALSE;
}

//special Script_FavorLadyOpenBagMenu: バッグ（またはきのみぶくろ）を開いて道具を選ばせる（CFRU の sp0B1）。台本が waitstate で待つ。選んだ道具は Var800E（0＝選ばなかった）
void HoennNative_FavorLadyOpenBagMenu(void)
{
	Var8000 = (Var8009 == 1) ? 4 : 0;   //仮・ユーザー確認待ち: 台本が「きのみですか？」ではいと答えたら 4＝きのみぶくろの選択画面（sp0B1 の 4）・いいえは 0＝バッグ
	Var800E = 0;
	Var800D = 0;
	sp0B1_ChooseItemFromBag();
}

//バッグから戻った直後（原文 ItemMenu_GiveFavorLady）: 選んだら 1 個減らして VAR_RESULT＝TRUE
void HoennNative_FavorLadyAfterBag(void)
{
	if (Var800E != 0)
	{
		RemoveBagItem(Var800E, 1);
		Var800D = TRUE;
	}
	else
		Var800D = FALSE;
}

//specialvar VAR_RESULT, Script_DoesFavorLadyLikeItem（原文 DoesFavorLadyLikeItem・渡した道具は Var800E＝gSpecialVar_ItemId）
void HoennNative_DoesFavorLadyLikeItem(void)
{
	struct HackLadySave* l = Lady();
	u16 itemId = Var800E;
	u8 n = NumAccepted(l->favorId), i;
	bool8 liked = FALSE;

	l->favorState = LADY_STATE_COMPLETED;
	CopyItemName(itemId, gStringVar2);
	l->favorItem = itemId;
	l->favorLiked = 0;
	for (i = 0; i < n; ++i)
	{
		if (sAcceptedLists[l->favorId][i] == itemId)
		{
			liked = TRUE;
			l->favorNumGiven++;
			l->favorLiked = 1;
			if (l->favorBest == itemId)
				l->favorNumGiven = LADY_GIFT_THRESHOLD;
			break;
		}
	}
	Save();
	Var800D = liked;
}

//specialvar VAR_RESULT, IsFavorLadyThresholdMet
void HoennNative_IsFavorLadyThresholdMet(void)
{
	Var800D = Lady()->favorNumGiven < LADY_GIFT_THRESHOLD ? FALSE : TRUE;
}

//specialvar VAR_0x8004, FavorLadyGetPrize（gStringVar2＝賞品の名前）
void HoennNative_FavorLadyGetPrize(void)
{
	struct HackLadySave* l = Lady();
	u16 prize = sFavorPrizes[l->favorId];

	CopyItemName(prize, gStringVar2);
	l->favorState = LADY_STATE_PRIZE;
	Save();
	Var8004 = prize;
}

//special SetFavorLadyState_Complete: 原文は InitLilycoveFavorLady（Random）してから COMPLETED
void HoennNative_SetFavorLadyState_Complete(void)
{
	struct HackLadySave* l = Lady();

	InitFavor(l, NULL);
	l->favorState = LADY_STATE_COMPLETED;
	Save();
}

//---- コンテストのおねえさん --------------------------------------------------

//specialvar VAR_RESULT, HasPlayerGivenContestLadyPokeblock
void HoennNative_HasPlayerGivenContestLadyPokeblock(void)
{
	Var800D = Lady()->contestGiven == TRUE ? TRUE : FALSE;
}

//specialvar VAR_RESULT, ShouldContestLadyShowGoOnAir
void HoennNative_ShouldContestLadyShowGoOnAir(void)
{
	struct HackLadySave* l = Lady();

	Var800D = (l->contestGood >= LADY_GIFT_THRESHOLD || l->contestOther >= LADY_GIFT_THRESHOLD) ? TRUE : FALSE;
}

//special Script_BufferContestLadyCategoryAndMonName（gStringVar1＝連れの名前・gStringVar2＝部門の語）
void HoennNative_BufferContestLadyCategoryAndMonName(void)
{
	u8 c = Lady()->contestCategory;

	StringCopy(gStringVar2, gHackLadyCategory[c]);
	StringCopy(gStringVar1, gHackLadyMonName[c]);
}

//special GetContestLadyMonSpecies（Var8005）
void HoennNative_GetContestLadyMonSpecies(void)
{
	Var8005 = sContestMonSpecies[Lady()->contestCategory];
}

//specialvar VAR_RESULT, GetContestLadyCategory
void HoennNative_GetContestLadyCategory(void)
{
	Var800D = Lady()->contestCategory;
}

//special SetContestLadyGivenPokeblock
void HoennNative_SetContestLadyGivenPokeblock(void)
{
	Lady()->contestGiven = TRUE;
	Save();
}

//原文 GivePokeblockToContestLady: 部門の味が 0 でなければ「よい」。ポロックの枝の「あげる」がここを呼ぶ（結果は Var8004）
bool8 GivePokeblockToContestLady(struct Pokeblock* pokeblock)
{
	struct HackLadySave* l = Lady();
	bool8 correct = FALSE;

	switch (l->contestCategory)
	{
	case 0: correct = pokeblock->spicy != 0; break;
	case 1: correct = pokeblock->dry != 0; break;
	case 2: correct = pokeblock->sweet != 0; break;
	case 3: correct = pokeblock->bitter != 0; break;
	case 4: correct = pokeblock->sour != 0; break;
	}
	if (correct)
		l->contestGood++;
	else
		l->contestOther++;
	Save();
	return correct;
}

//special OpenPokeblockCaseForContestLady: ポロックケースを PBLOCK_CASE_GIVE で開く（まとめ 0.66.0: ポロックの枝の本物のケースにつないだ。閉じたら台本の続きへ＝台本は waitstate で待つ）。
//ケースの「あげる」が GivePokeblockToContestLady を呼んで Var8004 に結果・Var800D＝選んだ番号を書く／やめたら Var800D＝0xFFFF（src/hack_pokeblock_case.c）
void HoennNative_OpenPokeblockCaseForContestLady(void)
{
	OpenPokeblockCase(PBLOCK_CASE_GIVE, PBC_RET_SCRIPT);
}

//---- クイズのおねえさん（S10・ユーザー決定 2026-11-10: 3 つの候補から選ぶ）-----------------------------------
//原文 lilycove_lady.c の QuizLady 系。EC（かんたんことば）の画面は使わず、問題文を会話の窓に出して CFRU の自由な選択肢（3 つ）で答えさせる。
//自分でクイズを作る枝・他のプレイヤーの問題（通信）は外した＝出題者はいつも「おねえさん」。台本は原文のまま（special だけ読み替え・hoenn_contest_scripts.py）。

static const struct HackQuiz* CurQuiz(void)
{
	u8 q = Lady()->quizQ;

	return &gHackQuiz[q < HACK_QUIZ_COUNT ? q : 0];
}

//specialvar VAR_RESULT, GetQuizLadyState
void HoennNative_GetQuizLadyState(void)
{
	u8 s = Lady()->quizState;

	Var800D = (s == LADY_STATE_PRIZE) ? LADY_STATE_PRIZE : (s == LADY_STATE_COMPLETED) ? LADY_STATE_COMPLETED : LADY_STATE_READY;
}

//specialvar VAR_RESULT, GetQuizAuthor: 原文は問題の出題者（おねえさん／自分／他のプレイヤー）。出題者はいつもおねえさん＝gStringVar1 に呼び名（原文 BufferQuizAuthorName）
void HoennNative_GetQuizAuthor(void)
{
	StringCopy(gStringVar1, gHackQuizLadyName);
	Var800D = QUIZ_AUTHOR_LADY;
}

//specialvar VAR_RESULT, BufferQuizAuthorNameAndCheckIfLady
void HoennNative_BufferQuizAuthorNameAndCheckIfLady(void)
{
	StringCopy(gStringVar1, gHackQuizLadyName);
	Var800D = TRUE;
}

//specialvar VAR_RESULT, IsQuizLadyWaitingForChallenger: 自分で作る枝が無い＝待っている問題は無い
void HoennNative_IsQuizLadyWaitingForChallenger(void)
{
	Var800D = FALSE;
}

//special ClearQuizLadyPlayerAnswer
void HoennNative_ClearQuizLadyPlayerAnswer(void)
{
	Lady()->quizSel = QUIZ_NONE;
}

//special QuizLadyShowQuizQuestion の前半: 問題文を gStringVar1（台本が [BUFFER1] の文で出す）・3 つの選択肢を自由な選択肢に・窓の幅を Var8006 に
void HoennNative_QuizLadyBufferQuestion(void)
{
	const struct HackQuiz* q = CurQuiz();
	u8 i;

	StringCopy(gStringVar1, q->question);
	for (i = 0; i < 3; ++i)
	{
		Var8006 = i;
		gLoadPointer = q->choice[i];
		sp025_AddTextByPointer();
	}
	Var8006 = gHackQuizMenuWidth;
}

//special QuizLadyShowQuizQuestion の後半（multichoice のあと）: VAR_RESULT＝選んだ位置（0〜2・B で 127）→ 選んだら 1（答えを出す）・やめたら 0（原文 EC 画面の戻り値と同じ）
void HoennNative_QuizLadyEvaluate(void)
{
	struct HackLadySave* l = Lady();

	if (Var800D < 3)
	{
		l->quizSel = (u8) Var800D;
		Var800D = 1;
	}
	else
	{
		l->quizSel = QUIZ_NONE;
		Var800D = 0;
	}
}

//specialvar VAR_RESULT, IsQuizAnswerCorrect
void HoennNative_IsQuizAnswerCorrect(void)
{
	Var800D = (Lady()->quizSel == CurQuiz()->answer) ? TRUE : FALSE;
}

//special BufferQuizCorrectAnswer（gStringVar3）
void HoennNative_BufferQuizCorrectAnswer(void)
{
	const struct HackQuiz* q = CurQuiz();

	StringCopy(gStringVar3, q->choice[q->answer]);
}

//special BufferQuizPrizeName（gStringVar1）
void HoennNative_BufferQuizPrizeName(void)
{
	CopyItemName(CurQuiz()->prize, gStringVar1);
}

//special BufferQuizPrizeItem（Var8005）
void HoennNative_BufferQuizPrizeItem(void)
{
	Var8005 = CurQuiz()->prize;
}

//special SetQuizLadyState_Complete／_GivePrize
void HoennNative_SetQuizLadyState_Complete(void)
{
	Lady()->quizState = LADY_STATE_COMPLETED;
	Save();
}

void HoennNative_SetQuizLadyState_GivePrize(void)
{
	Lady()->quizState = LADY_STATE_PRIZE;
	Save();
}

//special QuizLadyPickNewQuestion: 原文は前の問題と違う問題（乱数）。出題が済んだら READY に戻す
void HoennNative_QuizLadyPickNewQuestion(void)
{
	struct HackLadySave* l = Lady();
	u8 q;

	l->quizPrev = l->quizQ;
	do
		q = Random() % HACK_QUIZ_COUNT;
	while (q == l->quizPrev);
	l->quizQ = q;
	l->quizState = LADY_STATE_READY;
	l->quizSel = QUIZ_NONE;
	Save();
}
