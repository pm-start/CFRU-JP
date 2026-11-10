@ 伝説レイド（2026-10-06）: バトルフロンティアの広場（BattleFrontier_OutsideWest (23,47)）の人。
@ ユーザー「バトルフロンティアに話しかけるたびに異なる伝説とレイドバトルが始まるところ作ってほしい」。
@ 流れは CFRU の RaidScript_Start（assembly/overworld_scripts/system_scripts.s）と同じ。相手・星・助っ人は src/hack_legend_raid.c。
@ 文は strings/hack_legend_raid.string

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ SP_START_RAID_INTRO, 0x116
.equ SP_CREATE_RAID_MON, 0x117
.equ SP_START_RAID_BATTLE, 0x118
.equ FLAG_TAG_BATTLE, 0x908

.align 2
.global HackLegendRaidScr_Talk
HackLegendRaidScr_Talk:
	lock
	faceplayer
	msgbox gText_LegendRaid_Intro MSG_YESNO
	compare LASTRESULT 0x0
	if equal _goto HackLegendRaidScr_Decline
	callasm HackLegendRaid_Begin
	fadescreen FADEOUT_BLACK
	special SP_START_RAID_INTRO
	waitstate
	compare LASTRESULT 0x0
	if equal _goto HackLegendRaidScr_Cancel
	setflag FLAG_TAG_BATTLE
	special SP_CREATE_RAID_MON
	callasm HackLegendRaid_StrengthenBoss
	special SP_START_RAID_BATTLE
	waitstate
	callasm HackLegendRaid_End
	special2 LASTRESULT 0xB4
	compare LASTRESULT 0x1
	if equal _goto HackLegendRaidScr_Reward
	compare LASTRESULT 0x7
	if equal _goto HackLegendRaidScr_Reward
HackLegendRaidScr_After:
	msgbox gText_LegendRaid_After MSG_KEEPOPEN
	closemessage
	release
	end

@ 報酬（src/hack_raid_rewards.c・FEAT-raid-rewards）: 勝ち(1)・捕獲(7)のときだけ。1 つずつ「〇〇を てにいれた！」。バッグに入らなければパソコンのどうぐへ
HackLegendRaidScr_Reward:
	setvar 0x4000 0x0
HackLegendRaidScr_RewardNext:
	callasm HackRaidRewards_Next
	compare LASTRESULT 0x0
	if equal _goto HackLegendRaidScr_After
	copyvar 0x8004 0x8000
	copyvar 0x8005 0x8001
	callstd MSG_OBTAIN
	compare LASTRESULT 0x0
	if notequal _goto HackLegendRaidScr_RewardNext
	callasm HackRaidRewards_ToPc
	bufferitem 0x0 0x8004
	compare LASTRESULT 0x0
	if equal _goto HackLegendRaidScr_RewardLost
	msgbox gText_LegendRaid_RewardToPc MSG_KEEPOPEN
	goto HackLegendRaidScr_RewardNext
HackLegendRaidScr_RewardLost:
	msgbox gText_LegendRaid_RewardLost MSG_KEEPOPEN
	goto HackLegendRaidScr_RewardNext

HackLegendRaidScr_Cancel:
	callasm HackLegendRaid_End
HackLegendRaidScr_Decline:
	msgbox gText_LegendRaid_Decline MSG_KEEPOPEN
	closemessage
	release
	end
