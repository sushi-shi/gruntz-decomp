#include <StdAfx.h>

#include <Ints.h>

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

void SaveFrontBufferShot(Settings* reg, CGruntzMgr* mgr, i32 w, i32 h, char* name, i32 saveFlag) {
    SaveFrontBufferShotImpl(reg, mgr, w, h, name, saveFlag);
}

i32 SaveFrontBufferShotImpl(Settings* reg, CGruntzMgr* mgr, i32 w, i32 h, char* name, i32 saveFlag) {
    CDDrawFrontSurface* pair = mgr->m_world->GetDrawTarget()->GetFrontSurface();
    if (pair == NULL) {
        return 0;
    }
    if (pair->GetSurface() == NULL) {
        return 0;
    }

    return SaveScreenshot(pair->GetSurface(), reg, mgr, w, h, name, saveFlag);
}
