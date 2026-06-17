// UnknownAlbus.cpp - leaf factory methods of the tomalla-named ddrawmgr surface-
// family sub-manager UnknownAlbus (a CDirectDrawMgr surface/page sub-manager in
// the "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// UnknownAlbus owns a CMapStringToOb at +0x10 (m_unknownMap1) keyed by const char*
// strings. Unknown28/2C share ONE factory shape: allocate a 0x14-byte "worker" with
// the global operator new (??2@YAPAXI@Z @0x1b9b46), inline-construct it (seed its
// fields from the parent + stamp the foreign worker vftable 0x5f02d8), then call one
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
// SIBLING DEFERRED: UnknownAlbus::VirtualMethodUnknown1C @0x165b90 (169 B) is the
// map-teardown counterpart; it carries a C++ EH frame (a stack CString iteration
// key with a destructor) and a subtle GetNextAssoc loop with a per-iteration
// stack-layout shift. Deferred to its own pass to keep this factory TU /O2 /MT and
// free of /GX entropy.
// ---------------------------------------------------------------------------

// --- MFC placeholders (only the call symbols + the 0x10 map offset matter) -----
class CObject;

// CMapStringToOb lives at UnknownAlbus+0x10. operator[] is an out-of-line NAFXCW
// thunk (reloc-masked rel32 call); declared with the exact MFC signature so clang
// mangles it to ??ACMapStringToOb@@QAEAAPAVCObject@@PBD@Z .
class CMapStringToOb {
public:
    CObject *&operator[](const char *key);      // @0x1b804c
};

// The worker virtual interface. Slots laid out so the dispatched methods land at
// the byte offsets the target uses: +0x04 scalar-deleting dtor, +0x28/+0x2c the
// two factory siblings. Declarations only - never defined, so no ??_7 emitted.
class AlbusWorker {
public:
    virtual void Slot00();              // +0x00
    virtual int  ScalarDtor(int flag);  // +0x04  scalar-deleting destructor
    virtual void Slot08();              // +0x08
    virtual void Slot0C();              // +0x0c
    virtual void Slot10();              // +0x10
    virtual void Slot14();              // +0x14
    virtual void Slot18();              // +0x18
    virtual void Slot1C();              // +0x1c
    virtual void Slot20();              // +0x20
    virtual void Slot24();              // +0x24
    virtual int  Vfunc28(int a1, int a3);   // +0x28
    virtual int  Vfunc2C(int a1, int a3);   // +0x2c
};

// The 0x14-byte worker layout. Only the seeded offsets are load-bearing.
struct AlbusWorkerObj : public AlbusWorker {
    int m_04;   // +0x04  = parent->m_1c (an internal field of map1)
    int m_08;   // +0x08  = 0
    int m_0c;   // +0x0c  = parent->m_0c (the HarryPotter handle)
    int m_10;   // +0x10  = 0
};              // 0x14

// The foreign worker vftable, referenced as DIR32 data (RVA = VA-0x400000).
// @data: 0x1f02d8
extern void *g_albusWorkerVtbl;     // VA 0x5f02d8

// ---------------------------------------------------------------------------
// UnknownAlbus - only the load-bearing offsets are modeled: m_0c (parent handle,
// copied into the worker), m_1c (a CMapStringToOb-internal field of map1 also
// copied into the worker), and the CMapStringToOb at +0x10. The matched methods
// occupy lower vtable slots (slot numbers not load-bearing, only bodies), placed
// last.
// ---------------------------------------------------------------------------
class UnknownAlbus {
public:
    int   VirtualMethodUnknown14();
    void *VirtualMethodUnknown28(int a1, const char *key, int a3);
    void *VirtualMethodUnknown2C(int a1, const char *key, int a3);

    void          *m_vptr;                  // +0x00
    int            m_04;                     // +0x04  initialized to -1 when inactive
    char           m_pad08[0x0c - 0x08];    // +0x08..0x0b
    int            m_0c;                     // +0x0c  parent HarryPotter handle
    CMapStringToOb m_10;                     // +0x10  m_unknownMap1 (0x10..0x2b)
};

// CMapStringToOb internal field at parent+0x1c (seeds worker->m_04). Read off the
// parent so it reloc-frees as `mov ecx,[edi+0x1c]`.
static inline int AlbusReadField1c(const UnknownAlbus *p)
{
    return *(const int *)((const char *)p + 0x1c);
}

// Stamps the worker's foreign vftable into its first dword (manual vptr store).
static inline void StampAlbusWorkerVtbl(AlbusWorkerObj *w) { *(void **)w = &g_albusWorkerVtbl; }

// ---------------------------------------------------------------------------
// UnknownAlbus::VirtualMethodUnknown14  @0x156cd0  (__thiscall, ret 0)
// Reports ready when the parent/root handle is present and the base status word
// is no longer the inactive -1 sentinel.
// ---------------------------------------------------------------------------
// @address: 0x156cd0
// @size:    0x16
int UnknownAlbus::VirtualMethodUnknown14()
{
    if (m_0c == 0)
        goto fail;
    if (m_04 != -1)
        return 1;

fail:
    return 0;
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
static inline AlbusWorkerObj *MakeAlbusWorker(const UnknownAlbus *parent)
{
    AlbusWorkerObj *raw = (AlbusWorkerObj *)operator new(sizeof(AlbusWorkerObj));
    AlbusWorkerObj *w;
    if (raw != 0) {
        int harryPotter = parent->m_0c;
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
// UnknownAlbus::VirtualMethodUnknown28  @0x165990  (__thiscall, ret 0xc)
// Allocate + construct a worker, call its +0x28 virtual with (arg1, arg3). On
// success store it into the map under `key` and return it; on failure run its
// scalar-deleting dtor and return 0.
// ---------------------------------------------------------------------------
// @address: 0x165990
// @size:    0x77
void *UnknownAlbus::VirtualMethodUnknown28(int a1, const char *key, int a3)
{
    AlbusWorkerObj *w = MakeAlbusWorker(this);
    if (w->Vfunc28(a1, a3) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    m_10[key] = (CObject *)w;
    return w;
}

// ---------------------------------------------------------------------------
// UnknownAlbus::VirtualMethodUnknown2C  @0x165a10  (__thiscall, ret 0xc)
// As Unknown28 but dispatches the worker's +0x2c virtual.
// ---------------------------------------------------------------------------
// @address: 0x165a10
// @size:    0x77
void *UnknownAlbus::VirtualMethodUnknown2C(int a1, const char *key, int a3)
{
    AlbusWorkerObj *w = MakeAlbusWorker(this);
    if (w->Vfunc2C(a1, a3) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    m_10[key] = (CObject *)w;
    return w;
}
