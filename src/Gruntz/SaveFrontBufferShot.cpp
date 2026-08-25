#include <rva.h>

#include <Gruntz/SaveFrontBufferShot.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SaveScreenshot.h>
#include <Ints.h>

#include <stddef.h>

RVA(0x00114ec0, 0x27)
void SaveFrontBufferShot(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    i32 saveFlag
) {
    SaveFrontBufferShotImpl(bute, mgr, w, h, name, saveFlag);
}

RVA(0x00114f00, 0x3e)
i32 SaveFrontBufferShotImpl(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    i32 saveFlag
) {
    CDDrawFrontSurface* pair = mgr->m_world->m_drawTarget->m_frontSurface;
    if (pair == NULL) {
        return 0;
    }
    if (pair->m_surface == NULL) {
        return 0;
    }

    return SaveScreenshot(pair->m_surface, bute, mgr, w, h, name, saveFlag);
}
