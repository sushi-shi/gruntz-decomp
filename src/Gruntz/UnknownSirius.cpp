// UnknownSirius.cpp - leaf methods of the tomalla-named class UnknownSirius
// (a CDirectDrawMgr surface/page sub-manager in the "Harry Potter" family).
// VirtualMethodUnknown20 is a constant state-ID stub returning 0x13 (19).
// VirtualMethodUnknown24 is a factory: allocates a 0x17c-byte worker object,
// seeds it from parent fields, stamps the foreign vftable 0x5efb80, calls the
// worker's vtable+0x24 virtual with (arg1, arg3), on failure destroys the
// worker and returns 0, on success stores the worker into the CMapStringToOb
// at +0x10 under `key` (arg2) and returns it.
//
// Both are plain /O2 /MT leaves: NO SEH frame. The factory has reloc-masked
// rel32 calls (operator new @0x1b9b46 / CMapStringToOb::operator[] @0x1b804c)
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

// CMapStringToOb lives at UnknownSirius+0x10. operator[] is an out-of-line
// NAFXCW thunk (reloc-masked rel32 call).
class CMapStringToOb {
public:
    CObject *&operator[](const char *key);      // @0x1b804c
};

// The worker virtual interface. Slots laid out so the dispatched method lands
// at byte offset +0x24. Declarations only - never defined, so no ??_7 emitted.
class SiriusWorker {
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
    virtual int  Vfunc24(int a1, int a3);   // +0x24
};

// The 0x17c-byte worker layout. Only the seeded offsets are load-bearing.
struct SiriusWorkerObj : public SiriusWorker {
    int   m_04;                     // +0x04  = parent->m_1c
    int   m_08;                     // +0x08  = 0
    int   m_0c;                     // +0x0c  = parent->m_0c
    int   m_10;                     // +0x10  = 0
    int   m_14;                     // +0x14  = 0
    int   m_18;                     // +0x18  = 0
    int   m_1c;                     // +0x1c  = 0
    char  m_pad20[0x170 - 0x20];
    int   m_170;                    // +0x170 = 0
    int   m_174;                    // +0x174 = 0
    int   m_178;                    // +0x178 = 0
};  // size = 0x17c

// The foreign worker vftable, referenced as DIR32 data (RVA = VA-0x400000).
// @data: 0x1efb80
extern void *g_siriusWorkerVtbl;   // VA 0x5efb80

static inline void StampSiriusVtbl(SiriusWorkerObj *w) { *(void **)w = &g_siriusWorkerVtbl; }

// ---------------------------------------------------------------------------
// UnknownSirius - the CMapStringToOb at +0x10, and the parent fields copied
// into the worker (m_0c, m_1c from inside the map's internal area).
// ---------------------------------------------------------------------------
class UnknownSirius {
public:
    int   VirtualMethodUnknown20();
    void *VirtualMethodUnknown24(int a1, const char *key, int a3);

    void          *m_vptr;                  // +0x00
    int            m_04;                    // +0x04  -1 when inactive
    char           m_pad08[0x0c - 0x08];    // +0x08..0x0b
    int            m_0c;                    // +0x0c  parent/root handle
    CMapStringToOb m_10;                    // +0x10  map (internal field at +0x1c seeds worker->m_04)
};

// Read field at +0x1c from the parent (inside the CMapStringToOb), used to
// seed worker->m_04.
static inline int SiriusReadField1c(const UnknownSirius *p)
{
    return *(const int *)((const char *)p + 0x1c);
}

// ---------------------------------------------------------------------------
// UnknownSirius::VirtualMethodUnknown20  @0x1576f0  (__thiscall, ret 0)
// Constant state ID: returns 0x13 (19).
// ---------------------------------------------------------------------------
// @address: 0x1576f0
// @size:    0x6
int UnknownSirius::VirtualMethodUnknown20()
{
    return 0x13;
}

// Inline worker constructor. New's the raw 0x17c block; on success seeds
// the fields in the exact order the target writes them.
static inline SiriusWorkerObj *MakeSiriusWorker(const UnknownSirius *parent)
{
    SiriusWorkerObj *raw = (SiriusWorkerObj *)operator new(sizeof(SiriusWorkerObj));
    SiriusWorkerObj *w;
    if (raw != 0) {
        int field1c = SiriusReadField1c(parent);
        int harryPotter = parent->m_0c;
        raw->m_04 = field1c;
        raw->m_08 = 0;
        raw->m_0c = harryPotter;
        StampSiriusVtbl(raw);
        raw->m_10 = 0;
        raw->m_14 = 0;
        raw->m_18 = 0;
        raw->m_170 = 0;
        raw->m_1c = 0;
        raw->m_174 = 0;
        raw->m_178 = 0;
        w = raw;
    } else {
        w = 0;
    }
    return w;
}

// ---------------------------------------------------------------------------
// UnknownSirius::VirtualMethodUnknown24  @0x1652c0  (__thiscall, ret 0xc)
// Allocate + construct a 0x17c-byte worker, call its +0x24 virtual with
// (arg1, arg3). On success store it into the map under `key` and return it;
// on failure run its scalar-deleting dtor and return 0.
//
// NOTE: Vfunc24 is dispatched on the worker BEFORE the null check, matching
// the target asm (the NAFXCW operator new practically never fails).
// ---------------------------------------------------------------------------
// @address: 0x1652c0
// @size:    0x92
void *UnknownSirius::VirtualMethodUnknown24(int a1, const char *key, int a3)
{
    SiriusWorkerObj *w = MakeSiriusWorker(this);

    if (w->Vfunc24(a1, a3) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    m_10[key] = (CObject *)w;
    return w;
}
