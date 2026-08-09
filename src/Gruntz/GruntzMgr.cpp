#include <rva.h>

#include <Gruntz/GruntzMgr.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Bute/SymParser.h>
#include <Crypto/FecCrypt.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Blk6c.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/Demo.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/Fonts.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzApp.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/HeapDiag.h>
#include <Gruntz/HelpState.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LoadGameMenu.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapLogic.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/MovieId.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Play.h>
#include <Gruntz/PortalPath.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/Resolution.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SplashState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/StateMgrBZ.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WorldSoundSet.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Io/FileStream.h>
#include <Io/MoviePlayer.h>
#include <Io/SaveGame.h>
#include <Net/NetLobby.h>
#include <Net/NetMgr.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezMgr.h>
#include <Rez/RezSync.h>
#include <Utils/MapTyped.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/GameApp.h>
#include <Wap32/Object.h>
#include <Wap32/ScreenGeometry.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <dplobby.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// owner-TU unproven: bss sits in the pre-gruntzmgr window (before g_buteMgr)

DATA(0x00211054)
static char s_dataPath[] = "%c:\\DATA\\%s";

DATA(0x00211044)
static char s_fecName[] = "Gruntz.FEC";

DATA(0x00211034)
static char s_fecLoName[] = "GruntzLo.FEC";

DATA(0x00211024)
static char s_moviezPath[] = "%c:\\MOVIEZ\\%s";

DATA(0x0020c5b8)
char g_nameFmt[] = "%s";

DATA(0x002452d8)
char g_msgScratch[256];

DATA(0x002455e8)
i32 g_monologoShown;

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
// 64 B of initialized .data no instruction in the image reaches. The reloc
// table is a complete index of the absolute references, and it holds no entry
// anywhere in [0x20fa78, 0x20fab8) - so this is not a "we have not found the
// reader yet": there is no reader, and the payload carries no stride, so its
// MEANING is not recoverable and the name stays positional. What IS proven is
// that the bytes exist and belong here: the run is bounded on both sides by a
// gruntzmgr datum (g_remoteVersion below it, g_dplayAppGuid above it) and a
// .data contribution is contiguous, and every one of the 16 words is a small
// signed dword with -1 as its sentinel. Left unmodelled it is 64 bytes objdiff
// never looks at, which is the one thing a claim can fix.
DATA(0x0020fa78)
i32 g_table_20fa78[16] = {1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 8, 9, 10};
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

RVA_COMPGEN(0x00085b50, 0x56, ??1CSaveGame@@QAE@XZ)

RVA_COMPGEN(0x00085ed0, 0x4a, ??1CWorldSoundSet@@QAE@XZ)

RVA_COMPGEN(0x00085fc0, 0x57, ??1DirectInputMgr2@@QAE@XZ)

// @early-stop
RVA(0x0008b8c0, 0x76)
i32 PumpIdleFrame() {
    if (g_pendingFrame == 0) {
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    g_pendingFrame = 0;
    if (mgr == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = mgr->m_world;
    if (world == NULL) {
        return 0;
    }
    if (world->m_imageRegistry == NULL) {
        return 0;
    }
    if (mgr->m_curState == NULL) {
        return 0;
    }
    if (mgr->m_curState->InputVirtual() == 0) {
        g_gameReg->ReportError(IDX(IDS_RESTORE_GAME), 0x435);
        return 0;
    }
    g_gameReg->RefreshGameClock();
    g_pendingFrame = 1;
    return 1;
}

// @early-stop
// Two residues, and the first is NOT regalloc: retail EXPANDS the CCreditsState
// constructor into the GAMESTATE_CREDITS arm (36 instructions at 0x8bfa2 - the CState
// base ctor, two SetRect calls at 0x278e, the 0x5e9c64 vtable stamp and the field
// zeroing) where cl declines it and emits a call to ??0CCreditsState@@QAE@XZ.  It is
// already a header inline; this is the /Ob1 per-site budget
// (docs/patterns/ob1-inline-budget-divergence.md), and it accounts for 33 of the
// missing instructions on its own.  The rest is one whole-function regalloc swap -
// retail colours this=edi / stateId=ebp (it materializes the stateId argument BEFORE
// this) and keeps the `obj` join in esi, so every other `new` arm is exactly one
// `mov eax,<reg>` longer here.
RVA(0x0008b960, 0x808)
i32 CGruntzMgr::TransitionState(GameStateId stateId, i32 areaArg, i32 keepCurrent, i32 unused) {
    static_cast<void>(unused);
    GameStateId local10 = GAMESTATE_NONE;
    if (m_curState != NULL) {
        local10 = m_curState->Update();
        i32 savedSub = m_curState->m_levelIndex;
        m_curState->LeaveState(stateId);
        if (keepCurrent != 0) {
            PushState(m_curState);
            areaArg = savedSub;
            m_curState = NULL;
        } else {
            if (m_curState != NULL) {
                delete m_curState;
            }
            m_curState = NULL;
            ClearStateStack();
            m_curState = NULL;
        }
    } else if (keepCurrent == 0) {
        ClearStateStack();
    }

    if (m_delayedQuitPending != 0) {

        m_curState = new CState;
        return 1;
    }

    CState* obj = NULL;
    switch (stateId) {
        // arm order is byte-proven by the retail bodies' allocation sizes
        // (0x1c0, 0x520, 0x660, 0x528, 0x1c0, 0x1b8, 0x1bc, 0x320, 0x218, 0x244)
        case GAMESTATE_ATTRACT:
            obj = new CAttract;
            break;
        case GAMESTATE_PLAY:
            obj = new CPlay;
            break;
        case GAMESTATE_MULTI:
            obj = new CMulti;
            break;
        case GAMESTATE_DEMO:
            obj = new CDemo;
            break;
        case GAMESTATE_MENU:
            obj = new CMenuState;
            break;
        case GAMESTATE_HELP:
            obj = new CHelpState;
            break;
        case GAMESTATE_SPLASH:
            obj = new CSplashState;
            break;
        case GAMESTATE_BOOTY:
            obj = new CBootyState;
            break;
        case GAMESTATE_CREDITS:
            obj = new CCreditsState;
            break;
        case GAMESTATE_MULTIBOOTY:
            obj = new CMultiBootyState;
            break;
        default:
            break;
    }
    m_curState = obj;

    if (m_curState == NULL) {
        m_owner->m_running = 0;
        return 0;
    }
    RefreshGameClock();
    {
        CState* st = m_curState;

        i32 ok = st->LoadGameAssetNamespaces(this, areaArg, IDX(local10));
        st = m_curState;
        if (ok == 0) {
            if (st != NULL) {
                delete st;
            }
            m_curState = NULL;
            return 0;
        }
        st->EnterState(local10);
        m_owner->m_running = 1;
        g_inputMgr->ReadAll();
        RefreshGameClock();
        return 1;
    }
}

RVA_COMPGEN(0x0008c3d0, 0x1e, ??_GCRgn@@UAEPAXI@Z)
RVA_COMPGEN(0x0008c470, 0xb, ??1CState@@UAE@XZ)

RVA(0x0008c530, 0x8)
i32 CState::LeaveState(GameStateId) {
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
    CDemo::ReleaseResources();
}

RVA(0x0008d1e0, 0x6)
GameStateId CMulti::Update() {
    return GAMESTATE_MULTI;
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
    if (next > IDX(QUESTLEVEL_TRAINING_LAST)) {
        next = IDX(QUESTLEVEL_FIRST);
    }
    if (next <= IDX(QUESTLEVEL_CAMPAIGN_LAST) || next >= IDX(QUESTLEVEL_TRAINING_FIRST)) {
        st->LeaveState(st->Update());
        if ((static_cast<CPlay*>(st))->LoadByMode(next, 1)) {
            st->EnterState(st->Update());
            return 1;
        }
    }
    ReportError(IDX(IDS_CHANGE_LEVEL), 0x436);
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
        prev = IDX(QUESTLEVEL_TRAINING_LAST);
    }
    if (prev <= IDX(QUESTLEVEL_CAMPAIGN_LAST) || prev >= IDX(QUESTLEVEL_TRAINING_FIRST)) {
        st->LeaveState(st->Update());
        if ((static_cast<CPlay*>(st))->LoadByMode(prev, 1)) {
            st->EnterState(st->Update());
            return 1;
        }
    }
    ReportError(IDX(IDS_CHANGE_LEVEL), 0x437);
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
    if (list == NULL) {
        return;
    }
    POSITION pos = list->GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj) {
            obj->m_stateFlags ^= static_cast<SpriteStateFlags>(mask);
        }
    }
}

RVA(0x0008dc90, 0xb1)
void CGruntzMgr::RegisterLevelAssetKeys() {
    CDDrawSurfaceMgr* w = m_world;
    if (w == NULL) {
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

RVA(0x0008dd80, 0x31)
i32 CDDrawPtrCollections::GetCapsChecked() {
    i32 hr = m_device->GetCaps(&m_driverCaps, &m_helCaps);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(
            const_cast<char*>("c:\\proj\\incs\\ddrawmgr.h"),
            0x135,
            hr
        );
    }
    return hr;
}

RVA(0x0008ddd0, 0x7e)
i32 CGruntzMgr::RestoreVideoMode(i32 save) {
    if (m_modeSize.cx == SCREEN_W_PX && m_modeSize.cy == SCREEN_H_PX) {
        if (save) {
            m_savedModeSize = m_modeSize;
        }
        return 1;
    }
    if (!SetVideoMode(SCREEN_W_PX, SCREEN_H_PX, save)) {
        ReportError(IDX(IDS_SET_VIDEO_MODE), 0x438);
        return 0;
    }
    return 1;
}

RVA(0x0008e470, 0x50)
i32 CGruntzMgr::HandleDebugPosition() {
    i32 r = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        r = RunModalDialog("DEBUG_POSITION", WarpDialogProc, 1);
        if (r == 1) {
            HWND hwnd = m_gameWnd->m_hwnd;
            PostMessageA(hwnd, WM_COMMAND, 0x805c, 0);
        }
    }
    return r != 0;
}

RVA(0x0008f980, 0x21)
i32 CGruntzMgr::IsStandardMode() {
    if (m_modeSize.cx == SCREEN_W_PX && m_modeSize.cy == SCREEN_H_PX) {
        return 1;
    }
    return 0;
}

RVA(0x0008f9c0, 0x1d)
i32 CGruntzMgr::AppendChatMessage(char* msg) {
    CFontConfig* log = m_chatLog;
    if (log == NULL) {
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
    if (m_curState == NULL) {
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
    if (m_curState == NULL) {
        return 0;
    }
    if (m_curState->Update() == GAMESTATE_PLAY) {
        return 1;
    }
    return m_curState->Update() == GAMESTATE_MULTI;
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
        m_lobby = NULL;
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

        RecordBytes<CNetLobbyConnection> settings;
        settings.m_rec = m_connSettings;
        delete[] settings.m_bytes;
        m_connSettings = NULL;
    }

    DWORD dwSize = 0;
    hr = m_lobby->GetConnectionSettings(0, 0, &dwSize);
    if (hr != 0 && hr != static_cast<i32>(DPERR_BUFFERTOOSMALL)) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr, m_gameWnd->m_hwnd);
        m_lobby->Release();
        m_lobby = NULL;
        return 0;
    }

    RecordBytes<CNetLobbyConnection> settings;
    settings.m_bytes = new u8[dwSize];
    m_connSettings = settings.m_rec;
    if (!m_connSettings) {
        m_lobby->Release();
        m_lobby = NULL;
        return 0;
    }

    hr = m_lobby->GetConnectionSettings(0, m_connSettings, &dwSize);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1232, hr, m_gameWnd->m_hwnd);
        m_lobby->Release();
        m_lobby = NULL;
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
            // Only a 4-plane level has an object layer, and it is plane count-2:
            // any other size leaves idx == count, which the bounds check rejects.
            i32 idx = view->m_planes.GetSize();
            if (idx == LEVEL_EXTENDED_PLANE_COUNT) {
                idx -= 2;
            }
            i32 count = view->m_planes.GetSize();
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
    GameStateId st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY
        && st != GAMESTATE_DEMO) {
        return 0;
    }
    CString name = RunCustomWorldDialog(m_gameWnd->m_hwnd, 0);
    if (name.GetLength() == 0) {
        return 0;
    }
    m_strWorldFile = name;
    m_isMultiLevel = 0;
    m_isBattlezLevel = 0;
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEW_GAME), 0);
    return 1;
}

RVA(0x0008f620, 0x51)
void CGruntzMgr::RefreshGameClock() {
    if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {
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
        if (CheckPlayState() == 0
            && (m_curState == NULL || m_curState->Update() != GAMESTATE_CREDITS)) {
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

// Emit TU, wall-blocked: retail's CGruntzMgr::ChangeState calls this ctor
// out-of-line (via the inlined CMoviePlayer ctor, m_decodeStore member), and our
// ChangeState already references it as extern - but converting the body to a
// header inline makes our cl flatten it into ChangeState (caller-budget inline
// divergence, docs/patterns/msvc5-variable-ctor-inline-depth.md), losing this
// label. Dissolves into FecCrypt.h + a gruntzmgr pin when that wall breaks.
RVA(0x0008fea0, 0x6d)
CFecFile::CFecFile() {
    m_openGate = 0;
    m_readOpen = 0;
    m_writeOpen = 0;
    m_nextIndex = 0;
    srand(time(0));
}
RVA(0x0008ff30, 0x20c)
CString CGruntzMgr::BuildMoviePath(MovieId movie) {
    CString name;

    switch (movie) {
        case MOVIE_LOGO:
            name = "Logo.vob";
            break;
        case MOVIE_GRUNTZ0:
            name = "Gruntz0.vob";
            break;
        case MOVIE_GRUNTZ1:
            name = "Gruntz1.vob";
            break;
        case MOVIE_GRUNTZ2:
            name = "Gruntz2.vob";
            break;
        case MOVIE_GRUNTZ3:
            name = "Gruntz3.vob";
            break;
        case MOVIE_GRUNTZ4:
            name = "Gruntz4.vob";
            break;
        case MOVIE_GRUNTZ5:
            name = "Gruntz5.vob";
            break;
        case MOVIE_GRUNTZ6:
            name = "Gruntz6.vob";
            break;
        case MOVIE_GRUNTZ7:
            name = "Gruntz7.vob";
            break;
        case MOVIE_GRUNTZ8:
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
        return path;
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

    if ((m_modeSize.cx == m_savedModeSize.cx && m_modeSize.cy == m_savedModeSize.cy)
        || SetVideoMode(m_savedModeSize.cx, m_savedModeSize.cy, 1) || RestoreVideoMode(1)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x45e);
    return 0;
}

RVA(0x0008f480, 0x49)
i32 CGruntzMgr::ClearWorldFile() {
    GameStateId mode = m_curState->Update();
    if (mode == GAMESTATE_MENU || mode == GAMESTATE_ATTRACT || mode == GAMESTATE_PLAY) {
        m_strWorldFile.Empty();
        PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEW_GAME), 0);
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

RVA(0x00090550, 0x1e6)
i32 __stdcall LaunchPortalExe(char* outPath) {
    DWORD bufSize;
    char regBuf[0x100];
    Utils::RegistryHelper reg;

    if (!reg.Open("Monolith Productions", "Portal", "1.0", 0, HKEY_LOCAL_MACHINE, 0)) {
        return 0;
    }
    regBuf[0] = 0;
    bufSize = 0xde;
    if (!reg.GetValueString("filedir", regBuf, &bufSize, 0)) {
        return 0;
    }
    i32 len = strlen(regBuf);
    if (len < 1) {
        return 0;
    }
    if (regBuf[len - 1] != '\\') {
        strcat(regBuf, "\\");
    }
    strcat(regBuf, "portal.exe");
    if (!FileExists(regBuf)) {
        return 0;
    }
    if (outPath != NULL) {
        strcpy(outPath, regBuf);
    }
    return 1;
}

RVA(0x00090860, 0xd3)
i32 CGruntzMgr::LaunchProcessInDir(char* exe, char* dir) {
    char cmdline[256];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (dir && *dir) {
        i32 len = strlen(dir);
        if (len > 0 && dir[len - 1] == '\\') {
            wsprintfA(cmdline, "%s%s", dir, exe);
        } else {
            wsprintfA(cmdline, "%s\\%s", dir, exe);
        }
    } else {
        wsprintfA(cmdline, "%s", exe);
    }
    if (dir && *dir == 0) {
        dir = NULL;
    }
    return CreateProcessA(0, cmdline, 0, 0, 0, 0, 0, dir, &si, &pi);
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
    if (code > 0 && code <= IDX(QUESTLEVEL_POST_LAST)) {
        i32 v = (code == IDX(QUESTLEVEL_RESTART)) ? IDX(QUESTLEVEL_FIRST) : code;
        PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_WORLD), v);
    }
}

// @early-stop
RVA(0x00090ac0, 0x1cc)
void CGruntzMgr::ReportWorldStatus(WorldInitReportTag tag) {
    if (m_world == NULL) {
        ReportError(IDX(IDS_INITIALIZE_GAME), IDX(tag));
    }
    GZ_ENUM_STORAGE(WorldInitError, u32) status = m_world->m_lastError;
    if (status == WORLDERR_NONE) {
        ReportError(IDX(IDS_INITIALIZE_GAME), IDX(tag));
    }
    switch (static_cast<u32>(status)) {
        case WORLDERR_CREATE_PAGES:
            ReportError(IDX(IDS_WORLD_CREATE_PAGES), IDX(WORLDERR_CREATE_PAGES));
            return;
        case WORLDERR_SOUND_OUTPUT:
            ReportError(IDX(IDS_WORLD_SOUND_OUTPUT), IDX(WORLDERR_SOUND_OUTPUT));
            return;
        case WORLDERR_SOUND_REGISTRY:
            ReportError(IDX(IDS_WORLD_SOUND_REGISTRY), IDX(WORLDERR_SOUND_REGISTRY));
            return;
        case WORLDERR_FRONT_SURFACE:
            ReportError(IDX(IDS_WORLD_FRONT_SURFACE), IDX(WORLDERR_FRONT_SURFACE));
            return;
        case WORLDERR_BACK_SURFACE:
            ReportError(IDX(IDS_WORLD_BACK_SURFACE), IDX(WORLDERR_BACK_SURFACE));
            return;
        case WORLDERR_OVERLAY_SURFACE:
            ReportError(IDX(IDS_WORLD_OVERLAY_SURFACE), IDX(WORLDERR_OVERLAY_SURFACE));
            return;
        case WORLDERR_CREATE_DEVICE:
            ReportError(IDX(IDS_WORLD_CREATE_DEVICE), IDX(WORLDERR_CREATE_DEVICE));
            return;
        case WORLDERR_CREATE_PALETTE_SURFACE:
            ReportError(IDX(IDS_WORLD_CREATE_PALETTE_SURFACE), IDX(status));
            return;
        case WORLDERR_DDRAW_CREATE:
            ReportError(IDX(IDS_WORLD_DDRAW_CREATE), IDX(WORLDERR_DDRAW_CREATE));
            return;
        case WORLDERR_DDRAW_COOPERATIVE_LEVEL:
            ReportError(IDX(IDS_WORLD_DDRAW_COOPERATIVE_LEVEL), IDX(status));
            return;
        case WORLDERR_DDRAW_CAPABILITIES:
            ReportError(IDX(IDS_WORLD_DDRAW_CAPABILITIES), IDX(status));
            return;
        case WORLDERR_DDRAW_DISPLAY_MODE:
            ReportError(IDX(IDS_WORLD_DDRAW_DISPLAY_MODE), IDX(status));
            return;
        case WORLDERR_DDRAW_COLOR_MASKS:
            ReportError(IDX(IDS_WORLD_DDRAW_COLOR_MASKS), IDX(status));
            return;
        default:
            ReportError(IDX(IDS_WORLD_UNKNOWN), IDX(status));
            return;
    }
}

RVA(0x00090d10, 0x18e)
i32 CGruntzMgr::LoadMonologoSprite() {
    if (m_curState == NULL) {
        return 0;
    }
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    if (m_world == NULL) {
        return 0;
    }

    CDDrawWorker* rec;
    {
        CObject* out = 0;
        m_world->m_imageRegistry->m_10map.Lookup("GAME_MONOLITH", out);
        rec = static_cast<CDDrawWorker*>(out);
    }
    if (rec == NULL) {
        return 0;
    }
    i32 savedIdx = rec->m_minIndex;
    CImage* e = static_cast<CImage*>(rec->m_items.GetAt(savedIdx));
    if (e == NULL) {
        return 0;
    }
    i32 geoA = e->m_width;
    i32 geoB = e->m_height;
    CDDrawWorkerHost* found =
        static_cast<CDDrawWorkerHost*>(m_world->m_level->FindPlaneByName("MONOLITH"));
    if (found == NULL) {
        CDDrawWorkerHost* spr = m_world->m_level->ReadObjectPlane(
            0x20,
            0x20,
            geoA,
            geoB,
            -0x19,
            -0x19,
            const_cast<char*>("MONOLITH")
        );
        if (spr == NULL) {
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
                if (src != NULL) {
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
                    ReportError(IDX(IDS_SET_GAME_STATE), 0x43b);
                    return 0;
                }
                PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x80ca, 0);
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
    if (m_curState == NULL) {
        return 0;
    }
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    if (m_world == NULL) {
        return 0;
    }
    CObject* found = 0;
    m_world->m_imageRegistry->m_10map.Lookup("GAME_DEVHEADS", found);
    CDDrawWorker* out = static_cast<CDDrawWorker*>(found);
    if (out == NULL) {
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
                        case SHADE_DST_BY_SRC:
                            set->SetAllTypes(SHADE_COPY);
                            AppendChatMessage(const_cast<char*>("Back from the dead?"));
                            break;
                        default:
                            set->SetAllTypes(SHADE_DST_BY_SRC);
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
                    ShadeMode st = fmt->m_drawType;
                    if (st != SHADE_DST_BY_LEVEL) {
                        set->SetAllTypes(SHADE_DST_BY_LEVEL);
                        set->SetAllLightLevels(rand() % 256);
                        AppendChatMessage(const_cast<char*>("Me and my..."));
                    } else {
                        set->SetAllTypes(SHADE_COPY);
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

RVA(0x00091670, 0x2ac)
i32 CGruntzMgr::MakeRezPath() {
    char cwd[0x100];
    if (!GetCurrentDirectoryA(0xff, cwd)) {
        return 0;
    }

    char drive = GetGruntzDriveLetter();
    m_inGameDir = (drive == cwd[0]);

    i32 found = 1;

    CString rez("Gruntz.REZ");
    m_haveRez = false;
    m_strRezPath.Format("%s\\%s", cwd, static_cast<LPCTSTR>(rez));
    if (!FileExists(m_strRezPath)) {
        if (drive) {
            m_strRezPath.Format(s_dataPath, drive, static_cast<LPCTSTR>(rez));
            if (FileExists(m_strRezPath)) {
                m_haveRez = true;
            } else {
                found = 0;
            }
        } else {
            found = 0;
        }
    }

    i32 movFound = 1;
    CString fecHi(s_fecName);
    CString fecLo(s_fecLoName);
    // Retail selects Gruntz.FEC when HQ movies are DISABLED and GruntzLo.FEC
    // when they are enabled - inverted, but that is what 0x91779 branches on.
    CString fec(g_disableHqMovie ? fecHi : fecLo);

    m_haveMoviez = false;
    m_strMoviePath.Format("%s\\%s", cwd, static_cast<LPCTSTR>(fecHi));
    if (!m_inGameDir && !FileExists(m_strMoviePath)) {
        movFound = 0;
        if (!g_disableHqMovie) {
            m_strMoviePath.Format("%s\\%s", cwd, static_cast<LPCTSTR>(fecLo));
            if (FileExists(m_strMoviePath)) {
                movFound = 1;
            }
        }
    }
    if (!movFound && drive) {
        m_strMoviePath.Format(s_moviezPath, drive, static_cast<LPCTSTR>(fec));
        if (FileExists(m_strMoviePath)) {
            m_haveMoviez = true;
        }
    }

    if (!found) {
        ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x43e);
        return 0;
    }
    return 1;
}

RVA(0x00092180, 0x98)
i32 CGruntzMgr::ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, ScanCb cb, i32 user) {
    if (cb == NULL) {
        return 0;
    }
    i32 r2 = radius * radius;
    i32 count = 0;
    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != NULL) {
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
    if (cb == NULL) {
        return 0;
    }
    RECT* r = rect;
    if (r == NULL) {
        return 0;
    }
    i32 loX = r->left + offX;
    i32 hiX = r->right + offX;
    i32 loY = r->top + offY;
    i32 hiY = r->bottom + offY;
    i32 count = 0;
    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != NULL) {
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

RVA(0x00091170, 0xad)
i32 CGruntzMgr::SetColorDepth(ColorDepth depth) {
    if (depth != BPP_PALETTED_8 && depth != BPP_RGB_16 && depth != BPP_RGB_24) {
        return 0;
    }
    if (m_world == NULL) {
        return 0;
    }
    switch (depth) {
        case BPP_RGB_24:
            g_surfaceColorKey = 0xff0084;
            return 1;

        case BPP_RGB_16: {
            i32 packed = static_cast<u16>(((0xff >> g_rDown) << g_rUp));
            packed |= static_cast<u16>(((0 >> g_gDown) << g_gUp));
            packed |= static_cast<u16>((0x84 >> g_bDown));
            g_surfaceColorKey = packed;
            return 1;
        }

        case BPP_PALETTED_8:
            g_surfaceColorKey = 0;
            return 1;
    }
    return 1;
}

// @early-stop
RVA(0x00091a40, 0x2f9)
i32 CGruntzMgr::LoadWorldMode(ColorDepth mode) {
    if (m_world == NULL) {
        return 0;
    }
    if (m_colorDepth == mode) {
        return 1;
    }
    if (mode != BPP_PALETTED_8 && mode != BPP_RGB_16) {
        return 0;
    }

    if (m_inputState != NULL) {
        delete m_inputState;
        m_inputState = NULL;
    }

    CSymParser* surf = m_symParser;
    if (surf) {
        delete surf;
    }
    m_symParser = NULL;

    m_colorDepth = mode;
    g_enableTrueColor = 0;
    g_enableHiColor = 0;
    if (m_colorDepth == BPP_RGB_16) {
        g_enableHiColor = 1;
    }

    m_world->Cleanup();
    i32 kind = 1;
    if (g_disableAudio != 0) {
        kind = 5;
    }
    if (m_world->Init(m_gameWnd->m_hwnd, SCREEN_W_PX, SCREEN_H_PX, m_colorDepth, kind) == 0) {
        ReportWorldStatus(WORLD_REPORT_COLOR_DEPTH_REINIT);
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
        m_symParser = NULL;
    }

    m_symParser = new CSymParser;

    // The argument is spelled on the returned temporary, not on a named CString:
    // retail reads the buffer through the return register (`mov ecx,[eax]`), which a
    // named local turns into a direct frame-slot load.
    bool parseFailed =
        m_symParser->ParseBuffer(const_cast<char*>(static_cast<const char*>(GetRezPath())), 1, 0)
        == 0;
    if (parseFailed) {
        ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x441);
        return 0;
    }

    SetColorDepth(m_colorDepth);

    if (m_inputState != NULL) {
        delete m_inputState;
        m_inputState = NULL;
    }

    CWorldSoundSet* ni = new CWorldSoundSet();
    m_inputState = ni;
    if (ni->Init(m_world->m_soundRegistry, m_soundVolume) == 0) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x442);
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
    if (st == NULL) {
        return 1;
    }
    GameStateId stateId = st->Update();
    if (stateId != GAMESTATE_MENU && stateId != GAMESTATE_ATTRACT) {
        return 1;
    }

    CState* s = m_curState;
    m_modalBusy = 1;
    m_renderGate = 1;
    if (s) {
        delete s;
        m_curState = NULL;
    }

    int(WINAPI * show)(BOOL) = ShowCursor;
    while (show(1) < 0) {
    }

    CWaitCursor waitCursor;

    if (m_colorDepth == BPP_PALETTED_8) {
        if (LoadWorldMode(BPP_RGB_16) == 0) {
            ReportError(IDX(IDS_CHANGE_COLOR_DEPTH), 0x443);
            return 0;
        }
    } else {
        if (LoadWorldMode(BPP_PALETTED_8) == 0) {
            ReportError(IDX(IDS_CHANGE_COLOR_DEPTH), 0x444);
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
    if (path == NULL) {
        return 0;
    }
    CAssetRootStorage::s_value = path;
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_SHOW_STATE0), 0);
    return 1;
}

RVA(0x000920e0, 0x32)
i32 CGruntzMgr::PostSlotCommandB1(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x80b1, slot);
    return 1;
}

RVA(0x00092130, 0x32)
i32 CGruntzMgr::PostSlotCommandB6(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x80b6, slot);
    return 1;
}

RVA(0x00092a30, 0x52)
INT_PTR CALLBACK PsycheDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            return 1;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x00091500, 0x42)
i32 CGruntzMgr::IsLobbyHostReady() {
    if (m_curState == NULL) {
        return 0;
    }
    CGameApp* app = m_owner;
    if (app == NULL) {
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
    if (m_sound == NULL) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != NULL) {
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
    if (m_sound == NULL) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != NULL) {
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
    if (m_world == NULL) {
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
CState* CGruntzMgr::FindStateById(GameStateId id) {
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
    return static_cast<CPlay*>(FindStateById(GAMESTATE_PLAY));
}

RVA(0x000929b0, 0x19)
CState* CGruntzMgr::PickPausedThenPlayState() {
    CState* s = FindStateById(GAMESTATE_MULTI);
    if (s) {
        return s;
    }
    return FindStateById(GAMESTATE_PLAY);
}

RVA(0x000923b0, 0x47)
void CGruntzMgr::SetSoundLevelState(i32 loaded) {
    if (loaded == m_musicEnabled) {
        return;
    }
    m_musicEnabled = loaded;
    CGruntzSoundZ* snd = m_sound;
    if (snd == NULL) {
        return;
    }
    if (loaded != 0) {
        CGruntzSoundInnerZ* cur = snd->m_pCurrent;
        if (cur == NULL) {
            return;
        }
        if (cur->m_playMode != 0) {
            snd->Restart(1);
        } else if (snd->m_pCurrent != NULL) {

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
    if (m_saveSink == NULL) {
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
    if (m_saveInfoRec == NULL || !(m_saveInfoRec->m_flags & 1)) {
        return LoadSaveMessageSprite();
    }

    if (&(static_cast<CPlay*>(m_curState))->m_saveSlot == NULL) {
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
    if (m_saveSink == NULL) {
        return 0;
    }
    if (m_cueSink) {
        m_cueSink->PauseAllVoices();
    }
    if (m_saveInfoRec && (m_saveInfoRec->m_flags & 1)) {

        if (m_saveSink->VerifySlot(m_saveInfoRec) == 0) {
            return 1;
        }
        PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_SAVED_GAME), 0);
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
    if (s == NULL) {
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
        if (s != NULL) {
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
// /Ob1 inline-budget divergence (docs/patterns/ob1-inline-budget-divergence.md):
// retail CALLS ??1CFecFile (0x390a0) and the CArray<PLAYLISTINFOSTRUCT*> ctor/dtor
// COMDATs here; cl expands all three (Teardown + vptr stamp + operator delete,
// Close + ~CDWordArray + ~CFile). One authentic inline definition stays.
// Re-audited 2026-08-09 with the one-level-up xref rule: `sema xref 0x00038fc0` gives
// ??1CMoviePlayer exactly ONE caller, CCreditsState::ReleaseResources' `delete vh`,
// so retail EXPANDS the movie-player dtor here and only the ??1CFecFile inside it is
// a call - i.e. one shape per entity, no per-class split to model.  Declaring
// ~CMoviePlayer out of line in CreditsState.cpp (its 0xa5 COMDAT stays 100.00 either
// way) does remove our expansion but scores 74.88 -> 70.54; reverted.
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
        if (snd == NULL) {
            player.Teardown();
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(snd), "GAME", "_");
    }
    if (front == NULL || dd2 == NULL) {
        player.Teardown();
        return 0;
    }

    IDirectSound* dsound = m_world->m_soundStream ? m_world->m_soundStream->m_device : 0;
    if (player.InitMode(m_gameWnd->m_hwnd, dd2, front->m_ddSurface, front->m_apiDesc, dsound)) {
        if (player.Open(m_strMoviePath, arg, MOVIE_TILE, m_isInterlaced != 0, 0, 0)) {
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
    return m_gameWnd != NULL;
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
i32 CGameMgr::HandleCommand(i32, GruntzCommandId, i32) {
    return 0;
}

// @early-stop
RVA(0x0008f7f0, 0x131)
void CGruntzMgr::RecomputeViewScale() {
    if (m_world == NULL) {
        return;
    }
    CGameLevel* view = m_world->m_level;
    float fw = static_cast<float>((view->m_planeCtx.right - view->m_planeCtx.left + 1));
    float fh = static_cast<float>((view->m_planeCtx.bottom - view->m_planeCtx.top + 1));

    view->m_rectA.w = static_cast<i32>((fw * DATA_COMPGEN(0x001ea2bc, fp_1ea2bc, 1.4f)));
    view->m_rectA.h = static_cast<i32>((fh * 1.4f));
    view->MainPlaneNotify();

    view = m_world->m_level;
    view->m_rectB.w = static_cast<i32>((fw * DATA_COMPGEN(0x001ea2c0, fp_1ea2c0, 5.3f)));
    view->m_rectB.h = static_cast<i32>((fh * 5.3f));
    view->MainPlaneNotify();

    view = m_world->m_level;
    view->m_rectC.w = static_cast<i32>((fw * DATA_COMPGEN(0x001ea2c4, fp_1ea2c4, 1.12f)));
    view->m_rectC.h = static_cast<i32>((fh * 1.12f));
    view->MainPlaneNotify();

    CGameLevel* v = m_world->m_level;
    if (v->m_mainPlane == NULL) {
        return;
    }
    m_viewBounds.left = (v->m_mainPlane)->m_viewRect.left - 0x60;
    m_viewBounds.top = (m_world->m_level->m_mainPlane)->m_viewRect.top - 0x60;
    m_viewBounds.right = (m_world->m_level->m_mainPlane)->m_viewRect.right + 0x60;
    m_viewBounds.bottom = (m_world->m_level->m_mainPlane)->m_viewRect.bottom + 0x60;
}

RVA(0x00093460, 0x15c)
i32 CGruntzMgr::BroadcastCmd(CFileMemBase* ar, SerialMode cmd, LogicTypeId typeId, i32 pObj) {
    if (ar == NULL) {
        return 0;
    }
    switch (cmd) {
        case SERIAL_SAVE:

            if (SaveState(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (LoadState(ar) == 0) {
                return 0;
            }
            m_cueSink->ClearSprites();
            break;
    }

    i32 i;
    GruntzPlayer* slot;
    for (i = 0, slot = m_options; i < 4; i++) {
        if (slot == NULL || slot->Serialize(ar, cmd, typeId, pObj) == 0) {
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
    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
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
        g_gameReg->m_saveSink->SetCurLevel(static_cast<QuestLevel>(sub->m_levelIndex));
        g_gameReg->m_saveSink->SetMaxLevel(
            static_cast<QuestLevel>((sub->m_levelIndex % IDX(QUESTLEVEL_TRAINING_LAST)) + 1)
        );
        g_gameReg->m_saveSink->Save(0, 0x81a6);
    }
    m_scoreHud->SetCount(sub->m_levelIndex);
    m_scoreHud->m_isCustomLevel = 0;
}

RVA(0x00093620, 0x254)
i32 CGruntzMgr::SaveState(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (m_world == NULL) {
        return 0;
    }
    g_serialCounter++;

    char buf[SERIAL_NAME_LEN];
    memset(buf, 0, SERIAL_NAME_LEN);
    strcpy(buf, m_strWorldFile);
    ar->Write(buf, SERIAL_NAME_LEN);

    ar->Write(&m_loadingSaveGame, sizeof(m_loadingSaveGame));
    ar->Write(&m_soundVolume, sizeof(m_soundVolume));
    ar->Write(&m_isBattlezLevel, sizeof(m_isBattlezLevel));
    ar->Write(&m_isMultiLevel, sizeof(m_isMultiLevel));
    ar->Write(&m_isCustomLevel, sizeof(m_isCustomLevel));
    ar->Write(&m_gameMode, sizeof(m_gameMode));
    ar->Write(&m_optionsCount, sizeof(m_optionsCount));
    ar->Write(&m_viewBounds.left, 0x10);
    ar->Write(&g_lastNow, sizeof(g_lastNow));
    ar->Write(&g_frameDelta, sizeof(g_frameDelta));
    ar->Write(&g_frameTime, sizeof(g_frameTime));
    ar->Write(&g_frameTicks, sizeof(g_frameTicks));
    ar->Write(&g_timer32, sizeof(g_timer32));
    ar->Write(&g_timer100, sizeof(g_timer100));
    ar->Write(&g_timer200, sizeof(g_timer200));
    ar->Write(&g_timer400, sizeof(g_timer400));
    ar->Write(&g_timer500, sizeof(g_timer500));
    ar->Write(&g_traitorMode, sizeof(g_traitorMode));
    ar->Write(&g_gruntCreation, sizeof(g_gruntCreation));
    ar->Write(&g_gruntDestruction, sizeof(g_gruntDestruction));
    ar->Write(&g_gooPuddlez, sizeof(g_gooPuddlez));
    ar->Write(&g_explosionz, sizeof(g_explosionz));
    ar->Write(&m_isEasyMode, sizeof(m_isEasyMode));
    ar->Write(&g_monologoShown, sizeof(g_monologoShown));
    ar->Write(&g_jitterX, sizeof(g_jitterX));
    ar->Write(&g_jitterY, sizeof(g_jitterY));
    ar->Write(&g_panMinX, sizeof(g_panMinX));
    ar->Write(&g_panMaxX, sizeof(g_panMaxX));
    ar->Write(&g_warpX, sizeof(g_warpX));
    ar->Write(&g_warpY, sizeof(g_warpY));
    return 1;
}

RVA(0x00093920, 0x22f)
i32 CGruntzMgr::LoadState(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    g_serialCounter++;

    char buf[SERIAL_NAME_LEN];
    ar->Read(buf, SERIAL_NAME_LEN);
    m_strWorldFile = buf;

    ar->Read(&m_loadingSaveGame, sizeof(m_loadingSaveGame));
    ar->Read(&m_soundVolume, sizeof(m_soundVolume));
    ar->Read(&m_isBattlezLevel, sizeof(m_isBattlezLevel));
    ar->Read(&m_isMultiLevel, sizeof(m_isMultiLevel));
    ar->Read(&m_isCustomLevel, sizeof(m_isCustomLevel));
    ar->Read(&m_gameMode, sizeof(m_gameMode));
    ar->Read(&m_optionsCount, sizeof(m_optionsCount));
    ar->Read(&m_viewBounds.left, 0x10);
    ar->Read(&g_lastNow, sizeof(g_lastNow));
    ar->Read(&g_frameDelta, sizeof(g_frameDelta));
    ar->Read(&g_frameTime, sizeof(g_frameTime));
    ar->Read(&g_frameTicks, sizeof(g_frameTicks));
    ar->Read(&g_timer32, sizeof(g_timer32));
    ar->Read(&g_timer100, sizeof(g_timer100));
    ar->Read(&g_timer200, sizeof(g_timer200));
    ar->Read(&g_timer400, sizeof(g_timer400));
    ar->Read(&g_timer500, sizeof(g_timer500));
    ar->Read(&g_traitorMode, sizeof(g_traitorMode));
    ar->Read(&g_gruntCreation, sizeof(g_gruntCreation));
    ar->Read(&g_gruntDestruction, sizeof(g_gruntDestruction));
    ar->Read(&g_gooPuddlez, sizeof(g_gooPuddlez));
    ar->Read(&g_explosionz, sizeof(g_explosionz));
    ar->Read(&m_isEasyMode, sizeof(m_isEasyMode));
    ar->Read(&g_monologoShown, sizeof(g_monologoShown));
    ar->Read(&g_jitterX, sizeof(g_jitterX));
    ar->Read(&g_jitterY, sizeof(g_jitterY));
    ar->Read(&g_panMinX, sizeof(g_panMinX));
    ar->Read(&g_panMaxX, sizeof(g_panMaxX));
    ar->Read(&g_warpX, sizeof(g_warpX));
    ar->Read(&g_warpY, sizeof(g_warpY));
    return 1;
}

RVA(0x000927b0, 0xc4)
i32 CGruntzMgr::FillSaveInfo(SaveSlot* dst, void* snapshot) {
    if (dst == NULL) {
        return 0;
    }
    CPlay* src = PickPlayOrPausedState();
    if (src == NULL) {
        return 0;
    }

    strcpy(dst->m_levelName, GetWorldFileName());
    dst->m_isWon = (m_gameMode == GAMEMODE_REPLAY);
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
    if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {

        i32 done = 0;
        CNetCmdSlot* s = static_cast<CMulti*>(m_curState)->m_session->m_slots;
        for (i32 d = 4; d != 0; d--) {
            if (s != NULL && s->m_state == NETSLOT_ACTIVE) {
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

RVA(0x0008ef10, 0x9e)
void CGruntzMgr::EnterModalUI(const char* msg) {
    CGameApp* app = m_owner;
    if (app == NULL) {
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

    int(WINAPI * show)(BOOL) = ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    static_cast<CGruntzApp*>(app)->ShowMessage(msg, m_gameWnd->m_hwnd);
    NetLobby::g_curDlg = NULL;
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

    int(WINAPI * show)(BOOL) = ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    i32 result = dlg->DoModal();
    NetLobby::g_curDlg = NULL;
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
    if (next == NULL) {
        return 0;
    }
    if (m_curState == next) {
        return 0;
    }
    GameStateId oldId = GAMESTATE_NONE;
    if (m_curState) {
        oldId = m_curState->Update();
        m_curState->LeaveState(next->Update());
        if (m_curState) {
            delete m_curState;
        }
        m_curState = NULL;
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
    if (m_curState->Update() == GAMESTATE_MULTI) {
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
    return TransitionState(GAMESTATE_PLAY, areaArg, 0, 0);
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
        if (p != NULL) {
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
        m_settings->SetValueDword("Num Runs", m_numRuns);
        m_settings->SetValueDword("Num Movies", m_numMovies);
        m_settings->SetValueDword("Sound", m_soundEnabled);
        m_settings->SetValueDword("Voice", m_isVoiceEnabled);
        m_settings->SetValueDword("Ambient", m_isAmbientEnabled);
        m_settings->SetValueDword("Music", m_musicEnabled);
        m_settings->SetValueDword("Interlaced", m_isInterlaced);
        m_settings->SetValueDword("High Detail", m_isHighDetail);
        m_settings->SetValueDword("Effects", m_isEffectsEnabled);
        m_settings->SetValueDword("Disable Joystick", g_disableJoystick);
        if (m_sound) {
            m_settings->SetValueDword("Music Volume", m_sound->GetXMidiVolume());
        }
        if (m_cueSink) {
            m_settings->SetValueDword("Voice Volume", m_cueSink->m_voiceVolume);
        }
        if (m_world && m_world->m_soundRegistry) {
            m_settings->SetValueDword("Sound Volume", g_sndCueTag);
        }
        m_settings->SetValueDword("Scroll Speed", m_scrollSpeed);
        m_settings->SetValueDword("Easy Mode", m_isEasyMode);
        Resolution res = RES_640X480;
        if (m_savedModeSize.cx == DISPLAY_WIDTH_1024 && m_savedModeSize.cy == DISPLAY_HEIGHT_768) {
            res = RES_1024X768;
        } else if (m_savedModeSize.cx == DISPLAY_WIDTH_800
                   && m_savedModeSize.cy == DISPLAY_HEIGHT_600) {
            res = RES_800X600;
        }
        m_settings->SetValueDword("Resolution", IDX(res));
        m_settings->SetValueDword("Checkpoint Prompts", m_isCheckpointPrompts);
        m_settings->SetValueDword("Enable HiColor", m_colorDepth == BPP_RGB_16 ? 1 : 0);
        m_settings->SetValueDword("Enable TrueColor", 0);
    }
    ClearStateStack();
    if (m_curState) {
        delete m_curState;
        m_curState = NULL;
    }
    if (m_spriteFactory) {
        m_spriteFactory->Reset();
        operator delete(m_spriteFactory);
        m_spriteFactory = NULL;
    }
    if (m_cmdGrid) {
        delete m_cmdGrid;
        m_cmdGrid = NULL;
    }
    if (m_tileGrid) {

        delete m_tileGrid;
        m_tileGrid = NULL;
    }
    CBattlezData* scoreHud = m_scoreHud;
    if (scoreHud) {
        delete scoreHud;
        m_scoreHud = NULL;
    }
    if (m_cmdSubMgr) {

        delete m_cmdSubMgr;
        m_cmdSubMgr = NULL;
    }
    if (g_spawnConfig) {
        StateMgrBZ* v = g_spawnConfig;
        v->m_device = NULL;
        v->m_keyboard = NULL;
        v->m_joystick = NULL;
        v->m_mouse = NULL;
        v->m_deviceList = NULL;
        v->m_mode = INPUTDEV_NONE;
        operator delete(v);
        g_spawnConfig = NULL;
    }
    if (g_inputMgr) {

        delete g_inputMgr;
        g_inputMgr = NULL;
    }
    if (m_cheatMgr) {
        delete m_cheatMgr;
        m_cheatMgr = NULL;
    }
    if (m_sound) {
        m_sound->Shutdown();
        operator delete(m_sound);
        m_sound = NULL;
    }
    if (m_inputState) {
        delete m_inputState;
        m_inputState = NULL;
    }
    if (m_faderMgr) {
        delete m_faderMgr;
        m_faderMgr = NULL;
    }
    if (m_chatLog) {

        m_chatLog->~CFontConfig();
        operator delete(m_chatLog);
        m_chatLog = NULL;
    }
    if (m_cueSink) {
        m_cueSink->~CGruntSpawnConfig();
        operator delete(m_cueSink);
        m_cueSink = NULL;
    }
    if (m_world) {
        delete m_world;
        m_world = NULL;
    }
    if (m_symParser) {
        delete m_symParser;
        m_symParser = NULL;
    }
    if (m_settings) {

        delete m_settings;
        m_settings = NULL;
    }
    if (m_reserved3c) {
        delete m_reserved3c;
        m_reserved3c = NULL;
    }
    if (m_shadeCache) {
        delete m_shadeCache;
        m_shadeCache = NULL;
    }
    if (m_saveSink) {

        delete m_saveSink;
        m_saveSink = NULL;
    }
    if (m_logicPump) {
        m_logicPump->Reset();
        operator delete(m_logicPump);
        m_logicPump = NULL;
    }
    CloseSoundFontDevice();
    if (m_lobby) {
        m_lobby->Release();
        m_lobby = NULL;
    }
    if (m_connSettings) {
        RecordBytes<CNetLobbyConnection> settings;
        settings.m_rec = m_connSettings;
        delete[] settings.m_bytes;
        m_connSettings = NULL;
    }
    this->CGameMgr::Close();
    g_gameReg = NULL;
}

RVA(0x000861e0, 0xc5)
void CGruntzMgr::AccrueScoreTime() {
    CState* st = m_curState;
    if (m_gameMode == GAMEMODE_SINGLE) {
        if (m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
            UpdateScoreHud();
        }
        TransitionState(GAMESTATE_BOOTY, 1, 0, 0);
        return;
    }
    g_gameReg->m_scoreHud->SetCount(st->m_levelIndex);
    if (m_gameMode == GAMEMODE_REPLAY) {

        CTimer* clk = (static_cast<CPlay*>(st))->m_frameMarker;
        i64 d = static_cast<i64>(g_frameTime) - clk->m_startStamp.m_v;
        g_gameReg->m_scoreHud->m_elapsedTimeMs += (d < 0) ? 0 : static_cast<i32>(d);
        TransitionState(GAMESTATE_MULTIBOOTY, 1, 0, 0);
        return;
    }
    CBattlezData* hud = g_gameReg->m_scoreHud;
    u32 now = timeGetTime();
    hud->m_elapsedTimeMs += (now - g_scoreTimeBase);
    TransitionState(GAMESTATE_MULTIBOOTY, 1, 0, 0);
}

RVA(0x0008e6c0, 0x85)
void CGruntzMgr::OnCheckpointReached() {
    if (m_isCheckpointPrompts == 0) {
        return;
    }
    CCheckpointDlg dlg(0);
    if (ExitModalUI(&dlg, 0) == 1) {
        SendMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_QUICK_SAVE_PROMPT), 0);
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
    if (out != NULL) {
        out = NULL;
        MapLookup(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE", out);
        base = out->m_sound->m_durationMs + 0x1f4;
    } else {
        base = 0;
    }
    base += timeGetTime();
    u32 deadline = base;
    while (timeGetTime() < deadline) {
    }
    if (m_owner) {
        m_owner->m_running = 0;
    }
    if (m_gameWnd) {
        PostMessageA(m_gameWnd->m_hwnd, WM_CLOSE, 0, 0);
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
    if (tmpl == NULL) {
        return 0;
    }
    if (dlgProc == NULL) {
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

    int(WINAPI * show)(BOOL) = ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    i32 result = DialogBoxParamA(
        m_owner->m_hInstance,
        tmpl,
        m_gameWnd->m_hwnd,
        static_cast<DLGPROC>(dlgProc),
        0
    );
    NetLobby::g_curDlg = NULL;
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
    GameStateId st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY
        && st != GAMESTATE_DEMO) {
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
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEW_GAME_REPLAY), 0);
    return 1;
}

RVA(0x00083030, 0x1b6)
CGruntzMgr::CGruntzMgr() {
    m_curState = NULL;
    m_world = NULL;
    m_symParser = NULL;
    m_settings = NULL;
    m_scoreHud = NULL;
    m_reserved3c = NULL;
    m_faderMgr = NULL;
    m_cheatMgr = NULL;
    m_sound = NULL;
    m_reserved4c = 0;
    m_shadeCache = NULL;
    m_reserved64 = 0;
    m_lobby = NULL;
    m_inputState = NULL;
    m_saveSink = NULL;
    m_chatLog = NULL;
    m_cueSink = NULL;
    m_cmdGrid = NULL;
    m_cmdSubMgr = NULL;
    m_tileGrid = NULL;
    m_spriteFactory = NULL;
    m_logicPump = NULL;
    m_lobbyResult = 0;
    m_lobbyProbed = 0;
    m_delayedQuitPending = 0;
    m_reserveda8 = 0;
    m_modalBusy = 0;
    m_renderGate = 0;
    m_reservedb4 = 0;
    m_loadingSaveGame = 0;
    m_isCheckpointPrompts = 1;
    m_connSettings = NULL;
    m_saveInfoRec = NULL;
    m_numRuns = 0;
    m_numMovies = 0;
    m_reservedcc = 0x1e;
    m_modeSize.cx = 0;
    m_modeSize.cy = 0;
    m_colorDepth = BPP_RGB_16;
    m_inGameDir = 1;
    m_haveRez = false;
    m_haveMoviez = false;
    m_musicEnabled = 1;
    m_soundEnabled = 1;
    m_isVoiceEnabled = 1;
    m_isAmbientEnabled = 1;
    m_isInterlaced = 0;
    m_isEasyMode = 0;
    m_isCustomLevel = 0;
    m_isBattlezLevel = 0;
    m_isMultiLevel = 0;
    m_gameMode = GAMEMODE_NONE;
    m_isHighDetail = 1;
    m_isEffectsEnabled = 1;
    m_optionsCount = 3;
}

RVA(0x0008e1d0, 0xa5)
i32 CGruntzMgr::CheckDisplayBoundsA() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_MULTI) {
        return 1;
    }
    CDdModePair pt;
    pt = m_world->m_ptrColl->FindFwd(m_modeSize.cx, m_modeSize.cy, m_colorDepth);
    i32 x = pt.a;
    i32 y = pt.b;
    if (x > 0x514 || x == -1 || y == -1) {
        return 1;
    }
    if (SetVideoMode(x, y, 1)) {
        return 1;
    }
    if (SetVideoMode(SCREEN_W_PX, SCREEN_H_PX, 1)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x439);
    return 0;
}

RVA(0x0008e2b0, 0xb1)
i32 CGruntzMgr::CheckDisplayBoundsB() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_MULTI) {
        return 1;
    }
    CDdModePair pt;
    pt = m_world->m_ptrColl->FindBack(m_modeSize.cx, m_modeSize.cy, m_colorDepth);
    i32 x = pt.a;
    i32 y = pt.b;
    if (x == -1 || y == -1 || x < SCREEN_HALF_W_PX || y < 0xc8) {
        return 1;
    }
    if (SetVideoMode(x, y, 1)) {
        return 1;
    }
    if (SetVideoMode(SCREEN_W_PX, SCREEN_H_PX, 1)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x43a);
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
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
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
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
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
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0008df00, 0x238)
i32 CGruntzMgr::SetVideoMode(i32 w, i32 h, i32 flag) {
    if (w == m_modeSize.cx && h == m_modeSize.cy) {
        return 1;
    }
    if (m_world == NULL) {
        return 0;
    }
    if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MULTI) {
        if (m_world->m_level != NULL) {
            CDDrawWorkerHost* f = m_world->m_level->m_mainPlane;
            if (f != NULL) {
                if (w > f->m_wrapW || h > f->m_wrapH) {
                    CPlay* st = static_cast<CPlay*>(m_curState);
                    st->ResetViewport();
                    if (st->m_guts != NULL) {
                        st->m_guts->m_barFrameGate = m_modeSize.cy;
                        if (st->m_guts->m_position == STATUSBAR_DOCK_RIGHT) {
                            st->m_guts->RefreshA();
                            st->m_guts->DockStatusBarRight();
                            EnterModalUI(
                                "This map is too small to be displayed under your "
                                "desired video resolution. Default resolution will "
                                "be used."
                            );
                            return 0;
                        }
                        if (st->m_guts->m_position == STATUSBAR_DOCK_LEFT) {
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
    while (ShowCursor(0) >= 0) {
    }
    m_modeSize.cx = w;
    m_modeSize.cy = h;
    if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MULTI) {
        if (flag) {
            m_savedModeSize.cx = w;
            m_savedModeSize.cy = h;
        }
        CPlay* st = static_cast<CPlay*>(m_curState);
        st->ResetViewport();
        if (st->m_guts != NULL) {
            st->m_guts->m_barFrameGate = h;
            if (st->m_guts->m_position == STATUSBAR_DOCK_RIGHT) {
                st->m_guts->RefreshA();
                st->m_guts->DockStatusBarRight();
            } else if (st->m_guts->m_position == STATUSBAR_DOCK_LEFT) {
                st->m_guts->DockStatusBarRight();
                st->m_guts->RefreshA();
            }
        }
    }
    RecomputeViewScale();
    RefreshGameClock();
    if (g_resolutionChanged != 0) {
        g_resolutionChanged = 0;
        char buf[SERIAL_NAME_LEN];

        sprintf(buf, "Resolution is now %ix%ix%i", m_modeSize.cx, m_modeSize.cy, m_colorDepth);
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
