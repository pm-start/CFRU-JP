#include "defines.h"
#include "../include/bg.h"
#include "../include/event_data.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/field_fadetransition.h"
#include "../include/field_screen_effect.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/sound.h"
#include "../include/sprite.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/pokemon.h"
#include "../include/constants/battle.h"
#include "../include/constants/songs.h"
#include "../include/constants/flags.h"
#include "hack_oak_phone.h"
#include "Tables/hoenn_script_flags.h"
#include "Tables/johto_flags.h"
#include "Tables/sinnoh_flags.h"
#include "../include/new/dns.h"     //TransferPlttBuffer

void __attribute__((long_call)) ScanlineEffect_Stop(void);      /* BPRJ.ld 08087A48 */
void CB2_HackPokenavReturn(void);                                      //src/hack_pokenav.c（FEAT-pokenav・ポケナビから開いたときの戻り先）

/*
 * BW 風の電話帳とフィールドの着信（pm-start/poke-call の src/phone.c を CFRU の名前で書き直した・2026-10-11
 * ユーザー「poke-call これ取り込んで」「番号を交換する相手：まずはママだけ」）。
 *
 *  - スタートメニューの「でんわ」（src/bw_start_menu.c）→ CB2_HackPhoneBook。登録した相手を上下で選んで A でかける
 *    （フィールドに戻ってから、かける台本＝assembly/hack_phone/phone_scripts.s）。B でスタートメニューへ戻る
 *  - フィールドの毎コマ（src/overworld_speedup.c）→ HackPhone_FieldTick。登録した相手からかかってくる。
 *    ★ 2026-10-11 ユーザー「でんわの頻度がたかいので調整 30分とか」: プレイ時間（gSaveBlock2）が 30 分の倍数になった瞬間（秒が 0）に 1 回。
 *    タスクの数で待つ（poke-call の 30〜98 秒）と地図を移るたびに数え直して短すぎた。セーブの変数は増やさない
 *    （台本が動いている・暗転中・バトル施設の挑戦中は数えない。地図やメニューが替わると待ち時間を引き直す＝タスクが消えるため）
 *  - 番号の交換はママの台本の頭（assembly/kanto/kanto_home_scripts.s）。印は予約フラグ 0x12CA
 *  - お母さんの貯金の電話（src/hack_mom_savings.c）は今までどおり別
 *  - 状態はタスクの data[] だけに持つ（CFRU は書き換えられる static を置けない・CLAUDE.md 6.13）
 */

#ifndef HACK_FLAG_PHONE_MOM
#define HACK_FLAG_PHONE_MOM 0x12CA         //ママの番号を登録した（build_hoenn_scripts.py の HOENN_FLAG_END の下）
#endif
#define FLAG_BATTLE_FACILITY 0x930

extern const u8 gText_HackPhone_Title[], gText_HackPhone_Book[], gText_HackPhone_Registered[], gText_HackPhone_Empty[];
extern const u8 gText_HackPhone_Help[], gText_HackPhone_NameMom[];
extern const u8 HackScr_PhoneMomOutgoing[], HackScr_PhoneMomIncoming[];
extern const u8 HackScr_PhoneOakIncoming[];
extern const u8 HackScr_PhoneLeaderGet1[], HackScr_PhoneLeaderGetMany[];
extern const u8 gText_HackPhone_NameHaruka[], gText_HackPhone_NameHikari[];            //strings/hack_heroine.string（#76 段 1）
extern const u8 HackScr_PhoneHa_Out[], HackScr_PhoneHa_In[], HackScr_PhoneHa_Menu[];   //assembly/hack_phone/phone_heroine.s
extern const u8 HackScr_PhoneHi_Out[], HackScr_PhoneHi_In[], HackScr_PhoneHi_Menu[];
bool8 HackHeroine_Met(u8 h);                //src/hack_heroine.c（#76 段 1）
u8 HackHeroine_PendingCall(void);

struct HackPhoneContact
{
	u16 badgeFlag;     //ジムリーダーのとき、そのバッジの旗（0 = ママ）
	u16 flag;          //番号をもらった（登録した）印
	const u8 *name;
	const u8 *outgoing;
	const u8 *incoming;
	const u8 *menu;    //電話帳で A を押したときのメニュー（かける／さいせんに さそう／やめる・#117）。NULL ならすぐ outgoing
	const u8 *get;     //バッジをもらって番号を登録したときの、リーダーの一言の台本（#117・登録が 1 人のとき）。NULL なら無し
};

#include "Tables/hack_phone_leaders.h"

static const struct HackPhoneContact sContacts[] =
{
	{0, HACK_FLAG_PHONE_MOM, gText_HackPhone_NameMom, HackScr_PhoneMomOutgoing, HackScr_PhoneMomIncoming, NULL, NULL},
	PHONE_LEADER_ROWS     //ジムリーダー 32 人（tools/phone/build_phone_leaders.py・#115）
	//ハルカ・ヒカリ（#76 段 1・src/hack_heroine.c）: ★ 通常の着信の抽選（ママ・リーダー）には入れない（PHONE_RANDOM_COUNT より後ろ）。着信は専用経路
	{0, HACK_FLAG_PHONE_HARUKA, gText_HackPhone_NameHaruka, HackScr_PhoneHa_Out, HackScr_PhoneHa_In, HackScr_PhoneHa_Menu, NULL},
	{0, HACK_FLAG_PHONE_HIKARI, gText_HackPhone_NameHikari, HackScr_PhoneHi_Out, HackScr_PhoneHi_In, HackScr_PhoneHi_Menu, NULL},
};
#define PHONE_COUNT NELEMS(sContacts)
#define PHONE_RANDOM_COUNT (1 + PHONE_LEADER_COUNT)    //通常の着信の抽選とバッジでの登録を見る範囲（ママ + ジムリーダー）。その後ろがハルカ・ヒカリ

/* ---- 電話帳の画面 ---- */
enum { STATE, SELECTED, COUNT };     //data[]（登録した相手は旗から数える＝NthRegistered・33 人まで増えても data[] に持たない）
enum { BOOK, EXIT, CALL };

static const struct BgTemplate sBgTemplates[] =
{
	{.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
	{.bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = 30, .height = 20, .paletteNum = 15, .baseBlock = 1},
	DUMMY_WIN_TEMPLATE,
};

//poke-call と同じ 11 色（0 透明・1 白・2 濃い灰・3 水色・4 黄・5 紺・6 濃い灰・7 青・8 明るい灰・9 黒・10 緑）
static const u16 sPalette[16] = {0, 0x7FFF, 0x1084, 0x7FE0, 0x03FF, 0x0842, 0x18C6, 0x3180, 0x6739, 0x0421, 0x56A0};

static void Print(const u8 *text, u8 x, u8 y, u8 color, u8 bg)
{
	u8 colors[3];

	colors[0] = bg;
	colors[1] = color;
	colors[2] = (bg == 1) ? 8 : 2;
	AddTextPrinterParameterized3(0, 2, x, y, colors, 0, text);
}

static void Rect(u8 color, u16 x, u16 y, u16 w, u16 h)
{
	FillWindowPixelRect(0, color, x, y, w, h);
}

static void Frame(u16 x, u16 y, u16 w, u16 h, u8 fill)
{
	Rect(8, x, y, w, h);
	Rect(9, x + 1, y + 1, w - 2, h - 2);
	Rect(fill, x + 2, y + 2, w - 4, h - 4);
}

static void Receiver(u16 x, u16 y, u8 color)    //受話器の絵（poke-call の receiver と同じ形）
{
	Rect(color, x, y, 10, 6);
	Rect(color, x + 3, y + 5, 7, 8);
	Rect(color, x + 7, y + 12, 7, 6);
	Rect(color, x + 12, y + 17, 8, 6);
	Rect(color, x + 19, y + 19, 6, 9);
	Rect(color, x + 19, y + 25, 11, 6);
}

//電話帳に載せる相手か（旗が立っている。ハルカ・ヒカリはその地方のリーグ制覇まで載せない・旗は消さない・patches/0920 #76）
bool8 HackHeroine_Unlocked(u8 h);
static bool8 IsListed(u32 i)
{
	if (!FlagGet(sContacts[i].flag))
		return FALSE;
	return (i >= PHONE_RANDOM_COUNT) ? HackHeroine_Unlocked(i - PHONE_RANDOM_COUNT) : TRUE;
}

//登録した相手のうち n 番目（0 始まり）の sContacts の番号（無ければ PHONE_COUNT）
static u32 NthRegistered(u32 n)
{
	u32 i;

	for (i = 0; i < PHONE_COUNT; ++i)
	{
		if (IsListed(i))
		{
			if (n == 0)
				return i;
			--n;
		}
	}
	return PHONE_COUNT;
}

static void Draw(s16 *d)
{
	u32 j, top, len;
	u8 number[4];

	FillWindowPixelBuffer(0, 0x55);
	Rect(6, 0, 0, 240, 23);
	Rect(3, 0, 22, 240, 1);
	Print(gText_HackPhone_Title, 10, 5, 1, 6);
	Print(gText_HackPhone_Book, 150, 5, 8, 6);
	Rect(6, 0, 138, 240, 22);
	Rect(8, 0, 138, 240, 1);
	Frame(7, 28, 144, 104, 5);
	Frame(157, 28, 76, 104, 5);
	Receiver(180, 44, 3);
	Print(gText_HackPhone_Registered, 172, 85, 8, 5);
	{
		u32 tens = 0, ones = d[COUNT];     //★ 割り算も余りも使わない（CLAUDE.md 6.83）。33 人まで

		while (ones >= 10)
		{
			ones -= 10;
			++tens;
		}
		len = 0;
		if (tens != 0)
			number[len++] = 0xA1 + tens;     //全角の数字（0xA1 = ０）
		number[len++] = 0xA1 + ones;
		number[len] = EOS;
	}
	Print(number, (len == 1) ? 190 : 182, 103, 1, 5);
	if (d[COUNT] == 0)
	{
		Frame(7, 56, 226, 46, 1);
		Print(gText_HackPhone_Empty, 12, 68, 9, 1);
	}
	top = (d[SELECTED] >= 6) ? d[SELECTED] - 5 : 0;     //選んでいる行が 6 行目（いちばん下）になるまで 1 行ずつ送る
	for (j = top; j < (u32) d[COUNT] && j < top + 6; ++j)
	{
		u16 y = 32 + (j - top) * 16;
		bool8 selected = (j == (u32) d[SELECTED]);

		if (selected)
		{
			Rect(7, 10, y, 138, 16);
			Rect(3, 10, y, 3, 16);
		}
		else
			Rect(6, 14, y + 15, 130, 1);
		Print(sContacts[NthRegistered(j)].name, 22, y, 1, selected ? 7 : 5);
	}
	if (d[COUNT] > 6)     //つづきがあるとき右に細い目盛り（つまみの高さは 6 行ぶん ÷ 全体）
	{
		u32 thumb = 96 * 6 / d[COUNT];
		u32 room = 96 - thumb;
		u32 pos = room * top / (d[COUNT] - 6);

		Rect(6, 146, 32, 2, 96);
		Rect(3, 146, 32 + pos, 2, thumb);
	}
	Print(gText_HackPhone_Help, 8, 143, 1, 6);
	CopyWindowToVram(0, COPYWIN_BOTH);
}

static void MainCB2_HackPhoneBook(void)
{
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	UpdatePaletteFade();
}

static void VBlankCB_HackPhoneBook(void)
{
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

/*
 * ★ 2026-10-17 fix/phone-menu（ユーザー報告「でんわの項目でカーソルを上下すると主人公も一緒に動く」）
 * 前は電話帳で ScriptContext1_SetupScript(台本) を先に呼んでから CB2_ReturnToFieldContinueScriptPlayMapMusic で野外を作り直していた。
 * その形だと台本（状態 RUNNING）は地図を組み直してフェードインしている最中の最初のコマから走り出し、フェードが終わったあとに
 * 素の Task_ContinueScript が EnableBothScriptContexts で台本をもう一度「走れ」にする（待ち状態の選択肢を勝手に先へ進められる）。
 * 台本の最中にロックがほどける・入力が通る隙間もここにあった（推測: 実機で確かめていない）。
 * → 野外が完全に出てから（フェードが終わってから）、着信と同じ手順（ScriptContext2_Enable → SetupScript）で台本を始める。
 *   台本の番地は状態を持てない（CLAUDE.md 6.13）ので Var8004/8005（セーブされない台本の一時変数）に 16 ビットずつ置いて渡す。
 */
static void Task_HackPhoneStartScript(u8 taskId)
{
	const u8 *script;

	if (!FieldFadeTransitionBackgroundEffectIsFinished())
		return;
	script = (const u8 *) (((u32) Var8005 << 16) | Var8004);
	DestroyTask(taskId);
	ScriptContext2_Enable();
	ScriptContext1_SetupScript(script);
}

static void FieldCB_HackPhoneStartScript(void)
{
	ScriptContext2_Enable();            //フェードインの間も主人公を動かさない
	Overworld_PlaySpecialMapMusic();
	FadeInFromBlack();
	CreateTask(Task_HackPhoneStartScript, 10);
}

//callasm: 台本の選択肢を出す直前にもう一度フィールドの入力を止める（電話の項目の選択中に主人公を歩かせない）
void HackPhone_LockField(void)
{
	ScriptContext2_Enable();
}

static void Task_HackPhoneBook(u8 taskId)
{
	s16 *d = gTasks[taskId].data;
	u16 keys;

	if (gPaletteFade->active)
		return;
	if (d[STATE] == EXIT || d[STATE] == CALL)
	{
		const struct HackPhoneContact *picked = (d[STATE] == CALL) ? &sContacts[NthRegistered(d[SELECTED])] : NULL;
		const u8 *script = (picked == NULL) ? NULL : (picked->menu != NULL) ? picked->menu : picked->outgoing;   //★ #117 ジムリーダーは かける／さいせんに さそう のメニューから

		MainCallback backTo = (gMain.savedCallback == CB2_HackPokenavReturn) ? CB2_HackPokenavReturn : NULL;   //★ FEAT-pokenav: ポケナビから開いたときだけ入っている（それ以外は今までと同じ＝触らない）
		if (backTo != NULL)
			gMain.savedCallback = NULL;

		SetVBlankCallback(NULL);
		DestroyTask(taskId);
		FreeAllWindowBuffers();
		ResetSpriteData();
		FreeAllSpritePalettes();
		if (script != NULL)
		{
			Var8004 = (u16) ((u32) script & 0xFFFF);      //台本の番地を渡す（Task_HackPhoneStartScript が読む）
			Var8005 = (u16) ((u32) script >> 16);
			gFieldCallback = FieldCB_HackPhoneStartScript;
			SetMainCallback2(CB2_ReturnToField);
		}
		else if (backTo == CB2_HackPokenavReturn)
			SetMainCallback2(CB2_HackPokenavReturn);       //ポケナビから開いたときはポケナビへ（スタートメニューから開いたときは下の今までの動き）
		else
			SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
		return;
	}
	keys = gMain.newKeys;
	if (keys & B_BUTTON)
		d[STATE] = EXIT;
	else if ((keys & A_BUTTON) && d[COUNT] != 0)
		d[STATE] = CALL;
	else if (d[COUNT] != 0)
	{
		if (keys & DPAD_UP)
			d[SELECTED] = (d[SELECTED] != 0) ? d[SELECTED] - 1 : d[COUNT] - 1;
		if (keys & DPAD_DOWN)
			d[SELECTED] = (d[SELECTED] + 1 < d[COUNT]) ? d[SELECTED] + 1 : 0;
	}
	if (d[STATE] != BOOK)
	{
		PlaySE(SE_SELECT);
		BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
	}
	else if (keys & (DPAD_UP | DPAD_DOWN))
	{
		PlaySE(SE_SELECT);
		Draw(d);
	}
}

void CB2_HackPhoneBook(void)
{
	u32 i;
	u8 taskId;
	s16 *d;

	SetVBlankCallback(NULL);
	SetHBlankCallback(NULL);
	SetGpuReg(REG_OFFSET_DISPCNT, 0);
	DmaFill16(3, 0, (void *) VRAM, VRAM_SIZE);
	ResetBgsAndClearDma3BusyFlags(0);
	InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
	for (i = 0; i < 4; ++i)
	{
		ChangeBgX(i, 0, 0);
		ChangeBgY(i, 0, 0);
	}
	InitWindows(sWindowTemplates);
	DeactivateAllTextPrinters();
	for (i = REG_OFFSET_BLDCNT; i <= REG_OFFSET_BLDY; i += 2)
		SetGpuReg(i, 0);
	ResetTasks();
	ResetSpriteData();
	FreeAllSpritePalettes();
	ResetPaletteFade();
	ScanlineEffect_Stop();
	LoadPalette(sPalette, 0xF0, sizeof(sPalette));
	taskId = CreateTask(Task_HackPhoneBook, 0);
	d = gTasks[taskId].data;
	for (i = 0; i < PHONE_COUNT; ++i)
		if (IsListed(i))
			++d[COUNT];
	PutWindowTilemap(0);
	Draw(d);
	ShowBg(0);
	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_OBJ_ON);
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
	SetVBlankCallback(VBlankCB_HackPhoneBook);
	SetMainCallback2(MainCB2_HackPhoneBook);
}

/* ---- フィールドの着信 ---- */
#define CALL_EVERY_MINUTES 30     //プレイ時間がこの倍数（分）になった瞬間に 1 回（2026-10-11）
#define CALL_LOCKOUT_FRAMES 600   //鳴らしたあと 10 秒は見ない（同じ 1 秒の中で 2 度鳴らさない）

static bool8 IsCallMoment(void)
{
	u32 total = gSaveBlock2->playTimeHours * 60 + gSaveBlock2->playTimeMinutes;

	return total != 0 && total % CALL_EVERY_MINUTES == 0 && gSaveBlock2->playTimeSeconds == 0;
}

static u32 RegisteredCount(void)
{
	u32 i, n = 0;
	for (i = 0; i < PHONE_RANDOM_COUNT; ++i)
		n += FlagGet(sContacts[i].flag) ? 1 : 0;
	return n;
}

//ジムリーダーの番号（2026-10-16・#115）: そのジムのバッジの旗が立っていて、まだ番号をもらっていない人を見つけたら、
//全員ぶんをまとめて登録して台本 1 本で知らせる（前のセーブでバッジをたくさん持っていても何本も出さない）。
//毎コマの負担を減らすため、見るのは 1 コマに 1 人（data[1] が見る番号）。
//★ 2026-10-16 #117: 新しくバッジを取った（1 人だけ）ときは、そのリーダーの一言 → 「〇〇から でんわばんごうが とどいた！」。前のセーブでバッジをたくさん持っていて
//  まとめて登録するときは「ジムリーダーたちから …」の 1 本。ジムの台本（32 か所・カントーの分は素の ROM の台本）には手を入れず、バッジの会話が終わって
//  フィールドが動いた最初のコマで出す（登録の場所はここだけ＝台本とフィールドで二重に登録しない）
static bool8 TryGiveLeaderNumbers(s16 *d)
{
	u32 i, n = 0, first = 0;
	const struct HackPhoneContact *c = &sContacts[1 + d[1]];

	d[1] = (d[1] + 1 < PHONE_LEADER_COUNT) ? d[1] + 1 : 0;
	if (!FlagGet(c->badgeFlag) || FlagGet(c->flag))
		return FALSE;
	for (i = 1; i < PHONE_RANDOM_COUNT; ++i)
	{
		if (FlagGet(sContacts[i].badgeFlag) && !FlagGet(sContacts[i].flag))
		{
			if (n == 0)
			{
				first = i;
				StringCopy(gStringVar1, sContacts[i].name);
			}
			FlagSet(sContacts[i].flag);
			++n;
		}
	}
	ConvertIntToDecimalStringN(gStringVar2, n, STR_CONV_MODE_LEFT_ALIGN, 2);
	ScriptContext2_Enable();
	ScriptContext1_SetupScript((n == 1 && sContacts[first].get != NULL) ? sContacts[first].get : (n == 1) ? HackScr_PhoneLeaderGet1 : HackScr_PhoneLeaderGetMany);
	return TRUE;
}

//ハルカ・ヒカリの番号（#76 段 1）: 会った印（ホウエン序章・シンオウ序章）が立っていて まだ登録していなければ登録し、リーダーと同じ「とどいた！」の台本で知らせる
static bool8 TryGiveHeroineNumbers(void)
{
	u32 h;

	for (h = 0; h < 2; ++h)
	{
		const struct HackPhoneContact *c = &sContacts[PHONE_RANDOM_COUNT + h];

		if (HackHeroine_Met(h) && HackHeroine_Unlocked(h) && !FlagGet(c->flag))   //[hack] patches/0920 クリアするまで番号は届かない
		{
			FlagSet(c->flag);
			StringCopy(gStringVar1, c->name);
			ScriptContext2_Enable();
			ScriptContext1_SetupScript(HackScr_PhoneLeaderGet1);
			return TRUE;
		}
	}
	return FALSE;
}

//callasm: 再戦の案内を「もう できる」にするか（setvar 0x8004 = 地方 0 カントー 1 ホウエン）→ LASTRESULT 1 = できる
//  カントーは殿堂入り済み（FLAG_SYS_GAME_CLEAR）・ホウエンはホウエンリーグを 1 回以上制覇（HOENN_VAR_LEAGUE_WINS）＝リーダーの再戦と同じ条件
//★ 2026-10-16 ジョウト・シンオウのリーダーの再戦（assembly/hack_leader_rematch/js_leader_rematch.s）も同じ関数で見る:
//  ジョウト（2）＝ジョウトのバッジ 8 つ（ジョウトにはリーグの部屋が無い＝殿堂入りの旗が立たないので）・シンオウ（3）＝殿堂入りの旗（SINNOH_FLAG_LEAGUE_HOF）
//★ 2026-10-16 #117: 条件の判定は src/hack_gym_tiers.c の HackGymTier_RegionOpen に 1 本化した（再戦を誘える条件と同じ）。
//  シンオウは SINNOH_FLAG_LEAGUE_HOF（殿堂入りのあと消える一度きりの印）ではなく、残る印 HACK_FLAG_SINNOH_CLEARED を見る（前は殿堂入りのあとで偽になっていた）
extern bool8 HackGymTier_RegionOpen(u8 region);
bool8 HoennHasTrainerBeenFought(u16 trainerId);
void HoennSetTrainerFlag(u16 trainerId);
void HoennClearTrainerFlag(u16 trainerId);

/* 空欄とタマゴを除いた手持ちのレベルの整数平均。数えられる相手が 0 なら 0。 */
static u8 HackPhone_PartyAverage(void)
{
	u32 sum = 0;
	u32 n = 0;
	u32 i;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		const struct Pokemon *mon = &gPlayerParty[i];

		/* 欄を直接読む（毎コマ呼ばれるので GetMonData の 18 回は重い・6.80・docs/research/2026-11-02_perf-hotpath-2.md）。値は素の GetMonData と同じ:
		   SPECIES = 壊れたタマゴ（sanity の bit0）なら SPECIES_EGG・でなければ種族の欄／IS_EGG = isEgg の欄／LEVEL = level の欄（暗号は bytereplacement で外してある） */
		if (!(mon->sanity & 1) && mon->species == SPECIES_NONE)
			continue;
		if (mon->isEgg)
			continue;
		sum += mon->level;
		++n;
	}
	if (n == 0)
		return 0;
	return (u8)(sum / n);
}

static bool8 HackPhone_ShouldCallOak(void)
{
	if (FlagGet(HACK_FLAG_OAK_ACCEPTED))
		return FALSE;
	if (HackPhone_PartyAverage() < HACK_PHONE_OAK_LEVEL)
		return FALSE;
	/* 初回はすぐ鳴る。いちど断ったら、ほかの人と同じ 30 分の枠でだけ鳴らす（約 10 秒ごとに鳴り続けていた）。 */
	if (FlagGet(HACK_FLAG_OAK_DECLINED))
		return IsCallMoment();
	return TRUE;
}

void HackPhone_OakDeclined(void)
{
	FlagSet(HACK_FLAG_OAK_DECLINED);
}

void HackPhone_OakPrepareBattle(void)
{
	if (HoennHasTrainerBeenFought(HACK_PHONE_OAK_TRAINER))
		FlagSet(HACK_FLAG_OAK_RESTORE);
	else
		FlagClear(HACK_FLAG_OAK_RESTORE);
	HoennClearTrainerFlag(HACK_PHONE_OAK_TRAINER);
	FlagSet(HACK_FLAG_OAK_ACCEPTED);
	FlagSet(HACK_FLAG_OAK_PIC);
}

extern u8 gBattleOutcome;

void HackPhone_OakFinishBattle(void)
{
	if (FlagGet(HACK_FLAG_OAK_RESTORE))
		HoennSetTrainerFlag(HACK_PHONE_OAK_TRAINER);
	else
		HoennClearTrainerFlag(HACK_PHONE_OAK_TRAINER);
	FlagClear(HACK_FLAG_OAK_RESTORE);
	FlagClear(HACK_FLAG_OAK_PIC);
}

void HackPhone_OakOnBattleEnd(void)
{
	if (!FlagGet(HACK_FLAG_OAK_PIC) && !FlagGet(HACK_FLAG_OAK_RESTORE))
		return;
	FlagClear(HACK_FLAG_OAK_PIC);
	/* 勝ちでもここで借りた旗を戻す。trainerbattle 0 は勝ったあと元の台本に戻らない。
	   素の台本は EndOfBattleThings のあとで SetTrainerFlags するので、
	   勝ちのときは RESTORE を残し、続きの台本がもう一度戻してから消す。 */
	if (FlagGet(HACK_FLAG_OAK_RESTORE))
		HoennSetTrainerFlag(HACK_PHONE_OAK_TRAINER);
	else
		HoennClearTrainerFlag(HACK_PHONE_OAK_TRAINER);
	if (gBattleOutcome != B_OUTCOME_WON)
		FlagClear(HACK_FLAG_OAK_RESTORE);
}

/* 借りたトレーナーの手持ちは 1 匹。画像の旗があるあいだだけ、御三家の最終進化・レントラー・ガブリアス・カイリューを Lv170 で並べる。 */
void HackPhone_OakFillParty(void)
{
	static const u16 sSpecies[PARTY_SIZE] = {
		SPECIES_VENUSAUR,
		SPECIES_CHARIZARD,
		SPECIES_BLASTOISE,
		SPECIES_LUXRAY,
		SPECIES_GARCHOMP,
		SPECIES_DRAGONITE,
	};
	u8 i;

	if (!FlagGet(HACK_FLAG_OAK_PIC))
		return;
	ZeroEnemyPartyMons();
	for (i = 0; i < PARTY_SIZE; ++i)
		CreateMon(&gEnemyParty[i], sSpecies[i], HACK_PHONE_OAK_LEVEL, 31, FALSE, 0, OT_ID_RANDOM_NO_SHINY, 0);
	gEnemyPartyCount = PARTY_SIZE;
}

void HackPhone_RematchOpen(void)
{
	gSpecialVar_LastResult = HackGymTier_RegionOpen(VarGet(0x8004));
}

static void Task_HackPhoneIncoming(u8 taskId)
{
	s16 *d = gTasks[taskId].data;
	u32 i, count, chosen;

	if (gPaletteFade->active || ScriptContext2_IsEnabled() || gMain.callback2 != CB2_Overworld)
		return;
	if (FlagGet(FLAG_BATTLE_FACILITY))
		return;
	if (d[0] > 0)
	{
		--d[0];
		return;
	}
	if (TryGiveLeaderNumbers(d))
	{
		d[0] = 60;
		return;
	}
	if (TryGiveHeroineNumbers())
	{
		d[0] = 60;
		return;
	}
	if (HackPhone_ShouldCallOak())
	{
		d[0] = CALL_LOCKOUT_FRAMES;
		ScriptContext2_Enable();
		ScriptContext1_SetupScript(HackScr_PhoneOakIncoming);
		PlaySE(SE_SELECT);
		return;
	}
	if (!IsCallMoment())
		return;
	{
		u8 who = HackHeroine_PendingCall();      //[hack] #76 段 1: 段が上がったハルカ／ヒカリの専用の着信（通常の抽選に入れない）

		if (who < 2)
		{
			d[0] = CALL_LOCKOUT_FRAMES;
			ScriptContext2_Enable();
			ScriptContext1_SetupScript(sContacts[PHONE_RANDOM_COUNT + who].incoming);
			PlaySE(SE_SELECT);
			return;
		}
	}
	count = RegisteredCount();
	if (count == 0)
		return;
	d[0] = CALL_LOCKOUT_FRAMES;
	chosen = Random() % count;
	for (i = 0; i < PHONE_RANDOM_COUNT; ++i)
	{
		if (!FlagGet(sContacts[i].flag))
			continue;
		if (chosen != 0)
		{
			--chosen;
			continue;
		}
		ScriptContext2_Enable();
		ScriptContext1_SetupScript(sContacts[i].incoming);
		PlaySE(SE_SELECT);
		return;
	}
}

//デバッグメニュー（src/hack_debug_menu.c）から、登録の有無に関係なく着信の台本を取る
u8 HackPhone_ContactCount(void)
{
	return PHONE_COUNT;
}

const u8 *HackPhone_ContactName(u8 index)
{
	if (index >= PHONE_COUNT)
		index = 0;
	return sContacts[index].name;
}

const u8 *HackPhone_IncomingScript(u8 index)
{
	if (index >= PHONE_COUNT)
		index = 0;
	return sContacts[index].incoming;
}

//src/overworld_speedup.c が毎コマ呼ぶ（フィールドの間だけ）
void HackPhone_FieldTick(void)
{
	if (FindTaskIdByFunc(Task_HackPhoneIncoming) == 0xFF)
	{
		u8 taskId = CreateTask(Task_HackPhoneIncoming, 80);
		if (taskId != 0xFF)
			gTasks[taskId].data[0] = 60;   //地図に入って 1 秒は鳴らさない
	}
}
