#include <Mfc.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <Gruntz/GameText.h>
#include <rva.h>
#include <Bute/ButeMgr.h>        // the one CButeMgr (its 0x170210 ctor; the 0x82b20 in-place init)
#include <Gruntz/Attract.h>      // g_attractStateCount C-linkage declaration
#include <Gruntz/FreeNodePool.h> // g_coordPool (the 0x82fa0/0x82ff0 coord-pool reset/clear tail)
#include <Gruntz/Play.h>         // g_areaHazardParam C-linkage declaration
#include <Rez/RezSync.h>         // g_dlgVal_645538 declaration

// ---------------------------------------------------------------------------
// The two name tables are file-scope arrays of CString with brace-initializers.
// MSVC5 /O2 emits ONE dynamic-initializer per array - a flat run of
// `push lit; mov ecx,&g[i]; call CString::CString(const char*)` with NO placement-new
// null-check, ending in `ret` - which is EXACTLY the shape (and 0x79-byte size)
// of the target functions tomalla guessed as "GetWorldDisplayName" /
// "GetEndLevelStatLabels". They are in fact the compiler-generated array static
// initializers (MSVC local `_$E` thunks); pinned to their RVAs by @symbol.
//
// The 8 world display names, by world id 0..7:
//   "Rocky Roadz" / "Gruntziclez" / "Trouble in the Tropicz" / "High on Sweetz"
//   / "High Rollerz" / "Honey, I Shrunk the Gruntz!" / "The Miniature Masterz"
//   / "Gruntz in Space".
// ---------------------------------------------------------------------------
// The two file-scope CString name tables (g_worldName below, g_statLabel further down)
// each emit a compiler-generated dynamic-init `$E<n>` funclet that runs the 8
// CString::CString ctors at startup. The `<n>` is an unstable per-object counter, so the
// funclets are content-addressed by canonicalize_data_symbols (paired by body, not
// number); both pins live here, co-located in RVA order for the compgen_order ratchet
// (this TU's .text is multi-region, so the two funclets' addresses are far apart).
RVA_COMPGEN(0x00018740, 0x79, _$E4) // g_statLabel[8] initializer
RVA_COMPGEN(0x00082990, 0x79, _$E1) // g_worldName[8] initializer
static CString g_worldName[8] = {
    "Rocky Roadz",
    "Gruntziclez",
    "Trouble in the Tropicz",
    "High on Sweetz",
    "High Rollerz",
    "Honey, I Shrunk the Gruntz!",
    "The Miniature Masterz",
    "Gruntz in Space",
};

VTBL(zErrHandling, 0x001f04cc); // ??_7CContainerErr@@6B@ - ONE slot (the dtor)
DATA(0x002451a8)
CWinApp g_gruntzWinApp("Gruntz");
RVA_COMPGEN(0x00082a80, 0xa, _$E535168)
RVA_COMPGEN(0x00082aa0, 0x10, _$E535200)
RVA_COMPGEN(0x00082ac0, 0xe, _$E535232)
RVA_COMPGEN(0x00082ae0, 0xa, _$E535264)

// The resource-config bute manager @0x6453d8 (RVA 0x2453d8). Tree-wide it is the
// CButeMgr singleton g_buteMgr (?g_buteMgr@@3VCButeMgr@@A, DATA-bound in FontConfig.cpp,
// read by Projectile/DoNothing/... via GetInt/GetDword); this thunk constructs it in
// place through the real CButeMgr ctor (0x170210, ButeSectionCtor.cpp). Bind the reloc
// to the canonical g_buteMgr symbol (NOT a private g_resButeMgr - that leaves it
// UNBOUND). (The @identity-TODO here is RESOLVED 2026-07-19: CButeMgr and the ex
// "CButeSection" were the same 280-B config object; the twin class is dissolved and
// the (CButeSection*) conflation cast fell out with it.)
RVA(0x00082b20, 0xa)
void InitResButeMgr() {
    (&g_buteMgr)->CButeMgr::CButeMgr();
}

DATA(0x00245524)
CString g_brickText1;
DATA(0x00245528)
CString g_brickText2;
DATA(0x0024552c)
CString g_str64552c;
DATA(0x00245530)
CString g_str645530;

RVA(0x00082ba0, 0xa)
void InitStr645524() {
    g_brickText1.CString::CString();
}
RVA(0x00082c20, 0xa)
void InitStr645528() {
    g_brickText2.CString::CString();
}
RVA(0x00082ca0, 0xa)
void InitStr64552c() {
    g_str64552c.CString::CString();
}
RVA(0x00082d20, 0xa)
void InitStr645530() {
    g_str645530.CString::CString();
}

DATA(0x00245514)
CString g_str645514;
DATA(0x00245518)
CString g_str645518;
DATA(0x0024551c)
CString g_str64551c;
DATA(0x00245520)
CString g_str645520;

RVA(0x00082da0, 0xa)
void InitStr645514() {
    g_str645514.CString::CString();
}
RVA(0x00082e20, 0xa)
void InitStr645518() {
    g_str645518.CString::CString();
}
RVA(0x00082ea0, 0xa)
void InitStr64551c() {
    g_str64551c.CString::CString();
}
RVA(0x00082f20, 0xa)
void InitStr645520() {
    g_str645520.CString::CString();
}

DATA(0x00245534)
i32 g_attractStateCount = 0;
DATA(0x00245538)
i32 g_dlgVal_645538;
DATA(0x0024553c)
i32 g_areaHazardParam = 0;

DATA(0x00245540)
FreeNodePool g_coordPool;

RVA(0x00082fa0, 0x17)
void ResetCoordPool() {
    g_coordPool.m_block = 0;
    g_coordPool.m_freeHead = 0;
    g_coordPool.m_count = 0;
    g_coordPool.m_linkOffset = 0;
}

RVA(0x00082ff0, 0x2f)
void ClearCoordPool() {
    if (g_coordPool.m_block != 0) {
        // The retail call @0x82ffd targets 0x1b9b82 == ??3@YAXPAX@Z (MFC operator
        // delete, library row), NOT a RezFree wrapper - bind to the real symbol.
        ::operator delete(g_coordPool.m_block);
    }
    g_coordPool.m_block = 0;
    g_coordPool.m_freeHead = 0;
    g_coordPool.m_count = 0;
    g_coordPool.m_linkOffset = 0;
}

static CString g_statLabel[8] = {
    "Time:",
    "Survivorz:",
    "Deathz:",
    "Toolz:",
    "Toyz:",
    "Powerupz:",
    "Coinz:",
    "Secretz:",
};

RVA(0x0001ec20, 0x8d)
CString __stdcall GetWarlordName(i32 id) {
    // The target reserves and zero-inits one dead stack dword (`push ecx; mov
    // [esp+4],0; ...; pop ecx`) that no path reads - an MSVC5 return-slot/NRV
    // bookkeeping artifact. A `volatile int = 0` reproduces it exactly (the
    // zero-init survives DCE without emitting an address-store; scheduled after
    // the cmp, matching the target's `mov [esp+4],0`).
    volatile i32 slot = 0;
    switch (id) {
        case 0:
            return CString("KING");
        case 1:
            return CString("NAPOLEAN");
        case 2:
            return CString("PATTON");
        case 3:
            return CString("VIKING");
        default:
            return CString("");
    }
}

static char* g_errMsg_OutOfMem; // the lazy-init guard slot
static char* g_errMsg_BadData;
static char* g_errMsg_Overflow;
static char* g_errMsg_NoFile;
static char* g_errMsg_OutOfRng;
static char* g_errMsg_Exists;
static char* g_errMsg_NullArg;
static char* g_errMsg_BadArg;

RVA(0x0016d9c0, 0x75)
zErrHandling::zErrHandling(CVariantSlot* errSink) {
    // +0x04 stored first, the vptr after it (cl's implicit stamp). The arg is the sink to
    // register with, not a string: ~zErrHandling loads +0x04 into ecx as a __thiscall
    // `this` (see <Wap32/zBitVec.h>). A null argument selects the constructed global sink.
    m_errSink = errSink ? errSink : &g_globalErrorSlot;

    if (g_errMsg_OutOfMem == 0) {
        g_errMsg_OutOfMem = "Out of memory";
        g_errMsg_BadData = "Data structure is invalid";
        g_errMsg_Overflow = "Overflow";
        g_errMsg_NoFile = "No such file, handle or object";
        g_errMsg_OutOfRng = "Out of range";
        g_errMsg_Exists = "Target alrready exisits";
        g_errMsg_NullArg = "Null pointer argument";
        g_errMsg_BadArg = "Bad argument value";
    }
}
