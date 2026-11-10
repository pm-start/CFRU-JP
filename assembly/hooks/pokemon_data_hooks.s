.thumb
.text
.align 2

.include "../asm_defines.s"

@Repoint at 0x803FE70
GetMonPokeBallHook:
	mov r0, r8
	bl GetMonPokeBall
	mov r4, r0
	ldr r1, =0x803FA50 | 1
	bx r1

@Hook at 0x8040858 with r0
SetMonPokeBallHook:
	mov r0, r8 @Substruct0
	ldrb r1, [r4] @Ball To Set
	bl SetMonPokeBall
	ldr r0, =0x80401CE | 1
	bx r0

@Hook at 0x08040784 with r2
SetMonHeldItemHook:
	lsl r0, #0x8
	add r1, r0
	mov r0, r8 @Substruct0
	mov r2, r7 @struct Pokemon
	bl SetMonHeldItem
	ldr r0, =0x80401CE | 1
	bx r0

@Hook at 0x8031E40 with r0
PlayerHandleFormChange:
	bl HandleFormChange
	ldr r0, =0x80315CA | 1
	bx r0

@Hook at 0x08037760 with r0
OpponentHandleFormChange:
	bl HandleFormChange
	ldr r0, =0x8036EE8 | 1
	bx r0

@Hook at 0x0803C318 with r0
LinkOpponentHandleFormChange:
	bl HandleFormChange
	ldr r0, =0x803BA98 | 1
	bx r0

@Hook at 0x0803DAD2 with r0
CreateBoxMonHook:
	ldr r0, [sp, #0x40 + 0x0] @hasFixedPersonality
	str r0, [sp, #0x0]
	ldr r0, [sp, #0x40 + 0x4] @fixedPersonality
	str r0, [sp, #0x4]
	ldr r0, [sp, #0x40 + 0x8] @otIdType
	str r0, [sp, #0x8]
	ldr r0, [sp, #0x40 + 0xC] @fixedOtId
	str r0, [sp, #0xC]
	mov r0, r7
	bl CreateBoxMon
	ldr r0, =0x803D4F0 | 1
	bx r0

@Repoint at 0x803FDEC
CheckHasSpeciesJump:
	mov r0, r9
	bl GetAlternateHasSpecies
	mov r4, r0
	ldr r1, =0x803FA5C | 1
	bx r1


CheckRegularMonDataReturn:
	ldr r1, =0x803F52E | 1
	bx r1

@L 案（2026-10-16・patches/0431）: 出会った場所 16 ビット・出会ったレベル 11 ビット（ext は 80 バイト形のヘッダ +0x1E・src/hack_met_level.c）
@GetBoxMonData の跳び表（0803F544）の 35・36 の枝。r9 = 箱のポケモン・結果は r4 に入れて共通の出口 0803FA50 へ
@Repoint at 0x803F5D0
GetMonMetLocationHook:
	mov r0, r9
	bl HackMetLocation_Get
	mov r4, r0
	ldr r1, =0x803FA50 | 1
	bx r1

@Repoint at 0x803F5D4
GetMonMetLevelHook:
	mov r0, r9
	bl HackMetLevel_Get
	mov r4, r0
	ldr r1, =0x803FA50 | 1
	bx r1

@SetBoxMonData の跳び表（0803FC5C）の 35・36 の枝。r7 = 箱のポケモン・r4 = 値のアドレス（素の呼び元は u8 変数・1 バイトだけ読む）・共通の出口 080401CE
@Repoint at 0x803FCE8
SetMonMetLocationHook:
	mov r0, r7
	ldrb r1, [r4]
	bl HackMetLocation_Set
	ldr r0, =0x80401CE | 1
	bx r0

@Repoint at 0x803FCEC
SetMonMetLevelHook:
	mov r0, r7
	ldrb r1, [r4]
	bl HackMetLevel_Set
	ldr r0, =0x80401CE | 1
	bx r0
