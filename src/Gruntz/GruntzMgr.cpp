#include <StdAfx.h>
#include <Io/File.h>
#include <Utils/Text.h>

#include <Ints.h>

#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzMgrCmd.h>

#include <Bute/ButeMgr.h>
#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>
#include <Crypto/FecCrypt.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <DDrawMgr/WorkerLookup.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Dsndmgr/MidiManager.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Blk6c.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/Demo.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSettings.h>
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
#include <Gruntz/GameText.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzApp.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzDebugDialog.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgrMacros.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/HelpState.h>
#include <Gruntz/InputDeviceGroup.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/InputState.h>
#include <Gruntz/LevelCollisionInline.h>
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
#include <Gruntz/SoundCueInline.h>
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
#include <Gruntz/Utils.h>
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
#include <Lith/BDefs.h>
#include <Net/NetLobby.h>
#include <Net/NetMgr.h>
#include <Pix16.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezMgr.h>
#include <Rez/RezSync.h>
#include <Rez/RezTypeTag.h>
#include <SafeDelete.h>
#include <Utils/MapTyped.h>
#include <Io/Settings.h>
#include <Wap32/GameApp.h>
#include <Wap32/Object.h>
#include <Wap32/ScreenGeometry.h>
#include <Wap32/Wap32.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <dplobby.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strstrea.h>
#include <time.h>

static char s_dataPath[] = "%c:\\DATA\\%s";

static char s_fecName[] = "Gruntz.FEC";

static char s_fecLoName[] = "GruntzLo.FEC";

static char s_moviezPath[] = "%c:\\MOVIEZ\\%s";


i32 g_debugGruntRow;

i32 g_debugGruntToy;

i32 g_debugGruntPlayer;

i32 g_debugGruntRadius;

i32 g_debugGruntTool;

i32 g_debugGruntColor;

i32 g_debugGruntColumn;

i32 g_debugGruntMoveLeft;

i32 g_debugGruntMoveTop;

i32 g_debugGruntMoveRight;

i32 g_debugGruntMoveBottom;

i32 g_debugGruntAiType;

b32 g_monologoShown;

CGruntzMgr* g_gameReg = NULL;

u32 g_gruntDestruction;

u32 g_gruntCreation;

u32 g_gooPuddlez;

u32 g_explosionz;

u32 g_resolutionChanged;

DebugDisplayFlags g_debugDisplayFlags;

DirectInputMgr2* g_inputMgr = NULL;

CInputState* g_gameplayInput = NULL;

i32 g_localVersion = 1;

i32 g_remoteVersion = 1;

i32 g_unreferencedGruntzMgrValues[16] = {1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 8, 9, 10};

GUID g_dplayAppGuid =
    {0xf41cf640, 0x91b2, 0x11d1, {0x8d, 0xfc, 0x00, 0x60, 0x97, 0x9f, 0xa8, 0x1e}};

b32 g_pendingFrame = true;

i32 g_warpX = -1;

i32 g_warpY = -1;

CGruntzMgr::CGruntzMgr() {
    m_curState = NULL;
    m_completingStateChange = false;
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
    m_lobbyProbed = false;
    m_reserveda8 = 0;
    m_modalBusy = false;
    m_renderGate = false;
    m_reservedb4 = 0;
    m_loadingSaveGame = false;
    m_isCheckpointPrompts = true;
    m_connSettings = NULL;
    m_saveInfoRec = NULL;
    m_numRuns = 0;
    m_numMovies = 0;
    m_reservedcc = 0x1e;
    SET_SIZE_COMPONENTS(m_modeSize, 0, 0);
    m_colorDepth = BPP_RGB_16;
    m_inGameDir = true;
    m_haveRez = false;
    m_haveMoviez = false;
    m_musicEnabled = true;
    m_soundEnabled = true;
    m_isVoiceEnabled = true;
    m_isAmbientEnabled = true;
    m_isInterlaced = false;
    m_isEasyMode = false;
    m_isCustomLevel = false;
    m_isBuiltInBattlezLevel = false;
    m_isBuiltInMultiplayerLevel = false;
    m_gameMode = GAMEMODE_NONE;
    m_isHighDetail = true;
    m_isEffectsEnabled = true;
    m_computerPlayerCount = 3;
}

i32 CGruntzMgr::IsActive() {
    if (m_world) {
        if (m_curState) {
            return 1;
        }
    }
    return 0;
}

GZ_ENUM_CONST_BEGIN(GruntzGameTiming)
    GRUNTZ_PERIODIC_TIMER_MS = 33
GZ_ENUM_CONST_END(GruntzGameTiming)

b32 g_disableAudio = false;

b32 g_disableSound = false;

b32 g_disableMusic = false;

b32 g_disableFades = false;

b32 g_disableJoystick = false;

b32 g_disableSoundFonts = false;

b32 g_disableDirectVideo = false;

b32 g_enableHqMovie = false;

b32 g_enableTriple = false;

b32 g_enableHiColor = false;

b32 g_enableTrueColor = false;

b32 g_enableEmulation = false;

CGruntzMgr::~CGruntzMgr() {
    Close();
}

i32 CGruntzMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {

    if (!g_coordPool.Init(0x4e20, 4)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x404);
        return 0;
    }

    if (!CGameMgr::Run(pGameWnd, szCmdLine)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x462);
        return 0;
    }
    if (!InitializeFonts()) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x463);
        return 0;
    }
    srand((timeGetTime() + GetTickCount()) >> 1);
    m_frames.timing().setTimerPeriod(GRUNTZ_PERIODIC_TIMER_MS);
    while (ShowCursor(false) >= 0) {
    }

    Settings* reg = new Settings;
    m_settings = reg;
    if (!m_settings
             ->load(settingsPath())) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x406);
        return 0;
    }
    SET_SIZE_COMPONENTS(m_savedModeSize, SCREEN_W_PX, SCREEN_H_PX);
    m_numRuns = m_settings->getInt("Num Runs", 0);
    m_numMovies = m_settings->getInt("Num Movies", 0);
    g_enableHqMovie = m_settings->getInt("Disable High Quality Movie", 0) == 0;
    g_disableAudio = m_settings->getInt("Disable Audio", 0);
    g_disableSound = m_settings->getInt("Disable Sound", 0);
    g_disableMusic = m_settings->getInt("Disable Music", 0);
    g_disableFades = m_settings->getInt("Disable Fades", 0);
    g_disableDirectVideo = m_settings->getInt("Disable Direct Video Access", 0);
    g_disableJoystick = m_settings->getInt("Disable Joystick", 0);
    g_disableSoundFonts = m_settings->getInt("Disable SoundFonts", 0);
    g_enableTriple = m_settings->getInt("Enable Triple", 0);
    g_enableHiColor = m_settings->getInt("Enable HiColor", 0);
    g_enableTrueColor = m_settings->getInt("Enable TrueColor", 0);
    g_enableEmulation = m_settings->getInt("Enable Emulation", 0);
    m_isCheckpointPrompts = m_settings->getInt("Checkpoint Prompts", 1);
    g_enableHiColor = true;
    g_debugGruntPlayer = 0;
    g_debugGruntTool = 0;
    g_debugGruntToy = 0;
    g_debugGruntAiType = 0;
    g_debugGruntColumn = 0;
    g_debugGruntRow = 0;
    g_debugGruntColor = 1;
    g_debugGruntRadius = 0;
    g_debugGruntMoveLeft = 0;
    g_debugGruntMoveRight = 0;
    g_debugGruntMoveTop = 0;
    g_debugGruntMoveBottom = 0;

    i32 vMusic = m_settings->getInt("Music", m_musicEnabled);
    i32 vSound = m_settings->getInt("Sound", m_soundEnabled);
    i32 vVoice = m_settings->getInt("Voice", m_isVoiceEnabled);
    i32 vAmbient = m_settings->getInt("Ambient", m_isAmbientEnabled);
    i32 vInterlaced = m_settings->getInt("Interlaced", m_isInterlaced);
    i32 vHigh1 = m_settings->getInt("High Detail", m_isHighDetail);
    i32 vHigh2 = m_settings->getInt("High Detail", m_isEffectsEnabled);
    m_isEasyMode = m_settings->getInt("Easy Mode", m_isEasyMode);
    Resolution resolution =
        static_cast<Resolution>(m_settings->getInt("Resolution", IDX(RES_640X480)));
    if (resolution == RES_1024X768) {
        SET_SIZE_COMPONENTS(m_savedModeSize, DISPLAY_WIDTH_1024, DISPLAY_HEIGHT_768);
    } else if (resolution == RES_800X600) {
        SET_SIZE_COMPONENTS(m_savedModeSize, DISPLAY_WIDTH_800, DISPLAY_HEIGHT_600);
    } else {
        SET_SIZE_COMPONENTS(m_savedModeSize, SCREEN_W_PX, SCREEN_H_PX);
    }
    i32 musicVolume = m_settings->getInt("Music Volume", 0x64);
    i32 soundVolume = m_settings->getInt("Sound Volume", 0x3c);
    i32 voiceVolume = m_settings->getInt("Voice Volume", 0x50);
    i32 scrollSpeed = m_settings->getInt("Scroll Speed", 0x14);
    m_soundVolume = soundVolume;
    m_voiceVolume = voiceVolume;

    m_scrollSpeed = scrollSpeed;
    m_numRuns = m_numRuns + 1;
    if (g_disableDirectVideo != false) {
        g_disableFades = true;
        g_enableEmulation = true;
    }
    m_modalBusy = false;
    m_renderGate = false;
    m_driveLetterProbed = false;
    m_driveLetter = 0;
    GetGruntzDriveLetter();

    i32 noLogo = 0;
    GameStateId mode = GAMESTATE_ATTRACT;
    char cpy[300];
    char levelName[0x80];
    levelName[0] = 0;
    if (szCmdLine) {
        char buf[300];
        strcpy(buf, szCmdLine);
        _strupr(buf);
        if (strstr(buf, "PLAY")) {
            mode = GAMESTATE_PLAY;
        }
        if (strstr(buf, "MULTI")) {
            mode = GAMESTATE_MULTI;
        }
        if (strstr(buf, "DEMO")) {
            mode = GAMESTATE_DEMO;
        }
        if (strstr(buf, "SELECT")) {
            mode = GAMESTATE_LEVEL_SELECT;
        }
        if (strstr(buf, "NOLOGO")) {
            noLogo = 1;
        }
        strstr(buf, "NOMOVIES");
        if (strstr(buf, "LOAD:")) {
            strcpy(cpy, buf);
            char* tok = strstr(cpy, "LOAD:");
            if (tok && strlen(tok) > 5) {
                tok += 5;
                i32 j = 0;
                while (tok[j] != ' ' && tok[j] != 0) {
                    ++j;
                }
                tok[j] = 0;
                if (tok[0] != 0) {
                    for (char* q = tok; *q; ++q) {
                        if (*q == '_') {
                            *q = ' ';
                        }
                        if (*q == '+') {
                            *q = ' ';
                        }
                    }
                }
                strcpy(levelName, tok);
            }
        }
    }
    if (InitializeLobbyConnectionSettings()) {
        mode = GAMESTATE_MULTI;
        m_reservedb4 = 0;
    }

    g_gruntzWinApp.m_hInstance = m_owner->m_hInstance;
    char dpBuf[256];
    strcpy(dpBuf, szCmdLine);
    AfxWinInit(m_owner->m_hInstance, NULL, dpBuf, SW_SHOWNORMAL);
    (m_strWorldFile).erase();

    m_world = new CDDrawSurfaceMgr;
    i32 flags = 0xe1;
    if (g_disableAudio || g_disableSound) {
        flags = 0xe5;
    }
    if (g_enableEmulation) {
        flags |= 0x10;
    }
    m_colorDepth = BPP_RGB_16;
    if (!m_world->Init(m_gameWnd->m_hwnd, SCREEN_W_PX, SCREEN_H_PX, BPP_RGB_16, flags)) {
        ReportWorldStatus(WORLD_REPORT_STARTUP_INIT);
        return 0;
    }
    {
        LevelCoordRect rect;
        SET_RECT_COMPONENTS(rect, 0, 0, 0x1df, 0x1df);
        LevelOf(m_world)->UpdatePlaneViewports(&rect);
    }
    SET_SIZE_COMPONENTS(m_modeSize, SCREEN_W_PX, SCREEN_H_PX);
    m_world->SetRestoreHandler(&PumpIdleFrame);
    CGameLevel* view = LevelOf(m_world);
    view->m_maxStepX = 0xe;
    view->m_maxStepY = 0xe;
    m_world->m_drawTarget->CreateOverlay(0, 0x30000);
    RecomputeViewScale();
    RegisterGameObjectLogicTypes(m_world);
    if (!MakeRezPath()) {
        return 0;
    }

    SAFE_DELETE(m_resourceArchive);
    m_resourceArchive = new CRezMgr;
    bool parseFailed = ResourceArchive()->Open(
                           (GetRezPath()).c_str(),
                           true,
                           false
                       )
                       == 0;
    if (parseFailed) {
        ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x409);
        return 0;
    }
    if (!ResourceArchive()->OpenAdditional(const_cast<char*>("GRUNTZ.VRZ"), false)) {
        ReportError(IDX(IDS_LOAD_VOICE_RESOURCE_FILE), 0x460);
        return 0;
    }
    ResourceArchive()->OpenAdditional(const_cast<char*>("GRUNTZ.ZZZ"), true);
    ResourceArchive()->OpenAdditional(const_cast<char*>("GRUNTZ.XXX"), true);
    SetColorDepth(m_colorDepth);

    m_faderMgr = new CFaderMgr;
    if (!m_faderMgr->SetDefaults(NULL, NULL, NULL)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40a);
        return 0;
    }
    m_cheatMgr = new CCheatMgr;
    if (!CheatMgr()->Init(m_gameWnd->m_hwnd)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40b);
        return 0;
    }
    if (g_disableAudio == false && g_disableSoundFonts == false) {
        if (SFManager_SelectBestDevice()) {
            if (!BuildSoundFontPath(GetGruntzDriveLetter())) {
                CloseSoundFontDevice();
            }
        }
    }

    m_midi = new MidiManager;
    if (!m_midi->Initialize(m_owner->m_hInstance, m_gameWnd->m_hwnd, false)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40c);
        return 0;
    }
    if (g_disableAudio == false && g_disableMusic == false) {
        m_midi->SetMasterVolume(musicVolume);
    } else {
        m_midi->SetEnabled(false);
    }

    SAFE_DELETE(m_worldSounds);
    m_worldSounds = new CWorldSoundSet;
    if (!m_worldSounds->Init(m_world->SoundRegistry(), soundVolume)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40d);
        return 0;
    }
    m_worldSounds->SetEnabled(vAmbient);
    SetSoundVolume(soundVolume);

    SetVoiceVolume(voiceVolume);
    m_scrollSpeed = scrollSpeed;

    g_inputMgr = new DirectInputMgr2;
    if (!g_inputMgr->Create(
            m_gameWnd->m_hwnd,
            m_owner->m_hInstance,
            IDX(DIN_CREATE_ASYNC_KEYBOARD | DIN_CREATE_NO_MOUSE | DIN_CREATE_NO_JOYSTICKS)
        )) {
        SAFE_DELETE(g_inputMgr);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40e);
        return 0;
    }

    g_actorList = g_inputMgr->CreateDeviceGroup(
        g_inputMgr->GetJoystick(0),
        g_inputMgr->GetJoystick(1),
        g_inputMgr->GetJoystick(2),
        g_inputMgr->GetJoystick(3),
        NULL,
        NULL,
        0
    );
    if (!g_actorList) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40f);
        return 0;
    }

    CKeyboardDevice* keyboard = g_inputMgr->m_keyboard;
    if (keyboard != NULL) {
        keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON0)] = VK_CONTROL;
        keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON1)] = 'X';
        keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON2)] = VK_SPACE;
        keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON3)] = VK_RETURN;
        keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON8)] = 0;
    }

    m_shadeCache = new CShadeTableCache;
    if (!m_shadeCache->Init()) {
        SAFE_DELETE(m_shadeCache);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x410);
        return 0;
    }

    m_lightFxMgr = new CLightFxMgr;
    if (!m_lightFxMgr->Init(this, NULL)) {
        SAFE_DELETE(m_lightFxMgr);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x411);
        return 0;
    }
    m_saveGame = new CSaveGame;
    if (!m_saveGame->InitializeSaveDirectory(".")) {
        SAFE_DELETE(m_saveGame);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x412);
        return 0;
    }
    m_gameStats = new CGameStats;
    if (!m_gameStats->ResetWithLevelRecords(m_saveGame->m_levelStats)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x464);
        return 0;
    }

    g_gameplayInput = new CInputState;
    if (!g_gameplayInput->Init(g_inputMgr, INPUTDEV_KEYBOARD_JOYSTICK1)) {
        SAFE_DELETE(g_gameplayInput);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x413);
        return 0;
    }

    m_commandMgr = new CGruntzCmdMgr;

    if (!m_commandMgr->SetManager(this)) {
        SAFE_DELETE(m_commandMgr);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x414);
        return 0;
    }
    m_tileGrid = new CGruntzMapMgr;
    if (!m_tileGrid) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x415);
        return 0;
    }
    m_spriteFactory = new CSpriteRefTable;

    if (!m_spriteFactory->Init(m_shadeCache, m_world)) {
        SAFE_DELETE(m_spriteFactory);

        ReportError(IDX(IDS_INITIALIZE_GAME), 0x416);
    }

    g_gameReg = this;
    g_lastNow = timeGetTime();
    g_frameDelta = 0;
    for (i32 s = 0; s < 4; ++s) {
        if (!m_players[s].SeedForSlot(s)) {
            ReportError(IDX(IDS_INITIALIZE_GAME), 0x417);
            return 0;
        }
    }

    {
        CRezItm* stream =
            g_gameReg->ResourceArchive()->GetRezFromPath("GAME_ATTRIBUTEZ", REZ_TAG_TXT);

        if (0) {
            AfxTrace("%s\n", (std::string("parsing ") + "GAME_ATTRIBUTEZ").c_str());
        }
        g_buteMgr.Init(&ButeParseErrorSink);
        if (!g_buteMgr.Parse(stream, "1212C")) {
            ReportError(IDX(IDS_INITIALIZE_GAME), 0x418);
            return 0;
        }
    }

    CheatMgr()->RegisterCheats();
    m_chatLog = new CFontConfig;
    if (!ChatLog()->LoadFontConfig(0x1388, 0xbb8)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x41a);
        return 0;
    }
    m_triggerMgr = new CTriggerMgr;
    if (!m_triggerMgr->SetLevel(World())) {
        SAFE_DELETE(m_triggerMgr);
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x41b);
        return 0;
    }
    g_localVersion = static_cast<i32>(
        g_buteMgr.GetDword("General", "RezSync", static_cast<u32>(g_localVersion))
    );
    m_voiceManager = new CVoiceManager;
    if (!VoiceMgr()->Init(this)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x45f);
        return 0;
    }
    VoiceMgr()->SetVolume(voiceVolume);
    m_musicEnabled = vMusic;
    m_soundEnabled = vSound;
    g_soundEnabled = vSound;
    m_isVoiceEnabled = vVoice;
    m_isAmbientEnabled = vAmbient;
    m_isInterlaced = vInterlaced;
    m_isHighDetail = vHigh1;
    m_isEffectsEnabled = vHigh2;
    if (!World()->SoundRegistry()->HasWithPrefix("GAME")) {
        CRezDir* sz = ResourceArchive()->GetDirFromPath("GAME_SOUNDZ");
        if (!sz) {
            return 0;
        }
        m_world->SoundRegistry()->LoadFromTree(sz, "GAME", "_");
    }
    {
        SoundCue* movieCue = World()->SoundRegistry()->FindCue("GAME_MOVIE");
        World()->SoundRegistry()->ConfigurePrimaryFromCue(movieCue, 0);
    }
    CheckMovieFileExists();
    if (!InitializeLobbyConnectionSettings()) {
        if (m_numMovies > 0 && m_numRuns > 1) {
            i32 skipLogo = m_settings->getInt("Skip Logo Movies", 0);
            if (skipLogo == 0 && noLogo == 0) {
                PlayLogoMovie();
            }
        } else {
            PlayLogoMovie();
            if (PlayMovieEntry(IDX(MOVIE_ENTRY_INTRO))) {
                ++m_numMovies;
            }
        }
    }

    CRezDir* attract = ResourceArchive()->GetDirFromPath("STATEZ_ATTRACT");
    g_attractStateCount = 0;
    std::string title;
    title = formatText("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
    CRezItm* titleImage = attract->GetRezFromPath((title).c_str(), IMGTAG_XCP);
    while (titleImage != NULL) {
        g_attractStateCount++;
        title = formatText("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        titleImage = attract->GetRezFromPath((title).c_str(), IMGTAG_XCP);
    }
    StateChangeOptions startup(mode == GAMESTATE_MULTI ? 0x41c : 0x41d,
        IDS_SET_GAME_STATE, mode == GAMESTATE_MULTI ? GAMESTATE_ATTRACT : GAMESTATE_NONE);
    if (!TransitionState(mode, 1, false, 0, startup)) return 0;
    g_frameDelta = 0;
    return 1;
}

std::string CGruntzMgr::GetRezPath() {
    return m_strRezPath;
}


void CGruntzMgr::Close() {
    CancelStateChange();
    if (m_world) {
        World()->SetRestoreHandler(NULL);
    }
    FreeFontsMemory();
    if (m_settings) {
        m_settings->setInt("Num Runs", m_numRuns);
        m_settings->setInt("Num Movies", m_numMovies);
        m_settings->setInt("Sound", m_soundEnabled);
        m_settings->setInt("Voice", m_isVoiceEnabled);
        m_settings->setInt("Ambient", m_isAmbientEnabled);
        m_settings->setInt("Music", m_musicEnabled);
        m_settings->setInt("Interlaced", m_isInterlaced);
        m_settings->setInt("High Detail", m_isHighDetail);
        m_settings->setInt("Effects", m_isEffectsEnabled);
        m_settings->setInt("Disable Joystick", g_disableJoystick);
        if (m_midi) {
            m_settings->setInt("Music Volume", m_midi->GetMasterVolume());
        }
        if (m_voiceManager) {
            m_settings->setInt("Voice Volume", VoiceMgr()->m_voiceVolume);
        }
        if (m_world && World()->SoundRegistry()) {
            m_settings->setInt("Sound Volume", g_soundVolumePercent);
        }
        m_settings->setInt("Scroll Speed", m_scrollSpeed);
        m_settings->setInt("Easy Mode", m_isEasyMode);
        Resolution res = RES_640X480;
        if (m_savedModeSize.cx == DISPLAY_WIDTH_1024 && m_savedModeSize.cy == DISPLAY_HEIGHT_768) {
            res = RES_1024X768;
        } else if (m_savedModeSize.cx == DISPLAY_WIDTH_800
                   && m_savedModeSize.cy == DISPLAY_HEIGHT_600) {
            res = RES_800X600;
        }
        m_settings->setInt("Resolution", IDX(res));
        m_settings->setInt("Checkpoint Prompts", m_isCheckpointPrompts);
        if (m_colorDepth == BPP_RGB_16) {
            m_settings->setInt("Enable HiColor", 1);
        } else {
            m_settings->setInt("Enable HiColor", static_cast<DWORD>(0));
        }
        m_settings->setInt("Enable TrueColor", static_cast<DWORD>(0));
    }
    ClearStateStack();
    SAFE_DELETE(m_curState)
    SAFE_DELETE(m_spriteFactory)
    SAFE_DELETE(m_triggerMgr)
    SAFE_DELETE(m_tileGrid)
    CGameStats* gameStats = m_gameStats;
    if (gameStats) {
        delete gameStats;
        m_gameStats = NULL;
    }
    SAFE_DELETE(m_commandMgr)
    SAFE_DELETE(g_gameplayInput)
    SAFE_DELETE(g_inputMgr)
    SAFE_DELETE(m_cheatMgr)
    SAFE_DELETE(m_midi)
    SAFE_DELETE(m_worldSounds)
    SAFE_DELETE(m_faderMgr)
    SAFE_DELETE(m_chatLog)
    SAFE_DELETE(m_voiceManager)
    SAFE_DELETE(m_world)
    SAFE_DELETE(m_resourceArchive)
    if (m_settings && m_settings->loaded() && !m_settings->save()) {
        MessageBoxA(NULL, "Could not save your settings.", "Gruntz", MB_OK | MB_ICONERROR);
    }
    SAFE_DELETE(m_settings)
    SAFE_DELETE(m_reserved3c)
    SAFE_DELETE(m_shadeCache)
    SAFE_DELETE(m_saveGame)
    SAFE_DELETE(m_lightFxMgr)
    CloseSoundFontDevice();
    SAFE_RELEASE(m_lobby)
    if (m_connSettings) {
        RecordBytes<DPLCONNECTION> settings;
        settings.m_rec = m_connSettings;
        delete[] settings.m_bytes;
        m_connSettings = NULL;
    }
    this->CGameMgr::Close();
    g_gameReg = NULL;
}

void CGruntzMgr::CommitSinglePlayerProgress() {
    if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
        return;
    }
    CState* currentState = g_gameReg->m_curState;

    m_gameStats->m_gruntzExited += m_triggerMgr->m_gruntzExitedByPlayer[g_curPlayer];
    m_gameStats->m_gruntzLost += m_triggerMgr->m_gruntzLostByPlayer[g_curPlayer];

    if (!(m_strWorldFile).empty()) {
        m_gameStats->SetLevelNumber(1);
        m_gameStats->m_isCustomLevel = true;
        return;
    }

    if (CheatMgr()->m_cheatsUsed == false) {
        m_gameStats->UpdateLevelRecord(currentState->m_levelIndex, false);
        g_gameReg->m_saveGame->SetCurLevel(static_cast<QuestLevel>(currentState->m_levelIndex));
        g_gameReg->m_saveGame->SetMaxLevel(
            static_cast<QuestLevel>(
                (currentState->m_levelIndex % IDX(QUESTLEVEL_TRAINING_LAST)) + 1
            )
        );
        g_gameReg->m_saveGame->SaveProgress();
    }
    m_gameStats->SetLevelNumber(currentState->m_levelIndex);
    m_gameStats->m_isCustomLevel = false;
}

void CGruntzMgr::FinalizeLevelAndShowResults() {
    CState* currentState = m_curState;
    if (m_gameMode == GAMEMODE_QUESTZ) {
        if (m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
            CommitSinglePlayerProgress();
        }
        TransitionState(GAMESTATE_BOOTY, 1, false, 0);
        return;
    }
    g_gameReg->m_gameStats->SetLevelNumber(currentState->m_levelIndex);
    if (m_gameMode == GAMEMODE_BATTLEZ) {

        g_gameReg->m_gameStats->m_elapsedTimeMs +=
            static_cast<CPlay*>(currentState)->m_levelTimer->m_stamp.Elapsed();
        TransitionState(GAMESTATE_MULTIBOOTY, 1, false, 0);
        return;
    }
    CGameStats* gameStats = g_gameReg->m_gameStats;
    u32 now = timeGetTime();
    gameStats->m_elapsedTimeMs += (now - g_roundStartTimeMs);
    TransitionState(GAMESTATE_MULTIBOOTY, 1, false, 0);
}

i32 PumpIdleFrame() {
    if (g_pendingFrame == false) {
        return 0;
    }
    g_pendingFrame = false;
    if (g_gameReg == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = g_gameReg->World();
    if (world == NULL) {
        return 0;
    }
    if (world->m_imageRegistry == NULL) {
        return 0;
    }
    if (g_gameReg->m_curState == NULL) {
        return 0;
    }
    CState* state = g_gameReg->m_curState;
    state->CancelSceneFade();
    const bool restored = state->IsLoading() ? state->RestoreLoading() != 0
        : state->IsDeparting() ? state->RecoverDeparture() != 0
        : state->InputVirtual() != 0;
    if (!restored) {
        g_gameReg->ReportError(IDX(IDS_RESTORE_GAME), 0x435);
        return 0;
    }
    g_gameReg->RefreshGameClock();
    g_pendingFrame = true;
    return 1;
}

bool CGruntzMgr::QueueStateChange(const StateChange& change) {
    if (IsQuitPending() || IsStateTransitioning()) return false;
    m_stateChange = change;
    m_loadingSaveGame = change.options.restoreSave;
    if (!m_stateTransition.request()) return false;
    // A command can arrive after the previous state stopped normal rendering.
    if (m_owner) m_owner->m_running = true;
    return true;
}

i32 CGruntzMgr::TransitionState(GameStateId stateId, i32 areaArg, b32 keepCurrent,
    i32 unused, const StateChangeOptions& options) {
    StateChange change;
    change.target = stateId;
    change.level = areaArg;
    change.keepCurrent = keepCurrent != false;
    change.options = options;
    return QueueStateChange(change);
}

bool CGruntzMgr::BeginDeparture() {
    m_stateChange.previous = m_curState ? m_curState->Update() : GAMESTATE_NONE;
    if (!m_curState) return true;
    if (m_stateChange.keepCurrent) m_stateChange.level = m_curState->m_levelIndex;
    return m_curState->StartDeparture(m_stateChange.target);
}

TransitionProgress CGruntzMgr::AdvanceDeparture(u32 deltaMs) {
    return m_curState ? m_curState->AdvanceStateDeparture(deltaMs) : TransitionComplete;
}

bool CGruntzMgr::InstallDestination() {
    if (IsQuitPending()) return false;
    if (m_stateChange.kind == ReloadState) {
        if (!m_curState || !static_cast<CPlay*>(m_curState)->LoadByMode(
                m_stateChange.level, m_stateChange.loadMode)) return false;
    } else if (m_stateChange.kind == ResumeStackedState) {
        CState* next = TopState();
        if (!next || next == m_curState) return false;
        delete m_curState;
        m_curState = next;
        PopTopIfMatches(next);
    } else {
        if (m_curState && m_stateChange.keepCurrent) PushState(m_curState);
        else {
            delete m_curState;
            m_curState = NULL;
            ClearStateStack();
        }
        m_curState = NULL;
    switch (m_stateChange.target) {
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

        if (!m_curState) return false;
        if (!m_curState->LoadGameAssetNamespaces(this, m_stateChange.level,
                IDX(m_stateChange.previous))) {
            delete m_curState;
            m_curState = NULL;
            return false;
        }
    }
    return !IsQuitPending() && m_stateTransition.active();
}

TransitionProgress CGruntzMgr::AdvanceInstallation(u32 deltaMs) {
    if (IsQuitPending() || !m_curState) return TransitionFailed;
    const TransitionProgress result = m_curState->AdvanceLoading(deltaMs);
    if (result == TransitionFailed && m_stateChange.kind == ReplaceState) {
        delete m_curState;
        m_curState = NULL;
    }
    return result;
}

bool CGruntzMgr::BeginArrival() {
    if (IsQuitPending() || !m_stateTransition.active() || !m_curState) return false;
    if (!m_curState->EnterState(m_stateChange.previous)) {
        if (m_stateChange.kind != ResumeStackedState || !m_curState->RestoreDisplay()) {
            if (m_stateChange.kind == ReplaceState) {
                delete m_curState;
                m_curState = NULL;
            }
            return false;
        }
    }
    if (IsQuitPending() || !m_stateTransition.active()) return false;
    if (m_owner) m_owner->m_running = true;
    g_inputMgr->ReadAll();
    RefreshGameClock();
    return true;
}

TransitionProgress CGruntzMgr::AdvanceArrival(u32 deltaMs) {
    if (!m_curState) return TransitionFailed;
    if (m_curState->IsSceneFading()) {
        if (m_curState->AdvanceSceneFade(deltaMs) < 0) return TransitionFailed;
        if (m_curState->IsSceneFading()) return TransitionPending;
    }
    return TransitionComplete;
}

void CGruntzMgr::CancelStateChange() {
    m_stateTransition.cancel();
    m_stateChange = StateChange();
    m_loadingSaveGame = false;
    if (m_curState) {
        m_curState->CancelLoading();
        m_curState->CancelDeparture();
    }
}

void CGruntzMgr::AdvanceStateChange(u32 deltaMs) {
    if (!m_stateTransition.active()) return;
    const TransitionProgress result = m_stateTransition.advance(*this, deltaMs);
    if (IsQuitPending() || result == TransitionPending) return;
    StateChangeOptions options = m_stateChange.options;
    const bool failedLoading = result == TransitionFailed && m_curState && m_curState->IsLoading();
    m_stateChange = StateChange();
    if (result == TransitionFailed) {
        m_loadingSaveGame = false;
        if (m_curState) {
            m_curState->CancelLoading();
            m_curState->CancelDeparture();
            if (failedLoading) { delete m_curState; m_curState = NULL; }
        }
        if (options.fallback != GAMESTATE_NONE) {
            const GameStateId fallback = options.fallback;
            options.fallback = GAMESTATE_NONE;
            options.snapshot.erase();
            options.restoreSave = false;
            options.postCommand = false;
            options.connectRound = false;
            if (TransitionState(fallback, 1, false, 0, options)) return;
        }
        ReportError(IDX(options.error), options.site);
        return;
    }
    m_completingStateChange = true;
    bool completed = true;
    if (options.restoreSave) {
        if (!RestoreGameFromFile(this, options.snapshot)) {
            ReportError(IDX(IDS_SET_GAME_STATE), 0x465);
            completed = false;
        } else if (!IsQuitPending()) CheckSavedMode();
    }
    m_loadingSaveGame = false;
    if (completed && !IsQuitPending() && options.connectRound) {
        completed = m_curState && m_curState->Update() == GAMESTATE_MULTI
            && static_cast<CMulti*>(m_curState)->FinishConnect();
    }
    m_completingStateChange = false;
    if (completed && !IsQuitPending() && options.postCommand) {
        PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(options.afterCommand), 0);
    }
}

i32 CState::LeaveState(GameStateId nextState) {
    return 1;
}

CDemo::~CDemo() {
    CDemo::ReleaseResources();
}

GameStateId CMulti::Update() {
    return GAMESTATE_MULTI;
}

i32 CMulti::UnusedPlayQuery() {
    return 0;
}

i32 CMulti::GetFrame() {
    return m_session->m_commandTick;
}

i32 CGruntzMgr::SwitchToNextState(const StateChangeOptions& options) {
    CState* next = TopState();
    if (!IsActive() || !next || next == m_curState) return 0;
    StateChange change;
    change.kind = ResumeStackedState;
    change.target = next->Update();
    change.options = options;
    return QueueStateChange(change);
}

i32 CGruntzMgr::PassClickToPlayState(i32 areaArg, b32 forceTransition, i32 unused,
    const StateChangeOptions& options) {
    if (m_curState && !forceTransition && (m_curState->Update() == GAMESTATE_PLAY
            || m_curState->Update() == GAMESTATE_MULTI)) {
        StateChange change;
        change.kind = ReloadState;
        change.target = m_curState->Update();
        change.level = areaArg;
        change.loadMode = unused;
        change.options = options;
        return QueueStateChange(change);
    }
    return TransitionState(GAMESTATE_PLAY, areaArg, false, 0, options);
}

i32 CGruntzMgr::GoToNextLevel() {
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    (m_strWorldFile).erase();
    CState* st = m_curState;
    i32 next = st->m_levelIndex + 1;
    if (next > IDX(QUESTLEVEL_TRAINING_LAST)) {
        next = IDX(QUESTLEVEL_FIRST);
    }
    if (next <= IDX(QUESTLEVEL_CAMPAIGN_LAST) || next >= IDX(QUESTLEVEL_TRAINING_FIRST)) {
        return PassClickToPlayState(next, false, 1, StateChangeOptions(0x436, IDS_CHANGE_LEVEL));
    }
    ReportError(IDX(IDS_CHANGE_LEVEL), 0x436);
    return 0;
}

i32 CGruntzMgr::GoToPrevLevel() {
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    (m_strWorldFile).erase();
    CState* st = m_curState;
    i32 prev = st->m_levelIndex - 1;
    if (prev <= 0) {
        prev = IDX(QUESTLEVEL_TRAINING_LAST);
    }
    if (prev <= IDX(QUESTLEVEL_CAMPAIGN_LAST) || prev >= IDX(QUESTLEVEL_TRAINING_FIRST)) {
        return PassClickToPlayState(prev, false, 1, StateChangeOptions(0x437, IDS_CHANGE_LEVEL));
    }
    ReportError(IDX(IDS_CHANGE_LEVEL), 0x437);
    return 0;
}

i32 CGruntzMgr::ForwardCharToState(i32 charCode, i32 keyData) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnChar(charCode, keyData);
    }
    return 0;
}

i32 CGruntzMgr::ForwardKeyDownToState(i32 virtualKey, i32 keyData) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnKeyDown(virtualKey, keyData);
    }
    return 0;
}

i32 CGruntzMgr::ForwardKeyUpToState(i32 virtualKey, i32 keyData) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnKeyUp(virtualKey, keyData);
    }
    return 0;
}

i32 CGruntzMgr::ForwardLButtonDownToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnLButtonDown(keyFlags, x, y);
    }
    return 0;
}

i32 CGruntzMgr::ForwardLButtonUpToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnLButtonUp(keyFlags, x, y);
    }
    return 0;
}

i32 CGruntzMgr::ForwardLButtonDblClkToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnLButtonDblClk(keyFlags, x, y);
    }
    return 0;
}

i32 CGruntzMgr::ForwardRButtonDownToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnRButtonDown(keyFlags, x, y);
    }
    return 0;
}

i32 CGruntzMgr::ForwardRButtonUpToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnRButtonUp(keyFlags, x, y);
    }
    return 0;
}

i32 CGruntzMgr::ForwardRButtonDblClkToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnRButtonDblClk(keyFlags, x, y);
    }
    return 0;
}

i32 CGruntzMgr::ForwardMouseMoveToState(i32 keyFlags, i32 x, i32 y) {
    if (m_curState && !IsQuitPending() && !IsStateTransitioning() && !IsSceneFading()) {
        return m_curState->OnMouseMove(keyFlags, x, y);
    }
    return 0;
}

void CGruntzMgr::XorLiveObjectFlags(i32 mask) {
    std::list<CGameObject*>* list = &World()->ChildGroup()->m_list;
    if (list == NULL) {
        return;
    }
    std::list<CGameObject*>::iterator pos = list->begin();
    while (pos != list->end()) {
        CGameObject* obj = World()->ChildGroup()->NextChild(pos);
        if (obj) {
            obj->m_stateFlags ^= static_cast<SpriteStateFlags>(mask);
        }
    }
}

void CGruntzMgr::ReportError(WPARAM wParam, LPARAM lParam) {
    CGameApp* pApp = m_owner;
    if (pApp) {
        pApp->ReportError(wParam, lParam);
    }
}

void CGruntzMgr::RegisterLevelAssetKeys() {
    CDDrawSurfaceMgr* w = m_world;
    if (w == NULL) {
        return;
    }

    SoundCueRegistry* snd = w->SoundRegistry();
    w->m_imageRegistry->SumSizesEqual("", 1);
    snd->SumAudioBytes("");
    w->GetDeviceManager()->GetCapsChecked();
    w->GetDeviceManager()->GetCapsChecked();
    w->m_imageRegistry->SumSizesEqual("", 1);
    w->m_imageRegistry->SumSizesEqual("GRUNTZ", 1);
    w->m_imageRegistry->SumSizesEqual("GAME", 1);
    w->m_imageRegistry->SumSizesEqual("LEVEL", 1);
    w->m_imageRegistry->SumSizesEqual("ACTION", 1);
    w->SoundRegistry()->SumAudioBytes("");
    w->SoundRegistry()->SumAudioBytes("GRUNTZ");
    w->SoundRegistry()->SumAudioBytes("GAME");
    w->SoundRegistry()->SumAudioBytes("LEVEL");
}

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

i32 CGruntzMgr::RestoreVideoMode(b32 save) {
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

i32 CGruntzMgr::CheckSavedMode() {

    if ((m_modeSize.cx == m_savedModeSize.cx && m_modeSize.cy == m_savedModeSize.cy)
        || SetVideoMode(m_savedModeSize.cx, m_savedModeSize.cy, true) || RestoreVideoMode(true)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x45e);
    return 0;
}

i32 CGruntzMgr::SetVideoMode(i32 w, i32 h, b32 saveMode) {
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

    if (m_curState) m_curState->CancelSceneFade();
    if (!m_world->SetDimensions(w, h, m_colorDepth)) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    SET_SIZE_COMPONENTS(m_modeSize, w, h);
    if (m_curState->Update() == GAMESTATE_PLAY || m_curState->Update() == GAMESTATE_MULTI) {
        if (saveMode) {
            SET_SIZE_COMPONENTS(m_savedModeSize, w, h);
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
    if (g_resolutionChanged != false) {
        g_resolutionChanged = false;
        char buf[SERIAL_NAME_LEN];

        sprintf(buf, "Resolution is now %ix%ix%i", m_modeSize.cx, m_modeSize.cy, m_colorDepth);
        AppendChatMessage(buf);
    }
    return 1;
}

i32 CGruntzMgr::TryNextResolution() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_MULTI) {
        return 1;
    }
    DisplayResolution resolution;
    resolution =
        World()->GetDeviceManager()->FindNextResolution(m_modeSize.cx, m_modeSize.cy, m_colorDepth);
    i32 width = resolution.m_width;
    i32 height = resolution.m_height;
    if (width > 0x514 || width == -1 || height == -1) {
        return 1;
    }
    if (SetVideoMode(width, height, true)) {
        return 1;
    }
    if (SetVideoMode(SCREEN_W_PX, SCREEN_H_PX, true)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x439);
    return 0;
}

i32 CGruntzMgr::TryPreviousResolution() {
    if (m_curState->Update() != GAMESTATE_PLAY && m_curState->Update() != GAMESTATE_MULTI) {
        return 1;
    }
    DisplayResolution resolution;
    resolution = World()->GetDeviceManager()->FindPreviousResolution(
        m_modeSize.cx,
        m_modeSize.cy,
        m_colorDepth
    );
    i32 width = resolution.m_width;
    i32 height = resolution.m_height;
    if (width == -1 || height == -1 || width < SCREEN_HALF_W_PX || height < 0xc8) {
        return 1;
    }
    if (SetVideoMode(width, height, true)) {
        return 1;
    }
    if (SetVideoMode(SCREEN_W_PX, SCREEN_H_PX, true)) {
        return 1;
    }
    ReportError(IDX(IDS_SET_VIDEO_MODE), 0x43a);
    return 0;
}

RECT* CGruntzMgr::GetRect(RECT* out) {
    RECT local;
    SetRect(&local, 0, 0, 0x27f, 0x1df);
    if (!m_world) {
        *out = local;
        return out;
    }
    local = LevelOf(World())->m_viewportRect;
    *out = local;
    return out;
}

i32 CGruntzMgr::HandleDebugPosition() {
    i32 r = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        r = RunModalDialog("DEBUG_POSITION", WarpDialogProc, true);
        if (r == 1) {
            HWND hwnd = m_gameWnd->GetHwnd();
            PostMessageA(hwnd, WM_COMMAND, 0x805c, 0);
        }
    }
    return r != 0;
}

BOOL CALLBACK WarpDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szValue[64];

    switch (msg) {
        case WM_INITDIALOG: {

            CDDrawWorkerHost* warp = LevelOf(g_gameReg->World())->m_mainPlane;
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
                    g_gameReg->m_settings->setInt(szValue, valX);
                    sprintf(szValue, "Level %i Warp Y", g_gameReg->m_curState->m_levelIndex);
                    g_gameReg->m_settings->setInt(szValue, valY);
                    g_gameReg->m_settings->setInt(
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

void CGruntzMgr::OnCheckpointReached() {
    if (m_isCheckpointPrompts == false) {
        return;
    }
    CCheckpointDlg dlg(NULL);
    if (ExitModalUI(&dlg, false) == 1) {
        SendMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_QUICK_SAVE_PROMPT), 0);
    }
}

i32 CGruntzMgr::DebugJumpLevel() {
    i32 level = RunModalDialog("DEBUG_JUMPLEVEL", JumpLevelDialogProc, true);
    if (level > 0) {
        return PassClickToPlayState(level, false, 1);
    }
    return 0;
}

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

i32 CGruntzMgr::RegisterSetSkillDebugCmd() {
    if (m_curState->Update() == GAMESTATE_PLAY) {
        RunModalDialog("DEBUG_SETSKILL", SetSkillLevelDialogProc, true);
    }
    return 0;
}

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

i32 CGruntzMgr::FinishLevel(b32 pauseGame, b32 pauseMusic) {
    if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {

        i32 activePlayers = 0;
        CNetCmdSlot* slot = static_cast<CMulti*>(m_curState)->Session()->m_slots;
        for (i32 remainingSlots = 4; remainingSlots != 0; remainingSlots--) {
            if (slot != NULL && slot->m_state == NETSLOT_ACTIVE) {
                activePlayers++;
            }
            slot++;
        }
        if (activePlayers > 0) {
            m_frameGate = true;

            (static_cast<CMulti*>(m_curState))->RequestMultiplayerPause();
            m_frameGate = false;
            return 1;
        }
    }

    if (pauseGame) {
        if (m_worldSounds) {
            m_worldSounds->Stop();
        }
        if (m_world) {
            SoundCueRegistry* sub = World()->SoundRegistry();
            if (sub && sub->m_soundStream) {
                sub->m_soundStream->StopAllStreams();
            }
        }
        if (m_midi->IsCurrentPlaying() && pauseMusic) {
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

i32 CGruntzMgr::WarpCheat() {
    char key[64];
    sprintf(key, "Level %i Warp X", g_gameReg->m_curState->m_levelIndex);
    i32 wx = m_settings->getInt(key, -1);
    sprintf(key, "Level %i Warp Y", g_gameReg->m_curState->m_levelIndex);
    i32 wy = m_settings->getInt(key, -1);
    if (wx != -1 && wy != -1) {
        if (m_curState->Update() != GAMESTATE_PLAY) {
            i32 last = m_settings->getInt("Last Warp Level", -1);
            if (last != -1) {
                if (!PassClickToPlayState(last, false, 1, StateChangeOptions(0x43b).then(
                        static_cast<GruntzCommandId>(0x80ca)))) {
                    ReportError(IDX(IDS_SET_GAME_STATE), 0x43b);
                    return 0;
                }
                return 1;
            }
        } else {
            m_settings->setInt("Last Warp Level", m_curState->m_levelIndex);
            return 1;
        }
    }
    return 0;
}

i32 CGruntzMgr::CheckPlayState() {
    if (m_curState == NULL) {
        return 0;
    }
    if (m_curState->Update() == GAMESTATE_PLAY) {
        return 1;
    }
    return m_curState->Update() == GAMESTATE_MULTI;
}

i32 CGruntzMgr::InitializeLobbyConnectionSettings() {
    if (m_lobbyProbed) {
        return m_lobbyResult;
    }

    m_lobbyProbed = true;
    m_lobbyResult = 0;

    SAFE_RELEASE(m_lobby)

    i32 hr = DirectPlayLobbyCreate(NULL, &m_lobby, NULL, NULL, 0);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x120d, hr, m_gameWnd->GetHwnd());
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
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr, m_gameWnd->GetHwnd());
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
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1232, hr, m_gameWnd->GetHwnd());
        m_lobby->Release();
        m_lobby = NULL;
        return 0;
    }

    m_lobbyResult = 1;
    return m_lobbyResult;
}

i32 CGruntzMgr::ShowMessageBox(const char* text, u32 type) {
    if (m_world) {
        m_world->GetDrawTarget()->BlitPage(m_world->GetDrawTarget()->GetBackPair());

        CDDrawDeviceManager* deviceManager = m_world->GetDeviceManager();
        deviceManager->FlipToGDISurface();
    }
    i32 wasShown = ShowCursor(true);
    while (ShowCursor(true) < 0) {
    }
    i32 result = MessageBoxA(m_gameWnd->GetHwnd(), text, "Gruntz", type);
    if (wasShown <= 0) {
        while (ShowCursor(false) >= 0) {
        }
    }
    return result;
}

void CGruntzMgr::EnterModalUI(const char* msg) {
    CGameApp* app = m_owner;
    if (app == NULL) {
        return;
    }
    if (m_voiceManager) {
        m_voiceManager->PauseAllVoices();
    }
    if (m_world) {
        m_world->GetDrawTarget()->BlitPage(m_world->GetDrawTarget()->GetBackPair());

        CDDrawDeviceManager* deviceManager = m_world->GetDeviceManager();
        deviceManager->FlipToGDISurface();
    }

    int(WINAPI * show)(BOOL) = ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = true;
    static_cast<CGruntzApp*>(app)->ShowMessage(msg, m_gameWnd->GetHwnd());
    NetLobby::g_curDlg = NULL;
    m_modalBusy = false;
    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }
}

i32 CGruntzMgr::ToggleObjectLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = LevelOf(World());
        if (view) {
            u32 idx = static_cast<i32>(view->m_planes.size());
            if (idx == LEVEL_EXTENDED_PLANE_COUNT) {
                idx--;
            }
            CDDrawWorkerHost* layer = view->GetPlane(idx - 1);
            if (layer && !(layer->m_flags & IDX(WWD_PLANE_FLAG_MAIN))) {
                layer->m_flags ^= IDX(WWD_PLANE_FLAG_NO_DRAW);
                return 1;
            }
        }
    }
    return 0;
}

i32 CGruntzMgr::ToggleHeightLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = LevelOf(World());
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

i32 CGruntzMgr::ToggleBaseLayer() {
    if (IsActive() && m_world) {
        CGameLevel* view = LevelOf(World());
        if (view) {
            CDDrawWorkerHost* layer = view->GetPlane(0);
            if (layer && !(layer->m_flags & IDX(WWD_PLANE_FLAG_MAIN))) {
                layer->m_flags ^= IDX(WWD_PLANE_FLAG_NO_DRAW);
                return 1;
            }
        }
    }
    return 0;
}

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
        ExistProcess("IEXPLORE.EXE", 1, &quoted);
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

i32 CGruntzMgr::PollUnlessIdle() {
    if (m_curState->Update() != GAMESTATE_MENU) {
        CheckPlayState();
    }
    return 0;
}

i32 CGruntzMgr::RejectWorldFileCommand() {
    return 0;
}

i32 CGruntzMgr::CaptureWorldFile() {
    GameStateId st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY
        && st != GAMESTATE_DEMO) {
        return 0;
    }
    std::string name = RunCustomWorldDialog(m_gameWnd->GetHwnd());
    if ((name).empty()) {
        return 0;
    }
    m_strWorldFile = name;
    m_isBuiltInMultiplayerLevel = false;
    m_isBuiltInBattlezLevel = false;
    PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_NEW_GAME), 0);
    return 1;
}

i32 CGruntzMgr::ClearWorldFile() {
    GameStateId mode = m_curState->Update();
    if (mode == GAMESTATE_MENU || mode == GAMESTATE_ATTRACT || mode == GAMESTATE_PLAY) {
        (m_strWorldFile).erase();
        PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_NEW_GAME), 0);
        return 1;
    }
    return 0;
}

void CGruntzMgr::ResetClockGlobals() {
    g_resolutionChanged = false;
    g_traitorMode = false;
    g_gruntDestruction = false;
    g_gruntCreation = false;
    g_gooPuddlez = false;
    g_explosionz = false;
    g_debugDisplayFlags = DEBUG_DISPLAY_NONE;
}

bool CGruntzMgr::IsSceneFading() const {
    return m_curState && m_curState->IsSceneFading();
}

bool CGruntzMgr::IsQuitPending() const {
    return m_owner && m_owner->IsQuitPending();
}

void CGruntzMgr::DelayedQuit() {
    if (!m_owner || IsQuitPending()) return;
    CancelStateChange();
    u32 delayMs = 0;
    SoundCue* cue = World() && World()->SoundRegistry()
        ? World()->SoundRegistry()->FindCue("MENU_ACTIVATE") : NULL;
    if (cue && cue->m_sound) {
        const u32 duration = cue->m_sound->m_durationMs;
        delayMs = duration > 0x7fffffffU - 500 ? 0x7fffffffU : duration + 500;
    }
    m_owner->RequestQuit(delayMs);
}

void CGruntzMgr::RefreshGameClock() {
    if (m_curState && m_curState->Update() == GAMESTATE_MULTI) {
        return;
    }

    ResetFrameTiming();

    if (m_world) {
        g_soundCueTimeMs = timeGetTime();
        g_engineFrameDelta = 0;
    }

    g_lastNow = Timing().nowMs();
    g_frameDelta = Timing().deltaMs();
}

void CGruntzMgr::HandleAppActivation(b32 active, i32 unused) {
    if (IsActive() == 0) {
        return;
    }

    if (active) {
        RefreshGameClock();
        if (m_frameGate != false) {
            return;
        }
        if (m_musicEnabled == false) {
            return;
        }
        if (CheckPlayState() == 0
            && (m_curState == NULL || m_curState->Update() != GAMESTATE_CREDITS)) {
            return;
        }
        m_midi->ResumeCurrent(1);
        return;
    }

    if (m_musicEnabled == false) {
        return;
    }
    if (m_midi->IsCurrentPlaying() == false) {
        return;
    }
    m_midi->PauseCurrent();
}

void CGruntzMgr::StopAudioPlayback() {
    if (m_world) {
        SoundCueRegistry* soundRegistry = World()->SoundRegistry();
        if (soundRegistry) {
            SoundStream* soundStream = soundRegistry->m_soundStream;
            if (soundStream) {
                soundStream->StopAllStreams();
            }
        }
    }
    if (m_midi && m_midi->IsCurrentPlaying()) {
        m_midi->EndCurrent();
    }
}

void CGruntzMgr::SetGameClock(i32 now, i32 delta, i32 abs) {
    g_lastNow = now;
    g_frameDelta = delta;
    g_frameTime = abs;
    g_soundCueTimeMs = now;
    g_engineFrameDelta = delta;
}

void CGruntzMgr::RecomputeViewScale() {
    if (m_world == NULL) {
        return;
    }
    CGameLevel* view = LevelOf(World());
    LevelCoordRect ext = view->m_viewportRect;
    i32 iw = ext.right - ext.left + 1;
    i32 ih = ext.bottom - ext.top + 1;

    view->m_defaultActiveRegionSize.m_w = static_cast<i32>((static_cast<float>(iw) * 1.4f));
    view->m_defaultActiveRegionSize.m_h = static_cast<i32>((static_cast<float>(ih) * 1.4f));
    view->MainPlaneNotify();

    view = LevelOf(World());
    view->m_largeActiveRegionSize.m_w = static_cast<i32>((static_cast<float>(iw) * 5.3f));
    view->m_largeActiveRegionSize.m_h = static_cast<i32>((static_cast<float>(ih) * 5.3f));
    view->MainPlaneNotify();

    view = LevelOf(World());
    view->m_smallActiveRegionSize.m_w = static_cast<i32>((static_cast<float>(iw) * 1.12f));
    view->m_smallActiveRegionSize.m_h = static_cast<i32>((static_cast<float>(ih) * 1.12f));
    view->MainPlaneNotify();

    CGameLevel* v = LevelOf(World());
    if (v->m_mainPlane == NULL) {
        return;
    }
    SET_RECT_COMPONENTS(
        m_viewBounds,
        (LevelOf(World())->m_mainPlane)->m_planeViewRect.left - 0x60,
        (LevelOf(World())->m_mainPlane)->m_planeViewRect.top - 0x60,
        (LevelOf(World())->m_mainPlane)->m_planeViewRect.right + 0x60,
        (LevelOf(World())->m_mainPlane)->m_planeViewRect.bottom + 0x60
    );
}

i32 CGruntzMgr::IsStandardMode() {
    if (IS_STANDARD_VIDEO_MODE) {
        return 1;
    }
    return 0;
}

i32 CGruntzMgr::AppendChatMessage(const std::string& msg) {
    CFontConfig* log = m_chatLog;
    if (log == NULL) {
        return 0;
    }
    return log->AddItem(msg, FONT_ITEM_FLAGS_NONE, 0x11);
}

i32 CGruntzMgr::ShowToggleMessage(const std::string& itemName, i32 on) {
    return AppendChatMessage(itemName + (on ? " is ON" : " is OFF"));
}

i32 CGruntzMgr::IsInPlayState() {
    if (m_curState == NULL) {
        return 0;
    }
    return CheckPlayState() != 0;
}

char CGruntzMgr::GetGruntzDriveLetter() {
    if (m_driveLetterProbed) {
        return m_driveLetter;
    }
    m_driveLetter = ::GetGruntzDriveLetter();
    m_driveLetterProbed = true;
    return m_driveLetter;
}

i32 CGruntzMgr::PlayMovieEntry(i32 entryId) {
    if (entryId < IDX(MOVIE_ENTRY_FIRST) || entryId > IDX(MOVIE_ENTRY_LAST)) {
        return 0;
    }
    if (!FileExists((m_strMoviePath).c_str())) {
        return 0;
    }

    CMoviePlayer player;
    IDirectSound* dsound = NULL;

    CDDSurface* front = World()->GetDrawTarget()->GetFrontSurface()->GetSurface();
    IDirectDraw2* dd2 = World()->GetDeviceManager()->GetDirectDraw();

    if (World()->SoundRegistry()->HasWithPrefix("GAME") == 0) {
        CRezDir* snd = ResourceArchive()->GetDirFromPath("GAME_SOUNDZ");
        if (snd == NULL) {
            return 0;
        }
        World()->SoundRegistry()->LoadFromTree(static_cast<CRezDir*>(snd), "GAME", "_");
    }
    if (front == NULL || dd2 == NULL) {
        return 0;
    }

    if (World()->GetSoundStream() != NULL) {
        dsound = World()->GetSoundStream()->GetDirectSound();
    }
    if (player.InitMode(
            m_gameWnd->GetHwnd(),
            dd2,
            front->GetDirectDrawSurface(),
            front->GetDescription(),
            dsound
        )) {
        MovieOpenFlags openFlags =
            m_isInterlaced != false ? MOVIE_OPEN_INTERLACED : MOVIE_OPEN_DEFAULT;
        if (player.Open((m_strMoviePath).c_str(), IDX(entryId), MOVIE_TILE, openFlags, NULL, NULL)) {
            m_modalBusy = true;
            player.Pump(MOVIE_PUMP_SKIP_ON_KEY, 1);
            m_modalBusy = false;
        }
    }
    player.Teardown();
    return 1;
}

CFecFile::CFecFile() {
    m_openGate = false;
    m_readOpen = false;
    m_writeOpen = false;
    m_nextIndex = 0;
    srand(time(NULL));
}

std::string CGruntzMgr::BuildMoviePath(MovieId movie) {
    std::string name;

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

    if ((name).empty()) {
        return name;
    }

    std::string path;
    char szDir[GRUNTZ_PATH_BUFFER_SIZE];

    if (GetCurrentDirectoryA(GRUNTZ_PATH_BUFFER_MAX_CHARS, szDir)) {
        path = formatText("%s\\%s", szDir, (name).c_str());
        if (!FileExists((path).c_str())) {
            (path).erase();
        }
    }

    if ((path).empty()) {
        path = formatText("%c:\\Movies\\%s", GetGruntzDriveLetter(), (name).c_str());
        if ((path).empty()) {
            return path;
        }
    }

    if (!FileExists((path).c_str())) {
        (path).erase();
        return path;
    }

    return path;
}

i32 CGruntzMgr::IsMoviePathValid() {
    return FileExists((m_strMoviePath).c_str()) != 0;
}

i32 CGruntzMgr::PlayLogoMovie() {
    return PlayMovieEntry(IDX(MOVIE_ENTRY_LOGO));
}

void CGruntzMgr::Post(i32 code) {
    if (code > 0 && code <= IDX(QUESTLEVEL_POST_LAST)) {
        i32 v = (code == IDX(QUESTLEVEL_RESTART)) ? IDX(QUESTLEVEL_FIRST) : code;
        PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_LOAD_WORLD), v);
    }
}

i32 CGruntzMgr::RunModalDialog(const char* tmpl, DLGPROC dlgProc, b32 notify) {
    if (tmpl == NULL) {
        return 0;
    }
    if (dlgProc == NULL) {
        return 0;
    }
    if (m_voiceManager) {
        VoiceMgr()->PauseAllVoices();
    }
    if (m_triggerMgr && m_soundEnabled) {
        m_triggerMgr->DestroyAllAnims();
    }
    if (m_world) {
        if (notify && m_curState && m_curState->Update() != GAMESTATE_MENU) {
            m_curState->Present(0x32);
        } else {
            notify = false;
        }

        CDDrawDeviceManager* deviceManager = World()->GetDeviceManager();
        deviceManager->FlipToGDISurface();
    }

    int(WINAPI * show)(BOOL) = ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = true;
    i32 result =
        DialogBoxA(m_owner->m_hInstance, tmpl, m_gameWnd->GetHwnd(), static_cast<DLGPROC>(dlgProc));
    NetLobby::g_curDlg = NULL;
    m_modalBusy = false;
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

i32 CGruntzMgr::ExitModalUI(CDialog* dlg, b32 notify) {
    if (m_voiceManager) {
        VoiceMgr()->PauseAllVoices();
    }
    if (m_triggerMgr && m_soundEnabled) {
        m_triggerMgr->DestroyAllAnims();
    }
    if (m_world) {
        if (notify && m_curState && m_curState->Update() != GAMESTATE_MENU) {
            m_curState->Present(0x32);
        } else {
            notify = false;
        }

        CDDrawDeviceManager* deviceManager = World()->GetDeviceManager();
        deviceManager->FlipToGDISurface();
    }

    int(WINAPI * show)(BOOL) = ShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = true;
    i32 result = dlg->DoModal();
    NetLobby::g_curDlg = NULL;
    m_modalBusy = false;
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

std::string FindPortalExecutable() {
    Settings settings;
    if (!settings.load(settingsPath())) return std::string();
    const std::string path = settings.getString("Portal Executable");
    return !path.empty() && FileExists(path.c_str()) ? path : std::string();
}

i32 CGruntzMgr::LaunchPortal(i32 quitAfter) {
    const std::string path = FindPortalExecutable();
    if (path.empty() || !LaunchProcessInDir(path, "")) return 0;
    if (quitAfter) {
        DelayedQuit();
    }
    return 1;
}

i32 CGruntzMgr::LaunchProcessInDir(const std::string& app, const std::string& directory) {
    if (app.empty() || app.find('\0') != std::string::npos
        || directory.find('\0') != std::string::npos) return 0;
    std::string executable = directory;
    if (!executable.empty() && executable[executable.size() - 1] != '\\'
        && executable[executable.size() - 1] != '/') executable += '\\';
    executable += app;
    STARTUPINFOA startInfo;
    PROCESS_INFORMATION processInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    if (!CreateProcessA(executable.c_str(), NULL, NULL, NULL, false, 0, NULL,
        directory.empty() ? NULL : directory.c_str(), &startInfo, &processInfo)) return 0;
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return 1;
}

CState* CGruntzMgr::TopState() {
    if (static_cast<i32>(m_stateStack.size()) <= 0) {
        return NULL;
    }
    return static_cast<CState*>(m_stateStack[(static_cast<i32>(m_stateStack.size()) - 1)]);
}

void CGruntzMgr::PushState(CState* s) {
    if (!s) {
        return;
    }
    m_stateStack.push_back(s);
}

i32 CGruntzMgr::PopTopIfMatches(CState* s) {
    if (!s) {
        return 0;
    }
    i32 n = static_cast<i32>(m_stateStack.size());
    if (n <= 0) {
        return 0;
    }
    CState* top = static_cast<CState*>(m_stateStack[n - 1]);
    m_stateStack.erase(m_stateStack.begin() + n - 1, m_stateStack.begin() + (n - 1) + 1);
    return top == s;
}

void CGruntzMgr::ClearStateStack() {
    for (i32 i = 0; i < static_cast<i32>(m_stateStack.size()); i++) {
        CState* s = static_cast<CState*>(m_stateStack[i]);
        if (s) {
            delete s;
        }
    }
    m_stateStack.clear();
}

i32 CGruntzMgr::CheckMovieFileExists() {
    return FileExists((m_strMoviePath).c_str());
}

void CGruntzMgr::ReportWorldStatus(WorldInitReportTag tag) {
    if (m_world == NULL) {
        ReportError(IDX(IDS_INITIALIZE_GAME), IDX(tag));
    }
    GZ_ENUM_STORAGE(WorldInitError, u32) status = World()->m_lastError;
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
        rec = m_world->FindWorker("GAME_MONOLITH");
    }
    if (rec == NULL) {
        return 0;
    }
    i32 savedIdx = rec->GetMinIndex();
    CImage* e = DDRAW_WORKER_FRAME_AT_UNCHECKED(rec, savedIdx);
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
        growAndAssign(spr->m_imageSets, 0, (rec));
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
        g_monologoShown = true;
        return 1;
    }
    if (found->m_flags & 2) {
        found->m_flags &= ~2;
        g_monologoShown = true;
    } else {
        found->m_flags |= 2;
        g_monologoShown = false;
    }
    return 1;
}

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
    CDDrawWorker* out = World()->FindWorker("GAME_DEVHEADS");
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

i32 CGruntzMgr::SetGruntColor(CDDrawWorker* sink, const std::string& key, i32 idx) {
    if (sink) {
        CDDrawWorker* row = World()->FindWorker(key);
        if (row) {
            CImage* dst = DDRAW_WORKER_FRAME_AT_UNCHECKED(row, row->GetMinIndex());
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

void CGruntzMgr::CheatSkeletonToggle() {
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY && m_world) {

        CDDrawWorker* set;
        {
            set = World()->FindWorker("Gruntz");
        }
        if (set) {
            CImage* fr = DDRAW_WORKER_FRAME_AT_UNCHECKED(set, set->GetMinIndex());
            if (fr) {
                CDDrawShadeBlit* fmt = fr->m_owned;
                if (fmt) {
                    switch (fmt->m_drawType) {
                        case SHADE_DST_BY_SRC:
                            set->SetAllTypes(SHADE_COPY);
                            AppendChatMessage("Back from the dead?");
                            break;
                        default:
                            set->SetAllTypes(SHADE_DST_BY_SRC);
                            AppendChatMessage("You're scaring me...");
                            break;
                    }
                    PlayRegistryCueIfElapsed(World()->SoundRegistry(), "GAME_MINORCHEAT");
                }
            }
        }
    }
}

void CGruntzMgr::CheatEclipseToggle() {
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY && m_world) {

        CDDrawWorker* set;
        {
            set = World()->FindWorker("Gruntz");
        }
        if (set) {
            CImage* fr = DDRAW_WORKER_FRAME_AT_UNCHECKED(set, set->GetMinIndex());
            if (fr) {
                CDDrawShadeBlit* fmt = fr->m_owned;
                if (fmt) {
                    ShadeMode st = fmt->m_drawType;
                    if (st != SHADE_DST_BY_LEVEL) {
                        set->SetAllTypes(SHADE_DST_BY_LEVEL);
                        set->SetAllLightLevels(rand() % 256);
                        AppendChatMessage("Me and my...");
                    } else {
                        set->SetAllTypes(SHADE_COPY);
                        AppendChatMessage("Where did the sun go?");
                    }
                    PlayRegistryCueIfElapsed(World()->SoundRegistry(), "GAME_MINORCHEAT");
                }
            }
        }
    }
}

i32 CGruntzMgr::IsLobbyHostReady() {
    if (m_curState == NULL) {
        return 0;
    }
    CGameApp* app = m_owner;
    if (app == NULL) {
        return 0;
    }
    if (app->m_appActive == false) {
        return 0;
    }
    if (m_modalBusy != false) {
        return 0;
    }
    return m_curState->OnPaint() != 0;
}

void CGruntzMgr::OnMusicMuteBegin() {}

void CGruntzMgr::OnMusicMuteEnd() {}

void CGruntzMgr::OnMusicFadeStep(i32 value) {}

void CGruntzMgr::MuteMusicIfActive(i32 durationMs) {
    if (m_midi == NULL) {
        return;
    }
    if (m_musicEnabled == false) {
        return;
    }
    if (m_midi->IsCurrentPlaying() == false) {
        return;
    }
    m_midi->SetCurrentVolumePercent(0, durationMs);
}

void CGruntzMgr::RestoreMusicVolumeIfActive(i32 durationMs) {
    if (m_midi == NULL) {
        return;
    }
    if (m_musicEnabled == false) {
        return;
    }
    if (m_midi->IsCurrentPlaying() == false) {
        return;
    }
    m_midi->SetCurrentVolumePercent(kSoundVolumeMax, durationMs);
}

i32 CGruntzMgr::MakeRezPath() {
    char cwd[GRUNTZ_PATH_BUFFER_SIZE];
    if (!GetCurrentDirectoryA(GRUNTZ_PATH_BUFFER_MAX_CHARS, cwd)) {
        return 0;
    }

    char drive = GetGruntzDriveLetter();
    m_inGameDir = (drive == cwd[0]);

    b32 found = true;

    std::string rez("Gruntz.REZ");
    m_haveRez = false;
    m_strRezPath = formatText("%s\\%s", cwd, (rez).c_str());
    if (!FileExists((m_strRezPath).c_str())) {
        if (drive) {
            m_strRezPath = formatText(s_dataPath, drive, (rez).c_str());
            if (FileExists((m_strRezPath).c_str())) {
                m_haveRez = true;
            } else {
                found = false;
            }
        } else {
            found = false;
        }
    }

    i32 movFound = 1;
    std::string fecHi(s_fecName);
    std::string fecLo(s_fecLoName);
    std::string fec(g_enableHqMovie ? fecHi : fecLo);

    m_haveMoviez = false;
    m_strMoviePath = formatText("%s\\%s", cwd, (fecHi).c_str());
    if (!m_inGameDir && !FileExists((m_strMoviePath).c_str())) {
        movFound = 0;
        if (!g_enableHqMovie) {
            m_strMoviePath = formatText("%s\\%s", cwd, (fecLo).c_str());
            if (FileExists((m_strMoviePath).c_str())) {
                movFound = 1;
            }
        }
    }
    if (!movFound && drive) {
        m_strMoviePath = formatText(s_moviezPath, drive, (fec).c_str());
        if (FileExists((m_strMoviePath).c_str())) {
            m_haveMoviez = true;
        }
    }

    if (!found) {
        ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x43e);
        return 0;
    }
    return 1;
}

void CGruntzMgr::SetSoundVolume(i32 v) {
    m_soundVolume = v;
    if (m_world && World()->SoundRegistry()) {
        g_soundVolumePercent = v;
    }
    CWorldSoundSet* in = m_worldSounds;
    if (in) {
        in->SetMasterVolume(v);
    }
}

i32 CGruntzMgr::SetVoiceVolume(i32 v) {
    m_voiceVolume = v;
    CVoiceManager* timer = m_voiceManager;
    if (timer) {
        timer->SetVolume(v);
    }
    return v;
}

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

    SAFE_DELETE(m_worldSounds)

    CRezMgr* surf = m_resourceArchive;
    if (surf) {
        delete surf;
    }
    m_resourceArchive = NULL;

    m_colorDepth = mode;
    g_enableTrueColor = false;
    g_enableHiColor = false;
    if (m_colorDepth == BPP_RGB_16) {
        g_enableHiColor = true;
    }

    m_world->Cleanup();
    i32 kind = 1;
    if (g_disableAudio != false) {
        kind = 5;
    }
    if (m_world->Init(m_gameWnd->GetHwnd(), SCREEN_W_PX, SCREEN_H_PX, m_colorDepth, kind) == 0) {
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

    CRezMgr* old = m_resourceArchive;
    if (old) {
        delete old;
        m_resourceArchive = NULL;
    }

    m_resourceArchive = new CRezMgr;

    bool parseFailed = m_resourceArchive->Open(
                           (GetRezPath()).c_str(),
                           true,
                           false
                       )
                       == 0;
    if (parseFailed) {
        ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x441);
        return 0;
    }

    SetColorDepth(m_colorDepth);

    SAFE_DELETE(m_worldSounds)

    CWorldSoundSet* ni = new CWorldSoundSet();
    m_worldSounds = ni;
    if (ni->Init(m_world->m_soundRegistry, m_soundVolume) == 0) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x442);
        return 0;
    }

    m_worldSounds->SetEnabled(m_isAmbientEnabled);
    SetSoundVolume(m_soundVolume);
    return 1;
}

void CGruntzMgr::OnWorldModeLoaded(ColorDepth mode) {}

i32 CGruntzMgr::ResetWorldState() {
    if (IsStateTransitioning()) return 0;
    CState* st = m_curState;
    if (st == NULL) {
        return 1;
    }
    GameStateId stateId = st->Update();
    if (stateId != GAMESTATE_MENU && stateId != GAMESTATE_ATTRACT) {
        return 1;
    }

    CState* s = m_curState;
    m_modalBusy = true;
    m_renderGate = true;
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
    TransitionState(stateId, 1, false, 0);
    m_modalBusy = false;
    m_renderGate = false;
    return 1;
}

void CGruntzMgr::PauseMusicIfEnabled() {
    if (m_midi && m_musicEnabled) {
        m_midi->PauseCurrent();
    }
}

void CGruntzMgr::ResumeMusicIfEnabled() {
    if (m_midi && m_musicEnabled) {
        m_midi->ResumeCurrent(0);
    }
}

i32 CGruntzMgr::SetAssetRoot(const std::string& path) {
    CAssetRootStorage::s_value = path;
    PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_SHOW_STATE0), 0);
    return 1;
}

i32 CGruntzMgr::TickStateMgrs() {
    g_inputMgr->PollAll();
    g_gameplayInput->Update();
    return 1;
}

i32 CGruntzMgr::PostSlotCommandB1(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, 0x80b1, slot);
    return 1;
}

i32 CGruntzMgr::PostSlotCommandB6(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, 0x80b6, slot);
    return 1;
}

i32 CGruntzMgr::ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, ScanCb cb, i32 user) {
    if (cb == NULL) {
        return 0;
    }
    i32 r2 = SQR(radius);
    i32 count = 0;
    CDDrawChildGroup* children = World()->ChildGroup();
    std::list<CGameObject*>::iterator pos = children->m_list.begin();
    while (pos != children->m_list.end()) {
        CGameObject* obj = children->NextChild(pos);
        if (obj->m_objectType & mask) {
            i32 adx = abs(obj->m_screenX - x);
            i32 ady = abs(obj->m_screenY - y);
            if (SQR(adx) + ady + ady < r2) {
                count++;
                if (cb(obj, user) == 0) {
                    return count;
                }
            }
        }
    }
    return count;
}

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
    CDDrawChildGroup* children = World()->ChildGroup();
    std::list<CGameObject*>::iterator pos = children->m_list.begin();
    while (pos != children->m_list.end()) {
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

void CGruntzMgr::SetSoundEnabled(b32 enabled) {
    if (enabled == m_soundEnabled) {
        return;
    }
    m_soundEnabled = enabled;
    if (m_world == NULL) {
        return;
    }
    SoundStream* soundStream = World()->SoundRegistry()->m_soundStream;
    if (soundStream) {
        soundStream->StopAllStreams();
    }

    b32 soundEnabled = m_soundEnabled;
    g_soundEnabled = soundEnabled;
    if (m_soundEnabled) {
        m_worldSounds->Resume();
    } else {
        m_worldSounds->Stop();
    }
}

void CGruntzMgr::SetMusicEnabled(b32 enabled) {
    if (enabled == m_musicEnabled) {
        return;
    }
    m_musicEnabled = enabled;
    MidiManager* midi = m_midi;
    if (midi == NULL) {
        return;
    }
    if (enabled != false) {
        MidiSequence* sequence = midi->m_currentSequence;
        if (sequence == NULL) {
            return;
        }
        if (sequence->m_looping != false) {
            midi->RestartCurrent(true);
        } else if (midi->m_currentSequence != NULL) {

            midi->ResumeCurrent(1);
        }
        return;
    }
    midi->PauseCurrent();
}

i32 CGruntzMgr::LoadSaveMessageSprite() {
    if (CheatMgr()->m_cheatsUsed != false) {
        std::string name;
        loadResourceText(0x81aa, name);
        EnterModalUI((name).c_str());
    } else if (RunModalDialog("GAME_SAVE", SaveGameDialogProc, false) == 1) {
        RunModalDialog("GAME_SAVEMSG", OkCancelDialogProc, false);
    }
    return 1;
}

i32 CGruntzMgr::RunLoadGameDialog() {
    RunModalDialog("GAME_LOAD", GruntzLoadGameDlgProc, false);
    return 1;
}

i32 CGruntzMgr::Quicksave() {
    if (m_saveGame == NULL) {
        return 0;
    }
    if (m_curState->Update() != GAMESTATE_PLAY) {
        return 0;
    }
    if (CheatMgr()->m_cheatsUsed != false) {
        std::string name;
        loadResourceText(0x81aa, name);
        EnterModalUI((name).c_str());
        return 1;
    }
    if (m_saveInfoRec == NULL || !(m_saveInfoRec->m_flags & 1)) {
        return LoadSaveMessageSprite();
    }

    if (&(static_cast<CPlay*>(m_curState))->m_saveSlot == NULL) {
        return 0;
    }
    if (m_voiceManager) {
        VoiceMgr()->PauseAllVoices();
    }
    const SaveSlot previous = *m_saveInfoRec;
    if (!FillSaveInfo(m_saveInfoRec, NULL)) {
        *m_saveInfoRec = previous;
        EnterModalUI("ERROR - Cannot Save Game.");
        return 0;
    }

    if (m_saveGame->SaveSnapshot(m_saveInfoRec, 0x81a7) == 0) {
        *m_saveInfoRec = previous;
        EnterModalUI("ERROR - Cannot Save Game.");
        return 1;
    }
    ChatLog()->AddItem("Game Quicksaved successfully.", FONT_ITEM_FLAGS_NONE, 0x11);
    return 1;
}

i32 CGruntzMgr::Quickload() {
    if (m_saveGame == NULL) {
        return 0;
    }
    if (m_voiceManager) {
        VoiceMgr()->PauseAllVoices();
    }
    if (m_saveInfoRec && (m_saveInfoRec->m_flags & 1)) {

        if (m_saveGame->VerifySlot(m_saveInfoRec) == 0) {
            return 1;
        }
        PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_LOAD_SAVED_GAME), 0);
        ChatLog()->AddItem("Game Quickloaded successfully.", FONT_ITEM_FLAGS_NONE, 0x11);
        return 1;
    }
    return RunLoadGameDialog();
}

i32 CGruntzMgr::FillSaveInfo(SaveSlot* dst, const char* snapshot) {
    if (dst == NULL) {
        return 0;
    }
    CPlay* src = PickPlayOrPausedState();
    if (src == NULL) {
        return 0;
    }

    if (!copyTextToBuffer((GetWorldFileName()), dst->m_levelName, sizeof(dst->m_levelName))) return 0;
    dst->m_isBattlez = (m_gameMode == GAMEMODE_BATTLEZ);
    dst->m_isCustom = m_isCustomLevel;

    m_saveGame->CopySlot(dst, &src->m_saveSlot);
    m_saveInfoRec = dst;
    if (snapshot) {
        strncpy(static_cast<char*>(dst->m_snapshot), snapshot, 0x20);
    }
    return 1;
}

CState* CGruntzMgr::FindStateById(GameStateId id) {
    if (m_curState && m_curState->Update() == id) {
        return m_curState;
    }
    for (i32 i = 0; i < static_cast<i32>(m_stateStack.size()); i++) {
        CState* s = static_cast<CState*>(m_stateStack[i]);
        if (s && s->Update() == id) {
            return s;
        }
    }
    return NULL;
}

CPlay* CGruntzMgr::PickPlayOrPausedState() {
    return static_cast<CPlay*>(FindStateById(GAMESTATE_PLAY));
}

CState* CGruntzMgr::PickPausedThenPlayState() {
    CState* s = FindStateById(GAMESTATE_MULTI);
    if (s) {
        return s;
    }
    return FindStateById(GAMESTATE_PLAY);
}

i32 CGruntzMgr::RunDebugGruntTypeDialog() {
    i32 ran = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        ran = RunModalDialog("DEBUG_GRUNTTYPE", DebugGruntTypeDialogProc, true);
    }
    return ran != 0;
}

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

i32 CGruntzMgr::SetInactivePlayerName(i32 slot, i32, i32, i32, i32, const std::string& val, i32) {
    if (CheckPlayState()) {
        if (m_players[slot].m_active == false) {
            m_players[slot].m_name = val;
        }
    }
    return 0;
}

i32 CGruntzMgr::ResetPlayerSlot(i32 slot) {
    if (static_cast<u32>(slot) >= 4) {
        return 0;
    }
    GruntzPlayer* player = &m_players[slot];
    if (player == NULL) {
        return 0;
    }
    if (player->m_active == false) {
        return 0;
    }

    return player->Reset();
}

void CGruntzMgr::ResetAllPlayerSlots() {
    GruntzPlayer* player = &m_players[0];
    for (i32 remaining = 4; remaining != 0; remaining--) {
        if (player != NULL) {
            player->Reset();
        }
        player++;
    }
}

i32 CGruntzMgr::CountActivePlayers(b32 includeComputerPlayers) {
    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* slot = &m_players[i];
        if (slot && slot->m_active != false
            && (includeComputerPlayers != false || slot->m_humanControlled != false)) {
            count++;
        }
    }
    return count;
}

GruntzPlayer* CGruntzMgr::FindPlayerByNetworkId(i32 networkPlayerId) {

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* slot = &m_players[i];
        if (slot && slot->m_networkPlayerId == networkPlayerId) {
            return slot;
        }
    }
    return NULL;
}

void CGruntzMgr::DeactivateAllPlayers() {

    for (i32 i = 0; i < 4; i++) {
        GruntzPlayer* player = &m_players[i];
        if (player != NULL) {
            player->m_active = false;
            player->m_clearedRound = false;
        }
    }
}

i32 CGruntzMgr::OpenBattlezSetup() {
    CBattlezDlg dlg(this, NULL);
    GameStateId st = m_curState->Update();
    if (st != GAMESTATE_MENU && st != GAMESTATE_ATTRACT && st != GAMESTATE_PLAY
        && st != GAMESTATE_DEMO) {
        return 0;
    }
    ResetPlayerColorAvailability();
    if (ExitModalUI(&dlg, true) != 1) {
        return 0;
    }
    if (dlg.m_customNameFlag != false) {
        m_isBuiltInBattlezLevel = false;
        m_strWorldFile = "custom\\" + dlg.m_worldName;
    } else {
        m_isBuiltInBattlezLevel = true;
        m_strWorldFile = dlg.m_worldName;
    }
    if ((m_strWorldFile).empty()) {
        return 0;
    }
    PostMessageA(m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_START_BATTLEZ_GAME), 0);
    return 1;
}

i32 CGruntzMgr::InitializeBattlezPlayers() {
    i32 matched = 0;
    std::string s;
    if (loadResourceText(0x81ab, s)) {
        bool eq;
        eq = (s == m_strWorldFile);
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
            player->SetHumanControlled(true);
            difficulty = player->GetDifficulty();
            if (matched) {
                difficulty = BZDIFF_EASY;
            }
            if (!player->m_battlezConfig.LoadConfig(this, idx, difficulty)) {
                return 0;
            }
            player->m_battlezConfig.Clear();
            player++;
            idx++;
            player->SetHumanControlled(false);
            difficulty = player->GetDifficulty();
            if (matched) {
                difficulty = BZDIFF_EASY;
            }
            if (!player->m_battlezConfig.LoadConfig(this, idx, difficulty)) {
                return 0;
            }
        } else {
            player->SetHumanControlled(false);
            difficulty = player->GetDifficulty();
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

i32 CGruntzMgr::AdvanceComputerPlayerTurns() {
    i32 cursor = (g_battlezTurnPlayerIndex + 1) & 3;
    g_battlezTurnPlayerIndex = cursor;
    for (i32 i = 0; i < m_computerPlayerCount + 1; i++) {
        GruntzPlayer* slot = &m_players[i];
        if (cursor == i && slot->m_humanControlled == false && slot->m_active != false) {
            slot->m_battlezConfig.StepBoard();
            cursor = g_battlezTurnPlayerIndex;
        }
    }
    return 1;
}

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
            VoiceMgr()->ClearVoiceIndicatorSlots();
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
    if (!copyTextToBuffer((m_strWorldFile), buf, sizeof(buf))) return 0;
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

i32 CGruntzMgr::LoadState(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
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

i32 CGruntzMgr::IsBattlezMapFile(const std::string& path) {
    io::File file;
    char hdr[0x5f4];
    if (file.open((path).c_str(), io::ReadOnly)) {
        if (file.size() < 0x5f4) {
            file.finish();
            return 0;
        }
        if (file.read(hdr, sizeof(hdr)) != sizeof(hdr)) return 0;
        hdr[sizeof(hdr) - 1] = 0;
        file.finish();
        if (strstr(hdr + 0x10, "Battlez")) {
            return 1;
        }
    }
    return 0;
}
