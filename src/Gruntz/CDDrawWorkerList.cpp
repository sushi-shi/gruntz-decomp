#include <rva.h>
// CDDrawWorkerList.cpp - four leaf factory methods of the tomalla-named class
// CDDrawWorkerList (a CDirectDrawMgr surface/page sub-manager in the ddrawmgr
// "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// All four share ONE shape: allocate a 0x7c-byte "worker" object with the global
// operator new, inline-construct it (zero/seed its
// fields + stamp its vftable), then call one of the worker's own sibling virtuals
// forwarding the caller's args. On a 0 result the worker is destroyed via its
// scalar-deleting destructor (vtable +0x4, arg 1) and the method returns 0; on a
// nonzero result the worker is appended to CDDrawWorkerList's CObList (+0x10) - either
// AddHead or AddTail, selected by a trailing bool arg - and the worker is returned.
//
// Two distinct worker vftables appear: one used by Unknown24, whose +0x78
// flag is a BYTE, and one used by Unknown28/2C/30, whose +0x78 flag is an
// int. Both worker layouts are otherwise identical and both carry a scalar-
// deleting destructor at vtable slot +0x4 (the delinker tags slot +0x4 of each as
// the MFC delete-this thunk). The vftables are foreign
// engine data, referenced here as named `DATA(...)` DIR32 externs and stamped
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

// CObList lives at CDDrawWorkerList+0x10. AddHead/AddTail are out-of-line NAFXCW
// thunks (reloc-masked rel32 calls); declared with the exact MFC signatures so
// clang mangles them to the MFC-canonical names.
// RemoveAll / RemoveAt are called by VirtualMethodUnknown1C and Unknown34.
class CObList {
public:
    __POSITION *AddHead(CObject *newElement);
    __POSITION *AddTail(CObject *newElement);
    void RemoveAll();
    void RemoveAt(__POSITION *position);
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
    int   m_0c;     // +0x0c  = parent CDDrawWorkerList::m_pHarryPotter (+0xc)
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
    int   m_78;     // +0x78  = 0 (int flag for the int-flag workers)
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
    char  m_78;     // +0x78  = 0 (BYTE flag for the BYTE-flag worker)
    char  _pad79[0x7c - 0x79];
};                  // 0x7c

// The two foreign worker vftables, referenced as DIR32 data (RVA = VA-0x400000).
DATA(0x1efea0)
extern void *g_hagridWorkerVtblA;   // (BYTE-flag worker)
DATA(0x1efed0)
extern void *g_hagridWorkerVtblB;   // (int-flag worker)

// The child type used in the work-node linked-list at +0x14.
// Virtual slot +0x28 (Vfunc28) is dispatched in Unknown34; +0x74 is a
// reference count. Slot layout matches the child class in the engine.
class HagridChild {
public:
    virtual void Slot00();
    virtual int  ScalarDtor(int flag);          // +0x04
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual int  Vfunc28(int a1, int a2);       // +0x28
    char _pad04[0x74 - 0x04];                   // +0x04..+0x73
    int  m_74;                                  // +0x74  reference count
};

// Work nodes (CObList CNode-shaped: +0x00 = next, +0x08 = child pointer).
struct WorkNode {
    WorkNode  *m_next;     // +0x00
    int        m_discard;  // +0x04  (not accessed; prev ptr or padding)
    HagridChild *m_child;  // +0x08
};

// ---------------------------------------------------------------------------
// CDDrawWorkerList - only the load-bearing offsets are modeled: m_pHarryPotter at
// +0x0c (copied into the worker) and the CObList at +0x10. The four factory
// methods occupy lower vtable slots (their slot numbers are not load-bearing;
// only their bodies are matched), so they are placed last.
// ---------------------------------------------------------------------------
class CDDrawWorkerList {
public:
    int   VirtualMethodUnknown14();
    void  VirtualMethodUnknown1C();
    int   VirtualMethodUnknown20();
    void *VirtualMethodUnknown24(int a1, int a2, int a3);
    void *VirtualMethodUnknown28(int a1, int a2, int a3, int addHead);
    void *VirtualMethodUnknown2C(int a1, int a2, int a3, int a4, int addHead);
    void *VirtualMethodUnknown30(int a1, int a2, int a3, int a4, int addHead);
    void  VirtualMethodUnknown34(int a1, int a2);

    void  *m_vptr;                  // +0x00 (vptr; not stamped by these methods)
    int    m_04;                    // +0x04  initialized to -1 when inactive
    char   m_pad08[0x0c - 0x08];    // +0x08..0x0b
    int    m_pHarryPotter;          // +0x0c  (CDDrawSubMgr+0xc)
    CObList m_10;                   // +0x10  worker list (CObList)

    // Engine-label backlog stubs.
    void Stub_156f50();
    void Stub_156fc0();
};

// Stamps the worker's foreign vftable into its first dword (manual vptr store).
static inline void StampWorkerVtblB(HagridWorkerB *w) { *(void **)w = &g_hagridWorkerVtblB; }
static inline void StampWorkerVtblA(HagridWorkerA *w) { *(void **)w = &g_hagridWorkerVtblA; }

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several Lucius-derived managers.
RVA(0x156f00, 0x16)
int CDDrawWorkerList::VirtualMethodUnknown14()
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
static inline HagridWorkerB *MakeWorkerB(const CDDrawWorkerList *parent)
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

static inline HagridWorkerA *MakeWorkerA(const CDDrawWorkerList *parent)
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
// Allocates a BYTE-flag worker, constructs it, calls its +0x2c
// virtual with (a1,a2,a3). On success appends it to the list (AddTail) and returns
// it; on failure destroys it and returns 0.
RVA(0x156fd0, 0x8b)
void *CDDrawWorkerList::VirtualMethodUnknown24(int a1, int a2, int a3)
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
// As Unknown24 but the int-flag worker; on success the trailing
// bool selects AddHead vs AddTail.
RVA(0x1573e0, 0xa0)
void *CDDrawWorkerList::VirtualMethodUnknown28(int a1, int a2, int a3, int addHead)
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
// Int-flag worker; calls the worker's +0x30 virtual with (a1,a2,a3,a4); trailing
// bool selects AddHead vs AddTail.
RVA(0x157330, 0xa5)
void *CDDrawWorkerList::VirtualMethodUnknown2C(int a1, int a2, int a3, int a4, int addHead)
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
// As Unknown2C but dispatches the worker's +0x34 virtual.
RVA(0x157150, 0xa5)
void *CDDrawWorkerList::VirtualMethodUnknown30(int a1, int a2, int a3, int a4, int addHead)
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
// ---------------------------------------------------------------------------
// Walks the work-node list at m_pHarryPotter+0x04 (= this+0x14 in the MFC
// CObList's internal head pointer, offset +0x04 from the CObList base due to
// the inherited vptr).  Destroys each node's child if present, then clears
// the list.
RVA(0x163c60, 0x2c)
void CDDrawWorkerList::VirtualMethodUnknown1C()
{
    struct HLayout { char _pad[0x14]; WorkNode *m_head; };
    WorkNode *pNode = ((HLayout *)this)->m_head;
    while (pNode) {
        WorkNode *pCurrent = pNode;
        pNode = pNode->m_next;
        if (pCurrent->m_child)
            pCurrent->m_child->ScalarDtor(1);
    }
    m_10.RemoveAll();
}

// ---------------------------------------------------------------------------
// Returns constant 0x11 (17).
RVA(0x156f20, 0x6)
int CDDrawWorkerList::VirtualMethodUnknown20()
{
    return 0x11;
}

// ---------------------------------------------------------------------------
// Walks the work-node list at this+0x14. For each node: calls the child's
// +0x28 virtual (passing a1, a2), decrements child->m_74, and conditionally
// removes the node from the CObList + destroys the child.
//
// The remove-or-skip decision:
//    (a2 != 0 && (a2->field08 & 0x20000) == 0)  → always free
//    OR child->m_74 <= 0                          → free
//    Otherwise                                     → skip, keep node
RVA(0x163bf0, 0x6d)
void CDDrawWorkerList::VirtualMethodUnknown34(int a1, int a2)
{
    struct HLayout { char _pad[0x14]; WorkNode *m_head; };
    WorkNode *pNode = ((HLayout *)this)->m_head;
    while (pNode) {
        WorkNode *pCurrent = pNode;
        HagridChild *child = pCurrent->m_child;
        pNode = pNode->m_next;
        child->Vfunc28(a1, a2);
        child->m_74--;
        if ((*(int *)((char *)a2 + 0x2c) != 0
             && (*(int *)((char *)a2 + 0x08) & 0x20000) == 0)
            || child->m_74 <= 0)
        {
            m_10.RemoveAt((__POSITION *)pCurrent);
            if (child)
                child->ScalarDtor(1);
        }
    }
}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x156f50, 0x68)
void CDDrawWorkerList::Stub_156f50() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x156fc0, 0x6)
void CDDrawWorkerList::Stub_156fc0() {}
