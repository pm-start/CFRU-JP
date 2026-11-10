#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/new/Vanilla_functions.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/constants/items.h"
#include "../include/random.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/script.h"
#include "../include/sprite.h"
#include "../include/window.h"
#include "../include/menu.h"
#include "../include/sound.h"
#include "../include/script_menu.h"
#include "../include/decompress.h"
#include "hack_contest_addr.h"
#include "hack_contest.h"
#include "hack_condition.h"

/*
 * コンテスト移植 S5a（FEAT-contest・patches/0985）: 受付〜会場の special（出場者の決定・資格の判定・絵の番号・名前のバッファ）。
 * 原文 参考/pokeemerald/src/contest.c 2777〜3010（CreateContestMonFromParty・SetContestants・GetContestEntryEligibility）と
 * contest_util.c 1958〜（TryEnterContestMon）・2294（SetContestTrainerGfxIds）・2680（GenerateContestRand）・2367（ShouldReadyContestArtist）。
 * 引数は台本が Var8004〜8006 に入れて渡す（原文は gSpecialVar_ContestCategory／Rank／gContestMonPartyIndex）:
 *   Var8004＝手持ちの番号（choose の結果）・Var8005＝部門（0 かっこよさ〜4 たくましさ）・Var8006＝ランク（0 ノーマル〜3 マスター）。
 * 通信（LINK_CONTEST_FLAG）は書かない（S1 で範囲外）。NPC の 3 人は gHackContestOpponents（日本語版 ROM の表）から選び、
 * Keep（0x0203E800）に番号だけ残す＝会場の画面の始め（HackContest_BuildMon*）で表と手持ちから作り直す（Keep は 256 バイトで足りないため）。
 */

//原文 GetContestEntryEligibility。手持ちの卵・ひんしは入れない。部門のリボンのランクと今回のランクを比べる（読み口は HackContest_GetCategoryRibbonRank の 1 か所＝仮）
u8 HackContest_GetEligibility(u8 partyIndex, u8 category, u8 rank)
{
	struct Pokemon* mon = &gPlayerParty[partyIndex];
	u8 ribbon;

	if (GetMonData(mon, MON_DATA_IS_EGG, NULL))
		return HACK_CANT_ENTER_CONTEST_EGG;
	if (GetMonData(mon, MON_DATA_HP, NULL) == 0)
		return HACK_CANT_ENTER_CONTEST_FAINTED;
	if (category > 4)
		return HACK_CANT_ENTER_CONTEST;
	ribbon = HackContest_GetCategoryRibbonRank(partyIndex, category);
	if (ribbon > rank)
		return HACK_CAN_ENTER_CONTEST_HIGH_RANK;
	if (ribbon >= rank)
		return HACK_CAN_ENTER_CONTEST_EQUAL_RANK;
	return HACK_CANT_ENTER_CONTEST;
}

//コンディション 6 値の読み口（まとめ 0.66.0: 本物の hack_condition.c の表につないだ。ポロックを食べさせた値。表に無い子・卵は全部 0＝原文の新しい子と同じ）
void HackContest_GetPartyCondition(u8 partyIndex, u8* out6)
{
	HackCondition_Get((struct BoxPokemon*) &gPlayerParty[partyIndex], out6);
}

//原文 CreateContestMonFromParty（trainerName・nickname・5 つの能力に対応するスカーフ +20（255 まで）・sheen・技・personality・otId）
void HackContest_BuildMonFromParty(struct HackContestMon* out, u8 partyIndex)
{
	struct Pokemon* mon = &gPlayerParty[partyIndex];
	u8 cond[6];
	u16 held;
	s32 i;
	s16 v[5];

	memset(out, 0, sizeof(*out));
	StringCopy(out->trainerName, gSaveBlock2->playerName);
	out->aiFlags = 0;
	out->species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	GetMonData(mon, MON_DATA_NICKNAME, out->nickname);
	out->nickname[POKEMON_NAME_LENGTH] = 0xFF;
	HackContest_GetPartyCondition(partyIndex, cond);
	for (i = 0; i < 5; i++)
		v[i] = cond[i];
	held = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
	if (held == ITEM_RED_SCARF)
		v[0] += 20;
	else if (held == ITEM_BLUE_SCARF)
		v[1] += 20;
	else if (held == ITEM_PINK_SCARF)
		v[2] += 20;
	else if (held == ITEM_GREEN_SCARF)
		v[3] += 20;
	else if (held == ITEM_YELLOW_SCARF)
		v[4] += 20;
	for (i = 0; i < 5; i++)
		out->stats[i] = v[i] > 255 ? 255 : v[i];
	out->sheen = cond[5];
	out->moves[0] = GetMonData(mon, MON_DATA_MOVE1, NULL);
	out->moves[1] = GetMonData(mon, MON_DATA_MOVE2, NULL);
	out->moves[2] = GetMonData(mon, MON_DATA_MOVE3, NULL);
	out->moves[3] = GetMonData(mon, MON_DATA_MOVE4, NULL);
	out->personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
	out->otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
}

//NPC（gHackContestOpponents の index 番）→ 出場者
void HackContest_BuildMonFromOpponent(struct HackContestMon* out, u8 index)
{
	const struct HackContestOpponent* o = &gHackContestOpponents[index % HACK_CONTEST_OPPONENT_COUNT];
	s32 i;

	memset(out, 0, sizeof(*out));
	out->species = o->species;
	out->otId = o->otId;
	out->personality = o->personality;
	memcpy(out->nickname, o->nickname, sizeof(out->nickname));
	memcpy(out->trainerName, o->trainerName, sizeof(out->trainerName));
	for (i = 0; i < 4; i++)
		out->moves[i] = o->moves[i];
	for (i = 0; i < 5; i++)
		out->stats[i] = o->stats[i];
	out->sheen = o->stats[5];
	out->aiFlags = o->aiFlags;
}

//原文 CalculateRound1Points（hack_contest_main.c の HC_CalculateRound1Points と同じ式）を Keep の 4 人ぶんで
static s16 Round1Of(const struct HackContestMon* m, u8 category)
{
	const u8* st = m->stats;
	u8 mainStat, sub1, sub2;

	switch (category)
	{
	case 0: mainStat = st[0]; sub1 = st[4]; sub2 = st[1]; break;
	case 1: mainStat = st[1]; sub1 = st[0]; sub2 = st[2]; break;
	case 2: mainStat = st[2]; sub1 = st[1]; sub2 = st[3]; break;
	case 3: mainStat = st[3]; sub1 = st[2]; sub2 = st[4]; break;
	default: mainStat = st[4]; sub1 = st[3]; sub2 = st[0]; break;
	}
	return mainStat + (sub1 + sub2 + m->sheen) / 2;
}

//原文 SetContestants（TryPutPlayerLast＝プレイヤーは 4 番目）。クリア後は postgame 専用の人も混ぜ、前半専用の人は外す
static void SetContestants(u8 category, u8 rank, u8 partyIndex)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;
	u8 pool[HACK_CONTEST_OPPONENT_COUNT];
	u8 count = 0;
	bool8 postgame = FlagGet(FLAG_SYS_GAME_CLEAR);
	struct HackContestMon mon;
	s32 i;

	for (i = 0; i < HACK_CONTEST_OPPONENT_COUNT; i++)
	{
		const struct HackContestOpponent* o = &gHackContestOpponents[i];
		u8 filter = gHackContestPostgameFilter[i];

		if ((o->rankPool & 3) != rank)
			continue;
		if (postgame)
		{
			if (filter == HACK_CONTEST_FILTER_NO_POSTGAME)
				continue;
		}
		else if (filter == HACK_CONTEST_FILTER_ONLY_POSTGAME)
			continue;
		if (o->rankPool & (4 << category))
			pool[count++] = i;
	}
	memset(k, 0, sizeof(*k));
	k->magic = HACK_CONTEST_KEEP_MAGIC;
	k->playerIndex = 3;
	k->category = category;
	k->rank = rank;
	k->partyIndex = partyIndex;
	for (i = 0; i < 3 && count != 0; i++)
	{
		u16 rnd = Random() % count;
		s32 j;

		k->opp[i] = pool[rnd];
		for (j = rnd; j < count - 1; j++)
			pool[j] = pool[j + 1];
		count--;
	}
	for (i = 0; i < 4; i++)
	{
		if (i < 3)
			HackContest_BuildMonFromOpponent(&mon, k->opp[i]);
		else
			HackContest_BuildMonFromParty(&mon, partyIndex);
		k->mons[i].species = mon.species;
		k->mons[i].otId = mon.otId;
		k->mons[i].personality = mon.personality;
		memcpy(k->mons[i].nickname, mon.nickname, sizeof(k->mons[i].nickname));
		memcpy(k->mons[i].trainerName, mon.trainerName, sizeof(k->mons[i].trainerName));
		k->round1Points[i] = Round1Of(&mon, category); //台本の「観客のハート」が GetContestMonCondition で読む（原文 CalculateRound1Points）
	}
}

//原文 TryEnterContestMon: Var8004＝手持ち・Var8005＝部門・Var8006＝ランク → VAR_RESULT（0 入れない・1 同じランク・2 上のランクで勝った・3 卵・4 ひんし）
void TryEnterContestMon(void)
{
	u8 eligibility = HackContest_GetEligibility(Var8004, Var8005, Var8006);

	if (eligibility != HACK_CANT_ENTER_CONTEST && eligibility != HACK_CANT_ENTER_CONTEST_EGG && eligibility != HACK_CANT_ENTER_CONTEST_FAINTED)
		SetContestants(Var8005, Var8006, Var8004);
	Var800D = eligibility;
}

//原文 SetContestTrainerGfxIds: NPC 3 人の絵を VAR_OBJ_GFX_ID_0〜2 へ（会場の出場者 1〜3 の絵）
void SetContestTrainerGfxIds(void)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;
	s32 i;

	for (i = 0; i < 3; i++)
		VarSet(0x4010 + i, gHackContestOpponents[k->opp[i] % HACK_CONTEST_OPPONENT_COUNT].gfx);
}

//原文 BufferContestTrainerAndMonNames: Var8006 の出場者のトレーナー名→StringVar1・ニックネーム→StringVar3・種族→Var8004
void BufferContestTrainerAndMonNames(void)
{
	BufferContestantTrainerName();
	BufferContestantMonNickname();
	BufferContestantMonSpecies();
}

//原文 GenerateContestRand（通信なし）: VAR_RESULT = Random() % VAR_RESULT
void GenerateContestRand(void)
{
	Var800D = Random() % Var800D;
}

//原文 ShouldReadyContestArtist: 優勝・マスターランク・合計 800 点以上なら Var8004=TRUE（絵描きの場面＝S6 が読む）
void ShouldReadyContestArtist(void)
{
	struct HackContestKeep* k = &HACK_CONTEST_KEEP;

	Var8004 = (k->finalStandings[k->playerIndex] == 0 && k->rank == 3 && k->totalPoints[k->playerIndex] >= 800) ? 1 : 0;
}

// ---- 出場者の紹介の絵（原文 ShowContestEntryMonPic／HideContestEntryMonPic・contest_util.c 2577〜2660） ----
// フィールドの上に枠つきの窓（10,3 幅 8 高 8）を出し、その中に出場者（Var8006）のポケモンの正面の絵を置く。戻りは台本の `special HideContestEntryMonPic`
u16 __attribute__((long_call)) LoadCompressedSpritePalette(const struct CompressedSpritePalette* src);
#define gMonFrontPicTable ((const struct CompressedSpriteSheet*) *((u32*) 0x8000128))

static void Task_ShowContestEntryMonPic(u8 taskId)
{
	struct Task* task = &gTasks[taskId];
	struct Sprite* sprite;

	switch (task->data[0])
	{
	case 0:
		task->data[0]++;
		break;
	case 1:
		task->data[5] = CreateWindowFromRect(10, 3, 8, 8);
		DrawStdWindowFrame(task->data[5], TRUE);
		task->data[0]++;
		break;
	case 2:
		break;
	case 3:
		sprite = &gSprites[task->data[2]];
		FreeSpritePaletteByTag(GetSpritePaletteTagByPaletteNum(sprite->oam.paletteNum));
		if (sprite->oam.affineMode)
			FreeOamMatrix(sprite->oam.matrixNum);
		DestroySprite(sprite);
		task->data[0]++;
		break;
	case 4:
		ClearStdWindowAndFrameToTransparent(task->data[5], TRUE);
		RemoveWindow(task->data[5]);
		DestroyTask(taskId);
		break;
	}
}

void ShowContestEntryMonPic(void)
{
	const struct CompressedSpritePalette* palette;
	u32 personality, otId;
	u16 species;
	u8 spriteId, taskId, left = 10, top = 3;
	struct HackContestKeepMon* m = &HACK_CONTEST_KEEP.mons[Var8006 & 3];

	if (FindTaskIdByFunc(Task_ShowContestEntryMonPic) != 0xFF)
		return;
	ContestFr_AllocateMonSpritesGfx();
	species = m->species;
	if (species >= NUM_SPECIES)
		species = 0;
	personality = m->personality;
	otId = m->otId;
	taskId = CreateTask(Task_ShowContestEntryMonPic, 0x50);
	gTasks[taskId].data[0] = 0;
	gTasks[taskId].data[1] = species;
	HandleLoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species], gMonSpritesGfxPtr->sprites[1], species, personality);
	palette = GetMonSpritePalStructFromOtIdPersonality(species, otId, personality);
	LoadCompressedSpritePalette(palette);
	ContestFr_SetMultiuseSpriteTemplateToPokemon(species, 1); //B_POSITION_OPPONENT_LEFT
	((struct SpriteTemplate*) 0x0202443C)->paletteTag = palette->tag; //gMultiuseSpriteTemplate
	spriteId = CreateSprite((const struct SpriteTemplate*) 0x0202443C, (left + 1) * 8 + 32, (top * 8) + 40, 0);
	PlayCry1(species, 0); //差: 原文は鳴き声つきの正面の絵のアニメ（DoMonFrontSpriteAnimation）・FR に無いので静止＋鳴き声（結果発表の優勝ポケモンと同じ・S4b）
	gTasks[taskId].data[2] = spriteId;
	gTasks[taskId].data[3] = left;
	gTasks[taskId].data[4] = top;
	gSprites[spriteId].callback = SpriteCallbackDummy;
	gSprites[spriteId].oam.priority = 0;
}

void HideContestEntryMonPic(void)
{
	u8 taskId = FindTaskIdByFunc(Task_ShowContestEntryMonPic);

	if (taskId != 0xFF)
	{
		gTasks[taskId].data[0]++;
		ContestFr_FreeMonSpritesGfx();
	}
}
