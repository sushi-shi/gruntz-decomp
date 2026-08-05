#include <rva.h>

#include <Gruntz/CreditsState.h>

#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/GruntzSoundZ.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/Fader.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelPreview.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundFxEmitter.h>
#include <Gruntz/SplashParams.h>
#include <Gruntz/String.h>
#include <Io/FileMem.h>
#include <Io/MoviePlayer.h>
#include <Rez/RezMgr.h>
#include <Rez/RezTypeTag.h>
#include <Wap32/EngStr.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

static inline CGruntzMgr* Owner(CState* s) {
    return s->m_mgr;
}

DATA(0x0022bf74)
i32 g_clipRegionEnabled;

VTBL(CCreditsState, 0x001e9c64);

DATA(0x001e96f8)
static const double kScreenH = 480.0;
DATA(0x001e96f0)
static const double kScrollRate = 0.025;
DATA(0x001e9708)
static const double kStepScale = 1000.0;

RVA(0x00038d20, 0x176)
i32 CCreditsState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;

    m_flashColor = 0;
    m_flashTimer = 0;
    m_fadeCountdown = 0;
    m_fxEnabled = 0;
    m_stateBank = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_CREDITZ"));
    if (!m_stateBank) {
        return 0;
    }

    void* sounds = SymTab2c()->FindSub("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(sounds), "CREDITZ", "_");

    CSymTab* midiz = static_cast<CSymTab*>(SymTab2c()->ResolvePath("MIDIZ"));
    if (midiz) {
        CParseSource* e = midiz->Insert("PLAY", REZ_TAG_XMI);
        if (e) {
            char* val = e->BeginParse();
            if (val) {
                m_mgr->m_sound->CreateBank(val, e->m_length, "CREDITZ");
            }
        }
    }

    if (midiz) {
        CParseSource* e2 = midiz->Insert("MONOLITH", REZ_TAG_XMI);
        if (e2) {
            char* val = e2->BeginParse();
            if (val) {
                m_mgr->m_sound->CreateBank(val, e2->m_length, "MONOLITH");
            }
        }
    }

    if (!m_world->m_drawTarget->HasOverlay()) {
        if (!m_world->m_drawTarget->CreateOverlay(0, 0x30000)) {
            return 0;
        }
    }

    SetupTitle();
    m_reserved20c = 2;
    i32 r = FinishState();
    m_musicStarted = 0;
    return r;
}

RVA(0x00038f00, 0x87)
void CCreditsState::ReleaseResources() {
    if (m_world) {
        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->Stop();
        }
        m_world->m_soundRegistry->RemoveKeysEqual("CREDITZ", "_");
        m_world->m_imageRegistry->RemoveKeysEqual("CREDITZ", "_");

        m_world->m_animRegistry->RemoveKeysEqual("CREDITZ", "_");
    }

    CMoviePlayer* vh = m_videoHandle;
    if (vh) {

        delete vh;
        m_videoHandle = NULL;
    }
    CState::ReleaseResources();
}

RVA(0x00039120, 0x2c)
i32 CCreditsState::EnterState(GameStateId) {
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    return InitAttractTitle() != 0;
}

RVA(0x00039160, 0x46)
i32 CCreditsState::LeaveState(GameStateId unused) {
    owner()->m_sound->IsPlaying();
    owner()->m_sound->StopAndFlush();
    m_stateBank = static_cast<CSymTab*>(stateMgr()->ResolvePath("STATEZ_ATTRACT"));
    RunTitleSeq("TITLE", 0, 0, 1, 0);
    return 1;
}

RVA(0x000391d0, 0x17c)
i32 CCreditsState::Render() {
    IDirectDrawSurface* in = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            Owner(this)->ReportError(IDX(IDS_RESTORE_GAME), 0xfa0);
            return 0;
        }
    }

    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->PurgeVoiceList(-1);
    }

    {
        CFixedPtrArray32* L = g_actorList;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_items[i]->Poll();
        }
    }

    {
        CFixedPtrArray32* L = g_actorList;
        i32 n = L->m_count;
        for (i32 j = 0; j < n; j++) {
            if (L->m_items[j]->m_currentKeys & 0xffffff) {

                u32 wp = IDX(CMD_ATTRACT);
                if (m_previousStateId == GAMESTATE_MENU) {
                    wp = IDX(CMD_MAIN_MENU);
                }
                PostMessageA(Owner(this)->m_gameWnd->m_hwnd, WM_COMMAND, wp, 0);
                Owner(this)->m_owner->m_running = 0;
                break;
            }
        }
    }

    StepVideo();
    DrawScrollingCredits();

    CDDrawSubMgrPages* v4 = m_world->m_drawTarget;
    v4->m_frontPair->m_surface->Flip(0);
    v4->m_backPair->BltSelf(v4->m_overlayPair);

    if (!m_musicStarted && Owner(this)->m_musicEnabled) {
        Owner(this)->m_sound->PlayByName("CREDITZ", 1);
        m_musicStarted = 1;
    }

    if (m_fxEnabled) {
        CGruntzSoundInnerZ* s = Owner(this)->m_sound->FindBank("MONOLITH");
        if (s && !s->IsStarted()) {
            LoadCreditzAssets();
        }
    }
    return 1;
}

RVA(0x000393b0, 0x3a)
i32 CCreditsState::InputVirtual() {

    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    InitAttractTitle();
    return 1;
}

RVA(0x00039400, 0x2f)
i32 CCreditsState::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    return InitAttractTitle();
}

RVA(0x00039440, 0x46)
i32 CCreditsState::OnKeyDown(i32 code, i32 unused) {
    if (code == VK_ESCAPE || code == VK_SPACE || code == VK_RETURN) {
        if (m_previousStateId == GAMESTATE_MENU) {
            PostMessageA(Owner(this)->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
        } else {
            PostMessageA(Owner(this)->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_ATTRACT), 0);
        }
    }
    return 1;
}

// @early-stop
RVA(0x000394b0, 0x86)
i32 CCreditsState::OnLButtonDown(i32 x, i32 unused, i32 y) {
    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = 0x64;
    rc.bottom = 0x64;
    POINT pt;
    pt.x = x;
    pt.y = y;
    if (PtInRect(&rc, pt)) {
        LoadCreditzAssets();
        return 1;
    }
    i32 cmd;
    if (m_previousStateId == GAMESTATE_MENU) {
        cmd = 0x8023;
    } else {
        cmd = 0x8027;
    }
    PostMessageA(Owner(this)->m_gameWnd->m_hwnd, WM_COMMAND, cmd, 0);
    return 1;
}

// @early-stop
RVA(0x00039570, 0x122)
i32 CCreditsState::InitAttractTitle() {
    CDDrawSurfaceMgr* root = m_world;
    if (m_videoPlaying != 0) {
        (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->PresentBackPage();
        (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->TransTitle();
        (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->ClearAllPages(0);
        root->m_drawTarget->m_overlayPair->m_surface->Fill(0);
        return 1;
    }
    char stateName[0x20];
    char titleName[0x20];
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);
    CSymTab* saved = m_stateBank;
    CSymTab* state = static_cast<CSymTab*>(m_symParser->ResolvePath(stateName));
    m_stateBank = state;
    if (state == NULL) {
        return 0;
    }
    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_stateBank = saved;
    if (faded == 0) {
        return 0;
    }
    CDDSurface* tgt = root->m_drawTarget->m_backPair->m_surface;
    tgt->ShadeRect(g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32), 0);
    (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->TransTitle();
    RetireScene(0x50, 0x3e8, 0, 1);
    return 1;
}

// @early-stop
RVA(0x000396f0, 0x2b8)
i32 CCreditsState::DrawScrollingCredits() {
    if (m_world == NULL) {
        return 0;
    }

    CDDSurface* prov = m_world->m_drawTarget->m_backPair->m_surface;

    if (g_frameDelta >= m_scrollReseedTimer) {
        m_scrollReseedTimer = 0;
    } else {
        m_scrollReseedTimer -= g_frameDelta;
    }
    if (m_fxEnabled != 0) {
        if (g_frameDelta >= m_flashTimer) {
            m_flashTimer = 0;
        } else {
            m_flashTimer -= g_frameDelta;
        }
        if (g_frameDelta >= m_fadeCountdown) {
            m_fadeCountdown = 0;
        } else {
            m_fadeCountdown -= g_frameDelta;
        }
    }

    m_scrollAccum = m_scrollAccum + static_cast<double>(g_frameDelta) * m_scrollStep * 0.001;
    m_drawRect = m_scrollRect;
    i32 scrolled = static_cast<i32>(m_scrollAccum);
    m_drawRect.top -= scrolled;
    m_drawRect.bottom -= scrolled;
    if (m_drawRect.bottom < 0) {
        m_scrollAccum = 0.0;
        m_drawRect = m_scrollRect;
        m_scrollReseedTimer = static_cast<i32>((kScreenH / kScrollRate));
    }

    HDC hdc = 0;
    prov->m_ddSurface->GetDC(&hdc);
    if (hdc != NULL) {
        i32 oldBk = SetBkMode(hdc, TRANSPARENT);
        if (g_clipRegionEnabled != 0) {
            SelectClipRgn(hdc, m_clipRegion);
        }
        i32 oldColor = SetTextColor(hdc, FlashColor());
        DrawTextA(hdc, m_caption, -1, &m_drawRect, 0x50);
        SetTextColor(hdc, oldColor);
        if (m_fxEnabled != 0 && m_fadeCountdown != 0) {
            CString s("Now is the time at Monolith when we dance");
            RECT r = {0, 0, SCREEN_W_PX, SCREEN_H_PX};
            i32 oldColor2 = SetTextColor(hdc, 0xffffff);
            DrawTextA(hdc, s, -1, &r, 0x75);
            SetTextColor(hdc, oldColor2);
        }
        if (g_clipRegionEnabled != 0) {
            SelectClipRgn(hdc, 0);
        }
        SetBkMode(hdc, oldBk);
        prov->m_ddSurface->ReleaseDC(hdc);
    }
    return 1;
}

// @early-stop
RVA(0x00039a60, 0x179)
i32 CCreditsState::SetupTitle() {

    CParseSource* sect = SymTab2c()->Insert("CREDITZ", REZ_TAG_TXT);
    if (sect) {
        char* src = sect->BeginParse();
        if (!src) {
            return 0;
        }
        i32 len = sect->m_length;
        char* buf = static_cast<char*>(operator new(len + 1));
        if (!buf) {
            return 0;
        }
        memcpy(buf, src, len);
        buf[len] = 0;
        m_caption = buf;
        sect->EndParse();
        operator delete(buf);
    }
    m_clipRegion.Attach(CreateRectRgn(0x32, 0, 0x24e, SCREEN_H_PX));
    CDDSurface* prov = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = 0;
    prov->m_ddSurface->GetDC(&hdc);
    if (hdc) {
        i32 h = DrawTextA(hdc, m_caption, -1, &m_drawRect, 0x450);
        SetRect(&m_scrollRect, 0x32, SCREEN_H_PX, 0x24e, h + SCREEN_H_PX);
        prov->m_ddSurface->ReleaseDC(hdc);
    }
    m_scrollAccum = 0.0;
    m_scrollReseedTimer = static_cast<i32>((kScreenH / kScrollRate));
    m_scrollStep =
        (kScreenH * kStepScale) / static_cast<double>(static_cast<unsigned>(m_scrollReseedTimer));
    return 1;
}

RVA(0x00039c40, 0x10)
i32 CCreditsState::FinishState() {
    m_videoPlaying = 0;
    return 1;
}

// @early-stop
RVA(0x00039c60, 0x7a)
i32 CCreditsState::StepVideo() {
    if (!m_videoPlaying) {
        return 1;
    }
    i32 ret = 0;
    if (m_videoHandle) {
        CDDrawSubMgrPages* v = m_world->m_drawTarget;
        CDDrawSurfacePair* dst = v->m_overlayPair;
        CDDrawSurfacePair* src = v->m_backPair;
        if (!m_videoHandle->Advance(dst->m_surface->m_ddSurface, -1)) {
            m_videoHandle->CloseSmacker();
            ret = FinishState();
        }
        if (dst && src) {
            src->m_surface->BltFast(0, 0, dst->m_surface, &dst->m_srcRect, 0x10);
        }
    }
    return ret;
}

// @early-stop
RVA(0x00039d00, 0x8c)
i32 CCreditsState::FlashColor() {
    i32 color = 0xffffff;
    if (m_fxEnabled) {
        if (m_flashTimer == 0) {
            i32 r = rand() % 256;
            i32 g = rand() % 256;
            i32 b = rand() % 256;
            m_flashTimer = 0x12c;
            color = (b << 16) | ((r & 0xff) << 8) | (g & 0xff);
            m_flashColor = color;
        } else {
            return m_flashColor;
        }
    }
    return color;
}

RVA_COMPGEN(0x0008c400, 0x46, ??1CRgn@@UAE@XZ)
RVA_COMPGEN(0x0008d5b0, 0x1e, ??_GCCreditsState@@UAEPAXI@Z)
RVA(0x0008d5e0, 0x8b)
CCreditsState::~CCreditsState() {
    ReleaseResources();
}
