#include "defines.h"
#include "hack_kind.h"

/*
 * 個体の印の窓口（src/hack_kind.h に仕様）。生のバイト位置で読み書きする（hack_mon_id.c と同じ流儀・位置は src/hack_stat_layout.c が static assert で守る）。
 */
#define B_UNKNOWN 0x1E       //BoxPokemon.unknown（u16）
#define B_SUB0_UNKNOWN 0x2B  //BoxPokemon.substruct0.unknown（u8）
#define C_EXT 0x3A           //CompressedPokemon.ext（u16）
#define C_IVHI8 0x4D         //CompressedPokemon.ivHi[8]
#define KIND_SHIFT 9
#define KIND_MASK 0x0600     //unknown／ext の bit9〜10
#define EXT_VAL_SHIFT 11     //ext の bit11〜12 = 値の下位 2 ビット
#define EXT_VAL_MASK 0x1800
#define KIND_FIELD_MASK 0x1E00   //ext の bit9〜12（種類＋値の下位）
#define IVHI8_VAL_MASK 0xFC  //ivHi[8] の bit2〜7 = 値の上位 6 ビット（ivHi の 66 ビットの上）

static u16 Rd16(const u8* p) { return p[0] | (p[1] << 8); }
static void Wr16(u8* p, u16 v) { p[0] = v; p[1] = v >> 8; }

u8 HackKind_Get(const struct BoxPokemon* mon)
{
	return (Rd16((const u8*) mon + B_UNKNOWN) & KIND_MASK) >> KIND_SHIFT;
}

u8 HackKind_Value(const struct BoxPokemon* mon)
{
	return HackKind_Get(mon) ? ((const u8*) mon)[B_SUB0_UNKNOWN] : 0;
}

void HackKind_Set(struct BoxPokemon* mon, u8 kind, u8 value)
{
	u8* b = (u8*) mon;

	kind &= 3;
	Wr16(b + B_UNKNOWN, (Rd16(b + B_UNKNOWN) & ~KIND_MASK) | ((u16) kind << KIND_SHIFT));
	b[B_SUB0_UNKNOWN] = kind ? value : 0;
}

void HackKind_SetValue(struct BoxPokemon* mon, u8 value)
{
	if (HackKind_Get(mon))
		((u8*) mon)[B_SUB0_UNKNOWN] = value;
}

void HackKind_Clear(struct BoxPokemon* mon)
{
	HackKind_Set(mon, HACK_KIND_NONE, 0);
}

bool8 HackKind_IsTradeRefused(const struct BoxPokemon* mon)
{
	return HackKind_Get(mon) == HACK_KIND_DARK;
}

u8 HackKind_CompGet(const struct CompressedPokemon* comp)
{
	return (Rd16((const u8*) comp + C_EXT) & KIND_MASK) >> KIND_SHIFT;
}

u8 HackKind_CompValue(const struct CompressedPokemon* comp)
{
	const u8* c = (const u8*) comp;
	u16 ext = Rd16(c + C_EXT);

	if (!(ext & KIND_MASK))
		return 0;
	return ((ext & EXT_VAL_MASK) >> EXT_VAL_SHIFT) | (c[C_IVHI8] & IVHI8_VAL_MASK);
}

//箱の縮めた形の値だけを書き換える（種類が NONE のときは何もしない・種類は動かさない）。箱の中の心のゲージを減らす用（段 G・src/shadow_relive.c）
void HackKind_CompSetValue(struct CompressedPokemon* comp, u8 value)
{
	u8* c = (u8*) comp;
	u16 ext = Rd16(c + C_EXT);

	if (!(ext & KIND_MASK))
		return;
	ext = (ext & ~EXT_VAL_MASK) | ((u16) (value & 3) << EXT_VAL_SHIFT);
	c[C_IVHI8] = (c[C_IVHI8] & ~IVHI8_VAL_MASK) | (value & IVHI8_VAL_MASK);
	Wr16(c + C_EXT, ext);
}

void HackKind_PackToComp(const struct BoxPokemon* box, struct CompressedPokemon* comp)
{
	const u8* b = (const u8*) box;
	u8* c = (u8*) comp;
	u16 ext = Rd16(c + C_EXT) & ~KIND_FIELD_MASK;
	u8 kind = HackKind_Get(box);
	u8 value = b[B_SUB0_UNKNOWN];

	c[C_IVHI8] &= ~IVHI8_VAL_MASK;
	if (kind)
	{
		ext |= ((u16) kind << KIND_SHIFT) | ((u16) (value & 3) << EXT_VAL_SHIFT);
		c[C_IVHI8] |= value & IVHI8_VAL_MASK;
	}
	Wr16(c + C_EXT, ext);
}

void HackKind_UnpackFromComp(const struct CompressedPokemon* comp, struct BoxPokemon* box)
{
	u8* b = (u8*) box;
	u16 un = Rd16(b + B_UNKNOWN) & ~KIND_MASK;
	u8 kind = HackKind_CompGet(comp);

	b[B_SUB0_UNKNOWN] = kind ? HackKind_CompValue(comp) : 0;
	Wr16(b + B_UNKNOWN, un | ((u16) kind << KIND_SHIFT));
}
