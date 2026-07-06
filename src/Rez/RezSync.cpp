// RezSync.cpp - CGameMgr-derived game bootstrap Init(CGameWnd*, char* cmdLine)
// @ RVA 0x00083450 (6445 B). `this` (ebp) is the CGruntzMgr-shaped 0xa30 game
// manager; Init returns 1 on success / 0 on every early error path (ret 0x8).
//
// Large /GX C++-EH carcass: the retail body carries a full exception frame
// (push -1 / push handler / mov fs:0,esp / sub esp,0x428) driven by ~0x29 EH
// states over the `new T` resource allocations + destructible CString temps;
// every early error path is `return 0` funnelling through one shared fs:0-
// restoring epilogue. See docs/patterns/big-seh-fuzzy-desync.md +
// docs/patterns/throwing-operator-new-eh-state-transition.md. Objdiff rolls up
// LOW on such functions even for a faithful carcass (alignment desync at the
// multi-way error ladder) - the deliverable is the control-flow + offset +
// ordered-call carcass, not a byte-perfect frame.
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Utils/RegistryHelper.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <rva.h>
#include <Ints.h>
#include <Mfc.h>          // CString + the MFC collection ctors/dtors (reloc-masked)
#include <Bute/ButeMgr.h> // canonical CButeMgr / CButeStore (one shape)
#include <string.h>
#include <stdlib.h> // srand (0x11fed0)

#include <Gruntz/CoordNode.h>     // the shared coord-pool node
#include <Gruntz/ParseSource.h>   // canonical CParseSource (one shape)
#include <Dsndmgr/GruntzSoundZ.h> // canonical CGruntzSoundZ (m_48 audio host; SetXMidiVolume)
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
#include <Globals.h>

// retail's global allocator is the nothrow pool alloc; MSVC5 `new T` emits the
// post-alloc null-check + ctor-in-flight EH-state store the retail body has.
void* operator new(unsigned int);
void operator delete(void*);
extern "C" void* RezAlloc(unsigned int); // 0x1b9b46 (raw non-EH pool allocs)
extern "C" void RezFree(void*);          // 0x1b9b82

// ---------- reloc-masked engine globals ----------
extern CoordNode* g_coordPool;     // 0x645540
extern CoordNode* g_coordFreeList; // 0x645544
extern i32 g_coordCount;           // 0x645548
extern i32 g_freeBias;             // 0x64554c
extern void* g_mgrPtr;             // 0x64556c
extern u32 g_startTick;            // 0x645580
extern i32 g_645584;               // 0x645584
extern i32 g_653c5c;               // 0x653c5c
extern i32 g_sndEnabled;           // 0x61ab20
extern i32 g_2455b4, g_2455bc, g_2455c0, g_2455c4, g_2455c8, g_2455cc;
extern i32 g_2455d0, g_2455d4, g_2455d8, g_2455dc, g_2455e0, g_2455e4;
extern i32 g_64526c, g_6452d0, g_645268, g_645568, g_645538, g_6451a4;
extern i32 g_6452d4, g_6452a8, g_645558, g_645560, g_64555c, g_645564;
extern i32 g_645210, g_645534;
extern void* g_645570; // attract host
extern void* g_645578;
extern void* g_60fa70;
extern u32(WINAPI* g_pTimeGetTime)();  // 0x6c4650
extern u32(WINAPI* g_pGetTickCount)(); // 0x6c3fc8
extern i32(WINAPI* g_ShowCursor)(i32); // 0x6c44c4

extern "C" char* StrUpr(char*); // 0x18d330

extern "C" void cb_403193();
extern "C" void cb_401bc2();
extern char g_lab504358[]; // 0x504358
extern char g_lab545854[]; // 0x545854

extern "C" i32 Fn2423cdecl(void*);                    // 0x2423
extern "C" void Fn3526cdecl(void*);                   // 0x3526
void __stdcall Fn1d3eff(i32, i32, void*, i32);        // 0x1d3eff
void __stdcall Blowfish_InitKey(unsigned char*);      // 0x16f6c0
void __stdcall BitStreamBlowfishDecode(void*, void*); // 0x16f760

// generic thiscall MFC ctor/dtor helper (reloc-masked; only call shape matters)
SIZE_UNKNOWN(Mfc);
struct Mfc {
    void C_1b4867(i32);
    void C_1b48a6();
    void D_1b48c6();
    void C_1b4f0b();
    void C_1b527e();
    void C_1b7e17(i32);
    void C_1b8247(i32);
    void C_1b9b93();
    void C_1b9c69();
    void D_1b9cde();
    void C_11f5a0(i32, i32, void*, void*);
};

SIZE(RegHelper, 0x21c);
struct RegHelper { // m_38 (0x21c)
    i32 m_0;
};
struct CDDrawSurfaceMgr { // m_30 (0x40); polymorphic - VInit at vtable slot 6
    // vptr @ +0 (extern ctor 0x155840 stamps the real vtable)
    CDDrawSubMgrPages* m_04; // +4
    char _p08[0x24 - 0x08];
    CGameLevel* m_24;           // +0x24
    CDDrawSubMgrLeafScan* m_28; // +0x28
    char _p2c[0x40 - 0x2c];
    CDDrawSurfaceMgr();
    ~CDDrawSurfaceMgr();
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual i32 VInit(void*, i32, i32, i32, i32); // slot 6 (+0x18)
    void VMethod155f50(void*);                    // 0x155f50
};
struct CSymParser { // m_34 (0x94)
    CSymParser();
    ~CSymParser();
    char raw[0x94];
    i32 ParseBuffer(void*, i32, i32);           // 0x13ad00
    i32 Stub13b0c0(i32, const char*);           // 0x13b0c0
    void* ResolveQualified(const char*, void*); // 0x13bff0
    void* ResolvePath(const char*);             // 0x13c030
    i32 ResolveTab(const char*, void*);         // 0x13be40
};
struct CFaderMgr { // m_40 (0x28)
    CFaderMgr();
    ~CFaderMgr();
    char raw[0x28];
    i32 SetConfig(i32, i32, i32); // 0x17d980
};
// m_48 is the audio host = canonical CGruntzSoundZ (Init @0x138490 / SetXMidiVolume
// @0x138950 / m_enabled @+0x28); `new CGruntzSoundZ` inlines its real CMapStringToOb
// ctor (<Dsndmgr/GruntzSoundZ.h>).
SIZE(H70, 0x94);
struct H70 {
    char raw[0x94];
    H70();
    ~H70();
};

// GAME_ATTRIBUTEZ ButeMgr config load: the parse stream is the canonical
// CParseSource (BeginParse/EndParse @0x139960/0x1399d0), included above.
// The bute-config manager + its owned keyed stores are the canonical CButeMgr /
// CButeStore (include/Bute/ButeMgr.h): SetErrCallback (0x170380), Init (0x170330),
// GetDwordDef (0x1721e0), ParseGroup (0x171580), CButeStore::ClearRecursive
// (0x16e070) are all reloc-masked __thiscall.
extern CButeMgr g_buteMgr; // 0x6453d8
extern CButeStore g_store6453f0, g_store64544c;
extern i32 g_645460; // 0x645460 (RezSync-local; NOT the 0x24556c singleton)
extern u8 g_6454e6, g_6454e7, g_6453e5;
extern i32 g_645478, g_645420, g_645408, g_645418, g_645404;
extern i32 g_645438, g_645448, g_645434, g_645464, g_645474;

SIZE(DecodeObj, 0x60);
struct DecodeObj {
    char raw[0x60];
    void* M169700(void*, i32, i32);      // 0x169700
    void* M169700b(void*, i32);          // 0x169700 (2-arg overload site)
    void* M1698c0(void*, i32, i32, i32); // 0x1698c0
};
SIZE_UNKNOWN(MovieLookup);
struct MovieLookup {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at the call

SIZE_UNKNOWN(RezSync);
struct RezSync {
    u32 m_00;
    void* m_04;
    void* m_08;
    u32 m_0c;
    i32 m_sound; // Sound
    i32 m_music; // Music
    char _p18[0x30 - 0x18];
    CDDrawSurfaceMgr* m_30;
    CSymParser* m_34;
    RegHelper* m_38;
    char _p3c[0x40 - 0x3c];
    CFaderMgr* m_40;
    CCheatMgr* m_44;
    CGruntzSoundZ* m_48; // audio host (Init/SetXMidiVolume/m_enabled)
    char _p4c[0x50 - 0x4c];
    CShadeTableCache* m_50;
    CWorldSoundSet* m_54;
    CSaveGame* m_58;
    CFontConfig* m_5c;
    CGruntSpawnConfig* m_60;
    char _p64[0x68 - 0x64];
    CTriggerMgr* m_68;
    CGruntzCmdMgr* m_6c;
    H70* m_70;
    void* m_74;
    CLightFxMgr* m_78;
    CBattlezData* m_7c;
    i32 m_numRuns;   // Num Runs
    i32 m_numMovies; // Num Movies
    i32 m_88;
    i32 m_8c;
    i32 m_90;
    i32 m_width;  // width
    i32 m_height; // height
    char _p9c[0xac - 0x9c];
    i32 m_ac;
    i32 m_b0;
    i32 m_b4;
    i32 m_checkpointPrompts; // Checkpoint Prompts
    char _pbc[0xc8 - 0xbc];
    void* m_c8; // CString
    char _pcc[0xd0 - 0xcc];
    u8 m_d0;
    char _pd1[0xd4 - 0xd1];
    i32 m_d4;
    char _pd8[0x100 - 0xd8];
    i32 m_voice;      // Voice
    i32 m_ambient;    // Ambient
    i32 m_interlaced; // Interlaced
    i32 m_highDetail; // High Detail
    i32 m_110;        // High Detail 2 (retail store crosses to vEasy; ambiguous, left placeholder)
    char _p114[0x118 - 0x114];
    i32 m_118; // Easy Mode default / Resolution store (crossed; ambiguous, left placeholder)
    i32 m_soundVolume; // Sound Volume
    i32 m_voiceVolume; // Voice Volume
    i32 m_musicVolume; // Music Volume
    char _p128[0x150 - 0x128];
    char m_150[4 * 0x238];
    char _tail[0xa30 - (0x150 + 4 * 0x238)];

    i32 Init(void* a1, char* a2);
    i32 Run(void*, void*); // 0x13dd50
    void Error2(u32, u32); // 0x346d
    void Error1(u32);      // 0x3f80
    i32 Fn2db0();
    void* Fn4214();
    i32 Fn2112();
    void Fn1db6();
    i32 Fn1c12();
    void Fn1df7(i32);
    i32 Fn262b();
    void Fn129e();
    void Fn2cc5();
    i32 Fn201d(i32);
    void Fn1ed8();
    i32 Fn12d0(i32, i32, i32, i32);
    void* Fn320b(void**);
    void Fn40c0(i32);
    void Fn4174(i32);
    i32 ProbeSettings150(void*); // 0x40a7
};

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
// blocks, e.g. the ~250-B m_68 ctor @0x84948) are modeled as extern ctors, so
// those store runs are absent; (3) objdiff's global alignment DESYNCS at the
// long multi-way error ladder (docs/patterns/big-seh-fuzzy-desync.md), so the
// rollup understates the faithful carcass. Deferred to the final sweep: model
// each heap ctor inline once the child classes are reconstructed.
RVA(0x00083450, 0x192d)
i32 RezSync::Init(void* a1, char* a2) {
    // --- Phase 1: coord-pool free list -------------------------------
    CoordNode* pool = (CoordNode*)RezAlloc(0x3a980);
    g_coordPool = pool;
    if (!pool) {
        Error2(0x800a, 0x404);
        return 0;
    }
    g_coordCount = 0x4e20;
    CoordNode* p = pool;
    u32 i = 0;
    do {
        p->m_next = p + 1;
        p = p->m_next;
        ++i;
    } while (i < (u32)g_coordCount - 1);
    p->m_next = 0;
    g_coordFreeList = pool;
    g_freeBias = 4;

    // --- Phase 2: base game init + timers + cursor -------------------
    if (!Run(a1, a2)) {
        Error2(0x800a, 0x462);
        return 0;
    }
    if (!Fn2db0()) {
        Error2(0x800a, 0x463);
        return 0;
    }
    srand((g_pTimeGetTime() + g_pGetTickCount()) >> 1);
    g_wap32Run80 = 0x21;
    while (g_ShowCursor(0) >= 0) {
    }

    // --- Phase 3: "Monolith Productions" registry --------------------
    RegHelper* reg = (RegHelper*)RezAlloc(0x21c);
    if (reg) {
        reg->m_0 = 0;
    }
    m_38 = reg;
    if (!((Utils::RegistryHelper*)m_38)
             ->Open("Monolith Productions", "Gruntz", "1.0", 0, (HKEY)0x80000002, 0)) {
        Error2(0x800a, 0x406);
        return 0;
    }
    m_width = 0x280;
    m_height = 0x1e0;
    m_numRuns = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Num Runs", 0);
    m_numMovies = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Num Movies", 0);
    g_2455d4 =
        ((Utils::RegistryHelper*)m_38)->GetValueDword("Disable High Quality Movie", 0) ? 1 : 0;
    g_2455b4 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable Audio", 0);
    g_2455bc = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable Sound", 0);
    g_2455c0 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable Music", 0);
    g_2455c4 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable Fades", 0);
    g_2455d0 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable Direct Video Access", 0);
    g_2455c8 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable Joystick", 0);
    g_2455cc = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Disable SoundFonts", 0);
    g_2455d8 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Enable Triple", 0);
    g_2455dc = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Enable HiColor", 0);
    g_2455e0 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Enable TrueColor", 0);
    g_2455e4 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Enable Emulation", 0);
    m_checkpointPrompts =
        (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Checkpoint Prompts", 1);
    g_2455dc = 1;
    g_64526c = 0;
    g_6452d0 = 0;
    g_645268 = 0;
    g_645568 = 0;
    g_645538 = 0;
    g_6451a4 = 0;
    g_6452d4 = 1;
    g_6452a8 = 0;
    g_645558 = 0;
    g_645560 = 0;
    g_64555c = 0;
    g_645564 = 0;

    // --- Phase 4: audio/video settings -------------------------------
    i32 vMusic = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Music", m_music);
    i32 vSound = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Sound", m_sound);
    i32 vVoice = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Voice", m_voice);
    i32 vAmbient = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Ambient", m_ambient);
    i32 vInterlaced =
        (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Interlaced", m_interlaced);
    i32 vHigh1 = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("High Detail", m_highDetail);
    m_highDetail = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("High Detail", m_110);
    i32 vEasy = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Easy Mode", m_118);
    i32 res = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Resolution", 1);
    m_118 = res;
    if (res == 3) {
        m_width = 0x400;
        m_height = 0x300;
    } else if (res == 2) {
        m_width = 0x320;
        m_height = 0x258;
    } else {
        m_width = 0x280;
        m_height = 0x1e0;
    }
    i32 vMusVol = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Music Volume", 0x64);
    i32 vSndVol = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Sound Volume", 0x3c);
    i32 vVoiVol = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Voice Volume", 0x50);
    i32 vScroll = (i32)((Utils::RegistryHelper*)m_38)->GetValueDword("Scroll Speed", 0x14);
    m_soundVolume = vSndVol;
    m_voiceVolume = vVoiVol;
    m_musicVolume = vMusVol + 1;
    m_numRuns = m_numRuns + 1;
    if (g_2455d0 != 0) {
        g_2455c4 = 1;
        g_2455e4 = 1;
    }
    m_ac = 0;
    m_b0 = 0;
    m_d4 = 0;
    m_d0 = 0;
    Fn4214();

    // --- Phase 5: command-line flags ---------------------------------
    i32 mode = 2;
    i32 noLogo = 0;
    char levelName[0x80];
    levelName[0] = 0;
    if (a2) {
        char buf[0x130];
        strcpy(buf, a2);
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
    if (Fn2112()) {
        mode = 0x11;
        m_b4 = 0;
    }

    // --- Phase 6: surface manager + game level -----------------------
    g_645210 = *(i32*)((char*)m_08 + 0xc);
    char dpBuf[0x114];
    strcpy(dpBuf, a2);
    Fn1d3eff(*(i32*)((char*)m_08 + 0xc), 0, dpBuf, 1);
    ((Mfc*)&m_c8)->C_1b9c69();
    m_30 = new CDDrawSurfaceMgr;
    i32 flags = (g_2455b4 || g_2455bc) ? 0xe5 : 0xe1;
    if (g_2455e4) {
        flags |= 0x10;
    }
    m_88 = 0x10;
    if (!m_30->VInit(*(void**)((char*)m_04 + 4), 0x280, 0x1e0, 0x10, flags)) {
        Error1(0x407);
        return 0;
    }
    {
        i32 rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = 0x1df;
        rect[3] = 0x1df;
        m_8c = 0x280;
        m_90 = 0x1e0;
        m_30->m_24->BuildAllPlanes((LevelCoordRect*)rect);
    }
    m_30->VMethod155f50((void*)&cb_403193);
    m_30->m_24->m_maxStepX = 0xe;
    m_30->m_24->m_maxStepY = 0xe;
    m_30->m_04->Method_158cb0(0, 0x30000);
    Fn1db6();
    Fn3526cdecl(m_30);
    if (!Fn1c12()) {
        return 0;
    }

    // --- Phase 7: CSymParser bute load (GRUNTZ.VRZ/.ZZZ/.XXX) --------
    if (m_34) {
        m_34->~CSymParser();
        RezFree(m_34);
        m_34 = 0;
    }
    m_34 = new CSymParser;
    {
        CString fn;
        Fn320b((void**)&fn);
        i32 ok = m_34->ParseBuffer(*(void**)&fn, 1, 0) != 0;
        if (!ok) {
            Error2(0x800b, 0x409);
            return 0;
        }
    }
    if (!m_34->Stub13b0c0(0, "GRUNTZ.VRZ")) {
        Error2(0x8149, 0x460);
        return 0;
    }
    m_34->Stub13b0c0(1, "GRUNTZ.ZZZ");
    m_34->Stub13b0c0(1, "GRUNTZ.XXX");
    Fn1df7(m_88);

    // --- Phase 8: input device manager (m_40 slot) ------------------
    m_40 = new CFaderMgr;
    if (!m_40->SetConfig(0, 0, 0)) {
        Error2(0x800a, 0x40a);
        return 0;
    }
    m_44 = new CCheatMgr;
    if (!m_44->Init(*(i32*)((char*)m_04 + 4))) {
        Error2(0x800a, 0x40b);
        return 0;
    }
    if (g_2455b4 == 0 && g_2455cc == 0) {
        if (Fn262b()) {
            if (!Fn2423cdecl(Fn4214())) {
                Fn129e();
            }
        }
    }

    // --- Phase 9: audio host (m_48) ---------------------------------
    m_48 = new CGruntzSoundZ;
    g_653c5c = 0;
    if (!m_48->Init(*(i32*)((char*)m_08 + 0xc), *(i32*)((char*)m_04 + 4), 0)) {
        Error2(0x800a, 0x40c);
        return 0;
    }
    if (g_2455b4 == 0 && g_2455c0 == 0) {
        m_48->SetXMidiVolume(vMusic);
    } else {
        m_48->m_enabled = 0;
    }

    // --- Phase 10: sound-fx list (m_54) -----------------------------
    if (m_54) {
        m_54->Teardown();
        ((Mfc*)&m_54->m_list)->D_1b48c6();
        RezFree(m_54);
        m_54 = 0;
    }
    m_54 = new CWorldSoundSet;
    if (!m_54->Init(m_30->m_28, vSndVol)) {
        Error2(0x800a, 0x40d);
        return 0;
    }
    {
        i32 f = m_54->m_active;
        if (vMusVol != 0) {
            if (f == 0) {
                m_54->m_active = 1;
                m_54->Resume();
            }
        } else if (f != 0) {
            m_54->m_active = 0;
            m_54->Stop();
        }
    }
    Fn40c0(vSndVol);
    Fn4174(vScroll);
    m_musicVolume = vMusVol;

    // --- Phase 11: settings host (m_78, m_58) -----------------------
    m_78 = (CLightFxMgr*)RezAlloc(0x3c);
    if (m_78) {
        i32* z = (i32*)m_78;
        z[1] = z[2] = z[3] = z[4] = 0;
        for (i32 k = 0; k < 10; ++k) {
            *(i32*)((char*)m_78 + 0x14 + k * 4) = 0;
        }
    }
    if (!m_78->Init(0, (void*)this)) {
        if (m_78) {
            m_78->Reset();
            RezFree(m_78);
            m_78 = 0;
        }
        Error2(0x800a, 0x411);
        return 0;
    }
    m_58 = new CSaveGame;
    if (!m_58->SaveGameFile(g_lab545854)) {
        // (uses g_emptyString 0x6293f4 in retail)
        Error2(0x800a, 0x412);
        return 0;
    }
    m_7c = new CBattlezData;
    m_7c->InitWithRecords((char*)m_58 + 0x24);

    // --- Phase 12: attract host list (g_645578) ---------------------
    g_645578 = RezAlloc(0x28);
    if (g_645578) {
        i32* z = (i32*)g_645578;
        z[0] = z[1] = z[2] = z[4] = z[5] = 0;
    }
    if (!((CGruntSpawnConfig*)g_645578)->Init((CSpawnOwner*)g_645570)) {
        if (g_645578) {
            i32* z = (i32*)g_645578;
            z[0] = z[1] = z[2] = z[4] = z[5] = 0;
            RezFree(g_645578);
            g_645578 = 0;
        }
        Error2(0x800a, 0x413);
        return 0;
    }

    // --- Phase 13: fader list (m_6c) --------------------------------
    m_6c = new CGruntzCmdMgr;
    if (!m_6c->SetMgr((GzMgr*)this)) {
        Error2(0x800a, 0x414);
        return 0;
    }
    m_70 = new H70;
    if (!m_70) {
        Error2(0x800a, 0x415);
        return 0;
    }
    m_74 = RezAlloc(0x94);
    if (m_74) {
        i32* z = (i32*)m_74;
        z[0] = z[1] = 0;
        *(i32*)((char*)m_74 + 0x90) = 0;
        for (i32 k = 0; k < 0x11; ++k) {
            *(i32*)((char*)m_74 + 8 + k * 4) = 0;
            *(i32*)((char*)m_74 + 0x4c + k * 4) = 0;
        }
    }
    if (!((CTriggerMgr*)m_74)->SetLevel((CTmLevel*)m_50)) {
        Error2(0x800a, 0x416);
        return 0;
    }

    // --- Phase 14: register mgr + probe 4 settings subobjects -------
    g_mgrPtr = this;
    g_startTick = g_pTimeGetTime();
    g_645584 = 0;
    for (i32 s = 0; s < 4; ++s) {
        if (!ProbeSettings150(&m_150[s * 0x238])) {
            Error2(0x800a, 0x417);
            return 0;
        }
    }

    // --- Phase 15: GAME_ATTRIBUTEZ blowfish-decoded bute parse -------
    {
        CSymParser* mgr = m_34;
        void* node = mgr->ResolveQualified("GAME_ATTRIBUTEZ", &g_lab545854);
        g_buteMgr.SetErrCallback((ErrCallback)&cb_401bc2);
        i32 ok = 0;
        if (node) {
            CParseSource* stream = (CParseSource*)node;
            g_6454e6 = 1;
            i32 esz = stream->BeginParse();
            void* src = *(void**)((char*)stream + 0xc);
            DecodeObj* d0 = new DecodeObj;
            void* rdr = d0 ? d0->M169700(src, esz, 1) : 0;
            Blowfish_InitKey((unsigned char*)"1212C");
            DecodeObj* d1 = new DecodeObj;
            void* snk = d1 ? d1->M1698c0(src, esz, 2, 1) : 0;
            BitStreamBlowfishDecode(snk, rdr);
            DecodeObj* d2 = new DecodeObj;
            g_645478 = (i32)d2;
            stream->EndParse();
            g_buteMgr.Init();
            g_store6453f0.ClearRecursive(0);
            g_645408 = 0;
            g_645418 = 0;
            g_645404 = 0;
            g_store64544c.ClearRecursive(0);
            g_645438 = 0;
            g_645448 = 0;
            g_645434 = 0;
            g_645464 = 0;
            g_645474 = 0;
            g_645460 = 0;
            ok = 1;
            if (!g_buteMgr.ParseGroup()) {
                g_6453e5 = 1;
                ok = 0;
            }
            RezFree(rdr);
        }
        if (!ok) {
            Error2(0x800a, 0x418);
            return 0;
        }
    }

    // --- Phase 16: sound / movie config + attract title screens -----
    m_44->RegisterCheats();
    m_5c = new CFontConfig;
    m_5c->LoadFontConfig(0x1388, 0xbb8);
    m_68 = new CTriggerMgr;
    if (!m_68->SetLevel((CTmLevel*)m_30)) {
        Error2(0x800a, 0x41b);
        return 0;
    }
    g_60fa70 = (void*)g_buteMgr.GetDwordDef("General", "RezSync", (u32)g_60fa70);
    m_60 = new CGruntSpawnConfig;
    if (!m_60->Init((CSpawnOwner*)this)) {
        Error2(0x800a, 0x45f);
        return 0;
    }
    *(i32*)((char*)m_60 + 0x2c) = vScroll;
    m_music = vMusic;
    m_sound = vSound;
    g_sndEnabled = vSound;
    m_voice = vVoice;
    m_ambient = vAmbient;
    m_interlaced = vInterlaced;
    m_highDetail = vHigh1;
    m_110 = vEasy;
    if (!m_30->m_28->HasKeyEqual_1583c0("GAME")) {
        void* sz = m_34->ResolvePath("GAME_SOUNDZ");
        if (!sz) {
            return 0;
        }
        m_30->m_28->ScanTree_157ee0((DirNode*)sz, "GAME", "_");
    }
    {
        void* mv = 0;
        ((CMapStringToOb*)((char*)m_30->m_28 + 0x10))->Lookup("GAME_MOVIE", (CObject*&)mv);
        m_30->m_28->MatchSub_1584f0((LeafScanSoundArg*)mv, 0);
    }
    Fn1ed8();
    if (!Fn2112()) {
        if (m_numMovies > 0 && m_numRuns > 1) {
            if (((Utils::RegistryHelper*)m_38)->GetValueDword("Skip Logo Movies", 0) == 0
                && noLogo == 0) {
                Fn2cc5();
            }
        } else {
            Fn2cc5();
            if (Fn201d(2)) {
                ++m_numMovies;
            }
        }
    }
    // attract title screens
    {
        CSymParser* attract = (CSymParser*)m_34->ResolvePath("STATEZ_ATTRACT");
        CString title;
        g_645534 = 0;
        title.Format("\\SCREENZ\\TITLE%d", g_645534 + 1);
        while (attract->ResolveTab((const char*)*(void**)&title, &g_lab504358)) {
            g_645534++;
            title.Format("\\SCREENZ\\TITLE%d", g_645534 + 1);
        }
        if (Fn12d0(mode, 1, 0, 0)) {
            g_645584 = 0;
        } else if (mode == 0x11 && Fn12d0(2, 1, 0, 0)) {
            g_645584 = 0;
        } else {
            Error2(0x8005, mode == 0x11 ? 0x41c : 0x41d);
            return 0;
        }
    }
    return 1;
}
