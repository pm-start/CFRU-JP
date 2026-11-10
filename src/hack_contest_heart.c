#include "defines.h"
#include "../include/event_object_movement.h"
#include "../include/global.fieldmap.h"
#include "../include/sprite.h"
#include "../include/new/dynamic_ow_pals.h"
#include "hack_contest_addr.h"

/*
 * コンテスト移植 S10（FEAT-contest・patches/0985・ユーザー決定 2026-11-10）: 観客のハートの吹き出し（原文 emote_heart＝MOVEMENT_ACTION_EMOTE_HEART）。
 * 素の FR にはハートの吹き出しが無い。原文（pokeemerald）の次の 3 つを移した:
 *   - event_object_movement.c:6495 MovementAction_EmoteHeart_Step0 → HackContest_MoveEmoteHeart_Step0（動作の表 gSidewaysMovementActionFuncs の 0xEE・src/sideways_stairs.c）
 *       原文は FieldEffectStart(FLDEFF_HEART_ICON)。この改造は表へ足した動作から直接スプライトを作る（フィールド効果の表を増やさない）
 *   - trainer_see.c:716 FldEff_HeartIcon／SetIconSpriteData／745 SpriteCB_TrainerIcons → 下の Create／SpriteCB
 *   - trainer_see.c:142〜185 sSpriteImageTable_HeartIcon・sSpriteAnim_Icons1・sSpriteTemplate_HeartIcon・sOamData_Icons
 * 絵（16x16・4bpp）は日本語版エメラルドの ROM から（tools/contest/extract_jp_heart.py → hack_contest_heart_gfx.c）。色は原文が oam.paletteNum=2（エメラルドの NPC_1 の枠）＝
 * この改造はホウエンの OW の npc_1 のパレット（札 0x1162・gHoennOwPals の 3 つ目）を動的パレットから引く（FindOrLoadNPCPalette）。
 * ★ 書き換え可能な static は使えない（6.13）＝状態はスプライトの data だけ。
 */

#define HACK_HEART_PAL_TAG 0x1162
#define HACK_HEART_SUBPRIORITY 0x52

extern const u8 gHackContestHeartGfx[128];

//原文 sOamData_Icons（16x16・priority 1）
static const struct OamData sOamHeart = {
	.y = 0, .affineMode = 0, .objMode = 0, .mosaic = 0, .bpp = 0, .shape = 0, .x = 0, .matrixNum = 0, .size = 1, .tileNum = 0, .priority = 1, .paletteNum = 0, .affineParam = 0,
};

static const struct SpriteFrameImage sHeartImages[] = {
	{ gHackContestHeartGfx, sizeof(gHackContestHeartGfx), 0 },
};

static const union AnimCmd sHeartAnim[] = {
	ANIMCMD_FRAME(0, 60),
	ANIMCMD_END,
};

static const union AnimCmd* const sHeartAnims[] = { sHeartAnim };

static void SpriteCB_HeartIcon(struct Sprite* sprite);

static const struct SpriteTemplate sHeartTemplate = {
	.tileTag = 0xFFFF, .paletteTag = 0xFFFF, .oam = &sOamHeart, .anims = sHeartAnims, .images = sHeartImages,
	.affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCB_HeartIcon,
};

//data: 0 localId・1 mapNum・2 mapGroup・3 速さ（原文 sYVelocity＝-5 から始まり 1 ずつ増える）・4 高さの差（原文 sYOffset）
//原文 SpriteCB_TrainerIcons（TryGetObjectEventIdByLocalIdAndMap の代わりに素の GetEventObjectIdByLocalIdAndMap を使う＝見つからない印が 16 の違いだけ）
static void SpriteCB_HeartIcon(struct Sprite* sprite)
{
	u8 objId = ContestFr_GetObjectEventIdByLocalIdAndMap(sprite->data[0], sprite->data[1], sprite->data[2]);   //素の FR（0x0805D820）。見つからなければ EVENT_OBJECTS_COUNT（16）

	if (objId >= EVENT_OBJECTS_COUNT || sprite->animEnded)
	{
		DestroySprite(sprite);
	}
	else
	{
		struct Sprite* objSprite = &gSprites[gEventObjects[objId].spriteId];

		sprite->data[4] += sprite->data[3];
		sprite->pos1.x = objSprite->pos1.x;
		sprite->pos1.y = objSprite->pos1.y - 16;
		sprite->pos2.x = objSprite->pos2.x;
		sprite->pos2.y = objSprite->pos2.y + sprite->data[4];
		if (sprite->data[4])
			sprite->data[3]++;
		else
			sprite->data[3] = 0;
	}
}

//原文 FldEff_HeartIcon＋SetIconSpriteData。パレットは動的パレットの枠（札ごとに 1 つ・スプライトを消すと数が戻る）
static void CreateHeartIcon(u8 localId, u8 mapNum, u8 mapGroup)
{
	u8 id = CreateSpriteAtEnd(&sHeartTemplate, 0, 0, HACK_HEART_SUBPRIORITY);

	if (id != MAX_SPRITES)
	{
		struct Sprite* s = &gSprites[id];

		s->oam.priority = 1;
		s->coordOffsetEnabled = 1;
		s->data[0] = localId;
		s->data[1] = mapNum;
		s->data[2] = mapGroup;
		s->data[3] = -5;
		s->data[4] = 0;
		StartSpriteAnim(s, 0);
		s->oam.paletteNum = FindOrLoadNPCPalette(HACK_HEART_PAL_TAG);
	}
}

//原文 MovementAction_EmoteHeart_Step0（Step1 は何もしない動作の終わり）
bool8 HackContest_MoveEmoteHeart_Step0(struct EventObject* obj, struct Sprite* sprite)
{
	CreateHeartIcon(obj->localId, obj->mapNum, obj->mapGroup);
	sprite->data[2] = 1;
	return TRUE;
}
