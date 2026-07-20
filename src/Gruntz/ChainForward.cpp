#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (m_backPair/m_overlayPair)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair::m_surface (CDDSurface*)
#include <Gruntz/GruntzMgr.h>          // CGruntzMgr::m_world -> CDDrawSurfaceMgr::m_drawTarget
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
    CGruntzMgr* p2,
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
    CGruntzMgr* p2,
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
