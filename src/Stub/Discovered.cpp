// Discovered.cpp - functions found by GAP ANALYSIS that Ghidra never carved.
//
// `gruntz sema xref` bounds caller/callee attribution by each function's known size
// (symbol_names/functions.csv). That surfaces call sites landing in the UNRECOVERED
// GAPS between carved functions - real code Ghidra's auto-analysis missed entirely
// (no functions.csv entry, no FUN_ name, absent from src/). This file is the carve-out
// home for such finds: an RVA()+size stub pins each boundary so the delinker extracts
// the retail bytes and objdiff tracks it (initially ~0%), like the rest of the
// src/Stub backlog. Reconstruct + re-home each into its real class TU from here.
#include <rva.h>

// 0x0de9e0 (__cdecl, 0xf4 B, SEH / -GX frame). A COMMAND DISPATCHER over a boomerang-
// family object. It reads a command tag `host->m_7c->m_1c` and switches on it: tag 0
// does `new CBoomerang(owner)` (0x260 B; ctor 0x0e0650), stores it at the state's m_18
// and calls its +0x18 virtual; tags 0x1d/0x1e/0x50..0x53 dispatch other virtuals
// (+0x28..+0x3c) on m_18; the default frees m_18 (delete @0x16e4f0). The dispatcher's
// OWN class is not yet identified - its arg is a host whose +0x7c is the boomerang-state
// object. It lives in the boomerang/projectile .text cluster (just below CProjectile
// 0x0dec60 / CBoomerang::CBoomerang 0x0e0650), NOT the level-preview code that merely
// precedes it in memory. Found via the size-bounded xref-tree (it was the real caller
// behind the bogus "PreviewCancelHost::Cancel calls CBoomerang" edge).
RVA(0x000de9e0, 0xf4)
i32 BoomerangCmdDispatch_de9e0(void* host) {
    return 0;
} // @stub
