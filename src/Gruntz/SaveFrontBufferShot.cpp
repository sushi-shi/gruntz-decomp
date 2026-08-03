#include <rva.h>

#include <Gruntz/SaveFrontBufferShot.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SaveScreenshot.h>
#include <Ints.h>

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

// @early-stop
RVA(0x00114f00, 0x3e)
void SaveFrontBufferShotImpl(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    i32 saveFlag
) {
    CDDrawSurfaceChildA* pair = mgr->m_world->m_drawTarget->m_frontPair;
    if (pair == 0) {
        return;
    }
    if (pair->m_surface == 0) {
        return;
    }

    SaveScreenshot(pair->m_surface, bute, mgr, w, h, name, saveFlag);
}
