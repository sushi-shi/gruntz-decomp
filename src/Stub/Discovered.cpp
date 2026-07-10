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

// (0x0de9e0 BoomerangCmdDispatch re-homed to src/Stub/DiscoveredEh.cpp - it carries a
// /GX EH frame (heap `new CBoomerang`), so it needs the eh profile. The full switch/
// new/virtual-dispatch/ProjTypeXfer body is reconstructed there and is EXACT; the
// dispatcher's own owner class is still unrecovered (only inbound edge is ILT thunk
// 0x158c), so BoomState/BoomHost stay placeholder views.)
