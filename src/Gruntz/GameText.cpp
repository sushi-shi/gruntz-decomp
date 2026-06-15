// GameText.cpp - small name/string lookup + table-init leaves.
//
// These are the human-readable name/label tables the UI reads. Recovered from
// dump_target.py (real bytes + relocs). The string literals become file-scope
// .rdata consts (reloc-masked); only the per-fn code bytes are load-bearing.
#include "GameText.h"

// ---------------------------------------------------------------------------
// The two name tables are file-scope arrays of CString with brace-initializers.
// MSVC5 /O2 emits ONE dynamic-initializer per array - a flat run of
// `push lit; mov ecx,&g[i]; call ??0CString@@QAE@PBD@Z` with NO placement-new
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
// GetWorldDisplayName (the g_worldName[] array initializer) @ 0x82990 (121 B).
// @address: 0x82990
// @symbol:  _$E1
// @size:    0x79
static AfxString g_worldName[8] = {
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
// GetEndLevelStatLabels (the g_statLabel[] array initializer) @ 0x18740 (121 B).
// @address: 0x18740
// @symbol:  _$E4
// @size:    0x79
static AfxString g_statLabel[8] = {
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
// GetWarlordName  @ 0x1ec20 (141 B, __cdecl, returns CString by value, ret 8) -
// the boss/warlord display name by id, via a 4-entry jump table:
//   0 -> "KING"  1 -> "NAPOLEAN"  2 -> "PATTON"  3 -> "VIKING"  default -> "".
// ---------------------------------------------------------------------------
// @address: 0x1ec20
// @size:    0x8d
AfxString __stdcall GetWarlordName(int id)
{
    // The target reserves and zero-inits one dead stack dword (`push ecx; mov
    // [esp+4],0; ...; pop ecx`) that no path reads - an MSVC5 return-slot/NRV
    // bookkeeping artifact. A `volatile int = 0` reproduces it exactly (the
    // zero-init survives DCE without emitting an address-store; scheduled after
    // the cmp, matching the target's `mov [esp+4],0`).
    volatile int slot = 0;
    switch (id) {
    case 0:  return AfxString("KING");
    case 1:  return AfxString("NAPOLEAN");
    case 2:  return AfxString("PATTON");
    case 3:  return AfxString("VIKING");
    default: return AfxString("");
    }
}

// ---------------------------------------------------------------------------
// CContainerErr::CContainerErr  @ 0x16d9c0 (117 B, __thiscall(this, msg),
// ret 4) - the container-library exception ctor. Stores the (custom or default)
// message, installs the vtable, and on FIRST construction lazily seeds the
// static 8-entry container-error message table. The error strings (by code):
//   "Out of memory" / "Data structure is invalid" / "Overflow" /
//   "No such file, handle or object" / "Out of range" /
//   "Target alrready exisits" / "Null pointer argument" / "Bad argument value".
// ---------------------------------------------------------------------------

// The default message base (@0x6bf430) used when no custom message is supplied,
// and the runtime-seeded message-table slots (@0x6bf448..0x6bf464). The 8 slots
// are distinct named globals (non-contiguous in the EXE) so the source emits the
// exact 8 `mov ds:slot,imm32` stores in the target's order. All reloc-masked.
static char *g_defaultErrMsg;   // 0x6bf430
static char *g_errMsg_OutOfMem; // 0x6bf464 (the lazy-init guard slot)
static char *g_errMsg_BadData;  // 0x6bf448
static char *g_errMsg_Overflow; // 0x6bf44c
static char *g_errMsg_NoFile;   // 0x6bf460
static char *g_errMsg_OutOfRng; // 0x6bf450
static char *g_errMsg_Exists;   // 0x6bf458
static char *g_errMsg_NullArg;  // 0x6bf454
static char *g_errMsg_BadArg;   // 0x6bf45c

// @address: 0x16d9c0
// @size:    0x75
CContainerErr::CContainerErr(const char *msg)
{
    m_msg  = msg ? msg : g_defaultErrMsg;  // +0x04 stored first
    m_vtbl = &g_containerErrVtbl;          // +0x00 vtable stored after m_msg

    if (g_errMsg_OutOfMem == 0) {
        g_errMsg_OutOfMem = "Out of memory";
        g_errMsg_BadData  = "Data structure is invalid";
        g_errMsg_Overflow = "Overflow";
        g_errMsg_NoFile   = "No such file, handle or object";
        g_errMsg_OutOfRng = "Out of range";
        g_errMsg_Exists   = "Target alrready exisits";
        g_errMsg_NullArg  = "Null pointer argument";
        g_errMsg_BadArg   = "Bad argument value";
    }
}
