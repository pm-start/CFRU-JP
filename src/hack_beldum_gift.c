#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/random.h"
#include "../include/constants/species.h"
#include "hack_kind.h"
#include "hack_delta.h"
#include "hack_mon_id.h"
#include "hack_beldum_gift.h"

/*
 * ★ 2026-11-09 FEAT-beldum-gift（ユーザー「ダンバルの入手手段追加」→「ダンバル色違いもmoemonヌシポケモン確率あげといて」→「全部1/16で」）。
 * トクサネシティのダイゴの家（ホウエンのグループ 14 の 7 番＝この改造では 43 + 14 = 57 群の 7 番）の机のモンスターボールからもらうダンバルだけ、
 *   ・色違い   : ふだんの抽選（自然・かがやくおまもり）で当たっていなければ 1/BELDUM_GIFT_SHINY_ODDS で色違いに
 *   ・Moémon   : ふだんの当たり（自身の ID の判定）でなければ 1/BELDUM_GIFT_MOEMON_ODDS で当たりに
 *   ・ヌシの印 : 印が無いときだけ 1/BELDUM_GIFT_NUSHI_ODDS で付ける
 * の 3 つを別々に判定する。色違い・Moémon は自身の ID だけを替える（性格値は触らない・hack_wonder_trade.c と同じ HackMonId_SetParts）。
 * 場面の見分け: 場所（その地図）と元の種族がダンバルのときだけ（hack_gift_shiny.c の研究所の最初の 1 匹と同じ考え方・新しい旗は使わない）。
 * ヌシは「手持ちにヌシは 1 匹だけ」（FEAT-nushi-1-1・patches/0981）に従う: 手持ちにすでにヌシがいるときは印を付けない（まとめ 0.66.0・hack_delta.c の HackNushi_PartyHasNushi）。
 */

#define BELDUM_GIFT_MAP_GROUP 57   //43 + エメラルドのグループ 14（MossdeepCity_StevensHouse）
#define BELDUM_GIFT_MAP_NUM 7

bool8 HackBeldumGift_IsGift(u16 origSpecies)
{
	return origSpecies == SPECIES_BELDUM
		&& gSaveBlock1 != NULL
		&& gSaveBlock1->location.mapGroup == BELDUM_GIFT_MAP_GROUP
		&& gSaveBlock1->location.mapNum == BELDUM_GIFT_MAP_NUM;
}

void HackBeldumGift_Apply(struct Pokemon* mon, u16 origSpecies)
{
	struct BoxPokemon* box = (struct BoxPokemon*) mon;
	bool8 shiny, moemon, shiny0, moemon0, nushi = FALSE;

	if (!HackBeldumGift_IsGift(origSpecies))
		return;

	HackMonId_Ensure(box);
	shiny = shiny0 = HackMonId_IsShinyBox(box);
	moemon = moemon0 = HackMonId_IsMoemonHit(box);
	if (!shiny && Random() % BELDUM_GIFT_SHINY_ODDS == 0)
		shiny = TRUE;
	if (!moemon && Random() % BELDUM_GIFT_MOEMON_ODDS == 0)
		moemon = TRUE;
	if (shiny != shiny0 || moemon != moemon0)
		HackMonId_SetParts(box, shiny, moemon);

	if (HackKind_Get(box) == HACK_KIND_NONE && !HackNushi_PartyHasNushi(gPlayerParty, PARTY_SIZE) && Random() % BELDUM_GIFT_NUSHI_ODDS == 0)
	{
		HackKind_Set(box, HACK_KIND_NUSHI, 0);   //ヌシは値を使わない（hack_kind_roll.c の Mark と同じ）
		nushi = TRUE;
	}
	if (nushi)
		CalculateMonStats(mon);   //ヌシの能力の反映（印を付けたあとに計算し直す）
}
