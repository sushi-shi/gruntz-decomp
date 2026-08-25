#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Utils/RegistryHelper.h>

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
    RECT dstRect;
    RECT srcRect;

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
        i32 screenshotCount = bute->GetValueDword("Screen Dump Count", 0) + 1;
        bute->SetValueDword("Screen Dump Count", screenshotCount);
        wsprintfA(nameBuf, "Gruntz%04i.BMP", screenshotCount);
        name = nameBuf;
    }

    CDDrawDeviceManager* manager = owner->m_world->m_deviceManager;
    if (manager == NULL) {
        return 0;
    }
    CDDSurface* image = manager->CreateOffscreenSurface(width, height, BPP_RGB_16, 0, -1);
    if (image == NULL) {
        return 0;
    }

    CGruntzMgr* gameManager = g_gameReg;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = 0;
    srcRect.bottom = 0;
    dstRect.left = 0;
    dstRect.top = 0;
    dstRect.right = 0;
    dstRect.bottom = 0;
    srcRect.right = gameManager->GetModeSize().cx;
    srcRect.bottom = gameManager->GetModeSize().cy;
    dstRect.right = width;
    dstRect.bottom = height;
    if (image->BltEx(&dstRect, src, &srcRect, DDBLT_WAIT, NULL)) {
        manager->RemoveSurface(image);
        return 0;
    }
    i32 result = image->SaveFile(name, FMT_BMP, NULL, saveFlag);
    manager->RemoveSurface(image);
    return result;
}
