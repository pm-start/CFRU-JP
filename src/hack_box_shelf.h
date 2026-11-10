#pragma once

//ボックスの仮想化（src/hack_box_shelf.c・docs/research/2026-10-06_box-virtualization-design.md）
s32 BoxShelf_ShelfIndex(u8 boxId);
struct CompressedPokemon* BoxShelf_BoxMons(u8 boxId, bool8 forWrite);
struct CompressedPokemon* BoxShelf_PeekBoxMons(u8 boxId);   //読み込まずに見る（#70）
void BoxShelf_ReadMon(u8 boxId, u8 pos, struct CompressedPokemon* out);
bool8 BoxShelf_ReadBoxMons(u8 boxId, u8 first, u8 n, struct CompressedPokemon* out);   //箱の first 匹目から n 匹を続けて読む（0.63.1 Moémon の世代の控え・入れ物を動かさない）
void BoxShelf_MarkPtrDirty(const void* ptr);
u32 BoxShelf_DirtyPageCount(void);
bool8 BoxShelf_CanTouchBox(u8 boxId);
u8 BoxShelf_VisibleBoxes(void);   //FEAT-box100-unlock: 見える箱の数（50 か 100・箱の切り替えなどの折り返し用）
u8 BoxShelf_WrapNextBox(u8 box);   //次の箱へ進めた値を見える箱の数で折り返す（asm の hooks から）
bool8 BoxShelf_ExtAvailable(void);    //追加の領域が使えるか（使えると分かったら覚える・HackExtSave_Available を毎回呼ばない・patches/0741）
s32 BoxShelf_FindEmptyPos(u8 boxId);   //箱の最初の空きの位置（無ければ -1・入れ物を動かさない・patches/0732）
void BoxShelf_NewGame(void);
bool8 BoxShelf_IsOldFormat(void);   //続きからで見たセーブが古い形（'SHEL'・58 バイト）か（止める画面用・L 案）
void BoxShelf_OnLoad(void);
bool8 BoxShelf_TamperSeen(void);    //★ 2026-10-21 続きからで読んだページに MAC が合わないものがあったか
void BoxShelf_Commit(void);
void BoxShelf_AfterSave(bool8 ok);

//FEAT-box100 (0.61.0 patches/0913): 51-100 box names(9B x50)+wallpapers(1B x50) RAM offset from PokemonStorage (after ShelfCache, before HFR copy +0x7E88)
#define BOX2_NAMES_OFF 0x7C00
