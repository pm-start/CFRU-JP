#include "defines.h"
#include "../include/item.h"
#include "../include/item_use.h"
#include "../include/party_menu.h"
#include "../include/pokemon.h"
#include "../include/sound.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/constants/songs.h"
#include "hack_mon_id.h"

/*
 * きょせいどうぐ（ITEM_GENDER_TOOL 0x346・2026-10-06）: 手持ちのオス・メスを入れ替える消費アイテム。
 * ユーザー「きょせいどうぐを実装してほしい　オス・メスの切り替え　消費アイテム　タマムシデパートの6Fの人物から買える」。
 *
 * 性別は性格値（PID）の下位 8 ビットと種族の性別の比で決まる（第 3 世代）。性格値を作り直すのは禁止（6.5・色違いが戻る）。
 * そこで性格値の上位 16 ビットと下位 16 ビットに同じ値 d を XOR する。色違いの判定は
 * (トレーナー ID ^ 裏 ID ^ 上位 ^ 下位) なので、上位と下位を同じだけ変えれば色違いはそのまま。
 * そのうえで、性格（PID % 25）と特性の番号（PID の bit 0）と萌えもんかどうか（2026-10-09）が変わらず、性別だけ変わる d を 1 から順に探す。
 * 変わりうるもの: パッチールのぶち・ケムッソの進化先（上位 16 ビットの % 10）・大きさ。
 * 性別が決まっている種族（性別不明・オスだけ・メスだけ）は「つかっても こうかが ないよ」で減らさない。
 */

u8 __attribute__((long_call)) DisplayPartyMenuMessage(const u8* str, u8 b);
void __attribute__((long_call)) UpdateMonDisplayInfoAfterRareCandy(u8 slot, struct Pokemon *mon);
#define gText_WontHaveEffect ((const u8*) 0x83DDE0B)
extern const u8 gText_GenderTool_Male[];
extern const u8 gText_GenderTool_Female[];
extern const u8 gText_GenderTool_Partner[];
bool8 HackPartner_IsMon(const struct Pokemon* mon); //src/hack_story_partner.c

//★ 2026-10-09 ユーザー「きょせいどうぐを使用した際にmoemonが通常種になってしまうのでmoemonのままになるまで抽選してほしい通常種も同様」
//  「色違いがも考慮して」: 萌えもんかどうか（DPE の src/hack_moemon.c・CFRU の src/hack_moemon_switch.c の MOEMON_HIT と同じ式）も
//  変えない d を選ぶ。色違いは上の XOR の形で変わらない
//★ 2026-10-10 #70 第 2 段: 萌えもんは裏 ID で決まる（性格値を変える前に HackMonId_Ensure）ので、萌えもんの縛りは外した

static bool8 FindFlippedGender(struct Pokemon* mon, u32* newPersonality)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u8 ratio = gBaseStats[species].genderRatio;
	u32 pid = mon->personality;
	u8 gender = GetGenderFromSpeciesAndPersonality(species, pid);
	u32 d, npid;

	if (ratio == MON_MALE || ratio == MON_FEMALE || ratio == MON_GENDERLESS)
		return FALSE;

	/* An odd XOR mask changes the ability bit, so only even masks qualify. */
	for (d = 2; d <= 0xFFFF; d += 2)
	{
		npid = pid ^ (d | (d << 16));
		if (npid % NUM_NATURES != pid % NUM_NATURES
		|| GetGenderFromSpeciesAndPersonality(species, npid) == gender)
			continue;

		*newPersonality = npid;
		return TRUE;
	}

	return FALSE;
}

static void ItemUseCB_GenderTool(u8 taskId, TaskFunc func)
{
	struct Pokemon* mon = &gPlayerParty[gPartyMenu.slotId];
	u16 item = Var800E;
	u32 newPersonality;

	PlaySE(SE_SELECT);
	//★ 2026-10-08 ホウエンのストーリーの相棒には使えない（性格値で相棒を見分けている・ユーザー「きょせいどうぐとか使えなくして」）
	if (HackPartner_IsMon(mon))
	{
		gPartyMenuUseExitCallback = FALSE;
		DisplayPartyMenuMessage(gText_GenderTool_Partner, TRUE);
		ScheduleBgCopyTilemapToVram(2);
		gTasks[taskId].func = func;
		return;
	}
	if (GetMonData(mon, MON_DATA_IS_EGG, NULL) || !FindFlippedGender(mon, &newPersonality)
	|| !RemoveBagItem(item, 1))
	{
		gPartyMenuUseExitCallback = FALSE;
		DisplayPartyMenuMessage(gText_WontHaveEffect, TRUE);
		ScheduleBgCopyTilemapToVram(2);
		gTasks[taskId].func = func;
		return;
	}

	HackMonId_Ensure((struct BoxPokemon*) mon); // Preserve appearance before changing personality.
	mon->personality = newPersonality;
	UpdateMonDisplayInfoAfterRareCandy(gPartyMenu.slotId, mon); //手持ちの画面の性別の記号を描き直す
	GetMonNickname(mon, gStringVar1);
	StringExpandPlaceholders(gStringVar4, GetMonGender(mon) == MON_MALE ? gText_GenderTool_Male : gText_GenderTool_Female);
	DisplayPartyMenuMessage(gStringVar4, TRUE);
	ScheduleBgCopyTilemapToVram(2);
	gTasks[taskId].func = func;
}

void FieldUseFunc_GenderTool(u8 taskId)
{
	gItemUseCB = ItemUseCB_GenderTool;
	SetUpItemUseCallback(taskId);
}

/* 2026-10-18 しばり『ぜんぶレントラー』: 上の入れ替え（上下 16 ビットに同じ偶数を XOR）で 性別だけが変わる値が無い PID が約 0.2% ある（偶数も奇数も無いのが約 0.05%）。
 * メスにすると決めた子がオスのまま残らないよう、最後の手として 下位 1 バイトを「メスの側（境より下）」にし、性格（PID % 25）と特性の番号（PID の bit 0）を保つ値を探す。
 * 色違い・Moémon は自身の ID（HackMonId_Ensure で先に固める・そのあと HackMonId_MakeShinyMoemon が決める）で決まるので PID に縛られない。
 * 下位 1 バイトを決めたあと 256 の倍数を足して 25 で割った余りをそろえる（256 と 25 は互いに素＝0〜24 回のどれかで合う）。きょせいどうぐ（FindFlippedGender だけ）は変えない */
static bool8 FindFemalePersonality(struct Pokemon* mon, u32* newPersonality)
{
	u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	u8 ratio = gBaseStats[species].genderRatio;
	u32 pid = mon->personality;
	u32 b, k, base, cand;

	if (ratio == MON_MALE || ratio == MON_FEMALE || ratio == MON_GENDERLESS)
		return FALSE;
	for (b = 0; b < ratio; ++b)   //低いバイトがこの境より下ならメス
	{
		if (((b ^ pid) & 1) != 0)
			continue;   //特性の番号の bit は そのまま
		base = (pid & 0xFFFFFF00) | b;
		for (k = 0; k < NUM_NATURES; ++k)
		{
			cand = base + (k << 8);
			if (cand % NUM_NATURES == pid % NUM_NATURES && GetGenderFromSpeciesAndPersonality(species, cand) != MON_MALE)
			{
				*newPersonality = cand;
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* 最初のポケモン レントラー♀（src/hack_starter_luxray.c）・しばり『ぜんぶレントラー』: オスならメスへ。きょせいどうぐと同じ入れ替え（性格・特性の番号・色違いはそのまま・6.5）。
 * 入れ替えで届かない約 0.2% は 上の FindFemalePersonality（性格と特性の番号を保つ）。
 * 裏 ID を先に固める（HackMonId_Ensure）。もうメス・性別なしは何もしない */
void HackGenderTool_MakeFemale(struct Pokemon* mon)
{
	u32 newPersonality;

	if (GetMonGender(mon) != MON_MALE)
		return;
	if (!FindFlippedGender(mon, &newPersonality) && !FindFemalePersonality(mon, &newPersonality))
		return;
	HackMonId_Ensure((struct BoxPokemon*) mon);
	SetMonData(mon, MON_DATA_PERSONALITY, &newPersonality);
	CalculateMonStats(mon);
}
