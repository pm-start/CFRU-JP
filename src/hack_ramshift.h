/* 生成物（tools/rom/build_ramshift.py・元は tools/patch/make_demo_bundle.py の VERSION）。手で直さない */
#pragma once
/* SaveBlock2・SaveBlock1・PokemonStorage の RAM の位置（版ごとに 0〜0x7C ずらす・patches/0622・CLAUDE.md 6.102）。
   SB1・SB2 は gSaveBlock1/gSaveBlock2（ポインタ）で読むこと。PokemonStorage の中を固定番地で触るときだけ HACK_STORAGE_BASE */
#define HACK_SB2_SHIFT 0x14   /* 下駄。実際の位置は地図の読み込みごとにこれ + 乱数 0〜0x7C */
#define HACK_SB1_SHIFT 0x40
#define HACK_STORAGE_SHIFT 0x20
#define HACK_STORAGE_BASE 0x0202926C
