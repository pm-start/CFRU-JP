.text
.align 2
.thumb
.thumb_func

@ ホウエンのサファリのポロック（src/hack_safari_pokeblock.c・FEAT-pokeblock 段 6・patches/0978）の hooks の入口。
@ 素の FR（JP）の HandleInputChooseAction（0x080DE58C・サファリの行動のメニュー）と SafariHandleChooseAction（0x080DEC60）の途中。

@ hook は 0x080DE5AC（r1 経由・8 バイト 0x080DE5AC〜0x080DE5B3）。素のここは「CMP r0,#1; BEQ 0x080DE5D8; CMP r0,#1; BGT 0x080DE5C8」＝カーソルの位置（r0）で行き先を分ける所。
@ r0 = カーソル（A を押した行動の番号）。r1 は直後に使われない（行き先ごとに r0・r1 を組み直す）。
@ カーソル 1（素は「エサだま」）かつホウエンなら HackSafari_TryOpenCase がケースを開く（メニューの関数の終わり 0x080DE702 へ）。それ以外は素の行き先へ（r0 は変えない）。
.global HackSafari_ActionHook
HackSafari_ActionHook:
	cmp r0, #1
	bne HackSafari_ActionNotOne
	bl HackSafari_TryOpenCase
	cmp r0, #0
	beq HackSafari_ActionVanillaBait
	ldr r0, =0x080DE702 | 1          @ 素の「POP {r4,r5}; POP {r0}; BX r0」（A の枝の終わり）
	bx r0
HackSafari_ActionVanillaBait:
	ldr r0, =0x080DE5D8 | 1          @ 素のカーソル 1: MOV r0,#1; MOV r1,#6; ...EmitTwoReturnValues
	bx r0
HackSafari_ActionNotOne:
	cmp r0, #1
	bgt HackSafari_ActionBig
	ldr r1, =0x080DE5B4 | 1          @ 素の「CMP r0,#0; BEQ 0x080DE5D2; B 0x080DE5F4」
	bx r1
HackSafari_ActionBig:
	ldr r1, =0x080DE5C8 | 1          @ 素の「CMP r0,#2; BEQ 0x080DE5DE; CMP r0,#3; BEQ 0x080DE5EA; B 0x080DE5F4」
	bx r1

@ hook は 0x080DEC78（r0 経由・8 バイト 0x080DEC78〜0x080DEC7F）。素のここは「LDR r0,=gText_SafariZoneMenu; MOV r1,#2; BL BattlePutTextOnWindow」（メニューの文を窓 2 に出す）。
@ HackSafari_PutMenuText が同じことをし（ホウエンなら文だけ「ポロック」版）、素の続き 0x080DEC80（MOV r4,#0）へ戻る。r0・r1 は直後に使われない。
.global HackSafari_MenuHook
HackSafari_MenuHook:
	bl HackSafari_PutMenuText
	ldr r0, =0x080DEC80 | 1
	bx r0

.pool
