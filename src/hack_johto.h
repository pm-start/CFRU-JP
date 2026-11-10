#pragma once

/*
 * ジョウトの開閉の判定（FEAT-johto-j0・2026-11-09・patches/0986）。HACK_OMIT_JOHTO（コンパイル時）の置き換え＝実行時の判定 1 本。
 *   HackJohto_Open() が真のとき: 船・そらをとぶ・地続き・タウンマップ・バッジの数（32）がジョウトを含む。偽のとき（今・J7 まで）: 今までの「配る版」と同じ（ジョウトなし・バッジ 24・船は 3 択）。
 *   開く条件（栓を外したあと）: ストーリー（STORY 0x50C8＝シンオウのストーリー・hack_heroine.c と同じ定数）が 10 以上 ＋ 追加のセーブ領域が使える（ジョウトの変数の窓 0x5200〜 は追加の領域にしか無い）。
 *   ★ 栓: HACK_JOHTO_PLUG が 1 の間は何があっても偽（ユーザー決定 2026-11-09「omit は最後まで止める」）。**0.66.0 で 0 にした（ユーザー決定 2026-11-10「B でデモ版なんだから」＝ジョウトは S1 の入口まで・先は工事中のデモ版として開ける）**。戻すときはこの 1 行を 1 にする。検算 tools/rom/verify_johto_j0.py がこの値を守る。
 */
#define HACK_JOHTO_PLUG 0

#define HACK_SINNOH_STORY_VAR 0x50C8     /* シンオウのストーリー（tools/sinnoh/sinnoh_story.py）。10 が最大＝やりのはしらでアカギに勝った */
#define HACK_SINNOH_STORY_END 10

bool8 HackJohto_Open(void);
