#include <rva.h>
// SBI_RectOnlyEh.cpp - the /GX EH-framed CSBI_RectOnly methods, split off the
// frameless sbi_rectonly TU (C:\Proj\Gruntz). MSVC5's /GX always frames a method
// that owns a lifetime-spanning `new`+ctor or a destructible member, so these
// cannot share the base TU's frameless flags without re-framing its 100% leaves.
// The split is matching-neutral (each function is RVA-keyed); see
// docs/patterns/split-tu-eh-dtor-vs-frameless-cstring.md.
//
// LAYOUT NOTE: these methods touch only `this` + m_54c (a lazily-created
// sub-object pointer at +0x54c), so the class carries a minimal view here - the
// offset is the load-bearing fact, the mangled name (?<method>@CSBI_RectOnly@@...)
// is layout-independent. The full class lives in SBI_RectOnly.cpp.

// The lazily-created 0x40-byte sub-object held at CSBI_RectOnly+0x54c: a no-arg
// ctor + a 4-arg Init (the owner + the three forwarded args).
class CSbiLazySub {
public:
    CSbiLazySub();                              // FUN_005091..  no-arg ctor
    i32 Init(void* owner, i32 a, i32 b, i32 c); // __thiscall, ret 0x10
    char m_pad[0x40];
};

// The retail vtables stamped as the destructor unwinds the class hierarchy. The
// vtable VALUES are reproduced by address (DATA() externs, reloc-masked) - the
// transitional manual-stamp device while the full hierarchy's vtables are not yet
// modeled. Same symbols as CStatusBarMgr.cpp's g_vtbl_t3/t4.
DATA(0x001eac4c)
extern void* g_vtbl_t4[]; // 0x5eac4c
DATA(0x001eac0c)
extern void* g_vtbl_t3[]; // 0x5eac0c
DATA(0x001eab8c)
extern void* g_vtbl_rectBase[]; // 0x5eab8c (CSBI_RectOnly base subobject)
DATA(0x001eabcc)
extern void* g_vtbl_rectBase2[]; // 0x5eabcc (next base subobject)

class CSBI_RectOnly {
public:
    i32 EnsureSub(i32 a, i32 b, i32 c);
    ~CSBI_RectOnly();
    void DtorBaseA(); // FUN_.. base-subobject teardown A
    void DtorBaseB(); // base teardown B
    void DtorBaseC(); // base teardown C
    void DtorBaseD(); // base teardown D (final)
    char m_pad0[0x54c];
    CSbiLazySub* m_54c; // +0x54c  lazily-created sub-object
};

// The scalar destructor walks the class's base-subobject chain: before tearing
// down each base it re-stamps the vptr to that base's retail vtable, then calls
// that base's teardown. /GX frames the whole walk (the trylevel writes 0/1/2/-1
// are the EH-state machine's, auto-generated).
// @early-stop
// ~43%: the four vptr-stamp + base-dtor-call pairs are byte-exact, but the whole
// /GX SEH frame (push -1/handler/fs:0 + the 0/1/2/-1 trylevel stamps) is MISSING -
// MSVC only frames a dtor whose base SUBOBJECT has a non-trivial dtor, which the
// manual-vptr non-polymorphic model can't express. Documented wall
// (docs/patterns/eh-dtor-needs-base-subobject.md); converting to a real base
// hierarchy would re-shape the ctor + emit a ??_7/??_G and regress the 100% ctor +
// every exact sibling. Deferred to the final sweep (whole-class model).
RVA(0x00102000, 0x7f)
CSBI_RectOnly::~CSBI_RectOnly() {
    *(void**)this = g_vtbl_t4;
    DtorBaseA();
    *(void**)this = g_vtbl_t3;
    DtorBaseB();
    *(void**)this = g_vtbl_rectBase;
    DtorBaseC();
    *(void**)this = g_vtbl_rectBase2;
    DtorBaseD();
}

// Lazily create the +0x54c sub-object on first use: bail (return 0) if it already
// exists; otherwise `new` a 0x40-byte CSbiLazySub, default-construct it, store it,
// and forward the three args to its Init. /GX frames the new+ctor span.
RVA(0x00109ad0, 0xa9)
i32 CSBI_RectOnly::EnsureSub(i32 a, i32 b, i32 c) {
    if (m_54c) {
        return 0;
    }
    CSbiLazySub* o = new CSbiLazySub();
    m_54c = o;
    if (o == 0) {
        return (i32)o; // retail returns the null pointer already in eax (no re-xor)
    }
    return o->Init(this, a, b, c);
}
