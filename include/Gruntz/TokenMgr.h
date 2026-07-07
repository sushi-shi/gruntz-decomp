// TokenMgr.h - the token-manager singleton CTokenMgr (a REAL recovered type,
// ?g_tokenMgr@@3UCTokenMgr@@A @ VA 0x6459b0). Was viewed per-TU in MgrTokenQuery.cpp
// (Reset/Dispatch) + BoundaryThunks.cpp (the Reset3bac ILT thunk); folded here
// (wave 3). Both methods are reloc-masked externals (bodies in another TU).
#ifndef SRC_GRUNTZ_TOKENMGR_H
#define SRC_GRUNTZ_TOKENMGR_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CTokenMgr);
struct CTokenMgr {
    // Reset @0x9a0b0 IS CAreaMgr::Reset; cast at the call.
    // Dispatch @0x99d40 IS CAreaMgr::Dispatch; cast at the call.
};

#endif // SRC_GRUNTZ_TOKENMGR_H
