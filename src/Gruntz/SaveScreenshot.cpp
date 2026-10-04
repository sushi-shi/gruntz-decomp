#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <RectMacros.h>
#include <Io/Settings.h>

i32 SaveScreenshot(
    CDDSurface* src,
    Settings* reg,
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
    if (reg == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (owner->m_world == NULL) {
        return 0;
    }
    if (name == NULL) {
        i32 screenshotCount = reg->getInt("Screen Dump Count", 0) + 1;
        reg->setInt("Screen Dump Count", screenshotCount);
        wsprintfA(nameBuf, "Gruntz%04i.BMP", screenshotCount);
        name = nameBuf;
    }

    CDDrawDeviceManager* manager = owner->m_world->GetDeviceManager();
    if (manager == NULL) {
        return 0;
    }
    CDDSurface* image = manager->CreateOffscreenSurface(width, height, BPP_RGB_16, 0, -1);
    if (image == NULL) {
        return 0;
    }

    CGruntzMgr* gameManager = g_gameReg;
    SET_RECT_COMPONENTS(srcRect, 0, 0, 0, 0);
    SET_RECT_COMPONENTS(dstRect, 0, 0, 0, 0);
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
