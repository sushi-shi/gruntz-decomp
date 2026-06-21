#include <rva.h>
// CBoomerang.cpp - CBoomerang ctor (matched modulo the vptr-store schedule wall).
//
// Object-ctor archetype (no EH): forward the single by-value ctor arg to the
// CProjectile base ctor (engine fn, not matched -> external no-body,
// reloc-masked rel32 call via thunk 0x37d8), re-stamp the derived vftable into
// [this], then OR a flag bit into the m_154 sub-object's [+8] dword.
//
// WALL (manual-vptr-store schedule): retail emits the vptr store BEFORE the
// m_154 load; MSVC5 /O2 fills the post-base-call latency slot with the
// independent m_154 load and sinks the manual store after it. The store-first
// order is only reachable from a compiler-IMPLICIT vptr init, which here needs a
// 12-slot in-TU vtable pointing at engine fns in other TUs (diverges) AND shifts
// the layout by 4 (vptr@0). Reconstruction is otherwise byte-exact; ~97.3%.

struct CProjectileBase {
    CProjectileBase(int a);
    void* m_vptr; // +0x00 (the vftable slot the derived ctor re-stamps)
};

// The +0x154 sub-object whose [+8] dword gets the flag OR.
struct CBoomerangAux {
    char m_pad00[0x08];
    int m_08; // +0x08
};

class CBoomerang : public CProjectileBase {
public:
    CBoomerang(int a);
    char m_pad4[0x154 - 0x04]; // base vptr ends +0x04
    CBoomerangAux* m_154;      // +0x154
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5e792c)
extern void* g_boomerangVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x0e0650, 0x2b)
CBoomerang::CBoomerang(int a) : CProjectileBase(a) {
    *(void**)this = &g_boomerangVtbl;
    m_154->m_08 |= 0x2000002;
}
