// MgrTokenQuery.cpp - 0x099d10, a tiny __thiscall bool query helper over the
// singleton manager object at g_0x6459b0 (a 40-case token/command dispatcher).
//
// Reached from the level-load path (0x0ca200 via thunk 0x0039a4). It resets the
// manager's first dword (Reset, 0x49a0b0 = `mov [this],0; ret`), then runs the
// dispatcher (0x499d40, __thiscall(this, int) over a 40-entry jump table) on the
// caller's argument and returns whether the dispatch produced a nonzero result
// (the int->bool neg/sbb/neg normalize). Both manager methods are external
// (reloc-masked); the singleton is a file-scope object.
#include <Ints.h>
#include <Gruntz/TokenMgr.h> // canonical CTokenMgr (Reset/Dispatch on g_tokenMgr)
#include <Gruntz/AreaMgr.h> // canonical CAreaMgr (Reset @0x9a0b0 / Dispatch @0x99d40; non-virtual)
#include <rva.h>

// g_0x6459b0 - the manager singleton (ecx for both calls). Reloc-masked.
DATA(0x002459b0)
extern CTokenMgr g_tokenMgr;

// TokenMgrReset99b80 @0x099b80 - the standalone reset thunk that clears the token
// manager singleton (CAreaMgr::Reset @0x9a0b0, reloc-masked via ILT 0x3bac). __cdecl.
// Re-homed from src/Stub/BoundaryThunks.cpp (was TokenMgrReset99b80).
RVA(0x00099b80, 0xa)
void TokenMgrReset99b80() {
    ((CAreaMgr*)&g_tokenMgr)->Reset();
}

// __cdecl free helper: reset, dispatch(arg), return result != 0.
RVA(0x00099d10, 0x20)
i32 QueryToken(i32 arg) {
    ((CAreaMgr*)&g_tokenMgr)->Reset();
    return ((CAreaMgr*)&g_tokenMgr)->Dispatch(arg) != 0;
}
