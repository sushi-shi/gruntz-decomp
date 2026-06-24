#include <Ints.h>
#include <rva.h>
// CDDrawPtrCollections.cpp - tomalla-named standalone class in the ddrawmgr surface/page
// manager "Harry Potter" family (tomalla's UnknownFilch, 0x948 B, NO RTTI vtable).
// It owns two CPtrList item pools (+0x47c / +0x498) plus a CPtrArray (+0x4b4 - its own
// m_pData@+0x4b8 / m_nSize@+0x4bc internals are walked by Clear), and caches two surface
// objects at +0x00 / +0x04.  The ctor constructs the three MFC containers with the given
// block sizes (both lists 0xa), clears the scalar fields, and carries a C++ EH frame
// (/GX) to unwind the constructed containers if a later one throws.
//
// The two CPtrList pools hold engine surface/sprite objects:
//   - the +0x47c pool stores a polymorphic item (deleted via its virtual deleting
//     dtor) that caches its CPtrList POSITION at item+0x4;
//   - the +0x498 pool stores a lighter struct (size 0x38, RezAlloc'd) deleted by an
//     explicit teardown (CDDPalette_147530) + RezFree, caching its POSITION at item+0.
//
// Field names are tomalla placeholders; only the OFFSETS + the emitted code bytes are
// load-bearing (campaign doctrine).  The FLIRT names (~CInternetSession/~CByteArray)
// on the container dtors are misattributions of the real CPtrList/CPtrArray dtors.
// ---------------------------------------------------------------------------

// The engine allocator (reloc-masked rel32).  operator new == _RezAlloc here.
void* operator new(u32);
inline void* operator new(u32, void* p) {
    return p;
} // placement new (construct in place)
extern "C" void RezFree(void* p);

// Process-wide DirectDraw manager singleton (.data) - cleared by Clear().
class CDirectDrawMgr;
DATA(0x002bed00)
extern "C" CDirectDrawMgr* g_DirectDrawMgr; // 0x6bed00

// ---------------------------------------------------------------------------
// Minimal MFC container placeholders - only their ctor symbol + size matter.
// CPtrList: vptr@0 + 6 scalar fields = 0x1c.  CPtrArray: vptr@0 + 4 fields = 0x14.
// ---------------------------------------------------------------------------
class CPtrList {
public:
    CPtrList(i32 nBlockSize);
    ~CPtrList(); // (invoked on EH unwind / dtor)
    void* AddTail(void* p);
    void RemoveAt(void* pos);
    void RemoveAll();
    void* GetHeadPosition() const;
    void* vptr;          // +0x00
    char _raw[0x1c - 4]; // 0x1c incl vptr
};

class CPtrArray {
public:
    CPtrArray();
    ~CPtrArray();
    void SetSize(i32 nNewSize, i32 nGrowBy);
    void* vptr;     // +0x00
    void** m_pData; // +0x04 - element storage
    i32 m_nSize;    // +0x08 - element count
    char _raw[0x14 - 0xc];
}; // 0x14

// A CPtrList internal node: { Node* pNext; Node* pPrev; void* data; }.  The pool
// walk in Clear/Empty derefs node->pNext (+0) and node->data (+8) directly.
struct CPtrListNode {
    CPtrListNode* pNext;
    CPtrListNode* pPrev;
    void* data;
};

// A 0x14-byte CByteArray-shaped sub-object embedded at item+0x94.  Only its ctor
// symbol + size matter (constructed in place by the factories).
class CByteArrayMember {
public:
    CByteArrayMember(); // 0x1b4f0b (reloc-masked rel32)
    char _raw[0x14];
};

class CDDrawPtrCollections;

// ---------------------------------------------------------------------------
// The polymorphic +0x47c-pool item (0xc0 B).  The factories construct it by hand
// (inline field init + manual vtable stamp to the RETAIL vtable 0x5efa58 - a
// reloc-masked DATA extern - because the item's virtuals live in other TUs), then
// dispatch a virtual init slot.  Slot 0 is the scalar deleting dtor; slots 0x24 /
// 0x2c are the two init entry points the two factories use.
// ---------------------------------------------------------------------------
// The item's vtable is modeled as a struct of __thiscall pointer-to-member fns
// (the MSVC5-period idiom; raw `__thiscall` fn-ptrs are rejected), so each dispatch
// lowers to `mov ecx,item; call [vtbl+slot]` with no manual cast.
struct CPoolItemAVtbl;
class CPoolItemA {
public:
    CPoolItemAVtbl* vptr; // +0x00 - stamped to g_poolItemAVtbl (0x5efa58)
    void* m_pos;          // +0x04 - cached CPtrList POSITION
    i32 m_08;             // +0x08
    i32 m_0c;             // +0x0c
    char _10[0x7c - 0x10];
    i32 m_7c; // +0x7c
    char _80[0x94 - 0x80];
    CByteArrayMember m_94; // +0x94
    i32 m_a8;              // +0xa8
    char _ac[0xb8 - 0xac];
    i32 m_b8; // +0xb8
    char _bc[0xc0 - 0xbc];

    void Delete(u32 flags);                                     // vtbl[0x00]
    i32 Init24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // vtbl[0x24]
    i32 Init2c(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // vtbl[0x2c]
}; // 0xc0

typedef void (CPoolItemA::*PoolItemDeleteFn)(u32);
typedef i32 (CPoolItemA::*PoolItemInitFn)(CDDrawPtrCollections*, i32, i32, i32, i32, i32);
struct CPoolItemAVtbl {
    PoolItemDeleteFn Delete; // [0x00] scalar deleting dtor
    char _04[0x24 - 0x04];
    PoolItemInitFn Init24; // [0x24]
    char _28[0x2c - 0x28];
    PoolItemInitFn Init2c; // [0x2c]
};
inline void CPoolItemA::Delete(u32 flags) {
    (this->*(vptr->Delete))(flags);
}
inline i32 CPoolItemA::Init24(CDDrawPtrCollections* h, i32 a, i32 b, i32 c, i32 d, i32 e) {
    return (this->*(vptr->Init24))(h, a, b, c, d, e);
}
inline i32 CPoolItemA::Init2c(CDDrawPtrCollections* h, i32 a, i32 b, i32 c, i32 d, i32 e) {
    return (this->*(vptr->Init2c))(h, a, b, c, d, e);
}

// The retail vtable for the pool-A item (reloc-masked DATA datum).
DATA(0x001efa58)
extern CPoolItemAVtbl g_poolItemAVtbl; // 0x5efa58

// The +0x498-pool item: a 0x38-byte struct (RezAlloc'd, vtable-less).  Field 0
// caches the POSITION; the +0x4..+0x10 + +0x14/+0x18/+0x2c/+0x30/+0x34 dwords are
// zeroed at create.  Torn down by the engine helper (Teardown, __thiscall 0x147530)
// then RezFree'd.
struct CPoolItemB {
    void Teardown();                   // 0x147530 (__thiscall) - frees owned bufs
    i32 Init(void* arg, i32 a, i32 b); // 0x1474d0 (__thiscall) - returns success

    void* m_pos; // +0x00 cached POSITION
    i32 m_04;    // +0x04
    i32 m_08;    // +0x08
    i32 m_0c;    // +0x0c
    i32 m_10;    // +0x10
    i32 m_14;    // +0x14
    i32 m_18;    // +0x18
    char _1c[0x2c - 0x1c];
    i32 m_2c; // +0x2c
    i32 m_30; // +0x30
    i32 m_34; // +0x34
};

// A cached surface object (the +0x00 / +0x04 slots): polymorphic, with a
// Release-like vtbl[8] and a RestoreAll-like vtbl[0x4c].  External, no body.
// The vtable methods are stdcall-shaped: `this` is pushed and the callee cleans up
// (no caller `add esp,4`), so model the slot pointers as __stdcall.
struct CCachedSurfaceVtbl;
struct CCachedSurface {
    CCachedSurfaceVtbl* vtbl;
};
struct CCachedSurfaceVtbl {
    void(__stdcall* Method00)(CCachedSurface*);
    char _04[0x08 - 0x04];
    void(__stdcall* Release)(CCachedSurface*); // +0x08
    char _0c[0x4c - 0x0c];
    void(__stdcall* Restore)(CCachedSurface*); // +0x4c
};

// ---------------------------------------------------------------------------
// CDDrawPtrCollections - the host (tomalla UnknownFilch).  Only the offsets the
// reconstructed methods touch are modeled; the rest is padding.
// ---------------------------------------------------------------------------
class CDDrawPtrCollections {
public:
    CDDrawPtrCollections();
    ~CDDrawPtrCollections();

    void Clear(i32 mode);                                       // 0x142060
    void EmptyPoolA();                                          // 0x142120  (drain +0x47c list)
    void EmptyPoolB();                                          // 0x142ed0  (drain +0x498 list)
    void AddItemA(CPoolItemA* item);                            // 0x142100
    void AddItemB(CPoolItemB* item);                            // 0x142eb0
    void RemoveItemA(CPoolItemA* item);                         // 0x142160
    void RemoveItemB(CPoolItemB* item);                         // 0x142f10
    CPoolItemA* CreateA(i32 a, i32 b, i32 c, i32 d, i32 e);     // 0x142260
    CPoolItemA* CreateB(i32 a, i32 b, i32 c, i32 d, i32 e);     // 0x1423c0
    CPoolItemA* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142e60
    CPoolItemB* MakeB(i32 a, i32 b);                            // 0x142fc0

    CCachedSurface* m_surf0; // +0x00 - cached surface object (Release on Clear)
    CCachedSurface* m_surf4; // +0x04 - cached surface object (Release on Clear)
    char _pad008[0x47c - 0x08];
    CPtrList m_poolA;  // +0x47c  (block size 0xa) - CPoolItemA*
    CPtrList m_poolB;  // +0x498  (block size 0xa) - CPoolItemB*
    CPtrArray m_array; // +0x4b4  (default ctor); m_pData@+0x4b8 / m_nSize@+0x4bc
    char _pad4C8[0x534 - 0x4c8];
    i32 fieldUnknown534; // +0x534  - zeroed in ctor / Clear
    i32 fieldUnknown538; // +0x538  - zeroed in ctor
    char _pad53C[0x93c - 0x53c];
    i32 fieldUnknown93C; // +0x93c  - zeroed in ctor
    i32 fieldUnknown940; // +0x940  - zeroed in ctor
    i32 fieldUnknown944; // +0x944  - zeroed in ctor
}; // 0x948

// ---------------------------------------------------------------------------
// Constructor (0x141cc0).  /GX EH frame to unwind the three containers.
// ---------------------------------------------------------------------------
RVA(0x00141cc0, 0x84)
CDDrawPtrCollections::CDDrawPtrCollections() : m_poolA(0xa), m_poolB(0xa), m_array() {
    m_surf0 = 0;
    m_surf4 = 0;
    fieldUnknown534 = 0;
    fieldUnknown538 = 0;
    fieldUnknown93C = 0;
    fieldUnknown940 = 0;
    fieldUnknown944 = 0;
}

// ---------------------------------------------------------------------------
// Destructor (0x141d50).  Clear(2), then tear down the two CPtrLists + CPtrArray
// (reverse construction order).  /GX EH frame.
// ---------------------------------------------------------------------------
RVA(0x00141d50, 0x6f)
CDDrawPtrCollections::~CDDrawPtrCollections() {
    Clear(2);
}

// ---------------------------------------------------------------------------
// Clear (0x142060).  mode!=0 -> Release the cached +0x00 surface; free every entry
// in the raw pointer array (+0x4b8/+0x4bc); RemoveAll the CPtrArray; drain both
// pools; null g_DirectDrawMgr; Release+null both cached surfaces; zero +0x534.
// ---------------------------------------------------------------------------
RVA(0x00142060, 0x9d)
void CDDrawPtrCollections::Clear(i32 mode) {
    if (mode && m_surf0) {
        m_surf0->vtbl->Restore(m_surf0);
    }
    for (i32 i = 0; i < m_array.m_nSize; i++) {
        RezFree(m_array.m_pData[i]);
    }
    m_array.SetSize(0, -1);
    EmptyPoolA();
    EmptyPoolB();
    g_DirectDrawMgr = 0;
    if (m_surf0) {
        m_surf0->vtbl->Release(m_surf0);
        m_surf0 = 0;
    }
    if (m_surf4) {
        m_surf4->vtbl->Release(m_surf4);
        m_surf4 = 0;
    }
    fieldUnknown534 = 0;
}

// ---------------------------------------------------------------------------
// AddItemA (0x142100).  pool.AddTail(item); item->pos = position.
// ---------------------------------------------------------------------------
RVA(0x00142100, 0x18)
void CDDrawPtrCollections::AddItemA(CPoolItemA* item) {
    item->m_pos = m_poolA.AddTail(item);
}

// ---------------------------------------------------------------------------
// EmptyPoolA (0x142120).  Walk the +0x47c list, virtual-delete each item, RemoveAll.
// ---------------------------------------------------------------------------
RVA(0x00142120, 0x31)
void CDDrawPtrCollections::EmptyPoolA() {
    CPtrListNode* node = *(CPtrListNode**)((char*)&m_poolA + 4);
    while (node) {
        CPtrListNode* cur = node;
        node = node->pNext;
        CPoolItemA* item = (CPoolItemA*)cur->data;
        if (item) {
            item->Delete(1);
        }
    }
    m_poolA.RemoveAll();
}

// ---------------------------------------------------------------------------
// RemoveItemA (0x142160).  pool.RemoveAt(item->pos); virtual-delete item.
// ---------------------------------------------------------------------------
RVA(0x00142160, 0x24)
void CDDrawPtrCollections::RemoveItemA(CPoolItemA* item) {
    m_poolA.RemoveAt(item->m_pos);
    if (item) {
        item->Delete(1);
    }
}

// ---------------------------------------------------------------------------
// CreateA (0x142260).  new 0xc0 item, ctor (CPoolItemA shell @+0x94 / vtbl 0x5efa58),
// dispatch vtbl[0x24]; on success register via AddItemA, else virtual-delete. /GX.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: retail wraps `new CPoolItemA` (op-new ==
// RezAlloc, EH-tracked throwing member ctor) in a /GX frame (push -1/push 0xb/fs:0 +
// trylevel stamps + shared fs:0-restoring epilogue); MSVC5 placement-new emits NO
// ctor-in-flight EH state, so the body is byte-exact but the whole frame is absent.
// 60.5% (above the documented ~47% plateau); deferred to the final sweep.
RVA(0x00142260, 0xd2)
CPoolItemA* CDDrawPtrCollections::CreateA(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArrayMember;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemAVtbl;
    } else {
        item = 0;
    }
    if (item->Init24(this, a, b, c, d, e)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CreateB (0x1423c0).  Same as CreateA but dispatches vtbl[0x2c]. /GX.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall (same as CreateA, init slot 0x2c). 60.5%;
// /GX frame absent, body byte-exact. Deferred to the final sweep.
RVA(0x001423c0, 0xd2)
CPoolItemA* CDDrawPtrCollections::CreateB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArrayMember;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemAVtbl;
    } else {
        item = 0;
    }
    if (item->Init2c(this, a, b, c, d, e)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// MakeAndAddB (0x142e60).  Tail-thunk into CreateB with arg2 |= 0x840.
// ---------------------------------------------------------------------------
RVA(0x00142e60, 0x27)
CPoolItemA* CDDrawPtrCollections::MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    return CreateB(a, b, c, d | 0x840, e);
}

// ---------------------------------------------------------------------------
// AddItemB (0x142eb0).  pool.AddTail(item); item->pos = position.
// ---------------------------------------------------------------------------
RVA(0x00142eb0, 0x17)
void CDDrawPtrCollections::AddItemB(CPoolItemB* item) {
    item->m_pos = m_poolB.AddTail(item);
}

// ---------------------------------------------------------------------------
// EmptyPoolB (0x142ed0).  Walk the +0x498 list, tear down + RezFree each item,
// RemoveAll.
// ---------------------------------------------------------------------------
RVA(0x00142ed0, 0x3d)
void CDDrawPtrCollections::EmptyPoolB() {
    CPtrListNode* node = *(CPtrListNode**)((char*)&m_poolB + 4);
    while (node) {
        CPtrListNode* cur = node;
        node = node->pNext;
        CPoolItemB* item = (CPoolItemB*)cur->data;
        if (item) {
            item->Teardown();
            RezFree(item);
        }
    }
    m_poolB.RemoveAll();
}

// ---------------------------------------------------------------------------
// RemoveItemB (0x142f10).  pool.RemoveAt(item->pos); tear down + RezFree item.
// ---------------------------------------------------------------------------
RVA(0x00142f10, 0x2b)
void CDDrawPtrCollections::RemoveItemB(CPoolItemB* item) {
    m_poolB.RemoveAt(item->m_pos);
    if (item) {
        item->Teardown();
        RezFree(item);
    }
}

// ---------------------------------------------------------------------------
// MakeB (0x142fc0).  RezAlloc a 0x38-byte CPoolItemB, zero its fields, init it via
// the external Item498_Init (0x1474d0) with (vtbl-of-this, a, b); on success add to
// pool B and return it, else tear down + RezFree and return 0.
// ---------------------------------------------------------------------------
RVA(0x00142fc0, 0x7c)
CPoolItemB* CDDrawPtrCollections::MakeB(i32 a, i32 b) {
    void* mem = operator new(0x38);
    CPoolItemB* item;
    if (mem) {
        ((CPoolItemB*)mem)->m_04 = 0;
        ((CPoolItemB*)mem)->m_pos = 0;
        ((CPoolItemB*)mem)->m_08 = 0;
        ((CPoolItemB*)mem)->m_0c = 0;
        ((CPoolItemB*)mem)->m_10 = 0;
        ((CPoolItemB*)mem)->m_34 = 0;
        ((CPoolItemB*)mem)->m_18 = 0;
        ((CPoolItemB*)mem)->m_14 = 0;
        ((CPoolItemB*)mem)->m_2c = 0;
        ((CPoolItemB*)mem)->m_30 = 0;
        item = (CPoolItemB*)mem;
    } else {
        item = 0;
    }
    if (!item->Init(m_surf0, a, b)) {
        if (item) {
            item->Teardown();
            RezFree(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}
