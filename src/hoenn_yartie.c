#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/constants/items.h"
#include "../include/constants/moves.h"
#include "../include/main.h"
#include "../include/pokemon.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/new/frontier.h"
#include "../include/new/util.h"
#include "Tables/hoenn_script_flags.h"
#include "hack_stat_caps.h"
#include "hoenn_yartie.h"
#include "hoenn_yartie_data.h"

/*
 * ロジックアリーナ（役割論理＝ヤーティの専門の施設・Redmine #116・patches/0817〜0819）。相手は「ロンジャ」7 人（tools/frontier/yartie_input.csv）。
 * 流れ（1 戦ごとに完結・assembly/hack_yartie/yartie_scripts.s）:
 *   受付で話す → 見せ合い（通信対戦の選出画面を通信なしで使う・src/link_select_screen.c の offline）で相手の 6 匹を見て 3 匹を選ぶ
 *   → HoennYartie_PickOpponent3（相手が役割点で 6 匹から 3 匹）→ SP_SAVE_PARTY → HoennTower_PrepareTeam（全員ちょうど Lv50）→ 戦闘 → SP_LOAD_PARTY（勝敗にかかわらず）。
 * ★ 書き換え可能な static は使えない（6.13）。状態は旗 0xFD8／0xFD9・変数 HOENN_VAR_YARTIE_OPP／HOENN_VAR_YARTIE_PROGRESS。
 *   HOENN_VAR_YARTIE_OPP: 下位 9 ビット = 相手の 3 匹の添字（3 ビット × 3・0〜5）・最上位ビット = 見せ合いの画面を通信なしで出す印（YARTIE_SELECT_MARK）
 */

#define YARTIE_LEVEL 50


static u8 CurrentOpponent(void)
{
	u16 p = VarGet(HOENN_VAR_YARTIE_PROGRESS);

	return (p < HOENN_YARTIE_TRAINER_COUNT) ? p : HOENN_YARTIE_TRAINER_COUNT - 1;
}

/* ---- 状態（台本から） ----------------------------------------------------------------------------------------------- */
//施設の設定（受付・戦闘のたび）。レベルは 50 固定・シングル・3 匹
void HoennYartie_SetUpFacility(void)
{
	void HoennYartie_SetUpFacilityVars(void);   //src/hoenn_tower.c（SetUpFacility は static）

	HoennYartie_SetUpFacilityVars();
	VarSet(HOENN_VAR_YARTIE_OPP, VarGet(HOENN_VAR_YARTIE_OPP) & ~YARTIE_SELECT_MARK);
}

//挑戦を始める（受付）
void HoennYartie_Begin(void)
{
	VarSet(HOENN_VAR_YARTIE_PROGRESS, 0);
	VarSet(HOENN_VAR_YARTIE_OPP, 0);
	FlagSet(HACK_FLAG_YARTIE_ACTIVE);
}

//挑戦を終える（負け・やめる・7 人抜き）
void HoennYartie_End(void)
{
	VarSet(HOENN_VAR_YARTIE_PROGRESS, 0);
	VarSet(HOENN_VAR_YARTIE_OPP, 0);
	FlagClear(HACK_FLAG_YARTIE_ACTIVE);
	FlagClear(FLAG_BATTLE_FACILITY);
}

//→ LASTRESULT: 何人目か（0〜6・7 は全員に勝った）
void HoennYartie_GetProgress(void)
{
	gSpecialVar_LastResult = VarGet(HOENN_VAR_YARTIE_PROGRESS);
}

//勝った（次の相手へ）。→ LASTRESULT: 何人に勝ったか
void HoennYartie_Advance(void)
{
	u16 p = VarGet(HOENN_VAR_YARTIE_PROGRESS);

	if (p < YARTIE_BATTLES)
		++p;
	VarSet(HOENN_VAR_YARTIE_PROGRESS, p);
	gSpecialVar_LastResult = p;
}

//1 戦ごとの BP（フロンティアと同じ式の表 HoennTower_GiveBattlePoints の「ロジックアリーナ」の行・src/hoenn_tower.c）＋全員に勝ったら上乗せ。→ gStringVar1 = もらった数
void HoennTower_GiveYartieBattlePoints(void);   //src/hoenn_tower.c（表は static）
void HoennYartie_GiveBattlePoints(void)
{
	HoennTower_GiveYartieBattlePoints();
}

//→ gStringVar2 = 何人目か（1〜7・つづきの文用）
void HoennYartie_BufferProgress(void)
{
	ConvertIntToDecimalStringN(gStringVar2, CurrentOpponent() + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
}

//見せ合いの画面を出す（台本が fadescreen 1 とマークをしたあと・waitstate で戻りを待つ）
void CB2_StartCreateLinkBattleSelectScreen(void);   //src/link_select_screen.c
void HoennYartie_OpenSelectScreen(void)
{
	SetMainCallback2(CB2_StartCreateLinkBattleSelectScreen);
}

//次の相手を決める。VAR_FACILITY_TRAINER_ID = 特別なトレーナーの表の番号・名前を gStringVar1 へ
void HoennYartie_SetOpponent(void)
{
	u16 id = HOENN_YARTIE_FIRST_TRAINER + CurrentOpponent();

	VarSet(VAR_FACILITY_TRAINER_ID, id);
	StringCopy(gStringVar1, gSpecialTowerTrainers[id].name);
	ConvertIntToDecimalStringN(gStringVar2, CurrentOpponent() + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
}

/* ---- 見せ合いの画面（src/link_select_screen.c の offline から） -------------------------------------------------------- */
//台本が画面を出す直前に呼ぶ（印を立てる）
void HoennYartie_MarkSelectOffline(void)
{
	VarSet(HOENN_VAR_YARTIE_OPP, VarGet(HOENN_VAR_YARTIE_OPP) | YARTIE_SELECT_MARK);
}

bool8 HoennYartie_SelectIsOffline(void)
{
	return (VarGet(HOENN_VAR_YARTIE_OPP) & YARTIE_SELECT_MARK) != 0;
}

//画面を出るとき（印を消す）
void HoennYartie_SelectDone(void)
{
	VarSet(HOENN_VAR_YARTIE_OPP, VarGet(HOENN_VAR_YARTIE_OPP) & ~YARTIE_SELECT_MARK);
}

const u8* HoennYartie_OpponentName(void)
{
	return gSpecialTowerTrainers[HOENN_YARTIE_FIRST_TRAINER + CurrentOpponent()].name;   //見せ合いの相手の名前（ヤメタグロス など）
}

//相手の 6 匹を gEnemyParty に作る（見せ合いの表示用・Lv50）
void HoennYartie_FillEnemy6(void)
{
	const struct HoennYartieMon* set = sHoennYartieMons[CurrentOpponent()];
	u32 i;

	for (i = 0; i < HOENN_YARTIE_PER_TRAINER; ++i)
	{
		CreateMon(&gEnemyParty[i], set[i].species, YARTIE_LEVEL, 31, FALSE, 0, OT_ID_PLAYER_ID, 0);
		SetMonData(&gEnemyParty[i], MON_DATA_HELD_ITEM, &set[i].item);
	}
}

/* ---- 相手が 6 匹から 3 匹を選ぶ（役割点） -------------------------------------------------------------------------------- */
//[hack] patches/0915 技の倍率は 2 つ目のタイプ（type2）も掛ける HackAI_MoveMul（src/ai_hack.c・damage_calc.c と同じ考え方）
u32 HackAI_MoveMul(u16 move, u8 t1, u8 t2, u8 t3);
u32 HackAI_TypeMulReal(u8 moveType, u8 t1, u8 t2, u8 t3);   //等倍＝1000 の正しい倍率（gTypeEffectiveness の 0＝等倍・1＝効果なしを直す）

//foe の技の中に、(t1,t2) のタイプへ 2 倍以上で威力 60 以上のものがあるか
static bool8 HasSuperEffective(const struct HoennYartieMon* foe, u8 t1, u8 t2)
{
	u32 m;

	for (m = 0; m < MAX_MON_MOVES; ++m)
	{
		u16 move = foe->moves[m];

		if (move == MOVE_NONE || MOVE_POWER(move) < 60)
			continue;
		if (HackAI_MoveMul(move, t1, t2, t1) > TYPE_MUL_NORMAL)
			return TRUE;
	}
	return FALSE;
}

//こちらの手持ち mon が、種族のタイプではなく実際の技（威力 60 以上・2 つ目のタイプも）で、相手の (foeT1,foeT2) に半減以下しか通らないか。
//★ 実行時にタイプが決まる技（ノーマル Lv120）は受けられない扱い。*readable＝威力 60 以上の技を 1 つでも読めたか（読めなければ呼び出し側が種族タイプで代わりに判定する）
static bool8 ResistsPlayerMoves(struct Pokemon* mon, u8 foeT1, u8 foeT2, bool8* readable)
{
	u32 m;

	*readable = FALSE;
	for (m = 0; m < MAX_MON_MOVES; ++m)
	{
		u16 move = GetMonData(mon, MON_DATA_MOVE1 + m, NULL);

		if (move == MOVE_NONE || MOVE_POWER(move) < 60)
			continue;
		*readable = TRUE;
		if (move == MOVE_LV200_NORMAL_120 || HackAI_MoveMul(move, foeT1, foeT2, foeT1) > TYPE_MUL_NORMAL / 2)
			return FALSE;
	}
	return *readable;
}

//i の一致タイプ（2 つ）の技を、j のタイプが両方とも半減以下で受けるか（技が読めないときの代わり）
static bool8 ResistsStab(u8 foeT1, u8 foeT2, u8 mineT1, u8 mineT2)
{
	u8 mine[2];
	u32 k;

	mine[0] = mineT1;
	mine[1] = mineT2;
	for (k = 0; k < 2; ++k)
	{
		if (k == 1 && mineT2 == mineT1)
			break;
		//[hack] patches/0915 表の値をそのまま掛けると等倍（0）が「半減以下」になっていた → HackAI_TypeMulReal
		if (HackAI_TypeMulReal(mine[k], foeT1, foeT2, foeT1) > TYPE_MUL_NORMAL / 2)
			return FALSE;
	}
	return TRUE;
}

//役割点: こちらの 6 匹それぞれに対して「弱点を突ける」＋「一致技を半減以下で受けられる」。点の高い順に 3 匹。同点は耐久＋火力の種族値の合計。
//こちらの手持ち（SP_SAVE_PARTY の前の元の 6 匹・たまごは数えない）を見て、結果を HOENN_VAR_YARTIE_OPP の下位 9 ビットへ
void HoennYartie_PickOpponent3(void)
{
	const struct HoennYartieMon* set = sHoennYartieMons[CurrentOpponent()];
	u16 score[HOENN_YARTIE_PER_TRAINER];
	u32 j, i, n;
	u16 packed = 0;
	bool8 taken[HOENN_YARTIE_PER_TRAINER] = {FALSE};

	for (j = 0; j < HOENN_YARTIE_PER_TRAINER; ++j)
	{
		const struct BaseStats* fs = &gBaseStats[set[j].species];
		u16 pts = 0;

		for (i = 0; i < PARTY_SIZE; ++i)
		{
			u16 sp = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
			const struct BaseStats* ms;

			if (sp == SPECIES_NONE || sp == SPECIES_EGG)
				continue;
			ms = &gBaseStats[sp];
			bool8 readable;

			if (HasSuperEffective(&set[j], ms->type1, ms->type2))
				pts += 4;
			if (ResistsPlayerMoves(&gPlayerParty[i], fs->type1, fs->type2, &readable) || (!readable && ResistsStab(fs->type1, fs->type2, ms->type1, ms->type2)))
				pts += 4;   //[hack] patches/0915 実際の技で見る（読めない子だけ種族タイプ）
		}
		//同点は種族値の合計（耐久＋火力）。点を 1000 倍相当にして足す
		score[j] = pts * 64 + (fs->baseHP + fs->baseDefense + fs->baseSpDefense + MathMax(fs->baseAttack, fs->baseSpAttack)) / 16;
	}
	for (n = 0; n < 3; ++n)
	{
		u32 best = HOENN_YARTIE_PER_TRAINER;

		for (j = 0; j < HOENN_YARTIE_PER_TRAINER; ++j)
		{
			if (!taken[j] && (best == HOENN_YARTIE_PER_TRAINER || score[j] > score[best]))
				best = j;
		}
		taken[best] = TRUE;
		packed |= best << (3 * n);
	}
	VarSet(HOENN_VAR_YARTIE_OPP, (VarGet(HOENN_VAR_YARTIE_OPP) & YARTIE_SELECT_MARK) | packed); //交代の回数（9〜11 ビット）も 0 に戻す
}

//ロンジャの交代の回数（9〜11 ビット・src/ai_hack.c の上限と数える）
u8 HoennYartie_SwitchCount(void)
{
	return (VarGet(HOENN_VAR_YARTIE_OPP) >> 9) & 7;
}

void HoennYartie_NoteSwitch(void)
{
	u16 v = VarGet(HOENN_VAR_YARTIE_OPP);

	if (((v >> 9) & 7) < 7)
		VarSet(HOENN_VAR_YARTIE_OPP, v + 0x200);
}

/* ---- 戦闘の手持ち（src/build_pokemon.c の SPECIAL_TRAINER_REGULAR_SPREADS・patches/0817） ---------------------------------- */
bool8 HoennYartie_InBattle(void)
{
	return FlagGet(HACK_FLAG_YARTIE_ACTIVE) && FlagGet(FLAG_BATTLE_FACILITY)
	    && VarGet(HOENN_VAR_FRONTIER_FACILITY) == HOENN_FACILITY_YARTIE;
}

static const struct HoennYartieMon* PickedMon(u16 tableId, u8 slot)
{
	u8 idx = (tableId >= HOENN_YARTIE_FIRST_TRAINER) ? tableId - HOENN_YARTIE_FIRST_TRAINER : 0;
	u8 pick = (VarGet(HOENN_VAR_YARTIE_OPP) >> (3 * (slot % 3))) & 7;

	if (idx >= HOENN_YARTIE_TRAINER_COUNT)
		idx = HOENN_YARTIE_TRAINER_COUNT - 1;
	if (pick >= HOENN_YARTIE_PER_TRAINER)
		pick = 0;
	return &sHoennYartieMons[idx][pick];
}

//ニックネーム（頭を「ヤ」にした名前・tools/frontier/yartie_input.csv の nickname 列）
void HoennYartie_SetNickname(struct Pokemon* mon, u16 tableId, u8 slot)
{
	SetMonData(mon, MON_DATA_NICKNAME, PickedMon(tableId, slot)->nickname);
}

const struct BattleTowerSpread* HoennYartie_MakeSpread(struct BattleTowerSpread* out, u16 tableId, u8 slot)
{
	const struct HoennYartieMon* mon = PickedMon(tableId, slot);
	u32 i;

	Memset(out, 0, sizeof(struct BattleTowerSpread));
	out->species = mon->species;
	out->nature = mon->nature;
	out->ball = 3;   //BALL_TYPE_POKE_BALL（エメラルドの施設のポケモンと同じ）
	out->hpIv = out->atkIv = out->defIv = out->spdIv = out->spAtkIv = out->spDefIv = HACK_IV_MAX;
	out->hpEv = mon->ev[0];
	out->atkEv = mon->ev[1];
	out->defEv = mon->ev[2];
	out->spdEv = mon->ev[3];
	out->spAtkEv = mon->ev[4];
	out->spDefEv = mon->ev[5];
	out->item = mon->item;
	for (i = 0; i < MAX_MON_MOVES; ++i)
		out->moves[i] = mon->moves[i];
	out->ability = mon->ability;
	out->teraType = mon->teraType;
	out->forSingles = TRUE;
	out->forDoubles = TRUE;
	return out;
}
