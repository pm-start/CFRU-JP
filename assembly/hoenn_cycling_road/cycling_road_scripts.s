@ サイクリングロードは自転車でしか入れない（2026-10-15・遊ぶ人の報告 v0.42.3「自転車に乗っていなくても入れる」）。
@ 根拠は docs/research/2026-10-15_cycling-road-bike-only.md
@
@ ★ 110 ばんどうろのゲート 2 枚（Route110_SeasideCyclingRoadSouthEntrance／NorthEntrance・人を置く地図）の床のイベントが呼ぶ。
@   床のイベントは tools/rom/build_hoenn.py の CYCLING_ROAD_GATE_COORDS（PEOPLE_COORDS）:
@     (6, 4) 一時変数 0x4001 が 0 → HackScr_CyclingRoad_BikeCheck（通路の左端・ここを通らないとサイクリングロードの側へ行けない）
@     (5, 4) 一時変数 0x4001 が 1 → HackScr_CyclingRoad_Clear（判定を通ったあと地面の側へ戻ったら印を片付ける）
@ ★ エメラルドの形（VAR_TEMP_1・FLAG_SYS_CYCLING_ROAD・specialvar GetPlayerAvatarBike・係が押し戻す）を素 FRLG の形で書いた:
@   - 印は素 FRLG の FLAG_SYS_ON_CYCLING_ROAD（0x830）。立っている間は自転車から降りられない（素 FRLG の自転車を使う処理が見る）。
@     そらをとぶ・テレポート・目の前が真っ暗のときは素 FRLG が下ろす（推測・資料を見ること）
@   - GetPlayerAvatarBike は素 FRLG の special 0xE3（227・hoenn_scripts.json の specials と同じ番号＝Route110_EventScript_ChallengeGuy も使う）。
@     ダート（ACRO）なら 1・つうじょう／マッハ（MACH）なら 2・歩きなら 0（docs/research/2026-09-24_bike-modes.md のモードの旗）。
@     0 でなければ通す＝どのモードでも入れる（エメラルドもマッハ・ダートのどちらでも入れる）
@   - 乗っていないときは印も下ろす（エメラルドには無い。自転車のまま入ったあと何かで降ろされた人が、印が立ったまま地面の側へ
@     押し戻されて自転車に乗れなくなるのを防ぐ）

.text
.thumb
.align 2

.include "../xse_commands.s"
.include "../xse_defines.s"

.equ FLAG_SYS_ON_CYCLING_ROAD, 0x830   @ include/constants/flags.h（SYS_FLAGS 0x800 + 0x30）・検算が見る
.equ VAR_CYCLING_ROAD_GATE, 0x4001     @ 一時変数（エメラルドの VAR_TEMP_1・地図に入ると 0）・build_hoenn.CYCLING_ROAD_GATE_COORDS と同じ
.equ SP_GET_PLAYER_AVATAR_BIKE, 0xE3   @ 素 FRLG の special の表 08163068 の 227＝GetPlayerAvatarBike（CLAUDE 6.87・検算が見る）
.equ M_WALK_LEFT, 0x12
.equ M_END, 0xFE                       @ 動きの終わり（xse_defines.s に無い・assembly/hoenn_islands/islands_scripts.s と同じ）

.align 2
.global HackScr_CyclingRoad_BikeCheck
HackScr_CyclingRoad_BikeCheck:
	lockall
	specialvar LASTRESULT SP_GET_PLAYER_AVATAR_BIKE
	compare LASTRESULT 0
	if 0x1 _goto HackScr_CyclingRoad_NoBike
	setflag FLAG_SYS_ON_CYCLING_ROAD
	setvar VAR_CYCLING_ROAD_GATE 1
	releaseall
	end

HackScr_CyclingRoad_NoBike:
	msgbox gText_HoennCyclingRoad_TooDangerous MSG_KEEPOPEN
	closemessage
	clearflag FLAG_SYS_ON_CYCLING_ROAD
	applymovement PLAYER HackScr_CyclingRoad_Move_PushBack
	waitmovement 0
	releaseall
	end

HackScr_CyclingRoad_Move_PushBack:
	.byte M_WALK_LEFT, M_END

.align 2
.global HackScr_CyclingRoad_Clear
HackScr_CyclingRoad_Clear:
	lockall
	clearflag FLAG_SYS_ON_CYCLING_ROAD
	setvar VAR_CYCLING_ROAD_GATE 0
	releaseall
	end
