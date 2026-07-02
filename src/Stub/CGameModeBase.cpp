#include <rva.h>
// CGameModeBase.cpp - engine-label stubs for CGameModeBase (reloc-correlation).
//
// 0x3f53 ("BaseCleanup") was a 5-byte `jmp 0xfa150` in the leading ILT jmp-table
// region (RVA < ilt_end 0x7c20): a linker incremental-link thunk forwarding to the
// real body at 0xfa150, NOT reconstructable game source. Removed from src so
// status.py carves it under the "jump thunks" category (rva < ilt_end) instead of
// counting it as a claimed reconstruction target.
