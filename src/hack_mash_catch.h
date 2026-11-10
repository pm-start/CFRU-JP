#pragma once

//A ボタン連打で捕獲率が上がる隠し機能（src/hack_mash_catch.c・patches/0423）
void HackMashCatch_Begin(u16 item);     //ボールを使うと決まったターンの始めに（連打を数えるタスクを動かす）
void HackMashCatch_Finish(void);        //捕獲の命令が呼ばれたとき（数え終わり）
u16 HackMashCatch_Bonus100(void);       //ballMultiplier（10 = 1 倍）に足す値を 1/100 単位で（連打 10 回で 50 = +0.5）
void HackMashCatch_Clear(void);         //判定のあとに回数を 0 に戻す

#ifdef HACK_DEBUG_MENU
bool8 HackMashCatch_AppliesToBall(u8 ballType); //デバッグ版だけ: そのボールで連打が効くか（トレーナー戦など効かない場面も含む）
#endif
