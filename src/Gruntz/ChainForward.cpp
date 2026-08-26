#include <rva.h>

#include <Gruntz/ChainForward.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Enums.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SaveScreenshot.h>
#include <Ints.h>
#include <Utils/RegistryHelper.h>

#include <stddef.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00115080, 0x3e)
i32 SaveBackBufferShot(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfacePair* pair = owner->m_world->m_drawTarget->m_backPair;
    if (pair == NULL) {
        return 0;
    }
    CDDSurface* leaf = pair->m_surface;
    if (leaf == NULL) {
        return 0;
    }
    return SaveScreenshot(leaf, bute, owner, width, height, name, saveFlag);
}

RVA(0x001150d0, 0x3e)
i32 SaveOverlayBufferShot(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfacePair* pair = owner->m_world->m_drawTarget->m_overlayPair;
    if (pair == NULL) {
        return 0;
    }
    CDDSurface* leaf = pair->m_surface;
    if (leaf == NULL) {
        return 0;
    }
    return SaveScreenshot(leaf, bute, owner, width, height, name, saveFlag);
}
