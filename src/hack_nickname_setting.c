#include "defines.h"
#include "../include/event_data.h"
#include "../include/script.h"

/*
 * せってい「ニックネーム」（patches/0405・2026-10-15・docs/research/2026-10-15_nickname-setting.md）。
 * 旗 FLAG_DONT_OFFER_NICKNAMES_BATTLE（0xF8E・src/config.h）が立っていると、手に入れたときの
 * 「ニックネームを つけますか？」を出さず、いいえ扱いで進める。下りている（既定・古いセーブ）なら今までどおり。
 *
 * 入口は 3 つ:
 *  - 戦闘の捕獲: 上流の TrySkipBattleNicknameOffer（src/battle_script_util.c）が旗を見る
 *  - タマゴのふ化: 上流の ShouldSkipOfferEggHatchNickname（src/daycare.c・hooks 8046A7E）が旗を見る
 *  - 台本のもらいもの（givemon のあと）: msgbox YESNO（callstd 5）の入口を repoints で
 *    SystemScript_MsgboxYesNo（assembly/overworld_scripts/system_scripts.s）に差し替え、下の関数で
 *    「出そうとしている文が 〜ニックネームを つけますか？ か」を見る（素のカントーの台本 13 か所・ホウエンの変換 4 か所・
 *    将来の台本もみんな同じ文なので、文の中身で見分ける）
 */

//「ニックネームを つけますか？」（文の最後の 15 バイト。日本語版 ROM の 0x1933BA・0x19324E など 4 つの文が同じ並びで終わる）
static const u8 sNicknamePromptTail[15] =
{
	0x66, 0xA0, 0x58, 0x68, 0xAE, 0x71, 0x2D, 0x00, 0x12, 0x09, 0x1F, 0x0D, 0x06, 0xAC, EOS,
};

//callasm（SystemScript_MsgboxYesNo の最初）: 旗が立っていて、これから出す文（gScriptEnv1.data[0]）が
//ニックネームの問いなら LASTRESULT = 1（問わずに いいえ）。それ以外は 0（今までどおり出す）
void HackNicknameSetting_CheckPrompt(void)
{
	const u8* text;
	u32 len = 0;
	u32 i;

	gSpecialVar_LastResult = 0;
	if (!FlagGet(FLAG_DONT_OFFER_NICKNAMES_BATTLE))
		return;

	text = (const u8*) gScriptEnv1.data[0];
	if ((u32) text < 0x02000000)
		return;

	while (len < 96 && text[len] != EOS)
		++len;
	if (text[len] != EOS || len + 1 < sizeof(sNicknamePromptTail))
		return;

	text += len + 1 - sizeof(sNicknamePromptTail);
	for (i = 0; i < sizeof(sNicknamePromptTail); ++i)
	{
		if (text[i] != sNicknamePromptTail[i])
			return;
	}
	gSpecialVar_LastResult = 1;
}
