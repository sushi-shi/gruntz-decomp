#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ChainForward.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Enums.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SaveScreenshot.h>
#include <Ints.h>
#include <Io/Settings.h>

#include <stddef.h>

i32 SaveBackBufferShot(
    Settings* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfacePair* pair = owner->m_world->GetDrawTarget()->GetBackPair();
    if (pair == NULL) {
        return 0;
    }
    CDDSurface* leaf = pair->GetSurface();
    if (leaf == NULL) {
        return 0;
    }
    return SaveScreenshot(leaf, reg, owner, width, height, name, saveFlag);
}

i32 SaveOverlayBufferShot(
    Settings* reg,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfacePair* pair = owner->m_world->GetDrawTarget()->m_overlayPair;
    if (pair == NULL) {
        return 0;
    }
    CDDSurface* leaf = pair->GetSurface();
    if (leaf == NULL) {
        return 0;
    }
    return SaveScreenshot(leaf, reg, owner, width, height, name, saveFlag);
}
