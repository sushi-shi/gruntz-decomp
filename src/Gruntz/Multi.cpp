#include <rva.h>

#include <Gruntz/Multi.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/MidiManager.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BracketValueParse.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/WorldSoundSet.h>
#include <Io/FileStream.h>
#include <MsgParam.h>
#include <Net/LobbyDialogs.h>
#include <Net/NetCmdSlotInline.h>
#include <Net/NetLobby.h>
#include <Net/NetLobbyCtrlId.h>
#include <Net/NetMgr.h>
#include <Net/NetMgrReportError.h>
#include <Net/NetMsgId.h>
#include <Net/NetPackets.h>
#include <Net/NetProviderNode.h>
#include <Net/NetSession.h>
#include <Net/NetSlotState.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Utils/DebugTiming.h>
#include <Utils/MapTyped.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/EngStr.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <dplay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// owner-TU unproven: bss sits in the gametext..gruntzcmdmgr window
DATA(0x00245550)
i32 g_cfgWord;

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
CNetChannelStatPacket g_optionsAbsentPacket;
DATA(0x00246fd8)
CNetChannelStatPacket g_optionsPresentPacket;
DATA(0x00248ce0)
HWND g_sharedFlag = NULL;
DATA(0x00248ce4)
i32 g_playerLeftFlag;
DATA(0x00248ce8)
i32 g_scoreTimeBase;
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

DATA(0x00248cec)
i32 g_activePlayerCount = 0;

RVA_DYNINIT(0x000b5360, 0xa, g_sessionName)
RVA_DYNINIT(0x000b5380, 0xa, g_sessionName)
RVA_DYNINIT(0x000b53a0, 0xe, g_sessionName)
RVA_DYNINIT(0x000b53c0, 0xa, g_sessionName)
DATA(0x002473d8)
CString g_sessionName;

DATA(0x002473e0)
CChatPacket g_chatPacket;

DATA(0x00248d00)
HWND g_sessionListHwnd;

DATA(0x00248cf0)
i32 g_hostServicesMode;

DATA(0x00248cf4)
CNetMgr* g_netMgr;
DATA(0x00248cf8)
CMulti* g_connectRptMgr;

// cl's CPtrList default-constructor closure: emitted as a COMDAT by every TU that
// default-constructs a CPtrList array element (multi, play, rezsync). Retail kept one
// copy at 0x85460 and routes `push OFFSET <closure>` through ILT thunk 0x3774; without
// the pin the delinker spells that address as `?GetMaxAckLatency@CMulti@@QAEIXZ+5`.
RVA_COMPGEN(0x00085460, 0x8, ??_FCPtrList@@QAEXXZ)

RVA(0x0008d270, 0x124)
CMulti::~CMulti() {
    CMulti::ReleaseResources();
}

RVA_DYNINIT(0x000b53e0, 0xa, g_gruntzLogFile)
RVA_DYNINIT(0x000b5400, 0xa, g_gruntzLogFile)
RVA_DYNINIT(0x000b5420, 0xe, g_gruntzLogFile)
RVA_DYNINIT(0x000b5440, 0xa, g_gruntzLogFile)
DATA(0x00246778)
CFile g_gruntzLogFile;
DATA(0x002467d8)
char g_recvBuffer[0x800];

// @early-stop
// Residue is one register-allocation tie-break that cascades: retail homes `this`
// in ebx and the constant 0 in ebp, cl does the reverse, so almost every line of
// the diff is that swap.
//
// The m_beginMarker cleanup here is NOT a content gap and NOT an inline-vs-not
// question about the header. ~CTileTriggerContainer is one inline definition that
// retail EXPANDS at this site and DECLINES at CPlay::LoadGameAssetNamespaces
// (0xc7ec0 holds a rel32 call to 0xc8640), so forcing it out of line satisfies
// CPlay by breaking this caller - measured and rejected. It is not the /Ob1
// budget either: cb(callee) titrated 22 -> ~134 and cb(caller) +336 leave the site
// expanded in BOTH callers, so what splits the two decisions in retail is still
// unidentified. Do not re-derive the budget reading; it is closed.
RVA(0x000b5460, 0x914)
i32 CMulti::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    g_gameReg->m_gameMode = GAMEMODE_MULTIPLAYER;
    if (mgr == NULL) {
        return 0;
    }

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }
    g_connectRptMgr = this;

    m_region0Gate = 0;
    m_region1Gate = 0;
    m_region2Gate = 0;
    m_viewMode = VIEW_MODE_IDLE;
    m_hudSuppressed = 1;
    m_cameraBookmarkIndex = -1;
    m_defeatCountdownActive = 0;
    m_scrollEdgeActive = 0;
    m_scrollEdgeLock = 0;
    m_customLevelVerificationPending = 0;
    m_allPlayersReady = 0;
    m_sessionTerminated = 0;
    m_removedByHost = 0;
    m_gameClosed = 0;
    m_pollAbort = 0;
    m_colorSelectionRejected = 0;
    m_gameFull = 0;
    m_outOfSync = 0;
    m_notifyLatch = 0;
    m_completedFinalLevel = 0;
    m_syncGate = 0;
    m_connected = 0;
    m_pumpGuard = 0;
    m_waitDialogReplyReceived = 0;
    m_lobbyLaunch = 0;
    m_versionMismatch = 0;
    m_initialFramePending = 1;
    m_localPlayer = NULL;
    m_localPlayerId = 0;
    m_commandDelay = 0;
    m_autoCommandDelay = 1;
    m_resendInterval = 0;
    m_lightFx = NULL;
    m_savedClock = 0;
    m_rngSeed = static_cast<i32>(timeGetTime());
    m_connectAccepted = 0;
    m_roundComplete = 0;

    for (i32 i = 0; i < 4; i++) {
        m_channelLatency[i] = 0;
        PlayerLatency* lat = &g_gameReg->m_options[i].m_latency;
        lat->Clear();
    }

    NetGameMgr()->m_loadingSaveGame = 0;
    Mgr()->ResetClockGlobals();
    Mgr()->ClearOptionsSlots();
    ChannelSlots_InitAll();

    CNetMgr* peer = new CNetMgr();
    m_netMgr = peer;
    g_netMgr = peer;

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
            while (ShowCursor(0) >= 0) {
            }
            return 0;
        }
    }

    if (m_isHost != 0) {
        m_connectAccepted = 1;
    }
    NetGameMgr()->m_modalBusy = 0;
    memset(&m_saveSlot, 0, sizeof(m_saveSlot));
    m_savedEffectsEnabled = NetGameMgr()->m_isEffectsEnabled;
    NetGameMgr()->m_isEffectsEnabled = 1;
    if (LoadImageBanks() == 0) {
        return 0;
    }
    PostLoadImageBanks();
    m_stateBank = m_symParser->ResolvePath("STATEZ_MULTI");
    if (m_stateBank == NULL) {
        return 0;
    }
    if (ShowMultiStartDlg() == 0) {
        return 0;
    }
    while (ShowCursor(0) >= 0) {
    }
    if (CreateSession() == 0) {
        return 0;
    }

    if (m_customLevel != 0) {
        NetGameMgr()->m_isMultiLevel = 0;
        NetGameMgr()->m_strWorldFile = "custom\\" + CustomLevelName();
    } else {
        NetGameMgr()->m_isMultiLevel = 1;
        NetGameMgr()->m_strWorldFile = BuiltInLevelName();
    }
    if (Mgr()->GetWorldFileName().GetLength() == 0) {
        return 0;
    }

    CChatBoxOwner* iface = new CChatBoxOwner();
    m_hitTest = iface;

    if (iface->Attach(m_world, NetGameMgr()->m_chatLog) == 0) {
        CChatBoxOwner* io = m_hitTest;
        if (io == NULL) {
            return 0;
        }
        io->Deactivate();
        ::operator delete(io);
        m_hitTest = NULL;
        return 0;
    }
    m_hitTest->m_inputActive = 0;
    m_hitTest->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);

    CStatusBarMgr* sess = new CStatusBarMgr;
    m_guts = sess;
    if (sess->LoadBattlezItemConfig(m_world) == 0) {
        if (m_guts == NULL) {
            return 0;
        }
        delete m_guts;
        m_guts = NULL;
        return 0;
    }

    CTileTriggerContainer* cmd = new CTileTriggerContainer();
    m_beginMarker = cmd;
    if (cmd->Initialize() == 0) {
        if (m_beginMarker == NULL) {
            return 0;
        }
        delete m_beginMarker;
        m_beginMarker = NULL;
        return 0;
    }

    if (LoadByMode(1, 1) == 0) {
        return 0;
    }
    m_pumpGuard = 1;
    m_allPlayersReady = 0;
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
    if (m_netMgr && m_localPlayer && m_session && m_connected) {
        SendNetStat(NETMSG_WAIT_DIALOG_REPLY, IDX(IDC_NET_RESUME), 1);
        SendStatFlag(NETMSG_PLAYER_LEFT, 1);
    }

    CNetSession* session = m_session;
    if (session) {
        delete session;
        m_session = NULL;
    }
    if (m_netMgr) {
        delete m_netMgr;
        m_netMgr = NULL;
    }

    CLightFxRender* lightFx = m_lightFx;
    if (lightFx) {
        lightFx->Reset();
        ::operator delete(lightFx);
        m_lightFx = NULL;
    }
    Mgr()->m_isEffectsEnabled = m_savedEffectsEnabled;

    CPlay::ReleaseResources();
}

RVA(0x000b6220, 0x54)
CNetSession::~CNetSession() {
    Shutdown();
}

RVA(0x000b62a0, 0x4a)
CNetCmdSlot::~CNetCmdSlot() {
    ResetSlot();
}

RVA(0x000b6310, 0x5)
void CMulti::OnExit() {
    CPlay::OnExit();
}

RVA(0x000b6330, 0x89)
i32 CMulti::EnterState(GameStateId previousState) {
    if (CPlay::EnterState(previousState) == GAMESTATE_NONE) {
        return 0;
    }
    m_mgr->RefreshGameClock();
    g_frameTime = m_savedClock;
    DWORD(WINAPI * tg)(void) = timeGetTime;
    m_drainTimer = 0;
    m_lastTime = tg();
    m_frameDelta = 0;
    m_reserved5ec = 0;
    m_reserved5e8 = 0;
    m_accumTime = 0;
    m_lastFrameSyncTime = tg();
    if (m_connected != 0) {
        SendNetStat(NETMSG_WAIT_DIALOG_REPLY, IDX(IDC_NET_RESUME), 1);
    }
    return 1;
}

RVA(0x000b63f0, 0x11b)
i32 CMulti::LeaveState(GameStateId nextState) {
    m_mgr->m_cueSink->PauseAllVoices();
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_notifyLatch) {
        QuitToMenu();
    }
    if (nextState != GAMESTATE_HELP) {
        RECT r;
        m_world->m_drawTarget->m_overlayPair->m_surface->Fill(0);
        CString s;
        s.LoadString(0x81a9);
        tagSIZE mode = m_mgr->GetModeSize();
        r.right = mode.cx;
        r.bottom = mode.cy;
        r.left = 0;
        r.top = 0;
        ShowHudMessage(m_world, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
        RetireScene(0x50, 0x3e8, 0, 1);
        if (m_mgr && m_mgr->m_cmdGrid) {
            m_mgr->m_cmdGrid->RemovePlayerUnitsImmediately(TM_ALL_PLAYERS);
        }
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
    GruntzPlayer* host = Mgr()->FindOptionsSlot(m_localPlayerId);
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
    m_reserved5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_reserved5ec = 0;
    m_reserved5e8 = 0;
    m_accumTime = 0;
    m_lastFrameSyncTime = timeGetTime();
    m_processedCommandTick = m_session->m_commandTick - 1;
    m_outOfSync = 0;

    if (CPlay::LoadByMode(mode, 0) == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; ++i) {
        GruntzPlayer* e = &Mgr()->m_options[i];
        if (e == NULL) {
            return 0;
        }
        e->m_battlezConfig.FreeArrays();
        if (e->m_battlezConfig.LoadConfig(Mgr(), i, e->m_configId) == 0) {
            return 0;
        }
        if (e->m_humanControlled && e->m_liveGate) {
            e->m_battlezConfig.Clear();
        }
    }
    ResetPlayState();
    srand(m_rngSeed);
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    m_reserved5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_reserved5ec = 0;
    m_reserved5e8 = 0;
    m_accumTime = 0;
    m_lastFrameSyncTime = timeGetTime();
    m_processedCommandTick = m_session->m_commandTick - 1;
    m_outOfSync = 0;
    Mgr()->m_chatLog->FreeNodes();
    m_session->ResetRound();
    Mgr()->m_cueSink->PauseAllVoices();
    return 1;
}

RVA(0x000b67f0, 0x74)
i32 CMulti::Connect(i32 mode) {
    m_connected = 0;
    m_allPlayersReady = 0;
    if (Mgr()->PassClickToPlayState(mode, 0, 0) == 0) {
        Mgr()->ReportError(IDX(IDS_SET_GAME_STATE), 0x446);
        return 0;
    }
    m_pumpGuard = 1;
    if (WaitForOtherPlayers() == 0) {
        m_pumpGuard = 0;
        return 0;
    }
    m_pumpGuard = 0;
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
    i32 newId = m_session->m_commandTick;
    if (m_processedCommandTick != newId) {
        m_processedCommandTick = newId;
        CGruntzCmdMgr* mgr = Mgr()->m_cmdSubMgr;
        CGruntzCommand* node;
        if (mgr->m_pendingLocalCommands.GetCount() == 0) {
            node = NULL;
        } else {
            node = static_cast<CGruntzCommand*>(mgr->m_pendingLocalCommands.RemoveHead());
        }
        if (node) {
            node->m_submitFlags = COMMAND_SUBMIT_SCHEDULED;

            i32 v = m_processedCommandTick + static_cast<i32>(m_commandDelay) * 2;
            node->m_scheduleSlot = static_cast<u8>(v % 128);
        }
        m_session->ScheduleCommand(node, static_cast<u8>(static_cast<u8>(m_commandDelay) << 1));
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
        m_packetsSent = m_session->SendTick();
        m_drainTimer = m_resendInterval;
    }
    i32 fin = 0;
    if (m_session->AdvanceTick() && m_pollAbort == 0) {
        fin = 1;
    }
    TickStateMgrs();
    CDDrawWorkerHost* mainPlane = m_world->m_level->m_mainPlane;
    if (mainPlane) {
        mainPlane->ActivateVisibleObjects();
    }

    if (fin != 0) {
        if (m_session->VerifyChecksums() == 0 && m_outOfSync == 0) {
            if (m_isHost != 0) {
                SendStatFlag(NETMSG_OUT_OF_SYNC, 1);
                OnOutOfSync();
                AdvanceGameFrame();
                m_drainTimer = 0;
                return 1;
            }
            SendStatFlag(NETMSG_OUT_OF_SYNC_REPORT, 1);
        }
        AdvanceGameFrame();
        m_drainTimer = 0;
        return 1;
    }
    RenderGameFrame();
    CheckDropTimeout();
    SoundStream* win = m_world->m_soundStream;
    if (win) {
        i32 now = timeGetTime();
        win->TickVolumeRamps(now);
        win->TickStreams(now);
    }
    ActiveWait(2);
    return 1;
}

// @early-stop
RVA(0x000b6b40, 0x29e)
i32 CMulti::AdvanceGameFrame() {
    i32 ready = FrameSyncWait();
    if (m_roundComplete == 0 && Mgr()->m_frameGate != 0 && ready == 0) {
        RenderGameFrame();
        return 1;
    }

    g_frameDelta = 0x21;
    g_lastNow += 0x21;
    g_frameTime += 0x21;
    g_killCueClock = g_lastNow;
    g_engineFrameDelta = 0x21;
    if (m_ambientInitDone == 0) {
        if (static_cast<i64>(g_frameTime) - m_ambientTiming.m_start.m_v
            >= m_ambientTiming.m_interval.m_v) {
            char name[0x40];
            wsprintfA(name, "AMBIENT%d", GetAmbientId());
            if (g_gameReg->m_musicEnabled != 0) {
                Mgr()->m_midi->PlaySequence(name, 1);
            } else {

                MidiManager* midi = Mgr()->m_midi;
                MidiSequence* sequence = midi->FindSequence(name);
                if (sequence) {
                    midi->m_currentSequence = sequence;
                }
                if (Mgr()->m_midi->m_currentSequence) {
                    Mgr()->m_midi->m_currentSequence->SetLooping(1);
                }
            }
            m_ambientInitDone = 1;
        }
    }
    Mgr()->m_cmdSubMgr->ExecuteScheduledCommands(m_processedCommandTick % 128);
    m_session->ComputeChecksum();
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
    m_guts->UpdateStatusBar(g_frameDelta);
    SoundStream* win = m_world->m_soundStream;
    if (win) {
        i32 now = timeGetTime();
        win->TickVolumeRamps(now);
        win->TickStreams(now);
    }
    m_beginMarker->UpdateTimedLogics(g_frameDelta);
    (static_cast<CMapMgr*>(Mgr()->m_tileGrid))->UpdateDiagonals(Mgr());
    if (ready == 0) {
        RenderGameFrame();
    }
    Mgr()->AdvanceOptionsCycle();
    return 1;
}

// @early-stop
// Residue is two `cmp dword ptr [eax],imm` that cl CSEs into one `mov ecx,[eax]`,
// plus where cl schedules the second `rc.top` store. NOTE: the local
// `CDDrawSurfaceMgr* mgr = m_world;` this used to open with was an invented local -
// retail reloads m_world at every use - and removing it took the function 83.4 -> 92.4.
RVA(0x000b6e90, 0x34d)
void CMulti::RenderGameFrame() {
    if (m_roundComplete == 0 && Mgr()->m_frameGate != 0) {
        StepInputA();
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->RenderAndPruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
        m_guts->LoadMainStatusBarSprite();
        CDDrawSurfacePair* h = static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
        if (h == NULL) {
            return;
        }
        AdvanceCursorAnimation(g_frameDelta);
        DrawCursorSaveUnder(h);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
        return;
    }
    StepInputA();
    StepViewportResize();
    if (m_region0Gate != 0) {
        (static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair))->m_surface->Fill(0);
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
        (m_world->m_level->m_mainPlane)->m_snappedX,
        (m_world->m_level->m_mainPlane)->m_snappedY
    );
    if (m_region1Gate != 0) {
        NotifyVisibleEntities();
    } else {
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->RenderAndPruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
    }
    m_guts->LoadMainStatusBarSprite();
    if (m_lightFx != NULL) {
        CStatusBarMgr* fx = m_guts;
        if (fx->m_position != STATUSBAR_HIDDEN && fx->m_activeTab != TAB_GAME) {
            RECT rc;
            if (fx->m_position == STATUSBAR_DOCK_LEFT) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                rc.top = g_gameReg->m_modeSize.cy;
                i32 right = g_gameReg->m_modeSize.cx - 20;
                i32 left = g_gameReg->m_modeSize.cx - 140;
                rc.top = g_gameReg->m_modeSize.cy;
                SetRect(&rc, left, 5, right, 125);
            }
            m_lightFx->Resize(static_cast<i32>(g_frameDelta), 0);
            m_lightFx->ComputeRect(
                static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair),
                &rc
            );
        }
    }
    Mgr()->m_chatLog->Scroll(g_frameDelta);
    CDDrawSurfacePair* h = static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
    if (h == NULL) {
        return;
    }
    m_hitTest->LoadChatBoxSprite(h);
    DrawDebugStats();
    Mgr()->m_cmdGrid->OverlayRelease();
    AdvanceCursorAnimation(g_frameDelta);
    DrawCursorSaveUnder(h);
    if (m_worldReady != 0) {
        h->DrawBox(&m_hudRect, 0xff);
    }
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
    UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);
    if (m_world->m_level->m_mainPlane != NULL) {
        (m_world->m_level->m_mainPlane)->DeactivateDistantObjects();
    }
    if (m_region0Gate != 0) {
        if (static_cast<i64>(g_frameTime) - m_region0Timing.m_start.m_v
            >= m_region0Timing.m_interval.m_v) {
            SetTinyViewportCurse(0);
        }
    }
    if (m_region1Gate != 0) {
        if (static_cast<i64>(g_frameTime) - m_region1Timing.m_start.m_v
            >= m_region1Timing.m_interval.m_v) {
            SetDarknessCurse(0);
        }
    }
}

// @early-stop
// Two-instruction schedule: retail keeps m_netMgr in eax and materialises the
// strcpy destination address before the m_selectedSession store; cl uses edx and sinks
// the lea past it.
RVA(0x000b72c0, 0x30b)
i32 CMulti::StartTitle() {
    Mgr()->m_lobbyResult = 0;
    m_lobbyLaunch = 1;
    if (!m_netMgr) {
        return 0;
    }
    CSymTab* saved = m_stateBank;
    CSymTab* st = m_symParser->ResolvePath("STATEZ_ATTRACT");
    m_stateBank = st;
    if (!st) {
        return 0;
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString title;
    title.Format("TITLE%d", idx);

    if (LoadAndPresentTitlePage(title, 0, 0, 1, 0) == 0) {
        m_stateBank = saved;
        return 0;
    }

    m_world->m_drawTarget->PresentBackPage();

    m_world->m_deviceManager->m_device->FlipToGDISurface();
    m_stateBank = saved;
    while (ShowCursor(1) < 0) {
    }
    IDirectPlayLobby* lobby = Mgr()->m_lobby;
    if (!lobby) {
        return 0;
    }
    LPDPLCONNECTION connection = Mgr()->m_connSettings;
    if (!connection) {
        return 0;
    }
    if (connection->dwFlags & DPLCONNECTION_CREATESESSION) {
        m_isHost = 1;
    } else {
        m_isHost = 0;
    }

    if (m_netMgr->Initialize(lobby, g_dplayAppGuid) == 0) {
        return 0;
    }
    m_netMgr->ClearSessionListings();
    CNetSessionListNode* session = m_netMgr->AddSessionListing(connection->lpSessionDesc);
    if (session == NULL) {
        return 0;
    }
    m_netMgr->m_selectedSession = session;
    char hostName[12];
    strcpy(hostName, connection->lpPlayerName->lpszShortNameA);
    hostName[10] = '\0';
    SetPlayerName(hostName);
    SetGameName(session->SessionName());

    if ((m_isHost ? SetupTcpIpConfig() : CreateLocalPlayer()) == false) {
        return 0;
    }
    return 1;
}

RVA(0x000b76a0, 0x4)
char* CNetSessionListNode::SessionName() {
    return m_sessionDesc.lpszSessionNameA;
}

RVA(0x000b76c0, 0x4f)
void CMulti::SetGameName(CString s) {
    m_gameName = s;
}

RVA(0x000b7730, 0x4f)
void CMulti::SetPlayerName(CString s) {
    m_playerName = s;
}

RVA(0x000b77a0, 0xb5)
i32 CMulti::Open() {
    if (!Network()) {
        return 0;
    }
    LoadAndPresentTitlePage("BACKGND", 0, 0, 1, 0);
    m_world->m_drawTarget->PresentBackPage();
    CNetProviderNode* provider = SelectNetworkProvider();
    if (!provider) {
        return 0;
    }
    if (!Network()->InitializeFromProvider(provider, g_dplayAppGuid.m_guid)) {
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

// @identity-TODO: adjacency to Open is the only evidence for the method's name.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000b7890, 0x1)
void CMulti::Close() {}

RVA(0x000b78b0, 0x17f)
CNetProviderNode* CMulti::SelectNetworkProvider() {
    if (Network()->EnumServiceProviders(0) != 0) {
        ReportNetError(0);
        return NULL;
    }

    if (g_hostServicesMode != 0) {
        if (RunErrorDialog("MULTI_HOSTSERVICES", NetSetupDlgProc, 0) != 0) {
            Utils::RegistryHelper* store = NetGameMgr()->m_settings;
            if (store != NULL && g_serviceId != NETSERVICE_NONE) {
                store->SetValueDword("Service", g_serviceId);
                {
                    store->SetValueString(
                        "Player Name",
                        const_cast<char*>(static_cast<const char*>(PlayerName()))
                    );
                }
                {
                    store->SetValueString(
                        "Game Name",
                        const_cast<char*>(static_cast<const char*>(GameName()))
                    );
                }
            }
        }
    } else {
        if (RunErrorDialog("MULTI_JOINSERVICES", NetSetupDlgProc, 0) != 0) {
            Utils::RegistryHelper* store = NetGameMgr()->m_settings;
            if (store != NULL) {
                if (g_serviceId != NETSERVICE_NONE) {
                    store->SetValueDword("Service", g_serviceId);
                }
                store->SetValueString(
                    "Player Name",
                    const_cast<char*>(static_cast<const char*>(PlayerName()))
                );
            }
        }
    }
    return Network()->m_selectedProvider;
}

RVA(0x000b7a90, 0x23)
CString CMulti::GameName() {
    return m_gameName;
}

RVA(0x000b7b10, 0x27c)
BOOL CALLBACK NetSetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {

    char nameBuf[0xa];
    char gameBuf[0x44];
    NetLobby::g_curDlg = hDlg;
    if (BlockScreenSaver(hDlg, msg, wParam, lParam) != 0) {
        return 1;
    }

    switch (msg) {
        case WM_INITDIALOG: {
            HWND combo = GetDlgItem(hDlg, 0x3fc);
            g_netMgr->m_selectedProvider = NULL;
            g_netMgr->PopulateProviderList(combo, 0);
            if (g_serviceId == NETSERVICE_NONE) {
                SendMessageA(combo, LB_SETCURSEL, 0, 0);
            } else if (static_cast<i32>(SendMessageA(combo, LB_SETCURSEL, g_serviceId, 0)) == -1) {
                SendMessageA(combo, LB_SETCURSEL, 0, 0);
            }

            DWORD cap = 0xa;
            g_gameReg->m_settings->GetValueString(
                const_cast<char*>(static_cast<const char*>(("Player Name"))),
                nameBuf,
                &cap,
                "Player"
            );
            cap = 0x40;
            g_gameReg->m_settings->GetValueString(
                const_cast<char*>(static_cast<const char*>(("Game Name"))),
                gameBuf,
                &cap,
                "Multiplayer Gruntz"
            );

            HWND edName = GetDlgItem(hDlg, 0x51b);
            SendMessageA(edName, EM_LIMITTEXT, 9, 0);
            SetDlgItemTextA(hDlg, 0x51b, nameBuf);
            HWND edGame = GetDlgItem(hDlg, 0x51c);
            SendMessageA(edGame, EM_LIMITTEXT, 0x3f, 0);
            SetDlgItemTextA(hDlg, 0x51c, gameBuf);
            return 1;
        }
        case WM_COMMAND:
            break;
        default:
            goto ret_false;
    }

    if (wParam == IDCANCEL) {
        EndDialog(hDlg, 0);
        return 1;
    }

    if (wParam == 1) {

        GetDlgItemTextA(hDlg, 0x51b, gameBuf, 0xa);
        if (gameBuf[0] == 0) {
            MessageBeep(0);
            return wParam;
        }
        g_connectRptMgr->SetPlayerName(CString(gameBuf));

        if (g_hostServicesMode != 0) {
            GetDlgItemTextA(hDlg, 0x51c, gameBuf, 0x40);
            if (gameBuf[0] == 0) {
                MessageBeep(0);
                return 1;
            }
            g_connectRptMgr->SetGameName(CString(gameBuf));
        }

        HWND combo = GetDlgItem(hDlg, 0x3fc);
        i32 svc = static_cast<i32>(SendMessageA(combo, LB_GETCURSEL, 0, 0));
        if (svc != -1) {
            g_serviceId = svc;
        }
        g_netMgr->ReadProviderSelection(GetDlgItem(hDlg, 0x3fc));
        EndDialog(hDlg, 1);
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
            strcpy(buf, "Error.");
        }
        ReportVersionMsg(buf, level);
    }
}

RVA(0x000b7f60, 0x52)
void CMulti::ReportNetError(i32 level) {
    char buf[512];
    // g_code is the HRESULT's LOW WORD (NetMgrReportError sets it with
    // `hr & 0xffff`), so the comparison masks the SDK constant the same way.
    // The guard reads: report the failure unless the user cancelled it.
    if (Mgr() && g_code != (DPERR_USERCANCEL & 0xffff)) {
        sprintf(buf, "Error: %s - %i", g_szCode, g_code);
        ReportVersionMsg(buf, level);
    }
}

RVA(0x000b7fe0, 0x2f)
i32 CMulti::JoinSession() {
    if (RunErrorDialog("MULTI_JOIN", MultiJoinDlgProc, 0) == 0) {
        return 0;
    }
    SendStatFlag(NETMSG_REQUEST_CHANNELS, 1);
    return 1;
}

RVA(0x000b8020, 0x22f)
BOOL CALLBACK MultiJoinDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hDlg;
    if (BlockScreenSaver(hDlg, msg, wParam, lParam) != 0) {
        goto ret_true;
    }
    switch (msg) {
        case WM_INITDIALOG:
            g_sessionListHwnd = GetDlgItem(hDlg, 0x3fc);
            if (g_sessionListHwnd == NULL) {
                goto close;
            }

            if (g_netMgr != NULL) {
                g_netMgr->m_selectedSession = NULL;
                SetTimer(hDlg, 1, 0x9c4, NULL);
                SendMessageA(hDlg, WM_TIMER, 0, 0);
                return 1;
            }
            goto close;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                KillTimer(hDlg, 1);
                EndDialog(hDlg, 0);
                return 1;
            }

            if (wParam == 1) {
                KillTimer(hDlg, 1);

                if ((static_cast<CMulti*>(g_connectRptMgr))->OnJoinConfirm(hDlg) == 0) {
                    MessageBeep(0);
                    i32 t = 0x7d0;
                    CNetProviderNode* provider = g_netMgr->m_selectedProvider;
                    if (provider && provider->IsTcpIpProvider()) {
                        t = 0x1388;
                    }
                    SetTimer(hDlg, 1, t, NULL);
                    return 0;
                }
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
        case WM_TIMER:
            KillTimer(hDlg, 1);
            {
                i32 sel = static_cast<i32>(SendMessageA(g_sessionListHwnd, LB_GETCURSEL, 0, 0));
                i32 hr = g_netMgr->EnumerateSessions(0, 0);
                if (hr == static_cast<i32>(0x88770118)) {
                    goto close;
                }
                if (hr != 0) {
                    if (g_connectRptMgr == NULL) {
                        goto close;
                    }
                    (static_cast<CMulti*>(g_connectRptMgr))->ReportNetError(0);
                    EndDialog(hDlg, 0);
                    return 1;
                }
                FillSessionList(g_sessionListHwnd, g_netMgr);
                if (sel != -1) {
                    SendMessageA(g_sessionListHwnd, LB_SETCURSEL, sel, 0);
                } else {
                    SendMessageA(g_sessionListHwnd, LB_SETCURSEL, 0, 0);
                }
                RefreshSessionSelection(hDlg, g_sessionListHwnd);
                i32 t = 0x7d0;
                CNetProviderNode* provider = g_netMgr->m_selectedProvider;
                if (provider && provider->IsTcpIpProvider()) {
                    t = 0x1388;
                }
                SetTimer(hDlg, 1, t, NULL);
            }
            return 1;
    }
ret_false:
    return 0;
close:
    EndDialog(hDlg, 0);
ret_true:
    return 1;
}

RVA(0x000b82e0, 0x230)
i32 CMulti::DetectConnectionConfig() {
    m_gameClosed = 0;
    CNetProviderNode* provider = Network()->m_selectedProvider;
    if (provider == NULL) {
        return 0;
    }

    m_providerConfigPrefix = "Other";
    if (provider->IsIpxProvider()) {
        m_providerConfigPrefix = "Ipx";
        m_commandDelay = 2;
        m_resendInterval = 0xa;
    } else if (provider->IsTcpIpProvider()) {
        m_providerConfigPrefix = "TcpIp";
        m_commandDelay = 3;
        m_resendInterval = 0xa;
    } else if (provider->IsModemProvider()) {
        m_providerConfigPrefix = "Modem";
        m_commandDelay = 4;
        m_resendInterval = 0x1e;
    } else if (provider->IsSerialProvider()) {
        m_providerConfigPrefix = "Serial";
        m_commandDelay = 2;
        m_resendInterval = 0xa;
    } else {
        m_commandDelay = 2;
        m_resendInterval = 0xa;
    }

    Utils::RegistryHelper* cfg = NetGameMgr()->m_settings;
    CString kDelay = m_providerConfigPrefix + "_CmdDelay";
    CString kResend = m_providerConfigPrefix + "_Resend";
    CString kDyn = m_providerConfigPrefix + "_DynCmdDelay";
    i32 cd = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kDelay))), -1);
    i32 rs = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kResend))), -1);
    if (cd != -1 && rs != -1) {
        m_commandDelay = cd;
        m_resendInterval = rs;
    }

    GruntzPlayer* ch0 = NetGameMgr()->m_options;

    ch0->m_name = PlayerName();
    ch0->m_colorIndex = TINT_ORANGE;

    CNetSessionListNode* r = JoinAndRegisterChannel();
    if (r == NULL) {
        return 0;
    }
    Network()->m_selectedSession = r;
    return 1;
}

RVA(0x000b85a0, 0xd2)
void CMulti::ApplyCmdDelayDefaults() {
    Utils::RegistryHelper* reg = g_gameReg->m_settings;

    CString cmdDelayName = m_providerConfigPrefix + "_CmdDelay";
    CString resendName = m_providerConfigPrefix + "_Resend";
    CString dynCmdName = m_providerConfigPrefix + "_DynCmdDelay";

    reg->SetValueDword(const_cast<char*>(static_cast<const char*>(cmdDelayName)), m_commandDelay);
    reg->SetValueDword(const_cast<char*>(static_cast<const char*>(resendName)), m_resendInterval);
}

RVA(0x000b86c0, 0x206)
i32 CMulti::ShowMultiStartDlg() {
    CMultiStartDlg dlg(m_mgr, NULL);
    i32 r = m_mgr->ExitModalUI(&dlg, 0);
    g_sharedFlag = NULL;
    if (r != 1) {
        if (m_isHost != 0) {
            GruntzPlayer* rec = m_mgr->FindOptionsSlot(m_localPlayerId);
            if (rec == NULL) {
                return 0;
            }
            rec->m_liveGate = 0;
            ChannelSlots_Set(IDX(rec->m_colorIndex), 1);
            BroadcastChannelTable(NULL);
        }
        if (m_isHost == 0 && m_removedByHost == 0) {
            SendStatFlag(NETMSG_PLAYER_LEFT, 1);
        }
        return 0;
    }

    if (m_isHost != 0) {
        ApplyCmdDelayDefaults();
    } else {
        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_emitGate == 0) {
            LeafCue* found = NULL;
            MapLookup(reg->m_cues, s_GameKey, found);
            // LeafCue::PlayIfElapsed inlined: the call's `this` copy holds the cue
            // in a register across the m_lastPlayTime store.
            LeafCue* rec = found;
            if (rec != NULL) {
                i32 snd = g_sndEnabled;
                i32 cue = g_sndCueTag;
                if (snd != 0) {
                    i32 clk = g_killCueClock;
                    if (static_cast<u32>((clk - rec->m_lastPlayTime))
                        >= static_cast<u32>(rec->m_replayDelay)) {
                        rec->m_lastPlayTime = clk;
                        rec->m_sound->AcquireAndPlay(cue, 0, 0, 0);
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
// Declined CSE in the loop tail: retail re-reads m_sessionCursor for GetNext's
// reference parameter where cl shares the guard's load. Flat across a local guard
// copy and a local inside the arm.
RVA(0x000b89e0, 0xc8)
void FillSessionList(HWND hList, CNetMgr* manager) {
    char buf[256];
    if (!hList) {
        return;
    }
    if (!manager) {
        return;
    }
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    manager->m_sessionCursor = manager->m_sessionListings.GetHeadPosition();
    CNetSessionListNode* listing =
        manager->m_sessionCursor != NULL
            ? static_cast<CNetSessionListNode*>(
                  manager->m_sessionListings.GetNext(manager->m_sessionCursor)
              )
            : NULL;
    while (listing) {

        MsgParam name;
        i32 itemIndex;
        if (ExtractBracketValue(buf, listing->m_sessionDesc.lpszSessionNameA, "NAME")) {
            name.m_str = buf;
            itemIndex = static_cast<i32>(SendMessageA(hList, LB_ADDSTRING, 0, name.m_lparam));
        } else {
            name.m_str = listing->m_sessionDesc.lpszSessionNameA;
            itemIndex = static_cast<i32>(SendMessageA(hList, LB_ADDSTRING, 0, name.m_lparam));
        }
        if (itemIndex != -1) {
            MsgParam cookie;
            cookie.m_sessionListing = listing;
            SendMessageA(hList, LB_SETITEMDATA, itemIndex, cookie.m_lparam);
        }

        if (manager->m_sessionCursor != NULL) {
            listing = static_cast<CNetSessionListNode*>(
                manager->m_sessionListings.GetAt(manager->m_sessionCursor)
            );
            manager->m_sessionListings.GetNext(manager->m_sessionCursor);
        } else {
            listing = NULL;
        }
    }
}

RVA(0x000b8af0, 0x1)
void RefreshSessionSelection(HWND hDlg, HWND hList) {}

// @early-stop
// Calls, CFG, and relocations agree. Retail keeps the enumeration result in EBP
// and the final failure flag in EBX; cl currently coalesces the flag onto dead
// ESI and keeps the result in EBX. Declaration splitting is byte-identical.
RVA(0x000b8b10, 0x175)
CNetSessionListNode* CMulti::JoinAndRegisterChannel() {
    char buf[0x100];
    buf[0] = ""[0];
    memset(&buf[1], 0, 0xff);
    MakeButeSectionKey(buf, "NAME", m_gameName);
    AppendInt(buf, "CMDDELAY", m_commandDelay);
    AppendInt(buf, "RESEND", m_resendInterval);
    AppendInt(buf, "LEVEL", SelectedLevelIndex());

    CNetSessionListNode* enumResult = g_netMgr->CreateSession(4, buf, 0, "");
    if (enumResult == NULL) {
        g_connectRptMgr->ReportNetError(0);
        return NULL;
    }

    CNetPlayerNode* node = Network()->CreatePlayer(const_cast<char*>("Host"), "", 0);
    m_localPlayer = node;
    if (node == NULL) {
        ReportNetError(0);
        return NULL;
    }

    m_localPlayerId = node->m_playerId;
    GruntzPlayer* ch0 = NetGameMgr()->m_options;
    ColorTint chField = static_cast<ColorTint>(ch0->m_colorIndex);

    i32 failed = (RegisterChannelFrom(ch0->GetName(), chField, -1, m_localPlayerId) == TINT_ORANGE);
    return failed ? NULL : enumResult;
}

// @early-stop
RVA(0x000b8cf0, 0x23b)
i32 CMulti::OnJoinConfirm(HWND hDlg) {
    if (hDlg == NULL) {
        return 0;
    }

    g_netMgr->ReadSessionSelection(GetDlgItem(hDlg, 0x3fc));
    CNetSessionListNode* sel = Network()->m_selectedSession;
    if (sel == NULL) {
        return 0;
    }

    m_localPlayer =
        Network()->JoinSessionAndCreatePlayer(sel, static_cast<const char*>(PlayerName()), "", 0);
    if (LocalPlayer() == NULL) {
        ReportNetError(0);
        return 0;
    }

    char buf[0x100];

    if (ExtractBracketValue(buf, sel->m_sessionDesc.lpszSessionNameA, "CMDDELAY")) {
        m_commandDelay = atoi(buf);
    }
    if (ExtractBracketValue(buf, sel->m_sessionDesc.lpszSessionNameA, "RESEND")) {
        m_resendInterval = atoi(buf);
    }
    if (ExtractBracketValue(buf, sel->m_sessionDesc.lpszSessionNameA, "NAME")) {
        SetGameName(CString(buf));
    }
    m_syncGate = 0;
    SelectedLevelIndex() = 1;
    m_localPlayerId = LocalPlayer()->m_playerId;
    if (ExtractBracketValue(buf, sel->m_sessionDesc.lpszSessionNameA, "LEVEL")) {
        SelectedLevelIndex() = atoi(buf);
    }

    CNetChannelPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= 0x80;
    packet.m_statId = NETMSG_READY_COUNT;

    packet.m_playerId = m_localPlayerId;
    packet.m_present = 1;
    packet.m_kind = 0;
    packet.m_slot = 1;
    packet.m_flagsB = 0;
    packet.m_configId = 0x63;
    packet.m_humanControlled = 0;
    packet.m_colorIndex = 0xf;
    strcpy(packet.m_name, PlayerName());
    SendStatFrom(&packet, sizeof(packet), 1);
    return 1;
}

RVA(0x000b8fc0, 0x151)
i32 CMulti::VerifyCustomLevel(CNetSessionListNode* session, CNetPlayerNode* localPlayer) {
    if (session == NULL) {
        goto notVerified;
    }
    if (localPlayer == NULL) {
        goto notVerified;
    }

    if (m_customLevelVerificationPending != 0) {
        i32 cfgId = m_customLevel;

        i32 token = (g_gameReg)->ResolveLevelChecksum(
            0,
            0,
            cfgId,
            0,
            cfgId != 0 ? CustomLevelName() : BuiltInLevelName()
        );

        g_connectRptMgr->m_levelVerifyResult = 0;
        if (g_connectRptMgr->Poll(token) == 0) {
            m_customLevelVerificationPending = 0;
            g_gameReg->EnterModalUI(
                "Unable to verify custom level with other players. The game will not start."
            );
            goto notVerified;
        }

        if (g_connectRptMgr->m_levelVerifyResult != 0) {
            return 1;
        }
        g_gameReg->EnterModalUI("Not all players have the (same) custom level.");
        m_customLevelVerificationPending = 0;
        goto notVerified;
    }
    PollSession();
notVerified:
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000b9180, 0x4a)
i32 CMulti::PollSessionGated(i32 sessionGate, i32 pollGate) {
    if (sessionGate == 0) {
        return 0;
    }
    if (pollGate == 0) {
        return 0;
    }
    if (m_allPlayersReady != 0) {
        return 1;
    }
    PollSession();
    return m_allPlayersReady != 0;
}

RVA(0x000b91f0, 0x31)
i32 CMulti::SendStatBuf(CNetStatPacket* packet, i32 flags) {
    packet->m_flags |= 0x80;
    i32 hr = Network()->BroadcastFrom(LocalPlayer(), flags, packet, 0x10);
    return hr == 0;
}

RVA(0x000b9240, 0x38)
void CMulti::SendStatFlag(NetMsgId id, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_flags |= 0x80;
    pkt.m_statId = id;
    pkt.m_value = LocalPlayer()->m_playerId;
    SendStatBuf(&pkt, flag);
}

RVA(0x000b9290, 0x32)
void CMulti::SendNetStat(NetMsgId id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_flags |= 0x80;
    pkt.m_statId = id;
    pkt.m_value = value;
    SendStatBuf(&pkt, flag);
}

RVA(0x000b92e0, 0x34)
i32 CMulti::SendStatFrom(void* packet, i32 packetSize, i32 flags) {
    if (packet == NULL) {
        return 0;
    }
    i32 hr = Network()->BroadcastFrom(LocalPlayer(), flags, packet, packetSize);
    return hr == 0;
}

RVA(0x000b9330, 0x41)
i32 CMulti::SendStatPair(CNetPlayerNode* recipient, CNetStatPacket* packet, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    packet->m_flags |= 0x80;
    i32 hr = Network()->Send(LocalPlayer(), recipient, flags, packet, 0x10);
    return hr == 0;
}

RVA(0x000b93a0, 0x47)
i32 CMulti::SendStatTo(CNetPlayerNode* recipient, NetMsgId messageId, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_flags |= 0x80;
    pkt.m_statId = messageId;
    pkt.m_value = LocalPlayer()->m_playerId;
    return SendStatPair(recipient, &pkt, flags);
}

RVA(0x000b9410, 0x51)
i32 CMulti::SendStat3(i32 recipientId, NetMsgId messageId, i32 flags) {
    CNetStatPacket pkt;
    pkt.m_flags |= 0x80;
    pkt.m_statId = messageId;
    pkt.m_value = LocalPlayer()->m_playerId;
    i32 hr = Network()->SendById(LocalPlayer()->m_playerId, recipientId, flags, &pkt, 0x10);
    return hr == 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000b9490, 0x42)
i32 CMulti::SendNetStatTo(CNetPlayerNode* recipient, i32 messageId, u32 value, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_flags |= 0x80;
    pkt.m_statId = static_cast<NetMsgId>(messageId);
    pkt.m_value = value;
    return SendStatPair(recipient, &pkt, flags);
}

RVA(0x000b9500, 0x46)
i32 CMulti::SendStatPairRaw(CNetPlayerNode* recipient, void* packet, i32 packetSize, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    if (packet == NULL) {
        return 0;
    }
    i32 hr = Network()->Send(LocalPlayer(), recipient, flags, packet, packetSize);
    return hr == 0;
}

RVA(0x000b9570, 0x53)
i32 CMulti::SendStatValue(i32 recipientId, NetMsgId messageId, i32 value, i32 flags) {
    CNetStatPacket pkt;
    pkt.m_flags |= 0x80;
    pkt.m_statId = messageId;
    pkt.m_value = value;
    i32 hr = Network()->SendById(LocalPlayer()->m_playerId, recipientId, flags, &pkt, 0x10);
    return hr == 0;
}

// @early-stop
// The loop shape is now retail's (`hr == 0` as a loop condition, the success path as
// the else of the ReportError arm). What is left is the second `LocalPlayer() == NULL`
// test, which retail keeps and cl folds away: retail re-compares the SAME already-null-
// checked ecx (`cmp ecx,edi / jne / xor ebx,ebx / jmp`), which makes `count` a phi and
// forces the extra `cmp ebx,edi` before the early return. The operand-swap lever of
// docs/patterns/redundant-test-elimination-is-syntactic.md does not reach it - swapping
// the loop test, the pre-loop test and Yoda-spelling the pointer compare were measured.
RVA(0x000b95f0, 0x10f)
i32 CMulti::PollSession() {
    if (LocalPlayer() == NULL) {
        return 0;
    }

    i32 count;
    if (LocalPlayer() == NULL) {
        count = 0;
    } else {
        IDirectPlay4A* directPlay = Network()->m_directPlay;

        DWORD messageCount;
        i32 hr = directPlay->GetMessageCount(LocalPlayer()->m_playerId, &messageCount);
        count = hr ? 0 : messageCount;
    }
    if (count <= 0) {
        return 0;
    }

    i32 dispatched;
    DPID sender;
    sender = 0;
    dispatched = 0;

    // `hr == 0` is a LOOP CONDITION in retail, not a `break`: the top tests only
    // `count > 0` and the back edge is `test edi,edi / je <top>` on the receive
    // status, with the error arm falling into the same re-test.
    i32 hr = 0;
    while (hr == 0 && count > 0) {
        if (m_pollAbort) {
            break;
        }

        DWORD messageSize = 0x800;
        DPID recipient = LocalPlayer()->m_playerId;
        IDirectPlay4A* directPlay = Network()->m_directPlay;

        hr = directPlay->Receive(&sender, &recipient, DPRECEIVE_ALL, g_recvBuffer, &messageSize);

        if (hr) {
            CNetMgr::ReportError("c:\\proj\\incs\\netmgr.h", 0x141, hr, NULL);
        } else {
            count--;
            if (sender != LocalPlayer()->m_playerId) {
                DispatchRecvMsg(sender, g_recvBuffer, messageSize);
                dispatched++;
            }
        }
    }
    return dispatched;
}

// @early-stop
RVA(0x000b9750, 0x810)
i32 CMulti::DispatchRecvMsg(i32 senderId, char* packet, i32 packetSize) {

    CNetWireMsg wire;
    wire.m_bytes = packet;
    CNetMsg* msg = wire.m_msg;
    if (msg == NULL) {
        return 0;
    }
    if (senderId == 0) {

        return HandleSystemMessage(wire.m_system, packetSize);
    }

    CNetPlayerNode* senderPlayer = Network()->GetPlayerNodeData(senderId);
    if (m_connected != 0 || m_pumpGuard != 0) {
        if (senderPlayer != NULL) {
            CNetCmdSlot* slot = Session()->FindSlotByPlayerId(senderPlayer->m_playerId);
            if (slot != NULL) {
                slot->m_latency = 0;
            }
        }
    }

    if ((msg->m_flags & 0x80) == 0) {
        return 0;
    }

    switch (msg->m_msgId) {
        case NETMSG_ALL_PLAYERS_READY:
            m_allPlayersReady = 1;
            return 1;

        case NETMSG_VERIFY_CUSTOM_LEVEL:
            m_customLevelVerificationPending = 1;
            return 1;

        case NETMSG_DROP_PLAYER:
            if (m_allPlayersReady != 0) {
                break;
            }
            RecordDropAcknowledgement(senderPlayer, senderId);
            break;

        case NETMSG_OPTIONS_PRESENT: {
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(senderId));
            if (player == NULL) {
                return 1;
            }
            if (player->m_presenceCounted == 0) {
                player->m_presenceCounted = 1;
                g_activePlayerCount++;
            }
            OnMultiOptions();
            break;
        }

        case NETMSG_OPTIONS_ABSENT: {
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(senderId));
            if (player == NULL) {
                return 1;
            }
            if (player->m_presenceCounted == 0) {
                break;
            }
            player->m_presenceCounted = 0;
            g_activePlayerCount--;
            break;
        }

        case NETMSG_CHAT_LINE: {
            char* text = msg->m_payload;
            if (g_sharedFlag != NULL) {
                AppendEditLine(g_sharedFlag, text);
                break;
            }
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(senderId));
            if (player == NULL) {
                return 1;
            }
            (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))
                ->AddItem(text, 0x30, IDX(player->m_colorIndex));
            CDDrawSubMgrLeafScan* host = m_world->m_soundRegistry;
            if (host->m_emitGate != 0) {
                break;
            }
            LeafCue* e = NULL;
            MapLookup(host->m_cues, "GAME_CHAT", e);
            if (e == NULL) {
                break;
            }
            e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            break;
        }

        case NETMSG_POLL_ABORT:
            if (m_pollAbort != 0) {
                break;
            }
            ReportVersionMsg("You have been dropped from the game.", 0);
            PostMessageA(NetGameMgr()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            m_pollAbort = 1;
            break;

        case NETMSG_DROP_PLAYER_ACK:
            AckDropPlayer(msg->m_value);
            break;

        case NETMSG_PLAYER_LEFT:
            OnPlayerLeft(senderId);
            ResetPlayerCommands(senderId);
            return g_playerLeftFlag = 1;

        case NETMSG_REQUEST_CHANNELS:
            if (m_isHost == 0) {
                break;
            }
            BroadcastChannelTable(senderPlayer);
            break;

        case NETMSG_CHANNEL_TABLE:
            if (m_isHost != 0) {
                break;
            }
            ApplyChannelTable(wire.m_chanTable);
            g_playerLeftFlag = 1;
            break;

        case NETMSG_READY_COUNT: {
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
            RegisterChannelFromPacket(chan);
            BroadcastChannelTable(NULL);
            SaveConfig(senderPlayer);
            g_playerLeftFlag = 1;
            break;
        }

        case NETMSG_SWAP_CHANNEL: {
            if (m_isHost == 0) {
                break;
            }
            if (m_connected != 0) {
                break;
            }
            CNetOneChannelPacket* chan = wire.m_oneChannel;
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(chan->m_slotKey));
            if (player == NULL) {
                return 0;
            }
            if (player->SwapChannel(static_cast<ColorTint>(chan->m_colorIndex)) == TINT_ORANGE) {
                ColorTint colour = static_cast<ColorTint>(player->m_colorIndex);
                chan->m_colorIndex = static_cast<u8>(IDX(colour));
                SendStatTo(senderPlayer, NETMSG_COLOR_REJECTED, 1);
            }
            ApplyChannelUpdate(chan);
            BroadcastChannelTable(NULL);
            g_playerLeftFlag = 1;
            break;
        }

        case NETMSG_REMOVED_BY_HOST:
            if (m_isHost != 0) {
                break;
            }
            m_removedByHost = 1;
            break;

        case NETMSG_COLOR_REJECTED:
            if (m_isHost != 0) {
                break;
            }
            m_colorSelectionRejected = 1;
            break;

        case NETMSG_GAME_CLOSED:
            if (m_isHost != 0) {
                break;
            }
            m_gameClosed = 1;
            break;

        case NETMSG_GAME_FULL:
            if (m_isHost != 0) {
                break;
            }
            m_gameFull = 1;
            break;

        case NETMSG_STAT_REQUEST:
            SendStatValue(senderId, NETMSG_STAT_VALUE, msg->m_value, 0);
            break;

        case NETMSG_STAT_VALUE: {
            i32 stamp = msg->m_value;
            i32 delta = timeGetTime();
            delta -= stamp;
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>((g_gameReg)->FindOptionsSlot(senderId));
            if (player == NULL) {
                return 1;
            }
            i32 num = player->m_latency.m_avg * player->m_latency.m_count + delta;
            i32 np1 = player->m_latency.m_count + 1;
            player->m_latency.m_count = np1;
            player->m_latency.m_avg = num / np1;
            break;
        }

        case NETMSG_STAT_DONE: {
            if (m_isHost == 0) {
                break;
            }
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>((g_gameReg)->FindOptionsSlot(senderId));
            if (player == NULL) {
                return 1;
            }
            m_channelLatency[player->m_playerIndex] = msg->m_value;
            break;
        }

        case NETMSG_VERIFY_OK:
            m_verifyDone = 1;
            m_levelVerifyResult = 1;
            return 1;

        case NETMSG_VERIFY_FAILED:
            m_levelVerifyResult = 0;
            m_verifyDone = 1;
            return 1;

        case NETMSG_SLOT_QUERY: {
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>((g_gameReg)->FindOptionsSlot(senderId));
            if (player == NULL) {
                return 1;
            }
            m_levelChecksumReceived[player->m_playerIndex] = 1;
            m_levelChecksums[player->m_playerIndex] = msg->m_value;
            break;
        }

        case NETMSG_WAIT_DIALOG_REPLY:
            m_lastSenderId = msg->m_value;
            m_waitDialogReplyReceived = 1;
            return 1;

        case NETMSG_OUT_OF_SYNC_REPORT:
            if (m_isHost == 0) {
                break;
            }
            if (m_connected == 0) {
                break;
            }
            if (m_allPlayersReady == 0) {
                break;
            }
            SendStatFlag(NETMSG_OUT_OF_SYNC, 1);
            OnOutOfSync();
            break;

        case NETMSG_OUT_OF_SYNC:
            if (m_connected == 0) {
                break;
            }
            OnOutOfSync();
            break;

        case NETMSG_PAUSE:
            if (m_connected == 0) {
                break;
            }
            OnMultiPause();
            break;

        case NETMSG_SAVE_CONFIG:
            if (m_isHost == 0) {
                break;
            }
            SaveConfig(senderPlayer);
            break;

        case NETMSG_LOAD_CONFIG:
            if (LoadConfig(wire.m_config) == 0) {
                break;
            }
            m_connectAccepted = 1;
            break;

        case NETMSG_VERSION_CHECK:
            HandleVersionCheck(wire.m_version);
            break;

        case NETMSG_PLAYER_NAME: {
            CString result;
            if (senderPlayer != NULL) {
                result.Format(
                    "*** %s has a different version of the game.",
                    static_cast<const char*>(senderPlayer->ShortName())
                );
            } else {
                result.Format("*** A player had a different version of the game.");
            }
            if (g_sharedFlag != NULL) {
                AppendEditLine(g_sharedFlag, const_cast<char*>(static_cast<const char*>(result)));
            } else {
                (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))->AddItem(result, 0, 0x11);
            }
            break;
        }

        case NETMSG_LOBBY_TICK:
            break;

        default:
            return 0;
    }
    return 1;
}

RVA(0x000ba170, 0x20)
CString CNetPlayerNode::ShortName() {
    return m_shortName;
}

RVA(0x000ba1a0, 0x1a0)
i32 CMulti::HandleSystemMessage(LPDPMSG_GENERIC message, i32 unusedMessageSize) {
    if (message == NULL) {
        return 0;
    }

    // DirectPlay system message ids, from the SDK's dplay.h. The retail byte
    // index table (0xba238) maps code 0x101 -> the +0x528 arm and 0x31 -> the
    // +0x52c arm, so DPSYS_HOST sets m_isHost and DPSYS_SESSIONLOST sets
    // m_sessionTerminated - the labels, not the member names, were transposed.
    switch (message->dwType) {
        case DPSYS_DESTROYPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_system = message;
            if (wire.m_playerDestroyed->dwPlayerType != DPPLAYERTYPE_PLAYER) {
                return 1;
            }
            OnPlayerLeft(wire.m_playerDestroyed->dpId);
            g_playerLeftFlag = 1;
            return 1;
        }
        case DPSYS_CREATEPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_system = message;
            HandlePlayerCreated(wire.m_playerCreated);
            return 1;
        }
        case DPSYS_HOST:
            m_isHost = 1;
            return 1;
        case DPSYS_SESSIONLOST:
            m_sessionTerminated = 1;
            return 1;
        default:
            return 0;
    }
}

RVA(0x000ba3b0, 0x17f)
i32 CMulti::OnPlayerLeft(i32 playerId) {
    CNetPlayerNode* player = Network()->GetPlayerNodeData(playerId);
    if (player == LocalPlayer()) {
        return 0;
    }

    GruntzPlayer* slot = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(playerId));
    if (slot == NULL) {
        return 0;
    }
    if (slot->m_liveGate == 0) {
        return 0;
    }
    if (slot->m_humanControlled == 0) {
        return 0;
    }

    if (slot->m_presenceCounted != 0) {
        slot->m_presenceCounted = 0;
        g_activePlayerCount--;
    }
    slot->m_liveGate = 0;
    ChannelSlots_Set(IDX(slot->m_colorIndex), 1);

    CString line = slot->GetName() + " has left the game.";
    (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))
        ->AddItem(const_cast<char*>(static_cast<const char*>(line)), 0x20, 0x11);

    if (player != NULL) {
        Network()->RemovePlayer(player);
    }
    if (m_isHost != 0 && m_connected == 0) {
        BroadcastChannelTable(NULL);
        g_playerLeftFlag = 1;
    }
    return 1;
}

RVA(0x000ba590, 0x63)
void CMulti::AckDropPlayer(i32 playerId) {
    if (m_allPlayersReady == 0) {
        RecordDropAcknowledgement(NULL, playerId);
        CNetCmdSlot* slot = Session()->FindSlotByPlayerId(playerId);
        if (slot != NULL) {
            slot->BeginDrain();
            slot->ClearSyncState();
            slot->m_state = NETSLOT_DONE;
            slot->m_player->m_doneFlag = 1;
        }
        return;
    }

    OnPlayerLeft(playerId);
    ResetPlayerCommands(playerId);
}

RVA(0x000ba620, 0x14a)
i32 CMulti::HandlePlayerCreated(LPDPMSG_CREATEPLAYERORGROUP message) {
    if (message == NULL) {
        return 0;
    }
    if (message->dwPlayerType != DPPLAYERTYPE_PLAYER) {
        return 0;
    }
    CNetPlayerNode* player = Network()->GetPlayerNodeData(message->dpId);
    if (player == NULL) {

        player = Network()->AddPlayer(
            message->dpId,
            message->dpnName.lpszShortNameA,
            message->dpnName.lpszLongNameA,
            0
        );
        if (player == NULL) {
            return 0;
        }
    }
    if (m_customLevelVerificationPending == 0 && m_connected == 0) {
        if (m_isHost != 0) {
            if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
                SendStat3(message->dpId, NETMSG_GAME_FULL, 1);
                return 0;
            }
            if (m_isHost != 0) {
                AnnounceVersion(player);
            }
        }
        CDDrawSubMgrLeafScan* host = m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            LeafCue* found = NULL;
            MapLookup(host->m_cues, "GAME_MENUS_SELECT", found);
            // LeafCue::PlayIfElapsed inlined: the call's `this` copy holds the cue
            // in a register across the m_lastPlayTime store.
            LeafCue* e = found;
            if (e != NULL) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if (static_cast<u32>((now - e->m_lastPlayTime)) >= e->m_replayDelay) {
                        e->m_lastPlayTime = now;
                        e->m_sound->AcquireAndPlay(tag, 0, 0, 0);
                    }
                }
            }
        }
        return 1;
    }
    SendStat3(message->dpId, NETMSG_GAME_CLOSED, 1);
    return 1;
}

RVA(0x000ba7d0, 0x2e)
i32 CMulti::ResolveLocalPlayer() {
    if (Network() == NULL) {
        return 0;
    }
    m_localPlayer = Network()->FindPlayerById(m_localPlayerId);
    return LocalPlayer() != NULL;
}

RVA(0x000ba810, 0x11c)
i32 CMulti::BroadcastChannelTable(CNetPlayerNode* recipient) {
    CNetChannelTablePacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= 0x80;
    packet.m_statId = STAT_CHANNEL_TABLE;

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &NetGameMgr()->m_options[i];
        if (player != NULL) {
            i32 v = player->m_liveGate;
            packet.m_rows[i].m_liveGate = static_cast<u8>(v);
            v = player->m_colorIndex;
            packet.m_rows[i].m_colorIndex = static_cast<u8>(v);
            v = player->m_humanControlled;
            packet.m_rows[i].m_humanControlled = static_cast<u8>(v);
            v = player->m_configId;
            packet.m_rows[i].m_configId = static_cast<u8>(v);
            v = player->m_readyFlag;
            packet.m_rows[i].m_readyFlag = static_cast<u8>(v);
            v = player->m_comboSel;
            packet.m_rows[i].m_comboSel = static_cast<u8>(v);
            packet.m_rows[i].m_slotKey = player->m_slotKey;
            strcpy(packet.m_rows[i].m_name, static_cast<const char*>(player->GetName()));
        }
    }

    if (recipient != NULL) {
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
i32 CMulti::ApplyChannelTable(CNetChannelTablePacket* packet) {
    if (packet == NULL) {
        return 0;
    }
    if (m_isHost == 0) {
        ChannelSlots_InitAll();
    }

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &NetGameMgr()->m_options[i];
        if (player != NULL) {
            player->m_liveGate = packet->m_rows[i].m_liveGate;
            player->m_colorIndex = static_cast<ColorTint>(packet->m_rows[i].m_colorIndex);
            player->m_humanControlled = packet->m_rows[i].m_humanControlled;
            player->m_configId = packet->m_rows[i].m_configId;
            if (packet->m_rows[i].m_readyFlag != 0) {
                player->m_readyFlag = 1;
            } else {
                player->m_readyFlag = 0;
            }
            player->m_comboSel = packet->m_rows[i].m_comboSel;
            player->m_name = packet->m_rows[i].m_name;
            player->m_slotKey = packet->m_rows[i].m_slotKey;
            if (m_isHost == 0 && player->m_liveGate != 0) {
                ChannelSlots_Set(IDX(player->m_colorIndex), 0);
            }
        }
    }
    return 1;
}

RVA(0x000baa90, 0x20)
i32 CMulti::RegisterChannelFrom(
    const char* name,
    ColorTint color,
    i32 preferredIndex,
    i32 playerId
) {
    return RegisterChannel(name, color, 1, 0, preferredIndex, playerId);
}

RVA(0x000baac0, 0x12e)
i32 CMulti::RegisterChannel(
    const char* name,
    ColorTint color,
    i32 humanControlled,
    i32 configId,
    i32 preferredIndex,
    i32 playerId
) {
    if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
        return 0;
    }

    GruntzPlayer* slot = NULL;
    if (preferredIndex >= 0 && preferredIndex <= 4) {
        slot = &NetGameMgr()->m_options[preferredIndex];
        if (slot != NULL && slot->m_liveGate != 0) {
            slot = NULL;
        }
    }
    if (slot == NULL) {

        i32 i;
        GruntzPlayer* candidate;
        for (i = 0, candidate = NetGameMgr()->m_options; i < 4; i++, candidate++) {
            slot = candidate;
            if (candidate != NULL && candidate->m_liveGate == 0) {
                break;
            }
            slot = NULL;
        }
        if (slot == NULL) {
            return 0;
        }
    }

    ChannelSlots_Set(IDX(color), 0);

    slot->m_name = CString(name);
    slot->m_colorIndex = color;
    slot->m_humanControlled = humanControlled;
    slot->m_configId = configId;
    slot->m_readyFlag = 0;
    slot->m_slotKey = playerId;
    slot->m_liveGate = 1;
    slot->m_latency.Clear();
    return 1;
}

RVA(0x000bac40, 0x38)
i32 CMulti::RegisterChannelFromPacket(CNetChannelPacket* packet) {
    if (packet->m_present != 0) {
        RegisterChannel(
            packet->m_name,
            static_cast<ColorTint>(packet->m_kind),
            packet->m_slot,
            packet->m_flagsB,
            packet->m_configId,
            packet->m_playerId
        );
    }
    return 1;
}

RVA(0x000bac90, 0x46)
i32 CMulti::RemoveChannel(i32 slotIndex) {
    GruntzPlayer* ch = &NetGameMgr()->m_options[slotIndex];
    if (ch == NULL) {
        return 0;
    }
    if (ch->m_liveGate == 0) {
        return 0;
    }
    ch->m_liveGate = 0;
    ChannelSlots_Set(IDX(ch->m_colorIndex), 1);
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

    m_waitDialogReplyReceived = 0;
    g_pauseGuard = 1;
    NetLobbyCtrlId result =
        static_cast<NetLobbyCtrlId>(RunErrorDialog("MULTI_PAUSE", NetLobby::HostWaitDlgProc, 0));
    g_pauseGuard = 0;
    g_sharedFlag = NULL;

    if (result == IDC_NET_RESTART) {
        HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
        PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MULTI_CONNECT), SelectedLevelIndex());
    }
}

RVA(0x000badd0, 0x43)
void CMulti::OnMultiOptions() {
    if (g_optionzGuard) {
        return;
    }

    m_waitDialogReplyReceived = 0;
    g_optionzGuard = 1;
    RunErrorDialog("MULTI_OPTIONZ", NetLobby::JoinWaitDlgProc, 0);
    g_optionzGuard = 0;
    g_sharedFlag = NULL;
}

RVA(0x000bae40, 0x84)
void CMulti::OnOutOfSync() {
    if (m_outOfSync) {
        return;
    }

    m_outOfSync = 1;
    m_waitDialogReplyReceived = 0;
    NetLobbyCtrlId result = static_cast<NetLobbyCtrlId>(
        RunErrorDialog("MULTI_OUTOFSYNC", NetLobby::SessionWaitDlgProc, 0)
    );
    g_sharedFlag = NULL;

    switch (result) {
        case IDC_NET_RESTART: {
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MULTI_CONNECT), SelectedLevelIndex());
            break;
        }
        case IDC_NET_CONTINUE:
            break;
        default: {
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            break;
        }
    }
}

RVA(0x000baf00, 0xb2)
i32 CMulti::BroadcastOneChannel(GruntzPlayer* player) {
    CNetOneChannelPacket packet;
    memset(&packet, 0, 0x2c);
    packet.m_flags |= 0x80;
    packet.m_statId = STAT_CHANNEL_ONE;
    packet.m_playerIndex = player->m_playerIndex;

    i32 v = player->m_colorIndex;
    packet.m_colorIndex = static_cast<u8>(v);
    v = player->m_humanControlled;
    packet.m_humanControlled = static_cast<u8>(v);
    v = player->m_configId;
    packet.m_configId = static_cast<u8>(v);
    v = player->m_readyFlag;
    packet.m_readyFlag = static_cast<u8>(v);
    packet.m_present = 1;
    v = player->m_comboSel;
    packet.m_comboSel = static_cast<u8>(v);
    v = player->m_slotKey;
    packet.m_slotKey = v;
    strcpy(packet.m_name, static_cast<const char*>(player->GetName()));

    return SendStatFrom(&packet, 0x2c, 1);
}

RVA(0x000baff0, 0x88)
i32 CMulti::ApplyChannelUpdate(CNetOneChannelPacket* packet) {
    if (packet == NULL) {
        return 0;
    }
    i32 playerIndex = packet->m_playerIndex;
    if (playerIndex < 0 || playerIndex >= 4) {
        return 0;
    }
    GruntzPlayer* player = &NetGameMgr()->m_options[playerIndex];
    if (player == NULL) {
        return 0;
    }

    player->m_name = packet->m_name;
    player->m_colorIndex = static_cast<ColorTint>(packet->m_colorIndex);
    player->m_configId = packet->m_configId;
    if (packet->m_readyFlag != 0) {
        player->m_readyFlag = 1;
    } else {
        player->m_readyFlag = 0;
    }
    player->m_comboSel = packet->m_comboSel;
    player->m_humanControlled = packet->m_humanControlled;
    player->m_slotKey = packet->m_slotKey;
    player->m_liveGate = 1;
    return 1;
}

RVA(0x000bb0b0, 0x44)
i32 CMulti::BroadcastOptionsPresent() {
    g_optionsPresentPacket.m_statId = NETMSG_OPTIONS_PRESENT;
    g_optionsPresentPacket.m_flags |= 0x80;
    g_optionsPresentPacket.m_value = 0;
    Network()
        ->BroadcastFrom(LocalPlayer(), 1, &g_optionsPresentPacket, sizeof(g_optionsPresentPacket));
    return 1;
}

RVA(0x000bb120, 0x44)
i32 CMulti::BroadcastOptionsAbsent() {
    g_optionsAbsentPacket.m_statId = NETMSG_OPTIONS_ABSENT;
    g_optionsAbsentPacket.m_flags |= 0x80;
    g_optionsAbsentPacket.m_value = 0;
    Network()
        ->BroadcastFrom(LocalPlayer(), 1, &g_optionsAbsentPacket, sizeof(g_optionsAbsentPacket));
    return 1;
}

RVA(0x000bb190, 0x1c5)
i32 CMulti::BroadcastChatLine(char* text, i32 toChat, i32 showWnd, HWND hWnd) {
    if (text == NULL) {
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
                static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(LocalPlayer()->m_playerId))
                    ->GetName()
            ),
            text
        );
    } else {
        strcpy(line, text);
    }

    if (showWnd != 0 && hWnd != NULL) {
        AppendEditLine(hWnd, line);
    } else if (showWnd != 0) {

        GruntzPlayer* player = static_cast<GruntzPlayer*>(Mgr()->FindOptionsSlot(m_localPlayerId));
        if (player == NULL) {
            return 0;
        }
        (static_cast<CFontConfig*>(NetGameMgr()->m_chatLog))
            ->AddItem(line, 0x30, IDX(player->m_colorIndex));
    }

    g_chatPacket.m_id = STAT_CHAT;

    i32 packetLen = strlen(line) + 0xd;
    g_chatPacket.m_val = 0;
    strcpy(g_chatPacket.m_buf, line);
    g_chatPacket.m_flag |= 0x80;
    Network()->BroadcastFrom(LocalPlayer(), 1, &g_chatPacket, packetLen);
    return 1;
}

// Retail's two call sites (DispatchRecvMsg 0xb98e2, BroadcastChatLine 0xbb290)
// both load ecx with the CMulti before `call 0x3e31`, so this is a __thiscall
// member, not the free __stdcall it was reconstructed as. The body never
// touches `this`, which is why it byte-matched either way.
RVA(0x000bb3e0, 0xe5)
void CMulti::AppendEditLine(HWND edit, char* str) {
    if (!edit || !str || !str[0]) {
        return;
    }
    i32 len = GetWindowTextLengthA(edit);
    if (len == 0) {
        SendMessageA(edit, EM_SETSEL, len, -1);
    } else {
        SendMessageA(edit, EM_SETSEL, len, len);
    }
    char buf[0x80];
    buf[0] = 0;
    if (len > 0) {
        strcat(buf, "\r\n");
    }
    strcat(buf, str);
    MsgParam text;
    text.m_str = buf;
    SendMessageA(edit, EM_REPLACESEL, 0, text.m_lparam);
    SendMessageA(edit, EM_LINESCROLL, 0, 0x270f);
}

// @early-stop
RVA(0x000bb510, 0x9d)
i32 CMulti::DropChannelPlayer(i32 slotIndex) {
    if (slotIndex < 0 || slotIndex >= 4) {
        return 0;
    }
    if (m_isHost == 0) {
        return 0;
    }

    GruntzPlayer* ch = &NetGameMgr()->m_options[slotIndex];
    if (ch == NULL) {
        return 0;
    }

    CNetPlayerNode* player = Network()->GetPlayerNodeData(ch->m_slotKey);

    i32 active = ch->m_humanControlled;
    if (player == NULL) {
        if (active != 0) {
            return 0;
        }
    } else if (active != 0) {
        SendStatTo(player, STAT_CHANNEL_LEFT, 1);
    }

    if (RemoveChannel(slotIndex) == 0) {
        return 0;
    }
    BroadcastChannelTable(NULL);
    g_playerLeftFlag = 1;
    return 1;
}

RVA(0x000bb5e0, 0xd9)
void CMulti::RecordDropAcknowledgement(CNetPlayerNode* unusedPlayer, i32 playerId) {
    if (m_allPlayersReady != 0) {
        return;
    }
    if (playerId == m_localPlayerId) {
        return;
    }

    i32 count = m_readyPlayerIds.GetSize();
    i32 i;
    for (i = 0; i < count; i++) {
        if (static_cast<i32>(m_readyPlayerIds[i]) == playerId) {
            return;
        }
    }

    i32 slot = 0;
    while (slot < count) {
        if (m_readyPlayerIds[slot] == 0) {
            break;
        }
        slot++;
    }
    if (slot >= count) {
        return;
    }
    m_readyPlayerIds[slot] = playerId;

    i32 stateThree = 0;
    CNetCmdSlot* p = m_session->m_slots;
    for (i = 0; i < 4; i++) {
        if (p != NULL && p->m_state == NETSLOT_ACTIVE) {
            stateThree++;
        }
        p++;
    }

    i32 total = m_readyPlayerIds.GetSize();
    i32 recorded = 0;
    for (i = 0; i < total; i++) {
        if (m_readyPlayerIds[i] != 0) {
            recorded++;
        }
    }
    if (recorded < stateThree) {
        return;
    }

    SendStatFlag(STAT_DROP_ANNOUNCE, 1);
    SendStatFlag(STAT_DROP_ANNOUNCE, 1);
    m_allPlayersReady = 1;
}

RVA(0x000bb700, 0x265)
i32 CMulti::WaitForOtherPlayers() {
    CDWordArray* votes = &m_readyPlayerIds;
    votes->SetSize(0, -1);
    for (i32 k = 3; k != 0; k--) {
        votes->SetAtGrow(votes->GetSize(), 0);
    }
    if (Network()->m_players.GetCount() == 1) {
        goto ready;
    }
    {
        i32 count = 0;
        CNetCmdSlot* slot = m_session->m_slots;
        for (i32 j = NET_SLOT_COUNT; j != 0; j--) {
            if (slot != NULL && slot->m_state == NETSLOT_ACTIVE) {
                count++;
            }
            slot++;
        }
        if (count != 0) {
            SendStatFlag(NETMSG_DROP_PLAYER, 1);
            CString waitStr("Waiting for other playerz...");
            CGruntzMgr* g = g_gameReg;

            RECT rc;
            tagSIZE mode = g->GetModeSize();
            rc.right = g->GetModeSize().cx;
            rc.bottom = mode.cy;
            rc.left = 0;
            rc.top = 0;
            EngStr_DrawText(g->m_world, &waitStr, &rc, 0x82, 1, 0xff, 0xff, 0, 1);

            i32 resend = 0x1388;
            i32 abort = 0x1d4c0;
            while (m_allPlayersReady == 0) {
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
                    if (s->m_state == NETSLOT_ACTIVE) {
                        s->m_latency += elapsed;
                    }
                }
                if (abort == 0) {
                    CheckDropTimeout();
                    abort = 0x1d4c0;
                }
                if (resend == 0) {
                    resend = 0x1388;
                    AckJoinFailure();
                    SendStatFlag(NETMSG_DROP_PLAYER, 1);
                }
            }

            g_scoreTimeBase = timeGetTime();

            if (g_gameReg->m_musicEnabled != 0) {
                char buf[0x40];
                wsprintfA(buf, "AMBIENT%d", GetAmbientId());
                NetGameMgr()->m_midi->PlaySequence(buf, 1);
            }
            return 1;
        }
    }

ready:
    m_allPlayersReady = 1;
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
        m_levelChecksumReceived[i] = 0;
        m_levelChecksums[i] = 0;
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
            if (ch->m_slotKey != m_localPlayerId && ch->m_liveGate != 0
                && ch->m_humanControlled != 0) {
                if (m_levelChecksumReceived[i] == 0) {
                    allAcked = 0;
                } else if (!(m_levelChecksums[i] == token && token != 0)) {
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
    CNetSessionListNode* rec = g_netMgr->m_selectedSession;
    if (rec == NULL) {
        return 0;
    }
    Network()->EnumerateSessionPlayers(rec, 0);
    if (ResolveLocalPlayer() == 0) {
        return 0;
    }

    CNetSession* session = new CNetSession();
    m_session = session;
    if (session == NULL) {
        return 0;
    }
    if (session->Initialize(NetGameMgr(), this, Network()) == 0) {
        return 0;
    }

    Session()->m_localPlayer = LocalPlayer();
    i32 raw10 = m_session->m_commandTick;
    u8 b = static_cast<u8>(raw10);
    if (b == 0) {
        b = 0x7f;
    } else {
        b = b - 1;
    }
    m_processedCommandTick = b;

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* ch = &NetGameMgr()->m_options[i];
        NetSlotState state = NETSLOT_INACTIVE;
        if (ch->m_liveGate != 0 && ch->m_humanControlled != 0) {

            state = NETSLOT_LOCAL;
            if (ch->m_slotKey != m_localPlayerId) {
                state = NETSLOT_REMOTE;
            }
        }
        if (Session()->CreateSlot(i, state) == NULL) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000bbec0, 0x81)
CNetCmdSlot::CNetCmdSlot() {
    m_state = NETSLOT_EMPTY;
    m_isDraining = 0;
    m_drainSequence = 0;
    m_player = NULL;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    m_owner = NULL;
    ResetNetCmdSlotCommandWindow(this);
}

// @early-stop
// Strength reduction: retail biases the slot cursor by +8 and the record cursor
// by +8, and spills the outer loop counter to the stack; cl keeps the counter in
// edi and anchors both cursors at offset 0.
RVA(0x000bbf80, 0xb7)
void CNetSession::InitializeFields() {
    m_mgr = NULL;
    m_owner = NULL;
    m_netMgr = NULL;
    m_localPlayer = NULL;
    m_commandTick = 0;
    m_batchBuilt = 0;
    m_sequence = 0;
    m_commandPeriod = 1;

    i32 i;
    for (i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        slot->m_state = NETSLOT_EMPTY;
        slot->m_isDraining = 0;
        slot->m_drainSequence = 0;
        slot->m_player = NULL;
        slot->m_latency = 0;
        slot->m_contiguousSequence = 0;
        slot->m_peerWindowBase = 0;
        slot->m_owner = NULL;
        ResetNetCmdSlotCommandWindow(slot);
    }

    memset(m_commandByTick, 0, sizeof(m_commandByTick));

    for (i = 0; i < 0x80; i++) {
        m_commandRecords[i].m_sequence = 0;
        m_commandRecords[i].m_entryCount = 0;
        m_commandRecords[i].m_payloadLength = 0;
        m_commandRecords[i].m_checksum = 0;
    }
}

RVA(0x000bc070, 0x73)
u32 CMulti::FrameSyncWait() {
    u32 now = timeGetTime();
    u32 ret = 0;
    m_accumTime = now - m_lastFrameSyncTime;
    m_lastFrameSyncTime = now;

    if (m_accumTime <= 0x1e) {
        ActiveWait(0x1f - m_accumTime);
        m_lastFrameSyncTime = (now - m_accumTime) + 0x1f;
    } else if (m_accumTime > 0x28 && m_syncGate) {
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

    m_waitDialogReplyReceived = 0;
    g_dropGuard = 1;
    NetLobbyCtrlId result = static_cast<NetLobbyCtrlId>(
        RunErrorDialog("MULTI_DROPPLAYER", NetLobby::NetGameDlgProc, 0)
    );
    g_dropGuard = 0;
    g_sharedFlag = NULL;

    switch (result) {
        case IDC_NET_CONTINUE:
            Session()->ResetLatencies();
            break;
        case IDC_NET_ABORT: {
            Session()->ResetLatencies();
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            break;
        }
        case IDC_NET_DROP_PLAYER:
            if (g_dropPlayerId != -999) {
                if (Network()->FindPlayerById(g_dropPlayerId)) {
                    SendStat3(g_dropPlayerId, STAT_PLAYERLEFT_LOCAL, 1);
                }
            }
            SendNetStat(STAT_PLAYERLEFT, g_dropPlayerId, 1);
            AckDropPlayer(g_dropPlayerId);
            Session()->ResetLatencies();
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
void CMulti::CheckDropTimeout() {
    if (m_session->FindLaggingSlot(0x1388) == NULL) {
        return;
    }
    if (g_ackThrottleDeadline < static_cast<u32>(timeGetTime())) {
        AckJoinFailure();
        g_ackThrottleDeadline = timeGetTime() + 0x3e8;
    }
    CNetCmdSlot* slot = m_session->FindLaggingSlot(0x2710);
    if (slot == NULL) {
        return;
    }
    g_dropPlayerId = slot->m_player->m_slotKey;
    g_sessionName = slot->GetPlayerName();
    SendNetStat(NETMSG_DROP_TIMEOUT, g_dropPlayerId, 1);
    OnDropPlayer();
}

RVA(0x000bc3f0, 0x1e)
CString CNetCmdSlot::GetPlayerName() {
    return m_player->GetName();
}

RVA(0x000bc420, 0x2b)
void CMulti::AckJoinFailure() {
    if (m_netMgr && m_localPlayer && m_connected) {
        SendStatFlag(NETMSG_LOBBY_TICK, 1);
    }
}

// @early-stop
RVA(0x000bc460, 0x24e)
i32 CMulti::SetupTcpIpConfig() {
    m_providerConfigPrefix = "TcpIp";
    m_gameClosed = 0;
    m_commandDelay = 5;
    m_resendInterval = 0x3c;

    Utils::RegistryHelper* cfg = NetGameMgr()->m_settings;
    CString kDelay = m_providerConfigPrefix + "_CmdDelay";
    CString kResend = m_providerConfigPrefix + "_Resend";
    CString kDyn = m_providerConfigPrefix + "_DynCmdDelay";
    i32 cd = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kDelay))), -1);
    i32 rs = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kResend))), -1);
    if (cd != -1 && rs != -1) {
        m_commandDelay = cd;
        m_resendInterval = rs;
    }

    GruntzPlayer* ch0 = NetGameMgr()->m_options;

    ch0->m_name = PlayerName();
    ch0->m_colorIndex = TINT_ORANGE;

    m_localPlayer = static_cast<CNetPlayerNode*>(
        Network()->CreatePlayer(const_cast<char*>(static_cast<const char*>(ch0->GetName())), "", 0)
    );
    if (LocalPlayer() == NULL) {
        ReportNetError(0);
        return 0;
    }

    m_localPlayerId = LocalPlayer()->m_playerId;
    ColorTint chField = static_cast<ColorTint>(ch0->m_colorIndex);

    if (RegisterChannelFrom(ch0->GetName(), chField, -1, m_localPlayerId) == TINT_ORANGE) {
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x000bc750, 0x151)
i32 CMulti::CreateLocalPlayer() {
    {
        m_localPlayer = static_cast<CNetPlayerNode*>(Network()->CreatePlayer(
            const_cast<char*>(static_cast<const char*>(PlayerName())),
            "",
            0
        ));
    }
    if (LocalPlayer() == NULL) {
        ReportNetError(0);
        return 0;
    }

    m_localPlayerId = LocalPlayer()->m_playerId;
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

    pkt.m_humanControlled = 0;
    pkt.m_colorIndex = 0xf;
    pkt.m_configId = 0x63;
    pkt.m_playerId = m_localPlayerId;
    {
        strcpy(pkt.m_name, static_cast<const char*>(PlayerName()));
    }
    SendStatFrom(&pkt, 0x28, 1);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000bc910, 0xf6)
i32 CMulti::OpenHostChannel(
    void* hostToken,
    const char* name,
    i32 channelId,
    i32 cmdDelay,
    i32 resend,
    i32 unused6,
    i32 unused7,
    i32 unused8
) {
    if (hostToken == NULL) {
        return 0;
    }
    m_commandDelay = cmdDelay;
    m_resendInterval = resend;
    m_levelIndex = 1;
    m_rngSeed = timeGetTime();
    m_localPlayer =
        Network()->CreatePlayer(const_cast<char*>(static_cast<const char*>(PlayerName())), "", 0);
    if (m_localPlayer == NULL) {
        ReportNetError(0);
        return 0;
    }
    m_localPlayerId = m_localPlayer->m_playerId;
    return RegisterChannelFrom(name, static_cast<ColorTint>(channelId), -1, m_localPlayerId)
           != TINT_ORANGE;
}

RVA(0x000bca50, 0x155)
i32 CMulti::WaitForConnect() {
    if (Network() == NULL) {
        return 0;
    }
    if (LocalPlayer() == NULL) {
        return 0;
    }

    SendStatFlag(STAT_CONNECTING, 1);
    m_connectAccepted = 0;

    u32 deadline = timeGetTime() + 60000;

    while (m_connectAccepted == 0) {

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
        if (m_removedByHost) {
            ReportVersionMsg("You have been removed from the game by the host.", 0);
            return 0;
        }
        if (m_gameClosed) {
            ReportVersionMsg("This game is closed.", 0);
            return 0;
        }
        if (m_gameFull) {
            ReportVersionMsg("This game is already full.", 0);
            return 0;
        }
        if (m_versionMismatch) {
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
    if (m_customLevelVerificationPending != 0) {
        return 1;
    }

    u32 ping = static_cast<u32>(GetMaxAckLatency());
    u32 tuned = ping / 30 + 2;
    i32 base = (tuned < 3) ? 3 : static_cast<i32>(tuned);

    i32 probe = Mgr()->CountReadyOptionsSlots(0);

    i32 bump = (probe > 2) ? 2 : 1;
    base += bump;
    m_commandDelay = base;

    i32 resend;
    if (base <= 5) {
        resend = 0xa;
    } else {

        resend = (base <= 8 ? 0x14 : 0x1e);
    }
    m_resendInterval = resend;
    return SaveConfig(NULL);
}

RVA(0x000bccd0, 0x141)
i32 CMulti::SaveConfig(CNetPlayerNode* recipient) {
    CNetConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    blob.m_flags |= 0x80;
    blob.m_statId = STAT_CONFIG;
    blob.m_customLevel = m_customLevel;
    {
        wsprintfA(blob.m_nameA, static_cast<const char*>(BuiltInLevelName()));
    }
    {
        wsprintfA(blob.m_nameB, static_cast<const char*>(CustomLevelName()));
    }
    blob.m_commandDelay = m_commandDelay;
    blob.m_resendInterval = m_resendInterval;
    blob.m_autoCommandDelay = m_autoCommandDelay;
    blob.m_rngSeed = m_rngSeed;

    if (recipient != NULL) {
        SendStatPairRaw(recipient, &blob, 0x11c, 1);
    } else {
        SendStatFrom(&blob, 0x11c, 1);
    }
    return 1;
}

RVA(0x000bce80, 0x77)
i32 CMulti::LoadConfig(CNetConfigBlob* config) {
    if (config == NULL) {
        return 0;
    }

    m_customLevel = config->m_customLevel;
    m_builtInLevelName = config->m_nameA;
    m_customLevelName = config->m_nameB;
    m_commandDelay = config->m_commandDelay;
    m_resendInterval = config->m_resendInterval;
    m_autoCommandDelay = config->m_autoCommandDelay;
    m_rngSeed = config->m_rngSeed;
    return 1;
}

// @early-stop
// Register renaming plus one hoisted load in the per-slot reset loop.
RVA(0x000bcf20, 0xaf)
i32 CMulti::ResetPlayerCommands(i32 playerId) {
    if (m_connected == 0) {
        return 0;
    }

    CNetCmdSlot* slot = Session()->FindSlotByPlayerId(playerId);
    if (slot == NULL) {
        return 0;
    }
    if (slot->m_isDraining != 0) {
        return 0;
    }

    slot->BeginDrain();
    i32 seq = (slot->m_contiguousSequence + 1) * static_cast<i32>(m_commandDelay);
    i32 end = seq + static_cast<i32>(m_commandDelay) * 3;
    for (; seq < end; seq++) {

        NetGameMgr()->m_cmdSubMgr->RemoveScheduledCommand(slot->m_player->m_playerIndex, seq);
        slot->RemoveRecord(seq / static_cast<i32>(m_commandDelay));
    }
    slot->ClearSequenceSet(slot->m_receivedAhead);
    slot->ClearSequenceSet(slot->m_peerReceivedAhead);
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
            if (mgr->m_options[i].m_humanControlled && mgr->m_options[i].m_liveGate) {
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
    if (msg == NULL) {
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
        m_versionMismatch = 1;
        if (wasConnected) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            HWND hwnd = NetGameMgr()->m_gameWnd->m_hwnd;
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
        }
    }
    if (mismatch) {
        SendStatFlag(STAT_VERSIONMISMATCH, 1);
        Sleep(0xfa);
    }
}

// @early-stop
// Register renaming plus a two-push reorder in the SendPacket argument setup.
RVA(0x000bd180, 0x66)
void CMulti::AnnounceVersion(CNetPlayerNode* recipient) {
    CNetVersionPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.m_flags |= 0x80;
    packet.m_remoteVersion = g_remoteVersion;
    packet.m_cfgWord = g_cfgWord;
    packet.m_butePos = g_buteMgr.m_pos;
    packet.m_localVersion = g_localVersion;
    packet.m_statId = STAT_VERSIONPACKET;

    SendStatPairRaw(recipient, &packet, 0x20, 1);
}

RVA(0x000bd210, 0x14d)
i32 CMulti::OnChar(i32 charCode, i32 keyData) {
    if (m_hitTest && m_hitTest->m_inputActive) {
        if (m_connected) {
            if (Mgr()->m_chatLog->HandleInputChar(charCode, keyData)) {
                CString line = Mgr()->m_chatLog->GetInputText();
                i32 n = line.GetLength();
                if (n > 9) {
                    CString text = line.Right(n - 9);
                    char buf[0x100];
                    strcpy(buf, text);
                    BroadcastChatLine(buf, 1, 1, NULL);
                    Mgr()->m_chatLog->m_inputText.Empty();
                }
            }
        }
        return 1;
    }
    return CPlay::OnChar(charCode, keyData);
}
RVA(0x000bd3c0, 0x9)
void CMulti::TickStateMgrs() {
    m_mgr->TickStateMgrs();
}
