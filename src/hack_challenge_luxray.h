#pragma once
#include "../include/global.h"

/*
    しばり「ぜんぶレントラー」（#140 仮・2026-10-18・docs/research/2026-10-18_challenge-luxray-syakakiri.md）。せっていの「しばり」フォルダの 1 つ（既定は OFF）。

    手に入れたポケモンは ぜんぶ レントラー♀・色違い・Moémon になる（捕まえた子・もらった子・タマゴがかえった子・交換でもらった子）。
    レベル・性格・個体値・努力値・なつき度・ボール・親／出会いの記録・もたせた道具・ニックネーム（種族名のままなら新しい種族名）はそのまま。
    特性は 特性の番号（1 つ目／2 つ目／かくれ）をそのまま（レントラーにその番号が無ければ 1 つ目）。技は 今のレベルで覚えている技（最後の 4 つ）に入れ直す。
    図鑑: もとの種族は「見た」・レントラーは「つかまえた」。対象外: バトル施設／フロンティアの借り物・ポケセン 3F のダミー戦・？？？・相手側・通信対戦。

    旗は拡張フラグ（0x900〜0x18FF＝通常のレポートで保存）。台帳: ふういんルーレット（hack_fuuin.h・0xFC5）・ダミー戦（hack_dummy_battle.h・0xFC6）の次 → **0xFC7（ここ）** → フロンティアの金の前口上の印 0xFC8〜0xFCE（hoenn_tower.c・build_hoenn_scripts.LEDGER_FLAGS）→ 次の空きは 0xFD0（0xFCF は hack_tamper.h＝セーブの改造ありの印）。
    変数・セーブの欄は増やさない（旗 1 本だけ）。
*/
#define HACK_FLAG_CHE_LUXRAY     0xFC7   /* ぜんぶレントラー: 手に入れたポケモンは ぜんぶ レントラー♀・色違い・Moémon */
#define HACK_CHE_COUNT           1       /* せっていの行になる 縛り E の数（最後のバッジの数え方） */

bool8 HackChallengeLuxray_On(void);                                     /* 旗が立っているか（最後のバッジの数え方・AnyChallengeOn） */
bool8 HackChallengeLuxray_OnObtain(struct Pokemon* mon, bool8 markDex); /* 手に入れた子を 変える（変えたら TRUE）。markDex = レントラーを「つかまえた」にもする（捕獲の図鑑の判定より前は FALSE） */
void HackLuxray_ApplyIdentity(struct Pokemon* mon);                     /* src/hack_starter_luxray.c: ♀・色違い・Moémon の 1 本（最初のポケモンと共有） */

/* リルの あいじょう（ITEM_RIRU_LOVE 0x366・patches/0617・src/hack_luxray_love.c）: ぜんぶレントラーのレントラーの固定の持ち物・買った道具の置き換え先 */
bool8 HackLuxrayLove_IsFixed(const struct Pokemon* mon);   /* しばり ON で リルの あいじょうを持つ子か（はずせない・替えられない） */
void HackLuxrayLove_Apply(struct Pokemon* mon);            /* もとの持ち物をバッグ（いっぱいならパソコン）へ移して リルの あいじょうを持たせる */
u16 HackLuxrayLove_ShopItem(u16 item);                     /* お店・お母さん・じっせきで受け取る道具（ON のあいだ モンスターボール以外は リルの あいじょう） */
bool8 HackLuxrayLove_IsSelectedMonNotEgg(u8* slotPtr);     /* functionrewrites 0812054C: バッグの「もたせる」で固定の子を選べない */
bool8 HackMomSavings_AddPcItem(u16 item);                  /* src/hack_mom_savings.c: パソコンのどうぐへ 1 つ入れる（入らなければ FALSE） */
