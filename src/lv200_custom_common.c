#include "defines.h"
#include "defines_battle.h"
#include "lv200_custom_moves.h"
#include "lv200_custom_common.h"
#include "../include/new/util.h"

extern const u8 BattleScript_Lv200Common_SelfUp[];
extern const u8 BattleScript_Lv200Common_SelfUpQuiet[];
extern const u8 BattleScript_Lv200Common_SelfDown[];
extern const u8 BattleScript_Lv200Common_SelfDownQuiet[];
extern const u8 BattleScript_Lv200Common_FoeUp[];
extern const u8 BattleScript_Lv200Common_FoeUpQuiet[];
extern const u8 BattleScript_Lv200Common_FoeDown[];
extern const u8 BattleScript_Lv200Common_FoeDownQuiet[];
extern const u8 BattleScript_Lv200Common_Heal[];
extern const u8 BattleScript_Lv200Common_Hurt[];
extern const u8 BattleScript_Lv200Common_CureStatus[];
extern const u8 BattleScript_Lv200Common_CureConfusion[];
extern const u8 BattleScript_Lv200Common_Message[];
extern const u8 BattleScript_Lv200Common_Inflict[];
extern const u8 BattleScript_Lv200Common_Weather[];
extern const u8 BattleScript_Lv200Common_Infatuate[];
bool8 SetSunnyWeather(void);
bool8 SetRainyWeather(void);
bool8 SetSandstormWeather(void);
bool8 SetHailWeather(void);

#define LV200_COMMON_FLAG_POISON_STEEL 0x2
#define LV200_COMMON_FLAG_FORCE_AIRBORNE 0x4
#define LV200_COMMON_FLAG_SAME_GENDER 0x8
#define LV200_COMMON_FLAG_FROM_SOURCE 0x10

/* 戦闘中の値は ROM 内の static ではなく gNewBS の欄(lv200CustomCommon.poisonBypass)に持つ（ROM の static は書けない） */
bool8 Lv200Common_PoisonIgnoresSteel(void)
{
	return gNewBS != NULL && gNewBS->lv200CustomCommon.poisonBypass;
}

void Lv200Common_ClearPoisonBypass(void)
{
	if (gNewBS != NULL)
		gNewBS->lv200CustomCommon.poisonBypass = FALSE;
}

#define LV200_COMMON_FLAG_QUIET 0x1

static struct Lv200CustomCommonState *Lv200Common_State(void)
{
	return gNewBS == NULL ? NULL : &gNewBS->lv200CustomCommon;
}

static void Lv200Common_RestoreSaved(struct Lv200CustomCommonState *state)
{
	if (!state->banksSaved)
		return;
	gBankAttacker = state->savedAttacker;
	gBankTarget = state->savedTarget;
	gMoveResultFlags = state->savedResultFlags;
	gBattleMoveDamage = state->savedDamage;
	state->banksSaved = FALSE;
}

void Lv200Common_Reset(void)
{
	struct Lv200CustomCommonState *state = Lv200Common_State();

	if (state != NULL)
		Memset(state, 0, sizeof(*state));
}

/* 積んだ効果を捨てる（ターンの切れ目で持ち越さない）。取り出しの途中なら保存した値は戻す。 */
void Lv200Common_Clear(void)
{
	struct Lv200CustomCommonState *state = Lv200Common_State();

	if (state == NULL)
		return;
	Lv200Common_RestoreSaved(state);
	Memset(state, 0, sizeof(*state));
}

/* 交代・ひんしで場を離れた個体の分は捨てる（別の個体に文や HP を出さない）。 */
void Lv200Common_OnSwitchOut(u8 bank)
{
	struct Lv200CustomCommonState *state = Lv200Common_State();
	u8 i;

	if (state == NULL)
		return;
	for (i = state->head; i < state->count; ++i)
	{
		struct Lv200CommonEffect *effect = &state->queue[i];
		if (effect->bank == bank
		 || (effect->kind == LV200_COMMON_KIND_STAT_FOE && effect->source == bank))
			effect->kind = LV200_COMMON_KIND_NONE;
	}
}

static struct Lv200CommonEffect *Lv200Common_Push(u8 kind, u8 bank)
{
	struct Lv200CustomCommonState *state = Lv200Common_State();
	struct Lv200CommonEffect *effect;

	if (state == NULL || state->count >= LV200_COMMON_QUEUE_SIZE)
		return NULL;
	effect = &state->queue[state->count++];
	Memset(effect, 0, sizeof(*effect));
	effect->kind = kind;
	effect->bank = bank;
	effect->source = bank;
	return effect;
}

void Lv200Common_QueueStat(u8 bank, u8 source, u8 stat, s8 delta, bool8 quiet)
{
	struct Lv200CommonEffect *effect;
	u8 amount;

	if (bank >= gBattlersCount || source >= gBattlersCount || delta == 0
	 || stat < STAT_STAGE_ATK || stat >= BATTLE_STATS_NO)
		return;
	effect = Lv200Common_Push(bank == source ? LV200_COMMON_KIND_STAT_SELF : LV200_COMMON_KIND_STAT_FOE, bank);
	if (effect == NULL)
		return;
	effect->source = source;
	amount = delta > 0 ? delta : -delta;
	if (amount > 6)
		amount = 6;
	effect->statChanger = stat | (amount << 4) | (delta < 0 ? 0x80 : 0);
	effect->flags = quiet ? LV200_COMMON_FLAG_QUIET : 0;
}

u32 Lv200Common_HealBank(u8 bank, u32 amount)
{
	struct Lv200CommonEffect *effect;
	u32 hp;

	if (bank >= gBattlersCount || gBattleMons[bank].hp == 0 || amount == 0)
		return 0;
	hp = gBattleMons[bank].hp + amount;
	if (hp > gBattleMons[bank].maxHP)
		hp = gBattleMons[bank].maxHP;
	amount = hp - gBattleMons[bank].hp;
	if (amount == 0)
		return 0;
	effect = Lv200Common_Push(LV200_COMMON_KIND_HEAL, bank);
	if (effect != NULL)
	{
		effect->amount = amount;
		effect->hpBefore = gBattleMons[bank].hp;
	}
	gBattleMons[bank].hp = hp;
	return amount;
}

u32 Lv200Common_HurtBank(u8 bank, u32 amount)
{
	struct Lv200CommonEffect *effect;

	if (bank >= gBattlersCount || gBattleMons[bank].hp == 0 || amount == 0)
		return 0;
	if (amount >= gBattleMons[bank].hp)
		amount = gBattleMons[bank].hp;
	effect = Lv200Common_Push(LV200_COMMON_KIND_HURT, bank);
	if (effect != NULL)
	{
		effect->amount = amount;
		effect->hpBefore = gBattleMons[bank].hp;
	}
	gBattleMons[bank].hp -= amount;
	return amount;
}

/* だいちのめぐみ(#4)の治療を共通化: 戦闘中の値・悪夢・B 側のしもやけの印・手持ちのデータ。 */
void Lv200Common_CureStatusBank(u8 bank)
{
	if (bank >= gBattlersCount)
		return;
	gBattleMons[bank].status1 = 0;
	gBattleMons[bank].status2 &= ~STATUS2_NIGHTMARE;
	Lv200CustomMove_OnStatusCured(bank);
	gActiveBattler = bank;
	EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[bank].status1);
	MarkBufferBankForExecution(gActiveBattler);
}

void Lv200Common_CureConfusionBank(u8 bank)
{
	if (bank < gBattlersCount)
		gBattleMons[bank].status2 &= ~STATUS2_CONFUSION;
}

void Lv200Common_QueueCureStatus(u8 bank)
{
	if (bank >= gBattlersCount || gBattleMons[bank].status1 == 0)
		return;
	Lv200Common_CureStatusBank(bank);
	Lv200Common_Push(LV200_COMMON_KIND_CURE_STATUS, bank);
}

void Lv200Common_QueueCureConfusion(u8 bank)
{
	if (bank >= gBattlersCount || !(gBattleMons[bank].status2 & STATUS2_CONFUSION))
		return;
	Lv200Common_CureConfusionBank(bank);
	Lv200Common_Push(LV200_COMMON_KIND_CURE_CONFUSION, bank);
}

void Lv200Common_QueueMessage(u8 bank, const u8 *text)
{
	struct Lv200CommonEffect *effect;

	if (bank >= gBattlersCount || text == NULL)
		return;
	effect = Lv200Common_Push(LV200_COMMON_KIND_MESSAGE, bank);
	if (effect != NULL)
		effect->text = text;
}

/* 2 者の文用: [TARGET]=bank・[ATTACKER]=source（Lv200Common_QueueMessage は両方 bank）。 */
void Lv200Common_QueueMessageFrom(u8 bank, u8 source, const u8 *text)
{
	struct Lv200CommonEffect *effect;

	if (bank >= gBattlersCount || source >= gBattlersCount || text == NULL)
		return;
	effect = Lv200Common_Push(LV200_COMMON_KIND_MESSAGE, bank);
	if (effect != NULL)
	{
		effect->text = text;
		effect->source = source;
		effect->flags |= LV200_COMMON_FLAG_FROM_SOURCE;
	}
}

void Lv200Common_QueueInflict(u8 bank, u8 source, u8 moveEffect, bool8 poisonIgnoresSteel)
{
	struct Lv200CommonEffect *effect;

	if (bank >= gBattlersCount || source >= gBattlersCount || moveEffect == 0)
		return;
	effect = Lv200Common_Push(LV200_COMMON_KIND_INFLICT, bank);
	if (effect == NULL)
		return;
	effect->source = source;
	effect->statChanger = moveEffect;
	effect->flags = poisonIgnoresSteel ? LV200_COMMON_FLAG_POISON_STEEL : 0;
}

/* 素の CanBeInfatuated は「性別が違う」を要る。同じ性別を要る技(ときめきスパーク)のために、性別の見方だけ切り替える。 */
bool8 Lv200Common_CanInfatuate(u8 bank, u8 source, bool8 sameGender)
{
	u8 gDef, gAtk;

	if (bank >= gBattlersCount || source >= gBattlersCount)
		return FALSE;
	if (!sameGender)
		return CanBeInfatuated(bank, source);
	gDef = GetGenderFromSpeciesAndPersonality(gBattleMons[bank].species, gBattleMons[bank].personality);
	gAtk = GetGenderFromSpeciesAndPersonality(gBattleMons[source].species, gBattleMons[source].personality);
	return BATTLER_ALIVE(bank)
		&& !(gBattleMons[bank].status2 & STATUS2_INFATUATION)
		&& (!BankHasAbility(bank, ABILITY_OBLIVIOUS) || IsTargetAbilityIgnoredNoMove(ABILITY_OBLIVIOUS, ABILITY(source)))
		&& gDef == gAtk && gDef != MON_GENDERLESS
		&& !AbilityBattleEffects(ABILITYEFFECT_CHECK_BANK_SIDE, bank, ABILITY_AROMAVEIL, 0, 0);
}

void Lv200Common_QueueInfatuate(u8 bank, u8 source, bool8 sameGender)
{
	struct Lv200CommonEffect *effect;

	if (!Lv200Common_CanInfatuate(bank, source, sameGender))
		return;
	effect = Lv200Common_Push(LV200_COMMON_KIND_INFATUATE, bank);
	if (effect != NULL)
	{
		effect->source = source;
		effect->flags = sameGender ? LV200_COMMON_FLAG_SAME_GENDER : 0;
	}
}

bool8 Lv200Common_QueueWeather(u8 bank, u8 kind)
{
	struct Lv200CommonEffect *effect;
	u8 savedAttacker = gBankAttacker;
	u8 savedChooser = gBattleCommunication[MULTISTRING_CHOOSER];
	bool8 changed;

	if (bank >= gBattlersCount)
		return FALSE;
	gBankAttacker = bank; //持続を延ばす石は使い手のもの
	switch (kind)
	{
	case LV200_WEATHER_SUN: changed = SetSunnyWeather(); break;
	case LV200_WEATHER_RAIN: changed = SetRainyWeather(); break;
	case LV200_WEATHER_SAND: changed = SetSandstormWeather(); break;
	default: changed = SetHailWeather(); break;
	}
	gBankAttacker = savedAttacker;
	if (changed)
	{
		effect = Lv200Common_Push(LV200_COMMON_KIND_WEATHER, bank);
		if (effect != NULL)
			effect->statChanger = gBattleCommunication[MULTISTRING_CHOOSER]; //素の天気開始文の番号
	}
	gBattleCommunication[MULTISTRING_CHOOSER] = savedChooser;
	return changed;
}

/* 撃ち落とす（浮いている相手を地面に落とし、ふうせんも割る）。素のうちおとすの追加効果(MOVE_EFFECT_BRING_DOWN)を使う。
 * 呼ぶ側が「今浮いている」と確かめた相手にだけ使う（タイプ封印などで素の判定が変わる前に決めておくため）。 */
void Lv200Common_QueueBringDown(u8 bank, u8 source)
{
	struct Lv200CommonEffect *effect;

	if (bank >= gBattlersCount || source >= gBattlersCount)
		return;
	effect = Lv200Common_Push(LV200_COMMON_KIND_INFLICT, bank);
	if (effect == NULL)
		return;
	effect->source = source;
	effect->statChanger = MOVE_EFFECT_BRING_DOWN;
	effect->flags = LV200_COMMON_FLAG_FORCE_AIRBORNE;
}

static const u8 *Lv200Common_StatScript(const struct Lv200CommonEffect *effect)
{
	bool8 down = (effect->statChanger & 0x80) != 0;
	bool8 quiet = (effect->flags & LV200_COMMON_FLAG_QUIET) != 0;

	if (effect->kind == LV200_COMMON_KIND_STAT_SELF)
	{
		if (down)
			return quiet ? BattleScript_Lv200Common_SelfDownQuiet : BattleScript_Lv200Common_SelfDown;
		return quiet ? BattleScript_Lv200Common_SelfUpQuiet : BattleScript_Lv200Common_SelfUp;
	}
	if (down)
		return quiet ? BattleScript_Lv200Common_FoeDownQuiet : BattleScript_Lv200Common_FoeDown;
	return quiet ? BattleScript_Lv200Common_FoeUpQuiet : BattleScript_Lv200Common_FoeUp;
}

/* cmd49 の ATK49_LV200_MOVE_END から呼ぶ。1 回に 1 件。積み終わったら保存した値を戻して NULL。 */
const u8 *Lv200Common_GetMoveEndScript(void)
{
	struct Lv200CustomCommonState *state = Lv200Common_State();

	if (state == NULL)
		return NULL;
	while (state->head < state->count)
	{
		struct Lv200CommonEffect *effect = &state->queue[state->head++];
		u8 bank = effect->bank;

		if (effect->kind == LV200_COMMON_KIND_NONE || bank >= gBattlersCount)
			continue;
		/* 場に居ない個体には回復・能力変化・治療の知らせを出さない。ダメージ(ひんし)だけは通す。 */
		if (effect->kind != LV200_COMMON_KIND_HURT && gBattleMons[bank].hp == 0)
			continue;
		if (!state->banksSaved)
		{
			state->banksSaved = TRUE;
			state->savedAttacker = gBankAttacker;
			state->savedTarget = gBankTarget;
			state->savedResultFlags = gMoveResultFlags;
			state->savedDamage = gBattleMoveDamage;
		}
		switch (effect->kind)
		{
		case LV200_COMMON_KIND_STAT_SELF:
			gBankAttacker = bank;
			gBankTarget = bank;
			gBattleScripting.statChanger = effect->statChanger;
			return Lv200Common_StatScript(effect);
		case LV200_COMMON_KIND_STAT_FOE:
			gBankAttacker = effect->source;
			gBankTarget = bank;
			gBattleScripting.statChanger = effect->statChanger;
			return Lv200Common_StatScript(effect);
		case LV200_COMMON_KIND_HEAL:
			/* 直前の HP に巻き戻して、台本の datahpupdate に増減を任せる（HP バー・手持ちへ送る）。 */
			gBattleMons[bank].hp = effect->hpBefore;
			gBattleMoveDamage = -(s32) effect->amount;
			gBankAttacker = bank;
			gBankTarget = bank;
			return BattleScript_Lv200Common_Heal;
		case LV200_COMMON_KIND_HURT:
			gBattleMons[bank].hp = effect->hpBefore;
			gBattleMoveDamage = effect->amount;
			gBankAttacker = bank;
			gBankTarget = bank;
			return BattleScript_Lv200Common_Hurt;
		case LV200_COMMON_KIND_CURE_STATUS:
			gBankAttacker = bank;
			gBankTarget = bank;
			return BattleScript_Lv200Common_CureStatus;
		case LV200_COMMON_KIND_CURE_CONFUSION:
			gBankAttacker = bank;
			gBankTarget = bank;
			return BattleScript_Lv200Common_CureConfusion;
		case LV200_COMMON_KIND_MESSAGE:
			gBankAttacker = (effect->flags & LV200_COMMON_FLAG_FROM_SOURCE) ? effect->source : bank;
			gBankTarget = bank;
			gBattleStringLoader = effect->text;
			return BattleScript_Lv200Common_Message;
		case LV200_COMMON_KIND_INFLICT:
			gBankAttacker = effect->source;
			gBankTarget = bank;
			gBattleCommunication[MOVE_EFFECT_BYTE] = effect->statChanger;
			state->poisonBypass = (effect->flags & LV200_COMMON_FLAG_POISON_STEEL) != 0;
			if (effect->flags & LV200_COMMON_FLAG_FORCE_AIRBORNE)
				gStatuses3[bank] |= STATUS3_LEVITATING; /* 素の撃ち落としの「浮いている」判定を通す（落とすと消える） */
			return BattleScript_Lv200Common_Inflict;
		case LV200_COMMON_KIND_INFATUATE:
			if (!Lv200Common_CanInfatuate(bank, effect->source, (effect->flags & LV200_COMMON_FLAG_SAME_GENDER) != 0))
				continue; /* 積んだ後で変わって通らなくなったら何も出さない */
			gBattleMons[bank].status2 |= STATUS2_INFATUATED_WITH(effect->source);
			gBankAttacker = effect->source;
			gBankTarget = bank;
			return BattleScript_Lv200Common_Infatuate;
		case LV200_COMMON_KIND_WEATHER:
			gBankAttacker = bank;
			gBankTarget = bank;
			gBattleCommunication[MULTISTRING_CHOOSER] = effect->statChanger;
			return BattleScript_Lv200Common_Weather;
		default:
			break;
		}
	}
	Lv200Common_RestoreSaved(state);
	state->head = 0;
	state->count = 0;
	return NULL;
}
