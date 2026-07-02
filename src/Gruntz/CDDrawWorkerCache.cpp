#include <rva.h>
// CDDrawWorkerCache.cpp - leaf methods of the tomalla-named class CDDrawWorkerCache
// (a CDirectDrawMgr surface/page sub-manager in the "Harry Potter" family).
// VirtualMethodUnknown20 is a constant state-ID stub returning 0x13 (19).
// VirtualMethodUnknown24 is a factory: allocates a 0x17c-byte worker object,
// seeds it from parent fields, stamps the foreign vftable, calls the
// worker's vtable+0x24 virtual with (arg1, arg3), on failure destroys the
// worker and returns 0, on success stores the worker into the CMapStringToOb
// at +0x10 under `key` (arg2) and returns it.
//
// Both are plain /O2 /MT leaves: NO SEH frame. The factory has reloc-masked
// rel32 calls (operator new / CMapStringToOb::operator[])
// and a DIR32 store for the foreign worker vftable.
//
// The worker is modeled as polymorphic (virtuals at the right slots) ONLY so
// `w->Vfunc24(...)` lowers to the exact `mov eax,[w]; call [eax+0x24]`
// __thiscall dispatch; its virtuals are never defined, so no vtable is emitted
// in this TU - the real vtable is the foreign engine datum stamped manually.
// Vfunc24 is called on the worker regardless of null (the target asm does
// not guard the vtable dispatch against null, accepting that operator new
// practically never fails in the NAFXCW heap).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

class CObject;

// CMapStringToOb lives at CDDrawWorkerCache+0x10. operator[] is an out-of-line
// NAFXCW thunk (reloc-masked rel32 call).
#include <Gruntz/CMapStringToOb.h>

// The worker virtual interface. Slots laid out so the dispatched method lands
// at byte offset +0x24. Declarations only - never defined, so no ??_7 emitted.
class SiriusWorker {
public:
    virtual void FUN_005bef01();         // [0] 0x1bef01
    virtual i32 ScalarDtor(i32 flag);    // [1] 0x151d80 scalar-deleting destructor
    virtual void FUN_004028ec();         // [2] 0x0028ec
    virtual void FUN_0040106e();         // [3] 0x00106e
    virtual void FUN_00404034();         // [4] 0x004034
    virtual void FUN_00551d60();         // [5] 0x151d60
    virtual void FUN_00401c08();         // [6] 0x001c08
    virtual void FUN_00551e70();         // [7] 0x151e70
    virtual void FUN_00551d70();         // [8] 0x151d70
    virtual i32 Vfunc24(i32 a1, i32 a3); // [9] 0x151e20 (Init)
};

// The 0x17c-byte worker layout. Only the seeded offsets are load-bearing.
// Real polymorphic: `new SiriusWorkerObj` makes cl auto-emit ??_7SiriusWorkerObj
// (masks the retail vtable 0x5efb80) and stamp the vptr in the ctor - no manual
// `*(void**)w = &g_siriusWorkerVtbl` store (ALL-VTABLES mandate).
struct SiriusWorkerObj : public SiriusWorker {
    SiriusWorkerObj() {}
    i32 m_04; // +0x04  = parent->m_1c
    i32 m_08; // +0x08  = 0
    i32 m_0c; // +0x0c  = parent->m_0c
    i32 m_10; // +0x10  = 0
    i32 m_14; // +0x14  = 0
    i32 m_18; // +0x18  = 0
    i32 m_1c; // +0x1c  = 0
    char m_pad20[0x170 - 0x20];
    i32 m_170; // +0x170 = 0
    i32 m_174; // +0x174 = 0
    i32 m_178; // +0x178 = 0
}; // size = 0x17c

// ---------------------------------------------------------------------------
// CDDrawWorkerCache - the CMapStringToOb at +0x10, and the parent fields copied
// into the worker (m_0c, m_1c from inside the map's internal area).
// ---------------------------------------------------------------------------
// Real polymorphic now (own 10-slot vtable ??_7CDDrawWorkerCache @0x5efd00, was
// Vtbl_1efd00 / ClassWithUnknownVTable31). Slots 0/2/3/4 are the shared CObject
// thunks, slot 1 the virtual dtor (0x157700), slots 5/6/7 unreconstructed leaf
// virtuals (declared-only, reloc-masked), slot 8 = VirtualMethodUnknown20 (0x1576f0)
// and slot 9 = VirtualMethodUnknown24 (0x1652c0). cl auto-emits the vtable; the
// implicit vptr-stamp replaces the explicit m_vptr store.
class CDDrawWorkerCache {
public:
    virtual void FUN_005bef01();          // [0] 0x1bef01 (shared thunk, declared-only)
    virtual ~CDDrawWorkerCache();         // [1] 0x157700 scalar-deleting dtor
    virtual void FUN_004028ec();          // [2] 0x0028ec (shared thunk, declared-only)
    virtual void FUN_0040106e();          // [3] 0x00106e (shared thunk, declared-only)
    virtual void FUN_00404034();          // [4] 0x004034 (shared thunk, declared-only)
    virtual void FUN_005576d0();          // [5] 0x1576d0 (declared-only)
    virtual void FUN_00557790();          // [6] 0x157790 (declared-only)
    virtual void FUN_00565210();          // [7] 0x165210 (declared-only)
    virtual i32 VirtualMethodUnknown20(); // [8] 0x1576f0
    virtual void* VirtualMethodUnknown24(i32 a1, const char* key, i32 a3); // [9] 0x1652c0

    // Engine-label backlog stub (non-virtual).
    void VirtualMethod_157720();

    // vptr implicit @ +0x00
    i32 m_04;                  // +0x04  -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                  // +0x0c  parent/root handle
    CMapStringToOb m_10;       // +0x10  map (internal field at +0x1c seeds worker->m_04)
};

// Read field at +0x1c from the parent (inside the CMapStringToOb), used to
// seed worker->m_04.
static inline i32 SiriusReadField1c(const CDDrawWorkerCache* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

// ---------------------------------------------------------------------------
// Constant state ID: returns 0x13 (19).
// ---------------------------------------------------------------------------
RVA(0x001576f0, 0x6)
i32 CDDrawWorkerCache::VirtualMethodUnknown20() {
    return 0x13;
}

// Inline worker constructor. Real `new SiriusWorkerObj`: the ctor stamps the vptr
// (cl-implicit, vptr-first) and cl auto-emits ??_7SiriusWorkerObj; then seed the
// fields. (ALL-VTABLES mandate: the vptr store is now compiler-implicit, moving
// from vptr-middle to vptr-first - a code regression accepted for the real shape.)
static inline SiriusWorkerObj* MakeSiriusWorker(const CDDrawWorkerCache* parent) {
    SiriusWorkerObj* w = new SiriusWorkerObj;
    if (w != 0) {
        i32 field1c = SiriusReadField1c(parent);
        i32 harryPotter = parent->m_0c;
        w->m_04 = field1c;
        w->m_08 = 0;
        w->m_0c = harryPotter;
        w->m_10 = 0;
        w->m_14 = 0;
        w->m_18 = 0;
        w->m_170 = 0;
        w->m_1c = 0;
        w->m_174 = 0;
        w->m_178 = 0;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Allocate + construct a 0x17c-byte worker, call its +0x24 virtual with
// (arg1, arg3). On success store it into the map under `key` and return it;
// on failure run its scalar-deleting dtor and return 0.
//
// NOTE: Vfunc24 is dispatched on the worker BEFORE the null check, matching
// the target asm (the NAFXCW operator new practically never fails).
// ---------------------------------------------------------------------------
RVA(0x001652c0, 0x92)
void* CDDrawWorkerCache::VirtualMethodUnknown24(i32 a1, const char* key, i32 a3) {
    SiriusWorkerObj* w = MakeSiriusWorker(this);

    if (w->Vfunc24(a1, a3) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    m_10[key] = (CObject*)w;
    return w;
}

// Engine-label backlog stubs (moved from src/Stub/CDDrawWorkerCache.cpp).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00157700, 0x1e)
CDDrawWorkerCache::~CDDrawWorkerCache() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x00157720, 0x68)
void CDDrawWorkerCache::VirtualMethod_157720() {}

SIZE_UNKNOWN(CDDrawWorkerCache);
SIZE_UNKNOWN(SiriusWorker);
SIZE(SiriusWorkerObj, 0x17c);
VTBL(SiriusWorkerObj, 0x001efb80); // ??_7SiriusWorkerObj (was g_siriusWorkerVtbl)
// ??_7CDDrawWorkerCache (was Vtbl_1efd00 / ClassWithUnknownVTable31; 10 slots). cl
// auto-emits it from the real-polymorphic class; retail datum is reloc-masked ->
// matching-neutral catalog tracking.
VTBL(CDDrawWorkerCache, 0x001efd00);
