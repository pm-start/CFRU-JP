#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 4b1（patches/0978）: きのみブレンダーの画面（src/hack_berry_blender.c・原文 pokeemerald berry_blender.c）。
//  4b1 = 画面・矢印・きのみの投げ入れ・カウントダウン・プレイヤーの入力・進行バー・RPM。NPC の入力・結果・ランキング・ファンファーレは 4b2。
//  使う側は defines.h の後に読む。状態の入れ物は Alloc＋ポインタの語 0x203E038（ケース・あげる画面・演出と共用＝大きな画面は同時に開かない）。
#define BBL_STATE_PTR 0x203E038

//ブレンダーを始める。opponents = NPC の数（1〜3）・master = 名人（opponents が 1 のとき）・exitCallback = 終わったあとの戻り先。画面を暗くした状態で呼ぶ（原文は台本が fadescreen してから special を呼ぶ）。
//  今は debug の口（src/hack_debug_menu.c）からだけ呼ぶ。special 登録は段 5。
void HackBlender_Start(u8 opponents, bool8 master, MainCallback exitCallback);
