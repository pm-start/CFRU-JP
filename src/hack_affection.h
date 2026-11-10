#pragma once

//なつき度で戦いに効く 10 の効果（src/hack_affection.c・patches/0240）
//せってい「なかよし」（patches/0418・5e の台帳 0xF8F）: 旗が立っていると全部の効果を止める（下りている＝既定・古いセーブは今までどおり効く）
#define HACK_FLAG_AFFECTION_OFF 0xF8F
void HackAffection_MoveStart(u8 bankAtk);
bool8 HackAffection_TryLastMonMessage(u8 bankAtk);
bool8 HackAffection_TryEndure(u8 bankDef, s32 damage);
bool8 HackAffection_TryCureStatus(u8 bank);
bool8 HackAffection_TryEvade(u8 bankDef);
u8 HackAffection_CritBonus(u8 bankAtk);
bool8 HackAffection_BeforeDamageCalc(void);
s32 HackAffection_ModifyDamage(s32 damage);
bool8 HackAffection_AfterDamageCalc(void);
bool8 HackAffection_TryShakeOff(u8 bankAtk);
bool8 HackAffection_TryBlockStatDrop(u8 bank);
void HackAffection_BattleEnd(void);

extern const u8 BattleScript_AffectionEndured[];
extern const u8 BattleScript_AffectionCured[];
extern const u8 BattleScript_AffectionLastMon[];
extern const u8 BattleScript_AffectionBond[];
extern const u8 BattleScript_AffectionCover[];
extern const u8 BattleScript_AffectionWokeUp[];
extern const u8 BattleScript_AffectionUnfroze[];
extern const u8 BattleScript_AffectionShookParalysis[];
extern const u8 BattleScript_AffectionStatBlock[];
extern const u8 gText_Affection_Evaded[];
