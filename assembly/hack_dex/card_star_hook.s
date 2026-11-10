@ トレーナーカードの色を星の数で選ぶ表引きの「表の外を読まない」直し（2026-10-18・patches/0604・src/hack_dex_counts.c）。
@ 素の表（0x083955F4 ホウエン様式・0x08395608 カントー様式）は 0〜4 個の 5 色だけ。星が 5 個以上になると表の外の語をポインタとして読んで固まるので、4 までに頭打ちする。
@
@ フック先 0808A864（素のカードの色の読み込み・RSE／カントー様式のどちらもここへ合流する）
@   0808A864  ADD  r0, r2, r3        ← r2 = 構造体の先頭・r3 = 星の欄のずれ（0x449）
@   0808A866  LDRB r0, [r0]          ← 星の数
@   0808A868  LSL  r0, r0, #2
@   0808A86A  ADD  r0, r0, r1        ← r1 = 表
@   0808A86C  LDR  r0, [r0]          ← ここから先は素のまま（r3 はこのあと使わない）
@ ★ 潰す 8 バイトは上の 4 命令。やり直して 0808A86C へ戻る。r0 は 0808A864 の前では空き（hooks の 3 列目 = 0）。
@ ★ 戻るレジスタには r3 を使う（0808A86C 以降で r3 を読まない）。
	.text
	.thumb
	.align	2
	.arch	armv4t

	.global	HackCardPalHook
	.thumb_func
HackCardPalHook:
	add	r0, r2, r3
	ldrb	r0, [r0, #0]
	cmp	r0, #4
	bls	HackCardPalHook_ok
	mov	r0, #4
HackCardPalHook_ok:
	lsl	r0, r0, #2
	add	r0, r0, r1
	ldr	r3, =0x0808A86C + 1
	bx	r3

	.pool

@ 自分のトレーナーカードの星（2026-10-18・patches/0604 の直し）。
@ 素の 0808B1AC（InitTrainerCard の中）: BL 08089498 / LDR r0,=08089169 / BL 08000544（SetMainCallback2）/ POP {r4,r5} / POP {r0} / BX r0
@ ★ hooks 0808B1AC は 8 バイト（0808B1AC〜0808B1B3 = BL 08089498 + LDR r0 + BL 08000544 の前半）を潰す。
@   HackOwnCardBuild（C）が 08089498 → 星の差し替え → SetMainCallback2 まで全部やり、ここから素の 0808B1B6（POP {r4,r5}）へ戻る。r0 = カード。
	.global	HackOwnCardHook
	.thumb_func
HackOwnCardHook:
	bl	HackOwnCardBuild
	ldr	r3, =0x0808B1B6 + 1
	bx	r3

	.pool
