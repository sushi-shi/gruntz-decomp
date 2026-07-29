#ifndef GRUNTZ_CSOUNDFXEMITTER_H
#define GRUNTZ_CSOUNDFXEMITTER_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/FaderMgr.h>   // CFaderMgr::Add / Remove + the minimal CFader
#include <Gruntz/FxModeDesc.h> // CFxModeT2 / CFxModeT3 transition descriptors
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawSurfacePair.h> // the ONE CDDrawSurfacePair shape (m_surface @+0x2c)
#include <DDrawMgr/DDrawSubMgrPages.h>

extern "C" i32 g_disableFades;

void ActiveWait(u32 milliseconds); // 0x13dfe0 busy-wait

// (the ex FxResource pad-struct is DISSOLVED: it was CDDrawSurfaceMgr viewed
// through CState::m_world - +0x04 m_drawTarget, +0x1c m_ptrColl.)

class CSoundFxEmitter {
public:
    // The CFxModeT2 slots the two centre args feed are CFaderLight::ApplyInit's
    // m_centerX/m_centerY (0x1804a0); `dur`/`lead` go straight into
    // CFader::RunFade(dur, lead, vsync).
    i32 FadeSceneClear1(i32 centerX, i32 centerY, i32 dur, i32 lead); // 0xfa410
    i32 FadeScene1(i32 centerX, i32 centerY, i32 dur, i32 lead);      // 0xfa550
    // `pct` is the CFxModeT3 m_10 CFaderSine::ApplyInit range-checks to 0..100.
    i32 FadeScene2(i32 pct, i32 dur, i32 lead); // 0xfa790
    // (0xfa8f0 was Method_fa8f0 - HOISTED to CState::RetireScene: it is called on every
    //  screen state's own `this`, so it is a CState-level helper, not this facet's. The
    //  other four here have no such cross-state caller set and stay on this emitter view.)
    i32 FadeSceneClear2(i32 pct, i32 dur, i32 lead); // 0xfaa60

    char _00[0x04];
    class CGruntzMgr* m_gameMgr; // +0x04 the game-manager singleton (real class; the
                                 //        elaborated-type-specifier keeps this header MFC-free)
    char _08[0x04];
    // +0x0c the DDraw surface manager (mirrors CState::m_world - this facet views
    // the same object; real class, ex the FxResource pad-struct)
    class CDDrawSurfaceMgr* m_resChain;
    CFaderMgr* m_faderMgr; // +0x10 fader manager
};
SIZE_UNKNOWN();

extern "C" i32 g_disableFades;
#endif // GRUNTZ_CSOUNDFXEMITTER_H
