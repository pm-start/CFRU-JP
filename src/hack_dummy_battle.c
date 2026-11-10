#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/pokemon.h"
#include "../include/constants/items.h"
#include "../include/constants/moves.h"
#include "../include/constants/pokemon.h"
#include "../include/constants/species.h"
#include "../include/new/util.h"
#include "../include/new/wild_encounter.h"
#include "hack_dummy_battle.h"

/*
    ポケモンセンター 3F の「れんしゅうの あいて」（ダミー）。patches/0602・#138・docs/research/2026-10-17_pc-3f-dummy.md

    台本（assembly/kanto/pc_floor3_scripts.s）が一時変数 0x4001（種類）・0x4002（レベルの組）・0x4003（組の中の何番目）に選ばせたものを、callasm HackDummy_Start で戦いにする。
    作りは ？？？（hack_challenge.c の HackChallenge_StartLentrantBattle）と同じ: gEnemyParty[0] を自分で作って sp138_StartLegendaryBattle（野生戦）。
    トレーナー戦でなく野生戦にした理由: ① 賞金が出ない（トレーナー戦は getmoneyreward・倒した印・レベルスケーリング・ジムの段階・クラスの入れ替えが全部かかる）
    ② にげるが 野生戦のまま通る（トレーナー戦は「にげられない」）③ 相手のレベルは自分で作ったまま（スケーリングを通らない）。足りない分は下の旗 HACK_FLAG_DUMMY_BATTLE の 1 本で止める:

      ・技を出さない     OpponentHandleChooseMove（battle_controller_opponent.c）の頭で「はねる」を選ぶ（何も起きない）。CheckMoveLimitations の頭で
                         ダミー側は全部使える扱い＝かなしばり／ちょうはつ／じゅうりょく／いちゃもんでも わるあがきにならない
      ・にげる（R も）   TryRunFromBattle の頭で 自分の側は必ず成功・IsRunningFromBattleImpossible の頭で FALSE（end_battle.c）
      ・経験値・努力値   atk23_getexp の入口で 経験値の分配ごと飛ばす（exp.c）＝努力値も入らない
      ・賞金             atk91_givepaydaymoney（ピカチュウの「ネコにこばん」・Pay Day）を 0 円に（general_bs_commands.c）。野生戦なので ふつうの賞金は無い
      ・ぜんめつ         HandleEndTurn_BattleLost の頭で 「にげた」にして終わる＝ぜんめつの文・所持金を半分にする処理・ワープが走らない。EndOfBattleThings で 全滅していたら全回復
      ・しばり・上限     EndOfBattleThings の しばり／貯金／なつき度／ひんしでお別れ を飛ばす・HackBadgeSquash_BattleAllowed（上限をこえた子を戦闘のあいだ上限の
                         レベルにする）と HackChallengeC_OnBattleStart（モノタイプで出せない子を HP 0 にする）を止める
      ・つかまえる       FLAG_NO_CATCHING（0x902）でボールははじかれる（戦いの終わりで CFRU が下ろす）
*/

void CB2_EndScriptedWildBattle_2(void);

struct DummyDef
{
	u16 species;
	u8 nature;
};

//台本の並びと同じ（0 ヌケニン・1 ハガネール・2 ハピナス・3 ツボツボ）。努力値は 体力・ぼうぎょ・とくぼう に最大（252）・個体値 31・もちものなし・技は はねる だけ
static const struct DummyDef sDummies[HACK_DUMMY_KIND_COUNT] =
{
	{SPECIES_SHEDINJA, NATURE_HARDY},   //ふしぎな まもり（HP 1・こうかばつぐん以外は効かない）
	{SPECIES_STEELIX, NATURE_IMPISH},   //ぶつりがたの かべ
	{SPECIES_BLISSEY, NATURE_CALM},     //とくしゅがたの かべ
	{SPECIES_SHUCKLE, NATURE_BOLD},     //りょうほうの かべ
};

bool8 HackDummy_Active(void)
{
	return FlagGet(HACK_FLAG_DUMMY_BATTLE) && !(gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK));
}

bool8 HackDummy_ForceUsable(u8 bank)
{
	return HackDummy_Active() && SIDE(bank) == B_SIDE_OPPONENT;
}

void HackDummy_KeepSplash(u8 bank)
{
	gBattleMons[bank].moves[0] = MOVE_SPLASH;
	gBattleMons[bank].pp[0] = 40;
	gProtectStructs[bank].onlyStruggle = 0;
}

void HackDummy_Rescue(void)
{
	if (HackDummy_Active() && ViableMonCount(gPlayerParty) == 0)
		HealPlayerParty();
}

//戦いの終わり（saved callback）。旗を下ろして、ふつうの野生戦の終わり（CB2_EndScriptedWildBattle_2）へ。ぜんめつは HandleEndTurn_BattleLost で「にげた」にしてあるが、念のためここでも負け／引き分けを にげたにそろえる
static void HackDummy_CB2_EndBattle(void)
{
	FlagClear(HACK_FLAG_DUMMY_BATTLE);
	if (gBattleOutcome == B_OUTCOME_LOST || gBattleOutcome == B_OUTCOME_DREW)
		gBattleOutcome = B_OUTCOME_RAN;
	CB2_EndScriptedWildBattle_2();
}

static void BuildDummy(struct Pokemon* mon, const struct DummyDef* def, u16 level)
{
	u32 personality = 0x12AB5C00;
	u16 ev = EV_CAP;
	u16 zero = 0;
	u16 splash = MOVE_SPLASH;
	u8 pp = 40;
	u16 hp;
	u32 i;

	personality = personality - (personality % 25) + def->nature;   //性格値 % 25 ＝ 性格（性別・特性の欄は下位ビットのまま）
	CreateMon(mon, def->species, level, 31, TRUE, personality, OT_ID_RANDOM_NO_SHINY, 0);
	SetMonData(mon, MON_DATA_HP_EV, &ev);
	SetMonData(mon, MON_DATA_DEF_EV, &ev);
	SetMonData(mon, MON_DATA_SPDEF_EV, &ev);
	SetMonData(mon, MON_DATA_HELD_ITEM, &zero);
	SetMonData(mon, MON_DATA_MOVE1, &splash);
	SetMonData(mon, MON_DATA_PP1, &pp);
	for (i = 1; i < MAX_MON_MOVES; ++i)
	{
		SetMonData(mon, MON_DATA_MOVE1 + i, &zero);
		SetMonData(mon, MON_DATA_PP1 + i, &zero);
	}
	CalculateMonStats(mon);
	hp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	SetMonData(mon, MON_DATA_HP, &hp);
}

/*
    3F で書いたセーブを 古い版（群 79 の 10 番もレイアウト 1166 も無い）で開いても壊れないように、書く瞬間だけ「入ってきた階段（2 階・1 階）」へすり替える（セーブ互換・ユーザー 2026-10-17）。
    ゲーム中の位置・変数は触らない（SaveWriteToFlash が書き終えたらすぐ戻す）。つづきからは 新しい版でも 2 階の階段の隣から始まる。

    ★ 2026-10-18 直し（0.53.2 の mGBA: 3F で保存 → つづきから → 3F に出た・RAM の位置は 2 階）。位置の欄だけ替えても足りなかった:
      ① 素の「つづきから」（ワープなし）は 地図に入り直さず セーブの eventObjects（3F の受付の人・主人公の mapGroup／mapNum が 79／10）と mapView（3F の見た目）をそのまま使う
         ＝2 階に 3F の受付が立つ・主人公の絵の持ち主の地図が 3F。→ 素の continueGameWarp の仕組み（specialSaveWarpFlags の bit0＝UseContinueGameWarp）を立てて書く。
         つづきからは 2 階を ワープで読み直す（人は 2 階の表から・一時変数・一時フラグは 0・mapView は使わない）。旧版も同じ素の仕組みなので同じに読める。
      ② 階段のマス自身（warp の行き先 79/10 のワープがあるマス）に立たせると 階段のマスが動いて 3F へ飛び戻る。階段から出たあとの場所（x + 1）に立たせる。
      ③ eventObjects の mapGroup／mapNum（79／10）は 行き先の地図の番号に替える（主人公の絵を含む）。dynamicWarp／escapeWarp／lastHealLocation が 3F を指していたら行き先に。
    mapView・eventObjectTemplates は 3F の中身のまま残る（番号は持たない・ワープで読み直すので使われない）。
*/
static void CopyWarp(struct WarpData* d, const struct WarpData* s)   //構造体の代入は memcpy を呼ぶので 1 欄ずつ
{
	d->mapGroup = s->mapGroup;
	d->mapNum = s->mapNum;
	d->warpId = s->warpId;
	d->x = s->x;
	d->y = s->y;
}

static bool8 IsPc3f(const struct WarpData* w)
{
	return w->mapGroup == HACK_PC3F_GROUP && w->mapNum == HACK_PC3F_NUM;
}

//素の ROM のワープの 1 行（8 バイト）: x・y・高さ・行き先のワープ番号・行き先の地図番号・行き先の群。include/global.fieldmap.h の struct WarpEvent は 高さが無く並びも違う（mapGroup が 行き先のワープ番号の位置）ので使わない
struct HackPc3fWarpRow
{
	s16 x;
	s16 y;
	u8 elevation;
	u8 warpId;
	u8 mapNum;
	u8 mapGroup;
};

//行き先の地図 header のワープに「その座標から 3F へ行く」行があるか（＝3F への階段のマス）
static bool8 IsStairsToPc3f(const struct MapHeader* header, const struct WarpData* at)
{
	const struct MapEvents* ev = header->events;
	u32 i;

	if (ev == NULL || ev->warps == NULL)
		return FALSE;
	for (i = 0; i < ev->warpCount; ++i)
	{
		const struct HackPc3fWarpRow* w = (const struct HackPc3fWarpRow*) &ev->warps[i];

		if (w->mapGroup == HACK_PC3F_GROUP && w->mapNum == HACK_PC3F_NUM && w->x == at->x && w->y == at->y)
			return TRUE;
	}
	return FALSE;
}

void HackPc3f_SaveBegin(struct HackPc3fSave* b)
{
	struct WarpData home;
	const struct WarpData* to = &gSaveBlock1->dynamicWarp;
	struct MapHeader* header;
	u32 i;
	s16 x;

	b->on = FALSE;
	if (!IsPc3f(&gSaveBlock1->location))
		return;
	if (IsPc3f(to) || to->mapGroup == 0x7F || to->mapNum == 0x7F)
		to = &gSaveBlock1->lastHealLocation;                    //動的ワープが 3F／無効なら最後に回復した場所
	if (IsPc3f(to) || (to->mapGroup == 0 && to->mapNum == 0 && to->x == 0 && to->y == 0))
	{
		home.mapGroup = 4; home.mapNum = 0; home.warpId = -1; home.x = 8; home.y = 5;   //マサラの家（hack_challenge_c.h の HACK_CHC_HOME_*・全滅の戻り先と同じ）
		to = &home;
	}
	header = Overworld_GetMapHeaderByGroupAndId(to->mapGroup, to->mapNum);
	if (header == NULL)
		return;
	b->on = TRUE;
	b->flags = gSaveBlock2->specialSaveWarpFlags;
	b->pos.x = gSaveBlock1->pos.x;
	b->pos.y = gSaveBlock1->pos.y;
	b->mapLayoutId = gSaveBlock1->mapLayoutId;
	CopyWarp(&b->location, &gSaveBlock1->location);
	CopyWarp(&b->continueGameWarp, &gSaveBlock1->continueGameWarp);
	CopyWarp(&b->dynamicWarp, &gSaveBlock1->dynamicWarp);
	CopyWarp(&b->escapeWarp, &gSaveBlock1->escapeWarp);
	CopyWarp(&b->lastHealLocation, &gSaveBlock1->lastHealLocation);

	x = to->x;
	if (IsStairsToPc3f(header, to))
		++x;                                                    //階段のマスの隣（階段から出たあとに立つ所）
	b->objMask = 0;
	for (i = 0; i < MAP_OBJECTS_COUNT; ++i)
	{
		struct EventObject* o = &gSaveBlock1->eventObjects[i];

		if (o->mapGroup == HACK_PC3F_GROUP && o->mapNum == HACK_PC3F_NUM)
		{
			b->objMask |= (1 << i);
			o->mapGroup = to->mapGroup;
			o->mapNum = to->mapNum;
		}
	}
	if (IsPc3f(&gSaveBlock1->dynamicWarp))
		CopyWarp(&gSaveBlock1->dynamicWarp, to);
	if (IsPc3f(&gSaveBlock1->escapeWarp))
		CopyWarp(&gSaveBlock1->escapeWarp, to);
	if (IsPc3f(&gSaveBlock1->lastHealLocation))
		CopyWarp(&gSaveBlock1->lastHealLocation, to);
	CopyWarp(&gSaveBlock1->location, to);
	gSaveBlock1->location.warpId = -1;
	gSaveBlock1->location.x = x;
	CopyWarp(&gSaveBlock1->continueGameWarp, &gSaveBlock1->location);
	gSaveBlock1->pos.x = x;
	gSaveBlock1->pos.y = to->y;
	gSaveBlock1->mapLayoutId = header->mapLayoutId;
	gSaveBlock2->specialSaveWarpFlags |= 1;                     //つづきから＝continueGameWarp への ワープ（素の SetContinueGameWarpStatus と同じ bit0）
}

void HackPc3f_SaveEnd(const struct HackPc3fSave* b)
{
	u32 i;

	if (!b->on)
		return;
	for (i = 0; i < MAP_OBJECTS_COUNT; ++i)
	{
		if (b->objMask & (1 << i))
		{
			gSaveBlock1->eventObjects[i].mapGroup = HACK_PC3F_GROUP;
			gSaveBlock1->eventObjects[i].mapNum = HACK_PC3F_NUM;
		}
	}
	gSaveBlock2->specialSaveWarpFlags = b->flags;
	gSaveBlock1->pos.x = b->pos.x;
	gSaveBlock1->pos.y = b->pos.y;
	CopyWarp(&gSaveBlock1->location, &b->location);
	CopyWarp(&gSaveBlock1->continueGameWarp, &b->continueGameWarp);
	CopyWarp(&gSaveBlock1->dynamicWarp, &b->dynamicWarp);
	CopyWarp(&gSaveBlock1->escapeWarp, &b->escapeWarp);
	CopyWarp(&gSaveBlock1->lastHealLocation, &b->lastHealLocation);
	gSaveBlock1->mapLayoutId = b->mapLayoutId;
}

void HackDummy_Start(void)
{
	u16 kind = VarGet(HACK_DUMMY_VAR_KIND);
	u16 group = VarGet(HACK_DUMMY_VAR_GROUP);
	u16 step = VarGet(HACK_DUMMY_VAR_STEP);
	u16 level;

	if (kind >= HACK_DUMMY_KIND_COUNT)
		kind = 0;
	if (group > 3)
		group = 3;
	if (step > 4)
		step = 4;
	level = HACK_DUMMY_MIN_LEVEL + 50 * group + 10 * step;
	if (level < HACK_DUMMY_MIN_LEVEL)
		level = HACK_DUMMY_MIN_LEVEL;
	if (level > HACK_DUMMY_MAX_LEVEL || level > MAX_LEVEL)
		level = (HACK_DUMMY_MAX_LEVEL < MAX_LEVEL) ? HACK_DUMMY_MAX_LEVEL : MAX_LEVEL;

	ZeroEnemyPartyMons();
	BuildDummy(&gEnemyParty[0], &sDummies[kind], (u8) level);
	FlagSet(FLAG_NO_CATCHING);                   //ボールは はじかれる（戦いの終わりで CFRU が下ろす）
	FlagSet(HACK_FLAG_DUMMY_BATTLE);
	sp138_StartLegendaryBattle();
	gMain.savedCallback = HackDummy_CB2_EndBattle;
}
