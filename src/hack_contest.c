#include "defines.h"
#include "hack_contest.h"
#include "../include/malloc.h"
#include "defines_battle.h"
#include "../include/battle.h"
#include "../include/palette.h"
#include "../include/decompress.h"
#include "../include/battle_anim.h"
#include "hack_contest_addr.h"

/* コンテスト移植 S1（FEAT-contest・patches/0985）。説明は hack_contest.h。
 * 書き換え可能な static は使えない（6.13）＝状態は固定 RAM の語（HACK_CONTEST_PTR）とそのさきの構造体だけ。 */

//素の IsContest（0x08072F2C）の 110 か所の呼び元の付け替え先（hooks: 素の Unused_ResetBgControlStruct 0x0800108C の入口）。引数なし・戻り値 bool8
bool8 HackContest_IsContest(void)
{
	return HACK_CONTEST_PTR != NULL;
}

//素の GetBattlerSpriteBGPriorityRank（JP 0x08076054・pokefirered battle_anim_mons.c:1934・monbg が BG1／BG2 のどちらに絵を写すかを決める）の置き換え（hooks）。
//  原文エメラルド（pokeemerald battle_anim_mons.c:2075）は IsContest のとき 1（BG1）を返す。FR は位置だけで決めるので、会場の攻撃側（位置 3）が 2（BG2＝会場の枠の層）になり
//  へんしん・おんがえしなどの monbg が枠の層を壊す（S2c の実機で確認）。コンテスト中は 1、それ以外は素のとおり（位置 0 か 3 なら 2・それ以外 1）。
u8 __attribute__((long_call)) GetBattlerPosition(u8 battlerId);
u8 HackContest_GetBattlerSpriteBGPriorityRank(u8 battlerId)
{
	u8 position;

	if (HACK_CONTEST_PTR != NULL)
		return 1;
	position = GetBattlerPosition(battlerId);
	if (position == 0 || position == 3)
		return 2;
	return 1;
}

//素の LoadMoveBg（JP 0x080730C8・pokefirered battle_anim.c:1102・Task_FadeToBg／Cmd_changebg が背景を書く）の置き換え（hooks・S2d）。
//  FR は背景を地図 26・charblock 2 に書く＝会場では BG1（箱の絵の層・charblock 2）と BG3（観客の層・地図 26）を壊す。原文エメラルド（pokeemerald battle_anim.c:1185）は
//  コンテスト中だけ「地図は gDecompressionBuffer に展開して パレット番号 1・タイル +0x100 に直し BG3 の地図 26 へ・絵は BG_SCREEN_ADDR(4)（charblock 0 の後ろ半分）・パレット 1」に書く。
//  コンテスト外は素のとおり（地図 26・charblock 2・パレット 2）。gAnimationBackgrounds は CFRU が増やした表（素の参照語 0x08073104 を repoints で付け替え済み）
extern const struct BattleAnimBackground gAnimationBackgrounds[];
void HackContest_LoadMoveBg(u16 bgId)
{
	if (HACK_CONTEST_PTR != NULL)
	{
		LZ77UnCompWram(gAnimationBackgrounds[bgId].tilemap, gDecompressionBuffer);
		RelocateBattleBgPal(1, (u16*) gDecompressionBuffer, 0x100, 0);
		CpuCopy32(gDecompressionBuffer, (void*) BG_SCREEN_ADDR(26), 0x800);
		LZDecompressVram(gAnimationBackgrounds[bgId].image, (void*) BG_SCREEN_ADDR(4));
		LoadCompressedPalette(gAnimationBackgrounds[bgId].palette, 16 * 1, 0x20);
	}
	else
	{
		LZDecompressVram(gAnimationBackgrounds[bgId].tilemap, (void*) BG_SCREEN_ADDR(26));
		LZDecompressVram(gAnimationBackgrounds[bgId].image, (void*) BG_CHAR_ADDR(2));
		LoadCompressedPalette(gAnimationBackgrounds[bgId].palette, 16 * 2, 0x20);
	}
}

//素の LoadDefaultBg（JP 0x08073110・pokefirered battle_anim.c:1209・restorebg が背景を戦闘の背景に戻す）の置き換え（hooks・S2d）。
//  原文エメラルド（pokeemerald battle_anim.c:1209）はコンテスト中は LoadContestBgAfterMoveAnim（会場の絵・パレットを書き直す）。それ以外は素のとおり DrawMainBattleBackground
void HackContest_LoadContestBgAfterMoveAnim(void);
void __attribute__((long_call)) DrawMainBattleBackground(void);   //素の ROM の関数へは long_call（include/battle.h の宣言は BL のまま＝遠い置き場から届かない）
void HackContest_LoadDefaultBg(void)
{
	if (HACK_CONTEST_PTR != NULL)
		HackContest_LoadContestBgAfterMoveAnim();
	else
		DrawMainBattleBackground();
}

//素の GetBattlerSpriteBGPriority（JP 0x08076028・pokefirered battle_anim_mons.c:1924・BG の priority 値を返す。S2d で hooks）の置き換え。
//  原文エメラルド（pokeemerald battle_anim_mons.c:2063）は IsContest のとき 2 を返す。それ以外は素のとおり（位置が 0 か 3 なら BG2・それ以外 BG1 の priority）。
//  GetAnimBgAttribute は素の JP 0x080BD5CC（BPRJ.ld・素の GetBattlerSpriteBGPriority の BL の飛び先）。BG_ANIM_PRIORITY＝4
s32 __attribute__((long_call)) GetAnimBgAttribute(u8 bgId, u8 attributeId);
u8 HackContest_GetBattlerSpriteBGPriority(u8 battlerId)
{
	u8 position = GetBattlerPosition(battlerId);

	if (HACK_CONTEST_PTR != NULL)
		return 2;
	if (position == 0 || position == 3)
		return GetAnimBgAttribute(2, 4);
	return GetAnimBgAttribute(1, 4);
}

//素の GetBattleAnimBg1Data／GetBattleAnimBgData／GetBattleAnimBgDataByPriorityRank（JP 0x08074A44／0x08074A6C／0x08074AA4・pokefirered battle_anim_mons.c:866〜913・monbg・へんしんなどが使う BG の絵・地図・パレットの置き場を返す。S2d で hooks）の置き換え。
//  原文エメラルド（pokeemerald battle_anim_mons.c:911〜1000）は IsContest のとき パレット 14（BG_ANIM_PAL_CONTEST）・bgId 1・tilesOffset 0 を返す（へんしんの GetBgDataForTransform も同じ）。
//  FR は 8／1／0x200 で、会場では絵が BG1 の charblock 2 の 0x4000 先＝0xC000＝BG0（窓の層）の地図に重なって縞になる（S2d の実機で確認）。コンテスト外は素のとおり。
#define HC_BG_ANIM_PAL_1 8
#define HC_BG_ANIM_PAL_2 9
#define HC_BG_ANIM_PAL_CONTEST 14
void HackContest_GetBattleAnimBg1Data(struct BattleAnimBgData* d)
{
	d->bgTiles = (u8*) ContestFr_gBattleAnimBgTileBuffer;
	d->bgTilemap = (u16*) ContestFr_gBattleAnimBgTilemapBuffer;
	if (HACK_CONTEST_PTR != NULL)
	{
		d->paletteId = HC_BG_ANIM_PAL_CONTEST;
		d->bgId = 1;
		d->tilesOffset = 0;
	}
	else
	{
		d->paletteId = HC_BG_ANIM_PAL_1;
		d->bgId = 1;
		d->tilesOffset = 0x200;
	}
	d->unused = 0;
}

void HackContest_GetBattleAnimBgData(struct BattleAnimBgData* d, u32 bgId)
{
	if (HACK_CONTEST_PTR != NULL || bgId == 1)
	{
		HackContest_GetBattleAnimBg1Data(d);
		return;
	}
	d->bgTiles = (u8*) ContestFr_gBattleAnimBgTileBuffer;
	d->bgTilemap = (u16*) ContestFr_gBattleAnimBgTilemapBuffer;
	d->paletteId = HC_BG_ANIM_PAL_2;
	d->bgId = 2;
	d->tilesOffset = 0x300;
	d->unused = 0;
}

void HackContest_GetBattleAnimBgDataByPriorityRank(struct BattleAnimBgData* d, u8 unused)
{
	(void) unused;
	d->bgTiles = (u8*) ContestFr_gBattleAnimBgTileBuffer;
	d->bgTilemap = (u16*) ContestFr_gBattleAnimBgTilemapBuffer;
	if (HACK_CONTEST_PTR != NULL)
	{
		d->paletteId = HC_BG_ANIM_PAL_CONTEST;
		d->bgId = 1;
		d->tilesOffset = 0;
	}
	else if (HackContest_GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker) == 1)
	{
		d->paletteId = HC_BG_ANIM_PAL_1;
		d->bgId = 1;
		d->tilesOffset = 0x200;
	}
	else
	{
		d->paletteId = HC_BG_ANIM_PAL_2;
		d->bgId = 2;
		d->tilesOffset = 0x300;
	}
	d->unused = 0;
}

bool8 HackContest_Open(void)
{
	struct HackContestState* s;

	if (HACK_CONTEST_PTR != NULL)
		return FALSE;
	s = (struct HackContestState*) AllocZeroed(sizeof(struct HackContestState));
	if (s == NULL)
		return FALSE;
	s->magic = HACK_CONTEST_MAGIC;
	HACK_CONTEST_PTR = s;
	return TRUE;
}

void HackContest_Close(void)
{
	struct HackContestState* s = HACK_CONTEST_PTR;

	if (s == NULL)
		return;
	HACK_CONTEST_PTR = NULL;
	Free(s);
}

void HackContest_Reset(void)
{
	HACK_CONTEST_PTR = NULL;
	HACK_CONTEST_KEEP.screen = NULL; //S4b: 結果発表の作業領域の語も（ヒープが消えたあとに残さない）
}

//素の GetBattleBgPaletteNum（JP 0x08074BF8・FR は常に 2・pokeemerald battle_anim_mons.c:1024 は IsContest なら 1）の呼び元 BL 2 か所の付け替え先（S3・hooks の踏み台＋bytereplacement の CONTEST-PAL）。
//  呼び元＝サイコキネシスの背景の色回し（SetPsychicBackground_Step）と FadeScreenToWhite_Step。会場の背景はパレット 1（S2d の HackContest_LoadMoveBg）に書くので、色回しも 1 を回す
u8 HackContest_GetBattleBgPaletteNum(void)
{
	return HACK_CONTEST_PTR != NULL ? 1 : 2;
}
