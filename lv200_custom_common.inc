@ 新規技(Lv200)の共通マクロ（P3）。A/B の台本 .s から .include する（ラベルを作るのは各 .s の中だけ）。
@ 台本の中で自分の能力を上げる部品（アニメ → 1 能力ずつ statbuffchange → 文）。
@   LV200_SELF_STAT_ANIM  animmask : 複数の能力をまとめた上昇アニメを 1 回（素の Ancient Power などと同じ）
@   LV200_SELF_STAT_STEP  stat amount quiet : 1 能力。あまのじゃく・たんじゅん・+6 は statbuffchange が素の流れで判断。
@                          quiet=1 は「これ以上上がらない」文を出さない（複数能力の技）、0 は単体の技として出す。
.macro LV200_SELF_STAT_ANIM animmask
	setbyte STAT_ANIM_PLAYED 0x0
	playstatchangeanimation BANK_ATTACKER, \animmask, STAT_ANIM_UP | STAT_ANIM_IGNORE_ABILITIES
.endm

.macro LV200_SELF_STAT_STEP stat, amount, quiet
	setstatchanger \stat | \amount
	statbuffchange STAT_ATTACKER | STAT_BS_PTR | STAT_CERTAIN Lv200Common_Macro_Next_\@
	.if \quiet
	jumpifbyte EQUALS MULTISTRING_CHOOSER 0x2 Lv200Common_Macro_Next_\@
	.endif
	printfromtable gStatUpStringIds
	waitmessage DELAY_1SECOND
Lv200Common_Macro_Next_\@:
.endm
