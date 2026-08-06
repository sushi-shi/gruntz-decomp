#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Utils/RegistryHelper.h>

// @early-stop
RVA(0x00114ff0, 0x1b3)
i32 SaveScreenshot(
    CDDSurface* src,
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
) {
    char nameBuf[0x80];
    i32 descB[6];
    i32 descA[4];

    if (src == NULL) {
        return 0;
    }
    if (bute == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (owner->m_world == NULL) {
        return 0;
    }
    if (name == NULL) {
        i32 cnt = bute->GetValueDword("Screen Dump Count", 0) + 1;
        bute->SetValueDword("Screen Dump Count", cnt);
        wsprintfA(nameBuf, "Gruntz%04i.BMP", cnt);
        name = nameBuf;
    }

    CDDrawPtrCollections* surf = owner->m_world->m_ptrColl;
    if (surf == NULL) {
        return 0;
    }
    CDDSurface* img = surf->MakeAndAddB(width, height, BPP_RGB_16, 0, -1);
    if (img == NULL) {
        return 0;
    }

    CGruntzMgr* mgr = g_gameReg;
    descA[0] = 0;
    descA[1] = 0;
    descA[2] = 0;
    descA[3] = 0;
    descB[0] = 0;
    descB[1] = 0;
    descB[2] = 0;
    descB[3] = 0;
    descA[2] = mgr->m_modeSize.cx;
    descB[5] = mgr->m_modeSize.cy;
    descA[3] = mgr->m_modeSize.cy;
    descB[4] = mgr->m_modeSize.cx;
    descB[2] = width;
    descB[3] = height;
    if (img->BltEx(descB, src, descA, 0x1000000, 0)) {
        surf->RemoveItemA(img);
        return 0;
    }
    i32 r = img->SaveFile(name, FMT_BMP, 0, saveFlag);
    surf->RemoveItemA(img);
    return r;
}
