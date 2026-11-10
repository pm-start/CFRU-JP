#pragma once

//技進化（独自）段 1（src/hack_move_evo.c・patches/0864・Redmine なし・FEAT-move-evo-s1）
//戦闘中に使った技が、確率でターンの終わりに強い技へ変わる。道具なし・自分の子だけ・野生とふつうのトレーナーのシングルだけ。

void HackMoveEvo_OnMoveEnd(u8 arg1);   //cmd49.c の ATK49_MOVE_EVO から: 抽選して「保留」を記録するだけ（技の枠は書かない）
void HackMoveEvo_OnBattleEnd(void);    //end_battle.c の EndOfBattleThings の頭から: 使用回数の変わった分だけ表へ書く（patches/0878）
bool8 HackMoveEvo_TryApply(u8 bank);   //end_turn.c の ET_MoveEvo から: 保留があれば枠を差し替えて台本を積む（TRUE）

//技の熟練度（src/hack_move_evo.c・patches/0932・FEAT-move-mastery）。戦闘中の補正は「自分の側・数える戦闘と同じ許可リスト・その子の技の欄が『じゅくれん』・段 1 以上」のときだけ効く
u16 HackMoveMastery_PowerBonus(u8 bank, u16 move);            //damage_calc.c AdjustBasePower: 威力に足す分
u8 HackMoveMastery_Accuracy(u8 bank, u16 move, u8 acc);       //accuracy_calc.c: 命中型の命中（100 で止める・必中は変えない）
u32 HackMoveMastery_EffectChance(u8 bank, u16 move, u32 chance); //set_effect.c: 追加効果型の確率（100 で止める）
u8 HackMoveMastery_CritBonus(u8 bank, u16 move);              //damage_calc.c: 急所型の急所ランクの足し算
bool8 HackMoveMastery_TryAnnounce(u8 bank);                   //end_turn.c ET_MoveEvo: 段が上がったお知らせ
struct Pokemon;
u8 HackMoveMastery_SummaryState(struct Pokemon* mon, u16 move, u8* levelOut);   //サマリー: 0 = 対象外・1 = しんか・2 = じゅくれん（段 0〜10）
bool8 HackMoveMastery_SummarySet(struct Pokemon* mon, u16 move);                //「じゅくれん」にする（一方通行）
extern const u8 BattleScript_MoveMastery[];
extern const u8 BattleScript_MoveMasteryMax[];
extern const u8 gText_MoveMastery[];
extern const u8 gText_MoveMasteryMax[];
extern const u8 gText_MasteryShinka[];
extern const u8 gText_MasteryJukuren[];
extern const u8 gText_MasteryConfirm[];
extern const u8 gText_MasteryLocked[];
extern const u8 gText_MasteryHint[];
extern const u8 gText_MasteryForgetWarn[];

extern const u8 BattleScript_MoveEvo[];
extern const u8 gText_MoveEvo[];
