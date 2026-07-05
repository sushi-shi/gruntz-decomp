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
    void Reset();      // 0x49a0b0  mov [this],0; ret
    i32 Dispatch(i32); // 0x499d40  __thiscall(this,int), 40-case jump table
};

#endif // SRC_GRUNTZ_TOKENMGR_H
