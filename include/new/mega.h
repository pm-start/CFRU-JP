#pragma once

#include "../global.h"
#include "../pokemon.h"

/**
 * \file mega.h
 * \brief Contains various functions relating to Mega Evolution, as well
 *		  as sprite callbacks for Mega indicators and triggers.
 */

//Exported Functions
const struct Evolution* CanMegaEvolve(u8 bank, bool8 CheckUBInstead);
species_t GetMegaSpecies(u16 species, u16 item, const u16* moves);
ability_t GetBankMegaFormAbility(u8 megaBank, u8 foe);
const u8* DoMegaEvolution(u8 bank);
const u8* DoPrimalReversion(u8 bank, u8 caseId);
void MegaRevert(pokemon_t* mon);
void TryRevertMega(pokemon_t* mon);
bool8 MegaEvolutionEnabled(u8 bank);
bool8 BankMegaEvolved(u8 bank, bool8 checkUB);
bool8 IsMega(u8 bank);
bool8 IsBluePrimal(u8 bank);
bool8 IsRedPrimal(u8 bank);
bool8 IsMegaSpecies(u16 species);
const u8* HackGibleSwitch_Do(u8 bank); //[hack] patches/0791 src/hack_gible_switch.c
void HackGibleSwitch_NoteMega(u8 bank);
void HackGibleSwitch_PrepareSecond(u8 bank, u16 move); //[hack] patches/0793
bool8 IsBluePrimalSpecies(u16 species);
bool8 IsRedPrimalSpecies(u16 species);
bool8 IsUltraNecrozma(u8 bank);
bool8 IsUltraNecrozmaSpecies(u16 species);
const u8* GetTrainerName(u8 bank);
u8* TryGetRivalNameByTrainerClass(u8 class);
u8* TryGetRivalNameByTrainerId(u16 trainerId); //[hack] patches/0386

//[hack] patches/0386 素の FR（BPRJ）のトレーナーの数（0〜742・tools/data_editor/trainerlib.py の TRAINER_COUNT）。
//OVERWRITE_RIVAL はクラス 0x51・0x59・0x5A の名前をライバルの名前に替えるが、あとから足したチャンピオン（ワタル 916・シロナ 926）も
//クラス 0x5A なので「グリーン」と出ていた。ライバルの名前に替えるのは、この数より前（素のライバル 326〜440・739〜741）だけにする
#define HACK_FR_TRAINER_COUNT 743

//Functions Hooked In
bool8 HasMegaSymbol(u8 bank);
void MegaRetrieveData(void);

//Exported Constants
#define MEGA_EVOLUTION 0xFE // The evolutionary type for Megas

enum MegaEvoVariants
{
	MEGA_VARIANT_STANDARD,
	MEGA_VARIANT_PRIMAL,
	MEGA_VARIANT_WISH, //Rayquaza
	MEGA_VARIANT_ULTRA_BURST, //Necrozma
};
