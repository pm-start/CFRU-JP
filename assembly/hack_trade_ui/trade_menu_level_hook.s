@ 通信交換の手持ち一覧（素の trade.c・sTradeMenuData 02031CDC）のレベルの絵: 101 以上を 3 けたで出す。hooks 0804E976（r3）。
@ 素の 0804E8E0（1 匹ぶんの枠とレベルを描く）は level/10 + 0x60 と level%10 + 0x70 の 2 タイルだけ書く。
@ 0x60 + n は n = 0〜10 の「十の位」の絵（10 番目は「10」を 1 タイルに詰めた絵）で、11 以上（Lv110 から）は絵の無いタイルになり
@ 0x70 番台（一の位）とぶつかって別の数字に見えた。Lv101〜109 は偶然合っていたが、見え方をそろえるため 101 以上は全部 3 タイルにする。
@ 潰す所: 0804E976〜0804E97F（add r0,r6,#0 / movs r1,#10 / bl udiv / lsls r0,#24 の 10 バイト）。
@ 入ったときの値（素のコードのまま）: r4 = sTradeMenuData / r6 = レベル / r9 = x（枠の中の列）/ [sp,#12] = y（行）
@   タイルの並びは sTradeMenuData + 0x8F0 から（1 マス 2 バイト・x + y*32）。素の出口（0804EA0A の add sp,#16 以降）へ戻る。
@ 100 以下は素のまま（十の位 0x60+n・一の位 0x70+n の 2 タイル。Lv100 は「10」の絵 + 0）。
@ 101 以上は 0x70 番台（左に寄った数字の絵）を 3 つ: x-1 に百の位・x に十の位・x+1 に一の位。x-1 は枠の中の段の無地のタイル。
.text
.thumb
.align 2
.global TradeMenuLevelHook
TradeMenuLevelHook:
	ldr r0, [sp, #12]
	lsl r2, r0, #5
	add r2, r9
	lsl r2, r2, #1
	mov r3, #0x8F
	lsl r3, r3, #4          @ 0x8F0（ldr r3, =0x8F0 は as が Thumb-2 の mov.w にする・CLAUDE.md 6.63）
	add r3, r4
	add r3, r2
	mov r0, r6
	mov r1, #205
	mul r1, r0
	lsr r1, r1, #11
	mov r2, #10
	mul r2, r1
	sub r2, r0, r2
	cmp r0, #100
	bhi three_digits
	cmp r1, #0
	beq ones_only
	add r1, #0x60
	strh r1, [r3]
ones_only:
	add r2, #0x70
	strh r2, [r3, #2]
	b exit
three_digits:
	mov r5, #205
	mul r5, r1
	lsr r5, r5, #11
	mov r0, #10
	mul r0, r5
	sub r1, r1, r0
	sub r3, #2
	add r5, #0x70
	strh r5, [r3]
	add r1, #0x70
	strh r1, [r3, #2]
	add r2, #0x70
	strh r2, [r3, #4]
exit:
	ldr r0, =0x0804EA0A | 1
	bx r0
.pool
