@ 技の威力の表示の hooks（#126・patches/0582・src/hack_move_power.c）
@ 素の ROM の 3 つの画面が 1 バイト目の power を `ldrb r1,[row,#1]` → ConvertIntToDecimalStringN(dest, power, mode, 3) で書いていた所を、
@ HackPrintMovePower(dest, row, mode)（bigPower を見る）に差し替える。呼んだあとは素の BL の次の命令へ戻る。
@ r3 は hooks の飛び先の読み込みに使われた・呼ばれる側の r0〜r3 は素の BL でも壊れる（同じ）・r4〜r7 は C が保存する。
	.syntax unified
	.text
	.thumb
	.align	2

@ わざマシンケース（hooks: 08133CCE 3）。素: ldrb r1,[r5,#1] / adds r0,r4,#0 / movs r2,#1 / movs r3,#3 / bl 080089F4 → 08133CDA
	.global	HackTmCasePowerHook
	.thumb_func
HackTmCasePowerHook:
	adds	r0, r4, #0
	adds	r1, r5, #0
	movs	r2, #1
	bl	HackPrintMovePower
	ldr	r3, =0x08133CDA + 1
	bx	r3
	.align	2
	.pool

@ 素の要約画面（hooks: 08137258 2）。素: ldrb r1,[r1,#1] / movs r2,#0 / movs r3,#3 / bl 080089F4 → 08137262（r0 = 書く先・r1 = 技の行）
	.global	HackSummaryPowerHook
	.thumb_func
HackSummaryPowerHook:
	movs	r2, #0
	bl	HackPrintMovePower
	ldr	r3, =0x08137262 + 1
	bx	r3
	.align	2
	.pool

@ 素のわざ画面（hooks: 080E6338 3）。素: ldrb r1,[r4,#1] / add r0,sp,#8 / movs r2,#0 / movs r3,#3 / bl 080089F4 → 080E6344
	.global	HackMoveScreenPowerHook
	.thumb_func
HackMoveScreenPowerHook:
	add	r0, sp, #8
	adds	r1, r4, #0
	movs	r2, #0
	bl	HackPrintMovePower
	ldr	r3, =0x080E6344 + 1
	bx	r3
	.align	2
	.pool
