#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/event_data.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/pokemon.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/constants/pokemon.h"
#include "../include/constants/trainer_classes.h"
#include "../include/new/catching.h"
#include "../include/new/dynamax.h"
#include "hack_battle_squash.h"

/*
 * ロケットボール（patches/0914・FEAT-rocket-ball・2026-10-26 ユーザー）。
 *  ロケット団（トレーナークラス CLASS_TEAM_ROCKET の「したっぱ」「かんぶ」。ボスは別クラス＝対象外）が、場に出ている自分の子を 1 匹ボールで奪い、
 *  戦闘のあいだ ロケット団の側で戦わせる。戦闘が終われば（勝ち・負け・にげる・全滅・引き分け）必ず持ち主に戻る。
 *
 *  作り（hack_battle_squash.c と同じ流儀）:
 *   - 奪った時点の手持ちの子の丸ごと（struct Pokemon 100 バイト）と性格値・手持ちの番号を、追加の RAM（0x01000400〜・直した mGBA だけ）に控える。
 *     追加の RAM が無い環境（実機・SuperCard・素の mGBA）では使わない（HackBadgeSquash_BattleAllowed が見る）。
 *   - 自分の側の手持ちの枠は、ひんし（HP 0）の抜け殻として残す＝経験値・進化・技進化の回数は入らない。場の子は素のひんしと同じ流れ（交代／全員奪われたら負け）。
 *   - 相手の手持ちの空き枠（無ければ 場に出ていないひんしの枠）に 控えの写しを入れる（ボールの種類は BALL_TYPE_ROCKET_BALL）。
 *   - 戻し: 控えをそのまま書き戻す＝ロケット団の側で受けたダメージ・経験値・PP・持ち物の変化は戻った子に残らない。
 *     出口 ① EndOfBattleThings の頭（ひんしでお別れ・BringBackTheDead・ハルカ／ヒカリの回復より前・上限の戻しより前）
 *          ② フィールドの毎コマの安全装置（戦闘が終わって野外に戻っているのに印が残っているとき）
 *          ③ 電源断・リセットは 追加の RAM ごと消える。戦闘中はセーブできないので 元のセーブに戻る
 *   - ひんしでお別れ（縛り A）は 戦闘の終わりの戻しのあとに走る＝抜け殻を「ひんし」と見て外すことは無い。
 *   - 奪われた子には ひんしの なつき度 -1・ひんしの回数が付かない（素の tryfaintmon を通さず 同じ印だけ立てる）。
 *
 *  置き場: 追加の RAM +0 魔法の値（'RKB1' ＝ 控えあり）・+4 控えの数・+5 予約（使う候補の手持ちの番号 + 1）・+6 予約の場・+7 空き枠の番号
 *          +8 + 112 n 控え n: +0 性格値(u32)・+4 手持ちの番号・+8 子の丸ごと（100 バイト）。最大 4 匹（1 戦 4 回まで）。
 *
 *  成功率: なつき度 255（最大）は必ず防ぐ（はねかえす）。それ未満は 100 − なつき度 × 100 / 255（切り捨て）%。
 */

#define HACK_RKB_BASE      ((u8*) 0x01000400)
#define HACK_RKB_MAGIC     0x31424B52u   /* 'RKB1' */
#define HACK_RKB_STRIDE    112
#define HACK_RKB_MAX       4
#define HACK_RKB_ODDS      50            /* 使える場面ごとに 投げる確率（%） */

//台本へ返す結果（gBattleCommunication[0]）
#define RKB_RESULT_STEAL   0
#define RKB_RESULT_MISS    1
#define RKB_RESULT_REPEL   2

bool8 HackXram_Available(void);               //src/hack_xram.c
u8 __attribute__((long_call)) CalculateEnemyPartyCount(void);
void HackRocketBall_Restore(void);

static u32 ReadU32(const u8* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32) p[3] << 24);
}

static void WriteU32(u8* p, u32 v)
{
	p[0] = v;
	p[1] = v >> 8;
	p[2] = v >> 16;
	p[3] = v >> 24;
}

static void CopyBytes(u8* dst, const u8* src, u32 n)   //BIOS の CpuSet は写し元が 0x02000000 より前だと断る＝自前の写し
{
	u32 i;

	for (i = 0; i < n; ++i)
		dst[i] = src[i];
}

static bool8 Marked(void)
{
	return HackXram_Available() && ReadU32(HACK_RKB_BASE) == HACK_RKB_MAGIC;
}

//「したっぱ」（0x0C 0x10 0x50 0x4B）・「かんぶ」（0x06 0x2E 0x48）
static bool8 IsGruntOrExecutive(const u8* name)
{
	if (name[0] == 0x0C && name[1] == 0x10 && name[2] == 0x50 && name[3] == 0x4B)
		return TRUE;
	return name[0] == 0x06 && name[1] == 0x2E && name[2] == 0x48;
}

static bool8 BattleAllowed(void)
{
	const struct Trainer* trainer;

	if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) || (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) || IsRaidBattle())
		return FALSE;
	if (!HackBadgeSquash_BattleAllowed())   //通信・施設・フロンティア・相方・マルチ・ダミー・追加 RAM なし を除く
		return FALSE;
	trainer = &gTrainers[gTrainerBattleOpponent_A];
	return trainer->trainerClass == CLASS_TEAM_ROCKET && IsGruntOrExecutive(trainer->trainerName);
}

//相手の手持ちで 控えの写しを入れてよい枠（空き → 場に出ていないひんし）。無ければ PARTY_SIZE
static u8 FindEnemySlot(void)
{
	u8 i, b;

	for (i = 0; i < PARTY_SIZE; ++i)
		if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
			return i;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		bool8 onField = FALSE;

		for (b = 0; b < gBattlersCount; ++b)
			if (SIDE(b) == B_SIDE_OPPONENT && gBattlerPartyIndexes[b] == i)
				onField = TRUE;
		if (!onField && GetMonData(&gEnemyParty[i], MON_DATA_HP, NULL) == 0)
			return i;
	}
	return PARTY_SIZE;
}

//AI が道具を選ぶ所（ShouldAIUseItem の頭）から。投げるなら TRUE（ACTION_USE_ITEM を選ぶのは呼び元）
bool8 HackRocketBall_Try(u8 bank)
{
	u8* d = HACK_RKB_BASE;
	u8 target, slot, candidates[2], count = 0, b;

	if (SIDE(bank) != B_SIDE_OPPONENT || gNewBS == NULL || gNewBS->hackRocketUses >= HACK_RKB_MAX)
		return FALSE;
	if (!BattleAllowed())
		return FALSE;
	if (Marked() && d[4] >= HACK_RKB_MAX)
		return FALSE;
	slot = FindEnemySlot();
	if (slot >= PARTY_SIZE)
		return FALSE;
	for (b = 0; b < gBattlersCount; ++b)   //場に出ている自分の子（ダイマックス中は奪わない）
	{
		if (SIDE(b) == B_SIDE_PLAYER && !(gAbsentBattlerFlags & gBitTable[b]) && gBattleMons[b].hp != 0 && !IsDynamaxed(b) && count < 2)
			candidates[count++] = b;
	}
	if (count == 0)
		return FALSE;
	if ((Random() % 100) >= HACK_RKB_ODDS)
		return FALSE;
	target = candidates[(count == 1) ? 0 : (Random() & 1)];
	gNewBS->hackRocketUses++;
	d[5] = gBattlerPartyIndexes[target] + 1;
	d[6] = target;
	d[7] = slot;
	gBattleScripting.bank = target;
	return TRUE;
}

//台本の頭（BattleScript_AIUseGuardSpec）から。予約があれば 専用の台本へ。無ければ素の ガードスペシャルの流れ
extern const u8 BattleScript_HackRocketBall[];
void HackRocketBall_ScriptEntry(void)
{
	u8* d = HACK_RKB_BASE;

	if (HackXram_Available() && d[5] != 0 && gNewBS != NULL)
		gBattlescriptCurrInstr = BattleScript_HackRocketBall - 5;   //callasm は実行のあと 5 バイト進める
}

//台本から（callasm）: 結果を決めて 文に出す子の名前を入れる。奪うのは次の HackRocketBall_Steal
void HackRocketBall_Roll(void)
{
	u8* d = HACK_RKB_BASE;
	u8 idx = d[5] - 1;
	u8 target = d[6];
	u32 friendship = GetMonData(&gPlayerParty[idx], MON_DATA_FRIENDSHIP, NULL);

	gBattleScripting.bank = target;
	PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, target, idx);
	if (friendship >= MAX_FRIENDSHIP)
		gBattleCommunication[0] = RKB_RESULT_REPEL;   //なつき度が最大なら 必ず はねかえす
	else if ((Random() % 100) < 100 - (friendship * 100) / MAX_FRIENDSHIP)
		gBattleCommunication[0] = RKB_RESULT_STEAL;
	else
		gBattleCommunication[0] = RKB_RESULT_MISS;
	if (gBattleCommunication[0] != RKB_RESULT_STEAL)
		d[5] = 0;   //予約を消す
}

//台本から（callasm）: 控えを取り、相手の手持ちへ写し、自分の側の枠を HP 0 にする準備（HP の書き換えとひんしの流れは台本）
void HackRocketBall_Steal(void)
{
	u8* d = HACK_RKB_BASE;
	u8 idx = d[5] - 1;
	u8 target = d[6];
	u8 slot = d[7];
	u8 n;
	u8* e;
	struct Pokemon* mon = &gPlayerParty[idx];
	u8 ball = BALL_TYPE_ROCKET_BALL;

	if (ReadU32(d) != HACK_RKB_MAGIC)
	{
		WriteU32(d, HACK_RKB_MAGIC);
		d[4] = 0;
	}
	n = d[4];
	if (n >= HACK_RKB_MAX)
		return;
	e = d + 8 + HACK_RKB_STRIDE * n;
	WriteU32(e, GetMonData(mon, MON_DATA_PERSONALITY, NULL));
	e[4] = idx;
	CopyBytes(e + 8, (const u8*) mon, sizeof(struct Pokemon));
	d[4] = n + 1;   //控えを取り終えてから数える（途中で何かあっても 戻しは控えのぶんだけ）
	d[5] = 0;

	CopyBytes((u8*) &gEnemyParty[slot], (const u8*) mon, sizeof(struct Pokemon));
	SetMonData(&gEnemyParty[slot], MON_DATA_POKEBALL, &ball);
	CalculateEnemyPartyCount();

	//場の子はひんしの流れに乗せる（素の tryfaintmon と同じ印・なつき度とひんしの回数は動かさない）
	gBattleMoveDamage = gBattleMons[target].hp;
	gBattleScripting.bank = target;
	gNewBS->lastFainted = target;
	gHitMarker |= HITMARKER_FAINTED(target) | HITMARKER_PLAYER_FAINTED;
}

//戦闘のあと（end_battle.c の EndOfBattleThings の頭）／フィールドの安全装置。印が無ければ何もしない＝何度呼んでも壊れない
void HackRocketBall_Restore(void)
{
	u8* d = HACK_RKB_BASE;
	u32 i, j, n;

	if (!Marked())
	{
		if (HackXram_Available())
			d[5] = 0;   //使われなかった予約を消す
		return;
	}
	n = d[4];
	if (n > HACK_RKB_MAX)
		n = HACK_RKB_MAX;
	WriteU32(d, 0);   //先に印を消す（途中で何かが起きても二重に戻さない）
	d[4] = 0;
	d[5] = 0;
	for (i = 0; i < n; ++i)
	{
		u8* e = d + 8 + HACK_RKB_STRIDE * i;
		u32 pid = ReadU32(e);
		u32 dest = PARTY_SIZE;

		for (j = 0; j < PARTY_SIZE; ++j)   //抜け殻を性格値で探す（戦闘中に並びが入れ替わる）。ひんし（HP 0）のほうを先に
		{
			if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) != SPECIES_NONE
			&&  GetMonData(&gPlayerParty[j], MON_DATA_PERSONALITY, NULL) == pid
			&&  GetMonData(&gPlayerParty[j], MON_DATA_HP, NULL) == 0)
			{
				dest = j;
				break;
			}
		}
		if (dest == PARTY_SIZE)   //抜け殻が見つからない（想定外）: 空き枠に戻す。それも無ければ元の番号の枠に上書き
		{
			for (j = 0; j < PARTY_SIZE; ++j)
				if (GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
				{
					dest = j;
					break;
				}
			if (dest == PARTY_SIZE)
				dest = e[4] < PARTY_SIZE ? e[4] : 0;
		}
		CopyBytes((u8*) &gPlayerParty[dest], e + 8, sizeof(struct Pokemon));
		WriteU32(e, 0);
	}
	CalculatePlayerPartyCount();
}

//フィールドの毎コマ（HackChallengeC_FieldTick から）: 戦闘が終わって野外に戻っているのに印が残っているとき
void HackRocketBall_FieldTick(void)
{
	if (Marked() && gNewBS == NULL && !ScriptContext2_IsEnabled() && gMain.callback2 == CB2_Overworld)
		HackRocketBall_Restore();
}
