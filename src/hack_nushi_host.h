/* 生成物: tools/fishing/gen_nushi.py（元は tools/fishing/nushi_src/nushi_host.h・直に編集しない）。
 * 由来: liru 氏 pm-start/fishing-rework@aad8bcf（詳細は tools/fishing/README.md）。 */
#ifndef NUSHI_HOST_H
#define NUSHI_HOST_H
#include "hack_nushi_core.h"
struct NfView {
 nf_s16 frontX,frontY,playerX,playerY,screenX,screenY;
 nf_s16 dx,dy;
 nf_u16 palette[16],phaseDuration;
 nf_u8 sprites[5],palettes[5],lastState,panelTop;
 nf_u16 *objPaletteBackup;
};
nf_u32 NfHostFrame(void);
nf_u16 NfHostPressed(void);
nf_u16 NfHostHeld(void);
void NfHostBlockHelp(void);
nf_u8 NfHostFindTask(void(*)(nf_u8));
nf_u8 NfHostCreateTask(void(*)(nf_u8));
void NfHostDestroyTask(nf_u8);
nf_s16 *NfHostTaskData(nf_u8);
void *NfHostAlloc(nf_u32);
void NfHostFree(void*);
nf_u16 NfHostRandom(void);
void NfHostLock(void);
void NfHostUnlock(nf_u8 scriptManaged);
nf_u16 NfHostReadConfig(void);
void NfHostWriteConfig(nf_u16);
nf_u16 NfHostReadTackle(void);
nf_u32 NfHostMoney(void);
nf_u8 NfHostBuyTackle(nf_u8);
void NfHostContext(struct NfView*,struct NfContext*);
nf_u16 NfHostEntries(struct NfEntry*,nf_u16,const struct NfGear*,const struct NfContext*);
nf_u8 NfHostHasBait(nf_u8);
nf_u8 NfHostSpendBait(nf_u8);
void NfHostSound(nf_u16 events);
void NfHostPaletteSave(struct NfView*);
void NfHostPaletteRestore(struct NfView*);
void NfHostPaletteApply(void);
void NfHostRainbow(nf_u32);
nf_u8 NfHostWindow(nf_u8 full,nf_u8 top);
void NfHostRemoveWindow(nf_u8);
void NfHostFill(nf_u8,nf_u8);
void NfHostRect(nf_u8,int,int,int,int,int);
void NfHostText(nf_u8,int,int,const nf_u8*,nf_u8 color);
void NfHostFlush(nf_u8);
void NfHostMakeShadows(struct NfView*,const struct NfGame*);
void NfHostShadows(struct NfView*,const struct NfGame*);
void NfHostHideShadows(struct NfView*);
void NfHostDestroyShadows(struct NfView*);
void NfHostGenerate(const struct NfCatch*,nf_u8 rod);
void NfHostBattle(nf_u8 rod);
nf_u8 NfHostGiveItem(nf_u16);
void NfHostCatchName(const struct NfCatch*,nf_u8*);
void NfHostLegacy(nf_u8 rod);
void NfHostGauge(nf_u8 rod);
nf_u8 NfUiOpen(nf_u8 rod,nf_u8 setup,nf_u8 scriptManaged);
void NfUiGaugeOpen(nf_u8 rod);
void NfHostResult(nf_u8);
void NfUiTask(nf_u8 id);
nf_u8 NfUiMode(void);
#endif
