#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokemon.h"
#include "../include/constants/species.h"
#include "hack_mon_id.h"
#include "hack_debug_menu.h"

/*
 * 最初のポケモン レントラー♀・色違い・Moémon（2026-10-18・お遊び。ユーザー「初期のもらえるポケモンをレントラー♀にボールを見るとできる機能を
 * デバッグメニューに」「初手名前選択でliruにしたら…」「♀でmoemon確定で」「色違いも」）。
 *
 * 入口は build_pokemon.c の ScriptGiveMon の 2 か所だけ（どちらも HackGiftShiny_IsStarterLab ＝研究所で、旗 FLAG_SYS_POKEMON_GET が立っていて手持ちが空のとき。
 * ボールのどれを調べても同じ。ライバルの手持ちは VAR_STARTER_MON＝どのボールかで決まり、もらう種族を見ないので そのまま）。
 *   - 効く条件（Enabled）:
 *       1. 主人公の名前が、半角の小文字 "liru" ちょうど（バイト E0 DD E6 E9 のあと終端 FF）。ほかの名前（lir・liruu・LIRU・リル・空）は効かない。
 *          名前は新しい冒険の最初の名前入力でしか決まらず、旗は書かない（ふだんの版にこの旗は無い＝セーブに何も増えない）
 *       2. デバッグ版（HACK_DEBUG_MENU）だけ: デバッグメニューの「さいしょの ポケモン レントラー」の旗 HACK_FLAG_DEBUG_LUXRAY（0xF13）
 *   - 作り: 種族 → レントラー・Lv は台本のまま・技は CreateMon がレベルで覚える技。メス（きょせいどうぐと同じ性格値の入れ替え＝性格・特性の番号は変えない）、
 *     色違いと Moémon の当たりは自身の ID だけで決める（HackMonId_MakeShinyMoemon・性格値は作り直さない・6.5）。性別を先に直す＝裏 ID は性格値から先に固まる
 *     ので、あとの ID 書き込みで上書きされ、3 つは両立する
 */

//ここから NameMatches（tools/rom/verify_debug_starter_luxray.py がこの 1 関数を取り出して本物の C でためす・変えたら検算を見る）
bool8 HackStarterLuxray_NameMatches(const u8* name)
{
	//日本語版の文字の表（charmap.tbl）: E0 = l・DD = i・E6 = r・E9 = u・FF = 終わり。5 バイト目が終端でなければ（liruu など）効かない
	return name[0] == 0xE0 && name[1] == 0xDD && name[2] == 0xE6 && name[3] == 0xE9 && name[4] == 0xFF;
}
//ここまで

void HackGenderTool_MakeFemale(struct Pokemon* mon);   //src/hack_gender_tool.c

static bool8 Enabled(void)
{
	#ifdef HACK_DEBUG_MENU
	if (FlagGet(HACK_FLAG_DEBUG_LUXRAY))
		return TRUE;
	#endif
	return gSaveBlock2 != NULL && HackStarterLuxray_NameMatches(gSaveBlock2->playerName);
}

u16 HackStarterLuxray_Species(u16 species)
{
	return Enabled() ? SPECIES_LUXRAY : species;
}

//レントラーの「♀・色違い・Moémon」を付ける 1 本（2026-10-18・最初のポケモンと しばり『ぜんぶレントラー』〈src/hack_challenge_luxray.c〉が共有）。
//性別を先に直し（きょせいどうぐと同じ入れ替え）、色違いと Moémon の当たりを自身の ID だけで後に書く。種族がレントラーでなければ何もしない
void HackLuxray_ApplyIdentity(struct Pokemon* mon)
{
	if (GetMonData(mon, MON_DATA_SPECIES, NULL) != SPECIES_LUXRAY)
		return;
	HackGenderTool_MakeFemale(mon);
	HackMonId_MakeShinyMoemon((struct BoxPokemon*) mon);
}

void HackStarterLuxray_Finish(struct Pokemon* mon)
{
	if (!Enabled())
		return;
	HackLuxray_ApplyIdentity(mon);
}
