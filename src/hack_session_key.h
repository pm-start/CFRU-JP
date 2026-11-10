#pragma once
#include "../include/global.h"

/* patches/0627・src/hack_session_key.c */
struct HackKeySave
{
	u32 key;
};
void HackKey_ApplyAll(u32 key);                          /* 素の ApplyNewEncryptionKeyToAllEncryptedData（0804BD5C）の置き換え */
void HackKey_SaveBegin(struct HackKeySave* b);           /* セーブを書く直前: 鍵を 0 に（SaveWriteToFlash） */
void HackKey_SaveEnd(const struct HackKeySave* b);       /* 書き終えたら元の鍵へ */
