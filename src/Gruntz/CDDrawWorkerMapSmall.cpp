#include <rva.h>
// CDDrawWorkerMapSmall.cpp - leaf factory methods of the tomalla-named ddrawmgr surface-
// family sub-manager CDDrawWorkerMapSmall (a CDirectDrawMgr surface/page sub-manager in
// the "Harry Potter" family; see src/Stub/types/ddrawmgr_surface_family.h).
//
// CDDrawWorkerMapSmall owns a CMapStringToOb at +0x10 (m_unknownMap1) keyed by const char*
// strings. Unknown28/2C share ONE factory shape: allocate a 0x14-byte "worker" with
// the global operator new, inline-construct it (seed its
// fields from the parent + stamp the foreign worker vftable), then call one
// of the worker's sibling virtuals (+0x28 for Unknown28, +0x2c for Unknown2C)
// forwarding (arg1, arg3). On a 0 result the worker is destroyed via its scalar-
// deleting destructor (vtable +0x4, arg 1) and 0 is returned; on a nonzero result
// the worker is stored into the map under `key` (arg2) via CMapStringToOb::operator[]
// and returned.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine). These are plain /O2 /MT leaves: NO SEH frame; the
// only relocations are the reloc-masked rel32 library calls (operator new /
// CMapStringToOb::operator[]) and the DIR32 worker-vftable store. The worker is
// modeled as polymorphic (virtuals at the right slots) ONLY so `w->Vfunc(...)`
// lowers to the exact `mov eax,[w]; call [eax+slot]` __thiscall dispatch; its
// virtuals are never defined, so no vtable is emitted in this TU - the real vtable
// is the foreign engine datum stamped manually into the heap block.
//
// The TU is /GX ("eh" flag): besides the frameless /O2 factory leaves it now also
// carries the map-teardown VirtualMethodUnknown1C (0x165810, a /GX CString-iteration
// loop, byte-exact) and the /GX member-teardown destructor ~CDDrawWorkerMapSmall
// (0x156d20). /GX adds a frame only to the functions that need one; the factory
// leaves + VirtualMethodUnknown14/20 stay frameless and byte-exact. The full class
// shape recovered from the destructor: a base sub-object at +0x00..+0x0f (vptr + 3
// fields) and THREE CMapStringToOb maps at +0x10/+0x2c/+0x48, then an i32 counter at
// +0x64 (the factories + VirtualMethodUnknown14/20 only touch <=+0x2b).
// ---------------------------------------------------------------------------

// --- MFC placeholders (only the call symbols + the 0x10 map offset matter) -----
class CObject;

// CMapStringToOb lives at CDDrawWorkerMapSmall+0x10. operator[] / GetNextAssoc /
// RemoveAll / GetCount / ~CMapStringToOb are out-of-line NAFXCW thunks (reloc-masked
// rel32 calls); the REAL MFC CMapStringToOb (via <Mfc.h>) supplies them with the
// MFC-canonical mangling. <Mfc.h> also brings CString / POSITION for the teardown.
#include <Gruntz/CMapStringToOb.h>
#include <Gruntz/CString.h>

// The worker virtual interface. Slots laid out so the dispatched methods land at
// the byte offsets the target uses: +0x04 scalar-deleting dtor, +0x28/+0x2c the
// two factory siblings. Declarations only - never defined, so no ??_7 emitted.
class AlbusWorker {
public:
    virtual void Slot00();               // +0x00
    virtual i32 ScalarDtor(i32 flag);    // +0x04  scalar-deleting destructor
    virtual void Slot08();               // +0x08
    virtual void Slot0C();               // +0x0c
    virtual void Slot10();               // +0x10
    virtual void Slot14();               // +0x14
    virtual void Slot18();               // +0x18
    virtual void Slot1C();               // +0x1c
    virtual void Slot20();               // +0x20
    virtual void Slot24();               // +0x24
    virtual i32 Vfunc28(i32 a1, i32 a3); // +0x28
    virtual i32 Vfunc2C(i32 a1, i32 a3); // +0x2c
};

// The 0x14-byte worker layout. Only the seeded offsets are load-bearing.
struct AlbusWorkerObj : public AlbusWorker {
    i32 m_04; // +0x04  = parent->m_1c (an internal field of map1)
    i32 m_08; // +0x08  = 0
    i32 m_0c; // +0x0c  = parent->m_0c (the HarryPotter handle)
    i32 m_10; // +0x10  = 0
}; // 0x14

// The foreign worker vftable, referenced as DIR32 data (RVA = VA-0x400000). The
// worker class's own ctor/full vtable live in other (unmatched) TUs, so this stays
// the transitional manual stamp; the worker is never constructed here, only the
// raw heap block is stamped + dispatched.
DATA(0x001f02d8)
extern void* g_albusWorkerVtbl;

// The CObject-like grand-base shared by the whole "Harry Potter" surface family
// (its dtor vtable is g_remusBaseDtorVtbl @0x5e8cb4 = the 5-slot CObject interface
// sub_1bef01 / scalar-dtor / sub_0028ec / sub_00106e / sub_004034). Modeled as a
// REAL polymorphic base so cl emits the implicit grand-base vptr re-stamp (masks
// 0x5e8cb4) at ~CDDrawWorkerMapSmall's tail and stamps ??_7CDDrawWorkerMapSmall at
// its entry - no manual `*(void**)this = &g_*Vtbl`. The vptr is implicit at +0x00;
// the three managed fields (reset on teardown) follow. The base's virtual dtor body
// holds the field resets; being polymorphic + having the destructible CMapStringToOb
// members gives ~CDDrawWorkerMapSmall its /GX frame.
struct AlbusMapBase {
    virtual void Slot00();   // [0] grand-base sub_1bef01
    virtual ~AlbusMapBase(); // [1] scalar-deleting dtor
    virtual void Slot08();   // [2] sub_0028ec
    virtual void Slot0C();   // [3] sub_00106e
    virtual void Slot10();   // [4] sub_004034

    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c  parent HarryPotter handle
    AlbusMapBase() {}
};

// Field resets only -> cl emits the implicit ??_7-base re-stamp (masks 0x5e8cb4) as
// the dtor's tail. (vptr-position wall: cl sinks the stamp before/after the resets
// per its EH-state schedule; see ~CDDrawWorkerMapSmall.)
inline AlbusMapBase::~AlbusMapBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerMapSmall - only the load-bearing offsets are modeled: m_0c (parent handle,
// copied into the worker), m_1c (a CMapStringToOb-internal field of map1 also
// copied into the worker), and the CMapStringToOb at +0x10. The matched methods
// occupy lower vtable slots (slot numbers not load-bearing, only bodies), placed
// last.
// ---------------------------------------------------------------------------
class CDDrawWorkerMapSmall : public AlbusMapBase {
public:
    // The leaf vtable (??_7CDDrawWorkerMapSmall @0x5efcc8) is 13 slots: the 5 shared
    // CObject slots from AlbusMapBase, then 8 leaf virtuals at slots 5..12. They are
    // declared here in slot order so cl lays the emitted vtable out byte-for-byte
    // (the unreconstructed slots 6/8 are declared-only -> reloc-masked references).
    virtual i32 VirtualMethodUnknown14();  // [5]  0x156cd0
    virtual void Slot18_156db0();          // [6]  0x156db0 (declared-only)
    virtual void VirtualMethodUnknown1C(); // [7]  0x165810
    virtual void Slot20_156cf0();          // [8]  0x156cf0 (declared-only)
    virtual void Stub_1658c0();            // [9]  0x1658c0
    virtual void* VirtualMethodUnknown28(i32 a1, const char* key, i32 a3); // [10] 0x165990
    virtual void* VirtualMethodUnknown2C(i32 a1, const char* key, i32 a3); // [11] 0x165a10
    virtual void Stub_165a90();                                            // [12] 0x165a90
    virtual ~CDDrawWorkerMapSmall();                                       // overrides slot [1]

    // VirtualMethodUnknown20 (0x157600) is NOT a vtable slot - a plain method.
    i32 VirtualMethodUnknown20();

    // m_04/m_08/m_0c (and the implicit vptr) are inherited from AlbusMapBase.
    CMapStringToOb m_map1; // +0x10  worker-by-key map 1 (0x10..0x2b)
    CMapStringToOb m_map2; // +0x2c  worker-by-key map 2 (0x2c..0x47)
    CMapStringToOb m_map3; // +0x48  worker-by-key map 3 (0x48..0x63)
    i32 m_64;              // +0x64  entry counter cleared by the teardown

    // Engine-label backlog stubs (non-vtable).
    void Stub_157610();
    void Stub_165b90();
};

// CMapStringToOb internal field at parent+0x1c (seeds worker->m_04). Read off the
// parent so it reloc-frees as `mov ecx,[edi+0x1c]`.
static inline i32 AlbusReadField1c(const CDDrawWorkerMapSmall* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

// Stamps the worker's foreign vftable into its first dword (manual vptr store).
static inline void StampAlbusWorkerVtbl(AlbusWorkerObj* w) {
    *(void**)w = &g_albusWorkerVtbl;
}

// ---------------------------------------------------------------------------
// Reports ready when the parent/root handle is present and the base status word
// is no longer the inactive -1 sentinel.
RVA(0x00156cd0, 0x16)
i32 CDDrawWorkerMapSmall::VirtualMethodUnknown14() {
    if (m_0c == 0) {
        goto fail;
    }
    if (m_04 != -1) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// ~CDDrawWorkerMapSmall (0x156d20, __thiscall, /GX): now a REAL virtual dtor. cl
// stamps ??_7CDDrawWorkerMapSmall (masks g_albusClassVtbl @0x5efcc8) at entry, runs
// the map teardown (VirtualMethodUnknown1C), then destructs the three CMapStringToOb
// members (reverse decl order, descending trylevels) and the AlbusMapBase grand-base
// (field resets + implicit ??_7-base re-stamp masking 0x5e8cb4). No manual stamp.
// The /GX member-teardown frame falls out of the destructible CMapStringToOb members
// + the polymorphic base (docs/patterns/eh-dtor-model-members-as-destructible.md).
//
// @early-stop
// vptr-position wall (~94%, twin of CSeverusEntryList::~CSeverusEntryList): every
// instruction matches retail EXCEPT the grand-base vptr re-stamp POSITION - cl emits
// `mov [esi],??_7AlbusMapBase` BEFORE the m_04/m_08/m_0c resets (base-dtor entry
// stamp), retail sinks it AFTER (verified llvm-objdump -dr base vs target on
// CSeverusEntryList: base 0x41 stamp-then-resets, target 0x9a resets-then-stamp). The
// implicit base transition forces stamp-first; not source-steerable (the manual model
// could write the stamp last, but that is the decompiler hack we are removing - real
// devs' polymorphic shape per correctness-not-artifacts). Plus the reloc-masked EH
// Unwind/handler/member-dtor/vtable symbol names. Logic complete.
RVA(0x00156d20, 0x82)
CDDrawWorkerMapSmall::~CDDrawWorkerMapSmall() {
    VirtualMethodUnknown1C();
    // m_map3 / m_map2 / m_map1 (reverse decl order) and the AlbusMapBase grand-base
    // auto-destruct here under the /GX member-teardown trylevels.
}

// Inline worker constructor. New's the raw 0x14 block; on success seeds the fields
// THROUGH the allocation register and returns it, else returns 0. Defined inline so
// it folds into each factory, reproducing the target's "init via eax, commit to esi
// at the merge" register schedule. The parent fields are read INSIDE the init (after
// the null check) so their loads are not hoisted above the new call.
//
// RESIDUE (~99.74%, NOT a logic/offset/type/CFG error): the two interchangeable
// parent loads that seed worker->m_04 (from parent+0x1c) and worker->m_0c (from
// parent+0xc) get the OPPOSITE register pair vs the target - the retail compiler
// loads parent+0xc into ECX and parent+0x1c into EDX (storing EDX->+0x4, ECX->+0xc),
// while MSVC5 on this source picks ECX=parent+0x1c, EDX=parent+0xc. Same values,
// same offsets, same stores; only the ecx<->edx pairing flips. This is the
// documented register-allocation coin-flip (matching-patterns.md / match-learnings.md
// "tag<->key ebx/edi register-alloc coin-flip"): both reads are independent and
// equally-live, so the allocator's choice is name-/order-independent here. Every
// source ordering of the two reads and the two stores produced the identical pairing;
// no source lever flips it. All 42 other instructions + both ret paths byte-exact.
static inline AlbusWorkerObj* MakeAlbusWorker(const CDDrawWorkerMapSmall* parent) {
    AlbusWorkerObj* raw = (AlbusWorkerObj*)operator new(sizeof(AlbusWorkerObj));
    AlbusWorkerObj* w;
    if (raw != 0) {
        i32 harryPotter = parent->m_0c;
        raw->m_04 = AlbusReadField1c(parent);
        raw->m_08 = 0;
        raw->m_0c = harryPotter;
        StampAlbusWorkerVtbl(raw);
        raw->m_10 = 0;
        w = raw;
    } else {
        w = 0;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Map teardown (0x165810, __thiscall, /GX): iterate every entry of m_map1 via
// GetNextAssoc, destroying each CObject* value through its scalar-deleting
// destructor (vtbl +0x4, arg 1), RemoveAll the map, then clear the +0x64 counter.
// Same shape as CDDrawWorkerRegistry::VirtualMethodUnknown58, plus the m_64 clear.
// Carries a /GX EH frame for the local CString key (must be unwound through the
// iteration loop).
RVA(0x00165810, 0xa9)
void CDDrawWorkerMapSmall::VirtualMethodUnknown1C() {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_map1.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != 0) {
                ((AlbusWorker*)val)->ScalarDtor(1);
            }
        } while (pos != 0);
    }
    m_map1.RemoveAll();
    m_64 = 0;
}

// ---------------------------------------------------------------------------
// Allocate + construct a worker, call its +0x28 virtual with (arg1, arg3). On
// success store it into the map under `key` and return it; on failure run its
// scalar-deleting dtor and return 0.
RVA(0x00165990, 0x77)
void* CDDrawWorkerMapSmall::VirtualMethodUnknown28(i32 a1, const char* key, i32 a3) {
    AlbusWorkerObj* w = MakeAlbusWorker(this);
    if (w->Vfunc28(a1, a3) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    m_map1[key] = (CObject*)w;
    return w;
}

// ---------------------------------------------------------------------------
// As Unknown28 but dispatches the worker's +0x2c virtual.
RVA(0x00165a10, 0x77)
void* CDDrawWorkerMapSmall::VirtualMethodUnknown2C(i32 a1, const char* key, i32 a3) {
    AlbusWorkerObj* w = MakeAlbusWorker(this);
    if (w->Vfunc2C(a1, a3) == 0) {
        if (w != 0) {
            w->ScalarDtor(1);
        }
        return 0;
    }
    m_map1[key] = (CObject*)w;
    return w;
}

// ---------------------------------------------------------------------------
// Constant state id.
// ---------------------------------------------------------------------------
RVA(0x00157600, 0x6)
i32 CDDrawWorkerMapSmall::VirtualMethodUnknown20() {
    return 0x10;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: tomalla
// @stub
RVA(0x00157610, 0x1e)
void CDDrawWorkerMapSmall::Stub_157610() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x001658c0, 0xcc)
void CDDrawWorkerMapSmall::Stub_1658c0() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00165a90, 0xf4)
void CDDrawWorkerMapSmall::Stub_165a90() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00165b90, 0xa9)
void CDDrawWorkerMapSmall::Stub_165b90() {}

// ---------------------------------------------------------------------------
// WIP (DO NOT ENABLE AS-IS): reconstructed CDDrawWorkerMapSmall::VirtualMethodUnknown1C
// reaches ~82% objdiff here; the EH-frame register/layout schedule is TU-context
// dependent (it reached 100% only in its original TU layout). Kept for future reuse:
// re-enable, remove Stub_165b90, and reconcile this TU's symbol/layout context.
//
// This block carries the full reconstruction PLUS every supporting type it needs
// that the active file above does not already declare (POSITION, CString,
// CMapStringToOb's GetNextAssoc/RemoveAll + internal layout incl. m_nCount, and
// the map-value scalar-deleting-dtor type). Annotations are in macro form so the
// block is drop-in re-enableable. Body copied byte-faithfully from the reference.
// ---------------------------------------------------------------------------
#if 0
// POSITION is the opaque MFC iteration handle.  Use the native integer form so
// `int pos` variables from the ternary guard initialiser pass cleanly to
// GetNextAssoc's POSITION & parameter without a type mismatch; the reloc-masked
// call site is identical and the null/-1 sentinel values compile to the same
// `test reg,reg` / `cmp reg,-1` comparisons a pointer typedef would produce.
typedef i32 POSITION;

// CString (4-byte char* wrapper). Only the default ctor + dtor are needed (used
// as GetNextAssoc's key output, then destroyed). The class body is minimal so
// the compiler generates plain out-of-line ctor/dtor calls and an SEH scope
// table around the iteration.
#include <Gruntz/CString.h>

// CMapStringToOb extended for the teardown: GetNextAssoc/RemoveAll are out-of-line
// NAFXCW thunks (reloc-masked rel32 calls); declared with the exact MFC signatures
// so clang mangles them to the MFC-canonical names. The internal layout fields
// (vptr through m_nCount) are modeled so `m_map1.m_nCount` accesses parent+0x1c,
// matching the retail `mov eax,[edi+0x1c]` for the guard condition.

// Minimal polymorphic stub for the CObject-derived values stored in the map.
// Only the scalar-deleting destructor (+0x04) is load-bearing.
class AlbusMapValue {
public:
    virtual void Dummy();               // +0x00
    virtual i32  ScalarDtor(i32 flag);  // +0x04
};

// CDDrawWorkerMapSmall surface used by the teardown - same load-bearing offsets as the
// active class above, with the m_map1 map typed for the teardown signatures.
class UnknownAlbusTeardown {
public:
    void  VirtualMethodUnknown1C();

    void                  *m_vptr;                  // +0x00
    i32                    m_04;                     // +0x04
    char                   m_pad08[0x0c - 0x08];     // +0x08..0x0b
    i32                    m_0c;                      // +0x0c
    CMapStringToOb m_map1;                    // +0x10  worker-by-key map 1 (0x10..0x2b)
};

// ---------------------------------------------------------------------------
// Map teardown: iterate all entries in m_map1 via GetNextAssoc, destroying each
// CObject* value via its scalar-deleting destructor (vtbl +0x4 arg 1), then
// RemoveAll the map and clear the m_64 counter.
//
// Carries a /GX EH frame for the local CString key (destructor must fire on
// unwind through the iteration loop). Uses the guard variable (gated on the
// CMapStringToOb internal m_nCount) as the POSITION for GetNextAssoc; the
// compiler folds the `(m_nCount != 0) ? -1 : 0` initialisation into the
// `neg; sbb` mask-and-store idiom ahead of the CString ctor call.
// ---------------------------------------------------------------------------
// on re-enable, annotate this definition with the retail address 0x165b90
// size 0xa9 (the macro literal is omitted here so verify_stub_labels' text
// scan does not treat this preserved WIP copy as a duplicate of Stub_165b90)
void UnknownAlbusTeardown::VirtualMethodUnknown1C()
{
    CObject *val = 0;
    i32 pos = (m_map1.m_nCount != 0) ? -1 : 0;
    CString key;
    if (*(volatile i32 *)&pos != 0) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != 0)
                ((AlbusMapValue *)val)->ScalarDtor(1);
        } while (*(volatile i32 *)&pos != 0);
    }
    m_map1.RemoveAll();
    *(i32 *)((char *)this + 0x64) = 0;
}
#endif
