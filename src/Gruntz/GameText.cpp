// GameText.cpp - small name/string lookup + table-init leaves.
//
// These are the human-readable name/label tables the UI reads. Recovered from
// dump_target.py (real bytes + relocs). The string literals become file-scope
// .rdata consts (reloc-masked); only the per-fn code bytes are load-bearing.
#include <Gruntz/GameText.h>
#include <rva.h>
#include <Wap32/ZDArrayDerived.h> // CZDArrayDerived::Construct (the 0x82aa0 register thunk)
#include <Globals.h>              // g_desc60aac8 (the registered descriptor)
#include <Bute/ButeSection.h>     // real CButeSection (the 0x82b20 in-place ctor)
#include <Gruntz/FreeNodePool.h>  // g_coordPool (the 0x82fa0/0x82ff0 coord-pool reset/clear tail)

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
// GetWorldDisplayName (the g_worldName[] array initializer).
SYMBOL(_$E1)
RVA(0x00082990, 0x79)
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

// ---------------------------------------------------------------------------
// 0x082aa0 (RVA-homed from src/Stub/BoundaryLowerThunks.cpp) - register thunk: hand
// the address of a global descriptor (0x60aac8) to a manager singleton (0x6451a8)
// method (0x1d38c5 == CZDArrayDerived::Construct, __thiscall). __cdecl. RVA-contiguous
// with the g_worldName initializer @0x82990.
// @orphan: COMDAT-folded one-liner whose owning class is unrecovered.
struct CMgr6451a8 {
    // Register @0x3742 IS CZDArrayDerived::Construct (2nd arg reloc-masked); cast at the call.
};
SIZE_UNKNOWN(CMgr6451a8);
DATA(0x002451a8)
extern CMgr6451a8 g_mgr6451a8;
RVA(0x00082aa0, 0x10)
void Register82aa0() {
    ((CZDArrayDerived*)&g_mgr6451a8)->Construct((i32)(void*)&g_desc60aac8, 0);
}

// ---------------------------------------------------------------------------
// 0x082b20..0x082d20 (RVA-homed from src/Stub/BoundaryThunks.cpp) - compiler-
// generated dynamic-initializer (_$E) thunks for file-scope globals of the
// resource-config / debug-overlay subsystem, RVA-contiguous with the g_worldName
// initializer @0x82990. Each is a tail-jmp `mov ecx,&g; jmp <ctor>` - the object's
// in-place default construction (docs/patterns/explicit-ctor-call-inplace-tail-jmp.md).
// The callee 0x1b9b93 IS CString::CString() (NAFXCW default ctor, NOT the dtor);
// 0x170210 is CButeSection::CButeSection(). All callees + global addresses are
// reloc-masked, so only the OFFSETS + code bytes are load-bearing.
// ---------------------------------------------------------------------------

// The resource-config bute manager @0x6453d8 (RVA 0x2453d8). Tree-wide it is the
// CButeMgr singleton g_buteMgr (?g_buteMgr@@3VCButeMgr@@A, DATA-bound in FontConfig.cpp,
// read by Projectile/DoNothing/... via GetInt/GetDword); its dynamic init here constructs
// it in place through the CButeSection ctor (0x170210 == CButeSection::CButeSection). Bind
// the reloc to the canonical g_buteMgr symbol (NOT a private g_resButeMgr - that leaves it
// UNBOUND). @identity-TODO: CButeMgr and CButeSection are the same 280-B config object (one
// ctor 0x170210) modeled as two classes; the (CButeSection*) cast is that conflation, to be
// dissolved when the CButeMgr<->CButeSection hierarchy is unified. (Forward-decl only -
// ButeMgr.h re-defines zPTree/zErrHandling that ButeSection.h already brings.)
class CButeMgr;
extern CButeMgr g_buteMgr;
RVA(0x00082b20, 0xa)
void InitResButeMgr82b20() {
    ((CButeSection*)&g_buteMgr)->CButeSection::CButeSection();
}

// The debug-overlay / profiler text-sink CString globals (0x645524..0x645530).
// g_profSink is the shared text sink Play.cpp logs through (== GruntzMgrCmd's
// g_brickText1); the g_str6455xx names are the still-anonymous siblings. Externs
// (storage lives in the owning subsystem TU); the DATA pins name the ones this
// cluster owns.
// extern "C" so the reloc emits the canonical `_g_profSink` - the single name bound
// at 0x245524 (play owns it as extern "C"); the C++-mangled ?g_profSink@@3VCString@@A
// never bound.
extern "C" CString g_profSink; // 0x645524 (_g_profSink; pinned in Play.cpp)
DATA(0x00245528)
extern CString g_str645528; // 0x645528 (== GruntzMgrCmd's g_brickText2)
DATA(0x0024552c)
extern CString g_str64552c; // 0x64552c
DATA(0x00245530)
extern CString g_str645530; // 0x645530

RVA(0x00082ba0, 0xa)
void InitStr645524() {
    g_profSink.CString::CString();
}
RVA(0x00082c20, 0xa)
void InitStr645528() {
    g_str645528.CString::CString();
}
RVA(0x00082ca0, 0xa)
void InitStr64552c() {
    g_str64552c.CString::CString();
}
RVA(0x00082d20, 0xa)
void InitStr645530() {
    g_str645530.CString::CString();
}

// ---------------------------------------------------------------------------
// 0x082da0..0x082ff0 (folded from the former freenodepool unit, waveP). This is
// the RVA-contiguous TAIL of THIS obj: 0x82d20 + 0x80 = 0x82da0, and the
// g_str6455xx CString globals sit in ONE contiguous .data run 0x645514..0x645530
// with GameText's g_str645524..30 above (private-globals oracle: one obj). The
// four low-band CString dynamic-initializer (_$E) thunks of the debug-overlay /
// profiler text-sink set (0x645514..0x645520); each tail-constructs its empty
// CString in place (`mov ecx,&g; jmp CString::CString()` @0x1b9b93, reloc-masked).
// ---------------------------------------------------------------------------
DATA(0x00245514)
extern CString g_str645514; // 0x645514
DATA(0x00245518)
extern CString g_str645518; // 0x645518
DATA(0x0024551c)
extern CString g_str64551c; // 0x64551c
DATA(0x00245520)
extern CString g_str645520; // 0x645520

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

// The shared coord-node pool object (0x645540) - DEFINED here (owner TU: this obj's
// RVA-contiguous tail is the pool's reset/clear pair). Its +0 slot holds the RezAlloc'd
// backing block ClearCoordPool frees. ResetCoordPool (0x082fa0, __cdecl) zeroes
// the four pool words WITHOUT freeing; ClearCoordPool (0x082ff0) frees the backing
// block (m_0) if present, then zeroes.
//
// This is ONE 16-byte object, not four scalars: the +0x0/+0x4/+0x8/+0xc DIR32s the
// two routines emit are interior fields of it (addend-relative to the base symbol).
// The tree used to carry three extra names for the SAME storage - `g_dropList`
// (GruntArrivalScan, which held the DATA pin at 0x245540 and so kept every
// `?g_coordPool@@3VFreeNodePool@@A` reference UNBOUND) plus the per-field aliases
// `g_freeList` @0x245544 (== m_4) and `g_freeListBias` @0x24554c (== m_c). g_dropList
// is folded onto g_coordPool here; the two per-field aliases are still pinned by
// GruntArrivalScan (interior-address globals - a separate fold).
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

// ---------------------------------------------------------------------------
// The 8 end-of-level stat labels, in order:
//   "Time:" / "Survivorz:" / "Deathz:" / "Toolz:" / "Toyz:" / "Powerupz:"
//   / "Coinz:" / "Secretz:".
// ---------------------------------------------------------------------------
// GetEndLevelStatLabels (the g_statLabel[] array initializer).
SYMBOL(_$E4)
RVA(0x00018740, 0x79)
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

// ---------------------------------------------------------------------------
// GetWarlordName - returns CString by value; the boss/warlord display name by
// id, via a 4-entry jump table:
//   0 -> "KING"  1 -> "NAPOLEAN"  2 -> "PATTON"  3 -> "VIKING"  default -> "".
// @interleaver GetWarlordName emitted-in <boundary: unreconstructed / bootystateactivate?>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are iconloaders
// BuildPowerupIconKeys @0x1e720 (before) and bootystateactivate QueryGruntSlots
// @0x1ecf0 (after). It heads bootystateactivate's block but is preceded by
// iconloaders, so NOT surrounded by a single host; free __stdcall fn, no class
// anchor. Candidate home bootystateactivate, deferred - needs the 0x1e720 obj first.)
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

// ---------------------------------------------------------------------------
// CContainerErr::CContainerErr - the container-library exception ctor (__thiscall
// (this, msg)). Stores the (custom or default)
// message, installs the vtable, and on FIRST construction lazily seeds the
// static 8-entry container-error message table. The error strings (by code):
//   "Out of memory" / "Data structure is invalid" / "Overflow" /
//   "No such file, handle or object" / "Out of range" /
//   "Target alrready exisits" / "Null pointer argument" / "Bad argument value".
// ---------------------------------------------------------------------------

// The default message base used when no custom message is supplied,
// and the runtime-seeded message-table slots. The 8 slots
// are distinct named globals (non-contiguous in the EXE) so the source emits the
// exact 8 `mov ds:slot,imm32` stores in the target's order. All reloc-masked.
static char g_defaultErrMsg[24]; // taken by-address (the default-message buffer)
static char* g_errMsg_OutOfMem;  // the lazy-init guard slot
static char* g_errMsg_BadData;
static char* g_errMsg_Overflow;
static char* g_errMsg_NoFile;
static char* g_errMsg_OutOfRng;
static char* g_errMsg_Exists;
static char* g_errMsg_NullArg;
static char* g_errMsg_BadArg;

// @interleaver CContainerErr::CContainerErr emitted-in typekeycoll. Retail emits this ctor
// COMDAT just before typekeycoll's block, where its sibling ~CContainerErr @0x16da60 lives
// (getretaddr @0x16d990 is the lone fn before it). The old "dual-view wall" that blocked
// homing it there is GONE - GameText.h's duplicate CContainerErr view is dissolved and this
// TU now models the class through the canonical <Wap32/zBitVec.h>, so the two headers no
// longer conflict. Re-homing the body to TypeKeyColl.cpp is now unblocked (follow-up).
RVA(0x0016d9c0, 0x75)
CContainerErr::CContainerErr(void* errSink) {
    // +0x04 stored first, the vptr after it (cl's implicit stamp). The arg is the sink to
    // register with, not a string: ~CContainerErr loads +0x04 into ecx as a __thiscall
    // `this` (see <Wap32/zBitVec.h>). The default-sink buffer is taken by address.
    m_errSink = (CVariantSlot*)(errSink ? errSink : g_defaultErrMsg);

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
