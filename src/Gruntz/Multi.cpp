#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <ddraw.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Net/LobbyDialogs.h>
#include <rva.h>
#include <MsgParam.h>
#include <Gruntz/CurPlayer.h>
#include <Rez/FrameClock.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Wwd/WwdFile.h>
#include <Io/FileStream.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/SoundStream.h>
#include <Gruntz/SoundCue.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/GruntzSoundZ.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/Multi.h>
#include <Net/NetLobby.h>
#include <Net/NetMgr.h>
#include <Gruntz/Attract.h>

#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Wap32/EngStr.h>
#include <stdio.h>
#include <stdlib.h>

#include <Utils/DebugTiming.h>
#include <Net/NetMgrReportError.h>
#include <Gruntz/SoundState.h>
#include <Net/InterfaceObject.h>
#include <Gruntz/LeafCue.h>
#include <Bute/SymParser.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Utils/RegistryHelper.h>
#include <Gruntz/StatusBarMgr.h>
#include <Net/NetMgr.h>
#include <Net/NetPackets.h>
#include <rva.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/SoundCue.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/Play.h>
VTBL(CNetMgr, 0x001ea42c);
DATA(0x002455fc)
i32 g_optionsCursor = 0;

typedef enum NetServiceId {
    NETSERVICE_NONE = 999,
} NetServiceId;

DATA(0x00211d88)
i32 g_dropPlayerId = -999;
DATA(0x00211d8c)

i32 g_serviceId = NETSERVICE_NONE;
DATA(0x00211ec4)
char s_GameKey[] = "GAME_KEY";
DATA(0x00246378)
u8 g_chanStat423_flag;
DATA(0x0024637c)
i32 g_chanStat423_id;
DATA(0x00246380)
i32 g_chanStat423_val;
DATA(0x00246fd8)
u8 g_chanStat422_flag;
DATA(0x00246fdc)
i32 g_chanStat422_id;
DATA(0x00246fe0)
i32 g_chanStat422_val;
DATA(0x00248ce0)
HWND g_sharedFlag = 0;
DATA(0x00248d04)
i32 g_pauseGuard;
DATA(0x00248d08)
i32 g_optionzGuard;
DATA(0x00248d0c)
i32 g_syncToggle;
DATA(0x00248d10)
i32 g_dropGuard;
DATA(0x00248d14)
u32 g_ackThrottleDeadline;

enum {
    DISPATCH_RESYNC = 0x4cc,
    DISPATCH_RESET = 0x4cd,
    DISPATCH_ABORT = 0x4ce,
    DISPATCH_PLAYERLEFT = 0x4ea,
};

enum {
    STAT_DROP_ANNOUNCE = 0x3e8,
    STAT_CHAT = 0x3f0,
    STAT_CHANNEL_TABLE = 0x3f8,
    STAT_PLAYER_JOINED = 0x3f9,
    STAT_CHANNEL_ONE = 0x3fa,
    STAT_CHANNEL_LEFT = 0x3fb,
    STAT_PAUSE = 0x407,
    STAT_PLAYERLEFT = 0x410,
    STAT_PLAYERLEFT_LOCAL = 0x411,
    STAT_CONNECTING = 0x415,
    STAT_CONFIG = 0x416,
    STAT_VERSIONPACKET = 0x417,
    STAT_VERSIONMISMATCH = 0x418,
    STAT_ACKLATENCY = 0x421,
};

DATA(0x00248cec)
i32 g_activePlayerCount = 0;

DATA(0x002473d8)
CString g_sessionName;

DATA(0x00248d00)
HWND g_netPlayerListHwnd;

DATA(0x00248cf0)
i32 g_hostServicesMode;

DATA(0x00248cf4)
CNetMgr* g_groupEnumMgr;

enum {
    STAT_VERIFY_REQUEST = 0x41c,
    STAT_VERIFY_AGREE = 0x41d,
    STAT_VERIFY_DISAGREE = 0x41e,
};

// @early-stop
RVA(0x0008d270, 0x124)
CMulti::~CMulti() {

    CMulti::ReleaseResources();

    m_604.~CDWordArray();
    m_5b8.~CString();
    m_5b4.~CString();
    m_hostName.~CString();
    m_groupName.~CString();
    m_598.~CString();
}

DATA(0x00246778)
CFile g_obj646778;

// @early-stop
RVA(0x000b5460, 0x914)
i32 CMulti::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    g_gameReg->m_134 = 2;
    if (mgr == 0) {
        return 0;
    }

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }
    g_connectRptMgr = this;

    m_region0Gate = 0;
    m_region1Gate = 0;
    m_region2Gate = 0;
    m_viewMode = 0;
    m_hudSuppressed = 1;
    m_49c = -1;
    m_snapshotActive = 0;
    m_scrollEdgeActive = 0;
    m_scrollEdgeLock = 0;
    m_530 = 0;
    m_534 = 0;
    m_sessionTerminated = 0;
    m_538 = 0;
    m_5ac = 0;
    m_pollAbort = 0;
    m_56c = 0;
    m_568 = 0;
    m_574 = 0;
    m_notifyLatch = 0;
    m_1c0 = 0;
    m_syncGate = 0;
    m_connected = 0;
    m_pumpGuard = 0;
    m_584 = 0;
    m_588 = 0;
    m_570 = 0;
    m_1c4 = 1;
    m_5bc = 0;
    m_hostIndex = 0;
    m_5a4 = 0;
    m_600 = 1;
    m_drainReload = 0;
    m_lightFx = 0;
    m_savedClock = 0;
    m_rngSeed = static_cast<i32>(::timeGetTime());
    m_58c = 0;
    m_594 = 0;

    i32* clat = m_channelLatency;
    for (i32 i = 0; i < 4; i++) {
        *clat++ = 0;
        g_gameReg->m_options[i].m_latency.m_avg = 0;
        g_gameReg->m_options[i].m_latency.m_count = 0;
    }

    NetGameMgr()->m_114 = 0;
    Mgr()->ResetClockGlobals();
    Mgr()->ClearOptionsSlots();
    ChannelSlots_InitAll();

    CNetMgr* peer = new CNetMgr();
    m_netGate = peer;
    g_groupEnumMgr = peer;

    NetGameMgr()->m_modalBusy = 1;
    if (Mgr()->InitializeLobbyConnectionSettings() != 0) {
        if (StartTitle() == 0) {
            NetGameMgr()->m_modalBusy = 0;
            ReleaseResources();
            return 0;
        }
    } else {
        if (Open() == 0) {
            NetGameMgr()->m_modalBusy = 0;
            while (::ShowCursor(0) >= 0) {
            }
            return 0;
        }
    }

    if (m_isHost != 0) {
        m_58c = 1;
    }
    NetGameMgr()->m_modalBusy = 0;
    memset(&m_saveSlot, 0, sizeof(m_saveSlot));
    m_590 = NetGameMgr()->m_isEffectsEnabled;
    NetGameMgr()->m_isEffectsEnabled = 1;
    if (LoadImageBanks() == 0) {
        return 0;
    }
    PostLoadImageBanks();
    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_MULTI"));
    if (m_2c == 0) {
        return 0;
    }
    if (ShowMultiStartDlg() == 0) {
        return 0;
    }
    while (::ShowCursor(0) >= 0) {
    }
    if (CreateSession() == 0) {
        return 0;
    }

    if (m_5b0 != 0) {
        NetGameMgr()->m_12c = 0;
        NetGameMgr()->m_strWorldFile = "custom\\" + GetConfigNameB();
    } else {
        NetGameMgr()->m_12c = 1;
        NetGameMgr()->m_strWorldFile = GetConfigNameA();
    }
    if (Mgr()->GetWorldFileName().GetLength() == 0) {
        return 0;
    }

    CChatBoxOwner* iface = new CChatBoxOwner();
    m_hitTest = iface;

    if (iface->Attach(m_world, NetGameMgr()->m_chatLog) == 0) {
        CChatBoxOwner* io = m_hitTest;
        if (io == 0) {
            return 0;
        }
        io->Deactivate();
        ::operator delete(io);
        m_hitTest = 0;
        return 0;
    }
    m_hitTest->m_10 = 0;
    m_hitTest->Configure(1);

    CStatusBarMgr* sess = new CStatusBarMgr;
    m_guts = sess;
    if (sess->LoadBattlezItemConfig(m_world) == 0) {
        CStatusBarMgr* so = m_guts;
        if (so == 0) {
            return 0;
        }
        so->Teardown();
        delete so;
        m_guts = 0;
        return 0;
    }

    CTileTriggerContainer* cmd = new CTileTriggerContainer();
    m_beginMarker = cmd;
    if (cmd->GetFlag74() == 0) {
        CTileTriggerContainer* co = m_beginMarker;
        if (co == 0) {
            return 0;
        }
        delete co;
        m_beginMarker = 0;
        return 0;
    }

    if (LoadByMode(1, 1) == 0) {
        return 0;
    }
    m_pumpGuard = 1;
    m_534 = 0;
    i32 wr = WaitForOtherPlayers();
    m_pumpGuard = 0;
    if (wr == 0) {
        return 0;
    }
    if ((static_cast<CPlay*>(this))->LoadCursorSprites(0, 0) == 0) {
        return 0;
    }
    PollSession();
    srand(m_rngSeed);
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    NetGameMgr()->m_chatLog->FreeNodes();
    m_connected = 1;
    return 1;
}

RVA_COMPGEN(0x000b5fd0, 0x1e, ??_GCNetMgr@@UAEPAXI@Z)
RVA(0x000b6000, 0x6d)
CNetMgr::~CNetMgr() {
    Destroy();
}

RVA(0x000b6110, 0xc7)
void CMulti::ReleaseResources() {
    if (m_netGate && m_5bc && m_session && m_connected) {
        SendNetStat(0x402, 0x4d2, 1);
        SendStatFlag(0x3ea, 1);
    }

    CNetSession* session = m_session;
    if (session) {
        delete session;
        m_session = 0;
    }
    if (m_netGate) {
        delete m_netGate;
        m_netGate = 0;
    }

    CLightFxRender* lightFx = m_lightFx;
    if (lightFx) {
        lightFx->Ctor();
        ::operator delete(lightFx);
        m_lightFx = 0;
    }
    Mgr()->m_isEffectsEnabled = m_590;

    CPlay::ReleaseResources();
}

RVA(0x000b6220, 0x54)
CNetSession::~CNetSession() {
    ResetSync();
}

RVA(0x000b62a0, 0x4a)
CNetCmdSlot::~CNetCmdSlot() {
    ResetAll();
}

RVA(0x000b6310, 0x5)
void CMulti::OnExit() {
    CPlay::OnExit();
}

RVA(0x000b6330, 0x89)
i32 CMulti::EnterState(i32 arg) {
    if (CPlay::EnterState(arg) == 0) {
        return 0;
    }
    m_mgr->RefreshGameClock();
    g_frameTime = m_savedClock;
    DWORD(WINAPI * tg)(void) = ::timeGetTime;
    m_drainTimer = 0;
    m_lastTime = tg();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = tg();
    if (m_connected != 0) {
        SendNetStat(0x402, 0x4d2, 1);
    }
    return 1;
}

// @early-stop
RVA(0x000b63f0, 0x11b)
i32 CMulti::LeaveState(i32 arg) {
    m_mgr->m_cueSink->PauseAllVoices();
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_notifyLatch) {
        QuitToMenu();
    }
    if (arg == 9) {
        return 1;
    }
    RECT r;
    m_world->m_drawTarget->m_overlayPair->m_surface->Fill(0);
    CString s;
    s.LoadString(0x81a9);
    r.right = m_mgr->m_modeW;
    r.bottom = m_mgr->m_modeH;
    r.left = 0;
    r.top = 0;
    ShowHudMessage(m_world, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1);
    if (m_mgr && m_mgr->m_cmdGrid) {
        m_mgr->m_cmdGrid->ClearGridRange(5);
    }
    return 1;
}

RVA(0x000b6560, 0x5)
i32 CMulti::CompleteLevel() {
    return CPlay::CompleteLevel();
}

RVA(0x000b6580, 0x1eb)
i32 CMulti::LoadByMode(i32 mode, i32 unused) {
    g_optionsCursor = 0;
    GruntzPlayer* host = Mgr()->FindOptionsSlot(m_hostIndex);
    if (!host) {
        return 0;
    }
    g_curPlayer = host->m_playerIndex;
    srand(m_rngSeed);
    g_activePlayerCount = 0;
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    m_5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = timeGetTime();
    m_curSlotId = m_session->m_tick - 1;
    m_574 = 0;

    if (CPlay::LoadByMode(mode, 0) == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; ++i) {
        GruntzPlayer* e = &Mgr()->m_options[i];
        if (e == 0) {
            return 0;
        }
        e->m_038.FreeArrays();
        if (e->m_038.LoadConfig(Mgr(), i, e->m_configId) == 0) {
            return 0;
        }
        if (e->m_014 && e->m_liveGate) {
            e->m_038.Clear();
        }
    }
    ResetPlayState();
    srand(m_rngSeed);
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    m_5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = timeGetTime();
    m_curSlotId = m_session->m_tick - 1;
    m_574 = 0;
    Mgr()->m_chatLog->FreeNodes();
    m_session->Reset();
    Mgr()->m_cueSink->PauseAllVoices();
    return 1;
}

// @early-stop
RVA(0x000b67f0, 0x74)
i32 CMulti::Connect(i32 mode) {
    m_connected = 0;
    m_534 = 0;
    if (Mgr()->PassClickToPlayState(mode, 0, 0) == 0) {
        Mgr()->ReportError(0x8005, 0x446);
        return 0;
    }
    m_pumpGuard = 1;
    i32 r = PumpA();
    m_pumpGuard = 0;
    if (r == 0) {
        return 0;
    }
    m_connected = 1;
    return 1;
}

// @early-stop
RVA(0x000b6890, 0x21b)
i32 CMulti::Render() {
    m_drewThisFrame = 0;
    HandleDragMove(0, m_cursorX, m_cursorY);
    i32 oldT = m_lastTime;
    i32 t = timeGetTime();
    m_lastTime = t;

    m_frameDelta = t - oldT;
    m_accumTime += m_frameDelta;
    i32 newId = m_session->m_tick;
    if (m_curSlotId != newId) {
        m_curSlotId = newId;
        CGruntzCmdMgr* mgr = Mgr()->m_cmdSubMgr;
        CGruntzCommand* node;
        if (mgr->m_1c.GetCount() == 0) {
            node = 0;
        } else {
            node = static_cast<CGruntzCommand*>(mgr->m_1c.RemoveHead());
        }
        if (node) {
            node->m_submitted = 1;

            i32 v = m_curSlotId + static_cast<i32>(m_5a4) * 2;
            node->m_targetType = static_cast<u8>(v % 128);
        }
        m_session->ArmSlot(node, static_cast<u8>(static_cast<u8>(m_5a4) << 1));
    }
    i32 dt = m_frameDelta;
    if (static_cast<u32>(dt) >= g_frameDelta) {
        dt = static_cast<i32>(g_frameDelta);
    }
    m_packetsRcvd = m_session->Poll(dt);
    m_packetsSent = 0;

    if (static_cast<u32>(m_frameDelta) >= static_cast<u32>(m_drainTimer)) {
        m_drainTimer = 0;
    } else {
        m_drainTimer = m_drainTimer - m_frameDelta;
    }
    if (m_drainTimer == 0) {
        m_packetsSent = m_session->Tick();
        m_drainTimer = m_drainReload;
    }
    i32 fin = 0;
    if (m_session->Advance() && m_pollAbort == 0) {
        fin = 1;
    }
    TickStateMgrs();
    CDDrawWorkerHost* mainPlane = m_world->m_level->m_mainPlane;
    if (mainPlane) {
        mainPlane->ActivateVisibleObjects();
    }

    if (fin != 0) {
        if (m_session->Verify() == 0 && m_574 == 0) {
            if (m_isHost != 0) {
                SendStatFlag(0x404, 1);
                OnOutOfSync();
                PumpA();
                m_drainTimer = 0;
                return 1;
            }
            SendStatFlag(0x403, 1);
        }
        PumpA();
        m_drainTimer = 0;
        return 1;
    }
    PumpB();
    DropTimeout();
    SoundStream* win = m_world->m_soundStream;
    if (win) {
        i32 now = timeGetTime();
        win->PurgeVoiceList(now);
        win->TickSubManagers(now);
    }
    ActiveWait(2);
    return 1;
}

// @early-stop
RVA(0x000b6b40, 0x29e)
i32 CMulti::PumpA() {
    i32 ready = FrameSyncWait();
    if (m_594 == 0 && Mgr()->m_frameGate != 0 && ready == 0) {
        PumpB();
        return 1;
    }

    g_frameDelta = 0x21;
    g_lastNow += 0x21;
    g_frameTime += 0x21;
    g_killCueClock = g_lastNow;
    g_engineFrameDelta = 0x21;
    if (m_ambientInitDone == 0) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_ambientTimer64.m_v
            >= m_ambientInterval64.m_v) {
            char name[0x40];
            wsprintfA(name, "AMBIENT%d", GetAmbientId());
            if (g_gameReg->m_musicEnabled != 0) {
                Mgr()->m_sound->PlayByName(name, 1);
            } else {

                CGruntzSoundZ* snd = Mgr()->m_sound;
                CGruntzSoundInnerZ* p = snd->FindBank(name);
                if (p) {
                    snd->m_pCurrent = p;
                }
                if (Mgr()->m_sound->m_pCurrent) {
                    Mgr()->m_sound->m_pCurrent->SetLoop(1);
                }
            }
            m_ambientInitDone = 1;
        }
    }
    Mgr()->m_cmdSubMgr->ScanTargets(m_curSlotId % 128);
    m_session->Checksum();
    g_frameTicks++;
    u32 t1 = g_timer32 ? g_timer32 : 0x32;
    if (g_frameDelta >= t1) {
        g_timer32 = 0;
    } else {
        g_timer32 = t1 - g_frameDelta;
    }
    u32 t2 = g_timer100 ? g_timer100 : 0x64;
    if (g_frameDelta >= t2) {
        g_timer100 = 0;
    } else {
        g_timer100 = t2 - g_frameDelta;
    }
    u32 t3 = g_timer200 ? g_timer200 : 0xc8;
    if (g_frameDelta >= t3) {
        g_timer200 = 0;
    } else {
        g_timer200 = t3 - g_frameDelta;
    }
    u32 t4 = g_timer400 ? g_timer400 : 0x190;
    if (g_frameDelta >= t4) {
        g_timer400 = 0;
    } else {
        g_timer400 = t4 - g_frameDelta;
    }
    u32 t5 = g_timer500 ? g_timer500 : 0x1f4;
    if (g_frameDelta >= t5) {
        g_timer500 = 0;
    } else {
        g_timer500 = t5 - g_frameDelta;
    }
    m_world->m_childGroup->TickKillCues(0);
    m_world->m_childGroup->CollideBroadcast();
    Mgr()->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
    m_guts->LoadDestructButtonSprite(g_frameDelta);
    SoundStream* win = m_world->m_soundStream;
    if (win) {
        i32 now = timeGetTime();
        win->PurgeVoiceList(now);
        win->TickSubManagers(now);
    }
    m_beginMarker->FilterList2(g_frameDelta);
    (static_cast<CMapMgr*>(Mgr()->m_tileGrid))->UpdateDiagonals(Mgr());
    if (ready == 0) {
        PumpB();
    }
    Mgr()->AdvanceOptionsCycle();
    return 1;
}

// @early-stop
RVA(0x000b6e90, 0x34d)
void CMulti::PumpB() {
    CDDrawSurfaceMgr* mgr = m_world;
    if (m_594 == 0 && Mgr()->m_frameGate != 0) {
        StepInputA();
        mgr->m_level->VisitVisible(mgr->m_drawTarget->m_backPair, mgr->m_childGroup);
        mgr->m_workerList->PruneWorkers(
            mgr->m_drawTarget->m_backPair,
            mgr->m_drawTarget->m_overlayPair
        );
        m_guts->LoadMainStatusBarSprite();
        CDDrawSurfacePair* h = static_cast<CDDrawSurfacePair*>(mgr->m_drawTarget->m_backPair);
        if (h == 0) {
            return;
        }
        StepGridWalk(g_frameDelta);
        DrawCursorSaveUnder(h);
        mgr->m_drawTarget->m_frontPair->m_surface->Flip(0);
        return;
    }
    StepInputA();
    StepViewportResize();
    if (m_region0Gate != 0) {
        (static_cast<CDDrawSurfacePair*>(mgr->m_drawTarget->m_backPair))->m_surface->Fill(0);
        m_guts->Deactivate();
    }
    if (m_worldReady == 0) {
        if (Mgr()->m_cmdGrid->m_armed != 0) {
            Mgr()->m_cmdGrid->ScrollToActiveRecord();
        } else {
            LoadScrollSpeedOptions();
        }
    }
    StepScroll();
    Mgr()->m_inputState->Retune(
        (mgr->m_level->m_mainPlane)->m_snappedX,
        (mgr->m_level->m_mainPlane)->m_snappedY
    );
    if (m_region1Gate != 0) {
        NotifyVisibleEntities();
    } else {
        mgr->m_level->VisitVisible(mgr->m_drawTarget->m_backPair, mgr->m_childGroup);
        mgr->m_workerList->PruneWorkers(
            mgr->m_drawTarget->m_backPair,
            mgr->m_drawTarget->m_overlayPair
        );
    }
    m_guts->LoadMainStatusBarSprite();
    if (m_lightFx != 0) {
        CStatusBarMgr* fx = m_guts;
        if (fx->m_position != 2 && fx->m_activeTab != 5) {
            RECT rc;
            if (fx->m_position == 1) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                i32 cx = g_gameReg->m_modeH;
                i32 cy = g_gameReg->m_modeW;
                rc.top = cx;
                SetRect(&rc, cy - 140, 5, cy - 20, 125);
            }
            m_lightFx->Resize(static_cast<i32>(g_frameDelta), 0);
            m_lightFx->ComputeRect(
                static_cast<CDDrawSurfacePair*>(mgr->m_drawTarget->m_backPair),
                &rc
            );
        }
    }
    Mgr()->m_chatLog->Scroll(g_frameDelta);
    CDDrawSurfacePair* h = static_cast<CDDrawSurfacePair*>(mgr->m_drawTarget->m_backPair);
    if (h == 0) {
        return;
    }
    m_hitTest->LoadChatBoxSprite(h);
    DrawDebugStats();
    Mgr()->m_cmdGrid->OverlayRelease();
    StepGridWalk(g_frameDelta);
    DrawCursorSaveUnder(h);
    if (m_worldReady != 0) {
        h->DrawBox(&m_hudRect, 0xff);
    }
    mgr->m_drawTarget->m_frontPair->m_surface->Flip(0);
    PumpBRefresh2356(g_gameReg, m_guts, m_region0Gate);
    if (mgr->m_level->m_mainPlane != 0) {
        (mgr->m_level->m_mainPlane)->DeactivateDistantObjects();
    }
    if (m_region0Gate != 0) {
        if (static_cast<i64>(g_frameTime) - m_region0Timer64.m_v >= m_region0Interval64.m_v) {
            SetTinyViewportCurse(0);
        }
    }
    if (m_region1Gate != 0) {
        if (static_cast<i64>(g_frameTime) - m_region1Timer64.m_v >= m_region1Interval64.m_v) {
            SetDarknessCurse(0);
        }
    }
}

// @early-stop
RVA(0x000b72c0, 0x30b)
i32 CMulti::StartTitle() {
    Mgr()->m_lobbyResult = 0;
    m_588 = 1;
    if (!m_netGate) {
        return 0;
    }
    CSymTab* saved = m_2c;
    CSymTab* st = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_ATTRACT"));
    m_2c = st;
    if (!st) {
        return 0;
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString title;
    title.Format("TITLE%d", idx);

    if (RunTitleSeq(title, 0, 0, 1, 0) == 0) {
        m_2c = saved;
        return 0;
    }

    m_world->m_drawTarget->PresentBackPage();

    m_world->m_ptrColl->m_device->FlipToGDISurface();
    m_2c = saved;
    while (::ShowCursor(1) < 0) {
    }
    if (!Mgr()->m_lobby) {
        return 0;
    }
    CNetLobbyConnection* desc = Mgr()->m_connSettings;
    if (!desc) {
        return 0;
    }
    m_isHost = (desc->m_dwFlags & 2) ? 1 : 0;

    if (m_netGate->Init(Mgr()->m_lobby, g_dplayAppGuid) == 0) {
        return 0;
    }
    m_netGate->ClearPlayerList();
    CNetPlayerListNode* player = m_netGate->AddPlayerNode(desc->m_sessionDesc);
    if (player == 0) {
        return 0;
    }
    m_netGate->m_playerSel = player;
    CString hostName(desc->m_playerName->m_shortName);
    SetServiceName(hostName);
    char* grp = player->GroupName();
    CString grpName(grp);
    ApplyDynSetting(grpName);

    i32 r = m_isHost ? SetupTcpIpConfig() : CreateLocalPlayer();
    return r ? 1 : 0;
}

RVA(0x000b76a0, 0x4)
char* CNetPlayerListNode::GroupName() {
    return m_desc.m_lpszName;
}

RVA(0x000b76c0, 0x4f)
void CMulti::ApplyDynSetting(CString s) {
    m_groupName = s;
}

RVA(0x000b7730, 0x4f)
void CMulti::SetServiceName(CString s) {
    m_hostName = s;
}

RVA(0x000b77a0, 0xb5)
i32 CMulti::Open() {
    if (!Peer()) {
        return 0;
    }
    RunTitleSeq("BACKGND", 0, 0, 1, 0);
    m_world->m_drawTarget->PresentBackPage();
    InterfaceObject* descriptor = SetupServices();
    if (!descriptor) {
        return 0;
    }
    if (!Peer()->InitFromProvider(descriptor, g_dplayAppGuid.m_guid)) {
        return 0;
    }
    if (g_hostServicesMode) {
        m_isHost = 1;
        if (!DetectConnectionConfig()) {
            return 0;
        }
    } else {
        m_isHost = 0;
        if (!JoinSession()) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000b78b0, 0x17f)
InterfaceObject* CMulti::SetupServices() {
    if (Peer()->EnumServiceProviders(0) != 0) {
        ReportNetError(0);
        return 0;
    }

    if (g_hostServicesMode != 0) {
        if (RunErrorDialog("MULTI_HOSTSERVICES", NetSetupDlgProc, 0) != 0) {
            Utils::RegistryHelper* store = NetGameMgr()->m_settings;
            if (store != 0 && g_serviceId != NETSERVICE_NONE) {
                store->SetValueDword("Service", g_serviceId);
                {
                    store->SetValueString(
                        "Player_Name",
                        const_cast<char*>(static_cast<const char*>(GetString5a0()))
                    );
                }
                {
                    store->SetValueString(
                        "Game_Name",
                        const_cast<char*>(static_cast<const char*>(GetString59c()))
                    );
                }
            }
        }
    } else {
        if (RunErrorDialog("MULTI_JOINSERVICES", NetSetupDlgProc, 0) != 0) {
            Utils::RegistryHelper* store = NetGameMgr()->m_settings;
            if (store != 0) {
                if (g_serviceId != NETSERVICE_NONE) {
                    store->SetValueDword("Service", g_serviceId);
                }
                store->SetValueString(
                    "Player_Name",
                    const_cast<char*>(static_cast<const char*>(GetString5a0()))
                );
            }
        }
    }
    return Peer()->m_groupSel;
}

RVA(0x000b7a90, 0x23)
CString CMulti::GetString59c() {
    return m_groupName;
}

RVA(0x000b7b10, 0x27c)
INT_PTR CALLBACK NetSetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {

    char nameBuf[0xa];
    char gameBuf[0x44];
    g_setupDlgHwnd = hDlg;
    if (BaseDlgProc(hDlg, msg, wParam, lParam) != 0) {
        return 1;
    }

    switch (msg) {
        case 0x110: {
            HWND combo = ::GetDlgItem(hDlg, 0x3fc);
            g_groupEnumMgr->m_groupSel = 0;
            g_groupEnumMgr->PopulateGroupList(combo, 0);
            if (g_serviceId == NETSERVICE_NONE) {
                ::SendMessageA(combo, 0x186, 0, 0);
            } else if (static_cast<i32>(::SendMessageA(combo, 0x186, g_serviceId, 0)) == -1) {
                ::SendMessageA(combo, 0x186, 0, 0);
            }

            DWORD cap = 0xa;
            g_gameReg->m_settings->GetValueString(
                const_cast<char*>(static_cast<const char*>(("Player_Name"))),
                nameBuf,
                &cap,
                "Player"
            );
            cap = 0x40;
            g_gameReg->m_settings->GetValueString(
                const_cast<char*>(static_cast<const char*>(("Game_Name"))),
                gameBuf,
                &cap,
                "Multiplayer_Gruntz"
            );

            HWND edName = ::GetDlgItem(hDlg, 0x51b);
            ::SendMessageA(edName, 0xc5, 9, 0);
            ::SetDlgItemTextA(hDlg, 0x51b, nameBuf);
            HWND edGame = ::GetDlgItem(hDlg, 0x51c);
            ::SendMessageA(edGame, 0xc5, 0x3f, 0);
            ::SetDlgItemTextA(hDlg, 0x51c, gameBuf);
            return 1;
        }
        case 0x111:
            break;
        default:
            goto ret_false;
    }

    if (wParam == 2) {
        ::EndDialog(hDlg, 0);
        return 1;
    }

    if (wParam == 1) {

        ::GetDlgItemTextA(hDlg, 0x51b, gameBuf, 0xa);
        if (gameBuf[0] == 0) {
            ::MessageBeep(0);
            return wParam;
        }
        g_connectRptMgr->SetServiceName(CString(gameBuf));

        if (g_hostServicesMode != 0) {
            ::GetDlgItemTextA(hDlg, 0x51c, gameBuf, 0x40);
            if (gameBuf[0] == 0) {
                ::MessageBeep(0);
                return 1;
            }
            g_connectRptMgr->ApplyDynSetting(CString(gameBuf));
        }

        HWND combo = ::GetDlgItem(hDlg, 0x3fc);
        i32 svc = static_cast<i32>(::SendMessageA(combo, 0x188, 0, 0));
        if (svc != -1) {
            g_serviceId = svc;
        }
        g_groupEnumMgr->ReadGroupSel(::GetDlgItem(hDlg, 0x3fc));
        ::EndDialog(hDlg, 1);
        return 1;
    }
ret_false:
    return 0;
}

RVA(0x000b7e30, 0x63)
void CMulti::ReportVersionMsg(char* msg, i32 code) {
    char buf[512];
    if (msg && *msg && Mgr()) {
        if (code > 0) {
            sprintf(buf, "%s (%i)", msg, code);
            Mgr()->EnterModalUI(buf);
        } else {
            Mgr()->EnterModalUI(msg);
        }
    }
}

RVA(0x000b7ec0, 0x7d)
void CMulti::ReportStatusId(u32 strId, i32 level) {
    char buf[0x12a];
    if (Mgr() && Mgr()->m_owner->m_hInstance) {
        if (!LoadStringA(Mgr()->m_owner->m_hInstance, strId, buf, 0xfa)) {
            strcpy(buf, "Error");
        }
        ReportVersionMsg(buf, level);
    }
}

RVA(0x000b7f60, 0x52)
void CMulti::ReportNetError(i32 level) {
    char buf[512];
    if (Mgr() && g_code != 0x118) {
        sprintf(buf, "Error: %s - %i", g_szCode, g_code);
        ReportVersionMsg(buf, level);
    }
}

RVA(0x000b7fe0, 0x2f)
i32 CMulti::JoinSession() {
    if (RunErrorDialog("MULTI_JOIN", MultiJoinDlgProc, 0) == 0) {
        return 0;
    }
    SendStatFlag(0x3f7, 1);
    return 1;
}

RVA(0x000b8020, 0x22f)
INT_PTR CALLBACK MultiJoinDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    g_setupDlgHwnd = hDlg;
    if (BaseDlgProc(hDlg, msg, wParam, lParam) != 0) {
        goto ret_true;
    }
    switch (msg) {
        case 0x110:
            g_netPlayerListHwnd = ::GetDlgItem(hDlg, 0x3fc);
            if (g_netPlayerListHwnd == 0) {
                goto close;
            }

            if (g_groupEnumMgr != 0) {
                g_groupEnumMgr->m_playerSel = 0;
                SetTimer(hDlg, 1, 0x9c4, 0);
                ::SendMessageA(hDlg, 0x113, 0, 0);
                return 1;
            }
            goto close;
        case 0x111:
            if (wParam == 2) {
                KillTimer(hDlg, 1);
                ::EndDialog(hDlg, 0);
                return 1;
            }

            if (wParam == 1) {
                KillTimer(hDlg, 1);

                if ((static_cast<CMulti*>(g_connectRptMgr))->OnJoinConfirm(hDlg) == 0) {
                    ::MessageBeep(0);
                    i32 t = 0x7d0;
                    InterfaceObject* io = g_groupEnumMgr->m_groupSel;
                    if (io && io->IsTcpIpProvider()) {
                        t = 0x1388;
                    }
                    SetTimer(hDlg, 1, t, 0);
                    return 0;
                }
                ::EndDialog(hDlg, 1);
                return 1;
            }
            break;
        case 0x113:
            KillTimer(hDlg, 1);
            {
                i32 sel = static_cast<i32>(::SendMessageA(g_netPlayerListHwnd, 0x188, 0, 0));
                i32 hr = g_groupEnumMgr->EnumPlayersInto(0, 0);
                if (hr == static_cast<i32>(0x88770118)) {
                    goto close;
                }
                if (hr != 0) {
                    if (g_connectRptMgr == 0) {
                        goto close;
                    }
                    (static_cast<CMulti*>(g_connectRptMgr))->ReportNetError(0);
                    ::EndDialog(hDlg, 0);
                    return 1;
                }
                FillPlayerList(g_netPlayerListHwnd, g_groupEnumMgr);
                if (sel != -1) {
                    ::SendMessageA(g_netPlayerListHwnd, 0x186, sel, 0);
                } else {
                    ::SendMessageA(g_netPlayerListHwnd, 0x186, 0, 0);
                }
                RefreshPlayerRow(hDlg, g_netPlayerListHwnd);
                i32 t = 0x7d0;
                InterfaceObject* io = g_groupEnumMgr->m_groupSel;
                if (io && io->IsTcpIpProvider()) {
                    t = 0x1388;
                }
                SetTimer(hDlg, 1, t, 0);
            }
            return 1;
    }
ret_false:
    return 0;
close:
    ::EndDialog(hDlg, 0);
ret_true:
    return 1;
}

RVA(0x000b82e0, 0x230)
i32 CMulti::DetectConnectionConfig() {
    m_5ac = 0;
    InterfaceObject* provider = Peer()->m_groupSel;
    if (provider == 0) {
        return 0;
    }

    m_598 = "Other";
    if (provider->IsIpxProvider()) {
        m_598 = "IPX";
        m_5a4 = 2;
        m_drainReload = 0xa;
    } else if (provider->IsTcpIpProvider()) {
        m_598 = "TcpIp";
        m_5a4 = 3;
        m_drainReload = 0xa;
    } else if (provider->IsModemProvider()) {
        m_598 = "Modem";
        m_5a4 = 4;
        m_drainReload = 0x1e;
    } else if (provider->IsSerialProvider()) {
        m_598 = "Serial";
        m_5a4 = 2;
        m_drainReload = 0xa;
    } else {
        m_5a4 = 2;
        m_drainReload = 0xa;
    }

    Utils::RegistryHelper* cfg = NetGameMgr()->m_settings;
    CString kDelay = m_598 + "_CmdDelay";
    CString kResend = m_598 + "_Resend";
    CString kDyn = m_598 + "_DynCmdDelay";
    i32 cd = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kDelay))), -1);
    i32 rs = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kResend))), -1);
    if (cd != -1 && rs != -1) {
        m_5a4 = cd;
        m_drainReload = rs;
    }

    GruntzPlayer* ch0 = NetGameMgr()->m_options;

    ch0->m_name = GetString5a0();
    ch0->m_008 = 0;

    CNetPlayerListNode* r = JoinAndRegisterChannel();
    if (r == 0) {
        return 0;
    }
    Peer()->m_playerSel = r;
    return 1;
}

RVA(0x000b85a0, 0xd2)
void CMulti::ApplyCmdDelayDefaults() {
    Utils::RegistryHelper* reg = g_gameReg->m_settings;

    CString cmdDelayName = m_598 + "_CmdDelay";
    CString resendName = m_598 + "_Resend";
    CString dynCmdName = m_598 + "_DynCmdDelay";

    reg->SetValueDword(const_cast<char*>(static_cast<const char*>(cmdDelayName)), m_5a4);
    reg->SetValueDword(const_cast<char*>(static_cast<const char*>(resendName)), m_drainReload);
}

RVA(0x000b86c0, 0x206)
i32 CMulti::ShowMultiStartDlg() {
    CMultiStartDlg dlg(m_mgr, 0);
    i32 r = m_mgr->ExitModalUI(&dlg, 0);
    g_sharedFlag = 0;
    if (r != 1) {
        if (m_isHost != 0) {
            GruntzPlayer* rec = m_mgr->FindOptionsSlot(m_hostIndex);
            if (rec == 0) {
                return 0;
            }
            rec->m_liveGate = 0;
            ChannelSlots_Set(rec->m_008, 1);
            BroadcastChannelTable(0);
        }
        if (m_isHost == 0 && m_538 == 0) {
            SendStatFlag(0x3ea, 1);
        }
        return 0;
    }

    if (m_isHost != 0) {
        ApplyCmdDelayDefaults();
    } else {
        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_emitGate == 0) {
            void* rec_ob = 0;
            reg->m_cues.Lookup(s_GameKey, rec_ob);
            LeafCue* rec = static_cast<LeafCue*>(rec_ob);
            if (rec != 0) {
                i32 snd = g_sndEnabled;
                i32 cue = g_sndCueTag;
                if (snd != 0) {
                    i32 clk = g_killCueClock;
                    if (static_cast<u32>((clk - rec->m_lastPlayTime))
                        >= static_cast<u32>(rec->m_replayDelay)) {
                        rec->m_lastPlayTime = clk;
                        rec->m_sound->ConfigureItem(cue, 0, 0, 0);
                    }
                }
            }
        }
        ActiveWait(0xfa);
    }
    return 1;
}

RVA_COMPGEN(0x000b8960, 0x59, ??1CMultiStartDlg@@UAE@XZ)

// @early-stop
RVA(0x000b89e0, 0xc8)
void FillPlayerList(HWND hList, CNetMgr* sess) {
    char buf[256];
    if (!hList) {
        return;
    }
    if (!sess) {
        return;
    }
    ::SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    sess->m_playerSelId = sess->m_players.GetHeadPosition();
    CNetPlayerListNode* player =
        sess->m_playerSelId != 0
            ? static_cast<CNetPlayerListNode*>(sess->m_players.GetNext(sess->m_playerSelId))
            : 0;
    while (player) {

        MsgParam name;
        i32 idx;
        if (NetFormatKeyed(buf, player->m_desc.m_lpszName, "NAME")) {
            name.m_str = buf;
            idx = static_cast<i32>(::SendMessageA(hList, LB_ADDSTRING, 0, name.m_lparam));
        } else {
            name.m_str = player->m_desc.m_lpszName;
            idx = static_cast<i32>(::SendMessageA(hList, LB_ADDSTRING, 0, name.m_lparam));
        }
        if (idx != -1) {
            MsgParam cookie;
            cookie.m_player = player;
            ::SendMessageA(hList, LB_SETITEMDATA, idx, cookie.m_lparam);
        }

        if (sess->m_playerSelId != 0) {
            player = static_cast<CNetPlayerListNode*>(sess->m_players.GetAt(sess->m_playerSelId));
            sess->m_players.GetNext(sess->m_playerSelId);
        } else {
            player = 0;
        }
    }
}

// @early-stop
RVA(0x000b8b10, 0x175)
CNetPlayerListNode* CMulti::JoinAndRegisterChannel() {
    char buf[0x100];
    buf[0] = g_emptyString[0];
    memset(&buf[1], 0, 0xff);
    Cfg_SetSection(buf, "%s", m_groupName);
    Cfg_AppendKeyVal(buf, "CMDDELAY", m_5a4);
    Cfg_AppendKeyVal(buf, "RESEND", m_drainReload);
    Cfg_AppendKeyVal(buf, "LEVEL", ResyncLParam());

    CNetPlayerListNode* enumResult = g_groupEnumMgr->EnumGroupsInto(4, buf, 0, g_emptyString);
    if (enumResult == 0) {
        g_connectRptMgr->ReportNetError(0);
        return 0;
    }

    CNetSessionNode* node = Peer()->CreatePlayer(const_cast<char*>("Player"), g_emptyString, 0);
    m_5bc = node;
    if (node == 0) {
        ReportNetError(0);
        return 0;
    }

    m_hostIndex = node->m_id;
    GruntzPlayer* ch0 = NetGameMgr()->m_options;
    i32 chField = ch0->m_008;

    i32 failed = (RegisterChannelFrom(ch0->GetName(), chField, -1, m_hostIndex) == 0);
    return failed ? 0 : enumResult;
}

// @early-stop
RVA(0x000b8cf0, 0x23b)
i32 CMulti::OnJoinConfirm(void* hDlg) {
    if (hDlg == 0) {
        return 0;
    }

    g_groupEnumMgr->ReadPlayerSel(::GetDlgItem(static_cast<HWND>(hDlg), 0x3fc));
    CNetPlayerListNode* sel = Peer()->m_playerSel;
    if (sel == 0) {
        return 0;
    }

    m_5bc = Peer()->EnumPlayersCb(sel, static_cast<const char*>(GetString5a0()), g_emptyString, 0);
    if (LocalPlayer() == 0) {
        ReportNetError(0);
        return 0;
    }

    char buf[0x100];

    if (Cfg_GetKey(buf, sel->m_desc.m_lpszName, "CMDDELAY")) {
        m_5a4 = atoi(buf);
    }
    if (Cfg_GetKey(buf, sel->m_desc.m_lpszName, "RESEND")) {
        m_drainReload = atoi(buf);
    }
    if (Cfg_GetKey(buf, sel->m_desc.m_lpszName, "DynCmdDelay")) {
        ApplyDynSetting(CString(buf));
    }
    m_syncGate = 0;
    ResyncLParam() = 1;
    m_hostIndex = LocalPlayer()->m_id;
    if (Cfg_GetKey(buf, sel->m_desc.m_lpszName, "LEVEL")) {
        ResyncLParam() = atoi(buf);
    }

    CNetChannelPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= 0x80;
    packet.m_statId = 0x3f9;

    packet.m_hostIndex = m_hostIndex;
    packet.m_present = 1;
    packet.m_kind = 0;
    packet.m_slot = 1;
    packet.m_flagsB = 0;
    packet.m_configId = 0x63;
    packet.m_0e = 0;
    packet.m_0d = 0xf;
    strcpy(packet.m_name, GetString5a0());
    SendStatFrom(&packet, sizeof(packet), 1);
    return 1;
}

RVA(0x000b8fc0, 0x151)
i32 CMulti::VerifyCustomLevel(void* h, CNetSessionNode* playerTok) {
    if (h == 0) {
        goto notVerified;
    }
    if (playerTok == 0) {
        goto notVerified;
    }

    if (m_530 != 0) {
        i32 cfgId = m_5b0;

        i32 token = (g_gameReg)->BuildLevelRezPath(
            0,
            0,
            cfgId,
            0,
            cfgId != 0 ? GetConfigNameB() : GetConfigNameA()
        );

        g_connectRptMgr->m_levelVerifyResult = 0;
        if (g_connectRptMgr->Poll(token) == 0) {
            m_530 = 0;
            g_gameReg->EnterModalUI("Unable to verify custom level with other players");
            goto notVerified;
        }

        if (g_connectRptMgr->m_levelVerifyResult != 0) {
            return 1;
        }
        g_gameReg->EnterModalUI("Not all players have the (same) custom level.");
        m_530 = 0;
        goto notVerified;
    }
    PollSession();
notVerified:
    return 0;
}

RVA(0x000b9180, 0x4a)
i32 CMulti::PollSessionGated(i32 a1, i32 a2) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (m_534 != 0) {
        return 1;
    }
    PollSession();
    return m_534 != 0;
}

RVA(0x000b91f0, 0x31)
i32 CMulti::SendStatBuf(CNetStatPacket* pkt, i32 flag) {
    pkt->m_0 |= 0x80;
    i32 hr = Peer()->SetGroupDataFrom(LocalPlayer(), flag, pkt, 0x10);
    return hr == 0;
}

RVA(0x000b9240, 0x38)
void CMulti::SendStatFlag(i32 id, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_statId = id;
    pkt.m_value = LocalPlayer()->m_id;
    SendStatBuf(&pkt, flag);
}

RVA(0x000b9290, 0x32)
void CMulti::SendNetStat(i32 id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_statId = id;
    pkt.m_value = value;
    SendStatBuf(&pkt, flag);
}

RVA(0x000b92e0, 0x34)
i32 CMulti::SendStatFrom(void* pkt, i32 b, i32 c) {
    if (pkt == 0) {
        return 0;
    }
    i32 hr = Peer()->SetGroupDataFrom(LocalPlayer(), c, pkt, b);
    return hr == 0;
}

RVA(0x000b9330, 0x41)
i32 CMulti::SendStatPair(CNetSessionNode* recipient, CNetStatPacket* pkt, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    pkt->m_0 |= 0x80;
    i32 hr = Peer()->SetGroupData2(LocalPlayer(), recipient, c, pkt, 0x10);
    return hr == 0;
}

RVA(0x000b93a0, 0x47)
i32 CMulti::SendStatTo(CNetSessionNode* recipient, i32 id, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_statId = id;
    pkt.m_value = LocalPlayer()->m_id;
    return SendStatPair(recipient, &pkt, c);
}

RVA(0x000b9410, 0x51)
i32 CMulti::SendStat3(i32 id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_statId = value;
    pkt.m_value = LocalPlayer()->m_id;
    i32 hr = Peer()->SetData(LocalPlayer()->m_id, id, flag, &pkt, 0x10);
    return hr == 0;
}

RVA(0x000b9490, 0x42)
i32 CMulti::SendNetStatTo(CNetSessionNode* recipient, i32 id, u32 value, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_statId = id;
    pkt.m_value = value;
    return SendStatPair(recipient, &pkt, c);
}

RVA(0x000b9500, 0x46)
i32 CMulti::SendStatPairRaw(CNetSessionNode* recipient, void* pkt, i32 size, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    if (pkt == 0) {
        return 0;
    }
    i32 hr = Peer()->SetGroupData2(LocalPlayer(), recipient, c, pkt, size);
    return hr == 0;
}

RVA(0x000b9570, 0x53)
i32 CMulti::SendStatValue(i32 id, i32 statId, i32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_statId = statId;
    pkt.m_value = value;
    i32 hr = Peer()->SetData(LocalPlayer()->m_id, id, flag, &pkt, 0x10);
    return hr == 0;
}

// @early-stop
RVA(0x000b95f0, 0x10f)
i32 CMulti::PollSession() {
    if (LocalPlayer() == 0) {
        return 0;
    }

    i32 count;
    if (LocalPlayer() == 0) {
        count = 0;
    } else {
        IDirectPlay4Z* dp = Peer()->m_directPlay;

        i32 n;
        i32 hr = dp->GetMessageCount(LocalPlayer()->m_id, &n);
        count = hr ? 0 : n;
    }
    if (count <= 0) {
        return 0;
    }

    i32 dispatched;
    i32 sender;
    sender = 0;
    dispatched = 0;

    for (;;) {
        if (count <= 0) {
            break;
        }
        if (m_pollAbort) {
            break;
        }

        i32 size = 0x800;
        i32 idTo = LocalPlayer()->m_id;
        IDirectPlay4Z* dp = Peer()->m_directPlay;

        i32 hr = dp->Receive(&sender, &idTo, 1, g_recvBuffer, &size);

        if (hr) {
            CNetMgr::ReportError("c:\\proj\\incs\\netmgr.h", 0x141, hr, 0);
        }
        if (hr) {
            break;
        }
        count--;
        if (sender != LocalPlayer()->m_id) {
            DispatchRecvMsg(sender, g_recvBuffer, size);
            dispatched++;
        }
        if (hr) {
            break;
        }
    }
    return dispatched;
}

// @early-stop
RVA(0x000b9750, 0x810)
i32 CMulti::DispatchRecvMsg(i32 sender, char* buf, i32 size) {

    CNetWireMsg wire;
    wire.m_bytes = buf;
    CNetMsg* msg = wire.m_msg;
    if (msg == 0) {
        return 0;
    }
    if (sender == 0) {

        return HandleControlMsg(wire.m_ctrl, size);
    }

    CNetSessionNode* pd = static_cast<CNetSessionNode*>(Peer()->GetPlayerData(sender));
    if (m_connected != 0 || m_pumpGuard != 0) {
        if (pd != 0) {
            CNetCmdSlot* slot = Session()->FindCmdSlot(pd->m_id);
            if (slot != 0) {
                slot->m_latency = 0;
            }
        }
    }

    if ((msg->m_0 & 0x80) == 0) {
        return 0;
    }

    switch (msg->m_msgId) {
        case 0x3e8:
            m_534 = 1;
            return 1;

        case 0x3fc:
            m_530 = 1;
            return 1;

        case 0x3ed:
            if (m_534 != 0) {
                break;
            }
            RecordDropPlayer2(pd, sender);
            break;

        case 0x422: {
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(sender));
            if (player == 0) {
                return 1;
            }
            if (player->m_030 == 0) {
                player->m_030 = 1;
                g_activePlayerCount++;
            }
            OnMultiOptions();
            break;
        }

        case 0x423: {
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(sender));
            if (player == 0) {
                return 1;
            }
            if (player->m_030 == 0) {
                break;
            }
            player->m_030 = 0;
            g_activePlayerCount--;
            break;
        }

        case 0x3f0: {
            if (g_sharedFlag != 0) {
                NetLobby::AppendEditLine(g_sharedFlag, msg->m_c);
                break;
            }
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(sender));
            if (player == 0) {
                return 1;
            }
            (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))
                ->AddItem(msg->m_c, 0x30, player->m_008);
            CDDrawSubMgrLeafScan* host = m_world->m_soundRegistry;
            if (host->m_emitGate != 0) {
                break;
            }
            void* e_ob = 0;
            host->m_cues.Lookup("GAME_CHAT", e_ob);
            LeafCue* e = static_cast<LeafCue*>(e_ob);
            if (e == 0) {
                break;
            }
            PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            break;
        }

        case 0x411:
            if (m_pollAbort != 0) {
                break;
            }
            ReportVersionMsg("You have been dropped from the game.", 0);
            ::PostMessageA(NetGameMgr()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            m_pollAbort = 1;
            break;

        case 0x410:
            AckDropPlayer(msg->m_8);
            break;

        case 0x3ea:
            OnPlayerLeft(sender);
            ResetPlayerCommands(sender);
            g_playerLeftFlag = 1;
            break;

        case 0x3f7:
            if (m_isHost == 0) {
                break;
            }
            BroadcastChannelTable(pd);
            break;

        case 0x3f8:
            if (m_isHost != 0) {
                break;
            }
            ParseChannelTable(msg);
            g_playerLeftFlag = 1;
            break;

        case 0x3f9: {
            if (m_isHost == 0) {
                break;
            }
            if (m_connected != 0) {
                break;
            }
            if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
                break;
            }

            CNetChannelPacket* chan = wire.m_chan;
            if (ChannelSlots_Get(chan->m_kind) == 0) {
                chan->m_kind = static_cast<u8>(ChannelSlots_FindFree());
            }
            ChannelSlots_Set(chan->m_kind, 0);
            RegisterChannelRec(chan);
            BroadcastChannelTable(0);
            SaveConfig(pd);
            g_playerLeftFlag = 1;
            break;
        }

        case 0x3fa: {
            if (m_isHost == 0) {
                break;
            }
            if (m_connected != 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(msg->m_14));
            if (player == 0) {
                return 0;
            }
            if (player->SwapChannel(static_cast<u8>(msg->m_c[1])) == 0) {
                msg->m_c[1] = static_cast<char>(player->m_008);
                SendStatTo(pd, 0x419, 1);
            }
            ParseOneChannel(msg);
            BroadcastChannelTable(0);
            g_playerLeftFlag = 1;
            break;
        }

        case 0x3fb:
            if (m_isHost != 0) {
                break;
            }
            m_538 = 1;
            break;

        case 0x419:
            if (m_isHost != 0) {
                break;
            }
            m_568 = 1;
            break;

        case 0x3fd:
            if (m_isHost != 0) {
                break;
            }
            m_5ac = 1;
            break;

        case 0x3fe:
            if (m_isHost != 0) {
                break;
            }
            m_56c = 1;
            break;

        case 0x41f:
            SendStatValue(sender, 0x420, msg->m_8, 0);
            break;

        case 0x420: {
            i32 stamp = msg->m_8;
            u32 now = timeGetTime();
            i32 delta = now - stamp;
            GruntzPlayer* player = static_cast<GruntzPlayer*>((g_gameReg)->FindOptionsSlot(sender));
            if (player == 0) {
                return 1;
            }
            i32 num = player->m_latency.m_avg * player->m_latency.m_count + delta;
            i32 np1 = player->m_latency.m_count + 1;
            player->m_latency.m_count = np1;
            player->m_latency.m_avg = num / np1;
            break;
        }

        case 0x421: {
            if (m_isHost == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>((g_gameReg)->FindOptionsSlot(sender));
            if (player == 0) {
                return 1;
            }
            m_channelLatency[player->m_playerIndex] = msg->m_8;
            break;
        }

        case 0x41d:
            m_verifyDone = 1;
            m_levelVerifyResult = 1;
            return 1;

        case 0x41e:
            m_levelVerifyResult = 0;
            m_verifyDone = 1;
            return 1;

        case 0x41c: {
            GruntzPlayer* player = static_cast<GruntzPlayer*>((g_gameReg)->FindOptionsSlot(sender));
            if (player == 0) {
                return 1;
            }
            m_recordAcked[player->m_playerIndex] = 1;
            m_recordToken[player->m_playerIndex] = msg->m_8;
            break;
        }

        case 0x402:
            m_lastSenderId = msg->m_8;
            m_584 = 1;
            return 1;

        case 0x403:
            if (m_isHost == 0) {
                break;
            }
            if (m_connected == 0) {
                break;
            }
            if (m_534 == 0) {
                break;
            }
            SendStatFlag(0x404, 1);
            OnOutOfSync();
            break;

        case 0x404:
            if (m_connected == 0) {
                break;
            }
            OnOutOfSync();
            break;

        case 0x407:
            if (m_connected == 0) {
                break;
            }
            OnMultiPause();
            break;

        case 0x415:
            if (m_isHost == 0) {
                break;
            }
            SaveConfig(pd);
            break;

        case 0x416:
            if (LoadConfig(msg) == 0) {
                break;
            }
            m_58c = 1;
            break;

        case 0x417:
            HandleVersionCheck(wire.m_version);
            break;

        case 0x418: {
            CString result;
            if (pd != 0) {
                CString name = pd->GetName();
                result.Format(
                    "*** %s has a different version of the game.",
                    static_cast<const char*>(name)
                );
            } else {
                result.Format("*** A player had a different version of the game.");
            }
            if (g_sharedFlag != 0) {
                NetLobby::AppendEditLine(
                    g_sharedFlag,
                    const_cast<char*>(static_cast<const char*>(result))
                );
            } else {
                (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))->AddItem(result, 0, 0x11);
            }
            break;
        }

        case 0x3f6:
            break;

        default:
            return 0;
    }
    return 1;
}

RVA(0x000ba170, 0x20)
CString CNetSessionNode::GetName() {
    return m_name;
}

// @early-stop
RVA(0x000ba1a0, 0x1a0)
i32 CMulti::HandleControlMsg(CNetCtrlMsg* msg, i32 unused) {
    if (msg == 0) {
        return 0;
    }

    switch (msg->m_code) {
        case 3:
            LoadMenuSelectSprite(msg);
            return 1;
        case 5:
            if (msg->m_subCode != 1) {
                return 1;
            }
            OnPlayerLeft(msg->m_playerId);
            g_playerLeftFlag = 1;
            return 1;
        case 0x31:
            m_isHost = 1;
            return 1;
        case 0x101:
            m_sessionTerminated = 1;
            return 1;
        default:
            return 0;
    }
}

RVA(0x000ba3b0, 0x17f)
i32 CMulti::OnPlayerLeft(i32 playerId) {
    CNetSessionNode* blob = static_cast<CNetSessionNode*>(Peer()->GetPlayerData(playerId));
    if (blob == LocalPlayer()) {
        return 0;
    }

    GruntzPlayer* slot = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(playerId));
    if (slot == 0) {
        return 0;
    }
    if (slot->m_liveGate == 0) {
        return 0;
    }
    if (slot->m_014 == 0) {
        return 0;
    }

    if (slot->m_030 != 0) {
        slot->m_030 = 0;
        g_activePlayerCount--;
    }
    slot->m_liveGate = 0;
    ChannelSlots_Set(slot->m_008, 1);

    CString line = slot->GetName() + " has left the game.";
    (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))
        ->AddItem(const_cast<char*>(static_cast<const char*>(line)), 0x20, 0x11);

    if (blob != 0) {
        Peer()->RemovePlayerObj(blob);
    }
    if (m_isHost != 0 && m_connected == 0) {
        BroadcastChannelTable(0);
        g_playerLeftFlag = 1;
    }
    return 1;
}

RVA(0x000ba590, 0x63)
void CMulti::AckDropPlayer(i32 id) {
    if (m_534 == 0) {
        RecordDropPlayer2(0, id);
        CNetCmdSlot* slot = Session()->FindCmdSlot(id);
        if (slot != 0) {
            slot->Touch();
            slot->FullReset();
            slot->m_state = 1;
            slot->m_desc->m_doneFlag = 1;
        }
        return;
    }

    OnPlayerLeft(id);
    ResetPlayerCommands(id);
}

RVA(0x000ba620, 0x14a)
i32 CMulti::LoadMenuSelectSprite(void* evp) {
    MenuSelectEvent* ev = static_cast<MenuSelectEvent*>(evp);
    if (ev == 0) {
        return 0;
    }
    if (ev->m_armed != 1) {
        return 0;
    }
    CNetSessionNode* node = static_cast<CNetSessionNode*>(Peer()->GetPlayerData(ev->m_id));
    if (node == 0) {

        node = Peer()->AddSessionNode(ev->m_id, ev->m_nameA, ev->m_nameB, 0);
        if (node == 0) {
            return 0;
        }
    }
    if (m_530 == 0 && m_connected == 0) {
        if (m_isHost != 0) {
            if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
                SendStat3(ev->m_id, 0x3fe, 1);
                return 0;
            }
            if (m_isHost != 0) {
                AnnounceVersion(node);
            }
        }
        CDDrawSubMgrLeafScan* host = m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* out = 0;
            host->m_cues.Lookup("GAME_MENUS_SELECT", out);
            LeafCue* e = static_cast<LeafCue*>(out);
            if (e != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if (static_cast<u32>((now - e->m_lastPlayTime)) >= e->m_replayDelay) {
                        e->m_lastPlayTime = now;
                        e->m_sound->ConfigureItem(tag, 0, 0, 0);
                    }
                }
            }
        }
        return 1;
    }
    SendStat3(ev->m_id, 0x3fd, 1);
    return 1;
}

RVA(0x000ba7d0, 0x2e)
i32 CMulti::ResolveLocalPlayer() {
    if (Peer() == 0) {
        return 0;
    }
    m_5bc = Peer()->FindPlayerById(m_hostIndex);
    return LocalPlayer() != 0;
}

RVA(0x000ba810, 0x11c)
i32 CMulti::BroadcastChannelTable(CNetSessionNode* recipient) {
    CNetChannelTablePacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= 0x80;
    packet.m_statId = STAT_CHANNEL_TABLE;

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* ch = &NetGameMgr()->m_options[i];
        if (ch != 0) {
            i32 v = ch->m_liveGate;
            packet.m_rows[i].m_liveGate = static_cast<u8>(v);
            v = ch->m_008;
            packet.m_rows[i].m_008 = static_cast<u8>(v);
            v = ch->m_014;
            packet.m_rows[i].m_014 = static_cast<u8>(v);
            v = ch->m_configId;
            packet.m_rows[i].m_configId = static_cast<u8>(v);
            v = ch->m_readyFlag;
            packet.m_rows[i].m_readyFlag = static_cast<u8>(v);
            v = ch->m_comboSel;
            packet.m_rows[i].m_comboSel = static_cast<u8>(v);
            packet.m_rows[i].m_slotKey = ch->m_slotKey;
            strcpy(packet.m_rows[i].m_name, static_cast<const char*>(ch->GetName()));
        }
    }

    if (recipient != 0) {
        if (SendStatPairRaw(recipient, &packet, sizeof(packet), 1) == 0) {
            return 0;
        }
    } else {
        if (SendStatFrom(&packet, sizeof(packet), 1) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000ba980, 0xca)
i32 CMulti::ParseChannelTable(void* packet) {
    if (packet == 0) {
        return 0;
    }
    if (m_isHost == 0) {
        ChannelSlots_InitAll();
    }

    CNetChannelTablePacket* p = static_cast<CNetChannelTablePacket*>(packet);
    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* ch = &NetGameMgr()->m_options[i];
        if (ch != 0) {
            ch->m_liveGate = p->m_rows[i].m_liveGate;
            ch->m_008 = p->m_rows[i].m_008;
            ch->m_014 = p->m_rows[i].m_014;
            ch->m_configId = p->m_rows[i].m_configId;
            if (p->m_rows[i].m_readyFlag != 0) {
                ch->m_readyFlag = 1;
            } else {
                ch->m_readyFlag = 0;
            }
            ch->m_comboSel = p->m_rows[i].m_comboSel;
            ch->m_name = p->m_rows[i].m_name;
            ch->m_slotKey = p->m_rows[i].m_slotKey;
            if (m_isHost == 0 && ch->m_liveGate != 0) {
                ChannelSlots_Set(ch->m_008, 0);
            }
        }
    }
    return 1;
}

RVA(0x000baa90, 0x20)
i32 CMulti::RegisterChannelFrom(const char* name, i32 b, i32 e, i32 f) {
    return RegisterChannel(name, b, 1, 0, e, f);
}

RVA(0x000baac0, 0x12e)
i32 CMulti::RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e) {
    if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
        return 0;
    }

    GruntzPlayer* ch = 0;
    if (idx >= 0 && idx <= 4) {
        ch = &NetGameMgr()->m_options[idx];
        if (ch != 0 && ch->m_liveGate != 0) {
            ch = 0;
        }
    }
    if (ch == 0) {

        i32 i;
        GruntzPlayer* p;
        for (i = 0, p = NetGameMgr()->m_options; i < 4; i++, p++) {
            ch = p;
            if (p != 0 && p->m_liveGate == 0) {
                break;
            }
            ch = 0;
        }
        if (ch == 0) {
            return 0;
        }
    }

    ChannelSlots_Set(id, 0);

    ch->m_name = CString(name);
    ch->m_008 = id;
    ch->m_014 = c;
    ch->m_configId = d;
    ch->m_readyFlag = 0;
    ch->m_slotKey = e;
    ch->m_liveGate = 1;
    ch->m_latency.m_avg = 0;
    ch->m_latency.m_count = 0;
    return 1;
}

RVA(0x000bac40, 0x38)
i32 CMulti::RegisterChannelRec(void* rec) {
    CNetChannelPacket* r = static_cast<CNetChannelPacket*>(rec);
    if (r->m_present != 0) {
        RegisterChannel(
            r->m_name,
            r->m_kind,
            r->m_slot,
            r->m_flagsB,
            r->m_configId,
            r->m_hostIndex
        );
    }
    return 1;
}

RVA(0x000bac90, 0x46)
i32 CMulti::RemoveChannel(i32 idx) {
    GruntzPlayer* ch = &NetGameMgr()->m_options[idx];
    if (ch == 0) {
        return 0;
    }
    if (ch->m_liveGate == 0) {
        return 0;
    }
    ch->m_liveGate = 0;
    ChannelSlots_Set(ch->m_008, 1);
    return 1;
}

RVA(0x000bad00, 0x2d)
i32 CMulti::OnPauseChannel() {
    if (m_connected == 0) {
        return 0;
    }
    SendStatFlag(STAT_PAUSE, 1);
    OnMultiPause();
    return 1;
}

RVA(0x000bad40, 0x6c)
void CMulti::OnMultiPause() {
    if (g_pauseGuard) {
        return;
    }

    m_584 = 0;
    g_pauseGuard = 1;
    i32 r = RunErrorDialog("MULTI_PAUSE", NetLobby::HostWaitDlgProc, 0);
    g_pauseGuard = 0;
    g_sharedFlag = 0;

    if (r == DISPATCH_RESYNC) {
        HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
        ::PostMessageA(hwnd, WM_COMMAND, 0x80d7, ResyncLParam());
    }
}

RVA(0x000badd0, 0x43)
void CMulti::OnMultiOptions() {
    if (g_optionzGuard) {
        return;
    }

    m_584 = 0;
    g_optionzGuard = 1;
    RunErrorDialog("MULTI_OPTIONZ", NetLobby::JoinWaitDlgProc, 0);
    g_optionzGuard = 0;
    g_sharedFlag = 0;
}

RVA(0x000bae40, 0x84)
void CMulti::OnOutOfSync() {
    if (m_574) {
        return;
    }

    m_574 = 1;
    m_584 = 0;
    i32 r = RunErrorDialog("MULTI_OUTOFSYNC", NetLobby::SessionWaitDlgProc, 0);
    g_sharedFlag = 0;

    switch (r) {
        case DISPATCH_RESYNC: {
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x80d7, ResyncLParam());
            break;
        }
        case DISPATCH_RESET:
            break;
        default: {
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
    }
}

RVA(0x000baf00, 0xb2)
i32 CMulti::BroadcastOneChannel(GruntzPlayer* ch) {
    CNetOneChannelPacket packet;
    memset(&packet, 0, 0x2c);
    packet.m_flags |= 0x80;
    packet.m_statId = STAT_CHANNEL_ONE;
    packet.m_playerIndex = ch->m_playerIndex;

    i32 v = ch->m_008;
    packet.m_008 = static_cast<u8>(v);
    v = ch->m_014;
    packet.m_014 = static_cast<u8>(v);
    v = ch->m_configId;
    packet.m_configId = static_cast<u8>(v);
    v = ch->m_readyFlag;
    packet.m_readyFlag = static_cast<u8>(v);
    packet.m_present = 1;
    v = ch->m_comboSel;
    packet.m_comboSel = static_cast<u8>(v);
    v = ch->m_slotKey;
    packet.m_slotKey = v;
    strcpy(packet.m_name, static_cast<const char*>(ch->GetName()));

    return SendStatFrom(&packet, 0x2c, 1);
}

RVA(0x000baff0, 0x88)
i32 CMulti::ParseOneChannel(void* rec) {
    if (rec == 0) {
        return 0;
    }
    CNetOneChannelPacket* r = static_cast<CNetOneChannelPacket*>(rec);
    i32 idx = r->m_playerIndex;
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    GruntzPlayer* ch = &NetGameMgr()->m_options[idx];
    if (ch == 0) {
        return 0;
    }

    ch->m_name = r->m_name;
    ch->m_008 = r->m_008;
    ch->m_configId = r->m_configId;
    if (r->m_readyFlag != 0) {
        ch->m_readyFlag = 1;
    } else {
        ch->m_readyFlag = 0;
    }
    ch->m_comboSel = r->m_comboSel;
    ch->m_014 = r->m_014;
    ch->m_slotKey = r->m_slotKey;
    ch->m_liveGate = 1;
    return 1;
}

RVA(0x000bb0b0, 0x44)
i32 CMulti::SendChannelStat422() {
    g_chanStat422_id = 0x422;
    g_chanStat422_flag |= 0x80;
    g_chanStat422_val = 0;
    Peer()->SetGroupDataFrom(LocalPlayer(), 1, &g_chanStat422_flag, 0xc);
    return 1;
}

RVA(0x000bb120, 0x44)
i32 CMulti::SendChannelStat423() {
    g_chanStat423_id = 0x423;
    g_chanStat423_flag |= 0x80;
    g_chanStat423_val = 0;
    Peer()->SetGroupDataFrom(LocalPlayer(), 1, &g_chanStat423_flag, 0xc);
    return 1;
}

// @early-stop
RVA(0x000bb190, 0x1c5)
i32 CMulti::BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd) {
    if (text == 0) {
        return 0;
    }
    if (text[0] == 0) {
        return 0;
    }

    i32 len = strlen(text);
    if (len > 0x80) {
        text[0x80] = 0;
        len = 0x80;
    }

    if (len > 0 && text[len - 1] < 0x20) {
        text[len - 1] = 0;
        len--;
    }
    if (len > 0 && text[len - 1] < 0x20) {
        text[len - 1] = 0;
    }

    char line[0x12c];
    if (toChat != 0) {

        sprintf(
            line,
            "%s: %s",
            static_cast<const char*>(
                static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(LocalPlayer()->m_id))->GetName()
            ),
            text
        );
    } else {
        strcpy(line, text);
    }

    if (showWnd != 0 && hWnd != 0) {
        NetLobby::AppendEditLine(static_cast<HWND>(hWnd), line);
    } else if (showWnd != 0) {

        GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(m_hostIndex));
        if (player == 0) {
            return 0;
        }
        (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))->AddItem(line, 0x30, player->m_008);
    }

    g_chatPacket_id = STAT_CHAT;

    i32 n = strlen(line);
    g_chatPacket_val = 0;
    strcpy(&g_chatPacket_buf, line);
    g_chatPacket_flag |= 0x80;
    Peer()->SetGroupDataFrom(LocalPlayer(), 1, &g_chatPacket_flag, n + 0xd);
    return 1;
}

namespace NetLobby {

    RVA(0x000bb3e0, 0xe5)
    void WINAPI AppendEditLine(HWND edit, char* str) {
        if (!edit || !str || !str[0]) {
            return;
        }
        i32 len = GetWindowTextLengthA(edit);
        if (len == 0) {
            SendMessageA(edit, 0xb1, len, -1);
        } else {
            SendMessageA(edit, 0xb1, len, len);
        }
        char buf[0x80];
        buf[0] = 0;
        if (len > 0) {
            strcat(buf, "\r\n");
        }
        strcat(buf, str);
        MsgParam text;
        text.m_str = buf;
        SendMessageA(edit, 0xc2, 0, text.m_lparam);
        SendMessageA(edit, 0xb6, 0, 0x270f);
    }
} // namespace NetLobby

// @early-stop
RVA(0x000bb510, 0x9d)
i32 CMulti::DropChannelPlayer(i32 idx) {
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    if (m_isHost == 0) {
        return 0;
    }

    GruntzPlayer* ch = &NetGameMgr()->m_options[idx];
    if (ch == 0) {
        return 0;
    }

    void* data = Peer()->GetPlayerData(ch->m_slotKey);

    i32 active = ch->m_014;
    if (data == 0) {
        if (active != 0) {
            return 0;
        }
    } else if (active != 0) {
        SendStatTo(static_cast<CNetSessionNode*>(data), STAT_CHANNEL_LEFT, 1);
    }

    if (RemoveChannel(idx) == 0) {
        return 0;
    }
    BroadcastChannelTable(0);
    g_playerLeftFlag = 1;
    return 1;
}

RVA(0x000bb5e0, 0xd9)
void CMulti::RecordDropPlayer2(CNetSessionNode* a, i32 id) {
    if (m_534 != 0) {
        return;
    }
    if (id == m_hostIndex) {
        return;
    }

    i32 count = m_604.GetSize();
    i32 i;
    for (i = 0; i < count; i++) {
        if (static_cast<i32>(m_604[i]) == id) {
            return;
        }
    }

    i32 slot = 0;
    while (slot < count) {
        if (m_604[slot] == 0) {
            break;
        }
        slot++;
    }
    if (slot >= count) {
        return;
    }
    m_604[slot] = id;

    i32 stateThree = 0;
    CNetCmdSlot* p = m_session->m_slots;
    for (i = 0; i < 4; i++) {
        if (p != 0 && p->m_state == 3) {
            stateThree++;
        }
        p++;
    }

    i32 total = m_604.GetSize();
    i32 recorded = 0;
    for (i = 0; i < total; i++) {
        if (m_604[i] != 0) {
            recorded++;
        }
    }
    if (recorded < stateThree) {
        return;
    }

    SendStatFlag(STAT_DROP_ANNOUNCE, 1);
    SendStatFlag(STAT_DROP_ANNOUNCE, 1);
    m_534 = 1;
}

// @early-stop
RVA(0x000bb700, 0x265)
i32 CMulti::WaitForOtherPlayers() {
    CDWordArray* votes = &m_604;
    votes->SetSize(0, -1);
    for (i32 k = 3; k != 0; k--) {
        votes->SetAtGrow(votes->GetSize(), 0);
    }
    if (Peer()->m_sessions.GetCount() == 1) {
        goto ready;
    }
    {
        i32 count = 0;
        CNetCmdSlot* slot = m_session->m_slots;
        for (i32 j = 4; j != 0; j--) {
            if (slot != 0 && slot->m_state == 3) {
                count++;
            }
            slot++;
        }
        if (count != 0) {
            SendStatFlag(0x3ed, 1);
            CString waitStr("Waiting for other playerz...");
            CGruntzMgr* g = g_gameReg;

            SIZE
            mode;
            mode.cx = g->m_modeW;
            mode.cy = g->m_modeH;
            RECT rc;
            rc.left = 0;
            rc.top = 0;
            rc.right = mode.cx;
            rc.bottom = mode.cy;
            EngStr_DrawText(g->m_world, &waitStr, &rc, 0x82, 1, 0xff, 0xff, 0, 1);

            i32 resend = 0x1388;
            i32 abort = 0x1d4c0;
            while (m_534 == 0) {
                u32 start = timeGetTime();
                Sleep(0x32);
                PollSession();
                if (GetAsyncKeyState(0x1b) & 0x80000000) {
                    return 0;
                }
                u32 elapsed = timeGetTime() - start;
                if (elapsed >= static_cast<u32>(resend)) {
                    resend = 0;
                } else {
                    resend -= elapsed;
                }
                if (elapsed >= static_cast<u32>(abort)) {
                    abort = 0;
                } else {
                    abort -= elapsed;
                }
                for (i32 i = 0; i < 4; i++) {
                    CNetCmdSlot* s = &m_session->m_slots[i];
                    if (s->m_state == 3) {
                        s->m_latency += elapsed;
                    }
                }
                if (abort == 0) {
                    DropTimeout();
                    abort = 0x1d4c0;
                }
                if (resend == 0) {
                    resend = 0x1388;
                    AckJoinFailure();
                    SendStatFlag(0x3ed, 1);
                }
            }

            g_scoreTimeBase = timeGetTime();

            if (g_gameReg->m_musicEnabled != 0) {
                char buf[0x40];
                wsprintfA(buf, "AMBIENT%d", GetAmbientId());
                NetGameMgr()->m_sound->PlayByName(buf, 1);
            }
            return 1;
        }
    }

ready:
    m_534 = 1;
    return 1;
}

RVA(0x000bba10, 0x1fb)
i32 CMulti::Poll(i32 token) {
    if (m_isHost == 0) {
        SendNetStat(STAT_VERIFY_REQUEST, token, 1);
        i32 resend = 0x1388;
        i32 abort = 0x3a98;
        m_verifyDone = 0;

        while (m_verifyDone == 0) {
            u32 start = timeGetTime();
            Sleep(0x32);
            PollSession();
            u32 elapsed = timeGetTime() - start;
            if (elapsed >= static_cast<u32>(resend)) {
                resend = 0;
            } else {
                resend -= elapsed;
            }
            if (elapsed >= static_cast<u32>(abort)) {
                abort = 0;
            } else {
                abort -= elapsed;
            }
            if (abort == 0) {
                return 0;
            }
            if (resend == 0) {
                resend = 0x1388;
                AckJoinFailure();
                SendNetStat(STAT_VERIFY_REQUEST, token, 1);
            }
        }
        return 1;
    }

    i32 abort = 0x3a98;
    m_verifyDone = 0;
    for (i32 i = 0; i < 4; i++) {
        m_recordAcked[i] = 0;
        m_recordToken[i] = 0;
    }
    while (m_verifyDone == 0) {
        u32 start = timeGetTime();
        Sleep(0x32);
        PollSession();
        u32 elapsed = timeGetTime() - start;
        if (elapsed >= static_cast<u32>(abort)) {
            abort = 0;
        } else {
            abort -= elapsed;
        }
        if (abort == 0) {
            return 0;
        }

        i32 allAcked = 1;
        i32 allAgree = 1;

        for (i32 i = 0; i < 4; i++) {
            GruntzPlayer* ch = &g_gameReg->m_options[i];
            if (ch->m_slotKey != m_hostIndex && ch->m_liveGate != 0 && ch->m_014 != 0) {
                if (m_recordAcked[i] == 0) {
                    allAcked = 0;
                } else if (!(m_recordToken[i] == token && token != 0)) {
                    allAgree = 0;
                }
            }
        }
        if (allAcked != 0) {
            if (allAgree != 0) {
                SendStatFlag(STAT_VERIFY_AGREE, 1);
                m_levelVerifyResult = 1;
                m_verifyDone = 1;
            } else {
                SendStatFlag(STAT_VERIFY_DISAGREE, 1);
                m_levelVerifyResult = 0;
                m_verifyDone = 1;
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000bbc90, 0x1b8)
i32 CMulti::CreateSession() {
    CNetPlayerListNode* rec = g_groupEnumMgr->m_playerSel;
    if (rec == 0) {
        return 0;
    }
    Peer()->EnumGroupsRange(rec, 0);
    if (ResolveLocalPlayer() == 0) {
        return 0;
    }

    CNetSession* session = new CNetSession();
    m_session = session;
    if (session == 0) {
        return 0;
    }
    if (session->Init(NetGameMgr(), this, Peer()) == 0) {
        return 0;
    }

    Session()->m_localDesc = LocalPlayer();
    i32 raw10 = m_session->m_tick;
    u8 b = static_cast<u8>(raw10);
    if (b == 0) {
        b = 0x7f;
    } else {
        b = b - 1;
    }
    m_curSlotId = b;

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* ch = &NetGameMgr()->m_options[i];
        i32 code = 1;
        if (ch->m_liveGate != 0 && ch->m_014 != 0) {

            code = 2;
            if (ch->m_slotKey != m_hostIndex) {
                code = 3;
            }
        }
        if (Session()->CreateSlot(i, code) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000bbec0, 0x81)
CNetCmdSlot::CNetCmdSlot() {
    m_state = 0;
    m_isRemote = 0;
    m_latchedSeq = 0;
    m_desc = 0;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    m_owner = 0;
    ClearCmds();

    for (i32 i = 0; i < 4; i++) {
        m_ackFlags[i] = 0;
    }
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// @early-stop
RVA(0x000bbf80, 0xb7)
void CNetSession::ResetAll() {
    m_mgr = 0;
    m_session = 0;
    m_netMgr = 0;
    m_localDesc = 0;
    m_tick = 0;
    m_snapshotDone = 0;
    m_seq = 0;
    m_period = 1;

    i32 i;
    CNetCmdSlot* slot = m_slots;
    for (i = 4; i != 0; i--) {
        slot->m_state = 0;
        slot->m_isRemote = 0;
        slot->m_latchedSeq = 0;
        slot->m_desc = 0;
        slot->m_latency = 0;
        slot->m_baseSeq = 0;
        slot->m_maxSeq = 0;
        slot->m_owner = 0;
        slot->ClearCmds();

        for (i32 k = 0; k < 4; k++) {
            slot->m_ackFlags[k] = 0;
        }
        slot->ResetTriple(slot->m_rangeA);
        slot->ResetTriple(slot->m_rangeB);
        slot++;
    }

    memset(m_idMap, 0, sizeof(m_idMap));

    GruntRec* e = m_records;
    for (i = 0x80; i != 0; i--) {
        e->m_seq = 0;
        e->m_count = 0;
        e->m_payloadLen = 0;
        e->m_checksum = 0;
        e++;
    }
}

// @early-stop
RVA(0x000bc070, 0x73)
u32 CMulti::FrameSyncWait() {
    u32 now = timeGetTime();
    u32 delta = now - m_5e4;
    u32 ret = 0;
    m_accumTime = delta;
    m_5e4 = now;

    if (delta <= 0x1e) {
        ActiveWait(0x1f - delta);
        m_5e4 = (now - m_accumTime) + 0x1f;
        return ret;
    }
    if (delta > 0x28 && m_syncGate) {
        ret = g_syncToggle ^ 1;
        g_syncToggle = ret;
    }
    return ret;
}

RVA(0x000bc110, 0xf6)
void CMulti::OnDropPlayer() {
    if (g_dropGuard) {
        return;
    }

    m_584 = 0;
    g_dropGuard = 1;
    i32 r = RunErrorDialog("MULTI_DROPPLAYER", NetLobby::NetGameDlgProc, 0);
    g_dropGuard = 0;
    g_sharedFlag = 0;

    switch (r) {
        case DISPATCH_RESET:
            Session()->ResetCmdBuffers();
            break;
        case DISPATCH_ABORT: {
            Session()->ResetCmdBuffers();
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
        case DISPATCH_PLAYERLEFT:
            if (g_dropPlayerId != -999) {
                if (Peer()->FindPlayerById(g_dropPlayerId)) {
                    SendStat3(g_dropPlayerId, STAT_PLAYERLEFT_LOCAL, 1);
                }
            }
            SendNetStat(STAT_PLAYERLEFT, g_dropPlayerId, 1);
            AckDropPlayer(g_dropPlayerId);
            Session()->ResetCmdBuffers();
            break;
    }
}

RVA(0x000bc250, 0x55)
i32 CMulti::RunErrorDialog(char* tmpl, DLGPROC handler, i32 lparam) {
    if (!Mgr()) {
        return 2;
    }
    Mgr()->m_cueSink->PauseAllVoices();
    i32 r = Mgr()->RunModalDialog(tmpl, handler, lparam);
    SetActiveAndFocus(Mgr()->m_gameWnd->m_hwnd);
    AckJoinFailure();
    return r;
}

RVA(0x000bc2d0, 0xd2)
void CMulti::DropTimeout() {
    if (m_session->FindSlot(0x1388) == 0) {
        return;
    }
    if (g_ackThrottleDeadline < static_cast<u32>(timeGetTime())) {
        AckJoinFailure();
        g_ackThrottleDeadline = timeGetTime() + 0x3e8;
    }
    CNetCmdSlot* slot = m_session->FindSlot(0x2710);
    if (slot == 0) {
        return;
    }
    g_dropPlayerId = slot->m_desc->m_slotKey;
    g_sessionName = slot->BuildHostName();
    SendNetStat(0x40c, g_dropPlayerId, 1);
    OnDropPlayer();
}

RVA(0x000bc3f0, 0x1e)
CString CNetCmdSlot::BuildHostName() {
    return m_desc->GetName();
}

RVA(0x000bc420, 0x2b)
void CMulti::AckJoinFailure() {
    if (m_netGate && m_5bc && m_connected) {
        SendStatFlag(0x3f6, 1);
    }
}

// @early-stop
RVA(0x000bc460, 0x24e)
i32 CMulti::SetupTcpIpConfig() {
    m_598 = "TcpIp";
    m_5ac = 0;
    m_5a4 = 5;
    m_drainReload = 0x3c;

    Utils::RegistryHelper* cfg = NetGameMgr()->m_settings;
    CString kDelay = m_598 + "_CmdDelay";
    CString kResend = m_598 + "_Resend";
    CString kDyn = m_598 + "_DynCmdDelay";
    i32 cd = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kDelay))), -1);
    i32 rs = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kResend))), -1);
    if (cd != -1 && rs != -1) {
        m_5a4 = cd;
        m_drainReload = rs;
    }

    GruntzPlayer* ch0 = NetGameMgr()->m_options;

    ch0->m_name = GetString5a0();
    ch0->m_008 = 0;

    m_5bc = static_cast<CNetSessionNode*>(Peer()->CreatePlayer(
        const_cast<char*>(static_cast<const char*>(ch0->GetName())),
        g_emptyString,
        0
    ));
    if (LocalPlayer() == 0) {
        ReportNetError(0);
        return 0;
    }

    m_hostIndex = LocalPlayer()->m_id;
    i32 chField = ch0->m_008;

    if (RegisterChannelFrom(ch0->GetName(), chField, -1, m_hostIndex) == 0) {
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x000bc750, 0x151)
i32 CMulti::CreateLocalPlayer() {
    {
        m_5bc = static_cast<CNetSessionNode*>(Peer()->CreatePlayer(
            const_cast<char*>(static_cast<const char*>(GetString5a0())),
            g_emptyString,
            0
        ));
    }
    if (LocalPlayer() == 0) {
        ReportNetError(0);
        return 0;
    }

    m_hostIndex = LocalPlayer()->m_id;
    if (WaitForConnect() == 0) {
        return 0;
    }

    CNetChannelPacket pkt;
    memset(&pkt, 0, 0x28);

    pkt.m_flags |= 0x80;
    pkt.m_statId = STAT_PLAYER_JOINED;
    pkt.m_present = 1;
    pkt.m_kind = 0;
    pkt.m_slot = 1;
    pkt.m_flagsB = 0;

    pkt.m_0e = 0;
    pkt.m_0d = 0xf;
    pkt.m_configId = 0x63;
    pkt.m_hostIndex = m_hostIndex;
    {
        strcpy(pkt.m_name, static_cast<const char*>(GetString5a0()));
    }
    SendStatFrom(&pkt, 0x28, 1);
    return 1;
}

RVA(0x000bc910, 0xf6)
i32 CMulti::OpenHostChannel(
    void* a0,
    const char* name,
    i32 channelId,
    i32 cmdDelay,
    i32 resend,
    i32 unused6,
    i32 unused7,
    i32 unused8
) {
    if (a0 == 0) {
        return 0;
    }
    m_5a4 = cmdDelay;
    m_drainReload = resend;
    m_levelIndex = 1;
    m_rngSeed = timeGetTime();
    m_5bc = Peer()->CreatePlayer(
        const_cast<char*>(static_cast<const char*>(GetString5a0())),
        g_emptyString,
        0
    );
    if (m_5bc == 0) {
        ReportNetError(0);
        return 0;
    }
    m_hostIndex = m_5bc->m_id;
    return RegisterChannelFrom(name, channelId, -1, m_hostIndex) != 0;
}

RVA(0x000bca50, 0x155)
i32 CMulti::WaitForConnect() {
    if (Peer() == 0) {
        return 0;
    }
    if (LocalPlayer() == 0) {
        return 0;
    }

    SendStatFlag(STAT_CONNECTING, 1);
    m_58c = 0;

    u32 deadline = timeGetTime() + 60000;

    while (m_58c == 0) {

        if (timeGetTime() > deadline
            || (static_cast<i32>(GetAsyncKeyState(VK_ESCAPE)) & 0x80000000)) {
            ReportStatusId(0x8022, 0);
            return 0;
        }
        PollSession();
        if (m_sessionTerminated) {
            ReportVersionMsg("The game session has been terminated.", 0);
            return 0;
        }
        if (m_538) {
            ReportVersionMsg("You have been removed from the game by the host.", 0);
            return 0;
        }
        if (m_5ac) {
            ReportVersionMsg("This game is closed.", 0);
            return 0;
        }
        if (m_56c) {
            ReportVersionMsg("This game is already full.", 0);
            return 0;
        }
        if (m_570) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            return 0;
        }
    }
    return 1;
}

RVA(0x000bcc10, 0x8e)
i32 CMulti::AutoTuneCmdDelay() {
    if (m_530 != 0) {
        return 1;
    }

    u32 ping = static_cast<u32>(GetMaxAckLatency());
    u32 tuned = ping / 30 + 2;
    i32 base = (tuned < 3) ? 3 : static_cast<i32>(tuned);

    i32 probe = Mgr()->CountReadyOptionsSlots(0);

    i32 bump = (probe > 2) ? 2 : 1;
    base += bump;
    m_5a4 = base;

    i32 resend;
    if (base <= 5) {
        resend = 0xa;
    } else {

        resend = (base <= 8 ? 0x14 : 0x1e);
    }
    m_drainReload = resend;
    return SaveConfig(0);
}

RVA(0x000bccd0, 0x141)
i32 CMulti::SaveConfig(CNetSessionNode* recipient) {
    CNetConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    blob.m_0 |= 0x80;
    blob.m_statId = STAT_CONFIG;
    blob.m_8 = m_5b0;
    {
        wsprintfA(blob.m_nameA, static_cast<const char*>(GetConfigNameA()));
    }
    {
        wsprintfA(blob.m_nameB, static_cast<const char*>(GetConfigNameB()));
    }
    blob.m_10c = m_5a4;
    blob.m_110 = m_drainReload;
    blob.m_114 = m_600;
    blob.m_118 = m_rngSeed;

    if (recipient != 0) {
        SendStatPairRaw(recipient, &blob, 0x11c, 1);
    } else {
        SendStatFrom(&blob, 0x11c, 1);
    }
    return 1;
}

RVA(0x000bce80, 0x77)
i32 CMulti::LoadConfig(void* cfg) {
    if (cfg == 0) {
        return 0;
    }

    CNetConfigBlob* c = static_cast<CNetConfigBlob*>(cfg);
    m_5b0 = c->m_8;
    m_5b4 = c->m_nameA;
    m_5b8 = c->m_nameB;
    m_5a4 = c->m_10c;
    m_drainReload = c->m_110;
    m_600 = c->m_114;
    m_rngSeed = c->m_118;
    return 1;
}

// @early-stop
RVA(0x000bcf20, 0xaf)
i32 CMulti::ResetPlayerCommands(i32 id) {
    if (m_connected == 0) {
        return 0;
    }

    CNetCmdSlot* slot = Session()->FindCmdSlot(id);
    if (slot == 0) {
        return 0;
    }
    if (slot->m_isRemote != 0) {
        return 0;
    }

    slot->Touch();
    i32 seq = (slot->m_baseSeq + 1) * static_cast<i32>(m_5a4);
    i32 end = seq + static_cast<i32>(m_5a4) * 3;
    for (; seq < end; seq++) {

        NetGameMgr()->m_cmdSubMgr->RemoveMatchingTarget(slot->m_desc->m_playerIndex, seq);
        slot->RemoveCmd(seq / static_cast<i32>(m_5a4));
    }
    slot->ResetTriple(slot->m_rangeA);
    slot->ResetTriple(slot->m_rangeB);
    return 1;
}

RVA(0x000bd000, 0x19)
void CMulti::ReportAckLatency() {
    u32 latency = GetMaxAckLatency();
    SendNetStat(STAT_ACKLATENCY, latency, 0);
}

RVA(0x000bd030, 0x5d)
u32 CMulti::GetMaxAckLatency() {
    u32 max = 0;

    if (m_isHost != 0) {
        for (i32 i = 0; i < 4; i++) {
            if (m_channelLatency[i] > max) {
                max = m_channelLatency[i];
            }
        }
    } else {

        CGruntzMgr* mgr = NetGameMgr();
        for (i32 i = 0; i < 4; i++) {
            if (mgr->m_options[i].m_014 && mgr->m_options[i].m_liveGate) {
                if (mgr->m_options[i].m_latency.m_avg > max) {
                    max = mgr->m_options[i].m_latency.m_avg;
                }
            }
        }
    }
    return max;
}

RVA(0x000bd0b0, 0x9a)
void CMulti::HandleVersionCheck(CNetVersionMsg* msg) {
    if (msg == 0) {
        return;
    }

    i32 mismatch = 0;
    if (g_localVersion != msg->m_localVersion) {
        mismatch = 1;
    }
    if (g_remoteVersion != msg->m_remoteVersion) {
        mismatch = 1;
    }

    if (mismatch) {
        i32 wasConnected = m_connected;
        m_570 = 1;
        if (wasConnected) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x8023, 0);
        }
    }
    if (mismatch) {
        SendStatFlag(STAT_VERSIONMISMATCH, 1);
        Sleep(0xfa);
    }
}

// @early-stop
RVA(0x000bd180, 0x66)
void CMulti::AnnounceVersion(CNetSessionNode* param) {
    CNetVersionPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.m_0 |= 0x80;
    packet.m_remoteVersion = g_remoteVersion;
    packet.m_buteConfig = g_buteMgrField4;
    packet.m_cfgWord = g_cfgWord;
    packet.m_localVersion = g_localVersion;
    packet.m_statId = STAT_VERSIONPACKET;

    SendStatPairRaw(param, &packet, 0x20, 1);
}

RVA(0x000bd210, 0x14d)
i32 CMulti::OnChar(i32 key, i32 flag) {
    if (m_hitTest && m_hitTest->m_10) {
        if (m_connected) {
            if (Mgr()->m_chatLog->TypeChar(key, flag)) {
                CString line = Mgr()->m_chatLog->GetInputText();
                i32 n = line.GetLength();
                if (n > 9) {
                    CString text = line.Right(n - 9);
                    char buf[0x100];
                    strcpy(buf, text);
                    BroadcastChatLine(buf, 1, 1, 0);
                    Mgr()->m_chatLog->m_inputText.Empty();
                }
            }
        }
        return 1;
    }
    return CPlay::OnChar(key, flag);
}
RVA(0x000bd3c0, 0x9)
void CMulti::TickStateMgrs() {
    m_mgr->TickStateMgrs();
}
