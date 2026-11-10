#include "defines.h"
#include "defines_battle.h"
#include "../include/constants/items.h"
#include "../include/new/battle_util.h"
#include "../include/new/damage_calc.h"
#include "../include/new/dynamax.h"
#include "../include/new/form_change.h"
#include "../include/new/mega.h"
#include "../include/new/mega_battle_scripts.h"
#include "../include/new/terastal.h"

/*
 * ★ 2026-10-23 FEAT-gible-mega-switch（Redmine #58 とくべつなガブリアス 段階 3・patches/0791）
 * ユーザー「メガシンカしたまま、メガガブリアス ↔ メガガブリアスZ を行き来する」。
 *   - 対象: メガシンカしているガブリアスが、次のどちらかのとき
 *       ・ももアイスを持った れーとのおうの印の子（段階 3 のとき からの条件・Commander 決定 A）
 *       ・ガブリアスナイトX を持った とくべつなガブリアス（れーとのおうの印、または クラブ産＝HackGibleEgg_IsSpecialForX）
 *     （★ 2026-10-25 patches/0813: X を足した。ユーザー「b」で一度 X だけにしたが、「ももアイスは対応残しておいて」で ももアイスも残した。
 *      ガブリアスナイト／ナイトZ を 1 つだけ持つ子は切り替えない）
 *   - 切り替えはガブリアスが自分で判断する（遊ぶ人は選ばない）。ターンは使わず、メガシンカの判定（battle_start_turn_start.c
 *     RunTurnActionsFunctions の Mega_Check）の中で、1 ターンに 1 回・メガシンカしたあとのターンから。
 *   - 判断は乱数を使わない。見るのは両機で同じ値だけ（選んだ技 gChosenMovesByBanks・gBattleMons・ROM の種族値・タイプの表）。
 *     OT（親が主人公か）は見ない（通信で相手の子が自機では印なし、になるのを避ける＝Commander 決定 2026-10-23）。
 *   - 姿の入れ替えは DoFormChange をもう 1 回（タイプ・能力・特性・2 つ目の特性）。メガシンカの後処理（素早さ順の再計算・
 *     場に出たときの特性・2 つ目の特性）は Mega_Check のあとの段がそのまま働く。
 */

extern u8 BattleScript_GibleSwitch[];
bool8 HackGibleEgg_HasReitoMark(const struct Pokemon* mon);
bool8 HackGibleEgg_IsSpecialForX(const struct Pokemon* mon);   //[hack] patches/0813 れーとのおうの印 か クラブ産（OT を見ない）

#define GIBLE_SWITCH_GAIN_PERCENT 110   /* 今の姿より 10% 以上よい姿があるときだけ動く（ぱたぱたしない）。とくこう 120→141（1.175 倍）で動き、こうげき 130→170 のメガに戻る。25% だと とくしゅ技では動かない（算出で判明・2026-10-23） */

static bool8 IsGibleForm(u16 species)
{
	return species == SPECIES_GARCHOMP_MEGA || species == SPECIES_GARCHOMP_MEGA_Z;
}

//れーとのおうの姿ごとの種族値（hack_gible_egg.c HackGibleEgg_ReitoBase＝ステータス計算と同じ値・patches/0793）
u16 HackGibleEgg_ReitoBase(u16 species, u8 stat);
static u32 FormBase(u16 species, u8 stat)
{
	return HackGibleEgg_ReitoBase(species, stat);
}

static u8 FormType(u16 species, u8 slot)
{
	if (slot == 0)
		return gBaseStats[species].type1;
	if (species == SPECIES_GARCHOMP_MEGA_Z)
		return TYPE_ROCK;   //特別なガブリアスのメガZ は 2 つ目のタイプが いわ（HackDragonBoulder_Type2）
	return gBaseStats[species].type2;
}

//1 つのタイプへの倍率（表の 0 = 載っていない = 等倍・1 = こうかなし → 0）。CFRU の ModulateDmgByType と同じ読み方
static u32 TypeMul1(u8 moveType, u8 defType)
{
	u32 m;

	if (moveType >= NUMBER_OF_MON_TYPES || defType >= NUMBER_OF_MON_TYPES)
		return TYPE_MUL_NORMAL;
	m = gTypeEffectiveness[moveType][defType];
	if (m == TYPE_MUL_NO_DATA)
		return TYPE_MUL_NORMAL;
	if (m == TYPE_MUL_NO_EFFECT)
		return 0;
	return m;
}

static u32 TypeMul(u8 moveType, u8 t1, u8 t2, u8 t3)
{
	u32 mul = TypeMul1(moveType, t1);

	if (t2 != t1)
		mul = mul * TypeMul1(moveType, t2) / TYPE_MUL_NORMAL;
	if (t3 != t1 && t3 != t2 && t3 < NUMBER_OF_MON_TYPES)
		mul = mul * TypeMul1(moveType, t3) / TYPE_MUL_NORMAL;
	return mul;
}

static bool8 ActsThisTurn(u8 bank)
{
	return BATTLER_ALIVE(bank)
	    && gChosenActionByBank[bank] == ACTION_USE_MOVE
	    && gChosenMovesByBanks[bank] != MOVE_NONE;
}

//この姿の点 = 与える ÷ 受ける（整数・乱数なし）
static u32 FormScore(u8 bank, u16 form, u16 cur)
{
	u16 myMove = gChosenMovesByBanks[bank];
	u8 mySplit = SPLIT(myMove);
	u8 myType = GetMoveTypeSpecial(bank, myMove);
	u8 t1 = FormType(form, 0), t2 = FormType(form, 1);
	u32 dealt = 1, taken = 0, speed;
	u8 foes[2], i;
	bool8 faster = FALSE;

	foes[0] = FOE(bank);
	foes[1] = IS_DOUBLE_BATTLE ? PARTNER(FOE(bank)) : FOE(bank);

	/* 素早さ: 今の素早さを種族値の比で姿ごとに見積もる（ステージ・まひを含む今の値が基準） */
	speed = (u32) gBattleMons[bank].speed * FormBase(form, STAT_SPEED) / FormBase(cur, STAT_SPEED);

	for (i = 0; i < 2; ++i)
	{
		u8 foe = foes[i];
		u16 foeMove;

		if (!BATTLER_ALIVE(foe) || (i == 1 && foes[1] == foes[0]))
			continue;

		if (i == 0)   //最初の相手より先に動けるか（トリックルームでは逆）
		{
			u32 foeSpeed = gBattleMons[foe].speed;
			faster = gNewBS->TrickRoomTimer ? (speed < foeSpeed) : (speed > foeSpeed);
		}

		/* 与える */
		if (mySplit != SPLIT_STATUS && MOVE_POWER(myMove) != 0)
		{
			u32 mul = TypeMul(myType, gBattleMons[foe].type1, gBattleMons[foe].type2, gBattleMons[foe].type3);
			u32 stab = (myType == t1 || myType == t2) ? 150 : 100;
			u32 stat = FormBase(form, mySplit == SPLIT_PHYSICAL ? STAT_ATK : STAT_SPATK);

			if (myType == TYPE_GROUND && !CheckGrounding(foe))
				mul = 0;
			dealt += stat * stab * mul / TYPE_MUL_NORMAL;
		}

		/* 受ける（相手が実際に選んだ技） */
		if (!ActsThisTurn(foe))
			continue;
		foeMove = gChosenMovesByBanks[foe];
		if (SPLIT(foeMove) != SPLIT_STATUS)
		{
			u8 foeType = GetMoveTypeSpecial(foe, foeMove);
			u32 mul = TypeMul(foeType, t1, t2, TYPE_BLANK);
			u32 power = MOVE_POWER(foeMove) ? MOVE_POWER(foeMove) : 60;   //威力 256 以上の技は bigPower（patches/0582）
			u32 def = FormBase(form, SPLIT(foeMove) == SPLIT_PHYSICAL ? STAT_DEF : STAT_SPDEF);

			if (form == SPECIES_GARCHOMP_MEGA_Z)
			{
				if (foeType == TYPE_GROUND)
					mul = 0;                    //りゅうのいわお（ふゆう）
				else if (mul > TYPE_MUL_NORMAL)
					mul = mul * 3 / 4;          //りゅうのいわお（ハードロック）
			}
			taken += mul * power / def;
		}
	}

	return (faster ? dealt * 125 / 100 : dealt) * 64 / (taken + 256);
}

static bool8 EligibleBase(u8 bank)
{
	struct Pokemon* mon;

	if (!BATTLER_ALIVE(bank) || !IsGibleForm(gBattleMons[bank].species))
		return FALSE;
	/* ★ 2026-10-25 patches/0813 切り替えの石は 2 つ（ユーザー「ももアイスは対応残しておいて」）:
	   ・ももアイス（今までどおり）: れーとのおうの印の子だけ
	   ・ガブリアスナイトX: とくべつなガブリアス（れーとのおうの印 か クラブ産＝うんめいの印＋色違い＋個体値最大・親の ID は見ない＝通信対戦でずれないため）
	   どちらの判定も 親の ID（OT）を見ない */
	u16 item = gBattleMons[bank].item;

	if (item != ITEM_GARCHOMPITE_X && item != ITEM_MOMO_ICE)
		return FALSE;
	if (IsDynamaxed(bank) || IsTerastal(bank) || IS_TRANSFORMED(bank))
		return FALSE;
	mon = GetBankPartyData(bank);
	if (!(mon->species == SPECIES_GARCHOMP || IsGibleForm(mon->species))) /* 手持ちの種族はメガシンカ中ずっとメガの種族のまま（戦闘後 TryRevertMega で戻る）*/
		return FALSE;
	return (item == ITEM_MOMO_ICE) ? HackGibleEgg_HasReitoMark(mon) : HackGibleEgg_IsSpecialForX(mon);
}

static bool8 IsSecondPending(u8 bank)
{
	return gNewBS->hackGibleSecondBank == bank + 1;
}

static bool8 Eligible(u8 bank)
{
	if (!EligibleBase(bank))
		return FALSE;
	if (IsSecondPending(bank))   //2 回目の前の判断（PrepareSecond が 1 ターン 1 回を確かめてある）
		return TRUE;
	if (!ActsThisTurn(bank))
		return FALSE;
	if (gNewBS->hackGibleSwitchLock & gBitTable[bank])
		return FALSE;   //このターンはもう判断した／メガシンカした（end_turn.c で 0 に戻す）
	return TRUE;
}

/* patches/0793 2 回目の技を決めたあと（hack_gible_ai.c TryDouble）に呼ぶ。このターンまだ切り替えていなければ「2 回目の前の判断」を予約する。
   判断そのものは 2 回目の行動の頭の Mega_Check（Do）で行う＝ メガシンカしたターンでも・ターンの始めに判断だけして動かなかったときでも使える */
void HackGibleSwitch_PrepareSecond(u8 bank, u16 move)
{
	gNewBS->hackGibleSecondBank = 0;
	if (!EligibleBase(bank) || (gNewBS->hackGibleSwitched & gBitTable[bank]))
		return;
	gNewBS->hackGibleSecondBank = bank + 1;
	gNewBS->hackGibleSecondMove = move;
}

void HackGibleSwitch_NoteMega(u8 bank);

//切り替え先の種族（動かないなら SPECIES_NONE）
u16 HackGibleSwitch_Decide(u8 bank)
{
	u16 cur, other, saved = MOVE_NONE, result = SPECIES_NONE;
	bool8 second = IsSecondPending(bank);

	if (!Eligible(bank))
		return SPECIES_NONE;
	if (second)
	{
		saved = gChosenMovesByBanks[bank];   //点数は 2 回目の技で見る（見終わったら戻す）
		gChosenMovesByBanks[bank] = gNewBS->hackGibleSecondMove;
		gNewBS->hackGibleSecondBank = 0;
	}
	else
		HackGibleSwitch_NoteMega(bank);   //判断は 1 ターンに 1 回（動かなくても数える）
	cur = gBattleMons[bank].species;
	other = (cur == SPECIES_GARCHOMP_MEGA) ? SPECIES_GARCHOMP_MEGA_Z : SPECIES_GARCHOMP_MEGA;
	if (FormScore(bank, other, cur) * 100 > FormScore(bank, cur, cur) * GIBLE_SWITCH_GAIN_PERCENT)
		result = other;
	if (second)
		gChosenMovesByBanks[bank] = saved;
	return result;
}

//DoMegaEvolution から: メガシンカしたターンは切り替えない
void HackGibleSwitch_NoteMega(u8 bank)
{
	gNewBS->hackGibleSwitchLock |= gBitTable[bank];
}

//切り替えを実行して台本を返す（切り替えないなら NULL）
const u8* HackGibleSwitch_Do(u8 bank)
{
	u16 target = HackGibleSwitch_Decide(bank);

	if (target == SPECIES_NONE)
		return NULL;
	gNewBS->hackGibleSwitched |= gBitTable[bank];   //1 ターン 1 回（2 回目の前の判断はこれを見る）
	gNewBS->backupAbility = ABILITY(bank);
	DoFormChange(bank, target, TRUE, TRUE, TRUE);
	gBattleScripting.bank = bank;
	return BattleScript_GibleSwitch;
}
