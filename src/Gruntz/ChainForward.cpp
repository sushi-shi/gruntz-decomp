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
    if (pair == NULL) {
        return 0;
    }
    CDDSurface* leaf = pair->m_surface;
    if (leaf == NULL) {
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
    if (pair == NULL) {
        return 0;
    }
    CDDSurface* leaf = pair->m_surface;
    if (leaf == NULL) {
        return 0;
    }
    return SaveScreenshot(leaf, bute, owner, width, height, name, saveFlag);
}
