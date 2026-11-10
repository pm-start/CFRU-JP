@ ボックスの仮想化 M2（src/hack_box_shelf.c・2026-10-06）: 1 回のレポートで書けるページの数を超える操作を、ボックスの画面で止める。
@
@ 1. Task_PokeStorageMain（0808CCF8）の state 0 の HandleInput（080942E8）の呼び出し 0808CD48 に hooks（r0）。
@    潰す 3 命令（bl HandleInput / lsls / lsrs）の代わりに BoxShelf_PssHandleInput を呼び、0808CD50（subs r0,#1・入力で分ける）へ戻る。
@ 2. Task_DepositMenu（0808D7C4）の TryStorePartyMonInBox（08092B18）の呼び出し 0808D858 に hooks（r0）。
@    潰す 3 命令（mov r0,r4 / bl TryStorePartyMonInBox / lsls r0,#24）の代わりに BoxShelf_DepositTry を呼ぶ。
@    0 / 1 は素と同じ（lsls をここで行い 0808D860 の cmp へ）・2 は止めた（文は C が出した・state も C が 4 に）→ 0808D8F6（関数の終わり）へ。

.thumb
.text
.align 2

.global BoxShelf_PssInputHook
BoxShelf_PssInputHook:
	bl BoxShelf_PssHandleInput
	lsl r0, r0, #24
	lsr r0, r0, #24
	ldr r1, =0x0808CD50 | 1
	bx r1

.global BoxShelf_DepositHook
BoxShelf_DepositHook:
	mov r0, r4
	bl BoxShelf_DepositTry
	cmp r0, #2
	beq BoxShelf_DepositBlocked
	lsl r0, r0, #24
	ldr r1, =0x0808D860 | 1
	bx r1

BoxShelf_DepositBlocked:
	ldr r1, =0x0808D8F6 | 1
	bx r1

@ 3. [patches/0981] Task_WithdrawMon（0808D700 付近）の state 0 の CalculatePlayerPartyCount（0808D710）の呼びに hooks（r0）。
@    潰す 3 命令（bl CalculatePlayerPartyCount / lsls r0,#24 / lsrs r0,#24）の代わりに BoxShelf_WithdrawCount を呼ぶ。
@    0〜6 は手持ちの数＝素と同じ（lsls/lsrs をここで行い 0808D718 の cmp r0,#6 へ）・0xFF は「ヌシは手持ちに 1 匹だけ」で止めた（文と state 1 は C が出した）→ 0808D7BA（関数の終わりの pop {r0} / bx r0）へ。
.global BoxShelf_WithdrawHook
BoxShelf_WithdrawHook:
	bl BoxShelf_WithdrawCount
	cmp r0, #0xFF
	beq BoxShelf_WithdrawBlocked
	lsl r0, r0, #24
	lsr r0, r0, #24
	ldr r1, =0x0808D718 | 1
	bx r1

BoxShelf_WithdrawBlocked:
	ldr r1, =0x0808D7BA | 1
	bx r1

.pool

@ ===== FEAT-box100-unlock（0.61.0・ユーザー決定 2026-10-27）: 1〜50 箱目が満杯になるまで 51〜100 箱目を出さない =====
@ 素の箱の画面は箱の数を命令の即値（cmp rN,#99 / #100）に持つ（bytereplacement の「##Expanded Boxes##」33 か所）。
@ そのうち「次の箱・前の箱・箱を選ぶ・次の箱を探す・中身を探す」の折り返し 6 か所だけを、見える箱の数 BoxShelf_VisibleBoxes()（50 か 100）にする。
@ 残りの 27 か所は「手持ち＝箱 id 100」の印・全箱を回す処理・箱番号の妥当性の確かめなので 100 のまま（docs/claude/06_risks.md 6.118）。
@ どの hook も、置き換えた命令と同じ働きを書き、元の続きへ戻る。C の呼びで r0〜r3・lr が潰れるので、生きている物は push で守る（関数の頭で lr は push 済み）。

.global BoxShelf_VisNextHook   @ 0808C660（箱の次へ: lsrs / cmp #99 / bls / ldr r0,[r2] の 8 バイト）
BoxShelf_VisNextHook:
	lsr r0, r0, #24
	push {r0, r2}
	bl BoxShelf_VisibleBoxes
	mov r1, r0
	pop {r0, r2}
	sub r1, #1
	cmp r0, r1
	bls VisNext_ok
	ldr r0, [r2]
	ldr r3, =0x0808C668 | 1
	bx r3
VisNext_ok:
	ldr r3, =0x0808C672 | 1
	bx r3
.pool

.global BoxShelf_VisPrevHook   @ 0808C68C（箱の前へ: ldrb / cmp #0 / beq / subs r1,r0,#1 の 8 バイト・0 のとき 99 → 見える数 - 1）
BoxShelf_VisPrevHook:
	ldrb r0, [r0]
	cmp r0, #0
	bne VisPrev_nz
	push {r2}
	bl BoxShelf_VisibleBoxes
	pop {r2}
	sub r1, r0, #1
	b VisPrev_ret
VisPrev_nz:
	sub r1, r0, #1
VisPrev_ret:
	ldr r3, =0x0808C69E | 1
	bx r3
.pool

.global BoxShelf_VisChooseNextHook   @ 0808CEB0（箱を選ぶ画面の次: strh / cmp #99 / ble / movs r0,#0 の 8 バイト）
BoxShelf_VisChooseNextHook:
	strh r0, [r1]
	push {r0, r1, r2}
	bl BoxShelf_VisibleBoxes
	mov r3, r0
	pop {r0, r1, r2}
	sub r3, #1
	cmp r0, r3
	ble VisChN_ok
	mov r0, #0
	ldr r3, =0x0808CEB8 | 1
	bx r3
VisChN_ok:
	ldr r3, =0x0808CEBA | 1
	bx r3
.pool

.global BoxShelf_VisChoosePrevHook   @ 0808CEEC（箱を選ぶ画面の前: bge / movs r0,#99 / strh / ldr r1,[r4] の 8 バイト）
BoxShelf_VisChoosePrevHook:
	cmp r0, #0
	bge VisChP_pos
	push {r1, r2}
	bl BoxShelf_VisibleBoxes
	sub r0, #1
	pop {r1, r2}
	strh r0, [r1]
VisChP_pos:
	ldr r1, [r4]
	ldr r3, =0x0808CEF4 | 1
	bx r3
.pool

.global BoxShelf_VisSearchHook   @ 080CD9D8（次の箱を探す: cmp #100 / bne / movs r4,#0 と bl StorageGetCurrentBox の前半の 8 バイト）
BoxShelf_VisSearchHook:
	mov r0, r4
	bl BoxShelf_WrapNextBox
	mov r4, r0
	ldr r0, =0x080CD9E2 | 1
	mov lr, r0
	ldr r0, =0x0808B490 | 1
	bx r0
.pool

.global BoxShelf_VisScanHook   @ 08093004（中身を探す: asrs / cmp #99 / bgt / adds r0,r6,#1 の 8 バイト・r12 と r3 は生きている）
BoxShelf_VisScanHook:
	asr r0, r0, #24
	mov r2, ip
	push {r0, r2, r3}
	bl BoxShelf_VisibleBoxes
	mov r1, r0
	pop {r0, r2, r3}
	mov ip, r2
	sub r1, #1
	cmp r0, r1
	bgt VisScan_out
	add r0, r6, #1
	ldr r1, =0x0809300C | 1
	bx r1
VisScan_out:
	ldr r1, =0x08092F54 | 1
	bx r1
.pool
