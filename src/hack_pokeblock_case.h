#pragma once

//★ 2026-11-09 FEAT-pokeblock 段 2（patches/0978）: ポロックケースの画面（src/hack_pokeblock_case.c）。原文 pokeemerald include/pokeblock.h の定数。
//  使う側は defines.h（struct Pokeblock＝global.h・berry.h の FLAVOR_*）と hack_pokeblock.h の後に読む。
enum { PBLOCK_CLR_NONE, PBLOCK_CLR_RED, PBLOCK_CLR_BLUE, PBLOCK_CLR_PINK, PBLOCK_CLR_GREEN, PBLOCK_CLR_YELLOW, PBLOCK_CLR_PURPLE, PBLOCK_CLR_INDIGO,
	   PBLOCK_CLR_BROWN, PBLOCK_CLR_LITE_BLUE, PBLOCK_CLR_OLIVE, PBLOCK_CLR_GRAY, PBLOCK_CLR_BLACK, PBLOCK_CLR_WHITE, PBLOCK_CLR_GOLD };   //15 個（0 = 空き）
enum { PBLOCK_COLOR, PBLOCK_SPICY, PBLOCK_DRY, PBLOCK_SWEET, PBLOCK_BITTER, PBLOCK_SOUR, PBLOCK_FEEL };
enum { PBLOCK_CASE_FIELD, PBLOCK_CASE_BATTLE, PBLOCK_CASE_FEEDER, PBLOCK_CASE_GIVE };
#define PBC_RET_BAG 0                      //閉じたらバッグへ（CB2_BagMenuFromStartMenu）
#define PBC_RET_FIELD 1                    //閉じたらフィールドへ（レジスト＝Select から・CB2_ReturnToField）
#define PBC_RET_SCRIPT 2                   //閉じたら台本の続きへ（サファリのおき場・CB2_ReturnToFieldContinueScript・段 6）
#define PBC_RET_BATTLE 3                   //閉じたら戦闘の画面へ（素の ReshowBattleScreenAfterMenu・段 6）

void OpenPokeblockCase(u8 caseId, u8 returnKind);
void FieldUseFunc_PokeblockCase(u8 taskId);                   //道具表 ITEM_POKEBLOCK_CASE の fieldUseFunc
const u8* Pbc_GetRefusalText(void);                         //使えない版・レポート前・壊れのとき案内の文・使えるなら NULL（ミナモのロビーの台も使う）
void ResetPokeblockScrollPositions(void);
u8 CreatePokeblockCaseSprite(s16 x, s16 y, u8 subpriority);   //食べさせる演出（hack_pokeblock_feed.c）用。ケースの絵のスプライト
void PokeblockCase_LoadSpriteGfx(void);                       //その絵とパレットを読む
extern const u8* const gPokeblockNames[];
extern const s8 gPokeblockFlavorCompatibilityTable[];         //NUM_NATURES x FLAVOR_COUNT
s16 GetPokeblockData(const struct Pokeblock* pokeblock, u8 field);
u8 GetHighestPokeblocksFlavorLevel(const struct Pokeblock* pokeblock);
u8 GetPokeblocksFeel(const struct Pokeblock* pokeblock);
s16 PokeblockGetGain(u8 nature, const struct Pokeblock* pokeblock);
void PokeblockCopyName(const struct Pokeblock* pokeblock, u8* dest);
bool8 CopyMonFavoritePokeblockName(u8 nature, u8* dest);
u8 GetPokeblocksFlavor(const struct Pokeblock* pokeblock);
