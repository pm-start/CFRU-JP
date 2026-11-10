/* 生成物（tools/rom/build_debug_spots.py・配置は tools/rom/wonder_trade_spot.py の pick／watashitachi_spots）。手で直さない */
/* デバッグメニュー（HACK_DEBUG_MENU）の「ひとのところへ」。版 0.66.0 の配置・行き先 = 人の隣の歩けるマスと、人のほうへ向く向き */
#pragma once
#define HACK_DEBUG_SPOTS_VERSION "0.66.0"
#define HACK_DEBUG_SPOT_COUNT 22
/* 群, 地図, 立つ x, 立つ y, 向き（並びは わたしたち 10 人 → ミラクル交換の係 → 伝説 10 匹 → シンオウのミラクル交換の係） */
static const struct DebugSpot sDebugSpots[HACK_DEBUG_SPOT_COUNT] = {
	{46, 4, 12, 6, DIR_SOUTH},  /* りる: DewfordTown_Hall の (12, 7) に立つ */
	{64, 0, 2, 2, DIR_EAST},  /* うい: Route116_TunnelersRestHouse の (3, 2) に立つ */
	{61, 0, 5, 6, DIR_EAST},  /* しだれ: Route111_WinstrateFamilysHouse の (6, 6) に立つ */
	{54, 0, 4, 5, DIR_SOUTH},  /* みるく: RustboroCity_DevonCorp_1F の (4, 6) に立つ */
	{54, 4, 0, 3, DIR_SOUTH},  /* なお: RustboroCity_PokemonSchool の (0, 4) に立つ */
	{54, 9, 7, 3, DIR_SOUTH},  /* かいり: RustboroCity_Flat1_2F の (7, 4) に立つ */
	{75, 2, 6, 7, DIR_SOUTH},  /* りっか: Route119_House の (6, 8) に立つ */
	{45, 4, 1, 5, DIR_EAST},  /* しぃ: OldaleTown_Mart の (2, 5) に立つ */
	{48, 1, 4, 6, DIR_EAST},  /* らん: FallarborTown_BattleTentLobby の (5, 6) に立つ */
	{71, 0, 3, 7, DIR_EAST},  /* いちご: Route109_SeashoreHouse の (4, 7) に立つ */
	{58, 8, 6, 2, DIR_SOUTH},  /* ミラクルこうかん: SootopolisCity_House4 の (6, 3) に立つ */
	{43, 33, 74, 12, DIR_NORTH},  /* でんせつ ツキリル: Route118 の (74, 11) に立つ */
	{67, 0, 26, 15, DIR_NORTH},  /* でんせつ ウイシルシ: MeteorFalls_1F_1R の (26, 14) に立つ */
	{67, 7, 8, 9, DIR_NORTH},  /* でんせつ シダレアメ: GraniteCave_1F の (8, 8) に立つ */
	{43, 27, 35, 52, DIR_NORTH},  /* でんせつ ミルキハート: Route112 の (35, 51) に立つ */
	{67, 43, 40, 31, DIR_NORTH},  /* でんせつ ナオガネ: VictoryRoad_1F の (40, 30) に立つ */
	{43, 29, 25, 13, DIR_NORTH},  /* でんせつ カイリパルス: Route114 の (25, 12) に立つ */
	{43, 31, 39, 15, DIR_NORTH},  /* でんせつ リッカユキ: Route116 の (39, 14) に立つ */
	{43, 17, 36, 8, DIR_NORTH},  /* でんせつ シイバラ: Route102 の (36, 7) に立つ */
	{43, 35, 8, 7, DIR_NORTH},  /* でんせつ ランキツネ: Route120 の (8, 6) に立つ */
	{43, 32, 51, 16, DIR_NORTH},  /* でんせつ イチゴベリー: Route117 の (51, 15) に立つ */
	{80, 80, 5, 2, DIR_SOUTH},  /* シンオウ ミラクルこうかん: SUNYSHORE_CITY_IN2 の (5, 3) に立つ */
};
