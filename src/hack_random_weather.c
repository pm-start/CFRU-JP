#include "defines.h"
#include "../include/event_data.h"
#include "../include/global.fieldmap.h"
#include "map_region.h"
#include "Tables/hack_weather_tables.h"

/*
	ランダム天気（2026-10-12 ユーザー「レジェンズアルセウスのように、エリアごとに天候の確率テーブルがあり、時間で天候が変わる。屋内は天候なし固定」
	→「1 日 4 回ランダム」・#106・patches/0345）。5e の案: 戦闘にも入る（雨・晴れ・すなあらしは CFRU の「フィールドの天気 → 戦闘の天気」に乗る）・
	せっていの「てんき」ランダム／きまり（旗 0xF16 が立っていれば きまり＝地図の見出しのまま）。

	差し込み口: 素の SetSav1WeatherFromCurrMapHeader（0807A8A0・地図に入るとき）を functionrewrites で置き換える。中身は素と同じ
	（前の天気 → TranslateWeatherNum(見出しの天気) → UpdateRainCounter）で、間に差し替えの判定を 1 つ足す。
	スクリプトの setweather・天気を切り替えるマス（hoenn_weather.c）・異常気象（hoenn_abnormal_weather.c）は SetSav1Weather／SetWeather を通るので触らない。

	差し替えるのは全部そろったときだけ:
	  - 旗 0xF16 が下りている（ランダム）
	  - 地図の種類が 町（1）・都市（2）・道路（3）・海の道路（6）＝屋内・洞窟（4）・水中（5）・秘密基地は触らない。7 はこの ROM で 0 枚（2026-10-12 数えた）
	  - 訳した天気が 0（なし）・1（晴れ雲）・2（晴れ）＝灰 15・雨・霧・雪の決まった地図はそのまま
	  - 表がある（地名ごとの上書きが「変えない」でない）
	抽選は状態を持たない（6.13）: 日付 × 4 ＋ 時間帯（HackWild_DaySlot＝入れ替えた野生と同じ数え方・src/wild_shuffle.c）と地方・地名から決まる。
	同じ日の同じ時間帯に同じ所へ戻れば同じ天気。時間帯が変わっても、次に地図を出入りするまで変わらない（毎コマは見ない・6.80）。
*/

#define HACK_FLAG_WEATHER_FIXED 0xF16   //立っていれば きまり（せっていの「てんき」・5e の台帳 2026-10-12）

#define TranslateWeatherNum ((u8 (*)(u8)) 0x0807A92D)        //素（BPRJ.ld に無い・灰 15 の repoints はこの中の表）
#define UpdateRainCounter ((void (*)(u8, u8)) 0x0807AA1D)    //素（BPRJ.ld に無い）

u32 HackWild_DaySlot(void);   //src/wild_shuffle.c

static bool8 IsOutdoorType(u8 mapType)
{
	return mapType == MAP_TYPE_TOWN || mapType == MAP_TYPE_CITY || mapType == MAP_TYPE_ROUTE || mapType == MAP_TYPE_6;
}

#define WEATHER_FOG 6   //水平の霧（OBJ の優先度 2＝BG1 の上の層（木のてっぺんなど）には掛からない）

//霧を出さない地図か（地図に入るときだけ・二分探索。表は tools/weather/measure_fog_maps.py が地図のデータから数える）
static bool8 IsNoFogMap(void)
{
	u16 key = (u16) ((gSaveBlock1->location.mapGroup << 8) | (u8) gSaveBlock1->location.mapNum);
	u32 lo = 0, hi = WEATHER_NO_FOG_COUNT, mid;

	while (lo < hi)
	{
		mid = (lo + hi) >> 1;
		if (sWeatherNoFogMaps[mid] == key)
			return TRUE;
		if (sWeatherNoFogMaps[mid] < key)
			lo = mid + 1;
		else
			hi = mid;
	}
	return FALSE;
}

u8 HackRandomWeather_Pick(u8 weather)
{
	u8 region, sec, table, i;
	u32 x, raw, rest, prev;

	if (FlagGet(HACK_FLAG_WEATHER_FIXED) || weather > 2 || !IsOutdoorType(gMapHeader.mapType))
		return weather;
	region = GetMapRegionOf(gSaveBlock1->location.mapGroup, gSaveBlock1->location.mapNum);
	if (region >= NELEMS(sWeatherLand))
		return weather;
	sec = gMapHeader.regionMapSectionId;
	table = sWeatherSecOverride[region][sec];
	if (table == WEATHER_TABLE_NO_CHANGE)
		return weather;
	if (table == WEATHER_TABLE_KEEP_REGION)
		table = (gMapHeader.mapType == MAP_TYPE_6) ? sWeatherSea[region] : sWeatherLand[region];
	if (table >= NELEMS(sWeatherTables))
		return weather;
	x = HackWild_DaySlot() * 0x9E3779B1u ^ (sec * 31u + region * 1009u + 0x77E4u);
	x ^= x >> 15;
	x *= 0x2C1B3C6Du;
	x ^= x >> 13;
	raw = x;
	x %= 100;   //6.83: 同じ数の / と % を並べない（% だけ）
	for (i = 0; i < WEATHER_TABLE_WIDTH && sWeatherTables[table][i][1] != 0; ++i)
		if (x < sWeatherTables[table][i][1])
		{
			if (sWeatherTables[table][i][0] != WEATHER_FOG || !IsNoFogMap())
				return sWeatherTables[table][i][0];
			//霧が出ない地図: 霧の分を除いた残りの確率の按分で引き直す（同じ raw から・% だけ）
			for (rest = 0, prev = 0, i = 0; i < WEATHER_TABLE_WIDTH && sWeatherTables[table][i][1] != 0; ++i)
			{
				if (sWeatherTables[table][i][0] != WEATHER_FOG)
					rest += sWeatherTables[table][i][1] - prev;
				prev = sWeatherTables[table][i][1];
			}
			if (rest == 0)
				return weather;
			raw %= rest;
			for (prev = 0, i = 0; i < WEATHER_TABLE_WIDTH && sWeatherTables[table][i][1] != 0; ++i)
			{
				if (sWeatherTables[table][i][0] != WEATHER_FOG)
				{
					if (raw < sWeatherTables[table][i][1] - prev)
						return sWeatherTables[table][i][0];
					raw -= sWeatherTables[table][i][1] - prev;
				}
				prev = sWeatherTables[table][i][1];
			}
			return weather;
		}
	return weather;
}

//functionrewrites 0807A8A0（素の SetSav1WeatherFromCurrMapHeader）
void HackRandomWeather_SetSav1FromHeader(void)
{
	u8 old = gSaveBlock1->weather;

	gSaveBlock1->weather = HackRandomWeather_Pick(TranslateWeatherNum(gMapHeader.weather));
	UpdateRainCounter(gSaveBlock1->weather, old);
}
