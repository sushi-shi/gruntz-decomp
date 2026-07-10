#include <rva.h>
// RezUtil.cpp - WAP "Rez" resource-allocator leaf wrappers (region ~0x1b9b46),
// compiled /O1 (favor size). The size-favoring optimizer is the tell: a single
// __cdecl arg is forwarded with a direct `push [mem]` and cleaned with `pop ecx`
// (vs /O2's `mov reg,[mem]; push reg` + `add esp,4`), so these cannot match inside
// the /O2 engine_label_stubs aggregate.
//
// The sibling RezAlloc (0x1b9b46) is the MSVC5 CRT global `operator new`
// new-handler retry loop (Ghidra FID = operator_new); left as a skipped stub in
// src/Stub/EngineExternFns.cpp per the game-not-CRT policy.

extern "C" {
    void free(void* block); // 0x120c30 (CRT free)

    // resource free wrapper: forward the block straight to the CRT free.
    RVA(0x001b9b82, 0xb)
    SYMBOL(_RezFree)
    void RezFree(void* block) {
        free(block);
    }
}

// 0x1b9b8d - a getter that returns the address of a global descriptor (g_desc_6156f4,
// VA 0x6156f4; `mov eax, OFFSET g; ret`). Sits in the MFC CString/collection text region
// (immediate neighbours 0x1b9b46 operator new, 0x1b9b93 ??0CString), likely NAFXCW
// library - identity-TODO: carve to config/library_labels.csv once the exact MFC symbol
// for the &0x6156f4 descriptor getter is pinned. The g_desc_6156f4 DATA pin lives in
// src/Globals.cpp. Re-homed from src/Stub/ReconBatch2.cpp.
extern void* g_desc_6156f4; // 0x6156f4 (pinned in Globals.cpp)
RVA(0x001b9b8d, 0x6)
void** Get_1b9b8d() {
    return &g_desc_6156f4;
}
