#include <rva.h>

#include <Gruntz/GruntzMgr.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Crypto/FecCrypt.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/Attract.h>
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
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzApp.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzDebugDialog.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/HeapDiag.h>
#include <Gruntz/HelpState.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/InputState.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LoadGameMenu.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapLogic.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/MovieEntryId.h>
#include <Gruntz/MovieId.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PathBuffer.h>
#include <Gruntz/Play.h>
#include <Gruntz/PortalPath.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/Resolution.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SplashState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TraitorMode.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/WaitCursorScope.h>
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
#include <Rez/RezArchive.h>
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

DATA(0x00211054)
static char s_dataPath[] = "%c:\\DATA\\%s";

DATA(0x00211044)
static char s_fecName[] = "Gruntz.FEC";

DATA(0x00211034)
static char s_fecLoName[] = "GruntzLo.FEC";

DATA(0x00211024)
static char s_moviezPath[] = "%c:\\MOVIEZ\\%s";

DATA(0x002452d8)
char g_msgScratch[256];

DATA(0x002451a4)
i32 g_debugGruntRow;
DATA(0x00245268)
i32 g_debugGruntToy;
DATA(0x0024526c)
i32 g_debugGruntPlayer;
DATA(0x002452a8)
i32 g_debugGruntRadius;
DATA(0x002452d0)
i32 g_debugGruntTool;
DATA(0x002452d4)
i32 g_debugGruntColor;
DATA(0x00245538)
i32 g_debugGruntColumn;
DATA(0x00245558)
i32 g_debugGruntMoveLeft;
DATA(0x0024555c)
i32 g_debugGruntMoveTop;
DATA(0x00245560)
i32 g_debugGruntMoveRight;
DATA(0x00245564)
i32 g_debugGruntMoveBottom;
DATA(0x00245568)
i32 g_debugGruntAiType;

DATA(0x002455e8)
i32 g_monologoShown;

DATA(0x0024556c)
CGruntzMgr* g_gameReg = NULL;

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
DebugDisplayFlags g_debugDisplayFlags;

DATA(0x00245570)
DirectInputMgr2* g_inputMgr = NULL;
DATA(0x00245578)
CInputState* g_gameplayInput = NULL;

DATA(0x0020fa70)
i32 g_localVersion = 1;
DATA(0x0020fa74)
i32 g_remoteVersion = 1;
DATA(0x0020fa78)
i32 g_unreferencedGruntzMgrValues[16] = {1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 8, 9, 10};
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

RVA(0x00083030, 0x1b6)
CGruntzMgr::CGruntzMgr() {
    m_curState = NULL;
    m_world = NULL;
    m_resourceArchive = NULL;
    m_settings = NULL;
    m_gameStats = NULL;
    m_reserved3c = NULL;
    m_faderMgr = NULL;
    m_cheatMgr = NULL;
    m_midi = NULL;
    m_reserved4c = 0;
    m_shadeCache = NULL;
    m_reserved64 = 0;
    m_lobby = NULL;
    m_worldSounds = NULL;
    m_saveGame = NULL;
    m_chatLog = NULL;
    m_voiceManager = NULL;
    m_triggerMgr = NULL;
    m_commandMgr = NULL;
    m_tileGrid = NULL;
    m_spriteFactory = NULL;
    m_lightFxMgr = NULL;
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
    m_isBuiltInBattlezLevel = 0;
    m_isBuiltInMultiplayerLevel = 0;
    m_gameMode = GAMEMODE_NONE;
    m_isHighDetail = 1;
    m_isEffectsEnabled = 1;
    m_computerPlayerCount = 3;
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

RVA_COMPGEN(0x00083330, 0x1e, ??_GCGruntzMgr@@UAEPAXI@Z)

RVA(0x00083360, 0xb2)
CGruntzMgr::~CGruntzMgr() {
    Close();
}

RVA_COMPGEN(0x00085540, 0xb, ??1CGameMgr@@UAE@XZ)

RVA(0x00085560, 0xb)
i32 CGameMgr::IsActive() {
    return m_gameWnd != NULL;
}

RVA(0x00085580, 0x5)
i32 CGameMgr::HandleCommand(i32, GruntzCommandId, i32) {
    return 0;
}

RVA_COMPGEN(0x000855a0, 0x24, ??_GCGameMgr@@UAEPAXI@Z)

RVA(0x000855e0, 0x448)
void CGruntzMgr::Close() {
    if (m_world) {
        m_world->SetRestoreHandler(NULL);
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
        if (m_midi) {
            m_settings->SetValueDword("Music Volume", m_midi->GetMasterVolume());
        }
        if (m_voiceManager) {
            m_settings->SetValueDword("Voice Volume", m_voiceManager->m_voiceVolume);
        }
        if (m_world && m_world->m_soundRegistry) {
            m_settings->SetValueDword("Sound Volume", g_soundVolumePercent);
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
        if (m_colorDepth == BPP_RGB_16) {
            m_settings->SetValueDword("Enable HiColor", 1);
        } else {
            m_settings->SetValueDword("Enable HiColor", 0);
        }
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
    if (m_triggerMgr) {
        delete m_triggerMgr;
        m_triggerMgr = NULL;
    }
    if (m_tileGrid) {

        delete m_tileGrid;
        m_tileGrid = NULL;
    }
    CGameStats* gameStats = m_gameStats;
    if (gameStats) {
        delete gameStats;
        m_gameStats = NULL;
    }
    if (m_commandMgr) {

        delete m_commandMgr;
        m_commandMgr = NULL;
    }
    if (g_gameplayInput) {
        CInputState* v = g_gameplayInput;
        v->m_primaryDevice = NULL;
        v->m_keyboard = NULL;
        v->m_joystick = NULL;
        v->m_deviceGroup = NULL;
        v->m_deviceSelection = INPUTDEV_NONE;
        operator delete(v);
        g_gameplayInput = NULL;
    }
    if (g_inputMgr) {

        delete g_inputMgr;
        g_inputMgr = NULL;
    }
    if (m_cheatMgr) {
        delete m_cheatMgr;
        m_cheatMgr = NULL;
    }
    if (m_midi) {
        delete m_midi;
        m_midi = NULL;
    }
    if (m_worldSounds) {
        delete m_worldSounds;
        m_worldSounds = NULL;
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
    if (m_voiceManager) {
        m_voiceManager->~CVoiceManager();
        operator delete(m_voiceManager);
        m_voiceManager = NULL;
    }
    if (m_world) {
        delete m_world;
        m_world = NULL;
    }
    if (m_resourceArchive) {
        delete m_resourceArchive;
        m_resourceArchive = NULL;
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
    if (m_saveGame) {

        delete m_saveGame;
        m_saveGame = NULL;
    }
    if (m_lightFxMgr) {
        m_lightFxMgr->Reset();
        operator delete(m_lightFxMgr);
        m_lightFxMgr = NULL;
    }
    CloseSoundFontDevice();
    if (m_lobby) {
        m_lobby->Release();
        m_lobby = NULL;
    }
    if (m_connSettings) {
        RecordBytes<DPLCONNECTION> settings;
        settings.m_rec = m_connSettings;
        delete[] settings.m_bytes;
        m_connSettings = NULL;
    }
    this->CGameMgr::Close();
    g_gameReg = NULL;
}

RVA_COMPGEN(0x00085b50, 0x56, ??1CSaveGame@@QAE@XZ)

RVA_COMPGEN(0x00085ed0, 0x4a, ??1CWorldSoundSet@@QAE@XZ)

RVA_COMPGEN(0x00085fc0, 0x57, ??1DirectInputMgr2@@QAE@XZ)

RVA_COMPGEN(0x00086040, 0x49, ??1MidiManager@@QAE@XZ)

RVA(0x000860b0, 0xe8)
void CGruntzMgr::CommitSinglePlayerProgress() {
    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        return;
    }
    CState* currentState = g_gameReg->m_curState;

    m_gameStats->m_gruntzExited += m_triggerMgr->m_gruntzExitedByPlayer[g_curPlayer];
    m_gameStats->m_gruntzLost += m_triggerMgr->m_gruntzLostByPlayer[g_curPlayer];

    if (m_strWorldFile.GetLength() != 0) {
        m_gameStats->SetLevelNumber(1);
        m_gameStats->m_isCustomLevel = 1;
        return;
    }

    if (m_cheatMgr->m_cheatsUsed == 0) {
        m_gameStats->UpdateLevelRecord(currentState->m_levelIndex, 0);
        g_gameReg->m_saveGame->SetCurLevel(static_cast<QuestLevel>(currentState->m_levelIndex));
        g_gameReg->m_saveGame->SetMaxLevel(
            static_cast<QuestLevel>(
                (currentState->m_levelIndex % IDX(QUESTLEVEL_TRAINING_LAST)) + 1
            )
        );
        g_gameReg->m_saveGame->Save(NULL, 0x81a6);
    }
    m_gameStats->SetLevelNumber(currentState->m_levelIndex);
    m_gameStats->m_isCustomLevel = 0;
}

RVA(0x000861e0, 0xc5)
void CGruntzMgr::FinalizeLevelAndShowResults() {
    CState* currentState = m_curState;
    if (m_gameMode == GAMEMODE_QUESTZ) {
        if (m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
            CommitSinglePlayerProgress();
        }
        TransitionState(GAMESTATE_BOOTY, 1, 0, 0);
        return;
    }
    g_gameReg->m_gameStats->SetLevelNumber(currentState->m_levelIndex);
    if (m_gameMode == GAMEMODE_BATTLEZ) {

        CTimer* levelTimer = (static_cast<CPlay*>(currentState))->m_levelTimer;
        i64 elapsedMs = static_cast<i64>(g_frameTime) - levelTimer->m_startStamp.m_v;
        g_gameReg->m_gameStats->m_elapsedTimeMs +=
            (elapsedMs < 0) ? 0 : static_cast<i32>(elapsedMs);
        TransitionState(GAMESTATE_MULTIBOOTY, 1, 0, 0);
        return;
    }
    CGameStats* gameStats = g_gameReg->m_gameStats;
    u32 now = timeGetTime();
    gameStats->m_elapsedTimeMs += (now - g_roundStartTimeMs);
    TransitionState(GAMESTATE_MULTIBOOTY, 1, 0, 0);
}

RVA(0x0008b8c0, 0x76)
i32 PumpIdleFrame() {
    if (g_pendingFrame == 0) {
        return 0;
    }
    g_pendingFrame = 0;
    if (g_gameReg == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = g_gameReg->m_world;
    if (world == NULL) {
        return 0;
    }
    if (world->m_imageRegistry == NULL) {
        return 0;
    }
    if (g_gameReg->m_curState == NULL) {
        return 0;
    }
    if (g_gameReg->m_curState->InputVirtual() == 0) {
        g_gameReg->ReportError(IDX(IDS_RESTORE_GAME), 0x435);
        return 0;
    }
    g_gameReg->RefreshGameClock();
    g_pendingFrame = 1;
    return 1;
}

// @early-stop
RVA(0x0008b960, 0x808)
i32 CGruntzMgr::TransitionState(GameStateId stateId, i32 areaArg, i32 keepCurrent, i32 unused) {
    static_cast<void>(unused);
    TRACE("TransitionState %d\n", stateId);
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

    TRACE("creating state %d\n", stateId);
    switch (stateId) {
        case GAMESTATE_ATTRACT:
            m_curState = new CAttract;
            break;
        case GAMESTATE_PLAY:
            m_curState = new CPlay;
            break;
        case GAMESTATE_MULTI:
            m_curState = new CMulti;
            break;
        case GAMESTATE_DEMO:
            m_curState = new CDemo;
            break;
        case GAMESTATE_MENU:
            m_curState = new CMenuState;
            break;
        case GAMESTATE_HELP:
            m_curState = new CHelpState;
            break;
        case GAMESTATE_SPLASH:
            m_curState = new CSplashState;
            break;
        case GAMESTATE_BOOTY:
            m_curState = new CBootyState;
            break;
        case GAMESTATE_CREDITS:
            m_curState = new CCreditsState;
            break;
        case GAMESTATE_MULTIBOOTY:
            m_curState = new CMultiBootyState;
            break;
    }

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
        TRACE("TransitionState %d done\n", stateId);
        return 1;
    }
}

RVA_COMPGEN(0x0008c3d0, 0x1e, ??_GCRgn@@UAEPAXI@Z)

RVA_COMPGEN(0x0008c470, 0xb, ??1CState@@UAE@XZ)

RVA(0x0008c530, 0x8)
i32 CState::LeaveState(GameStateId nextState) {
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
    return m_session->m_commandTick;
}

RVA_COMPGEN(0x0008d240, 0x1e, ??_GCMulti@@UAEPAXI@Z)

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
    if (m_curState->EnterState(oldId) == GAMESTATE_NONE && m_curState->RestoreDisplay() == 0) {
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
        CState* st = m_curState;
        m_curState->LeaveState(st->Update());
        if (static_cast<CPlay*>(st)->LoadByMode(areaArg, unused) == 0) {
            return 0;
        }
        m_curState->EnterState(m_curState->Update());
        return 1;
    }
    return TransitionState(GAMESTATE_PLAY, areaArg, 0, 0);
}

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

RVA(0x0008d9d0, 0x1e)
i32 CGruntzMgr::ForwardCharToState(i32 charCode, i32 keyData) {
    if (m_curState) {
        return m_curState->OnChar(charCode, keyData);
    }
    return 0;
}

RVA(0x0008da00, 0x1e)
i32 CGruntzMgr::ForwardKeyDownToState(i32 virtualKey, i32 keyData) {
    if (m_curState) {
        return m_curState->OnKeyDown(virtualKey, keyData);
    }
    return 0;
}

RVA(0x0008da30, 0x1e)
i32 CGruntzMgr::ForwardKeyUpToState(i32 virtualKey, i32 keyData) {
    if (m_curState) {
        return m_curState->OnKeyUp(virtualKey, keyData);
    }
    return 0;
}

RVA(0x0008da60, 0x23)
i32 CGruntzMgr::ForwardLButtonDownToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnLButtonDown(keyFlags, x, y);
    }
    return 0;
}

RVA(0x0008daa0, 0x23)
i32 CGruntzMgr::ForwardLButtonUpToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnLButtonUp(keyFlags, x, y);
    }
    return 0;
}

RVA(0x0008dae0, 0x23)
i32 CGruntzMgr::ForwardLButtonDblClkToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnLButtonDblClk(keyFlags, x, y);
    }
    return 0;
}

RVA(0x0008db20, 0x23)
i32 CGruntzMgr::ForwardRButtonDownToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnRButtonDown(keyFlags, x, y);
    }
    return 0;
}

RVA(0x0008db60, 0x23)
i32 CGruntzMgr::ForwardRButtonUpToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnRButtonUp(keyFlags, x, y);
    }
    return 0;
}

RVA(0x0008dba0, 0x23)
i32 CGruntzMgr::ForwardRButtonDblClkToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnRButtonDblClk(keyFlags, x, y);
    }
    return 0;
}

RVA(0x0008dbe0, 0x23)
i32 CGruntzMgr::ForwardMouseMoveToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState) {
        return m_curState->OnMouseMove(keyFlags, x, y);
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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

RVA(0x0008dc60, 0x19)
void CGruntzMgr::ReportError(WPARAM wParam, LPARAM lParam) {
    CGameApp* pApp = m_owner;
    if (pApp) {
        pApp->ReportError(wParam, lParam);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008dc90, 0xb1)
void CGruntzMgr::RegisterLevelAssetKeys() {
    CDDrawSurfaceMgr* w = m_world;
    if (w == NULL) {
        return;
    }

    SoundCueRegistry* snd = w->m_soundRegistry;
    w->m_imageRegistry->SumSizesEqual(NULL, 1);
    snd->SumAudioBytes(NULL);
    w->m_deviceManager->GetCapsChecked();
    w->m_deviceManager->GetCapsChecked();
    w->m_imageRegistry->SumSizesEqual(NULL, 1);
    w->m_imageRegistry->SumSizesEqual("GRUNTZ", 1);
    w->m_imageRegistry->SumSizesEqual("GAME", 1);
    w->m_imageRegistry->SumSizesEqual("LEVEL", 1);
    w->m_imageRegistry->SumSizesEqual("ACTION", 1);
    w->m_soundRegistry->SumAudioBytes(NULL);
    w->m_soundRegistry->SumAudioBytes("GRUNTZ");
    w->m_soundRegistry->SumAudioBytes("GAME");
    w->m_soundRegistry->SumAudioBytes("LEVEL");
}

RVA(0x0008dd80, 0x31)
i32 CDDrawDeviceManager::GetCapsChecked() {
    i32 hr = m_device->GetCaps(&m_driverCaps, &m_helCaps);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(
            const_cast<char*>("c:\\proj\\incs\\ddrawmgr.h"),
            0x135,
            hr
        );
    }
    return hr;
}

#define IS_STANDARD_VIDEO_MODE (m_modeSize.cx == SCREEN_W_PX && m_modeSize.cy == SCREEN_H_PX)

RVA(0x0008ddd0, 0x7e)
i32 CGruntzMgr::RestoreVideoMode(i32 save) {
    if (IS_STANDARD_VIDEO_MODE) {
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

RVA(0x0008de70, 0x61)
i32 CGruntzMgr::CheckSavedMode() {

    if ((m_modeSize.cx == m_savedModeSize.cx && m_modeSize.cy == m_savedModeSize.cy)
        || SetVideoMode(m_savedModeSize.cx, m_savedModeSize.cy, 1) || RestoreVideoMode(1)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x45e);
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
                if (w > f->m_planePixelWidth || h > f->m_planePixelHeight) {
                    CPlay* st = static_cast<CPlay*>(m_curState);
                    st->ResetViewport();
                    if (st->m_statusBar != NULL) {
                        st->m_statusBar->m_barFrameGate = m_modeSize.cy;
                        if (st->m_statusBar->m_position == STATUSBAR_DOCK_RIGHT) {
                            st->m_statusBar->DockStatusBarLeft();
                            st->m_statusBar->DockStatusBarRight();
                            EnterModalUI(
                                "This map is too small to be displayed under your "
                                "desired video resolution. Default resolution will "
                                "be used."
                            );
                            return 0;
                        }
                        if (st->m_statusBar->m_position == STATUSBAR_DOCK_LEFT) {
                            st->m_statusBar->DockStatusBarRight();
                            st->m_statusBar->DockStatusBarLeft();
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
    while (ShowCursor(false) >= 0) {
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
        if (st->m_statusBar != NULL) {
            st->m_statusBar->m_barFrameGate = h;
            if (st->m_statusBar->m_position == STATUSBAR_DOCK_RIGHT) {
                st->m_statusBar->DockStatusBarLeft();
                st->m_statusBar->DockStatusBarRight();
            } else if (st->m_statusBar->m_position == STATUSBAR_DOCK_LEFT) {
                st->m_statusBar->DockStatusBarRight();
                st->m_statusBar->DockStatusBarLeft();
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

RVA(0x0008e1d0, 0xa5)
i32 CGruntzMgr::TryNextResolution() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_MULTI) {
        return 1;
    }
    DisplayResolution resolution;
    resolution =
        m_world->m_deviceManager->FindNextResolution(m_modeSize.cx, m_modeSize.cy, m_colorDepth);
    i32 width = resolution.m_width;
    i32 height = resolution.m_height;
    if (width > 0x514 || width == -1 || height == -1) {
        return 1;
    }
    if (SetVideoMode(width, height, 1)) {
        return 1;
    }
    if (SetVideoMode(SCREEN_W_PX, SCREEN_H_PX, 1)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x439);
    return 0;
}

RVA(0x0008e2b0, 0xb1)
i32 CGruntzMgr::TryPreviousResolution() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_MULTI) {
        return 1;
    }
    DisplayResolution resolution;
    resolution = m_world->m_deviceManager
                     ->FindPreviousResolution(m_modeSize.cx, m_modeSize.cy, m_colorDepth);
    i32 width = resolution.m_width;
    i32 height = resolution.m_height;
    if (width == -1 || height == -1 || width < SCREEN_HALF_W_PX || height < 0xc8) {
        return 1;
    }
    if (SetVideoMode(width, height, 1)) {
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
    local = m_world->m_level->m_viewportRect;
    *out = local;
    return out;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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

RVA(0x0008e4e0, 0x172)
BOOL CALLBACK WarpDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szValue[64];

    switch (msg) {
        case WM_INITDIALOG: {

            CDDrawWorkerHost* warp = g_gameReg->m_world->m_level->m_mainPlane;
            i32 seedX = warp->m_scrollPixelX;
            i32 seedY = warp->m_scrollPixelY;
            SetDlgItemInt(hDlg, 0x40e, seedX, false);
            SetDlgItemInt(hDlg, 0x40f, seedY, false);
            return true;
        }

        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return true;
            }
            if (wParam == IDOK) {
                i32 valX = GetDlgItemInt(hDlg, 0x40e, NULL, false);
                i32 valY = GetDlgItemInt(hDlg, 0x40f, NULL, false);
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
                return true;
            }
            break;
    }
    return false;
}

RVA(0x0008e6c0, 0x85)
void CGruntzMgr::OnCheckpointReached() {
    if (m_isCheckpointPrompts == 0) {
        return;
    }
    CCheckpointDlg dlg(NULL);
    if (ExitModalUI(&dlg, 0) == 1) {
        SendMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_QUICK_SAVE_PROMPT), 0);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008e780, 0x2a)
i32 CGruntzMgr::DebugJumpLevel() {
    i32 level = RunModalDialog("DEBUG_JUMPLEVEL", JumpLevelDialogProc, 1);
    if (level > 0) {
        return PassClickToPlayState(level, 0, 1);
    }
    return 0;
}

RVA(0x0008e7c0, 0x86)
BOOL CALLBACK JumpLevelDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, g_gameReg->m_curState->m_levelIndex, false);
            return true;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return true;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, NULL, false));
                return true;
            }
            break;
    }
    return false;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008e880, 0x27)
i32 CGruntzMgr::RegisterSetSkillDebugCmd() {
    if (m_curState->Update() == GAMESTATE_PLAY) {
        RunModalDialog("DEBUG_SETSKILL", SetSkillLevelDialogProc, 1);
    }
    return 0;
}

RVA(0x0008e8c0, 0x86)
BOOL CALLBACK SetSkillLevelDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, g_gameReg->m_curState->m_levelIndex, false);
            return true;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return true;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, NULL, false));
                return true;
            }
            break;
    }
    return false;
}

RVA(0x0008e980, 0x11e)
i32 CGruntzMgr::FinishLevel(i32 pauseGame, i32 pauseMusic) {
    if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {

        i32 activePlayers = 0;
        CNetCmdSlot* slot = static_cast<CMulti*>(m_curState)->m_session->m_slots;
        for (i32 remainingSlots = 4; remainingSlots != 0; remainingSlots--) {
            if (slot != NULL && slot->m_state == NETSLOT_ACTIVE) {
                activePlayers++;
            }
            slot++;
        }
        if (activePlayers > 0) {
            m_frameGate = 1;

            (static_cast<CMulti*>(m_curState))->RequestMultiplayerPause();
            m_frameGate = 0;
            return 1;
        }
    }

    if (pauseGame) {
        if (m_worldSounds) {
            m_worldSounds->Stop();
        }
        if (m_world) {
            SoundCueRegistry* sub = m_world->m_soundRegistry;
            if (sub && sub->m_soundStream) {
                sub->m_soundStream->StopAllStreams();
            }
        }
        MidiManager* midi = m_midi;
        if ((midi->m_currentSequence ? midi->m_currentSequence->IsPlaying() : 0) && pauseMusic) {
            m_midi->PauseCurrent();
        }
        m_curState->PauseGame();
    }
    if (pauseGame) {
        return 1;
    }

    if (m_musicEnabled) {
        if (CheckPlayState()) {
            m_midi->ResumeCurrent(1);
        }
    }
    if (m_soundEnabled) {
        m_worldSounds->Resume();
        if (m_triggerMgr && m_soundEnabled) {
            m_triggerMgr->DestroyAllAnims();
        }
    }
    m_curState->ResumeGame();
    g_inputMgr->ReadAll();
    RefreshGameClock();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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

    i32 hr = DirectPlayLobbyCreate(NULL, &m_lobby, NULL, NULL, 0);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x120d, hr, m_gameWnd->m_hwnd);
        return 0;
    }
    if (!m_lobby) {
        return 0;
    }

    if (m_connSettings) {

        RecordBytes<DPLCONNECTION> settings;
        settings.m_rec = m_connSettings;
        delete[] settings.m_bytes;
        m_connSettings = NULL;
    }

    DWORD dwSize = 0;
    hr = m_lobby->GetConnectionSettings(0, NULL, &dwSize);
    if (hr != 0 && hr != static_cast<i32>(DPERR_BUFFERTOOSMALL)) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr, m_gameWnd->m_hwnd);
        m_lobby->Release();
        m_lobby = NULL;
        return 0;
    }

    RecordBytes<DPLCONNECTION> settings;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008ee70, 0x7c)
i32 CGruntzMgr::ShowMessageBox(const char* text, u32 type) {
    if (m_world) {
        m_world->m_drawTarget->BlitPage(m_world->m_drawTarget->m_backPair);

        CDDrawDeviceManager* deviceManager = m_world->m_deviceManager;
        deviceManager->m_device->FlipToGDISurface();
    }
    i32 wasShown = ShowCursor(true);
    while (ShowCursor(true) < 0) {
    }
    i32 result = MessageBoxA(m_gameWnd->m_hwnd, text, "Gruntz", type);
    if (wasShown <= 0) {
        while (ShowCursor(false) >= 0) {
        }
    }
    return result;
}

RVA(0x0008ef10, 0x9e)
void CGruntzMgr::EnterModalUI(const char* msg) {
    CGameApp* app = m_owner;
    if (app == NULL) {
        return;
    }
    if (m_voiceManager) {
        m_voiceManager->PauseAllVoices();
    }
    if (m_world) {
        m_world->m_drawTarget->BlitPage(m_world->m_drawTarget->m_backPair);

        CDDrawDeviceManager* deviceManager = m_world->m_deviceManager;
        deviceManager->m_device->FlipToGDISurface();
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

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008efe0, 0x54)
i32 CGruntzMgr::ToggleObjectLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = m_world->m_level;
        if (view) {
            i32 idx = view->m_planes.GetSize();
            if (idx == LEVEL_EXTENDED_PLANE_COUNT) {
                idx--;
            }
            idx--;
            i32 count = view->m_planes.GetSize();
            CDDrawWorkerHost* layer = (idx < 0 || idx >= count)
                                          ? NULL
                                          : static_cast<CDDrawWorkerHost*>(view->m_planes[idx]);
            if (layer && !(layer->m_flags & IDX(WWD_PLANE_FLAG_MAIN))) {
                layer->m_flags ^= IDX(WWD_PLANE_FLAG_NO_DRAW);
                return 1;
            }
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008f060, 0x35)
i32 CGruntzMgr::ToggleHeightLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = m_world->m_level;
        if (view) {
            CDDrawWorkerHost* layer = view->m_mainPlane;
            if (layer) {
                layer->m_flags ^= IDX(WWD_PLANE_FLAG_NO_DRAW);
                return 1;
            }
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008f0b0, 0x46)
i32 CGruntzMgr::ToggleBaseLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = m_world->m_level;
        if (view) {
            CDDrawWorkerHost* layer = (view->m_planes.GetSize() > 0)
                                          ? static_cast<CDDrawWorkerHost*>(view->m_planes[0])
                                          : NULL;
            if (layer && !(layer->m_flags & IDX(WWD_PLANE_FLAG_MAIN))) {
                layer->m_flags ^= IDX(WWD_PLANE_FLAG_NO_DRAW);
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x0008f120, 0x170)
i32 CGruntzMgr::LaunchWebBrowser(char* url) {
    LONG len = 0x104;
    char cmd[0x104];
    if (RegQueryValueA(HKEY_CLASSES_ROOT, "http\\shell\\open\\command", cmd, &len)) {
        return 0;
    }
    if (strlen(cmd) < 3) {
        return 0;
    }
    HANDLE quoted = NULL;

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
    return CreateProcessA(NULL, cmdline, NULL, NULL, false, 0, NULL, NULL, &si, &pi);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008f2f0, 0x1b)
i32 CGruntzMgr::PollUnlessIdle() {
    if (m_curState->Update() != GAMESTATE_MENU) {
        CheckPlayState();
    }
    return 0;
}

// @identity-TODO: owner, ABI, and false result are proven; the command identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008f320, 0x3)
i32 CGruntzMgr::RejectWorldFileCommand() {
    return 0;
}

RVA(0x0008f340, 0xf6)
i32 CGruntzMgr::CaptureWorldFile() {
    GameStateId st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY
        && st != GAMESTATE_DEMO) {
        return 0;
    }
    CString name = RunCustomWorldDialog(m_gameWnd->m_hwnd, NULL);
    if (name.GetLength() == 0) {
        return 0;
    }
    m_strWorldFile = name;
    m_isBuiltInMultiplayerLevel = 0;
    m_isBuiltInBattlezLevel = 0;
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEW_GAME), 0);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
    g_debugDisplayFlags = DEBUG_DISPLAY_NONE;
}

static inline SoundCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    SoundCue* found = NULL;
    MapLookup(cues, name, found);
    return found;
}

// @early-stop
RVA(0x0008f530, 0xbd)
void CGruntzMgr::DelayedQuit() {
    if (m_delayedQuitPending != 0) {
        return;
    }
    m_delayedQuitPending = 1;
    SoundCue* out = LookupCue(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE");
    i32 base;
    if (out != NULL) {
        out = LookupCue(m_world->m_soundRegistry->m_cues, "MENU_ACTIVATE");
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

RVA(0x0008f620, 0x51)
void CGruntzMgr::RefreshGameClock() {
    if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {
        return;
    }

    ResetFrameTiming();

    if (m_world) {
        g_soundCueTimeMs = timeGetTime();
        g_engineFrameDelta = 0;
    }

    g_lastNow = g_gameAppNowMs;
    g_frameDelta = g_gameAppFrameDeltaMs;
}

RVA(0x0008f6a0, 0x7d)
void CGruntzMgr::HandleAppActivation(i32 active, i32 unused) {
    if (IsActive() == 0) {
        return;
    }

    if (active) {
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
        m_midi->ResumeCurrent(1);
        return;
    }

    if (m_musicEnabled == 0) {
        return;
    }
    if ((m_midi->m_currentSequence ? m_midi->m_currentSequence->IsPlaying() : 0) == false) {
        return;
    }
    m_midi->PauseCurrent();
}

RVA(0x0008f740, 0x46)
void CGruntzMgr::StopAudioPlayback() {
    if (m_world) {
        SoundCueRegistry* soundRegistry = m_world->m_soundRegistry;
        if (soundRegistry) {
            SoundStream* soundStream = soundRegistry->m_soundStream;
            if (soundStream) {
                soundStream->StopAllStreams();
            }
        }
    }
    MidiManager* midi = m_midi;
    if (midi && (midi->m_currentSequence ? midi->m_currentSequence->IsPlaying() : 0)) {
        m_midi->EndCurrent();
    }
}

RVA(0x0008f7b0, 0x2b)
void CGruntzMgr::SetGameClock(i32 now, i32 delta, i32 abs) {
    g_lastNow = now;
    g_frameDelta = delta;
    g_frameTime = abs;
    g_soundCueTimeMs = now;
    g_engineFrameDelta = delta;
}

// @early-stop
RVA(0x0008f7f0, 0x131)
void CGruntzMgr::RecomputeViewScale() {
    if (m_world == NULL) {
        return;
    }
    CGameLevel* view = m_world->m_level;
    LevelCoordRect ext = view->m_viewportRect;
    i32 iw = ext.right - ext.left + 1;
    i32 ih = ext.bottom - ext.top + 1;
    float fw = static_cast<float>(iw);
    float fh = static_cast<float>(ih);

    view->m_defaultActiveRegionSize.w = static_cast<i32>((fw * 1.4f));
    view->m_defaultActiveRegionSize.h = static_cast<i32>((fh * 1.4f));
    view->MainPlaneNotify();

    view = m_world->m_level;
    view->m_largeActiveRegionSize.w = static_cast<i32>((fw * 5.3f));
    view->m_largeActiveRegionSize.h = static_cast<i32>((fh * 5.3f));
    view->MainPlaneNotify();

    view = m_world->m_level;
    view->m_smallActiveRegionSize.w = static_cast<i32>((fw * 1.12f));
    view->m_smallActiveRegionSize.h = static_cast<i32>((fh * 1.12f));
    view->MainPlaneNotify();

    CGameLevel* v = m_world->m_level;
    if (v->m_mainPlane == NULL) {
        return;
    }
    m_viewBounds.left = (v->m_mainPlane)->m_planeViewRect.left - 0x60;
    m_viewBounds.top = (m_world->m_level->m_mainPlane)->m_planeViewRect.top - 0x60;
    m_viewBounds.right = (m_world->m_level->m_mainPlane)->m_planeViewRect.right + 0x60;
    m_viewBounds.bottom = (m_world->m_level->m_mainPlane)->m_planeViewRect.bottom + 0x60;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0008f980, 0x21)
i32 CGruntzMgr::IsStandardMode() {
    if (IS_STANDARD_VIDEO_MODE) {
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
    return log->AddItem(msg, FONT_ITEM_FLAGS_NONE, 0x11);
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

RVA(0x0008fab0, 0x318)
i32 CGruntzMgr::PlayMovieEntry(i32 entryId) {
    if (entryId < IDX(MOVIE_ENTRY_FIRST) || entryId > IDX(MOVIE_ENTRY_LAST)) {
        return 0;
    }
    if (!FileExists(const_cast<char*>(static_cast<const char*>(m_strMoviePath)))) {
        return 0;
    }

    CMoviePlayer player;
    IDirectSound* dsound = NULL;

    CDDSurface* front = m_world->m_drawTarget->m_frontSurface->m_surface;
    IDirectDraw2* dd2 = m_world->m_deviceManager->m_device;

    if (m_world->m_soundRegistry->HasWithPrefix("GAME") == 0) {
        CRezArchiveDir* snd = m_resourceArchive->FindDirectoryByPath("GAME_SOUNDZ");
        if (snd == NULL) {
            return 0;
        }
        m_world->m_soundRegistry->LoadFromTree(static_cast<CRezArchiveDir*>(snd), "GAME", "_");
    }
    if (front == NULL || dd2 == NULL) {
        return 0;
    }

    if (m_world->m_soundStream != NULL) {
        dsound = m_world->m_soundStream->m_device;
    }
    if (player.InitMode(m_gameWnd->m_hwnd, dd2, front->m_ddSurface, front->m_apiDesc, dsound)) {
        MovieOpenFlags openFlags = m_isInterlaced != 0 ? MOVIE_OPEN_INTERLACED : MOVIE_OPEN_DEFAULT;
        if (player.Open(m_strMoviePath, IDX(entryId), MOVIE_TILE, openFlags, NULL, NULL)) {
            m_modalBusy = 1;
            player.Pump(MOVIE_PUMP_SKIP_ON_KEY, 1);
            m_modalBusy = 0;
        }
    }
    player.Teardown();
    return 1;
}

RVA(0x0008fea0, 0x6d)
CFecFile::CFecFile() {
    m_openGate = 0;
    m_readOpen = 0;
    m_writeOpen = 0;
    m_nextIndex = 0;
    srand(time(NULL));
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
    char szDir[GRUNTZ_PATH_BUFFER_SIZE];

    if (GetCurrentDirectoryA(GRUNTZ_PATH_BUFFER_MAX_CHARS, szDir)) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000901d0, 0x16)
i32 CGruntzMgr::IsMoviePathValid() {
    return FileExists(const_cast<char*>(static_cast<const char*>(m_strMoviePath))) != 0;
}

RVA(0x00090200, 0x8)
i32 CGruntzMgr::PlayLogoMovie() {
    return PlayMovieEntry(IDX(MOVIE_ENTRY_LOGO));
}

RVA(0x00090220, 0x2f)
void CGruntzMgr::Post(i32 code) {
    if (code > 0 && code <= IDX(QUESTLEVEL_POST_LAST)) {
        i32 v = (code == IDX(QUESTLEVEL_RESTART)) ? IDX(QUESTLEVEL_FIRST) : code;
        PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_WORLD), v);
    }
}

RVA(0x00090260, 0x13e)
i32 CGruntzMgr::RunModalDialog(const char* tmpl, DLGPROC dlgProc, i32 flag) {
    if (tmpl == NULL) {
        return 0;
    }
    if (dlgProc == NULL) {
        return 0;
    }
    if (m_voiceManager) {
        m_voiceManager->PauseAllVoices();
    }
    if (m_triggerMgr && m_soundEnabled) {
        m_triggerMgr->DestroyAllAnims();
    }
    if (m_world) {
        if (flag && m_curState && m_curState->Update() != GAMESTATE_MENU) {
            m_curState->Present(0x32);
        } else {
            flag = 0;
        }

        CDDrawDeviceManager* deviceManager = m_world->m_deviceManager;
        deviceManager->m_device->FlipToGDISurface();
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
        if (o->m_statusBar) {
            (static_cast<CStatusBarMgr*>(o->m_statusBar))->Deactivate();
        }
        o->PostHudRect();
    }
    return result;
}

RVA(0x000903f0, 0x10c)
i32 CGruntzMgr::ExitModalUI(CDialog* dlg, i32 notify) {
    if (m_voiceManager) {
        m_voiceManager->PauseAllVoices();
    }
    if (m_triggerMgr && m_soundEnabled) {
        m_triggerMgr->DestroyAllAnims();
    }
    if (m_world) {
        if (notify && m_curState && m_curState->Update() != GAMESTATE_MENU) {
            m_curState->Present(0x32);
        } else {
            notify = 0;
        }

        CDDrawDeviceManager* deviceManager = m_world->m_deviceManager;
        deviceManager->m_device->FlipToGDISurface();
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
        if (o->m_statusBar) {
            (static_cast<CStatusBarMgr*>(o->m_statusBar))->Deactivate();
        }
        o->PostHudRect();
    }
    return result;
}

RVA(0x00090550, 0x1e6)
i32 __stdcall LaunchPortalExe(char* outPath) {
    DWORD bufSize;
    char regBuf[0x100];
    Utils::RegistryHelper reg;

    if (!reg.Open("Monolith Productions", "Portal", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL)) {
        return 0;
    }
    regBuf[0] = 0;
    bufSize = 0xde;
    if (!reg.GetValueString("filedir", regBuf, &bufSize, NULL)) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
    if (!LaunchProcessInDir(path, NULL)) {
        return 0;
    }
    if (quitAfter) {
        DelayedQuit();
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
    return CreateProcessA(NULL, cmdline, NULL, NULL, false, 0, NULL, dir, &si, &pi);
}

RVA(0x00090980, 0x18)
CState* CGruntzMgr::TopState() {
    CPtrArray* st = &m_stateStack;
    if (st->GetSize() <= 0) {
        return NULL;
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
        CObject* out = NULL;
        m_world->m_imageRegistry->m_workersByName.Lookup("GAME_MONOLITH", out);
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
    i32 monolithWidth = e->m_width;
    i32 monolithHeight = e->m_height;
    CDDrawWorkerHost* found =
        static_cast<CDDrawWorkerHost*>(m_world->m_level->FindPlaneByName("MONOLITH"));
    if (found == NULL) {
        CDDrawWorkerHost* spr = m_world->m_level->ReadObjectPlane(
            0x20,
            0x20,
            monolithWidth,
            monolithHeight,
            -0x19,
            -0x19,
            const_cast<char*>("MONOLITH")
        );
        if (spr == NULL) {
            return 0;
        }
        spr->m_imageSets.SetAtGrow(0, static_cast<CObject*>(rec));
        spr->m_flags |= IDX(WWD_PLANE_FLAG_WRAP_X | WWD_PLANE_FLAG_WRAP_Y);
        spr->m_zCoord = 0xf4241;
        i32 parity = 1;
        for (i32 i = 0; i < spr->m_tileRows; i++) {
            for (i32 j = 0; j < spr->m_tileColumns; j++) {
                i32 val = parity ? savedIdx : -1;
                parity ^= 1;
                SET_WORKER_HOST_CELL(spr, j, i, val);
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
    CObject* found = NULL;
    m_world->m_imageRegistry->m_workersByName.Lookup("GAME_DEVHEADS", found);
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

RVA(0x000910d0, 0x75)
i32 CGruntzMgr::SetGruntColor(CDDrawWorker* sink, const char* key, i32 idx) {
    if (sink && key) {
        CObject* out = NULL;
        m_world->m_imageRegistry->m_workersByName.Lookup(key, out);
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00091250, 0x100)
void CGruntzMgr::CheatSkeletonToggle() {
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY && m_world) {

        CDDrawWorker* set;
        {
            CObject* found = NULL;
            m_world->m_imageRegistry->m_workersByName.Lookup("Gruntz", found);
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
                    SoundCueRegistry* registry = m_world->m_soundRegistry;
                    if (registry->m_silentMode == 0) {

                        SoundCue* found = NULL;
                        MapLookup(registry->m_cues, "GAME_MINORCHEAT", found);
                        SoundCue* cue = found;
                        if (cue) {
                            i32 volumePercent = g_soundVolumePercent;
                            if (g_soundEnabled) {
                                if (static_cast<u32>((g_soundCueTimeMs - cue->m_lastPlayTimeMs))
                                    >= static_cast<u32>(cue->m_replayDelayMs)) {
                                    cue->m_lastPlayTimeMs = g_soundCueTimeMs;
                                    cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00091390, 0x11d)
void CGruntzMgr::CheatEclipseToggle() {
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY && m_world) {

        CDDrawWorker* set;
        {
            CObject* found = NULL;
            m_world->m_imageRegistry->m_workersByName.Lookup("Gruntz", found);
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
                    SoundCueRegistry* registry = m_world->m_soundRegistry;
                    if (registry->m_silentMode == 0) {

                        SoundCue* found = NULL;
                        MapLookup(registry->m_cues, "GAME_MINORCHEAT", found);
                        SoundCue* cue = found;
                        if (cue) {
                            i32 volumePercent = g_soundVolumePercent;
                            if (g_soundEnabled) {
                                if (static_cast<u32>((g_soundCueTimeMs - cue->m_lastPlayTimeMs))
                                    >= static_cast<u32>(cue->m_replayDelayMs)) {
                                    cue->m_lastPlayTimeMs = g_soundCueTimeMs;
                                    cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
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

// @identity-TODO: placement by the music controls is the only evidence for the hook's name.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00091570, 0x1)
void CGruntzMgr::OnMusicMuteBegin() {}

// @identity-TODO: placement by the music controls is the only evidence for the hook's name.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00091590, 0x1)
void CGruntzMgr::OnMusicMuteEnd() {}

// @identity-TODO: placement by the music controls is the only evidence for the hook's name.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000915b0, 0x3)
void CGruntzMgr::OnMusicFadeStep(i32 value) {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000915d0, 0x3f)
void CGruntzMgr::MuteMusicIfActive(i32 durationMs) {
    if (m_midi == NULL) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 isPlaying;
    if (m_midi->m_currentSequence != NULL) {
        isPlaying = m_midi->m_currentSequence->IsPlaying();
    } else {
        isPlaying = 0;
    }
    if (isPlaying == 0) {
        return;
    }

    MidiManager* midi = m_midi;
    if (midi->m_currentSequence) {
        midi->m_currentSequence->SetVolumePercent(0, durationMs);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00091620, 0x3f)
void CGruntzMgr::RestoreMusicVolumeIfActive(i32 durationMs) {
    if (m_midi == NULL) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 isPlaying;
    if (m_midi->m_currentSequence != NULL) {
        isPlaying = m_midi->m_currentSequence->IsPlaying();
    } else {
        isPlaying = 0;
    }
    if (isPlaying == 0) {
        return;
    }

    MidiManager* midi = m_midi;
    if (midi->m_currentSequence) {
        midi->m_currentSequence->SetVolumePercent(kSoundVolumeMax, durationMs);
    }
}

RVA(0x00091670, 0x2ac)
i32 CGruntzMgr::MakeRezPath() {
    char cwd[GRUNTZ_PATH_BUFFER_SIZE];
    if (!GetCurrentDirectoryA(GRUNTZ_PATH_BUFFER_MAX_CHARS, cwd)) {
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

RVA(0x000919d0, 0x30)
void CGruntzMgr::SetSoundVolume(i32 v) {
    m_soundVolume = v;
    if (m_world && m_world->m_soundRegistry) {
        g_soundVolumePercent = v;
    }
    CWorldSoundSet* in = m_worldSounds;
    if (in) {
        in->SetMasterVolume(v);
    }
}

RVA(0x00091a10, 0x17)
i32 CGruntzMgr::SetVoiceVolume(i32 v) {
    m_voiceVolume = v;
    CVoiceManager* timer = m_voiceManager;
    if (timer) {
        timer->m_voiceVolume = v;
    }
    return v;
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

    if (m_worldSounds != NULL) {
        delete m_worldSounds;
        m_worldSounds = NULL;
    }

    CRezArchive* surf = m_resourceArchive;
    if (surf) {
        delete surf;
    }
    m_resourceArchive = NULL;

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
    RegisterGameObjectLogicTypes(m_world);
    if (MakeRezPath() == 0) {
        return 0;
    }

    CRezArchive* old = m_resourceArchive;
    if (old) {
        delete old;
        m_resourceArchive = NULL;
    }

    m_resourceArchive = new CRezArchive;

    bool parseFailed =
        m_resourceArchive->Open(const_cast<char*>(static_cast<const char*>(GetRezPath())), 1, 0)
        == 0;
    if (parseFailed) {
        ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x441);
        return 0;
    }

    SetColorDepth(m_colorDepth);

    if (m_worldSounds != NULL) {
        delete m_worldSounds;
        m_worldSounds = NULL;
    }

    CWorldSoundSet* ni = new CWorldSoundSet();
    m_worldSounds = ni;
    if (ni->Init(m_world->m_soundRegistry, m_soundVolume) == 0) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x442);
        return 0;
    }

    CWorldSoundSet* cur = m_worldSounds;
    if (m_isAmbientEnabled != 0) {
        if (cur->m_enabled == 0) {
            cur->m_enabled = 1;
            cur->Resume();
        }
    } else {
        if (cur->m_enabled != 0) {
            cur->m_enabled = 0;
            cur->Stop();
        }
    }
    SetSoundVolume(m_soundVolume);
    return 1;
}

// @identity-TODO: placement after LoadWorldMode is the only evidence for the hook's name.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00091e00, 0x3)
void CGruntzMgr::OnWorldModeLoaded(ColorDepth mode) {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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

    CWaitCursorScope waitCursor;

    if (m_colorDepth == BPP_PALETTED_8) {
        if (LoadWorldMode(BPP_RGB_16) == BPP_UNSET) {
            ReportError(IDX(IDS_CHANGE_COLOR_DEPTH), 0x443);
            return 0;
        }
    } else {
        if (LoadWorldMode(BPP_PALETTED_8) == BPP_UNSET) {
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
void CGruntzMgr::PauseMusicIfEnabled() {
    if (m_midi && m_musicEnabled) {
        m_midi->PauseCurrent();
    }
}

RVA(0x00092030, 0x18)
void CGruntzMgr::ResumeMusicIfEnabled() {
    if (m_midi && m_musicEnabled) {
        m_midi->ResumeCurrent(0);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00092060, 0x3c)
i32 CGruntzMgr::SetAssetRoot(char* path) {
    if (path == NULL) {
        return 0;
    }
    CAssetRootStorage::s_value = path;
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_SHOW_STATE0), 0);
    return 1;
}

RVA(0x000920b0, 0x1c)
i32 CGruntzMgr::TickStateMgrs() {
    g_inputMgr->PollAll();
    g_gameplayInput->Update();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @identity-TODO: the slot range and posted command ID are proven, but retail
// has no command handler or resource label that identifies the operation.
RVA(0x000920e0, 0x32)
i32 CGruntzMgr::PostSlotCommandB1(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x80b1, slot);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @identity-TODO: the slot range and posted command ID are proven, but retail
// has no command handler or resource label that identifies the operation.
RVA(0x00092130, 0x32)
i32 CGruntzMgr::PostSlotCommandB6(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, 0x80b6, slot);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00092180, 0x98)
i32 CGruntzMgr::ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, ScanCb cb, i32 user) {
    if (cb == NULL) {
        return 0;
    }
    i32 r2 = radius * radius;
    i32 count = 0;
    CDDrawChildGroup* children = m_world->m_childGroup;
    POSITION pos = children->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = children->NextChild(pos);
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00092250, 0xba)
i32 CGruntzMgr::ScanObjectsInRect(i32 offX, i32 offY, RECT* rect, i32 mask, ScanCb cb, i32 user) {
    if (cb == NULL) {
        return 0;
    }
    RECT* r = rect;
    if (r == NULL) {
        return 0;
    }
    RECT box;
    box.left = r->left + offX;
    box.right = r->right + offX;
    box.top = r->top + offY;
    box.bottom = r->bottom + offY;
    i32 count = 0;
    CDDrawChildGroup* children = m_world->m_childGroup;
    POSITION pos = children->m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = children->NextChild(pos);
        if (obj->m_objectType & mask) {
            i32 ox = obj->m_screenX;
            if (ox >= box.left && ox <= box.right) {
                i32 oy = obj->m_screenY;
                if (oy >= box.top && oy <= box.bottom) {
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

RVA(0x00092340, 0x49)
void CGruntzMgr::SetSoundEnabled(i32 enabled) {
    if (enabled == m_soundEnabled) {
        return;
    }
    m_soundEnabled = enabled;
    if (m_world == NULL) {
        return;
    }
    SoundStream* soundStream = m_world->m_soundRegistry->m_soundStream;
    if (soundStream) {
        soundStream->StopAllStreams();
    }

    i32 soundEnabled = m_soundEnabled;
    g_soundEnabled = soundEnabled;
    if (m_soundEnabled) {
        m_worldSounds->Resume();
    } else {
        m_worldSounds->Stop();
    }
}

RVA(0x000923b0, 0x47)
void CGruntzMgr::SetMusicEnabled(i32 enabled) {
    if (enabled == m_musicEnabled) {
        return;
    }
    m_musicEnabled = enabled;
    MidiManager* midi = m_midi;
    if (midi == NULL) {
        return;
    }
    if (enabled != 0) {
        MidiSequence* sequence = midi->m_currentSequence;
        if (sequence == NULL) {
            return;
        }
        if (sequence->m_looping != 0) {
            midi->RestartCurrent(1);
        } else if (midi->m_currentSequence != NULL) {

            midi->ResumeCurrent(1);
        }
        return;
    }
    midi->PauseCurrent();
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

RVA(0x00092500, 0x17)
i32 CGruntzMgr::RunLoadGameDialog() {
    RunModalDialog("GAME_LOAD", GruntzLoadGameDlgProc, 0);
    return 1;
}

RVA(0x00092530, 0x17c)
i32 CGruntzMgr::Quicksave() {
    if (m_saveGame == NULL) {
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
    if (m_voiceManager) {
        m_voiceManager->PauseAllVoices();
    }
    FillSaveInfo(m_saveInfoRec, NULL);

    if (g_gameReg->m_saveGame->Save(m_saveInfoRec->m_serial, 0x81a7) == 0) {
        EnterModalUI("ERROR - Cannot Save Game.");
        return 1;
    }
    m_chatLog->AddItem("Game Quicksaved successfully.", FONT_ITEM_FLAGS_NONE, 0x11);
    return 1;
}

RVA(0x00092710, 0x77)
i32 CGruntzMgr::Quickload() {
    if (m_saveGame == NULL) {
        return 0;
    }
    if (m_voiceManager) {
        m_voiceManager->PauseAllVoices();
    }
    if (m_saveInfoRec && (m_saveInfoRec->m_flags & 1)) {

        if (m_saveGame->VerifySlot(m_saveInfoRec) == 0) {
            return 1;
        }
        PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_SAVED_GAME), 0);
        m_chatLog->AddItem("Game Quickloaded successfully.", FONT_ITEM_FLAGS_NONE, 0x11);
        return 1;
    }
    return RunLoadGameDialog();
}

RVA(0x000927b0, 0xc4)
i32 CGruntzMgr::FillSaveInfo(SaveSlot* dst, const char* snapshot) {
    if (dst == NULL) {
        return 0;
    }
    CPlay* src = PickPlayOrPausedState();
    if (src == NULL) {
        return 0;
    }

    strcpy(dst->m_levelName, GetWorldFileName());
    dst->m_isBattlez = (m_gameMode == GAMEMODE_BATTLEZ);
    dst->m_isCustom = m_isCustomLevel;

    m_saveGame->CopySlot(dst, &src->m_saveSlot);
    m_saveInfoRec = dst;
    if (snapshot) {
        strncpy(static_cast<char*>(dst->m_snapshot), snapshot, 0x20);
    }
    return 1;
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
    return NULL;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000929e0, 0x32)
i32 CGruntzMgr::RunDebugGruntTypeDialog() {
    i32 ran = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        ran = RunModalDialog("DEBUG_GRUNTTYPE", DebugGruntTypeDialogProc, 1);
    }
    return ran != 0;
}

RVA(0x00092a30, 0x52)
BOOL CALLBACK PsycheDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            return true;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return true;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, 1);
                return true;
            }
            break;
    }
    return false;
}

RVA(0x00092ab0, 0x20d)
BOOL CALLBACK DebugGruntTypeDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x4db, g_debugGruntPlayer, false);
            SetDlgItemInt(hDlg, 0x4da, g_debugGruntTool, false);
            SetDlgItemInt(hDlg, 0x4dc, g_debugGruntToy, false);
            SetDlgItemInt(hDlg, 0x4dd, g_debugGruntAiType, false);
            SetDlgItemInt(hDlg, 0x4de, g_debugGruntColumn, false);
            SetDlgItemInt(hDlg, 0x4df, g_debugGruntRow, false);
            SetDlgItemInt(hDlg, 0x4e0, g_debugGruntColor, false);
            SetDlgItemInt(hDlg, 0x4e9, g_debugGruntRadius, false);
            SetDlgItemInt(hDlg, 0x4e3, g_debugGruntMoveLeft, false);
            SetDlgItemInt(hDlg, 0x4e4, g_debugGruntMoveRight, false);
            SetDlgItemInt(hDlg, 0x4e5, g_debugGruntMoveTop, false);
            SetDlgItemInt(hDlg, 0x4e6, g_debugGruntMoveBottom, false);
            return true;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return true;
            }
            if (wParam == IDOK) {
                g_debugGruntPlayer = GetDlgItemInt(hDlg, 0x4db, NULL, false);
                g_debugGruntTool = GetDlgItemInt(hDlg, 0x4da, NULL, false);
                g_debugGruntToy = GetDlgItemInt(hDlg, 0x4dc, NULL, false);
                g_debugGruntAiType = GetDlgItemInt(hDlg, 0x4dd, NULL, false);
                g_debugGruntColumn = GetDlgItemInt(hDlg, 0x4de, NULL, false);
                g_debugGruntRow = GetDlgItemInt(hDlg, 0x4df, NULL, false);
                g_debugGruntColor = GetDlgItemInt(hDlg, 0x4e0, NULL, false);
                g_debugGruntRadius = GetDlgItemInt(hDlg, 0x4e9, NULL, false);
                g_debugGruntMoveLeft = GetDlgItemInt(hDlg, 0x4e3, NULL, false);
                g_debugGruntMoveRight = GetDlgItemInt(hDlg, 0x4e4, NULL, false);
                g_debugGruntMoveTop = GetDlgItemInt(hDlg, 0x4e5, NULL, false);
                g_debugGruntMoveBottom = GetDlgItemInt(hDlg, 0x4e6, NULL, false);
                EndDialog(hDlg, 1);
                return true;
            }
            break;
    }
    return false;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00092d50, 0x3c)
i32 CGruntzMgr::SetInactivePlayerName(i32 slot, i32, i32, i32, i32, const CString& val, i32) {
    if (CheckPlayState()) {
        if (m_players[slot].m_active == 0) {
            m_players[slot].m_name = val;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00092da0, 0x3a)
i32 CGruntzMgr::ResetPlayerSlot(i32 slot) {
    if (static_cast<u32>(slot) >= 4) {
        return 0;
    }
    GruntzPlayer* player = &m_players[slot];
    if (player == NULL) {
        return 0;
    }
    if (player->m_active == 0) {
        return 0;
    }

    return player->Reset();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00092df0, 0x24)
void CGruntzMgr::ResetAllPlayerSlots() {
    GruntzPlayer* player = &m_players[0];
    for (i32 remaining = 4; remaining != 0; remaining--) {
        if (player != NULL) {
            player->Reset();
        }
        player++;
    }
}

RVA(0x00092e30, 0x39)
i32 CGruntzMgr::CountActivePlayers(i32 includeComputerPlayers) {
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* slot = &m_players[i];
        if (slot && slot->m_active != 0
            && (includeComputerPlayers != 0 || slot->m_humanControlled != 0)) {
            count++;
        }
    }
    return count;
}

RVA(0x00092e80, 0x25)
GruntzPlayer* CGruntzMgr::FindPlayerByNetworkId(i32 networkPlayerId) {

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* slot = &m_players[i];
        if (slot && slot->m_networkPlayerId == networkPlayerId) {
            return slot;
        }
    }
    return NULL;
}

RVA(0x00092ec0, 0x24)
void CGruntzMgr::DeactivateAllPlayers() {

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &m_players[i];
        if (player != NULL) {
            player->m_active = 0;
            player->m_clearedRound = 0;
        }
    }
}

RVA(0x00092f00, 0x1ef)
i32 CGruntzMgr::OpenBattlezSetup() {
    CBattlezDlg dlg(this, NULL);
    GameStateId st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY
        && st != GAMESTATE_DEMO) {
        return 0;
    }
    ResetPlayerColorAvailability();
    if (ExitModalUI(&dlg, 1) != 1) {
        return 0;
    }
    if (dlg.m_customNameFlag != 0) {
        m_isBuiltInBattlezLevel = 0;
        m_strWorldFile = "custom\\" + dlg.m_worldName;
    } else {
        m_isBuiltInBattlezLevel = 1;
        m_strWorldFile = dlg.m_worldName;
    }
    if (m_strWorldFile.GetLength() == 0) {
        return 0;
    }
    PostMessageA(m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_START_BATTLEZ_GAME), 0);
    return 1;
}

// @early-stop
RVA(0x00093170, 0x1e3)
i32 CGruntzMgr::InitializeBattlezPlayers() {
    i32 matched = 0;
    CString s;
    if (s.LoadString(0x81ab)) {
        bool eq;
        eq = (strcmp(s, m_strWorldFile) == 0);
        if (eq) {
            matched = 1;
        }
    }
    srand(static_cast<u32>(time(NULL)));
    g_battlezTurnPlayerIndex = 0;

    i32 idx = 0;
    GruntzPlayer* player = &m_players[0];
    for (i32 i = 0; i < m_computerPlayerCount; i++) {
        BattlezDifficulty difficulty;
        if (idx == g_curPlayer) {
            player->m_humanControlled = 1;
            difficulty = player->m_difficulty;
            if (matched) {
                difficulty = BZDIFF_EASY;
            }
            if (!player->m_battlezConfig.LoadConfig(this, idx, difficulty)) {
                return 0;
            }
            player->m_battlezConfig.Clear();
            player++;
            idx++;
            player->m_humanControlled = 0;
            difficulty = player->m_difficulty;
            if (matched) {
                difficulty = BZDIFF_EASY;
            }
            if (!player->m_battlezConfig.LoadConfig(this, idx, difficulty)) {
                return 0;
            }
        } else {
            player->m_humanControlled = 0;
            difficulty = player->m_difficulty;
            if (matched) {
                difficulty = BZDIFF_EASY;
            }
            if (!player->m_battlezConfig.LoadConfig(this, idx, difficulty)) {
                return 0;
            }
        }
        idx++;
        player++;
    }
    return 1;
}

RVA(0x000933e0, 0x5e)
i32 CGruntzMgr::AdvanceComputerPlayerTurns() {
    i32 cursor = (g_battlezTurnPlayerIndex + 1) & 3;
    g_battlezTurnPlayerIndex = cursor;
    for (i32 i = 0; i < m_computerPlayerCount + 1; i++) {
        GruntzPlayer* slot = &m_players[i];
        if (cursor == i && slot->m_humanControlled == 0 && slot->m_active != 0) {
            slot->m_battlezConfig.StepBoard();
            cursor = g_battlezTurnPlayerIndex;
        }
    }
    return 1;
}

RVA(0x00093460, 0x15c)
i32 CGruntzMgr::SerializeGameState(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:

            if (SaveState(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (LoadState(ar) == 0) {
                return 0;
            }
            m_voiceManager->ClearVoiceIndicatorSlots();
            break;
    }

    i32 i;
    GruntzPlayer* player;
    for (i = 0, player = m_players; i < 4; i++) {
        if (player == NULL || player->Serialize(ar, mode, typeId, payload) == 0) {
            return 0;
        }
        player++;
    }

    if (m_triggerMgr->Serialize(ar, mode, typeId, payload) == 0) {
        return 0;
    }
    if (PickPlayOrPausedState()->SerializeDispatch(ar, mode, typeId, payload) == 0) {
        return 0;
    }
    if (m_commandMgr->Serialize(ar, mode, typeId, payload) == 0) {
        return 0;
    }

    if (m_tileGrid->SerializeDispatch(ar, mode, typeId, payload) == 0) {
        return 0;
    }

    if (SerializeScrollState(ar, mode, typeId, payload) == 0) {
        return 0;
    }
    return m_gameStats->Serialize(ar, mode, typeId, payload) != 0;
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
    ar->Write(&m_isBuiltInBattlezLevel, sizeof(m_isBuiltInBattlezLevel));
    ar->Write(&m_isBuiltInMultiplayerLevel, sizeof(m_isBuiltInMultiplayerLevel));
    ar->Write(&m_isCustomLevel, sizeof(m_isCustomLevel));
    ar->Write(&m_gameMode, sizeof(m_gameMode));
    ar->Write(&m_computerPlayerCount, sizeof(m_computerPlayerCount));
    ar->Write(&m_viewBounds.left, sizeof(m_viewBounds));
    ar->Write(&g_lastNow, sizeof(g_lastNow));
    ar->Write(&g_frameDelta, sizeof(g_frameDelta));
    ar->Write(&g_frameTime, sizeof(g_frameTime));
    ar->Write(&g_frameTicks, sizeof(g_frameTicks));
    ar->Write(&g_period50CountdownMs, sizeof(g_period50CountdownMs));
    ar->Write(&g_period100CountdownMs, sizeof(g_period100CountdownMs));
    ar->Write(&g_period200CountdownMs, sizeof(g_period200CountdownMs));
    ar->Write(&g_period400CountdownMs, sizeof(g_period400CountdownMs));
    ar->Write(&g_period500CountdownMs, sizeof(g_period500CountdownMs));
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
    ar->Read(&m_isBuiltInBattlezLevel, sizeof(m_isBuiltInBattlezLevel));
    ar->Read(&m_isBuiltInMultiplayerLevel, sizeof(m_isBuiltInMultiplayerLevel));
    ar->Read(&m_isCustomLevel, sizeof(m_isCustomLevel));
    ar->Read(&m_gameMode, sizeof(m_gameMode));
    ar->Read(&m_computerPlayerCount, sizeof(m_computerPlayerCount));
    ar->Read(&m_viewBounds.left, sizeof(m_viewBounds));
    ar->Read(&g_lastNow, sizeof(g_lastNow));
    ar->Read(&g_frameDelta, sizeof(g_frameDelta));
    ar->Read(&g_frameTime, sizeof(g_frameTime));
    ar->Read(&g_frameTicks, sizeof(g_frameTicks));
    ar->Read(&g_period50CountdownMs, sizeof(g_period50CountdownMs));
    ar->Read(&g_period100CountdownMs, sizeof(g_period100CountdownMs));
    ar->Read(&g_period200CountdownMs, sizeof(g_period200CountdownMs));
    ar->Read(&g_period400CountdownMs, sizeof(g_period400CountdownMs));
    ar->Read(&g_period500CountdownMs, sizeof(g_period500CountdownMs));
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

RVA(0x00093be0, 0x107)
i32 CGruntzMgr::IsBattlezMapFile(CString path) {
    CFile file;
    char hdr[0x5f4];
    if (file.Open(path, 0, NULL)) {
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
