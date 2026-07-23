#include <Mfc.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <Gruntz/GameText.h>
#include <rva.h>
#include <Bute/ButeMgr.h>         // the one CButeMgr (its 0x170210 ctor; the 0x82b20 in-place init)
#include <Gruntz/Attract.h>       // g_attractStateCount C-linkage declaration
#include <Gruntz/FreeNodePool.h>  // g_coordPool (the 0x82fa0/0x82ff0 coord-pool reset/clear tail)
#include <Gruntz/MgrAutoScroll.h> // g_panMinX/g_panMaxX declarations
#include <Gruntz/Play.h>          // g_areaHazardParam C-linkage declaration
#include <Rez/RezSync.h>          // g_dlgVal_645538 declaration

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
// Each CString table emits the complete wrapper/initializer/atexit/array-destructor
// family. The `<n>` suffix is an unstable per-object counter, so the helpers are
// content-addressed by canonicalize_data_symbols (paired by body and relocations,
// not number). This TU's .text is multi-region, hence the distant family RVAs.
RVA_COMPGEN(0x00018720, 0xa, _$E100128)
RVA_COMPGEN(0x00018740, 0x79, _$E100160)
RVA_COMPGEN(0x000187e0, 0xe, _$E100320)
RVA_COMPGEN(0x00018800, 0x14, _$E100352)

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

RVA_COMPGEN(0x00082970, 0xa, _$E534896)
RVA_COMPGEN(0x00082990, 0x79, _$E534928)
RVA_COMPGEN(0x00082a30, 0xe, _$E535088)
RVA_COMPGEN(0x00082a50, 0x14, _$E535120)
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

DATA(0x002453d8)
CButeMgr g_buteMgr;
RVA_COMPGEN(0x00082b00, 0xa, _$E535296)
RVA_COMPGEN(0x00082b20, 0xa, _$E535328)
RVA_COMPGEN(0x00082b40, 0xe, _$E535360)
RVA_COMPGEN(0x00082b60, 0xa, _$E535392)

DATA(0x00245508)
i32 g_panMinX;
DATA(0x0024550c)
i32 g_panMaxX;

DATA(0x00245524)
CString g_brickText1;
RVA_COMPGEN(0x00082b80, 0xa, _$E535424)
RVA_COMPGEN(0x00082ba0, 0xa, _$E535456)
RVA_COMPGEN(0x00082bc0, 0xe, _$E535488)
RVA_COMPGEN(0x00082be0, 0xa, _$E535520)

DATA(0x00245528)
CString g_brickText2;
RVA_COMPGEN(0x00082c00, 0xa, _$E535552)
RVA_COMPGEN(0x00082c20, 0xa, _$E535584)
RVA_COMPGEN(0x00082c40, 0xe, _$E535616)
RVA_COMPGEN(0x00082c60, 0xa, _$E535648)

DATA(0x0024552c)
CString g_str64552c;
RVA_COMPGEN(0x00082c80, 0xa, _$E535680)
RVA_COMPGEN(0x00082ca0, 0xa, _$E535712)
RVA_COMPGEN(0x00082cc0, 0xe, _$E535744)
RVA_COMPGEN(0x00082ce0, 0xa, _$E535776)

DATA(0x00245530)
CString g_str645530;
RVA_COMPGEN(0x00082d00, 0xa, _$E535808)
RVA_COMPGEN(0x00082d20, 0xa, _$E535840)
RVA_COMPGEN(0x00082d40, 0xe, _$E535872)
RVA_COMPGEN(0x00082d60, 0xa, _$E535904)

DATA(0x00245514)
CString g_str645514;
RVA_COMPGEN(0x00082d80, 0xa, _$E535936)
RVA_COMPGEN(0x00082da0, 0xa, _$E535968)
RVA_COMPGEN(0x00082dc0, 0xe, _$E536000)
RVA_COMPGEN(0x00082de0, 0xa, _$E536032)

DATA(0x00245518)
CString g_str645518;
RVA_COMPGEN(0x00082e00, 0xa, _$E536064)
RVA_COMPGEN(0x00082e20, 0xa, _$E536096)
RVA_COMPGEN(0x00082e40, 0xe, _$E536128)
RVA_COMPGEN(0x00082e60, 0xa, _$E536160)

DATA(0x0024551c)
CString g_str64551c;
RVA_COMPGEN(0x00082e80, 0xa, _$E536192)
RVA_COMPGEN(0x00082ea0, 0xa, _$E536224)
RVA_COMPGEN(0x00082ec0, 0xe, _$E536256)
RVA_COMPGEN(0x00082ee0, 0xa, _$E536288)

DATA(0x00245520)
CString g_str645520;
RVA_COMPGEN(0x00082f00, 0xa, _$E536320)
RVA_COMPGEN(0x00082f20, 0xa, _$E536352)
RVA_COMPGEN(0x00082f40, 0xe, _$E536384)
RVA_COMPGEN(0x00082f60, 0xa, _$E536416)

DATA(0x00245534)
i32 g_attractStateCount = 0;
DATA(0x00245538)
i32 g_dlgVal_645538;
DATA(0x0024553c)
i32 g_areaHazardParam = 0;

DATA(0x00245540)
FreeNodePool g_coordPool;

RVA_COMPGEN(0x00082f80, 0xa, _$E536448)
RVA_COMPGEN(0x00082fa0, 0x17, _$E536480)
RVA_COMPGEN(0x00082fd0, 0xe, _$E536528)
RVA_COMPGEN(0x00082ff0, 0x2f, _$E536560)

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
