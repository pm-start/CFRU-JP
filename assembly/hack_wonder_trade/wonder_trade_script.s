.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

@ ミラクル交換（src/hack_wonder_trade.c・2026-10-07）。元は pkhk_BeGiNNeR さんの Wonder_Trades.s（通信交換の場面版）。
@ クチバのポケモンだいすきクラブの 9 人目（けんきゅういん・(9, 8)・assembly/kanto/kanto_gible_egg.s の人物の表）が呼ぶ。
@ special の番号は日本語版（decomp より 1 つ前にずれる）: 0x9F 手持ちを選ぶ・0x147 選んだ子の種族・0xFE 交換の場面

.equ SPECIAL_CHOOSE_PARTY_MON, 0x9F
.equ SPECIAL_GET_PARTY_MON_SPECIES, 0x147
.equ SPECIAL_DO_INGAME_TRADE_SCENE, 0xFE
@ ★ 2026-10-11 ユーザー「話しかけると『いま ミラクル交換は メンテナンス中です』と出て交換できないように」（patches/0324）。
@   クチバの人の入口 EventScript_Wonder_Trades_Kanto だけ（1 なら文を出して終わる・0 なら今までどおり EventScript_Wonder_Trades へ）。
@   EventScript_Wonder_Trades 本体は触らない（f8 のホウエン・カイナの 2 人目（0322）が共有しても止まらないように・5e）。2026-10-16 ユーザー「カントーのミラクル復活させれる？」で 0 に戻した（メンテナンスにするときは 1）。
.equ WONDER_TRADE_MAINTENANCE, 0

.equ WONDER_TRADE_KANTO_CLEAR_FLAG, 0x82C   @ FLAG_SYS_GAME_CLEAR（SYS_FLAGS 0x800 + 0x2C・素の FR のカントーの殿堂入り）
.equ WONDER_TRADE_PRICE, 1000   @ 1 回の値段（円・patches/0429）

.global EventScript_Wonder_Trades_Kanto
EventScript_Wonder_Trades_Kanto:
.if WONDER_TRADE_MAINTENANCE
	lock
	faceplayer
	msgbox gText_WonderTrade_Maintenance MSG_NORMAL
	release
	end
.else
	@ ★ 2026-10-16 クチバの係だけ カントーの殿堂入り（素の FR の FLAG_SYS_GAME_CLEAR 0x82C・ホウエンの HOENN_FLAG_IS_CHAMPION 0x1052 などとは別）のあとに解禁。
	@   メンテナンスのあと・お金の確認より前で止める。カイナの係（HackScr_WonderTrade_Hoenn）は条件を変えない
	checkflag WONDER_TRADE_KANTO_CLEAR_FLAG
	if 0x0 _goto WonderTrade_NotHallOfFame
	goto EventScript_Wonder_Trades
.endif

WonderTrade_NotHallOfFame:
	lock
	faceplayer
	msgbox gText_WonderTrade_NeedHallOfFame MSG_NORMAL
	release
	end

.global EventScript_Wonder_Trades
EventScript_Wonder_Trades:
	lock
	faceplayer
	setvar 0x8007 0x0                  @ 0 = クチバ（ふつう）。src/hack_wonder_trade.c の IsHoennWonderTrade が見る
	goto WonderTrade_Ask

@ ★ 2026-10-11 ユーザー「ホウエンにミラクル交換実装 伝説 幻のピックアップ 色違いも」: カイナシティの ポケモンだいすきクラブ の係
@   （build_hoenn.PEOPLE_APPEND）。Var8007 = 1 で、1/8 伝説・幻・1/16 色違い（C 側）。あとの流れはクチバと同じ
@ ★ 2026-10-23 #49 ユーザー「ホウエンのミラクル交換の配置方法を真似て　シンオウのミラクル交換作成して　ピックアップはウルトラビーストとパラドックス」
@   シンオウの係（tools/sinnoh/sinnoh_wonder_spot.py が版ごとに家・店のどこか 1 軒に置く）。Var8007 = 2 で、1/8 ウルトラビースト・パラドックス（バッジ 16 個から）・
@   1/16 色違い（C 側）。値段・回数・あとの流れはホウエン／クチバと同じ（WonderTrade_Ask 以降を共用）
.global HackScr_WonderTrade_Sinnoh
HackScr_WonderTrade_Sinnoh:
	lock
	faceplayer
	setvar 0x8007 0x2
	msgbox gText_WonderTrade_SinnohIntro MSG_NORMAL
	goto WonderTrade_Ask

.global HackScr_WonderTrade_Hoenn
HackScr_WonderTrade_Hoenn:
	lock
	faceplayer
	setvar 0x8007 0x1
	msgbox gText_WonderTrade_HoennIntro MSG_NORMAL

WonderTrade_Ask:
	@ ★ 2026-10-07 ユーザー「1日5回まで制限はずして」: 回数の確かめ（CheckWonderTradeLimit）をやめた・何回でもできる
	msgbox gText_WonderTrade_Ask MSG_NORMAL
	msgbox gText_WonderTrade_Ask2 MSG_YESNO
	compare LASTRESULT 0x1
	if equal _goto WonderTrade_PriceAsk
	msgbox gText_WonderTrade_ComeAgain MSG_NORMAL
	release
	end

@ ★ 2026-10-16 ユーザー「ミラクル交換の際に 1000 円かかるように」（patches/0429）: 選ぶ前に値段を確かめる（いいえ・足りない＝交換しない）。
@   お金を引くのは交換が成立したあと（WonderTrade_AfterScene）。値段は WONDER_TRADE_PRICE 1 つ（なんいどの半額は効かない）
@   所持金の窓（showmoneybox）は確認のあいだだけ出し、どの出口（いいえ・足りない・選びへ進む）でも hidemoneybox で消す。交換のあとは窓を出さない（更新も不要）
WonderTrade_PriceAsk:
	showmoneybox 0 0 0
	checkmoney WONDER_TRADE_PRICE 0
	compare LASTRESULT 0x0
	if equal _goto WonderTrade_NoMoney
	msgbox gText_WonderTrade_Price MSG_YESNO
	compare LASTRESULT 0x1
	if equal _goto WonderTrade_PriceYes
	hidemoneybox 0 0
	msgbox gText_WonderTrade_ComeAgain MSG_NORMAL
	release
	end

WonderTrade_PriceYes:
	hidemoneybox 0 0
	goto WonderTrade_SelectMon

WonderTrade_NoMoney:
	msgbox gText_WonderTrade_NoMoney MSG_NORMAL
	hidemoneybox 0 0
	release
	end

WonderTrade_SelectMon:
	msgbox gText_WonderTrade_SelectMon MSG_NORMAL
	special SPECIAL_CHOOSE_PARTY_MON
	waitstate
	special2 0x8003 SPECIAL_GET_PARTY_MON_SPECIES
	compare 0x8004 0x6
	if greaterorequal _goto WonderTrade_Cancel @ やめた（0xFF）
	compare 0x8003 0x19C @ SPECIES_EGG
	if equal _goto WonderTrade_NoEgg
	@ ★ 2026-10-07 ユーザー「ポケモン選択時に確認が無いのが危ない」「持ち物を持ってる場合は…確認があった方が良さそう」: いいえ なら選び直し
	callasm WonderTrade_BufferChosen
	@ ★ 2026-10-09 ユーザー「ももアイスをトレード不可アイテムに」: ももアイスを持った子は出せない（0x8006 = 2・src/hack_wonder_trade.c）
	compare 0x8006 0x2
	if equal _goto WonderTrade_NoMomoIce
	@ ★ 2026-10-11 特別なウルガモス（#95）は出せない（0x8006 = 3・src/hack_volcarona.c）
	compare 0x8006 0x3
	if equal _goto WonderTrade_NoSpecialMon
	@ ★ 2026-11-08 ダークの子は出せない（0x8006 = 4・patches/0964・FEAT-shadow-i1）
	compare 0x8006 0x4
	if equal _goto WonderTrade_NoDarkMon
	msgbox gText_WonderTrade_Confirm MSG_YESNO
	compare LASTRESULT 0x1
	if notequal _goto WonderTrade_SelectMon
	compare 0x8006 0x0
	if equal _goto WonderTrade_Go
	msgbox gText_WonderTrade_ItemWarn MSG_YESNO
	compare LASTRESULT 0x1
	if notequal _goto WonderTrade_SelectMon
WonderTrade_Go:
	msgbox gText_WonderTrade_Wait MSG_NORMAL
	msgbox gText_WonderTrade_Wait2 MSG_NORMAL
	@ 受け取るポケモンを作って、通信交換の場面を流す
	copyvar 0x8005 0x8004
	callasm CreateWonderTradePokemon @ gEnemyParty[0] に作る・Var8004 をミラクル交換の目印（0xFFFF）にする
	callasm WonderTrade_TrySkipScene @ せっていで みない なら場面を出さずに入れ替える（LASTRESULT = 1）
	compare LASTRESULT 0x1
	if equal _goto WonderTrade_SkippedScene
	special SPECIAL_DO_INGAME_TRADE_SCENE
	waitstate
WonderTrade_AfterScene:
	removemoney WONDER_TRADE_PRICE 0 @ 交換が成立したあとにだけ引く（場面あり・なしの両方がここを通る）
	copyvar 0x8004 0x8005 @ 目印を外す
	callasm WonderTrade_CountTrade @ 2026-10-10 回数を数える（じっせき）
	@ ★ 2026-10-16 バッジ 32 個のごほうびの知らせ（0x800B: 1 = 6V・2 = 隠れ特性・3 = 両方・src/hack_wonder_trade.c・patches/0429）
	compare 0x800B 0x1
	if equal _goto WonderTrade_Notice6V
	compare 0x800B 0x2
	if equal _goto WonderTrade_NoticeHidden
	compare 0x800B 0x3
	if equal _goto WonderTrade_NoticeBoth
WonderTrade_Thanks:
	msgbox gText_WonderTrade_Thanks MSG_NORMAL
	msgbox gText_WonderTrade_ComeAgain MSG_NORMAL
	release
	end

WonderTrade_Notice6V:
	msgbox gText_WonderTrade_Notice6V MSG_NORMAL
	goto WonderTrade_Thanks

WonderTrade_NoticeHidden:
	msgbox gText_WonderTrade_NoticeHidden MSG_NORMAL
	goto WonderTrade_Thanks

WonderTrade_NoticeBoth:
	msgbox gText_WonderTrade_NoticeBoth MSG_NORMAL
	goto WonderTrade_Thanks

WonderTrade_SkippedScene:
	fanfare 0x101 @ ポケモンを もらったときの曲（MUS_LEVEL_UP＝素の givemon と同じ）
	msgbox gText_WonderTrade_Received MSG_KEEPOPEN
	waitfanfare
	goto WonderTrade_AfterScene

WonderTrade_Cancel:
	msgbox gText_WonderTrade_ComeAgain MSG_NORMAL
	release
	end

WonderTrade_NoMomoIce:
	msgbox gText_MomoIce_NoTrade MSG_NORMAL
	goto WonderTrade_SelectMon

WonderTrade_NoSpecialMon:
	msgbox gText_WonderTrade_NoSpecialMon MSG_NORMAL
	goto WonderTrade_SelectMon

WonderTrade_NoDarkMon:
	msgbox gText_WonderTrade_NoDarkMon MSG_NORMAL
	goto WonderTrade_SelectMon

WonderTrade_NoEgg:
	msgbox gText_WonderTrade_NoEgg MSG_NORMAL
	release
	end

