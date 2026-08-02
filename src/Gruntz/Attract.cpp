#include <rva.h>

#include <Gruntz/Attract.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/Fader.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelPreview.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundFxEmitter.h>
#include <Gruntz/SplashParams.h>
#include <Gruntz/String.h>
#include <Image/ImageFormatTag.h>
#include <Io/FileMem.h>
#include <Wap32/EngStr.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

DATA(0x0024e360)
i32 g_suppress_64e360 = 0;

DATA(0x0024e35c)
i32 g_playActive;

RVA(0x00039160, 0x46)
i32 CCreditsState::LeaveState(i32 unused) {
    owner()->m_sound->IsPlaying();
    owner()->m_sound->StopAndFlush();
    m_stateBank = static_cast<CSymTab*>(stateMgr()->ResolvePath("STATEZ_ATTRACT"));
    RunTitleSeq("TITLE", 0, 0, 1, 0);
    return 1;
}

RVA(0x000a03f0, 0x14b)
i32 CMenuState::EnterState(i32 mode) {
    char stateName[0x20];
    char titleName[0x20];

    if (mode != 2) {
        i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
        sprintf(stateName, "STATEZ_ATTRACT");
        sprintf(titleName, "TITLE%d", idx);

        CSymTab* saved = attractState();
        CSymTab* state = static_cast<CSymTab*>(stateMgr()->ResolvePath(stateName));
        m_stateBank = (state);
        if (state == 0) {
            return 0;
        }

        i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
        if (faded == 0) {
            m_stateBank = (saved);
            return 0;
        }
        m_stateBank = (saved);

        CDDSurface* tgt = menuRoot()->m_drawTarget->m_backPair->m_surface;
        (static_cast<CDDSurface*>(tgt))
            ->ShadeRect(
                g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32),
                static_cast<tagRECT*>(0)
            );
        menuRoot()->m_drawTarget->TransTitle();
    } else {
        menuRoot()->m_drawTarget->TransEnter();
        CDDSurface* tgt = menuRoot()->m_drawTarget->m_overlayPair->m_surface;
        (static_cast<CDDSurface*>(tgt))
            ->ShadeRect(
                g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32),
                static_cast<tagRECT*>(0)
            );
        menuRoot()->m_drawTarget->TransExit();
    }

    RetireScene(0x50, 0x3e8, 0, 1);

    if (ShowCursor(1) < 0) {
        do {
        } while (ShowCursor(1) < 0);
    }
    StartMusic();
    return 1;
}

RVA(0x000a0a30, 0x110)
i32 CMenuState::RestoreDisplay() {
    char stateName[0x20];
    char titleName[0x20];

    i32 gate = IsActive();
    if (gate == 0) {
        return gate;
    }

    menuRoot()->m_drawTarget->m_backPair->m_surface->Fill(0);

    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);

    CSymTab* saved = attractState();
    CSymTab* state = static_cast<CSymTab*>(stateMgr()->ResolvePath(stateName));
    m_stateBank = (state);
    if (state == 0) {
        return 0;
    }

    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    if (faded == 0) {
        m_stateBank = (saved);
        return 0;
    }
    m_stateBank = (saved);

    CDDSurface* tgt = menuRoot()->m_drawTarget->m_backPair->m_surface;
    tgt->ShadeRect(
        g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32),
        static_cast<tagRECT*>(0)
    );
    menuRoot()->m_drawTarget->TransTitle();

    RetireScene(0x50, 0x3e8, 0, 1);

    if (ShowCursor(1) < 0) {
        do {
        } while (ShowCursor(1) < 0);
    }
    return 1;
}

// @early-stop
RVA(0x000fa1f0, 0xc6)
i32 CState::FadeInTitle(const char* name, i32 a, i32 b, i32 c, i32 d, i32 e) {
    static_cast<void>(a);
    static_cast<void>(b);
    static_cast<void>(c);
    static_cast<void>(d);
    if (!m_world) {
        return 0;
    }
    if (!m_symParser) {
        return 0;
    }
    if (!m_stateBank) {
        return 0;
    }

    char buf[0x40];
    sprintf(buf, "\\SCREENZ\\%s", name);
    CParseSource* page = SymTab2c()->ResolveQualified(buf, IMGTAG_XCP);
    if (page == 0) {
        return 0;
    }

    i32 mode = 1;
    if (e != 0) {
        mode = 2;
    }

    if (menuRoot()->m_drawTarget->LoadPageImage(page, mode) == 0) {
        if (e != 0) {
            if (menuRoot()->m_drawTarget->LoadPageImage(page, 1) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000fa300, 0x3a)
i32 CState::RunTitle(const char* a, i32 b, i32 c, i32 d, i32 e) {
    if (!m_world) {
        return 0;
    }
    if (!m_symParser) {
        return 0;
    }
    if (!m_stateBank) {
        return 0;
    }
    menuRoot()->m_drawTarget->m_frontPair->m_surface->Flip(0);
    return 1;
}

RVA(0x000fa350, 0x84)
i32 CState::RunTitleSeq(const char* name, i32 a, i32 b, i32 c, i32 d) {
    if (!m_world) {
        return 0;
    }
    if (!m_symParser) {
        return 0;
    }
    if (!m_stateBank) {
        return 0;
    }
    if (FadeInTitle(name, a, b, c, d, 0) == 0) {
        return 0;
    }
    return RunTitle(name, a, b, c, d) != 0;
}

RVA(0x000fa410, 0xf5)
i32 CSoundFxEmitter::FadeSceneClear1(i32 centerX, i32 centerY, i32 dur, i32 lead) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_ptrColl == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_drawTarget->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_clearMode = 1;
    t.m_centerX = centerX;
    t.m_centerY = centerY;
    t.m_targetSurface = chan;
    t.m_sourceSurface = 0;
    CFader* f = mgr->Add(1, &t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(dur);
        m_resChain->m_drawTarget->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(dur, lead, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// @early-stop
RVA(0x000fa550, 0x10c)
i32 CSoundFxEmitter::FadeScene1(i32 centerX, i32 centerY, i32 dur, i32 lead) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_ptrColl == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_drawTarget->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_drawTarget->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_centerX = centerX;
    t.m_clearMode = 0;
    t.m_centerY = centerY;
    t.m_targetSurface = chanA;
    t.m_sourceSurface = chanB;
    CFader* f = mgr->Add(1, &t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(dur);
        m_resChain->m_drawTarget->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(dur, lead, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

RVA(0x000fa6b0, 0xa7)
i32 CState::DrawStateText(i32 x, i32 y, char* str, i32 color, i32 bkMode) {
    if (str == 0) {
        return 0;
    }
    CDDSurface* s = m_world->m_drawTarget->m_frontPair->m_surface;
    if (s == 0) {
        return 0;
    }
    HDC hdc = 0;
    s->m_ddSurface->GetDC(&hdc);
    if (hdc == 0) {
        return 0;
    }
    SetBkMode(hdc, bkMode);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, str, strlen(str));
    s->m_ddSurface->ReleaseDC(hdc);
    return 1;
}

// @early-stop
RVA(0x000fa790, 0x104)
i32 CSoundFxEmitter::FadeScene2(i32 pct, i32 dur, i32 lead) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_ptrColl == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_drawTarget->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_drawTarget->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_clearToBlack = 0;
    t.m_intensityPercent = pct;
    t.m_targetSurface = chanA;
    t.m_sourceSurface = chanB;
    CFader* f = mgr->Add(2, &t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(dur);
        m_resChain->m_drawTarget->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(dur, lead, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// @early-stop
RVA(0x000fa8f0, 0x118)
i32 CState::RetireScene(i32 pct, i32 dur, i32 lead, i32 useOverlay) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_world->m_ptrColl == 0) {
        return 0;
    }
    CDDSurface* chanA = m_world->m_drawTarget->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDrawSurfacePair* holderB;
    if (useOverlay != 0 && m_world->m_drawTarget->HasOverlay() != 0) {
        holderB = m_world->m_drawTarget->m_overlayPair;
    } else {
        holderB = m_world->m_drawTarget->m_backPair;
    }
    CDDSurface* chanB = holderB->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_clearToBlack = 0;
    t.m_intensityPercent = pct;
    t.m_targetSurface = chanA;
    t.m_sourceSurface = chanB;
    CFader* f = mgr->Add(2, &t);
    if (f == 0) {
        return 0;
    }

    if (g_disableFades != 0) {
        ActiveWait(dur);
        m_world->m_drawTarget->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(dur, lead, 0);
    }
    mgr->Remove(f);
    return 1;
}

RVA(0x000faa60, 0xed)
i32 CSoundFxEmitter::FadeSceneClear2(i32 pct, i32 dur, i32 lead) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_ptrColl == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_drawTarget->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_clearToBlack = 1;
    t.m_intensityPercent = pct;
    t.m_targetSurface = chan;
    t.m_sourceSurface = 0;
    CFader* f = mgr->Add(2, &t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_disableFades != 0) {
        ActiveWait(dur);
        m_resChain->m_drawTarget->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(dur, lead, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

RVA(0x000fac70, 0x4c)
i32 CState::OnPaint() {
    if (!m_mgr) {
        return 0;
    }
    if (!m_mgr->m_gameWnd) {
        return 0;
    }
    PAINTSTRUCT ps;
    BeginPaint(m_mgr->m_gameWnd->m_hwnd, &ps);
    EndPaint(m_mgr->m_gameWnd->m_hwnd, &ps);
    return 1;
}

// @early-stop
RVA(0x000face0, 0x17c)
i32 CState::InputVirtual() {
    if (m_world == 0) {
        return 0;
    }

    while (ShowCursor(0) >= 0)
        ;
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    if (g_playActive == 0) {
        SplashParams sp;
        sp.text.LoadString(0x81a9);
        sp.rect.right = m_mgr->m_modeW;
        sp.rect.bottom = m_mgr->m_modeH;
        sp.rect.left = 0;
        sp.rect.top = 0;
        EngStr_DrawText(m_world, &sp.text, &sp.rect, 0x78, 1, 0xff, 0xff, 0, 1);
    }
    while (ShowCursor(0) >= 0)
        ;
    g_playActive = 0;
    char* path = static_cast<char*>(m_symParser->ResolvePath("GAME_IMAGEZ"));
    if (path == 0) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(path, "GAME", "_") == -1) {
        return 0;
    }
    m_inputWarmup1 = 0;
    m_inputWarmup2 = 1;
    m_inputHalfSel = 0;
    return 1;
}

RVA(0x000faec0, 0x67)
void CState::Present(i32 pct) {
    if (g_suppress_64e360 != 0) {
        g_suppress_64e360 = 0;
        return;
    }
    m_world->m_drawTarget->BlitPage(m_world->m_drawTarget->m_backPair);
    m_world->m_drawTarget->m_backPair->m_surface->ShadeRect(pct, static_cast<RECT*>(0));
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(static_cast<CDDSurface*>(0));
    m_world->m_drawTarget->BlitPage(m_world->m_drawTarget->m_backPair);
}

RVA(0x000faf50, 0x31)
i32 CState::ShadeScreen(i32 pct) {
    i32 v = g_suppress_64e360;
    if (v != 0) {
        g_suppress_64e360 = 0;
        return v;
    }
    return m_world->m_drawTarget->m_backPair->m_surface->ShadeRect(pct, 0);
}

RVA(0x000fafa0, 0x3b)
i32 CPlay::HeaderSerialize(CFileMemBase* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (HeaderWrite(ar) == 0) {
                return 0;
            }
            break;
        case 7:
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
    ar->Write(&m_levelIndex, 4);
    ar->Write(&m_levelType, 4);
    ar->Write(&m_previousStateId, 4);
    ar->Write(&m_reserved38, 4);
    ar->Write(&m_ready, 4);
    ar->Write(&m_notifyLatch, 4);
    ar->Write(&m_reserved44, 4);
    ar->Write(&m_reserved48, 4);
    ar->Write(m_versionString, 0x100);
    ar->Write(&m_reserved14c, 4);
    ar->Write(&m_cursorX, 4);
    ar->Write(&m_cursorY, 4);
    ar->Write(&m_snapOriginX, 4);
    ar->Write(&m_snapOriginY, 4);
    ar->Write(&m_cursorSaveSrc0, 0x10);
    ar->Write(&m_cursorSaveSrc1, 0x10);
    ar->Write(&m_cursorSaveDst0, 0x10);
    ar->Write(&m_cursorSaveDst1, 0x10);
    ar->Write(&m_inputWarmup1, 4);
    ar->Write(&m_inputWarmup2, 4);
    ar->Write(&m_inputHalfSel, 4);
    return 1;
}

RVA(0x000fb1c0, 0x168)
i32 CState::HeaderRead(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    ar->Read(&m_levelIndex, 4);
    ar->Read(&m_levelType, 4);
    ar->Read(&m_previousStateId, 4);
    ar->Read(&m_reserved38, 4);
    ar->Read(&m_ready, 4);
    ar->Read(&m_notifyLatch, 4);
    ar->Read(&m_reserved44, 4);
    ar->Read(&m_reserved48, 4);
    ar->Read(m_versionString, 0x100);
    ar->Read(&m_reserved14c, 4);
    ar->Read(&m_cursorX, 4);
    ar->Read(&m_cursorY, 4);
    ar->Read(&m_snapOriginX, 4);
    ar->Read(&m_snapOriginY, 4);
    ar->Read(&m_cursorSaveSrc0, 0x10);
    ar->Read(&m_cursorSaveSrc1, 0x10);
    ar->Read(&m_cursorSaveDst0, 0x10);
    ar->Read(&m_cursorSaveDst1, 0x10);
    ar->Read(&m_inputWarmup1, 4);
    ar->Read(&m_inputWarmup2, 4);
    ar->Read(&m_inputHalfSel, 4);
    return 1;
}
