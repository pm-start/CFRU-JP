@ クチバシティ ポケモンだいすきクラブの フカマルのタマゴをくれる人（2026-10-03）。
@ ユーザー「クチバのだいすきクラブにフカマルの卵くれるNPCほしい」。1 回だけ（印 KANTO_FLAG_GIBLE_EGG 0x12FB）。
@ ユーザー「特別な設定　個体値MAX 色違い ガブリアスをみせるとメガストーンとZ両方もらえる」:
@   タマゴは個体値 6 つとも 31・色違い（src/hack_gible_egg.c HackGibleEgg_Give）。
@   もらったあと、手持ちにガブリアスがいると ガブリアスナイト と ガブリアスナイトZ（Z-A）を 1 回だけ（印 KANTO_FLAG_GIBLE_STONES 0x12FA）。
@ 素の人物は 6 人（0837ED00・地図のイベント 0837EDC0）。人物の表を写して 7 人目（localId 7・エリートトレーナー♀・(9, 5)・下向き）を足し、
@ 地図のイベントの人数（0837EDC0）を 7 に・表の語（0837EDC4）を repoints で向ける。文は strings/kanto_gible_egg.string

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ KANTO_FLAG_GIBLE_EGG, 0x12FB     @ build_hoenn_scripts.RESERVED_FLAGS と同じ
.equ KANTO_FLAG_GIBLE_STONES, 0x12FA  @ 同上
.equ KANTO_FLAG_GIBLE_X, 0x12DD       @ 同上（★ 2026-10-25 ガブリアスナイトX をもらった印・patches/0813）
.equ ITEM_GARCHOMPITE_X, 0x367        @ 同上（include/constants/items.h）
.equ ITEM_GARCHOMPITE, 0x23E          @ include/constants/items.h
.equ ITEM_ZA_GARCHOMPITE_Z, 0x31D     @ 同上（tools/megas/za_megas_ids.json）
.equ OBJ_EVENT_GFX_COOLTRAINER_F, 42
.equ OBJ_EVENT_GFX_POKE_MANIAC, 52          @ ★ 2026-10-04 ボールを入れ替えてくれる人（pokefirered constants/event_objects.h）
.equ OBJ_EVENT_GFX_SCIENTIST, 55            @ ★ 2026-10-07 ミラクル交換の係（同上）
.equ MUS_OBTAIN_ITEM, 258

@ ---- 人物の表（素の 6 人をそのまま写す + フカマルの人）----
.align 2
.global KantoFanClub_Objects
KantoFanClub_Objects:
	.byte 0x01, 0x3D, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x08, 0x11, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0xFD, 0x26, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
	.byte 0x02, 0x30, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0x04, 0x08, 0x11, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x7A, 0x27, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
	.byte 0x03, 0x78, 0x00, 0x00, 0x07, 0x00, 0x06, 0x00, 0x03, 0x01, 0x11, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x1C, 0x28, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
	.byte 0x04, 0x7E, 0x00, 0x00, 0x04, 0x00, 0x06, 0x00, 0x03, 0x01, 0x11, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x2F, 0x28, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
	.byte 0x05, 0x17, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x08, 0x11, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0xA6, 0x27, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
	.byte 0x06, 0x1B, 0x00, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x08, 0x11, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0xE1, 0x27, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00
	.byte 7, OBJ_EVENT_GFX_COOLTRAINER_F, 0, 0
	.2byte 9, 5
	.byte 3, 8, 0x11, 0          @ 高さ 3・下を向く（MOVEMENT_TYPE_FACE_DOWN = 8）
	.2byte 0, 0
	.4byte KantoGibleEgg_EventScript_Talk
	.2byte 0, 0
	@ ★ 2026-10-04 8 人目: ボールを入れ替えてくれる人（assembly/kanto/kanto_ball_change.s・localId 8・(2, 7)・下向き）
	.byte 8, OBJ_EVENT_GFX_POKE_MANIAC, 0, 0
	.2byte 2, 7
	.byte 3, 8, 0x11, 0
	.2byte 0, 0
	.4byte KantoBallChange_EventScript_Talk
	.2byte 0, 0
	@ ★ 2026-10-07 9 人目: ミラクル交換の係（assembly/hack_wonder_trade/wonder_trade_script.s・localId 9・(9, 8)・下向き・けんきゅういん）
	.byte 9, OBJ_EVENT_GFX_SCIENTIST, 0, 0
	.2byte 9, 8
	.byte 3, 8, 0x11, 0
	.2byte 0, 0
	.4byte EventScript_Wonder_Trades_Kanto   @ 2026-10-11 メンテナンス中の入口（patches/0324・wonder_trade_script.s の WONDER_TRADE_MAINTENANCE）
	.2byte 0, 0

.align 2
.global KantoGibleEgg_EventScript_Talk
KantoGibleEgg_EventScript_Talk:
	lock
	faceplayer
	checkflag KANTO_FLAG_GIBLE_EGG
	if 0x1 _goto KantoGibleEgg_AlreadyGiven
	msgbox gText_KantoGibleEgg_Intro MSG_YESNO
	compare LASTRESULT 1
	if 0x5 _goto KantoGibleEgg_Declined
	countpokemon
	compare LASTRESULT 6
	if 0x4 _goto KantoGibleEgg_PartyFull
	callasm HackGibleEgg_Give            @ 個体値 31・色違いのタマゴ（src/hack_gible_egg.c）
	setflag KANTO_FLAG_GIBLE_EGG
	fanfare MUS_OBTAIN_ITEM
	msgbox gText_KantoGibleEgg_Received MSG_KEEPOPEN
	waitfanfare
	msgbox gText_KantoGibleEgg_Afterwards MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_PartyFull:
	msgbox gText_KantoGibleEgg_PartyFull MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_Declined:
	msgbox gText_KantoGibleEgg_Declined MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_AlreadyGiven:
	@ ★ 2026-10-23 FEAT-gible-reitonoou（Redmine #58・patches/0790）: Lv200 のとくべつなガブリアスが手持ちにいたら れーとのおうの印を付けるか聞く
	callasm HackGibleEgg_ReitonoouCheck  @ LASTRESULT 1 = いる（Var8004・名前 gStringVar1）/ 2 = Lv200 に届かない子だけ / 0 = いない
	compare LASTRESULT 1
	if 0x1 _goto KantoGibleEgg_ReitoOffer
KantoGibleEgg_AlreadyGivenNormal:
	@ v0.11 だけの「もう 1 回」（印 0x12F9）は v0.12 で外した。印は予約に残す
	checkflag KANTO_FLAG_GIBLE_STONES
	if 0x1 _goto KantoGibleEgg_AllDone
	callasm HackGibleEgg_HasGarchomp
	compare LASTRESULT 1
	if 0x5 _goto KantoGibleEgg_NoGarchomp
	msgbox gText_KantoGibleEgg_SawGarchomp MSG_KEEPOPEN
	checkitemspace ITEM_GARCHOMPITE 1
	compare LASTRESULT 0
	if 0x1 _goto KantoGibleEgg_BagFull
	checkitemspace ITEM_ZA_GARCHOMPITE_Z 1
	compare LASTRESULT 0
	if 0x1 _goto KantoGibleEgg_BagFull
	obtainitem ITEM_GARCHOMPITE 1
	obtainitem ITEM_ZA_GARCHOMPITE_Z 1
	setflag KANTO_FLAG_GIBLE_STONES
	msgbox gText_KantoGibleEgg_StonesAfter MSG_KEEPOPEN
	goto KantoGibleEgg_TryX                @ ★ 2026-10-25 とくべつなガブリアスがいれば ガブリアスナイトX もこのまま渡す

KantoGibleEgg_NoGarchomp:
	msgbox gText_KantoGibleEgg_AlreadyGiven MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_BagFull:
	msgbox gText_KantoGibleEgg_BagFull MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_AllDone:
	checkflag KANTO_FLAG_GIBLE_X
	if 0x1 _goto KantoGibleEgg_AllDoneNoX
	callasm HackGibleEgg_HasSpecialForX     @ ★ 2026-10-25 patches/0813 とくべつなガブリアス（れーとのおうの印 か クラブ産）が手持ちにいれば ガブリアスナイトXを 1 回だけ
	compare LASTRESULT 1
	if 0x1 _goto KantoGibleEgg_GiveX
KantoGibleEgg_AllDoneNoX:
	callasm HackGibleEgg_ReitonoouCheck
	compare LASTRESULT 2
	if 0x1 _goto KantoGibleEgg_ReitoHint
	msgbox gText_KantoGibleEgg_AllDone MSG_KEEPOPEN
	closemessage
	release
	end

@ ガブリアスナイトZ のあと（同じ会話の続き）: まだ X をもらっていなくて、とくべつなガブリアスがいれば渡す。いなければここで終わる
KantoGibleEgg_TryX:
	checkflag KANTO_FLAG_GIBLE_X
	if 0x1 _goto KantoGibleEgg_EndTalk
	callasm HackGibleEgg_HasSpecialForX
	compare LASTRESULT 1
	if 0x1 _goto KantoGibleEgg_GiveX
KantoGibleEgg_EndTalk:
	closemessage
	release
	end

KantoGibleEgg_GiveX:
	msgbox gText_KantoGibleEgg_SawSpecial MSG_KEEPOPEN
	checkitemspace ITEM_GARCHOMPITE_X 1
	compare LASTRESULT 0
	if 0x1 _goto KantoGibleEgg_BagFull
	callasm HackGibleEgg_MarkForX            @ v0.10 で印が消えた子には うんめいの印を付ける（付けないと X が効かない）
	obtainitem ITEM_GARCHOMPITE_X 1
	setflag KANTO_FLAG_GIBLE_X
	msgbox gText_KantoGibleEgg_XAfter MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_ReitoHint:
	msgbox gText_KantoGibleEgg_ReitoHint MSG_KEEPOPEN
	closemessage
	release
	end

KantoGibleEgg_ReitoOffer:
	msgbox gText_KantoGibleEgg_ReitoOffer MSG_YESNO
	compare LASTRESULT 1
	if 0x5 _goto KantoGibleEgg_ReitoDeclined
	callasm HackGibleEgg_ReitonoouApply  @ Var8004 の子に印（PokemonSubstruct3.blank の bit0・src/hack_gible_egg.c）
	fanfare MUS_OBTAIN_ITEM
	msgbox gText_KantoGibleEgg_ReitoDone MSG_KEEPOPEN
	waitfanfare
	closemessage
	release
	end

KantoGibleEgg_ReitoDeclined:
	msgbox gText_KantoGibleEgg_ReitoDeclined MSG_KEEPOPEN
	goto KantoGibleEgg_AlreadyGivenNormal
