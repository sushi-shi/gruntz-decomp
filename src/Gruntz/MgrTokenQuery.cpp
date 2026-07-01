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
#include <rva.h>

// The singleton manager: only Reset() + Dispatch() are reached here, both
// __thiscall on g_tokenMgr. The body/layout live in another TU.
struct CTokenMgr {
    void Reset();      // 0x49a0b0 - mov [this],0; ret
    i32 Dispatch(i32); // 0x499d40 - __thiscall(this, int), 40-case jump table
};

// g_0x6459b0 - the manager singleton (ecx for both calls). Reloc-masked.
DATA(0x002459b0)
extern CTokenMgr g_tokenMgr;

// __cdecl free helper: reset, dispatch(arg), return result != 0.
RVA(0x00099d10, 0x20)
i32 QueryToken(i32 arg) {
    g_tokenMgr.Reset();
    return g_tokenMgr.Dispatch(arg) != 0;
}
