#include "defines.h"
#include "hack_wild_chain.h"
#include "../include/event_data.h"
#include "../include/load_save.h"
#include "Tables/hoenn_script_flags.h"

/* 表の古い行は 5 つ目以降の欄を書かない（生成物の書き方を前のままにする）。検算の -Wextra で出る警告だけ止める */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

struct HackVersionReset
{
	u16 version;
	u16 kind;     /* 0 フラグを下ろす・1 フラグを立てる・2 変数に値・3 C の関数を呼ぶ（id = sHackVersionFuncs の添字） */
	u16 id;
	u16 value;
	u16 row;      /* when／call を持つ行の通し番号（1〜・0 = 条件も関数も無い行）。同じ行の項目は同じ番号 */
	u16 cond;     /* when の条件の番号（1〜・0 = 条件なし）。sHackVersionConds の cond と同じ */
};
struct HackVersionCond
{
	u16 cond;
	u16 kind;     /* 0 旗が立っている・1 旗が立っていない・2 変数の比較 */
	u16 id;
	u16 op;       /* kind 2 のとき 0 == ・1 != ・2 >= ・3 < ・4 > ・5 <= */
	u16 value;
};
#define HACK_SAVE_VERSION_TABLE
#include "hack_save_version.h"

/*
 * ★ 2026-10-10 ユーザー「セーブデータに版いれられる？」→「はい」: セーブに版を入れる。
 *
 * - 版の数 HACK_SAVE_VERSION は配る版（make_demo_bundle.py の VERSION）から作る生成物（hack_save_version.h・
 *   tools/build/build_save_version.py）。0.<更新>.<バグ修正> → 更新 * 256 + バグ修正（0.35.2 は 0x2302）
 * - HACK_VAR_SAVE_VERSION（0x50EF）: 最後にレポートを書いた版。src/save.c の SaveWriteToFlash の頭で書く（patches/0265）
 * - HACK_VAR_SAVE_VERSION_FIRST（0x50EE）: はじめからを始めた版。NewGameWipeNewSaveData（拡張のセーブを 0 で消す）の直後に書く。
 *   拡張変数（0x5000〜）は拡張のセーブの中なので、そのあとの ClearSav1 では消えない（なんいどと同じ）
 * - どちらも 0 は「書かれていない」＝前の版（0.35.2 より前）のセーブ。前のセーブの始めた版は分からないので 0 のまま
 */

//今の版（★ 2026-10-12 ネット交換のプロファイルの番号・src/hack_network_trade.c。版の見出しを読む C を増やさない＝tools/build/verify_build_speed.py）
u16 HackSaveVersion_Current(void)
{
	return HACK_SAVE_VERSION;
}

/*
 * ★ 2026-10-15 ユーザー確定「今の ROM より新しい版でレポートしたセーブを開いたら知らせて、つづきから を使えないようにする」
 *   （版を下げて遊ぼうとした人のセーブを壊さないため）。タイトルのあとのメニュー（src/bw_main_menu.c の BwMainSetup）が見る。
 *   saved はレポートの版（予約変数 0x50EF）。版の数は 更新 * 256 + バグ修正（0.45.0 は 0x2D00）なので、数の大小が版の新旧と同じ。
 *   0（版の無い 0.35.2 より前・空）は「古すぎる」側。どちらもレポートは書き換えない（つづきから を出さないだけ）。
 *   ※ この判定は入れた版より後の ROM でしか効かない（配布済みの古い版の ROM は新しいセーブを止めない）。
 *   ホストの検算 tools/build/verify_save_version_guard.py がこの 2 つの関数を clang でそのまま組んで版の大小を確かめる（引数と定数だけ・ほかを呼ばない）
 */
bool8 HackSaveVersion_IsTooOld(u16 saved)
{
	return saved < HACK_SAVE_MIN_VERSION;
}

bool8 HackSaveVersion_IsTooNew(u16 saved)
{
	return saved > HACK_SAVE_VERSION;
}

void HackSaveVersion_OnSave(void)
{
	VarSet(HACK_VAR_SAVE_VERSION, HACK_SAVE_VERSION);
}

void HackRamCheat_Resync(void);   //[hack] patches/0682 src/hack_ram_cheat.c
void HackContest_Reset(void);       //[hack] patches/0985 src/hack_contest.c

void HackSaveVersion_NewGame(void)
{
	VarSet(HACK_VAR_SAVE_VERSION_FIRST, HACK_SAVE_VERSION);
	VarSet(HACK_VAR_SAVE_VERSION, HACK_SAVE_VERSION);
	HackRamCheat_Resync();   //[hack] patches/0682 はじめから: お金・コインの影を今の値から作る
}

/*
 * ★ 2026-10-11 ユーザー「ポケセンにテレポート（パッチ更新のたびに）」「パッチによってフラグ リセット」→（確認）「最後に使ったポケセン」
 *   「版ごとの表をファイルで持つ」:
 *   つづきから を選んだとき（src/bw_main_menu.c）、レポートの版（0x50EF）が今のビルドより古ければ、
 *   (1) tools/build/version_resets.json の「セーブの版より新しい行」を全部当て（フラグを下ろす／立てる・変数に値・生成物 hack_save_version.h の表）、
 *   (2) 続きからを「最後に使ったポケセン（gSaveBlock1->lastHealLocation＝全滅の戻り先と同じ・ホウエンなどは src/region_fly.c が書く）」への
 *       ワープにする（素の continueGameWarp の仕組み・HoennTower_SaveGame と同じ）。地図に入り直すので一時フラグ・一時変数も 0 に戻る。
 *   版の変数はレポートを書くまで古いままなので、書かずにまた つづきから をすれば同じことがもう一度起きる（フラグの戻しは何度当てても同じ）。
 *   版が 0（0.35.2 より前）のレポートは つづきから に出ないのでここには来ない。
 *   ★ 2026-10-15 今の版より新しいレポートも つづきから に出ない（HackSaveVersion_IsTooNew）。来ても下の saved >= HACK_SAVE_VERSION で何もしない。
 */
#define SetContinueGameWarp ((void (*)(s8, s8, s8, s8, s8)) (0x08054FB8 | 1))   //素（src/hoenn_tower.c と同じ）

bool8 HackChallengeC_VersionHome(void); //[hack] patches/0550 縛り C（src/hack_challenge_c.c）

/*
 * ★ 2026-10-28 ユーザー決定「版をまたぐフラグの直しはロード時だけ」（段 1・docs/research/2026-10-28_version-flag-fix-policy.md）:
 *   表の行に when（旗が立っている／立っていない・変数の比較・全部 AND）と call（HackVersionFix_ で始まる C の関数）を足せる。
 *   当て方: 行ごとに when を 1 回だけ確かめ（その時点の旗・変数で）、真なら flags_clear → flags_set → vars → call の順に当てる。
 *   どの直しも何度当てても同じ結果にする（版の変数はレポートを書くまで古いまま＝書かずにもう一度 つづきから すると同じ行がまた当たる）。
 *   call の関数は void(void)・自分の中でも冪等に書く。表の生成は tools/build/build_save_version.py。
 */
static bool8 HackVersionCondHolds(u16 cond)
{
	u32 i;

	for (i = 0; i < HACK_VERSION_COND_COUNT; ++i)
	{
		const struct HackVersionCond* t = &sHackVersionConds[i];
		bool8 ok;

		if (t->cond != cond)
			continue;
		if (t->kind == 0)
			ok = FlagGet(t->id) != 0;
		else if (t->kind == 1)
			ok = FlagGet(t->id) == 0;
		else
		{
			u16 v = VarGet(t->id);

			switch (t->op)
			{
				case 0: ok = v == t->value; break;
				case 1: ok = v != t->value; break;
				case 2: ok = v >= t->value; break;
				case 3: ok = v < t->value; break;
				case 4: ok = v > t->value; break;
				default: ok = v <= t->value; break;
			}
		}
		if (!ok)
			return FALSE;
	}
	return TRUE;
}

//セーブの版 saved より新しい行を古い順に当てる（ホストの検算 tools/build/verify_save_version_when.py がこの関数をそのまま組む）
void HackSaveVersion_ApplyRows(u16 saved)
{
	u16 lastRow = 0;
	bool8 lastOk = TRUE;
	u32 i;

	for (i = 0; i < HACK_VERSION_RESET_COUNT; ++i)
	{
		const struct HackVersionReset* r = &sHackVersionResets[i];

		if (r->version <= saved)
			continue;
		if (r->cond != 0)
		{
			if (r->row != lastRow)
			{
				lastRow = r->row;
				lastOk = HackVersionCondHolds(r->cond);
			}
			if (!lastOk)
				continue;
		}
		if (r->kind == 0)
			FlagClear(r->id);
		else if (r->kind == 1)
			FlagSet(r->id);
		else if (r->kind == 2)
			VarSet(r->id, r->value);
		else if (r->kind == 3)
			sHackVersionFuncs[r->id]();
	}
}

/*
 * ★ 2026-10-28 洗い出し C（D:/agents/flag-audit/FLAG_AUDIT.md）: 施設の挑戦中にレポートしたセーブを、版が上がってから つづきから すると、
 *   ポケセンへ飛ぶだけで施設の旗 0x930・状態・（ピラミッドの）バトルバッグ・連勝が残る。ここで 緊急脱出の施設の後始末（HoennFacility_EscapeCleanup）と同じことをする。
 *   表の 1 回きりの行ではなく ここ（版が上がる つづきから のたび）で呼ぶ理由: どの版からでも起きうる（0.59.2 以降の差し替えレポート）ので版を決められない。
 *   何度呼んでも同じ（後始末のあとは状態 0・旗なしなので次は何もしない）。ガードは src/hoenn_tower.c の HackVersionFix_GiveUpFacility。
 */
void HackVersionFix_GiveUpFacility(void);   //src/hoenn_tower.c
void HackVersionFix_GibleTenp(void);        //src/hack_gible_egg.c（FIX-gible-tenp-old-saves・patches/0938・版が上がるたび・冪等）

void HackSaveVersion_OnContinue(void)
{
	u16 saved = VarGet(HACK_VAR_SAVE_VERSION);
	const struct WarpData* heal = &gSaveBlock1->lastHealLocation;

	HackContest_Reset();     //[hack] patches/0985 つづきから: コンテストの状態の語を NULL に（ソフトリセットで EWRAM が残っても IsContest は 0）
	HackRamCheat_Resync();   //[hack] patches/0682 つづきから: 読み込んだ値から影を作り直す（保存された値の改ざんはセーブ側の検出の仕事）
	HackWildChain_Reset();   //[hack] patches/0353 野生のむれの旗（セーブしない特別な旗）が前の遊びから残っていても消す
	if (saved == 0 || saved >= HACK_SAVE_VERSION)
		return;
	HackSaveVersion_ApplyRows(saved);
	HackVersionFix_GibleTenp();        //配布済みのクチバのフカマル系（はずれ）を Tenp の当たりに（色違いは保つ・もう当たりの子・他人の子・野生は触らない）
	HackVersionFix_GiveUpFacility();   //施設の挑戦中のレポートは 挑戦をやめた扱い（版が上がるたび・上の ★ 2026-10-28 を参照）
	if (HackChallengeC_VersionHome()) //[hack] patches/0550 ポケセンの縛りが ON のときは 最後のポケセンではなく マサラタウンの家へ（全回復）
	{
		SetContinueGameWarp(4, 0, -1, 8, 5);   //HACK_CHC_HOME_GROUP／NUM／X／Y（src/hack_challenge_c.h・素の全滅の戻り先と同じ）
		SetContinueGameWarpStatus();
		return;
	}
	if (heal->mapGroup == 0 && heal->mapNum == 0 && heal->x == 0 && heal->y == 0)
		return;   //回復地点がまだ無い（起きないはず）: その場で続ける
	SetContinueGameWarp(heal->mapGroup, heal->mapNum, -1, heal->x, heal->y);
	SetContinueGameWarpStatus();
}
