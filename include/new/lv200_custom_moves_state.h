#pragma once

#include "../global.h"
#include "../constants/battle.h"

struct Lv200CustomMovesAState
{
	u8 endTurnCursor;
	u8 expiryCursor; /* 効果が切れる文を出す位置（ターンの終わり） */
	u8 finalCount[MAX_BATTLERS_COUNT];
	u8 fightingLink[MAX_BATTLERS_COUNT];
	u8 duelTimer[MAX_BATTLERS_COUNT];
	u8 stanceTimer[MAX_BATTLERS_COUNT];
	u8 typeSealTimer[MAX_BATTLERS_COUNT];
	u8 typeSealMask[MAX_BATTLERS_COUNT];
	u8 afterimageUses[MAX_BATTLERS_COUNT];
	u8 flyingBarrierTimer[MAX_BATTLERS_COUNT];
	u8 venomLayers[MAX_BATTLERS_COUNT];
	u8 poisonKingTimer[MAX_BATTLERS_COUNT];
	u8 sandTrapOwner[MAX_BATTLERS_COUNT];
	u8 eggTimer[MAX_BATTLERS_COUNT];
	u8 eggOwner[MAX_BATTLERS_COUNT];
	u8 lostTimer[MAX_BATTLERS_COUNT];
	u8 lostOwner[MAX_BATTLERS_COUNT];
	u8 soulContractTimer[MAX_BATTLERS_COUNT];
	u8 contractPeer[MAX_BATTLERS_COUNT];
	u8 fearTimer[MAX_BATTLERS_COUNT];
	u8 pupaTimer[MAX_BATTLERS_COUNT];
	u8 metalArmorTimer[MAX_BATTLERS_COUNT];
	u8 mirrorArmorTimer[MAX_BATTLERS_COUNT];
	u8 turboTimer[MAX_BATTLERS_COUNT];
	u8 crackedTimer[MAX_BATTLERS_COUNT];
	u8 cocoonArmorTimer[MAX_BATTLERS_COUNT];
	u8 burnMarkTimer[MAX_BATTLERS_COUNT];
	u8 blazeFormTimer[MAX_BATTLERS_COUNT];
	u8 stitchedOwner[MAX_BATTLERS_COUNT];
	u8 stitchedTimer[MAX_BATTLERS_COUNT];
	u8 rockHitIndex[MAX_BATTLERS_COUNT];
	u8 quartzTimer[NUM_BATTLE_SIDES];
	u8 powderCurtainTimer[NUM_BATTLE_SIDES];
	u8 steelWallTimer[NUM_BATTLE_SIDES];
	u8 mudFieldTimer;
	u8 aftershockTimer;
	u8 rockFieldTimer;
	u8 rockFieldOwner;
	u8 darkVeilTimer;
	u8 focusBoostBits;
	u8 lastStandBits;
	u8 lastStandBoostBits;
	u8 garnetGuardBits;
	u8 pupaBits;
	u8 huskSubstituteBits;
	u8 grudgeBits;
	u8 lastSpiritBits;
	u8 steelTuneBits;
	u8 steelSpikesBits;
	u8 sandHazardBits;
	u8 rockCurtainBits;
	u8 fireChargeBits;
	u8 fireCloakBits;
	u8 fireCloakBoostBits;
	u8 wingGuardBits;
	u8 forcedSwitchBank;
	u8 pendingTarget[MAX_BATTLERS_COUNT];
	u16 pendingMove[MAX_BATTLERS_COUNT];
	u16 echoMove[MAX_BATTLERS_COUNT];
	u16 miningReward[NUM_BATTLE_SIDES];
	u32 echoDamage[MAX_BATTLERS_COUNT];
	u32 physicalDamageHistory[MAX_BATTLERS_COUNT][2];
};

struct Lv200CustomMovesBState
{
	u8 endTurnCursor;
	u8 expiryCursor; /* 効果が切れる文を出す位置（ターンの終わり） */
	/* 場全体・側ごと。 */
	u8 customFieldKind;
	u8 customFieldTimer;
	u8 abilityItemNullTimer;
	u8 iceAgeTimer;
	u8 dragonPalaceTimer;
	u8 dragonPalaceSide;
	u8 terrainOwner;
	u8 gravityOwner;
	u8 snowOwner;
	u8 terrainOwnerSide;
	u8 tideTimer[NUM_BATTLE_SIDES];
	u8 frozenHazardPresent[NUM_BATTLE_SIDES];
	u8 frozenHazardBank[NUM_BATTLE_SIDES];

	/* 出場位置へ一度だけ渡す効果。値は PendingPositionEffect のビット和。 */
	u8 pendingPositionEffects[MAX_BATTLERS_COUNT];

	/* バンクごとの残りターン・段階。交代時に ClearBattler で消す。 */
	u8 deepSeaTimer[MAX_BATTLERS_COUNT];
	u8 timedTrapTimer[MAX_BATTLERS_COUNT];
	u8 timedTrapOwner[MAX_BATTLERS_COUNT];
	u8 chargeStacks[MAX_BATTLERS_COUNT];
	u8 electricPpPhase[MAX_BATTLERS_COUNT];
	u8 microwaveTimer[MAX_BATTLERS_COUNT];
	u8 mirrorTimer[MAX_BATTLERS_COUNT];
	u8 mirrorSource[MAX_BATTLERS_COUNT];
	u8 coldGazeTimer[MAX_BATTLERS_COUNT];
	u8 fixedFreezeTimer[MAX_BATTLERS_COUNT];
	u8 selfSureHitTimer[MAX_BATTLERS_COUNT];
	u8 timedAbilitySuppress[MAX_BATTLERS_COUNT];
	u8 anxietyTimer[MAX_BATTLERS_COUNT];
	u8 mimicTimer[MAX_BATTLERS_COUNT];
	u8 mimicSavedPp[MAX_BATTLERS_COUNT];
	u8 mimicSlot[MAX_BATTLERS_COUNT];
	u8 temporaryRilTimer[MAX_BATTLERS_COUNT];
	u8 statRiseBlockedTimer[MAX_BATTLERS_COUNT];
	u8 sleepFixTurns[MAX_BATTLERS_COUNT];
	u8 perishOwnerBits;
	u8 substituteEffect[MAX_BATTLERS_COUNT];
	u16 mimicSavedMove[MAX_BATTLERS_COUNT];
	u16 copiedItem[MAX_BATTLERS_COUNT];
	u32 mirrorSpecialDamage[MAX_BATTLERS_COUNT];
	u32 currentRoundDamage[MAX_BATTLERS_COUNT];
	u32 previousRoundDamage[MAX_BATTLERS_COUNT];
	u32 lastDamageTaken[MAX_BATTLERS_COUNT];
	u8 lastDamageSource[MAX_BATTLERS_COUNT];

	/* 4バンク分の真偽値。 */
	u8 strongLeechBits;
	u8 armoredRootBits;
	u8 suspiciousSleepBits;
	u8 frostbiteBits;
	u8 frostbitePendingBits;
	u8 dragonSoulBits;
	u8 dragon130SpeedDownBits;
	u8 dragon170SpeedUpBits;
	u8 flinchedThisRoundBits;
	u8 flinchedPreviousRoundBits;
	u8 heartGuardBits;
	u8 dreamPrincessBits;
	u8 statRiseBlockedBits;
	u8 abilitySuppressOwnedBits;
	u8 darknessHoleItemSuppressedBits;
	u8 destinySuccessorBits;
	u8 tearMoonArmedBits;
	u8 tearMoonBoostBits;
	u8 psychic170BoostBits;
	u8 psychicBarrierBits;
};

/* 共通部品(P3): 技の効果で起きた「能力変化・回復・ダメージ・治療」を積み、
 * cmd49 の ATK49_LV200_MOVE_END で 1 件ずつ共通台本(アニメ・文・HP バー・手持ちへ送る・ひんし)に流す。
 * 戦闘ごとに gNewBS 末尾へ置く（固定 RAM を使わない）。 */
#define LV200_COMMON_QUEUE_SIZE 12

struct Lv200CommonEffect
{
	u8 kind;        /* LV200_COMMON_KIND_*（0=空き） */
	u8 bank;
	u8 source;      /* 能力変化の使い手（相手に掛けるとき） */
	u8 statChanger; /* STAT_* | INCREASE_n / DECREASE_n */
	u16 amount;     /* 実際に増減した HP */
	u16 hpBefore;   /* この効果の直前の HP（画面に出す前に巻き戻す値） */
	u8 flags;       /* bit0=単体の「これ以上上がらない」文を出さない */
	u8 pad;
	const u8 *text; /* MESSAGE: 出す文（BATTLE_STRING_LOADER に入れる） */
};

struct Lv200CustomCommonState
{
	u8 head;
	u8 count;
	u8 banksSaved;
	u8 savedAttacker;
	u8 savedTarget;
	u8 savedResultFlags;
	u8 poisonBypass; /* ひふくぶんかい: 付与の処理中だけ、はがねにも毒が通る */
	u8 pad;
	s32 savedDamage;
	struct Lv200CommonEffect queue[LV200_COMMON_QUEUE_SIZE];
};
