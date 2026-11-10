#include "defines.h"
#include "defines_battle.h"
#include "../include/pokemon.h"
#include "../include/constants/abilities.h"
#include "hack_mon_id.h"
#include "hack_debug_mon.h"

/*
	デバッグモン（デバッグ用の種族・見た目はレントラー・tools/custom_mon/debug_mon・patches/0422）。
	2026-10-16 ユーザー「デバッグ用の種族とポケモンが欲しい。グラフィックはレントラーで」→「D（全部入り）」。

	種族は 2 つ（SPECIES_DEBUGMON_MAX＝種族値が全部 255・SPECIES_DEBUGMON_MIN＝全部 1）。種族の表（ROM）は書き換えられない（6.13）ので、
	タイプ 2 つと特性は「ポケモン自身」に持たせる。置き場は **性格値（personality・32 ビット）**:
	    bit 0〜4   タイプ 1（0〜31）           bit 5〜9   タイプ 2            bit 10〜19  特性の番号（0 = 種族の標準）
	    bit 20〜31 目印 0xDB5（目印が無い子 ＝ 切り替えていない子は種族の表のまま＝レントラーと同じ でんき）
	理由（テラスタイプなど別の置き場を使わなかった理由）:
	  - テラスタイプ（teratype）は 1 バイト・箱に預けると下 5 ビットしか残らない（上 3 ビットは #70 の ID の置き場・hack_mon_id.c）ので 2 つのタイプと特性は入らない
	  - 性格値は箱に預けても丸ごと残る（縮めた 58 バイトの頭 0x1C バイトに入る）。性格・性別・色違いは性格値に頼らない作りが既に整っている:
	    性格は natureMint（ミントの欄）、色違い・Moémon は #70 の表 ID・裏 ID、デバッグモンは性別なし（MON_GENDERLESS）。
	    切り替える前に HackMonId_Ensure で ID を固定し、性格を natureMint に写してから性格値を書く（6.5 の「性格値を作り直す」は色違いが変わる
	    実装の禁止＝ID を先に固定するので色違いは変わらない）
	  - 目印を 12 ビットにしたので、切り替えていない子（ふつうに作った子の性格値はでたらめ）が目印に当たるのは約 1/4096
	    （さらにタイプ 2 つが有効な組でなければ読まない）。当たったときはタイプ・特性が変わる＝デバッグ用なので許容（デバッグメニューで直せる）
	特性は GetMonAbility（src/build_pokemon.c）の頭・タイプは GetMonType（src/util.c）と戦闘に出るとき（switching.c・form_change.c）に差し込む
	（りゅうのいわお patches/0207 と同じ口）。

	★ 配る版（HACK_DEBUG_MENU の無いビルド）: HackDebugMon_Type は base をそのまま返し、HackDebugMon_Ability は ABILITY_NONE を返すだけ
	  （タイプ・特性を切り替える処理は入らない）。種族の番号と表の行だけが同じ（番号はセーブの互換のため）
*/

#ifdef HACK_DEBUG_MENU

#define DM_MARK 0xDB5u                 //性格値の上 12 ビット
#define DM_TYPE_BITS 5
#define DM_ABILITY_SHIFT 10
#define DM_ABILITY_MASK 0x3FFu         //10 ビット（ABILITIES_COUNT は 327）

//選べるタイプ（リルを含む 19 種・メニューの添字の順）
static const u8 sDebugTypes[] =
{
	TYPE_NORMAL, TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND, TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL,
	TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY, TYPE_RIL,
};

u8 HackDebugMon_TypeCount(void)
{
	return ARRAY_COUNT(sDebugTypes);
}

u8 HackDebugMon_TypeOfIndex(s32 index)
{
	if (index < 0 || index >= (s32) ARRAY_COUNT(sDebugTypes))
		index = 0;
	return sDebugTypes[index];
}

static s32 IndexOfType(u8 type)
{
	u32 i;

	for (i = 0; i < ARRAY_COUNT(sDebugTypes); ++i)
		if (sDebugTypes[i] == type)
			return i;
	return -1;
}

//目印があり、タイプ 2 つが選べるタイプで、特性の番号が範囲内なら TRUE（そうでなければ切り替えていない子）
static bool8 Decode(const struct Pokemon* mon, u8* t1, u8* t2, u16* ability)
{
	u32 p = mon->personality;
	u8 a = p & ((1 << DM_TYPE_BITS) - 1);
	u8 b = (p >> DM_TYPE_BITS) & ((1 << DM_TYPE_BITS) - 1);
	u16 ab = (p >> DM_ABILITY_SHIFT) & DM_ABILITY_MASK;

	if ((p >> 20) != DM_MARK || IndexOfType(a) < 0 || IndexOfType(b) < 0 || ab >= ABILITIES_COUNT)
		return FALSE;
	*t1 = a;
	*t2 = b;
	*ability = ab;
	return TRUE;
}

u8 HackDebugMon_Type(const struct Pokemon* mon, u16 species, u8 slot, u8 base)
{
	u8 t1, t2;
	u16 ab;

	if (!HACK_IS_DEBUG_MON(species) || mon == NULL || !Decode(mon, &t1, &t2, &ab))
		return base;
	return slot ? t2 : t1;
}

u16 HackDebugMon_Ability(const struct Pokemon* mon, u16 species)
{
	u8 t1, t2;
	u16 ab;

	if (!HACK_IS_DEBUG_MON(species) || mon == NULL || !Decode(mon, &t1, &t2, &ab))
		return ABILITY_NONE;
	return ab;
}

bool8 HackDebugMon_IsMon(const struct Pokemon* mon)
{
	return HACK_IS_DEBUG_MON(mon->species);
}

//今のタイプ 2 つと特性（切り替えていなければ種族の表のタイプ・標準の特性）
static void Current(const struct Pokemon* mon, u8* t1, u8* t2, u16* ability)
{
	if (Decode(mon, t1, t2, ability))
		return;
	*t1 = gBaseStats[mon->species].type1;
	*t2 = gBaseStats[mon->species].type2;
	*ability = ABILITY_NONE;
}

//性格値を書く。書く前に色違い・Moémon を表 ID・裏 ID に固定し、性格を natureMint に写す（以後その子は性格値を見ない）
static void Write(struct Pokemon* mon, u8 t1, u8 t2, u16 ability)
{
	u8 nature = mon->natureMint != 0 ? mon->natureMint - 1 : GetNature(mon);

	HackMonId_Ensure((struct BoxPokemon*) mon);
	mon->natureMint = nature + 1;
	mon->personality = (DM_MARK << 20) | ((u32) ability << DM_ABILITY_SHIFT) | ((u32) t2 << DM_TYPE_BITS) | t1;
}

s32 HackDebugMon_EditGet(struct Pokemon* mon, u8 field)
{
	u8 t1, t2;
	u16 ab;
	s32 i;

	if (!HackDebugMon_IsMon(mon))
		return 0;
	Current(mon, &t1, &t2, &ab);
	switch (field)
	{
		case 0: return mon->species == SPECIES_DEBUGMON_MIN;
		case 1: i = IndexOfType(t1); return i < 0 ? 0 : i;
		case 2: i = IndexOfType(t2); return i < 0 ? 0 : i;
		default: return ab;
	}
}

void HackDebugMon_EditSet(struct Pokemon* mon, u8 field, s32 value)
{
	u8 t1, t2;
	u16 ab;

	if (!HackDebugMon_IsMon(mon))
		return;
	Current(mon, &t1, &t2, &ab);
	switch (field)
	{
		case 0:
		{
			u32 sp = value ? SPECIES_DEBUGMON_MIN : SPECIES_DEBUGMON_MAX;

			SetMonData(mon, MON_DATA_SPECIES, &sp);
			return;
		}
		case 1: t1 = HackDebugMon_TypeOfIndex(value); break;
		case 2: t2 = HackDebugMon_TypeOfIndex(value); break;
		default:
			if (value < 0 || value >= ABILITIES_COUNT)
				value = 0;
			ab = value;
			break;
	}
	Write(mon, t1, t2, ab);
}

#else   //配る版: 切り替えは無い（種族の表のまま）

u8 HackDebugMon_Type(const struct Pokemon* mon, u16 species, u8 slot, u8 base)
{
	(void) mon; (void) species; (void) slot;
	return base;
}

u16 HackDebugMon_Ability(const struct Pokemon* mon, u16 species)
{
	(void) mon; (void) species;
	return ABILITY_NONE;
}

#endif
