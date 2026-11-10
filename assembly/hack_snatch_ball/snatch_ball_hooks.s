.text
.align 2
.thumb
.thumb_func

@ スナッチボール（src/hack_snatch_ball.c・patches/0961）: 捕まえたジングル。
@ hook は 0x80F0F96（r0 経由）: ボールの成功の演出の 95 フレーム目。原版はここで
@ 「bl m4aMPlayAllStop; ldr r0, =0x13F; bl PlaySE」（10 バイト・0x80F0F96〜0x80F0F9F）をして 0x80F0FA0 へ続く。
@ HackSnatch_PlayCaughtJingle は同じことをし、スナッチで捕まえている最中だけ戦闘 BGM を続きから鳴らすタスクを足す。
.global HackSnatch_CaughtJingleHook
HackSnatch_CaughtJingleHook:
	bl HackSnatch_PlayCaughtJingle
	ldr r0, =0x080F0FA0 | 1
	bx r0

.pool
