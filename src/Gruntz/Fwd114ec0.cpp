#include <Ints.h>

#include <Gruntz/GruntzMgr.h>          // the real CGruntzMgr (arg2's true class) + m_world chain
#include <Gruntz/GameRegistry.h>       // CDDrawSurfaceMgr (m_world's real class)
#include <DDrawMgr/DDrawSubMgrPages.h> // m_drawTarget (the ex-CWorldSub4 +0x4 child)
#include <DDrawMgr/DDrawSurfacePair.h> // m_frontPair (CDDrawSurfacePair: m_surface @+0x2c)
#include <Gruntz/SaveScreenshot.h>     // the real callee (thunk 0x267b -> 0x114ff0)

#include <rva.h>

void SaveFrontBufferShotImpl(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    void* arg7
);

RVA(0x00114ec0, 0x27)
void SaveFrontBufferShot(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    void* arg7
) {
    SaveFrontBufferShotImpl(bute, mgr, w, h, name, arg7);
}

// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns): cl shares one pop;ret
// tail across the two null guards; retail emits the inline ret at each site. Deref
// chain + 6-arg re-push forward are byte-faithful.
RVA(0x00114f00, 0x3e)
void SaveFrontBufferShotImpl(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    void* arg7
) {
    CDDrawSurfacePair* pair = mgr->m_world->m_drawTarget->m_frontPair;
    if (pair == 0) {
        return;
    }
    if (pair->m_surface == 0) {
        return;
    }
    // (the CGameRegistry cast is the documented CGruntzMgr/CGameRegistry dual-view of
    // the one 0x24556c singleton - SaveScreenshot's own signature names that view.)
    SaveScreenshot(pair->m_surface, bute, mgr, w, h, name, arg7);
}
