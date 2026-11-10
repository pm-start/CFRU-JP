#include "defines.h"
#include "../include/battle.h"
#include "../include/main.h"
#include "../include/overworld.h"
#include "../include/pokemon.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/constants/species.h"
#include "../include/new/Vanilla_functions_battle.h"
#include "hack_mon_id.h"

/*
 * トキワシティの「ポケモンの捕まえ方を教えてくれるおじいさん」のお手本の野生を、毎回 色違い・伝説・Moémon の姿 にする（2026-11-03・ユーザー依頼・patches 無し＝自作）。
 *   - 素の StartOldManTutorialBattle（FR 日本語版 0807EFB0・プールまで 0x3C バイト・呼び元は特殊 StartOldManTutorialBattle の表 1 か所）は CreateMaleMon(&gEnemyParty[0], ビードル, 5) の
 *     あと 画面を止めて 戦いを始めるだけ。functionrewrites で下の関数に置き換え、ビードルの代わりに 表 sOldManLegends から Random() で 1 匹（来るたび）選ぶ。
 *   - CreateMaleMon は「オス」が出るまで性格値を回し続けるので、性別なし・メスだけの伝説で止まる → 使わず CreateMon を直に呼ぶ（性格値は乱数）。
 *   - 色違い・Moémon の当たりは自身の ID だけで決める（HackMonId_MakeShinyMoemon・性格値は触らない・6.5）。Moémon の絵が出るかは たいせつなものの Moemon の旗（OFF なら色違いの伝説の普通の絵）。
 *   - レベル・戦いの型（BATTLE_TYPE_OLD_MAN）・画面の切り替え（8）・戻り先は素のまま。おじいさんがボールを投げて必ず捕まる演出は素の戦闘の台本（相手の技・特性は使わない）。
 *     手持ち・図鑑（見た／捕まえた）は素のとおり触らない（相手は gEnemyParty[0] だけ・素の戦いは BATTLE_TYPE_OLD_MAN で図鑑の「見た」を付けない）。
 * 表は Moémon の絵がある伝説・幻だけ（伝説レイド src/hack_legend_raid.c の一覧のうち DPE の gMoemonSpeciesFirst／Count が 0 でない種族・tools/rom/verify_oldman_legend.py が検算）。
 * わたしたちの伝説 10 匹は入れていない（Lv180 の専用の出会いがあるため）。
 */

#define OLDMAN_LEVEL 5   //素のビードルと同じ

static const u16 sOldManLegends[] =
{
	SPECIES_ARTICUNO, SPECIES_ZAPDOS, SPECIES_MOLTRES, SPECIES_MEWTWO, SPECIES_MEW, SPECIES_RAIKOU,
	SPECIES_ENTEI, SPECIES_SUICUNE, SPECIES_LUGIA, SPECIES_HO_OH, SPECIES_CELEBI, SPECIES_REGIROCK,
	SPECIES_REGICE, SPECIES_REGISTEEL, SPECIES_LATIAS, SPECIES_LATIOS, SPECIES_KYOGRE, SPECIES_GROUDON,
	SPECIES_RAYQUAZA, SPECIES_JIRACHI, SPECIES_DEOXYS, SPECIES_UXIE, SPECIES_MESPRIT, SPECIES_AZELF,
	SPECIES_DIALGA, SPECIES_PALKIA, SPECIES_HEATRAN, SPECIES_REGIGIGAS, SPECIES_GIRATINA, SPECIES_CRESSELIA,
	SPECIES_PHIONE, SPECIES_MANAPHY, SPECIES_DARKRAI, SPECIES_SHAYMIN, SPECIES_ARCEUS, SPECIES_VICTINI,
	SPECIES_COBALION, SPECIES_TERRAKION, SPECIES_VIRIZION, SPECIES_TORNADUS, SPECIES_THUNDURUS,
	SPECIES_RESHIRAM, SPECIES_ZEKROM, SPECIES_LANDORUS, SPECIES_KYUREM, SPECIES_KELDEO, SPECIES_MELOETTA,
	SPECIES_GENESECT, SPECIES_XERNEAS, SPECIES_YVELTAL, SPECIES_ZYGARDE, SPECIES_DIANCIE, SPECIES_HOOPA,
	SPECIES_SILVALLY, SPECIES_TAPU_KOKO, SPECIES_TAPU_LELE, SPECIES_TAPU_BULU, SPECIES_TAPU_FINI,
	SPECIES_SOLGALEO, SPECIES_LUNALA, SPECIES_NECROZMA, SPECIES_MAGEARNA, SPECIES_MARSHADOW, SPECIES_ZERAORA,
	SPECIES_MELMETAL, SPECIES_ZACIAN, SPECIES_ZAMAZENTA, SPECIES_ETERNATUS, SPECIES_URSHIFU_SINGLE,
	SPECIES_REGIELEKI, SPECIES_REGIDRAGO, SPECIES_SPECTRIER, SPECIES_CALYREX, SPECIES_ARTICUNO_G,
	SPECIES_WO_CHIEN, SPECIES_CHIEN_PAO, SPECIES_TING_LU, SPECIES_CHI_YU, SPECIES_KORAIDON, SPECIES_MIRAIDON,
	SPECIES_OKIDOGI, SPECIES_MUNKIDORI, SPECIES_FEZANDIPITI, SPECIES_PECHARUNT,
};

u16 HackOldMan_LegendCount(void)   //verify 用
{
	return ARRAY_COUNT(sOldManLegends);
}

u16 HackOldMan_LegendAt(u16 i)
{
	return i < ARRAY_COUNT(sOldManLegends) ? sOldManLegends[i] : SPECIES_NONE;
}

//functionrewrites: StartOldManTutorialBattle 0807EFB0（引数なし・戻り値なし）
void HackOldMan_StartTutorialBattle(void)
{
	struct Pokemon* mon = &gEnemyParty[0];

	CreateMon(mon, sOldManLegends[Random() % ARRAY_COUNT(sOldManLegends)], OLDMAN_LEVEL, 32, FALSE, 0, OT_ID_PRESET, Random32());
	HackMonId_MakeShinyMoemon((struct BoxPokemon*) mon);
	ScriptContext2_Enable();   //素の LockPlayerFieldControls（0x08069200・ScriptContext2_Enable と同じ番地）
	gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
	gBattleTypeFlags = BATTLE_TYPE_OLD_MAN;
	CreateBattleStartTask(8, 0);   //B_TRANSITION_SLICE
}
