#include "defines.h"
#include "../include/global.h"
#include "../include/new/ram_locs.h"
#include "hack_session_key.h"

/*
    お金・コインの「セッションの鍵」（patches/0627・CLAUDE.md 6.103・docs/research/2026-10-20_anticheat-stage2.md）

    素の FRLG は SaveBlock2 の encryptionKey（+0xF20）で お金・コイン・ゲームの記録・バッグの数・きのみ粉・トレーナータワーの記録を XOR し、
    地図の読み込み・戦闘の開始のたびに（MoveSaveBlocks_ResetHeap）新しい乱数の鍵へ付け替える。CFRU は 0804B8F6 で鍵を 0 にして切っていた。
    今は鍵を戻した（bytereplacement の RAMSHIFT 塊）。守るもの:
      お金      SaveBlock1 +0x290 … 素の GetMoney／AddMoney／RemoveMoney／IsEnoughMoney が鍵で XOR（呼び元は全部これを通る・src の `&gSaveBlock1->money` 21 か所）
      コイン    CFRU は SaveBlock1 の coins を使わず、拡張の保存領域の gPlayerCoins（0x0203B78C）に平文で持つ → GetCoins／SetCoins（scripting.c）が鍵で XOR
      記録      GetGameStat／SetGameStat／IncrementGameStat（素・64 個）
      きのみ粉・トレーナータワー  素のアクセサ
    守らないもの: バッグの数（CFRU の item.c が quantity を平文で直に読み書きする 18 か所。素の GetBagItemQuantity も CFRU が平文に差し替え済み）。
      素のバッグの付け替え ApplyNewEncryptionKeyToBagItems（08099840）は bytereplacement で bx lr にして何もさせない。

    フラッシュの形は変えない: SaveWriteToFlash の前後で鍵を 0 に付け替えて書く（HackKey_SaveBegin／End）。
      書かれるのは今まで（鍵 0・平文）と同じバイト。古い版は鍵 0 の平文を読み、鍵は つづきから のあと最初の地図の読み込みで新しい乱数に変わる。
*/

typedef void (*ApplyWordFn)(u32* word, u32 key);
typedef void (*ApplyHwordFn)(u16* hword, u32 key);
typedef void (*ApplyKeyFn)(u32 key);
#define ApplyNewEncryptionKeyToWord_ ((ApplyWordFn) (0x0804BD40 | 1))
#define ApplyNewEncryptionKeyToHword_ ((ApplyHwordFn) (0x0804BD24 | 1))
#define ApplyNewEncryptionKeyToGameStats_ ((ApplyKeyFn) (0x080547F8 | 1))
#define ApplyNewEncryptionKeyToBerryPowder_ ((ApplyKeyFn) (0x0816217C | 1))
#define TRAINER_TOWER_WORD_OFF 0x3D38        /* SaveBlock1 +0x3D38（素のループは 1 回・bestTime） */

//素の ApplyNewEncryptionKeyToAllEncryptedData（0804BD5C）の置き換え（functionrewrites）。呼ぶ側は新しい鍵を渡し、
//この関数のあとで gSaveBlock2->encryptionKey に書く（素の MoveSaveBlocks_ResetHeap）＝この中では SB2 の鍵はまだ古い鍵
void HackKey_ApplyAll(u32 key)
{
	u32 old = gSaveBlock2->encryptionKey;

	gPlayerCoins ^= old ^ key;                                          //コイン（CFRU の拡張の保存領域）
	ApplyNewEncryptionKeyToWord_((u32*) ((u8*) gSaveBlock1 + TRAINER_TOWER_WORD_OFF), key);
	ApplyNewEncryptionKeyToGameStats_(key);
	//ApplyNewEncryptionKeyToBagItems は呼ばない（CFRU のバッグは鍵を使わない）
	ApplyNewEncryptionKeyToBerryPowder_(key);
	ApplyNewEncryptionKeyToWord_(&gSaveBlock1->money, key);
	ApplyNewEncryptionKeyToHword_(&gSaveBlock1->coins, key);
}

//セーブを書く直前: 鍵があるなら全部を素に戻して鍵を 0 に（書くのは今までと同じバイト）
void HackKey_SaveBegin(struct HackKeySave* b)
{
	b->key = gSaveBlock2->encryptionKey;
	if (b->key != 0)
	{
		HackKey_ApplyAll(0);
		gSaveBlock2->encryptionKey = 0;
	}
}

//書き終えたら元の鍵へ（このセッションは同じ鍵のまま続く）
void HackKey_SaveEnd(const struct HackKeySave* b)
{
	if (b->key != 0)
	{
		HackKey_ApplyAll(b->key);
		gSaveBlock2->encryptionKey = b->key;
	}
}
