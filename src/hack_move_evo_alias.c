/* 生成物。tools/moves/move_evo_newmoves.py が docs/research/move-evo/new_moves.csv の alias_to 列から作る。手で編集しないこと。
   技進化の新技（0x1410〜）のうち、効果元の本家の技の特別処理（ドラゴンテールの入れ替え等）を受け継ぐ物の「効果元」を返す。EffectiveMove／MoveIs の実体（src/hack_move_alias.h）。 */
#include "defines.h"
#include "../include/constants/moves.h"
#include "hack_move_alias.h"

u16 HackEffectiveMove(u16 move)
{
	switch (move)
	{
		case MOVE_DORAGONJABU: return MOVE_DRAGONTAIL;
		case MOVE_RYUUNOKIBA: return MOVE_DRAGONTAIL;
		case MOVE_WEZAASHOTTO: return MOVE_WEATHERBALL;
		case MOVE_WEZAABURASUTO: return MOVE_WEATHERBALL;
		case MOVE_HIBIKINOUTA: return MOVE_ECHOEDVOICE;
		case MOVE_TODOROKIBOISU: return MOVE_ECHOEDVOICE;
		case MOVE_FUKIAGEKAZE: return MOVE_GUST;
		case MOVE_FUKISUSABUKAZE: return MOVE_GUST;
		case MOVE_UZUMAKINOIKI: return MOVE_TWISTER;
		case MOVE_RYUUNOTSUMUJI: return MOVE_TWISTER;
		case MOVE_HEDOROUNERI: return MOVE_VENOSHOCK;
		case MOVE_NOROINOKAGE: return MOVE_HEX;
		case MOVE_CHIKUSEKIPAWAA: return MOVE_STOREDPOWER;
		case MOVE_CHIKUSEKIWEEBU: return MOVE_STOREDPOWER;
		case MOVE_TAKERINOKIBA: return MOVE_POWERTRIP;
		case MOVE_TAKERINOAGITO: return MOVE_POWERTRIP;
		case MOVE_KIRIKIZAMI: return MOVE_FURYCUTTER;
		case MOVE_MIDAREBASAMI: return MOVE_FURYCUTTER;
		case MOVE_OIUCHIGIRI: return MOVE_ASSURANCE;
		case MOVE_OIUCHIBUROO: return MOVE_ASSURANCE;
		case MOVE_SHIKAESHIDUKI: return MOVE_PAYBACK;
		case MOVE_YARIKAESHI: return MOVE_PAYBACK;
		case MOVE_LV200_NORMAL_122: return MOVE_WEATHERBALL;
		case MOVE_LV200_DRAGON_106: return MOVE_TWISTER;
		case MOVE_LV200_POISON_110: return MOVE_VENOSHOCK;
		case MOVE_LV200_GHOST_103: return MOVE_HEX;
		case MOVE_LV200_PSYCHIC_105: return MOVE_STOREDPOWER;
		case MOVE_LV200_DARK_138: return MOVE_POWERTRIP;
		case MOVE_LV200_DARK_103: return MOVE_ASSURANCE;
		case MOVE_LV200_DARK_107: return MOVE_PAYBACK;
		case MOVE_LV200_WATER_112: return MOVE_SURF;
	}
	return Lv200MoveAlias(move);
}
