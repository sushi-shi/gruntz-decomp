// CBoomerang.cpp - the CBoomerang projectile ctor (its own TU; no other CBoomerang
// method is reconstructed yet, so this is the class's first/real TU). Re-homed from
// src/Stub/CBoomerang.cpp.
//
// Object-ctor archetype (no EH): forward the single by-value ctor arg to the
// CProjectile base ctor (engine fn, not matched -> external no-body, reloc-masked
// rel32 call via thunk 0x37d8), the compiler stamps the derived vftable into
// [this] (implicit, real polymorphic class), then OR a flag bit into the m_154
// sub-object's [+8] dword.
//
// Real polymorphic base (18 declared-only virtuals): cl emits ??_7CBoomerang +
// the implicit post-base-ctor vptr stamp, and the leaf vtable name
// ??_7CBoomerang@@6B@ auto-derives (RTTI; config/vtable_names.csv). The base ctor
// stays DECLARED only (out-of-line; its `call` reloc-masks via thunk 0x37d8).
#include <rva.h>

SIZE_UNKNOWN(CProjectileBase);
struct CProjectileBase {
    CProjectileBase(i32 a);
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual void Vf5();
    virtual void Vf6();
    virtual void Vf7();
    virtual void Vf8();
    virtual void Vf9();
    virtual void Vf10();
    virtual void Vf11();
    virtual void Vf12();
    virtual void Vf13();
    virtual void Vf14();
    virtual void Vf15();
    virtual void Vf16();
    virtual void Vf17();
    // +0x00  implicit vptr (the vftable slot the derived ctor re-stamps)
};

// The +0x154 sub-object whose [+8] dword gets the flag OR.
SIZE_UNKNOWN(CBoomerangAux);
struct CBoomerangAux {
    char m_pad00[0x08];
    i32 m_08; // +0x08
};

SIZE_UNKNOWN(CBoomerang);
class CBoomerang : public CProjectileBase {
public:
    CBoomerang(i32 a);
    char m_pad4[0x154 - 0x04]; // base vptr ends +0x04
    CBoomerangAux* m_154;      // +0x154
};

// @confidence: high
// @source: rtti-vptr
// @early-stop
// vptr-store-schedule wall (~97.3%): retail emits the implicit vptr store BEFORE
// the m_154 load; MSVC5 /O2 fills the post-base-call latency slot with the
// independent m_154 load and sinks the store after it. Now a real polymorphic
// class (implicit vptr init), so the store is no longer source-orderable - the
// schedule is the optimizer's, not steerable. Byte-exact otherwise.
RVA(0x000e0650, 0x2b)
CBoomerang::CBoomerang(i32 a) : CProjectileBase(a) {
    // vptr stamp is now IMPLICIT (real polymorphic class).
    m_154->m_08 |= 0x2000002;
}
