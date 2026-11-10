@ ホウエンのサファリゾーンのポロックおき場の台本（FEAT-pokeblock 段 6・patches/0978）。
@ 原文: 参考/pokeemerald/data/scripts/safari_zone.inc の EventScript_PokeBlockFeeder（42〜77 行）。床の印 HOENN_MB_POKEBLOCK_FEEDER（0xEE）を A で調べると
@ src/overworld.c の GetInteractedMetatileScript がここへ来る。文は strings/hack_safari_pokeblock.string（日本語版エメラルド ROM の原文）。
@ 差: 原文の cancel 側は #ifdef BUGFIX の releaseall（EventScript_PokeBlockFeeder は lockall なのに BUGFIX 無しだと release しない）を入れた（直す側）。
@ 原文は special OpenPokeblockCaseOnFeeder の後に waitstate が無い（CB2_ReturnToField で戻る）が、FR は CB2_ReturnToFieldContinueScript＋waitstate で台本を続ける。

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ MSGBOX_DEFAULT, 4         @ 素の標準スクリプト 4（message／waitmessage／waitbuttonpress／closemessage 相当）
.equ MSGBOX_YESNO, 5           @ 素の標準スクリプト 5（はい＝LASTRESULT 1）

.global HoennSafari_EventScript_PokeblockFeeder
HoennSafari_EventScript_PokeblockFeeder:
	lockall
	callnative HackSafari_Native_FeederInFront
	compare LASTRESULT, 0xFFFF
	if notequal _goto HoennSafari_EventScript_PokeblockPresent
	loadpointer 0x0, gText_HoennSafari_PlacePokeblock
	callstd MSGBOX_YESNO
	compare LASTRESULT, 1
	if equal _goto HoennSafari_EventScript_ChoosePokeblock
	releaseall
	end

HoennSafari_EventScript_ChoosePokeblock:
	fadescreen FADEOUT_BLACK
	callnative HackSafari_Native_OpenCaseOnFeeder
	waitstate
	compare LASTRESULT, 0xFFFF
	if notequal _goto HoennSafari_EventScript_PokeblockPlaced
	releaseall
	end

HoennSafari_EventScript_PokeblockPlaced:
	loadpointer 0x0, gText_HoennSafari_WasPlaced
	callstd MSGBOX_DEFAULT
	releaseall
	end

HoennSafari_EventScript_PokeblockPresent:
	loadpointer 0x0, gText_HoennSafari_StillHere
	callstd MSGBOX_DEFAULT
	releaseall
	end
