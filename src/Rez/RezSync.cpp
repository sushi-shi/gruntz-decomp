#include <rva.h>

#include <Rez/RezSync.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Dsndmgr/GruntzSoundZ.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/Fonts.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/Resolution.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/SoundFxEmitter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StateMgrBZ.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/WorldSoundSet.h>
#include <Ints.h>
#include <Io/SaveGame.h>
#include <Net/NetMgr.h>
#include <Rez/FrameClock.h>
#include <Rez/RezTypeTag.h>
#include <strstrea.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/GameApp.h>
#include <Wap32/Wap32.h>

#include <stdlib.h>
#include <string.h>

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

DATA(0x00245210)
HINSTANCE g_appHInstance;

// @early-stop
RVA(0x00083450, 0x192d)
i32 CGruntzMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {

    CoordPoolNode* pool = static_cast<CoordPoolNode*>(::operator new(0x3a980));
    g_coordPool.m_block = pool;
    if (!pool) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x404);
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
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x462);
        return 0;
    }
    if (!InitializeFonts()) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x463);
        return 0;
    }
    srand((timeGetTime() + GetTickCount()) >> 1);
    g_wap32Run80 = 0x21;
    while (ShowCursor(0) >= 0) {
    }

    Utils::RegistryHelper* reg = static_cast<Utils::RegistryHelper*>(::operator new(0x21c));
    if (reg) {
        reg->m_open = 0;
    }
    m_settings = reg;
    if (!m_settings->Open("Monolith Productions", "Gruntz", "1.0", 0, HKEY_LOCAL_MACHINE, 0)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x406);
        return 0;
    }
    m_savedModeW = 0x280;
    m_savedModeH = 0x1e0;
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
    g_dlgVal_64526c = 0;
    g_dlgVal_6452d0 = 0;
    g_dlgVal_645268 = 0;
    g_dlgVal_645568 = 0;
    g_dlgVal_645538 = 0;
    g_dlgVal_6451a4 = 0;
    g_dlgVal_6452d4 = 1;
    g_dlgVal_6452a8 = 0;
    g_dlgVal_645558 = 0;
    g_dlgVal_645560 = 0;
    g_dlgVal_64555c = 0;
    g_dlgVal_645564 = 0;

    i32 vMusic = m_settings->GetValueDword("Music", m_musicEnabled);
    i32 vSound = m_settings->GetValueDword("Sound", m_soundEnabled);
    i32 vVoice = m_settings->GetValueDword("Voice", m_isVoiceEnabled);
    i32 vAmbient = m_settings->GetValueDword("Ambient", m_isAmbientEnabled);
    i32 vInterlaced = m_settings->GetValueDword("Interlaced", m_isInterlaced);
    i32 vHigh1 = m_settings->GetValueDword("High Detail", m_isHighDetail);
    m_isHighDetail = m_settings->GetValueDword("High Detail", m_isEffectsEnabled);
    i32 vEasy = m_settings->GetValueDword("Easy Mode", m_isEasyMode);
    i32 res = m_settings->GetValueDword("Resolution", 1);
    m_isEasyMode = res;
    if (res == 3) {
        m_savedModeW = 0x400;
        m_savedModeH = 0x300;
    } else if (res == 2) {
        m_savedModeW = 0x320;
        m_savedModeH = 0x258;
    } else {
        m_savedModeW = 0x280;
        m_savedModeH = 0x1e0;
    }
    i32 vMusVol = m_settings->GetValueDword("Music Volume", 0x64);
    i32 vSndVol = m_settings->GetValueDword("Sound Volume", 0x3c);
    i32 vVoiVol = m_settings->GetValueDword("Voice Volume", 0x50);
    i32 vScroll = m_settings->GetValueDword("Scroll Speed", 0x14);
    m_soundVolume = vSndVol;
    m_voiceVolume = vVoiVol;

    m_scrollSpeed = vScroll;
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

    i32 mode = 2;
    i32 noLogo = 0;
    char levelName[0x80];
    levelName[0] = 0;
    if (szCmdLine) {
        char buf[0x130];
        strcpy(buf, szCmdLine);
        _strupr(buf);
        if (strstr(buf, "PLAY")) {
            mode = 3;
        }
        if (strstr(buf, "MULTI")) {
            mode = 0x11;
        }
        if (strstr(buf, "DEMO")) {
            mode = 7;
        }
        if (strstr(buf, "SELECT")) {
            mode = 0x10;
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
        mode = 0x11;
        m_reservedb4 = 0;
    }

    g_appHInstance = m_owner->m_hInstance;
    char dpBuf[0x114];
    strcpy(dpBuf, szCmdLine);
    AfxWinInit(m_owner->m_hInstance, 0, dpBuf, 1);
    m_strWorldFile.Empty();

    CDDrawSurfaceMgr* world = new CDDrawSurfaceMgr;
    m_world = static_cast<CDDrawSurfaceMgr*>(world);
    i32 flags = (g_disableAudio || g_disableSound) ? 0xe5 : 0xe1;
    if (g_enableEmulation) {
        flags |= 0x10;
    }
    m_colorDepth = 0x10;
    if (!world->Init(m_gameWnd->m_hwnd, 0x280, 0x1e0, 0x10, flags)) {
        ReportWorldStatus(0x407);
        return 0;
    }
    {
        LevelCoordRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = 0x1df;
        rect.bottom = 0x1df;
        m_modeW = 0x280;
        m_modeH = 0x1e0;
        world->m_level->BuildAllPlanes(&rect);
    }
    world->SetRestoreHandler(&PumpIdleFrame);
    world->m_level->m_maxStepX = 0xe;
    world->m_level->m_maxStepY = 0xe;
    world->m_drawTarget->CreateOverlay(0, 0x30000);
    RecomputeViewScale();
    RegisterGameObjectTypes(world);
    if (!MakeRezPath()) {
        return 0;
    }

    if (m_symParser) {
        m_symParser->CSymParser::~CSymParser();
        ::operator delete(m_symParser);
        m_symParser = NULL;
    }
    m_symParser = new CSymParser;
    {
        CString fn = GetRezPath();

        i32 ok =
            m_symParser->ParseBuffer(const_cast<char*>(static_cast<const char*>(fn)), 1, 0) != 0;
        if (!ok) {
            ReportError(static_cast<GruntzCommandId>(0x800b), 0x409);
            return 0;
        }
    }
    if (!m_symParser->LoadEntry(const_cast<char*>("GRUNTZ.VRZ"), 0)) {
        ReportError(static_cast<GruntzCommandId>(0x8149), 0x460);
        return 0;
    }
    m_symParser->LoadEntry(const_cast<char*>("GRUNTZ.ZZZ"), 1);
    m_symParser->LoadEntry(const_cast<char*>("GRUNTZ.XXX"), 1);
    SetColorDepth(m_colorDepth);

    m_faderMgr = new CFaderMgr;
    if (!m_faderMgr->SetConfig(0, 0, 0)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x40a);
        return 0;
    }
    m_cheatMgr = new CCheatMgr;
    if (!m_cheatMgr->Init(m_gameWnd->m_hwnd)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x40b);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableSoundFonts == 0) {
        if (SFManager_SelectBestDevice()) {
            if (!BuildSoundFontPath(GetGruntzDriveLetter())) {
                CloseSoundFontDevice();
            }
        }
    }

    m_sound = new CGruntzSoundZ;
    g_ailMidiDriver = NULL;
    if (!m_sound->Init(m_owner->m_hInstance, m_gameWnd->m_hwnd, 0)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x40c);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableMusic == 0) {
        m_sound->SetXMidiVolume(vMusic);
    } else {
        m_sound->m_enabled = 0;
    }

    if (m_inputState) {
        m_inputState->Teardown();

        (&m_inputState->m_list)->CPtrList::~CPtrList();
        ::operator delete(m_inputState);
        m_inputState = NULL;
    }
    m_inputState = new CWorldSoundSet;
    if (!m_inputState->Init(world->m_soundRegistry, vSndVol)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x40d);
        return 0;
    }
    {
        i32 f = m_inputState->m_active;
        if (vMusVol != 0) {
            if (f == 0) {
                m_inputState->m_active = 1;
                m_inputState->Resume();
            }
        } else if (f != 0) {
            m_inputState->m_active = 0;
            m_inputState->Stop();
        }
    }
    SetSoundVolume(vSndVol);

    SetVoiceVolume(vVoiVol);
    m_scrollSpeed = vScroll;

    m_logicPump = static_cast<CLightFxMgr*>(::operator new(0x3c));
    if (m_logicPump) {
        m_logicPump->m_reg = NULL;
        m_logicPump->m_world = NULL;
        m_logicPump->m_cache = NULL;
        m_logicPump->m_greyTable = NULL;
        for (i32 k = 0; k < 10; ++k) {
            m_logicPump->m_tables[k] = NULL;
        }
    }
    if (!m_logicPump->Init(0, this)) {
        if (m_logicPump) {
            m_logicPump->Reset();
            ::operator delete(m_logicPump);
            m_logicPump = NULL;
        }
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x411);
        return 0;
    }
    m_saveSink = new CSaveGame;
    if (!m_saveSink->SaveGameFile(g_emptyString)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x412);
        return 0;
    }
    m_scoreHud = new CBattlezData;
    m_scoreHud->InitWithRecords(m_saveSink->m_pad24);

    g_spawnConfig = static_cast<StateMgrBZ*>(::operator new(sizeof(StateMgrBZ)));
    if (g_spawnConfig) {
        g_spawnConfig->m_device = NULL;
        g_spawnConfig->m_keyboard = NULL;
        g_spawnConfig->m_joystick = NULL;
        g_spawnConfig->m_mouse = NULL;
        g_spawnConfig->m_deviceList = NULL;
        g_spawnConfig->m_mode = INPUTDEV_NONE;
    }
    if (!g_spawnConfig->Init(g_inputMgr, INPUTDEV_KEYBOARD_JOYSTICK1)) {
        if (g_spawnConfig) {
            g_spawnConfig->m_device = NULL;
            g_spawnConfig->m_keyboard = NULL;
            g_spawnConfig->m_joystick = NULL;
            g_spawnConfig->m_mouse = NULL;
            g_spawnConfig->m_deviceList = NULL;
            g_spawnConfig->m_mode = INPUTDEV_NONE;
            ::operator delete(g_spawnConfig);
            g_spawnConfig = NULL;
        }
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x413);
        return 0;
    }

    m_cmdSubMgr = new CGruntzCmdMgr;

    if (!m_cmdSubMgr->SetMgr(this)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x414);
        return 0;
    }
    m_tileGrid = new CGruntzMapMgr;
    if (!m_tileGrid) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x415);
        return 0;
    }
    m_spriteFactory = static_cast<CSpriteRefTable*>(::operator new(0x94));
    if (m_spriteFactory) {
        m_spriteFactory->m_factory = NULL;
        m_spriteFactory->m_spriteMgrHolder = NULL;
        m_spriteFactory->m_built = 0;
        for (i32 k = 0; k < 0x11; ++k) {
            m_spriteFactory->m_toolRefs[k] = NULL;
            m_spriteFactory->m_toyRefs[k] = NULL;
        }
    }

    if (!m_spriteFactory->Init(m_shadeCache, m_world)) {
        if (m_spriteFactory) {
            m_spriteFactory->Reset();
            ::operator delete(m_spriteFactory);
            m_spriteFactory = NULL;
        }

        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x416);
    }

    g_gameReg = this;
    g_lastNow = timeGetTime();
    g_frameDelta = 0;
    for (i32 s = 0; s < 4; ++s) {

        if (!m_options[s].SeedForSlot(s)) {
            ReportError(IDX(CMD_TOGGLE_MUSIC), 0x417);
            return 0;
        }
    }

    {
        CSymParser* mgr = m_symParser;
        CParseSource* stream =
            mgr->ResolveQualified("GAME_ATTRIBUTEZ", static_cast<RezTypeTag>('TXT'));
        g_buteMgr.SetErrCallback(&ButeParseErrorSink);
        i32 ok = 0;
        if (stream) {
            g_buteMgr.m_encrypted = 1;
            char* esz = stream->BeginParse();
            // This entry kind stores bytes and length in the opposite word/address arms.
            AddrWord<char> lenSlot;
            AddrWord<char> dataSlot;
            lenSlot.m_addr = esz;
            dataSlot.m_word = stream->m_length;
            i32 eszLen = lenSlot.m_word;
            char* src = dataSlot.m_addr;
            // Preserve the swapped raw-width representation.
            istrstream* rdr = new istrstream(src, eszLen);
            Blowfish_InitKey("1212C");
            ostrstream* snk = new ostrstream(src, eszLen, 2);
            g_buteMgr.m_crypt.Decode(rdr, snk);

            g_buteMgr.m_stream = static_cast<istream*>(::operator new(0x60));
            stream->EndParse();
            g_buteMgr.Init();
            g_buteMgr.m_tree.ClearRecursive(0);
            g_buteMgr.m_tree.m_root = NULL;
            g_buteMgr.m_tree.m_lookupPending = 0;
            g_buteMgr.m_tree.m_nodeCount = 0;
            g_buteMgr.m_tree48.ClearRecursive(0);
            g_buteMgr.m_tree48.m_root = NULL;
            g_buteMgr.m_tree48.m_lookupPending = 0;
            g_buteMgr.m_tree48.m_nodeCount = 0;
            g_buteMgr.m_tree74.ClearRecursive(0);
            g_buteMgr.m_tree74.m_root = NULL;
            g_buteMgr.m_tree74.m_lookupPending = 0;
            g_buteMgr.m_tree74.m_nodeCount = 0;
            ok = 1;
            if (!g_buteMgr.ParseGroup()) {
                g_buteMgr.m_parseFailed = 1;
                ok = 0;
            }
            ::operator delete(rdr);
        }
        if (!ok) {
            ReportError(IDX(CMD_TOGGLE_MUSIC), 0x418);
            return 0;
        }
    }

    m_cheatMgr->RegisterCheats();
    m_chatLog = new CFontConfig;
    m_chatLog->LoadFontConfig(0x1388, 0xbb8);
    m_cmdGrid = new CTriggerMgr;
    if (!m_cmdGrid->SetLevel(m_world)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x41b);
        return 0;
    }
    g_localVersion = static_cast<i32>(
        g_buteMgr.GetDwordDef("General", "RezSync", static_cast<u32>(g_localVersion))
    );
    m_cueSink = new CGruntSpawnConfig;
    if (!m_cueSink->Init(this)) {
        ReportError(IDX(CMD_TOGGLE_MUSIC), 0x45f);
        return 0;
    }
    m_cueSink->m_voiceVolume = vScroll;
    m_musicEnabled = vMusic;
    m_soundEnabled = vSound;
    g_sndEnabled = vSound;
    m_isVoiceEnabled = vVoice;
    m_isAmbientEnabled = vAmbient;
    m_isInterlaced = vInterlaced;
    m_isHighDetail = vHigh1;
    m_isEffectsEnabled = vEasy;
    if (!m_world->m_soundRegistry->HasKeyEqual("GAME")) {
        void* sz = m_symParser->ResolvePath("GAME_SOUNDZ");
        if (!sz) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(sz), "GAME", "_");
    }
    {
        void* mv = 0;
        m_world->m_soundRegistry->m_cues.Lookup("GAME_MOVIE", mv);
        m_world->m_soundRegistry->MatchSub(static_cast<LeafCue*>(mv), 0);
    }
    CheckMovieFileExists();
    if (!InitializeLobbyConnectionSettings()) {
        if (m_numMovies > 0 && m_numRuns > 1) {
            if (m_settings->GetValueDword("Skip Logo Movies", 0) == 0 && noLogo == 0) {
                RunFromState();
            }
        } else {
            RunFromState();
            if (ChangeState(2)) {
                ++m_numMovies;
            }
        }
    }

    {

        CSymTab* attract = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_ATTRACT"));
        CString title;
        g_attractStateCount = 0;
        title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        while (attract->ResolveQualified(static_cast<const char*>(title), IMGTAG_XCP)) {
            g_attractStateCount++;
            title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        }
        if (TransitionState(static_cast<GameStateId>(mode), 1, 0, 0)) {
            g_frameDelta = 0;
        } else if (mode == 0x11 && TransitionState(GAMESTATE_ATTRACT, 1, 0, 0)) {
            g_frameDelta = 0;
        } else {
            ReportError(IDX(CMD_NEW_GAME), mode == 0x11 ? 0x41c : 0x41d);
            return 0;
        }
    }
    return 1;
}

RVA(0x000853d0, 0x10)
void __stdcall RezFreeStdcall(void* a) {
    ::operator delete(a);
}

RVA(0x00085500, 0x23)
CString CGruntzMgr::GetRezPath() {
    return m_strRezPath;
}
