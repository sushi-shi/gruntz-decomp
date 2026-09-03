#include <rva.h>

#include <Gruntz/MenuState.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/Fader.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageState.h>
#include <Gruntz/LevelPreview.h>
#include <Gruntz/MainMenuBuilder.h>
#include <Gruntz/MenuTree.h>
#include <Gruntz/MenuVersion.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <MakeRect.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StartUpPrompt.h>
#include <Gruntz/String.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
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

static inline SoundCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    SoundCue* foundCue = NULL;
    MapLookup(cues, name, foundCue);
    return foundCue;
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
    m_mgr->RestoreVideoMode(false);
    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_MENU");
    if (m_stateResources == NULL) {
        return 0;
    }

    if (!m_world->m_imageRegistry->HasWithPrefix("MENU")) {
        CRezDir* imageSymbols = StateResources()->GetDirFromPath("IMAGEZ");
        if (imageSymbols == NULL) {
            return 0;
        }
        g_resourceInstallActive = true;
        m_world->m_imageRegistry->InstallTree(imageSymbols, "MENU", "_");
        g_resourceInstallActive = false;
    }

    if (!m_world->m_soundRegistry->HasWithPrefix("MENU")) {
        CRezDir* soundSymbols = StateResources()->GetDirFromPath("SOUNDZ");
        if (soundSymbols == NULL) {
            return 0;
        }
        m_world->m_soundRegistry->LoadFromTree(static_cast<CRezDir*>(soundSymbols), "MENU", "_");
    }

    if (!m_world->m_drawTarget->HasOverlay()) {
        if (!m_world->m_drawTarget->CreateOverlay(0, 0x30000)) {
            return 0;
        }
    }

    CRect menuBounds = MakeRect(0, 8, 0x27f, 0x1df);
    m_menuTree = new CMenuTree;
    if (!m_menuTree->Configure(m_world, m_mgr->m_gameWnd->m_hwnd, &menuBounds, 0x14, 0xa, 1)) {
        return 0;
    }

    CMenuTree* menuTree = m_menuTree;
    if (menuTree->ConfigureLeftCursorAnimation("MENU_CURSOR", 0x64, 0x20)) {
        menuTree->ConfigureRightCursorAnimation("MENU_CURSOR", 0x64, 0x20);
    }
    menuTree = m_menuTree;
    menuTree->m_focusSoundKey = "MENU_SELECT";
    menuTree->m_activationSoundKey = "MENU_ACTIVATE";

    {
        SoundCue* activationCue = LookupCue(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE");
        if (activationCue != NULL) {
            activationCue = LookupCue(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE");
            m_activateCueDurationMs = activationCue->m_sound->m_durationMs;
        } else {
            m_activateCueDurationMs = 0;
        }
    }

    if (!BuildMainMenuTree(m_menuTree, prevStateId)) {
        return 0;
    }

    SoundCue* menuMusicCue = LookupCue(
        (static_cast<SoundCueRegistry*>(g_gameReg->m_world->m_soundRegistry))->m_cues,
        "MENU_MENU"
    );
    m_menuMusicCue = menuMusicCue;
    return 1;
}

RVA(0x000a0280, 0x2b)
void CMenuTree::InitializeMembers() {
    INITIALIZE_MENU_TREE_MEMBERS;
}

RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {

    m_world->m_imageRegistry->RemoveWithPrefix("MENU", "_");
    m_world->m_soundRegistry->RemoveWithPrefix("MENU", "_");
    if (m_world) {

        SoundCueRegistry* soundRegistry = m_world->m_soundRegistry;
        if (soundRegistry->m_soundStream) {
            soundRegistry->m_soundStream->StopAllStreams();
        }
        m_world->m_workerList->ClearWorkers();
    }

    CMenuTree* menuTree = m_menuTree;
    if (menuTree) {
        delete menuTree;
        m_menuTree = NULL;
    }
    CState::ReleaseResources();
}

RVA(0x000a0360, 0x64)
CMenuTree::~CMenuTree() {
    Reset();
}

RVA(0x000a03f0, 0x14b)
i32 CMenuState::EnterState(GameStateId previousState) {
    char stateName[0x20];
    char titleName[0x20];

    if (previousState != GAMESTATE_ATTRACT) {
        i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
        sprintf(stateName, "STATEZ_ATTRACT");
        sprintf(titleName, "TITLE%d", idx);

        CRezDir* saved = StateResources();
        CRezDir* state = ResourceArchive()->GetDirFromPath(stateName);
        m_stateResources = (state);
        if (state == NULL) {
            return 0;
        }

        i32 faded = LoadTitlePage(titleName, 0, 0, 1, 0, false);
        if (faded == 0) {
            m_stateResources = (saved);
            return 0;
        }
        m_stateResources = (saved);

        CDDSurface* tgt = menuRoot()->m_drawTarget->m_backPair->m_surface;
        (static_cast<CDDSurface*>(tgt))
            ->ShadeRect(
                g_buteMgr.GetInt("Menu", "BrightnessPercent", 0x32),
                static_cast<tagRECT*>(0)
            );
        menuRoot()->m_drawTarget->TransTitle();
    } else {
        menuRoot()->m_drawTarget->TransEnter();
        CDDSurface* tgt = menuRoot()->m_drawTarget->m_overlayPair->m_surface;
        (static_cast<CDDSurface*>(tgt))
            ->ShadeRect(
                g_buteMgr.GetInt("Menu", "BrightnessPercent", 0x32),
                static_cast<tagRECT*>(0)
            );
        menuRoot()->m_drawTarget->TransExit();
    }

    RetireScene(0x50, 0x3e8, 0, true);

    if (ShowCursor(true) < 0) {
        do {
        } while (ShowCursor(true) < 0);
    }
    StartMusic();
    return 1;
}

RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_menuMusicCue == NULL) {
        return;
    }
    if (g_gameReg->m_soundEnabled == false) {
        return;
    }
    b32 saved = g_soundEnabled;
    if (!saved) {
        g_soundEnabled = true;
    }
    i32 item = g_gameReg->m_soundVolume;
    PlaySoundCueIfElapsed(m_menuMusicCue, item, 0, 0, true);
    if (!saved) {
        g_soundEnabled = saved;
    }
}

RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_menuMusicCue == NULL) {
        return;
    }
    SoundCue* mus = m_menuMusicCue;
    if (!mus->m_sound->IsPlaying()) {
        return;
    }
    m_menuMusicCue->m_sound->RampVolumeTo(0, 0x1f4, true);
    if (!m_menuMusicCue->m_sound->IsPlaying()) {
        return;
    }
    do {
        TickSoundVolumeRamps(m_world->m_soundRegistry);
    } while (m_menuMusicCue->m_sound->IsPlaying());
}

RVA(0x000a06d0, 0x5f)
i32 CMenuState::LeaveState(GameStateId) {
    m_world->m_drawTarget->TransExit();
    m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
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
        if (static_cast<u32>(L->m_items[c]->m_pressedButtons) & IDX(INPUT_DOWN)) {
            m_menuTree->MoveFocusDown();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_pressedButtons) & IDX(INPUT_UP)) {
            m_menuTree->MoveFocusUp();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_pressedButtons) & IDX(INPUT_RIGHT)) {
            m_menuTree->MoveFocusRight();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_items[c]->m_pressedButtons) & IDX(INPUT_LEFT)) {
            m_menuTree->MoveFocusLeft();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_items[c]->m_pressedButtons & IDX(INPUT_BUTTON0 | INPUT_BUTTON1)) {
            m_menuTree->ActivateFocusedItem();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_items[c]->m_pressedButtons & IDX(INPUT_BUTTON8)) {
            if (!m_menuTree->ReturnToPreviousPage()) {
                PostMessageA(owner()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEXT_STATE), 0);
            }
            goto tail;
        }
    }
tail:

    m_menuTree->Update(g_frameDelta);
    m_menuTree->DrawActivePage();
    BuildVersionString(g_versionRect);
    m_menuTree->PresentFrame();
    return 1;
}

RVA(0x000a09a0, 0x6a)
i32 CMenuState::InputVirtual() {
    if (CState::InputVirtual() == 0) {
        return 0;
    }
    CRezDir* tree = StateResources()->GetDirFromPath("IMAGEZ");
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

    b32 gate = IsActive();
    if (gate == false) {
        return gate;
    }

    menuRoot()->m_drawTarget->m_backPair->m_surface->Fill(0);

    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);

    CRezDir* saved = StateResources();
    CRezDir* state = ResourceArchive()->GetDirFromPath(stateName);
    m_stateResources = (state);
    if (state == NULL) {
        return 0;
    }

    i32 faded = LoadTitlePage(titleName, 0, 0, 1, 0, false);
    if (faded == 0) {
        m_stateResources = (saved);
        return 0;
    }
    m_stateResources = (saved);

    CDDSurface* tgt = menuRoot()->m_drawTarget->m_backPair->m_surface;
    tgt->ShadeRect(g_buteMgr.GetInt("Menu", "BrightnessPercent", 0x32), static_cast<tagRECT*>(0));
    menuRoot()->m_drawTarget->TransTitle();

    RetireScene(0x50, 0x3e8, 0, true);

    if (ShowCursor(true) < 0) {
        do {
        } while (ShowCursor(true) < 0);
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
    DrawTextToOverlaySurface(m_world, &str, &r, 0x64, 1, 0xff, 0xff, 0, 0);
}
