#include "defines.h"
#include "../include/event_data.h"
#include "../include/item.h"
#include "../include/item_use.h"
#include "../include/pokemon.h"
#include "../include/script.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/util.h"
#include "../include/party_menu.h"
#include "../include/constants/items.h"
#include "../include/constants/species.h"
#include "hack_stat_access.h" //[hack] patches/0763 努力値・個体値の窓口

/*
 * さびたおうかん（ITEM_RUSTY_CROWN 0x365・2026-10-16・patches/0433）: ぎんのおうかん（すごいとっくん）の逆。
 * ユーザー「王冠の逆になる、任意の個体値を 0（最低値）にできる道具」（遊んでいる人の要望）＝第 2 特性で天気が変わりやすく、素早さを最低に・
 * 攻撃を下げてイカサマのダメージを下げたい、という使い方。
 *
 *  - バッグから使う（ITEM_USE_FIELD・おもいだしノートと同じ道）: バッグを閉じて EventScript_RustyCrown（assembly/hack_rusty_crown/rusty_crown_scripts.s）
 *    ＝ ポケモンを選ぶ → 能力を選ぶ（HP・こうげき・ぼうぎょ・とくこう・とくぼう・すばやさ）→ 今の個体値を見せて はい／いいえ → 0 にする。
 *  - 王冠は個体値を書き換えず hyperTraining のビットを立てる（src/scripting.c の sp12F_SetHyperTraining・計算とステータス画面だけが 31 として読む）。
 *    なので 0 にするときは その能力の hyperTraining のビット（1 << 能力）も下ろす（下ろさないと 31 扱いのまま）。
 *    hyperTraining の bit6（0427 の古い持ち方）と bit7（うんめいの印・patches/0286）には触らない。
 *  - 「今の個体値」＝ 計算と同じ読み方（ビットが立っていれば 31・そうでなければ生の値）。すでに 0（ビットも無し）なら「こうかが ないようだ」で減らさない。
 *  - 性格値（PID）・表 ID／裏 ID（#70）・性格・特性には触らない（6.5＝色違いは色違いのまま）。セーブの形も変えない（個体値の欄とビットを書くだけ）。
 *  - 減らすのは 0 にできたとき（RemoveBagItem が通ったとき）だけ。タマゴには使えない。
 *
 * 能力の番号は MON_DATA_HP_IV + i と gBitTable[i] の並び（HP 0・こうげき 1・ぼうぎょ 2・すばやさ 3・とくこう 4・とくぼう 5）。
 * 台本の引数: Var8004 = 手持ちの番号・Var8005 = 能力の番号。結果は Var800D（LASTRESULT）。
 */

#define RUSTY_STAT_COUNT 6
#define HYPER_STAT_BITS 0x3F   //hyperTraining の bit0〜5（能力ごと）。bit6・bit7 は別の使い道

extern const u8 gText_KantoHyper_HP[];
extern const u8 gText_KantoHyper_Atk[];
extern const u8 gText_KantoHyper_Def[];
extern const u8 gText_KantoHyper_Spe[];
extern const u8 gText_KantoHyper_SpAtk[];
extern const u8 gText_KantoHyper_SpDef[];
extern const u8 EventScript_RustyCrown[];

//能力の番号（MON_DATA_HP_IV + i の並び）→ 名前
static const u8* const sStatNames[RUSTY_STAT_COUNT] =
{
	gText_KantoHyper_HP,
	gText_KantoHyper_Atk,
	gText_KantoHyper_Def,
	gText_KantoHyper_Spe,
	gText_KantoHyper_SpAtk,
	gText_KantoHyper_SpDef,
};

//計算（CalculateMonStatsNew）と同じ読み方の「今の個体値」: hyperTraining のビットが立っていれば 31
static u8 EffectiveIv(struct Pokemon* mon, u8 stat)
{
	if (mon->hyperTraining & gBitTable[stat] & HYPER_STAT_BITS)
		return 31;
	return HackMon_GetIv(mon, stat);
}

static bool8 TargetIsValid(void)
{
	if (Var8004 >= PARTY_SIZE || Var8005 >= RUSTY_STAT_COUNT)
		return FALSE;
	return !GetMonData(&gPlayerParty[Var8004], MON_DATA_IS_EGG, NULL)
	    && GetMonData(&gPlayerParty[Var8004], MON_DATA_SPECIES, NULL) != SPECIES_NONE;
}

//callasm: 文の差し込み（[BUFFER1] = ニックネーム・[BUFFER2] = 能力の名前・[BUFFER3] = 今の個体値）を用意して、
//0 にできるか（効果があるか）を Var800D（LASTRESULT）に返す（1 = できる・0 = もう 0 / 選び方がおかしい）
void HackRustyCrown_Prepare(void)
{
	struct Pokemon* mon;
	u8 iv;

	Var800D = 0;
	if (!TargetIsValid())
		return;
	mon = &gPlayerParty[Var8004];
	GetMonNickname(mon, gStringVar1);
	StringCopy(gStringVar2, sStatNames[Var8005]);
	iv = EffectiveIv(mon, Var8005);
	ConvertIntToDecimalStringN(gStringVar3, iv, STR_CONV_MODE_LEFT_ALIGN, 2);
	Var800D = (iv != 0) ? 1 : 0;
}

//callasm: 個体値を 0 にする（道具を 1 つ減らせたときだけ）。Var800D = 1 なら 0 にした
void HackRustyCrown_Apply(void)
{
	struct Pokemon* mon;
	u16 oldHP, maxHP;

	Var800D = 0;
	if (!TargetIsValid())
		return;
	mon = &gPlayerParty[Var8004];
	if (EffectiveIv(mon, Var8005) == 0)
		return;                                      //効果なし（道具は減らさない）
	if (!RemoveBagItem(ITEM_RUSTY_CROWN, 1))
		return;

	oldHP = GetMonData(mon, MON_DATA_HP, NULL);
	HackMon_SetIv(mon, Var8005, 0);   //生の個体値
	mon->hyperTraining &= ~(u8) (gBitTable[Var8005] & HYPER_STAT_BITS);   //その能力の「鍛えた」印も下ろす（bit6・bit7 はそのまま）
	CalculateMonStats(mon);

	maxHP = GetMonData(mon, MON_DATA_MAX_HP, NULL);
	if (oldHP == 0)
		SetMonData(mon, MON_DATA_HP, &oldHP);   //ひんしのまま
	else if (GetMonData(mon, MON_DATA_HP, NULL) > maxHP || GetMonData(mon, MON_DATA_HP, NULL) == 0)
	{
		oldHP = (oldHP > maxHP) ? maxHP : oldHP;
		SetMonData(mon, MON_DATA_HP, &oldHP);   //ひんしにしない・最大をこえない
	}
	Var800D = 1;
}

/* ---- バッグから使う（おもいだしノートと同じ道） ---- */
static void Task_RustyCrownField(u8 taskId)
{
	ScriptContext1_SetupScript(EventScript_RustyCrown);
	DestroyTask(taskId);
}

void FieldUseFunc_RustyCrown(u8 taskId)
{
	sItemUseOnFieldCB = Task_RustyCrownField;
	SetUpItemUseOnFieldCallback(taskId);
}
