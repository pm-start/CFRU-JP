#pragma once
#include "../include/global.h"

struct Pokemon;

/* ダイゴの家（トクサネシティ）のダンバルの贈り物だけ、色違い・Moémon・ヌシの 3 つが出る確率を上げる（FEAT-beldum-gift・patches 0979）。
 * 呼び元は src/build_pokemon.c の ScriptGiveMon の 1 か所（HealMon の前）。それ以外のポケモン・野生・ほかの贈り物には何もしない。 */

//確率の分母（1/N・3 つは別々に判定する・ユーザー決定 2026-11-09「ダンバル全部1/16で」）。変えるのはここの 3 つだけ
#define BELDUM_GIFT_SHINY_ODDS 16     //色違い（ふだんの抽選で当たっていればそのまま）
#define BELDUM_GIFT_MOEMON_ODDS 4     //Moémon の姿の上乗せ（ふだんの 1/16 で当たっていればそのまま・合計 1-(15/16)(3/4)＝約 29.7%）
#define BELDUM_GIFT_NUSHI_ODDS 16     //ヌシの印（まだ印が無いときだけ）

//もらう子（元の種族 origSpecies・しばりで入れ替わる前）が、この贈り物の場面か
bool8 HackBeldumGift_IsGift(u16 origSpecies);
//贈り物の場面なら 3 つの抽選をして mon に反映する（場面でなければ何もしない）
void HackBeldumGift_Apply(struct Pokemon* mon, u16 origSpecies);
