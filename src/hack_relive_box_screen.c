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
#include "hack_relive_box.h"
#include "hack_kind.h"
#include "hack_delta.h" //[hack] patches/0981 HackNushi_PartyHasOther
#include "shadow_relive.h"
#include "hack_ribbon.h"   //[hack] patches/0977

/*
 * リライブボックスの画面（2026-11-09 FEAT-relive-box-ext・patches/0975・案 B）。作りは たまごボックスの画面（src/hack_egg_box_screen.c）と同じ（べんりきのう・どこでも開ける）。
 * 入れ物・保存は src/hack_relive_box.c・歩数は src/shadow_relive.c。ここは画面と、手持ちとの出し入れだけ。
 *   一覧（30 匹・7 行ずつ）: ↑↓ で選ぶ・L／R で 7 行ずつ・A（入っている所）= ひきとる（確認）・A（空き）か START = 手持ちの ダークを あずける・B = もどる
 *   あずけるのは手持ちの「ダーク」だけ。手持ちに たたかえる ポケモンが いなくなる あずけ方はできない。ひきとるのは手持ちが 6 匹未満のとき。
 *   各行にこころのゲージ（0〜255・0 で リライブできる）を出す。リライブそのものは看護師（手持ちに連れてきてから）。
 */

void CompressedMonToMon(struct CompressedPokemon* compMon, struct Pokemon* dst);
void CreateBoxMonFromCompressedMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);
void CreateCompressedMonFromBoxMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);
void __attribute__((long_call)) ScanlineEffect_Stop(void);

extern const u8 gText_ReliveBox_Title[], gText_ReliveBox_Empty[], gText_ReliveBox_Gauge[], gText_ReliveBox_Ready[];
extern const u8 gText_ReliveBox_PartyTitle[], gText_ReliveBox_HelpList[], gText_ReliveBox_HelpParty[], gText_ReliveBox_AskTake[];
extern const u8 gText_ReliveBox_Full1[], gText_ReliveBox_Full2[], gText_ReliveBox_NotDark1[], gText_ReliveBox_NotDark2[];
extern const u8 gText_Nushi_PartyOnly1[], gText_Nushi_PartyOnly2[]; //[hack] patches/0981 strings/hack_nushi_party.string
extern const u8 gText_ReliveBox_LastMon1[], gText_ReliveBox_LastMon2[], gText_ReliveBox_PartyFull1[], gText_ReliveBox_PartyFull2[];
extern const u8 gText_ReliveBox_NoDark[], gText_ReliveBox_NoDark2[], gText_ReliveBox_Unavail1[], gText_ReliveBox_Unavail2[];
extern const u8 gText_ReliveBox_NeedReport1[], gText_ReliveBox_NeedReport2[], gText_ReliveBox_BadData1[], gText_ReliveBox_BadData2[];
extern const u8 gText_ReliveBox_WriteFail1[], gText_ReliveBox_WriteFail2[], gText_ReliveBox_Done[];

#define ROWS 7
#define ROW_Y0 28
#define ROW_H 16

enum { ST_LIST, ST_PARTY, ST_MSG, ST_CONFIRM, ST_EXIT };

struct RlbScreen
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

static struct RlbScreen* ScreenOf(u8 taskId)
{
	return (struct RlbScreen*) (((u32) (u16) gTasks[taskId].data[0]) | ((u32) (u16) gTasks[taskId].data[1] << 16));
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

static u8 PartyDarkCount(void)
{
	u8 i, n = 0;

	for (i = 0; i < PARTY_SIZE; ++i)
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) != SPECIES_NONE
		 && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL)
		 && Relive_IsDark((struct BoxPokemon*) &gPlayerParty[i]))
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

static void DrawList(struct RlbScreen* s)
{
	static const u8 sCursor[] = {0xEF, EOS};
	u8 buf[32];
	u8* p;
	u8 r;

	DrawFrame(gText_ReliveBox_Title, gText_ReliveBox_HelpList);
	p = NumberText(buf, ReliveBox_Count(), 2);
	*p++ = 0xBA;     //'/'
	p = NumberText(p, RELIVE_BOX_SLOTS, 2);
	*p = EOS;
	Print(buf, 190, 5, 8, 6);
	for (r = 0; r < ROWS; ++r)
	{
		u8 slot = s->top + r;
		u8 y = ROW_Y0 + r * ROW_H;

		if (slot >= RELIVE_BOX_SLOTS)
			break;
		if (s->cursor == slot)
			Print(sCursor, 4, y, 4, 5);
		p = NumberText(buf, slot + 1, 2);
		*p++ = 0;
		if (!ReliveBox_SlotUsed(s->image, slot))
			p = StringCopy(p, gText_ReliveBox_Empty);
		else
		{
			struct BoxPokemon box;
			struct CompressedPokemon* comp = ReliveBox_SlotMon(s->image, slot);
			u8 gauge = HackKind_CompValue(comp);

			CreateBoxMonFromCompressedMon(&box, comp);
			GetBoxMonData(&box, MON_DATA_NICKNAME, p);
			while (*p != EOS)
				++p;
			*p++ = 0;
			if (gauge == 0)
				p = StringCopy(p, gText_ReliveBox_Ready);
			else
			{
				p = StringCopy(p, gText_ReliveBox_Gauge);
				p = NumberText(p, gauge, 3);
			}
		}
		Print(buf, 20, y, 1, 5);
	}
	PutWindowTilemap(0);
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void DrawParty(struct RlbScreen* s)
{
	static const u8 sCursor[] = {0xEF, EOS};
	u8 buf[32];
	u8* p;
	u8 i, n = PartyCount();

	DrawFrame(gText_ReliveBox_PartyTitle, gText_ReliveBox_HelpParty);
	for (i = 0; i < n; ++i)
	{
		u8 y = ROW_Y0 + i * ROW_H;

		if (s->partyCursor == i)
			Print(sCursor, 4, y, 4, 5);
		p = NumberText(buf, i + 1, 1);
		*p++ = 0;
		GetMonData(&gPlayerParty[i], MON_DATA_NICKNAME, p);
		while (*p != EOS)
			++p;
		if (Relive_IsDark((struct BoxPokemon*) &gPlayerParty[i]))
		{
			*p++ = 0;
			p = StringCopy(p, gText_ReliveBox_Gauge);
			p = NumberText(p, HackKind_Value((struct BoxPokemon*) &gPlayerParty[i]), 3);
		}
		Print(buf, 20, y, 1, 5);
	}
	PutWindowTilemap(0);
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void DrawMessage(struct RlbScreen* s)
{
	Rect(6, 0, 96, 240, 64);
	Rect(8, 0, 96, 240, 1);
	Print(s->msg1, 10, 104, 1, 6);
	if (s->msg2 != NULL)
		Print(s->msg2, 10, 120, 1, 6);
	if (s->state == ST_MSG)
		Print(gText_ReliveBox_Done, 10, 142, 8, 6);
	PutWindowTilemap(0);   //使えない版の画面は一覧を描かないので、ここでも窓を出す（出さないと真っ暗なまま）
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void Redraw(struct RlbScreen* s)
{
	if (s->state == ST_PARTY)
		DrawParty(s);
	else if (s->usable == RELIVE_BOX_OK || s->retState != ST_EXIT)
		DrawList(s);
	else
		DrawFrame(gText_ReliveBox_Title, gText_ReliveBox_Done);
	if (s->state == ST_MSG || s->state == ST_CONFIRM)
		DrawMessage(s);
}

static void ShowMsg(struct RlbScreen* s, const u8* m1, const u8* m2, u8 ret)
{
	s->msg1 = m1;
	s->msg2 = m2;
	s->retState = ret;
	s->state = ST_MSG;
	PlaySE(SE_ERROR);
	Redraw(s);
}

//---------------------------------------------------------------- 出し入れ

static void DoPut(struct RlbScreen* s)
{
	struct Pokemon* mon = &gPlayerParty[s->partyCursor];
	struct CompressedPokemon comp;
	u8 slot;

	if (GetMonData(mon, MON_DATA_IS_EGG, NULL) || !Relive_IsDark((struct BoxPokemon*) mon))
	{
		ShowMsg(s, gText_ReliveBox_NotDark1, gText_ReliveBox_NotDark2, ST_PARTY);
		return;
	}
	if (PartyNonEggCount() <= 1)   //手持ちの たたかえる子が この 1 匹だけなら あずけられない
	{
		ShowMsg(s, gText_ReliveBox_LastMon1, gText_ReliveBox_LastMon2, ST_PARTY);
		return;
	}
	if (ReliveBox_Count() >= RELIVE_BOX_SLOTS)
	{
		ShowMsg(s, gText_ReliveBox_Full1, gText_ReliveBox_Full2, ST_LIST);
		return;
	}
	CreateCompressedMonFromBoxMon((struct BoxPokemon*) mon, &comp);
	if (!ReliveBox_Put(s->image, &comp, &slot))
	{
		ShowMsg(s, gText_ReliveBox_WriteFail1, gText_ReliveBox_WriteFail2, ST_LIST);
		return;
	}
	HackRibbon_StoreFromMon((struct BoxPokemon*) mon); //[hack] patches/0977 預けるとき手持ちのリボンを表へ
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

static void DoTake(struct RlbScreen* s)
{
	struct Pokemon mon;
	struct CompressedPokemon comp;

	if (PartyCount() >= PARTY_SIZE)
	{
		ShowMsg(s, gText_ReliveBox_PartyFull1, gText_ReliveBox_PartyFull2, ST_LIST);
		return;
	}
	comp = *ReliveBox_SlotMon(s->image, s->cursor);
	CompressedMonToMon(&comp, &mon);
	if (HackNushi_PartyHasOther(gPlayerParty, &mon, PARTY_SIZE)) //[hack] patches/0981 ヌシは手持ちに 1 匹だけ（念のため・この箱の子は印がダークだけ＝ふつうは通らない）
	{
		ShowMsg(s, gText_Nushi_PartyOnly1, gText_Nushi_PartyOnly2, ST_LIST);
		return;
	}
	if (!ReliveBox_Take(s->image, s->cursor))
	{
		ShowMsg(s, gText_ReliveBox_WriteFail1, gText_ReliveBox_WriteFail2, ST_LIST);
		return;
	}
	gPlayerParty[PartyCount()] = mon;
	CalculatePlayerPartyCount();
	PlaySE(SE_SELECT);
	s->state = ST_LIST;
	Redraw(s);
}

//---------------------------------------------------------------- 画面の動き

static void MainCB2_ReliveBox(void)
{
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	UpdatePaletteFade();
}

static void VBlankCB_ReliveBox(void)
{
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

static void RlbExit(struct RlbScreen* s)
{
	s->state = ST_EXIT;
	PlaySE(SE_PC_OFF);
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
}

static void Task_ReliveBox(u8 taskId)
{
	struct RlbScreen* s = ScreenOf(taskId);
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
				RlbExit(s);
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
			RlbExit(s);
		else if (keys & (DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT | L_BUTTON | R_BUTTON))
		{
			s16 c = s->cursor;

			if (keys & DPAD_UP)
				c = (c == 0) ? RELIVE_BOX_SLOTS - 1 : c - 1;
			else if (keys & DPAD_DOWN)
				c = (c + 1 >= RELIVE_BOX_SLOTS) ? 0 : c + 1;
			else if (keys & (DPAD_LEFT | L_BUTTON))
				c = (c - ROWS < 0) ? 0 : c - ROWS;
			else
				c = (c + ROWS >= RELIVE_BOX_SLOTS) ? RELIVE_BOX_SLOTS - 1 : c + ROWS;
			s->cursor = c;
			if (s->cursor < s->top)
				s->top = s->cursor;
			else if (s->cursor >= s->top + ROWS)
				s->top = s->cursor - ROWS + 1;
			PlaySE(SE_SELECT);
			Redraw(s);
		}
		else if ((keys & START_BUTTON) || ((keys & A_BUTTON) && !ReliveBox_SlotUsed(s->image, s->cursor)))
		{
			if (PartyDarkCount() == 0)
				ShowMsg(s, gText_ReliveBox_NoDark, gText_ReliveBox_NoDark2, ST_LIST);
			else if (ReliveBox_Count() >= RELIVE_BOX_SLOTS)
				ShowMsg(s, gText_ReliveBox_Full1, gText_ReliveBox_Full2, ST_LIST);
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
			s->msg1 = gText_ReliveBox_AskTake;
			s->msg2 = NULL;
			s->state = ST_CONFIRM;
			PlaySE(SE_SELECT);
			Redraw(s);
		}
		break;
	}
}

void CB2_HackReliveBox(void)
{
	u8 taskId;
	struct RlbScreen* s;

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
	s = AllocZeroed(sizeof(struct RlbScreen));
	taskId = CreateTask(Task_ReliveBox, 0);
	gTasks[taskId].data[0] = (u32) s;
	gTasks[taskId].data[1] = (u32) s >> 16;
	s->usable = ReliveBox_Usable();
	s->state = ST_LIST;
	if (s->usable != RELIVE_BOX_OK)
	{
		if (s->usable == RELIVE_BOX_NEED_REPORT)
			ShowMsg(s, gText_ReliveBox_NeedReport1, gText_ReliveBox_NeedReport2, ST_EXIT);
		else
			ShowMsg(s, gText_ReliveBox_Unavail1, gText_ReliveBox_Unavail2, ST_EXIT);
	}
	else
	{
		s->image = Malloc(ReliveBox_ImageSize());
		if (s->image == NULL || !ReliveBox_Load(s->image))
		{
			s->usable = RELIVE_BOX_BAD_DATA;
			ShowMsg(s, gText_ReliveBox_BadData1, gText_ReliveBox_BadData2, ST_EXIT);
		}
		else
			Redraw(s);
	}
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
	SetVBlankCallback(VBlankCB_ReliveBox);
	SetMainCallback2(MainCB2_ReliveBox);
}
