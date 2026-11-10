#pragma once
#include "../include/global.h"

/*
    使用禁止エリア（BUG-yuuran-safari・Redmine #192・patches/0922 → 0931 で ゆうらん だけに縮めた）。
    サファリゾーンの中で ゆうらん（Fly Reloaded の独自の移動）を「使えない」と止める。サファリの中は地図の種別が道路（屋外）なので
    ゆうらんが素の判定を通り、サファリの旗が消えてサファリの地図の中で「ふつうの戦闘」になっていた。
    そらをとぶ・テレポートは原作でも使える（出た後に旗が消える＝サファリが終わる）ので禁止しない（ユーザー 2026-10-28・
    参考/pokefirered/src/overworld.c:289-314・参考/pokeemerald/src/overworld.c:368-387）。
    呼ぶのは tools/fly_reloaded/source/src/cfru_menu.c の FlyCfruFreeCommand だけ（NativeHackFlyAreaForbidden）。
    tools/rom/verify_fly_forbid.py が表・呼び出しを ROM／ソースと突き合わせる。
*/
bool8 HackFly_InSafariMap(void);       /* 今の地図がサファリゾーン（カントー 群1 の 63〜71・ホウエン 群69 の 0〜3・11〜13） */
bool8 HackFly_AreaForbidden(void);     /* ゆうらんの禁止か: サファリの旗が立っている／サファリの地図の中 */
void HackSafariGuardTick(void);        /* 毎フレーム（hoenn_steps.c の HoennStepTick）: すでに壊れた状態（サファリの地図にいるのに旗が無い）を直す */
