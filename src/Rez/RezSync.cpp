#include <Rez/RezSync.h> // own extern surface
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (ex .cpp extern)
#include <Net/NetMgr.h>           // g_localVersion (ex .cpp extern; def in Multi.cpp)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr (m_30; ex the local view)
#include <Utils/RegistryHelper.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Rez/FrameClock.h> // g_lastNow (the frame-clock now cell Init seeds)
#include <rva.h>
#include <Ints.h>
#include <Mfc.h> // CString + the MFC collection ctors/dtors (reloc-masked)
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <Wap32/Wap32.h>              // CGameMgr (the base ~CGameMgr tail-calls its Close)
#include <Bute/ButeMgr.h>             // canonical CButeMgr / CButeStore (one shape)
#include <Bute/SymParser.h>           // the ONE CSymParser (m_symParser; ParseBuffer/...)
#include <Gruntz/GruntzMgr.h>         // CGruntzMgr - the class this whole TU implements
#include <Gruntz/GruntzMapMgr.h>      // CGruntzMapMgr (m_tileGrid; the 0x94-byte board)
#include <Gruntz/FaderMgr.h>          // CFaderMgr (m_faderMgr; SetConfig @0x17d980)
#include <Gruntz/GruntzPlayer.h>      // GruntzPlayer (m_options[4]; SeedForSlot @0xda870)
#include <Gruntz/Fonts.h>             // InitializeFonts (thunk 0x2db0 -> 0x115810)
#include <Gruntz/SoundFont.h>         // SFManager_SelectBestDevice / BuildSoundFontPath / Close
#include <Gruntz/GameObjectFactory.h> // RegisterGameObjectTypes (thunk 0x3526 -> 0xa3b0)
#include <strstrea.h> // real CRT istrstream/ostrstream (the GAME_ATTRIBUTEZ decode pair)
#include <string.h>
#include <stdlib.h> // srand (0x11fed0)

#include <Gruntz/CoordNode.h>     // the shared coord-pool node
#include <Gruntz/FreeNodePool.h>  // the ONE g_coordPool object (0x645540) Init builds
#include <Gruntz/ParseSource.h>   // canonical CParseSource (one shape)
#include <Dsndmgr/GruntzSoundZ.h> // canonical CGruntzSoundZ (m_sound audio host; SetXMidiVolume)
#include <Gruntz/CheatMgr.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/WorldSoundSet.h>
#include <Io/SaveGame.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/SoundFxEmitter.h> // ex Globals.h
#include <Wap32/GameApp.h> // ex Globals.h
#include <Gruntz/SoundState.h> // ex Globals.h transitive

#include <Rez/RezSyncGlobals.h> // RezSync's private split-views of g_inputMgr/g_spawnConfig

void* operator new(unsigned int);
void operator delete(void*);

DATA(0x002455b4)
i32 g_disableAudio = 0; // "Disable Audio"        master audio kill
DATA(0x002455bc)
i32 g_disableSound = 0; // "Disable Sound"        sound FX
DATA(0x002455c0)
i32 g_disableMusic = 0; // "Disable Music"        XMidi music
DATA(0x002455c4)
i32 g_disableFades = 0; // "Disable Fades"        (was Attract's g_fxDirectGate)
DATA(0x002455c8)
i32 g_disableJoystick = 0; // "Disable Joystick"
DATA(0x002455cc)
i32 g_disableSoundFonts = 0; // "Disable SoundFonts"
DATA(0x002455d0)
i32 g_disableDirectVideo = 0; // "Disable Direct Video Access"
DATA(0x002455d4)
i32 g_disableHqMovie = 0; // "Disable High Quality Movie"
DATA(0x002455d8)
i32 g_enableTriple = 0; // "Enable Triple"        triple buffering
DATA(0x002455dc)
i32 g_enableHiColor = 0; // "Enable HiColor"
DATA(0x002455e0)
i32 g_enableTrueColor = 0; // "Enable TrueColor"
DATA(0x002455e4)
i32 g_enableEmulation = 0; // "Enable Emulation"



DATA(0x00245534)
i32 g_attractStateCount = 0; // 0x645534



void __stdcall Blowfish_InitKey(unsigned char*);      // 0x16f6c0
void __stdcall BitStreamBlowfishDecode(void*, void*); // 0x16f760


DATA(0x00245210)
i32 g_appHInstance;

// =====================================================================
// @early-stop
// 6445-B /GX C++-EH function; complete carcass, ~5.6%->63.3% fuzzy. The /GX
// prologue is byte-EXACT (mov fs:0/push -1/push handler/mov fs:0,esp/sub
// esp,0x428) and the phase-1..4 blocks (coord free-list, Run/Fn2db0 gates, srand,
// ShowCursor while-loop, "Monolith Productions" registry reads, settings) verify
// byte-identical vs the target under llvm-objdump -dr (only the reloc-masked
// call/DIR32 operands differ). Three documented walls cap it: (1) the ~0x29-state
// `new T`-in-flight EH-state numbering + zero-register/pointer-register regalloc
// (retail pins the alloc ptr in edi/esi, docs/patterns/zero-register-pinning.md);
// (2) the heap objects' INLINE constructors (member sub-ctors + field-zeroing
// blocks, e.g. the ~250-B m_cmdGrid ctor @0x84948) are modeled as extern ctors, so
// those store runs are absent; (3) objdiff's global alignment DESYNCS at the
// long multi-way error ladder (docs/patterns/big-seh-fuzzy-desync.md), so the
// rollup understates the faithful carcass. Deferred to the final sweep: model
// each heap ctor inline once the child classes are reconstructed.
RVA(0x00083450, 0x192d)
i32 CGruntzMgr::Run(CGameWnd* pGameWnd, char* szCmdLine) {
    // --- Phase 1: coord-pool free list -------------------------------
    CoordPoolNode* pool = static_cast<CoordPoolNode*>(RezAlloc(0x3a980));
    g_coordPool.m_block = pool;
    if (!pool) {
        ReportError(0x800a, 0x404);
        return 0;
    }
    g_coordPool.m_count = 0x4e20; // 0x3a980 / sizeof(CoordPoolNode) 0xc
    CoordPoolNode* p = pool;
    u32 i = 0;
    do {
        p->m_next = p + 1;
        p = p->m_next;
        ++i;
    } while (i < static_cast<u32>(g_coordPool.m_count) - 1);
    p->m_next = 0;
    g_coordPool.m_freeHead = pool;
    g_coordPool.m_linkOffset = 4;

    // --- Phase 2: base game init + timers + cursor -------------------
    if (!CGameMgr::Run(pGameWnd, szCmdLine)) { // the base CGameMgr::Run (0x13dd50)
        ReportError(0x800a, 0x462);
        return 0;
    }
    if (!InitializeFonts()) {
        ReportError(0x800a, 0x463);
        return 0;
    }
    srand((::timeGetTime() + ::GetTickCount()) >> 1);
    g_wap32Run80 = 0x21;
    while (::ShowCursor(0) >= 0) {
    }

    // --- Phase 3: "Monolith Productions" registry --------------------
    Utils::RegistryHelper* reg = static_cast<Utils::RegistryHelper*>(RezAlloc(0x21c));
    if (reg) {
        reg->m_open = 0;
    }
    m_settings = reg;
    if (!m_settings->Open(
            "Monolith Productions",
            "Gruntz",
            "1.0",
            0,
            reinterpret_cast<HKEY>(0x80000002),
            0
        )) {
        ReportError(0x800a, 0x406);
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

    // --- Phase 4: audio/video settings -------------------------------
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
    // RETAIL-PROVEN (0x838a4): +0x124 receives eax = the LAST GetValueDword result
    // (vScroll); the old `vMusVol + 1` misread the adjacent `inc ecx` (m_numRuns+1).
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

    // --- Phase 5: command-line flags ---------------------------------
    i32 mode = 2;
    i32 noLogo = 0;
    char levelName[0x80];
    levelName[0] = 0;
    if (szCmdLine) {
        char buf[0x130];
        strcpy(buf, szCmdLine);
        StrUpr(buf);
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
        m_b4 = 0;
    }

    // --- Phase 6: surface manager + game level -----------------------
    g_appHInstance = reinterpret_cast<i32>(m_owner->m_hInstance);
    char dpBuf[0x114];
    strcpy(dpBuf, szCmdLine);
    ::AfxWinInit(m_owner->m_hInstance, 0, dpBuf, 1); // 0x1d3eff (NAFXCW)
    m_strWorldFile.Empty(); // 0x1b9c69 (?Empty@CString@@ - the ex "Mfc" C_1b9c69)
    // ONE new-site, TWO header names: the world/surface manager (the settled
    // CDDrawSurfaceMgr == CDDrawSurfaceMgr dual view; the DDraw side carries the
    // boot Init/SetHwnd virtuals, the game side is what m_world is typed as).
    CDDrawSurfaceMgr* world = new CDDrawSurfaceMgr;
    m_world = static_cast<CDDrawSurfaceMgr*>(world);
    i32 flags = (g_disableAudio || g_disableSound) ? 0xe5 : 0xe1;
    if (g_enableEmulation) {
        flags |= 0x10;
    }
    m_colorDepth = 0x10;
    if (!world->Init(static_cast<void*>(m_gameWnd->m_hwnd), 0x280, 0x1e0, 0x10, flags)) {
        ReportWorldStatus(0x407);
        return 0;
    }
    {
        i32 rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = 0x1df;
        rect[3] = 0x1df;
        m_modeW = 0x280;
        m_modeH = 0x1e0;
        world->m_level->BuildAllPlanes(reinterpret_cast<LevelCoordRect*>(rect));
    }
    world->SetHwnd(static_cast<void*>(&cb_403193));
    world->m_level->m_maxStepX = 0xe;
    world->m_level->m_maxStepY = 0xe;
    world->m_drawTarget->CreateOverlay(0, 0x30000);
    RecomputeViewScale();
    RegisterGameObjectTypes(world); // 0xa3b0 (the ctx IS this world holder)
    if (!MakeRezPath()) {           // 0x91670 (RezMgr.cpp; ex the RezMgr facet cast)
        return 0;
    }

    // --- Phase 7: CSymParser bute load (GRUNTZ.VRZ/.ZZZ/.XXX) --------
    if (m_symParser) {
        m_symParser->CSymParser::~CSymParser();
        RezFree(m_symParser);
        m_symParser = 0;
    }
    m_symParser = new CSymParser;
    {
        CString fn = GetRezPath();
        i32 ok = m_symParser->ParseBuffer(*reinterpret_cast<void**>(&fn), 1, 0) != 0;
        if (!ok) {
            ReportError(0x800b, 0x409);
            return 0;
        }
    }
    if (!m_symParser
             ->LoadEntry(const_cast<char*>("GRUNTZ.VRZ"), 0)) { // 0x13b0c0 (canonical arg order)
        ReportError(0x8149, 0x460);
        return 0;
    }
    m_symParser->LoadEntry(const_cast<char*>("GRUNTZ.ZZZ"), 1);
    m_symParser->LoadEntry(const_cast<char*>("GRUNTZ.XXX"), 1);
    SetColorDepth(m_colorDepth);

    // --- Phase 8: input device manager (m_faderMgr slot) ------------------
    m_faderMgr = new CFaderMgr;
    if (!m_faderMgr->SetConfig(0, 0, 0)) {
        ReportError(0x800a, 0x40a);
        return 0;
    }
    m_cheatMgr = new CCheatMgr;
    if (!m_cheatMgr->Init(reinterpret_cast<i32>(m_gameWnd->m_hwnd))) {
        ReportError(0x800a, 0x40b);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableSoundFonts == 0) {
        if (SFManager_SelectBestDevice()) {
            if (!BuildSoundFontPath(GetGruntzDriveLetter())) {
                CloseSoundFontDevice();
            }
        }
    }

    // --- Phase 9: audio host (m_sound) ---------------------------------
    m_sound = new CGruntzSoundZ;
    g_ailMidiDriver = 0;
    if (!m_sound->Init(
            reinterpret_cast<i32>(m_owner->m_hInstance),
            reinterpret_cast<i32>(m_gameWnd->m_hwnd),
            0
        )) {
        ReportError(0x800a, 0x40c);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableMusic == 0) {
        m_sound->SetXMidiVolume(vMusic);
    } else {
        m_sound->m_enabled = 0;
    }

    // --- Phase 10: sound-fx list (m_inputState) -----------------------------
    if (m_inputState) {
        m_inputState->Teardown();
        // 0x1b48c6 == ??1CPtrList (the ex "Mfc" D_1b48c6); MSVC5 needs the
        // qualified explicit-dtor spelling.
        (&m_inputState->m_list)->CPtrList::~CPtrList();
        RezFree(m_inputState);
        m_inputState = 0;
    }
    m_inputState = new CWorldSoundSet;
    if (!m_inputState->Init(world->m_soundRegistry, vSndVol)) {
        ReportError(0x800a, 0x40d);
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
    // RETAIL-PROVEN (0x83fb6/0x83fc2): the 0x4174 setter is SetVoiceVolume (0x91a10
    // stores its arg at +0x120, the voice-volume slot), and the +0x124 re-store is
    // the SPILLED "Scroll Speed" value - NOT a music volume (music volume is not a
    // mgr scalar; it lives on m_soundEnabled via SetXMidiVolume).
    SetVoiceVolume(vVoiVol);
    m_scrollSpeed = vScroll;

    // --- Phase 11: settings host (m_logicPump, m_saveSink) -----------------------
    m_logicPump = static_cast<CLightFxMgr*>(RezAlloc(0x3c));
    if (m_logicPump) {
        m_logicPump->m_reg = 0;
        m_logicPump->m_world = 0;
        m_logicPump->m_cache = 0;
        m_logicPump->m_greyTable = 0;
        for (i32 k = 0; k < 10; ++k) {
            m_logicPump->m_tables[k] = 0;
        }
    }
    if (!m_logicPump->Init(0, this)) {
        if (m_logicPump) {
            m_logicPump->Reset();
            RezFree(m_logicPump);
            m_logicPump = 0;
        }
        ReportError(0x800a, 0x411);
        return 0;
    }
    m_saveSink = new CSaveGame;
    if (!m_saveSink->SaveGameFile(g_lab545854)) {
        // (uses g_emptyString 0x6293f4 in retail)
        ReportError(0x800a, 0x412);
        return 0;
    }
    m_scoreHud = new CBattlezData;
    m_scoreHud->InitWithRecords(m_saveSink->m_pad24); // the opaque header tail = the records blob

    // --- Phase 12: the grunt spawn-config singleton (g_spawnConfig) -------
    g_spawnConfig = static_cast<CGruntSpawnConfig*>(RezAlloc(0x28));
    if (g_spawnConfig) {
        i32* z = reinterpret_cast<i32*>(g_spawnConfig);
        z[0] = z[1] = z[2] = z[4] = z[5] = 0;
    }
    if (!g_spawnConfig->Init(static_cast<CSpawnOwner*>(g_inputMgr))) {
        if (g_spawnConfig) {
            i32* z = reinterpret_cast<i32*>(g_spawnConfig);
            z[0] = z[1] = z[2] = z[4] = z[5] = 0;
            RezFree(g_spawnConfig);
            g_spawnConfig = 0;
        }
        ReportError(0x800a, 0x413);
        return 0;
    }

    // --- Phase 13: fader list (m_cmdSubMgr) --------------------------------
    m_cmdSubMgr = new CGruntzCmdMgr;
    // (the cross-cast documents the open RezSync==CGruntzMgr fold: this whole Init
    // runs on the 0xa30 CGruntzMgr singleton; see the TU header dossier.)
    if (!m_cmdSubMgr->SetMgr(this)) {
        ReportError(0x800a, 0x414);
        return 0;
    }
    m_tileGrid = new CGruntzMapMgr; // the 0x94-byte board (ex the H70 shell)
    if (!m_tileGrid) {
        ReportError(0x800a, 0x415);
        return 0;
    }
    m_spriteFactory = static_cast<CSpriteRefTable*>(RezAlloc(0x94));
    if (m_spriteFactory) {
        m_spriteFactory->m_factory = 0;
        m_spriteFactory->m_spriteMgrHolder = 0;
        m_spriteFactory->m_built = 0;
        for (i32 k = 0; k < 0x11; ++k) {
            m_spriteFactory->m_refA[k] = 0;
            m_spriteFactory->m_refB[k] = 0;
        }
    }
    if (!(reinterpret_cast<CTriggerMgr*>(m_spriteFactory))
             ->SetLevel(reinterpret_cast<CDDrawSurfaceMgr*>(m_shadeCache))) {
        ReportError(0x800a, 0x416);
        return 0;
    }

    // --- Phase 14: register mgr + probe 4 settings subobjects -------
    g_gameReg = this;
    g_lastNow = ::timeGetTime();
    g_frameDelta = 0;
    for (i32 s = 0; s < 4; ++s) {
        // retail (0x84591): ecx = &m_options[s], push s -> GruntzPlayer::SeedForSlot
        if (!m_options[s].SeedForSlot(s)) {
            ReportError(0x800a, 0x417);
            return 0;
        }
    }

    // --- Phase 15: GAME_ATTRIBUTEZ blowfish-decoded bute parse -------
    {
        CSymParser* mgr = m_symParser;
        CParseSource* stream = mgr->ResolveQualified("GAME_ATTRIBUTEZ", &g_lab545854);
        g_buteMgr.SetErrCallback(reinterpret_cast<ErrCallback>(&cb_401bc2));
        i32 ok = 0;
        if (stream) {
            g_buteMgr.m_10e = 1;
            i32 esz = stream->BeginParse();
            void* src = reinterpret_cast<void*>(
                stream->m_length
            ); // +0x0c doubles as the data ptr for this entry kind
            istrstream* rdr =
                new istrstream(static_cast<char*>(src), esz); // ??0istrstream (0x169700)
            Blowfish_InitKey(reinterpret_cast<unsigned char*>(const_cast<char*>("1212C")));
            ostrstream* snk =
                new ostrstream(static_cast<char*>(src), esz, 2); // ??0ostrstream (0x1698c0)
            BitStreamBlowfishDecode(snk, rdr);
            // carcass gap: retail allocs a third 0x60 stream object here with no
            // visible ctor call in the recovered bytes; kept as the bare allocation.
            g_buteMgr.m_stream = static_cast<istream*>(::operator new(0x60));
            stream->EndParse();
            g_buteMgr.Init();
            g_store6453f0.ClearRecursive(0);
            g_buteMgr.m_tree.m_root = 0;
            g_buteMgr.m_tree.m_lookupPending = 0;
            g_buteMgr.m_tree.m_nodeCount = 0;
            g_store64544c.ClearRecursive(0);
            g_buteMgr.m_tree48.m_root = 0;
            g_buteMgr.m_tree48.m_lookupPending = 0;
            g_buteMgr.m_tree48.m_nodeCount = 0;
            g_buteMgr.m_tree74.m_root = 0;
            g_buteMgr.m_tree74.m_lookupPending = 0;
            g_buteMgr.m_tree74.m_nodeCount = 0;
            ok = 1;
            if (!g_buteMgr.ParseGroup()) {
                g_buteMgr.m_0d = 1;
                ok = 0;
            }
            RezFree(rdr);
        }
        if (!ok) {
            ReportError(0x800a, 0x418);
            return 0;
        }
    }

    // --- Phase 16: sound / movie config + attract title screens -----
    m_cheatMgr->RegisterCheats();
    m_chatLog = new CFontConfig;
    m_chatLog->LoadFontConfig(0x1388, 0xbb8);
    m_cmdGrid = new CTriggerMgr;
    if (!m_cmdGrid->SetLevel(m_world)) {
        ReportError(0x800a, 0x41b);
        return 0;
    }
    g_localVersion = static_cast<i32>(
        g_buteMgr.GetDwordDef("General", "RezSync", static_cast<u32>(g_localVersion))
    );
    m_cueSink = new CGruntSpawnConfig;
    if (!m_cueSink->Init(reinterpret_cast<CSpawnOwner*>(this))) {
        ReportError(0x800a, 0x45f);
        return 0;
    }
    m_cueSink->m_voiceVolume = vScroll; // retail seeds the voice-volume slot from this local
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
        m_world->m_soundRegistry->m_10.Lookup("GAME_MOVIE", mv);
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
    // attract title screens
    {
        // ResolvePath returns the resolved CSymTab node; its 0x13be40 resolver is
        // CSymTab::ResolveQualified (the old view's "CSymParser::ResolveTab").
        CSymTab* attract = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_ATTRACT"));
        CString title;
        g_attractStateCount = 0;
        title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        while (attract->ResolveQualified(
            static_cast<const char*>(*reinterpret_cast<void**>(&title)),
            &g_lab504358
        )) {
            g_attractStateCount++;
            title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        }
        if (TransitionState(mode, 1, 0, 0)) {
            g_frameDelta = 0;
        } else if (mode == 0x11 && TransitionState(2, 1, 0, 0)) {
            g_frameDelta = 0;
        } else {
            ReportError(0x8005, mode == 0x11 ? 0x41c : 0x41d);
            return 0;
        }
    }
    return 1;
}

RVA(0x000853d0, 0x10)
void __stdcall RezFreeStdcall(void* a) {
    ::operator delete(a); // 0x1b9b82 == ??3@YAXPAX@Z (the __cdecl rez-free helper)
}

RVA(0x00085500, 0x23)
CString CGruntzMgr::GetRezPath() {
    return m_strRezPath;
}

// ---------------------------------------------------------------------------
// 0x085540 - the base ~CGameMgr out-of-line copy: cl emits it AUTOMATICALLY (the
// COMDAT copy of Wap32.h's inline `~CGameMgr() { Close(); }` that the emitted
// ??_G/vtable machinery references) - vptr restamp (0x5e9b8c) + devirtualized Close
// tail-call, 0xb bytes. No dtor-emitter scaffold is needed; the compiler copy is
// emitted (and RVA_COMPGEN-pinned) in src/Gruntz/GruntzMgr.cpp, whose obj carries
// the COMDAT.
