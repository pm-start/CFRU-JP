#include "defines.h"
#include "../include/event_data.h"
#include "../include/pokedex.h"
#include "hack_tamper.h" //★ 2026-10-21 改造ありの印（トレーナーカード）
/*
* カントー図鑑の数で決まるものを全国図鑑の数に（2026-10-16・patches/0480・ユーザー「フラッシュがカントー図鑑参照してるから全国図鑑参照させて」）。
*
* ・フラッシュ（2 ばんどうろ）などの助手・オーキド（モモカ）の評価の台本は VAR_0x8004 = 0 で GetPokedexCount（special 0xD4・080CB5A4）を呼び、
*   カントーの見た数・捕まえた数（8005／8006）を読む。080CB5AC の条件分岐（bne）を常に分岐（b）にして、0 のときも全国図鑑の数を返す（bytereplacement）。
* ・素の全国図鑑の数 08088A68 は 386 種までしか数えない（DPE の図鑑は 1039 種）。CFRU の GetNationalPokedexCount（util.c）へ差し替える（hooks）。
* ・素の カントーの数 08088AB8／カントー全部そろえた 08088B34 は DPE が「地方図鑑」の数・判定へ差し替え済み。呼び元（ずかんの数の表示・
*   セーブの画面・ヘルプ・トレーナーカード・タマムシのゲームデザイナーの賞状）は patches/0480 で全国図鑑の数・判定にした（DPE の updated_code.c）。
*   ずかんの画面（地方図鑑の一覧）は別の関数なので変わらない。
* ・オーキドの評価の文（GetProfOaksRatingMessageByCount・080CB5F0）はカントーの数 0〜151 で段階を決めていた。全国の数で決めるので、
*   全国図鑑の種数に対する割合で 15 段階にする（素の 10 刻み・150 を NATIONAL_DEX_COUNT に引き伸ばす）。全部そろえたときだけ「完成」。
*/

#define HACK_HAS_ALL_MONS ((bool16 (*)(void)) 0x08088B61)
#define FINAL_RATING_TIERS 15 //素の「〜10／〜20／…／〜150」の 15 段階

//素の文（日本語版 ROM 080CB5F0 の ldr のリテラル・tools/rom/verify_national_dex_counts.py が素の ROM と同じ番地か確かめる）
static const u32 sRatingTexts[FINAL_RATING_TIERS] =
{
	0x0819459C, 0x081945C9, 0x081945F9, 0x0819462E, 0x0819465C,
	0x0819468B, 0x081946BE, 0x081946DF, 0x08194704, 0x08194727,
	0x08194748, 0x0819476D, 0x0819478E, 0x081947B4, 0x081947D5,
};
#define RATING_TEXT_COMPLETE 0x081947F6

const u8* HackOakRatingMessageByCount(u16 count) //hooks 080CB5F0（素の GetProfOaksRatingMessageByCount の入口・戻り値は文・結果は gSpecialVar_LastResult）
{
	u32 tier;

	gSpecialVar_LastResult = FALSE;

	if (HACK_HAS_ALL_MONS()) //全国図鑑を全部そろえた（DPE が 08088B60 に差し替えた HasAllMons・1〜全国図鑑の数）
	{
		gSpecialVar_LastResult = TRUE;
		return (const u8*) RATING_TEXT_COMPLETE;
	}

	tier = ((u32) count * FINAL_RATING_TIERS) / NATIONAL_DEX_COUNT;
	if (tier >= FINAL_RATING_TIERS)
		tier = FINAL_RATING_TIERS - 1;

	return (const u8*) sRatingTexts[tier];
}

/*
* トレーナーカードの「ずかんの星」（2026-10-18・patches/0604・ユーザー「図鑑揃えるたびに星増やしたい」）。
* 素の自分のカード（SetPlayerCardData 0808934C・GetTrainerStarCount 0808930C）は殿堂入り・08088B00（素の HasAllHoennMons）・連勝・絵 の 4 つで星を数える。
* ここでは GetTrainerStarCount を差し替え、殿堂入り（素のまま）＋ この改造の地方図鑑 13 個（tools/regional_dex の RegionForNationalNumber が決める
* 全国番号の範囲）を 1 つ そろえるごとに +1 にする。星の最大は 殿堂入り 1 ＋ 地方図鑑 13 ＝ 14（連勝・絵は素の FR の自分のカードでは 0 のまま）。
* 「そろえた」＝その地方の全国番号が全部「捕まえた」（幻（ミュウなど）は素の HasAllKantoMons と同じく入れない・下の表）。
* カードの星の欄はセーブに入らない（開くたびに作る）。星の数はカードの 1 バイト（素のまま）・描くのは 1 つ 1 タイルで x=15〜28 の 14 個まで・
* 色は素の表が 0〜4 個の 5 色だけなので 5 個以上は金（HackCardPalHook・assembly/hack_dex/card_star_hook.s）・通信の色名も 4 まで（HackGetTrainerCardStars）。
*/
u8 RegionForNationalNumber(u16 number); //tools/regional_dex/regional_dex.c（同じビルドの中）
u8 RegionSegmentAt(u8 index, u16* lo, u16* hi); //同上（地方ごとの全国番号の範囲の表・終わりは 0xFF）
#define HACK_REGION_COUNT 13 //REGION_KANTO〜REGION_BLUEBERRY（REGION_COUNT = 13 は「どの地方でもない」）
#define HACK_GET_SET_POKEDEX_FLAG ((u8 (*)(u16, u8)) 0x08088A51) //素の GetSetPokedexFlag(nationalNum, caseId)

//幻のポケモン（そろえるのに入れない・全国番号）。素の HasAllKantoMons がミュウを外すのと同じ考え方
static const u16 sHackCardMythicals[] =
{
	151, 251, 385, 386, 489, 490, 491, 492, 493, 494, 647, 648, 649, 719, 720, 721, 801, 802, 807, 808, 809, 893, 1025,
};

static bool8 HackCardIsMythical(u16 num)
{
	u32 i;

	for (i = 0; i < sizeof(sHackCardMythicals) / sizeof(sHackCardMythicals[0]); ++i)
		if (sHackCardMythicals[i] == num)
			return TRUE;
	return FALSE;
}

//そろえた地方図鑑の数（0〜13）。regionDone が NULL でなければ地方ごとの 0／1 を入れる
//2026-11-03 perf-region-1: 1039 種を毎回 RegionForNationalNumber で振り分けず、地方ごとの範囲の表（RegionSegmentAt）を引く。
//「捕まえていない」が 1 つ見つかった地方はそこで打ち切る（旗を読む回数は前と同じ・読む順だけ地方ごとになる）
u8 HackCardCompletedRegions(u8* regionDone)
{
	u8 done[HACK_REGION_COUNT];
	u8 missing[HACK_REGION_COUNT];
	u8 members[HACK_REGION_COUNT];
	u16 num, lo, hi;
	u8 r, i, count;

	for (r = 0; r < HACK_REGION_COUNT; ++r)
		missing[r] = members[r] = 0;

	for (i = 0; (r = RegionSegmentAt(i, &lo, &hi)) != 0xFF; ++i)
	{
		if (r >= HACK_REGION_COUNT)
			continue;
		for (num = lo; num <= hi && !missing[r]; ++num)
		{
			if (HackCardIsMythical(num))
				continue;
			members[r] = 1;
			if (!HACK_GET_SET_POKEDEX_FLAG(num, FLAG_GET_CAUGHT))
				missing[r] = 1;
		}
	}

	for (r = 0, count = 0; r < HACK_REGION_COUNT; ++r)
	{
		done[r] = members[r] && !missing[r];
		if (regionDone != NULL)
			regionDone[r] = done[r];
		count += done[r];
	}
	return count;
}

//hooks 0808930C（素の GetTrainerStarCount・r0 = struct TrainerCard*・戻り値 u8）。素の呼び元は SetPlayerCardData(mode 0) の 08089430 だけで、mode 0 を呼ぶ所は無い（実際の自分のカードは HackOwnCardBuild が使う）
u8 HackTrainerCardStars(const u8* card)
{
	u8 stars = 0;

	if (*(const u16*)(card + 6) != 0 || *(const u32*)(card + 8) != 0)	//殿堂入りの時間（素のまま）
		++stars;
	stars += HackCardCompletedRegions(NULL);	//地方図鑑 1 つにつき +1
	if (*(const u16*)(card + 26) > 49)	//素のまま（自分のカードでは 0）
		++stars;
	if (card[4] != 0)	//素のまま（自分のカードでは 0）
		++stars;
	return stars;
}

//hooks 0808B1AC（HackOwnCardHook・assembly/hack_dex/card_star_hook.s）。素の InitTrainerCard が自分のカード（sData+0x448）を作る BL 08089498
//（TrainerCard_GenerateCardForLinkPlayer・通信で送るカードと同じ関数）の呼び出し。ここで作ったカードの [+1]（星）を差し替える。
//素の 08089498 は 殿堂入り＋カントー全部＋全国全部＋連勝（[+0x44]・[+0x48] が 200 以上）で星を数える（DPE が中の 2 つを同じ HasAllMons にした）。
//ここでは 殿堂入り＋地方図鑑＋連勝 に差し替える（連勝の分は素のまま残す）。
//通信で送るカード（0808048C・08080518・08117F1E の 3 か所）は素の関数のまま呼ぶ＝送る星は変えない。
void HackOwnCardBuild(u8* card)
{
	u8 stars;

	((void (*)(u8*)) 0x08089499)(card);	//素の TrainerCard_GenerateCardForLinkPlayer
	stars = HackTrainerCardStars(card);	//殿堂入り＋地方図鑑（[+6][+8] は上で入っている）
	if (*(const u32*)(card + 0x44) > 199 && *(const u32*)(card + 0x48) > 199)	//素の連勝の星（素のまま）
		++stars;
	card[1] = stars;
	if (HackTamper_IsMarked()) //★ 2026-10-21 改造ありの印: 名前の後ろに「！」を 1 つ足す（表示用の写しだけ。セーブの名前は変えない・遊びは止めない）
	{
		u8* name = card + 0x30;   //struct TrainerCard の playerName（pokefirered の並び: TrainerCardRSE +0x30・8 バイト。日本語版 ROM では未確認）
		u32 n = 0;

		while (n < 7 && name[n] != 0xFF)
			++n;
		if (n == 7)
			n = 6;   //7 文字ちょうどの名前は 最後の 1 字を「！」に替える（表示だけ）
		name[n] = 0xAB;   //「！」（上書き用ファイル/charmap.tbl の AB）
		name[n + 1] = 0xFF;
	}
	((void (*)(u32)) 0x08000545)(0x08089169);	//素の続き: SetMainCallback2(0x08089169)（BL 08000544）
}
//hooks 0808AC90（素の GetTrainerCardStars・r0 = 通信の相手の番号）。通信で出す色の名前の表は 4 個なので 4 まで
u8 HackGetTrainerCardStars(u8 id)
{
	u8 stars = *((u8*) 0x02039570 + id * 0x60 + 1);	//素は gTrainerCards[id].stars（0x60 バイトずつ）
	return stars > 4 ? 4 : stars;
}

//hooks 08088B00（素の HasAllHoennMons・呼び元は 080893A6 だけ）。カードの [+3] に入る印（星の数には使わなくなった）はカントー図鑑
bool8 HackCardCaughtAllKanto(void)
{
	u8 done[HACK_REGION_COUNT];

	HackCardCompletedRegions(done);
	return done[0];
}