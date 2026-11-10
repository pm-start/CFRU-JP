#include "defines.h"
#include "defines_battle.h"
#include "../include/battle.h"

/*
 * 戦闘の最初の 1 匹（場に出ている子）のタイプ（[hack] patches/0974・BUG-first-battler-type）。
 * 素の機械語は戦闘の始めに gBattleMons へ写すとき、種族の表のタイプをそのまま入れる。場に出す・入れ替えは switching.c の atk4D が
 * 個体のタイプ（GetMonType の順＝デバッグモン→りゅうのいわお）に上書きするが、最初の 1 匹はそこを通らず、
 * デバッグメニューのタイプ変更・りゅうのいわおが効かなかった。そこで戦闘の最初のターンの頭（battle_start_turn_start.c の
 * BattleBeginFirstTurn・BTSTART_BACKUP_PARTY_ITEMS）で、場に出ている子の type1／type2 を個体のタイプ（GetMonType）に書く。
 * この時点は 場に出たときの特性・ゲンシカイキ・テラスタル・ダイマックス・持ち物の発動より前なので、それらの変化を上書きしない。
 * type3 は触らない・カモモンズ（技のタイプを使う）は触らない。
 * デルタ種の最初の 1 匹もここで書く（GetMonType が HackDelta_Type を通すので デバッグ→デルタ→りゅうのいわお の順で返る）。
 * 旧 HackDelta_OnBattleStart（patches/0962）はこの関数に 1 つにまとめた（呼び出しも外した）。
 */
void HackFirstBattler_OnBattleStart(void)
{
	u8 bank;

	if (gBattleTypeFlags & BATTLE_TYPE_CAMOMONS)
		return;
	for (bank = 0; bank < gBattlersCount; ++bank)
	{
		struct Pokemon* mon;

		if (gBattleMons[bank].species == SPECIES_NONE)
			continue;
		mon = GetBankPartyData(bank);
		if (mon == NULL)
			continue;
		gBattleMons[bank].type1 = GetMonType(mon, 0);
		gBattleMons[bank].type2 = GetMonType(mon, 1);
	}
}
