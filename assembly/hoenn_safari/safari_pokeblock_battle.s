@ ホウエンのサファリの「ポロックをなげる」の戦闘の台本（FEAT-pokeblock 段 6・patches/0978）。
@ 原文: 参考/pokeemerald/data/battle_scripts_2.s の BattleScript_ActionThrowPokeblock（printstring THREWPOKEBLOCKATPKMN → waitmessage → playanimation POKEBLOCK_THROW →
@ printfromtable gSafariPokeblockResultStringIds → waitmessage → end2）。FR の戦闘文字の表には「ポロック」の文が無いので、台本の中で文を選ぶ（gBattleCommunication[5]＝0 気になる・1 夢中・2 無視）。
@ アニメ: HACK_PKBL_THROW_ANIM＝gBattleAnims_General の最後の台本 HACK_ANIM_POKEBLOCK_THROW（assembly/data/special_battle_anim_table.s・原文 B_ANIM_POKEBLOCK_THROW・段 6b）。番号はこの .equ と src/hack_safari_pokeblock.h の 2 か所。

.text
.thumb
.align 2

.include "../asm_defines.s"
.include "../battle_script_macros.s"

.equ PKBL_THROW_ANIM, 0x53              @ HACK_PKBL_THROW_ANIM（gBattleAnims_General の ANIM_AI_ITEM_HEAL（0x52）の次）

.equ B_WAIT_TIME_LONG, 0x40

.global HoennSafari_BattleScript_ThrowPokeblock
HoennSafari_BattleScript_ThrowPokeblock:
	setword BATTLE_STRING_LOADER gText_HoennSafari_ThrewPokeblock
	printstring 0x184
	waitmessage B_WAIT_TIME_LONG
	playanimation BANK_ATTACKER, PKBL_THROW_ANIM, 0x0
	jumpifbyte EQUALS, MULTISTRING_CHOOSER, 0x0, HoennSafari_BattleScript_Curious
	jumpifbyte EQUALS, MULTISTRING_CHOOSER, 0x1, HoennSafari_BattleScript_Enthralled
	setword BATTLE_STRING_LOADER gText_HoennSafari_Ignored
	goto HoennSafari_BattleScript_Reaction

HoennSafari_BattleScript_Curious:
	setword BATTLE_STRING_LOADER gText_HoennSafari_Curious
	goto HoennSafari_BattleScript_Reaction

HoennSafari_BattleScript_Enthralled:
	setword BATTLE_STRING_LOADER gText_HoennSafari_Enthralled

HoennSafari_BattleScript_Reaction:
	printstring 0x184
	waitmessage B_WAIT_TIME_LONG
	end2
