#ifndef GUARD_HACK_OAK_PHONE_H
#define GUARD_HACK_OAK_PHONE_H

/*
 * オーキドからの電話（平均レベル 170）。セーブ構造は増やさない。
 * 拡張フラグ 0xFD0 は台帳コメントが空きと書いていた枠（0xFCF の次）。定義はここだけ。
 * 0xFD1 は戦闘中だけオーキドの画像にする印（戦いの終わりに消す）。
 * 0xFD2 は、借りたトレーナーが元から倒されていたとき戻す印。
 * 0xFD5 は、いちど「いいえ」で断った印。立っている間はオーキドも 30 分の着信の枠（IsCallMoment）にだけ鳴る。
 * 0xFD3・0xFD4 はカントー HG の枝（task/MERGE-kanto-hg）がデバッグの旗に使う。
 * 古い台帳コメントの「次の空きは 0xFD0」は、その文言を見る検算のため残してある。実際の次は 0xFD6。
 */
#define HACK_FLAG_OAK_ACCEPTED  0xFD0
#define HACK_FLAG_OAK_PIC       0xFD1
#define HACK_FLAG_OAK_RESTORE   0xFD2
#define HACK_FLAG_OAK_DECLINED  0xFD5
#define HACK_PHONE_OAK_LEVEL    170
/* 手持ちは戦いの直前に作り直す。地図にいる人の旗は戦いの前後で戻す。 */
#define HACK_PHONE_OAK_TRAINER  1
/* オーキド戦の戦闘 BGM の曲の表の番号（後ろ半分の PCM の曲 x_oak_bw・src/hack_oak_bgm.c・検算 tools/bgm/verify_back_pcm.py が song_table.s と突き合わせる） */
#define HACK_OAK_BGM_SONG       544

void HackPhone_OakPrepareBattle(void);
void HackPhone_OakFinishBattle(void);
void HackPhone_OakOnBattleEnd(void);
void HackPhone_OakDeclined(void);
void HackPhone_OakFillParty(void);
u16 HackOak_BattleMusic(void);

#endif
