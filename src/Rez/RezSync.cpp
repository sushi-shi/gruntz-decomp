// RezSync.cpp - CGruntzMgr::Run(CGameWnd*, char* cmdLine) @ RVA 0x00083450
// (6445 B), the boot override of WAP32::CGameMgr::Run - SLOT-PROVEN: CGruntzMgr's
// retail vtable (0x1e9b64) slot 1 is ILT thunk 0x249b -> jmp 0x83450. `this` (ebp)
// is the 0xa30 CGruntzMgr singleton; returns 1 on success / 0 on every early error
// 2026-07-16 - every field was a canonical <Gruntz/GruntzMgr.h> member.)
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
#include <Rez/RezSyncViews.h> // the global ::CGameMgr out-of-line-dtor emitter (scaffolding)
#include <rva.h>
#include <Ints.h>
#include <Mfc.h> // CString + the MFC collection ctors/dtors (reloc-masked)
// AfxWinInit (the boot MFC init Run calls @0x1d3eff). The afxwin*.inl bodies are
// skipped for the clang LABEL step only (implicit-int CMenu::operator== that clang
// rejects, wine cl accepts) - docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <Wap32/Wap32.h>              // WAP32::CGameMgr (the base ~CGameMgr tail-calls its Close)
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
#include <Globals.h>

// retail's global allocator is the nothrow pool alloc; MSVC5 `new T` emits the
// post-alloc null-check + ctor-in-flight EH-state store the retail body has.
void* operator new(unsigned int);
void operator delete(void*);

// ---------- reloc-masked engine globals ----------
// The coord-node recycle pool - ONE 16-byte FreeNodePool object at 0x645540, NOT four
// scalars. Init below builds it (block / count / free-list head / link offset); the four
// former per-field globals (g_coordPool/g_coordFreeList/g_coordCount/g_freeBias) were
// separate fabricated symbols at 0x645540/44/48/4c - interior addresses of this object,
// so they could never resolve at link. Defined in src/Gruntz/GameText.cpp (owner TU).
// 0x24556c is the game-registry singleton g_gameReg (DEFINED in GruntzMgr.cpp). The
// assignment below is CGruntzMgr::Init SELF-REGISTERING - which is exactly what proves
// the cell holds this manager. Spelling it `g_mgrPtr` with C++ linkage made a second,
// unresolvable symbol (?g_mgrPtr@@3PAXA) for a cell 50 other TUs already share.
extern "C" void* g_gameReg; // 0x64556c (typed CGruntzMgr* in its owner TU)
// g_lastNow (0x245580, the frame-clock "now" cell) is declared in <Rez/FrameClock.h>;
// Init seeds it with the boot timeGetTime().
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
// emitted ?g_inputMgr@@3PAXA - a divergent symbol for the same address, unresolved on both
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
    extern void* g_inputMgr;
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

// The real 0x2c engine base, aliased at file scope: MSVC 5.0's class-scope lookup
// of the WAP32 namespace misparses a qualified `WAP32::CGameMgr::` call inside a
// member function (both Run's base call and the placeholder ~CGameMgr below need it).
typedef WAP32::CGameMgr CGameMgrBase;

extern "C" void cb_403193();
extern "C" void cb_401bc2();
extern char g_lab504358[]; // 0x504358
extern char g_lab545854[]; // 0x545854

void __stdcall Blowfish_InitKey(unsigned char*);      // 0x16f6c0
void __stdcall BitStreamBlowfishDecode(void*, void*); // 0x16f760

// real MFC methods - C_1b9c69 == CString::Empty (FID-anchored NAFXCW) on
// m_strWorldFile, and D_1b48c6 == ~CPtrList on the sound set's m_list.)

// <DDrawMgr/DDrawSurfaceMgr.h>: VInit was the slot-6 Init, VMethod155f50 is
// SetHwnd, m_04/m_24/m_28 are m_drawTarget/m_level/m_soundRegistry. The local
// CSymParser/CFaderMgr re-declarations resolve to <Bute/SymParser.h> /
// <Gruntz/FaderMgr.h>, and the 0x94-byte H70 (m_tileGrid) IS the canonical
// CGruntzMapMgr - the RTTI-proven +0x70 board whose teardown thunk 0x35b7 is
// ~CGruntzMapMgr @0x85d10. m_sound is the audio host = canonical CGruntzSoundZ
// (Init @0x138490 / SetXMidiVolume @0x138950 / m_enabled @+0x28).)

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
i32 g_appHInstance;

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

// ctors - 0x169700 == ??0istrstream@@QAE@PADH@Z and 0x1698c0 ==
// ??0ostrstream@@QAE@PADHH@Z (both FID-anchored LIBCIMT); the trailing `1` arg at
// each retail call site is the virtual-base most-derived ctor flag. The whole
//   Init          == this Run override (vtable slot 1 -> 0x83450)
//   Run(0x13dd50) == WAP32::CGameMgr::Run (the base call)
//   Error2 0x346d == ReportError            Error1 0x3f80 == ReportWorldStatus
//   Fn2db0  -> 0x115810 InitializeFonts     Fn4214 -> 0x8fa70 GetGruntzDriveLetter
//   Fn2112  -> 0x8eca0 InitializeLobbyConnectionSettings
//   Fn1db6  -> 0x8f7f0 RecomputeViewScale (the "Step1db6" duplicate id)
//   Fn1c12  -> 0x91670 MakeRezPath (CGruntzMgr's own; RezMgr.cpp - facet fold DONE)
//   Fn1df7  -> 0x91170 SetColorDepth        Fn262b -> 0xf8970 SFManager_SelectBestDevice
//   Fn129e  -> 0xf8e20 CloseSoundFontDevice Fn2423 -> 0xf8f30 BuildSoundFontPath
//   Fn2cc5  -> 0x90200 RunFromState         Fn201d -> 0x8fab0 ChangeState_8fab0
//   Fn1ed8  -> 0x90aa0 CheckMovieFileExists Fn12d0 -> 0x8b960 TransitionState
//   Fn320b  -> 0x85500 GetRezPath           Fn3526 -> 0xa3b0 RegisterGameObjectTypes
//   Fn40c0  -> 0x919d0 SetSoundVolume       Fn4174 -> 0x91a10 SetVoiceVolume
//   Fn1d3eff = 0x1d3eff AfxWinInit (NAFXCW) ProbeSettings150 -> 0xda870
//                                           GruntzPlayer::SeedForSlot(i32)
// )

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
    CoordPoolNode* pool = (CoordPoolNode*)RezAlloc(0x3a980);
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
    if (!CGameMgrBase::Run(pGameWnd, szCmdLine)) { // the base WAP32::CGameMgr::Run (0x13dd50)
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
    Utils::RegistryHelper* reg = (Utils::RegistryHelper*)RezAlloc(0x21c);
    if (reg) {
        reg->m_open = 0;
    }
    m_settings = reg;
    if (!m_settings->Open("Monolith Productions", "Gruntz", "1.0", 0, (HKEY)0x80000002, 0)) {
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
    m_world = (CDDrawSurfaceMgr*)world;
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
        world->m_level->BuildAllPlanes((LevelCoordRect*)rect);
    }
    world->SetHwnd((void*)&cb_403193);
    world->m_level->m_maxStepX = 0xe;
    world->m_level->m_maxStepY = 0xe;
    world->m_drawTarget->Method_158cb0(0, 0x30000);
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
        i32 ok = m_symParser->ParseBuffer(*(void**)&fn, 1, 0) != 0;
        if (!ok) {
            ReportError(0x800b, 0x409);
            return 0;
        }
    }
    if (!m_symParser->LoadEntry(const_cast<char*>("GRUNTZ.VRZ"), 0)) { // 0x13b0c0 (canonical arg order)
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
    if (!m_sound->Init(reinterpret_cast<i32>(m_owner->m_hInstance), reinterpret_cast<i32>(m_gameWnd->m_hwnd), 0)) {
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
    m_logicPump = (CLightFxMgr*)RezAlloc(0x3c);
    if (m_logicPump) {
        i32* z = reinterpret_cast<i32*>(m_logicPump);
        z[1] = z[2] = z[3] = z[4] = 0;
        for (i32 k = 0; k < 10; ++k) {
            *(i32*)(reinterpret_cast<char*>(m_logicPump) + 0x14 + k * 4) = 0;
        }
    }
    if (!m_logicPump->Init(0, static_cast<void*>(this))) {
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
    m_scoreHud->InitWithRecords(reinterpret_cast<char*>(m_saveSink) + 0x24);

    // --- Phase 12: the grunt spawn-config singleton (g_spawnConfig) -------
    g_spawnConfig = (CGruntSpawnConfig*)RezAlloc(0x28);
    if (g_spawnConfig) {
        i32* z = (i32*)g_spawnConfig;
        z[0] = z[1] = z[2] = z[4] = z[5] = 0;
    }
    if (!g_spawnConfig->Init((CSpawnOwner*)g_inputMgr)) {
        if (g_spawnConfig) {
            i32* z = (i32*)g_spawnConfig;
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
    m_spriteFactory = (CSpriteRefTable*)RezAlloc(0x94);
    if (m_spriteFactory) {
        i32* z = reinterpret_cast<i32*>(m_spriteFactory);
        z[0] = z[1] = 0;
        *(i32*)(reinterpret_cast<char*>(m_spriteFactory) + 0x90) = 0;
        for (i32 k = 0; k < 0x11; ++k) {
            *(i32*)(reinterpret_cast<char*>(m_spriteFactory) + 8 + k * 4) = 0;
            *(i32*)(reinterpret_cast<char*>(m_spriteFactory) + 0x4c + k * 4) = 0;
        }
    }
    if (!(reinterpret_cast<CTriggerMgr*>(m_spriteFactory))->SetLevel(reinterpret_cast<CDDrawSurfaceMgr*>(m_shadeCache))) {
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
        i32 node = mgr->ResolveQualified("GAME_ATTRIBUTEZ", &g_lab545854);
        g_buteMgr.SetErrCallback((ErrCallback)&cb_401bc2);
        i32 ok = 0;
        if (node) {
            CParseSource* stream = (CParseSource*)node;
            g_buteMgr.m_10e = 1;
            i32 esz = stream->BeginParse();
            void* src = *(void**)(reinterpret_cast<char*>(stream) + 0xc);
            istrstream* rdr = new istrstream(static_cast<char*>(src), esz); // ??0istrstream (0x169700)
            Blowfish_InitKey((unsigned char*)"1212C");
            ostrstream* snk = new ostrstream(static_cast<char*>(src), esz, 2); // ??0ostrstream (0x1698c0)
            BitStreamBlowfishDecode(snk, rdr);
            // carcass gap: retail allocs a third 0x60 stream object here with no
            // visible ctor call in the recovered bytes; kept as the bare allocation.
            g_buteMgr.m_stream = (istream*)::operator new(0x60);
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
    g_localVersion = static_cast<i32>(g_buteMgr.GetDwordDef("General", "RezSync", static_cast<u32>(g_localVersion)));
    m_timer = new CGruntSpawnConfig;
    if (!m_timer->Init(reinterpret_cast<CSpawnOwner*>(this))) {
        ReportError(0x800a, 0x45f);
        return 0;
    }
    *(i32*)(reinterpret_cast<char*>(m_timer) + 0x2c) = vScroll;
    m_musicEnabled = vMusic;
    m_soundEnabled = vSound;
    g_sndEnabled = vSound;
    m_isVoiceEnabled = vVoice;
    m_isAmbientEnabled = vAmbient;
    m_isInterlaced = vInterlaced;
    m_isHighDetail = vHigh1;
    m_isEffectsEnabled = vEasy;
    if (!m_world->m_soundRegistry->HasKeyEqual_1583c0("GAME")) {
        void* sz = m_symParser->ResolvePath("GAME_SOUNDZ");
        if (!sz) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree_157ee0((CSymTab*)sz, "GAME", "_");
    }
    {
        void* mv = 0;
        m_world->m_soundRegistry->m_10.Lookup("GAME_MOVIE", mv);
        m_world->m_soundRegistry->MatchSub_1584f0((LeafCue*)mv, 0);
    }
    CheckMovieFileExists();
    if (!InitializeLobbyConnectionSettings()) {
        if (m_numMovies > 0 && m_numRuns > 1) {
            if (m_settings->GetValueDword("Skip Logo Movies", 0) == 0 && noLogo == 0) {
                RunFromState();
            }
        } else {
            RunFromState();
            if (ChangeState_8fab0(2)) {
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
        while (attract->ResolveQualified(static_cast<const char*>(*(void**)&title), &g_lab504358)) {
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

// ---------------------------------------------------------------------------
// 0x0853d0 (RVA-homed from src/Stub/BoundaryThunks.cpp) - __stdcall forwarder: hand
// the single arg to the __cdecl rez-free helper (0x1b9b82). Returns void; one arg
// (ret 4). A standalone forwarder with no owning class.
RVA(0x000853d0, 0x10)
void __stdcall RezFreeStdcall(void* a) {
    ::operator delete(a); // 0x1b9b82 == ??3@YAXPAX@Z (the __cdecl rez-free helper)
}

// 0x85500 - CGruntzMgr::GetRezPath: return the +0xec assembled-REZ-path CString BY
// VALUE. IDENTITY RECOVERED (ex the Obj85500 placeholder): both callers - Run above
// and LoadWorldMode - invoke it with ecx == the manager `this`, and +0xec is the
// mgr's m_strRezPath (ex the RezMgr view's "m_pathA - assembled archive path #1",
// the slot MakeRezPath fills). 100% EXACT once folded onto the real class - the old "/O2
// dead-local wall" (~74%) was the misattributed Obj85500 view's artifact, not a wall.
RVA(0x00085500, 0x23)
CString CGruntzMgr::GetRezPath() {
    return m_strRezPath;
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
// The global-namespace ::CGameMgr placeholder (the byte-necessary out-of-line emitter of
// the ??1CGameMgr@WAP32@@ dtor below) is defined in <Rez/RezSyncViews.h> (included above):
// genuine inline-XOR-out-of-line dtor-wall scaffolding - it IS WAP32::CGameMgr but its
// out-of-line dtor here would collide with WAP32::CGameMgr's load-bearing inline dtor. The
// RELOC_VTBL there binds the emitted vptr restamp to the retail ??_7CGameMgr@WAP32@@ (0x1e9b8c).
RVA(0x00085540, 0xb)
CGameMgr::~CGameMgr() {
    // devirtualized tail-call to WAP32::CGameMgr::Close (0x13ddb0); the qualified call
    // binds the reloc directly to ?Close@CGameMgr@WAP32@@UAEXXZ (this@+0 == the base).
    (reinterpret_cast<CGameMgrBase*>(this))->CGameMgrBase::Close();
}
