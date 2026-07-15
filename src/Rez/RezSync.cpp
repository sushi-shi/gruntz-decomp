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
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr (m_30; ex the local view)
#include <Utils/RegistryHelper.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Rez/FrameClock.h> // g_lastNow (the frame-clock now cell Init seeds)
#include <rva.h>
#include <Ints.h>
#include <Mfc.h>                      // CString + the MFC collection ctors/dtors (reloc-masked)
#include <Wap32/Wap32.h>              // WAP32::CGameMgr (the base ~CGameMgr tail-calls its Close)
#include <Bute/ButeMgr.h>             // canonical CButeMgr / CButeStore (one shape)
#include <Gruntz/BoundaryTailViews.h> // Obj85500 (fuzzy-identity 0x85500 CString getter)
#include <string.h>
#include <stdlib.h> // srand (0x11fed0)

#include <Gruntz/CoordNode.h>     // the shared coord-pool node
#include <Gruntz/FreeNodePool.h>  // the ONE g_coordPool object (0x645540) Init builds
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
// The coord-node recycle pool - ONE 16-byte FreeNodePool object at 0x645540, NOT four
// scalars. Init below builds it (block / count / free-list head / link offset); the four
// former per-field globals (g_coordPool/g_coordFreeList/g_coordCount/g_freeBias) were
// separate fabricated symbols at 0x645540/44/48/4c - interior addresses of this object,
// so they could never resolve at link. Defined in src/Gruntz/GameText.cpp (owner TU).
extern FreeNodePool g_coordPool; // 0x645540
// 0x24556c is the game-registry singleton g_gameReg (DEFINED in GruntzMgr.cpp). The
// assignment below is CGruntzMgr::Init SELF-REGISTERING - which is exactly what proves
// the cell holds this manager. Spelling it `g_mgrPtr` with C++ linkage made a second,
// unresolvable symbol (?g_mgrPtr@@3PAXA) for a cell 50 other TUs already share.
extern "C" void* g_gameReg; // 0x64556c (typed CGruntzMgr* in its owner TU)
// g_lastNow (0x245580, the frame-clock "now" cell) is declared in <Rez/FrameClock.h>;
// Init seeds it with the boot timeGetTime().
extern i32 g_sndEnabled; // 0x61ab20
// 0x645584 is extern-"C" tree-wide (RezMgr's g_frameDelta, the frame delta); a plain
// C++ `extern` here emitted the divergent ?g_frameDelta@@3HA.
extern "C" i32 g_frameDelta; // 0x645584
// 0x20fa70: the local protocol/rez-sync version word (canonical decl in <Net/NetMgr.h>,
// DEFINED in Multi.cpp). Init loads it from the .bute [General] "RezSync" key; Net
// compares it against the host packet's version field. This TU had it as a private
// `void* g_60fa70` - same cell, divergent symbol, and the void* forced two casts.
extern "C" i32 g_localVersion; // 0x60fa70
// ---------------------------------------------------------------------------
// The [Config] gate band, 0x6455b4..0x6455e4 - DEFINED HERE (storage), declared in
// <Globals.h>.
//
// Nothing in the tree defined any of these twelve. They were `extern`-only under THREE
// different namings - this TU's C++-linkage `g_2455b4..` (-> ?g_2455b4@@3HA), Globals.h's
// `g_optLock*` / `g_6455xx`, and Attract's `g_fxDirectGate` - so every reference was an
// unresolved external (objdiff masks the DIR32, the linker would not).
//
// OWNER: this TU. RezSync::Init is the sole initializer of the whole band - it reads
// every one of the twelve straight out of the .bute [Config] section, and those KEY
// NAMES are what name them (no invention; see the assignments in Init below).
//
// The old names `g_optLockAudio` (0x2455bc) / `g_optLockSpeech` (0x2455c0) were simply
// WRONG, and three independent sites agree on the correction:
//   * Init loads 0x2455bc from "Disable Sound" and 0x2455c0 from "Disable Music";
//   * VideoConfig gates the XMidi *music* control on 0x2455c0 (not speech);
//   * GruntzMgr writes SetValueDword("Disable_Joystick", <0x2455c8>) - confirming c8.
extern "C" {
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
}

// The settings-dialog numeric cells (0x2451a4..0x245568). Init bulk-resets them; the
// dialog at 0x092ab0 (GruntzCmdMgr.cpp) owns them and DEFINES the storage. This TU had
// spelled them g_6451a4/... with C++ linkage - a second symbol for the same memory, so
// neither name resolved. Reference the owner's names.
extern i32 g_dlgVal_6451a4, g_dlgVal_645268, g_dlgVal_64526c, g_dlgVal_6452a8;
extern i32 g_dlgVal_6452d0, g_dlgVal_6452d4, g_dlgVal_645538, g_dlgVal_645558;
extern i32 g_dlgVal_64555c, g_dlgVal_645560, g_dlgVal_645564, g_dlgVal_645568;

// LINKAGE FIX: 0x245570/0x245578 are extern-"C" tree-wide (GruntzMgr owns and DEFINES
// them, typed DirectInputMgr2*/StateMgrBZ*). Declaring them with C++ linkage here
// emitted ?g_645570@@3PAXA - a divergent symbol for the same address, unresolved on both
// sides. Same names, one linkage. (This TU's void* view of them contradicts GruntzMgr's
// types - RezSync casts 0x245578 to CGruntSpawnConfig* and 0x245570 to CSpawnOwner*.
// extern "C" carries no type, so it links either way; the disagreement is REAL and
// unresolved - flagged, not papered over.)
extern "C" {
    // @identity-TODO: 0x645570 has TWO readings and they contradict - here it is the
    // CSpawnOwner* handed to CGruntSpawnConfig::Init, in GruntzMgr it is read as a
    // DirectInputMgr2* ("attract host"). Typing it either way would be a guess, so it
    // stays void* and the `(CSpawnOwner*)` cast at the call site STAYS: the cast is
    // telling the truth - the type above it is still unresolved. Settle the conflation
    // (one object, two class names) and the cast falls out on its own.
    extern void* g_645570;
    // 0x645578 IS the CGruntSpawnConfig singleton - PROVEN in this very function:
    // RezAlloc(0x28) (its exact size), then ->Init(CSpawnOwner*), then RezFree on the
    // failure path. Typed, so the `(CGruntSpawnConfig*)` cast at the Init call is gone.
    extern CGruntSpawnConfig* g_spawnConfig;
}

// DEFINED HERE (owner = the producer): the attract title-screen count. Init counts how
// many "\SCREENZ\TITLE%d" tabs resolve under STATEZ_ATTRACT (loop below); AttractState /
// CreditsState then pick a screen with `g_gameReg->m_numRuns % g_attractStateCount + 1`.
// It had THREE spellings and no storage: ?g_attractStateCount@@3HA (Attract), this TU's
// C++-linkage ?g_645534@@3HA, and the extern-"C" _g_645534 (AttractState/CreditsState/
// Multi). One name (Attract's - the only semantic one), one linkage (extern "C").
DATA(0x00245534)
extern "C" {
    i32 g_attractStateCount = 0; // 0x645534
}

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

// (The local CDDrawSurfaceMgr call-view is DISSOLVED onto the canonical
// <DDrawMgr/DDrawSurfaceMgr.h>: VInit was the slot-6 Init, VMethod155f50 is
// SetHwnd, m_04/m_24/m_28 are m_pages/m_resolveSubMgr/m_leafScan.)
struct CSymParser { // m_34 (0x94)
    CSymParser();
    ~CSymParser();
    char raw[0x94];
    i32 ParseBuffer(void*, i32, i32); // 0x13ad00
    i32 Stub13b0c0(i32, const char*); // 0x13b0c0
    i32
    ResolveQualified(const char*, void*); // 0x13bff0 (real returns i32; caller reinterprets as ptr)
    void* ResolvePath(const char*);       // 0x13c030
    i32 ResolveTab(const char*, void*);   // 0x13be40
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
// g_buteMgr (@0x6453d8) comes from <Bute/ButeMgr.h>; its CRT-init note is at the field block below.
// g_store6453f0 / g_store64544c are NOT separate objects: 0x6453f0 is g_buteMgr + 0x18
// (== m_tree) and 0x64544c is g_buteMgr + 0x74 (== m_tree74) - two of the manager's three
// EMBEDDED zPTrees, aliased here as standalone CButeStore globals. (CButeStore IS zPTree;
// see <Bute/ButeStore.h>.) They stay declared for now only because ClearRecursive is
// modelled on CButeStore, not on zPTree - folding the two calls onto
// g_buteMgr.m_tree / .m_tree74 is the next step, and it clears 2 more undefined-DATA rows.
extern CButeStore g_store6453f0, g_store64544c; // == g_buteMgr.m_tree / .m_tree74

// This TU's own .bss scalar (0x645210 is outside every modelled object).
DATA(0x00245210)
i32 g_645210;

// ---------------------------------------------------------------------------
// SHREDDED-OBJECT FIX. Fourteen "globals" used to be DEFINED here as separate .bss
// scalars (the g_6453e5 / g_645404 / ... / g_6454e7 family) on the reasoning that this TU
// referenced them and nobody defined them. Every one is pinned STRICTLY INSIDE g_buteMgr
// (0x6453d8, size 0x110 -> 0x6453d8..0x6454e8): they are its INTERIOR FIELDS, so defining
// them fabricated fourteen .bss variables at interior offsets of one real object. The
// field map (from the compiler's own record layout) is exact:
//
//   0x6453e5 +0x00d  m_0d                        0x645460 +0x088  m_tree74.m_nodeCount
//   0x645404 +0x02c  m_tree.m_nodeCount          0x645464 +0x08c  m_tree74.m_root
//   0x645408 +0x030  m_tree.m_root               0x645474 +0x09c  m_tree74.m_lookupPending
//   0x645418 +0x040  m_tree.m_lookupPending      0x645478 +0x0a0  m_stream
//   0x645420 +0x048  m_tree48 (start; unused)    0x6454e6 +0x10e  m_10e
//   0x645434 +0x05c  m_tree48.m_nodeCount        0x6454e7 +0x10f  m_10f (CButeTail)
//   0x645438 +0x060  m_tree48.m_root
//   0x645448 +0x070  m_tree48.m_lookupPending
//
// The USE SITES prove it independently: the Phase-15 block zeroes three consecutive
// {m_root, m_lookupPending, m_nodeCount} triples, one per embedded zPTree, each right
// after that tree's ClearRecursive - i.e. zPTree::Reset inlined three times over
// g_buteMgr's trees. (0x645420 was written by nothing at all.)
//
// g_buteMgr ITSELF STAYS UNDEFINED, and NOT for the reason previously recorded here.
// The old note said defining it would emit a CRT dynamic-initialiser entry "whose order is
// the link order". Retail's dynamic-init table is 30 entries (rva 0x2096e4..0x20975c,
// NULL-bounded); I decoded all 30 and NONE constructs 0x6453d8 - and the address lies past
// .data's raw extent, so the loader zero-fills it. So retail does NOT dynamically construct
// g_buteMgr. But our CButeMgr models four CString members (+0x10, +0x100, +0x104, +0x108)
// and a CButeTail, which WOULD force a dyninit if we defined it. Those two facts cannot both
// be true: either the CString members are mis-modelled (a raw char*/POD string in retail), or
// g_buteMgr is placement-constructed at runtime like the CActReg registries are. That is the
// open question a definition must answer FIRST - defining it on today's model would fabricate
// a 31st initializer the binary does not have. Left undefined and honest.
// ---------------------------------------------------------------------------

SIZE(DecodeObj, 0x60);
struct DecodeObj {
    char raw[0x60];
    void* M169700(void*, i32, i32);      // 0x169700
    void* M169700b(void*, i32);          // 0x169700 (2-arg overload site)
    void* M1698c0(void*, i32, i32, i32); // 0x1698c0
};
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
    Utils::RegistryHelper* m_38;
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
    g_coordPool.m_block = pool;
    if (!pool) {
        Error2(0x800a, 0x404);
        return 0;
    }
    g_coordPool.m_count = 0x4e20; // 0x3a980 / sizeof(CoordNode) 0xc
    CoordNode* p = pool;
    u32 i = 0;
    do {
        p->m_next = p + 1;
        p = p->m_next;
        ++i;
    } while (i < (u32)g_coordPool.m_count - 1);
    p->m_next = 0;
    g_coordPool.m_freeHead = pool;
    g_coordPool.m_linkOffset = 4;

    // --- Phase 2: base game init + timers + cursor -------------------
    if (!Run(a1, a2)) {
        Error2(0x800a, 0x462);
        return 0;
    }
    if (!Fn2db0()) {
        Error2(0x800a, 0x463);
        return 0;
    }
    srand((::timeGetTime() + ::GetTickCount()) >> 1);
    g_wap32Run80 = 0x21;
    while (::ShowCursor(0) >= 0) {
    }

    // --- Phase 3: "Monolith Productions" registry --------------------
    Utils::RegistryHelper* reg = (Utils::RegistryHelper*)RezAlloc(0x21c);
    if (reg) {
        reg->m_open = 0;
    }
    m_38 = reg;
    if (!m_38->Open("Monolith Productions", "Gruntz", "1.0", 0, (HKEY)0x80000002, 0)) {
        Error2(0x800a, 0x406);
        return 0;
    }
    m_width = 0x280;
    m_height = 0x1e0;
    m_numRuns = m_38->GetValueDword("Num Runs", 0);
    m_numMovies = m_38->GetValueDword("Num Movies", 0);
    g_disableHqMovie = m_38->GetValueDword("Disable High Quality Movie", 0) ? 1 : 0;
    g_disableAudio = m_38->GetValueDword("Disable Audio", 0);
    g_disableSound = m_38->GetValueDword("Disable Sound", 0);
    g_disableMusic = m_38->GetValueDword("Disable Music", 0);
    g_disableFades = m_38->GetValueDword("Disable Fades", 0);
    g_disableDirectVideo = m_38->GetValueDword("Disable Direct Video Access", 0);
    g_disableJoystick = m_38->GetValueDword("Disable Joystick", 0);
    g_disableSoundFonts = m_38->GetValueDword("Disable SoundFonts", 0);
    g_enableTriple = m_38->GetValueDword("Enable Triple", 0);
    g_enableHiColor = m_38->GetValueDword("Enable HiColor", 0);
    g_enableTrueColor = m_38->GetValueDword("Enable TrueColor", 0);
    g_enableEmulation = m_38->GetValueDword("Enable Emulation", 0);
    m_checkpointPrompts = m_38->GetValueDword("Checkpoint Prompts", 1);
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
    i32 vMusic = m_38->GetValueDword("Music", m_music);
    i32 vSound = m_38->GetValueDword("Sound", m_sound);
    i32 vVoice = m_38->GetValueDword("Voice", m_voice);
    i32 vAmbient = m_38->GetValueDword("Ambient", m_ambient);
    i32 vInterlaced = m_38->GetValueDword("Interlaced", m_interlaced);
    i32 vHigh1 = m_38->GetValueDword("High Detail", m_highDetail);
    m_highDetail = m_38->GetValueDword("High Detail", m_110);
    i32 vEasy = m_38->GetValueDword("Easy Mode", m_118);
    i32 res = m_38->GetValueDword("Resolution", 1);
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
    i32 vMusVol = m_38->GetValueDword("Music Volume", 0x64);
    i32 vSndVol = m_38->GetValueDword("Sound Volume", 0x3c);
    i32 vVoiVol = m_38->GetValueDword("Voice Volume", 0x50);
    i32 vScroll = m_38->GetValueDword("Scroll Speed", 0x14);
    m_soundVolume = vSndVol;
    m_voiceVolume = vVoiVol;
    m_musicVolume = vMusVol + 1;
    m_numRuns = m_numRuns + 1;
    if (g_disableDirectVideo != 0) {
        g_disableFades = 1;
        g_enableEmulation = 1;
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
    i32 flags = (g_disableAudio || g_disableSound) ? 0xe5 : 0xe1;
    if (g_enableEmulation) {
        flags |= 0x10;
    }
    m_88 = 0x10;
    if (!m_30->Init(*(void**)((char*)m_04 + 4), 0x280, 0x1e0, 0x10, flags)) {
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
        m_30->m_resolveSubMgr->BuildAllPlanes((LevelCoordRect*)rect);
    }
    m_30->SetHwnd((void*)&cb_403193);
    m_30->m_resolveSubMgr->m_maxStepX = 0xe;
    m_30->m_resolveSubMgr->m_maxStepY = 0xe;
    m_30->m_pages->Method_158cb0(0, 0x30000);
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
    if (g_disableAudio == 0 && g_disableSoundFonts == 0) {
        if (Fn262b()) {
            if (!Fn2423cdecl(Fn4214())) {
                Fn129e();
            }
        }
    }

    // --- Phase 9: audio host (m_48) ---------------------------------
    m_48 = new CGruntzSoundZ;
    g_ailMidiDriver = 0;
    if (!m_48->Init(*(i32*)((char*)m_08 + 0xc), *(i32*)((char*)m_04 + 4), 0)) {
        Error2(0x800a, 0x40c);
        return 0;
    }
    if (g_disableAudio == 0 && g_disableMusic == 0) {
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
    if (!m_54->Init(m_30->m_leafScan, vSndVol)) {
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

    // --- Phase 12: the grunt spawn-config singleton (g_spawnConfig) -------
    g_spawnConfig = (CGruntSpawnConfig*)RezAlloc(0x28);
    if (g_spawnConfig) {
        i32* z = (i32*)g_spawnConfig;
        z[0] = z[1] = z[2] = z[4] = z[5] = 0;
    }
    if (!g_spawnConfig->Init((CSpawnOwner*)g_645570)) {
        if (g_spawnConfig) {
            i32* z = (i32*)g_spawnConfig;
            z[0] = z[1] = z[2] = z[4] = z[5] = 0;
            RezFree(g_spawnConfig);
            g_spawnConfig = 0;
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
    g_gameReg = this;
    g_lastNow = ::timeGetTime();
    g_frameDelta = 0;
    for (i32 s = 0; s < 4; ++s) {
        if (!ProbeSettings150(&m_150[s * 0x238])) {
            Error2(0x800a, 0x417);
            return 0;
        }
    }

    // --- Phase 15: GAME_ATTRIBUTEZ blowfish-decoded bute parse -------
    {
        CSymParser* mgr = m_34;
        i32 node = mgr->ResolveQualified("GAME_ATTRIBUTEZ", &g_lab545854);
        g_buteMgr.SetErrCallback((ErrCallback)&cb_401bc2);
        i32 ok = 0;
        if (node) {
            CParseSource* stream = (CParseSource*)node;
            g_buteMgr.m_10e = 1;
            i32 esz = stream->BeginParse();
            void* src = *(void**)((char*)stream + 0xc);
            DecodeObj* d0 = new DecodeObj;
            void* rdr = d0 ? d0->M169700(src, esz, 1) : 0;
            Blowfish_InitKey((unsigned char*)"1212C");
            DecodeObj* d1 = new DecodeObj;
            void* snk = d1 ? d1->M1698c0(src, esz, 2, 1) : 0;
            BitStreamBlowfishDecode(snk, rdr);
            DecodeObj* d2 = new DecodeObj;
            g_buteMgr.m_stream = (void*)d2; // m_stream is the manager's void* parse stream (+0xa0)
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
    g_localVersion = (i32)g_buteMgr.GetDwordDef("General", "RezSync", (u32)g_localVersion);
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
    if (!m_30->m_leafScan->HasKeyEqual_1583c0("GAME")) {
        void* sz = m_34->ResolvePath("GAME_SOUNDZ");
        if (!sz) {
            return 0;
        }
        m_30->m_leafScan->ScanTree_157ee0((CSymTab*)sz, "GAME", "_");
    }
    {
        void* mv = 0;
        m_30->m_leafScan->m_10.Lookup("GAME_MOVIE", mv);
        m_30->m_leafScan->MatchSub_1584f0((LeafCue*)mv, 0);
    }
    Fn1ed8();
    if (!Fn2112()) {
        if (m_numMovies > 0 && m_numRuns > 1) {
            if (m_38->GetValueDword("Skip Logo Movies", 0) == 0 && noLogo == 0) {
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
        g_attractStateCount = 0;
        title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        while (attract->ResolveTab((const char*)*(void**)&title, &g_lab504358)) {
            g_attractStateCount++;
            title.Format("\\SCREENZ\\TITLE%d", g_attractStateCount + 1);
        }
        if (Fn12d0(mode, 1, 0, 0)) {
            g_frameDelta = 0;
        } else if (mode == 0x11 && Fn12d0(2, 1, 0, 0)) {
            g_frameDelta = 0;
        } else {
            Error2(0x8005, mode == 0x11 ? 0x41c : 0x41d);
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x0853d0 (RVA-homed from src/Stub/BoundaryThunks.cpp) - __stdcall forwarder: hand
// the single arg to the __cdecl rez-free helper (0x1b9b82). Returns void; one arg
// (ret 4). A standalone forwarder with no owning class.
RVA(0x000853d0, 0x10)
void __stdcall RezFreeStdcall(void* a) {
    ::operator delete(a); // 0x1b9b82 == ??3@YAXPAX@Z (the __cdecl rez-free helper)
}

// 0x85500 (re-homed from src/Stub/BoundaryTail.cpp): return a CString member (offset
// 0xec) BY VALUE. Called by RezSync::Init (above, RVA-contiguous 0x83450) and
// CGruntzMgr::LoadWorldMode; `this` identity genuinely unrecovered (kept placeholder
// Obj85500 in BoundaryTailViews.h).
// @early-stop
// /O2 dead-local wall (~74%): the copyctor-into-retslot logic is exact, but retail
// reserves + zeroes one extra stack dword (`push reg; mov [slot],0`) that our /O2
// elides as dead. Confirmed NOT /O1 (o1 profile scored 40%). The origin of the kept
// zero store is not yet spellable; the copy itself is byte-exact.
RVA(0x00085500, 0x23)
CString Obj85500::GetName() {
    return m_ec;
}

// ---------------------------------------------------------------------------
// 0x085540 (RVA-homed from src/Stub/BoundaryLowerThunks.cpp) - the base CGameMgr
// destructor: restamp the base vtable (0x5e9b8c) then tail-call the base teardown
// (0x13ddb0 == Close). __thiscall. The delinker names its vtable ??_7CGameMgr@@6B@
// globally (config/vtable_names.csv), so cl's implicit entry vptr-store emits it
// (masks 0x5e9b8c). This is the GLOBAL-namespace ??1CGameMgr@@ that retail keeps at
// 0x85540; the reconstruction's real manager is modeled as the namespaced
// WAP32::CGameMgr (to disambiguate CGruntzMgr), so this distinct global placeholder is
// the byte-necessary out-of-line emitter. RVA-contiguous with RezSync::Init (this TU
// is the CGameMgr bootstrap). (The scalar-deleting twin 0x855a0 was a separate inline
// dtor COMDAT emitter in the now-deleted src/Stub/BoundaryLowerThunks.cpp; it was
// dropped - it can only be emitted by an inline dtor whose ??_G inlines it, which
// conflicts with this TU's out-of-line ~CGameMgr, so no clean single-TU home exists.)
// The real 0x2c engine base whose Close (0x13ddb0) the placeholder dtor tail-calls;
// aliased at file scope so the qualified call avoids MSVC 5.0's class-scope lookup of
// the WAP32 namespace inside the same-named global placeholder below.
typedef WAP32::CGameMgr CGameMgrBase;

struct CGameMgr {
    virtual ~CGameMgr(); // 0x85540 (slot 0): implicit base-vtable restamp + Close
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
};
SIZE_UNKNOWN(CGameMgr);
// The vptr restamp at +0x2 is the cl-emitted ??_7CGameMgr@@6B@ of THIS global
// placeholder, reloc-MASKING the real WAP32::CGameMgr vtable at 0x1e9b8c (bound in
// gruntzmgr as ??_7CGameMgr@WAP32@@6B@). It cannot take a VTBL() row at 0x1e9b8c - that
// rva already carries the WAP32 name, and the two would keep-last-collide - and making
// this dtor a real WAP32::CGameMgr method would collide with WAP32::CGameMgr's INLINE
// header dtor (load-bearing for CGruntzMgr's inlined base teardown - a cross-unit
// regression). RELOC_VTBL records exactly that: the placeholder's vtable IS the retail
// vtable at 0x1e9b8c, so the vptr-store REFERENCE is bound to the right rva (no dangling
// reloc) while the symbol_names row stays the real WAP32 name.
// @identity-TODO: this global-namespace CGameMgr IS WAP32::CGameMgr (0x85540 is the
// out-of-line COMDAT copy of its inline dtor, emitted for the vtable slot); dissolving it
// needs the inline-XOR-out-of-line dtor conflict resolved first.
RELOC_VTBL(CGameMgr, 0x001e9b8c); // == ??_7CGameMgr@WAP32@@6B@ (the real base vtable)
RVA(0x00085540, 0xb)
CGameMgr::~CGameMgr() {
    // devirtualized tail-call to WAP32::CGameMgr::Close (0x13ddb0); the qualified call
    // binds the reloc directly to ?Close@CGameMgr@WAP32@@UAEXXZ (this@+0 == the base).
    ((CGameMgrBase*)this)->CGameMgrBase::Close();
}
