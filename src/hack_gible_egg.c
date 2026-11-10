#include "defines.h"
#include "hack_base_stats_wide.h" //[hack] patches/0678 種族値の u16 の窓口
#include "../include/pokemon.h"
#include "../include/overworld.h"
#include "../include/event_data.h"
#include "../include/text.h"
#include "../include/new/build_pokemon.h"
#include "../include/new/daycare.h"
#include "../include/new/catching.h"
#include "../include/constants/abilities.h"
#include "../include/constants/moves.h"
#include "../include/constants/pokemon.h"
#include "../include/new/learn_move.h"
#include "hack_mon_id.h"
#include "hack_met_level.h" //[hack] 2026-10-27 出会ったレベル（タマゴ産の判定）
#include "hack_box_shelf.h" //[hack] FIX-gible-tenp-old-saves 箱を全部見る
#include "hack_daycare_region.h" //[hack] FIX-gible-tenp-old-saves 預け屋の控え
#include "../include/pokemon_storage_system.h"
#include "../include/new/pokemon_storage_system.h"
#include "hack_stat_access.h" //[hack] patches/0763 努力値・個体値の窓口

/*
 * クチバのポケモンだいすきクラブの フカマルのタマゴ（2026-10-03・assembly/kanto/kanto_gible_egg.s から callasm）。
 * ユーザー「特別な設定　個体値MAX 色違い ガブリアスをみせるとメガストーンとZ両方もらえる」。
 *
 *   HackGibleEgg_Give         … フカマルのタマゴ（個体値 6 つとも 31・色違い）を手持ちへ → LASTRESULT = GiveMonToPlayer の結果
 *   HackGibleEgg_HasGarchomp  … 手持ちにガブリアス（タマゴでない）がいるか → LASTRESULT
 *
 *   （タマゴは ドラゴンのちかい（MOVE_DRAGONSOATH・優先度 -3・すべて 2 段階）も覚えている）
 *   HackGibleEgg_SpecialAbility … この子（印あり）とその進化・メガシンカの特性 → りゅうのよろい（ABILITY_DRAGONARMOR 319）
 *   ★ 2026-10-23 FEAT-gible-reitonoou（Redmine #58 とくべつなガブリアス 段階 1・patches/0790）:
 *     Lv200 のこの子（手持ち）にクチバのお姉さんが話しかけると「れーとのおうの印」を付ける（HackGibleEgg_ReitonoouCheck / _Apply・台本 kanto_gible_egg.s）。
 *     印 = うんめいの印（MON_DATA_OBEDIENCE）＋ hiddenAbility ビット（PokemonSubstruct3 の u16 の bit12・ボックスの縮め形の先頭 8 バイトに入り、
 *     hack_mon_id.c が使う bit13〜14 とは別なので預けても戻っても残る）。★ 最初は blank（bit13〜14）に付けたが、ボックスで消えた
 *     （hack_mon_id.c の ID の空きビット・2026-10-23 実機で確認）ので hiddenAbility に変えた。とくせいカプセル／パッチが hiddenAbility を立てる穴は、
 *     印付きのフカマル系にはカプセルが効かないようにして塞いだ（src/party_menu.c GetAbilityCapsuleNewAbility）。
 *     印の付いたガブリアス（ふつう・メガ・メガZ）の特性は れーとのおう（ABILITY_REITONOOU 329・りゅうのよろいと同じ 4 効果）。
 *     付いた後はレベルに関係なく そのまま。フカマル・ガバイトは りゅうのよろいのまま。
 *
 * ★ 2026-10-03 ユーザー「特性をちからもちとマルチスケイルとさめはだを複合した特性を作成してその特性にしてほしい
 *   メガ進化してもそのままで」。1 匹だけに付けるので、タマゴを作るとき MON_DATA_OBEDIENCE（うんめいてきな であい の印・80）を立て、
 *   GetMonAbility（src/build_pokemon.c・patches/0150）がフカマル・ガバイト・ガブリアス・メガガブリアス（・Z）でこの印を見る。
 *   メガシンカは DoFormChange が GetMonAbility で特性を読み直すので、そのまま りゅうのよろい。
 *   同じ印はお母さんのピカチュウの 2 つ目の特性（src/second_ability.c）でも使うが、どちらも種族も見るのでまざらない。
 *   効き目は ちからもち（こうげき 2 倍）・マルチスケイル（HP まんたんで受けるダメージ半分・かたやぶりの技なら効かない）・
 *   さめはだ（ふれた相手に最大 HP の 1/8）を、それぞれの特性の所に足した（damage_calc.c・battle_util.c・ability_battle_effects.c・AI）。
 *
 * 色違いは CFRU の ForceMonShiny（性格・性別はそのまま性格値を選び直す・最初のポケモンの色違いと同じ・src/hack_gift_shiny.c）。
 * 親はタマゴを作ったときの主人公（CreateEgg）なので、孵っても色違いのまま。
 */

void HackGibleEgg_Give(void)
{
	struct Pokemon mon;
	u8 iv = HACK_IV_MAX; //[hack] patches/0677
	u32 i;

	CreateEgg(&mon, SPECIES_GIBLE);
	for (i = 0; i < NUM_STATS; ++i)
		SetMonData(&mon, MON_DATA_HP_IV + i, &iv);
	ForceMonShiny(&mon);
	/* ★ 2026-10-20 FEAT-gible-moemon-tenp ユーザー「クチバでもらえるフカマル、1/16で添付のガブリアスになるmoemonにして」
	   「タマゴのときにmoemonかどうか決まる」「色違いだからあっている」: Tenp（Kogasa）の絵の Moémon ・15/16 は一生ふつうの絵。
	   Moémon の当たり・はずれと絵は自身の裏 ID（src/hack_mon_id.c）で決まる＝個体の印。ここで決めた裏 ID は孵化・進化・ボックスを通っても変わらず、
	   性格値の 1/16（Moémon の ON/OFF の旗が立っているとき）には左右されない（ID が付いている子は性格値を見ない）。タマゴは必ず色違い＝表 ID も色違いの当たり */
	HackMonId_MakeClubGible((struct BoxPokemon*) &mon, HackMonId_RollTenp());
	/* ★ 2026-10-03 ユーザー「ドラゴンのちかい 優先度-3 すべてののうりょくをぐーんとあげる ってわざ作って覚えさせて」:
	   MOVE_DRAGONSOATH（0x3E5）を覚えている。空きが無ければ 4 つ目と入れ替える */
	if (GiveMoveToMon(&mon, MOVE_DRAGONSOATH) == MON_HAS_MAX_MOVES)
		SetMonMoveSlot(&mon, MOVE_DRAGONSOATH, MAX_MON_MOVES - 1);
	{
		u8 mark = TRUE;     /* りゅうのよろいの印（HackGibleEgg_SpecialAbility） */
		SetMonData(&mon, MON_DATA_OBEDIENCE, &mark);
	}
	gSpecialVar_LastResult = GiveMonToPlayer(&mon);
}

void HackGibleEgg_HasGarchomp(void)
{
	u32 i;

	gSpecialVar_LastResult = FALSE;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2, NULL) == SPECIES_GARCHOMP)
		{
			gSpecialVar_LastResult = TRUE;
			return;
		}
	}
}

/* ★ 2026-10-03 ユーザー「010の孵化したフカマルの特性が通常になっている」: CFRU の CreateHatchedMon（src/daycare.c）は
   うんめいの印を引き継いでいなかった（patches/0155 で直した）。v0.10 で孵した子は印が消えているので、
   「色違い・個体値 6 つとも 31（すごいとっくんでない素の値）・親が主人公」のフカマルの仲間もこの子とみなす
   （クチバのタマゴ以外でこの 3 つがそろうことは、まず無い） */
static bool8 LooksLikeClubGible(const struct Pokemon* mon)
{
	struct Pokemon* m = (struct Pokemon*) mon;
	u32 otId, i;

	if (!IsMonShiny(m))
		return FALSE;
	for (i = 0; i < NUM_STATS; ++i)
	{
		if (HackMon_GetIv(m, i) != HACK_IV_MAX) //[hack] patches/0677
			return FALSE;
	}
	otId = gSaveBlock2->playerTrainerId[0] | (gSaveBlock2->playerTrainerId[1] << 8)
	     | (gSaveBlock2->playerTrainerId[2] << 16) | ((u32) gSaveBlock2->playerTrainerId[3] << 24);
	return GetMonData(m, MON_DATA_OT_ID, NULL) == otId;
}

/* ★ 2026-10-05 ユーザー「バトルタワーかなんかでとくべつなガブリアスでてくるみたい」:
   CFRU はフロンティアのポケモンを作るとき mon->obedient = TRUE（src/build_pokemon.c）にするので、相手のガブリアスにも
   印（MON_DATA_OBEDIENCE）が付いていた。印は「親が主人公」のときだけ見る（相手は親の ID がでたらめ）。src/second_ability.c（ピカチュウの印）も使う。
   ★ 同じ日 ユーザー「バトルフロンティアなどの施設でたたかえるようにして」: 施設の中でも自分の子は特別のまま。
     施設が主人公に渡すポケモン（ファクトリーのレンタルなど・親は主人公・印あり）だけ外す＝施設の中で、今いる地名で出会っていて、
     タマゴからかえった子（出会ったレベル 0）でないもの */
bool8 HackMark_IsPlayersMarkedMon(const struct Pokemon* mon)
{
	struct Pokemon* m = (struct Pokemon*) mon;
	u32 otId;

	if (!GetMonData(m, MON_DATA_OBEDIENCE, NULL))
		return FALSE;
	if (FlagGet(FLAG_BATTLE_FACILITY)
	 && GetMonData(m, MON_DATA_MET_LEVEL, NULL) != 0
	 && GetMonData(m, MON_DATA_MET_LOCATION, NULL) == GetCurrentRegionMapSectionId())
		return FALSE;   //施設が渡したレンタル
	otId = gSaveBlock2->playerTrainerId[0] | (gSaveBlock2->playerTrainerId[1] << 8)
	     | (gSaveBlock2->playerTrainerId[2] << 16) | ((u32) gSaveBlock2->playerTrainerId[3] << 24);
	return GetMonData(m, MON_DATA_OT_ID, NULL) == otId;
}

/* ★ 2026-10-05 ユーザー「フカマルの卵だけどおとこからもらったになっている」:
   ステータス画面のタマゴのトレーナーメモ（src/bw_summary_screen.c PrintEggInfo）で、クラブのタマゴか見分ける */
bool8 HackGibleEgg_IsClubEgg(const struct Pokemon* mon)
{
	struct Pokemon* m = (struct Pokemon*) mon;

	if (!GetMonData(m, MON_DATA_IS_EGG, NULL) || GetMonData(m, MON_DATA_SPECIES, NULL) != SPECIES_GIBLE)
		return FALSE;
	return HackMark_IsPlayersMarkedMon(mon) || LooksLikeClubGible(mon);
}

/* ★ 2026-10-23 れーとのおうの印を読む＝うんめいの印（MON_DATA_OBEDIENCE）と hiddenAbility ビットの両方が立っている。
   種族と親（OT）は見ない: 種族は呼ぶ側（HackGibleEgg_SpecialAbility がガブリアス・メガ・メガZ だけ見る）。段階 3 など他のファイルは
   `extern bool8 HackGibleEgg_HasReitoMark(const struct Pokemon* mon);` で使い、種族は自分で見ること（通信対戦の決定性のため OT を見ない） */
/* ★ 2026-10-27 BUG-frontier-reitonoou（再発）: 施設（バトルフロンティア）のポケモンは CFRU が必ず obedient=TRUE にし、隠れ特性の型なら
   hiddenAbility も立つ（build_pokemon.c）。「hiddenAbility＋うんめいの印」だけで決めると、施設の相手・レンタルのガブリアスが れーとのおうになった。
   前回（2026-10-05）はりゅうのよろい側を「親が主人公」で直したが、この判定は通信対戦のため親を見ない作りで、その防御をすり抜けていた。
   親の代わりに「出会ったレベルが 0＝タマゴからかえった子」を要る（クチバのタマゴ産は必ず 0・施設の子は出会いレベル＝そのレベルで 1 以上）。
   個体そのものの値だけなので通信対戦でも両機で同じ答え。 */
static bool8 IsHatchedFromEgg(const struct Pokemon* mon)
{
	return HackMetLevel_Get((const struct BoxPokemon*) mon) == 0;
}

bool8 HackGibleEgg_HasReitoMark(const struct Pokemon* mon)
{
	struct Pokemon* m = (struct Pokemon*) mon;

	return m->hiddenAbility && GetMonData(m, MON_DATA_OBEDIENCE, NULL) != 0 && IsHatchedFromEgg(mon);
}

/* ★ 2026-10-25 FEAT-garchompite-x（Redmine #111・patches/0813）ガブリアスナイトX を使える「とくべつなガブリアス」の判定。
   れーとのおうの印の子、または クラブのタマゴ産の子（うんめいの印＋色違い＋個体値 6 つとも 31）。
   ★ 親の ID（OT）を見ない: 戦闘（メガシンカ・メガ↔メガZ の切り替え）で使う判定なので、通信対戦で相手のガブリアスを自機でも同じ答えにしないと
     2 台の計算がずれる。HackMark_IsPlayersMarkedMon と LooksLikeClubGible は「自分の主人公の ID」と比べるので、相手の子では自機が FALSE・相手の機械が TRUE になる。
     ここで見るのは個体そのものの値（印・色違い＝その子自身の ID と性格値・個体値）だけ＝両機で同じ。種族は呼ぶ側が見る。
   v0.10 で孵って印が消えた子（色違い＋31 だけ）は、お姉さんが X を渡すとき（自機だけの場面）に うんめいの印を付けて救う（HackGibleEgg_MarkForX）。 */
bool8 HackGibleEgg_IsSpecialForX(const struct Pokemon* mon)
{
	struct Pokemon* m = (struct Pokemon*) mon;
	u32 i;

	if (mon == NULL)
		return FALSE;
	if (HackGibleEgg_HasReitoMark(mon))
		return TRUE;
	if (!GetMonData(m, MON_DATA_OBEDIENCE, NULL) || !IsMonShiny(m) || !IsHatchedFromEgg(mon)) //[hack] 2026-10-27 施設の子（出会いレベル≥1）を除く
		return FALSE;
	for (i = 0; i < NUM_STATS; ++i)
	{
		if (HackMon_GetIv(m, i) != HACK_IV_MAX)
			return FALSE;
	}
	return TRUE;
}

/* メガシンカの判定（src/mega.c CanMegaEvolve）から: X を持った子が ガブリアス（とくべつ）か */
bool8 HackGibleEgg_CanUseX(const struct Pokemon* mon)
{
	return mon != NULL && mon->species == SPECIES_GARCHOMP && HackGibleEgg_IsSpecialForX(mon);
}

/* callasm（kanto_gible_egg.s）: 手持ちに X を渡せる子がいるか → LASTRESULT 1/0。
   ガブリアスそのもの（タマゴでない）で、HackGibleEgg_IsSpecialForX か、v0.10 の子（LooksLikeClubGible＝親が主人公・色違い・個体値 31）。自機の場面なので OT を見てよい */
void HackGibleEgg_HasSpecialForX(void)
{
	u32 i;

	gSpecialVar_LastResult = FALSE;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];

		if (GetMonData(mon, MON_DATA_SPECIES2, NULL) != SPECIES_GARCHOMP)
			continue;
		if (HackGibleEgg_IsSpecialForX(mon) || LooksLikeClubGible(mon))
		{
			gSpecialVar_LastResult = TRUE;
			return;
		}
	}
}

/* callasm: X を渡す前に、v0.10 で印が消えた子（LooksLikeClubGible）へ うんめいの印を付ける（付けないと X が効かない） */
void HackGibleEgg_MarkForX(void)
{
	u32 i;
	u8 mark = TRUE;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];

		if (GetMonData(mon, MON_DATA_SPECIES2, NULL) != SPECIES_GARCHOMP)
			continue;
		if (!GetMonData(mon, MON_DATA_OBEDIENCE, NULL) && LooksLikeClubGible(mon))
			SetMonData(mon, MON_DATA_OBEDIENCE, &mark);
	}
}

/* callasm（kanto_gible_egg.s の KantoGibleEgg_AlreadyGiven / AllDone）: 手持ちの「とくべつなガブリアス」（印あり・親が主人公・ガブリアスそのもの・まだ れーとのおうの印なし）を調べる。
   LASTRESULT 1 = Lv200 以上の子がいる（Var8004 = 手持ちの番号・gStringVar1 = 名前）/ 2 = いるが Lv200 に届かない / 0 = いない（印済みだけ・普通のガブリアスだけ・手持ちにいない）。
   見るのは手持ちだけ（ボックスの子は対象外・ユーザー q165） */
void HackGibleEgg_ReitonoouCheck(void)
{
	u32 i;
	u8 result = 0;

	gSpecialVar_LastResult = 0;
	for (i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];

		if (GetMonData(mon, MON_DATA_SPECIES2, NULL) != SPECIES_GARCHOMP)
			continue;
		if (!(HackMark_IsPlayersMarkedMon(mon) || LooksLikeClubGible(mon)) || HackGibleEgg_HasReitoMark(mon))
			continue;
		if (!IsHatchedFromEgg(mon)) //[hack] 2026-10-27 印を付けても効かない子（タマゴ産でない）は対象にしない
			continue;
		if (GetMonData(mon, MON_DATA_LEVEL, NULL) >= MAX_LEVEL)
		{
			VarSet(0x8004, i);
			GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
			gSpecialVar_LastResult = 1;
			return;
		}
		result = 2;
	}
	gSpecialVar_LastResult = result;
}

/* callasm: Var8004 の手持ちに れーとのおうの印（hiddenAbility ビット＋うんめいの印）を付ける（HackGibleEgg_ReitonoouCheck が 1 を返したあと）。gStringVar1 = 名前 */
void HackGibleEgg_ReitonoouApply(void)
{
	struct Pokemon* mon;

	if (Var8004 >= PARTY_SIZE)
		return;
	mon = &gPlayerParty[Var8004];
	mon->hiddenAbility = TRUE;
	{
		u8 mark = TRUE;     /* v0.10 で孵した印なしの子（LooksLikeClubGible）にも、うんめいの印を付ける（れーとのおうは印＋hiddenAbility の組で決まる） */
		SetMonData(mon, MON_DATA_OBEDIENCE, &mark);
	}
	GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
}

u16 HackGibleEgg_SpecialAbility(const struct Pokemon* mon, u16 species)
{
	if (mon == NULL)
		return ABILITY_NONE;
	switch (species)
	{
		case SPECIES_GIBLE:
		case SPECIES_GABITE:
			if (HackMark_IsPlayersMarkedMon(mon) || LooksLikeClubGible(mon))
				return ABILITY_DRAGONARMOR;
			break;
		case SPECIES_GARCHOMP:
		case SPECIES_GARCHOMP_MEGA:
		case SPECIES_GARCHOMP_MEGA_Z:
			/* ★ 2026-10-23 れーとのおう: うんめいの印＋hiddenAbility ビット＋この 3 種族だけで決める。親が主人公か（OT）は見ない
			   （通信対戦で相手のガブリアスも自機で同じ特性にしないと 2 台の計算がずれるため・儀式のときに OT を確かめている）。りゅうのよろいの判定（OT を見る）は変えない */
			if (HackGibleEgg_HasReitoMark(mon))
				return ABILITY_REITONOOU;
			if (HackMark_IsPlayersMarkedMon(mon) || LooksLikeClubGible(mon))
				return ABILITY_DRAGONARMOR;
			break;
	}
	return ABILITY_NONE;
}

/* ★ 2026-10-03 ユーザー「とくべつなガブリアスのメガ進化のメガガブリアスの種族値をベースガブリアスで上昇値だけ反映したポケモンにしてほしい」:
   印のある子がメガガブリアス・メガガブリアスZ のとき、能力ごとに ガブリアスの種族値 ＋（メガの方が高ければ その差）＝ 2 つの大きい方。
   下がる分（メガガブリアスの すばやさ 102→92・メガガブリアスZ の ぼうぎょ 95→85）は反映しない。
     メガガブリアス:  108 / 170 / 115 / 120 / 95 / 102（HP こうげき ぼうぎょ とくこう とくぼう すばやさ）
     メガガブリアスZ: 108 / 130 /  95 / 141 / 85 / 151
   CFRU の CalculateMonStatsNew（src/build_pokemon.c・patches/0152）が種族値を読む所から呼ぶ。HP はどれも 108 で同じ。
   stat は CFRU の並び（STAT_ATK など・gBaseStats の baseHP からのバイトの並びと同じ） */
/* ★ 2026-10-23 れーとのおうの種族値（patches/0793・ユーザー「れーとのおうになると種族値計算に上昇補正をかけて そこからメガ進化の補正も」・補正の数は Commander 決定）:
   通常 = ガブリアスの種族値 ×1.2（四捨五入）・メガ／メガZ = それ ＋（メガ／メガZ − ガブリアス・マイナスは 0）。HP は ×1.2 だけで姿が変わっても同じ。
   108/130/95/80/85/102 → 130/156/114/96/102/122（720）/ メガ 130/196/134/136/112/122（830）/ メガZ 130/156/114/157/102/171（830）
   種族だけで決まる純関数（hack_gible_switch.c の点数も同じ値を使う）。stat は CFRU の並び（STAT_HP=0 …） */
u16 HackGibleEgg_ReitoBase(u16 species, u8 stat) //[hack] patches/0678 u8 → u16
{
	u16 g = HackBaseStat(SPECIES_GARCHOMP, stat); //[hack] patches/0678
	u32 v = (g * 12 + 5) / 10;

	if (stat != STAT_HP && (species == SPECIES_GARCHOMP_MEGA || species == SPECIES_GARCHOMP_MEGA_Z))
	{
		u16 f = HackBaseStat(species, stat); //[hack] patches/0678
		if (f > g)
			v += f - g;
	}
	return v;
}

u16 HackGibleEgg_BaseStat(const struct Pokemon* mon, u16 species, u8 stat) //[hack] patches/0678 u8 → u16
{
	u16 base = HackBaseStat(species, stat);

	if ((species == SPECIES_GARCHOMP || species == SPECIES_GARCHOMP_MEGA || species == SPECIES_GARCHOMP_MEGA_Z)
	 && mon != NULL && HackGibleEgg_HasReitoMark(mon))
		return HackGibleEgg_ReitoBase(species, stat);   //れーとのおう（ガブリアス・メガ・メガZ のどの姿でも）

	if ((species == SPECIES_GARCHOMP_MEGA || species == SPECIES_GARCHOMP_MEGA_Z)
	 && HackGibleEgg_SpecialAbility(mon, species) != ABILITY_NONE)
	{
		u16 normal = HackBaseStat(SPECIES_GARCHOMP, stat);
		if (normal > base)
			base = normal;
	}
	return base;
}



/* ★ 2026-10-28 FIX-gible-tenp-old-saves（patches/0938）: 0.61.x までに配ったクチバのフカマル系の「はずれ」（Tenp の当たりでない裏 ID）を、
   版が上がった つづきから で Tenp の当たりに付け直す（src/hack_save_version.c の HackSaveVersion_OnContinue から版が上がるたびに呼ぶ・表の行にしない＝VERSION を機能の枝で上げない）。
   見分け方（クチバのタマゴ産）: 種族がフカマル・ガバイト・ガブリアス・メガ・メガZ（タマゴも）で、親が主人公で、出会ったレベルが 0（タマゴからかえった子・タマゴの間も 0）で、
   うんめいの印がある（またはv0.10 の印の消えた子＝色違い＋個体値 6 つとも 31）。他人の子（交換）・野生のフカマル・施設の子（出会いレベル 1 以上）は触らない。
   FLAG_BATTLE_FACILITY はタイトルで残りうるので HackMark_IsPlayersMarkedMon は使わず自分で見る。
   直し方: HackMonId_ToTenp（今の色違いのまま・もう当たりなら何もしない＝何度呼んでも同じ）。置き場: 手持ち（RAM とセーブの写し）・施設の退避・預け屋（本体と 3 地方の控え・カントーの 5ばん）・ボックス全部（縮めた形は ID だけ書き換える） */
static bool8 IsClubGibleSpecies(u16 sp)
{
	return sp == SPECIES_GIBLE || sp == SPECIES_GABITE || sp == SPECIES_GARCHOMP || sp == SPECIES_GARCHOMP_MEGA || sp == SPECIES_GARCHOMP_MEGA_Z;
}

static bool8 IsClubGibleBox(struct BoxPokemon* b)
{
	u32 otId = gSaveBlock2->playerTrainerId[0] | (gSaveBlock2->playerTrainerId[1] << 8)
	         | (gSaveBlock2->playerTrainerId[2] << 16) | ((u32) gSaveBlock2->playerTrainerId[3] << 24);
	u32 i;

	if (!IsClubGibleSpecies(b->substruct0.species) || b->otId != otId || HackMetLevel_Get(b) != 0)
		return FALSE;
	if (b->substruct3.obedient)
		return TRUE;
	for (i = 0; i < NUM_STATS; ++i)   //v0.10 の印の消えた子: 色違い＋個体値 6 つとも 31
	{
		if (HackBoxMon_GetIv(b, i) != HACK_IV_MAX)
			return FALSE;
	}
	return HackMonId_IsShinyBox(b);
}

static void FixGibleBox(struct BoxPokemon* b)
{
	u32 id, now;

	if (b->substruct0.species == SPECIES_NONE || !IsClubGibleBox(b))
		return;
	id = HackMonId_Get(b);
	now = HackMonId_ToTenp(id, HackMonId_IsShinyBox(b), b->personality);
	if (now != id)
		HackMonId_Set(b, now);
}

static void FixGibleDaycare(struct DayCare* d)
{
	u32 i;

	if (d == NULL)
		return;
	for (i = 0; i < DAYCARE_MON_COUNT; ++i)
		FixGibleBox(&d->mons[i].mon);
}

void CreateBoxMonFromCompressedMon(struct BoxPokemon* boxMon, struct CompressedPokemon* compMon);   //src/pokemon_storage_system.c

void HackVersionFix_GibleTenp(void)
{
	u32 i, box, pos;

	for (i = 0; i < PARTY_SIZE; ++i)
	{
		FixGibleBox((struct BoxPokemon*) &gPlayerParty[i]);
		FixGibleBox((struct BoxPokemon*) &gSaveBlock1->playerParty[i]);
		FixGibleBox(&gSaveBlock1->safeBackupParty[i]);
	}
	FixGibleDaycare(&gSaveBlock1->daycare);
	for (i = 0; i < 3; ++i)
		FixGibleDaycare(HackDaycare_GetStashed(i));
	FixGibleBox(&gSaveBlock1->route5DayCareMon.mon);
	for (box = 0; box < TOTAL_BOXES_COUNT; ++box)
	{
		struct CompressedPokemon* c = BoxShelf_BoxMons(box, FALSE);   //読むだけ（汚れを付けない）

		if (c == NULL)
			continue;
		for (pos = 0; pos < IN_BOX_COUNT; ++pos)
		{
			struct BoxPokemon b;
			u32 id, now;

			if (!IsClubGibleSpecies(c[pos].substruct0.species))
				continue;
			CreateBoxMonFromCompressedMon(&b, &c[pos]);
			if (!IsClubGibleBox(&b))
				continue;
			id = HackMonId_Get(&b);
			now = HackMonId_ToTenp(id, HackMonId_IsShinyBox(&b), b.personality);
			if (now == id)
				continue;
			if (!BoxShelf_CanTouchBox(box))
				break;   //この箱は書けない（汚せる箱の上限・古い形）: 次の版のつづきからでやり直す
			c = BoxShelf_BoxMons(box, TRUE);   //書く（汚れの印）
			HackMonId_PackComp(&c[pos], now);  //縮めた形は ID の欄だけ書き換える（ほかの欄は触らない）
		}
	}
}
