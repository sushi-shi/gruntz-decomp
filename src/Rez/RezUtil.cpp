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

    // 0x1b9b82 - resource free wrapper: forward the block straight to the CRT free.
    RVA(0x001b9b82, 0xb)
    SYMBOL(_RezFree)
    void RezFree(void* block) {
        free(block);
    }
}
