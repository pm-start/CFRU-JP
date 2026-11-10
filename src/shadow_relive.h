#pragma once
#include "defines.h"

/*
 * リライブボックスとリライブ（段 G・patches/0965・FEAT-shadow-g）。設計 work/tmp/audit/SHADOW_S1_PLAN.md（G の章）・DARK_DESIGN.md（6・7 章）。
 *  - リライブボックス＝追加のセーブ領域の専用の箱（src/hack_relive_box.c・30 匹・画面は べんりきのう）。★ 2026-11-09 FEAT-relive-box-ext（patches/0975・案 B）:
 *    0.65.0 の「PC の箱 26 を専用にする」はやめた。ダークの子は PC のふつうの箱には預けられない（Relive_BoxAccepts は全部の箱でダークを断る）。追加の領域が使えない版では専用の箱が使えない。
 *    0.65.0 のセーブで箱 26（RELIVE_LEGACY_BOX_ID）にいるダークは、つづきから後の最初の 128 歩（心のゲージの刻みと同じ歩数）で専用の箱へ移す（入りきらない・書けないときは箱 26 に残す＝消さない）。
 *  - 専用の箱に入れておくと 128 歩ごとに心のゲージが 1 刻み（SHADOW_ACTION_WALK）下がる。歩数は SaveBlock1 +0x3A20 の 1 バイト。
 *  - ゲージが 0 になった子は手持ちに連れてきて、リライブ（印を消す＝ダークでなくなる。ためていた経験値は無い・ダーク技は組み立て方式なので戻す処理も無い）。
 *  - かおり 3 種（ルンルン・ワクワク・ドキドキ）はバッグから使うとゲージが下がる（SHADOW_ACTION_SCENT・強さ 1〜3 倍）。
 *  - スナッチボールの入手: 初回 5 個・以降は所持 5 個未満なら 5 個まで補充（店では売らない）。
 * 場所（看護師の台本・仮）に依らない処理は全部この C の special に置く。台本は assembly/hack_shadow_relive/relive_scripts.s。
 */
#define RELIVE_LEGACY_BOX_ID 25     //0.65.0 で専用だった PC の箱（0 始まり＝26 番目）。今はふつうの箱。残っているダークを専用の箱へ移すときと、名前を戻すときだけ見る
#define RELIVE_SB1_OFF 0x3A20       //SaveBlock1: +0 = 歩数 0〜127・+1 = スナッチボールの初回の贈り物を渡した印（bit0）
#define RELIVE_SB1_FLAGS_OFF 0x3A21
#define RELIVE_FLAG_GIFT_DONE 1     //+0x3A21 bit0: スナッチボールの初回の贈り物を渡した
#define RELIVE_FLAG_DARK_MET 2      //+0x3A21 bit1: 相手にダークが出た戦闘が終わった（初回の贈り物はこのあと次に看護師と話したとき）
#define RELIVE_SNATCH_STOCK 5       //スナッチボールの補充の上限（初回もこの数）

struct Pokemon;
struct BoxPokemon;
struct CompressedPokemon;

//預ける規則
bool8 Relive_IsDark(const struct BoxPokemon* mon);
//ダークは PC のどの箱にも預けられない（専用の箱は追加の領域・hack_relive_box.c）・ダーク以外はどの箱にも入る。ホストで箱の棚（hack_box_shelf.c）だけを動かす検算が この 1 つのヘッダだけ足せば済むよう static inline にしてある。
//印の読み方は src/hack_kind.h の「80 バイト形: +0x1E（unknown）の bit9〜10 = 種類」と同じ（HACK_KIND_DARK = 3・検算 verify_shadow_relive が HackKind_Get と全種類で突き合わせる）
static inline bool8 Relive_BoxAccepts(u8 boxId, const struct BoxPokemon* mon)
{
	const u8* b = (const u8*) mon;
	bool8 dark = (((b[0x1E] | (b[0x1F] << 8)) >> 9) & 3) == 3;

	(void) boxId;
	return !dark;
}
bool8 Relive_HasRoom(void);                                         //専用の箱が使えて空きがあるか（追加の領域なし・書けない版・満杯は FALSE）
bool8 Relive_PutComp(const struct CompressedPokemon* comp, u8* slotOut);   //ダークの子（縮めた形）を専用の箱へ入れて書く。FALSE = 入らない

//箱の縮めた形（78 バイト）がダークか。印は ext（+0x3A）の bit9〜10 = 3（src/hack_kind.h と同じ・検算 verify_shadow_relive が HackKind_CompGet と全種類で突き合わせる）
static inline bool8 Relive_CompIsDark(const struct CompressedPokemon* comp)
{
	const u8* b = (const u8*) comp;

	return (((b[0x3A] | (b[0x3B] << 8)) >> 9) & 3) == 3;
}

//歩数（overworld.c の TryStartStepCountScript から 1 歩ごと）
void Relive_OnStep(void);

//かおり（party_menu.c から）
u8 Relive_ScentStrength(u16 item);                                  //かおり 1〜3・かおりでなければ 0
bool8 Relive_UseScent(struct Pokemon* mon, u16 item);               //使えたら TRUE（ダークでゲージが残っているときだけ）

//台本から（special／callasm）
void Relive_ScriptPrepare(void);          //LASTRESULT: 0 = ふつうのポケセン／1 = リライブの話をする（手持ちにダーク・または初回の贈り物がまだ）
void Relive_ScriptReadyCount(void);       //LASTRESULT = 手持ちのゲージ 0 のダークの数（gStringVar1 にも）
void Relive_ScriptPurifyNext(void);       //手持ちのゲージ 0 のダークを 1 匹リライブ。LASTRESULT = 1 したら gStringVar1 に名前・0 = もういない
void Relive_ScriptGiveSnatch(void);       //スナッチボールを足りない分だけ。LASTRESULT = 渡した数（0 = 足りている）・0xFFFF = バッグがいっぱい。gStringVar1 に数
void Relive_ScriptGiftDone(void);         //初回の贈り物を渡した印を立てる
void Relive_NoteDarkMet(void);            //戦闘（shadow_battle.c）から: 相手にダークが出た戦闘が終わった印を立てる
void Relive_ScriptGiftNeeded(void);       //LASTRESULT: 1 = 初回の贈り物がまだ
