@ しばり 縛り C（0.50.3・patches/0550・src/hack_challenge_c.c）の hooks の入れ物。
@
@ HackChallengeC_BattleStartHook（hooks: 0807EDB8 3）: 素の CreateBattleStartTask（全部の戦闘・野生・トレーナー・固定の戦いが通る入口）。
@   素の頭 8 バイト（4 の倍数の番地・LDR r3 / BX r3 / 語 に潰される）:
@       30 B5  push {r4, r5, lr}
@       04 1C  mov  r4, r0        （transition）
@       0D 1C  mov  r5, r1        （song）
@       24 06  lsl  r4, r4, #24
@   をここで写し、間に C（HackChallengeC_OnBattleStart＝モノタイプで出せない子を ひんしにする）を挟んで 0807EDC0（24 0E lsr r4, r4, #24）へ戻る。
@   0.50.6 #124: 先に HackBadgeSquash_OnBattleStart（src/hack_battle_squash.c）も呼ぶ。
@   r4・r5 は C が壊さない（呼ばれる側が保存する）ので r0・r1 の代わりに使える。r3 は hooks の飛び先の読み込みに使われた（引数ではない）
	.text
	.thumb
	.align	2

	.global	HackChallengeC_BattleStartHook
	.thumb_func
HackChallengeC_BattleStartHook:
	push	{r4, r5, lr}
	mov	r4, r0
	mov	r5, r1
	bl	HackBadgeSquash_OnBattleStart		@ #124: 上限をこえた子を戦闘のあいだだけ上限のレベルに（モノタイプの記録より先＝記録する HP も入れ替え後の値）
	bl	HackChallengeC_OnBattleStart
	bl	HackChallengeF_OnBattleStart		@ 縛り F さかさバトル（0.63.0）: 縛り ON なら FLAG_INVERSE を立てる（戦闘が終わると end_battle.c の gEndBattleFlagClearTable が下ろす）
	lsl	r4, r4, #24
	ldr	r3, =0x0807EDC0 + 1
	bx	r3
	.align	2
	.pool