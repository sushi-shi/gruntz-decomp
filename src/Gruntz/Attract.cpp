#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/Attract.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderConfig.h>
#include <Gruntz/FaderKind.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSettings.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelPreview.h>
#include <Gruntz/Play.h>
#include <Gruntz/PreviewState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/String.h>
#include <Io/FileMem.h>
#include <RectMacros.h>
#include <Rez/RezArchive.h>
#include <Rez/RezTypeTag.h>
#include <Wap32/EngStr.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

DATA(0x0024e360)
b32 g_skipNextScreenEffect = false;

DATA(0x0024e35c)
b32 g_skipNextRestoreMessage;

// @early-stop
RVA(0x000fa1f0, 0xc6)
i32 CState::LoadTitlePage(
    const char* titleName,
    i32 unused1,
    i32 unused2,
    i32 unused3,
    i32 unused4,
    b32 useOverlay
) {
    static_cast<void>(unused1);
    static_cast<void>(unused2);
    static_cast<void>(unused3);
    static_cast<void>(unused4);
    if (!m_world) {
        return 0;
    }
    if (!m_resourceArchive) {
        return 0;
    }
    if (!m_stateResources) {
        return 0;
    }

    char buf[0x40];
    sprintf(buf, "\\SCREENZ\\%s", titleName);
    CRezItm* page = StateResources()->GetRezFromPath(buf, IMGTAG_XCP);
    if (page == NULL) {
        return 0;
    }

    DDrawPageKind mode = DDRAW_PAGE_BACK;
    if (useOverlay != false) {
        mode = DDRAW_PAGE_OVERLAY;
    }

    if (menuRoot()->GetDisplayBuffers()->LoadPageImage(page, mode) == 0) {
        if (useOverlay != false) {
            if (menuRoot()->GetDisplayBuffers()->LoadPageImage(page, DDRAW_PAGE_BACK) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000fa300, 0x3a)
i32 CState::PresentTitlePage(
    const char* unusedTitleName,
    i32 unused1,
    i32 unused2,
    i32 unused3,
    i32 unused4
) {
    static_cast<void>(unusedTitleName);
    static_cast<void>(unused1);
    static_cast<void>(unused2);
    static_cast<void>(unused3);
    static_cast<void>(unused4);
    if (!m_world) {
        return 0;
    }
    if (!m_resourceArchive) {
        return 0;
    }
    if (!m_stateResources) {
        return 0;
    }
    menuRoot()->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Flip(NULL);
    return 1;
}

RVA(0x000fa350, 0x84)
i32 CState::LoadAndPresentTitlePage(
    const char* titleName,
    i32 unused1,
    i32 unused2,
    i32 unused3,
    i32 unused4
) {
    if (!m_world) {
        return 0;
    }
    if (!m_resourceArchive) {
        return 0;
    }
    if (!m_stateResources) {
        return 0;
    }
    if (LoadTitlePage(titleName, unused1, unused2, unused3, unused4, false) == 0) {
        return 0;
    }
    return PresentTitlePage(titleName, unused1, unused2, unused3, unused4) != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000fa410, 0xf5)
i32 CState::FadeLightToBlack(i32 centerX, i32 centerY, i32 durationMs, i32 leadMs) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == NULL) {
        return 0;
    }
    if (m_world->GetDeviceManager() == NULL) {
        return 0;
    }
    CDDSurface* surface = m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface();
    if (surface == NULL) {
        return 0;
    }

    CLightFaderConfig t;
    t.m_clearMode = true;
    // Component stores preserve argument-load scheduling without a CPoint temporary.
    SET_POINT_COMPONENTS(t.m_center, centerX, centerY);
    t.m_targetSurface = surface;
    t.m_sourceSurface = NULL;
    CFader* f = mgr->Add(FADERKIND_LIGHT, &t);
    if (f == NULL) {
        return 0;
    }

    m_mgr->PauseMusicIfEnabled();
    if (g_disableFades != false) {
        ActiveWait(durationMs);
        m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Fill(0);
    } else {
        f->RunFade(durationMs, leadMs, 0);
    }
    m_mgr->ResumeMusicIfEnabled();
    mgr->Remove(f);
    return 1;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000fa550, 0x10c)
i32 CState::FadeLightToBackBuffer(i32 centerX, i32 centerY, i32 durationMs, i32 leadMs) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == NULL) {
        return 0;
    }
    if (m_world->GetDeviceManager() == NULL) {
        return 0;
    }
    CDDSurface* targetSurface = m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface();
    if (targetSurface == NULL) {
        return 0;
    }
    CDDSurface* sourceSurface = m_world->GetDisplayBuffers()->GetBackBuffer()->GetSurface();
    if (sourceSurface == NULL) {
        return 0;
    }

    CLightFaderConfig t;
    t.m_clearMode = false;
    // Component stores preserve argument-load scheduling without a CPoint temporary.
    SET_POINT_COMPONENTS(t.m_center, centerX, centerY);
    t.m_targetSurface = targetSurface;
    t.m_sourceSurface = sourceSurface;
    CFader* f = mgr->Add(FADERKIND_LIGHT, &t);
    if (f == NULL) {
        return 0;
    }

    m_mgr->PauseMusicIfEnabled();
    if (g_disableFades != false) {
        ActiveWait(durationMs);
        m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Blt(sourceSurface);
    } else {
        f->RunFade(durationMs, leadMs, 0);
    }
    m_mgr->ResumeMusicIfEnabled();
    mgr->Remove(f);
    return 1;
}

RVA(0x000fa6b0, 0xa7)
i32 CState::DrawStateText(i32 x, i32 y, char* str, i32 color, i32 bkMode) {
    if (str == NULL) {
        return 0;
    }
    CDDSurface* s = m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface();
    if (s == NULL) {
        return 0;
    }
    HDC hdc = NULL;
    s->GetDirectDrawSurface()->GetDC(&hdc);
    if (hdc == NULL) {
        return 0;
    }
    SetBkMode(hdc, bkMode);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, str, strlen(str));
    s->GetDirectDrawSurface()->ReleaseDC(hdc);
    return 1;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000fa790, 0x104)
i32 CState::FadeSineToBackBuffer(i32 intensityPercent, i32 durationMs, i32 leadMs) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == NULL) {
        return 0;
    }
    if (m_world->GetDeviceManager() == NULL) {
        return 0;
    }
    CDDSurface* targetSurface = m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface();
    if (targetSurface == NULL) {
        return 0;
    }
    CDDSurface* sourceSurface = m_world->GetDisplayBuffers()->GetBackBuffer()->GetSurface();
    if (sourceSurface == NULL) {
        return 0;
    }

    CSineFaderConfig t;
    t.m_clearToBlack = false;
    t.m_intensityPercent = intensityPercent;
    t.m_targetSurface = targetSurface;
    t.m_sourceSurface = sourceSurface;
    CFader* f = mgr->Add(FADERKIND_SINE, &t);
    if (f == NULL) {
        return 0;
    }

    m_mgr->PauseMusicIfEnabled();
    if (g_disableFades != false) {
        ActiveWait(durationMs);
        m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Blt(sourceSurface);
    } else {
        f->RunFade(durationMs, leadMs, 0);
    }
    m_mgr->ResumeMusicIfEnabled();
    mgr->Remove(f);
    return 1;
}

// @early-stop
RVA(0x000fa8f0, 0x118)
i32 CState::RetireScene(i32 pct, i32 dur, i32 lead, b32 useOverlay) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == NULL) {
        return 0;
    }
    if (m_world->GetDeviceManager() == NULL) {
        return 0;
    }
    CDDSurface* targetSurface = m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface();
    if (targetSurface == NULL) {
        return 0;
    }
    CRenderBuffer* sourcePair;
    if (useOverlay != false && m_world->GetDisplayBuffers()->HasOverlay() != 0) {
        sourcePair = m_world->GetDisplayBuffers()->m_overlayBuffer;
    } else {
        sourcePair = m_world->GetDisplayBuffers()->GetBackBuffer();
    }
    CDDSurface* sourceSurface = sourcePair->GetSurface();
    if (sourceSurface == NULL) {
        return 0;
    }

    CSineFaderConfig t;
    t.m_clearToBlack = false;
    t.m_intensityPercent = pct;
    t.m_targetSurface = targetSurface;
    t.m_sourceSurface = sourceSurface;
    CFader* f = mgr->Add(FADERKIND_SINE, &t);
    if (f == NULL) {
        return 0;
    }

    if (g_disableFades != false) {
        ActiveWait(dur);
        m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Blt(sourceSurface);
    } else {
        f->RunFade(dur, lead, 0);
    }
    mgr->Remove(f);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000faa60, 0xed)
i32 CState::FadeSineToBlack(i32 intensityPercent, i32 durationMs, i32 leadMs) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == NULL) {
        return 0;
    }
    if (m_world->GetDeviceManager() == NULL) {
        return 0;
    }
    CDDSurface* surface = m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface();
    if (surface == NULL) {
        return 0;
    }

    CSineFaderConfig t;
    t.m_clearToBlack = true;
    t.m_intensityPercent = intensityPercent;
    t.m_targetSurface = surface;
    t.m_sourceSurface = NULL;
    CFader* f = mgr->Add(FADERKIND_SINE, &t);
    if (f == NULL) {
        return 0;
    }

    m_mgr->PauseMusicIfEnabled();
    if (g_disableFades != false) {
        ActiveWait(durationMs);
        m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Fill(0);
    } else {
        f->RunFade(durationMs, leadMs, 0);
    }
    m_mgr->ResumeMusicIfEnabled();
    mgr->Remove(f);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @early-stop
RVA(0x000fab90, 0xaa)
i32 CPreviewState::LoadScreen(char* name, i32 doFlip, i32 unused3, i32 unused4) {
    if (m_world == NULL) {
        return 0;
    }
    if (m_resourceArchive == NULL) {
        return 0;
    }
    if (m_stateResources == NULL) {
        return 0;
    }
    char buf[64];
    sprintf(buf, "\\SCREENZ\\%s", name);
    CRezItm* sym = StateResources()->GetRezFromPath(buf, IMGTAG_XCP);
    if (sym == NULL) {
        return 0;
    }
    if (menuRoot()->GetDisplayBuffers()->LoadPageImage(sym, DDRAW_PAGE_BACK) == 0) {
        return 0;
    }
    if (doFlip != 0) {
        menuRoot()->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Flip(NULL);
    }
    return 1;
}

RVA(0x000fac70, 0x4c)
i32 CState::OnPaint() {
    if (!m_mgr) {
        return 0;
    }
    if (!m_mgr->GetGameWindow()) {
        return 0;
    }
    PAINTSTRUCT ps;
    BeginPaint(m_mgr->GetGameWindow()->GetHwnd(), &ps);
    EndPaint(m_mgr->GetGameWindow()->GetHwnd(), &ps);
    return 1;
}

// @early-stop
RVA(0x000face0, 0x17c)
i32 CState::RestoreGraphics() {
    if (m_world == NULL) {
        return 0;
    }

    while (ShowCursor(false) >= 0)
        ;
    if (m_world->GetDisplayBuffers()->RestoreLostSurfaces() == 0) {
        return 0;
    }
    if (g_skipNextRestoreMessage == false) {
        CString text;
        RECT rect;
        text.LoadString(0x81a9);
        tagSIZE mode = m_mgr->GetModeSize();
        rect.right = mode.cx;
        rect.bottom = mode.cy;
        rect.left = 0;
        rect.top = 0;
        DrawTextToFrontSurface(m_world, &text, &rect, 0x78, 1, 0xff, 0xff, 0, 1);
    }
    while (ShowCursor(false) >= 0)
        ;
    g_skipNextRestoreMessage = false;
    CRezDir* path = m_resourceArchive->GetDirFromPath("GAME_IMAGEZ");
    if (path == NULL) {
        return 0;
    }
    if (m_world->GetImageRegistry()->ReloadImageSetsFromTree(path, "GAME", "_") == -1) {
        return 0;
    }
    m_cursorSavedSurfaceValid[0] = 0;
    m_cursorSavedSurfaceValid[1] = 1;
    m_cursorBufferIndex = 0;
    return 1;
}

RVA(0x000faec0, 0x67)
void CState::Present(i32 pct) {
    if (g_skipNextScreenEffect != false) {
        g_skipNextScreenEffect = false;
        return;
    }
    m_world->GetDisplayBuffers()->CopyFrontToSurface(m_world->GetDisplayBuffers()->GetBackBuffer());
    m_world->GetDisplayBuffers()->GetBackBuffer()->GetSurface()->ShadeRect(
        pct,
        static_cast<RECT*>(0)
    );
    m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->Flip(
        static_cast<CDDSurface*>(0)
    );
    m_world->GetDisplayBuffers()->CopyFrontToSurface(m_world->GetDisplayBuffers()->GetBackBuffer());
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000faf50, 0x31)
i32 CState::ShadeScreen(i32 pct) {
    b32 v = g_skipNextScreenEffect;
    if (v != false) {
        g_skipNextScreenEffect = false;
        return v;
    }
    return m_world->GetDisplayBuffers()->GetBackBuffer()->GetSurface()->ShadeRect(pct, NULL);
}

RVA(0x000fafa0, 0x3b)
i32 CPlay::SerializeHeader(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (HeaderWrite(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (HeaderRead(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x000faff0, 0x163)
i32 CState::HeaderWrite(CFileMemBase* ar) {
    if (!ar) {
        return 0;
    }
    if (!m_world) {
        return 0;
    }
    ar->Write(&m_levelIndex, sizeof(m_levelIndex));
    ar->Write(&m_levelType, sizeof(m_levelType));
    ar->Write(&m_previousStateId, sizeof(m_previousStateId));
    ar->Write(&m_reserved38, sizeof(m_reserved38));
    ar->Write(&m_ready, sizeof(m_ready));
    ar->Write(&m_returningToMenu, sizeof(m_returningToMenu));
    ar->Write(&m_reserved44, sizeof(m_reserved44));
    ar->Write(&m_reserved48, sizeof(m_reserved48));
    ar->Write(m_versionString, 0x100);
    ar->Write(&m_reserved14c, sizeof(m_reserved14c));
    ar->Write(&m_cursorX, sizeof(m_cursorX));
    ar->Write(&m_cursorY, sizeof(m_cursorY));
    ar->Write(&m_statusBarDragOffsetX, sizeof(m_statusBarDragOffsetX));
    ar->Write(&m_statusBarDragOffsetY, sizeof(m_statusBarDragOffsetY));
    ar->Write(&m_cursorSavedRects[0], sizeof(m_cursorSavedRects[0]));
    ar->Write(&m_cursorSavedRects[1], sizeof(m_cursorSavedRects[1]));
    ar->Write(&m_cursorScreenRects[0], sizeof(m_cursorScreenRects[0]));
    ar->Write(&m_cursorScreenRects[1], sizeof(m_cursorScreenRects[1]));
    ar->Write(&m_cursorSavedSurfaceValid[0], sizeof(m_cursorSavedSurfaceValid[0]));
    ar->Write(&m_cursorSavedSurfaceValid[1], sizeof(m_cursorSavedSurfaceValid[1]));
    ar->Write(&m_cursorBufferIndex, sizeof(m_cursorBufferIndex));
    return 1;
}

RVA(0x000fb1c0, 0x168)
i32 CState::HeaderRead(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    ar->Read(&m_levelIndex, sizeof(m_levelIndex));
    ar->Read(&m_levelType, sizeof(m_levelType));
    ar->Read(&m_previousStateId, sizeof(m_previousStateId));
    ar->Read(&m_reserved38, sizeof(m_reserved38));
    ar->Read(&m_ready, sizeof(m_ready));
    ar->Read(&m_returningToMenu, sizeof(m_returningToMenu));
    ar->Read(&m_reserved44, sizeof(m_reserved44));
    ar->Read(&m_reserved48, sizeof(m_reserved48));
    ar->Read(m_versionString, 0x100);
    ar->Read(&m_reserved14c, sizeof(m_reserved14c));
    ar->Read(&m_cursorX, sizeof(m_cursorX));
    ar->Read(&m_cursorY, sizeof(m_cursorY));
    ar->Read(&m_statusBarDragOffsetX, sizeof(m_statusBarDragOffsetX));
    ar->Read(&m_statusBarDragOffsetY, sizeof(m_statusBarDragOffsetY));
    ar->Read(&m_cursorSavedRects[0], sizeof(m_cursorSavedRects[0]));
    ar->Read(&m_cursorSavedRects[1], sizeof(m_cursorSavedRects[1]));
    ar->Read(&m_cursorScreenRects[0], sizeof(m_cursorScreenRects[0]));
    ar->Read(&m_cursorScreenRects[1], sizeof(m_cursorScreenRects[1]));
    ar->Read(&m_cursorSavedSurfaceValid[0], sizeof(m_cursorSavedSurfaceValid[0]));
    ar->Read(&m_cursorSavedSurfaceValid[1], sizeof(m_cursorSavedSurfaceValid[1]));
    ar->Read(&m_cursorBufferIndex, sizeof(m_cursorBufferIndex));
    return 1;
}
