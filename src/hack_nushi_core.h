/* 生成物: tools/fishing/gen_nushi.py（元は tools/fishing/nushi_src/nushi_core.h・直に編集しない）。
 * 由来: liru 氏 pm-start/fishing-rework@aad8bcf（詳細は tools/fishing/README.md）。 */
#ifndef NUSHI_CORE_H
#define NUSHI_CORE_H

/* Engine independent. No writable globals or save-layout assumptions. */
typedef unsigned char nf_u8;
typedef unsigned short nf_u16;
typedef signed short nf_s16;
typedef unsigned int nf_u32;

enum NfState {NF_AIM,NF_CAST,NF_APPROACH,NF_ORBIT,NF_TEASE,NF_STRIKE,NF_FIGHT,NF_RESULT};
enum NfResult {NF_NONE,NF_LANDED,NF_CANCELLED,NF_DIRECT_HIT,NF_NO_BITE,NF_EARLY,NF_LATE,NF_LINE_BROKE,NF_HOOK_LOST,NF_TIMEOUT,NF_NO_BAIT,NF_WRONG_RIG,NF_TOO_DEEP};
enum NfAction {NF_RUN,NF_REST,NF_THRASH};
enum NfHabitat {NF_RIVER=1,NF_SEA=2,NF_DEEP_SEA=4,NF_LAKE=8};
enum NfRig {NF_REEL,NF_BOAT,NF_LURE,NF_TROLL};
enum {NF_A=1,NF_B=2,NF_RIGHT=16,NF_LEFT=32,NF_UP=64,NF_DOWN=128};
enum NfEvent {NF_EV_CAST=1,NF_EV_NIBBLE=2,NF_EV_LONG_BEEP=4,NF_EV_HOOKED=8,NF_EV_WARNING=16,NF_EV_RESULT=32,NF_EV_CONSUME_BAIT=64};
struct NfGear {nf_u8 rod,hook,bait,rig,weight,useFloat;};
struct NfContext {nf_u8 habitat,time,rigMask,maxDepth;nf_u32 waterMask;};
struct NfEntry {
 nf_u16 species,item,weight;
 nf_u8 minLevel,maxLevel,rodMask,depthMask,timeMask,habitatMask,rigMask,hookMask,baitMask,size,rarity;
};
struct NfCatch {nf_u16 species,item;nf_u8 level,size,rarity,sourceSlot;};
struct NfShadow {nf_u8 x,y;struct NfCatch fish;};
struct NfGame {
 nf_u32 rng,frames,waterMask;
 struct NfGear gear;
 struct NfShadow shadows[3];
 struct NfCatch fish;
 nf_u16 timer,actionTimer,elapsedFight,stamina,distance,tension,missedRest,overload,events;
 nf_s16 orbitX,orbitY;
 nf_u8 state,result,action,shadowCount,target,cursorX,cursorY,floatX,floatY;
 nf_u8 baitSpent,hookPenalty,safeReels,restUsed,tick,perfect,castCount,actionAge;
};

nf_u16 NfRandom(struct NfGame*);
nf_u8 NfDepth(const struct NfGear*);
nf_u16 NfPackGear(const struct NfGear*,nf_u8 mode,nf_u8 bonus);
void NfUnpackGear(nf_u16,struct NfGear*,nf_u8 rod);
nf_u16 NfEntryWeight(const struct NfEntry*,const struct NfGear*,const struct NfContext*);
int NfSelect(struct NfGame*,const struct NfEntry*,nf_u16,const struct NfContext*,struct NfCatch*);
void NfInit(struct NfGame*,const struct NfGear*,nf_u32 seed,nf_u32 waterMask);
nf_u8 NfSpawn(struct NfGame*,const struct NfCatch*,nf_u8 count);
void NfStep(struct NfGame*,nf_u16 pressed,nf_u16 held,nf_u8 frames);
void NfRejectBait(struct NfGame*);

#endif
