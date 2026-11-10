@ ★ 生成物（手で直さない）: tools/build/gen_stat_caps.py が tools/stat_caps/stat_caps.json から作る。
@ asm_defines.s が .include する（[hack] patches/0677）。C の src/hack_stat_caps.h と同じ値。

.equ HACK_EV_PER_STAT_CAP, 252 @ 努力値 1 つの上限
.equ HACK_EV_TOTAL_CAP, 510 @ 努力値の合計の上限
.equ HACK_IV_MAX, 31 @ 個体値の上限（保存する値）
.equ HACK_IV_BONUS_EXTRA, 32 @ Lv100 をこえたとき個体値に足せる最大（差を保つ方式・#128）
.equ HACK_IV_BONUS_CEILING, 63 @ 実効の個体値の上限（IV_MAX + BONUS_EXTRA）
.equ HACK_BASE_STAT_MAX, 255 @ 種族値（u8 の欄）の上限・スケールモンの頭打ち
.equ HACK_MAX_LEVEL, 200 @ レベルの上限（MAX_LEVEL）
.equ HACK_EV_DIGITS, 3 @ 努力値の表示の桁数
.equ HACK_IV_DIGITS, 2 @ 個体値の表示の桁数
.equ HACK_LEVEL_DIGITS, 3 @ レベルの表示の桁数
.equ HACK_EV_FIELD_MAX, 255 @ 努力値の u8 の欄（手持ち・BattleTowerSpread など）の入れ物の上限＝上限そのものではない
.equ HACK_STAT_VALUE_MAX, 65535 @ 能力値・種族値・努力値・個体値の u16 の入れ物の上限

@ 旧名（special_inserts.asm ほかが使う）
.equ MAX_LEVEL, HACK_MAX_LEVEL
.equ EV_CAP, HACK_EV_PER_STAT_CAP

@ 入れ物の幅の守り（C のヘッダと同じ条件）
.if !(HACK_EV_PER_STAT_CAP <= 255)
.err
.endif
@  ↑ 努力値 1 つが手持ち・80 バイト形の u8 の欄に入らない（段階 T1 の実装が要る）
.if !(HACK_IV_MAX <= 31)
.err
.endif
@  ↑ 個体値が手持ち・80 バイト形の 5 ビットの欄に入らない（段階 T1 の実装が要る）
.if !(HACK_EV_TOTAL_CAP <= 6 * HACK_EV_PER_STAT_CAP)
.err
.endif
@  ↑ 努力値の合計が 6 つの上限の和をこえている（意味が無い設定）
.if !(HACK_BASE_STAT_MAX <= 255)
.err
.endif
@  ↑ 種族値の u8 の欄（gBaseStats・DPE のデータ）に入らない。gBaseStatsWide の上書きを使う
.if !(HACK_MAX_LEVEL <= 254)
.err
.endif
@  ↑ 手持ちの保存のレベル（経験値の表は 254 番まで・+0x54 の u8）に入らない。案を再検討する
.if !(HACK_MAX_LEVEL <= 65535)
.err
.endif
@  ↑ 戦闘・計算のレベル（u16）に入らない
.if !(HACK_IV_BONUS_CEILING <= 65535)
.err
.endif
@  ↑ 計算の u16 の入れ物に入らない
