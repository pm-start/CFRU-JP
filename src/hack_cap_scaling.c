#include "defines.h"
#include "defines_battle.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/constants/pokemon.h"
#include "hack_dummy_battle.h"

/*
 * 相手トレーナーのレベルスケーリングを「今のレベル上限」でそろえる（patches/0620・docs/research/2026-10-20_level-cap-opponent-clamp.md）。
 *
 *  不具合: 殿堂入り済みの地方からシンオウに入ると、自分の子は上限（バッジ 0〜1 個で 125）をこえていて経験値が入らないのに、
 *  ジムリーダーは SCALED_TRAINERS で手持ちの高さに合わせて上限より上（約 140）に作られ、追いつけなくなった。
 *  追加の RAM が無い環境（HackBadgeSquash が効かない）では 自分の子の本当の高さが そのままスケーリングに入る。効く環境でも ボス戦は「最高 + 1」で 上限 + 1 になる。
 *
 *  決まり: 相手の子のレベルが、自分に効いている上限（HackChallengeB_LevelCap・場所の上限と縛り B の低いほう・0 ＝ なし）をこえるときは
 *    max(上限, その子の表のままのレベル) まで下げる。表のレベルより下には下げない・上限なし（0）は何もしない。
 *  呼ぶ場所: build_pokemon.c の CreateNPCTrainerParty、HackDifficulty_AdjustTrainerMon（ももアイス／しゃかりき／やさしい のレベル増減）のあと・
 *    HackGymTier_AdjustMon（ジムリーダー再戦の段階のレベル）の前＝「上限が最後に勝つ」。再戦の段階のレベルは 設計したレベルなので 自分の上限（バッジの数）で別に止まる。
 *  対象外: 相手側以外（パートナー）・通信・フロンティア・バトル施設・ポケセン 3F のダミー（自分の子が上限に収められない戦い）。野生は対象外（スケーリングの旗が使われていない）。
 *  状態を持たない（6.13）。除算を使わない（6.83）。
 */

u8 HackChallengeB_LevelCap(void);   //src/hack_challenge_b.c

//クランプしてよい戦いか（hack_challenge_b.c の InCountedBattle と同じ範囲）
static bool8 ClampApplies(void)
{
	return !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER)) && !FlagGet(FLAG_BATTLE_FACILITY) && !HackDummy_Active();
}

//cap（0 ＝ なし）と 表のレベル designed から 相手の子が取ってよい最高レベルを返す。0 ＝ 制限なし
u8 HackCapScaling_MaxLevel(u8 cap, u8 designed)
{
	if (cap == 0)
		return 0;
	return (cap < designed) ? designed : cap;
}

//相手の子 mon（表のレベル designed）のレベルを 上限で止める。side は B_SIDE_*
void HackCapScaling_ClampOpponentMon(struct Pokemon* mon, u8 side, u8 designed)
{
	u8 max, level;
	u16 species;

	if (side != B_SIDE_OPPONENT || !ClampApplies())
		return;
	max = HackCapScaling_MaxLevel(HackChallengeB_LevelCap(), designed);
	if (max == 0)
		return;
	level = GetMonData(mon, MON_DATA_LEVEL, NULL);
	if (level <= max)
		return;
	species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	SetMonData(mon, MON_DATA_EXP, &gExperienceTables[gBaseStats[species].growthRate][max]);
	SetMonData(mon, MON_DATA_LEVEL, &max);
}
