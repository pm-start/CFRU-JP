#pragma once

/*
 * ジョウト J0（FEAT-johto-j0・2026-11-09）: ジョウト用の旗と変数の置き場。設計の記録は docs/claude/06_risks.md 6.141・patches/APPLIED.md 0986。
 *
 * 旗（一般・トレーナー共通の窓）: 0x1900〜0x1AFF（512 個）＝SaveBlock1 +0x3A94〜+0x3AD3（global.h の filler_3A94・64 バイト・本体の組の中＝レポートで手持ちと一緒に保存）。
 *   0x1900〜0x19FF … トレーナー（番号 1792〜2047・HoennTrainerFlag の式・src/hoenn_trainer_flags.c）
 *   0x1A00〜0x1AFF … ジョウトの一般の旗
 *   GetExpandedFlagPointer（src/save.c）がこの範囲を SaveBlock1 の中へ向ける。FlagGet/FlagSet・スクリプトの setflag/checkflag は同じ道。
 * 変数: 0x5200〜0x52FF（u16 × 256）。GetExpandedVarPointer が、RAM の写し（EWRAM JOHTO_VAR_RAM_ADDR）へ向ける。
 *   保存は追加のセーブ領域の 2 区画（JOHTO_VAR_SECTOR と +1・交互）。状態は SaveBlock1 に置かず、各区画の頭（世代・主人公の ID・和）に持つ
 *   （決まり: 以後の機能の状態は追加区画の頭・SaveBlock1 の filler は未マージの枝が使い切っている）。読み込みは両方の頭を見て、
 *   主人公の ID が合い・和が合う区画のうち世代の新しいほうを正とする。書くのは「レポートが成功したあと」（HackJohtoVar_AfterSave）で、正でない区画へ。
 *   ★ FEAT-johto-p1（2026-11-10）旗の窓の拡張: 一般の旗 0x1B00〜0x1CFF（512 個・64 バイト）も同じ区画（変数の後ろ＝区画の +16+512）に持つ。変数と旗は同じ書き込みで一緒に切り替わる（ずれない）。
 *   ブロックの版 1→2（版 1＝旗なし・和は変数だけ・読めば旗は全部 0）。EWRAM の写しは 16+512+64 バイト（0x0203EA00〜0x0203EC4F）。窓は追加の領域が使えなければ無い（読めば 0・書きは捨てる）。
 *   追加の領域が使えない環境では窓は無い（読めば 0・書いても捨てる）＝ジョウトは開かない（HackJohto_Open が偽）。
 *   読み込み／最後の書き込みから変わっていなければ書かない（読むだけでは書かない・全部 0 のままなら区画も書かない）。
 */

#define HACK_JOHTO_VERSION    0x4200   /* ジョウトの変数・旗の区画（95・96）を書く最初の版（0.66.0・リボン HACK_RIBBON_VERSION・コンディション HACK_CONDITION_VERSION と同じ）。これより古い版は表を読まない・古い版が表つきのセーブを「新しすぎる」と止める */
#define JOHTO_FLAG_FIRST      0x1900
#define JOHTO_FLAG_LAST       0x1AFF
#define JOHTO_FLAG_SB1_OFF    0x3A94
#define JOHTO_FLAG_BYTES      0x40
#define JOHTO2_FLAG_FIRST     0x1B00      /* 一般の旗の窓 2（追加の区画の中・変数の後ろ） */
#define JOHTO2_FLAG_LAST      0x1CFF
#define JOHTO2_FLAG_BYTES     0x40
#define JOHTO_VAR_FIRST       0x5200
#define JOHTO_VAR_COUNT       256
#ifndef JOHTO_VAR_RAM_ADDR   /* ホストの検算が差し替える。ゲームでは下の番地 */
#define JOHTO_VAR_RAM_ADDR    0x0203EA00   /* EWRAM 0x0203EA00〜0x0203EC4F（16 バイトのヘッダ + 変数 512 バイト + 旗 64 バイト・Commander が全 task 枝で空きを確認 2026-11-09）。空き 0x0203E800〜0x0203EF7F の中・BGM 0x0203E300〜E800・RAM チートの影 0x0203EF80〜・連れ歩きの覚え 0x0203EFC0〜の外 */
#endif

struct JohtoVarBlock         /* 追加の区画の先頭から（残りは 0）・16 バイトのヘッダ */
{
	u32 magic;               /* 'JVB1' */
	u16 version;
	u16 gen;                 /* 書くたび +1（大きいほうが新しい・16 ビットで回っても差で比べる） */
	u32 trainerId;           /* 主人公の ID（はじめからで同じ区画を見ても 0 から始まる） */
	u16 sum;                 /* 版 1: 変数 256 個の 16 ビットの印／版 2: 変数と旗 64 バイトの 16 ビットの印 */
	u16 reserved;
	u16 vars[JOHTO_VAR_COUNT];
	u8 flags[JOHTO2_FLAG_BYTES];   /* 版 2 から（版 1 の区画ではここは 0）。旗 0x1B00〜0x1CFF */
};

u8* HackJohto_FlagPtr(u16 id);                  /* 0x1900〜0x1AFF の旗が入っているバイト（範囲の外は NULL）。ビットは id & 7 */
u8* HackJohto_Flag2Ptr(u16 id);                 /* 0x1B00〜0x1CFF の旗が入っているバイト（範囲の外は NULL・追加の領域が使えなければ NULL＝読めば 0・書きは捨てる）。ビットは id & 7 */
u16* HackJohto_VarPtr(u16 id);                  /* 0x5200〜0x52FF の変数。追加の領域が使えなければ (u16*) 1（＝0 を返し書きは捨てる・GetExpandedVarPointer の約束）。範囲の外は NULL */
void HackJohtoVar_AfterSave(bool8 ok);          /* レポートが成功したあと: 変えた写しがあれば正でない区画へ書く（書けなければ次のレポートでまた試す） */
void HackJohtoVar_OnLoad(void);                 /* つづきから: RAM の写しを捨てる（次に触れたときに読み直す） */
void HackJohtoVar_NewGame(void);                /* はじめから: RAM の写しを捨てる */
