@ ヌシにしてくれる人（2026-11-09・FEAT-nushi-npc・patches/0982・BattleFrontier_PokemonCenter_1F (11,4)・build_hoenn.EXTRA_NPCS）。
@ ユーザー「1度だけヌシポケモンにしてくれるNPCの追加／条件lv200」。
@ 手持ちを選ぶ（special 0x9F）→ src/hack_nushi_npc.c の Check で断る子を分ける → 確認 → Apply（印・能力の計算し直し・旗 0x13B8）。
@ 文は strings/hack_nushi_npc.string。手本は assembly/hack_tera_changer/

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ SP_CHOOSE_PARTY_MON, 0x9F
.equ HACK_FLAG_NUSHI_NPC, 0x13B8     @ build_hoenn_scripts.RESERVED_FLAGS

.align 2
.global HackNushiNpcScr_Talk
HackNushiNpcScr_Talk:
	lock
	faceplayer
	checkflag HACK_FLAG_NUSHI_NPC
	if 0x1 _goto HackNushiNpcScr_After
	msgbox gText_NushiNpc_Intro MSG_YESNO
	compare LASTRESULT 1
	if 0x5 _goto HackNushiNpcScr_Bye
HackNushiNpcScr_Choose:
	msgbox gText_NushiNpc_WhichMon MSG_KEEPOPEN
	closemessage
	special SP_CHOOSE_PARTY_MON
	waitstate
	callasm HackPartyChoice_RestoreSlot
	compare 0x8004 6
	if 0x4 _goto HackNushiNpcScr_Bye
	bufferpartypokemon 0x0 0x8004
	callasm HackNushiNpc_Check
	compare LASTRESULT 1
	if 0x1 _goto HackNushiNpcScr_Egg
	compare LASTRESULT 2
	if 0x1 _goto HackNushiNpcScr_LowLevel
	compare LASTRESULT 3
	if 0x1 _goto HackNushiNpcScr_Already
	compare LASTRESULT 4
	if 0x1 _goto HackNushiNpcScr_Dark
	compare LASTRESULT 5
	if 0x1 _goto HackNushiNpcScr_Delta
	compare LASTRESULT 6
	if 0x1 _goto HackNushiNpcScr_Other
	compare LASTRESULT 0
	if 0x5 _goto HackNushiNpcScr_Bye
	msgbox gText_NushiNpc_Confirm MSG_YESNO
	compare LASTRESULT 1
	if 0x5 _goto HackNushiNpcScr_Bye
	callasm HackNushiNpc_Apply
	bufferpartypokemon 0x0 0x8004
	fanfare 0x101
	msgbox gText_NushiNpc_Done MSG_KEEPOPEN
	waitfanfare
	closemessage
	release
	end

HackNushiNpcScr_Egg:
	msgbox gText_NushiNpc_Egg MSG_KEEPOPEN
	goto HackNushiNpcScr_Choose

HackNushiNpcScr_LowLevel:
	msgbox gText_NushiNpc_LowLevel MSG_KEEPOPEN
	goto HackNushiNpcScr_Choose

HackNushiNpcScr_Already:
	msgbox gText_NushiNpc_Already MSG_KEEPOPEN
	goto HackNushiNpcScr_Choose

HackNushiNpcScr_Dark:
	msgbox gText_NushiNpc_Dark MSG_KEEPOPEN
	goto HackNushiNpcScr_Choose

HackNushiNpcScr_Delta:
	msgbox gText_NushiNpc_Delta MSG_KEEPOPEN
	goto HackNushiNpcScr_Choose

HackNushiNpcScr_Other:
	msgbox gText_NushiNpc_Other MSG_KEEPOPEN
	closemessage
	release
	end

HackNushiNpcScr_After:
	msgbox gText_NushiNpc_After MSG_KEEPOPEN
	closemessage
	release
	end

HackNushiNpcScr_Bye:
	msgbox gText_NushiNpc_Bye MSG_KEEPOPEN
	closemessage
	release
	end
