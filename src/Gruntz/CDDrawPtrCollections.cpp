#include <Mfc.h> // real MFC CPtrList / CPtrArray / CByteArray (NAFXCW, reloc-masked)
#include <Ints.h>
#include <rva.h>

#include <Io/FileStream.h> // engine CFileIO (palette loaders)
#include <string.h>        // memset (inlined to rep stos at /O2 /Oi)
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
class CDirectDrawMgr {
public:
    // Static HRESULT->log helper the surface/palette methods report failures through
    // (reloc-masked rel32; same archetype as DirectInputMgr2::GetErrorString).
    static void GetErrorString(char* file, i32 line, i32 hr); // 0x141400
};
extern "C" CDirectDrawMgr* g_DirectDrawMgr; // 0x6bed00

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
    ~CPoolItemA();      // non-deleting dtor variant 0x142820 (vtbl 0x5ef7f0)
    void FreeSurfaces(); // 0x13e4d0 (CFileImage::FreeSurfaces, __thiscall) - external

    CPoolItemAVtbl* vptr; // +0x00 - stamped to g_poolItemAVtbl (0x5efa58)
    void* m_pos;          // +0x04 - cached CPtrList POSITION
    i32 m_08;             // +0x08
    i32 m_0c;             // +0x0c
    char _10[0x7c - 0x10];
    i32 m_7c; // +0x7c
    char _80[0x94 - 0x80];
    CByteArray m_94; // +0x94
    i32 m_a8;              // +0xa8
    char _ac[0xb8 - 0xac];
    i32 m_b8; // +0xb8
    char _bc[0xc0 - 0xbc];

    void Delete(u32 flags);                                          // vtbl[0x00] scalar deleting dtor
    i32 Init08(CDDrawPtrCollections*, i32);                          // vtbl[0x08]  (1-arg init)
    i32 Init24(CDDrawPtrCollections*, i32, i32, i32, i32, i32);      // vtbl[0x24]  (5-arg)
    i32 Init24_3(CDDrawPtrCollections*, i32, i32, i32);              // vtbl[0x24]  (3-arg)
    i32 Init24_6(CDDrawPtrCollections*, i32, i32, i32, i32, i32, i32); // vtbl[0x24]  (6-arg)
    i32 Init28(CDDrawPtrCollections*, i32, i32, i32);                // vtbl[0x28]  (3-arg init)
    i32 Init2c(CDDrawPtrCollections*, i32, i32, i32, i32, i32);      // vtbl[0x2c]
}; // 0xc0

typedef void (CPoolItemA::*PoolItemDeleteFn)(u32);
typedef i32 (CPoolItemA::*PoolItemInit1Fn)(CDDrawPtrCollections*, i32);
typedef i32 (CPoolItemA::*PoolItemInit3Fn)(CDDrawPtrCollections*, i32, i32, i32);
typedef i32 (CPoolItemA::*PoolItemInitFn)(CDDrawPtrCollections*, i32, i32, i32, i32, i32);
typedef i32 (CPoolItemA::*PoolItemInit6Fn)(CDDrawPtrCollections*, i32, i32, i32, i32, i32, i32);
struct CPoolItemAVtbl {
    PoolItemDeleteFn Delete; // [0x00] scalar deleting dtor
    char _04[0x08 - 0x04];
    PoolItemInit1Fn Init08; // [0x08]
    char _0c[0x24 - 0x0c];
    // slot [0x24]: the init arity varies per subclass (3 / 5 / 6 args), so the
    // same 4-byte pointer-to-member slot is viewed through the right type.
    union {
        PoolItemInitFn Init24;    // 5-arg (CreateA/CreateB style)
        PoolItemInit3Fn Init24_3; // 3-arg (vtbl ab8/a88 style)
        PoolItemInit6Fn Init24_6; // 6-arg (vtbl ae8 style)
    };
    PoolItemInit3Fn Init28; // [0x28]
    PoolItemInitFn Init2c; // [0x2c]
};
inline void CPoolItemA::Delete(u32 flags) {
    (this->*(vptr->Delete))(flags);
}
inline i32 CPoolItemA::Init08(CDDrawPtrCollections* h, i32 a) {
    return (this->*(vptr->Init08))(h, a);
}
inline i32 CPoolItemA::Init24(CDDrawPtrCollections* h, i32 a, i32 b, i32 c, i32 d, i32 e) {
    return (this->*(vptr->Init24))(h, a, b, c, d, e);
}
inline i32 CPoolItemA::Init24_3(CDDrawPtrCollections* h, i32 a, i32 b, i32 c) {
    return (this->*(vptr->Init24_3))(h, a, b, c);
}
inline i32 CPoolItemA::Init24_6(CDDrawPtrCollections* h, i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    return (this->*(vptr->Init24_6))(h, a, b, c, d, e, f);
}
inline i32 CPoolItemA::Init28(CDDrawPtrCollections* h, i32 a, i32 b, i32 c) {
    return (this->*(vptr->Init28))(h, a, b, c);
}
inline i32 CPoolItemA::Init2c(CDDrawPtrCollections* h, i32 a, i32 b, i32 c, i32 d, i32 e) {
    return (this->*(vptr->Init2c))(h, a, b, c, d, e);
}

// The retail vtables for the four pool-A item subclasses (reloc-masked DATA data).
DATA(0x001ef7f0)
extern CPoolItemAVtbl g_poolItemVtbl7f0; // 0x5ef7f0
DATA(0x001efa58)
extern CPoolItemAVtbl g_poolItemAVtbl; // 0x5efa58
DATA(0x001efa88)
extern CPoolItemAVtbl g_poolItemVtbla88; // 0x5efa88
DATA(0x001efab8)
extern CPoolItemAVtbl g_poolItemVtblab8; // 0x5efab8
DATA(0x001efae8)
extern CPoolItemAVtbl g_poolItemVtblae8; // 0x5efae8

// A SECOND pool-A item subclass that shares the vtbl-7f0 non-deleting dtor shape
// (vptr restamp -> FreeSurfaces -> CByteArray member dtor).  Distinct retail RVA
// (0x142d40 vs CPoolItemA::~CPoolItemA @0x142820) but byte-identical codegen.
class CPoolItemA7f0 {
public:
    ~CPoolItemA7f0();    // 0x142d40
    void FreeSurfaces(); // 0x13e4d0 (__thiscall) - external
    CPoolItemAVtbl* vptr; // +0x00
    char _04[0x94 - 0x04];
    CByteArray m_94; // +0x94
    char _a8[0xc0 - 0xa8];
};


// The +0x498-pool item: a 0x38-byte struct (RezAlloc'd, vtable-less).  Field 0
// caches the POSITION; the +0x4..+0x10 + +0x14/+0x18/+0x2c/+0x30/+0x34 dwords are
// zeroed at create.  Torn down by the engine helper (Teardown, __thiscall 0x147530)
// then RezFree'd.
struct CPoolItemB {
    void Teardown();                    // 0x147530 (__thiscall) - frees owned bufs
    i32 Init(void* arg, i32 a, i32 b);  // 0x1474d0 (__thiscall) - returns success
    i32 Init2(void* arg, i32 a, i32 b); // 0x147410 (__thiscall) - alt init, returns success
    i32 Init3(void* arg, i32 a, i32 b, i32 c); // 0x147840 (__thiscall) - 3-param init, returns success

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
    char _0c[0x30 - 0x0c];
    i32(__stdcall* GetSurfaceDesc)(CCachedSurface*, void* desc); // +0x30
    char _34[0x4c - 0x34];
    void(__stdcall* Restore)(CCachedSurface*); // +0x4c
    char _50[0x54 - 0x50];
    // +0x54  reconfigure the surface (5 forwarded args); returns an HRESULT.
    i32(__stdcall* Configure)(CCachedSurface*, i32, i32, i32, i32, i32);
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
    CPoolItemA* Create7f0_1(i32 a);                             // 0x1421a0 (vtbl 7f0, slot08)
    CPoolItemA* CreateA(i32 a, i32 b, i32 c, i32 d, i32 e);     // 0x142260
    CPoolItemA* CreateB(i32 a, i32 b, i32 c, i32 d, i32 e);     // 0x1423c0
    CPoolItemA* Createa58_1(i32 a);                             // 0x1424a0 (vtbl a58, slot08)
    CPoolItemA* Createa58_3(i32 a, i32 b, i32 c);               // 0x142560 (vtbl a58, slot28)
    CPoolItemA* Createa88_3(i32 a, i32 b, i32 c);               // 0x142730 (vtbl a88, slot24)
    CPoolItemA* Createa88_1(i32 a);                             // 0x142880 (vtbl a88, slot08)
    CPoolItemA* Createab8_3(i32 a, i32 b, i32 c);               // 0x142940 (vtbl ab8, slot24, +538)
    CPoolItemA* Createab8_1(i32 a);                             // 0x142aa0 (vtbl ab8, slot08, +538)
    CPoolItemA* Createab8_24_3(i32 a);                          // 0x142b70 (vtbl ab8, slot24 3-arg, +538)
    CPoolItemA* Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x142c40 (vtbl ae8, slot24 6-arg)
    CPoolItemA* Createae8_1(i32 a);                             // 0x142da0 (vtbl ae8, slot08)
    CPoolItemA* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142e60
    CPoolItemB* MakeB(i32 a, i32 b);                            // 0x142fc0
    CPoolItemB* MakeB2(i32 a, i32 b);                           // 0x142f40 (init via 0x147410)
    CPoolItemB* MakeB3(i32 a, i32 b, i32 c);                    // 0x1430c0 (init via 0x147840)

    // Read the trailing 0x300-byte palette from a file and register a pool-B item built
    // from it (0x143150 -> MakeB; 0x143a30 -> Make950, the sibling builder).
    CPoolItemB* LoadPaletteMakeB(const char* path, i32 z);     // 0x143150
    CPoolItemB* LoadPaletteMake950(const char* path, i32 z);   // 0x143a30
    void* Make950(void* buf, i32 z);                           // 0x143950 (external sibling of MakeB)
    // Derive the R/G/B low-bit shift + 8-minus-count tables from the cached surface's
    // pixel format, then apply (Func13f740). __thiscall, no stack args (0x143b20).
    i32 ComputeColorMasks();                                   // 0x143b20
    // Reconfigure the cached surface (vtbl +0x54) and, on success, recompute the color
    // masks; report + latch the failure code on either error. 0x143c20.
    i32 ConfigureSurface(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4); // 0x143c20

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
    m_poolA.RemoveAt((POSITION)item->m_pos);
    if (item) {
        item->Delete(1);
    }
}

// ---------------------------------------------------------------------------
// Create7f0_1 (0x1421a0).  new 0xc0 item; ctor (CByteArray @+0x94, vtbl 0x5ef7f0
// stamped FIRST, then zero fields); dispatch vtbl[0x08] with 1 arg; on success
// AddItemA, else virtual-delete. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall (see docs/patterns/rezalloc-placement-new-
// no-eh-frame.md): `new CPoolItemA` w/ EH-tracked throwing member ctor needs the
// retail /GX frame; MSVC5 placement-new emits no ctor-in-flight EH state, so the body
// is byte-exact but the frame is absent. Deferred to the final sweep.
RVA(0x001421a0, 0xbe)
CPoolItemA* CDDrawPtrCollections::Create7f0_1(i32 a) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->vptr = &g_poolItemVtbl7f0;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
    } else {
        item = 0;
    }
    if (item->Init08(this, a)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
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
        new (&item->m_94) CByteArray;
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
        new (&item->m_94) CByteArray;
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
// Createa58_1 (0x1424a0).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall (see docs/patterns/...): body byte-exact,
// /GX ctor-in-flight frame absent. Deferred to the final sweep.
RVA(0x001424a0, 0xbe)
CPoolItemA* CDDrawPtrCollections::Createa58_1(i32 a) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
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
    if (item->Init08(this, a)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Createa58_3 (0x142560).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x28]
// with 3 args; AddItemA on success. /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142560, 0xc8)
CPoolItemA* CDDrawPtrCollections::Createa58_3(i32 a, i32 b, i32 c) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
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
    if (item->Init28(this, a, b, c)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Createa88_3 (0x142730).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x24]
// with 3 args; AddItemA on success. /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142730, 0xc8)
CPoolItemA* CDDrawPtrCollections::Createa88_3(i32 a, i32 b, i32 c) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtbla88;
    } else {
        item = 0;
    }
    if (item->Init24(this, a, b, c, 0, 0)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ~CPoolItemA (0x142820).  Non-deleting dtor of the pool-A item subclass (vtbl
// 0x5ef7f0): re-stamp vptr, FreeSurfaces() teardown, then the CByteArray member
// dtor (auto). /GX (trylevel 0 -> -1 around the member dtor). __thiscall, ret 0x0.
// (Proximity-attributed to CDDrawPtrCollections; it is really the item's ~dtor.)
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (docs/patterns/eh-state-numbering-base.md + eh-ctor-vptr-restamp-
// position.md): body byte-identical, residue is (a) the unwind-funcinfo push value
// 0xe vs 0x0 (global __ehfuncinfo state index, not reproducible from one TU) and
// (b) the vptr restamp scheduled one instr before vs after the trylevel-0 init.
// 94%; deferred to the final sweep.
RVA(0x00142820, 0x53)
CPoolItemA::~CPoolItemA() {
    vptr = &g_poolItemVtbl7f0;
    FreeSurfaces();
}

// ---------------------------------------------------------------------------
// Createa88_1 (0x142880).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142880, 0xbe)
CPoolItemA* CDDrawPtrCollections::Createa88_1(i32 a) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtbla88;
    } else {
        item = 0;
    }
    if (item->Init08(this, a)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_3 (0x142940).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x24]
// with 3 args; AddItemA + cache item->m_a8 into host->fieldUnknown538 on success.
// /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142940, 0xd4)
CPoolItemA* CDDrawPtrCollections::Createab8_3(i32 a, i32 b, i32 c) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtblab8;
    } else {
        item = 0;
    }
    if (item->Init24(this, a, b, c, 0, 0)) {
        AddItemA(item);
        fieldUnknown538 = item->m_a8;
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_1 (0x142aa0).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x08]
// with 1 arg; AddItemA + cache item->m_a8 into host->fieldUnknown538 on success.
// /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142aa0, 0xca)
CPoolItemA* CDDrawPtrCollections::Createab8_1(i32 a) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtblab8;
    } else {
        item = 0;
    }
    if (item->Init08(this, a)) {
        AddItemA(item);
        fieldUnknown538 = item->m_a8;
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_24_3 (0x142b70).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch
// vtbl[0x24] as a 3-arg init with the two literal tags (0x18, 0x21) + the incoming
// arg; AddItemA + cache item->m_a8 into host->fieldUnknown538 on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142b70, 0xce)
CPoolItemA* CDDrawPtrCollections::Createab8_24_3(i32 a) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtblab8;
    } else {
        item = 0;
    }
    if (item->Init24_3(this, 0x18, 0x21, a)) {
        AddItemA(item);
        fieldUnknown538 = item->m_a8;
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Createae8_6 (0x142c40).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x24]
// as a 6-arg init with all six incoming args; AddItemA on success. /GX. ret 0x18.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142c40, 0xd7)
CPoolItemA* CDDrawPtrCollections::Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtblae8;
    } else {
        item = 0;
    }
    if (item->Init24_6(this, a, b, c, d, e, f)) {
        AddItemA(item);
        return item;
    }
    if (item) {
        item->Delete(1);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ~CPoolItemA7f0 (0x142d40).  Second pool-A subclass non-deleting dtor (vtbl
// 0x5ef7f0): re-stamp vptr, FreeSurfaces() teardown, then the CByteArray member
// dtor (auto). /GX. __thiscall, ret 0x0.  Byte-identical to CPoolItemA::~CPoolItemA
// (0x142820); a distinct subclass whose dtor compiles to the same code.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (same as ~CPoolItemA @0x142820): body byte-identical, residue is the
// unwind-funcinfo push value (global __ehfuncinfo state index) + the vptr-restamp
// scheduling around the trylevel-0 init. Deferred to the final sweep.
RVA(0x00142d40, 0x53)
CPoolItemA7f0::~CPoolItemA7f0() {
    vptr = &g_poolItemVtbl7f0;
    FreeSurfaces();
}

// ---------------------------------------------------------------------------
// Createae8_1 (0x142da0).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// rezalloc-placement-new-no-eh-frame wall: body byte-exact, /GX frame absent.
RVA(0x00142da0, 0xbe)
CPoolItemA* CDDrawPtrCollections::Createae8_1(i32 a) {
    CPoolItemA* item = (CPoolItemA*)operator new(0xc0);
    if (item) {
        new (&item->m_94) CByteArray;
        item->m_08 = 0;
        item->m_0c = 0;
        item->m_pos = 0;
        item->m_7c = 0;
        item->m_a8 = 0;
        item->m_b8 = 0;
        item->vptr = &g_poolItemVtblae8;
    } else {
        item = 0;
    }
    if (item->Init08(this, a)) {
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

// ---------------------------------------------------------------------------
// MakeB3 (0x1430c0).  Third sibling of MakeB: RezAlloc a 0x38-byte CPoolItemB,
// zero its fields, init it via the 3-param Init3 (0x147840) with (m_surf0, a, b,
// c); on success add to pool B and return it, else tear down + RezFree and return
// 0.  No EH frame (no destructible local) -> matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x001430c0, 0x81)
CPoolItemB* CDDrawPtrCollections::MakeB3(i32 a, i32 b, i32 c) {
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
