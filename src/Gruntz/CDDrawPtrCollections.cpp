#include <Mfc.h> // real MFC CPtrList / CPtrArray / CByteArray (NAFXCW, reloc-masked)
#include <Ints.h>
#include <rva.h>

#include <Io/FileStream.h>               // engine CFileIO (palette loaders)
#include <string.h>                      // memset (inlined to rep stos at /O2 /Oi)
#include <Gruntz/CDDrawPtrCollections.h> // single-source CDDrawPtrCollections class shape
#include <Globals.h>
// CDDrawPtrCollections.cpp - tomalla-named standalone class in the ddrawmgr surface/page
// manager "DDraw surface manager" family (tomalla's CDDrawPtrCollections, 0x948 B, NO RTTI vtable).
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
class CDirectDrawMgr {
public:
    // Static HRESULT->log helper the surface/palette methods report failures through
    // (reloc-masked rel32; same archetype as DirectInputMgr2::GetErrorString).
    static void GetErrorString(char* file, i32 line, i32 hr); // 0x141400
};

// The DDrawMgr source-path $SG the surface/palette methods pass to GetErrorString.
#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"

// The RGB low-bit-position / 8-minus-bitcount pair tables ComputeColorMasks fills from
// the back-buffer's pixel format (reloc-masked .data globals; named g_683* across the
// run - GruntzMgr.cpp's 16-bit pack reads the same six words).
extern "C" {
    DATA(0x00283ea0)
    extern i32 g_683ea0; // red   low-bit shift
    DATA(0x00283ea4)
    extern i32 g_683ea4; // green low-bit shift
    DATA(0x00283ea8)
    extern i32 g_683ea8; // blue  low-bit shift
    DATA(0x00283eac)
    extern i32 g_683eac; // red   8-minus-count
    DATA(0x00283eb0)
    extern i32 g_683eb0; // green 8-minus-count
    DATA(0x00283eb4)
    extern i32 g_683eb4; // blue  8-minus-count
}

// The post-mask surface-format apply (CFileImage/CDDSurface area, 0x13f740); takes no
// args here (ecx is dead at the call), so a free function emits the bare `call rel32`.
// Named to pair with the engine_boundary stub symbol so its rel32 reloc matches retail.
void Boundary_13f740();

// The engine KERNEL32 file wrapper CFileIO (include/Io/FileStream.h) the palette
// loaders open; its virtual dtor gives the /GX local frame.

// A CPtrList internal node: { Node* pNext; Node* pPrev; void* data; }.  The pool
// walk in Clear/Empty derefs node->pNext (+0) and node->data (+8) directly.
struct CPtrListNode {
    CPtrListNode* pNext;
    CPtrListNode* pPrev;
    void* data;
};

// The real MFC CByteArray sub-object embedded at item+0x94 (ctor 0x1b4f0b /
// dtor 0x1b4f3e, reloc-masked; constructed in place by the factories).
// (CDDrawPtrCollections is defined in <Gruntz/CDDrawPtrCollections.h>, above.)

// The pool-A items' operator delete (invoked by the scalar-deleting dtors); the
// engine free, reloc-masked rel32.
void operator delete(void*);

// ---------------------------------------------------------------------------
// The +0x47c-pool surface-item family.  GENUINE C++ VTABLES (verified against the
// retail .rdata, not a pointer-to-member table): a 9-slot polymorphic BASE (vtable
// 0x5ef7f0) plus four DERIVED subclasses (0x5efa58 / a88 / ab8 / ae8) that override
// {virtual dtor slot 0, the slot-2 init, the slot-6 surface op} and add per-subclass
// init tail slots.  Modeled real-polymorphic so cl emits each ??_7 and auto-stamps
// the implicit vptr in the ctor/dtor; every slot fn is defined in a sibling TU
// (Image.cpp / boundary) so the emitted vtable slots are reloc-masked DIR32 relocs.
// (The earlier struct-of-pointer-to-member-fns model was a transitional workaround.)
//
// The init entry points the factories dispatch are ordinary virtual slots:
//   slot 2  (byte +0x08)  the 1-arg init  - base virtual (overridden by ab8 / ae8)
//   slot 9  (byte +0x24)  the primary init - per-subclass tail virtual
//   slot 10 (byte +0x28)  a58's 3-arg init - a58 tail virtual
//   slot 11 (byte +0x2c)  a58's 5-arg init - a58 tail virtual
// A pool item is 0xc0 bytes and owns a CByteArray @+0x94 (its throwing ctor is what
// gives each factory `new` its /GX frame).
// ---------------------------------------------------------------------------
class CPoolItemBase {
public:
    CPoolItemBase();

    virtual ~CPoolItemBase();                          // slot 0  0x141350 (??_G 0x141330)
    virtual i32 Refresh();                             // slot 1  0x13e140
    virtual i32 Init1(CDDrawPtrCollections* h, i32 a); // slot 2  0x13e0a0
    virtual i32 BlitSurf(void*, i32, i32, i32, i32);   // slot 3  0x13e0d0
    virtual void FreeSurfaces();                       // slot 4  0x13e4d0
    virtual i32 v14();                                 // slot 5  0x1412d0
    virtual i32 v18();                                 // slot 6  0x141300
    virtual i32 v1c();                                 // slot 7  0x13f960
    virtual i32 v20();                                 // slot 8  0x13e2e0

    // implicit vptr        // +0x00
    void* m_pos; // +0x04 - cached CPtrList POSITION
    i32 m_08;    // +0x08
    i32 m_0c;    // +0x0c
    char _10[0x7c - 0x10];
    i32 m_7c; // +0x7c
    char _80[0x94 - 0x80];
    CByteArray m_94; // +0x94
    i32 m_a8;        // +0xa8
    char _ac[0xb8 - 0xac];
    i32 m_b8; // +0xb8
    char _bc[0xc0 - 0xbc];
};
SIZE(CPoolItemBase, 0xc0);
// The base vtable 0x5ef7f0 is bound in CDirectDrawMgr.cpp (g_poolItemVtbl / CDdPoolVtbl);
// CPoolItemBase's emitted ??_7 is left unbound here so it does not collide with that.

// Zero the scalar fields (the CByteArray member + vptr are the compiler's job).
inline CPoolItemBase::CPoolItemBase() {
    m_08 = 0;
    m_0c = 0;
    m_pos = 0;
    m_7c = 0;
    m_a8 = 0;
    m_b8 = 0;
}

// vtable 0x5efa58: overrides the dtor (??_G 0x142340 / ~ 0x142820) and slot 6
// (0x143cc0); adds three init tail slots (9 = 0x148890, 10 = 0x148940, 11 = 0x148840).
class CPoolItemA : public CPoolItemBase {
public:
    virtual ~CPoolItemA();                                           // slot 0  ~ 0x142820
    virtual i32 v18() OVERRIDE;                                      // slot 6  0x143cc0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 9  0x148890
    virtual i32 v28(CDDrawPtrCollections*, i32, i32, i32);           // slot 10 0x148940
    virtual i32 v2c(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 11 0x148840
};
SIZE(CPoolItemA, 0xc0);
VTBL(CPoolItemA, 0x001efa58);

// vtable 0x5efa88: overrides the dtor (??_G 0x142800) and slot 6 (0x143cb0); adds two
// init tail slots (9 = 0x148a50, 10 = 0x148ac0).
class CPoolItemA88 : public CPoolItemBase {
public:
    virtual ~CPoolItemA88();                                         // slot 0  ??_G 0x142800
    virtual i32 v18() OVERRIDE;                                      // slot 6  0x143cb0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 9  0x148a50
    virtual i32 v28(CDDrawPtrCollections*, i32, i32, i32);           // slot 10 0x148ac0
};
SIZE(CPoolItemA88, 0xc0);
VTBL(CPoolItemA88, 0x001efa88);

// vtable 0x5efab8: overrides the dtor (??_G 0x142a20), slot 2 (0x148b50) and slot 6
// (0x143cd0); adds two init tail slots (9 = 0x148af0, 10 = 0x148b80).
class CPoolItemAB8 : public CPoolItemBase {
public:
    virtual ~CPoolItemAB8();                                         // slot 0  ??_G 0x142a20
    virtual i32 Init1(CDDrawPtrCollections*, i32) OVERRIDE;          // slot 2  0x148b50
    virtual i32 v18() OVERRIDE;                                      // slot 6  0x143cd0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 9  0x148af0
    virtual i32 v28(CDDrawPtrCollections*, i32, i32, i32);           // slot 10 0x148b80
};
SIZE(CPoolItemAB8, 0xc0);
VTBL(CPoolItemAB8, 0x001efab8);

// vtable 0x5efae8: overrides the dtor (??_G 0x142d20 / ~ 0x142d40), slot 2 (0x148cc0)
// and slot 6 (0x143ce0); adds one init tail slot (9 = 0x148c40, 6-arg).
class CPoolItemAE8 : public CPoolItemBase {
public:
    virtual ~CPoolItemAE8();                                              // slot 0  ~ 0x142d40
    virtual i32 Init1(CDDrawPtrCollections*, i32) OVERRIDE;               // slot 2  0x148cc0
    virtual i32 v18() OVERRIDE;                                           // slot 6  0x143ce0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32, i32); // slot 9  0x148c40
};
SIZE(CPoolItemAE8, 0xc0);
VTBL(CPoolItemAE8, 0x001efae8);

// The +0x498-pool item: a 0x38-byte struct (RezAlloc'd, vtable-less).  Field 0
// caches the POSITION; the +0x4..+0x10 + +0x14/+0x18/+0x2c/+0x30/+0x34 dwords are
// zeroed at create.  Torn down by the engine helper (Teardown, __thiscall 0x147530)
// then RezFree'd.
struct CPoolItemB {
    inline CPoolItemB();
    inline void* operator new(u32);

    void Teardown();                    // 0x147530 (__thiscall) - frees owned bufs
    i32 Init(void* arg, i32 a, i32 b);  // 0x1474d0 (__thiscall) - returns success
    i32 Init2(void* arg, i32 a, i32 b); // 0x147410 (__thiscall) - alt init, returns success
    i32
    Init3(void* arg, i32 a, i32 b, i32 c); // 0x147840 (__thiscall) - 3-param init, returns success

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
SIZE(CPoolItemB, 0x38); // measured: new(0x38) -> RezAlloc'd 0x38-byte item

inline CPoolItemB::CPoolItemB() {
    m_04 = 0;
    m_pos = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_34 = 0;
    m_18 = 0;
    m_14 = 0;
    m_2c = 0;
    m_30 = 0;
}

inline void* CPoolItemB::operator new(u32) {
    return ::operator new(0x38);
}

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
    char _0c[0x30 - 0x0c];
    i32(__stdcall* GetSurfaceDesc)(CCachedSurface*, void* desc); // +0x30
    char _34[0x4c - 0x34];
    void(__stdcall* Restore)(CCachedSurface*); // +0x4c
    char _50[0x54 - 0x50];
    // +0x54  reconfigure the surface (5 forwarded args); returns an HRESULT.
    i32(__stdcall* Configure)(CCachedSurface*, i32, i32, i32, i32, i32);
};

// The CDDrawPtrCollections host class (0x948 B) is the single-source shape from
// <Gruntz/CDDrawPtrCollections.h> (included at the top); the method bodies below
// implement it and the CPoolItem* / CCachedSurface helper family it dispatches.

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
// Destructor (0x141d50).  Clear(1), then tear down the two CPtrLists + CPtrArray
// (reverse construction order).  /GX EH frame.
// ---------------------------------------------------------------------------
RVA(0x00141d50, 0x6f)
CDDrawPtrCollections::~CDDrawPtrCollections() {
    Clear(1);
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
    for (i32 i = 0; i < m_array.GetSize(); i++) {
        RezFree(m_array.GetData()[i]);
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
void CDDrawPtrCollections::AddItemA(CPoolItemBase* item) {
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
        CPoolItemBase* item = (CPoolItemBase*)cur->data;
        delete item;
    }
    m_poolA.RemoveAll();
}

// ---------------------------------------------------------------------------
// RemoveItemA (0x142160).  pool.RemoveAt(item->pos); virtual-delete item.
// ---------------------------------------------------------------------------
RVA(0x00142160, 0x24)
void CDDrawPtrCollections::RemoveItemA(CPoolItemBase* item) {
    m_poolA.RemoveAt((POSITION)item->m_pos);
    delete item;
}

// ---------------------------------------------------------------------------
// Create7f0_1 (0x1421a0).  new 0xc0 item; ctor (CByteArray @+0x94, vtbl 0x5ef7f0
// stamped FIRST, then zero fields); dispatch vtbl[0x08] with 1 arg; on success
// AddItemA, else virtual-delete. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall: real-polymorphic `new CPoolItemBase` now emits the /GX ctor-in-flight
// frame (the throwing CByteArray member ctor), but the global __ehfuncinfo state-index
// push differs from retail (not reproducible from one TU); body byte-exact. Deferred.
RVA(0x001421a0, 0xbe)
CPoolItemBase* CDDrawPtrCollections::Create7f0_1(i32 a) {
    CPoolItemBase* item = new CPoolItemBase;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// CreateA (0x142260).  new 0xc0 item, ctor (CPoolItemA shell @+0x94 / vtbl 0x5efa58),
// dispatch vtbl[0x24]; on success register via AddItemA, else virtual-delete. /GX.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall: real-polymorphic `new CPoolItemA` emits the /GX frame; residue is the
// global __ehfuncinfo state-index push (per-TU) + the redundant base-then-derived vptr
// stamp order. Body byte-faithful. Deferred to the final sweep.
RVA(0x00142260, 0xd2)
CPoolItemBase* CDDrawPtrCollections::CreateA(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CPoolItemA* item = new CPoolItemA;
    if (item->v24(this, a, b, c, d, e)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// CreateB (0x1423c0).  Same as CreateA but dispatches vtbl[0x2c]. /GX.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (same as CreateA, init slot 11). Body byte-faithful, /GX state-index
// residue. Deferred to the final sweep.
RVA(0x001423c0, 0xd2)
CPoolItemBase* CDDrawPtrCollections::CreateB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CPoolItemA* item = new CPoolItemA;
    if (item->v2c(this, a, b, c, d, e)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createa58_1 (0x1424a0).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x001424a0, 0xbe)
CPoolItemBase* CDDrawPtrCollections::Createa58_1(i32 a) {
    CPoolItemA* item = new CPoolItemA;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createa58_3 (0x142560).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x28]
// with 3 args; AddItemA on success. /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142560, 0xc8)
CPoolItemBase* CDDrawPtrCollections::Createa58_3(i32 a, i32 b, i32 c) {
    CPoolItemA* item = new CPoolItemA;
    if (item->v28(this, a, b, c)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createa88_3 (0x142730).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x24]
// with 3 args; AddItemA on success. /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142730, 0xc8)
CPoolItemBase* CDDrawPtrCollections::Createa88_3(i32 a, i32 b, i32 c) {
    CPoolItemA88* item = new CPoolItemA88;
    if (item->v24(this, a, b, c, 0, 0)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// The shared base teardown the derived dtors inline: re-stamp the base vptr
// (0x5ef7f0), run FreeSurfaces(), then destroy the owned CByteArray member (auto).
// /GX (trylevel 0 -> -1 around the member dtor).  cl folds the redundant derived
// vptr stamp (dead store), leaving the base 0x5ef7f0 stamp - matching retail's dtors.
// (Base ~CPoolItemBase itself is CFileImage::~CFileImage @0x141350 in a sibling TU;
// left unbound here so it does not collide.)
// ---------------------------------------------------------------------------
CPoolItemBase::~CPoolItemBase() {
    FreeSurfaces();
}

// ---------------------------------------------------------------------------
// ~CPoolItemA (0x142820).  Derived a58 non-deleting dtor - trivial body; inlines the
// base teardown above.  __thiscall, ret 0x0.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (docs/patterns/eh-state-numbering-base.md + eh-ctor-vptr-restamp-
// position.md): body byte-identical, residue is the unwind-funcinfo push value (global
// __ehfuncinfo state index, not reproducible from one TU) + the vptr-restamp schedule.
// Deferred to the final sweep.
RVA(0x00142820, 0x53)
CPoolItemA::~CPoolItemA() {}

// ---------------------------------------------------------------------------
// Createa88_1 (0x142880).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142880, 0xbe)
CPoolItemBase* CDDrawPtrCollections::Createa88_1(i32 a) {
    CPoolItemA88* item = new CPoolItemA88;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_3 (0x142940).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x24]
// with 3 args; AddItemA + cache item->m_a8 into host->fieldUnknown538 on success.
// /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142940, 0xd4)
CPoolItemBase* CDDrawPtrCollections::Createab8_3(i32 a, i32 b, i32 c) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->v24(this, a, b, c, 0, 0)) {
        AddItemA(item);
        fieldUnknown538 = item->m_a8;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_1 (0x142aa0).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x08]
// with 1 arg; AddItemA + cache item->m_a8 into host->fieldUnknown538 on success.
// /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142aa0, 0xca)
CPoolItemBase* CDDrawPtrCollections::Createab8_1(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->Init1(this, a)) {
        AddItemA(item);
        fieldUnknown538 = item->m_a8;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_24_3 (0x142b70).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch
// vtbl[0x24] as a 3-arg init with the two literal tags (0x18, 0x21) + the incoming
// arg; AddItemA + cache item->m_a8 into host->fieldUnknown538 on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall + arity: this call site invokes ab8 slot 9 with only 3 game args, but
// slot 9 (0x148af0) is the same virtual Createab8_3 calls with 5, so a single C++
// signature can serve only one - the 3-arg site over-pushes two zeros here (accepted;
// the retail author called the same slot with two arities). /GX state-index residue.
RVA(0x00142b70, 0xce)
CPoolItemBase* CDDrawPtrCollections::Createab8_24_3(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->v24(this, 0x18, 0x21, a, 0, 0)) {
        AddItemA(item);
        fieldUnknown538 = item->m_a8;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createae8_6 (0x142c40).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x24]
// as a 6-arg init with all six incoming args; AddItemA on success. /GX. ret 0x18.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142c40, 0xd7)
CPoolItemBase* CDDrawPtrCollections::Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (item->v24(this, a, b, c, d, e, f)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// ~CPoolItemAE8 (0x142d40).  Derived ae8 non-deleting dtor - trivial body; inlines the
// shared base teardown (stamp 0x5ef7f0 + FreeSurfaces + member dtor).  /GX, ret 0x0.
// Byte-identical codegen to ~CPoolItemA (0x142820); a distinct subclass.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (same as ~CPoolItemA @0x142820): body byte-identical, residue is the
// unwind-funcinfo push value (global __ehfuncinfo state index) + the vptr-restamp
// scheduling. Deferred to the final sweep.
RVA(0x00142d40, 0x53)
CPoolItemAE8::~CPoolItemAE8() {}

// ---------------------------------------------------------------------------
// Createae8_1 (0x142da0).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142da0, 0xbe)
CPoolItemBase* CDDrawPtrCollections::Createae8_1(i32 a) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// MakeAndAddB (0x142e60).  Tail-thunk into CreateB with arg2 |= 0x840.
// ---------------------------------------------------------------------------
RVA(0x00142e60, 0x27)
CPoolItemBase* CDDrawPtrCollections::MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e) {
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
    m_poolB.RemoveAt((POSITION)item->m_pos);
    if (item) {
        item->Teardown();
        RezFree(item);
    }
}

// ---------------------------------------------------------------------------
// MakeB2 (0x142f40).  Sibling of MakeB: RezAlloc a 0x38-byte CPoolItemB, zero its
// fields, init it via the alternate Init2 (0x147410) with (m_surf0, a, b); on success
// add to pool B and return it, else tear down + RezFree and return 0.  NO EH frame
// (no destructible local), so this matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x00142f40, 0x7c)
CPoolItemB* CDDrawPtrCollections::MakeB2(i32 a, i32 b) {
    CPoolItemB* item = new CPoolItemB;
    if (!item->Init2(m_surf0, a, b)) {
        if (item) {
            item->Teardown();
            RezFree(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

// ---------------------------------------------------------------------------
// MakeB (0x142fc0).  RezAlloc a 0x38-byte CPoolItemB, zero its fields, init it via
// the external Item498_Init (0x1474d0) with (vtbl-of-this, a, b); on success add to
// pool B and return it, else tear down + RezFree and return 0.
// ---------------------------------------------------------------------------
RVA(0x00142fc0, 0x7c)
CPoolItemB* CDDrawPtrCollections::MakeB(i32 a, i32 b) {
    CPoolItemB* item = new CPoolItemB;
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

// ---------------------------------------------------------------------------
// MakeB3 (0x1430c0).  Third sibling of MakeB: RezAlloc a 0x38-byte CPoolItemB,
// zero its fields, init it via the 3-param Init3 (0x147840) with (m_surf0, a, b,
// c); on success add to pool B and return it, else tear down + RezFree and return
// 0.  No EH frame (no destructible local) -> matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x001430c0, 0x81)
CPoolItemB* CDDrawPtrCollections::MakeB3(i32 a, i32 b, i32 c) {
    CPoolItemB* item = new CPoolItemB;
    if (!item->Init3(m_surf0, a, b, c)) {
        if (item) {
            item->Teardown();
            RezFree(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

// ---------------------------------------------------------------------------
// LoadPaletteMakeB (0x143150).  Open `path` via CFileIO, seek 0x300 from the end and
// read the trailing 0x300-byte palette into a stack buffer, then register a pool-B item
// built from it (MakeB(buf, 0)).  Any failure unwinds the CFileIO + returns 0.  The
// second arg slot is reused as the (always-0) MakeB tag.  /GX EH frame.  ret 0x8.
// ---------------------------------------------------------------------------
// @early-stop
// ~98%: logic + offsets + CFG + the CFileIO open/seek/read shapes are byte-faithful.
// Residue is (a) the /GX funcinfo state index push (retail `push 0xb` vs the per-TU
// compiler-generated funcinfo @+0 - the global __ehfuncinfo numbering, not reproducible
// from one TU; docs/patterns/eh-state-numbering-base.md) and (b) MSVC folds the
// reloaded-from-param-slot MakeB tag to an immediate 0 where retail reloads it. Deferred.
RVA(0x00143150, 0xe9)
CPoolItemB* CDDrawPtrCollections::LoadPaletteMakeB(const char* path, i32 z) {
    CFileIO file;
    z = 0;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return MakeB((i32)buf, z);
}

// ---------------------------------------------------------------------------
// LoadPaletteMake950 (0x143a30).  Identical shape to LoadPaletteMakeB but the trailing
// palette is handed to the sibling builder Make950 (0x143950) instead of MakeB.  /GX. ret 0x8.
// ---------------------------------------------------------------------------
// @early-stop
// ~98%: same wall as LoadPaletteMakeB (EH funcinfo state index + MakeB-tag const-fold);
// additionally the Make950 callee (0x143950) is still an unreconstructed engine_boundary
// stub, so its rel32 reloc pairs by code bytes but not by symbol name. Deferred.
RVA(0x00143a30, 0xe9)
CPoolItemB* CDDrawPtrCollections::LoadPaletteMake950(const char* path, i32 z) {
    CFileIO file;
    z = 0;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return (CPoolItemB*)Make950(buf, z);
}

// The back-buffer surface description ComputeColorMasks fills (a DDSURFACEDESC, 0x6c
// bytes); only dwSize@+0 and the pixel-format R/G/B masks @+0x58/+0x5c/+0x60 are touched.
struct SurfDesc {
    u32 dwSize; // +0x00
    char _04[0x58 - 0x04];
    u32 rMask; // +0x58
    u32 gMask; // +0x5c
    u32 bMask; // +0x60
    char _64[0x6c - 0x64];
};

// ---------------------------------------------------------------------------
// ComputeColorMasks (0x143b20).  Query the cached surface's pixel format (vtbl +0x30),
// and for each of the R/G/B bit masks record the low set-bit position (the shift) and
// 8-minus-popcount (the scale) into the six g_683* globals, then apply (Func13f740).
// On a failed GetSurfaceDesc, report via GetErrorString and return 0.  No EH frame.
// ---------------------------------------------------------------------------
RVA(0x00143b20, 0xfc)
i32 CDDrawPtrCollections::ComputeColorMasks() {
    SurfDesc desc;
    memset(&desc, 0, 0x6c);
    desc.dwSize = 0x6c;
    i32 hr = m_surf0->vtbl->GetSurfaceDesc(m_surf0, &desc);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x82c, hr);
        return 0;
    }

    u32 m = desc.rMask;
    i32 count = 0;
    i32 shift = -1;
    for (i32 b = 0; b < 0x20; b++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b;
            }
            count++;
        }
        m >>= 1;
    }
    g_683ea0 = shift;
    g_683eac = 8 - count;

    m = desc.gMask;
    count = 0;
    shift = -1;
    for (i32 b2 = 0; b2 < 0x20; b2++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b2;
            }
            count++;
        }
        m >>= 1;
    }
    g_683ea4 = shift;
    g_683eb0 = 8 - count;

    m = desc.bMask;
    count = 0;
    shift = -1;
    for (i32 b3 = 0; b3 < 0x20; b3++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b3;
            }
            count++;
        }
        m >>= 1;
    }
    g_683ea8 = shift;
    g_683eb4 = 8 - count;

    Boundary_13f740();
    return 1;
}

// ---------------------------------------------------------------------------
// ConfigureSurface (0x143c20).  Reconfigure the cached surface through its vtbl +0x54
// slot (the five args forwarded verbatim).  On a non-zero HRESULT, report through
// GetErrorString, latch fieldUnknown944 = 0x3ec if unset, and return the HRESULT.
// Otherwise recompute the color masks; if that fails, latch fieldUnknown944 = 0x3ed
// if unset and return E_FAIL (0x80004005).  __thiscall, ret 0x14 (5 stack args). No EH.
// ---------------------------------------------------------------------------
RVA(0x00143c20, 0x84)
i32 CDDrawPtrCollections::ConfigureSurface(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    i32 hr = m_surf0->vtbl->Configure(m_surf0, a0, a1, a2, a3, a4);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x8a2, hr);
        if (fieldUnknown944 == 0) {
            fieldUnknown944 = 0x3ec;
        }
        return hr;
    }
    if (ComputeColorMasks() == 0) {
        hr = (i32)0x80004005;
        if (fieldUnknown944 == 0) {
            fieldUnknown944 = 0x3ed;
        }
    }
    return hr;
}

SIZE_UNKNOWN(CCachedSurface);
SIZE_UNKNOWN(CCachedSurfaceVtbl);
SIZE_UNKNOWN(CPtrListNode);
SIZE_UNKNOWN(SurfDesc);
