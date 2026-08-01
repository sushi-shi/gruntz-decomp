#include <Gruntz/MenuState.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/GameMode.h>
#include <Rez/FrameClock.h>
#include <Gruntz/MenuVersion.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GruntzMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/WwdGameReg.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>

#include <rva.h>
#include <Bute/SymParser.h>
#include <Image/CImage.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/MainMenuBuilder.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <Win32.h>
#include <Gruntz/SoundState.h>
#include <Utils/MapTyped.h>

DATA(0x00245574)
CFixedPtrArray32* g_actorList = 0;
DATA(0x00245cc8)
tagRECT g_versionRect;

DATA(0x00251608)
i32 g_versionMajor = 0;
DATA(0x0025160c)
i32 g_versionMid = 0;
DATA(0x00251610)
i32 g_versionMinor = 0;

static inline CGruntzMgr* Owner(CState* s) {
    return s->m_mgr;
}

RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

// @early-stop
RVA(0x0009fe50, 0x343)
i32 CMenuState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    if (prevStateId == 0) {
        return 0;
    }

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    m_mgr->RestoreVideoMode(0);
    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_MENU"));
    if (m_2c == 0) {
        return 0;
    }

    if (!m_world->m_imageRegistry->HasKeyEqual("MENU")) {
        void* set = SymTab2c()->ResolvePath("IMAGEZ");
        if (set == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_world->m_imageRegistry->InstallTree(set, "MENU", "_");
        g_resourceInstallActive = 0;
    }

    if (!m_world->m_soundRegistry->HasKeyEqual("MENU")) {
        void* set = SymTab2c()->ResolvePath("SOUNDZ");
        if (set == 0) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(set), "MENU", "_");
    }

    if (!m_world->m_drawTarget->HasOverlay()) {
        if (!m_world->m_drawTarget->CreateOverlay(0, 0x30000)) {
            return 0;
        }
    }

    RECT rc;
    rc.left = 0;
    rc.top = 8;
    rc.right = 0x27f;
    rc.bottom = 0x1df;
    m_1b4 = new CChatBox;
    m_1b4->Init();

    if (!m_1b4->InitRegion(m_world, m_mgr->m_gameWnd->m_hwnd, &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    if (m_1b4->ConfigureLeftCursorAnimation(const_cast<char*>("MENU_CURSOR"), 0x64, 0x20)) {
        m_1b4->ConfigureRightCursorAnimation(const_cast<char*>("MENU_CURSOR"), 0x64, 0x20);
    }
    m_1b4->m_row0Key = "MENU_SELECT";
    m_1b4->m_row1Key = "MENU_ACTIVATE";

    LeafCue* e;
    MapLookup(m_world->m_soundRegistry->m_10, "MENU_ACTIVATE", e);
    if (e != 0) {
        MapLookup(m_world->m_soundRegistry->m_10, "MENU_ACTIVATE", e);
        m_1b8 = e->m_10->m_durationMs;
    } else {
        m_1b8 = 0;
    }

    if (!BuildMainMenuTree(m_1b4, -1)) {
        return 0;
    }

    LeafCue* fm;
    MapLookup(
        (static_cast<CDDrawSubMgrLeafScan*>(g_gameReg->m_world->m_soundRegistry))->m_10,
        "MENU_MENU",
        fm
    );
    m_1bc = fm;
    return 1;
}

RVA(0x000a0280, 0x2b)
void CChatBox::Init() {
    m_page = 0;
    m_wnd = 0;
    m_activeNode = 0;
    m_row0Anim = 0;
    m_row1Anim = 0;
    m_row0Frame = 0;
    m_row1Frame = 0;
    m_row0Key.Empty();
    m_row1Key.Empty();
}

RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {

    m_world->m_imageRegistry->RemoveKeysEqual("MENU", "_");
    m_world->m_soundRegistry->RemoveKeysEqual("MENU", "_");
    if (m_world) {

        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->Stop();
        }
        m_world->m_workerList->ClearWorkers();
    }

    CChatBox* ui = m_1b4;
    if (ui) {
        delete ui;
        m_1b4 = 0;
    }
    CState::ReleaseResources();
}

RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_1bc == 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    i32 saved = g_sndEnabled;
    i32 flag = saved;
    if (!saved) {
        flag = 1;
        g_sndEnabled = 1;
    }
    i32 item = g_gameReg->m_soundVolume;
    LeafCue* mus = m_1bc;
    if (flag) {
        u32 clk = g_killCueClock;
        if (clk - mus->m_14 >= static_cast<u32>(mus->m_18)) {
            mus->m_14 = clk;
            mus->m_10->ConfigureItem(item, 0, 0, 1);
        }
    }
    if (!saved) {
        g_sndEnabled = saved;
    }
}

RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_1bc == 0) {
        return;
    }
    LeafCue* mus = m_1bc;
    if (!mus->m_10->IsPlaying()) {
        return;
    }
    m_1bc->m_10->CloneAndPlay(0, 0x1f4, 1);
    if (!m_1bc->m_10->IsPlaying()) {
        return;
    }
    do {
        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->PurgeVoiceList(-1);
        }
    } while (m_1bc->m_10->IsPlaying());
}

RVA(0x000a06d0, 0x5f)
i32 CMenuState::LeaveState(i32) {
    m_world->m_drawTarget->TransExit();
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_1b8)
        ;
    return 1;
}

RVA(0x000a0750, 0x1d0)
i32 CMenuState::Render() {
    CFixedPtrArray32* L = g_actorList;

    for (i32 i = 0; i < L->m_count; i++) {
        L->m_items[i]->Poll();
    }

    i32 c;
    L = g_actorList;
    i32 n = L->m_count;
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x80000000) {
            m_1b4->MoveFocusUp();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x40000000) {
            m_1b4->MoveFocusDown();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x20000000) {
            m_1b4->MoveFocusRight();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x10000000) {
            m_1b4->MoveFocusLeft();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_items[c]->m_currentKeys & 0x3) {
            m_1b4->ActivateFocusedItem();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_items[c]->m_currentKeys & 0x100) {
            if (!m_1b4->ReturnToPreviousPage()) {
                PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
            }
            goto tail;
        }
    }
tail:

    m_1b4->Step(g_frameDelta);
    m_1b4->Pre();
    BuildVersionString(g_versionRect);
    m_1b4->Post();
    return 1;
}

RVA(0x000a0b90, 0xc7)
i32 CMenuState::OnKeyDown(i32 key, i32 unused) {
    if (key == 0x28) {
        m_1b4->MoveFocusDownFollowingLinks();
    } else if (key == 0x26) {
        m_1b4->MoveFocusUpFollowingLinks();
    } else if (key == 0x27) {
        m_1b4->MoveFocusRightFollowingLinks();
    } else if (key == 0x25) {
        m_1b4->MoveFocusLeftFollowingLinks();
    } else if (key == 0xd || key == 0x20) {
        m_1b4->ActivateFocusedItem();
    } else if (key == 0x1b) {
        if (m_1b4->ReturnToPreviousPage() == 0) {
            m_1b8 = 0;
            PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
        }
    }
    return 1;
}

RVA(0x000a0ca0, 0x21)
i32 CMenuState::OnLButtonDown(i32 unused, i32 x, i32 y) {
    if (m_1b4) {
        m_1b4->ClickAt(x, y);
    }
    return 1;
}
RVA(0x000a0ce0, 0x21)
i32 CMenuState::OnLButtonDblClk(i32 unused, i32 x, i32 y) {
    if (m_1b4) {
        m_1b4->ClickAt(x, y);
    }
    return 1;
}

RVA(0x000a0d20, 0x8)
i32 CMenuState::OnMouseMove(i32, i32, i32) {
    return 1;
}

RVA(0x000a0d40, 0x24)
i32 CMenuState::OnPaint() {
    i32 r = IsActive();
    if (r == 0) {
        return r;
    }

    r = CState::OnPaint();
    if (r == 0) {
        return r;
    }
    return RestoreDisplay();
}

RVA(0x000a0d80, 0xd7)
void CMenuState::BuildVersionString(tagRECT r) {
    CString str;
    if (g_versionMid == 0) {
        str.Format("Gruntz v%d.%d", g_versionMajor, g_versionMinor);
    } else {
        str.Format("Gruntz v%d.%d%d", g_versionMajor, g_versionMid, g_versionMinor);
    }
    if (g_cdPromptResult) {
        str += " (SPAWN MODE)";
    }
    ShowHudMessage(m_world, &str, &r, 0x64, 1, 0xff, 0xff, 0, 0);
}
