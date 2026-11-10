@ はじめの話の主人公選択のメイ対応（FEAT-player-mei）。素（日本語版）の番地は hooks に書いてある。
@ 4 の倍数の番地から 8 バイト潰す hooks。潰した範囲の途中へ飛んでくる分岐が無いことは tools/rom/verify_player_mei.py が素の ROM の分岐を全部数えて確かめる。

.text
.thumb
.align 2

HackOak_CallR0:
	bx r0

HackOak_CallR1:
	bx r1

@ LoadTrainerPic の女の枝 08131A44（ldr pal / mov / mov / bl LoadPalette / ldr tiles / b 08131A68）の代わり。r4 は触らない
.global HackOakFemalePicHook
.thumb_func
HackOakFemalePicHook:
	ldr r0, =HackOak_LoadFemalePicPal
	bl HackOak_CallR0
	ldr r1, =0x08131A68 | 1
	bx r1

@ PrintNameChoiceOptions の 08131F10（4 件を並べるループの中・ループの先頭は 08131F0E なので F10 から潰す）。
@ r6 = 素の表（男／女／ライバル）。メイのときだけ 女の表・ライバルの表をメイ用に差し替える（何度通っても同じ結果）。
@ 潰す 08131F10〜17: lsl r1,r4,#2 / add r1,r1,r6 / ldr r2,[r1] / lsl r1,r4,#28 → 同じことをして 08131F18 へ
.global HackOakNameLoopHook
.thumb_func
HackOakNameLoopHook:
	add r0, r6, #0
	ldr r1, =HackOak_FixNameTable
	bl HackOak_CallR1
	add r6, r0, #0
	ldrb r0, [r7, #26]
	lsl r1, r4, #2
	add r1, r1, r6
	ldr r2, [r1, #0]
	lsl r1, r4, #28
	ldr r3, =0x08131F18 | 1
	bx r3

@ GetDefaultName の主人公側 08131F84（r4 = 表のあと 08131F9E の Random へ）
.global HackOakDefaultPlayerNameHook
.thumb_func
HackOakDefaultPlayerNameHook:
	ldr r0, =HackOak_PlayerNameTable
	bl HackOak_CallR0
	add r4, r0, #0
	ldr r1, =0x08131F9E | 1
	bx r1

@ GetDefaultName のライバル側 08131FE0（r2 = 選んだ番号）。r3 = 表[r2] にして 08131FE8 へ
.global HackOakDefaultRivalNameHook
.thumb_func
HackOakDefaultRivalNameHook:
	add r4, r2, #0
	ldr r0, =HackOak_RivalNameTable
	bl HackOak_CallR0
	lsl r1, r4, #2
	add r0, r0, r1
	ldr r3, [r0, #0]
	ldr r1, =0x08131FE8 | 1
	bx r1
.pool
