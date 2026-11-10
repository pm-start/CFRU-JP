#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/constants/pokemon.h"
#include "../include/constants/trainer_classes.h"

/*
 * ロケット団のしたっぱの実数値補正（patches/0660・FEAT-rocket-grunt-stats・2026-10-21 ユーザー確定）。
 *  相手のポケモンの実数値（CalculateMonStatsNew で出したあと）: 攻撃・とくこう・すばやさ ×1.3、HP・防御・とくぼう ×0.7。
 *  丸め: 切り捨て（値 × 13 / 10・値 × 7 / 10）。最小 1・最大 0xFFFF。ヌケニンの HP は 1 のまま。
 *  対象: トレーナークラスが CLASS_TEAM_ROCKET（0x55）で、名前が「したっぱ」の相手（同じクラスの「かんぶ」は対象外）。地方は問わない。
 *  対象外: 相手側以外（味方）・通信・フロンティア・バトル施設。なんいどのレベル調整とは併用（レベルは従来どおり）。
 *  呼ぶ場所: build_pokemon.c の CreateNPCTrainerParty、CalculateMonStatsNew・HealMon のあと（HP は最大に戻す＝満タンで始まる）。
 *  状態を持たない（6.13）。除算は / だけ（6.83）。
 */

//「したっぱ」（0x0C 0x10 0x50 0x4B）
static bool8 IsGruntName(const u8* name)
{
	return name[0] == 0x0C && name[1] == 0x10 && name[2] == 0x50 && name[3] == 0x4B;
}

bool8 HackGruntStats_Applies(u8 trainerClass, const u8* trainerName, u8 side)
{
	if (side != B_SIDE_OPPONENT || trainerClass != CLASS_TEAM_ROCKET || !IsGruntName(trainerName))
		return FALSE;
	return !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)) && !FlagGet(FLAG_BATTLE_FACILITY);
}

static u16 Scale(u16 value, u32 mul)
{
	u32 v = (value * mul) / 10;

	if (v < 1)
		v = 1;
	if (v > 0xFFFF)
		v = 0xFFFF;
	return v;
}

void HackGruntStats_Apply(struct Pokemon* mon, u8 side, u8 trainerClass, const u8* trainerName)
{
	if (!HackGruntStats_Applies(trainerClass, trainerName, side))
		return;
	mon->maxHP = (mon->species == SPECIES_SHEDINJA) ? mon->maxHP : Scale(mon->maxHP, 7);
	mon->attack = Scale(mon->attack, 13);
	mon->defense = Scale(mon->defense, 7);
	mon->speed = Scale(mon->speed, 13);
	mon->spAttack = Scale(mon->spAttack, 13);
	mon->spDefense = Scale(mon->spDefense, 7);
	mon->hp = mon->maxHP;   //HealMon のあとに呼ぶので、下げた最大 HP に合わせて満タンに戻す
}
