#include <rva.h>

#include <Gruntz/MenuState.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrLeafScanInline.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/Fader.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageState.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LeafCueInline.h>
#include <Gruntz/LevelPreview.h>
#include <Gruntz/MainMenuBuilder.h>
#include <Gruntz/MenuVersion.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundFxEmitter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StartUpPrompt.h>
#include <Gruntz/String.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/EngStr.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

DATA(0x00245574)
CFixedPtrArray32* g_actorList = NULL;
DATA(0x00251608)
i32 g_versionMajor = 0;
DATA(0x0025160c)
i32 g_versionMid = 0;
DATA(0x00251610)
i32 g_versionMinor = 0;

static inline LeafCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    LeafCue* found = NULL;
    MapLookup(cues, name, found);
    return found;
}

RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

RVA(0x0009fe50, 0x343)
i32 CMenuState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    if (prevStateId == 0) {
        return 0;
    }

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    m_mgr->RestoreVideoMode(0);
    m_stateBank = m_symParser->ResolvePath("STATEZ_MENU");
    if (m_stateBank == NULL) {
        return 0;
    }

    if (!m_world->m_imageRegistry->HasKeyEqual("MENU")) {
        CSymTab* set = SymTab2c()->ResolvePath("IMAGEZ");
        if (set == NULL) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_world->m_imageRegistry->InstallTree(set, "MENU", "_");
        g_resourceInstallActive = 0;
    }

    if (!m_world->m_soundRegistry->HasKeyEqual("MENU")) {
        CSymTab* set = SymTab2c()->ResolvePath("SOUNDZ");
        if (set == NULL) {
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
    m_menuTree = new CChatBox;
    if (!m_menuTree->InitRegion(m_world, m_mgr->m_gameWnd->m_hwnd, &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    CChatBox* ui = m_menuTree;
    if (ui->ConfigureLeftCursorAnimation("MENU_CURSOR", 0x64, 0x20)) {
        ui->ConfigureRightCursorAnimation("MENU_CURSOR", 0x64, 0x20);
    }
    ui = m_menuTree;
    ui->m_row0Key = "MENU_SELECT";
    ui->m_row1Key = "MENU_ACTIVATE";

    {
        LeafCue* e = LookupCue(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE");
        if (e != NULL) {
            e = LookupCue(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE");
            m_activateCueDurationMs = e->m_sound->m_durationMs;
        } else {
            m_activateCueDurationMs = 0;
        }
    }

    if (!BuildMainMenuTree(m_menuTree, prevStateId)) {
        return 0;
    }

    LeafCue* fm = LookupCue(
        (static_cast<CDDrawSubMgrLeafScan*>(g_gameReg->m_world->m_soundRegistry))->m_cues,
        "MENU_MENU"
    );
    m_menuMusicCue = fm;
    return 1;
}

RVA(0x000a0280, 0x2b)
void CChatBox::Init() {
    INIT_CHAT_BOX_MEMBERS;
}

RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {

    m_world->m_imageRegistry->RemoveKeysEqual("MENU", "_");
    m_world->m_soundRegistry->RemoveKeysEqual("MENU", "_");
    if (m_world) {

        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->StopAllStreams();
        }
        m_world->m_workerList->ClearWorkers();
    }

    CChatBox* ui = m_menuTree;
    if (ui) {
        delete ui;
        m_menuTree = NULL;
    }
    CState::ReleaseResources();
}

RVA(0x000a0360, 0x64)
CChatBox::~CChatBox() {
    Reset();
}

RVA(0x000a03f0, 0x14b)
i32 CMenuState::EnterState(GameStateId mode) {
    char stateName[0x20];
    char titleName[0x20];

    if (mode != GAMESTATE_ATTRACT) {
        i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
        sprintf(stateName, "STATEZ_ATTRACT");
        sprintf(titleName, "TITLE%d", idx);

        CSymTab* saved = attractState();
        CSymTab* state = stateMgr()->ResolvePath(stateName);
        m_stateBank = (state);
        if (state == NULL) {
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

RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_menuMusicCue == NULL) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    i32 saved = g_sndEnabled;
    if (!saved) {
        g_sndEnabled = 1;
    }
    i32 item = g_gameReg->m_soundVolume;
    PlayLeafCueIfElapsed(m_menuMusicCue, item, 0, 0, 1);
    if (!saved) {
        g_sndEnabled = saved;
    }
}

RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_menuMusicCue == NULL) {
        return;
    }
    LeafCue* mus = m_menuMusicCue;
    if (!mus->m_sound->IsPlaying()) {
        return;
    }
    m_menuMusicCue->m_sound->RampVolumeTo(0, 0x1f4, 1);
    if (!m_menuMusicCue->m_sound->IsPlaying()) {
        return;
    }
    do {
        PurgeVoices(m_world->m_soundRegistry);
    } while (m_menuMusicCue->m_sound->IsPlaying());
}

RVA(0x000a06d0, 0x5f)
i32 CMenuState::LeaveState(GameStateId) {
    m_world->m_drawTarget->TransExit();
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_activateCueDurationMs)
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
            m_menuTree->MoveFocusUp();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x40000000) {
            m_menuTree->MoveFocusDown();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x20000000) {
            m_menuTree->MoveFocusRight();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_currentKeys) & 0x10000000) {
            m_menuTree->MoveFocusLeft();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_items[c]->m_currentKeys & 0x3) {
            m_menuTree->ActivateFocusedItem();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_items[c]->m_currentKeys & 0x100) {
            if (!m_menuTree->ReturnToPreviousPage()) {
                PostMessageA(owner()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEXT_STATE), 0);
            }
            goto tail;
        }
    }
tail:

    m_menuTree->Step(g_frameDelta);
    m_menuTree->Pre();
    BuildVersionString(g_versionRect);
    m_menuTree->Post();
    return 1;
}

RVA(0x000a09a0, 0x6a)
i32 CMenuState::InputVirtual() {
    if (CState::InputVirtual() == 0) {
        return 0;
    }
    CSymTab* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (tree == NULL) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(tree, "MENU", "_") == -1) {
        return 0;
    }
    if (RestoreDisplay() == 0) {
        return 0;
    }
    int(WINAPI * sc)(BOOL) = ShowCursor;
    i32 r = sc(1);
    while (r < 0) {
        r = sc(1);
    }
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
    CSymTab* state = stateMgr()->ResolvePath(stateName);
    m_stateBank = (state);
    if (state == NULL) {
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

RVA(0x000a0b90, 0xc7)
i32 CMenuState::OnKeyDown(i32 key, i32 unused) {
    if (key == VK_DOWN) {
        m_menuTree->MoveFocusDownFollowingLinks();
    } else if (key == VK_UP) {
        m_menuTree->MoveFocusUpFollowingLinks();
    } else if (key == VK_RIGHT) {
        m_menuTree->MoveFocusRightFollowingLinks();
    } else if (key == VK_LEFT) {
        m_menuTree->MoveFocusLeftFollowingLinks();
    } else if (key == VK_RETURN || key == VK_SPACE) {
        m_menuTree->ActivateFocusedItem();
    } else if (key == VK_ESCAPE) {
        if (m_menuTree->ReturnToPreviousPage() == 0) {
            m_activateCueDurationMs = 0;
            PostMessageA(owner()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_ATTRACT), 0);
        }
    }
    return 1;
}

RVA(0x000a0ca0, 0x21)
i32 CMenuState::OnLButtonDown(i32 unused, i32 x, i32 y) {
    if (m_menuTree) {
        m_menuTree->ClickAt(x, y);
    }
    return 1;
}
RVA(0x000a0ce0, 0x21)
i32 CMenuState::OnLButtonDblClk(i32 unused, i32 x, i32 y) {
    if (m_menuTree) {
        m_menuTree->ClickAt(x, y);
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
void CMenuState::BuildVersionString(CRect r) {
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
