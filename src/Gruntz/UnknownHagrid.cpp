#include "../rva.h"
// UnknownHagrid.cpp - four leaf factory methods of the tomalla-named class
// UnknownHagrid (a CDirectDrawMgr surface/page sub-manager in the ddrawmgr
// "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// All four share ONE shape: allocate a 0x7c-byte "worker" object with the global
// operator new (??2@YAPAXI@Z @0x1b9b46), inline-construct it (zero/seed its
// fields + stamp its vftable), then call one of the worker's own sibling virtuals
// forwarding the caller's args. On a 0 result the worker is destroyed via its
// scalar-deleting destructor (vtable +0x4, arg 1) and the method returns 0; on a
// nonzero result the worker is appended to UnknownHagrid's CObList (+0x10) - either
// AddHead or AddTail, selected by a trailing bool arg - and the worker is returned.
//
// Two distinct worker vftables appear: 0x5efea0 (used by Unknown24, whose +0x78
// flag is a BYTE) and 0x5efed0 (used by Unknown28/2C/30, whose +0x78 flag is an
// int). Both worker layouts are otherwise identical and both carry a scalar-
// deleting destructor at vtable slot +0x4 (the delinker tags slot +0x4 of each as
// ??_G__non_rtti_object, the MFC delete-this thunk). The vftables are foreign
// engine data, referenced here as named `// @data:` DIR32 externs and stamped
// manually into the raw heap block; the worker classes are modeled as polymorphic
// (virtuals at the right slots) ONLY so `worker->Vfunc(...)` lowers to the exact
// `mov eax,[obj]; call [eax+slot]` __thiscall dispatch - their virtuals are never
// defined, so no vtable is emitted in this TU.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine). These are plain /O2 /MT leaves:
// NO SEH frame; the only relocations are the two reloc-masked rel32 library calls
// (operator new / CObList::AddHead/AddTail) and the DIR32 worker-vftable stores.
// ---------------------------------------------------------------------------

// --- MFC placeholders (only the call symbols / the 0x10 CObList offset matter) --
struct __POSITION;
class CObject;

// CObList lives at UnknownHagrid+0x10. AddHead/AddTail are out-of-line NAFXCW
// thunks (reloc-masked rel32 calls); declared with the exact MFC signatures so
// clang mangles them to ?AddHead@CObList@@... / ?AddTail@CObList@@... .
class CObList {
public:
    __POSITION *AddHead(CObject *newElement);
    __POSITION *AddTail(CObject *newElement);
};

// The worker virtual interface. Slots are laid out so the dispatched methods land
// at the byte offsets the target uses:
//   +0x04 (slot 1)  scalar-deleting destructor (delete-flag arg)
//   +0x2c (slot 11) Vfunc2C(3 args)   +0x30 (slot 12) Vfunc30(4 args)
//   +0x34 (slot 13) Vfunc34(4 args)
// Only declarations - never defined, so no ??_7 vtable is emitted here; the real
// vtable is the foreign engine datum stamped into the object below.
class HagridWorker {
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
    virtual void Slot28();              // +0x28
    virtual int  Vfunc2C(int a1, int a2, int a3);          // +0x2c
    virtual int  Vfunc30(int a1, int a2, int a3, int a4);  // +0x30
    virtual int  Vfunc34(int a1, int a2, int a3, int a4);  // +0x34
};

// The 0x7c-byte worker layouts. Only the seeded offsets are load-bearing; m_78 is
// the one field whose store width differs between the two workers (BYTE vs int).
struct HagridWorkerB : public HagridWorker {
    int   m_04;     // +0x04
    int   m_08;     // +0x08
    int   m_0c;     // +0x0c  = parent UnknownHagrid::m_pHarryPotter (+0xc)
    char  _pad10[0x20 - 0x10];
    int   m_20;     // +0x20  = 0x80000000
    char  _pad24[0x38 - 0x24];
    int   m_38;     // +0x38  = -1
    int   m_3c;     // +0x3c  = 0
    int   m_40;     // +0x40  = 0
    char  _pad44[0x5c - 0x44];
    int   m_5c;     // +0x5c  = 0x80000000
    char  _pad60[0x64 - 0x60];
    int   m_64;     // +0x64  = 0x80000000
    char  _pad68[0x78 - 0x68];
    int   m_78;     // +0x78  = 0 (int flag for the 0x5efed0 workers)
};                  // 0x7c

struct HagridWorkerA : public HagridWorker {
    int   m_04;     // +0x04
    int   m_08;     // +0x08
    int   m_0c;     // +0x0c
    char  _pad10[0x20 - 0x10];
    int   m_20;     // +0x20  = 0x80000000
    char  _pad24[0x38 - 0x24];
    int   m_38;     // +0x38  = -1
    int   m_3c;     // +0x3c  = 0
    int   m_40;     // +0x40  = 0
    char  _pad44[0x5c - 0x44];
    int   m_5c;     // +0x5c  = 0x80000000
    char  _pad60[0x64 - 0x60];
    int   m_64;     // +0x64  = 0x80000000
    char  _pad68[0x78 - 0x68];
    char  m_78;     // +0x78  = 0 (BYTE flag for the 0x5efea0 worker)
    char  _pad79[0x7c - 0x79];
};                  // 0x7c

// The two foreign worker vftables, referenced as DIR32 data (RVA = VA-0x400000).
DATA(0x1efea0)
extern void *g_hagridWorkerVtblA;   // VA 0x5efea0  (BYTE-flag worker)
DATA(0x1efed0)
extern void *g_hagridWorkerVtblB;   // VA 0x5efed0  (int-flag worker)

// ---------------------------------------------------------------------------
// UnknownHagrid - only the load-bearing offsets are modeled: m_pHarryPotter at
// +0x0c (copied into the worker) and the CObList at +0x10. The four factory
// methods occupy lower vtable slots (their slot numbers are not load-bearing;
// only their bodies are matched), so they are placed last.
// ---------------------------------------------------------------------------
class UnknownHagrid {
public:
    int   VirtualMethodUnknown14();
    void *VirtualMethodUnknown24(int a1, int a2, int a3);
    void *VirtualMethodUnknown28(int a1, int a2, int a3, int addHead);
    void *VirtualMethodUnknown2C(int a1, int a2, int a3, int a4, int addHead);
    void *VirtualMethodUnknown30(int a1, int a2, int a3, int a4, int addHead);

    void  *m_vptr;                  // +0x00 (vptr; not stamped by these methods)
    int    m_04;                    // +0x04  initialized to -1 when inactive
    char   m_pad08[0x0c - 0x08];    // +0x08..0x0b
    int    m_pHarryPotter;          // +0x0c  (UnknownCGruntzMgrLucius+0xc)
    CObList m_10;                   // +0x10  worker list (CObList)

    // Engine-label backlog stubs.
    void Stub_156f20();
    void Stub_156f50();
    void Stub_156fc0();
    void Stub_163bf0();
    void Stub_163c60();
};

// Stamps the worker's foreign vftable into its first dword (manual vptr store).
static inline void StampWorkerVtblB(HagridWorkerB *w) { *(void **)w = &g_hagridWorkerVtblB; }
static inline void StampWorkerVtblA(HagridWorkerA *w) { *(void **)w = &g_hagridWorkerVtblA; }

// ---------------------------------------------------------------------------
// UnknownHagrid::VirtualMethodUnknown14  @0x156f00  (__thiscall, ret 0)
// Same base readiness predicate used by several Lucius-derived managers.
RVA(0x156f00, 0x16)
int UnknownHagrid::VirtualMethodUnknown14()
{
    if (m_pHarryPotter == 0)
        goto fail;
    if (m_04 != -1)
        return 1;

fail:
    return 0;
}

// Inline worker constructors. Each new's the raw block, and on success seeds the
// fields THROUGH the allocation register and returns it; the null path returns 0.
// Defined inline so they fold into each factory, reproducing the target's "init
// via eax, commit to esi only at the merge" register schedule. The parent's
// m_pHarryPotter is read INSIDE the init (after the null check), not passed as a
// pre-evaluated argument, so its load is not hoisted above the new call.
static inline HagridWorkerB *MakeWorkerB(const UnknownHagrid *parent)
{
    HagridWorkerB *raw = (HagridWorkerB *)operator new(sizeof(HagridWorkerB));
    HagridWorkerB *w;
    if (raw != 0) {
        int harryPotter = parent->m_pHarryPotter;
        raw->m_04 = 0;
        raw->m_0c = harryPotter;
        raw->m_08 = 0;
        raw->m_20 = (int)0x80000000;
        raw->m_38 = -1;
        raw->m_5c = (int)0x80000000;
        raw->m_64 = (int)0x80000000;
        raw->m_3c = 0;
        raw->m_40 = 0;
        StampWorkerVtblB(raw);
        raw->m_78 = 0;
        w = raw;
    } else {
        w = 0;
    }
    return w;
}

static inline HagridWorkerA *MakeWorkerA(const UnknownHagrid *parent)
{
    HagridWorkerA *raw = (HagridWorkerA *)operator new(sizeof(HagridWorkerA));
    HagridWorkerA *w;
    if (raw != 0) {
        int harryPotter = parent->m_pHarryPotter;
        raw->m_04 = 0;
        raw->m_0c = harryPotter;
        raw->m_08 = 0;
        raw->m_20 = (int)0x80000000;
        raw->m_38 = -1;
        raw->m_5c = (int)0x80000000;
        raw->m_64 = (int)0x80000000;
        raw->m_3c = 0;
        raw->m_40 = 0;
        StampWorkerVtblA(raw);
        raw->m_78 = 0;
        w = raw;
    } else {
        w = 0;
    }
    return w;
}

// ---------------------------------------------------------------------------
// UnknownHagrid::VirtualMethodUnknown24  @0x156fd0  (__thiscall, ret 0xc)
// Allocates a BYTE-flag worker (vftable 0x5efea0), constructs it, calls its +0x2c
// virtual with (a1,a2,a3). On success appends it to the list (AddTail) and returns
// it; on failure destroys it and returns 0.
RVA(0x156fd0, 0x8b)
void *UnknownHagrid::VirtualMethodUnknown24(int a1, int a2, int a3)
{
    HagridWorkerA *w = MakeWorkerA(this);
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    m_10.AddTail((CObject *)w);
    return w;
}

// ---------------------------------------------------------------------------
// UnknownHagrid::VirtualMethodUnknown28  @0x1573e0  (__thiscall, ret 0x10)
// As Unknown24 but the int-flag worker (vftable 0x5efed0); on success the trailing
// bool selects AddHead vs AddTail.
RVA(0x1573e0, 0xa0)
void *UnknownHagrid::VirtualMethodUnknown28(int a1, int a2, int a3, int addHead)
{
    HagridWorkerB *w = MakeWorkerB(this);
    if (w->Vfunc2C(a1, a2, a3) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    if (addHead & 1)
        m_10.AddHead((CObject *)w);
    else
        m_10.AddTail((CObject *)w);
    return w;
}

// ---------------------------------------------------------------------------
// UnknownHagrid::VirtualMethodUnknown2C  @0x157330  (__thiscall, ret 0x14)
// Int-flag worker; calls the worker's +0x30 virtual with (a1,a2,a3,a4); trailing
// bool selects AddHead vs AddTail.
RVA(0x157330, 0xa5)
void *UnknownHagrid::VirtualMethodUnknown2C(int a1, int a2, int a3, int a4, int addHead)
{
    HagridWorkerB *w = MakeWorkerB(this);
    if (w->Vfunc30(a1, a2, a3, a4) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    if (addHead & 1)
        m_10.AddHead((CObject *)w);
    else
        m_10.AddTail((CObject *)w);
    return w;
}

// ---------------------------------------------------------------------------
// UnknownHagrid::VirtualMethodUnknown30  @0x157150  (__thiscall, ret 0x14)
// As Unknown2C but dispatches the worker's +0x34 virtual.
RVA(0x157150, 0xa5)
void *UnknownHagrid::VirtualMethodUnknown30(int a1, int a2, int a3, int a4, int addHead)
{
    HagridWorkerB *w = MakeWorkerB(this);
    if (w->Vfunc34(a1, a2, a3, a4) == 0) {
        if (w != 0)
            w->ScalarDtor(1);
        return 0;
    }
    if (addHead & 1)
        m_10.AddHead((CObject *)w);
    else
        m_10.AddTail((CObject *)w);
    return w;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: tomalla
// @stub
RVA(0x156f20, 0x6)
void UnknownHagrid::Stub_156f20() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x156f50, 0x68)
void UnknownHagrid::Stub_156f50() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x156fc0, 0x6)
void UnknownHagrid::Stub_156fc0() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x163bf0, 0x6d)
void UnknownHagrid::Stub_163bf0() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x163c60, 0x2c)
void UnknownHagrid::Stub_163c60() {}
