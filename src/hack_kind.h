#pragma once
#include "../include/global.h"

struct CompressedPokemon;

/*
 * 個体の「印」（ヌシ・デルタ種・ダーク）の窓口（段 A1・FEAT-shadow-a1・patches/0958・設計 work/tmp/audit/SHADOW_S0_MEASURE.md 1 章 案 P）。
 * 個体の中の今まで使っていなかった 10 ビットに「種類 2 ビット＋値 8 ビット」を持つ（保存の大きさは増えない）。
 *   80 バイト形（手持ち・育て屋・箱から出した形）: BoxPokemon.unknown（+0x1E）の bit9〜10 = 種類 / substruct0.unknown（+0x2B）= 値
 *   箱の縮めた形（78 バイト）: ext（+0x3A）の bit9〜10 = 種類・bit11〜12 = 値の下位 2 ビット / ivHi[8]（+0x4D）の bit2〜7 = 値の上位 6 ビット
 * 門番: 種類が HACK_KIND_NONE のとき値は読まない・書かない・変換でも写さない（印の無い個体は今までと 1 ビットも変わらない）。
 *   値の意味は種類ごと（デルタ＝タイプ番号・ダーク＝心のゲージの割合 0〜255・ヌシ＝使わない）。段 A1 では窓口と保存だけ。
 * struct Pokemon は先頭が BoxPokemon と同じなので HackKind_Get／Value／Set には (struct BoxPokemon*) にキャストして渡す。
 */
enum
{
	HACK_KIND_NONE = 0,
	HACK_KIND_NUSHI = 1,
	HACK_KIND_DELTA = 2,
	HACK_KIND_DARK = 3,
};

//80 バイト形（struct Pokemon も同じ先頭）
u8 HackKind_Get(const struct BoxPokemon* mon);                       //HACK_KIND_*（0〜3）
u8 HackKind_Value(const struct BoxPokemon* mon);                     //値（種類が NONE なら 0）
void HackKind_Set(struct BoxPokemon* mon, u8 kind, u8 value);        //kind は 0〜3 に丸める・NONE なら値も消す
void HackKind_SetValue(struct BoxPokemon* mon, u8 value);            //種類が NONE のときは何もしない
void HackKind_Clear(struct BoxPokemon* mon);
bool8 HackKind_IsTradeRefused(const struct BoxPokemon* mon);         //交換（通信・ゲーム内の人・ネット・ミラクル）に出せない個体＝ダーク（段 I・patches/0964）

//箱の縮めた形（78 バイト）
u8 HackKind_CompGet(const struct CompressedPokemon* comp);
u8 HackKind_CompValue(const struct CompressedPokemon* comp);
void HackKind_CompSetValue(struct CompressedPokemon* comp, u8 value);  //値だけ書く（種類 NONE なら何もしない・段 G）

//src/pokemon_storage_system.c の変換 2 関数から（80 ⇄ 78 で印と値を運ぶ）
void HackKind_PackToComp(const struct BoxPokemon* box, struct CompressedPokemon* comp);        //ext と ivHi は呼ぶ前に作ってあること
void HackKind_UnpackFromComp(const struct CompressedPokemon* comp, struct BoxPokemon* box);    //box の unknown は ext & 0xE1FF を入れたあとに
