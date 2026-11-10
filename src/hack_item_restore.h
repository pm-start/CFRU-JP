#pragma once
#include "../include/global.h"

/*
    使いきった持ち物（きのみ以外）を戦いのあとに手持ちへ戻す。patches 番号は台帳待ち・docs/research/2026-10-19_consumed-items-restore.md

    ・記録: 持ち物が「使われて消えた」道（atk6A_removeitem・ひやしのこがし系の BURN_BERRY）が HackItemRestore_NoteConsumed を呼ぶ＝自分の側の子の もとの持ち物と同じものが消えたときだけ印を立てる
    ・戻し: EndOfBattleThings の RestoreNonConsumableItems の最後（HackItemRestore_Apply）。印のある子の持ち物が空のときだけ もとの持ち物を戻す（戦いの中で拾った別の物は消さない）
    ・手持ちの位置は戦いの前に写した性格値（PID）で引く＝戦いの中で並びが変わっても 別の子へ戻さない
    ・戻さないもの: きのみ（今までどおり）・あぶないはっぱ／にのや（使ったら戻らない決まり）・はたきおとす／どろぼう／トリック（使われていない＝今までどおり）
    ・戻さない戦い: 通信・フロンティア・施設・トレーナータワー・e リーダー・バトルチューブ（今までどおり）
*/

bool8 HackItemRestore_IsRestorable(u16 item);              /* 使いきったあと戻す持ち物か（きのみ・使ったら戻らない道具・空は FALSE） */
void HackItemRestore_Save(void);                           /* 戦いの前（SavePartyItems の直後）: 手持ちの PID を写す・印を消す */
void HackItemRestore_NoteConsumed(u8 bank, u16 item);      /* 使われて消えた持ち物を記録（bank は消えた側・自分の側だけ） */
void HackItemRestore_Apply(void);                          /* 戦いの終わり（RestoreNonConsumableItems の最後）: 印のある子へ戻す */
