#ifndef GRUNTZ_CSOUNDFXEMITTER_H
#define GRUNTZ_CSOUNDFXEMITTER_H

#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FxModeDesc.h>
#include <Gruntz/GameRegistry.h>
#include <Ints.h>

extern "C" i32 g_disableFades;

void ActiveWait(u32 milliseconds);

class CSoundFxEmitter {
public:
    i32 FadeSceneClear1(i32 centerX, i32 centerY, i32 dur, i32 lead);
    i32 FadeScene1(i32 centerX, i32 centerY, i32 dur, i32 lead);

    i32 FadeScene2(i32 pct, i32 dur, i32 lead);

    i32 FadeSceneClear2(i32 pct, i32 dur, i32 lead);

    char _00[0x04];
    class CGruntzMgr* m_gameMgr;

    char _08[0x04];

    class CDDrawSurfaceMgr* m_resChain;
    CFaderMgr* m_faderMgr;
};
SIZE_UNKNOWN();
#endif // GRUNTZ_CSOUNDFXEMITTER_H
