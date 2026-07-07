#include <rva.h>
#include <Wap32/Object.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
// CDDrawWorkerCache.cpp - leaf methods of the tomalla-named class CDDrawWorkerCache
// (a CDirectDrawMgr surface/page sub-manager in the "DDraw surface manager" family).
// GetStateId is a constant state-ID stub returning 0x13 (19).
// CreateWorker is a factory: allocates a 0x17c-byte worker object,
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
#include <Gruntz/MapStringToOb.h>

// The worker virtual interface. Slots laid out so the dispatched method lands
// at byte offset +0x24. Declarations only - never defined, so no ??_7 emitted.
class AnimWorker : public CObject {
public:
    virtual ~AnimWorker() OVERRIDE; // slot 1 (deleting dtor); slots 0/2/3/4 inherited from CObject
    virtual void Slot05_151d60();        // [5] 0x151d60
    virtual void IsValidImage();         // [6] 0x001c08
    virtual void Clear();                // [7] 0x151e70 = Clear (B_151e70)
    virtual void Slot08_151d70();        // [8] 0x151d70
    virtual i32 Vfunc24(i32 a1, i32 a3); // [9] 0x151e20 (Init)
};

// The 0x17c-byte worker layout. Only the seeded offsets are load-bearing.
// Real polymorphic: `new AnimWorkerObj` makes cl auto-emit ??_7AnimWorkerObj
// (masks the retail vtable 0x5efb80) and stamp the vptr in the ctor - no manual
// `*(void**)w = &g_animWorkerVtbl` store (ALL-VTABLES mandate).
struct AnimWorkerObj : public AnimWorker {
    AnimWorkerObj() {}
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

// operator delete (called by the scalar-deleting dtor under the delete flag).
void operator delete(void*);

// The CObject-like family grand-base (vptr + the three header fields +0x04..+0x0c).
// Modeled as a REAL polymorphic base (its 5-slot vtable is the shared
// g_wapObjectDtorVtbl @0x5e8cb4) so cl emits the implicit grand-base vptr
// re-stamp (masks 0x5e8cb4) at the member-teardown dtor's tail - no manual
// `*(void**)this = &g_*Vtbl`. Slot 1 is a REGULAR virtual (not a C++ dtor) so the
// derived can override it with its explicit ??_G scalar-deleting destructor WITHOUT
// cl auto-generating a clashing ??_G. Same shape as CDDrawSubMgrGrandBase.
// NAME-AUDIT (vtable_hierarchy --name-audit): maps to RTTI CObject @0x1e8cb4, but
// KEPT as a real intermediate - it carries the m_04/m_08/m_0c header past the bare
// vptr, so it is NOT a bare-CObject fold (Wap32/Object.h). Do not rename to
// CObject (would ODR-clash + collapse the /GX dtor teardown level).
SIZE_UNKNOWN(CDDrawWorkerCacheBase);
// ---------------------------------------------------------------------------
// CDDrawWorkerCache - the CMapStringToOb at +0x10, and the parent fields copied
// into the worker (m_0c, m_1c from inside the map's internal area).
// ---------------------------------------------------------------------------
// Real polymorphic now (own 10-slot vtable ??_7CDDrawWorkerCache @0x5efd00, was
// Vtbl_1efd00 / the CDDrawWorkerCache vtable). Slots 0/2/3/4 are the shared CObject
// thunks, slot 1 the ??_G scalar-deleting dtor (0x157700), slots 5/6/7 unreconstructed
// leaf virtuals (declared-only, reloc-masked), slot 8 = GetStateId
// (0x1576f0) and slot 9 = CreateWorker (0x1652c0). cl auto-emits the vtable.
class CDDrawWorkerCache : public CObject {
public:
    i32 m_04, m_08, m_0c;                  // +0x04..0x0f (merged CDDrawWorkerCacheBase)
    virtual ~CDDrawWorkerCache() OVERRIDE; // [1] dtor 0x157720 (??_G 0x157700 pinned below)
    virtual void IsReady(); // [5] 0x1576d0 (= CDDrawWorkerRegistry::IsReady, declared-only)
    virtual void GetStateId_157790(); // [6] 0x157790 (= CDDrawSubMgr::GetStateId, declared-only)
    virtual void
    DestroyAll(); // [7] 0x165210 (= CDDrawWorkerRegistry::DestroyAll, defined in Registry TU)
    virtual StateId GetStateId();                                // [8] 0x1576f0
    virtual void* CreateWorker(i32 a1, const char* key, i32 a3); // [9] 0x1652c0

    // m_04/m_08/m_0c (and the implicit vptr) are inherited from CDDrawWorkerCacheBase.
    CMapStringToOb m_10; // +0x10  map (internal field at +0x1c seeds worker->m_04)
};

// Read field at +0x1c from the parent (inside the CMapStringToOb), used to
// seed worker->m_04.
static inline i32 ReadWorkerCacheField1c(const CDDrawWorkerCache* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

// ---------------------------------------------------------------------------
// Constant state ID: returns 0x13 (19).
// ---------------------------------------------------------------------------
RVA(0x001576f0, 0x6)
StateId CDDrawWorkerCache::GetStateId() {
    return STATE_WORKERCACHE; // 0x13
}

// Inline worker constructor. Real `new AnimWorkerObj`: the ctor stamps the vptr
// (cl-implicit, vptr-first) and cl auto-emits ??_7AnimWorkerObj; then seed the
// fields. (ALL-VTABLES mandate: the vptr store is now compiler-implicit, moving
// from vptr-middle to vptr-first - a code regression accepted for the real shape.)
static inline AnimWorkerObj* MakeAnimWorker(const CDDrawWorkerCache* parent) {
    AnimWorkerObj* w = new AnimWorkerObj;
    if (w != 0) {
        i32 field1c = ReadWorkerCacheField1c(parent);
        i32 surfaceMgr = parent->m_0c;
        w->m_04 = field1c;
        w->m_08 = 0;
        w->m_0c = surfaceMgr;
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
void* CDDrawWorkerCache::CreateWorker(i32 a1, const char* key, i32 a3) {
    AnimWorkerObj* w = MakeAnimWorker(this);

    if (w->Vfunc24(a1, a3) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_10[key] = (::CObject*)w;
    return w;
}

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (the vtable slot+4 override, ??_G at 0x157700): run
// the real member-teardown ~, then operator delete this if the low flag bit is set.
// SYMBOL() pins the ??_G mangling; it overrides CDDrawWorkerCacheBase's slot-1 regular
// virtual so the leaf vtable carries it at slot 1 WITHOUT cl auto-generating a
// clashing ??_G. Same idiom as CDDrawSubMgrLeaf::scalar-dtor.
// @rva-symbol: ??_GCDDrawWorkerCache@@UAEPAXI@Z 0x00157700 0x1e

// ---------------------------------------------------------------------------
// The real member-teardown destructor (0x157720, /GX): cl stamps ??_7CDDrawWorkerCache
// (masks 0x5efd00) at entry, runs the map teardown (FUN_00565210, the shared
// DestroyAll @0x165210), then destructs the CMapStringToOb member and the
// CDDrawWorkerCacheBase grand-base (field resets + implicit ??_7-base re-stamp masking
// 0x5e8cb4). No manual stamp. /GX member-teardown frame from the destructible map.
// @early-stop
// vptr-position wall (~95%, twin of CDDrawSubMgrLeaf/CDDrawWorker): every
// instruction matches retail EXCEPT the grand-base vptr re-stamp POSITION (cl emits
// the base-dtor entry stamp before the m_04/m_08/m_0c resets; retail sinks it after)
// + the reloc-masked EH-state/teardown/vtable symbol names. Logic complete.
RVA(0x00157720, 0x68)
CDDrawWorkerCache::~CDDrawWorkerCache() {
    DestroyAll();
    // implicit: ~m_10 (CMapStringToOb), then ~CDDrawWorkerCacheBase (field resets + the
    // grand-base ??_7 re-stamp) - reproduces retail's teardown order.
}

SIZE_UNKNOWN(CDDrawWorkerCache);
SIZE_UNKNOWN(AnimWorker);
SIZE(AnimWorkerObj, 0x17c);
VTBL(AnimWorkerObj, 0x001efb80); // ??_7AnimWorkerObj@@6B@ (10-slot vtable; the +0x7c/LogicRecord worker)
// ??_7CDDrawWorkerCache (was Vtbl_1efd00 / the CDDrawWorkerCache vtable; 10 slots). cl
// auto-emits it from the real-polymorphic class; retail datum is reloc-masked ->
// matching-neutral catalog tracking.
VTBL(CDDrawWorkerCache, 0x001efd00);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
