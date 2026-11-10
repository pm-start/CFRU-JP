#include "defines.h"
#include "../include/new/dynamic_ow_pals.h"
#include "../include/new/character_customization.h"
#include "../include/field_player_avatar.h"
#include "../include/sprite.h"

/*
    主人公の姿を切り替えたときのパレットの枠を CFRU の動的 OW パレットに合わせる（#20・2026-09-23）

    ユーザー報告（実機）:
      「なみのりで色味がおかしくなる」（BW の主人公は正しいが、波乗りの台の色が化ける）
      「画面切り替えが入ると元の主人公が出てくる」（主人公も台もレッドの色になる）
      「2 つの現象が同じような気がする」 → そのとおりで、原因は 1 つ。

    ## 原因

    CFRU は動的 OW パレット（src/dynamic_ow_pals.c）で、スプライトのパレットを
    **札（paletteTag）ごとに空いている枠へ**割り振る。出現時（SetPalNPC1 など）は全部これを通る。

    ところが素の `ObjectEventSetGraphicsId`（0805E920・歩き → なみのり などの切り替えで呼ばれる）は
    これを通らず、絵の情報の paletteSlot が 0（主人公）のとき
      ① `PatchObjectPalette(paletteTag, 0)` で**枠 0 に決め打ちで**書き込み（0805E95A）
      ② `sprite->oam.paletteNum = paletteSlot` で**主人公を枠 0 に付け替える**（0805E9C6）
    枠 0 には動的パレットが別の札を置いていることがある。

    素のレッドは歩きも**波乗りの台**（`LoadPaletteForEmotionBubbles` ＝ 札 0x1100）も同じ札なので、
    どちらが枠 0 を書いても同じ色になり、ぶつかっても見えなかった。
    BW の主人公は札 0x1151 / 0x1152 なので、
      - 台が枠 0 の 0x1100 を使っているところへ主人公が 0x1151 を上書き → 台の色が化ける
      - 画面切り替えのあと、枠 0 に 0x1100 が読み直される → 枠 0 を指す主人公もレッドの色になる

    ## 直し方

    ① の書き込みは bytereplacement で飛ばす（0805E952 の BNE を B に）。
    ② のあと（0805EA40 のフック・assembly/bwplayer/bw_player_pal_hook.s）でこの関数を呼び、
    主人公の枠を**動的パレットが札で割り振った枠**に付け直す。

    ★ 札がもう読まれていれば（歩き・なみのり… は男女それぞれ同じ札）その枠を使うだけで、
      数（Count）は増やさない。増やしても主人公は消えないので害は無いが、切り替えのたびに積み上がる。
    ★ paletteSlot が 0 以外（ふつうの NPC・連れ歩きの 10 番）は素の動きのまま。
*/

void BwFixSetGraphicsIdPalette(struct Sprite* sprite, const struct EventObjectGraphicsInfo* info)
{
	u8 slot;

	if (info->paletteSlot != 0)
		return;

	slot = FindPalRef(PalTypeNPC, info->paletteTag1);
	if (slot == 0xFF)
		slot = FindOrLoadNPCPalette(info->paletteTag1);

	sprite->oam.paletteNum = slot;
}

/* 札を持たないボールが置き場を借りる分を加える。取得できないときはボールを作らない。 */
bool8 BwPalRefAddIfUsed(u8 slot)
{
	/* dynamic_ow_pals.c の PalRef と同じ配置。上流のヘッダーでは公開されていない。 */
	struct HackPalRef { u8 Type; u8 Count; u16 PalTag; };
	struct HackPalRef *refs = (struct HackPalRef *)0x0203B74C;

	if (slot >= 16 || refs[slot].Type == PalTypeUnused || refs[slot].Count == 0 || refs[slot].Count == 0xFF)
		return FALSE;
	refs[slot].Count++;
	return TRUE;
}

/*
    主人公の札が帳簿（sPalRefs 0x0203B74C）から消えていたら登録し直す（2026-10-15・CLAUDE 6.36・資料 2026-10-15_lavender-pc-exit-gfx.md）

    mGBA（0.44.1・ピカチュウ 1 匹・ポケセンの受付の前）: 回復の演出の後片付けのころ（584 コマ目に演出 25・748 コマ目）に
    主人公の札 0x1151 の枠 0 が「使っていない」になり（主人公のスプライトは paletteNum 0 のまま使用中）、
    ボールから出し直した連れ歩き（800 コマ目）が空いた枠 0 に 0x2018 を積んで、主人公がピカチュウの色で描かれ
    ポケセンを出るまで直らなかった。回復しない・話しかけないときは起きない。
    CFRU の帳簿は DestroySprite（フック DeleteOBJFreeTiles）が消したスプライトの paletteNum の数を 1 つ減らし、0 になると枠を空ける。
    数を増やさずに作られたスプライトが消えると、そのとき指していた枠の数が余分に減る。どのスプライトかは静的には決めきれなかった（資料）。
    → 毎コマ（src/bw_follower.c の BwFollowerHideForFieldEffect の頭）主人公の枠を見て、空いていたら主人公の札で数 1 に戻す。
      色はまだ枠に残っているので積み直さない（同じコマの後ろで連れ歩きが作り直されても、もう空き枠として渡されない）。
    ★ 書き換え可能な static は使わない（6.13）。帳簿は CFRU の RAM（dynamic_ow_pals.c と同じ並び）。
*/
void BwPlayerKeepPalRef(void)
{
	struct HackPalRef { u8 Type; u8 Count; u16 PalTag; };
	struct HackPalRef *refs = (struct HackPalRef *)0x0203B74C;
	struct EventObject *player;
	struct Sprite *sprite;
	const struct EventObjectGraphicsInfo *info;
	u8 slot;

	if (gPlayerAvatar->eventObjectId >= EVENT_OBJECTS_COUNT)
		return;
	player = &gEventObjects[gPlayerAvatar->eventObjectId];
	if (!player->active || player->spriteId >= MAX_SPRITES)
		return;
	sprite = &gSprites[player->spriteId];
	if (!sprite->inUse)
		return;
	slot = sprite->oam.paletteNum;
	if (refs[slot].Type != PalTypeUnused)
		return;		/* 使用中（ふだんはここで帰る） */

	info = GetEventObjectGraphicsInfo(GetEventObjectGraphicsId(player));
	if (info == NULL || info->paletteTag1 == 0xFFFF)
		return;
	refs[slot].Type = PalTypeNPC;
	refs[slot].PalTag = info->paletteTag1;
	refs[slot].Count = 1;
}
