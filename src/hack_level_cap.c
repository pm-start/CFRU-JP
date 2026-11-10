#include "defines.h"
#include "map_region.h"
#include "Tables/hack_badge_cap.h"

/*
 * 「バッジの数 → レベルの上限」の共通の関数（#117・2026-10-16 ユーザー「ジムのバッジ数でキャップ作っといて」「これ使い回すよー」「上限はそちらの塩梅に任せる」）。
 *  バッジの数は全地方の合計 0〜32（GetAllBadgeCount・src/badge_count.c）。数字は下の sCapPoints の表 1 か所だけ。
 *  表は「区切り」（4 個ごと）で、間は直線でつなぐ（整数・四捨五入）。
 *    バッジ   0   4   8  12  16  20  24  28  32
 *    上限    20  50  80 100 120 135 150 165 180
 *  再戦はカントーでも殿堂入り（バッジ 8 個）から開くので、8 個の上限を今の再戦 Lv55 より上（80）にして、段階が意味を持つようにした。
 *  使う所: ジムリーダーの再戦の相手の上限（src/hack_gym_tiers.c）。あとで 自分のポケモン・野生・ほかのトレーナーにも使う予定（呼ぶだけ）。
 *  ミラクル交換の上限（src/hack_wonder_trade.c の WonderLevelCap）は別の数字（15〜110・31 個まで・32 個は上限なし）で HackLevelCap_Lerp を呼ぶ（この表とは別・値はそのまま）。
 *  表は tools/kanto/verify_gym_tiers.py が同じ式で作って確かめる
 *
 *  ■ もう 1 つ: 「今いる地方のバッジによるレベル制限」HackLevelCap_FromPlace（#124・0.50.6・いつも効く・docs/research/2026-10-17_badge-level-cap.md）。
 *    上の全地方の合計（HackLevelCap_FromBadges）とは別の表 sBadgeCap（src/Tables/hack_badge_cap.h＝tools/kanto/badge_level_cap.py が実物の手持ちの表から作る）。
 *    その地方のバッジ k 個 → 次のジムリーダーの最高レベル + 10（8 個は四天王・チャンピオン）。今いる地方がカントー（ナナシマ含む）・ホウエン・シンオウならその地方の上限、
 *    ジョウト・キタカミ・バトルフロンティアなど 3 地方の外では 3 地方の上限のいちばん高いもの。今いる地方が殿堂入り済みなら上限なし（0）。
 *    縛り B のレベルキャップとの低いほうを取るのは src/hack_challenge_b.c の HackChallengeB_LevelCap（経験値の道を塞ぐ場所は そこの 1 か所）
 */
#define HACK_LEVEL_CAP_POINTS 9

//{バッジの数, 上限}（バッジの数は増える順・最後が 32）
static const u8 sCapPoints[HACK_LEVEL_CAP_POINTS][2] =
{
	{0, 20},
	{4, 50},
	{8, 80},
	{12, 100},
	{16, 120},
	{20, 135},
	{24, 150},
	{28, 165},
	{32, 180},
};

//base から top まで steps 等分（四捨五入）。badges が steps を超えたら steps（ミラクル交換が使う）
u8 HackLevelCap_Lerp(u8 badges, u8 base, u8 top, u8 steps)
{
	u32 b = (badges > steps) ? steps : badges;

	return base + (2 * (u32) (top - base) * b + steps) / (2 * (u32) steps);
}

u8 HackLevelCap_FromBadges(u8 badges)
{
	u32 i;

	if (badges >= sCapPoints[HACK_LEVEL_CAP_POINTS - 1][0])
		return sCapPoints[HACK_LEVEL_CAP_POINTS - 1][1];
	for (i = 1; i < HACK_LEVEL_CAP_POINTS; ++i)
	{
		if (badges <= sCapPoints[i][0])
			return HackLevelCap_Lerp(badges - sCapPoints[i - 1][0], sCapPoints[i - 1][1], sCapPoints[i][1], sCapPoints[i][0] - sCapPoints[i - 1][0]);
	}
	return sCapPoints[HACK_LEVEL_CAP_POINTS - 1][1];
}
/* ---- 今いる地方のバッジによるレベル制限（#124）---- */

u8 GetRegionBadgeHighest(u8 region);         //src/badge_count.c（[hack] patches/0620 持っているバッジのいちばん大きいジム番号 0〜8）
bool8 HackBadgeReset_AfterHof(u8 region);    //src/hack_badge_reset.c（#125: 殿堂入り済みの地方でバッジをリセットしたあと＝殿堂入りを数えない）
bool8 HackGymTier_RegionOpen(u8 region);     //src/hack_gym_tiers.c（カントー 0・ホウエン 1・シンオウ 3 では「殿堂入りした」＝残る印を見る・CLAUDE.md 6.91）

#define BADGE_CAP_NONE 0xFF
static const u8 sBadgeCapRegion[BADGE_CAP_REGION_COUNT] = BADGE_CAP_REGION_IDS;
static const u8 sBadgeCap[BADGE_CAP_REGION_COUNT][9] = {BADGE_CAP_TABLE};   //行 = 地方・列 = その地方のバッジの数 0〜8

//表の行の上限。0 ＝ その地方では上限なし（殿堂入りした）。count < 0 ＝ その地方の今のバッジの数／0〜8 ＝ その数だけ持っているとしたときの上限（バッジ画面の一覧・[hack] patches/0599）
static u8 BadgeCapOfRow(u32 row, s32 count)
{
	u8 region = sBadgeCapRegion[row];
	u8 n;

	if (HackGymTier_RegionOpen(region) && !HackBadgeReset_AfterHof(region))   //#125: バッジリセットのあとは殿堂入りを数えない
		return 0;
	n = (count < 0) ? GetRegionBadgeHighest(region) : (u8) count;   //[hack] patches/0620 行は「バッジの数」ではなく「持っているバッジのいちばん大きいジム番号」（7 番だけ持っていれば 7 の行）
	return sBadgeCap[row][n > 8 ? 8 : n];
}

//今いる所が表のある地方（カントー・ホウエン・シンオウ）ならその行、ジョウト・キタカミ・フロンティアなど外なら BADGE_CAP_NONE
static u32 PlaceRow(void)
{
	u32 row = BADGE_CAP_NONE, i;

	if (!MapRegion_CurrentIsFrontier())   //フロンティアは群がホウエンでも地方としては別扱い
	{
		u8 region = GetCurrentMapRegion();

		for (i = 0; i < BADGE_CAP_REGION_COUNT; ++i)
		{
			if (sBadgeCapRegion[i] == region)
				row = i;
		}
	}
	return row;
}

//[hack] patches/0599 バッジ画面の一覧用: 今いる所が表のある地方ならその地方の番号（map_region.h の MAP_REGION_*）、外なら 0xFF
u8 HackLevelCap_PlaceRegion(void)
{
	u32 row = PlaceRow();

	return (row == BADGE_CAP_NONE) ? 0xFF : sBadgeCapRegion[row];
}

//[hack] patches/0599 バッジ画面の一覧用: その地方でバッジを badges 個持っているときの上限（0 ＝ なし・バッジリセットと殿堂入りは FromPlace と同じ判定）。表の無い地方は 0xFF
u8 HackLevelCap_FromRegion(u8 region, u8 badges)
{
	u32 i;

	for (i = 0; i < BADGE_CAP_REGION_COUNT; ++i)
	{
		if (sBadgeCapRegion[i] == region)
			return BadgeCapOfRow(i, badges);
	}
	return 0xFF;
}

//今いる所のバッジによる上限。0 ＝ 上限なし
u8 HackLevelCap_FromPlace(void)
{
	u32 row = PlaceRow(), i;
	u8 best = 0, cap;

	if (row != BADGE_CAP_NONE)
		return BadgeCapOfRow(row, -1);
	for (i = 0; i < BADGE_CAP_REGION_COUNT; ++i)   //ジョウト・キタカミ・フロンティアなど: 3 地方の上限のいちばん高いもの（どこか 1 つでも殿堂入りなら上限なし）
	{
		cap = BadgeCapOfRow(i, -1);
		if (cap == 0)
			return 0;
		if (cap > best)
			best = cap;
	}
	return best;
}