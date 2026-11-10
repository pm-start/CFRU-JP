@ 卵専用ボックスの台本（FEAT-save16-B4・Redmine #38）。歩いているとき、箱の中の卵が孵ったら 1 回だけ出す（src/hack_egg_box.c の EggBox_OnStep → src/overworld.c）。
@ 文は strings/hack_egg_box.string。
.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.align 2
.global EggBoxScript_Hatched
EggBoxScript_Hatched:
	lock
	msgbox gText_EggBox_Hatched MSG_NORMAL
	release
	end
