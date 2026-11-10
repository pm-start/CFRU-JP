#include "defines.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "../include/item.h"
#include "../include/string_util.h"
#include "../include/party_menu.h"
#include "../include/constants/items.h"
#include "hack_box_shelf.h"
#include "hack_kind.h"
#include "hack_relive_box.h"
#include "hack_ribbon.h"
#include "shadow_gauge.h"
#include "shadow_relive.h"

/*
 * リライブボックスとリライブ（src/shadow_relive.h に仕様）。
 * 専用の箱（追加のセーブ領域・src/hack_relive_box.c）の中の子は縮めた形（78 バイト）のまま読み、心のゲージ（印の値）だけを HackKind_CompSetValue で書き換える（80 バイトに戻さない＝ほかの欄は 1 ビットも動かない）。
 * 書けないとき（最初のレポートの前・書き込み失敗）は歩数を 127 に戻して次の 1 歩でやり直す。追加の領域が無い版は何もしない。
 */

#define STEPS (*((u8*) gSaveBlock1 + RELIVE_SB1_OFF))
#define GIFT_FLAGS (*((u8*) gSaveBlock1 + RELIVE_SB1_FLAGS_OFF))
#define GIFT_SNATCH_DONE RELIVE_FLAG_GIFT_DONE
#define GIFT_DARK_MET RELIVE_FLAG_DARK_MET
#define GIFT_PENDING() ((GIFT_FLAGS & GIFT_DARK_MET) && !(GIFT_FLAGS & GIFT_SNATCH_DONE))
#define READ_CHUNK 8
typedef char ReliveSb1Fits[(RELIVE_SB1_OFF == 0x3A20 && RELIVE_SB1_FLAGS_OFF == RELIVE_SB1_OFF + 1 && RELIVE_SB1_FLAGS_OFF + 1 <= 0x3A4C) ? 1 : -1];   //SaveBlock1 の未使用の帯の中（技の回数・Moémon の世代の控えの次・rivalName の手前）

bool8 Relive_IsDark(const struct BoxPokemon* mon)
{
	return mon != NULL && HackKind_Get(mon) == HACK_KIND_DARK;
}

bool8 Relive_HasRoom(void)
{
	return ReliveBox_Usable() == RELIVE_BOX_OK && ReliveBox_Count() < RELIVE_BOX_SLOTS;
}

bool8 Relive_PutComp(const struct CompressedPokemon* comp, u8* slotOut)
{
	void* image = ReliveBox_ScratchImage();

	if (ReliveBox_Usable() != RELIVE_BOX_OK || ReliveBox_Count() >= RELIVE_BOX_SLOTS)
		return FALSE;
	if (!ReliveBox_Load(image))
		return FALSE;
	return ReliveBox_Put(image, comp, slotOut);
}

//専用の箱の 1 匹ぶんの 128 歩の処理。TRUE = 終わった（何も無かったのも含む）・FALSE = 書きたいのに書けない（次の 1 歩でやり直す）
static bool8 ApplyWalk(void)
{
	void* image;
	u8 i, u = ReliveBox_Usable();
	bool8 changed = FALSE;

	if (ReliveBox_Count() == 0 || u == RELIVE_BOX_NO_EXT)
		return TRUE;
	if (u != RELIVE_BOX_OK)
		return FALSE;                  //最初のレポートの前: 使えるかを決められない＝次の 1 歩でやり直す
	image = ReliveBox_ScratchImage();
	if (!ReliveBox_Load(image))
		return TRUE;
	for (i = 0; i < RELIVE_BOX_SLOTS; ++i)
	{
		struct CompressedPokemon* mon = ReliveBox_SlotMon(image, i);
		u8 gauge, after;

		if (!ReliveBox_SlotUsed(image, i) || HackKind_CompGet(mon) != HACK_KIND_DARK)
			continue;
		gauge = HackKind_CompValue(mon);
		after = Shadow_GaugeAfter(gauge, GetNatureFromPersonality(mon->personality), SHADOW_ACTION_WALK);
		if (after == gauge)
			continue;
		HackKind_CompSetValue(mon, after);
		changed = TRUE;
	}
	return changed ? ReliveBox_Commit(image) : TRUE;
}

//---- 0.65.0 のセーブの移行: 箱 26 に残っているダークを専用の箱へ ----
//※ 試した印は持たない（書き換えられる static は ROM に置かれて書けない・tools/rom/verify_cfru_audit.py の許可表）。128 歩ごとに箱 26 を 1 回読むだけ（30 匹・RAM の中・ダークがいなければ何もしない）

static void FindLegacyDark(u32* mask)
{
	struct CompressedPokemon buf[READ_CHUNK];
	u8 first, n, i;

	*mask = 0;
	for (first = 0; first < IN_BOX_COUNT; first += READ_CHUNK)
	{
		n = (IN_BOX_COUNT - first < READ_CHUNK) ? IN_BOX_COUNT - first : READ_CHUNK;
		if (!BoxShelf_ReadBoxMons(RELIVE_LEGACY_BOX_ID, first, n, buf))
			return;
		for (i = 0; i < n; ++i)
			if (HackKind_CompGet(&buf[i]) == HACK_KIND_DARK)
				*mask |= 1u << (first + i);
	}
}

//TRUE = 移すものが無い・追加の領域なし・満杯で入りきらない。FALSE = あとでやり直す（レポートの前・書けない）。どちらでも 128 歩ごとに呼ばれる
static bool8 TryMigrate(void)
{
	u32 mask;
	u8 pos, u = ReliveBox_Usable();

	if (u == RELIVE_BOX_NO_EXT)
		return TRUE;
	if (u != RELIVE_BOX_OK)
		return FALSE;
	FindLegacyDark(&mask);
	if (mask == 0)
		return TRUE;
	if (!BoxShelf_CanTouchBox(RELIVE_LEGACY_BOX_ID))
		return FALSE;
	for (pos = 0; pos < IN_BOX_COUNT; ++pos)
	{
		struct CompressedPokemon comp;
		u8* p;
		u8 i;

		if (!(mask & (1u << pos)))
			continue;
		if (ReliveBox_Count() >= RELIVE_BOX_SLOTS)
			return TRUE;               //専用の箱が満杯: 残りは箱 26 に残す（消さない）
		BoxShelf_ReadMon(RELIVE_LEGACY_BOX_ID, pos, &comp);
		if (!Relive_PutComp(&comp, NULL))
			return FALSE;              //書けなかった: 箱 26 はそのまま（次にやり直す）
		p = (u8*) &BoxShelf_BoxMons(RELIVE_LEGACY_BOX_ID, TRUE)[pos];
		for (i = 0; i < sizeof(comp); ++i)
			p[i] = 0;                  //専用の箱へ書けたあとで箱 26 から消す（レポートで SaveBlock1 の sel と一緒に保存される）
	}
	return TRUE;
}

void Relive_OnStep(void)
{
	bool8 fire, walked;

	STEPS = Shadow_StepAdvance(STEPS, &fire);
	if (!fire)
		return;
	walked = ApplyWalk();          //先に専用の箱の 1 刻み（移したばかりの子はこの回は下げない＝移行でバイトが変わらない）
	TryMigrate();                  //0.65.0 の箱 26 に残ったダークを専用の箱へ（つづきから後の最初の 128 歩で・移せなければ次の 128 歩で）
	if (!walked)
		STEPS = SHADOW_RELIVE_STEPS - 1;
}

u8 Relive_ScentStrength(u16 item)
{
	switch (item)
	{
		case ITEM_JOY_SCENT: return 1;
		case ITEM_EXCITE_SCENT: return 2;
		case ITEM_VIVID_SCENT: return 3;
	}
	return 0;
}

bool8 Relive_UseScent(struct Pokemon* mon, u16 item)
{
	u8 strength = Relive_ScentStrength(item), gauge, i, nature;
	struct BoxPokemon* box = (struct BoxPokemon*) mon;

	if (strength == 0 || GetMonData(mon, MON_DATA_IS_EGG, NULL) || !Relive_IsDark(box))
		return FALSE;
	gauge = HackKind_Value(box);
	if (Shadow_IsOpen(gauge))
		return FALSE;
	nature = GetNatureFromPersonality(GetMonData(mon, MON_DATA_PERSONALITY, NULL));
	for (i = 0; i < strength; ++i)
		gauge = Shadow_GaugeAfter(gauge, nature, SHADOW_ACTION_SCENT);
	HackKind_SetValue(box, gauge);
	GetMonNickname(mon, gStringVar1);
	return TRUE;
}

static u8 PartyDarkCount(u8* ready)
{
	u8 i, n = 0;

	*ready = 0;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];

		if (GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG, NULL))
			continue;
		if (!Relive_IsDark((struct BoxPokemon*) mon))
			continue;
		++n;
		if (Shadow_IsOpen(HackKind_Value((struct BoxPokemon*) mon)))
			++(*ready);
	}
	return n;
}

//LASTRESULT: 0 = ふつうのポケセン／1 = リライブの話（手持ちにダークがいる・または初回の贈り物がまだ）。gStringVar1 に ゲージ 0 の数
void Relive_ScriptPrepare(void)
{
	u8 ready, n = PartyDarkCount(&ready);

	ConvertIntToDecimalStringN(gStringVar1, ready, STR_CONV_MODE_LEFT_ALIGN, 1);
	gSpecialVar_LastResult = (n > 0 || GIFT_PENDING()) ? 1 : 0;
}

void Relive_ScriptReadyCount(void)
{
	u8 ready;

	PartyDarkCount(&ready);
	ConvertIntToDecimalStringN(gStringVar1, ready, STR_CONV_MODE_LEFT_ALIGN, 1);
	gSpecialVar_LastResult = ready;
}

void Relive_ScriptPurifyNext(void)
{
	u8 i;

	gSpecialVar_LastResult = 0;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];
		struct BoxPokemon* box = (struct BoxPokemon*) mon;

		if (GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG, NULL))
			continue;
		if (!Relive_IsDark(box) || !Shadow_IsOpen(HackKind_Value(box)))
			continue;
		GetMonNickname(mon, gStringVar1);
		HackKind_Clear(box);   //ダークの印とゲージを消す（ダーク技は組み立て方式＝戻す技は無い・ためていた経験値も無い）
		HackRibbon_AddBits(box, HACK_RIBBON_NATIONAL);   //[hack] patches/0977 リライブの記念にナショナルリボン（エメラルドの配布リボン 5 番目・giftRibbon5。名前と説明は画面側の固定の文）
		gSpecialVar_LastResult = 1;
		return;
	}
}

void Relive_ScriptGiveSnatch(void)
{
	u16 have = CountTotalItemQuantityInBag(ITEM_SNATCH_BALL), give;

	if (have >= RELIVE_SNATCH_STOCK)
	{
		gSpecialVar_LastResult = 0;
		return;
	}
	give = RELIVE_SNATCH_STOCK - have;
	if (!AddBagItem(ITEM_SNATCH_BALL, give))
	{
		gSpecialVar_LastResult = 0xFFFF;
		return;
	}
	ConvertIntToDecimalStringN(gStringVar1, give, STR_CONV_MODE_LEFT_ALIGN, 1);
	gSpecialVar_LastResult = give;
}

void Relive_ScriptGiftNeeded(void)
{
	gSpecialVar_LastResult = GIFT_PENDING() ? 1 : 0;
}

void Relive_NoteDarkMet(void)
{
	GIFT_FLAGS |= GIFT_DARK_MET;
}

void Relive_ScriptGiftDone(void)
{
	GIFT_FLAGS |= GIFT_SNATCH_DONE;
}
