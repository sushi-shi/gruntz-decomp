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
SIZE_UNKNOWN(CSbiLazySub);

// The two scalar destructors (~CSBI_RectOnly @0x100700, ~CSBI_ImageSet @0x102000)
// that used to live here moved to SBI_RectOnlyDtorEh.cpp / SBI_ImageSetEh.cpp,
// where each is modeled as a REAL polymorphic chain so MSVC emits the /GX frame
// (a manual vtable stamp could not). The shared base CSBI_RectOnly is the
// out-of-line leaf in one and the inline base in the other, so they must be
// separate TUs. The SBI item vtables are now emitted by the compiler from the real
// polymorphic classes (the catalog ??_7 auto-namer in config/vtable_names.csv).

// The MSVC 'eh vector destructor iterator' runtime (0x51f640): runs `dtor` over
// `count` elements of `stride` from `base`, descending. Reloc-masked rel32 callee.
void Tm_DestroyArray(void* base, i32 stride, i32 count, void* dtor); // 0x11f640

// The CByteArray member at +0x530: a real destructor (reloc-masked ~CByteArray @0x5b4f3e)
// so its teardown drives a /GX trylevel level in the member-teardown dtor.
struct CSbiByteArray {
    void Dtor(); // 0x5b4f3e  ~CByteArray
    ~CSbiByteArray() {
        Dtor();
    }
    char m_pad[0x14];
};
SIZE_UNKNOWN(CSbiByteArray);

// The per-element list dtor (~CPtrList, aliased ~CInternetSession @0x5b48c6) passed to
// the vector-destroy iterator for the eight +0x2c notify lists.
void SbiList_Dtor(); // 0x5b48c6

class CSBI_RectOnly {
public:
    i32 EnsureSub(i32 a, i32 b, i32 c);
    void DtorMembers(); // 0xc8980  CSBI_RectOnly member teardown (/GX member-array dtor)
    void Teardown();    // 0xfe350  pre-teardown (drains the pooled ptr table)
    char m_pad0[0x54c];
    CSbiLazySub* m_54c; // +0x54c  lazily-created sub-object
};

// 0xc8980: CSBI_RectOnly member teardown - the /GX dtor body that drains the pooled
// state (Teardown), destructs the +0x530 CByteArray, then runs the eh-vector-destroy
// iterator over the eight +0x2c notify lists (stride 0x1c, ~CPtrList per element). The
// three teardown stages each carry their own descending /GX trylevel.
// @early-stop
// ~49% /GX member-array dtor wall (same family as ~CTriggerMgr 0x85c50): the body is
// byte-correct - Teardown call, lea [this+0x530] + ~CByteArray,
// and the four-arg eh-vector-destroy push sequence (push &dtor / 8 / add esi,0x2c / 0x1c /
// push base) all match retail. But the whole /GX SEH frame (push -1 / push handler / mov
// fs:0,esp) and the descending [esp+0x10]=1/0/-1 trylevel stamps are MISSING: MSVC only
// emits them for a real `~Class()` whose VALUE members have non-trivial dtors. 0xc8980 is
// a standalone teardown HELPER (not the C++ destructor - that slot is 0x100700), so the
// member-as-destructible steering can't apply. Documented wall (docs/patterns/
// eh-dtor-model-members-as-destructible.md). Deferred to the final sweep (whole-class model).
RVA(0x000c8980, 0x64)
void CSBI_RectOnly::DtorMembers() {
    Teardown();
    ((CSbiByteArray*)((char*)this + 0x530))->Dtor();
    Tm_DestroyArray((char*)this + 0x2c, 0x1c, 8, (void*)&SbiList_Dtor);
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
