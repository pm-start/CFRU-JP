#include "defines.h"
#include "bw_follower_internal.h"
#include "../include/sprite.h"
#include "../include/pokemon.h"
#include "../include/task.h"
#include "../include/event_data.h"
#include "../include/event_object_movement.h"
#include "../include/field_effect.h"
#include "../include/new/follow_me.h"

/*
    なみのりのポケモンの絵（#114・pm-start/surf-reloaded を CFRU の書き方に移した・作者は pm-start）。
    なみのりの間、主人公と黒い台のかわりに「なみのりを使ったポケモン」の連れ歩きの絵（src/bw_follower_gfx.c の表）を出し、足もとに
    4 コマの波紋を重ねて、ゆっくり上下させる。向き・高さ・乗り降り・重なりは素の UpdateSurfBlobFieldEffect（080DD4E0）に先にやらせる。

    - 誰がなみのりを使ったか: ROM のフック FollowMe_SurfHook（080866E4・assembly/hooks/follow_me_hooks.s）から HackSurf_Capture が
      手持ちの何番目かを読み、種族 | 色違い << 15 を予約変数 HACK_VAR_SURFER（0x50C4）に書く（色違いは IsMonShiny＝自身の ID・6.5）。
    - 絵: 連れ歩きのシート（札 BW_FOLLOWER_TILE_TAG）を借りる。なみのりの間は連れ歩きはボールの中（物体ごと消えている）。
      借りたら gFollowerState.gfxId を 0（なにも積んでいない）に戻し、連れ歩きが出てくるときに積み直させる。
      パレットは自前の札 HACK_SURF_PAL_TAG で積む。波紋は札 HACK_SURF_RIPPLE_TAG（assembly/hack_surf/surf_ripple.s）。
    - 素の FldEff_SurfBlob（080DD3B8）を functionrewrites で置き換える（HackSurf_Create）。絵が積めない・連れ歩きが出ている最中・
      なみのりを使ったポケモンが分からないときは、素と同じ見た目の台のままにして、HackSurf_Update が 16 コマごとに積み直しを試す。
    - 書き換え可能な static は使わない（6.13）。波紋は自分で親を見て消える（毎コマの gSprites の走査はしない）。
*/

#define HACK_VAR_SURFER 0x50C4
#define HACK_SURF_PAL_TAG 0x4F20
#define HACK_SURF_RIPPLE_TAG 0x4F21
#define SURF_BLOB_TEMPLATE (((const struct SpriteTemplate *const *) 0x08364098)[7])   /* 素の FldEff_SurfBlob が使う gFieldEffectObjectTemplatePointers[7] */

extern const u8 gHackSurfRippleTiles[];
extern const u16 gHackSurfRipplePal[];
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTable1[256];
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTable2[256];
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTable3[256];
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTable4[256];
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTable5[256];
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTableExt1[256];   /* 表 9（patches/0409） */
extern const struct EventObjectGraphicsInfo *const gBwFollowerOwTableExt2[256];   /* 表 10 */
extern const u16 gBwFollowerCount;
bool8 HackHeroine_IsFollowing(void);   /* src/hack_heroine.c（patches/0923 #195: ハルカ・ヒカリが付いてきても主人公の足もとはポケモンの絵） */
u8 LoadPaletteForEmotionBubbles(void);  /* src/dynamic_ow_pals.c（札 0x1100 の枠を探すか積む・hooks の SetPalSurf と同じ） */

static void HackSurf_Update(struct Sprite *sprite);
static void HackSurf_RippleUpdate(struct Sprite *ripple);

/* 波紋（水平な 32x16 / 64x32・4 コマ） */
static const struct OamData sRippleOam32 = { .shape = 1, .size = 2, .priority = 2 };
static const struct OamData sRippleOam64 = { .shape = 1, .size = 3, .priority = 2 };
#define RIPPLE_FRAME(n) ANIMCMD_FRAME(n, 10)
static const union AnimCmd sRippleAnim32[] = { RIPPLE_FRAME(0), RIPPLE_FRAME(8), RIPPLE_FRAME(16), RIPPLE_FRAME(24), ANIMCMD_JUMP(0) };
static const union AnimCmd sRippleAnim64[] = { RIPPLE_FRAME(0), RIPPLE_FRAME(32), RIPPLE_FRAME(64), RIPPLE_FRAME(96), ANIMCMD_JUMP(0) };
static const union AnimCmd *const sRippleAnims32[] = { sRippleAnim32 };
static const union AnimCmd *const sRippleAnims64[] = { sRippleAnim64 };
#define RIPPLE_TEMPLATE(o, a) { .tileTag = HACK_SURF_RIPPLE_TAG, .paletteTag = HACK_SURF_RIPPLE_TAG, .oam = o, .anims = a, \
    .images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = HackSurf_RippleUpdate }
static const struct SpriteTemplate sRippleTemplate32 = RIPPLE_TEMPLATE(&sRippleOam32, sRippleAnims32);
static const struct SpriteTemplate sRippleTemplate64 = RIPPLE_TEMPLATE(&sRippleOam64, sRippleAnims64);

/* ---- 手持ちの何番目がなみのりを使ったか（FollowMe_SurfHook から・r0 = タスク。data[15] が手持ちの番号） ---- */
void HackSurf_Capture(struct Task *task)
{
    u16 slot = task->data[15];

    if (slot < PARTY_SIZE)
    {
        struct Pokemon *mon = &gPlayerParty[slot];
        u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);

        VarSet(HACK_VAR_SURFER, species | (IsMonShiny(mon) ? 0x8000 : 0));
    }
}

/* ---- 波紋 ---- */
static void RipplePosition(struct Sprite *ripple, struct Sprite *parent)
{
    ripple->pos1.x = parent->pos1.x;
    ripple->pos1.y = parent->pos1.y - parent->centerToCornerVecY - 2;
    ripple->oam.priority = parent->oam.priority;
    ripple->invisible = parent->invisible;
}

static void HackSurf_RippleUpdate(struct Sprite *ripple)
{
    u8 id = ripple->data[0];

    if (id >= MAX_SPRITES || !gSprites[id].inUse || gSprites[id].callback != HackSurf_Update)
    {
        /* 岸に上がった／台が消えた。借りていた連れ歩きの札・パレットも返す（連れ歩きが出ていなければ） */
        if ((!gFollowerState.inProgress || HackHeroine_IsFollowing()) && !BwFollowerBallBusy())
        {
            FreeSpriteTilesByTag(BW_FOLLOWER_TILE_TAG);
            if (!HackHeroine_IsFollowing())     /* 人の連れ歩きの gfxId は連れ歩き自身の絵（0 にしない・patches/0923） */
                gFollowerState.gfxId = 0;
        }
        FreeSpritePaletteByTag(HACK_SURF_PAL_TAG);
        DestroySpriteAndFreeResources(ripple);
        return;
    }
    RipplePosition(ripple, &gSprites[id]);
}

static void CreateRipple(u8 parentId, u16 width)
{
    struct SpriteSheet sheet;
    struct SpritePalette palette;
    u8 id;

    FreeSpriteTilesByTag(HACK_SURF_RIPPLE_TAG);
    FreeSpritePaletteByTag(HACK_SURF_RIPPLE_TAG);
    sheet.data = gHackSurfRippleTiles + (width == 64 ? 1024 : 0);
    sheet.size = width == 64 ? 4096 : 1024;
    sheet.tag = HACK_SURF_RIPPLE_TAG;
    palette.data = gHackSurfRipplePal;
    palette.tag = HACK_SURF_RIPPLE_TAG;
    if (LoadSpriteSheet(&sheet) == 0xFFFF)
        return;
    if (LoadSpritePalette(&palette) == 0xFF)
    {
        FreeSpriteTilesByTag(HACK_SURF_RIPPLE_TAG);
        return;
    }
    id = CreateSpriteAtEnd(width == 64 ? &sRippleTemplate64 : &sRippleTemplate32, 0, 0, 0x97);
    if (id < MAX_SPRITES)
    {
        gSprites[id].data[0] = parentId;
        gSprites[id].coordOffsetEnabled = TRUE;
        RipplePosition(&gSprites[id], &gSprites[parentId]);
    }
    else
    {
        FreeSpriteTilesByTag(HACK_SURF_RIPPLE_TAG);
        FreeSpritePaletteByTag(HACK_SURF_RIPPLE_TAG);
    }
}

/* ---- 絵の情報（連れ歩きの表 1〜5。絵の番号 = 表 << 8 | 位置） ---- */
static const struct EventObjectGraphicsInfo *SurferInfo(u16 gfxId)
{
    u8 pos = gfxId & 0xFF;

    switch (gfxId >> 8)
    {
        case 1: return gBwFollowerOwTable1[pos];
        case 2: return gBwFollowerOwTable2[pos];
        case 3: return gBwFollowerOwTable3[pos];
        case 4: return gBwFollowerOwTable4[pos];
        case 5: return gBwFollowerOwTable5[pos];
        case 9: return gBwFollowerOwTableExt1[pos];
        case 10: return gBwFollowerOwTableExt2[pos];
    }
    return NULL;
}

/* なみのりを使ったポケモンの絵を積んで、スプライトを作る。作れなければ MAX_SPRITES */
static u8 CreateSurferSprite(s16 x, s16 y)
{
    u16 stored = VarGet(HACK_VAR_SURFER);
    u16 species = stored & 0x7FFF;
    u16 gfxId, index;
    bool8 shiny = (stored & 0x8000) != 0;
    const struct EventObjectGraphicsInfo *info;
    struct SpriteTemplate template;
    struct SpritePalette palette;
    u8 id;

    if (species == SPECIES_NONE || (gFollowerState.inProgress && !HackHeroine_IsFollowing()) || BwFollowerBallBusy())   /* 人の連れ歩きは別のシートなので札を取り合わない（patches/0923） */
        return MAX_SPRITES;
    gfxId = BwFollowerGfxIdForSpecies(species);
    if (gfxId == 0)
        return MAX_SPRITES;
    index = BwFollowerIndexOf(gfxId);
    if (index >= gBwFollowerCount)
        return MAX_SPRITES;
    info = SurferInfo(gfxId);
    if (info == NULL || (info->width != 32 && info->width != 64))
        return MAX_SPRITES;

    if (!BwFollowerLoadSheet(gfxId))
        return MAX_SPRITES;                         /* 積めない。16 コマごとにやり直す（HackSurf_Update） */
    if (!HackHeroine_IsFollowing())
        gFollowerState.gfxId = 0;                   /* 連れ歩きが出てくるときに自分のシートを積み直させる（人の連れ歩きは gfxId がその絵なので触らない・patches/0923） */

    FreeSpritePaletteByTag(HACK_SURF_PAL_TAG);
    palette.data = gBwFollowerOwPalTable[BwFollowerPalPos(index, shiny)].data;
    palette.tag = HACK_SURF_PAL_TAG;
    if (LoadSpritePalette(&palette) == 0xFF)
    {
        FreeSpriteTilesByTag(BW_FOLLOWER_TILE_TAG);
        return MAX_SPRITES;
    }

    template.tileTag = BW_FOLLOWER_TILE_TAG;
    template.paletteTag = HACK_SURF_PAL_TAG;
    template.oam = info->oam;
    template.anims = info->anims;
    template.images = NULL;
    template.affineAnims = gDummySpriteAffineAnimTable;
    template.callback = HackSurf_Update;
    id = CreateSpriteAtEnd(&template, x, y, 0x96);
    if (id >= MAX_SPRITES)
    {
        FreeSpriteTilesByTag(BW_FOLLOWER_TILE_TAG);
        FreeSpritePaletteByTag(HACK_SURF_PAL_TAG);
        return MAX_SPRITES;
    }
    gSprites[id].pos1.y += gSprites[id].centerToCornerVecY + 8;   /* 引数の座標は台のマスの中心。足もとをそのマスの下の端に */
    gSprites[id].coordOffsetEnabled = TRUE;
    CreateRipple(id, info->width);
    return id;
}

/* ---- 素の FldEff_SurfBlob（080DD3B8）の置き換え（functionrewrites・戻り値あり） ---- */
u32 HackSurf_Create(void)
{
    s32 *args = (s32 *) gFieldEffectArguments;
    s32 x = args[0], y = args[1];
    u8 id = MAX_SPRITES;
    struct Sprite *sprite;

    SetSpritePosToOffsetMapCoords(&x, &y, 8, 8);
    args[0] = x;
    args[1] = y;

    /* 主人公の台だけ（連れ歩きの台は素のまま） */
    if ((u8) args[2] == gPlayerAvatar->eventObjectId)
    {
        id = CreateSurferSprite(x, y);
        if (id < MAX_SPRITES)
        {
            sprite = &gSprites[id];
            sprite->data[2] = args[2];
            sprite->data[3] = 0;
            sprite->data[6] = -1;
            sprite->data[7] = -1;
            FieldEffectActiveListRemove(FLDEFF_SURF_BLOB);
            return id;
        }
    }

    id = CreateSpriteAtEnd(SURF_BLOB_TEMPLATE, x, y, 0x96);
    if (id < MAX_SPRITES)
    {
        sprite = &gSprites[id];
        sprite->coordOffsetEnabled = TRUE;
        /* ★ 2026-10-15（ダイビングで浮上した直後の見た目・docs/research/2026-10-15_dive-surface-sprite.md）:
           素の FldEff_SurfBlob の「枠 0 決め打ち」（oam.paletteNum = 0）は CFRU が hooks の SetPalSurf（080DD3F8＝この関数の中）で
           動的パレットの札 0x1100 の枠に直していた。関数ごと置き換えたので SetPalSurf は通らない → 同じ呼び出しをここで（6.36）。
           枠 0 は地図を読み直すと主人公（札 0x1151／0x1152）が先に取るので、素の台がその色で描かれていた（src/bw_player_pal.c の同じ形） */
        sprite->oam.paletteNum = LoadPaletteForEmotionBubbles();
        sprite->data[2] = args[2];
        sprite->data[3] = 0;
        sprite->data[6] = -1;
        sprite->data[7] = -1;
        if ((u8) args[2] == gPlayerAvatar->eventObjectId)
            sprite->callback = HackSurf_Update;     /* 絵が積めたら作り直す（下） */
    }
    FieldEffectActiveListRemove(FLDEFF_SURF_BLOB);
    return id;
}

static const s8 sBob[8] = { 0, 1, 2, 1, 0, -1, -2, -1 };

static void HackSurf_Update(struct Sprite *sprite)
{
    u8 objId = sprite->data[2];
    struct EventObject *obj;
    struct Sprite *player;
    u8 playerId;

    UpdateSurfBlobFieldEffect(sprite);          /* 向き・高さ・上下・乗り降り・重なりは素のまま */
    if (objId >= NUM_FIELD_OBJECTS)
        return;
    obj = &gEventObjects[objId];
    playerId = obj->spriteId;
    if (playerId >= MAX_SPRITES)
        return;
    player = &gSprites[playerId];

    if (sprite->images != NULL)
    {
        /* 素の台のまま。16 コマごとに、なみのりのポケモンの絵が積めるか試して、積めたら作り直す */
        if ((sprite->data[4] & 15) == 0 && (sprite->data[0] & 15) != 0)
        {
            u8 newId = CreateSurferSprite(sprite->pos1.x, sprite->pos1.y);

            if (newId < MAX_SPRITES)
            {
                struct Sprite *n = &gSprites[newId];
                u8 i;

                for (i = 0; i < 8; ++i)
                    n->data[i] = sprite->data[i];
                n->pos2.y = 0;
                obj->fieldEffectSpriteId = newId;
                DestroySprite(sprite);
            }
        }
        return;
    }

    if ((sprite->data[0] & 15) == 1)            /* 主人公といっしょに上下している間（乗っている間） */
    {
        player->invisible = TRUE;
        player->pos2.y = 0;                     /* 素は主人公の y2 に台の y2 を写す。隠れている間に残さない（降りるとき浮く） */
        sprite->pos1.y = player->pos1.y + 16 + sprite->centerToCornerVecY;
        sprite->invisible = obj->invisible;
        sprite->pos2.y = sBob[(sprite->data[4] >> 3) & 7];
    }
}
