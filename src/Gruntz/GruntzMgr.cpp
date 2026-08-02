#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/SerialCounter.h>
#include <DDrawMgr/PixelShift.h>
#include <Gruntz/TraitorMode.h>
#include <Io/FileMem.h>
#include <Gruntz/MapLogic.h>
#include <ddraw.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <new>
#include <Gruntz/LeafCue.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <Gruntz/BoundaryUpperViews.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Io/SaveGame.h>
#include <Gruntz/Play.h>
#include <Gruntz/StateMgrBZ.h>
#include <Rez/FrameClock.h>
#include <Gruntz/Fonts.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/Demo.h>
#include <Gruntz/Attract.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/SplashState.h>
#include <Gruntz/Multi.h>
#include <Gruntz/HelpState.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/BattlezMapConfig.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <Dsndmgr/SoundStream.h>
#include <Io/MoviePlayer.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzApp.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/FaderMgr.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/Enums.h>
#include <Io/FileStream.h>
#include <dplobby.h>
#include <rva.h>
#include <Utils/MapTyped.h>
#include <stdio.h>
#include <string.h>
#include <Utils/RegistryHelper.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Bute/SymParser.h>
#include <Image/ImageSet.h>
#include <Net/NetMgr.h>
#include <Gruntz/StatusBarMgr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Rez/RezSync.h>
#include <Wap32/GameApp.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Dialogs.h>
#include <Net/NetLobby.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LoadGameMenu.h>

// owner-TU unproven: bss sits in the pre-gruntzmgr window (before g_buteMgr)
DATA(0x002452d8)
char g_msgScratch[256];

DATA(0x002455e8)
i32 g_monologoShown;

VTBL(CGruntzMgr, 0x001e9b64);
VTBL(CSplashState, 0x001e9d74);
VTBL(CMenuState, 0x001e9e84);

DATA(0x0021ab20)
i32 g_sndEnabled = 1;
DATA(0x0021ab24)
i32 g_sndCueTag = 100;

DATA(0x0024556c)
CGruntzMgr* g_gameReg = 0;

DATA(0x002455a4)
u32 g_gruntDestruction;
DATA(0x002455a8)
u32 g_gruntCreation;
DATA(0x002455ac)
u32 g_gooPuddlez;
DATA(0x002455f8)
u32 g_explosionz;
DATA(0x00245600)
u32 g_resolutionChanged;
DATA(0x002455f4)
i32 g_debugDisplayFlags;

DATA(0x00245570)
DirectInputMgr2* g_inputMgr = 0;
DATA(0x00245578)
StateMgrBZ* g_spawnConfig = 0;

DATA(0x0020fa70)
i32 g_localVersion = 1;
DATA(0x0020fa74)
i32 g_remoteVersion = 1;
DATA(0x0020fab8)
NetGuid g_dplayAppGuid = {
    {0xf41cf640, 0x91b2, 0x11d1, {0x8d, 0xfc, 0x00, 0x60, 0x97, 0x9f, 0xa8, 0x1e}}
};
DATA(0x0020fac8)
i32 g_pendingFrame = 1;
DATA(0x00212610)
i32 g_warpX = -1;
DATA(0x00212614)
i32 g_warpY = -1;

RVA_COMPGEN(0x00083330, 0x1e, ??_GCGruntzMgr@@UAEPAXI@Z)

RVA_COMPGEN(0x00085540, 0xb, ??1CGameMgr@@UAE@XZ)

RVA_COMPGEN(0x000855a0, 0x24, ??_GCGameMgr@@UAEPAXI@Z)

// @early-stop
RVA(0x0008b8c0, 0x76)
i32 PumpIdleFrame() {
    if (g_pendingFrame == 0) {
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    g_pendingFrame = 0;
    if (mgr == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* world = mgr->m_world;
    if (world == 0) {
        return 0;
    }
    if (world->m_imageRegistry == 0) {
        return 0;
    }
    if (mgr->m_curState == 0) {
        return 0;
    }
    if (mgr->m_curState->InputVirtual() == 0) {
        g_gameReg->ReportError(0x8006, 0x435);
        return 0;
    }
    g_gameReg->RefreshGameClock();
    g_pendingFrame = 1;
    return 1;
}

CMulti::CMulti() {
    m_session = 0;
    m_netGate = 0;
    m_savedEffectsEnabled = 1;
    m_customLevel = 0;
    m_autoCommandDelay = 1;
}

RVA(0x0008b960, 0x808)
i32 CGruntzMgr::TransitionState(i32 stateId, i32 areaArg, i32 keepCurrent, i32 unused) {
    static_cast<void>(unused);
    CState* cur = m_curState;
    i32 local10 = 0;
    if (cur != 0) {
        local10 = cur->Update();
        i32 savedSub = cur->m_levelIndex;
        cur->LeaveState(stateId);
        if (keepCurrent != 0) {
            PushState(m_curState);
            areaArg = savedSub;
            m_curState = 0;
        } else {
            if (m_curState != 0) {
                delete m_curState;
            }
            m_curState = 0;
            ClearStateStack();
            m_curState = 0;
        }
    } else if (keepCurrent == 0) {
        ClearStateStack();
    }

    if (m_delayedQuitPending != 0) {

        m_curState = new CState;
        return 1;
    }

    CState* obj;
    switch (stateId) {
        case 2:
            obj = new CAttract;
            break;
        case 3:
            obj = new CPlay;
            break;
        case 5:
            obj = new CMenuState;
            break;
        case 7:
            obj = new CDemo;
            break;
        case 8:
            obj = new CCreditsState;
            break;
        case 9:
            obj = new CHelpState;
            break;
        case 10:
            obj = new CBootyState;
            break;
        case 14:
            obj = new CSplashState;
            break;
        case 17:
            obj = new CMulti;
            break;
        case 18:
            obj = new CMultiBootyState;
            break;
        default:
            goto install;
    }
    m_curState = obj;

install:
    if (m_curState == 0) {
        m_owner->m_running = 0;
        return 0;
    }
    RefreshGameClock();
    {
        CState* st = m_curState;

        i32 ok = st->LoadGameAssetNamespaces(this, areaArg, local10);
        st = m_curState;
        if (ok == 0) {
            if (st != 0) {
                delete st;
            }
            m_curState = 0;
            return 0;
        }
        st->EnterState(local10);
        m_owner->m_running = 1;
        g_inputMgr->ReadAll();
        RefreshGameClock();
        return 1;
    }
}

VTBL(CHelpState, 0x001e9dfc);
VTBL(CMulti, 0x001e9fe4);

VTBL(CPlay, 0x001ea0bc);

RVA_COMPGEN(0x0008c470, 0xb, ??1CState@@UAE@XZ)

RVA(0x0008c530, 0x8)
i32 CState::LeaveState(i32) {
    return 1;
}

RVA_COMPGEN(0x0008c710, 0x24, ??_GCState@@UAEPAXI@Z)
RVA_COMPGEN(0x0008c750, 0xa9, ??0CState@@QAE@XZ)
RVA_COMPGEN(0x0008c830, 0xaf, ??1CPlay@@UAE@XZ)
RVA_COMPGEN(0x0008c9a0, 0x1e, ??_GCPlay@@UAEPAXI@Z)
RVA_COMPGEN(0x0008c9d0, 0x2bd, ??0CPlay@@QAE@XZ)
RVA_COMPGEN(0x0008ce30, 0x1e, ??_GCMenuState@@UAEPAXI@Z)
RVA_COMPGEN(0x0008cf00, 0x1e, ??_GCHelpState@@UAEPAXI@Z)
RVA_COMPGEN(0x0008cfd0, 0x1e, ??_GCSplashState@@UAEPAXI@Z)
RVA_COMPGEN(0x0008d0a0, 0x1e, ??_GCDemo@@UAEPAXI@Z)
RVA(0x0008d0d0, 0xc4)
CDemo::~CDemo() {

    CPlay::ReleaseResources();
}

RVA(0x0008d1e0, 0x6)
GameStateId CMulti::Update() {
    return GAMESTATE_NONE;
}

RVA(0x0008d200, 0x3)
i32 CMulti::UnusedPlayQuery() {
    return 0;
}

RVA(0x0008d220, 0xa)
i32 CMulti::GetFrame() {
    return m_session->m_tick;
}

RVA_COMPGEN(0x0008d240, 0x1e, ??_GCMulti@@UAEPAXI@Z)
RVA(0x0008d850, 0x83)
i32 CGruntzMgr::GoToNextLevel() {
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    m_strWorldFile.Empty();
    CState* st = m_curState;
    i32 next = st->m_levelIndex + 1;
    if (next > 0x28) {
        next = 1;
    }
    if (next <= 0x20 || next >= 0x25) {
        st->LeaveState(st->Update());
        if ((static_cast<CPlay*>(st))->LoadByMode(next, 1)) {
            st->EnterState(st->Update());
            return 1;
        }
    }
    ReportError(0x8007, 0x436);
    return 0;
}

RVA(0x0008d910, 0x82)
i32 CGruntzMgr::GoToPrevLevel() {
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    m_strWorldFile.Empty();
    CState* st = m_curState;
    i32 prev = st->m_levelIndex - 1;
    if (prev <= 0) {
        prev = 0x28;
    }
    if (prev <= 0x20 || prev >= 0x25) {
        st->LeaveState(st->Update());
        if ((static_cast<CPlay*>(st))->LoadByMode(prev, 1)) {
            st->EnterState(st->Update());
            return 1;
        }
    }
    ReportError(0x8007, 0x437);
    return 0;
}

RVA(0x0008dc60, 0x19)
void CGruntzMgr::ReportError(WPARAM wParam, LPARAM lParam) {
    CGameApp* pApp = m_owner;
    if (pApp) {
        pApp->ReportError(wParam, lParam);
    }
}

RVA(0x0008dc20, 0x2b)
void CGruntzMgr::XorLiveObjectFlags(i32 mask) {
    CObList* list = &m_world->m_childGroup->m_list;
    if (list == 0) {
        return;
    }
    POSITION pos = list->GetHeadPosition();
    while (pos != 0) {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj) {
            obj->m_stateFlags ^= mask;
        }
    }
}

RVA(0x0008dc90, 0xb1)
void CGruntzMgr::RegisterLevelAssetKeys() {
    CDDrawSurfaceMgr* w = m_world;
    if (w == 0) {
        return;
    }

    CDDrawSubMgrLeafScan* snd = w->m_soundRegistry;
    w->m_imageRegistry->SumSizesEqual(0, 1);
    snd->SumField(0);
    w->m_ptrColl->GetCapsChecked();
    w->m_ptrColl->GetCapsChecked();
    w->m_imageRegistry->SumSizesEqual(0, 1);
    w->m_imageRegistry->SumSizesEqual("GRUNTZ", 1);
    w->m_imageRegistry->SumSizesEqual("GAME", 1);
    w->m_imageRegistry->SumSizesEqual("LEVEL", 1);
    w->m_imageRegistry->SumSizesEqual("ACTION", 1);
    w->m_soundRegistry->SumField(0);
    w->m_soundRegistry->SumField("GRUNTZ");
    w->m_soundRegistry->SumField("GAME");
    w->m_soundRegistry->SumField("LEVEL");
}

// @early-stop
RVA(0x0008ddd0, 0x7e)
i32 CGruntzMgr::RestoreVideoMode(i32 save) {
    i32 w = m_modeW;
    i32 h = m_modeH;
    if (w == 0x280 && h == 0x1e0) {
        if (save) {
            m_savedModeW = w;
            m_savedModeH = h;
        }
        return 1;
    }
    if (SetVideoMode(0x280, 0x1e0, save)) {
        return 1;
    }
    ReportError(0x8008, 0x438);
    return 0;
}

RVA(0x0008f980, 0x21)
i32 CGruntzMgr::IsStandardMode() {
    if (m_modeW == 0x280 && m_modeH == 0x1e0) {
        return 1;
    }
    return 0;
}

RVA(0x0008f9c0, 0x1d)
i32 CGruntzMgr::AppendChatMessage(char* msg) {
    CFontConfig* log = m_chatLog;
    if (log == 0) {
        return 0;
    }
    return log->AddItem(msg, 0, 0x11);
}

RVA(0x0008f9f0, 0x3e)
i32 CGruntzMgr::ShowToggleMessage(char* itemName, i32 on) {
    if (on) {
        sprintf(g_msgScratch, "%s is ON", itemName);
    } else {
        sprintf(g_msgScratch, "%s is OFF", itemName);
    }
    return AppendChatMessage(g_msgScratch);
}

RVA(0x0008fa40, 0x16)
i32 CGruntzMgr::IsInPlayState() {
    if (m_curState == 0) {
        return 0;
    }
    return CheckPlayState() != 0;
}

RVA(0x0008fa70, 0x2c)
char CGruntzMgr::GetGruntzDriveLetter() {
    if (m_driveLetterProbed) {
        return m_driveLetter;
    }
    m_driveLetter = ::GetGruntzDriveLetter();
    m_driveLetterProbed = 1;
    return m_driveLetter;
}

RVA(0x0008ec50, 0x33)
i32 CGruntzMgr::CheckPlayState() {
    if (m_curState == 0) {
        return 0;
    }
    if (m_curState->Update() == GAMESTATE_PLAY) {
        return 1;
    }
    return m_curState->Update() == GAMESTATE_NONE;
}

RVA(0x0008eca0, 0x164)
i32 CGruntzMgr::InitializeLobbyConnectionSettings() {
    if (m_lobbyProbed) {
        return m_lobbyResult;
    }

    m_lobbyProbed = 1;
    m_lobbyResult = 0;

    if (m_lobby) {
        m_lobby->Release();
        m_lobby = 0;
    }

    i32 hr = DirectPlayLobbyCreate(0, &m_lobby, 0, 0, 0);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x120d, hr, m_gameWnd->m_hwnd);
        return 0;
    }
    if (!m_lobby) {
        return 0;
    }

    if (m_connSettings) {

        ::operator delete(m_connSettings);
        m_connSettings = 0;
    }

    DWORD dwSize = 0;
    hr = m_lobby->GetConnectionSettings(0, 0, &dwSize);
    if (hr != 0 && hr != static_cast<i32>(DPERR_BUFFERTOOSMALL)) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr, m_gameWnd->m_hwnd);
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    m_connSettings = static_cast<CNetLobbyConnection*>(operator new(dwSize));
    if (!m_connSettings) {
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    hr = m_lobby->GetConnectionSettings(0, m_connSettings, &dwSize);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1232, hr, m_gameWnd->m_hwnd);
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    m_lobbyResult = 1;
    return m_lobbyResult;
}

RVA(0x0008ee70, 0x7c)
i32 CGruntzMgr::ShowMessageBox(const char* text, u32 type) {
    if (m_world) {
        m_world->m_drawTarget->BlitPage(m_world->m_drawTarget->m_backPair);

        CDDrawPtrCollections* pc = m_world->m_ptrColl;
        pc->m_device->FlipToGDISurface();
    }
    i32 wasShown = ShowCursor(1);
    while (ShowCursor(1) < 0) {
    }
    i32 result = MessageBoxA(m_gameWnd->m_hwnd, text, "Gruntz", type);
    if (wasShown <= 0) {
        while (ShowCursor(0) >= 0) {
        }
    }
    return result;
}

// @early-stop
RVA(0x0008efe0, 0x54)
i32 CGruntzMgr::ToggleObjectLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = m_world->m_level;
        if (view) {
            i32 count = view->m_planes.GetSize();

            i32 idx = (count == 4 ? count - 1 : count) - 1;
            CDDrawWorkerHost* layer =
                (idx < 0 || idx >= count) ? 0 : static_cast<CDDrawWorkerHost*>(view->m_planes[idx]);
            if (layer && !(layer->m_flags & 1)) {
                layer->m_flags ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x0008f060, 0x35)
i32 CGruntzMgr::ToggleHeightLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = m_world->m_level;
        if (view) {
            CDDrawWorkerHost* layer = view->m_mainPlane;
            if (layer) {
                layer->m_flags ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x0008f0b0, 0x46)
i32 CGruntzMgr::ToggleBaseLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = m_world->m_level;
        if (view) {
            CDDrawWorkerHost* layer = (view->m_planes.GetSize() > 0)
                                          ? static_cast<CDDrawWorkerHost*>(view->m_planes[0])
                                          : 0;
            if (layer && !(layer->m_flags & 1)) {
                layer->m_flags ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x0008f120, 0x170)
i32 __stdcall LaunchWebBrowser(char* url) {
    LONG len = 0x104;
    char cmd[0x104];
    if (RegQueryValueA(HKEY_CLASSES_ROOT, "http\\shell\\open\\command", cmd, &len)) {
        return 0;
    }
    if (strlen(cmd) < 3) {
        return 0;
    }
    HANDLE quoted = 0;

    _strupr(cmd);
    if (strstr(cmd, "IEXPLORE.EXE")) {
        FindProcessByName("IEXPLORE.EXE", 1, &quoted);
    }
    char* dash = strchr(cmd, '-');
    i32 dn = dash - cmd + 1;
    if (dash) {
        if (dn <= 2) {
            return 0;
        }
    }
    if (dash) {
        cmd[dn - 2] = 0;
    }
    char* slash = strchr(cmd, '/');
    i32 sn = slash - cmd + 1;
    if (slash) {
        if (sn <= 2) {
            return 0;
        }
    }
    if (slash) {
        cmd[sn - 2] = 0;
    }
    char cmdline[0x104];
    sprintf(cmdline, "%s %s", cmd, url);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    PROCESS_INFORMATION pi;
    si.cb = sizeof(si);
    return CreateProcessA(0, cmdline, 0, 0, FALSE, 0, 0, 0, &si, &pi);
}

RVA(0x0008f2f0, 0x1b)
i32 CGruntzMgr::PollUnlessIdle() {
    if (m_curState->Update() != GAMESTATE_MENU) {
        CheckPlayState();
    }
    return 0;
}

RVA(0x0008f340, 0xf6)
i32 CGruntzMgr::CaptureWorldFile() {
    i32 st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY && st != 7) {
        return 0;
    }
    CString name = RunCustomWorldDialog(m_gameWnd->m_hwnd, 0);
    if (name.GetLength() == 0) {
        return 0;
    }
    m_strWorldFile = name;
    m_isMultiLevel = 0;
    m_isBattlezLevel = 0;
    ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x8005, 0);
    return 1;
}

RVA(0x0008f620, 0x51)
void CGruntzMgr::RefreshGameClock() {
    if (m_curState && m_curState->Update() == GAMESTATE_NONE) {
        return;
    }

    InitializeTimeGlobal();

    if (m_world) {
        g_killCueClock = timeGetTime();
        g_engineFrameDelta = 0;
    }

    g_lastNow = g_wap32Now;
    g_frameDelta = g_wap32FrameDelta;
}

RVA(0x0008f6a0, 0x7d)
void CGruntzMgr::AdvanceFrame(i32 doDraw, i32) {
    if (IsActive() == 0) {
        return;
    }

    if (doDraw) {
        RefreshGameClock();
        if (m_frameGate != 0) {
            return;
        }
        if (m_musicEnabled == 0) {
            return;
        }
        if (CheckPlayState() == 0 && (m_curState == 0 || m_curState->Update() != 8)) {
            return;
        }
        m_sound->StopBank(1);
        return;
    }

    if (m_musicEnabled == 0) {
        return;
    }
    if ((m_sound->m_pCurrent ? m_sound->m_pCurrent->IsBusy() : 0) == 0) {
        return;
    }
    m_sound->StopAll();
}

// @early-stop
RVA(0x0008ff30, 0x20c)
CString CGruntzMgr::BuildMoviePath(i32 movie) {
    CString name;

    switch (movie) {
        case -1:
            name = "Logo.vob";
            break;
        case 0:
            name = "Gruntz0.vob";
            break;
        case 2:
            name = "Gruntz1.vob";
            break;
        case 4:
            name = "Gruntz2.vob";
            break;
        case 6:
            name = "Gruntz3.vob";
            break;
        case 8:
            name = "Gruntz4.vob";
            break;
        case 10:
            name = "Gruntz5.vob";
            break;
        case 12:
            name = "Gruntz6.vob";
            break;
        case 13:
            name = "Gruntz7.vob";
            break;
        case 14:
            name = "Gruntz8.vob";
            break;
    }

    if (name.GetLength() == 0) {
        return name;
    }

    CString path;
    char szDir[256];

    if (GetCurrentDirectoryA(0xff, szDir)) {
        path.Format("%s\\%s", szDir, static_cast<const char*>(name));
        if (!FileExists(const_cast<char*>(static_cast<const char*>(path)))) {
            path.Empty();
        }
    }

    if (path.GetLength() == 0) {
        path.Format("%c:\\Movies\\%s", GetGruntzDriveLetter(), static_cast<const char*>(name));
        if (path.GetLength() == 0) {
            return path;
        }
    }

    if (!FileExists(const_cast<char*>(static_cast<const char*>(path)))) {
        path.Empty();
    }

    return path;
}

RVA(0x0008d9d0, 0x1e)
i32 CGruntzMgr::ForwardCharToState(i32 a, i32 b) {
    if (m_curState) {
        return m_curState->OnChar(a, b);
    }
    return 0;
}
RVA(0x0008da00, 0x1e)
i32 CGruntzMgr::ForwardKeyDownToState(i32 a, i32 b) {
    if (m_curState) {
        return m_curState->OnKeyDown(a, b);
    }
    return 0;
}
RVA(0x0008da30, 0x1e)
i32 CGruntzMgr::ForwardKeyUpToState(i32 a, i32 b) {
    if (m_curState) {
        return m_curState->OnKeyUp(a, b);
    }
    return 0;
}
RVA(0x0008da60, 0x23)
i32 CGruntzMgr::ForwardLButtonDownToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnLButtonDown(a, b, c);
    }
    return 0;
}
RVA(0x0008daa0, 0x23)
i32 CGruntzMgr::ForwardLButtonUpToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnLButtonUp(a, b, c);
    }
    return 0;
}
RVA(0x0008dae0, 0x23)
i32 CGruntzMgr::ForwardLButtonDblClkToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnLButtonDblClk(a, b, c);
    }
    return 0;
}
RVA(0x0008db20, 0x23)
i32 CGruntzMgr::ForwardRButtonDownToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnRButtonDown(a, b, c);
    }
    return 0;
}
RVA(0x0008db60, 0x23)
i32 CGruntzMgr::ForwardRButtonUpToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnRButtonUp(a, b, c);
    }
    return 0;
}
RVA(0x0008dba0, 0x23)
i32 CGruntzMgr::ForwardRButtonDblClkToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnRButtonDblClk(a, b, c);
    }
    return 0;
}
RVA(0x0008dbe0, 0x23)
i32 CGruntzMgr::ForwardMouseMoveToState(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->OnMouseMove(a, b, c);
    }
    return 0;
}

RVA(0x0008de70, 0x61)
i32 CGruntzMgr::CheckSavedMode() {

    if ((m_modeW == m_savedModeW && m_modeH == m_savedModeH)
        || SetVideoMode(m_savedModeW, m_savedModeH, 1) || RestoreVideoMode(1)) {
        return 1;
    }
    ReportError(0x8008, 0x45e);
    return 0;
}

RVA(0x0008f480, 0x49)
i32 CGruntzMgr::ClearWorldFile() {
    GameStateId mode = m_curState->Update();
    if (mode == 5 || mode == 2 || mode == 3) {
        m_strWorldFile.Empty();
        PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x8005, 0);
        return 1;
    }
    return 0;
}

RVA(0x0008f4f0, 0x26)
void CGruntzMgr::ResetClockGlobals() {
    g_resolutionChanged = 0;
    g_traitorMode = 0;
    g_gruntDestruction = 0;
    g_gruntCreation = 0;
    g_gooPuddlez = 0;
    g_explosionz = 0;
    g_debugDisplayFlags = 0;
}

RVA(0x0008f7b0, 0x2b)
void CGruntzMgr::SetGameClock(i32 now, i32 delta, i32 abs) {
    g_lastNow = now;
    g_frameDelta = delta;
    g_frameTime = abs;
    g_killCueClock = now;
    g_engineFrameDelta = delta;
}

RVA(0x00090200, 0x8)
i32 CGruntzMgr::RunFromState() {
    return ChangeState(1);
}

RVA(0x00090980, 0x18)
CState* CGruntzMgr::TopState() {
    CPtrArray* st = &m_stateStack;
    if (st->GetSize() <= 0) {
        return 0;
    }
    return static_cast<CState*>(st->GetAt(st->GetSize() - 1));
}

RVA(0x000909b0, 0x1b)
void CGruntzMgr::PushState(CState* s) {
    if (!s) {
        return;
    }
    CPtrArray* st = &m_stateStack;
    st->SetAtGrow(st->GetSize(), s);
}

RVA(0x000909e0, 0x46)
i32 CGruntzMgr::PopTopIfMatches(CState* s) {
    if (!s) {
        return 0;
    }
    i32 n = m_stateStack.GetSize();
    if (n <= 0) {
        return 0;
    }
    CState* top = static_cast<CState*>(m_stateStack.GetAt(n - 1));
    m_stateStack.RemoveAt(n - 1, 1);
    return top == s;
}

RVA(0x00090a50, 0x40)
void CGruntzMgr::ClearStateStack() {
    for (i32 i = 0; i < m_stateStack.GetSize(); i++) {
        CState* s = static_cast<CState*>(m_stateStack.GetAt(i));
        if (s) {
            delete s;
        }
    }
    m_stateStack.SetSize(0, -1);
}

RVA(0x00090aa0, 0x10)
i32 CGruntzMgr::CheckMovieFileExists() {
    return FileExists(const_cast<char*>(static_cast<const char*>(m_strMoviePath)));
}

RVA(0x000901d0, 0x16)
i32 CGruntzMgr::IsMoviePathValid() {
    return FileExists(const_cast<char*>(static_cast<const char*>(m_strMoviePath))) != 0;
}

RVA(0x00090220, 0x2f)
void CGruntzMgr::Post(i32 code) {
    if (code > 0 && code <= 0x29) {
        i32 v = (code == 0x29) ? 1 : code;
        ::PostMessageA(m_gameWnd->m_hwnd, 0x111, GOTOLEVEL, v);
    }
}

// @early-stop
RVA(0x00090ac0, 0x1cc)
void CGruntzMgr::ReportWorldStatus(i32 a) {
    if (m_world == 0) {
        ReportError(0x800a, a);
    }
    u32 status = m_world->m_lastError;
    if (status == 0) {
        ReportError(0x800a, a);
    }
    switch (status) {
        case 0x3f0:
            ReportError(0x8015, 0x3f0);
            return;
        case 0x3f1:
            ReportError(0x8013, 0x3f1);
            return;
        case 0x3f2:
            ReportError(0x8012, 0x3f2);
            return;
        case 0x7d1:
            ReportError(0x8019, 0x7d1);
            return;
        case 0x7d2:
            ReportError(0x8018, 0x7d2);
            return;
        case 0x7d3:
            ReportError(0x8017, 0x7d3);
            return;
        case 0xbb9:
            ReportError(0x8014, 0xbb9);
            return;
        case 0xbba:
            ReportError(0x8016, status);
            return;
        case 0x80e9:
            ReportError(0x801e, 0x80e9);
            return;
        case 0x80ea:
            ReportError(0x801a, status);
            return;
        case 0x80eb:
            ReportError(0x801b, status);
            return;
        case 0x80ec:
            ReportError(0x801c, status);
            return;
        case 0x80ed:
            ReportError(0x801d, status);
            return;
        default:
            ReportError(0x8011, status);
            return;
    }
}

RVA(0x00090d10, 0x18e)
i32 CGruntzMgr::LoadMonologoSprite() {
    if (m_curState == 0) {
        return 0;
    }
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }

    CDDrawWorker* rec;
    {
        CObject* out = 0;
        m_world->m_imageRegistry->m_10map.Lookup("GAME_MONOLITH", out);
        rec = static_cast<CDDrawWorker*>(out);
    }
    if (rec == 0) {
        return 0;
    }
    i32 savedIdx = rec->m_minIndex;
    CImage* e = static_cast<CImage*>(rec->m_items.GetAt(savedIdx));
    if (e == 0) {
        return 0;
    }
    i32 geoA = e->m_width;
    i32 geoB = e->m_height;
    CDDrawWorkerHost* found =
        static_cast<CDDrawWorkerHost*>(m_world->m_level->FindPlaneByName("MONOLITH"));
    if (found == 0) {
        CDDrawWorkerHost* spr = m_world->m_level->ReadObjectPlane(
            0x20,
            0x20,
            geoA,
            geoB,
            -0x19,
            -0x19,
            const_cast<char*>("MONOLITH")
        );
        if (spr == 0) {
            return 0;
        }
        spr->m_frameSets.SetAtGrow(0, static_cast<CObject*>(rec));
        spr->m_flags |= 0xc;
        spr->m_zBound = 0xf4241;
        i32 parity = 1;
        for (i32 i = 0; i < spr->m_gridH; i++) {
            for (i32 j = 0; j < spr->m_gridW; j++) {
                i32 val = parity ? savedIdx : -1;
                parity ^= 1;
                spr->m_tileGrid[spr->m_colOffsets[i] + j] = val;
            }
            parity ^= 1;
        }
        g_monologoShown = 1;
        return 1;
    }
    if (found->m_flags & 2) {
        found->m_flags &= ~2;
        g_monologoShown = 1;
    } else {
        found->m_flags |= 2;
        g_monologoShown = 0;
    }
    return 1;
}

RVA(0x000910d0, 0x75)
i32 CGruntzMgr::SetGruntColor(CDDrawWorker* sink, const char* key, i32 idx) {
    if (sink && key) {
        CObject* out = 0;
        m_world->m_imageRegistry->m_10map.Lookup(key, out);
        CDDrawWorker* row = static_cast<CDDrawWorker*>(out);
        if (row) {
            CImage* dst = static_cast<CImage*>(row->m_items.GetAt(row->m_minIndex));
            if (dst) {
                CImage* src = sink->GetAt(idx);
                if (src != 0) {
                    dst->CopyFrom(src);
                    return 1;
                }
            }
        }
    }
    return 0;
}

RVA(0x0008eaf0, 0x10b)
i32 CGruntzMgr::WarpCheat() {
    char key[64];
    sprintf(key, "Level %i Warp X", g_gameReg->m_curState->m_levelIndex);
    i32 wx = m_settings->GetValueDword(key, -1);
    sprintf(key, "Level %i Warp Y", g_gameReg->m_curState->m_levelIndex);
    i32 wy = m_settings->GetValueDword(key, -1);
    if (wx != -1 && wy != -1) {
        if (m_curState->Update() != GAMESTATE_PLAY) {
            i32 last = m_settings->GetValueDword("Last Warp Level", -1);
            if (last != -1) {
                if (!PassClickToPlayState(last, 0, 1)) {
                    ReportError(0x8005, 0x43b);
                    return 0;
                }
                ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x80ca, 0);
                return 1;
            }
        } else {
            m_settings->SetValueDword("Last Warp Level", m_curState->m_levelIndex);
            return 1;
        }
    }
    return 0;
}

RVA(0x00090f10, 0x151)
i32 CGruntzMgr::CheatRevealTreasures() {
    if (m_curState == 0) {
        return 0;
    }
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    CObject* found = 0;
    m_world->m_imageRegistry->m_10map.Lookup("GAME_DEVHEADS", found);
    CDDrawWorker* out = static_cast<CDDrawWorker*>(found);
    if (out == 0) {
        return 0;
    }
    SetGruntColor(out, "GAME_TREASURE_GECKOS_RED", 0);
    SetGruntColor(out, "GAME_TREASURE_GECKOS_GREEN", 0);
    SetGruntColor(out, "GAME_TREASURE_GECKOS_BLUE", 0);
    SetGruntColor(out, "GAME_TREASURE_GECKOS_PURPLE", 0);
    SetGruntColor(out, "GAME_TREASURE_SCEPTERS_RED", 0);
    SetGruntColor(out, "GAME_TREASURE_SCEPTERS_GREEN", 0);
    SetGruntColor(out, "GAME_TREASURE_SCEPTERS_BLUE", 0);
    SetGruntColor(out, "GAME_TREASURE_SCEPTERS_PURPLE", 0);
    SetGruntColor(out, "GAME_TREASURE_CROSSES_RED", 1);
    SetGruntColor(out, "GAME_TREASURE_CROSSES_GREEN", 1);
    SetGruntColor(out, "GAME_TREASURE_CROSSES_BLUE", 1);
    SetGruntColor(out, "GAME_TREASURE_CROSSES_PURPLE", 1);
    SetGruntColor(out, "GAME_TREASURE_CHALICES_RED", 2);
    SetGruntColor(out, "GAME_TREASURE_CHALICES_GREEN", 2);
    SetGruntColor(out, "GAME_TREASURE_CHALICES_BLUE", 2);
    SetGruntColor(out, "GAME_TREASURE_CHALICES_PURPLE", 2);
    return 1;
}

// @early-stop
RVA(0x00091250, 0x100)
void CGruntzMgr::CheatSkeletonToggle() {
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY && m_world) {

        CDDrawWorker* set;
        {
            CObject* found = 0;
            m_world->m_imageRegistry->m_10map.Lookup("Gruntz", found);
            set = static_cast<CDDrawWorker*>(found);
        }
        if (set) {
            CImage* fr = static_cast<CImage*>(set->m_items.GetAt(set->m_minIndex));
            if (fr) {
                CDDrawShadeBlit* fmt = fr->m_owned;
                if (fmt) {
                    switch (fmt->m_drawType) {
                        case 2:
                            set->SetAllTypes(1);
                            AppendChatMessage(const_cast<char*>("Back from the dead?"));
                            break;
                        default:
                            set->SetAllTypes(2);
                            AppendChatMessage(const_cast<char*>("You're scaring me..."));
                            break;
                    }
                    CDDrawSubMgrLeafScan* host = m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {

                        void* cue_ob = 0;
                        host->m_cues.Lookup("GAME_MINORCHEAT", cue_ob);
                        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
                        if (cue) {
                            i32 tag = g_sndCueTag;
                            if (g_sndEnabled) {
                                if (static_cast<u32>((g_killCueClock - cue->m_lastPlayTime))
                                    >= static_cast<u32>(cue->m_replayDelay)) {
                                    cue->m_lastPlayTime = g_killCueClock;
                                    cue->m_sound->ConfigureItem(tag, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Deliberately leave the return register unchanged.
}

RVA(0x00091390, 0x11d)
void CGruntzMgr::CheatEclipseToggle() {
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY && m_world) {

        CDDrawWorker* set;
        {
            CObject* found = 0;
            m_world->m_imageRegistry->m_10map.Lookup("Gruntz", found);
            set = static_cast<CDDrawWorker*>(found);
        }
        if (set) {
            CImage* fr = static_cast<CImage*>(set->m_items.GetAt(set->m_minIndex));
            if (fr) {
                CDDrawShadeBlit* fmt = fr->m_owned;
                if (fmt) {
                    i32 st = fmt->m_drawType;
                    if (st != 3) {
                        set->SetAllTypes(3);
                        set->SetAllLightLevels(rand() % 256);
                        AppendChatMessage(const_cast<char*>("Me and my..."));
                    } else {
                        set->SetAllTypes(1);
                        AppendChatMessage(const_cast<char*>("Where did the sun go?"));
                    }
                    CDDrawSubMgrLeafScan* host = m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {

                        void* cue_ob = 0;
                        host->m_cues.Lookup("GAME_MINORCHEAT", cue_ob);
                        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
                        if (cue) {
                            i32 tag = g_sndCueTag;
                            if (g_sndEnabled) {
                                if (static_cast<u32>((g_killCueClock - cue->m_lastPlayTime))
                                    >= static_cast<u32>(cue->m_replayDelay)) {
                                    cue->m_lastPlayTime = g_killCueClock;
                                    cue->m_sound->ConfigureItem(tag, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Deliberately leave the return register unchanged.
}

RVA(0x00092180, 0x98)
i32 CGruntzMgr::ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, ScanCb cb, i32 user) {
    if (cb == 0) {
        return 0;
    }
    i32 r2 = radius * radius;
    i32 count = 0;
    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != 0) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj->m_objectType & mask) {
            i32 adx = abs(obj->m_screenX - x);
            i32 ady = abs(obj->m_screenY - y);
            if (adx * adx + ady + ady < r2) {
                count++;
                if (cb(obj, user) == 0) {
                    return count;
                }
            }
        }
    }
    return count;
}

// @early-stop
RVA(0x00092250, 0xba)
i32 CGruntzMgr::ScanObjectsInRect(i32 offX, i32 offY, RECT* rect, i32 mask, ScanCb cb, i32 user) {
    if (cb == 0) {
        return 0;
    }
    RECT* r = rect;
    if (r == 0) {
        return 0;
    }
    i32 loX = r->left + offX;
    i32 hiX = r->right + offX;
    i32 loY = r->top + offY;
    i32 hiY = r->bottom + offY;
    i32 count = 0;
    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != 0) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj->m_objectType & mask) {
            i32 ox = obj->m_screenX;
            if (ox >= loX && ox <= hiX) {
                i32 oy = obj->m_screenY;
                if (oy >= loY && oy <= hiY) {
                    count++;
                    if (cb(obj, user) == 0) {
                        return count;
                    }
                }
            }
        }
    }
    return count;
}

// @early-stop
RVA(0x00091170, 0xad)
i32 CGruntzMgr::SetColorDepth(i32 depth) {
    if (depth != 8 && depth != 0x10 && depth != 0x18) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    if (depth == 8) {
        g_surfaceColorKey = 0;
        return 1;
    }
    if (depth == 0x10) {

        i32 packed = static_cast<u16>(((0xff >> g_rDown) << g_rUp));
        packed |= static_cast<u16>(((0 >> g_gDown) << g_gUp));
        packed |= static_cast<u16>((0x84 >> g_bDown));
        g_surfaceColorKey = packed;
        return 1;
    }
    if (depth == 0x18) {
        g_surfaceColorKey = 0xff0084;
        return 1;
    }
    return 1;
}

// @early-stop
RVA(0x00091a40, 0x2f9)
i32 CGruntzMgr::LoadWorldMode(i32 mode) {
    if (m_world == 0) {
        return 0;
    }
    if (m_colorDepth == mode) {
        return 1;
    }
    if (mode != 8 && mode != 0x10) {
        return 0;
    }

    CWorldSoundSet* in = m_inputState;
    if (in) {
        in->Deactivate();
        in->m_list.CPtrList::~CPtrList();
        ::operator delete(in);
    }
    m_inputState = 0;

    CSymParser* surf = m_symParser;
    if (surf) {
        delete surf;
    }
    m_symParser = 0;

    m_colorDepth = mode;
    g_enableTrueColor = 0;
    g_enableHiColor = 0;
    if (m_colorDepth == 0x10) {
        g_enableHiColor = 1;
    }

    m_world->Cleanup();
    i32 kind = (g_disableAudio == 0) ? 1 : 5;
    if (m_world->Init(m_gameWnd->m_hwnd, 0x280, 0x1e0, m_colorDepth, kind) == 0) {
        ReportWorldStatus(0x43f);
        return 0;
    }

    m_world->SetRestoreHandler(&PumpIdleFrame);
    CGameLevel* view = m_world->m_level;
    view->m_maxStepX = 0xe;
    view->m_maxStepY = 0xe;
    RegisterGameObjectTypes(m_world);
    if (MakeRezPath() == 0) {
        return 0;
    }

    CSymParser* old = m_symParser;
    if (old) {
        delete old;
        m_symParser = 0;
    }

    m_symParser = new CSymParser;

    CString path = GetRezPath();
    if (m_symParser->ParseBuffer(const_cast<char*>(static_cast<const char*>(path)), 1, 0)) {
        ReportError(0x800b, 0x441);
        return 0;
    }

    SetColorDepth(m_colorDepth);

    CWorldSoundSet* in2 = m_inputState;
    if (in2) {
        in2->Deactivate();
        in2->m_list.CPtrList::~CPtrList();
        ::operator delete(in2);
    }
    m_inputState = 0;

    CWorldSoundSet* ni = new CWorldSoundSet();
    m_inputState = ni;
    if (ni->Init(m_world->m_soundRegistry, m_soundVolume) == 0) {
        ReportError(0x800a, 0x442);
        return 0;
    }

    CWorldSoundSet* cur = m_inputState;
    if (m_isAmbientEnabled != 0) {
        if (cur->m_active == 0) {
            cur->m_active = 1;
            cur->Resume();
        }
    } else {
        if (cur->m_active != 0) {
            cur->m_active = 0;
            cur->Stop();
        }
    }
    SetSoundVolume(m_soundVolume);
    return 1;
}

RVA(0x00091e20, 0x17d)
i32 CGruntzMgr::ResetWorldState() {
    CState* st = m_curState;
    if (st == 0) {
        return 1;
    }
    i32 stateId = st->Update();
    if (stateId != GAMESTATE_MENU && stateId != GAMESTATE_ATTRACT) {
        return 1;
    }

    CState* s = m_curState;
    m_modalBusy = 1;
    m_renderGate = 1;
    if (s) {
        delete s;
        m_curState = 0;
    }

    int(WINAPI * show)(BOOL) = ::ShowCursor;
    while (show(1) < 0) {
    }

    CWaitCursor waitCursor;

    if (m_colorDepth == 8) {
        if (LoadWorldMode(0x10) == 0) {
            ReportError(0x801f, 0x443);
            return 0;
        }
    } else {
        if (LoadWorldMode(8) == 0) {
            ReportError(0x801f, 0x444);
            return 0;
        }
    }

    while (show(0) >= 0) {
    }
    TransitionState(stateId, 1, 0, 0);
    m_modalBusy = 0;
    m_renderGate = 0;
    return 1;
}

RVA(0x00092000, 0x16)
void CGruntzMgr::StopBankIfActive() {
    if (m_sound && m_musicEnabled) {
        m_sound->StopAll();
    }
}

RVA(0x00092030, 0x18)
void CGruntzMgr::StopBank0IfActive() {
    if (m_sound && m_musicEnabled) {
        m_sound->StopBank(0);
    }
}

RVA(0x00092060, 0x3c)
i32 CGruntzMgr::SetAssetRoot(char* path) {
    if (path == 0) {
        return 0;
    }
    CAssetRootStorage::s_value = path;
    ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x80ab, 0);
    return 1;
}

RVA(0x000920e0, 0x32)
i32 CGruntzMgr::PostSlotCommandB1(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x80b1, slot);
    return 1;
}

RVA(0x00092130, 0x32)
i32 CGruntzMgr::PostSlotCommandB6(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x80b6, slot);
    return 1;
}

RVA(0x00092a30, 0x52)
INT_PTR CALLBACK PsycheDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x00091500, 0x42)
i32 CGruntzMgr::IsLobbyHostReady() {
    if (m_curState == 0) {
        return 0;
    }
    CGameApp* app = m_owner;
    if (app == 0) {
        return 0;
    }
    if (app->m_appActive == 0) {
        return 0;
    }
    if (m_modalBusy != 0) {
        return 0;
    }
    return m_curState->OnPaint() != 0;
}

RVA(0x0008e880, 0x27)
i32 CGruntzMgr::RegisterSetSkillDebugCmd() {
    if (m_curState->Update() == GAMESTATE_PLAY) {
        RunModalDialog("DEBUG_SETSKILL", SetSkillLevelDialogProc, 1);
    }
    return 0;
}

RVA(0x000915d0, 0x3f)
void CGruntzMgr::MuteMusicIfActive(i32 ms) {
    if (m_sound == 0) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != 0) {
        ok = m_sound->m_pCurrent->IsBusy();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }

    CGruntzSoundZ* snd = m_sound;
    if (snd->m_pCurrent) {
        snd->m_pCurrent->SetVolume(0, ms);
    }
}

RVA(0x00091620, 0x3f)
void CGruntzMgr::RestoreMusicVolumeIfActive(i32 ms) {
    if (m_sound == 0) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != 0) {
        ok = m_sound->m_pCurrent->IsBusy();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }

    CGruntzSoundZ* snd = m_sound;
    if (snd->m_pCurrent) {
        snd->m_pCurrent->SetVolume(kSoundVolumeMax, ms);
    }
}

RVA(0x00091a10, 0x17)
i32 CGruntzMgr::SetVoiceVolume(i32 v) {
    m_voiceVolume = v;
    CGruntSpawnConfig* timer = m_cueSink;
    if (timer) {
        timer->m_voiceVolume = v;
    }
    return v;
}

RVA(0x000920b0, 0x1c)
i32 CGruntzMgr::TickStateMgrs() {
    g_inputMgr->PollAll();
    g_spawnConfig->Flush();
    return 1;
}

RVA(0x00092340, 0x49)
void CGruntzMgr::SetRunState(i32 v) {
    if (v == m_soundEnabled) {
        return;
    }
    m_soundEnabled = v;
    if (m_world == 0) {
        return;
    }
    SoundStream* sub = m_world->m_soundRegistry->m_soundStream;
    if (sub) {
        sub->Stop();
    }

    i32 run = m_soundEnabled;
    g_sndEnabled = run;
    if (m_soundEnabled) {
        m_inputState->Resume();
    } else {
        m_inputState->Stop();
    }
}

RVA(0x00092900, 0x6e)
CState* CGruntzMgr::FindStateById(i32 id) {
    if (m_curState && m_curState->Update() == id) {
        return m_curState;
    }
    CPtrArray* st = &m_stateStack;
    for (i32 i = 0; i < st->GetSize(); i++) {
        CState* s = static_cast<CState*>(st->GetAt(i));
        if (s && s->Update() == id) {
            return s;
        }
    }
    return 0;
}

RVA(0x00092990, 0x8)
CPlay* CGruntzMgr::PickPlayOrPausedState() {
    return static_cast<CPlay*>(FindStateById(3));
}

RVA(0x000929b0, 0x19)
CState* CGruntzMgr::PickPausedThenPlayState() {
    CState* s = FindStateById(0x11);
    if (s) {
        return s;
    }
    return FindStateById(3);
}

RVA(0x000923b0, 0x47)
void CGruntzMgr::SetSoundLevelState(i32 loaded) {
    if (loaded == m_musicEnabled) {
        return;
    }
    m_musicEnabled = loaded;
    CGruntzSoundZ* snd = m_sound;
    if (snd == 0) {
        return;
    }
    if (loaded != 0) {
        CGruntzSoundInnerZ* cur = snd->m_pCurrent;
        if (cur == 0) {
            return;
        }
        if (cur->m_playMode != 0) {
            snd->Restart(1);
        } else if (snd->m_pCurrent != 0) {

            snd->StopBank(1);
        }
        return;
    }
    snd->StopAll();
}

RVA(0x00092500, 0x17)
i32 CGruntzMgr::RunLoadGameDialog() {
    RunModalDialog("GAME_LOAD", GruntzLoadGameDlgProc, 0);
    return 1;
}

RVA(0x00092530, 0x17c)
i32 CGruntzMgr::Quicksave() {
    if (m_saveSink == 0) {
        return 0;
    }
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    if (m_cheatMgr->m_cheatsUsed != 0) {
        CString name;
        name.LoadStringA(0x81aa);
        EnterModalUI(name);
        return 1;
    }
    if (m_saveInfoRec == 0 || !(m_saveInfoRec->m_flags & 1)) {
        return LoadSaveMessageSprite();
    }

    if (&(static_cast<CPlay*>(m_curState))->m_saveSlot == 0) {
        return 0;
    }
    if (m_cueSink) {
        m_cueSink->PauseAllVoices();
    }
    FillSaveInfo(m_saveInfoRec, 0);

    if (g_gameReg->m_saveSink->Save(m_saveInfoRec->m_serial, 0x81a7) == 0) {
        EnterModalUI("ERROR - Cannot Save Game.");
        return 1;
    }
    m_chatLog->AddItem("Game Quicksaved successfully.", 0, 0x11);
    return 1;
}

RVA(0x00092710, 0x77)
i32 CGruntzMgr::Quickload() {
    if (m_saveSink == 0) {
        return 0;
    }
    if (m_cueSink) {
        m_cueSink->PauseAllVoices();
    }
    if (m_saveInfoRec && (m_saveInfoRec->m_flags & 1)) {

        if (m_saveSink->VerifySlot(m_saveInfoRec) == 0) {
            return 1;
        }
        ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x807e, 0);
        m_chatLog->AddItem("Game Quickloaded successfully.", 0, 0x11);
        return 1;
    }
    return RunLoadGameDialog();
}

RVA(0x000929e0, 0x32)
i32 CGruntzMgr::RunDebugGruntTypeDialog() {
    i32 ran = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        ran = RunModalDialog("DEBUG_GRUNTTYPE", DebugGruntTypeDialogProc, 1);
    }
    return ran != 0;
}

RVA(0x0008e780, 0x2a)
i32 CGruntzMgr::DebugJumpLevel() {
    i32 level = RunModalDialog("DEBUG_JUMPLEVEL", JumpLevelDialogProc, 1);
    if (level > 0) {
        return PassClickToPlayState(level, 0, 1);
    }
    return 0;
}

RVA(0x00092d50, 0x3c)
i32 CGruntzMgr::LoadOptionsSlotName(i32 slot, i32, i32, i32, i32, const char* val, i32) {
    if (CheckPlayState()) {
        if (m_options[slot].m_liveGate == 0) {
            m_options[slot].m_name = val;
        }
    }
    return 0;
}

RVA(0x00092da0, 0x3a)
i32 CGruntzMgr::ResetOptionsSlot(i32 idx) {
    if (static_cast<u32>(idx) >= 4) {
        return 0;
    }
    GruntzPlayer* s = &m_options[idx];
    if (s == 0) {
        return 0;
    }
    if (s->m_liveGate == 0) {
        return 0;
    }

    return s->Reset();
}

RVA(0x00092df0, 0x24)
void CGruntzMgr::ResetAllOptionsSlots() {
    GruntzPlayer* s = &m_options[0];
    for (i32 d = 4; d != 0; d--) {
        if (s != 0) {
            s->Reset();
        }
        s++;
    }
}

RVA(0x00092e30, 0x39)
i32 CGruntzMgr::CountReadyOptionsSlots(i32 anyState) {
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* slot = &m_options[i];
        if (slot && slot->m_liveGate != 0 && (anyState != 0 || slot->m_humanControlled != 0)) {
            count++;
        }
    }
    return count;
}

RVA(0x00092e80, 0x25)
GruntzPlayer* CGruntzMgr::FindOptionsSlot(i32 x) {

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* slot = &m_options[i];
        if (slot && slot->m_slotKey == x) {
            return slot;
        }
    }
    return 0;
}

// @early-stop
RVA(0x0008fab0, 0x318)
i32 CGruntzMgr::ChangeState(i32 arg) {
    if (arg < 1 || arg > 3) {
        return 0;
    }
    if (!FileExists(const_cast<char*>(static_cast<const char*>(m_strMoviePath)))) {
        return 0;
    }

    CMoviePlayer player;

    CDDSurface* front = m_world->m_drawTarget->m_frontPair->m_surface;
    IDirectDraw2* dd2 = m_world->m_ptrColl->m_device;

    if (m_world->m_soundRegistry->HasKeyEqual("GAME") == 0) {
        void* snd = m_symParser->ResolvePath("GAME_SOUNDZ");
        if (snd == 0) {
            player.Teardown();
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(snd), "GAME", "_");
    }
    if (front == 0 || dd2 == 0) {
        player.Teardown();
        return 0;
    }

    IDirectSound* dsound = m_world->m_soundStream ? m_world->m_soundStream->m_device : 0;
    if (player.InitMode(m_gameWnd->m_hwnd, dd2, front->m_ddSurface, front->m_apiDesc, dsound)) {
        if (player.Open(m_strMoviePath, arg, 0, m_isInterlaced != 0, 0, 0)) {
            m_modalBusy = 1;
            player.Pump(1, 1);
            m_modalBusy = 0;
        }
    }
    player.Teardown();
    return 1;
}

RVA(0x00083360, 0xb2)
CGruntzMgr::~CGruntzMgr() {
    Close();
}

RVA(0x00085560, 0xb)
i32 CGameMgr::IsActive() {
    return m_gameWnd != 0;
}

RVA(0x00083300, 0x17)
i32 CGruntzMgr::IsActive() {
    if (m_world) {
        if (m_curState) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00085580, 0x5)
i32 CGameMgr::HandleCommand(i32, GruntzCommand, i32) {
    return 0;
}

// @early-stop
RVA(0x0008f7f0, 0x131)
void CGruntzMgr::RecomputeViewScale() {
    if (m_world == 0) {
        return;
    }
    CGameLevel* view = m_world->m_level;
    float fw = static_cast<float>((view->m_planeCtx.right - view->m_planeCtx.left + 1));
    float fh = static_cast<float>((view->m_planeCtx.bottom - view->m_planeCtx.top + 1));

    view->m_rectA.w = static_cast<i32>((fw * 1.4f));
    view->m_rectA.h = static_cast<i32>((fh * 1.4f));
    view->MainPlaneNotify();

    view = m_world->m_level;
    view->m_rectB.w = static_cast<i32>((fw * 5.3f));
    view->m_rectB.h = static_cast<i32>((fh * 5.3f));
    view->MainPlaneNotify();

    view = m_world->m_level;
    view->m_rectC.w = static_cast<i32>((fw * 1.12f));
    view->m_rectC.h = static_cast<i32>((fh * 1.12f));
    view->MainPlaneNotify();

    CGameLevel* v = m_world->m_level;
    if (v->m_mainPlane == 0) {
        return;
    }
    m_viewBounds.left = (v->m_mainPlane)->m_viewRect.left - 0x60;
    m_viewBounds.top = (m_world->m_level->m_mainPlane)->m_viewRect.top - 0x60;
    m_viewBounds.right = (m_world->m_level->m_mainPlane)->m_viewRect.right + 0x60;
    m_viewBounds.bottom = (m_world->m_level->m_mainPlane)->m_viewRect.bottom + 0x60;
}

RVA(0x00093460, 0x15c)
i32 CGruntzMgr::BroadcastCmd(CFileMemBase* ar, i32 cmd, i32 typeId, i32 pObj) {
    if (ar == 0) {
        return 0;
    }
    switch (cmd) {
        case 4:

            if (SaveState(ar) == 0) {
                return 0;
            }
            break;
        case 7:
            if (LoadState(ar) == 0) {
                return 0;
            }
            m_cueSink->ClearSprites();
            break;
    }

    i32 i;
    GruntzPlayer* slot;
    for (i = 0, slot = m_options; i < 4; i++) {
        if (slot == 0 || slot->Serialize(ar, cmd, typeId, pObj) == 0) {
            return 0;
        }
        slot++;
    }

    if (m_cmdGrid->Serialize(ar, cmd, typeId, pObj) == 0) {
        return 0;
    }
    if (PickPlayOrPausedState()->SyncState(ar, cmd, typeId, pObj) == 0) {
        return 0;
    }
    if (m_cmdSubMgr->Serialize(ar, cmd, typeId, pObj) == 0) {
        return 0;
    }

    if (m_tileGrid->Visit(ar, cmd, typeId, pObj) == 0) {
        return 0;
    }

    if (MapSerializeCurve(ar, cmd, typeId, pObj) == 0) {
        return 0;
    }
    return m_scoreHud->Serialize(ar, cmd, typeId, pObj) != 0;
}

RVA(0x000860b0, 0xe8)
void CGruntzMgr::UpdateScoreHud() {
    if (g_gameReg->m_gameMode != 1) {
        return;
    }
    CState* sub = g_gameReg->m_curState;

    m_scoreHud->m_gruntzExited += m_cmdGrid->m_gruntzExitedByPlayer[g_curPlayer];
    m_scoreHud->m_gruntzLost += m_cmdGrid->m_gruntzLostByPlayer[g_curPlayer];

    if (m_strWorldFile.GetLength() != 0) {
        m_scoreHud->SetCount(1);
        m_scoreHud->m_isCustomLevel = 1;
        return;
    }

    if (m_cheatMgr->m_cheatsUsed == 0) {
        m_scoreHud->FillRecord(sub->m_levelIndex, 0);
        g_gameReg->m_saveSink->SetCurLevel(sub->m_levelIndex);
        g_gameReg->m_saveSink->SetMaxLevel((sub->m_levelIndex % 0x28) + 1);
        g_gameReg->m_saveSink->Save(0, 0x81a6);
    }
    m_scoreHud->SetCount(sub->m_levelIndex);
    m_scoreHud->m_isCustomLevel = 0;
}

RVA(0x00093620, 0x254)
i32 CGruntzMgr::SaveState(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    g_serialCounter++;

    char buf[0x80];
    memset(buf, 0, 0x80);
    strcpy(buf, m_strWorldFile);
    ar->Write(buf, 0x80);

    ar->Write(&m_loadingSaveGame, 4);
    ar->Write(&m_soundVolume, 4);
    ar->Write(&m_isBattlezLevel, 4);
    ar->Write(&m_isMultiLevel, 4);
    ar->Write(&m_isCustomLevel, 4);
    ar->Write(&m_gameMode, 4);
    ar->Write(&m_optionsCount, 4);
    ar->Write(&m_viewBounds.left, 0x10);
    ar->Write(&g_lastNow, 4);
    ar->Write(&g_frameDelta, 4);
    ar->Write(&g_frameTime, 4);
    ar->Write(&g_frameTicks, 4);
    ar->Write(&g_timer32, 4);
    ar->Write(&g_timer100, 4);
    ar->Write(&g_timer200, 4);
    ar->Write(&g_timer400, 4);
    ar->Write(&g_timer500, 4);
    ar->Write(&g_traitorMode, 4);
    ar->Write(&g_gruntCreation, 4);
    ar->Write(&g_gruntDestruction, 4);
    ar->Write(&g_gooPuddlez, 4);
    ar->Write(&g_explosionz, 4);
    ar->Write(&m_isEasyMode, 4);
    ar->Write(&g_monologoShown, 4);
    ar->Write(&g_jitterX, 4);
    ar->Write(&g_jitterY, 4);
    ar->Write(&g_panMinX, 4);
    ar->Write(&g_panMaxX, 4);
    ar->Write(&g_warpX, 4);
    ar->Write(&g_warpY, 4);
    return 1;
}

RVA(0x00093920, 0x22f)
i32 CGruntzMgr::LoadState(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    g_serialCounter++;

    char buf[0x80];
    ar->Read(buf, 0x80);
    m_strWorldFile = buf;

    ar->Read(&m_loadingSaveGame, 4);
    ar->Read(&m_soundVolume, 4);
    ar->Read(&m_isBattlezLevel, 4);
    ar->Read(&m_isMultiLevel, 4);
    ar->Read(&m_isCustomLevel, 4);
    ar->Read(&m_gameMode, 4);
    ar->Read(&m_optionsCount, 4);
    ar->Read(&m_viewBounds.left, 0x10);
    ar->Read(&g_lastNow, 4);
    ar->Read(&g_frameDelta, 4);
    ar->Read(&g_frameTime, 4);
    ar->Read(&g_frameTicks, 4);
    ar->Read(&g_timer32, 4);
    ar->Read(&g_timer100, 4);
    ar->Read(&g_timer200, 4);
    ar->Read(&g_timer400, 4);
    ar->Read(&g_timer500, 4);
    ar->Read(&g_traitorMode, 4);
    ar->Read(&g_gruntCreation, 4);
    ar->Read(&g_gruntDestruction, 4);
    ar->Read(&g_gooPuddlez, 4);
    ar->Read(&g_explosionz, 4);
    ar->Read(&m_isEasyMode, 4);
    ar->Read(&g_monologoShown, 4);
    ar->Read(&g_jitterX, 4);
    ar->Read(&g_jitterY, 4);
    ar->Read(&g_panMinX, 4);
    ar->Read(&g_panMaxX, 4);
    ar->Read(&g_warpX, 4);
    ar->Read(&g_warpY, 4);
    return 1;
}

RVA(0x000927b0, 0xc4)
i32 CGruntzMgr::FillSaveInfo(SaveSlot* dst, void* snapshot) {
    if (dst == 0) {
        return 0;
    }
    CPlay* src = PickPlayOrPausedState();
    if (src == 0) {
        return 0;
    }

    strcpy(dst->m_levelName, GetWorldFileName());
    dst->m_isWon = (m_gameMode == 3);
    dst->m_isCustom = m_isCustomLevel;

    m_saveSink->CopySlot(dst, &src->m_saveSlot);
    m_saveInfoRec = dst;
    if (snapshot) {
        strncpy(static_cast<char*>(dst->m_snapshot), static_cast<char*>(snapshot), 0x20);
    }
    return 1;
}

RVA(0x0008e980, 0x11e)
i32 CGruntzMgr::FinishLevel(i32 full, i32 stopBank) {
    if (m_curState && m_curState->Update() == GAMESTATE_NONE) {

        i32 done = 0;
        CNetCmdSlot* s = static_cast<CMulti*>(m_curState)->m_session->m_slots;
        for (i32 d = 4; d != 0; d--) {
            if (s != 0 && s->m_state == 3) {
                done++;
            }
            s++;
        }
        if (done > 0) {
            m_frameGate = 1;

            (static_cast<CMulti*>(m_curState))->OnPauseChannel();
            m_frameGate = 0;
            return 1;
        }
    }

    if (full) {
        if (m_inputState) {
            m_inputState->Stop();
        }
        if (m_world) {
            CDDrawSubMgrLeafScan* sub = m_world->m_soundRegistry;
            if (sub && sub->m_soundStream) {
                sub->m_soundStream->Stop();
            }
        }
        CGruntzSoundZ* snd = m_sound;
        if ((snd->m_pCurrent ? snd->m_pCurrent->IsBusy() : 0) && stopBank) {
            m_sound->StopAll();
        }
        m_curState->PauseGame();
    }
    if (full) {
        return 1;
    }

    if (m_musicEnabled) {
        if (CheckPlayState()) {
            m_sound->StopBank(1);
        }
    }
    if (m_soundEnabled) {
        m_inputState->Resume();
        if (m_cmdGrid && m_soundEnabled) {
            m_cmdGrid->DestroyAllAnims();
        }
    }
    m_curState->ResumeGame();
    g_inputMgr->ReadAll();
    RefreshGameClock();
    return 1;
}

DATA_SYMBOL(0x0024556c, 0x4, _g_gameReg)

RVA(0x0008ef10, 0x9e)
void CGruntzMgr::EnterModalUI(const char* msg) {
    CGameApp* app = m_owner;
    if (app == 0) {
        return;
    }
    if (m_cueSink) {
        m_cueSink->PauseAllVoices();
    }
    if (m_world) {
        m_world->m_drawTarget->BlitPage(m_world->m_drawTarget->m_backPair);

        CDDrawPtrCollections* pc = m_world->m_ptrColl;
        pc->m_device->FlipToGDISurface();
    }

    int(WINAPI * show)(BOOL) = ::ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    static_cast<CGruntzApp*>(app)->ShowMessage(msg, m_gameWnd->m_hwnd);
    NetLobby::g_curDlg = 0;
    m_modalBusy = 0;
    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }
}

RVA(0x000903f0, 0x10c)
i32 CGruntzMgr::ExitModalUI(CDialog* dlg, i32 notify) {
    if (m_cueSink) {
        m_cueSink->PauseAllVoices();
    }
    if (m_cmdGrid && m_soundEnabled) {
        m_cmdGrid->DestroyAllAnims();
    }
    if (m_world) {
        if (notify && m_curState && m_curState->Update() != GAMESTATE_MENU) {
            m_curState->Present(0x32);
        } else {
            notify = 0;
        }

        CDDrawPtrCollections* pc = m_world->m_ptrColl;
        pc->m_device->FlipToGDISurface();
    }

    int(WINAPI * show)(BOOL) = ::ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    i32 result = dlg->DoModal();
    NetLobby::g_curDlg = 0;
    m_modalBusy = 0;
    if (m_curState && notify) {
        m_curState->RestoreDisplay();
    }

    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }

    RefreshGameClock();

    CPlay* o = static_cast<CPlay*>(PickPausedThenPlayState());
    if (o) {
        if (o->m_guts) {
            (static_cast<CStatusBarMgr*>(o->m_guts))->Deactivate();
        }
        o->PostHudRect();
    }
    return result;
}

RVA(0x0008d6a0, 0xaf)
i32 CGruntzMgr::SwitchToNextState() {
    if (IsActive() == 0) {
        return 0;
    }
    CState* next = TopState();
    if (next == 0) {
        return 0;
    }
    if (m_curState == next) {
        return 0;
    }
    i32 oldId = 0;
    if (m_curState) {
        oldId = m_curState->Update();
        m_curState->LeaveState(next->Update());
        if (m_curState) {
            delete m_curState;
        }
        m_curState = 0;
    }
    m_curState = next;
    PopTopIfMatches(next);
    if (m_curState->EnterState(oldId) == 0 && m_curState->RestoreDisplay() == 0) {
        return 0;
    }
    m_owner->m_running = 1;
    RefreshGameClock();
    return 1;
}

// @early-stop
RVA(0x0008d780, 0x95)
i32 CGruntzMgr::PassClickToPlayState(i32 areaArg, i32 forceTransition, i32 unused) {
    i32 inPlay = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        inPlay = 1;
    }
    if (m_curState->Update() == GAMESTATE_NONE) {
        inPlay = 1;
    }
    if (inPlay && forceTransition == 0) {
        m_curState->LeaveState(m_curState->Update());
        if (static_cast<CPlay*>(m_curState)->LoadByMode(areaArg, unused) == 0) {
            return 0;
        }
        m_curState->EnterState(m_curState->Update());
        return 1;
    }
    return TransitionState(3, areaArg, 0, 0);
}

RVA(0x0008f740, 0x46)
void CGruntzMgr::UnloadSoundChain() {
    if (m_world) {
        CDDrawSubMgrLeafScan* sub = m_world->m_soundRegistry;
        if (sub) {
            SoundStream* obj = sub->m_soundStream;
            if (obj) {
                obj->Stop();
            }
        }
    }
    CGruntzSoundZ* snd = m_sound;
    if (snd && (snd->m_pCurrent ? snd->m_pCurrent->IsBusy() : 0)) {
        m_sound->IsPlaying();
    }
}

RVA(0x000919d0, 0x30)
void CGruntzMgr::SetSoundVolume(i32 v) {
    m_soundVolume = v;
    if (m_world && m_world->m_soundRegistry) {
        g_sndCueTag = v;
    }
    CWorldSoundSet* in = m_inputState;
    if (in) {
        in->Restart(v);
    }
}

RVA(0x00092ec0, 0x24)
void CGruntzMgr::ClearOptionsSlots() {

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* p = &m_options[i];
        if (p != 0) {
            p->m_liveGate = 0;
            p->m_clearedRound = 0;
        }
    }
}

RVA(0x000933e0, 0x5e)
i32 CGruntzMgr::AdvanceOptionsCycle() {
    i32 cursor = (g_optionsCursor + 1) & 3;
    g_optionsCursor = cursor;
    for (i32 i = 0; i < m_optionsCount + 1; i++) {
        GruntzPlayer* slot = &m_options[i];
        if (cursor == i && slot->m_humanControlled == 0 && slot->m_liveGate != 0) {
            slot->m_battlezConfig.StepBoard();
            cursor = g_optionsCursor;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00093170, 0x1e3)
i32 CGruntzMgr::SyncOptionsState() {
    i32 matched = 0;
    CString s;
    if (s.LoadString(0x81ab)) {
        bool eq;
        eq = (strcmp(s, m_strWorldFile) == 0);
        if (eq) {
            matched = 1;
        }
    }
    srand(static_cast<u32>(time(0)));
    g_optionsCursor = 0;

    i32 idx = 0;
    GruntzPlayer* opt = &m_options[0];
    for (i32 i = 0; i < m_optionsCount; i++) {
        i32 cfg;
        if (idx == g_curPlayer) {
            opt->m_humanControlled = 1;
            cfg = opt->m_configId;
            if (matched) {
                cfg = 0;
            }
            if (!opt->m_battlezConfig.LoadConfig(this, idx, cfg)) {
                return 0;
            }
            opt->m_battlezConfig.Clear();
            opt++;
            idx++;
            opt->m_humanControlled = 0;
            cfg = opt->m_configId;
            if (matched) {
                cfg = 0;
            }
            if (!opt->m_battlezConfig.LoadConfig(this, idx, cfg)) {
                return 0;
            }
        } else {
            opt->m_humanControlled = 0;
            cfg = opt->m_configId;
            if (matched) {
                cfg = 0;
            }
            if (!opt->m_battlezConfig.LoadConfig(this, idx, cfg)) {
                return 0;
            }
        }
        idx++;
        opt++;
    }
    return 1;
}

// @early-stop
RVA(0x000855e0, 0x448)
void CGruntzMgr::Close() {
    if (m_world) {
        m_world->SetRestoreHandler(0);
    }
    FreeFontsMemory();
    if (m_settings) {
        m_settings->SetValueDword("Num_Runs", m_numRuns);
        m_settings->SetValueDword("Num_Movies", m_numMovies);
        m_settings->SetValueDword("Sound", m_soundEnabled);
        m_settings->SetValueDword("Voice", m_isVoiceEnabled);
        m_settings->SetValueDword("Ambient", m_isAmbientEnabled);
        m_settings->SetValueDword("Music", m_musicEnabled);
        m_settings->SetValueDword("Interlaced", m_isInterlaced);
        m_settings->SetValueDword("High_Detail", m_isHighDetail);
        m_settings->SetValueDword("Effects", m_isEffectsEnabled);
        m_settings->SetValueDword("Disable_Joystick", g_disableJoystick);
        if (m_sound) {
            m_settings->SetValueDword("Music_Volume", m_sound->GetXMidiVolume());
        }
        if (m_cueSink) {
            m_settings->SetValueDword("Voice_Volume", m_cueSink->m_voiceVolume);
        }
        if (m_world && m_world->m_soundRegistry) {
            m_settings->SetValueDword("Sound_Volume", g_sndCueTag);
        }
        m_settings->SetValueDword("Scroll_Speed", m_scrollSpeed);
        m_settings->SetValueDword("Easy_Mode", m_isEasyMode);
        i32 res = RES_640x480;
        if (m_savedModeW == 0x400 && m_savedModeH == 0x300) {
            res = RES_1024x768;
        } else if (m_savedModeW == 0x320 && m_savedModeH == 0x258) {
            res = RES_800x600;
        }
        m_settings->SetValueDword("Resolution", res);
        m_settings->SetValueDword("Checkpoint_Prompts", m_isCheckpointPrompts);
        m_settings->SetValueDword("Enable_HiColor", m_colorDepth == 0x10 ? 1 : 0);
        m_settings->SetValueDword("Enable_TrueColor", 0);
    }
    ClearStateStack();
    if (m_curState) {
        delete m_curState;
        m_curState = 0;
    }
    if (m_spriteFactory) {
        m_spriteFactory->Reset();
        operator delete(m_spriteFactory);
        m_spriteFactory = 0;
    }
    if (m_cmdGrid) {
        delete m_cmdGrid;
        m_cmdGrid = 0;
    }
    if (m_tileGrid) {

        delete m_tileGrid;
        m_tileGrid = 0;
    }
    CBattlezData* scoreHud = m_scoreHud;
    if (scoreHud) {
        delete scoreHud;
        m_scoreHud = 0;
    }
    if (m_cmdSubMgr) {

        delete m_cmdSubMgr;
        m_cmdSubMgr = 0;
    }
    if (g_spawnConfig) {
        StateMgrBZ* v = g_spawnConfig;
        v->m_device = 0;
        v->m_keyboard = 0;
        v->m_joystick = 0;
        v->m_joystick2 = 0;
        v->m_deviceList = 0;
        v->m_mode = 0;
        operator delete(v);
        g_spawnConfig = 0;
    }
    if (g_inputMgr) {

        delete g_inputMgr;
        g_inputMgr = 0;
    }
    if (m_cheatMgr) {
        delete m_cheatMgr;
        m_cheatMgr = 0;
    }
    if (m_sound) {
        m_sound->Shutdown();
        operator delete(m_sound);
        m_sound = 0;
    }
    if (m_inputState) {
        m_inputState->Teardown();
        operator delete(m_inputState);
        m_inputState = 0;
    }
    if (m_faderMgr) {
        delete m_faderMgr;
        m_faderMgr = 0;
    }
    if (m_chatLog) {

        m_chatLog->~CFontConfig();
        operator delete(m_chatLog);
        m_chatLog = 0;
    }
    if (m_cueSink) {
        m_cueSink->~CGruntSpawnConfig();
        operator delete(m_cueSink);
        m_cueSink = 0;
    }
    if (m_world) {
        delete m_world;
        m_world = 0;
    }
    if (m_symParser) {
        delete m_symParser;
        m_symParser = 0;
    }
    if (m_settings) {

        delete m_settings;
        m_settings = 0;
    }
    if (m_reserved3c) {
        delete m_reserved3c;
        m_reserved3c = 0;
    }
    if (m_shadeCache) {
        delete m_shadeCache;
        m_shadeCache = 0;
    }
    if (m_saveSink) {

        delete m_saveSink;
        m_saveSink = 0;
    }
    if (m_logicPump) {
        m_logicPump->Reset();
        operator delete(m_logicPump);
        m_logicPump = 0;
    }
    CloseSoundFontDevice();
    if (m_lobby) {
        m_lobby->Release();
        m_lobby = 0;
    }
    if (m_connSettings) {
        operator delete(m_connSettings);
        m_connSettings = 0;
    }
    this->CGameMgr::Close();
    g_gameReg = 0;
}

RVA(0x000861e0, 0xc5)
void CGruntzMgr::AccrueScoreTime() {
    CState* st = m_curState;
    if (m_gameMode == 1) {
        if (m_cmdGrid->m_phase == 1) {
            UpdateScoreHud();
        }
        TransitionState(0xa, 1, 0, 0);
        return;
    }
    g_gameReg->m_scoreHud->SetCount(st->m_levelIndex);
    if (m_gameMode == 3) {

        CTimer* clk = (static_cast<CPlay*>(st))->m_frameMarker;
        i64 d = static_cast<i64>(g_frameTime) - clk->m_startStamp.m_v;
        g_gameReg->m_scoreHud->m_elapsedTimeMs += (d < 0) ? 0 : static_cast<i32>(d);
        TransitionState(0x12, 1, 0, 0);
        return;
    }
    CBattlezData* hud = g_gameReg->m_scoreHud;
    u32 now = ::timeGetTime();
    hud->m_elapsedTimeMs += (now - g_scoreTimeBase);
    TransitionState(0x12, 1, 0, 0);
}

RVA(0x0008e6c0, 0x85)
void CGruntzMgr::OnCheckpointReached() {
    if (m_isCheckpointPrompts == 0) {
        return;
    }
    CCheckpointDlg dlg(0);
    if (ExitModalUI(&dlg, 0) == 1) {
        ::SendMessageA(m_gameWnd->m_hwnd, 0x111, 0x80cf, 0);
    }
}

// @early-stop
RVA(0x0008f530, 0xbd)
void CGruntzMgr::DelayedQuit() {
    if (m_delayedQuitPending != 0) {
        return;
    }
    m_delayedQuitPending = 1;
    LeafCue* out = 0;
    MapLookup(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE", out);
    i32 base;
    if (out != 0) {
        out = 0;
        MapLookup(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE", out);
        base = out->m_sound->m_durationMs + 0x1f4;
    } else {
        base = 0;
    }
    base += ::timeGetTime();
    u32 deadline = base;
    while (::timeGetTime() < deadline) {
    }
    if (m_owner) {
        m_owner->m_running = 0;
    }
    if (m_gameWnd) {
        ::PostMessageA(m_gameWnd->m_hwnd, 0x10, 0, 0);
    }
}

RVA(0x000907c0, 0x77)
i32 CGruntzMgr::LaunchPortal(i32 quitAfter) {
    char path[256];
    path[0] = 0;
    if (!LaunchPortalExe(path)) {
        return 0;
    }
    if (path[0] == 0) {
        return 0;
    }
    if (!LaunchProcessInDir(path, 0)) {
        return 0;
    }
    if (quitAfter) {
        DelayedQuit();
    }
    return 1;
}

RVA(0x00090260, 0x13e)
i32 CGruntzMgr::RunModalDialog(const char* tmpl, DLGPROC dlgProc, i32 flag) {
    if (tmpl == 0) {
        return 0;
    }
    if (dlgProc == 0) {
        return 0;
    }
    if (m_cueSink) {
        m_cueSink->PauseAllVoices();
    }
    if (m_cmdGrid && m_soundEnabled) {
        m_cmdGrid->DestroyAllAnims();
    }
    if (m_world) {
        if (flag && m_curState && m_curState->Update() != GAMESTATE_MENU) {
            m_curState->Present(0x32);
        } else {
            flag = 0;
        }

        CDDrawPtrCollections* pc = m_world->m_ptrColl;
        pc->m_device->FlipToGDISurface();
    }

    int(WINAPI * show)(BOOL) = ::ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    i32 result = ::DialogBoxParamA(
        m_owner->m_hInstance,
        tmpl,
        m_gameWnd->m_hwnd,
        static_cast<DLGPROC>(dlgProc),
        0
    );
    NetLobby::g_curDlg = 0;
    m_modalBusy = 0;
    if (m_curState && flag) {
        m_curState->RestoreDisplay();
    }
    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }

    RefreshGameClock();
    CPlay* o = static_cast<CPlay*>(PickPausedThenPlayState());
    if (o) {
        if (o->m_guts) {
            (static_cast<CStatusBarMgr*>(o->m_guts))->Deactivate();
        }
        o->PostHudRect();
    }
    return result;
}

RVA(0x00092420, 0xa4)
i32 CGruntzMgr::LoadSaveMessageSprite() {
    if (m_cheatMgr->m_cheatsUsed != 0) {
        CString name;
        name.LoadStringA(0x81aa);
        EnterModalUI(name);
    } else if (RunModalDialog("GAME_SAVE", SaveGameDialogProc, 0) == 1) {
        RunModalDialog("GAME_SAVEMSG", OkCancelDialogProc, 0);
    }
    return 1;
}

RVA(0x00092f00, 0x1ef)
i32 CGruntzMgr::SaveGameAs() {
    CBattlezDlg dlg(this, 0);
    i32 st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY && st != 7) {
        return 0;
    }
    ChannelSlots_InitAll();
    if (ExitModalUI(&dlg, 1) != 1) {
        return 0;
    }
    if (dlg.m_customNameFlag != 0) {
        m_isBattlezLevel = 0;
        m_strWorldFile = "custom\\" + dlg.m_worldName;
    } else {
        m_isBattlezLevel = 1;
        m_strWorldFile = dlg.m_worldName;
    }
    if (m_strWorldFile.GetLength() == 0) {
        return 0;
    }
    ::PostMessageA(m_gameWnd->m_hwnd, 0x111, 0x80e3, 0);
    return 1;
}

RVA(0x00083030, 0x1b6)
CGruntzMgr::CGruntzMgr() {
    m_curState = 0;
    m_world = 0;
    m_symParser = 0;
    m_settings = 0;
    m_scoreHud = 0;
    m_reserved3c = 0;
    m_faderMgr = 0;
    m_cheatMgr = 0;
    m_sound = 0;
    m_reserved4c = 0;
    m_shadeCache = 0;
    m_reserved64 = 0;
    m_lobby = 0;
    m_inputState = 0;
    m_saveSink = 0;
    m_chatLog = 0;
    m_cueSink = 0;
    m_cmdGrid = 0;
    m_cmdSubMgr = 0;
    m_tileGrid = 0;
    m_spriteFactory = 0;
    m_logicPump = 0;
    m_lobbyResult = 0;
    m_lobbyProbed = 0;
    m_delayedQuitPending = 0;
    m_reserveda8 = 0;
    m_modalBusy = 0;
    m_renderGate = 0;
    m_reservedb4 = 0;
    m_loadingSaveGame = 0;
    m_isCheckpointPrompts = 1;
    m_connSettings = 0;
    m_saveInfoRec = 0;
    m_numRuns = 0;
    m_numMovies = 0;
    m_reservedcc = 0x1e;
    m_modeW = 0;
    m_modeH = 0;
    m_colorDepth = 0x10;
    m_inGameDir = 1;
    m_haveRez = 0;
    m_haveMoviez = 0;
    m_musicEnabled = 1;
    m_soundEnabled = 1;
    m_isVoiceEnabled = 1;
    m_isAmbientEnabled = 1;
    m_isInterlaced = 0;
    m_isEasyMode = 0;
    m_isCustomLevel = 0;
    m_isBattlezLevel = 0;
    m_isMultiLevel = 0;
    m_gameMode = 0;
    m_isHighDetail = 1;
    m_isEffectsEnabled = 1;
    m_optionsCount = 3;
}

RVA(0x0008e1d0, 0xa5)
i32 CGruntzMgr::CheckDisplayBoundsA() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_NONE) {
        return 1;
    }
    CDdModePair pt;
    pt = m_world->m_ptrColl->FindFwd(m_modeW, m_modeH, m_colorDepth);
    i32 x = pt.a;
    i32 y = pt.b;
    if (x > 0x514 || x == -1 || y == -1) {
        return 1;
    }
    if (SetVideoMode(x, y, 1)) {
        return 1;
    }
    if (SetVideoMode(0x280, 0x1e0, 1)) {
        return 1;
    }
    ReportError(0x8008, 0x439);
    return 0;
}

RVA(0x0008e2b0, 0xb1)
i32 CGruntzMgr::CheckDisplayBoundsB() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_NONE) {
        return 1;
    }
    CDdModePair pt;
    pt = m_world->m_ptrColl->FindBack(m_modeW, m_modeH, m_colorDepth);
    i32 x = pt.a;
    i32 y = pt.b;
    if (x == -1 || y == -1 || x < 0x140 || y < 0xc8) {
        return 1;
    }
    if (SetVideoMode(x, y, 1)) {
        return 1;
    }
    if (SetVideoMode(0x280, 0x1e0, 1)) {
        return 1;
    }
    ReportError(0x8008, 0x43a);
    return 0;
}

RVA(0x0008e3a0, 0x94)
RECT* CGruntzMgr::GetRect(RECT* out) {
    RECT local;
    SetRect(&local, 0, 0, 0x27f, 0x1df);
    if (!m_world) {
        *out = local;
        return out;
    }
    local = m_world->m_level->m_planeCtx;
    *out = local;
    return out;
}

RVA(0x0008e4e0, 0x172)
INT_PTR CALLBACK WarpDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szValue[64];

    switch (msg) {
        case WM_INITDIALOG: {

            CDDrawWorkerHost* warp = g_gameReg->m_world->m_level->m_mainPlane;
            i32 seedX = warp->m_snappedX;
            i32 seedY = warp->m_snappedY;
            SetDlgItemInt(hDlg, 0x40e, seedX, 0);
            SetDlgItemInt(hDlg, 0x40f, seedY, 0);
            return 1;
        }

        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                i32 valX = GetDlgItemInt(hDlg, 0x40e, 0, 0);
                i32 valY = GetDlgItemInt(hDlg, 0x40f, 0, 0);
                g_warpX = valX;
                g_warpY = valY;
                if (IsDlgButtonChecked(hDlg, 0x410)) {
                    sprintf(szValue, "Level %i Warp X", g_gameReg->m_curState->m_levelIndex);
                    g_gameReg->m_settings->SetValueDword(szValue, valX);
                    sprintf(szValue, "Level %i Warp Y", g_gameReg->m_curState->m_levelIndex);
                    g_gameReg->m_settings->SetValueDword(szValue, valY);
                    g_gameReg->m_settings->SetValueDword(
                        "Last Warp Level",
                        g_gameReg->m_curState->m_levelIndex
                    );
                }
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0008e7c0, 0x86)
INT_PTR CALLBACK JumpLevelDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, g_gameReg->m_curState->m_levelIndex, 0);
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0008e8c0, 0x86)
INT_PTR CALLBACK SetSkillLevelDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, g_gameReg->m_curState->m_levelIndex, 0);
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0008df00, 0x238)
i32 CGruntzMgr::SetVideoMode(i32 w, i32 h, i32 flag) {
    if (w == m_modeW && h == m_modeH) {
        return 1;
    }
    if (m_world == 0) {
        return 0;
    }
    if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_NONE) {
        if (m_world->m_level != 0) {
            CDDrawWorkerHost* f = m_world->m_level->m_mainPlane;
            if (f != 0) {
                if (w > f->m_wrapW || h > f->m_wrapH) {
                    CPlay* st = static_cast<CPlay*>(m_curState);
                    st->ResetViewport();
                    if (st->m_guts != 0) {
                        st->m_guts->m_barFrameGate = m_modeH;
                        if (st->m_guts->m_position == 0) {
                            st->m_guts->RefreshA();
                            st->m_guts->DockStatusBarRight();
                            EnterModalUI(
                                "This map is too small to be displayed under your "
                                "desired video resolution. Default resolution will "
                                "be used."
                            );
                            return 0;
                        }
                        if (st->m_guts->m_position == 1) {
                            st->m_guts->DockStatusBarRight();
                            st->m_guts->RefreshA();
                        }
                    }
                    EnterModalUI(
                        "This map is too small to be displayed under your desired "
                        "video resolution. Default resolution will be used."
                    );
                    return 0;
                }
            }
        }
    }

    if (!m_world->SetDimensions(w, h, m_colorDepth)) {
        return 0;
    }
    while (::ShowCursor(0) >= 0) {
    }
    m_modeW = w;
    m_modeH = h;
    if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_NONE) {
        if (flag) {
            m_savedModeW = w;
            m_savedModeH = h;
        }
        CPlay* st = static_cast<CPlay*>(m_curState);
        st->ResetViewport();
        if (st->m_guts != 0) {
            st->m_guts->m_barFrameGate = h;
            if (st->m_guts->m_position == 0) {
                st->m_guts->RefreshA();
                st->m_guts->DockStatusBarRight();
            } else if (st->m_guts->m_position == 1) {
                st->m_guts->DockStatusBarRight();
                st->m_guts->RefreshA();
            }
        }
    }
    RecomputeViewScale();
    RefreshGameClock();
    if (g_resolutionChanged != 0) {
        g_resolutionChanged = 0;
        char buf[0x80];

        sprintf(buf, "Resolution is now %ix%ix%i", m_modeW, m_modeH, m_colorDepth);
        AppendChatMessage(buf);
    }
    return 1;
}

RVA(0x00093be0, 0x107)
i32 CGruntzMgr::IsBattlezMapFile(CString path) {
    CFile file;
    char hdr[0x5f4];
    if (file.Open(path, 0, 0)) {
        if (file.GetLength() < 0x5f4) {
            file.Close();
            return 0;
        }
        file.Read(hdr, 0x5f4);
        file.Close();
        if (strstr(hdr + 0x10, "Battlez")) {
            return 1;
        }
    }
    return 0;
}

VTBL(CDemo, 0x001e9f0c);
