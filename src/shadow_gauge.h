#pragma once
#include "defines.h"

/*
 * ダークの心のゲージと、ダーク技の「組み立て」（段 E・patches/0963・FEAT-shadow-e）。設計 work/tmp/audit/SHADOW_S1_PLAN.md（E の章・N8）。
 * ゲージ＝個体の印（HackKind・種類 DARK）の値 8 ビット（割合・255 が満タン）。ポケモン XD の「5 本のバー」を 255 の割合で持つ。
 * ダーク技は個体の技欄に書かない。戦闘の技の並び（gBattleMons）と つよさの画面のたびに Shadow_BuildMoveList で並びを作る（手持ちの技・PP は戦闘のあとも元のまま）。
 * このファイルは ROM の外の部品に頼らない（検算 verify_shadow_gauge がホストでそのまま動かす）。
 */
#define SHADOW_GAUGE_FULL 255
#define SHADOW_GAUGE_STAGES 5            //バーの本数（満タン 5 → 空 0）
#define SHADOW_DARK_MOVES_MAX 3

//ゲージを減らす動作（XD の表の列）。段 E では BATTLE だけ使う（CALL・WALK・SCENT は後続の段）
enum
{
	SHADOW_ACTION_BATTLE = 0,
	SHADOW_ACTION_CALL = 1,
	SHADOW_ACTION_WALK = 2,
	SHADOW_ACTION_SCENT = 3,
	SHADOW_ACTION_COUNT = 4,
};

//並びの src の意味
#define SHADOW_SRC_DARK 0xFF             //ダーク技
#define SHADOW_SRC_SEALED 0xFD           //まだ開いていない元の技（つよさの画面だけ「？？？」の行にする・技は MOVE_NONE）
#define SHADOW_SRC_NONE 0xFE             //空き

u8 Shadow_GaugeStage(u8 gauge);                       //バーの残り 5（満タン）〜 0（空＝リライブできる）。少しでも残っていればその 1 本は数える
u8 Shadow_BarsCleared(u8 gauge);                      //開いたバーの本数 0〜5
bool8 Shadow_OwnMoveOpen(u8 ownIndex, u8 gauge);      //元の技（技欄の順に数えて 0 始まり）が使えるか。1 つめ＝1 本開いたら・2 つめ＝3 本開いたら・残りはリライブまで出ない
u8 Shadow_GaugeAmount(u8 nature, u8 action);          //減る量（255 の割合・各 1 以上）
u8 Shadow_GaugeAfter(u8 gauge, u8 nature, u8 action); //減らしたあとのゲージ（0 未満にならない）

//リライブボックスの歩数（段 G）: 128 歩ごとに 1 回ゲージを減らす。steps は SaveBlock1 の 1 バイト（0〜127・それ以上は壊れた値として 127 と見なす）
#define SHADOW_RELIVE_STEPS 128
u8 Shadow_StepAdvance(u8 steps, bool8* fire);         //1 歩進める。128 歩目で *fire = TRUE・0 に戻す。戻り値＝次の歩数
bool8 Shadow_IsOpen(u8 gauge);                        //ゲージが 0（心が全部開いた＝リライブできる）

//ダーク技の自動割り当て（仮の規則・タイプと種族値の合計で決める）。out に 2〜3 個入れて個数を返す
u8 Shadow_AutoMoves(u8 type1, u8 type2, u16 atk, u16 spatk, u16 bst, u16* out);

//並びを作る。own＝元の技 4 つ・moves／src＝4 つ。forSummary が 0 なら開いていない元の技は並びに出さない（戦闘）・1 なら つよさの画面の「？？？」の行（SHADOW_SRC_SEALED）として続ける。
//返り値＝本物の技の行数（ダーク技＋開いた元の技・4 まで）。src は 0〜3＝元の技の枠・SHADOW_SRC_DARK・SHADOW_SRC_SEALED・SHADOW_SRC_NONE
u8 Shadow_BuildMoveList(u8 gauge, const u16* dark, u8 darkCount, const u16* own, bool8 forSummary, u16* moves, u8* src);
