#include <rva.h>

#include <Rez/RezSync.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Dsndmgr/MidiManager.h>
#include <Enums.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/Fonts.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GameText.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzDebugDialog.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/InputState.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/Resolution.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/SoundFxEmitter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/WorldSoundSet.h>
#include <Ints.h>
#include <Io/SaveGame.h>
#include <Net/NetMgr.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/GameApp.h>
#include <Wap32/ScreenGeometry.h>
#include <Wap32/Wap32.h>

#include <stdlib.h>
#include <string.h>
#include <strstrea.h>

GZ_ENUM_CONST_BEGIN(GruntzGameTiming)
    GRUNTZ_PERIODIC_TIMER_MS = 33
GZ_ENUM_CONST_END(GruntzGameTiming)

DATA(0x002455b4)
i32 g_disableAudio = 0;
DATA(0x002455bc)
i32 g_disableSound = 0;
DATA(0x002455c0)
i32 g_disableMusic = 0;
DATA(0x002455c4)
i32 g_disableFades = 0;
DATA(0x002455c8)
i32 g_disableJoystick = 0;
DATA(0x002455cc)
i32 g_disableSoundFonts = 0;
DATA(0x002455d0)
i32 g_disableDirectVideo = 0;
DATA(0x002455d4)
i32 g_disableHqMovie = 0;
DATA(0x002455d8)
i32 g_enableTriple = 0;
DATA(0x002455dc)
i32 g_enableHiColor = 0;
DATA(0x002455e0)
i32 g_enableTrueColor = 0;
DATA(0x002455e4)
i32 g_enableEmulation = 0;

// @early-stop
// Two residues remain. The frame is 8 B larger than retail's (`add esp,0x43c` vs
// `0x434`) - the cmd-line scratch buffers land at different offsets, so every
// [esp+N] operand in the parse block shifts. And cl cross-jumps the `xor eax,eax`
// that precedes the shared epilogue, so each early `return 0` reaches it with a
// bare `jmp` where retail materialises the zero at every site.
RVA(0x00083450, 0x192d)
i32 CGruntzMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {

    CoordPoolNode* pool = new CoordPoolNode[0x4e20];
    g_coordPool.m_block = pool;
    if (!pool) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x404);
        return 0;
    }
    g_coordPool.m_count = 0x4e20;
    CoordPoolNode* p = pool;
    u32 i = 0;
    do {
        p->m_next = p + 1;
        p = p->m_next;
        ++i;
    } while (i < static_cast<u32>(g_coordPool.m_count) - 1);
    p->m_next = NULL;
    g_coordPool.m_freeHead = pool;
    g_coordPool.m_linkOffset = 4;

    if (!CGameMgr::Run(pGameWnd, szCmdLine)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x462);
        return 0;
    }
    if (!InitializeFonts()) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x463);
        return 0;
    }
    srand((timeGetTime() + GetTickCount()) >> 1);
    g_gameAppTimerPeriodMs = GRUNTZ_PERIODIC_TIMER_MS;
    while (ShowCursor(0) >= 0) {
    }

    Utils::RegistryHelper* reg = new Utils::RegistryHelper;
    m_settings = reg;
    if (!m_settings
             ->Open("Monolith Productions", "Gruntz", "1.0", NULL, HKEY_LOCAL_MACHINE, NULL)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x406);
        return 0;
    }
    m_savedModeSize.cx = SCREEN_W_PX;
    m_savedModeSize.cy = SCREEN_H_PX;
    m_numRuns = m_settings->GetValueDword("Num Runs", 0);
    m_numMovies = m_settings->GetValueDword("Num Movies", 0);
    g_disableHqMovie = m_settings->GetValueDword("Disable High Quality Movie", 0) ? 1 : 0;
    g_disableAudio = m_settings->GetValueDword("Disable Audio", 0);
    g_disableSound = m_settings->GetValueDword("Disable Sound", 0);
    g_disableMusic = m_settings->GetValueDword("Disable Music", 0);
    g_disableFades = m_settings->GetValueDword("Disable Fades", 0);
    g_disableDirectVideo = m_settings->GetValueDword("Disable Direct Video Access", 0);
    g_disableJoystick = m_settings->GetValueDword("Disable Joystick", 0);
    g_disableSoundFonts = m_settings->GetValueDword("Disable SoundFonts", 0);
    g_enableTriple = m_settings->GetValueDword("Enable Triple", 0);
    g_enableHiColor = m_settings->GetValueDword("Enable HiColor", 0);
    g_enableTrueColor = m_settings->GetValueDword("Enable TrueColor", 0);
    g_enableEmulation = m_settings->GetValueDword("Enable Emulation", 0);
    m_isCheckpointPrompts = m_settings->GetValueDword("Checkpoint Prompts", 1);
    g_enableHiColor = 1;
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

    i32 vMusic = m_settings->GetValueDword("Music", m_musicEnabled);
    i32 vSound = m_settings->GetValueDword("Sound", m_soundEnabled);
    i32 vVoice = m_settings->GetValueDword("Voice", m_isVoiceEnabled);
    i32 vAmbient = m_settings->GetValueDword("Ambient", m_isAmbientEnabled);
    i32 vInterlaced = m_settings->GetValueDword("Interlaced", m_isInterlaced);
    i32 vHigh1 = m_settings->GetValueDword("High Detail", m_isHighDetail);
    i32 vHigh2 = m_settings->GetValueDword("High Detail", m_isEffectsEnabled);
    // Every probe above defaults from - and is written back to - its OWN member;
    // "Easy Mode" is the one retail applies immediately (0x0837e5
    // `mov [ebp+0x118],eax`, scheduled between the Resolution call's two pushes),
    // while the other seven are held in locals until after subsystem init.
    // "Resolution" is consumed only by the m_savedModeSize ladder below - retail
    // stores its result to no member at all.
    m_isEasyMode = m_settings->GetValueDword("Easy Mode", m_isEasyMode);
    i32 resolutionRaw = m_settings->GetValueDword("Resolution", IDX(RES_640X480));
    Resolution resolution = static_cast<Resolution>(resolutionRaw);
    if (resolution == RES_1024X768) {
        m_savedModeSize.cx = DISPLAY_WIDTH_1024;
        m_savedModeSize.cy = DISPLAY_HEIGHT_768;
    } else if (resolution == RES_800X600) {
        m_savedModeSize.cx = DISPLAY_WIDTH_800;
        m_savedModeSize.cy = DISPLAY_HEIGHT_600;
    } else {
        m_savedModeSize.cx = SCREEN_W_PX;
        m_savedModeSize.cy = SCREEN_H_PX;
    }
    i32 musicVolume = m_settings->GetValueDword("Music Volume", 0x64);
    i32 soundVolume = m_settings->GetValueDword("Sound Volume", 0x3c);
    i32 voiceVolume = m_settings->GetValueDword("Voice Volume", 0x50);
    i32 scrollSpeed = m_settings->GetValueDword("Scroll Speed", 0x14);
    m_soundVolume = soundVolume;
    m_voiceVolume = voiceVolume;

    m_scrollSpeed = scrollSpeed;
    m_numRuns = m_numRuns + 1;
    if (g_disableDirectVideo != 0) {
        g_disableFades = 1;
        g_enableEmulation = 1;
    }
    m_modalBusy = 0;
    m_renderGate = 0;
    m_driveLetterProbed = 0;
    m_driveLetter = 0;
    GetGruntzDriveLetter();

    GameStateId mode = GAMESTATE_ATTRACT;
    i32 noLogo = 0;
    char levelName[0x80];
    levelName[0] = 0;
    if (szCmdLine) {
        char buf[0x130];
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
            char cpy[0x11c];
            strcpy(cpy, buf);
            char* tok = strstr(cpy, "LOAD:");
            if (tok && strlen(tok) > 5) {
                tok += 5;
                i32 j = 0;
                char c = tok[0];
                while (c != ' ' && c != 0) {
                    c = tok[j + 1];
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
    char dpBuf[0x114];
    strcpy(dpBuf, szCmdLine);
    AfxWinInit(m_owner->m_hInstance, NULL, dpBuf, 1);
    m_strWorldFile.Empty();

    CDDrawSurfaceMgr* world = new CDDrawSurfaceMgr;
    m_world = static_cast<CDDrawSurfaceMgr*>(world);
    i32 flags = (g_disableAudio || g_disableSound) ? 0xe5 : 0xe1;
    if (g_enableEmulation) {
        flags |= 0x10;
    }
    m_colorDepth = BPP_RGB_16;
    if (!world->Init(m_gameWnd->m_hwnd, SCREEN_W_PX, SCREEN_H_PX, BPP_RGB_16, flags)) {
        ReportWorldStatus(WORLD_REPORT_STARTUP_INIT);
        return 0;
    }
    {
        LevelCoordRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = 0x1df;
        rect.bottom = 0x1df;
        m_modeSize.cx = SCREEN_W_PX;
        m_modeSize.cy = SCREEN_H_PX;
        world->m_level->UpdatePlaneViewports(&rect);
    }
    world->SetRestoreHandler(&PumpIdleFrame);
    world->m_level->m_maxStepX = 0xe;
    world->m_level->m_maxStepY = 0xe;
    world->m_drawTarget->CreateOverlay(0, 0x30000);
    RecomputeViewScale();
    RegisterGameObjectLogicTypes(world);
    if (!MakeRezPath()) {
        return 0;
    }

    if (m_resourceArchive) {
        m_resourceArchive->CRezArchive::~CRezArchive();
        ::operator delete(m_resourceArchive);
        m_resourceArchive = NULL;
    }
    m_resourceArchive = new CRezArchive;
    {
        CString resourcePath = GetRezPath();

        i32 parsed =
            m_resourceArchive->Open(const_cast<char*>(static_cast<const char*>(resourcePath)), 1, 0)
            != 0;
        if (!parsed) {
            ReportError(IDX(IDS_LOAD_RESOURCE_FILE), 0x409);
            return 0;
        }
    }
    if (!m_resourceArchive->MergeArchive(const_cast<char*>("GRUNTZ.VRZ"), 0)) {
        ReportError(IDX(IDS_LOAD_VOICE_RESOURCE_FILE), 0x460);
        return 0;
    }
    m_resourceArchive->MergeArchive(const_cast<char*>("GRUNTZ.ZZZ"), 1);
    m_resourceArchive->MergeArchive(const_cast<char*>("GRUNTZ.XXX"), 1);
    SetColorDepth(m_colorDepth);

    m_faderMgr = new CFaderMgr;
    if (!m_faderMgr->SetDefaults(NULL, NULL, NULL)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40a);
        return 0;
    }
    m_cheatMgr = new CCheatMgr;
    if (!m_cheatMgr->Init(m_gameWnd->m_hwnd)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40b);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableSoundFonts == 0) {
        if (SFManager_SelectBestDevice()) {
            if (!BuildSoundFontPath(GetGruntzDriveLetter())) {
                CloseSoundFontDevice();
            }
        }
    }

    m_midi = new MidiManager;
    g_ailMidiDriver = NULL;
    if (!m_midi->Initialize(m_owner->m_hInstance, m_gameWnd->m_hwnd, 0)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40c);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableMusic == 0) {
        m_midi->SetMasterVolume(musicVolume);
    } else {
        m_midi->m_midiAvailable = 0;
    }

    if (m_worldSounds) {
        delete m_worldSounds;
        m_worldSounds = NULL;
    }
    m_worldSounds = new CWorldSoundSet;
    if (!m_worldSounds->Init(world->m_soundRegistry, soundVolume)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40d);
        return 0;
    }
    {
        i32 f = m_worldSounds->m_enabled;
        if (vMusic != 0) {
            if (f == 0) {
                m_worldSounds->m_enabled = 1;
                m_worldSounds->Resume();
            }
        } else if (f != 0) {
            m_worldSounds->m_enabled = 0;
            m_worldSounds->Stop();
        }
    }
    SetSoundVolume(soundVolume);

    SetVoiceVolume(voiceVolume);
    m_scrollSpeed = scrollSpeed;

    g_inputMgr = new DirectInputMgr2;
    if (!g_inputMgr->Create(
            m_gameWnd->m_hwnd,
            m_owner->m_hInstance,
            IDX(DIN_CREATE_ASYNC_KEYBOARD | DIN_CREATE_NO_MOUSE | DIN_CREATE_NO_JOYSTICKS)
        )) {
        if (g_inputMgr) {
            delete g_inputMgr;
            g_inputMgr = NULL;
        }
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x40e);
        return 0;
    }

    i32 joystickCount = g_inputMgr->m_joysticks.GetSize();
    g_actorList = g_inputMgr->CreateDeviceGroup(
        joystickCount > 0 ? static_cast<CInputDevBase*>(g_inputMgr->m_joysticks[0]) : NULL,
        joystickCount > 1 ? static_cast<CInputDevBase*>(g_inputMgr->m_joysticks[1]) : NULL,
        joystickCount > 2 ? static_cast<CInputDevBase*>(g_inputMgr->m_joysticks[2]) : NULL,
        joystickCount > 3 ? static_cast<CInputDevBase*>(g_inputMgr->m_joysticks[3]) : NULL,
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
        delete m_shadeCache;
        m_shadeCache = NULL;
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x410);
        return 0;
    }

    m_lightFxMgr = new CLightFxMgr;
    if (!m_lightFxMgr->Init(this, NULL)) {
        if (m_lightFxMgr) {
            m_lightFxMgr->Reset();
            ::operator delete(m_lightFxMgr);
            m_lightFxMgr = NULL;
        }
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x411);
        return 0;
    }
    m_saveGame = new CSaveGame;
    if (!m_saveGame->InitializeSaveDirectory("")) {
        if (m_saveGame) {
            delete m_saveGame;
            m_saveGame = NULL;
        }
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
        // The zeroing runs off a cached pointer and skips m_mouse - retail's own
        // hand-written teardown, not the constructor's six stores replayed.
        CInputState* dead = g_gameplayInput;
        if (dead) {
            dead->m_primaryDevice = NULL;
            dead->m_keyboard = NULL;
            dead->m_joystick = NULL;
            dead->m_deviceGroup = NULL;
            dead->m_deviceSelection = INPUTDEV_NONE;
            ::operator delete(dead);
            g_gameplayInput = NULL;
        }
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x413);
        return 0;
    }

    m_commandMgr = new CGruntzCmdMgr;

    if (!m_commandMgr->SetManager(this)) {
        if (m_commandMgr) {
            delete m_commandMgr;
            m_commandMgr = NULL;
        }
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
        if (m_spriteFactory) {
            m_spriteFactory->Reset();
            ::operator delete(m_spriteFactory);
            m_spriteFactory = NULL;
        }

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
        CRezArchiveEntry* stream =
            g_gameReg->m_resourceArchive->FindEntryByPath("GAME_ATTRIBUTEZ", REZ_TAG_TXT);
        TRACE("%s\n", static_cast<LPCTSTR>(CString("parsing ") + "GAME_ATTRIBUTEZ"));
        g_buteMgr.SetErrCallback(&ButeParseErrorSink);
        bool ok = false;
        if (stream) {
            g_buteMgr.m_encrypted = 1;
            char* esz = stream->LoadData();
            i32 eszLen = stream->m_size;
            istrstream* rdr = new istrstream(esz, eszLen);
            g_buteMgr.m_crypt.InitKey("1212C");
            char* decoded = new char[eszLen];
            ostrstream* snk = new ostrstream(decoded, eszLen, 2);
            g_buteMgr.m_crypt.Decode(rdr, snk);

            g_buteMgr.m_stream = new istrstream(decoded, snk->rdbuf()->out_waiting());
            delete rdr;
            delete snk;
            stream->ReleaseData();
            g_buteMgr.Init();
            g_buteMgr.m_tags.ClearRecursive(NULL);
            g_buteMgr.m_tags.m_root = NULL;
            g_buteMgr.m_tags.m_lookupPending = 0;
            g_buteMgr.m_tags.m_nodeCount = 0;
            g_buteMgr.m_modifiedTags.ClearRecursive(NULL);
            g_buteMgr.m_modifiedTags.m_root = NULL;
            g_buteMgr.m_modifiedTags.m_lookupPending = 0;
            g_buteMgr.m_modifiedTags.m_nodeCount = 0;
            g_buteMgr.m_addedTags.ClearRecursive(NULL);
            g_buteMgr.m_addedTags.m_root = NULL;
            g_buteMgr.m_addedTags.m_lookupPending = 0;
            g_buteMgr.m_addedTags.m_nodeCount = 0;
            ok = true;
            if (!g_buteMgr.ParseGroup()) {
                g_buteMgr.m_parseFailed = 1;
                ok = false;
            }
            delete g_buteMgr.m_stream;
            delete[] decoded;
        }
        if (!ok) {
            ReportError(IDX(IDS_INITIALIZE_GAME), 0x418);
            return 0;
        }
    }

    m_cheatMgr->RegisterCheats();
    m_chatLog = new CFontConfig;
    if (!m_chatLog->LoadFontConfig(0x1388, 0xbb8)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x41a);
        return 0;
    }
    m_triggerMgr = new CTriggerMgr;
    if (!m_triggerMgr->SetLevel(m_world)) {
        if (m_triggerMgr) {
            delete m_triggerMgr;
            m_triggerMgr = NULL;
        }
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x41b);
        return 0;
    }
    g_localVersion = static_cast<i32>(
        g_buteMgr.GetDwordDef("General", "RezSync", static_cast<u32>(g_localVersion))
    );
    m_voiceManager = new CVoiceManager;
    if (!m_voiceManager->Init(this)) {
        ReportError(IDX(IDS_INITIALIZE_GAME), 0x45f);
        return 0;
    }
    m_voiceManager->m_voiceVolume = voiceVolume;
    m_musicEnabled = vMusic;
    m_soundEnabled = vSound;
    g_soundEnabled = vSound;
    m_isVoiceEnabled = vVoice;
    m_isAmbientEnabled = vAmbient;
    m_isInterlaced = vInterlaced;
    m_isHighDetail = vHigh1;
    m_isEffectsEnabled = vHigh2;
    if (!m_world->m_soundRegistry->HasWithPrefix("GAME")) {
        CRezArchiveDir* sz = m_resourceArchive->FindDirectoryByPath("GAME_SOUNDZ");
        if (!sz) {
            return 0;
        }
        m_world->m_soundRegistry->LoadFromTree(static_cast<CRezArchiveDir*>(sz), "GAME", "_");
    }
    {
        SoundCue* movieCue = NULL;
        MapLookup(m_world->m_soundRegistry->m_cues, "GAME_MOVIE", movieCue);
        m_world->m_soundRegistry->ConfigurePrimaryFromCue(movieCue, 0);
    }
    CheckMovieFileExists();
    if (!InitializeLobbyConnectionSettings()) {
        if (m_numMovies > 0 && m_numRuns > 1) {
            if (m_settings->GetValueDword("Skip Logo Movies", 0) == 0 && noLogo == 0) {
                PlayLogoMovie();
            }
        } else {
            PlayLogoMovie();
            if (PlayMovieEntry(2)) {
                ++m_numMovies;
            }
        }
    }

    {

        CRezArchiveDir* attract = m_resourceArchive->FindDirectoryByPath("STATEZ_ATTRACT");
        g_attractStateCount = 0;
        CString title;
        title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        while (attract->FindEntryByPath(static_cast<const char*>(title), IMGTAG_XCP)) {
            g_attractStateCount++;
            title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        }
        if (TransitionState(mode, 1, 0, 0)) {
            g_frameDelta = 0;
        } else if (mode == GAMESTATE_MULTI) {
            if (TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
                g_frameDelta = 0;
            } else {
                ReportError(IDX(IDS_SET_GAME_STATE), 0x41c);
                return 0;
            }
        } else {
            ReportError(IDX(IDS_SET_GAME_STATE), 0x41d);
            return 0;
        }
    }
    return 1;
}

RVA(0x00085500, 0x23)
CString CGruntzMgr::GetRezPath() {
    return m_strRezPath;
}
