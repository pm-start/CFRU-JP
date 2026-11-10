/* 生成物: tools/fishing/gen_nushi.py（元は tools/fishing/nushi_src/nushi_context.h・直に編集しない）。
 * 由来: liru 氏 pm-start/fishing-rework@aad8bcf（詳細は tools/fishing/README.md）。 */
#ifndef NUSHI_CONTEXT_H
#define NUSHI_CONTEXT_H
#include "hack_nushi_core.h"
/* Per-map/boat event context, independent of the selected equipment. */
static inline void NfApplyContextFlags(struct NfContext *ctx,nf_u16 flags){
 nf_u8 habitat=(flags>>2)&7,depth=(flags>>5)&3,time=(flags>>8)&7;
 ctx->rigMask=(1<<NF_REEL)|(1<<NF_LURE)|((flags&1)?1<<NF_BOAT:0)|((flags&2)?1<<NF_TROLL:0);
 if(habitat>=1&&habitat<=4)ctx->habitat=1u<<(habitat-1);
 ctx->maxDepth=depth?depth-1:((flags&3)||ctx->habitat==NF_DEEP_SEA)?2:1;
 if(time>=1&&time<=4)ctx->time=time-1;
}
#endif
