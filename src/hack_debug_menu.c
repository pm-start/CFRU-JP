#include "defines.h"
#include "defines_battle.h"
#include "../include/bg.h"
#include "../include/event_object_movement.h"
#include "../include/field_player_avatar.h"
#include "../include/sprite.h"
#include "../include/event_data.h"
#include "../include/field_screen_effect.h"
#include "../include/item.h"
#include "../include/constants/items.h"
#include "../include/main.h"
#include "../include/menu.h"
#include "../include/money.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/pokemon.h"
#include "../include/script.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/window.h"
#include "../include/string_util.h"
#include "../include/new/learn_move.h"
#include "../include/new/battle_strings.h"
#include "../include/malloc.h"
#include "../include/m4a.h"
#include "../include/sound.h"
#include "../include/constants/songs.h"
#include "../include/script_pokemon_80F8.h"
#include "../include/new/Vanilla_functions.h"
#include "map_region.h"
#include "region_fly.h"
#include "Tables/hoenn_script_flags.h"   /* JOHTO_FLAG_TOWNMAP / SINNOH_FLAG_TOWNMAP */
#include "../include/constants/maps.h"
#include "../include/constants/map_types.h"
#include "../include/constants/region_map_sections.h"
#include "Tables/johto_flags.h"           /* JOHTO_FLAG_BADGE1〜8（クリア後の状態） */
#include "Tables/sinnoh_flags.h"          /* SINNOH_FLAG_BADGE1〜8 */
#include "hack_debug_menu.h"
#include "hack_mon_id.h"
#include "hack_debug_mon.h"               /* デバッグモン（種族 0x5E0／0x5E1）のタイプ・特性・つよさ（手持ち編集の欄 21〜24） */
#include "hack_watashitachi.h"            /* WATASHI_TRAINER_FIRST / WATASHI_COUNT（わたしたちと たたかう） */

/*
	デバッグメニュー（2026-10-11 ユーザー「https://github.com/pm-start/debug-menu これ取り込んで」「ビルドオプションで切り替えできるように」）。

	元は pm-start/debug-menu（作者 liru さん・806579e）: 素の日本語版 FR に IPS で当てる、フラグ 0x8FF で開けるメニュー。
	こちらは CFRU の C に書き直した:
	  - ビルドの指定 `build_rom.py --debug-menu`（デバッグメニュー付きビルド.bat）で -DHACK_DEBUG_MENU を渡したときだけ入る。
	    指定なしのビルドとデモ版（--demo とは一緒に使えない）には処理が入らない（文字列 strings/hack_debug_menu.string だけは CFRU の build.py が
	    .string を全部組むので入る・数百バイト）。入れたビルドでは常に開ける（元の 0x8FF・0x8FD は使わない）
	  - 開き方は「フィールドで B を押しながら Select」（元は A＋Select・2026-10-12 ネット交換（src/hack_network_trade.c・pm-start/enchanted-network-trade）が
	    A＋Select を使うので、ユーザー「競合するなら取り込みを優先」で B＋Select に移した）。素の処理にフックを足さず、野外の毎フレーム（src/overworld_speedup.c）で
	    gMain.callback1 を HackDebugMenu_CB1（見てから素の CB1_Overworld）に差し替える。開いたフレームの Select は newKeys から消す（べんりボタンを使わない）
	  - 捕獲 100%: ON/OFF を予約フラグ HACK_FLAG_DEBUG_CATCH に保つ。捕獲率の式（src/hack_difficulty.c の HackDifficulty_CatchOdds・patches/0326 の差し込み口）
	    が 0xFF を返す＝通常の野生の捕獲だけ（トレーナー戦・ゴースト・チュートリアルは前の分岐のまま）。CFRU の FLAG_ALWAYS_CATCHABLE は戦いのたびに下ろされるので使わない
	  - アイテムの番号は 1〜ITEMS_COUNT-1・個数 1〜999。所持金はこちらの上限 9,999,999 まで足す（AddMoney が頭打ちにする）
	  - ワープは全部の地図（2026-10-11 ユーザー「ワープのマップ全域対応」）: 群と地図の番号を選び、地名（地方ごとの番号＝MapRegion_GetName）を出す。
	    地図にワープの入口があればその 0 番、無ければ地図の真ん中へ。初めは今いる地図。群の数は DEBUG_MAP_GROUP_COUNT、群ごとの地図の数は
	    ROM の表から数える（MapCountInGroup）。フラグ編集は 1〜0x18FF
	  - 「デバッグをむこうに」は外した（ビルドで入れるので要らない）
	  - 2026-10-12 ユーザー「デバッグメニュー追加」（萌えもん化と色違い化／野生に合わない／手持ち編集／欲しいポケモンが手に入る／無敵／一撃で倒す／
	    任意のショッピングへ／Lv200／トレーナー無視／図鑑コンプリート）:
	      いちばん上の一覧は 19 行のスクロール（9 行見せる）。切り替え 4 つは旗 0xF0E〜0xF11（hack_debug_menu.h・セーブに残る・ふだんの版では読まない）。
	      効き目は上流の .c の #ifdef HACK_DEBUG_MENU の中から聞く（patches/0342）: やせいなし＝wild_encounter.c の Standard／SweetScent／
	      DoesFishBite／RockSmash と dexnav.c の InitDexNavHUD の頭・トレーナーむし＝overworld.c の CheckForTrainersWantingBattle の頭・
	      むてき／いちげき＝general_bs_commands.c の atk0C_datahpupdate の頭（HackDebugMenu_AdjustDamage・通信では効かない）。
	      萌えもん化／色違い化は手持ちを選んで「当たり ⇔ もとに」を入れ替える（src/hack_mon_id.c の HackMonId_SetMoemon／SetShiny＝裏 ID・表 ID だけ・6.5）。
	      手持ち編集は 21 の欄を左右で直してすぐ書く（性格はミント natureMint・特性は枠 0/1/2・Lv は経験値の表・個体値は hyperTraining を触らない）。
	      ショップは道具の番号を 50 個ずつのページ（名前が「？」と値段 0 の道具は外す）にして、RAM に組んだ小さな台本（callasm → waitstate → release）で
	      素の CreatePokemartMenu（0809BB54）を開く（閉じたら台本が主人公を放す）。台本と品の表は Malloc のまま（開くたびに約 110 バイト・デバッグ版だけ）
	  - 2026-10-16 デバッグルーム: ワープのしゅるいの 5 つ目。群 79 の 1〜3 番（tools/kanto/build_debug_room.py・docs/research/2026-10-16_debug-room.md）へ。
	    地図のつながりは作らず、飛ぶ前に今の位置を dynamicWarp に書く（出口のマットが動的ワープ (0x7F,0x7F)＝入ってきた場所へ戻る）
	状態はタスクの data に持つ（6.13: 書き換えられる静的変数は使えない）。
*/

#ifdef HACK_DEBUG_MENU

#include "hack_ram_cheat.h"
#include "hack_stat_access.h" //[hack] patches/0763 努力値・個体値の窓口
#include "hack_kind.h" //[hack] patches/0958 個体の印
#define gHelpSystemEnabled (*((bool8 *)0x03005F1C))   //L/R のヘルプ（src/bw_start_menu.c と同じ）

#define DEBUG_FLAG_MAX 0x1CFF   //[hack] patches/0986 ジョウトの旗の窓（0x1900〜0x1AFF）・FEAT-johto-p1 の窓 2（0x1B00〜0x1CFF）まで
#define DEBUG_ITEM_COUNT_MAX 999
#define DEBUG_MAP_GROUP_COUNT 81     //群 0〜80（素 0〜42・ホウエン 43〜76・シンオウ 77・ジョウト 78・カントーの追加 79・シンオウの建物 80＝src/map_region.h）
#define MAP_GROUPS_LITERAL 0x08054B0C   //素の Overworld_GetMapHeaderByGroupAndId（08054AF8）の定数＝gMapGroups の番地（tools/rom/verify_hoenn_wild.py と同じ）
#define DEBUG_MONEY_GIVE 9999999

#define TOP_ROWS 9                   //いちばん上の一覧の見せる行（y 16〜112）
#define EDIT_ROWS 8                  //手持ち編集の見せる行
#define EDIT_FIELDS 27               //種族・Lv・性格・特性の枠・持ち物・技 4・個体値 6・努力値 6・（デバッグモンだけ）つよさ・タイプ 1・タイプ 2・特性（全部）・しるし 2（patches/0958）
#define EDIT_KIND_FIRST 25           //しるし（25 = 種類 0〜3・26 = あたい 0〜255・src/hack_kind.c）
#define EDIT_DM_FIRST 21             //デバッグモンの欄の始まり（2026-10-16・hack_debug_mon.c の欄 0〜3）
#define SHOP_PAGE 50                 //ショップの 1 ページの道具の数
#define CREATE_POKEMART_MENU ((void (*)(const u16*)) (0x0809BB54 | 1))   //素の CreatePokemartMenu（ScrCmd_pokemart 0806BBA0 から）
#define NATURE_NAMES ((const u8* const*) 0x0842D1EC)                      //性格名 25 本（素の日本語版・src/bw_summary_screen.c と同じ）
#define DEX_FLAG_SET_SEEN 2
#define DEX_FLAG_SET_CAUGHT 3
#define BGM_ROWS 8                   //BGMテスト: 曲の一覧の 1 ページの行（L／R で 1 ページずつ）

#include "hack_debug_bgm.h"          //BGMテスト: 分類・曲の番号・かなのラベル（tools/bgm/build_debug_bgm.py が曲の表から生成・全部 static const）

enum
{
	MODE_TOP,
	MODE_ITEM,
	MODE_WARP,
	MODE_WARP_MENU,
	MODE_WARP_REGION,
	MODE_WARP_GYM,
	MODE_WARP_PERSON,   //わたしたち 10 人・ミラクル交換の係のところ（この版の配置・src/hack_debug_spots.h）
	MODE_WARP_ROOM,     //デバッグルーム 3 部屋（群 79 の 1〜4 番・tools/kanto/build_debug_room.py）
	MODE_FLAG,
	MODE_PICK,     //手持ちを選ぶ（萌えもん化・色違い化・手持ち編集）
	MODE_EDIT,
	MODE_GETMON,
	MODE_SHOP,
	MODE_WATASHI,  //わたしたち 10 人から選んで その場で戦う
	MODE_WILD,     //種族・レベル・色違い・Moémon を選んで その場で野生戦
	MODE_BGM_CAT,  //BGMテスト: 分類を選ぶ
	MODE_BGM_LIST, //BGMテスト: 分類の中の曲を選んで鳴らす
	MODE_PHONE,    //電話帳の相手を選んで、その人から着信させる
	MODE_TRAINER,  //トレーナーの番号を入れて その場で戦う（2026-10-26 FEAT-debug-trainer-battle・連れ歩き中でも そのまま戦闘に入る＝#76 の相方の試験用）
	MODE_BLENDER,  //きのみブレンダー: 人数（1・2・3 にん・めいじん）を選んで直接開く（FEAT-pokeblock 段 4b1）
};

enum
{
	ITEM_TOP_CATCH,
	ITEM_TOP_ITEM,
	ITEM_TOP_WARP,
	ITEM_TOP_HEAL,
	ITEM_TOP_MONEY,
	ITEM_TOP_BADGES,
	ITEM_TOP_FLAG,
	ITEM_TOP_NOWILD,
	ITEM_TOP_INVINCIBLE,
	ITEM_TOP_OHKO,
	ITEM_TOP_NOTRAINER,
	ITEM_TOP_MOEMON,
	ITEM_TOP_SHINY,
	ITEM_TOP_EDIT,
	ITEM_TOP_GETMON,
	ITEM_TOP_SHOP,
	ITEM_TOP_LV200,
	ITEM_TOP_GENDER,
	ITEM_TOP_DEX,
	ITEM_TOP_FLY,
	ITEM_TOP_WATASHI,
	ITEM_TOP_WILD,
	ITEM_TOP_CLEAR,
	ITEM_TOP_LUXRAY,      //2026-10-18 さいしょの ポケモン レントラー（切り替え・旗 0xF13）
	ITEM_TOP_CATCHINFO,   //2026-10-16 戦闘中の捕獲情報（patches/0425・切り替え）。既存の番号を動かさないよう とじる の手前に
	ITEM_TOP_BGM,         //2026-10-20 BGMテスト（分類ごとの曲を選んで鳴らす・src/hack_debug_bgm.h）
	ITEM_TOP_KAIZOU,      //2026-10-21 かいぞう チェック（RAM チート検出を 1 回走らせて結果の文を出す・印は付けない・src/hack_ram_cheat.c）
	ITEM_TOP_PHONE,       //電話帳の相手から着信（登録していなくても。src/hack_phone.c）
	ITEM_TOP_TRAINER,     //2026-10-26 トレーナーの番号を入れて その場で戦う（FEAT-debug-trainer-battle）
	ITEM_TOP_BLENDER,     //2026-11-09 きのみブレンダーを直接開く（1・2・3 にん・めいじん・FEAT-pokeblock 段 4b1・src/hack_berry_blender.c）
	ITEM_TOP_CLOSE,
	ITEM_TOP_COUNT,
};

//タスクの data
#define tWindow   data[0]
#define tCursor   data[1]
#define tMode     data[2]
#define tValue    data[3]   //アイテムの番号・町・フラグの番号
#define tCount    data[4]   //アイテムの個数
#define tField    data[5]   //アイテムの画面で 0 = 番号・1 = 個数
#define tResult   data[6]   //アイテム: 1 もらった・2 失敗
#define tScroll   data[7]   //いちばん上の一覧の先頭の行
#define tSlot     data[8]   //手持ちの何番目
#define tPurpose  data[9]   //手持ちを選ぶ画面の用事（ITEM_TOP_MOEMON など）
#define tEScroll  data[10]  //手持ち編集の先頭の行
#define tMsg      data[11]  //下の行に出す文の番号（0 = 操作の説明）
#define tShiny    data[12]  //やせいを だす: 色違い 0/1
#define tMoemon   data[13]  //やせいを だす: Moémon 0/1
#define tBgmTouched data[12] //BGMテスト（tShiny と同じ枠・別の画面では使わない）: 1 = 曲を鳴らした（BGM の再生器を入れ替えた）→ 出るとき地図の曲に戻す
#define tBgmNow   data[13]  //BGMテスト（tMoemon と同じ枠）: いま鳴らしている曲の番号 + 1（0 = 鳴らしていない）
#define tHelp     data[14]  //開く前の gHelpSystemEnabled
#define tWaitA    data[15]  //開いた B（2026-10-12 まで A）を離すまで待つ

extern const u8 gText_HackDebug_Title[], gText_HackDebug_Cursor[];
extern const u8 gText_HackDebug_Catch[], gText_HackDebug_Item[], gText_HackDebug_Warp[], gText_HackDebug_Heal[];
extern const u8 gText_HackDebug_Money[], gText_HackDebug_Badges[], gText_HackDebug_Flag[], gText_HackDebug_Close[];
extern const u8 gText_HackDebug_CatchOn[], gText_HackDebug_CatchOff[];
extern const u8 gText_HackDebug_ItemNumber[], gText_HackDebug_ItemNumberSel[], gText_HackDebug_ItemCount[], gText_HackDebug_ItemCountSel[];
extern const u8 gText_HackDebug_ItemUnused[], gText_HackDebug_HelpUpDown[], gText_HackDebug_HelpStep[], gText_HackDebug_HelpBig[];
extern const u8 gText_HackDebug_ItemGot[], gText_HackDebug_ItemFail[], gText_HackDebug_ItemHelp[];
extern const u8 gText_HackDebug_WarpTitle[], gText_HackDebug_WarpHelp[];
extern const u8 gText_HackDebug_FlagTitle[], gText_HackDebug_On[], gText_HackDebug_Off[], gText_HackDebug_FlagHelp[];
extern const u8 gText_HackDebug_Healed[], gText_HackDebug_MoneyDone[], gText_HackDebug_BadgesDone[];
extern const u8 gText_HackDebug_Fly[], gText_HackDebug_FlyDone[];
extern const u8 gText_HackDebug_Wild[], gText_HackDebug_WildHelp[], gText_HackDebug_WildShiny[], gText_HackDebug_WildMoemon[], gText_HackDebug_WildYes[], gText_HackDebug_WildNo[];
extern const u8 SystemScript_StartDexNavBattle[];   //assembly/overworld_scripts/system_scripts.s（lockall → 連れ歩きの位置直し → dowildbattle → releaseall）
extern const u8 gText_HackDebug_Watashi[], gText_HackDebug_WatashiHelp[], gText_HackDebug_Clear[], gText_HackDebug_ClearDone[];
extern const u8 gText_Watashi_Liru_Defeat[], gText_Watashi_Ui_Defeat[], gText_Watashi_Shidare_Defeat[], gText_Watashi_Milk_Defeat[], gText_Watashi_Nao_Defeat[];
extern const u8 gText_Watashi_Kairi_Defeat[], gText_Watashi_Rikka_Defeat[], gText_Watashi_Shii_Defeat[], gText_Watashi_Ran_Defeat[], gText_Watashi_Ichigo_Defeat[];
extern const u8 gText_HackDebug_WarpGroup[], gText_HackDebug_WarpGroupSel[], gText_HackDebug_WarpMap[], gText_HackDebug_WarpMapSel[];
extern const u8 gText_HackDebug_WarpNoName[], gText_HackDebug_WarpHelpUD[];
extern const u8 gText_HackDebug_NoWild[], gText_HackDebug_Invincible[], gText_HackDebug_Ohko[], gText_HackDebug_NoTrainer[];
extern const u8 gText_HackDebug_CatchInfo[], gText_HackDebug_Luxray[];
extern const u8 gText_HackDebug_Kaizou[], gText_HackDebug_KaizouOk[], gText_HackDebug_KaizouNg[], gText_HackDebug_KaizouMoney[], gText_HackDebug_KaizouCoin[], gText_HackDebug_KaizouParty[], gText_HackDebug_KaizouBox[];
extern const u8 gText_HackDebug_Moemon[], gText_HackDebug_Shiny[], gText_HackDebug_Edit[], gText_HackDebug_GetMon[];
extern const u8 gText_HackDebug_Shop[], gText_HackDebug_Lv200[], gText_HackDebug_Dex[], gText_HackDebug_TopHelp[];
extern const u8 gText_HackDebug_Phone[], gText_HackDebug_PhoneHelp[];
extern const u8 gText_HackDebug_Trainer[], gText_HackDebug_TrainerHelp[], gText_HackDebug_TrainerDefeat[], gText_HackDebug_TrainerClass[];
extern const u8 gText_HackDebug_Blender[], gText_HackDebug_BlenderHelp[], gText_HackDebug_Blender1[], gText_HackDebug_Blender2[], gText_HackDebug_Blender3[], gText_HackDebug_BlenderMaster[];
void HackBlender_Start(u8 opponents, bool8 master, MainCallback exitCallback);   //src/hack_berry_blender.c
u8 HackPhone_ContactCount(void);
const u8 *HackPhone_ContactName(u8 index);
const u8 *HackPhone_IncomingScript(u8 index);
extern const u8 gText_HackDebug_PickHelp[], gText_HackDebug_Lv[], gText_HackDebug_MarkShiny[], gText_HackDebug_MarkMoemon[];
extern const u8 gText_HackDebug_Changed[], gText_HackDebug_EditHelp[], gText_HackDebug_GetMonHelp[];
extern const u8 gText_HackDebug_GotParty[], gText_HackDebug_GotBox[], gText_HackDebug_GotFail[];
extern const u8 gText_HackDebug_ShopRange[], gText_HackDebug_ShopHelp[], gText_HackDebug_ShopEmpty[];
extern const u8 gText_HackDebug_Lv200Done[], gText_HackDebug_DexDone[], gText_HackDebug_NoMon[];
extern const u8 gText_HackDebug_ESpecies[], gText_HackDebug_ELevel[], gText_HackDebug_ENature[], gText_HackDebug_EAbility[], gText_HackDebug_EItem[];
extern const u8 gText_HackDebug_EMove1[], gText_HackDebug_EMove2[], gText_HackDebug_EMove3[], gText_HackDebug_EMove4[];
extern const u8 gText_HackDebug_EIvHp[], gText_HackDebug_EIvAtk[], gText_HackDebug_EIvDef[], gText_HackDebug_EIvSpe[], gText_HackDebug_EIvSpa[], gText_HackDebug_EIvSpd[];
extern const u8 gText_HackDebug_EEvHp[], gText_HackDebug_EEvAtk[], gText_HackDebug_EEvDef[], gText_HackDebug_EEvSpe[], gText_HackDebug_EEvSpa[], gText_HackDebug_EEvSpd[];
extern const u8 gText_HackDebug_Gender[], gText_HackDebug_GenderBoy[], gText_HackDebug_GenderGirl[];
extern const u8 gText_HackDebug_EDmPower[], gText_HackDebug_EDmType1[], gText_HackDebug_EDmType2[], gText_HackDebug_EDmAbility[];
extern const u8 gText_HackDebug_DmMax[], gText_HackDebug_DmMin[], gText_HackDebug_DmDefault[];
extern const u8 gText_HackDebug_EKind[], gText_HackDebug_EKindValue[], gText_HackDebug_KindNone[], gText_HackDebug_KindNushi[], gText_HackDebug_KindDelta[], gText_HackDebug_KindDark[];
/* gTypeNames は data2.h（defines_battle.h 経由）の宣言が [7] だが、実際の表は 1 つ 6 バイト（TYPE_NAME_LENGTH 5 + 終わり・strings/type_names.string）
   なので、添字では引かず 6 バイトずつ数える（move_menu.c／bw_option_menu.c の宣言は [TYPE_NAME_LENGTH + 1]） */
extern const u8 gTypeNames[][TYPE_NAME_LENGTH + 1];   /* src/bw_option_menu.c と同じ宣言（data2.h は include の守りで読まれない） */
#define DEBUG_TYPE_NAME(type) (gTypeNames[(type)])
extern u16 GetPlayerAvatarGraphicsIdByCurrentState(void);   //src/character_customization.c（今の状態＝歩き・自転車・なみのり… と gPlayerAvatar->gender の絵）
extern u8 gMoveNames[][MOVE_NAME_LENGTH + 1];

static const struct WindowTemplate sDebugWindow = {0, 1, 1, 28, 18, 15, 0x100};    //元と同じ

static const u8* const sTopLabels[ITEM_TOP_COUNT] =
{
	gText_HackDebug_Catch, gText_HackDebug_Item, gText_HackDebug_Warp, gText_HackDebug_Heal,
	gText_HackDebug_Money, gText_HackDebug_Badges, gText_HackDebug_Flag,
	gText_HackDebug_NoWild, gText_HackDebug_Invincible, gText_HackDebug_Ohko, gText_HackDebug_NoTrainer,
	gText_HackDebug_Moemon, gText_HackDebug_Shiny, gText_HackDebug_Edit, gText_HackDebug_GetMon,
	gText_HackDebug_Shop, gText_HackDebug_Lv200, gText_HackDebug_Gender, gText_HackDebug_Dex, gText_HackDebug_Fly,
	gText_HackDebug_Watashi, gText_HackDebug_Wild, gText_HackDebug_Clear, gText_HackDebug_Luxray, gText_HackDebug_CatchInfo, sBgmTextTitle, gText_HackDebug_Kaizou, gText_HackDebug_Phone, gText_HackDebug_Trainer, gText_HackDebug_Blender, gText_HackDebug_Close,
};

//切り替えの行の旗（0 = 切り替えではない）
static const u16 sTopFlags[ITEM_TOP_COUNT] =
{
	[ITEM_TOP_CATCH] = HACK_FLAG_DEBUG_CATCH,
	[ITEM_TOP_NOWILD] = HACK_FLAG_DEBUG_NOWILD,
	[ITEM_TOP_INVINCIBLE] = HACK_FLAG_DEBUG_INVINCIBLE,
	[ITEM_TOP_OHKO] = HACK_FLAG_DEBUG_OHKO,
	[ITEM_TOP_NOTRAINER] = HACK_FLAG_DEBUG_NOTRAINER,
	[ITEM_TOP_LUXRAY] = HACK_FLAG_DEBUG_LUXRAY,         //2026-10-18 さいしょの ポケモン レントラー
	[ITEM_TOP_CATCHINFO] = HACK_FLAG_DEBUG_CATCHINFO,   //2026-10-16 戦闘中の捕獲情報（patches/0425）
};

static const u8* const sEditLabels[EDIT_FIELDS] =
{
	gText_HackDebug_ESpecies, gText_HackDebug_ELevel, gText_HackDebug_ENature, gText_HackDebug_EAbility, gText_HackDebug_EItem,
	gText_HackDebug_EMove1, gText_HackDebug_EMove2, gText_HackDebug_EMove3, gText_HackDebug_EMove4,
	gText_HackDebug_EIvHp, gText_HackDebug_EIvAtk, gText_HackDebug_EIvDef, gText_HackDebug_EIvSpe, gText_HackDebug_EIvSpa, gText_HackDebug_EIvSpd,
	gText_HackDebug_EEvHp, gText_HackDebug_EEvAtk, gText_HackDebug_EEvDef, gText_HackDebug_EEvSpe, gText_HackDebug_EEvSpa, gText_HackDebug_EEvSpd,
	gText_HackDebug_EDmPower, gText_HackDebug_EDmType1, gText_HackDebug_EDmType2, gText_HackDebug_EDmAbility,
	gText_HackDebug_EKind, gText_HackDebug_EKindValue,
};

//下の行の文（tMsg の番号）
static const u8* const sMessages[] =
{
	NULL, gText_HackDebug_Changed, gText_HackDebug_GotParty, gText_HackDebug_GotBox, gText_HackDebug_GotFail,
	gText_HackDebug_ShopEmpty, gText_HackDebug_NoMon,
};
enum { MSG_NONE, MSG_CHANGED, MSG_GOT_PARTY, MSG_GOT_BOX, MSG_GOT_FAIL, MSG_SHOP_EMPTY, MSG_NO_MON };


static void Task_HackDebugMenu(u8 taskId);

static void Line(u8 window, u8 y, const u8* str)
{
	AddTextPrinterParameterized(window, 2, str, 8, y, 0xFF, NULL);
}

static void Number(u8 window, u8 y, u32 n)
{
	u8 s[8];
	u32 power = 10000;
	u32 j;

	for (j = 0; j < 5; ++j)
	{
		s[j] = 0xA1;            //「0」
		while (n >= power)
		{
			n -= power;
			s[j]++;
		}
		power /= 10;
	}
	s[5] = EOS;
	Line(window, y, s);
}

static void Message(u8 window, const u8* str)
{
	FillWindowPixelRect(window, 0x11, 0, 128, 224, 16);     //下の行を塗りなおす（前の説明の字「:」などが重なって出た）
	Line(window, 128, str);
	CopyWindowToVram(window, 2);
}

static void LineAt(u8 window, u8 x, u8 y, const u8* str)
{
	AddTextPrinterParameterized(window, 2, str, x, y, 0xFF, NULL);
}

static void NumberAt(u8 window, u8 x, u8 y, u32 n, u8 digits)
{
	u8 s[12];
	ConvertIntToDecimalStringN(s, n, STR_CONV_MODE_LEADING_ZEROS, digits);
	LineAt(window, x, y, s);
}

static bool8 IsUsableMon(u8 slot)
{
	return slot < PARTY_SIZE && GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES, NULL) != SPECIES_NONE
	    && !GetMonData(&gPlayerParty[slot], MON_DATA_IS_EGG, NULL);
}

/* ---- BGMテスト（2026-10-20 ユーザー「デバッグにBGMテストほしいでー」「戦闘用とかわけてな」）----
   曲の一覧は src/hack_debug_bgm.h（曲の表 assembly/bgm/song_table.s から tools/bgm/build_debug_bgm.py が生成）。
   鳴らすのは素の PlayBGM（m4aSongNumStart）かファンファーレ・ME 用の m4aSongNumStart だけ＝曲の表の枠で呼ぶ普通の道
   （SuperCard の曲の窓 src/hack_music_window.c が m4aSongNumStart／Stop／Continue を包んでいるので、split ROM でも窓へ SD から読んでから鳴る）。
   ・曲の見出し（songHeader）・再生器の中身には触らない。止めるのは m4aMPlayStop（一時停止）だけ
   ・ファンファーレ・ME（枠の ms が BGM の再生器でないもの）は、BGM の再生器を止めてから自分の再生器で鳴らす（素の PlayFanfareByFanfareNum と同じ手順。
     Task_Fanfare は作らない＝あとで BGM を勝手に再開しない）
   ・出るとき（B で分類の画面から戻る）に、曲を鳴らしていれば PlayBGM(GetCurrentMapMusic()) で地図の曲に戻す
     （GetCurrentMapMusic は地図の曲の番号＝PlayBGM で直に鳴らしても変わらない）
   状態はタスクの data（6.13）: tBgmTouched・tBgmNow */

static u16 BgmSongOf(u32 idx)
{
	return sBgmSongNum[idx] & ~HACK_BGM_OTHER_PLAYER;
}

//BGM の再生器とファンファーレ・ME の再生器（SE2・SE3）を止める（一時停止。戻すときは MPlayStart が止まった状態から始める）
static void BgmStopAll(void)
{
	m4aMPlayStop(&gMPlayInfo_BGM);
	m4aMPlayStop(&gMPlayInfo_SE2);
	m4aMPlayStop(&gMPlayInfo_SE3);
}

//一覧の idx 番目を鳴らす。先に止めるのは、優先度の高い曲が鳴り続けていて新しい曲が始まらないのを避けるため
static void BgmPlay(u32 idx)
{
	BgmStopAll();
	if (sBgmSongNum[idx] & HACK_BGM_OTHER_PLAYER)
		m4aSongNumStart(BgmSongOf(idx));
	else
		PlayBGM(BgmSongOf(idx));
}

//地図の曲に戻す（鳴らしたファンファーレ・ME も止める）
static void BgmRestoreMapMusic(void)
{
	BgmStopAll();
	PlayBGM(GetCurrentMapMusic());
}

static u32 BgmCatSize(u32 cat)
{
	return sBgmCatStart[cat + 1] - sBgmCatStart[cat];
}

/* ---- 手持ち編集（2026-10-12）---- */

static s32 EditGet(struct Pokemon* mon, u8 f)
{
	if (f >= EDIT_KIND_FIRST)
		return (f == EDIT_KIND_FIRST) ? HackKind_Get((struct BoxPokemon*) mon) : HackKind_Value((struct BoxPokemon*) mon);
	if (f >= EDIT_DM_FIRST)
		return HackDebugMon_EditGet(mon, f - EDIT_DM_FIRST);   //デバッグモン以外は 0
	switch (f)
	{
		case 0: return GetMonData(mon, MON_DATA_SPECIES, NULL);
		case 1: return GetMonData(mon, MON_DATA_LEVEL, NULL);
		case 2: return mon->natureMint != 0 ? mon->natureMint - 1 : GetNature(mon);
		case 3: return mon->hiddenAbility ? 2 : GetMonData(mon, MON_DATA_ALT_ABILITY, NULL);
		case 4: return GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
	}
	if (f < 9)
		return GetMonData(mon, MON_DATA_MOVE1 + f - 5, NULL);
	if (f < 15)
		return HackMon_GetIv(mon, f - 9);
	return HackMon_GetEv(mon, f - 15);
}

static s32 EditMax(u8 f)
{
	if (f >= EDIT_KIND_FIRST)
		return (f == EDIT_KIND_FIRST) ? HACK_KIND_DARK : 255;
	if (f >= EDIT_DM_FIRST)
	{
		switch (f - EDIT_DM_FIRST)
		{
			case 0: return 1;                                   //0 全部255・1 全部1
			case 1:
			case 2: return HackDebugMon_TypeCount() - 1;        //選べるタイプ 19 種（リルを含む）の添字
			default: return ABILITIES_COUNT - 1;                //0 は種族の標準
		}
	}
	switch (f)
	{
		case 0: return NUM_SPECIES - 1;
		case 1: return MAX_LEVEL;
		case 2: return NUM_NATURES - 1;
		case 3: return 2;
		case 4: return ITEMS_COUNT - 1;
	}
	if (f < 9)
		return MOVES_COUNT - 1;
	if (f < 15)
		return 31;
	return EV_CAP;
}

static s32 EditMin(u8 f)
{
	return (f <= 1) ? 1 : 0;
}

//欄 f を v に（すぐ書いて能力を計算し直す）。性格値は触らない（性格はミント・特性は枠の番号）
static void EditSet(struct Pokemon* mon, u8 f, s32 v)
{
	u32 x = v;
	u16 species;

	switch (f)
	{
		case 0:
			SetMonData(mon, MON_DATA_SPECIES, &x);
			break;
		case 1:
			species = GetMonData(mon, MON_DATA_SPECIES, NULL);
			x = gExperienceTables[gBaseStats[species].growthRate][v];
			SetMonData(mon, MON_DATA_EXP, &x);
			break;
		case 2:
			mon->natureMint = v + 1;
			break;
		case 3:
			x = (v == 1);
			mon->hiddenAbility = (v == 2);
			SetMonData(mon, MON_DATA_ALT_ABILITY, &x);
			break;
		case 4:
			SetMonData(mon, MON_DATA_HELD_ITEM, &x);
			break;
		default:
			if (f >= EDIT_KIND_FIRST)
			{
				if (f == EDIT_KIND_FIRST)
					HackKind_Set((struct BoxPokemon*) mon, v, HackKind_Value((struct BoxPokemon*) mon));   //しるしを変えても値はそのまま（種類 なし にすると値は消える）
				else
					HackKind_SetValue((struct BoxPokemon*) mon, v);                                        //種類が なし のあいだは何もしない
			}
			else if (f >= EDIT_DM_FIRST)
				HackDebugMon_EditSet(mon, f - EDIT_DM_FIRST, v);   //デバッグモンのつよさ（種族の入れ替え）・タイプ・特性（性格値に書く＝hack_debug_mon.c）
			else if (f < 9)
				SetMonMoveSlot(mon, v, f - 5);
			else if (f < 15)
				HackMon_SetIv(mon, f - 9, v);
			else
				HackMon_SetEv(mon, f - 15, v);
			break;
	}
	species = GetMonData(mon, MON_DATA_SPECIES, NULL);
	if (mon->hiddenAbility && gBaseStats[species].hiddenAbility == ABILITY_NONE)   //5e: 種族を変えて特性の枠が無くなったら 1 つ目に
		mon->hiddenAbility = FALSE;
	CalculateMonStats(mon);
}

//欄の横に出す名前（無ければ NULL）
static const u8* EditName(struct Pokemon* mon, u8 f, s32 v, u8* buf)
{
	if (f >= EDIT_KIND_FIRST)
	{
		static const u8* const kindNames[] = {gText_HackDebug_KindNone, gText_HackDebug_KindNushi, gText_HackDebug_KindDelta, gText_HackDebug_KindDark};

		return (f == EDIT_KIND_FIRST && v >= 0 && v <= HACK_KIND_DARK) ? kindNames[v] : NULL;
	}
	if (f >= EDIT_DM_FIRST)             //デバッグモンの欄（デバッグモン以外の子には何も出さない）
	{
		if (!HackDebugMon_IsMon(mon))
			return NULL;
		switch (f - EDIT_DM_FIRST)
		{
			case 0: return v ? gText_HackDebug_DmMin : gText_HackDebug_DmMax;
			case 1:
			case 2: return DEBUG_TYPE_NAME(HackDebugMon_TypeOfIndex(v));
			default: return v == 0 ? gText_HackDebug_DmDefault : GetAbilityName(v);
		}
	}
	switch (f)
	{
		case 0: return gSpeciesNames[v];
		case 2: return NATURE_NAMES[v];
		case 3: return GetAbilityName(GetMonAbility(mon));
		case 4:
			if (v == 0)
				return NULL;
			CopyItemName(v, buf);
			return buf;
	}
	if (f >= 5 && f < 9 && v != 0)
		return gMoveNames[v];
	return NULL;
}

/* ---- ショップ（道具の番号の 50 個ずつ・2026-10-12 5e「案 A」）---- */

void HackDebugMenu_OpenShop(void)          //RAM に組んだ台本の callasm から: Var8004／Var8005 が品の表の番地
{
	CREATE_POKEMART_MENU((const u16*) (Var8004 | ((u32) Var8005 << 16)));
}

//品の表と台本を組む（流すのはメニューを閉じてから）。品が 1 つも無ければ NULL
static u8* BuildShop(u16 page)
{
	u8* buf = Malloc(8 + 2 * (SHOP_PAGE + 1));
	u16* list;
	u32 fn = (u32) HackDebugMenu_OpenShop | 1;
	u16 id, n = 0;
	u8 name[64];

	if (buf == NULL)
		return NULL;
	list = (u16*) (buf + 8);
	for (id = 1 + page * SHOP_PAGE; id < ITEMS_COUNT && id < 1 + (page + 1) * SHOP_PAGE; ++id)
	{
		CopyItemName(id, name);
		if (name[0] != 0xAC && ItemId_GetPrice(id) != 0)       //「？」の名前（使っていない番号）と値段 0 は外す（6.59）
			list[n++] = id;
	}
	list[n] = ITEM_NONE;
	if (n == 0)
	{
		Free(buf);
		return NULL;
	}
	buf[0] = 0x23;                          //callasm（callnative）
	buf[1] = fn;
	buf[2] = fn >> 8;
	buf[3] = fn >> 16;
	buf[4] = fn >> 24;
	buf[5] = 0x27;                          //waitstate（CreatePokemartMenu が閉じたら EnableBothScriptContexts）
	buf[6] = 0x6C;                          //release
	buf[7] = 0x02;                          //end
	Var8004 = (u32) list;
	Var8005 = (u32) list >> 16;
	return buf;
}

/* ---- 効き目（上流の .c の #ifdef HACK_DEBUG_MENU から・patches/0342）---- */

bool8 HackDebugMenu_NoWild(void)
{
	return FlagGet(HACK_FLAG_DEBUG_NOWILD);
}

bool8 HackDebugMenu_NoTrainer(void)
{
	return FlagGet(HACK_FLAG_DEBUG_NOTRAINER);
}

//atk0C_datahpupdate の頭: 守る側が主人公の側なら 0（むてき）・主人公の側の技で相手の側なら今の HP 全部（いちげき）。通信では効かない
void HackDebugMenu_AdjustDamage(u8 bank)
{
	if (gBattleTypeFlags & BATTLE_TYPE_LINK || gBattleMoveDamage <= 0)
		return;
	if (SIDE(bank) == B_SIDE_PLAYER)
	{
		if (FlagGet(HACK_FLAG_DEBUG_INVINCIBLE))
			gBattleMoveDamage = 0;
	}
	else if (FlagGet(HACK_FLAG_DEBUG_OHKO) && SIDE(gBankAttacker) == B_SIDE_PLAYER
	      && !(gHitMarker & HITMARKER_NON_ATTACK_DMG) && gBattleMons[bank].hp > 0)
		gBattleMoveDamage = gBattleMons[bank].hp;
}

/* ---- 全部の地図（ワープ）---- */

static bool8 IsRomPtr(const void* p)
{
	u32 a = (u32) p;
	return a >= 0x08000000 && a < 0x0A000000 && !(a & 3);
}

static bool8 IsRomText(const u8* p)    //文字列は 4 の倍数に置かれるとは限らない
{
	u32 a = (u32) p;
	return a >= 0x08000000 && a < 0x0A000000 && *p != EOS;
}

static const struct MapHeader* const* const* MapGroups(void)
{
	return *(const struct MapHeader* const* const* const*) MAP_GROUPS_LITERAL;
}

//群の中の地図の数: 次に始まる群の表の手前まで、かつ地図の頭として正しい間（番地が ROM・地形・イベントも ROM）。
//群の地図の数は ROM のどこにも持っていないので、表の並び（群の表は前から続けて置かれる）と中身で数える
static u16 MapCountInGroup(u8 group)
{
	const struct MapHeader* const* const* groups = MapGroups();
	const struct MapHeader* const* list = groups[group];
	u32 limit = 256;
	u32 g, n;

	if (!IsRomPtr(list))
		return 0;
	for (g = 0; g < DEBUG_MAP_GROUP_COUNT; ++g)
	{
		u32 other = (u32) groups[g];
		if (other > (u32) list && (other - (u32) list) / 4 < limit)
			limit = (other - (u32) list) / 4;
	}
	for (n = 0; n < limit; ++n)
	{
		const struct MapHeader* h = list[n];
		if (!IsRomPtr(h) || !IsRomPtr(h->mapLayout) || !IsRomPtr(h->events) || h->mapLayoutId == 0)
			break;
	}
	return n;
}

static const struct MapHeader* DebugMapHeader(u8 group, u8 num)
{
	return MapGroups()[group][num];
}

static bool8 UnovaRomPtr(u32 a)    //ROM の 4 の倍数の番地で、そこから 32 バイト読んでも ROM の外に出ない
{
	return a >= 0x08000000 && a < 0x09FFFF00 && !(a & 3);
}

//イッシュを接ぎ木した版か（tools/unova/graft.py が足した印を読む。足していない版では 0xFF の領域や素の関数なので false）。
//  ① 0x08054AF8（Overworld_GetMapHeaderByGroupAndId）の先頭が「ldr r3,[pc]; bx r3」＋追加領域の関数への飛び先
//  ② その関数の先頭 4 バイトと、群 81・82 の表の番地の定数（関数の先頭＋0x24）
//  ③ 表の 2 本 → 群の表 → 地図ヘッダーの mapLayout が全部 ROM の番地。途中で外れたらその先は読まない
static bool8 DebugUnovaGrafted(void)
{
	const u32* hook = (const u32*) 0x08054AF8;
	u32 fn, tbl, i;
	if (hook[0] != 0x47184B00)
		return FALSE;
	fn = hook[1];
	if (!(fn & 1) || fn < 0x091DE200 || fn >= 0x09200000 || ((fn - 1) & 3))
		return FALSE;
	if (*(const u32*) (fn - 1) != 0x0C000400)
		return FALSE;
	tbl = *(const u32*) (fn - 1 + 0x24);
	if (!UnovaRomPtr(tbl))
		return FALSE;
	for (i = 0; i < 2; ++i)
	{
		u32 grp = ((const u32*) tbl)[i];
		u32 hdr;
		if (!UnovaRomPtr(grp))
			return FALSE;
		hdr = ((const u32*) grp)[0];
		if (!UnovaRomPtr(hdr) || !UnovaRomPtr(*(const u32*) hdr))   //MapHeader の先頭は mapLayout
			return FALSE;
	}
	return TRUE;
}

extern const u8 gText_HackDebug_WarpMenu[], gText_HackDebug_WarpRegions[], gText_HackDebug_WarpGyms[], gText_HackDebug_WarpRaw[];
extern const u8 gText_HackDebug_WarpRegionHelp[], gText_HackDebug_WarpGymHelp[], gText_HackDebug_WarpUnavailable[];
extern const u8 gText_HackDebug_RegionKanto[], gText_HackDebug_RegionHoenn[], gText_HackDebug_RegionJohto[], gText_HackDebug_RegionSinnoh[], gText_HackDebug_RegionKitakami[];
extern const u8 gText_HackDebug_WarpPeople[], gText_HackDebug_WarpPeopleHelp[];
extern const u8 gText_HackDebug_Spot0[], gText_HackDebug_Spot1[], gText_HackDebug_Spot2[], gText_HackDebug_Spot3[], gText_HackDebug_Spot4[], gText_HackDebug_Spot5[];
extern const u8 gText_HackDebug_Spot6[], gText_HackDebug_Spot7[], gText_HackDebug_Spot8[], gText_HackDebug_Spot9[], gText_HackDebug_Spot10[];
extern const u8 gText_HackDebug_Spot11[], gText_HackDebug_Spot12[], gText_HackDebug_Spot13[], gText_HackDebug_Spot14[], gText_HackDebug_Spot15[];
extern const u8 gText_HackDebug_Spot16[], gText_HackDebug_Spot17[], gText_HackDebug_Spot18[], gText_HackDebug_Spot19[], gText_HackDebug_Spot20[], gText_HackDebug_Spot21[];
extern const u8 gText_HackDebug_WarpUnova[];   //イッシュ（tools/unova/graft.py で接ぎ木した版だけ・群 81 の 0 番＝カノコタウン）
extern const u8 gText_HackDebug_WarpRoom[], gText_HackDebug_WarpRoomHelp[], gText_HackDebug_Room0[], gText_HackDebug_Room1[], gText_HackDebug_Room2[], gText_HackDebug_Room3[];
extern const u8 gText_HackDebug_Room4[], gText_HackDebug_Room5[], gText_HackDebug_Room6[], gText_HackDebug_Room7[], gText_HackDebug_Room8[];

static void DebugWarpTo(u8 group, u8 num)
{
	const struct MapHeader* h = DebugMapHeader(group, num);

	if (h->events->warpCount != 0)
		SetWarpDestination(group, num, 0, -1, -1);     //ワープの入口の 0 番
	else
		SetWarpDestination(group, num, -1, h->mapLayout->width / 2, h->mapLayout->height / 2);  //入口が無い地図は真ん中
	DoWarp();
}


/* 地方は既存の飛行・回復地点を再利用。新しい保存領域や進行フラグを使わない。 */
static const u8* const sWarpRegionNames[] = {
 gText_HackDebug_RegionKanto, gText_HackDebug_RegionHoenn, gText_HackDebug_RegionJohto,
 gText_HackDebug_RegionSinnoh, gText_HackDebug_RegionKitakami,
};
/* 素の ROM の WarpEvent は x(2) y(2) 高さ(1) warpId(1) mapNum(1) mapGroup(1)。
   include/global.fieldmap.h の struct WarpEvent は高さが無く並びも違う（warpId が高さ・mapGroup が warpId の位置）ので使わない
   （2026-10-15 ジムワープが 31/32 で動かなかった原因・ROM の素のカントーのジムの出口バイトで確認） */
struct DebugWarpEvent
{
 s16 x, y;
 u8 elevation;
 s8 warpId;
 u8 mapNum;
 u8 mapGroup;
};

static const u16 sKantoGyms[] = {
 MAP_PEWTER_CITY_GYM, MAP_CERULEAN_CITY_GYM, MAP_VERMILION_CITY_GYM, MAP_CELADON_CITY_GYM,
 MAP_FUCHSIA_CITY_GYM, MAP_SAFFRON_CITY_GYM, MAP_CINNABAR_ISLAND_GYM, MAP_VIRIDIAN_CITY_GYM,
};
/* tools/sinnoh/sinnoh_maps.py の地図の番号。ジョウトは「取った順」（tools/johto/johto_gyms.GYMS＝タンバ・アサギ・エンジュ・コガネ・ヒワダ・キキョウ・チョウジ・フスベ・FEAT-johto-p1 2026-11-10）の部屋の番号。検算で追従を確認する。 */
static const u8 sJohtoGyms[] = { 49, 48, 47, 46, 45, 44, 50, 51 };
static const u8 sSinnohGyms[] = { 79, 80, 81, 82, 83, 84, 85, 86 };
/* ホウエン: (群 << 8) | 番号。群 = 43 + エメラルドの群（assembly/hoenn/maps.s の HoennMap_<g>_<n>）。
   並びは カナズミ・ムロ・キンセツ・フエン・トウカ・ヒワマキ・トクサネ・ルネ（検算 verify_debug_warp.py が maps.s の見出しと照合）。
   以前は mapScripts の番地で探したが、ラスボロのジムの地図の mapScripts は HoennScript_Empty で一致せず探せなかった */
static const u16 sHoennGyms[] = {
 (54 << 8) | 3,    /* RustboroCity_Gym   HoennMap_11_3 */
 (46 << 8) | 3,    /* DewfordTown_Gym    HoennMap_3_3 */
 (53 << 8) | 0,    /* MauvilleCity_Gym   HoennMap_10_0 */
 (47 << 8) | 1,    /* LavaridgeTown_Gym_1F HoennMap_4_1 */
 (51 << 8) | 1,    /* PetalburgCity_Gym  HoennMap_8_1 */
 (55 << 8) | 1,    /* FortreeCity_Gym    HoennMap_12_1 */
 (57 << 8) | 0,    /* MossdeepCity_Gym   HoennMap_14_0 */
 (58 << 8) | 0,    /* SootopolisCity_Gym_1F HoennMap_15_0 */
};
extern const u8 gExpandedMapSecFly[];
extern const u8 gHoennMapsecKind[];       /* 253 件。2 + 街の番号（tools/rom/build_hoenn_fly.py） */
extern const u16 gHoennVisitFlagBase[];
#define DEBUG_MAPSEC_TOTAL 253

static bool8 DebugGymMap(u8 region, u8 gym, u8* group, u8* num)
{
 if (region >= MAP_REGION_KITAKAMI || gym >= 8)
  return FALSE;
 if (region == MAP_REGION_KANTO) {
  *group = sKantoGyms[gym] >> 8;
  *num = sKantoGyms[gym] & 0xFF;
 } else if (region == MAP_REGION_JOHTO) {
  *group = JOHTO_MAP_GROUP_FIRST;
  *num = sJohtoGyms[gym];
 } else if (region == MAP_REGION_SINNOH) {
  *group = SINNOH_MAP_GROUP_FIRST;
  *num = sSinnohGyms[gym];
 } else {
  *group = sHoennGyms[gym] >> 8;
  *num = sHoennGyms[gym] & 0xFF;
 }
 return *group < DEBUG_MAP_GROUP_COUNT && *num < MapCountInGroup(*group);
}

static bool8 DebugGymSetWarp(u8 region, u8 gym)
{
 u8 group, num;
 u32 i;
 const struct MapHeader* h;
 const struct DebugWarpEvent* warps;
 if (!DebugGymMap(region, gym, &group, &num))
  return FALSE;
 h = DebugMapHeader(group, num);
 if (!IsRomPtr(h->events->warps))
  return FALSE;
 warps = (const struct DebugWarpEvent*) h->events->warps;
 /* ジムの外へ出る通常の出口を選ぶ。階段や奥の部屋へは飛ばない。 */
 for (i = 0; i < h->events->warpCount; ++i) {
  const struct DebugWarpEvent* e = &warps[i];
  const struct MapHeader* outside;
  if (e->mapGroup >= DEBUG_MAP_GROUP_COUNT || e->mapNum >= MapCountInGroup(e->mapGroup))
   continue;
  outside = DebugMapHeader(e->mapGroup, e->mapNum);
  if ((outside->mapType != MAP_TYPE_TOWN && outside->mapType != MAP_TYPE_CITY)
   || e->warpId < 0 || e->warpId >= outside->events->warpCount
   || GetMapRegionOf(e->mapGroup, e->mapNum) != region)
   continue;
  SetWarpDestination(e->mapGroup, e->mapNum, e->warpId, -1, -1);
  return TRUE;
 }
 return FALSE;
}

/* わたしたち 10 人（りる・うい・しだれ・みるく・なお・かいり・りっか・しぃ・らん・いちご）とミラクル交換の係のところ。
   どちらも版ごとのランダムな場所なので、行き先は決め打ちせず、ビルドが配置を決める関数から作った表（src/hack_debug_spots.h・
   tools/rom/build_debug_spots.py）を読む。行き先は人の隣の歩けるマス。向きは人のほうへ（話しかけられる） */
struct DebugSpot
{
 u8 group, num, x, y, face;
};
#include "hack_debug_spots.h"
static const u8* const sSpotNames[HACK_DEBUG_SPOT_COUNT] = {
 gText_HackDebug_Spot0, gText_HackDebug_Spot1, gText_HackDebug_Spot2, gText_HackDebug_Spot3, gText_HackDebug_Spot4, gText_HackDebug_Spot5,
 gText_HackDebug_Spot6, gText_HackDebug_Spot7, gText_HackDebug_Spot8, gText_HackDebug_Spot9, gText_HackDebug_Spot10,
 gText_HackDebug_Spot11, gText_HackDebug_Spot12, gText_HackDebug_Spot13, gText_HackDebug_Spot14, gText_HackDebug_Spot15,
 gText_HackDebug_Spot16, gText_HackDebug_Spot17, gText_HackDebug_Spot18, gText_HackDebug_Spot19, gText_HackDebug_Spot20,
 gText_HackDebug_Spot21,
};

/* デバッグルーム（群 79 の 1〜9 番・tools/kanto/build_debug_room.py）。地図のつながりは作らず、ここからしか入れない。
   出口は動的ワープ (0x7F,0x7F)＝gSaveBlock1->dynamicWarp へ戻る → 入る前に今の位置（地図・座標）を dynamicWarp に書く。
   すでにデバッグルームの中なら書き換えない（部屋どうしを行き来しても最初の場所へ戻れる）。
   着地: へや1〜7（共有レイアウト）は出口の 2 つ上 (7,14)・へや8 は (15,3)・へや9 は (11,3)（どちらも上の壁のドアの 2 つ下） */
#define DEBUG_ROOM_GROUP 79
#define DEBUG_ROOM_FIRST 1
#define DEBUG_ROOM_COUNT 9
static const u8 sRoomSpawn[DEBUG_ROOM_COUNT][2] = {
 {7, 14}, {7, 14}, {7, 14}, {7, 14}, {7, 14}, {7, 14}, {7, 14}, {15, 3}, {11, 3},
};
static const u8* const sRoomNames[DEBUG_ROOM_COUNT] = {
 gText_HackDebug_Room0, gText_HackDebug_Room1, gText_HackDebug_Room2, gText_HackDebug_Room3, gText_HackDebug_Room4,
 gText_HackDebug_Room5, gText_HackDebug_Room6, gText_HackDebug_Room7, gText_HackDebug_Room8,
};

static bool8 DebugRoomValid(u8 i)
{
 return i < DEBUG_ROOM_COUNT && DEBUG_ROOM_FIRST + i < MapCountInGroup(DEBUG_ROOM_GROUP);
}

static bool8 DebugInRoom(void)
{
 return (u8) gSaveBlock1->location.mapGroup == DEBUG_ROOM_GROUP
     && (u8) gSaveBlock1->location.mapNum >= DEBUG_ROOM_FIRST
     && (u8) gSaveBlock1->location.mapNum < DEBUG_ROOM_FIRST + DEBUG_ROOM_COUNT;
}

static bool8 DebugRoomSetWarp(u8 i)
{
 if (!DebugRoomValid(i))
  return FALSE;
 if (!DebugInRoom())
 {
  gSaveBlock1->dynamicWarp.mapGroup = gSaveBlock1->location.mapGroup;
  gSaveBlock1->dynamicWarp.mapNum = gSaveBlock1->location.mapNum;
  gSaveBlock1->dynamicWarp.warpId = -1;
  gSaveBlock1->dynamicWarp.x = gSaveBlock1->pos.x;     //pos は地図の座標そのまま（+7 の補正はスクリーン座標のとき）
  gSaveBlock1->dynamicWarp.y = gSaveBlock1->pos.y;
 }
 SetWarpDestination(DEBUG_ROOM_GROUP, DEBUG_ROOM_FIRST + i, -1, sRoomSpawn[i][0], sRoomSpawn[i][1]);
 return TRUE;
}

static bool8 DebugSpotValid(u8 i)
{
 return i < HACK_DEBUG_SPOT_COUNT && sDebugSpots[i].group < DEBUG_MAP_GROUP_COUNT && sDebugSpots[i].num < MapCountInGroup(sDebugSpots[i].group);
}

/* 着いたあとの向き（Redmine #260）: 素の GetAdjustedInitialDirection はドア・矢印などの特別な足場以外では StoreInitialPlayerAvatarState で
   覚えさせた向きを使わず南にするため、着地後に 1 回だけ表の向きへ向け直す。
   ワープの終わりの素のコールバック（FieldCB_DefaultWarpExit・DoWarp が gFieldCallback に入れる関数）を先に呼び、そのあとで向きだけを替える。
   向き先は今の位置（地図・x・y）から表を引く＝状態を持たない（新しい旗・変数・RAM を使わない）。表に無い位置なら何もしない。
   番地は CFRU の見出し・リンカの記号に無い＝名前を付けた定数。DoWarp が入れる値と同じことは tools/rom/verify_debug_spots.py が ROM から突き合わせる */
#define DEBUG_FIELDCB_DEFAULT_WARP_EXIT 0x0807D695
static void DebugSpotFieldCallback(void)
{
 struct EventObject* o;
 u8 i;
 ((void (*)(void)) DEBUG_FIELDCB_DEFAULT_WARP_EXIT)();
 o = &gEventObjects[gPlayerAvatar->eventObjectId];
 for (i = 0; i < HACK_DEBUG_SPOT_COUNT; ++i) {
  const struct DebugSpot* s = &sDebugSpots[i];
  if (s->group == (u8) gSaveBlock1->location.mapGroup && s->num == (u8) gSaveBlock1->location.mapNum
   && s->x == o->currentCoords.x - 7 && s->y == o->currentCoords.y - 7) {
   EventObjectTurn(o, s->face);
   break;
  }
 }
}

static bool8 DebugSpotSetWarp(u8 i)
{
 const struct DebugSpot* s;
 if (!DebugSpotValid(i))
  return FALSE;
 s = &sDebugSpots[i];
 /* 着いたとき人のほうを向かせる: いまの主人公を向けて、その向きを「ワープ前の姿」として覚えさせる（FollowHiddenGrottoWarp と同じ手順） */
 EventObjectTurn(&gEventObjects[gPlayerAvatar->eventObjectId], s->face);
 StoreInitialPlayerAvatarState();
 SetWarpDestination(s->group, s->num, -1, s->x, s->y);
 return TRUE;
}

static bool8 DebugRegionSetWarp(u8 region)
{
 u32 i;
 if (region == MAP_REGION_KANTO || region == MAP_REGION_HOENN) {
  /* 拡張された通常の飛行先表のマサラ／ミシロを使用する。 */
  u32 sec = MAPSEC_PALLET_TOWN;
  u8 heal;
  if (region == MAP_REGION_HOENN) {
   /* ホウエンの地名の番号はカントー側の番号に振り直してある（ミシロは 0x1B・0 番はバトルフロンティア）。
      gHoennMapsecKind は「2 + 街の番号」で、ミシロは街の 0 番 = 2（tools/rom/build_hoenn_fly.py の HOENN_CITIES 先頭） */
   for (sec = 0; sec < DEBUG_MAPSEC_TOTAL; ++sec)
    if (gHoennMapsecKind[sec] == 2)
     break;
   if (sec == DEBUG_MAPSEC_TOTAL)
    return FALSE;
  }
  heal = gExpandedMapSecFly[sec * 3 + 2];
  if (heal == 0)
   return FALSE;
  ((void (*)(u8)) (0x08054D2C | 1))(heal);
  return TRUE;
 }
 for (i = 0; i < gRegionFlyCityCount[0]; ++i) {
  const struct RegionFlyCity* c = &gRegionFlyCities[i];
  if (c->region == region && c->group < DEBUG_MAP_GROUP_COUNT
   && c->num < MapCountInGroup(c->group)) {
   SetWarpDestination(c->group, c->num, -1, c->x, c->y);
   return TRUE;
  }
 }
 return FALSE;
}

/* そらをとぶ行き先の旗（訪問済み）を全地方ぶん立てる。行き先の旗と、地図の切り替え画面を開く旗だけ（ほかのイベントの旗は触らない）。
   ・カントー・ナナシマ: 素の GetMapsecType（080C4994〜080C4A50）が見る 0x890〜0x8A3 の 20 個＋ナナシマの地図の旗 0x845／0x846
   ・ホウエン: gHoennVisitFlagBase + (gHoennMapsecKind - 2)（17 街）
   ・ジョウト・シンオウ・キタカミ: gRegionFlyCities の行ごとの flag ＋ 切り替え画面の旗 JOHTO/SINNOH_FLAG_TOWNMAP
   ジョウトは旗を立てるだけ（0.45.1 で通常移動を止めている HACK_OMIT_JOHTO は、そらをとぶ側が旗に関係なく止める） */
static void DebugUnlockFlySpots(void)
{
 u32 i;
 for (i = 0x890; i <= 0x8A3; ++i)
  FlagSet(i);
 FlagSet(FLAG_SYS_SEVII_MAP_123);
 FlagSet(FLAG_SYS_SEVII_MAP_4567);
 for (i = 0; i < DEBUG_MAPSEC_TOTAL; ++i)
  if (gHoennMapsecKind[i] >= 2)
   FlagSet((u16) (gHoennVisitFlagBase[0] + gHoennMapsecKind[i] - 2));
 for (i = 0; i < gRegionFlyCityCount[0]; ++i)
  FlagSet(gRegionFlyCities[i].flag);
 FlagSet(JOHTO_FLAG_TOWNMAP);
 FlagSet(SINNOH_FLAG_TOWNMAP);
}

static void Draw(u8 taskId)
{
	s16* data = gTasks[taskId].data;
	u8 w = tWindow;
	u32 j;

	FillWindowPixelBuffer(w, 0x11);
	Line(w, 0, gText_HackDebug_Title);

	if (tMode == MODE_TOP)
	{
		for (j = 0; j < TOP_ROWS && tScroll + j < ITEM_TOP_COUNT; ++j)
		{
			u32 k = tScroll + j;
			Line(w, 16 + j * 12, sTopLabels[k]);
			if (sTopFlags[k])
				LineAt(w, 176, 16 + j * 12, FlagGet(sTopFlags[k]) ? gText_HackDebug_On : gText_HackDebug_Off);
			if (k == ITEM_TOP_GENDER)
				LineAt(w, 150, 16 + j * 12, gSaveBlock2->playerGender == FEMALE ? gText_HackDebug_GenderGirl : gText_HackDebug_GenderBoy);
			if (k == (u32) tCursor)
				AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, 16 + j * 12, 0xFF, NULL);
		}
		Line(w, 128, tMsg ? sMessages[tMsg] : gText_HackDebug_TopHelp);
	}
	else if (tMode == MODE_PHONE)
	{
		u8 n = HackPhone_ContactCount();

		for (j = 0; j < TOP_ROWS && tScroll + j < n; ++j)
		{
			u32 k = tScroll + j;

			Line(w, 16 + j * 12, HackPhone_ContactName(k));
			if (k == (u32) tCursor)
				AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, 16 + j * 12, 0xFF, NULL);
		}
		Line(w, 128, gText_HackDebug_PhoneHelp);
	}
	else if (tMode == MODE_BLENDER)
	{
		static const u8* const sBlenderLabels[4] = { gText_HackDebug_Blender1, gText_HackDebug_Blender2, gText_HackDebug_Blender3, gText_HackDebug_BlenderMaster };

		for (j = 0; j < 4; ++j)
		{
			Line(w, 16 + j * 12, sBlenderLabels[j]);
			if (j == tCursor)
				AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, 16 + j * 12, 0xFF, NULL);
		}
		Line(w, 128, gText_HackDebug_BlenderHelp);
	}
	else if (tMode == MODE_PICK)
	{
		Line(w, 14, sTopLabels[tPurpose]);
		for (j = 0; j < PARTY_SIZE; ++j)
		{
			u8 y = 30 + j * 14;
			if (!IsUsableMon(j))
				continue;
			LineAt(w, 16, y, gSpeciesNames[GetMonData(&gPlayerParty[j], MON_DATA_SPECIES, NULL)]);
			LineAt(w, 100, y, gText_HackDebug_Lv);
			NumberAt(w, 116, y, GetMonData(&gPlayerParty[j], MON_DATA_LEVEL, NULL), 3);
			if (HackMonId_IsShinyBox((struct BoxPokemon*) &gPlayerParty[j]))
				LineAt(w, 150, y, gText_HackDebug_MarkShiny);
			if (HackMonId_IsMoemonHit((struct BoxPokemon*) &gPlayerParty[j]))
				LineAt(w, 186, y, gText_HackDebug_MarkMoemon);
			if (j == (u32) tSlot)
				AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 4, y, 0xFF, NULL);
		}
		Line(w, 128, tMsg ? sMessages[tMsg] : gText_HackDebug_PickHelp);
	}
	else if (tMode == MODE_EDIT)
	{
		struct Pokemon* mon = &gPlayerParty[tSlot];
		u8 buf[64];
		for (j = 0; j < EDIT_ROWS && tEScroll + j < EDIT_FIELDS; ++j)
		{
			u8 f = tEScroll + j, y = 14 + j * 14;
			s32 v = EditGet(mon, f);
			const u8* name = EditName(mon, f, v, buf);
			Line(w, y, sEditLabels[f]);
			NumberAt(w, 80, y, v, 4);
			if (name != NULL)
				LineAt(w, 120, y, name);
			if (f == (u32) tField)
				AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, y, 0xFF, NULL);
		}
		Line(w, 128, gText_HackDebug_EditHelp);
	}
	else if (tMode == MODE_GETMON)
	{
		Line(w, 16, sTopLabels[ITEM_TOP_GETMON]);
		AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, tField ? 62 : 34, 0xFF, NULL);
		Line(w, 34, gText_HackDebug_ESpecies);
		NumberAt(w, 80, 34, tValue, 4);
		LineAt(w, 120, 34, gSpeciesNames[tValue]);
		Line(w, 62, gText_HackDebug_ELevel);
		NumberAt(w, 80, 62, tCount, 3);
		Line(w, 90, gText_HackDebug_HelpBig);
		Line(w, 128, tMsg ? sMessages[tMsg] : gText_HackDebug_GetMonHelp);
	}
	else if (tMode == MODE_SHOP)
	{
		u32 first = 1 + tValue * SHOP_PAGE;
		Line(w, 16, sTopLabels[ITEM_TOP_SHOP]);
		Line(w, 40, gText_HackDebug_ShopRange);
		NumberAt(w, 16, 56, first, 4);
		NumberAt(w, 80, 56, first + SHOP_PAGE - 1 < ITEMS_COUNT - 1 ? first + SHOP_PAGE - 1 : ITEMS_COUNT - 1, 4);
		Line(w, 128, tMsg ? sMessages[tMsg] : gText_HackDebug_ShopHelp);
	}
	else if (tMode == MODE_TRAINER)
	{
		Line(w, 16, sTopLabels[ITEM_TOP_TRAINER]);
		NumberAt(w, 16, 40, tValue, 4);                  //トレーナーの番号 1〜DEBUG_TRAINER_MAX
		LineAt(w, 64, 40, gTrainers[tValue].trainerName);
		Line(w, 58, gText_HackDebug_TrainerClass);
		NumberAt(w, 80, 58, gTrainers[tValue].trainerClass, 3);
		Line(w, 128, gText_HackDebug_TrainerHelp);
	}
	else if (tMode == MODE_WATASHI)
	{
		Line(w, 16, sTopLabels[ITEM_TOP_WATASHI]);
		Number(w, 44, tValue + 1);                       //1〜10
		Line(w, 62, sSpotNames[tValue]);                 //りる〜いちご（ひとのところへと同じ名前の文字列 Spot0〜9）
		Line(w, 128, gText_HackDebug_WatashiHelp);
	}
	else if (tMode == MODE_WILD)
	{
		Line(w, 16, sTopLabels[ITEM_TOP_WILD]);
		Line(w, 30, gText_HackDebug_ESpecies);
		NumberAt(w, 80, 30, tValue, 4);
		LineAt(w, 120, 30, gSpeciesNames[tValue]);
		Line(w, 44, gText_HackDebug_ELevel);
		NumberAt(w, 80, 44, tCount, 3);
		Line(w, 58, gText_HackDebug_WildShiny);
		LineAt(w, 80, 58, tShiny ? gText_HackDebug_WildYes : gText_HackDebug_WildNo);
		Line(w, 72, gText_HackDebug_WildMoemon);
		LineAt(w, 80, 72, tMoemon ? gText_HackDebug_WildYes : gText_HackDebug_WildNo);
		AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, 30 + tField * 14, 0xFF, NULL);
		Line(w, 90, gText_HackDebug_HelpBig);
		Line(w, 128, gText_HackDebug_WildHelp);
	}
	else if (tMode == MODE_BGM_CAT || tMode == MODE_BGM_LIST)
	{
		u32 pages = 0, page = 0;

		FillWindowPixelRect(w, 0x11, 0, 0, 224, 14);     //一番上の行（「デバッグメニュー」）を消して、この画面の見出しを書く
		if (tMode == MODE_BGM_CAT)
		{
			Line(w, 0, sBgmTextTitle);
			for (j = 0; j < TOP_ROWS && tEScroll + j < HACK_BGM_CAT_COUNT; ++j)
			{
				u32 k = tEScroll + j;
				u8 y = 14 + j * 12;
				LineAt(w, 10, y, sBgmCatNames[k]);
				NumberAt(w, 190, y, BgmCatSize(k), 3);
				if (k == (u32) tField)
					AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, y, 0xFF, NULL);
			}
		}
		else
		{
			u32 cat = tField, first = sBgmCatStart[cat], n = BgmCatSize(cat);

			page = tCount / BGM_ROWS;
			pages = (n + BGM_ROWS - 1) / BGM_ROWS;
			Line(w, 0, sBgmCatNames[cat]);
			for (j = 0; j < BGM_ROWS && page * BGM_ROWS + j < n; ++j)
			{
				u32 idx = first + page * BGM_ROWS + j;
				u8 y = 14 + j * 12;
				NumberAt(w, 10, y, BgmSongOf(idx), 3);
				LineAt(w, 44, y, sBgmSongLabel[idx]);
				if (tBgmNow == BgmSongOf(idx) + 1)
					LineAt(w, 206, y, sBgmTextMark);
				if (page * BGM_ROWS + j == (u32) tCount)
					AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, y, 0xFF, NULL);
			}
		}
		if (tBgmNow)
		{
			LineAt(w, 8, 112, sBgmTextNow);
			NumberAt(w, 66, 112, tBgmNow - 1, 3);
		}
		if (tMode == MODE_BGM_LIST)
		{
			LineAt(w, 98, 112, sBgmTextPageHint);
			NumberAt(w, 168, 112, page + 1, 2);
			LineAt(w, 186, 112, sBgmTextSlash);
			NumberAt(w, 194, 112, pages, 2);
		}
		Line(w, 128, tMode == MODE_BGM_CAT ? sBgmTextHelpCat : sBgmTextHelpList);
	}
	else if (tMode == MODE_ITEM)
	{
		u8 name[64];
		CopyItemName(tValue, name);
		Line(w, 16, tField ? gText_HackDebug_ItemNumber : gText_HackDebug_ItemNumberSel);
		Number(w, 30, tValue);
		Line(w, 44, name[0] == 0xAC ? gText_HackDebug_ItemUnused : name);      //「？」で始まる名前＝使っていない番号
		Line(w, 60, tField ? gText_HackDebug_ItemCountSel : gText_HackDebug_ItemCount);
		Number(w, 74, tCount);
		Line(w, 90, gText_HackDebug_HelpUpDown);
		Line(w, 102, gText_HackDebug_HelpStep);
		Line(w, 114, gText_HackDebug_HelpBig);
		Line(w, 130, tResult == 1 ? gText_HackDebug_ItemGot : tResult == 2 ? gText_HackDebug_ItemFail : gText_HackDebug_ItemHelp);
	}
	else if (tMode == MODE_WARP_MENU)
 {
  const u8* const choices[] = {gText_HackDebug_WarpRegions, gText_HackDebug_WarpGyms, gText_HackDebug_WarpRaw, gText_HackDebug_WarpPeople, gText_HackDebug_WarpRoom, gText_HackDebug_WarpUnova};
  u32 j;
  Line(w, 16, gText_HackDebug_WarpMenu);
  for (j = 0; j < (DebugUnovaGrafted() ? 6u : 5u); ++j) {
   AddTextPrinterParameterized(w, 2, choices[j], 16, 36 + j * 14, 0xFF, NULL);
   if (j == tCursor)
    AddTextPrinterParameterized(w, 2, gText_HackDebug_Cursor, 0, 36 + j * 14, 0xFF, NULL);
  }
  Line(w, 128, gText_HackDebug_WarpHelp);
 }
 else if (tMode == MODE_WARP_ROOM)
 {
  Line(w, 16, gText_HackDebug_WarpRoom);
  Line(w, 40, sRoomNames[tValue]);
  Number(w, 60, tValue + 1);
  Line(w, 110, gText_HackDebug_WarpRoomHelp);
  Line(w, 128, tResult ? gText_HackDebug_WarpUnavailable : gText_HackDebug_WarpHelp);
 }
 else if (tMode == MODE_WARP_PERSON)
 {
  const u8* name = gText_HackDebug_WarpUnavailable;
  Line(w, 16, gText_HackDebug_WarpPeople);
  Line(w, 40, sSpotNames[tValue]);
  Number(w, 60, tValue + 1);
  if (DebugSpotValid(tValue))
   name = MapRegion_GetName(DebugMapHeader(sDebugSpots[tValue].group, sDebugSpots[tValue].num)->regionMapSectionId,
                            GetMapRegionOf(sDebugSpots[tValue].group, sDebugSpots[tValue].num));
  Line(w, 80, IsRomText(name) ? name : gText_HackDebug_WarpUnavailable);
  Line(w, 110, gText_HackDebug_WarpPeopleHelp);
  Line(w, 128, tResult ? gText_HackDebug_WarpUnavailable : gText_HackDebug_WarpHelp);
 }
 else if (tMode == MODE_WARP_REGION || tMode == MODE_WARP_GYM)
 {
  u8 group, num;
  const u8* name = gText_HackDebug_WarpUnavailable;
  Line(w, 16, tMode == MODE_WARP_REGION ? gText_HackDebug_WarpRegions : gText_HackDebug_WarpGyms);
  Line(w, 40, sWarpRegionNames[tValue]);
  if (tMode == MODE_WARP_GYM) {
   Number(w, 60, tCount + 1);
   if (DebugGymMap(tValue, tCount, &group, &num))
    name = MapRegion_GetName(DebugMapHeader(group, num)->regionMapSectionId, tValue);
   Line(w, 80, IsRomText(name) ? name : gText_HackDebug_WarpUnavailable);
  }
  Line(w, 110, tMode == MODE_WARP_REGION ? gText_HackDebug_WarpRegionHelp : gText_HackDebug_WarpGymHelp);
  Line(w, 128, tResult ? gText_HackDebug_WarpUnavailable : gText_HackDebug_WarpHelp);
 }
	else if (tMode == MODE_WARP)
	{
		const u8* name = MapRegion_GetName(DebugMapHeader(tValue, tCount)->regionMapSectionId, GetMapRegionOf(tValue, tCount));
		Line(w, 16, gText_HackDebug_WarpTitle);
		Line(w, 30, tField ? gText_HackDebug_WarpGroup : gText_HackDebug_WarpGroupSel);
		Number(w, 42, tValue);
		Line(w, 56, tField ? gText_HackDebug_WarpMapSel : gText_HackDebug_WarpMap);
		Number(w, 68, tCount);
		Line(w, 82, IsRomText(name) ? name : gText_HackDebug_WarpNoName);
		Line(w, 98, gText_HackDebug_WarpHelpUD);
		Line(w, 110, gText_HackDebug_HelpBig);
		Line(w, 128, gText_HackDebug_WarpHelp);
	}
	else
	{
		Line(w, 22, gText_HackDebug_FlagTitle);
		Number(w, 44, tValue);
		Line(w, 68, FlagGet(tValue) ? gText_HackDebug_On : gText_HackDebug_Off);
		NumberAt(w, 150, 68, VarGet(0x5200 + (tValue & 0xFF)), 5);   //[hack] patches/0986 ジョウトの変数の窓の確かめ: 0x5200 + (旗の番号の下 8 ビット) の値（START で +1）
		Line(w, 92, gText_HackDebug_HelpStep);
		Line(w, 108, gText_HackDebug_HelpBig);
		Line(w, 128, gText_HackDebug_FlagHelp);
	}

	PutWindowTilemap(w);
	CopyWindowToVram(w, 3);
	ShowBg(0);
	ScheduleBgCopyTilemapToVram(0);
}

//きのみブレンダーを直接開く（FEAT-pokeblock 段 4b1）。原文は台本が fadescreen してから special を呼ぶ＝ここは暗転を待ってから HackBlender_Start（戻り先はフィールド）
#define tBlOpp    data[0]
#define tBlMaster data[1]

static void Task_DebugBlenderStart(u8 taskId)
{
	s16* data = gTasks[taskId].data;

	if (!gPaletteFade->active)
	{
		u8 opp = tBlOpp;
		bool8 master = tBlMaster;

		DestroyTask(taskId);
		HackBlender_Start(opp, master, CB2_ReturnToField);
	}
}

static void DebugBlenderStart(u8 opponents, bool8 master)
{
	u8 taskId = CreateTask(Task_DebugBlenderStart, 80);

	gTasks[taskId].data[0] = opponents;
	gTasks[taskId].data[1] = master;
	BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, 0);
}

static void Close(u8 taskId)
{
	s16* data = gTasks[taskId].data;
	u8 w = tWindow;

	gHelpSystemEnabled = tHelp;
	ClearStdWindowAndFrameToTransparent(w, TRUE);
	RemoveWindow(w);
	DestroyTask(taskId);
	ScriptContext2_Disable();
}

/* ---- クリア後の状態にする（遊びを開ける旗と道具だけ・出典は docs/research/2026-10-11_debug-menu.md の表）----
   ★ 隠す旗（わたしたち 0xF0D・拾った道具・倒したトレーナー・イベントの人を消す旗）は立てない。
   ★ 殿堂入りの「戻り先の印」（SINNOH_FLAG_LEAGUE_HOF 0x138B・HOENN_FLAG_HOF_IN_HOENN 0x13EA）は次の殿堂入りで使われて消える一度きりの印なので立てない */
#define ENABLE_NATIONAL_POKEDEX ((void (*)(void)) (0x0806DA20 | 1))   //素の EnableNationalPokedex（docs/research/2026-10-10_national-dex-habitat.md）
#define DEBUG_FLAG_GOT_COIN_CASE 0x243                                //FLAG_GOT_COIN_CASE（include/constants/flags.h・assembly/hack_gamecorner/coin_clerk.s の COIN_FLAG_CASE と同じ）
#define SINNOH_FLAG_CHAMPION 0x138A                                   //assembly/sinnoh_scripts/sinnoh_gyms.s 1189 の setflag（シンオウのチャンピオンになった）


static void GiveKeyItem(u16 item)
{
	if (!CheckBagHasItem(item, 1))
		AddBagItem(item, 1);
}

static void DebugClearState(void)
{
	static const u16 hoennBadges[8] = {HOENN_FLAG_BADGE1, HOENN_FLAG_BADGE2, HOENN_FLAG_BADGE3, HOENN_FLAG_BADGE4,
	                                   HOENN_FLAG_BADGE5, HOENN_FLAG_BADGE6, HOENN_FLAG_BADGE7, HOENN_FLAG_BADGE8};
	//include/constants/flags.h の FLAG_GOT_HM01〜05・06（そのヘッダでは /* */ の中＝名前が使えないので番号で書く）
	static const u16 hmGot[] = {0x237, 0x238, 0x239, 0x23A, 0x23B, 0x2EF};
	u32 i;

	//バッジ（4 地方）。ホウエンの番号は 8 個とも別の飛び飛びの番号（hoenn_script_flags.h）、ジョウト 0x1300〜・シンオウ 0x1370〜
	for (i = 0; i < 8; ++i)
	{
		FlagSet(FLAG_BADGE01_GET + i);
		FlagSet(hoennBadges[i]);
		FlagSet(JOHTO_FLAG_BADGE1 + i);
		FlagSet(SINNOH_FLAG_BADGE1 + i);
	}
	DebugUnlockFlySpots();                          //そらをとぶ行き先（既存の項目と同じ）
	//リーグ・殿堂入り（カントー＝リーグクリア・ホウエン＝チャンピオン・シンオウ＝チャンピオン。ジョウトのリーグはまだ無い）
	FlagSet(FLAG_SYS_GAME_CLEAR);
	FlagSet(HOENN_FLAG_IS_CHAMPION);
	FlagSet(SINNOH_FLAG_CHAMPION);
	//行き先の鍵
	FlagSet(HOENN_FLAG_FRONTIER_INVITED);           //バトルフロンティア（わたしたちの戦える条件もこれ）
	FlagSet(HOENN_FLAG_ISLANDS_OPEN);               //ホウエンのみなみ／さいはてのことう（ホウエンリーグ 2 回目の印）
	//図鑑・ダッシュ・ずかんナビ
	FlagSet(FLAG_SYS_POKEMON_GET);                  //スタートメニューの「ポケモン」（0x828・立てないと 0x829 だけで「ポケモン」が出ない）
	FlagSet(FLAG_SYS_POKEDEX_GET);
	FlagSet(FLAG_SYS_DEXNAV);
	ENABLE_NATIONAL_POKEDEX();
	FlagSet(FLAG_SYS_B_DASH);
	//コインケース（もらった旗＋道具）
	FlagSet(DEBUG_FLAG_GOT_COIN_CASE);
	GiveKeyItem(ITEM_COIN_CASE);
	//ひでん技（もらった旗 HM01〜06 ＋ HM01〜08 の道具）
	for (i = 0; i < ARRAY_COUNT(hmGot); ++i)
		FlagSet(hmGot[i]);
	for (i = ITEM_HM01_CUT; i <= ITEM_HM08_ROCK_CLIMB; ++i)
		GiveKeyItem(i);
	//ダイマックスバンド・メガリング・Z パワーリング・テラスタルオーブ（もらった旗は無く、持っているかで見る）
	GiveKeyItem(ITEM_DYNAMAX_BAND);
	GiveKeyItem(ITEM_MEGA_RING);
	GiveKeyItem(ITEM_Z_POWER_RING);
	GiveKeyItem(ITEM_TERA_ORB);
}

/* わたしたちと たたかう: 番号 idx（0〜9）のトレーナー 940+idx と その場で戦う。
   RAM に組んだ小さな台本を ScriptContext1 で流す（ショップと同じ作り・台本は Malloc のまま 16 バイトを使うたびに残す＝デバッグ版だけ）:
     cleartrainerflag id → trainerbattle3(type 3＝会話なし・負けの文だけ) id 0 負けの文 → release → end
   本物の話しかけの台本（assembly/hack_watashitachi/watashitachi_scripts.s）と同じ trainerbattle で始まるので gTrainerBattleOpponent_A が
   940〜949 になり、自分のボール・全員色違い・Moémon（build_pokemon.c）・相手だけダイマックス・種族値 +50 がそのまま効く。
   隠す旗 0xF0D・行ける印 0x13EB・地図の配置は見ない。初めて勝った印 0xF01〜0xF0A・実績の台本は流さない */
static u8* BuildWatashiBattle(u8 idx)
{
	static const u8* const defeat[WATASHI_COUNT] =
	{
		gText_Watashi_Liru_Defeat, gText_Watashi_Ui_Defeat, gText_Watashi_Shidare_Defeat, gText_Watashi_Milk_Defeat, gText_Watashi_Nao_Defeat,
		gText_Watashi_Kairi_Defeat, gText_Watashi_Rikka_Defeat, gText_Watashi_Shii_Defeat, gText_Watashi_Ran_Defeat, gText_Watashi_Ichigo_Defeat,
	};
	u8* buf;
	u16 id = WATASHI_TRAINER_FIRST + idx;
	u32 text;

	if (idx >= WATASHI_COUNT)
		return NULL;
	buf = Malloc(16);
	if (buf == NULL)
		return NULL;
	text = (u32) defeat[idx];
	buf[0] = 0x62;                          //cleartrainerflag
	buf[1] = id;
	buf[2] = id >> 8;
	buf[3] = 0x5C;                          //trainerbattle
	buf[4] = 0x03;                          //type 3（会話なしの 1 対 1）
	buf[5] = id;
	buf[6] = id >> 8;
	buf[7] = 0;                             //filler
	buf[8] = 0;
	buf[9] = text;
	buf[10] = text >> 8;
	buf[11] = text >> 16;
	buf[12] = text >> 24;
	buf[13] = 0x6C;                         //release
	buf[14] = 0x02;                         //end
	return buf;
}

/* トレーナーと たたかう（2026-10-26 FEAT-debug-trainer-battle・Redmine #76 の相方の試験用）: 番号 id のトレーナーとその場で戦う。
   わたしたちと同じ作り（RAM の台本・Malloc のまま 16 バイトを使うたびに残す＝デバッグ版だけ）:
     cleartrainerflag id → trainerbattle3（会話なし・負けの文だけ） id 0 文 → release → end
   本物の trainerbattle で始まるので gTrainerBattleOpponent_A が id になり、BattleSetup_StartTrainerBattle（連れ歩き中のハルカの相方の入口・patches/0865）をそのまま通る。
   連れ歩き中の状態（gFollowerState・変数 HACK_VAR_HEROINE_FOLLOWER）には触らない */
#define DEBUG_TRAINER_MAX 1746    //トレーナーの表の最後（ホウエンの追加分まで・tools/rom/verify_debug_trainer_battle.py が ROM の表と同じ数か見る）
static u8* BuildTrainerBattle(u16 id)
{
	u8* buf;
	u32 text = (u32) gText_HackDebug_TrainerDefeat;

	if (id < 1 || id > DEBUG_TRAINER_MAX)
		return NULL;
	buf = Malloc(16);
	if (buf == NULL)
		return NULL;
	buf[0] = 0x62;                          //cleartrainerflag
	buf[1] = id;
	buf[2] = id >> 8;
	buf[3] = 0x5C;                          //trainerbattle
	buf[4] = 0x03;                          //type 3（会話なしの 1 対 1）
	buf[5] = id;
	buf[6] = id >> 8;
	buf[7] = 0;                             //filler
	buf[8] = 0;
	buf[9] = text;
	buf[10] = text >> 8;
	buf[11] = text >> 16;
	buf[12] = text >> 24;
	buf[13] = 0x6C;                         //release
	buf[14] = 0x02;                         //end
	return buf;
}

/* やせいを だす: 種族 species・レベル level の野生戦をその場で始める（わたしたちと同じ作り: RAM の台本を ScriptContext1 で流す・16 バイトを使うたびに残す＝デバッグ版だけ）:
     setwildbattle species level 0 → callasm HackDebugMenu_ApplyWildId（色違い・Moémon の ID を gEnemyParty[0] に）→ goto SystemScript_StartDexNavBattle
   goto 先は ずかんナビの野生戦の台本（lockall → FollowerPositionFixScript → びっくり → dowildbattle → releaseall → end）。
   dowildbattle は戦闘のあと野外へ戻って台本の続き（releaseall）を流すので、主人公の固定が解け固まらない（本物の野生・ずかんナビと同じ流れ）。
   逃げる・捕まえるはふつうの野生戦。色違い・Moémon は Var8004 の bit0・bit1 で callasm に渡す */
#define DEBUG_FLAG_MOEMON_ON 0x12E9   //hack_moemon_switch.c の HACK_FLAG_MOEMON_ON（Moémon の絵を出す旗）

void HackDebugMenu_ApplyWildId(void)
{
	struct BoxPokemon* box = (struct BoxPokemon*) &gEnemyParty[0];

	if (GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
		return;
	HackMonId_SetShiny(box, (Var8004 & 1) != 0);          //性格値は触らない（6.5）。しない＝通常色に決める
	HackMonId_SetMoemon(box, (Var8004 & 2) != 0);
	if (Var8004 & 2)
		FlagSet(DEBUG_FLAG_MOEMON_ON);                    //絵は旗 ON のときだけ出る（ふだんは せってい で切り替え）
}

/* デバッグルーム（へや7）のどうぐの台（callasm）: 使っている番号の道具を 1 個ずつバッグへ入れる（入りきらない分は AddBagItem が FALSE を返すので捨てる）。
   バッグでアイコン・並びを見る用。「？」の名前（使っていない番号）は入れない */
void HackDebugRoom_GiveAllItems(void)
{
	u16 id;
	u8 name[64];

	for (id = 1; id < ITEMS_COUNT; ++id)
	{
		CopyItemName(id, name);
		if (name[0] != 0xAC)
			AddBagItem(id, 1);
	}
}

static u8* BuildWildBattle(u16 species, u16 level, bool8 shiny, bool8 moemon)
{
	u8* buf = Malloc(16);
	u32 fn = (u32) HackDebugMenu_ApplyWildId | 1;
	u32 next = (u32) SystemScript_StartDexNavBattle;

	if (buf == NULL)
		return NULL;
	buf[0] = 0xB6;                          //setwildbattle
	buf[1] = species;
	buf[2] = species >> 8;
	buf[3] = level;
	buf[4] = 0;                             //持ち物 ITEM_NONE
	buf[5] = 0;
	buf[6] = 0x23;                          //callasm
	buf[7] = fn;
	buf[8] = fn >> 8;
	buf[9] = fn >> 16;
	buf[10] = fn >> 24;
	buf[11] = 0x05;                         //goto
	buf[12] = next;
	buf[13] = next >> 8;
	buf[14] = next >> 16;
	buf[15] = next >> 24;
	Var8004 = (shiny ? 1 : 0) | (moemon ? 2 : 0);
	return buf;
}

//出せる種族か（空き番号＝種族値が全部 0・タマゴは除く）
static bool8 IsWildSpecies(s32 s)
{
	if (s < 1 || s >= NUM_SPECIES || s == SPECIES_EGG)
		return FALSE;
	return (gBaseStats[s].baseHP | gBaseStats[s].baseAttack | gBaseStats[s].baseDefense
	      | gBaseStats[s].baseSpeed | gBaseStats[s].baseSpAttack | gBaseStats[s].baseSpDefense) != 0;
}

//cur から delta 動かした先（範囲に収め、出せない番号は進む向きに、端なら逆向きに探す）
static s32 StepWildSpecies(s32 cur, s32 delta)
{
	s32 t = cur + delta, dir = delta > 0 ? 1 : -1, s;

	if (t < 1)
		t = 1;
	if (t > NUM_SPECIES - 1)
		t = NUM_SPECIES - 1;
	for (s = t; s >= 1 && s < NUM_SPECIES; s += dir)
		if (IsWildSpecies(s))
			return s;
	for (s = t; s >= 1 && s < NUM_SPECIES; s -= dir)
		if (IsWildSpecies(s))
			return s;
	return cur;
}

//数を入れる画面の送り: L ＋左右 ±100・R ＋左右 ±1000・なしは ±1（L と R が両方なら L）
static s32 BigStep(u16 held)
{
	return (held & L_BUTTON) ? 100 : (held & R_BUTTON) ? 1000 : 1;
}

static s32 Adjust(s32 n, s32 delta, s32 lo, s32 hi)
{
	n += delta;
	if (n < lo)
		n = lo;
	if (n > hi)
		n = hi;
	return n;
}

//しゅじんこうの せいべつ（2026-10-16 ユーザー「デバッグで主人公の性別変えたい」）: セーブの playerGender と gPlayerAvatar->gender を入れ替え、
//今の状態（歩き・自転車・なみのり・潜水…）の絵に その場で替える（岩のぼりの終わりと同じ EventObjectSetGraphicsId。パレットは bw_player_pal.c のフックが付け直す）。
//絵の番号は性別から毎回引く表（character_customization.c の sPlayerAvatarGfxIds）で、セーブに主人公の絵の番号は残らない（つづきからは InitPlayerAvatar が playerGender で作る）
static void FlipPlayerGender(void)
{
	struct EventObject* player;

	gSaveBlock2->playerGender = (gSaveBlock2->playerGender == FEMALE) ? MALE : FEMALE;
	gPlayerAvatar->gender = gSaveBlock2->playerGender;
	if (gPlayerAvatar->eventObjectId >= EVENT_OBJECTS_COUNT)
		return;
	player = &gEventObjects[gPlayerAvatar->eventObjectId];
	if (player->active && player->spriteId < MAX_SPRITES && gPlayerAvatar->tileTransitionState == 0)
		EventObjectSetGraphicsId(player, GetPlayerAvatarGraphicsIdByCurrentState());
}

static void Task_HackDebugMenu(u8 taskId)
{
	s16* data = gTasks[taskId].data;
	u16 keys = gMain.newKeys | (gMain.newAndRepeatedKeys & (DPAD_RIGHT | DPAD_LEFT));
	u16 held = gMain.heldKeysRaw;
	u16 newRaw = gMain.newKeysRaw;      //BGMテスト: L／R の押した瞬間（下で L／R を消す前に控える）

	//メニューの間は L/R のヘルプを開かせない（元と同じ・閉じるときに tHelp へ戻す）
	gMain.heldKeysRaw &= ~(L_BUTTON | R_BUTTON);
	gMain.newKeysRaw &= ~(L_BUTTON | R_BUTTON);
	gMain.heldKeys &= ~(L_BUTTON | R_BUTTON);
	gMain.newKeys &= ~(L_BUTTON | R_BUTTON);
	gMain.newAndRepeatedKeys &= ~(L_BUTTON | R_BUTTON);

	if (tWaitA)
	{
		if (held & (A_BUTTON | B_BUTTON))
			return;
		tWaitA = FALSE;
	}
	if (keys == 0)
		return;

	if (keys & B_BUTTON)
	{
		tMsg = MSG_NONE;
		if (tMode == MODE_EDIT)          //手持ち編集からは手持ちを選ぶ画面へ
		{
			tMode = MODE_PICK;
			Draw(taskId);
		}
		else if (tMode == MODE_BGM_LIST)   //BGMテスト: 曲の一覧からは分類の画面へ（鳴っている曲はそのまま）
		{
			tMode = MODE_BGM_CAT;
			Draw(taskId);
		}
		else if (tMode == MODE_BGM_CAT)    //分類の画面からは一覧へ。曲を鳴らしていたら地図の曲に戻す
		{
			if (tBgmTouched)
				BgmRestoreMapMusic();
			tBgmTouched = 0;
			tBgmNow = 0;
			tMode = MODE_TOP;
			Draw(taskId);
		}
		else if (tMode != MODE_TOP)
		{
			tMode = MODE_TOP;
			Draw(taskId);
		}
		else
			Close(taskId);
		return;
	}
	tMsg = MSG_NONE;

	if (tMode == MODE_BGM_CAT || tMode == MODE_BGM_LIST)
	{
		//BGMテスト。A = 分類へ入る／曲を鳴らす・SELECT = 止める・L／R = 1 ページずつ・上下 = 1 つずつ（端でひと回り）・B は上で処理済み
		bool8 paged = (newRaw & (L_BUTTON | R_BUTTON)) != 0;
		bool8 isCat = tMode == MODE_BGM_CAT;
		s32 count = isCat ? HACK_BGM_CAT_COUNT : (s32) BgmCatSize(tField);
		s32 cur = isCat ? tField : tCount;
		s32 rows = isCat ? TOP_ROWS : BGM_ROWS;

		if (keys & DPAD_UP)
			cur = cur ? cur - 1 : count - 1;
		if (keys & DPAD_DOWN)
			cur = cur == count - 1 ? 0 : cur + 1;
		if (newRaw & L_BUTTON)
			cur = Adjust(cur, -rows, 0, count - 1);
		if (newRaw & R_BUTTON)
			cur = Adjust(cur, rows, 0, count - 1);
		if (isCat)
		{
			tField = cur;
			if (cur < tEScroll)
				tEScroll = cur;
			if (cur >= tEScroll + TOP_ROWS)
				tEScroll = cur - TOP_ROWS + 1;
		}
		else
			tCount = cur;

		if (keys & SELECT_BUTTON)                                  //止める（地図の曲へは出るときに戻す）
		{
			BgmStopAll();
			tBgmTouched = 1;
			tBgmNow = 0;
		}
		else if ((keys & A_BUTTON) && !paged)                      //L=A の設定で L／R が A にもなる分は無視する
		{
			if (isCat)
			{
				tMode = MODE_BGM_LIST;
				tCount = 0;
			}
			else
			{
				BgmPlay(sBgmCatStart[tField] + tCount);
				tBgmTouched = 1;
				tBgmNow = BgmSongOf(sBgmCatStart[tField] + tCount) + 1;
			}
		}
	}
	else if (tMode == MODE_TOP)
	{
		if (keys & DPAD_UP)
			tCursor = tCursor ? tCursor - 1 : ITEM_TOP_COUNT - 1;
		if (keys & DPAD_DOWN)
			tCursor = tCursor == ITEM_TOP_COUNT - 1 ? 0 : tCursor + 1;
		if (tCursor < tScroll)
			tScroll = tCursor;
		if (tCursor >= tScroll + TOP_ROWS)
			tScroll = tCursor - TOP_ROWS + 1;
		if ((keys & A_BUTTON) && sTopFlags[tCursor])        //切り替え（捕獲 100%・やせいなし・むてき・いちげき・トレーナーむし）
		{
			if (FlagGet(sTopFlags[tCursor]))
				FlagClear(sTopFlags[tCursor]);
			else
				FlagSet(sTopFlags[tCursor]);
		}
		else if (keys & A_BUTTON)
		{
			switch (tCursor)
			{
				case ITEM_TOP_MOEMON:
				case ITEM_TOP_SHINY:
				case ITEM_TOP_EDIT:
					tMode = MODE_PICK;
					tPurpose = tCursor;
					tSlot = 0;
					break;
				case ITEM_TOP_GETMON:
					tMode = MODE_GETMON;
					tValue = 1;
					tCount = 50;
					tField = 0;
					break;
				case ITEM_TOP_SHOP:
					tMode = MODE_SHOP;
					tValue = 0;
					break;
				case ITEM_TOP_LV200:
				{
					u32 i;
					for (i = 0; i < PARTY_SIZE; ++i)
					{
						if (IsUsableMon(i))
							EditSet(&gPlayerParty[i], 1, MAX_LEVEL);
					}
					Message(tWindow, gText_HackDebug_Lv200Done);
					return;
				}
				case ITEM_TOP_GENDER:
						FlipPlayerGender();            //行の右の「おとこのこ／おんなのこ」は このあとの Draw で変わる
						break;
					case ITEM_TOP_DEX:
				{
					u16 n;
					for (n = 1; n < NATIONAL_DEX_COUNT; ++n)
					{
						GetSetPokedexFlag(n, DEX_FLAG_SET_SEEN);
						GetSetPokedexFlag(n, DEX_FLAG_SET_CAUGHT);
					}
					Message(tWindow, gText_HackDebug_DexDone);
					return;
				}
				case ITEM_TOP_ITEM:
					tMode = MODE_ITEM;
					tValue = 1;
					tCount = 1;
					tField = 0;
					tResult = 0;
					break;
				case ITEM_TOP_WARP:
				tMode = MODE_WARP_MENU;
				tCursor = 0;
				tResult = 0;
				break;
			case ITEM_TOP_HEAL:
					HealPlayerParty();
					Message(tWindow, gText_HackDebug_Healed);
					return;
				case ITEM_TOP_MONEY:
					AddMoney(&gSaveBlock1->money, DEBUG_MONEY_GIVE);
					Message(tWindow, gText_HackDebug_MoneyDone);
					return;
				case ITEM_TOP_BADGES:
				{
					u16 f;
					for (f = FLAG_BADGE01_GET; f <= FLAG_BADGE08_GET; ++f)
						FlagSet(f);
					Message(tWindow, gText_HackDebug_BadgesDone);
					return;
				}
				case ITEM_TOP_FLY:
						DebugUnlockFlySpots();
						Message(tWindow, gText_HackDebug_FlyDone);
						return;
					case ITEM_TOP_BGM:
						tMode = MODE_BGM_CAT;
						tField = 0;
						tEScroll = 0;
						tBgmTouched = 0;
						tBgmNow = 0;
						break;
					case ITEM_TOP_WATASHI:
						tMode = MODE_WATASHI;
						tValue = 0;
						break;
					case ITEM_TOP_TRAINER:
						tMode = MODE_TRAINER;
						tValue = (tValue >= 1 && tValue <= DEBUG_TRAINER_MAX) ? tValue : 414;   //初めはカントーのタケシ（TRAINER_LEADER_BROCK）
						break;
					case ITEM_TOP_WILD:
						tMode = MODE_WILD;
						tValue = 1;
						tCount = 50;
						tField = 0;
						tShiny = 0;
						tMoemon = 0;
						break;
					case ITEM_TOP_KAIZOU:
					{
						struct HackRamCheatReport rep;
						u8* out = gStringVar4;

						HackRamCheat_RunNow(&rep);   //状態を変えず・印も付けない（自動の入口は通常版だけ）
						if (rep.mask == 0)
							StringCopy(out, gText_HackDebug_KaizouOk);
						else
						{
							out = StringCopy(out, gText_HackDebug_KaizouNg);
							if (rep.mask & (HACK_RC_MONEY_RANGE | HACK_RC_MONEY_SHADOW))
								out = StringAppend(out, gText_HackDebug_KaizouMoney);
							if (rep.mask & (HACK_RC_COIN_RANGE | HACK_RC_COIN_SHADOW))
								out = StringAppend(out, gText_HackDebug_KaizouCoin);
							if (rep.mask & HACK_RC_PARTY)
							{
								out = StringAppend(out, gText_HackDebug_KaizouParty);
								out = ConvertIntToDecimalStringN(out, rep.partySlot + 1, STR_CONV_MODE_LEFT_ALIGN, 1);   //手持ちの番号
								*out++ = 0xF0;   //「:」
								out = ConvertIntToDecimalStringN(out, rep.partyReason, STR_CONV_MODE_LEFT_ALIGN, 2);   //HRC_*
							}
							if (rep.mask & HACK_RC_BOX)
							{
								out = StringAppend(out, gText_HackDebug_KaizouBox);
								out = ConvertIntToDecimalStringN(out, rep.boxId + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
							}
							*out = 0xFF;
						}
						Message(tWindow, gStringVar4);
						return;
					}
					case ITEM_TOP_CLEAR:
						DebugClearState();
						Message(tWindow, gText_HackDebug_ClearDone);
						return;
					case ITEM_TOP_FLAG:
					tMode = MODE_FLAG;
					tValue = 1;
					break;
				case ITEM_TOP_PHONE:
					tMode = MODE_PHONE;
					tCursor = 0;
					tScroll = 0;
					break;
				case ITEM_TOP_BLENDER:
					tMode = MODE_BLENDER;
					tCursor = 0;
					break;
				case ITEM_TOP_CLOSE:
					Close(taskId);
					return;
			}
		}
	}
	else if (tMode == MODE_PHONE)
	{
		u8 n = HackPhone_ContactCount();

		if (n == 0)
		{
			tMode = MODE_TOP;
		}
		else
		{
			if (keys & DPAD_UP)
				tCursor = tCursor ? tCursor - 1 : n - 1;
			if (keys & DPAD_DOWN)
				tCursor = tCursor == n - 1 ? 0 : tCursor + 1;
			if (tCursor < tScroll)
				tScroll = tCursor;
			if (tCursor >= tScroll + TOP_ROWS)
				tScroll = tCursor - TOP_ROWS + 1;
			if (keys & A_BUTTON)
			{
				const u8 *script = HackPhone_IncomingScript(tCursor);

				Close(taskId);
				PlaySE(SE_SELECT);
				ScriptContext2_Enable();
				ScriptContext1_SetupScript(script);
				return;
			}
		}
	}
	else if (tMode == MODE_BLENDER)
	{
		if (keys & DPAD_UP)
			tCursor = tCursor ? tCursor - 1 : 3;
		if (keys & DPAD_DOWN)
			tCursor = tCursor == 3 ? 0 : tCursor + 1;
		if (keys & A_BUTTON)
		{
			u8 opponents = tCursor == 3 ? 1 : tCursor + 1;       //0・1・2 → 1・2・3 にん・3 → めいじん（NPC 1 人）
			bool8 master = tCursor == 3;

			Close(taskId);
			PlaySE(SE_SELECT);
			DebugBlenderStart(opponents, master);
			return;
		}
	}
	else if (tMode == MODE_PICK)
	{
		u32 k;
		for (k = 0; k < PARTY_SIZE && (keys & (DPAD_UP | DPAD_DOWN)); ++k)     //次の手持ち（タマゴ・空きは飛ばす）
		{
			tSlot = (keys & DPAD_UP) ? (tSlot + PARTY_SIZE - 1) % PARTY_SIZE : (tSlot + 1) % PARTY_SIZE;
			if (IsUsableMon(tSlot))
				break;
		}
		if (keys & A_BUTTON)
		{
			struct BoxPokemon* box = (struct BoxPokemon*) &gPlayerParty[tSlot];
			if (!IsUsableMon(tSlot))
				tMsg = MSG_NO_MON;
			else if (tPurpose == ITEM_TOP_MOEMON)
			{
				HackMonId_SetMoemon(box, !HackMonId_IsMoemonHit(box));
				tMsg = MSG_CHANGED;
			}
			else if (tPurpose == ITEM_TOP_SHINY)
			{
				HackMonId_SetShiny(box, !HackMonId_IsShinyBox(box));
				tMsg = MSG_CHANGED;
			}
			else
			{
				tMode = MODE_EDIT;
				tField = 0;
				tEScroll = 0;
			}
		}
	}
	else if (tMode == MODE_EDIT)
	{
		s32 step = BigStep(held);
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : 0;
		struct Pokemon* mon = &gPlayerParty[tSlot];

		if (keys & DPAD_UP)
			tField = tField ? tField - 1 : EDIT_FIELDS - 1;
		if (keys & DPAD_DOWN)
			tField = tField == EDIT_FIELDS - 1 ? 0 : tField + 1;
		if (tField < tEScroll)
			tEScroll = tField;
		if (tField >= tEScroll + EDIT_ROWS)
			tEScroll = tField - EDIT_ROWS + 1;
		if (delta)
			EditSet(mon, tField, Adjust(EditGet(mon, tField), delta, EditMin(tField), EditMax(tField)));
	}
	else if (tMode == MODE_GETMON)
	{
		s32 step = BigStep(held);
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : 0;

		if (keys & (DPAD_UP | DPAD_DOWN))
			tField ^= 1;
		if (tField)
			tCount = Adjust(tCount, delta, 1, MAX_LEVEL);
		else
			tValue = Adjust(tValue, delta, 1, NUM_SPECIES - 1);
		if (keys & A_BUTTON)
		{
			u8 r = ScriptGiveMon(tValue, tCount, ITEM_NONE, 0, 0, 0);    //0 手持ち・1 ボックス・2 入らない（素の流れ）
			tMsg = r == 0 ? MSG_GOT_PARTY : r == 1 ? MSG_GOT_BOX : MSG_GOT_FAIL;
		}
	}
	else if (tMode == MODE_SHOP)
	{
		s32 pages = (ITEMS_COUNT - 1 + SHOP_PAGE - 1) / SHOP_PAGE;
		s32 step = BigStep(held);
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : 0;

		tValue = Adjust(tValue, delta, 0, pages - 1);
		if (keys & A_BUTTON)
		{
			u8* script = BuildShop(tValue);
			if (script == NULL)
				tMsg = MSG_SHOP_EMPTY;
			else
			{
				Close(taskId);                       //ScriptContext2 を下ろしてから台本を流す（台本の release で主人公を放す）
				ScriptContext1_SetupScript(script);
				return;
			}
		}
	}
	else if (tMode == MODE_TRAINER)
	{
		//左右 ±1・L＋左右 ±100・R＋左右 ±1000・上下 ±10。A でその場で戦う（連れ歩きの相方はそのまま付く＝HackHeroine の入口が見る）
		s32 step = BigStep(held);
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : (keys & DPAD_UP) ? 10 : (keys & DPAD_DOWN) ? -10 : 0;

		if (delta)
			tValue = Adjust(tValue, delta, 1, DEBUG_TRAINER_MAX);
		if (keys & A_BUTTON)
		{
			u8* script = BuildTrainerBattle(tValue);

			if (script != NULL)
			{
				Close(taskId);
				ScriptContext1_SetupScript(script);
				return;
			}
		}
	}
	else if (tMode == MODE_WATASHI)
	{
		//左右・上下で 1 人ずつ（10 人を回る）。A でその場で戦う
		s32 delta = (keys & (DPAD_RIGHT | DPAD_DOWN)) ? 1 : (keys & (DPAD_LEFT | DPAD_UP)) ? -1 : 0;

		tValue = (tValue + delta + WATASHI_COUNT) % WATASHI_COUNT;
		if (keys & A_BUTTON)
		{
			u8* script = BuildWatashiBattle(tValue);
			if (script != NULL)
			{
				Close(taskId);                       //ScriptContext2 を下ろしてから台本を流す（ショップと同じ）
				ScriptContext1_SetupScript(script);
				return;
			}
		}
	}
	else if (tMode == MODE_WILD)
	{
		s32 step = BigStep(held);
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : 0;

		if (keys & DPAD_UP)
			tField = tField ? tField - 1 : 3;
		if (keys & DPAD_DOWN)
			tField = tField == 3 ? 0 : tField + 1;
		if (delta)
		{
			if (tField == 0)
				tValue = StepWildSpecies(tValue, delta);
			else if (tField == 1)
				tCount = Adjust(tCount, delta, 1, MAX_LEVEL);
			else if (tField == 2)
				tShiny ^= 1;
			else
				tMoemon ^= 1;
		}
		if (keys & A_BUTTON)
		{
			u8* script = BuildWildBattle(tValue, tCount, tShiny, tMoemon);
			if (script != NULL)
			{
				Close(taskId);                       //ScriptContext2 を下ろしてから台本を流す（台本の releaseall で主人公を放す）
				ScriptContext1_SetupScript(script);
				return;
			}
		}
	}
	else if (tMode == MODE_WARP_MENU)
 {
  if (keys & DPAD_UP) tCursor = Adjust(tCursor, -1, 0, DebugUnovaGrafted() ? 5 : 4);
  if (keys & DPAD_DOWN) tCursor = Adjust(tCursor, 1, 0, DebugUnovaGrafted() ? 5 : 4);
  if (keys & A_BUTTON) {
   if (tCursor == 5 && DebugUnovaGrafted()) {   //イッシュ（群 81 の 0 番）。接ぎ木していない版では項目ごと出さない（DebugUnovaGrafted）
    Close(taskId);
    SetWarpDestination(81, 0, 0, -1, -1);   //DebugMapHeader（素の群の表）は群 81 を知らないので、ヘッダーを読まず入口の 0 番へ直に飛ぶ
    DoWarp();
    return;
   }
   tMode = tCursor == 0 ? MODE_WARP_REGION : tCursor == 1 ? MODE_WARP_GYM : tCursor == 2 ? MODE_WARP : tCursor == 3 ? MODE_WARP_PERSON : MODE_WARP_ROOM;
   tValue = 0; tCount = 0; tField = 0; tResult = 0;
  }
 }
 else if (tMode == MODE_WARP_ROOM)
 {
  tResult = 0;
  tValue = Adjust(tValue, (keys & DPAD_DOWN) ? 1 : (keys & DPAD_UP) ? -1 : 0, 0, DEBUG_ROOM_COUNT - 1);
  if (keys & A_BUTTON) {
   if (DebugRoomSetWarp(tValue)) {
    Close(taskId);
    DoWarp();
    return;
   }
   tResult = 1;
  }
 }
 else if (tMode == MODE_WARP_PERSON)
 {
  tResult = 0;
  tValue = Adjust(tValue, (keys & DPAD_DOWN) ? 1 : (keys & DPAD_UP) ? -1 : 0, 0, HACK_DEBUG_SPOT_COUNT - 1);
  if (keys & A_BUTTON) {
   if (DebugSpotSetWarp(tValue)) {
    Close(taskId);
    DoWarp();
    gFieldCallback = DebugSpotFieldCallback;   /* DoWarp が入れた素のコールバックの代わり（中で素を先に呼ぶ）・#260 */
    return;
   }
   tResult = 1;
  }
 }
 else if (tMode == MODE_WARP_REGION || tMode == MODE_WARP_GYM)
 {
  s32 delta = (keys & DPAD_DOWN) ? 1 : (keys & DPAD_UP) ? -1 : 0;
  tResult = 0;
  if (tMode == MODE_WARP_REGION)
   tValue = Adjust(tValue, delta, 0, MAP_REGION_KITAKAMI);
  else {
   tCount = Adjust(tCount, delta, 0, 7);
   delta = (keys & DPAD_RIGHT) ? 1 : (keys & DPAD_LEFT) ? -1 : 0;
   tValue = Adjust(tValue, delta, 0, MAP_REGION_SINNOH);
  }
  if (keys & A_BUTTON) {
   if (tMode == MODE_WARP_REGION ? DebugRegionSetWarp(tValue) : DebugGymSetWarp(tValue, tCount)) {
    Close(taskId);
    DoWarp();
    return;
   }
   tResult = 1;
  }
 }
	else if (tMode == MODE_WARP)
	{
		s32 step = BigStep(held);
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : 0;

		if (keys & (DPAD_UP | DPAD_DOWN))
			tField ^= 1;
		if (tField == 0)
		{
			s32 g = Adjust(tValue, delta, 0, DEBUG_MAP_GROUP_COUNT - 1);    //上限・下限で止まる（±100 でも群の外へ出ない）
			while (delta && g >= 0 && g < DEBUG_MAP_GROUP_COUNT && MapCountInGroup(g) == 0)   //地図の無い群は飛ばす
				g += delta > 0 ? 1 : -1;
			if (delta && g >= 0 && g < DEBUG_MAP_GROUP_COUNT && g != tValue)
			{
				tValue = g;
				tCount = 0;
			}
		}
		else
			tCount = Adjust(tCount, delta, 0, MapCountInGroup(tValue) - 1);
		if (keys & A_BUTTON)
		{
			u8 group = tValue, num = tCount;
			Close(taskId);
			DebugWarpTo(group, num);
			return;
		}
	}
	else
	{
		s32 step = BigStep(held);    //L＋左右 ±100・R＋左右 ±1000（2026-10-15 patches/0406・前は R ±10）
		s32 delta = (keys & DPAD_RIGHT) ? step : (keys & DPAD_LEFT) ? -step : 0;

		if (tMode == MODE_ITEM)
		{
			if (keys & (DPAD_UP | DPAD_DOWN))
			{
				tField ^= 1;
				tResult = 0;
			}
			if (delta)
				tResult = 0;
			if (tField)
				tCount = Adjust(tCount, delta, 1, DEBUG_ITEM_COUNT_MAX);
			else
				tValue = Adjust(tValue, delta, 1, ITEMS_COUNT - 1);
			if (keys & A_BUTTON)
			{
				u8 name[64];
				CopyItemName(tValue, name);
				tResult = (name[0] != 0xAC && AddBagItem(tValue, tCount)) ? 1 : 2;
			}
		}
		else
		{
			tValue = Adjust(tValue, delta, 1, DEBUG_FLAG_MAX);
			if (keys & START_BUTTON)   //[hack] patches/0986 ジョウトの変数の窓の確かめ（0x5200 + 旗の番号の下 8 ビットを +1）
				VarSet(0x5200 + (tValue & 0xFF), VarGet(0x5200 + (tValue & 0xFF)) + 1);
			if (keys & A_BUTTON)
			{
				if (FlagGet(tValue))
					FlagClear(tValue);
				else
					FlagSet(tValue);
			}
		}
	}

	Draw(taskId);
}

//B を押しながら Select でメニューを開く（入力の処理＝素の CB1_Overworld の直前に呼ぶ・ネット交換の NetTrade_CB1 から）
void HackDebugMenu_TryOpen(void)
{
	u8 taskId;
	u16 window;
	s16* data;

	if (!(gMain.heldKeysRaw & B_BUTTON) || !(gMain.newKeysRaw & SELECT_BUTTON))
		return;
	if (ScriptContext2_IsEnabled() || gPaletteFade->active || FindTaskIdByFunc(Task_HackDebugMenu) != 0xFF)
		return;

	taskId = CreateTask(Task_HackDebugMenu, 80);
	if (taskId == 0xFF)
		return;
	window = AddWindow(&sDebugWindow);
	if (window == 0xFF)
	{
		DestroyTask(taskId);
		return;
	}

	data = gTasks[taskId].data;
	tWindow = window;
	tCursor = 0;
	tMode = MODE_TOP;
	tWaitA = TRUE;
	tHelp = gHelpSystemEnabled;
	gHelpSystemEnabled = FALSE;

	//このフレームの Select は入力の処理に渡さない（登録した道具を使わない）
	gMain.newKeys &= ~SELECT_BUTTON;
	gMain.newKeysRaw &= ~SELECT_BUTTON;
	gMain.newAndRepeatedKeys &= ~SELECT_BUTTON;

	ScriptContext2_Enable();
	Draw(taskId);
}

//★ 2026-10-12 gMain.callback1 の差し替え（HackDebugMenu_CB1・HackDebugMenu_FieldTick）はネット交換の NetTrade_CB1（src/hack_network_trade.c）に
//  まとめた（2 つが別々に差し替えると先に差し替えたほうだけが動く）。NetTrade_CB1 がこの HackDebugMenu_TryOpen を呼んでから素の CB1_Overworld を呼ぶ

bool8 HackDebugMenu_CatchAll(void)
{
	return FlagGet(HACK_FLAG_DEBUG_CATCH);
}
#endif
