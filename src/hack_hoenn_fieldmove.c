#include "defines.h"
#include "../include/event_data.h"
#include "../include/constants/items.h"
#include "../include/constants/moves.h"
#include "../include/new/overworld.h"

/*
 * ホウエンのいわくだきの岩の「手持ちにいわくだきを使える子がいるか」（2026-10-18・fix/hoenn-safari-ne）。
 *
 * エメラルドの EventScript_RockSmash は `checkpartymove MOVE_ROCK_SMASH`（＝命令 0x7C checkattack）で、手持ちが「いわくだきを覚えている」ことを要求する。
 * 素 FRLG の岩・木・かいりき岩も同じ 0x7C を使うが、CFRU の ONLY_CHECK_ITEM_FOR_HM_USAGE（ひでんマシンが道具にあり、覚えられる子がいれば覚えさせなくても使える・
 * patches/0013・0014）に合わせて命令のところを special 0x10D／0x120／0x121 に差し替えてある。ホウエンの岩は生成した別の台本なので
 * その差し替えが効かず、ヒデンマシン 06 を持ってリザードンを先頭にしていても「いわくだきを覚えていない」ため割れなかった（サファリゾーン右上の岩・ユーザー報告）。
 *
 * ここではバッジは見ない（ホウエンの台本が、その前にホウエンのバッジ＝ダイナモバッジの旗を見ている。素の sp120 は
 * カントーのバッジ6の旗を見るので、ホウエンだけ遊ぶ人が割れなくなる）。結果は命令 0x7C と同じく VAR_RESULT（gSpecialVar_LastResult）へ。
 * 覚えさせた子がいれば その子の手持ちの番号・いなければ 6（PARTY_SIZE）。
 * CLAUDE.md 6.42: callnative の関数は HoennNative_ で始める。
 */

void HoennNative_PartyCanRockSmash(void)
{
	gSpecialVar_LastResult = PartyHasMonWithFieldMovePotential(MOVE_ROCKSMASH, ITEM_HM06_ROCK_SMASH, SHOULDNT_BE_SURFING);
}

/*
 * ジョウトのいあいぎりの木（2026-11-09・FEAT-johto-j2c・assembly/johto_scripts/johto_people.s の JohtoCutTree が呼ぶ）。
 * 「手持ちにいあいぎりを使える子がいるか」を、いわくだきと同じ規則（ひでんマシン 01 を持っていて覚えられる子がいれば覚えさせなくても使える）で見る。
 * バッジは見ない（台本の前の行がジョウトのバッジの旗を見る＝素の sp10D はカントーのバッジ 2 の旗を見るので、ジョウトだけ遊ぶ人が切れなくなる）。
 * 結果は命令 0x7C と同じく VAR_RESULT（gSpecialVar_LastResult）へ。覚えさせた子がいれば その子の手持ちの番号・いなければ 6（PARTY_SIZE）。
 */
void HoennNative_PartyCanCut(void)
{
	u16 item = ITEM_NONE;

	#ifdef ONLY_CHECK_ITEM_FOR_HM_USAGE
	item = ITEM_HM01_CUT;
	#endif

	gSpecialVar_LastResult = PartyHasMonWithFieldMovePotential(MOVE_CUT, item, SHOULDNT_BE_SURFING);
}
