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
#include <Io/FileTransaction.h>
#include <Gruntz/SaveScreenshot.h>

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

    if (saveFlag) {
        io::File file;
        if (!file.open(name, io::Update) || !file.seek(0, io::End)) return 0;
        return SaveScreenshot(src, owner, width, height, file) && file.finish();
    }
    io::FileTransaction file(name);
    return file.good() && SaveScreenshot(src, owner, width, height, file) && file.commit();
}

i32 SaveScreenshot(CDDSurface* src, CGruntzMgr* owner, i32 width, i32 height, io::Output& target) {
    if (!src || !owner || !owner->m_world || !target.good()) return 0;
    RECT dstRect;
    RECT srcRect;
    CDDrawDeviceManager* manager = owner->m_world->GetDeviceManager();
    if (manager == NULL) {
        return 0;
    }
    CDDSurface* image = manager->CreateOffscreenSurface(width, height, BPP_RGB_16, 0, -1);
    if (image == NULL) {
        return 0;
    }

    CGruntzMgr* gameManager = owner;
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
    i32 result = image->SaveRle16(target);
    manager->RemoveSurface(image);
    return result;
}
