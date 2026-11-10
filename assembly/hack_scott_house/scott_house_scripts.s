@ エニシダの家（エメラルドの BattleFrontier_ScottsHouse・群 69 の 46）のエニシダ（2026-10-25・Redmine #115・patches/0816）。
@ 原文 参考/pokeemerald/data/maps/BattleFrontier_ScottsHouse/scripts.inc の順:
@   シンボルがそろっていたら きのみ（銀 7 つ → サンのみ・金 7 つ → スターのみ・一人に 1 回ずつ・旗 0xFD6／0xFD7）
@   そうでなければ ふだんの話（初回の挨拶＋BP の説明・ブレーン・図鑑・招待の理由の 4 択）
@ 省いた物: 記念の BP の贈り物（ユーザー決定）・銀と金の盾（飾りが無い）・一時旗（受け取り済みの旗で足りる）・歩く向きの演出
@ 文は strings/hack_scott_house.string（エメラルド日本語版 ROM の文）。きのみ袋が満杯のときは素の「持ち物がいっぱい」の文（obtainitem の標準の文）

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"
.include "../asm_defines.s"

.equ HACK_FLAG_SCOTT_SILVER_BERRY, 0xFD6     @ src/hack_scott_house.h
.equ HACK_FLAG_SCOTT_GOLD_BERRY, 0xFD7

.align 2
.global HackScott_EventScript_Talk
HackScott_EventScript_Talk:
	lock
	faceplayer
	callasm HackScott_Check       @ LASTRESULT 1 = サンのみ／2 = スターのみ／0 = ふだんの話
	compare LASTRESULT 1
	if 0x1 _goto HackScott_Silver
	compare LASTRESULT 2
	if 0x1 _goto HackScott_Gold
	callasm HackScott_RandomLine  @ LASTRESULT 0〜3
	compare LASTRESULT 1
	if 0x1 _goto HackScott_Brain
	compare LASTRESULT 2
	if 0x1 _goto HackScott_Dex
	compare LASTRESULT 3
	if 0x1 _goto HackScott_Why
	msgbox gText_Scott_Welcome MSG_KEEPOPEN
	msgbox gText_Scott_Effort MSG_KEEPOPEN
	msgbox gText_Scott_Remark MSG_KEEPOPEN
	msgbox gText_Scott_ExplainBP MSG_KEEPOPEN
	closemessage
	release
	end

HackScott_Brain:
	msgbox gText_Scott_Brain MSG_KEEPOPEN
	closemessage
	release
	end

HackScott_Dex:
	msgbox gText_Scott_Dex MSG_KEEPOPEN
	closemessage
	release
	end

HackScott_Why:
	msgbox gText_Scott_Why MSG_KEEPOPEN
	closemessage
	release
	end

HackScott_Silver:
	msgbox gText_Scott_Silver MSG_KEEPOPEN
	checkitemspace ITEM_LANSAT_BERRY 1
	compare LASTRESULT 0
	if 0x1 _goto HackScott_SilverBagFull
	obtainitem ITEM_LANSAT_BERRY 1
	setflag HACK_FLAG_SCOTT_SILVER_BERRY
	msgbox gText_Scott_SoGlad MSG_KEEPOPEN
	closemessage
	release
	end

HackScott_SilverBagFull:
	obtainitem ITEM_LANSAT_BERRY 1       @ 入らないので標準の「持ち物がいっぱい」の文が出る（旗は立てない＝また話しかければ渡す）
	closemessage
	release
	end

HackScott_Gold:
	msgbox gText_Scott_Gold MSG_KEEPOPEN
	checkitemspace ITEM_STARF_BERRY 1
	compare LASTRESULT 0
	if 0x1 _goto HackScott_GoldBagFull
	obtainitem ITEM_STARF_BERRY 1
	setflag HACK_FLAG_SCOTT_GOLD_BERRY
	msgbox gText_Scott_SoGlad MSG_KEEPOPEN
	closemessage
	release
	end

HackScott_GoldBagFull:
	obtainitem ITEM_STARF_BERRY 1
	closemessage
	release
	end
