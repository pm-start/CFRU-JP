#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/main.h"
#include "../include/money.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/constants/hold_effects.h"
#include "hack_johto.h"   //[hack] patches/0986
#include "../include/constants/items.h"
#include "../include/new/battle_util.h"
#include "../include/new/item.h"
#include "../include/new/wild_encounter.h"
#include "hack_challenge.h"
#include "hack_flag_fast.h" //[hack] 2026-11-03 perf-region-2 毎コマの旗の読み
#include "hack_challenge_b.h" //[hack] patches/0540 しばり B
#include "hack_challenge_c.h" //縛り C（0.50.3・patches/0550）
#include "hack_fuuin.h" //縛り ふういんルーレット（#123・patches/0560）
#include "hack_challenge_luxray.h" //縛り ぜんぶレントラー（patches/0609）
#include "hack_challenge_f.h" //縛り F 6 本（さかさ・ボーナス・めかくし・いっぱつ・ローテ・ガチャ・0.63.0）
#include "hack_difficulty.h" //なんいど しゃかりき: 回復の道具は キズぐすりだけ（patches/0616）
#include "hack_mon_id.h"
#include "map_region.h"
#include "region_fly.h"

/*
    しばり（#118・2026-10-16・docs/research/2026-10-16_settings-folders-challenges.md）。せっていの「しばり」フォルダの 5 つ（既定は全部 OFF）。

      アイテムきんし   戦闘: ボール以外の どうぐは つかえない（item.c の表 → HackChallenge_ItemMenuUseInBattle・トレーナー戦は IsBagDisabled）
                       フィールド: かいふくの どうぐ（battleUsage 1）は つかえない（HackChallenge_ItemMenuUseOutOfBattle）。ポケセンの回復は つかえる
      もちものきんし   自分のポケモンの もちものは戦闘で こうかなし（battle_util.c の GetBankItemEffect）。姿を変えるもの
                       （メガストーン・Z クリスタル・ゲンシの玉・プレート・ドライブ・メモリ・あいいろ／べにいろの玉・こんごうだま・さびた剣／盾）は のぞく
      ポケセンりょうきん   ポケセンの 1 階に入るたび 100 円（HackChallenge_FieldTick → 台本 EventScript_ChallengeCenterEntry）
      PC りょうきん    パソコン（[MB_PC]）・ボックスリンク（EventScript_BoxLink）を使うたび 500 円（台本 HackScr_Challenge_PcGate）
      ポケセンの ？？？  ポケセンに入るたび ？？？（メスのレントラー）との戦い。払えないとき（罰金）は その ON/OFF に関係なく同じ戦い

    ？？？との戦い（HackChallenge_StartLentrantBattle）: 旗 0x904（CFRU の FLAG_NO_CATCHING_AND_RUNNING＝ボールは はじかれる・にげる／R／けむりだま／にげあし などは
    AreAllKindsOfRunningPrevented で止まる）＋ 印 HACK_FLAG_CH_BATTLE。ひんしにならない（atk0C_datahpupdate の頭でダメージ 0・atk19_tryfaintmon の頭で HP 0 を 1 に戻す）。
    戦いの終わりは かならず「負け」（HackChallenge_CB2_EndBattle が gBattleOutcome を B_OUTCOME_LOST に）＝ほえる・ともえなげ・ワープ・どんな終わり方でも ぜんめつ。
    ぜんめつで戻されたときは「ポケセンに入った」と数えない（HACK_FLAG_CH_WHITEOUT）＝りょうきんも ？？？も 無し（無限ループにしない）。
*/

extern const u8 gText_Challenge_Name[];
extern const u8 gText_Challenge_OtName[];
extern const u8 EventScript_ChallengeCenterEntry[];
extern const u8 EventScript_ChallengeFinale[];
u8 GetAllBadgeCount(void);   //src/badge_count.c（カントー・ホウエン・ジョウト・シンオウの合計）

void CB2_EndScriptedWildBattle_2(void);

/* ---- 設定（旗）---- */

bool8 HackChallenge_NoItems(void)
{
	return FlagGet(HACK_FLAG_CH_NOITEMS) != 0;
}

bool8 HackChallenge_NoHeld(void)
{
	return FlagGet(HACK_FLAG_CH_NOHELD) != 0;
}

/* ---- ポケモンセンター 1 階の見分け ---- */

//カントー・ななしまの 1 階（素の map_groups.json の PokemonCenter_1F・群 5〜37・19 か所）。ホウエン・ジョウト・シンオウは region_fly.c の表（gRegionPokeCenters）
static const u8 sKantoCenters[][2] =
{
	{5, 4}, {6, 5}, {7, 3}, {8, 0}, {9, 1}, {10, 12}, {11, 5}, {12, 5}, {13, 0}, {14, 6},
	{16, 0}, {21, 0}, {31, 3}, {32, 0}, {33, 2}, {34, 1}, {35, 1}, {36, 0}, {37, 0},
};

static bool8 InPokemonCenter1F(void)
{
	u8 g = (u8) gSaveBlock1->location.mapGroup;
	u8 n = (u8) gSaveBlock1->location.mapNum;
	u32 i;

	if (g < HOENN_MAP_GROUP_FIRST)
	{
		for (i = 0; i < NELEMS(sKantoCenters); ++i)
		{
			if (sKantoCenters[i][0] == g && sKantoCenters[i][1] == n)
				return TRUE;
		}
		return FALSE;
	}
	for (i = 0; i < gRegionPokeCenterCount[0]; ++i)
	{
		if (gRegionPokeCenters[i].pcGroup == g && gRegionPokeCenters[i].pcNum == n)
			return TRUE;
	}
	return FALSE;
}

/* ---- 料金 ---- */

static bool8 PayFee(u32 fee)
{
	if (!IsEnoughMoney(&gSaveBlock1->money, fee))
		return FALSE;
	RemoveMoney(&gSaveBlock1->money, fee);
	return TRUE;
}

//callasm（台本 EventScript_ChallengeCenterEntry）: LASTRESULT 0 何もない / 1 りょうきんを払った / 2 払った＋スイッチ / 3 払えない（罰金） / 4 スイッチだけ
void HackChallenge_CenterCheck(void)
{
	bool8 lentrant = FlagGet(HACK_FLAG_CH_LENTRANT) != 0;

	if (FlagGet(HACK_FLAG_CH_CENTERFEE))
	{
		if (PayFee(HACK_CH_CENTER_FEE))
			gSpecialVar_LastResult = lentrant ? 2 : 1;
		else
			gSpecialVar_LastResult = 3;   //払えない: スイッチに関係なく ？？？
	}
	else
		gSpecialVar_LastResult = lentrant ? 4 : 0;
}

//callasm（台本 HackScr_Challenge_PcGate）: LASTRESULT 0 しばりなし 1 払った 2 払えない
void HackChallenge_PcCheck(void)
{
	if (!FlagGet(HACK_FLAG_CH_PCFEE))
		gSpecialVar_LastResult = 0;
	else if (PayFee(HACK_CH_PC_FEE))
		gSpecialVar_LastResult = 1;
	else
		gSpecialVar_LastResult = 2;
}

/* ---- 最後のバッジ（しばりを ずっと ON にしたまま・2026-10-16 ユーザー）---- */

//この版で取れるバッジの数: ジョウトを外した版（HACK_OMIT_JOHTO・配る版）は カントー・ホウエン・シンオウの 24 個・ジョウトがある版は 32 個
//★ patches/0986（FEAT-johto-j0）: コンパイル時の HACK_OMIT_JOHTO を実行時の判定に（ジョウトが開くまで 24）
#define CH_BADGES_TOTAL (HackJohto_Open() ? 32 : 24)

static bool8 AnyChallengeOn(void)
{
	return FlagGet(HACK_FLAG_CH_NOITEMS) || FlagGet(HACK_FLAG_CH_NOHELD) || FlagGet(HACK_FLAG_CH_CENTERFEE)
		|| FlagGet(HACK_FLAG_CH_PCFEE) || FlagGet(HACK_FLAG_CH_LENTRANT)
		|| FlagGet(HACK_FLAG_CH_DEATH) || FlagGet(HACK_FLAG_CH_FIRSTONLY) || FlagGet(HACK_FLAG_CH_NICKNAME)   //縛り A（patches/0530）
		|| HackChallengeB_AnyOn()   //[hack] patches/0540 しばり B（src/hack_challenge_b.c）
		|| HackChallengeC_AnyOn()   //縛り C（patches/0550）
		|| HackFuuin_On()   //ふういんルーレット（patches/0560）
		|| HackChallengeLuxray_On()   //ぜんぶレントラー（patches/0609）
		|| HackChallengeF_On();   //縛り F 6 本（0.63.0）
}

//ジムで最後のバッジを受け取った直後だけ（同じ地図の中でバッジの数が合計に届いたとき・16 コマごとに見る）。地図に入ったとき すでに全部そろっていれば出ない。
//入ったときの数はタスクに置く（一時変数 0x400E はトクサネの白い岩の回数とバトルアリーナのターンが使うので、ここでは触らない）。
//地図が変わったら 0 に戻す。メニューでタスクが消えても、消えた回は「入ったばかり」として数え、メッセージは出さない。
static void Task_ChallengeBadgeSeen(u8 taskId)
{
	(void) taskId;
}

static void DropBadgeSeen(void)
{
	u8 id = FindTaskIdByFunc(Task_ChallengeBadgeSeen);

	if (id != 0xFF)
		DestroyTask(id);
}

static u8 BadgeSeenTask(void)
{
	u8 id = FindTaskIdByFunc(Task_ChallengeBadgeSeen);
	u8 group = (u8) gSaveBlock1->location.mapGroup;
	u8 num = (u8) gSaveBlock1->location.mapNum;

	if (id == 0xFF)
	{
		id = CreateTask(Task_ChallengeBadgeSeen, 0xFF);
		if (id == 0xFF)
			return 0xFF;
		gTasks[id].data[0] = 0;
		gTasks[id].data[1] = group;
		gTasks[id].data[2] = num;
		return id;
	}
	if ((u8) gTasks[id].data[1] != group || (u8) gTasks[id].data[2] != num)
	{
		gTasks[id].data[0] = 0;
		gTasks[id].data[1] = group;
		gTasks[id].data[2] = num;
	}
	return id;
}

static void FinaleTick(void)
{
	u16 seen;
	u8 count;
	u8 id;

	if ((gMain.vblankCounter2 & 0xF) != 0)
		return;
	if (FlagGet(HACK_FLAG_CH_FINALE) || FlagGet(HACK_FLAG_CH_BROKEN) || !AnyChallengeOn())
	{
		DropBadgeSeen();
		return;
	}
	id = BadgeSeenTask();
	if (id == 0xFF)
		return;
	count = GetAllBadgeCount();
	seen = (u16) gTasks[id].data[0];
	if (seen == 0)
	{
		gTasks[id].data[0] = count + 1;
		return;
	}
	if (count >= CH_BADGES_TOTAL && (u16) (seen - 1) < CH_BADGES_TOTAL)
	{
		if (gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld)
			return;   //台本が終わるまで待つ（次の周期でまた見る）
		FlagSet(HACK_FLAG_CH_FINALE);
		ScriptContext1_SetupScript(EventScript_ChallengeFinale);
		return;
	}
	if (count + 1 != seen)
		gTasks[id].data[0] = count + 1;
}

//全部の しばりの数: しばり 5＋縛り A 3（HackChallenge_CountAllOn の sFlags の 8 本）＋B＋C＋D＋E＋F
#define HACK_CH_ALL_COUNT (8 + HACK_CHB_COUNT + HACK_CHC_COUNT + HACK_CHD_COUNT + HACK_CHE_COUNT + HACK_CHF_COUNT)

//いま ON の しばりの数（全部の縛り）。縛り F の「しばりボーナス」自身も 1 本として数える。HackChallengeF_BonusCount は ここから ボーナス自身を引く
u8 HackChallenge_CountAllOn(void)
{
	static const u16 sFlags[] = {HACK_FLAG_CH_NOITEMS, HACK_FLAG_CH_NOHELD, HACK_FLAG_CH_CENTERFEE, HACK_FLAG_CH_PCFEE, HACK_FLAG_CH_LENTRANT,
		HACK_FLAG_CH_DEATH, HACK_FLAG_CH_FIRSTONLY, HACK_FLAG_CH_NICKNAME};   //縛り A（patches/0530）
	u8 n = 0;
	u32 i;

	for (i = 0; i < NELEMS(sFlags); ++i)
	{
		if (FlagGet(sFlags[i]))
			++n;
	}
	n += HackChallengeB_CountOn();   //[hack] patches/0540 しばり B（5 つ）も数える
	n += HackChallengeC_CountOn();   //縛り C（patches/0550）
	n += HackFuuin_On() ? HACK_CHD_COUNT : 0;   //ふういんルーレット（patches/0560）
	n += HackChallengeLuxray_On() ? HACK_CHE_COUNT : 0;   //ぜんぶレントラー（patches/0609）
	n += HackChallengeF_CountOn();   //縛り F 6 本（0.63.0）
	return n;
}

//callasm（台本 EventScript_ChallengeFinale）: gStringVar1 に「いま ON の しばりの数」・LASTRESULT = 全部 ON なら 1
//（最後のバッジの時点で ON の数＝途中で OFF にした人には出さないので「ずっと ON だった数」と同じ）
void HackChallenge_FinaleCount(void)
{
	u8 n = HackChallenge_CountAllOn();

	ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 2);   //10 を超えるので 2 けた
	gSpecialVar_LastResult = (n == HACK_CH_ALL_COUNT) ? 1 : 0;   //全部 ON（しばり 5＋A 3＋B 5＋C 5＋D 1＋E 1＋F 6＝26・シャカキリ〈せっていのボタン〉は これを全部 ON にする）
}

/* ---- 縛り A（0.50.3・patches/0530）: ひんしでお別れ・最初の 1 匹だけ・ニックネームひっす ---- */

extern const u8 EventScript_ChallengeFarewell[];
extern const u8 EventScript_ChallengeGameOver[];
extern const u8 EventScript_ChallengeNickname[];
bool8 HoennPike_InWildRoom(void);   //src/hoenn_pike.c

static bool8 FieldReady(void)
{
	return !(gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld);
}

//フロンティアなどの施設（借りた手持ち）の中は縛り A の対象外
static bool8 InFacility(void)
{
	return FlagGet(FLAG_BATTLE_FACILITY) != 0;
}

//ひんしでお別れ: 手持ちのひんし（タマゴを除く）を手持ちから外す（もちものは バッグへ）。
//生きている（たたかえる）子が 1 匹もいないときは外さず ゲームオーバー（外すと 手持ちが空になる＝戻るのでロードし直すだけ）。戻り値: 外した数
static u8 BuryPartyFainted(void)
{
	u8 i, alive = 0, fainted = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);

		if (species == SPECIES_NONE || species == SPECIES_EGG)
			continue;
		if (GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) == 0)
			++fainted;
		else
			++alive;
	}
	if (fainted == 0)
		return 0;
	if (alive == 0)
	{
		FlagSet(HACK_FLAG_CH_GAMEOVER);   //全員ひんし: ゲームオーバー（台本が ソフトリセット）
		return 0;
	}
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL);
		u16 item;

		if (species == SPECIES_NONE || species == SPECIES_EGG || GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) != 0)
			continue;
		item = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, NULL);
		if (item != ITEM_NONE)
			AddBagItem(item, 1);   //入らなければ もちものは一緒にお別れ
		ZeroMonData(&gPlayerParty[i]);
	}
	CompactPartySlots();
	CalculatePlayerPartyCount();
	FlagSet(HACK_FLAG_CH_DEATH_MSG);
	return fainted;
}

//野生の戦いのあと: 野外へ戻す（素の CB2_EndWildBattle と同じ ScriptContext2_Disable ＋ CB2_ReturnToField）
static void CB2_ChallengeGameOverWild(void)
{
	ScriptContext2_Disable();
	CB2_ReturnToField();
}

//end_battle.c の EndOfBattleThings の最後（BringBackTheDead のあと）。通信・フロンティア・チュートリアルは除く
void HackChallenge_BuryFainted(void)
{
	if (!FlagGet(HACK_FLAG_CH_DEATH) || InFacility()
	|| (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_TOWER | BATTLE_TYPE_OLD_MAN | BATTLE_TYPE_POKE_DUDE)))
		return;
	BuryPartyFainted();
	if (FlagGet(HACK_FLAG_CH_GAMEOVER))
	{	//全滅のぜんめつ（ハーフマネー・ポケセン）を通らず 野外へ戻して ゲームオーバーの台本。トレーナー戦は 台本を止めたまま（待っている台本の続き）にしないよう 続きを走らせる
		if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
			gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
		else
			gMain.savedCallback = CB2_ChallengeGameOverWild;
	}
}

//callasm（EventScript_ChallengeGameOver の最後）: ソフトリセット（RESET_ALL）。セーブには何も書かないので 最後のレポートからになる
//（vanilla の SoftReset の写し: IME 0・スタックを IWRAM の後ろへ・SWI 1（RegisterRamReset）・SWI 0）
void HackChallenge_GameOverReset(void)
{
	*(vu16*) 0x4000208 = 0;     //REG_IME
	*(vu16*) 0x40000C6 = 0;     //DMA1 ・ 2（サウンド）を止める
	*(vu16*) 0x40000D2 = 0;
	asm volatile(
		"mov r1, #3\n\t"          //r1 = 0x03007FE0（ldr = だと リテラルプールが遠い）
		"lsl r1, r1, #24\n\t"
		"mov r2, #0xFF\n\t"
		"lsl r2, r2, #7\n\t"
		"add r1, r1, r2\n\t"
		"add r1, #0x60\n\t"
		"mov sp, r1\n\t"
		"mov r0, #0xFF\n\t"
		"swi 0x01\n\t"
		"swi 0x00\n\t"
		::: "r0", "r1", "r2", "memory");
}

//最初の 1 匹だけ: SaveBlock2 の filler_B20 の中の一覧（0xA5・個数 2 バイト・(群,番号)…）。一覧が空・古いセーブ（0 のまま）は「まだ誰も出会っていない」
static u8* FirstList(void)
{
	return gSaveBlock2->filler_B20 + HACK_CH_FIRST_OFFSET;
}

static u16 FirstCount(void)
{
	u8* list = FirstList();

	u16 n = list[1] | (list[2] << 8);

	if (list[0] != HACK_CH_FIRST_MAGIC || n > HACK_CH_FIRST_MAX)
		return 0;
	return n;
}

static bool8 FirstMapUsed(u8 group, u8 num)
{
	u8* list = FirstList();
	u16 n = FirstCount(), i;

	for (i = 0; i < n; ++i)
	{
		if (list[3 + i * 2] == group && list[4 + i * 2] == num)
			return TRUE;
	}
	return FALSE;
}

//覚える（満杯なら覚えない＝それ以上の地図は縛られない）
static void FirstMapMark(u8 group, u8 num)
{
	u8* list = FirstList();
	u16 n = FirstCount();

	if (n >= HACK_CH_FIRST_MAX)
		return;
	if (list[0] != HACK_CH_FIRST_MAGIC)
		list[0] = HACK_CH_FIRST_MAGIC;
	list[3 + n * 2] = group;
	list[4 + n * 2] = num;
	++n;
	list[1] = n & 0xFF;
	list[2] = n >> 8;
}

//wild_encounter.c の DoStandardWildBattle（ふつうの野生: 草むら・水・釣り・洞窟など）の終わり。
//使い切りの地図なら つかまえられない（CFRU の FLAG_NO_CATCHING＝戦闘の終わりで下りる）。まだなら この戦いで使い切る。色違いは例外（数えない・捕まえられる）
void HackChallenge_OnWildBattleStart(void)
{
	u8 group, num;

	if (!FlagGet(HACK_FLAG_CH_FIRSTONLY) || InFacility() || FlagGet(HACK_FLAG_CH_BATTLE) || HoennPike_InWildRoom())
		return;
	if (IsMonShiny(&gEnemyParty[0]))
		return;
	group = (u8) gSaveBlock1->location.mapGroup;
	num = (u8) gSaveBlock1->location.mapNum;
	if (FirstMapUsed(group, num))
		FlagSet(FLAG_NO_CATCHING);
	else
		FirstMapMark(group, num);
}

//ニックネームなし（種族名のまま）か。タマゴは対象外
static bool8 MonNeedsNickname(struct Pokemon* mon)
{
	u8 nick[POKEMON_NAME_LENGTH + 1];
	u8 name[POKEMON_NAME_LENGTH + 1];
	u16 species = GetMonData(mon, MON_DATA_SPECIES2, NULL);

	if (species == SPECIES_NONE || species == SPECIES_EGG)
		return FALSE;
	GetMonData(mon, MON_DATA_NICKNAME, nick);
	GetSpeciesName(name, species);
	return StringCompare(nick, name) == 0;
}

//callasm（EventScript_ChallengeNickname）: ニックネームなしの手持ちを 1 匹探し Var8004 に番号・Var8003 = 0（手持ち）・gStringVar1 に名前。LASTRESULT 1 見つけた / 0 無い
void HackChallenge_NickPrepare(void)
{
	u8 i;

	gSpecialVar_LastResult = 0;
	if (!FlagGet(HACK_FLAG_CH_NICKNAME))
		return;
	for (i = 0; i < gPlayerPartyCount && i < PARTY_SIZE; ++i)
	{
		if (MonNeedsNickname(&gPlayerParty[i]))
		{
			VarSet(0x8003, 0);
			VarSet(0x8004, i);
			GetMonData(&gPlayerParty[i], MON_DATA_NICKNAME, gStringVar1);
			gSpecialVar_LastResult = 1;
			return;
		}
	}
}

//フィールドの毎コマ（HackChallenge_FieldTick の頭）。ゲームオーバー（毎コマ）・ほかは 16 コマごとに 1 つずつ
static void ChallengeATick(void)
{
	if (HackXFlagGet(HACK_FLAG_CH_GAMEOVER))   //★ 2026-11-03 毎コマなので FlagGet を使わず直に読む（hack_flag_fast.h）
	{
		if (FieldReady())
			ScriptContext1_SetupScript(EventScript_ChallengeGameOver);
		return;
	}
	if ((gMain.vblankCounter2 & 0xF) != 4 || InFacility())
		return;
	if (FlagGet(HACK_FLAG_CH_DEATH) && FieldReady())
	{
		BuryPartyFainted();   //フィールドで ひんしになったまま（どく など）の子もお別れ
		if (FlagGet(HACK_FLAG_CH_GAMEOVER))
			return;           //次のコマで ゲームオーバー
		if (FlagGet(HACK_FLAG_CH_DEATH_MSG))
		{
			FlagClear(HACK_FLAG_CH_DEATH_MSG);
			ScriptContext1_SetupScript(EventScript_ChallengeFarewell);
			return;
		}
	}
	else if (FlagGet(HACK_FLAG_CH_DEATH_MSG) && !FlagGet(HACK_FLAG_CH_DEATH))
		FlagClear(HACK_FLAG_CH_DEATH_MSG);
	if (FlagGet(HACK_FLAG_CH_NICKNAME) && FieldReady())
	{
		u8 slot = (gMain.vblankCounter2 >> 4) % PARTY_SIZE;   //1 回に 1 匹（GetMonData は暗号を解くので重い・6.80）

		if (slot < gPlayerPartyCount && MonNeedsNickname(&gPlayerParty[slot]))
			ScriptContext1_SetupScript(EventScript_ChallengeNickname);
	}
}

/* ---- ポケセンに入った直後 ---- */

//フィールドの毎コマ（overworld_speedup.c）。ポケセンの 1 階の地図に入ったあと 1 回だけ台本を始める。入ったかの印は一時変数（地図を読み込むたびに 0 に戻る・
//つづきからで中にいるときは立ったまま＝数えない）。ぜんめつで戻されたときは数えない
void HackChallenge_FieldTick(void)
{
	FinaleTick();
	HackChallengeF_FieldTick();   //縛り F: しばりボーナスのバッジのごほうび（0.63.0）
	ChallengeATick();   //縛り A（patches/0530）
	HackChallengeC_FieldTick();   //縛り C（patches/0550）: モノタイプの記録・安全装置・？？？のあとのメッセージ
	HackFuuin_FieldTick();   //ふういんルーレット（patches/0560）: レベルアップのルーレット・封印された子の戻し
	if (!HackXFlagGet(HACK_FLAG_CH_CENTERFEE) && !HackXFlagGet(HACK_FLAG_CH_LENTRANT))   //★ 2026-11-03 毎コマ（しばりが OFF のふだんもここを通る）なので直に読む
	{
		if (HackXFlagGet(HACK_FLAG_CH_WHITEOUT))
			FlagClear(HACK_FLAG_CH_WHITEOUT);   //しばりを OFF にしたあとに残さない
		return;
	}
	if (!InPokemonCenter1F())
	{
		if (FlagGet(HACK_FLAG_CH_WHITEOUT))
			FlagClear(HACK_FLAG_CH_WHITEOUT);   //ぜんめつの戻り先がポケセンでなかった（マサラの家など）
		return;
	}
	if (VarGet(HACK_CH_TEMP_VAR) != 0)
		return;
	if (gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld)
		return;
	VarSet(HACK_CH_TEMP_VAR, 1);
	if (FlagGet(HACK_FLAG_CH_WHITEOUT))
	{
		FlagClear(HACK_FLAG_CH_WHITEOUT);   //詰み防止: ぜんめつで戻されたのは「入った」と数えない
		return;
	}
	ScriptContext1_SetupScript(EventScript_ChallengeCenterEntry);
}

//end_battle.c の HandleEndTurn_BattleLost の頭（通信・フロンティアは除く）。ぜんめつ → ポケセンへの戻りを「入った」と数えないための印
void HackChallenge_OnBattleLost(void)
{
	if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER))
		return;
	if (FlagGet(HACK_FLAG_CH_CENTERFEE) || FlagGet(HACK_FLAG_CH_LENTRANT))
		FlagSet(HACK_FLAG_CH_WHITEOUT);
}

/* ---- ？？？（メスのレントラー）---- */

static void CopyName(u8* dst, const u8* src, u8 size)   //size 文字（0xFF で終わり・残りも 0xFF）
{
	u8 i;
	bool8 ended = FALSE;

	for (i = 0; i < size; ++i)
	{
		if (!ended && src[i] != EOS)
			dst[i] = src[i];
		else
		{
			ended = TRUE;
			dst[i] = EOS;
		}
	}
}

//gEnemyParty[0] を ？？？に作り直す。メス（性格値の下位バイトが性別の境より下）・Lv200・個体値 31・親は liru・表 ID 669（5 けた 00669）・
//色違いと Moémon は自身の ID だけで決める（性格値は触らない・6.5）。名前は ？？？
static void BuildLentrant(struct Pokemon* mon)
{
	u8 name[POKEMON_NAME_LENGTH + 1];
	u8 ot[PLAYER_NAME_LENGTH + 1];
	u8 ratio = gBaseStats[SPECIES_LUXRAY].genderRatio;
	u32 personality = 0x12AB5C00 | (u32) (ratio >> 1);   //下位バイト < 境 ＝ メス
	u16 hp;

	CreateMon(mon, SPECIES_LUXRAY, HACK_CH_LENTRANT_LEVEL, 31, TRUE, personality, OT_ID_PRESET, HACK_CH_LENTRANT_DEX_ID);
	CopyName(name, gText_Challenge_Name, POKEMON_NAME_LENGTH + 1);
	SetMonData(mon, MON_DATA_NICKNAME, name);
	CopyName(ot, gText_Challenge_OtName, PLAYER_NAME_LENGTH + 1);
	SetMonData(mon, MON_DATA_OT_NAME, ot);
	{
		u16 ice = ITEM_MOMO_ICE;   //ももアイス（HP・5 つの能力 2 倍・状態異常と能力ダウンを受けない・毎ターン回復・うばわれない）を持たせる（0.50.3 ユーザー・patches/0550）

		SetMonData(mon, MON_DATA_HELD_ITEM, &ice);
	}
	HackMonId_MakeShinyMoemon((struct BoxPokemon*) mon);
	CalculateMonStats(mon);
	hp = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	SetMonData(mon, MON_DATA_HP, &hp);
}

//台本 setwildbattle のあとに呼ぶ版（必要なら）。いまは StartLentrantBattle が自分で作る
void HackChallenge_FixLentrant(void)
{
	BuildLentrant(&gEnemyParty[0]);
}

static void HackChallenge_CB2_EndBattle(void)
{
	FlagClear(HACK_FLAG_CH_BATTLE);
	FlagClear(FLAG_NO_CATCHING_AND_RUNNING);
	gBattleOutcome = B_OUTCOME_LOST;           //勝ち・にげ・つかまえた・テレポート…どんな終わり方でも「負け」＝ぜんめつ（にげ道を全部ふさぐ）
	if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
		FlagSet(HACK_FLAG_CH_WHITEOUT);        //戻された先を「入った」と数えない
	CB2_EndScriptedWildBattle_2();             //素: 負けならぜんめつ（CB2_WhiteOut）
}

//callasm: ？？？との戦いを始める（台本は直後に waitstate）
void HackChallenge_StartLentrantBattle(void)
{
	ZeroEnemyPartyMons();
	BuildLentrant(&gEnemyParty[0]);
	FlagSet(FLAG_NO_CATCHING_AND_RUNNING);     //ボールは はじかれる・にげられない（CFRU の旗・戦いの終わりで下りる）
	FlagSet(HACK_FLAG_CH_BATTLE);
	sp138_StartLegendaryBattle();
	gMain.savedCallback = HackChallenge_CB2_EndBattle;
}

bool8 HackChallenge_IsLentrantBattle(void)
{
	return FlagGet(HACK_FLAG_CH_BATTLE) != 0;
}

bool8 HackChallenge_IsLentrantBank(u8 bank)
{
	return FlagGet(HACK_FLAG_CH_BATTLE) && SIDE(bank) == B_SIDE_OPPONENT;
}

//atk0C_datahpupdate の頭（デバッグのむてきと同じ入口）: ？？？へのダメージ（回復は除く）は 0。通信では効かない（通信に ？？？は出ない）
void HackChallenge_AdjustDamage(u8 bank)
{
	u8 b;

	if (!FlagGet(HACK_FLAG_CH_BATTLE) || (gBattleTypeFlags & BATTLE_TYPE_LINK))
		return;
	if (gBattleMoveDamage > 0)
	{
		if (HackChallenge_IsLentrantBank(bank))
		{
			gBattleMoveDamage = 0;                    //？？？の HP は減らさない
			HackChallengeC_LentrantWrath(HACK_CHC_WRATH_TOUCH);   //縛り C: ダメージを入れた＝手持ち全員ひんし・負け（patches/0550）
		}
		else if (SIDE(bank) == B_SIDE_PLAYER && bank != gBankAttacker && HackChallenge_IsLentrantBank(gBankAttacker)
		      && !(gHitMarker & HITMARKER_NON_ATTACK_DMG))
			HackChallengeC_LentrantWrath(HACK_CHC_WRATH_HIT);     //縛り C: ？？？の攻撃が当たった（みがわりに当たっても同じ＝仮）
	}
	for (b = 0; b < gBattlersCount; ++b)               //ダメージ以外（いたみわけ など）で ？？？の HP が減ったときも
	{
		if (SIDE(b) == B_SIDE_OPPONENT && gBattleMons[b].hp < gBattleMons[b].maxHP)
			HackChallengeC_LentrantWrath(HACK_CHC_WRATH_TOUCH);
	}
}

//atk19_tryfaintmon の頭: ダメージ以外（みちづれ・ほろびのうた・いたみわけ など）で HP が 0 になっていても 1 に戻す
void HackChallenge_KeepLentrantAlive(u8 bank)
{
	if (gBattleMons[bank].hp == 0 && HackChallenge_IsLentrantBank(bank))
	{
		gBattleMons[bank].hp = 1;
		HackChallengeC_LentrantWrath(HACK_CHC_WRATH_TOUCH);   //縛り C: みちづれ・ほろびのうた など HP に触れる道も いかり（patches/0550）
	}
}

//戦闘の枠のレベル（src/bw_hpbar.c の BwDrawLevel）: ？？？だけ 9999（見かけだけ・中身は Lv200）
u16 HackChallenge_ShownLevel(struct Pokemon* mon, u16 level)
{
	if (FlagGet(HACK_FLAG_CH_BATTLE) && mon == &gEnemyParty[0])
		return HACK_CH_LENTRANT_SHOWN_LEVEL;
	return level;
}

/* ---- もちものきんし ---- */

//battle_util.c の GetBankItemEffect から: 自分の側の もちものの こうかを消すか。通信は除く（ずれるため）・マルチ／ふたりの味方は自分ではない
bool8 HackChallenge_HeldItemNullified(u8 bank, u8 effect)
{
	if (effect == 0 || SIDE(bank) != B_SIDE_PLAYER || (gBattleTypeFlags & BATTLE_TYPE_LINK) || !FlagGet(HACK_FLAG_CH_NOHELD))
		return FALSE;
	if ((gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)) && bank == GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT))
		return FALSE;
	switch (effect)   //姿を変える持ち物は例外
	{
		case ITEM_EFFECT_MEGA_STONE:
		case ITEM_EFFECT_PRIMAL_ORB:
		case ITEM_EFFECT_Z_CRYSTAL:
		case ITEM_EFFECT_PLATE:
		case ITEM_EFFECT_DRIVE:
		case ITEM_EFFECT_MEMORY:
		case ITEM_EFFECT_GRISEOUS_ORB:
		case ITEM_EFFECT_ADAMANT_ORB: //[hack] patches/0810 オリジンフォルムになる持ち物
		case ITEM_EFFECT_LUSTROUS_ORB:
		case ITEM_EFFECT_RUSTED_SWORD:
		case ITEM_EFFECT_RUSTED_SHIELD:
			return FALSE;
	}
	return TRUE;
}

/* ---- アイテムきんし（バッグの「つかう」）---- */

extern const u8 gText_Challenge_CantUseItem[];
extern const struct Item gItemData[];   //src/Tables/item_tables.c

#define VanillaItemMenu_UseInBattle ((void (*)(u8)) 0x810AE21)
#define VanillaItemMenu_UseOutOfBattle ((void (*)(u8)) 0x810A74D)

extern const u8 gText_Fuuin_ItemSealed[];

extern const u8 gText_Shakariki_CantUseItem[];
extern const u8 gText_Pyramid_DadsAdvice[];   //strings/hoenn_pyramid.string（日本語版エメラルドの文）
bool8 HoennPyramid_BagIsApplied(void);   //src/hoenn_pyramid.c

static void BlockedItemMessage(u8 taskId)
{
	const u8* text = gText_Challenge_CantUseItem;

	if (HackFuuin_ItemSealed(gSpecialVar_ItemId))
		text = gText_Fuuin_ItemSealed;   //ふういんルーレット: 封印された道具は専用の一言
	else if (HackDifficulty_ItemBlocked(gSpecialVar_ItemId))
		text = gText_Shakariki_CantUseItem;   //しゃかりき: キズぐすり以外の回復の道具
	DisplayItemMessageInBag(taskId, 2, text, Task_ReturnToBagFromContextMenu);
}

//item.c の sItemMenuActions の「つかう」（戦闘）: ボール以外は つかえない
void HackChallenge_ItemMenuUseInBattle(u8 taskId)
{
	if ((FlagGet(HACK_FLAG_CH_NOITEMS) && GetPocketByItemId(gSpecialVar_ItemId) != POCKET_POKEBALLS) || HackChallengeC_ReviveBlocked(gSpecialVar_ItemId) || HackFuuin_ItemSealed(gSpecialVar_ItemId) || HackDifficulty_ItemBlocked(gSpecialVar_ItemId))
	{
		BlockedItemMessage(taskId);
		return;
	}
	VanillaItemMenu_UseInBattle(taskId);
}

//バトルフロンティアの挑戦中（FLAG_BATTLE_FACILITY）は 合体・分離の道具 4 種（いでんしのくさび・ネクロプラスソル／ルナ・キズナのタヅナ）を使わせない（2026-10-29 ユーザー）
bool8 HackFrontier_FusionItemBlocked(u16 item)
{
	return FlagGet(FLAG_BATTLE_FACILITY)
	    && (item == ITEM_DNA_SPLICERS || item == ITEM_N_SOLARIZER || item == ITEM_N_LUNARIZER || item == ITEM_REINS_OF_UNITY);
}

//item.c の sItemMenuActions の「つかう」（フィールド）: かいふくの どうぐ（battleUsage 1）は つかえない
void HackChallenge_ItemMenuUseOutOfBattle(u8 taskId)
{
	if ((FlagGet(HACK_FLAG_CH_NOITEMS) && gItemData[SanitizeItemId(gSpecialVar_ItemId)].battleUsage == 1) || HackChallengeC_ReviveBlocked(gSpecialVar_ItemId) || HackFuuin_ItemSealed(gSpecialVar_ItemId) || HackDifficulty_ItemBlocked(gSpecialVar_ItemId))
	{
		BlockedItemMessage(taskId);
		return;
	}
	if (HackFrontier_FusionItemBlocked(gSpecialVar_ItemId)) //挑戦中の合体・分離は 父の言葉（ピラミッドのバッグと同じ一言）で止める・道具は減らない
	{
		DisplayItemMessageInBag(taskId, 2, gText_Pyramid_DadsAdvice, Task_ReturnToBagFromContextMenu);
		return;
	}
	//バトルピラミッドの中のバトルバッグ（エメラルド battle_pyramid_bag.c BagAction_UseOnField）: キーアイテム・ボール・わざマシン・メールは「つかう」で父の言葉を出して使わせない（BUG-pyramid-held-items-B）
	if (HoennPyramid_BagIsApplied() && (GetPocketByItemId(gSpecialVar_ItemId) == POCKET_KEYITEMS || GetPocketByItemId(gSpecialVar_ItemId) == POCKET_POKEBALLS
	 || GetPocketByItemId(gSpecialVar_ItemId) == POCKET_TM_HM || IsMail(gSpecialVar_ItemId)))
	{
		DisplayItemMessageInBag(taskId, 2, gText_Pyramid_DadsAdvice, Task_ReturnToBagFromContextMenu);
		return;
	}
	VanillaItemMenu_UseOutOfBattle(taskId);
}

//move_menu.c の IsBagDisabled: トレーナー戦は ボールも 意味が無いのでバッグごと閉じる（野生はボールのために開ける）
bool8 HackChallenge_BagClosedInBattle(void)
{
	return (FlagGet(HACK_FLAG_CH_NOITEMS) && (gBattleTypeFlags & BATTLE_TYPE_TRAINER))
	    || (FlagGet(HACK_FLAG_CH_BATTLE) && !(gBattleTypeFlags & BATTLE_TYPE_LINK));   //縛り C: ？？？戦はバッグも開けない（patches/0550）
}
