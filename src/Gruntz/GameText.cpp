// GameText.cpp - small name/string lookup + table-init leaves.
//
// These are the human-readable name/label tables the UI reads. Recovered from
// dump_target.py (real bytes + relocs). The string literals become file-scope
// .rdata consts (reloc-masked); only the per-fn code bytes are load-bearing.
#include <Gruntz/GameText.h>
#include <rva.h>

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
RVA(0x0001ec20, 0x8d)
CString __stdcall GetWarlordName(int id) {
    // The target reserves and zero-inits one dead stack dword (`push ecx; mov
    // [esp+4],0; ...; pop ecx`) that no path reads - an MSVC5 return-slot/NRV
    // bookkeeping artifact. A `volatile int = 0` reproduces it exactly (the
    // zero-init survives DCE without emitting an address-store; scheduled after
    // the cmp, matching the target's `mov [esp+4],0`).
    volatile int slot = 0;
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
static char* g_defaultErrMsg;
static char* g_errMsg_OutOfMem; // the lazy-init guard slot
static char* g_errMsg_BadData;
static char* g_errMsg_Overflow;
static char* g_errMsg_NoFile;
static char* g_errMsg_OutOfRng;
static char* g_errMsg_Exists;
static char* g_errMsg_NullArg;
static char* g_errMsg_BadArg;

RVA(0x0016d9c0, 0x75)
CContainerErr::CContainerErr(const char* msg) {
    m_msg = msg ? msg : g_defaultErrMsg; // +0x04 stored first
    m_vtbl = &g_containerErrVtbl;        // +0x00 vtable stored after m_msg

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
