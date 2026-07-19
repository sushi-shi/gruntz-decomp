// Fwd114ec0.cpp - the world-present toolbar-builder forwarder pair (its own dev obj:
// the contiguous 0x114ec0-0x114f3e .text block). Carved out of GruntzMgrCmd.cpp in
// REHOME D9 (that file's real body is CGruntzMgr::HandleCommand @0x862f0, whose
// WM_COMMAND 0x8070 path calls Fwd114ec0 via the 0x277a thunk).
//
// Fwd114ec0 @0x114ec0 - straight 6-arg forwarder to the guarded forwarder Fwd114f00.
// Fwd114f00 @0x114f00 - the SCREENSHOT dispatch: arg2 IS the CGruntzMgr `this`
// (HandleCommand's case-0x8070 passes it), the chain is m_world
// (CDDrawSurfaceMgr) -> m_drawTarget (+0x4) -> m_frontPair (+0x10, the FRONT
// CDDrawSurfacePair) -> m_surface (+0x2c, its held CDDSurface), and the callee
// thunk 0x267b IS ?SaveScreenshot@@ @0x114ff0 (the very next fn in this band) -
// so the six args are SaveScreenshot's own tail (settings helper, owner, mode
// W/H, name, arg7). The ex-`CObj114f` placeholder (identity-TODO) and the
// extern-"C" Func267b hex thunk are DISSOLVED (2026-07-16). Both __cdecl.
#include <Ints.h>

#include <Gruntz/GruntzMgr.h>          // the real CGruntzMgr (arg2's true class) + m_world chain
#include <Gruntz/GameRegistry.h>       // CDDrawSurfaceMgr (m_world's real class)
#include <DDrawMgr/DDrawSubMgrPages.h> // m_drawTarget (the ex-CWorldSub4 +0x4 child)
#include <DDrawMgr/DDrawSurfacePair.h> // m_frontPair (CDDrawSurfacePair: m_surface @+0x2c)
#include <Gruntz/SaveScreenshot.h>     // the real callee (thunk 0x267b -> 0x114ff0)

#include <rva.h>

void Fwd114f00(Utils::RegistryHelper* bute, CGruntzMgr* mgr, i32 w, i32 h, char* name, void* arg7);

// 0x114ec0 - straight 6-arg forwarder to the guarded forwarder Fwd114f00 (via the
// 0x21c1 thunk). The HandleCommand screenshot path (thunk 0x277a) calls this.
RVA(0x00114ec0, 0x27)
void Fwd114ec0(Utils::RegistryHelper* bute, CGruntzMgr* mgr, i32 w, i32 h, char* name, void* arg7) {
    Fwd114f00(bute, mgr, w, h, name, arg7);
}

// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns): cl shares one pop;ret
// tail across the two null guards; retail emits the inline ret at each site. Deref
// chain + 6-arg re-push forward are byte-faithful.
RVA(0x00114f00, 0x3e)
void Fwd114f00(Utils::RegistryHelper* bute, CGruntzMgr* mgr, i32 w, i32 h, char* name, void* arg7) {
    CDDrawSurfacePair* pair = mgr->m_world->m_drawTarget->m_frontPair;
    if (pair == 0) {
        return;
    }
    if (pair->m_surface == 0) {
        return;
    }
    // (the CGameRegistry cast is the documented CGruntzMgr/CGameRegistry dual-view of
    // the one 0x24556c singleton - SaveScreenshot's own signature names that view.)
    SaveScreenshot(pair->m_surface, bute, reinterpret_cast<CGameRegistry*>(mgr), w, h, name, arg7);
}
