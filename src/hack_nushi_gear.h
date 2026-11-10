/* 生成物: tools/fishing/gen_nushi.py（元は tools/fishing/nushi_src/nushi_gear.h・直に編集しない）。
 * 由来: liru 氏 pm-start/fishing-rework@aad8bcf（詳細は tools/fishing/README.md）。 */
#ifndef NUSHI_GEAR_H
#define NUSHI_GEAR_H
#include "hack_nushi_core.h"

/* No new item IDs or save-block expansion. Bits 0..7 own the shop upgrades;
 * bit 15 marks migration of a previously saved configuration. */
#define NF_TACKLE_INITIALIZED 0x8000
enum NfPurchase { NF_BUY_OK, NF_BUY_OWNED, NF_BUY_NO_MONEY, NF_BUY_INVALID };
static const nf_u16 nf_tackle_prices[8]={300,800,500,300,800,2000,1500,4000};
static inline nf_u16 NfTackleForGear(const struct NfGear *gear){
 nf_u16 mask=0;
 if(gear->hook==1)mask|=1;if(gear->hook==2)mask|=2;
 if(gear->useFloat)mask|=4;
 if(gear->weight==1)mask|=8;if(gear->weight==2)mask|=16;
 if(gear->rig)mask|=1u<<(gear->rig+4);
 return mask;
}
/* field: 0 hook, 1 rig, 2 float, 3 weight. Starter values are always usable. */
static inline nf_u8 NfTackleAllows(nf_u16 owned,nf_u8 field,nf_u8 value){
 nf_u16 mask=0;
 if(!value)return 1;
 if(field==0&&value<=2)mask=1u<<(value-1);
 if(field==1&&value<=3)mask=1u<<(value+4);
 if(field==2&&value==1)mask=4;
 if(field==3&&value<=2)mask=1u<<(value+2);
 return mask&&(owned&mask)==mask;
}
#endif
