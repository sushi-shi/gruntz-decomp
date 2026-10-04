#include <StdAfx.h>
#include <Io/File.h>
#include <Utils/Text.h>

#include <Ints.h>

#include <Gruntz/Multi.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/MidiManager.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BattlezMapConfig.h>
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
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Minimap.h>
#include <Gruntz/Play.h>
#include <Gruntz/PlayInline.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sparam.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Utils.h>
#include <Gruntz/VoiceManager.h>
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
#include <Net/NetSlotState.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezSync.h>
#include <SafeDelete.h>
#include <Utils/DebugTiming.h>
#include <Utils/MapTyped.h>
#include <Io/Settings.h>
#include <Wap32/EngStr.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <dplay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>

i32 g_cfgWord;

i32 g_battlezTurnPlayerIndex = 0;

GZ_ENUM_CONST_BEGIN(NetSentinels)
    NET_SERVICE_NONE = 999,
    NET_PLAYER_ID_NONE = -999,
    NET_PREFERRED_PLAYER_INDEX_ANY = 99
GZ_ENUM_CONST_END(NetSentinels)

i32 g_dropPlayerId = NET_PLAYER_ID_NONE;

i32 g_serviceId = NET_SERVICE_NONE;

char g_gameKey[] = "GAME_KEY";

CNetOptionsStatePacket g_optionsClosedPacket;

CNetOptionsStatePacket g_optionsOpenedPacket;

HWND g_netMessageEditHwnd = NULL;

b32 g_playerRosterChanged;

i32 g_roundStartTimeMs;

b32 g_pauseDialogOpen;

b32 g_optionsDialogOpen;

b32 g_frameSkipToggle;

b32 g_dropDialogOpen;

u32 g_ackThrottleDeadline;

i32 g_playersInOptionsCount = 0;

std::string g_sessionName;

CNetChatPacket g_netChatPacket;

HWND g_sessionListHwnd;

b32 g_hostServicesMode;

CNetMgr* g_netMgr;

CMulti* g_connectRptMgr;

CMulti::~CMulti() {
    CMulti::ReleaseResources();
}



char g_recvBuffer[NET_RECEIVE_BUFFER_BYTES];

i32 CMulti::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    g_gameReg->m_gameMode = GAMEMODE_MULTIPLAYER;
    if (mgr == NULL) {
        return 0;
    }

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }
    g_connectRptMgr = this;

    m_region0Gate = false;
    m_region1Gate = false;
    m_region2Gate = false;
    m_viewportResizeMode = VIEW_RESIZE_IDLE;
    m_hudSuppressed = true;
    m_cameraBookmarkIndex = -1;
    m_defeatCountdownActive = false;
    m_scrollEdgeActive = 0;
    m_scrollEdgeLock = 0;
    m_customLevelVerificationPending = false;
    m_allPlayersReady = false;
    m_sessionTerminated = false;
    m_removedByHost = false;
    m_gameClosed = false;
    m_pollAbort = false;
    m_colorSelectionRejected = false;
    m_gameFull = false;
    m_outOfSync = false;
    m_notifyLatch = false;
    m_completedFinalLevel = false;
    m_syncGate = false;
    m_connected = false;
    m_pumpGuard = false;
    m_waitDialogReplyReceived = false;
    m_lobbyLaunch = false;
    m_versionMismatch = false;
    m_initialFramePending = true;
    m_localPlayer = NULL;
    m_localPlayerId = 0;
    m_commandDelay = 0;
    m_autoCommandDelay = true;
    m_resendInterval = 0;
    m_minimap = NULL;
    m_savedClock = 0;
    m_rngSeed = static_cast<i32>(timeGetTime());
    m_connectAccepted = false;
    m_roundComplete = false;

    for (i32 i = 0; i < 4; i++) {
        m_playerLatencyMs[i] = 0;
        PlayerLatency* lat = &g_gameReg->m_players[i].m_latency;
        lat->Clear();
    }

    NetGameMgr()->m_loadingSaveGame = false;
    Mgr()->ResetClockGlobals();
    Mgr()->DeactivateAllPlayers();
    ResetPlayerColorAvailability();

    CNetMgr* peer = new CNetMgr();
    m_netMgr = peer;
    g_netMgr = peer;

    NetGameMgr()->m_modalBusy = true;
    if (Mgr()->InitializeLobbyConnectionSettings() != 0) {
        if (StartTitle() == 0) {
            NetGameMgr()->m_modalBusy = false;
            ReleaseResources();
            return 0;
        }
    } else {
        if (Open() == 0) {
            NetGameMgr()->m_modalBusy = false;
            while (ShowCursor(false) >= 0) {
            }
            return 0;
        }
    }

    if (m_isHost != false) {
        m_connectAccepted = true;
    }
    NetGameMgr()->m_modalBusy = false;
    memset(&m_saveSlot, 0, sizeof(m_saveSlot));
    m_savedEffectsEnabled = NetGameMgr()->m_isEffectsEnabled;
    NetGameMgr()->m_isEffectsEnabled = true;
    if (LoadImageBanks() == 0) {
        return 0;
    }
    PostLoadImageBanks();
    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_MULTI");
    if (m_stateResources == NULL) {
        return 0;
    }
    if (ShowMultiStartDlg() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    if (CreateSession() == 0) {
        return 0;
    }

    if (m_usesCustomLevel != false) {
        NetGameMgr()->m_isBuiltInMultiplayerLevel = false;
        NetGameMgr()->m_strWorldFile = "custom\\" + CustomLevelName();
    } else {
        NetGameMgr()->m_isBuiltInMultiplayerLevel = true;
        NetGameMgr()->m_strWorldFile = BuiltInLevelName();
    }
    if ((Mgr()->GetWorldFileName()).empty()) {
        return 0;
    }

    CChatBoxOwner* iface = new CChatBoxOwner();
    m_chatBox = iface;

    if (iface->Attach(m_world, NetGameMgr()->ChatLog()) == 0) {
        CChatBoxOwner* io = m_chatBox;
        if (io == NULL) {
            return 0;
        }
        io->Deactivate();
        delete io;
        m_chatBox = NULL;
        return 0;
    }
    m_chatBox->m_inputActive = false;
    m_chatBox->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);

    CStatusBarMgr* sess = new CStatusBarMgr;
    m_statusBar = sess;
    if (sess->LoadBattlezItemConfig(m_world) == 0) {
        if (m_statusBar == NULL) {
            return 0;
        }
        delete m_statusBar;
        m_statusBar = NULL;
        return 0;
    }

    CTileTriggerContainer* cmd = new CTileTriggerContainer();
    m_tileTriggers = cmd;
    if (cmd->Initialize() == 0) {
        if (m_tileTriggers == NULL) {
            return 0;
        }
        delete m_tileTriggers;
        m_tileTriggers = NULL;
        return 0;
    }

    if (LoadByMode(1, 1) == 0) {
        return 0;
    }
    m_pumpGuard = true;
    m_allPlayersReady = false;
    i32 wr = WaitForOtherPlayers();
    m_pumpGuard = false;
    if (wr == 0) {
        return 0;
    }
    if ((static_cast<CPlay*>(this))->LoadCursorSprites(0, false) == 0) {
        return 0;
    }
    PollSession();
    srand(m_rngSeed);
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    NetGameMgr()->ChatLog()->FreeNodes();
    m_connected = true;
    return 1;
}

CNetMgr::~CNetMgr() {
    Destroy();
}

void CMulti::ReleaseResources() {
    if (m_netMgr && m_localPlayer && m_session && m_connected) {
        BroadcastValueMessage(NETMSG_WAIT_DIALOG_REPLY, IDX(IDC_NET_RESUME), DPSEND_GUARANTEED);
        BroadcastPlayerIdMessage(NETMSG_PLAYER_LEFT, DPSEND_GUARANTEED);
    }

    CNetSession* session = m_session;
    if (session) {
        delete session;
        m_session = NULL;
    }
    SAFE_DELETE(m_netMgr)

    CMinimap* minimap = m_minimap;
    if (minimap) {
        minimap->Reset();
        delete minimap;
        m_minimap = NULL;
    }
    Mgr()->m_isEffectsEnabled = m_savedEffectsEnabled;

    CPlay::ReleaseResources();
}

CNetSession::~CNetSession() {
    Shutdown();
}

CNetCmdSlot::~CNetCmdSlot() {
    ResetSlot();
}

void CMulti::OnExit() {
    CPlay::OnExit();
}

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
    if (m_connected != false) {
        BroadcastValueMessage(NETMSG_WAIT_DIALOG_REPLY, IDX(IDC_NET_RESUME), DPSEND_GUARANTEED);
    }
    return 1;
}

i32 CMulti::LeaveState(GameStateId nextState) {
    m_mgr->VoiceMgr()->PauseAllVoices();
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_notifyLatch) {
        QuitToMenu();
    }
    if (nextState != GAMESTATE_HELP) {
        return PrepareDepartureFade();
    }
    return 1;
}

i32 CMulti::CompleteLevel() {
    return CPlay::CompleteLevel();
}

i32 CMulti::LoadByMode(i32 mode, i32 unused) {
    g_battlezTurnPlayerIndex = 0;
    GruntzPlayer* host = Mgr()->FindPlayerByNetworkId(m_localPlayerId);
    if (!host) {
        return 0;
    }
    g_curPlayer = host->m_playerIndex;
    srand(m_rngSeed);
    g_playersInOptionsCount = 0;
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
    m_outOfSync = false;

    if (CPlay::LoadByMode(mode, 0) == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; ++i) {
        GruntzPlayer* e = &Mgr()->m_players[i];
        if (e == NULL) {
            return 0;
        }
        e->m_battlezConfig.FreeArrays();
        if (e->m_battlezConfig.LoadConfig(Mgr(), i, e->GetDifficulty()) == 0) {
            return 0;
        }
        if (e->m_humanControlled && e->m_active) {
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
    m_outOfSync = false;
    Mgr()->ChatLog()->FreeNodes();
    m_session->ResetRound();
    Mgr()->VoiceMgr()->PauseAllVoices();
    return 1;
}

i32 CMulti::Connect(i32 mode) {
    m_connected = false;
    m_allPlayersReady = false;
    StateChangeOptions options(0x446);
    options.connectRound = true;
    return Mgr()->PassClickToPlayState(mode, false, 0, options);
}

i32 CMulti::FinishConnect() {
    m_pumpGuard = true;
    if (WaitForOtherPlayers() == 0) {
        m_pumpGuard = false;
        return 0;
    }
    m_pumpGuard = false;
    m_connected = true;
    return 1;
}

i32 CMulti::Render() {
    m_drewThisFrame = false;
    HandleDragMove(0, m_cursorX, m_cursorY);
    i32 oldT = m_lastTime;
    i32 t = timeGetTime();
    m_lastTime = t;

    m_frameDelta = t - oldT;
    m_accumTime += m_frameDelta;
    i32 newId = m_session->m_commandTick;
    if (m_processedCommandTick != newId) {
        m_processedCommandTick = newId;
        CGruntzCmdMgr* mgr = Mgr()->m_commandMgr;
        CGruntzCommand* node;
        if (mgr->m_pendingLocalCommands.empty()) {
            node = NULL;
        } else {
            node = static_cast<CGruntzCommand*>(takeFront(mgr->m_pendingLocalCommands));
        }
        if (node) {
            node->m_submitFlags = COMMAND_SUBMIT_SCHEDULED;

            i32 v = m_processedCommandTick + static_cast<i32>(m_commandDelay) * 2;
            node->m_scheduleSlot = static_cast<u8>(v % 128);
        }
        m_session->ScheduleCommand(node, static_cast<u8>(static_cast<u8>(m_commandDelay) << 1));
    }
    i32 dt = m_frameDelta;
    dt = static_cast<i32>(min(static_cast<u32>(dt), g_frameDelta));
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
    if (m_session->AdvanceTick() && m_pollAbort == false) {
        fin = 1;
    }
    TickStateMgrs();
    CDDrawWorkerHost* mainPlane = m_world->m_level->m_mainPlane;
    if (mainPlane) {
        mainPlane->ActivateVisibleObjects();
    }

    if (fin != 0) {
        if (m_session->VerifyChecksums() == 0 && m_outOfSync == false) {
            if (m_isHost != false) {
                BroadcastPlayerIdMessage(NETMSG_OUT_OF_SYNC, DPSEND_GUARANTEED);
                OnOutOfSync();
                AdvanceGameFrame();
                m_drainTimer = 0;
                return 1;
            }
            BroadcastPlayerIdMessage(NETMSG_OUT_OF_SYNC_REPORT, DPSEND_GUARANTEED);
        }
        AdvanceGameFrame();
        m_drainTimer = 0;
        return 1;
    }
    RenderGameFrame();
    CheckDropTimeout();
    SoundStream* win = m_world->GetSoundStream();
    if (win) {
        i32 now = timeGetTime();
        win->TickVolumeRamps(now);
        win->TickStreams(now);
    }
    ActiveWait(2);
    return 1;
}

i32 CMulti::AdvanceGameFrame() {
    b32 ready = FrameSyncWait();
    if (m_roundComplete == false && Mgr()->m_frameGate != false && ready == false) {
        RenderGameFrame();
        return 1;
    }

    g_frameDelta = 0x21;
    g_lastNow += 0x21;
    g_frameTime += 0x21;
    g_soundCueTimeMs = g_lastNow;
    g_engineFrameDelta = 0x21;
    UpdateAmbientMusic();
    Mgr()->m_commandMgr->ExecuteScheduledCommands(m_processedCommandTick % 128);
    m_session->ComputeChecksum();
    g_frameTicks++;
    u32 t1 = g_period50CountdownMs ? g_period50CountdownMs : FRAME_CLOCK_PERIOD_50_MS;
    if (g_frameDelta >= t1) {
        g_period50CountdownMs = 0;
    } else {
        g_period50CountdownMs = t1 - g_frameDelta;
    }
    u32 t2 = g_period100CountdownMs ? g_period100CountdownMs : FRAME_CLOCK_PERIOD_100_MS;
    if (g_frameDelta >= t2) {
        g_period100CountdownMs = 0;
    } else {
        g_period100CountdownMs = t2 - g_frameDelta;
    }
    u32 t3 = g_period200CountdownMs ? g_period200CountdownMs : FRAME_CLOCK_PERIOD_200_MS;
    if (g_frameDelta >= t3) {
        g_period200CountdownMs = 0;
    } else {
        g_period200CountdownMs = t3 - g_frameDelta;
    }
    u32 t4 = g_period400CountdownMs ? g_period400CountdownMs : FRAME_CLOCK_PERIOD_400_MS;
    if (g_frameDelta >= t4) {
        g_period400CountdownMs = 0;
    } else {
        g_period400CountdownMs = t4 - g_frameDelta;
    }
    u32 t5 = g_period500CountdownMs ? g_period500CountdownMs : FRAME_CLOCK_PERIOD_500_MS;
    if (g_frameDelta >= t5) {
        g_period500CountdownMs = 0;
    } else {
        g_period500CountdownMs = t5 - g_frameDelta;
    }
    m_world->ChildGroup()->TickKillCues(0);
    m_world->ChildGroup()->CollideBroadcast();
    Mgr()->m_triggerMgr->UpdateFrame(static_cast<i32>(g_frameDelta));
    m_statusBar->UpdateStatusBar(g_frameDelta);
    SoundStream* win = m_world->GetSoundStream();
    if (win) {
        i32 now = timeGetTime();
        win->TickVolumeRamps(now);
        win->TickStreams(now);
    }
    m_tileTriggers->UpdateTimedLogics(g_frameDelta);
    (static_cast<CMapMgr*>(Mgr()->m_tileGrid))->UpdateDiagonals(Mgr());
    if (ready == false) {
        RenderGameFrame();
    }
    Mgr()->AdvanceComputerPlayerTurns();
    return 1;
}

void CMulti::RenderGameFrame() {
    if (m_roundComplete == false && Mgr()->m_frameGate != false) {
        RestoreCursorSaveUnder();
        DrawVisibleWorld();
        m_statusBar->LoadMainStatusBarSprite();
        CDDrawSurfacePair* h =
            static_cast<CDDrawSurfacePair*>(m_world->GetDrawTarget()->GetBackPair());
        if (h == NULL) {
            return;
        }
        AdvanceCursorAnimation(g_frameDelta);
        SaveUnderAndDrawCursor(h);
        m_world->GetDrawTarget()->GetFrontSurface()->GetSurface()->Flip(NULL);
        return;
    }
    RestoreCursorSaveUnder();
    StepViewportResize();
    if (m_region0Gate != false) {
        (static_cast<CDDrawSurfacePair*>(m_world->GetDrawTarget()->GetBackPair()))
            ->GetSurface()
            ->Fill(0);
        m_statusBar->Deactivate();
    }
    if (m_worldReady == false) {
        if (Mgr()->m_triggerMgr->m_armed != false) {
            Mgr()->m_triggerMgr->ScrollToActiveRecord();
        } else {
            LoadScrollSpeedOptions();
        }
    }
    StepScroll();
    Mgr()->m_worldSounds->SetListenerPosition(
        (m_world->m_level->m_mainPlane)->m_scrollPixelX,
        (m_world->m_level->m_mainPlane)->m_scrollPixelY
    );
    DrawWorldView();
    m_statusBar->LoadMainStatusBarSprite();
    if (m_minimap != NULL && m_statusBar->m_position != STATUSBAR_HIDDEN
        && m_statusBar->m_activeTab != TAB_GAME) {
        RECT rc;
        if (m_statusBar->m_position == STATUSBAR_DOCK_LEFT) {
            SetRect(&rc, 20, 5, 140, 125);
        } else {
            SetRect(
                &rc,
                g_gameReg->GetModeSize().cx - 140,
                5,
                g_gameReg->GetModeSize().cx - 20,
                125
            );
        }
        m_minimap->Refresh(static_cast<i32>(g_frameDelta), false);
        m_minimap->Draw(static_cast<CDDrawSurfacePair*>(m_world->GetDrawTarget()->GetBackPair()), &rc);
    }
    Mgr()->ChatLog()->Scroll(g_frameDelta);
    CDDrawSurfacePair* h = static_cast<CDDrawSurfacePair*>(m_world->GetDrawTarget()->GetBackPair());
    if (h == NULL) {
        return;
    }
    m_chatBox->LoadChatBoxSprite(h);
    DrawDebugStats();
    Mgr()->m_triggerMgr->RenderActionOptionsMenu();
    AdvanceCursorAnimation(g_frameDelta);
    SaveUnderAndDrawCursor(h);
    if (m_worldReady != false) {
        h->DrawBox(&m_hudRect, 0xff);
    }
    m_world->GetDrawTarget()->GetFrontSurface()->GetSurface()->Flip(NULL);
    UpdateMgrScroll(g_gameReg, m_statusBar, m_region0Gate);
    if (m_world->m_level->m_mainPlane != NULL) {
        (m_world->m_level->m_mainPlane)->DeactivateDistantObjects();
    }
    if (m_region0Gate != false) {
        if (m_region0Timing.Expired()) {
            SetTinyViewportCurse(false);
        }
    }
    if (m_region1Gate != false) {
        if (m_region1Timing.Expired()) {
            SetDarknessCurse(false);
        }
    }
}

i32 CMulti::StartTitle() {
    Mgr()->m_lobbyResult = 0;
    m_lobbyLaunch = true;
    if (!m_netMgr) {
        return 0;
    }
    CRezDir* saved = m_stateResources;
    CRezDir* st = m_resourceArchive->GetDirFromPath("STATEZ_ATTRACT");
    m_stateResources = st;
    if (!st) {
        return 0;
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    std::string title;
    title = formatText("TITLE%d", idx);

    if (LoadAndPresentTitlePage((title), 0, 0, 1, 0) == 0) {
        m_stateResources = saved;
        return 0;
    }

    m_world->GetDrawTarget()->PresentBackPage();

    m_world->GetDeviceManager()->FlipToGDISurface();
    m_stateResources = saved;
    while (ShowCursor(true) < 0) {
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
        m_isHost = true;
    } else {
        m_isHost = false;
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

char* CNetSessionListNode::SessionName() {
    return m_sessionDesc.lpszSessionNameA;
}

void CMulti::SetGameName(const std::string& s) {
    m_gameName = s;
}

void CMulti::SetPlayerName(const std::string& s) {
    m_playerName = s;
}

i32 CMulti::Open() {
    if (!Network()) {
        return 0;
    }
    LoadAndPresentTitlePage("BACKGND", 0, 0, 1, 0);
    m_world->GetDrawTarget()->PresentBackPage();
    CNetProviderNode* provider = SelectNetworkProvider();
    if (!provider) {
        return 0;
    }
    if (!Network()->InitializeFromProvider(provider, g_dplayAppGuid)) {
        return 0;
    }
    if (g_hostServicesMode) {
        m_isHost = true;
        if (!DetectConnectionConfig()) {
            return 0;
        }
    } else {
        m_isHost = false;
        if (!JoinSession()) {
            return 0;
        }
    }
    return 1;
}

void CMulti::Close() {}

CNetProviderNode* CMulti::SelectNetworkProvider() {
    if (Network()->EnumServiceProviders(false) != 0) {
        ReportNetError(0);
        return NULL;
    }

    if (g_hostServicesMode != false) {
        if (RunErrorDialog("MULTI_HOSTSERVICES", NetSetupDlgProc, 0) != 0) {
            Settings* store = NetGameMgr()->m_settings;
            if (store != NULL && g_serviceId != NET_SERVICE_NONE) {
                store->setInt("Service", g_serviceId);
                {
                    store->setString(
                        "Player Name",
                        PlayerName()
                    );
                }
                {
                    store->setString(
                        "Game Name",
                        GameName()
                    );
                }
            }
        }
    } else {
        if (RunErrorDialog("MULTI_JOINSERVICES", NetSetupDlgProc, 0) != 0) {
            Settings* store = NetGameMgr()->m_settings;
            if (store != NULL) {
                if (g_serviceId != NET_SERVICE_NONE) {
                    store->setInt("Service", g_serviceId);
                }
                store->setString(
                    "Player Name",
                    PlayerName()
                );
            }
        }
    }
    return Network()->m_selectedProvider;
}

std::string CMulti::GameName() {
    return m_gameName;
}

BOOL CALLBACK NetSetupDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {

    char nameBuf[0xa];
    char gameBuf[0x44];
    NetLobby::g_curDlg = hDlg;
    if (BlockScreenSaver(hDlg, msg, wParam, lParam) != 0) {
        return true;
    }

    switch (msg) {
        case WM_INITDIALOG: {
            HWND combo = GetDlgItem(hDlg, 0x3fc);
            g_netMgr->m_selectedProvider = NULL;
            g_netMgr->PopulateProviderList(combo, 0);
            if (g_serviceId == NET_SERVICE_NONE) {
                ListBox_SetCurSel(combo, 0);
            } else if (ListBox_SetCurSel(combo, g_serviceId) == LB_ERR) {
                ListBox_SetCurSel(combo, 0);
            }

            const std::string playerName = g_gameReg->m_settings->getString("Player Name", "Player").substr(0, 9);
            const std::string gameName = g_gameReg->m_settings->getString("Game Name", "Multiplayer Gruntz").substr(0, 63);

            HWND edName = GetDlgItem(hDlg, 0x51b);
            Edit_LimitText(edName, 9);
            SetDlgItemTextA(hDlg, 0x51b, playerName.c_str());
            HWND edGame = GetDlgItem(hDlg, 0x51c);
            Edit_LimitText(edGame, 0x3f);
            SetDlgItemTextA(hDlg, 0x51c, gameName.c_str());
            return true;
        }
        case WM_COMMAND:
            break;
        default:
            goto ret_false;
    }

    if (wParam == IDCANCEL) {
        EndDialog(hDlg, 0);
        return true;
    }

    if (wParam == IDOK) {

        GetDlgItemTextA(hDlg, 0x51b, gameBuf, 0xa);
        if (gameBuf[0] == 0) {
            MessageBeep(MB_OK);
            return wParam;
        }
        g_connectRptMgr->SetPlayerName(std::string(gameBuf));

        if (g_hostServicesMode != false) {
            GetDlgItemTextA(hDlg, 0x51c, gameBuf, 0x40);
            if (gameBuf[0] == 0) {
                MessageBeep(MB_OK);
                return true;
            }
            g_connectRptMgr->SetGameName(std::string(gameBuf));
        }

        HWND combo = GetDlgItem(hDlg, 0x3fc);
        i32 svc = ListBox_GetCurSel(combo);
        if (svc != LB_ERR) {
            g_serviceId = svc;
        }
        g_netMgr->ReadProviderSelection(GetDlgItem(hDlg, 0x3fc));
        EndDialog(hDlg, 1);
        return true;
    }
ret_false:
    return false;
}

void CMulti::ReportVersionMsg(const char* msg, i32 code) {
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

void CMulti::ReportStatusId(u32 strId, i32 level) {
    char buf[0x12a];
    if (Mgr() && Mgr()->m_owner->m_hInstance) {
        if (!LoadStringA(Mgr()->m_owner->m_hInstance, strId, buf, 0xfa)) {
            strcpy(buf, "Error.");
        }
        ReportVersionMsg(buf, level);
    }
}

void CMulti::ReportNetError(i32 level) {
    char buf[512];
    if (Mgr() && g_code != HRESULT_CODE(DPERR_USERCANCEL)) {
        sprintf(buf, "Error: %s - %i", g_szCode, g_code);
        ReportVersionMsg(buf, level);
    }
}

i32 CMulti::JoinSession() {
    if (RunErrorDialog("MULTI_JOIN", MultiJoinDlgProc, 0) == 0) {
        return 0;
    }
    BroadcastPlayerIdMessage(NETMSG_REQUEST_PLAYER_TABLE, DPSEND_GUARANTEED);
    return 1;
}

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
                return true;
            }
            goto close;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                KillTimer(hDlg, 1);
                EndDialog(hDlg, 0);
                return true;
            }

            if (wParam == IDOK) {
                KillTimer(hDlg, 1);

                if ((static_cast<CMulti*>(g_connectRptMgr))->OnJoinConfirm(hDlg) == 0) {
                    MessageBeep(MB_OK);
                    i32 t = 0x7d0;
                    CNetProviderNode* provider = g_netMgr->m_selectedProvider;
                    if (provider && provider->IsTcpIpProvider()) {
                        t = 0x1388;
                    }
                    SetTimer(hDlg, 1, t, NULL);
                    return false;
                }
                EndDialog(hDlg, 1);
                return true;
            }
            break;
        case WM_TIMER:
            KillTimer(hDlg, 1);
            {
                i32 sel = ListBox_GetCurSel(g_sessionListHwnd);
                i32 hr = g_netMgr->EnumerateSessions(0, 0);
                if (hr == DPERR_USERCANCEL) {
                    goto close;
                }
                if (hr != 0) {
                    if (g_connectRptMgr == NULL) {
                        goto close;
                    }
                    (static_cast<CMulti*>(g_connectRptMgr))->ReportNetError(0);
                    EndDialog(hDlg, 0);
                    return true;
                }
                FillSessionList(g_sessionListHwnd, g_netMgr);
                if (sel != LB_ERR) {
                    ListBox_SetCurSel(g_sessionListHwnd, sel);
                } else {
                    ListBox_SetCurSel(g_sessionListHwnd, 0);
                }
                RefreshSessionSelection(hDlg, g_sessionListHwnd);
                i32 t = 0x7d0;
                CNetProviderNode* provider = g_netMgr->m_selectedProvider;
                if (provider && provider->IsTcpIpProvider()) {
                    t = 0x1388;
                }
                SetTimer(hDlg, 1, t, NULL);
            }
            return true;
    }
ret_false:
    return false;
close:
    EndDialog(hDlg, 0);
ret_true:
    return true;
}

i32 CMulti::DetectConnectionConfig() {
    m_gameClosed = false;
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

    Settings* cfg = NetGameMgr()->m_settings;
    std::string kDelay = m_providerConfigPrefix + "_CmdDelay";
    std::string kResend = m_providerConfigPrefix + "_Resend";
    std::string kDyn = m_providerConfigPrefix + "_DynCmdDelay";
    i32 cd = cfg->getInt(kDelay, -1);
    i32 rs = cfg->getInt(kResend, -1);
    if (cd != -1 && rs != -1) {
        m_commandDelay = cd;
        m_resendInterval = rs;
    }

    GruntzPlayer* hostPlayer = NetGameMgr()->m_players;

    hostPlayer->m_name = PlayerName();
    hostPlayer->m_color = TINT_ORANGE;

    CNetSessionListNode* r = CreateHostSessionAndPlayer();
    if (r == NULL) {
        return 0;
    }
    Network()->m_selectedSession = r;
    return 1;
}

void CMulti::ApplyCmdDelayDefaults() {
    Settings* reg = g_gameReg->m_settings;

    std::string cmdDelayName = m_providerConfigPrefix + "_CmdDelay";
    std::string resendName = m_providerConfigPrefix + "_Resend";
    std::string dynCmdName = m_providerConfigPrefix + "_DynCmdDelay";

    reg->setInt(cmdDelayName, m_commandDelay);
    reg->setInt(resendName, m_resendInterval);
}

i32 CMulti::ShowMultiStartDlg() {
    CMultiStartDlg dlg(m_mgr, NULL);
    i32 r = m_mgr->ExitModalUI(&dlg, false);
    g_netMessageEditHwnd = NULL;
    if (r != 1) {
        if (m_isHost != false) {
            GruntzPlayer* rec = m_mgr->FindPlayerByNetworkId(m_localPlayerId);
            if (rec == NULL) {
                return 0;
            }
            rec->m_active = false;
            SetPlayerColorAvailable(static_cast<ColorTint>(rec->m_color), true);
            BroadcastPlayerTable(NULL);
        }
        if (m_isHost == false && m_removedByHost == false) {
            BroadcastPlayerIdMessage(NETMSG_PLAYER_LEFT, DPSEND_GUARANTEED);
        }
        return 0;
    }

    if (m_isHost != false) {
        ApplyCmdDelayDefaults();
    } else {
        PlayRegistryCueIfElapsed(m_world->SoundRegistry(), g_gameKey);
        ActiveWait(0xfa);
    }
    return 1;
}

void FillSessionList(HWND hList, CNetMgr* manager) {
    char buf[256];
    if (!hList) {
        return;
    }
    if (!manager) {
        return;
    }
    ListBox_ResetContent(hList);
    CNetSessionListNode* listing = manager->GetFirstSession();
    while (listing) {

        MsgParam name;
        i32 itemIndex;
        if (Sparam_Get(buf, listing->m_sessionDesc.lpszSessionNameA, "NAME")) {
            name.m_str = buf;
            itemIndex = ListBox_AddString(hList, name.m_lparam);
        } else {
            name.m_str = listing->m_sessionDesc.lpszSessionNameA;
            itemIndex = ListBox_AddString(hList, name.m_lparam);
        }
        if (itemIndex != LB_ERR) {
            MsgParam cookie;
            cookie.m_sessionListing = listing;
            ListBox_SetItemData(hList, itemIndex, cookie.m_lparam);
        }

        listing = manager->GetNextSession();
    }
}

void RefreshSessionSelection(HWND hDlg, HWND hList) {}

CNetSessionListNode* CMulti::CreateHostSessionAndPlayer() {
    char buf[0x100] = "";
    Sparam_Add(buf, "NAME", (m_gameName).c_str());
    Sparam_Add(buf, "CMDDELAY", m_commandDelay);
    Sparam_Add(buf, "RESEND", m_resendInterval);
    Sparam_Add(buf, "LEVEL", SelectedLevelIndex());

    CNetSessionListNode* enumResult = g_netMgr->CreateSession(4, buf, 0, "");
    if (enumResult == NULL) {
        g_connectRptMgr->ReportNetError(0);
        return NULL;
    }

    CNetPlayerNode* node = Network()->CreatePlayer(const_cast<char*>("Host"), "", NULL);
    m_localPlayer = node;
    if (node == NULL) {
        ReportNetError(0);
        return NULL;
    }

    m_localPlayerId = node->m_playerId;
    GruntzPlayer* hostPlayer = NetGameMgr()->m_players;
    ColorTint hostColor = static_cast<ColorTint>(hostPlayer->m_color);

    bool failed = RegisterLocalPlayer((hostPlayer->GetName()), hostColor, -1, m_localPlayerId) == 0;
    return failed ? NULL : enumResult;
}

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
        Network()
            ->JoinSessionAndCreatePlayer(sel, PlayerName().c_str(), "", NULL);
    if (LocalPlayer() == NULL) {
        ReportNetError(0);
        return 0;
    }

    char buf[0x100];

    if (Sparam_Get(buf, sel->m_sessionDesc.lpszSessionNameA, "CMDDELAY")) {
        m_commandDelay = atoi(buf);
    }
    if (Sparam_Get(buf, sel->m_sessionDesc.lpszSessionNameA, "RESEND")) {
        m_resendInterval = atoi(buf);
    }
    if (Sparam_Get(buf, sel->m_sessionDesc.lpszSessionNameA, "NAME")) {
        SetGameName(std::string(buf));
    }
    m_syncGate = false;
    SelectedLevelIndex() = 1;
    m_localPlayerId = LocalPlayer()->m_playerId;
    if (Sparam_Get(buf, sel->m_sessionDesc.lpszSessionNameA, "LEVEL")) {
        SelectedLevelIndex() = atoi(buf);
    }

    CNetPlayerRegistrationPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= NET_PACKET_APPLICATION;
    packet.m_messageId = NETMSG_REGISTER_PLAYER;

    packet.m_networkPlayerId = m_localPlayerId;
    packet.m_active = true;
    packet.m_color = TINT_ORANGE;
    packet.m_humanControlled = true;
    packet.m_difficulty = BZDIFF_EASY;
    packet.m_preferredPlayerIndex = NET_PREFERRED_PLAYER_INDEX_ANY;
    packet.m_ready = false;
    packet.m_maxGruntz = TM_UNITS_PER_PLAYER;
    if (!copyTextToBuffer((PlayerName()), packet.m_name, sizeof(packet.m_name))) return 0;
    BroadcastPacket(&packet, sizeof(packet), DPSEND_GUARANTEED);
    return 1;
}

i32 CMulti::VerifyCustomLevel(CNetSessionListNode* session, CNetPlayerNode* localPlayer) {
    if (session == NULL) {
        goto notVerified;
    }
    if (localPlayer == NULL) {
        goto notVerified;
    }

    if (m_customLevelVerificationPending != false) {
        i32 cfgId = m_usesCustomLevel;

        i32 token = (g_gameReg)->ResolveLevelChecksum(
            false,
            false,
            cfgId,
            0,
            cfgId != 0 ? CustomLevelName() : BuiltInLevelName()
        );

        g_connectRptMgr->m_levelVerifyResult = false;
        if (g_connectRptMgr->Poll(token) == 0) {
            m_customLevelVerificationPending = false;
            g_gameReg->EnterModalUI(
                "Unable to verify custom level with other players. The game will not start."
            );
            goto notVerified;
        }

        if (g_connectRptMgr->m_levelVerifyResult != false) {
            return 1;
        }
        g_gameReg->EnterModalUI("Not all players have the (same) custom level.");
        m_customLevelVerificationPending = false;
        goto notVerified;
    }
    PollSession();
notVerified:
    return 0;
}

i32 CMulti::PollSessionGated(i32 sessionGate, i32 pollGate) {
    if (sessionGate == 0) {
        return 0;
    }
    if (pollGate == 0) {
        return 0;
    }
    if (m_allPlayersReady != false) {
        return 1;
    }
    PollSession();
    return m_allPlayersReady != false;
}

i32 CMulti::BroadcastValuePacket(CNetValuePacket* packet, i32 flags) {
    packet->m_flags |= NET_PACKET_APPLICATION;
    i32 hr = Network()->BroadcastFrom(LocalPlayer(), flags, packet, sizeof(*packet));
    return hr == 0;
}

void CMulti::BroadcastPlayerIdMessage(NetMsgId id, i32 flag) {
    CNetValuePacket pkt;
    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = id;
    pkt.m_value = LocalPlayer()->m_playerId;
    BroadcastValuePacket(&pkt, flag);
}

void CMulti::BroadcastValueMessage(NetMsgId id, u32 value, i32 flag) {
    CNetValuePacket pkt;
    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = id;
    pkt.m_value = value;
    BroadcastValuePacket(&pkt, flag);
}

i32 CMulti::BroadcastPacket(void* packet, i32 packetSize, i32 flags) {
    if (packet == NULL) {
        return 0;
    }
    i32 hr = Network()->BroadcastFrom(LocalPlayer(), flags, packet, packetSize);
    return hr == 0;
}

i32 CMulti::SendValuePacketTo(CNetPlayerNode* recipient, CNetValuePacket* packet, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    packet->m_flags |= NET_PACKET_APPLICATION;
    i32 hr = Network()->Send(LocalPlayer(), recipient, flags, packet, sizeof(*packet));
    return hr == 0;
}

i32 CMulti::SendPlayerIdMessageTo(CNetPlayerNode* recipient, NetMsgId messageId, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    CNetValuePacket pkt;
    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = messageId;
    pkt.m_value = LocalPlayer()->m_playerId;
    return SendValuePacketTo(recipient, &pkt, flags);
}

i32 CMulti::SendPlayerIdMessageToId(i32 recipientId, NetMsgId messageId, i32 flags) {
    CNetValuePacket pkt;
    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = messageId;
    pkt.m_value = LocalPlayer()->m_playerId;
    i32 hr = Network()->SendById(LocalPlayer()->m_playerId, recipientId, flags, &pkt, sizeof(pkt));
    return hr == 0;
}

i32 CMulti::SendValueMessageTo(CNetPlayerNode* recipient, i32 messageId, u32 value, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    CNetValuePacket pkt;
    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = static_cast<NetMsgId>(messageId);
    pkt.m_value = value;
    return SendValuePacketTo(recipient, &pkt, flags);
}

i32 CMulti::SendPacketTo(CNetPlayerNode* recipient, void* packet, i32 packetSize, i32 flags) {
    if (recipient == NULL) {
        return 0;
    }
    if (packet == NULL) {
        return 0;
    }
    i32 hr = Network()->Send(LocalPlayer(), recipient, flags, packet, packetSize);
    return hr == 0;
}

i32 CMulti::SendValueMessageToId(i32 recipientId, NetMsgId messageId, i32 value, i32 flags) {
    CNetValuePacket pkt;
    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = messageId;
    pkt.m_value = value;
    i32 hr = Network()->SendById(LocalPlayer()->m_playerId, recipientId, flags, &pkt, sizeof(pkt));
    return hr == 0;
}

i32 CMulti::PollSession() {
    if (LocalPlayer() == NULL) {
        return 0;
    }

    i32 count = Network()->GetMessageCount(LocalPlayer());
    if (count <= 0) {
        return 0;
    }

    i32 dispatched;
    DPID sender;
    sender = 0;
    dispatched = 0;

    i32 hr = 0;
    while (hr == 0 && count > 0) {
        if (m_pollAbort) {
            break;
        }

        DWORD messageSize = sizeof(g_recvBuffer);
        hr = Network()->ReceiveMessage(&sender, LocalPlayer(), g_recvBuffer, &messageSize);
        if (hr == 0) {
            count--;
            if (sender != LocalPlayer()->m_playerId) {
                DispatchRecvMsg(sender, g_recvBuffer, messageSize);
                dispatched++;
            }
        }
    }
    return dispatched;
}

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
    if (m_connected != false || m_pumpGuard != false) {
        if (senderPlayer != NULL) {
            CNetCmdSlot* slot = Session()->FindSlotByPlayerId(senderPlayer->m_playerId);
            if (slot != NULL) {
                slot->m_latency = 0;
            }
        }
    }

    if (!HAS(msg->m_flags, NET_PACKET_APPLICATION)) {
        return 0;
    }

    switch (msg->m_messageId) {
        case NETMSG_ALL_PLAYERS_READY:
            m_allPlayersReady = true;
            return 1;

        case NETMSG_VERIFY_CUSTOM_LEVEL:
            m_customLevelVerificationPending = true;
            return 1;

        case NETMSG_PLAYER_READY:
            if (m_allPlayersReady != false) {
                break;
            }
            RecordPlayerReady(senderPlayer, senderId);
            break;

        case NETMSG_OPTIONS_OPENED: {
            if (m_connected == false) {
                break;
            }
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>(Mgr()->FindPlayerByNetworkId(senderId));
            if (player == NULL) {
                return 1;
            }
            if (player->m_optionsPresenceCounted == false) {
                player->m_optionsPresenceCounted = true;
                g_playersInOptionsCount++;
            }
            ShowMultiplayerOptionsDialog();
            break;
        }

        case NETMSG_OPTIONS_CLOSED: {
            if (m_connected == false) {
                break;
            }
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>(Mgr()->FindPlayerByNetworkId(senderId));
            if (player == NULL) {
                return 1;
            }
            if (player->m_optionsPresenceCounted == false) {
                break;
            }
            player->m_optionsPresenceCounted = false;
            g_playersInOptionsCount--;
            break;
        }

        case NETMSG_CHAT_LINE: {
            char* text = wire.m_chat->m_text;
            if (g_netMessageEditHwnd != NULL) {
                AppendEditLine(g_netMessageEditHwnd, text);
                break;
            }
            if (m_connected == false) {
                break;
            }
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>(Mgr()->FindPlayerByNetworkId(senderId));
            if (player == NULL) {
                return 1;
            }
            (static_cast<CFontConfig*>(NetGameMgr()->ChatLog()))
                ->AddItem(text, FONT_ITEM_COLORED | FONT_ITEM_SHADOW, IDX(player->m_color));
            SoundCueRegistry* registry = m_world->SoundRegistry();
            if (registry->m_silentMode != false) {
                break;
            }
            SoundCue* cue = registry->FindCue("GAME_CHAT");
            if (cue == NULL) {
                break;
            }
            cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
            break;
        }

        case NETMSG_YOU_WERE_DROPPED:
            if (m_pollAbort != false) {
                break;
            }
            ReportVersionMsg("You have been dropped from the game.", 0);
            PostMessageA(NetGameMgr()->m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            m_pollAbort = true;
            break;

        case NETMSG_APPLY_PLAYER_DROP:
            ApplyPlayerDrop(msg->m_value);
            break;

        case NETMSG_PLAYER_LEFT:
            OnPlayerLeft(senderId);
            ResetPlayerCommands(senderId);
            return g_playerRosterChanged = true;

        case NETMSG_REQUEST_PLAYER_TABLE:
            if (m_isHost == false) {
                break;
            }
            BroadcastPlayerTable(senderPlayer);
            break;

        case NETMSG_PLAYER_TABLE:
            if (m_isHost != false) {
                break;
            }
            ApplyPlayerTable(wire.m_playerTable);
            g_playerRosterChanged = true;
            break;

        case NETMSG_REGISTER_PLAYER: {
            if (m_isHost == false) {
                break;
            }
            if (m_connected != false) {
                break;
            }
            if (Mgr()->CountActivePlayers(true) >= 4) {
                break;
            }

            CNetPlayerRegistrationPacket* registration = wire.m_playerRegistration;
            if (IsPlayerColorAvailable(static_cast<ColorTint>(registration->m_color)) == 0) {
                registration->m_color = FindAvailablePlayerColor();
            }
            SetPlayerColorAvailable(static_cast<ColorTint>(registration->m_color), false);
            RegisterPlayerFromPacket(registration);
            BroadcastPlayerTable(NULL);
            SendGameConfig(senderPlayer);
            g_playerRosterChanged = true;
            break;
        }

        case NETMSG_UPDATE_PLAYER: {
            if (m_isHost == false) {
                break;
            }
            if (m_connected != false) {
                break;
            }
            CNetPlayerUpdatePacket* update = wire.m_playerUpdate;
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>(Mgr()->FindPlayerByNetworkId(update->m_networkPlayerId));
            if (player == NULL) {
                return 0;
            }
            if (player->TrySetColor(static_cast<ColorTint>(update->m_color)) == 0) {
                ColorTint color = static_cast<ColorTint>(player->m_color);
                update->m_color = color;
                SendPlayerIdMessageTo(senderPlayer, NETMSG_COLOR_REJECTED, DPSEND_GUARANTEED);
            }
            ApplyPlayerUpdate(update);
            BroadcastPlayerTable(NULL);
            g_playerRosterChanged = true;
            break;
        }

        case NETMSG_REMOVED_BY_HOST:
            if (m_isHost != false) {
                break;
            }
            m_removedByHost = true;
            break;

        case NETMSG_COLOR_REJECTED:
            if (m_isHost != false) {
                break;
            }
            m_colorSelectionRejected = true;
            break;

        case NETMSG_GAME_CLOSED:
            if (m_isHost != false) {
                break;
            }
            m_gameClosed = true;
            break;

        case NETMSG_GAME_FULL:
            if (m_isHost != false) {
                break;
            }
            m_gameFull = true;
            break;

        case NETMSG_LATENCY_PROBE:
            SendValueMessageToId(senderId, NETMSG_LATENCY_REPLY, msg->m_value, 0);
            break;

        case NETMSG_LATENCY_REPLY: {
            i32 stamp = msg->m_value;
            i32 delta = timeGetTime();
            delta -= stamp;
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>((g_gameReg)->FindPlayerByNetworkId(senderId));
            if (player == NULL) {
                return 1;
            }
            i32 num = player->m_latency.m_avg * player->m_latency.m_count + delta;
            i32 np1 = player->m_latency.m_count + 1;
            player->m_latency.m_count = np1;
            player->m_latency.m_avg = num / np1;
            break;
        }

        case NETMSG_ACK_LATENCY_REPORT: {
            if (m_isHost == false) {
                break;
            }
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>((g_gameReg)->FindPlayerByNetworkId(senderId));
            if (player == NULL) {
                return 1;
            }
            m_playerLatencyMs[player->m_playerIndex] = msg->m_value;
            break;
        }

        case NETMSG_VERIFY_OK:
            m_verifyDone = true;
            m_levelVerifyResult = true;
            return 1;

        case NETMSG_VERIFY_FAILED:
            m_levelVerifyResult = false;
            m_verifyDone = true;
            return 1;

        case NETMSG_LEVEL_CHECKSUM: {
            GruntzPlayer* player =
                static_cast<GruntzPlayer*>((g_gameReg)->FindPlayerByNetworkId(senderId));
            if (player == NULL) {
                return 1;
            }
            m_levelChecksumReceived[player->m_playerIndex] = 1;
            m_levelChecksums[player->m_playerIndex] = msg->m_value;
            break;
        }

        case NETMSG_WAIT_DIALOG_REPLY:
            m_lastSenderId = msg->m_value;
            m_waitDialogReplyReceived = true;
            return 1;

        case NETMSG_OUT_OF_SYNC_REPORT:
            if (m_isHost == false) {
                break;
            }
            if (m_connected == false) {
                break;
            }
            if (m_allPlayersReady == false) {
                break;
            }
            BroadcastPlayerIdMessage(NETMSG_OUT_OF_SYNC, DPSEND_GUARANTEED);
            OnOutOfSync();
            break;

        case NETMSG_OUT_OF_SYNC:
            if (m_connected == false) {
                break;
            }
            OnOutOfSync();
            break;

        case NETMSG_PAUSE:
            if (m_connected == false) {
                break;
            }
            ShowMultiplayerPauseDialog();
            break;

        case NETMSG_REQUEST_CONFIG:
            if (m_isHost == false) {
                break;
            }
            SendGameConfig(senderPlayer);
            break;

        case NETMSG_CONFIG:
            if (ApplyGameConfig(wire.m_gameConfig) == 0) {
                break;
            }
            m_connectAccepted = true;
            break;

        case NETMSG_VERSION_CHECK:
            HandleVersionCheck(wire.m_versionCheck);
            break;

        case NETMSG_VERSION_MISMATCH: {
            std::string result;
            if (senderPlayer != NULL) {
                result = formatText("*** %s has a different version of the game.", (senderPlayer->ShortName()).c_str());
            } else {
                result = formatText("*** A player had a different version of the game.");
            }
            if (g_netMessageEditHwnd != NULL) {
                AppendEditLine(
                    g_netMessageEditHwnd,
                    (result)
                );
            } else {
                (static_cast<CFontConfig*>(NetGameMgr()->ChatLog()))
                    ->AddItem((result), FONT_ITEM_FLAGS_NONE, 0x11);
            }
            break;
        }

        case NETMSG_KEEP_ALIVE:
            break;

        default:
            return 0;
    }
    return 1;
}

std::string CNetPlayerNode::ShortName() {
    return m_shortName;
}

i32 CMulti::HandleSystemMessage(LPDPMSG_GENERIC message, i32 unusedMessageSize) {
    if (message == NULL) {
        return 0;
    }

    switch (message->dwType) {
        case DPSYS_DESTROYPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_system = message;
            if (wire.m_playerDestroyed->dwPlayerType != DPPLAYERTYPE_PLAYER) {
                return 1;
            }
            OnPlayerLeft(wire.m_playerDestroyed->dpId);
            g_playerRosterChanged = true;
            return 1;
        }
        case DPSYS_CREATEPLAYERORGROUP: {
            CNetWireMsg wire;
            wire.m_system = message;
            HandlePlayerCreated(wire.m_playerCreated);
            return 1;
        }
        case DPSYS_HOST:
            m_isHost = true;
            return 1;
        case DPSYS_SESSIONLOST:
            m_sessionTerminated = true;
            return 1;
        default:
            return 0;
    }
}

i32 CMulti::OnPlayerLeft(i32 playerId) {
    CNetPlayerNode* player = Network()->GetPlayerNodeData(playerId);
    if (player == LocalPlayer()) {
        return 0;
    }

    GruntzPlayer* slot = static_cast<GruntzPlayer*>(Mgr()->FindPlayerByNetworkId(playerId));
    if (slot == NULL) {
        return 0;
    }
    if (slot->m_active == false) {
        return 0;
    }
    if (slot->m_humanControlled == false) {
        return 0;
    }

    if (slot->m_optionsPresenceCounted != false) {
        slot->m_optionsPresenceCounted = false;
        g_playersInOptionsCount--;
    }
    slot->m_active = false;
    SetPlayerColorAvailable(slot->m_color, true);

    std::string line = slot->GetName() + " has left the game.";
    (static_cast<CFontConfig*>(NetGameMgr()->ChatLog()))
        ->AddItem((line), FONT_ITEM_SHADOW, 0x11);

    if (player != NULL) {
        Network()->RemovePlayer(player);
    }
    if (m_isHost != false && m_connected == false) {
        BroadcastPlayerTable(NULL);
        g_playerRosterChanged = true;
    }
    return 1;
}

void CMulti::ApplyPlayerDrop(i32 playerId) {
    if (m_allPlayersReady == false) {
        RecordPlayerReady(NULL, playerId);
        CNetCmdSlot* slot = Session()->FindSlotByPlayerId(playerId);
        if (slot != NULL) {
            slot->BeginDrain();
            slot->ClearSyncState();
            slot->m_state = NETSLOT_DONE;
            slot->m_player->m_doneFlag = true;
        }
        return;
    }

    OnPlayerLeft(playerId);
    ResetPlayerCommands(playerId);
}

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
            NULL
        );
        if (player == NULL) {
            return 0;
        }
    }
    if (m_customLevelVerificationPending == false && m_connected == false) {
        if (m_isHost != false) {
            if (Mgr()->CountActivePlayers(true) >= 4) {
                SendPlayerIdMessageToId(message->dpId, NETMSG_GAME_FULL, DPSEND_GUARANTEED);
                return 0;
            }
            if (m_isHost != false) {
                SendVersionCheck(player);
            }
        }
        PlayRegistryCueIfElapsed(m_world->SoundRegistry(), "GAME_MENUS_SELECT");
        return 1;
    }
    SendPlayerIdMessageToId(message->dpId, NETMSG_GAME_CLOSED, DPSEND_GUARANTEED);
    return 1;
}

i32 CMulti::ResolveLocalPlayer() {
    if (Network() == NULL) {
        return 0;
    }
    m_localPlayer = Network()->FindPlayerById(m_localPlayerId);
    return LocalPlayer() != NULL;
}

i32 CMulti::BroadcastPlayerTable(CNetPlayerNode* recipient) {
    CNetPlayerTablePacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= NET_PACKET_APPLICATION;
    packet.m_messageId = STAT_PLAYER_TABLE;

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &NetGameMgr()->m_players[i];
        if (player != NULL) {
            i32 v = player->m_active;
            packet.m_rows[i].m_active = static_cast<u8>(v);
            v = player->m_color;
            packet.m_rows[i].m_color = static_cast<u8>(v);
            v = player->m_humanControlled;
            packet.m_rows[i].m_humanControlled = static_cast<u8>(v);
            v = IDX(player->GetDifficulty());
            packet.m_rows[i].m_difficulty = static_cast<u8>(v);
            v = player->m_ready;
            packet.m_rows[i].m_ready = static_cast<u8>(v);
            v = player->m_maxGruntz;
            packet.m_rows[i].m_maxGruntz = static_cast<u8>(v);
            packet.m_rows[i].m_networkPlayerId = player->m_networkPlayerId;
            if (!copyTextToBuffer((player->GetName()), packet.m_rows[i].m_name, sizeof(packet.m_rows[i].m_name))) return 0;
        }
    }

    if (recipient != NULL) {
        if (SendPacketTo(recipient, &packet, sizeof(packet), DPSEND_GUARANTEED) == 0) {
            return 0;
        }
    } else {
        if (BroadcastPacket(&packet, sizeof(packet), DPSEND_GUARANTEED) == 0) {
            return 0;
        }
    }
    return 1;
}

i32 CMulti::ApplyPlayerTable(CNetPlayerTablePacket* packet) {
    if (packet == NULL) {
        return 0;
    }
    if (m_isHost == false) {
        ResetPlayerColorAvailability();
    }

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &NetGameMgr()->m_players[i];
        if (player != NULL) {
            player->m_active = packet->m_rows[i].m_active;
            player->m_color = static_cast<ColorTint>(packet->m_rows[i].m_color);
            player->SetHumanControlled(packet->m_rows[i].m_humanControlled);
            player->m_difficulty = static_cast<BattlezDifficulty>(packet->m_rows[i].m_difficulty);
            if (packet->m_rows[i].m_ready != false) {
                player->m_ready = true;
            } else {
                player->m_ready = false;
            }
            player->m_maxGruntz = packet->m_rows[i].m_maxGruntz;
            player->m_name = packet->m_rows[i].m_name;
            player->m_networkPlayerId = packet->m_rows[i].m_networkPlayerId;
            if (m_isHost == false && player->m_active != false) {
                SetPlayerColorAvailable(player->m_color, false);
            }
        }
    }
    return 1;
}

i32 CMulti::RegisterLocalPlayer(
    const std::string& name,
    ColorTint color,
    i32 preferredPlayerIndex,
    i32 networkPlayerId
) {
    return RegisterPlayer(name, color, true, BZDIFF_EASY, preferredPlayerIndex, networkPlayerId);
}

i32 CMulti::RegisterPlayer(
    const std::string& name,
    ColorTint color,
    b32 humanControlled,
    BattlezDifficulty difficulty,
    i32 preferredPlayerIndex,
    i32 networkPlayerId
) {
    if (Mgr()->CountActivePlayers(true) >= 4) {
        return 0;
    }

    GruntzPlayer* slot = NULL;
    if (preferredPlayerIndex >= 0 && preferredPlayerIndex <= 4) {
        slot = &NetGameMgr()->m_players[preferredPlayerIndex];
        if (slot != NULL && slot->m_active != false) {
            slot = NULL;
        }
    }
    if (slot == NULL) {

        i32 i;
        GruntzPlayer* candidate;
        for (i = 0, candidate = NetGameMgr()->m_players; i < 4; i++, candidate++) {
            slot = candidate;
            if (candidate != NULL && candidate->m_active == false) {
                break;
            }
            slot = NULL;
        }
        if (slot == NULL) {
            return 0;
        }
    }

    SetPlayerColorAvailable(color, false);

    slot->m_name = name;
    slot->m_color = color;
    slot->SetHumanControlled(humanControlled);
    slot->m_difficulty = difficulty;
    slot->m_ready = false;
    slot->m_networkPlayerId = networkPlayerId;
    slot->m_active = true;
    slot->m_latency.Clear();
    return 1;
}

i32 CMulti::RegisterPlayerFromPacket(CNetPlayerRegistrationPacket* packet) {
    if (packet->m_active != false) {
        RegisterPlayer(
            packet->m_name,
            static_cast<ColorTint>(packet->m_color),
            packet->m_humanControlled,
            static_cast<BattlezDifficulty>(packet->m_difficulty),
            packet->m_preferredPlayerIndex,
            packet->m_networkPlayerId
        );
    }
    return 1;
}

i32 CMulti::DeactivatePlayer(i32 slotIndex) {
    GruntzPlayer* player = &NetGameMgr()->m_players[slotIndex];
    if (player == NULL) {
        return 0;
    }
    if (player->m_active == false) {
        return 0;
    }
    player->m_active = false;
    SetPlayerColorAvailable(player->m_color, true);
    return 1;
}

i32 CMulti::RequestMultiplayerPause() {
    if (m_connected == false) {
        return 0;
    }
    BroadcastPlayerIdMessage(STAT_PAUSE, DPSEND_GUARANTEED);
    ShowMultiplayerPauseDialog();
    return 1;
}

void CMulti::ShowMultiplayerPauseDialog() {
    if (g_pauseDialogOpen) {
        return;
    }

    m_waitDialogReplyReceived = false;
    g_pauseDialogOpen = true;
    NetLobbyCtrlId result =
        static_cast<NetLobbyCtrlId>(RunErrorDialog("MULTI_PAUSE", NetLobby::HostWaitDlgProc, 0));
    g_pauseDialogOpen = false;
    g_netMessageEditHwnd = NULL;

    if (result == IDC_NET_RESTART) {
        HWND hwnd = NetGameMgr()->m_gameWnd->GetHwnd();
        PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MULTI_CONNECT), SelectedLevelIndex());
    }
}

void CMulti::ShowMultiplayerOptionsDialog() {
    if (g_optionsDialogOpen) {
        return;
    }

    m_waitDialogReplyReceived = false;
    g_optionsDialogOpen = true;
    RunErrorDialog("MULTI_OPTIONZ", NetLobby::JoinWaitDlgProc, 0);
    g_optionsDialogOpen = false;
    g_netMessageEditHwnd = NULL;
}

void CMulti::OnOutOfSync() {
    if (m_outOfSync) {
        return;
    }

    m_outOfSync = true;
    m_waitDialogReplyReceived = false;
    NetLobbyCtrlId result = static_cast<NetLobbyCtrlId>(
        RunErrorDialog("MULTI_OUTOFSYNC", NetLobby::SessionWaitDlgProc, 0)
    );
    g_netMessageEditHwnd = NULL;

    switch (result) {
        case IDC_NET_RESTART: {
            HWND hwnd = NetGameMgr()->m_gameWnd->GetHwnd();
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MULTI_CONNECT), SelectedLevelIndex());
            break;
        }
        case IDC_NET_CONTINUE:
            break;
        default: {
            HWND hwnd = NetGameMgr()->m_gameWnd->GetHwnd();
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            break;
        }
    }
}

i32 CMulti::BroadcastPlayerUpdate(GruntzPlayer* player) {
    CNetPlayerUpdatePacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.m_flags |= NET_PACKET_APPLICATION;
    packet.m_messageId = STAT_PLAYER_UPDATE;
    packet.m_playerIndex = player->m_playerIndex;

    i32 v = player->m_color;
    packet.m_color = static_cast<u8>(v);
    v = player->m_humanControlled;
    packet.m_humanControlled = static_cast<u8>(v);
    v = IDX(player->GetDifficulty());
    packet.m_difficulty = static_cast<u8>(v);
    v = player->m_ready;
    packet.m_ready = static_cast<u8>(v);
    packet.m_active = true;
    v = player->m_maxGruntz;
    packet.m_maxGruntz = static_cast<u8>(v);
    v = player->m_networkPlayerId;
    packet.m_networkPlayerId = v;
    if (!copyTextToBuffer((player->GetName()), packet.m_name, sizeof(packet.m_name))) return 0;

    return BroadcastPacket(&packet, sizeof(packet), DPSEND_GUARANTEED);
}

i32 CMulti::ApplyPlayerUpdate(CNetPlayerUpdatePacket* packet) {
    if (packet == NULL) {
        return 0;
    }
    i32 playerIndex = packet->m_playerIndex;
    if (playerIndex < 0 || playerIndex >= 4) {
        return 0;
    }
    GruntzPlayer* player = &NetGameMgr()->m_players[playerIndex];
    if (player == NULL) {
        return 0;
    }

    player->m_name = packet->m_name;
    player->m_color = static_cast<ColorTint>(packet->m_color);
    player->m_difficulty = static_cast<BattlezDifficulty>(packet->m_difficulty);
    if (packet->m_ready != false) {
        player->m_ready = true;
    } else {
        player->m_ready = false;
    }
    player->m_maxGruntz = packet->m_maxGruntz;
    player->SetHumanControlled(packet->m_humanControlled);
    player->m_networkPlayerId = packet->m_networkPlayerId;
    player->m_active = true;
    return 1;
}

i32 CMulti::AnnounceOptionsOpened() {
    g_optionsOpenedPacket.m_messageId = NETMSG_OPTIONS_OPENED;
    g_optionsOpenedPacket.m_flags |= NET_PACKET_APPLICATION;
    g_optionsOpenedPacket.m_value = 0;
    Network()->BroadcastFrom(
        LocalPlayer(),
        DPSEND_GUARANTEED,
        &g_optionsOpenedPacket,
        sizeof(g_optionsOpenedPacket)
    );
    return 1;
}

i32 CMulti::AnnounceOptionsClosed() {
    g_optionsClosedPacket.m_messageId = NETMSG_OPTIONS_CLOSED;
    g_optionsClosedPacket.m_flags |= NET_PACKET_APPLICATION;
    g_optionsClosedPacket.m_value = 0;
    Network()->BroadcastFrom(
        LocalPlayer(),
        DPSEND_GUARANTEED,
        &g_optionsClosedPacket,
        sizeof(g_optionsClosedPacket)
    );
    return 1;
}

i32 CMulti::BroadcastChatLine(std::string text, i32 prefixPlayerName, i32 echoLocally, HWND edit) {
    if (text.empty()) {
        return 0;
    }

    i32 len = static_cast<i32>(text.size());
    if (len > 0x80) {
        text.resize(0x80);
        len = 0x80;
    }

    if (len > 0 && text[len - 1] < ' ') {
        text.resize(len - 1);
        len--;
    }
    if (len > 0 && text[len - 1] < ' ') {
        text.resize(len - 1);
    }

    std::string line = text;
    if (prefixPlayerName != 0) {
        GruntzPlayer* player =
            Mgr()->FindPlayerByNetworkId(LocalPlayer()->m_playerId);
        if (player == NULL) return 0;
        line = player->GetName() + ": " + text;
    }
    if (!copyTextToBuffer(line, g_netChatPacket.m_text, sizeof(g_netChatPacket.m_text))) {
        return 0;
    }

    if (echoLocally != 0 && edit != NULL) {
        AppendEditLine(edit, line);
    } else if (echoLocally != 0) {

        GruntzPlayer* player =
            static_cast<GruntzPlayer*>(Mgr()->FindPlayerByNetworkId(m_localPlayerId));
        if (player == NULL) {
            return 0;
        }
        (static_cast<CFontConfig*>(NetGameMgr()->ChatLog()))
            ->AddItem(line, FONT_ITEM_COLORED | FONT_ITEM_SHADOW, IDX(player->m_color));
    }

    g_netChatPacket.m_messageId = STAT_CHAT;

    i32 packetLen = static_cast<i32>(line.size()) + offsetof(CNetChatPacket, m_text) + 1;
    g_netChatPacket.m_value = 0;
    g_netChatPacket.m_flags |= NET_PACKET_APPLICATION;
    Network()->BroadcastFrom(LocalPlayer(), DPSEND_GUARANTEED, &g_netChatPacket, packetLen);
    return 1;
}

void CMulti::AppendEditLine(HWND edit, const std::string& str) {
    if (!edit || str.empty()) {
        return;
    }
    i32 len = Edit_GetTextLength(edit);
    if (len == 0) {
        Edit_SetSel(edit, len, -1);
    } else {
        Edit_SetSel(edit, len, len);
    }
    const std::string buffer = (len > 0 ? "\r\n" : "") + str;
    MsgParam text;
    text.m_str = buffer.c_str();
    Edit_ReplaceSel(edit, text.m_lparam);
    Edit_Scroll(edit, 0x270f, 0);
}

i32 CMulti::DropLobbyPlayer(i32 slotIndex) {
    if (slotIndex < 0 || slotIndex >= 4) {
        return 0;
    }
    if (m_isHost == false) {
        return 0;
    }

    GruntzPlayer* slot = &NetGameMgr()->m_players[slotIndex];
    if (slot == NULL) {
        return 0;
    }

    CNetPlayerNode* player = Network()->GetPlayerNodeData(slot->m_networkPlayerId);

    b32 humanControlled = slot->m_humanControlled;
    if (player == NULL) {
        if (humanControlled != false) {
            return 0;
        }
    } else if (humanControlled != false) {
        SendPlayerIdMessageTo(player, STAT_REMOVED_BY_HOST, DPSEND_GUARANTEED);
    }

    if (DeactivatePlayer(slotIndex) == 0) {
        return 0;
    }
    BroadcastPlayerTable(NULL);
    g_playerRosterChanged = true;
    return 1;
}

void CMulti::RecordPlayerReady(CNetPlayerNode* unusedPlayer, i32 playerId) {
    if (m_allPlayersReady != false) {
        return;
    }
    if (playerId == m_localPlayerId) {
        return;
    }

    i32 count = static_cast<i32>(m_readyPlayerIds.size());
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

    i32 total = static_cast<i32>(m_readyPlayerIds.size());
    i32 recorded = 0;
    for (i = 0; i < total; i++) {
        if (m_readyPlayerIds[i] != 0) {
            recorded++;
        }
    }
    if (recorded < stateThree) {
        return;
    }

    BroadcastPlayerIdMessage(STAT_ALL_PLAYERS_READY, DPSEND_GUARANTEED);
    BroadcastPlayerIdMessage(STAT_ALL_PLAYERS_READY, DPSEND_GUARANTEED);
    m_allPlayersReady = true;
}

i32 CMulti::WaitForOtherPlayers() {
    std::vector<u32>* votes = &m_readyPlayerIds;
    votes->clear();
    for (i32 k = 3; k != 0; k--) {
        votes->push_back(0);
    }
    if (static_cast<i32>(Network()->m_players.size()) == 1) {
        goto ready;
    }
    {
        i32 count = 0;
        CNetCmdSlot* slot = m_session->m_slots;
        for (i32 j = PLAYER_SLOT_COUNT; j != 0; j--) {
            if (slot != NULL && slot->m_state == NETSLOT_ACTIVE) {
                count++;
            }
            slot++;
        }
        if (count != 0) {
            BroadcastPlayerIdMessage(NETMSG_PLAYER_READY, DPSEND_GUARANTEED);
            std::string waitStr("Waiting for other playerz...");
            CGruntzMgr* g = g_gameReg;

            RECT rc;
            tagSIZE mode = g->GetModeSize();
            rc.right = g->GetModeSize().cx;
            rc.bottom = mode.cy;
            rc.left = 0;
            rc.top = 0;
            DrawTextToFrontSurface(g->m_world, waitStr, &rc, 0x82, 1, 0xff, 0xff, 0, 1);

            i32 resend = 0x1388;
            i32 abort = 0x1d4c0;
            while (m_allPlayersReady == false) {
                u32 start = timeGetTime();
                Sleep(0x32);
                PollSession();
                if (GetAsyncKeyState(VK_ESCAPE) & 0x80000000) {
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
                    SendLobbyKeepAlive();
                    BroadcastPlayerIdMessage(NETMSG_PLAYER_READY, DPSEND_GUARANTEED);
                }
            }

            g_roundStartTimeMs = timeGetTime();

            if (g_gameReg->m_musicEnabled != false) {
                NetGameMgr()->m_midi->PlaySequence(formatText("AMBIENT%d", GetAmbientId()), true);
            }
            return 1;
        }
    }

ready:
    m_allPlayersReady = true;
    return 1;
}

i32 CMulti::Poll(i32 token) {
    if (m_isHost == false) {
        BroadcastValueMessage(STAT_LEVEL_CHECKSUM, token, DPSEND_GUARANTEED);
        i32 resend = 0x1388;
        i32 abort = 0x3a98;
        m_verifyDone = false;

        while (m_verifyDone == false) {
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
                SendLobbyKeepAlive();
                BroadcastValueMessage(STAT_LEVEL_CHECKSUM, token, DPSEND_GUARANTEED);
            }
        }
        return 1;
    }

    i32 abort = 0x3a98;
    m_verifyDone = false;
    for (i32 i = 0; i < 4; i++) {
        m_levelChecksumReceived[i] = 0;
        m_levelChecksums[i] = 0;
    }
    while (m_verifyDone == false) {
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
            GruntzPlayer* player = &g_gameReg->m_players[i];
            if (player->m_networkPlayerId != m_localPlayerId && player->m_active != false
                && player->m_humanControlled != false) {
                if (m_levelChecksumReceived[i] == 0) {
                    allAcked = 0;
                } else if (!(m_levelChecksums[i] == token && token != 0)) {
                    allAgree = 0;
                }
            }
        }
        if (allAcked != 0) {
            if (allAgree != 0) {
                BroadcastPlayerIdMessage(STAT_VERIFY_AGREE, DPSEND_GUARANTEED);
                m_levelVerifyResult = true;
                m_verifyDone = true;
            } else {
                BroadcastPlayerIdMessage(STAT_VERIFY_DISAGREE, DPSEND_GUARANTEED);
                m_levelVerifyResult = false;
                m_verifyDone = true;
            }
        }
    }
    return 1;
}

i32 CMulti::CreateSession() {
    CNetSessionListNode* rec = g_netMgr->m_selectedSession;
    if (rec == NULL) {
        return 0;
    }
    Network()->EnumerateSessionPlayers(rec, DPENUMPLAYERS_ALL);
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
    i32 commandTickSnapshot = m_session->m_commandTick;
    u8 b = static_cast<u8>(commandTickSnapshot);
    if (b == 0) {
        b = 0x7f;
    } else {
        b = b - 1;
    }
    m_processedCommandTick = b;

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &NetGameMgr()->m_players[i];
        NetSlotState state = NETSLOT_INACTIVE;
        if (player->m_active != false && player->m_humanControlled != false) {

            state = NETSLOT_LOCAL;
            if (player->m_networkPlayerId != m_localPlayerId) {
                state = NETSLOT_REMOTE;
            }
        }
        if (Session()->CreateSlot(i, state) == NULL) {
            return 0;
        }
    }
    return 1;
}

CNetCmdSlot::CNetCmdSlot() {
    m_state = NETSLOT_EMPTY;
    m_isDraining = false;
    m_drainSequence = 0;
    m_player = NULL;
    m_latency = 0;
    m_contiguousSequence = 0;
    m_peerWindowBase = 0;
    m_owner = NULL;
    ResetNetCmdSlotCommandWindow(this);
}

void CNetSession::InitializeFields() {
    m_mgr = NULL;
    m_owner = NULL;
    m_netMgr = NULL;
    m_localPlayer = NULL;
    m_commandTick = 0;
    m_batchBuilt = false;
    m_sequence = 0;
    m_commandPeriod = 1;

    i32 i;
    for (i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &m_slots[i];
        slot->m_state = NETSLOT_EMPTY;
        slot->m_isDraining = false;
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

u32 CMulti::FrameSyncWait() {
    u32 now = timeGetTime();
    u32 ret = 0;
    m_accumTime = now - m_lastFrameSyncTime;
    m_lastFrameSyncTime = now;

    if (m_accumTime <= 0x1e) {
        ActiveWait(0x1f - m_accumTime);
        m_lastFrameSyncTime = (now - m_accumTime) + 0x1f;
    } else if (m_accumTime > 0x28 && m_syncGate) {
        g_frameSkipToggle ^= 1;
        ret = g_frameSkipToggle;
    }
    return ret;
}

void CMulti::ShowDropPlayerDialog() {
    if (g_dropDialogOpen) {
        return;
    }

    m_waitDialogReplyReceived = false;
    g_dropDialogOpen = true;
    NetLobbyCtrlId result = static_cast<NetLobbyCtrlId>(
        RunErrorDialog("MULTI_DROPPLAYER", NetLobby::NetGameDlgProc, 0)
    );
    g_dropDialogOpen = false;
    g_netMessageEditHwnd = NULL;

    switch (result) {
        case IDC_NET_CONTINUE:
            Session()->ResetLatencies();
            break;
        case IDC_NET_ABORT: {
            Session()->ResetLatencies();
            HWND hwnd = NetGameMgr()->m_gameWnd->GetHwnd();
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            break;
        }
        case IDC_NET_DROP_PLAYER:
            if (g_dropPlayerId != NET_PLAYER_ID_NONE) {
                if (Network()->FindPlayerById(g_dropPlayerId)) {
                    SendPlayerIdMessageToId(
                        g_dropPlayerId,
                        STAT_YOU_WERE_DROPPED,
                        DPSEND_GUARANTEED
                    );
                }
            }
            BroadcastValueMessage(STAT_APPLY_PLAYER_DROP, g_dropPlayerId, DPSEND_GUARANTEED);
            ApplyPlayerDrop(g_dropPlayerId);
            Session()->ResetLatencies();
            break;
    }
}

i32 CMulti::RunErrorDialog(char* tmpl, DLGPROC handler, i32 lparam) {
    if (!Mgr()) {
        return 2;
    }
    Mgr()->VoiceMgr()->PauseAllVoices();
    i32 r = Mgr()->RunModalDialog(tmpl, handler, lparam);
    SetActiveAndFocus(Mgr()->m_gameWnd->GetHwnd());
    SendLobbyKeepAlive();
    return r;
}

void CMulti::CheckDropTimeout() {
    if (m_session->FindLaggingSlot(0x1388) == NULL) {
        return;
    }
    if (g_ackThrottleDeadline < static_cast<u32>(timeGetTime())) {
        SendLobbyKeepAlive();
        g_ackThrottleDeadline = timeGetTime() + 0x3e8;
    }
    CNetCmdSlot* slot = m_session->FindLaggingSlot(0x2710);
    if (slot == NULL) {
        return;
    }
    g_dropPlayerId = slot->m_player->m_networkPlayerId;
    g_sessionName = slot->GetPlayerName();
    BroadcastValueMessage(NETMSG_DROP_TIMEOUT, g_dropPlayerId, DPSEND_GUARANTEED);
    ShowDropPlayerDialog();
}

std::string CNetCmdSlot::GetPlayerName() {
    return m_player->GetName();
}

void CMulti::SendLobbyKeepAlive() {
    if (m_netMgr && m_localPlayer && m_connected) {
        BroadcastPlayerIdMessage(NETMSG_KEEP_ALIVE, DPSEND_GUARANTEED);
    }
}

i32 CMulti::SetupTcpIpConfig() {
    m_providerConfigPrefix = "TcpIp";
    m_gameClosed = false;
    m_commandDelay = 5;
    m_resendInterval = 0x3c;

    Settings* cfg = NetGameMgr()->m_settings;
    std::string kDelay = m_providerConfigPrefix + "_CmdDelay";
    std::string kResend = m_providerConfigPrefix + "_Resend";
    std::string kDyn = m_providerConfigPrefix + "_DynCmdDelay";
    i32 cd = cfg->getInt(kDelay, -1);
    i32 rs = cfg->getInt(kResend, -1);
    if (cd != -1 && rs != -1) {
        m_commandDelay = cd;
        m_resendInterval = rs;
    }

    GruntzPlayer* hostPlayer = NetGameMgr()->m_players;

    hostPlayer->m_name = PlayerName();
    hostPlayer->m_color = TINT_ORANGE;

    m_localPlayer = static_cast<CNetPlayerNode*>(Network()->CreatePlayer(
        (hostPlayer->GetName()).c_str(),
        "",
        NULL
    ));
    if (LocalPlayer() == NULL) {
        ReportNetError(0);
        return 0;
    }

    m_localPlayerId = LocalPlayer()->m_playerId;
    ColorTint hostColor = static_cast<ColorTint>(hostPlayer->m_color);

    if (RegisterLocalPlayer((hostPlayer->GetName()), hostColor, -1, m_localPlayerId) == 0) {
        return 0;
    }
    return 1;
}

i32 CMulti::CreateLocalPlayer() {
    {
        m_localPlayer = static_cast<CNetPlayerNode*>(Network()->CreatePlayer(
            PlayerName().c_str(),
            "",
            NULL
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

    CNetPlayerRegistrationPacket pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.m_flags |= NET_PACKET_APPLICATION;
    pkt.m_messageId = STAT_REGISTER_PLAYER;
    pkt.m_active = true;
    pkt.m_color = TINT_ORANGE;
    pkt.m_humanControlled = true;
    pkt.m_difficulty = BZDIFF_EASY;

    pkt.m_ready = false;
    pkt.m_maxGruntz = TM_UNITS_PER_PLAYER;
    pkt.m_preferredPlayerIndex = NET_PREFERRED_PLAYER_INDEX_ANY;
    pkt.m_networkPlayerId = m_localPlayerId;
    {
        if (!copyTextToBuffer((PlayerName()), pkt.m_name, sizeof(pkt.m_name))) return 0;
    }
    BroadcastPacket(&pkt, sizeof(pkt), DPSEND_GUARANTEED);
    return 1;
}

i32 CMulti::CreateHostPlayer(
    void* hostToken,
    const std::string& name,
    ColorTint color,
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
    m_localPlayer = Network()->CreatePlayer(
        PlayerName().c_str(),
        "",
        NULL
    );
    if (m_localPlayer == NULL) {
        ReportNetError(0);
        return 0;
    }
    m_localPlayerId = m_localPlayer->m_playerId;
    return RegisterLocalPlayer(name, color, -1, m_localPlayerId) != 0;
}

i32 CMulti::WaitForConnect() {
    if (Network() == NULL) {
        return 0;
    }
    if (LocalPlayer() == NULL) {
        return 0;
    }

    BroadcastPlayerIdMessage(STAT_REQUEST_CONFIG, DPSEND_GUARANTEED);
    m_connectAccepted = false;

    u32 deadline = timeGetTime() + 60000;

    while (m_connectAccepted == false) {

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

i32 CMulti::AutoTuneCmdDelay() {
    if (m_customLevelVerificationPending != false) {
        return 1;
    }

    u32 ping = static_cast<u32>(GetMaxAckLatency());
    u32 tuned = ping / 30 + 2;
    i32 base = static_cast<i32>(max(3u, tuned));

    i32 probe = Mgr()->CountActivePlayers(false);

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
    return SendGameConfig(NULL);
}

i32 CMulti::SendGameConfig(CNetPlayerNode* recipient) {
    CNetGameConfigPacket blob;
    memset(&blob, 0, sizeof(blob));
    blob.m_flags |= NET_PACKET_APPLICATION;
    blob.m_messageId = STAT_CONFIG;
    blob.m_usesCustomLevel = m_usesCustomLevel;
    {
        if (!copyTextToBuffer((BuiltInLevelName()), blob.m_builtInLevelName, sizeof(blob.m_builtInLevelName))) return 0;
    }
    {
        if (!copyTextToBuffer((CustomLevelName()), blob.m_customLevelName, sizeof(blob.m_customLevelName))) return 0;
    }
    blob.m_commandDelay = m_commandDelay;
    blob.m_resendInterval = m_resendInterval;
    blob.m_autoCommandDelay = m_autoCommandDelay;
    blob.m_rngSeed = m_rngSeed;

    if (recipient != NULL) {
        SendPacketTo(recipient, &blob, sizeof(blob), DPSEND_GUARANTEED);
    } else {
        BroadcastPacket(&blob, sizeof(blob), DPSEND_GUARANTEED);
    }
    return 1;
}

i32 CMulti::ApplyGameConfig(CNetGameConfigPacket* config) {
    if (config == NULL) {
        return 0;
    }

    m_usesCustomLevel = config->m_usesCustomLevel;
    m_builtInLevelName = config->m_builtInLevelName;
    m_customLevelName = config->m_customLevelName;
    m_commandDelay = config->m_commandDelay;
    m_resendInterval = config->m_resendInterval;
    m_autoCommandDelay = config->m_autoCommandDelay;
    m_rngSeed = config->m_rngSeed;
    return 1;
}

i32 CMulti::ResetPlayerCommands(i32 playerId) {
    if (m_connected == false) {
        return 0;
    }

    CNetCmdSlot* slot = Session()->FindSlotByPlayerId(playerId);
    if (slot == NULL) {
        return 0;
    }
    if (slot->m_isDraining != false) {
        return 0;
    }

    slot->BeginDrain();
    i32 seq = (slot->m_contiguousSequence + 1) * static_cast<i32>(m_commandDelay);
    i32 end = seq + static_cast<i32>(m_commandDelay) * 3;
    for (; seq < end; seq++) {

        NetGameMgr()->m_commandMgr->RemoveScheduledCommand(slot->m_player->m_playerIndex, seq);
        slot->RemoveRecord(seq / static_cast<i32>(m_commandDelay));
    }
    slot->ClearSequenceSet(slot->m_receivedAhead);
    slot->ClearSequenceSet(slot->PeerReceivedAhead());
    return 1;
}

void CMulti::ReportMaxAckLatency() {
    u32 latency = GetMaxAckLatency();
    BroadcastValueMessage(STAT_ACK_LATENCY_REPORT, latency, 0);
}

u32 CMulti::GetMaxAckLatency() {
    u32 max = 0;

    if (m_isHost != false) {
        for (i32 i = 0; i < 4; i++) {
            max = max(m_playerLatencyMs[i], max);
        }
    } else {

        CGruntzMgr* mgr = NetGameMgr();
        for (i32 i = 0; i < 4; i++) {
            if (mgr->m_players[i].m_humanControlled && mgr->m_players[i].m_active) {
                max = max(mgr->m_players[i].m_latency.m_avg, max);
            }
        }
    }
    return max;
}

void CMulti::HandleVersionCheck(CNetVersionPacket* packet) {
    if (packet == NULL) {
        return;
    }

    b32 mismatch = false;
    if (g_localVersion != packet->m_localVersion) {
        mismatch = true;
    }
    if (g_remoteVersion != packet->m_remoteVersion) {
        mismatch = true;
    }

    if (mismatch) {
        b32 wasConnected = m_connected;
        m_versionMismatch = true;
        if (wasConnected) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            HWND hwnd = NetGameMgr()->m_gameWnd->GetHwnd();
            PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
        }
    }
    if (mismatch) {
        BroadcastPlayerIdMessage(STAT_VERSION_MISMATCH, DPSEND_GUARANTEED);
        Sleep(0xfa);
    }
}

void CMulti::SendVersionCheck(CNetPlayerNode* recipient) {
    CNetVersionPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.m_flags |= NET_PACKET_APPLICATION;
    packet.m_cfgWord = g_cfgWord;
    packet.m_remoteVersion = g_remoteVersion;
    packet.m_butePos = g_buteMgr.GetChecksum();
    packet.m_localVersion = g_localVersion;
    packet.m_messageId = STAT_VERSION_CHECK;

    SendPacketTo(recipient, &packet, sizeof(packet), DPSEND_GUARANTEED);
}

i32 CMulti::OnChar(i32 charCode, i32 keyData) {
    if (m_chatBox && m_chatBox->m_inputActive) {
        if (m_connected) {
            if (Mgr()->ChatLog()->HandleInputChar(charCode, keyData)) {
                std::string line = Mgr()->ChatLog()->GetInputText();
                i32 n = static_cast<i32>((line).size());
                if (n > 9) {
                    std::string text = rightText(line, n - 9);
                    BroadcastChatLine(text, 1, 1, NULL);
                    (Mgr()->ChatLog()->m_inputText).erase();
                }
            }
        }
        return 1;
    }
    return CPlay::OnChar(charCode, keyData);
}

void CMulti::TickStateMgrs() {
    m_mgr->TickStateMgrs();
}
