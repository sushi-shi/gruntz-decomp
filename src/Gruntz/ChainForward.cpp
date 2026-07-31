#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (m_backPair/m_overlayPair)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair::m_surface (CDDSurface*)
#include <Gruntz/GruntzMgr.h>          // CGruntzMgr::m_world -> CDDrawSurfaceMgr::m_drawTarget
#include <Gruntz/ChainForward.h>       // this TU's own owner decls
#include <Gruntz/SaveScreenshot.h>     // SaveScreenshot (0x114ff0) owner decl
#include <Utils/RegistryHelper.h>      // Utils::RegistryHelper (forwarded bute param)

// ChainForward14 (0x114f50) - the +0x14 (back-pair) sibling of ChainForward below.
RVA(0x00114f50, 0x3e)
i32 ChainForward14(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfacePair* pair = owner->m_world->m_drawTarget->m_backPair;
    if (pair == 0) {
        return 0;
    }
    CDDSurface* leaf = pair->m_surface;
    if (leaf == 0) {
        return 0;
    }
    return SaveScreenshot(leaf, bute, owner, width, height, name, saveFlag);
}

RVA(0x00114fa0, 0x3e)
i32 ChainForward(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfacePair* pair = owner->m_world->m_drawTarget->m_overlayPair;
    if (pair == 0) {
        return 0;
    }
    CDDSurface* leaf = pair->m_surface;
    if (leaf == 0) {
        return 0;
    }
    return SaveScreenshot(leaf, bute, owner, width, height, name, saveFlag);
}
