#include "defines.h"
#include "../include/bg.h"
#include "../include/field_message_box.h"
#include "../include/map_name_popup.h"
#include "../include/script.h"
#include "hack_player_kind.h"

/*
 * メイ（男の娘）専用のせりふ（FEAT-player-mei-3・docs/research/2026-10-19_player-mei-dialog.md）。
 *
 * 仕組み: スクリプト命令 `message`（0x67・msgbox の中身）を、文の番地を表で引いてから ShowFieldMessage に渡す版に置き換える。
 *   - メイ（GetPlayerGender3()==2）のときだけ表を引く。男・女のときは素のまま（表を見ない）。
 *   - メイは土台の playerGender=1（女）なので、checkplayergender の分岐は「女の枝」に入り、その文の番地が表のキーになる
 *     （素の 2 本型の分岐が 2 を返すと素通りする問題を避けるため、土台は女のまま・メイの判定は印で見る）。
 *   - 「くん／ちゃん」の {くん}（FD 05）は土台が女なので「ちゃん」になる。「さん」「くん」にしたい文と、書き直した文は表に入れた
 *     （お母さんの 1 文の「くん」は文に直書き）。
 *   - 表の文は strings/hack_mei_dialog.string・表は src/hack_mei_dialog_table.h（tools/rom/build_mei_dialog.py が生成）。
 * 元の ScrCmd_message（src/scripting.c）と同じ動き（マップ名の表示を消す・msg==NULL のときは ctx->data[0]）に、差し替えを足しただけ。
 */

struct MeiDialogEntry
{
	const u8* from;
	const u8* to;
};

#include "hack_mei_dialog_table.h"

const u8* HackMeiDialog_Replace(const u8* msg)
{
	u32 i;

	if (GetPlayerGender3() != PLAYER_GENDER3_MEI)
		return msg;

	for (i = 0; i < sizeof(sMeiDialogTable) / sizeof(sMeiDialogTable[0]); ++i)
	{
		if (sMeiDialogTable[i].from == msg)
			return sMeiDialogTable[i].to;
	}
	return msg;
}

bool8 HackMeiDialog_ScrCmdMessage(struct ScriptContext* ctx)
{
	const u8* msg = (const u8*) ScriptReadWord(ctx);

	if (IsMapNamePopupTaskActive())
	{
		ChangeBgY(0, 0, 0);
		DismissMapNamePopup();
	}

	if (msg == NULL)
		msg = (const u8*) ctx->data[0];

	ShowFieldMessage(HackMeiDialog_Replace(msg));
	return FALSE;
}
