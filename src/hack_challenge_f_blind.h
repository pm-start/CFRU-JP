#pragma once
#include "../include/global.h"

/*
    しばり「めかくしバトル」「いっぱつしょうぶ」（縛り F・0.63.0・patches/0951 仮・docs/research/2026-11-03_challenge-f-blind-oneshot.md）。
    旗は担当 A の hack_challenge_f.h の HACK_FLAG_CHF_BLIND／HACK_FLAG_CHF_ONESHOT（番地は台帳 docs/research/flag-ledger.md）。ここでは番号を書かない。
    変数・保存の欄は増やさない（旗だけ）。static も持たない（CLAUDE.md 6.13）。
*/
bool8 HackChallengeF_BlindBattler(u8 battlerId);   /* めかくし: その戦闘位置は 相手側で、今の戦いは ？？？表示の対象か（bw_hpbar.c の名前・Lv・HP バー・印） */
bool8 HackChallengeF_OneShotApplies(void);         /* いっぱつ: ボールが外れた今の戦いは 野生が逃げる対象か（catching.c の atkEF_handleballthrow の「外れ」の分岐） */
