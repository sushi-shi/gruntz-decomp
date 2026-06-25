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

// The retail vtables stamped as the destructors unwind the class hierarchy. The
// vtable VALUES are reproduced by address (DATA() externs, reloc-masked) - the
// transitional manual-stamp device while the full hierarchy's vtables are not yet
// modeled. Same symbols as CStatusBarMgr.cpp's g_vtbl_t3/t4.
DATA(0x001eac4c)
extern void* g_vtbl_t4[]; // 0x5eac4c (CSBI_ImageSet most-derived subobject)
DATA(0x001eac0c)
extern void* g_vtbl_t3[]; // 0x5eac0c (CSBI_Image base subobject)
DATA(0x001eab8c)
extern void* g_vtbl_rectBase[]; // 0x5eab8c (CSBI_RectOnly subobject)
DATA(0x001eabcc)
extern void* g_vtbl_rectBase2[]; // 0x5eabcc (CStatusBarItem base subobject)

// The MSVC 'eh vector destructor iterator' runtime (0x51f640): runs `dtor` over
// `count` elements of `stride` from `base`, descending. Reloc-masked rel32 callee.
void Tm_DestroyArray(void* base, i32 stride, i32 count, void* dtor); // 0x11f640

// The CByteArray member at +0x530: a real destructor (reloc-masked ~CByteArray @0x5b4f3e)
// so its teardown drives a /GX trylevel level in the member-teardown dtor.
struct CSbiByteArray {
    void Dtor();             // 0x5b4f3e  ~CByteArray
    ~CSbiByteArray() { Dtor(); }
    char m_pad[0x14];
};

// The per-element list dtor (~CPtrList, aliased ~CInternetSession @0x5b48c6) passed to
// the vector-destroy iterator for the eight +0x2c notify lists.
void SbiList_Dtor(); // 0x5b48c6

class CSBI_RectOnly {
public:
    i32 EnsureSub(i32 a, i32 b, i32 c);
    ~CSBI_RectOnly();
    void DtorMembers(); // 0xc8980  CSBI_RectOnly member teardown (/GX member-array dtor)
    void Teardown();    // 0xfe350  pre-teardown (drains the pooled ptr table)
    void DtorRect();    // 0xe8760  CSBI_RectOnly member teardown
    void DtorStatus();  // 0x10bfa0 CStatusBarItem base teardown
    char m_pad0[0x54c];
    CSbiLazySub* m_54c; // +0x54c  lazily-created sub-object
};

// CSBI_ImageSet is the most-derived of the chain
//   CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem
// its scalar destructor (0x102000) unwinds all four levels. (Originally mislabeled
// ~CSBI_RectOnly by the rtti-vptr heuristic; the real ~CSBI_RectOnly is the 2-level
// dtor at 0x100700 below.)
class CSBI_ImageSet {
public:
    ~CSBI_ImageSet();
    void DtorImageSet(); // most-derived (ImageSet) member teardown
    void DtorImage();    // CSBI_Image base teardown
    void DtorRect();     // CSBI_RectOnly base teardown
    void DtorStatus();   // CStatusBarItem base teardown
    char m_pad0[0x60];
};

// 0xc8980: CSBI_RectOnly member teardown - the /GX dtor body that drains the pooled
// state (Teardown), destructs the +0x530 CByteArray, then runs the eh-vector-destroy
// iterator over the eight +0x2c notify lists (stride 0x1c, ~CPtrList per element). The
// three teardown stages each carry their own descending /GX trylevel.
// @early-stop
// ~49% /GX member-array dtor wall (same family as ~CTriggerMgr 0x85c50 and the 0x100700
// dtor below): the body is byte-correct - Teardown call, lea [this+0x530] + ~CByteArray,
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

// The real CSBI_RectOnly scalar destructor (0x100700): stamps the RectOnly vptr,
// runs its member teardown, stamps the CStatusBarItem base vptr, runs the base
// teardown.
// @early-stop
// ~34% (eh-dtor-needs-base-subobject wall): the two vptr-stamp + dtor-call pairs
// are byte-exact, but the whole /GX SEH frame (push -1/handler/fs:0 + the 0/-1
// trylevel stamps) is MISSING - MSVC only frames a dtor whose base SUBOBJECT has a
// non-trivial dtor, which the manual-vptr non-polymorphic model can't express.
// Documented wall (docs/patterns/eh-dtor-needs-base-subobject.md). Deferred to the
// final sweep (whole-class model).
RVA(0x00100700, 0x55)
CSBI_RectOnly::~CSBI_RectOnly() {
    *(void**)this = g_vtbl_rectBase;
    DtorRect();
    *(void**)this = g_vtbl_rectBase2;
    DtorStatus();
}

// The CSBI_ImageSet scalar destructor (0x102000): walks the full 4-level chain,
// re-stamping each base's vtable before its teardown. /GX frames the whole walk
// (the trylevel writes 0/1/2/-1 are the EH-state machine's, auto-generated).
// @early-stop
// ~43% (eh-dtor-needs-base-subobject wall): the four vptr-stamp + base-dtor-call
// pairs are byte-exact, but the whole /GX SEH frame is MISSING (same wall as above).
// Deferred to the final sweep (whole-class model).
RVA(0x00102000, 0x7f)
CSBI_ImageSet::~CSBI_ImageSet() {
    *(void**)this = g_vtbl_t4;
    DtorImageSet();
    *(void**)this = g_vtbl_t3;
    DtorImage();
    *(void**)this = g_vtbl_rectBase;
    DtorRect();
    *(void**)this = g_vtbl_rectBase2;
    DtorStatus();
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
