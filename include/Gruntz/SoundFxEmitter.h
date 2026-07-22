#ifndef GRUNTZ_CSOUNDFXEMITTER_H
#define GRUNTZ_CSOUNDFXEMITTER_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/FaderMgr.h>   // CFaderMgr::Add / Remove + the minimal CFader
#include <Gruntz/FxModeDesc.h> // CFxModeT2 / CFxModeT3 transition descriptors

extern "C" i32 g_disableFades;

#include <DDrawMgr/DDSurface.h>

#include <Gruntz/GameRegistry.h>

#include <DDrawMgr/DDrawSurfacePair.h> // the ONE CDDrawSurfacePair shape (m_surface @+0x2c)
#include <DDrawMgr/DDrawSubMgrPages.h>

void ActiveWait(u32 milliseconds); // 0x13dfe0 busy-wait

struct FxResource {
    char _00[0x04];
    CDDrawSubMgrPages* m_worker; // +0x04 the shared DDraw worker manager
    char _08[0x14];
    i32 m_gate; // +0x1c gate field (must be set to proceed)
};
SIZE_UNKNOWN();

class CSoundFxEmitter {
public:
    i32 FadeSceneClear1(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa410
    i32 FadeScene1(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa550
    i32 FadeScene2(i32 a1, i32 a2, i32 a3);         // 0xfa790
    // (0xfa8f0 was Method_fa8f0 - HOISTED to CState::RetireScene: it is called on every
    //  screen state's own `this`, so it is a CState-level helper, not this facet's. The
    //  other four here have no such cross-state caller set and stay on this emitter view.)
    i32 FadeSceneClear2(i32 a1, i32 a2, i32 a3); // 0xfaa60

    char _00[0x04];
    class CGruntzMgr* m_gameMgr; // +0x04 the game-manager singleton (real class; the
                                 //        elaborated-type-specifier keeps this header MFC-free)
    char _08[0x04];
    FxResource* m_resChain; // +0x0c resource chain root
    CFaderMgr* m_faderMgr;  // +0x10 fader manager
};
SIZE_UNKNOWN();

extern "C" i32 g_disableFades;
#endif // GRUNTZ_CSOUNDFXEMITTER_H
