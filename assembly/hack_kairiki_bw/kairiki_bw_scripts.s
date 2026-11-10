@ かいりきの BW 化（#107・pm-start/kairiki-bw・liru さん・2026-10-12）の地図の側。
@ 仕組み（素の 2 か所の差し替え）は tools/kairiki_bw/apply_kairiki_bw.py。岩を挙動 0x1F（MB_BW_STRENGTH_HOLE）のマスへ押すと、
@ 岩が消えて岩の旗が立ち、そのマスの床のイベントの台本が走る＝ここで穴を「通れる床」に描き替える。
@ 地図に入るたびは ON_LOAD で旗を見て同じ描き替え（作者の使い方の「マップスクリプト種別 0x01」）。
@ 人・床のイベント・マップスクリプトは tools/rom/build_hoenn.py の EXTRA_NPCS／EXTRA_COORDS／EXTRA_MAPSCRIPTS（'MagmaHideout_2F_3R'＝
@ マグマ団のアジト 2F の 3 の部屋・エメラルドの人の無い地図。ほのおのぬけみちと同じタイルセット）。
@ 穴のマスは HOENN_CELL_EDITS・穴のメタタイルは EXTRA_SECONDARY_METATILES。
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"

.equ FLAG_KAIRIKI_FIERY, 0xF17          @ 岩の旗（5e が切った・広げるときは 0xF18〜）
.equ MT_FIERY_FLOOR, 850                 @ このタイルセットの床（通れる）
.equ SP_DRAW_WHOLE_MAP_VIEW, 0x8E        @ 日本語版の special の表 08163068 の 0x8E（CLAUDE 6.87）

@ ---- マグマ団のアジト 2F の 3 の部屋 (14, 13) の穴 ----
.global HackScr_KairikiBw_Magma3RHole
HackScr_KairikiBw_Magma3RHole:
	lockall
	call HackScr_KairikiBw_Magma3RFill
	special SP_DRAW_WHOLE_MAP_VIEW
	releaseall
	end

HackScr_KairikiBw_Magma3RFill:
	setmetatile 14 13 MT_FIERY_FLOOR 0
	return

.align 2
.global HackScr_KairikiBw_Magma3R_MapScripts
HackScr_KairikiBw_Magma3R_MapScripts:
	.byte 1
	.word HackScr_KairikiBw_Magma3R_OnLoad
	.byte 0

.align 2
HackScr_KairikiBw_Magma3R_OnLoad:
	checkflag FLAG_KAIRIKI_FIERY
	if 0x1 _call HackScr_KairikiBw_Magma3RFill
	end

@ ---- ほのおのぬけみち（FieryPath・人を置く地図）の穴 4 つ（2026-10-12・ユーザー承認の見本）----
@ 穴のマスは build_hoenn.HOENN_CELL_EDITS・床のイベントは PEOPLE_COORDS・岩の旗は PEOPLE_OBJECT_FLAGS（0xF1A〜0xF1D）
.global HackScr_KairikiBw_FieryHole1
HackScr_KairikiBw_FieryHole1:
	lockall
	setmetatile 16 15 MT_FIERY_FLOOR 0
	special SP_DRAW_WHOLE_MAP_VIEW
	releaseall
	end

.global HackScr_KairikiBw_FieryHole2
HackScr_KairikiBw_FieryHole2:
	lockall
	setmetatile 7 11 MT_FIERY_FLOOR 0
	special SP_DRAW_WHOLE_MAP_VIEW
	releaseall
	end

.global HackScr_KairikiBw_FieryHole3
HackScr_KairikiBw_FieryHole3:
	lockall
	setmetatile 3 13 MT_FIERY_FLOOR 0
	special SP_DRAW_WHOLE_MAP_VIEW
	releaseall
	end

.global HackScr_KairikiBw_FieryHole4
HackScr_KairikiBw_FieryHole4:
	lockall
	setmetatile 5 25 MT_FIERY_FLOOR 0
	special SP_DRAW_WHOLE_MAP_VIEW
	releaseall
	end

.align 2
.global HackScr_KairikiBw_Fiery_MapScripts
HackScr_KairikiBw_Fiery_MapScripts:
	.byte 1
	.word HackScr_KairikiBw_Fiery_OnLoad
	.byte 0

.align 2
HackScr_KairikiBw_Fiery_OnLoad:
	checkflag 0xF1A
	if 0x1 _call HackScr_KairikiBw_FieryFill1
	checkflag 0xF1B
	if 0x1 _call HackScr_KairikiBw_FieryFill2
	checkflag 0xF1C
	if 0x1 _call HackScr_KairikiBw_FieryFill3
	checkflag 0xF1D
	if 0x1 _call HackScr_KairikiBw_FieryFill4
	end

HackScr_KairikiBw_FieryFill1:
	setmetatile 16 15 MT_FIERY_FLOOR 0
	return

HackScr_KairikiBw_FieryFill2:
	setmetatile 7 11 MT_FIERY_FLOOR 0
	return

HackScr_KairikiBw_FieryFill3:
	setmetatile 3 13 MT_FIERY_FLOOR 0
	return

HackScr_KairikiBw_FieryFill4:
	setmetatile 5 25 MT_FIERY_FLOOR 0
	return
