@ 捕まえたときの図鑑の画面（2026-10-10・src/hack_dex_habitat.c）。すみかの表に無い種族を基本の姿に読み替えてから素の処理へ。
@ 素（DexScreen_RegisterMonToPokedex・081076B8〜）: push {r4,r5,lr} / lsls r0,r0,#16 / lsrs r5,r0,#16 / adds r0,r5,#0 /
@   081076C0 movs r1,#2 / movs r2,#1 / bl DexScreen_GetSetPokedexFlag …
@ hooks で 081076B8 via r1（4 の倍数・潰すのは 081076B8〜081076BF の 4 命令＝関数の入口なので途中へ飛んでくる分岐は無い）。
@ r1 は 081076C0 で入れ直されるので使ってよい。

.text
.thumb
.align 2

.global HackDex_RegisterHook
.thumb_func
HackDex_RegisterHook:
	push {r4, r5, lr}
	bl HackDex_HabitatSpecies
	lsl r0, r0, #16
	lsr r5, r0, #16
	mov r0, r5
	ldr r1, =0x081076C0 | 1
	bx r1
.pool

@ 図鑑の絵を窓に描く素の CreateTrainerCardSprite（0810CB98）が借りる大きさ（2026-10-15・patches/0384・docs/research/2026-10-15_dex-register-heap-freeze.md）。
@ patches/0308 で 0x2000 → 0x1000 にしたが、ポワルン（種族 0x181）の正面の絵だけは 4 つの姿の 0x2000 で、展開すると借りた所の後ろ 0x1000 を
@ 書きつぶしてヒープの鎖が切れた（211ばんどうろでリーシャンを捕まえる → 同じすみかのページにポワルン → 図鑑の画面で固まる）。
@ → ポケモンの絵（isTrainer = 0）でポワルンのときだけ 0x2000、ほかは 0x1000（0x1000 をこえる絵がほかに無いことは tools/rom/verify_dex_pic_alloc.py が見る）。
@ 素（0810CBD2〜）: lsls r6,r6,#24 / lsrs r6,r6,#24 / movs r0,#0x80 / lsls r0,r0,#6 / bl Malloc（0810CBDA〜0810CBDD）/ 0810CBDE adds r4,r0,#0 …
@ hooks で 0810CBD4 via r0（4 の倍数・潰すのは 0810CBD4〜0810CBDB＝lsrs r6・movs・lsls・bl の前半。途中へ飛んでくる分岐は無い）。
@ r0 はすぐ後で入れ直される。r9 = 種族（0810CBB6）・r6 = isTrainer。Malloc を呼んで 0810CBDE（r0 = 借りた所）へ戻る。r1〜r3 は戻り先で入れ直される。
.global HackDexPic_AllocHook
.thumb_func
HackDexPic_AllocHook:
	lsr r6, r6, #24
	mov r0, #0x80
	lsl r0, r0, #5
	cmp r6, #0
	bne HackDexPic_Alloc
	mov r1, #0x18
	lsl r1, r1, #4
	add r1, #1
	cmp r1, r9
	bne HackDexPic_Alloc
	lsl r0, r0, #1
HackDexPic_Alloc:
	ldr r1, =0x08002B9C | 1
	bl HackDexPic_bxr1
	ldr r1, =0x0810CBDE | 1
HackDexPic_bxr1:
	bx r1
.pool

@ 図鑑の絵は♂（ふつう）（2026-11-03・patches/0945・src/hack_dex_male.c）。素の DexScreen の絵を作る所:
@   081070C0 movs r1,#8 / 081070C2 movs r3,#1 / 081070C4 bl CreateMonPicSprite_HandleDeoxys / 081070C8 ldr r1,[r7]…
@ hooks で 081070C0 via r1（4 の倍数・潰すのは 081070C0〜081070C7＝movs・movs・bl。途中へ飛んでくる分岐は素 ROM に無いことを走査で確認）。
@ 同じ引数（r0 species・r1 = 8・r2 personality・r3 = TRUE・スタックに x・y・slot・tag）で、旗を立てる C の包み HackDexMale_CreateMonPic を呼び、081070C8（r1 は入れ直される）へ戻る。
.global HackDexMale_PicHook
.thumb_func
HackDexMale_PicHook:
	mov r1, #8
	mov r3, #1
	bl HackDexMale_CreateMonPic
	ldr r1, =0x081070C8 | 1
	bx r1
.pool
