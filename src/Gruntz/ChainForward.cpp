// ChainForward.cpp - 0x114fa0 / 0x114f50: the two null-guarded pre-forwarders in
// front of SaveScreenshot (0x114ff0). Each resolves the active capture surface off
// the game registry - g_gameReg->m_world->m_drawTarget->{overlay,back}Pair->m_surface -
// and, if it (and every link) is intact, forwards it + all six pass-through args to
// SaveScreenshot.
//
// XREF IDENTITY (2026-07-14): the arg2 root was a fake `ChainRoot`/`Chain4`/`Chain18`/
// `Chain2c` view chain; it is the real DDraw resource spine:
//   * arg2 is forwarded unchanged to SaveScreenshot's 3rd param, whose retail mangling
//     is PAUCGameRegistry -> arg2 IS CGameRegistry* (owner).
//   * +0x30  = CGameRegistry::m_world      (CDDrawSurfaceMgr*)
//   * +0x04  = CDDrawSurfaceMgr::m_drawTarget(CDDrawSubMgrPages*)
//   * +0x18/+0x14 = CDDrawSubMgrPages::m_overlayPair / m_backPair (CDDrawSurfacePair*)
//   * +0x2c  = CDDrawSurfacePair::m_surface (CDDSurface* = SaveScreenshot's 1st param).
// The six forwarded params map 1:1 onto SaveScreenshot's (bute, owner, arg4, arg5,
// name, arg7); retyping them is byte-neutral (all 4-byte pushes) and drops the arg
// casts at the call.
#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (m_backPair/m_overlayPair)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair::m_surface (CDDSurface*)
#include <Gruntz/GameRegistry.h>       // CGameRegistry::m_world -> CDDrawSurfaceMgr::m_drawTarget
#include <Gruntz/SaveScreenshot.h>     // SaveScreenshot (0x114ff0) owner decl
#include <Utils/RegistryHelper.h>      // Utils::RegistryHelper (forwarded bute param)

// ChainForward14 (0x114f50) - the +0x14 (back-pair) sibling of ChainForward below.
// @early-stop
// same return-merge codegen-selection wall (~88%) as ChainForward below: the two
// null-guard `jne; ret` exits are byte-identical but cl /O2 merges them into one
// shared `je ret`. Logic 100% correct; deferred (same non-steerable heuristic).
RVA(0x00114f50, 0x3e)
void ChainForward14(
    Utils::RegistryHelper* p1,
    CGameRegistry* p2,
    i32 p3,
    i32 p4,
    char* p5,
    void* p6
) {
    CDDrawSurfacePair* pair = p2->m_world->m_drawTarget->m_backPair;
    if (pair) {
        CDDSurface* leaf = pair->m_surface;
        if (leaf) {
            SaveScreenshot(leaf, p1, p2, p3, p4, p5, p6);
        }
    }
}

// @early-stop
// return-merge codegen-selection wall (88%): the pointer-chain chase, the two null
// guards, and the 7-arg forward to 0x114ff0 are byte-identical; the only residual
// is that retail emits each null-guard exit as an inline `jne next; ret` (3 separate
// rets) while cl /O2 merges both guards into one `je shared_ret`. A cl tail-
// duplication heuristic (not flag- or source-steerable: nested-if and /Oy- both
// tested, no change). Logic 100% correct; deferred.
RVA(0x00114fa0, 0x3e)
void ChainForward(
    Utils::RegistryHelper* p1,
    CGameRegistry* p2,
    i32 p3,
    i32 p4,
    char* p5,
    void* p6
) {
    CDDrawSurfacePair* pair = p2->m_world->m_drawTarget->m_overlayPair;
    if (pair) {
        CDDSurface* leaf = pair->m_surface;
        if (leaf) {
            SaveScreenshot(leaf, p1, p2, p3, p4, p5, p6);
        }
    }
}
