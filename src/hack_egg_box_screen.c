#include "defines.h"
#include "../include/bg.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/malloc.h"
#include "../include/menu.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/pokemon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/pokedex.h"
#include "../include/constants/game_stat.h"
#include "../include/constants/songs.h"
#include "../include/constants/species.h"
#include "../include/new/Vanilla_functions.h"
#include "hack_egg_box.h"

/*
 * 卵専用ボックスの画面（2026-10-23 FEAT-save16-B4・Redmine #38・ユーザー決定 q115「べんりきのう」から・どこでも開ける）。
 * 入れ物・保存・歩数は src/hack_egg_box.c。ここは画面と、手持ちとの出し入れだけ。
 *   一覧（30 匹・7 行ずつ）: ↑↓ で選ぶ・L／R で 7 行ずつ・A（入っている所）= ひきとる（確認）・A（空き）か START = 手持ちから あずける・B = もどる
 *   あずけるのは手持ちの「タマゴ」だけ。手持ちに たたかえる ポケモンが いなくなる あずけ方（タマゴしか いない）はできない。
 *   ひきとるのは手持ちが 6 匹未満のとき。かえった子は、ひきとるときに素と同じ作り（CreateHatchedMon）で子にする。
 */

void CreateHatchedMon(struct Pokemon* egg, struct Pokemon* temp);   //src/daycare.c
void CompressedMonToMon(struct CompressedPokemon* compMon, struct Pokemon* dst);
void CreateBoxMonFromCompressedMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);
void CreateCompressedMonFromBoxMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);
void __attribute__((long_call)) ScanlineEffect_Stop(void);

extern const u8 gText_EggBox_Title[], gText_EggBox_Empty[], gText_EggBox_Egg[], gText_EggBox_Hatched1[];
extern const u8 gText_EggBox_PartyTitle[], gText_EggBox_HelpList[], gText_EggBox_HelpParty[], gText_EggBox_AskTake[];
extern const u8 gText_EggBox_Full1[], gText_EggBox_Full2[], gText_EggBox_NotEgg1[], gText_EggBox_NotEgg2[];
extern const u8 gText_EggBox_LastMon1[], gText_EggBox_LastMon2[], gText_EggBox_PartyFull1[], gText_EggBox_PartyFull2[];
extern const u8 gText_EggBox_NoEgg[], gText_EggBox_NoEgg2[], gText_EggBox_Unavail1[], gText_EggBox_Unavail2[];
extern const u8 gText_EggBox_NeedReport1[], gText_EggBox_NeedReport2[], gText_EggBox_BadData1[], gText_EggBox_BadData2[];
extern const u8 gText_EggBox_WriteFail1[], gText_EggBox_WriteFail2[], gText_EggBox_Done[];

#define ROWS 7
#define ROW_Y0 28
#define ROW_H 16

enum { ST_LIST, ST_PARTY, ST_MSG, ST_CONFIRM, ST_EXIT };

struct EggScreen
{
	u8 state;
	u8 retState;
	u8 cursor;
	u8 top;
	u8 partyCursor;
	u8 usable;
	const u8* msg1;
	const u8* msg2;
	u8* image;
};

static const struct BgTemplate sBgTemplates[] =
{
	{.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
	{.bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = 30, .height = 20, .paletteNum = 15, .baseBlock = 1},
	DUMMY_WIN_TEMPLATE,
};

static const u16 sPalette[16] = {0, 0x7FFF, 0x1084, 0x7FE0, 0x03FF, 0x0842, 0x18C6, 0x3180, 0x6739, 0x0421};

static void Print(const u8* text, u8 x, u8 y, u8 color, u8 bg)
{
	u8 colors[3] = {bg, color, 2};

	AddTextPrinterParameterized3(0, 2, x, y, colors, 0, text);
}

static void Rect(u8 color, u16 x, u16 y, u16 width, u16 height)
{
	FillWindowPixelRect(0, color, x, y, width, height);
}

static struct EggScreen* ScreenOf(u8 taskId)
{
	return (struct EggScreen*) (((u32) (u16) gTasks[taskId].data[0]) | ((u32) (u16) gTasks[taskId].data[1] << 16));
}

//---------------------------------------------------------------- 図鑑・孵化数（箱の中で孵った瞬間・src/hack_egg_box.c の EggBox_OnStep から）

void HackEggBox_MarkHatched(const struct CompressedPokemon* comp)
{
	struct BoxPokemon box;
	u16 species, dex;
	u32 personality;

	CreateBoxMonFromCompressedMon(&box, (struct CompressedPokemon*) comp);
	species = GetBoxMonData(&box, MON_DATA_SPECIES, NULL);
	personality = GetBoxMonData(&box, MON_DATA_PERSONALITY, NULL);
	if (species != SPECIES_NONE && species != SPECIES_EGG)
	{
		dex = SpeciesToNationalPokedexNum(species);
		HandleSetPokedexFlag(dex, FLAG_SET_SEEN, personality);
		HandleSetPokedexFlag(dex, FLAG_SET_CAUGHT, personality);
	}
	IncrementGameStat(GAME_STAT_HATCHED_EGGS);
}

//---------------------------------------------------------------- 手持ち

static u8 PartyCount(void)
{
	return CalculatePlayerPartyCount();
}

static u8 PartyNonEggCount(void)
{
	u8 i, n = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_NONE
		 && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
			++n;
	return n;
}

static u8 PartyEggCount(void)
{
	u8 i, n = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_NONE
		 && GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
			++n;
	return n;
}

//---------------------------------------------------------------- 描く

static u8* NumberText(u8* p, u32 n, u8 digits)
{
	return ConvertIntToDecimalStringN(p, n, STR_CONV_MODE_RIGHT_ALIGN, digits);
}

static void DrawFrame(const u8* title, const u8* help)
{
	FillWindowPixelBuffer(0, 0x55);
	Rect(6, 0, 0, 240, 24);
	Rect(3, 0, 23, 240, 1);
	Rect(6, 0, 138, 240, 22);
	Rect(8, 0, 138, 240, 1);
	Print(title, 8, 5, 1, 6);
	Print(help, 8, 143, 1, 6);
}

static void DrawList(struct EggScreen* s)
{
	static const u8 sCursor[] = {0xEF, EOS};
	u8 buf[32];
	u8* p;
	u8 r;

	DrawFrame(gText_EggBox_Title, gText_EggBox_HelpList);
	p = NumberText(buf, EggBox_Count(), 2);
	*p++ = 0xBA;     //'/'
	p = NumberText(p, EGGBOX_SLOTS, 2);
	*p = EOS;
	Print(buf, 190, 5, 8, 6);
	for (r = 0; r < ROWS; ++r)
	{
		u8 slot = s->top + r;
		u8 y = ROW_Y0 + r * ROW_H;
		u32 at;

		if (slot >= EGGBOX_SLOTS)
			break;
		if (s->cursor == slot)
			Print(sCursor, 4, y, 4, 5);
		p = NumberText(buf, slot + 1, 2);
		*p++ = 0;
		at = EggBox_SlotHatchAt(s->image, slot);
		if (at == 0)
			p = StringCopy(p, gText_EggBox_Empty);
		else if (EggBox_IsHatched(at))
		{
			struct BoxPokemon box;

			CreateBoxMonFromCompressedMon(&box, (struct CompressedPokemon*) EggBox_SlotMon(s->image, slot));
			GetSpeciesName(p, GetBoxMonData(&box, MON_DATA_SPECIES, NULL));
			while (*p != EOS)
				++p;
			*p++ = 0;
			p = StringCopy(p, gText_EggBox_Hatched1);
		}
		else
			p = StringCopy(p, gText_EggBox_Egg);
		Print(buf, 20, y, 1, 5);
	}
	PutWindowTilemap(0);
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void DrawParty(struct EggScreen* s)
{
	static const u8 sCursor[] = {0xEF, EOS};
	u8 buf[32];
	u8* p;
	u8 i, n = PartyCount();

	DrawFrame(gText_EggBox_PartyTitle, gText_EggBox_HelpParty);
	for (i = 0; i < n; ++i)
	{
		u8 y = ROW_Y0 + i * ROW_H;

		if (s->partyCursor == i)
			Print(sCursor, 4, y, 4, 5);
		p = NumberText(buf, i + 1, 1);
		*p++ = 0;
		if (GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
			p = StringCopy(p, gText_EggBox_Egg);
		else
			GetMonData(&gPlayerParty[i], MON_DATA_NICKNAME, p);
		Print(buf, 20, y, 1, 5);
	}
	PutWindowTilemap(0);
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void DrawMessage(struct EggScreen* s)
{
	Rect(6, 0, 96, 240, 64);
	Rect(8, 0, 96, 240, 1);
	Print(s->msg1, 10, 104, 1, 6);
	if (s->msg2 != NULL)
		Print(s->msg2, 10, 120, 1, 6);
	if (s->state == ST_MSG)
		Print(gText_EggBox_Done, 10, 142, 8, 6);
	PutWindowTilemap(0);   //使えない版の画面は一覧を描かないので、ここでも窓を出す（出さないと真っ暗なまま）
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void Redraw(struct EggScreen* s)
{
	if (s->state == ST_PARTY)
		DrawParty(s);
	else if (s->usable == EGGBOX_OK || s->retState != ST_EXIT)
		DrawList(s);
	else
		DrawFrame(gText_EggBox_Title, gText_EggBox_Done);
	if (s->state == ST_MSG || s->state == ST_CONFIRM)
		DrawMessage(s);
}

static void ShowMsg(struct EggScreen* s, const u8* m1, const u8* m2, u8 ret)
{
	s->msg1 = m1;
	s->msg2 = m2;
	s->retState = ret;
	s->state = ST_MSG;
	PlaySE(SE_ERROR);
	Redraw(s);
}

//---------------------------------------------------------------- 出し入れ

static void DoPut(struct EggScreen* s)
{
	struct Pokemon* mon = &gPlayerParty[s->partyCursor];
	struct CompressedPokemon comp;
	u8 slot;

	if (!GetMonData(mon, MON_DATA_IS_EGG, NULL))
	{
		ShowMsg(s, gText_EggBox_NotEgg1, gText_EggBox_NotEgg2, ST_PARTY);
		return;
	}
	if (PartyNonEggCount() == 0)   //手持ちが タマゴだけ（最後の 1 匹がタマゴ）なら あずけられない
	{
		ShowMsg(s, gText_EggBox_LastMon1, gText_EggBox_LastMon2, ST_PARTY);
		return;
	}
	if (EggBox_Count() >= EGGBOX_SLOTS)
	{
		ShowMsg(s, gText_EggBox_Full1, gText_EggBox_Full2, ST_LIST);
		return;
	}
	CreateCompressedMonFromBoxMon((struct BoxPokemon*) mon, &comp);
	if (!EggBox_Put(s->image, &comp, GetMonData(mon, MON_DATA_FRIENDSHIP, NULL), &slot))
	{
		ShowMsg(s, gText_EggBox_WriteFail1, gText_EggBox_WriteFail2, ST_LIST);
		return;
	}
	ZeroMonData(mon);
	CompactPartySlots();
	CalculatePlayerPartyCount();
	PlaySE(SE_SELECT);
	s->cursor = slot;
	if (s->cursor < s->top || s->cursor >= s->top + ROWS)
		s->top = (slot / ROWS) * ROWS;
	s->state = ST_LIST;
	Redraw(s);
}

static void DoTake(struct EggScreen* s)
{
	struct Pokemon mon, temp;
	struct CompressedPokemon comp;
	u32 at = EggBox_SlotHatchAt(s->image, s->cursor);
	bool8 hatched = EggBox_IsHatched(at);

	if (PartyCount() >= PARTY_SIZE)
	{
		ShowMsg(s, gText_EggBox_PartyFull1, gText_EggBox_PartyFull2, ST_LIST);
		return;
	}
	comp = *EggBox_SlotMon(s->image, s->cursor);
	CompressedMonToMon(&comp, &mon);
	if (hatched)
	{
		u16 species;
		u32 personality;
		u8 metLoc;

		CreateHatchedMon(&mon, &temp);          //素の孵化と同じ作り（子の姿・なつき度・かえった場所は今の地方）
		species = GetMonData(&mon, MON_DATA_SPECIES, NULL);
		personality = GetMonData(&mon, MON_DATA_PERSONALITY, NULL);
		metLoc = GetCurrentRegionMapSectionId();
		SetMonData(&mon, MON_DATA_MET_LOCATION, &metLoc);
		HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_SEEN, personality);
		HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT, personality);
	}
	else
	{
		u32 left = at - EggBox_Clock();
		u8 friendship = left > 255 ? 255 : (left == 0 ? 1 : left);

		SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);   //残りサイクルを、手持ちの卵と同じ持ち方（なつき度の欄）に戻す
	}
	if (!EggBox_Take(s->image, s->cursor))
	{
		ShowMsg(s, gText_EggBox_WriteFail1, gText_EggBox_WriteFail2, ST_LIST);
		return;
	}
	gPlayerParty[PartyCount()] = mon;
	CalculatePlayerPartyCount();
	PlaySE(SE_SELECT);
	s->state = ST_LIST;
	Redraw(s);
}

//---------------------------------------------------------------- 画面の動き

static void MainCB2_EggBox(void)
{
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	UpdatePaletteFade();
}

static void VBlankCB_EggBox(void)
{
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

static void EggExit(struct EggScreen* s)
{
	s->state = ST_EXIT;
	PlaySE(SE_PC_OFF);
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
}

static void Task_EggBox(u8 taskId)
{
	struct EggScreen* s = ScreenOf(taskId);
	u16 keys;

	if (gPaletteFade->active)
		return;
	if (s->state == ST_EXIT)
	{
		SetVBlankCallback(NULL);
		if (s->image != NULL)
			Free(s->image);
		Free(s);
		DestroyTask(taskId);
		FreeAllWindowBuffers();
		ResetSpriteData();
		FreeAllSpritePalettes();
		SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
		return;
	}
	keys = gMain.newKeys;
	switch (s->state)
	{
	case ST_MSG:
		if (keys & (A_BUTTON | B_BUTTON))
		{
			if (s->retState == ST_EXIT)
			{
				EggExit(s);
				return;
			}
			s->state = s->retState;
			Redraw(s);
		}
		break;
	case ST_CONFIRM:
		if (keys & A_BUTTON)
			DoTake(s);
		else if (keys & B_BUTTON)
		{
			s->state = ST_LIST;
			Redraw(s);
		}
		break;
	case ST_PARTY:
		if (keys & B_BUTTON)
		{
			s->state = ST_LIST;
			Redraw(s);
		}
		else if (keys & (DPAD_UP | DPAD_DOWN))
		{
			u8 n = PartyCount();

			if (n == 0)
				break;
			if (keys & DPAD_UP)
				s->partyCursor = (s->partyCursor == 0) ? n - 1 : s->partyCursor - 1;
			else
				s->partyCursor = (s->partyCursor + 1 >= n) ? 0 : s->partyCursor + 1;
			PlaySE(SE_SELECT);
			Redraw(s);
		}
		else if ((keys & A_BUTTON) && PartyCount() != 0)
			DoPut(s);
		break;
	default: //ST_LIST
		if (keys & B_BUTTON)
			EggExit(s);
		else if (keys & (DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT | L_BUTTON | R_BUTTON))
		{
			s16 c = s->cursor;

			if (keys & DPAD_UP)
				c = (c == 0) ? EGGBOX_SLOTS - 1 : c - 1;
			else if (keys & DPAD_DOWN)
				c = (c + 1 >= EGGBOX_SLOTS) ? 0 : c + 1;
			else if (keys & (DPAD_LEFT | L_BUTTON))
				c = (c - ROWS < 0) ? 0 : c - ROWS;
			else
				c = (c + ROWS >= EGGBOX_SLOTS) ? EGGBOX_SLOTS - 1 : c + ROWS;
			s->cursor = c;
			if (s->cursor < s->top)
				s->top = s->cursor;
			else if (s->cursor >= s->top + ROWS)
				s->top = s->cursor - ROWS + 1;
			PlaySE(SE_SELECT);
			Redraw(s);
		}
		else if ((keys & START_BUTTON) || ((keys & A_BUTTON) && EggBox_SlotHatchAt(s->image, s->cursor) == 0))
		{
			if (PartyEggCount() == 0)
				ShowMsg(s, gText_EggBox_NoEgg, gText_EggBox_NoEgg2, ST_LIST);
			else if (EggBox_Count() >= EGGBOX_SLOTS)
				ShowMsg(s, gText_EggBox_Full1, gText_EggBox_Full2, ST_LIST);
			else
			{
				s->partyCursor = 0;
				s->state = ST_PARTY;
				PlaySE(SE_SELECT);
				Redraw(s);
			}
		}
		else if (keys & A_BUTTON)
		{
			s->msg1 = gText_EggBox_AskTake;
			s->msg2 = NULL;
			s->state = ST_CONFIRM;
			PlaySE(SE_SELECT);
			Redraw(s);
		}
		break;
	}
}

void CB2_HackEggBox(void)
{
	u8 taskId;
	struct EggScreen* s;

	SetVBlankCallback(NULL);
	ScanlineEffect_Stop();
	ResetTasks();
	ResetSpriteData();
	FreeAllSpritePalettes();
	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
	InitWindows(sWindowTemplates);
	DeactivateAllTextPrinters();
	LoadPalette(sPalette, 15 * 16, sizeof(sPalette));
	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
	ShowBg(0);
	s = AllocZeroed(sizeof(struct EggScreen));
	taskId = CreateTask(Task_EggBox, 0);
	gTasks[taskId].data[0] = (u32) s;
	gTasks[taskId].data[1] = (u32) s >> 16;
	s->usable = EggBox_Usable();
	s->state = ST_LIST;
	if (s->usable != EGGBOX_OK)
	{
		if (s->usable == EGGBOX_NEED_REPORT)
			ShowMsg(s, gText_EggBox_NeedReport1, gText_EggBox_NeedReport2, ST_EXIT);
		else
			ShowMsg(s, gText_EggBox_Unavail1, gText_EggBox_Unavail2, ST_EXIT);
	}
	else
	{
		s->image = Malloc(EggBox_ImageSize());
		if (s->image == NULL || !EggBox_Load(s->image))
		{
			s->usable = EGGBOX_BAD_DATA;
			ShowMsg(s, gText_EggBox_BadData1, gText_EggBox_BadData2, ST_EXIT);
		}
		else
			Redraw(s);
	}
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
	SetVBlankCallback(VBlankCB_EggBox);
	SetMainCallback2(MainCB2_EggBox);
}
