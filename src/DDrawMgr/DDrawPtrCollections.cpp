#include <Mfc.h> // real MFC CPtrList / CPtrArray / CByteArray (NAFXCW, reloc-masked)
#include <Ints.h>
#include <rva.h>

#include <Io/FileStream.h>                // engine CFileIO (palette loaders)
#include <string.h>                       // memset (inlined to rep stos at /O2 /Oi)
#include <DDrawMgr/DDrawPtrCollections.h> // single-source CDDrawPtrCollections class shape
#include <DDrawMgr/DDSurface.h>           // the UNIFIED CDDSurface (the +0x47c pool item base)
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
// (CDDrawPtrCollections is defined in <DDrawMgr/DDrawPtrCollections.h>, above.)

// The pool-A items' operator delete (invoked by the scalar-deleting dtors); the
// engine free, reloc-masked rel32.
void operator delete(void*);

// ---------------------------------------------------------------------------
// The +0x47c-pool surface-item family.  GENUINE C++ VTABLES (verified against the
// retail .rdata): a 9-slot polymorphic BASE (vtable 0x5ef7f0) plus four DERIVED
// subclasses (0x5efa58 / a88 / ab8 / ae8) that override {virtual dtor slot 0, the
// slot-2 init, the slot-6 surface op} and add per-subclass init tail slots.  Modeled
// real-polymorphic so cl emits each ??_7 and auto-stamps the implicit vptr in the
// ctor/dtor; every base slot fn is defined in a sibling TU so the emitted vtable slots
// are reloc-masked DIR32 relocs.
//
// The init entry points the factories dispatch are ordinary virtual slots:
//   slot 2  (byte +0x08)  the 1-arg init  - base virtual (overridden by ab8 / ae8)
//   slot 9  (byte +0x24)  the primary init - per-subclass tail virtual
//   slot 10 (byte +0x28)  a58's 3-arg init - a58 tail virtual
//   slot 11 (byte +0x2c)  a58's 5-arg init - a58 tail virtual
// A pool item is 0xc0 bytes and owns a CPtrArray @+0x94 (its throwing ctor is what
// gives each factory `new` its /GX frame).
// ---------------------------------------------------------------------------
// The +0x47c-pool surface item IS the UNIFIED CDDSurface (the 0xc0 DIRSURF.CPP base
// surface, defined in <DDrawMgr/DDSurface.h> - formerly modeled here as the local view
// CPoolItemBase) plus its four DERIVED subclasses below (a58/a88/ab8/ae8). The base
// vtable 0x5ef7f0 is bound in DirectDrawMgr.cpp (the g_poolItemVtbl by-address factory
// stamp); the ddraw unit's emitted ??_7CDDSurface is left unbound so it does not collide.
//
// The base ctor + dtor are defined INLINE here (before the derived classes) so each
// derived dtor inlines the shared teardown - retail has no out-of-line base dtor in this
// TU; its kept COMDAT is CDDSurface::~CDDSurface @0x141350 in the image TU (the build
// delinks per-unit, so the two definitions of ~CDDSurface do not collide). See the
// surface-family doc: docs/patterns/surface-pool-comdat-dtors.md.

// Zero the scalar fields (the CPtrArray member + vptr are the compiler's job).
inline CDDSurface::CDDSurface() {
    m_8 = 0;
    m_c = 0;
    m_pos = 0;
    m_dontOwn = 0;
    m_bitDepth = 0;
    m_b8 = 0;
}
inline CDDSurface::~CDDSurface() {
    FreeSurfaces();
}

// vtable 0x5efa58: overrides the dtor (??_G 0x142340 / ~ 0x142360) and slot 6
// (0x143cc0); adds three init tail slots (9 = 0x148890, 10 = 0x148940, 11 = 0x148840).
// This a58 subclass is the SAME class the Image TU models as CFileImageSurface: its
// ??_G/~ COMDAT (0x142340/0x142360) is emitted by BOTH the image AND ddraw TUs (both
// `new` a58 items) but the linker kept the image copy - so a58's non-deleting dtor lives
// at 0x142360 in <image> (?ScalarDelete@CFileImageSurface + ??1CFileImageSurface), and
// here it is declared-only (its emitted COMDAT is the discarded copy). See the surface-
// family doc: docs/patterns/surface-pool-comdat-dtors.md.
class CPoolItemA : public CDDSurface {
public:
    virtual ~CPoolItemA() OVERRIDE; // slot 0  ~ 0x142360 (image copy)
    virtual i32 v18() OVERRIDE;     // slot 6  0x143cc0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 9  0x148890
    virtual i32 v28(CDDrawPtrCollections*, i32, i32, i32);           // slot 10 0x148940
    virtual i32 v2c(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 11 0x148840
};
SIZE(CPoolItemA, 0xc0);
VTBL(CPoolItemA, 0x001efa58);

// vtable 0x5efa88: overrides the dtor (??_G 0x142800 / ~ 0x142820) and slot 6 (0x143cb0);
// adds two init tail slots (9 = 0x148a50, 10 = 0x148ac0).  Its ~ (0x142820) is emitted
// here (only the ddraw TU `new`s a88 items).
class CPoolItemA88 : public CDDSurface {
public:
    virtual ~CPoolItemA88() OVERRIDE;                                // slot 0  ~ 0x142820
    virtual i32 v18() OVERRIDE;                                      // slot 6  0x143cb0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 9  0x148a50
    virtual i32 v28(CDDrawPtrCollections*, i32, i32, i32);           // slot 10 0x148ac0
};
SIZE(CPoolItemA88, 0xc0);
VTBL(CPoolItemA88, 0x001efa88);

// vtable 0x5efab8: overrides the dtor (??_G 0x142a20 / ~ 0x142a40), slot 2 (0x148b50) and
// slot 6 (0x143cd0); adds two init tail slots (9 = 0x148af0, 10 = 0x148b80).  Its ~
// (0x142a40) is emitted here (was formerly mislabeled ~CDDSurface in DDSurfaceDtor.cpp).
class CPoolItemAB8 : public CDDSurface {
public:
    virtual ~CPoolItemAB8() OVERRIDE;                                // slot 0  ~ 0x142a40
    virtual i32 Init1(CDDrawPtrCollections*, i32) OVERRIDE;          // slot 2  0x148b50
    virtual i32 v18() OVERRIDE;                                      // slot 6  0x143cd0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32); // slot 9  0x148af0
    virtual i32 v28(CDDrawPtrCollections*, i32, i32, i32);           // slot 10 0x148b80
};
SIZE(CPoolItemAB8, 0xc0);
VTBL(CPoolItemAB8, 0x001efab8);

// vtable 0x5efae8: overrides the dtor (??_G 0x142d20 / ~ 0x142d40), slot 2 (0x148cc0)
// and slot 6 (0x143ce0); adds one init tail slot (9 = 0x148c40, 6-arg).
class CPoolItemAE8 : public CDDSurface {
public:
    virtual ~CPoolItemAE8() OVERRIDE;                                     // slot 0  ~ 0x142d40
    virtual i32 Init1(CDDrawPtrCollections*, i32) OVERRIDE;               // slot 2  0x148cc0
    virtual i32 v18() OVERRIDE;                                           // slot 6  0x143ce0
    virtual i32 v24(CDDrawPtrCollections*, i32, i32, i32, i32, i32, i32); // slot 9  0x148c40
};
SIZE(CPoolItemAE8, 0xc0);
VTBL(CPoolItemAE8, 0x001efae8);

struct IDirectDrawPalette; // <ddraw.h> in the palette-owning TUs; pointer-only here

// The +0x498-pool item IS a CDDPalette (DIRPAL.CPP, 0x38-byte RezAlloc'd palette
// wrapper) - the same class DirectDrawMgr.h models fully (as `CDDPalette`) and
// BoundaryUpper2.cpp's Create@0x143040 news+returns (`CDDPalette*`, mangled PAU =>
// struct).  PROVEN by method-RVA equality: the pool's Init/Init2/Init3/Teardown ARE
// CDDPalette's CreateRGB(0x1474d0)/LoadFromFile(0x147410)/CreateFromTrailing(0x147840)/
// Destroy(0x147530); the 0x38 layout + the ctor field-zero set match exactly.  The
// pool-context method names + signatures are kept here (reconciling them with the
// DirectDrawMgr.h view's full signatures - m_surf0's real DirectDraw type - is a
// separate dedup; each TU currently carries its own view, none in the same TU so no
// ODR clash).  Field 0 caches the CPtrList POSITION.
struct CDDPalette {
    inline CDDPalette();
    inline void* operator new(u32);

    void Teardown();                           // 0x147530  == CDDPalette::Destroy
    i32 Init(void* dd, void* rgb, i32 flags);  // 0x1474d0  == CDDPalette::CreateRGB
    i32 Init2(void* arg, i32 a, i32 b);        // 0x147410  == CDDPalette::LoadFromFile
    i32 Init3(void* arg, i32 a, i32 b, i32 c); // 0x147840  == CDDPalette::CreateFromTrailing

    // Field names migrated from the DirectDrawMgr.h CDDPalette view (proven same class).
    POSITION m_pos;                // +0x00 cached CPtrList POSITION (this pool-context use of
                                   //       DirectDrawMgr's +0x00 field)
    IDirectDrawPalette* m_palette; // +0x04 held palette interface
    i32 m_8;                       // +0x08 cleared by Destroy
    u8* m_cacheA;                  // +0x0c PALETTEENTRY cache A (0x400 bytes)
    u8* m_cacheB;                  // +0x10 PALETTEENTRY cache B (0x400 bytes)
    i32 m_14;                      // +0x14 Flush pending fill color
    u8* m_18;                      // +0x18 third buffer freed by Destroy
    char _1c[0x2c - 0x1c];
    i32 m_2c; // +0x2c Flush blit start
    i32 m_30; // +0x30 Flush blit count
    i32 m_34; // +0x34 Flush pending flag (cleared by Destroy)
};
SIZE(CDDPalette, 0x38); // measured: new(0x38) -> RezAlloc'd 0x38-byte item

inline CDDPalette::CDDPalette() {
    m_palette = 0;
    m_pos = 0;
    m_8 = 0;
    m_cacheA = 0;
    m_cacheB = 0;
    m_34 = 0;
    m_18 = 0;
    m_14 = 0;
    m_2c = 0;
    m_30 = 0;
}

inline void* CDDPalette::operator new(u32) {
    return ::operator new(0x38);
}

// A cached surface object (the +0x00 / +0x04 slots): a real abstract C++ class with
// __stdcall virtuals (`this` pushed, callee-cleaned - no caller `add esp,4`), so
// `surf->Slot(args)` lowers to the same `mov reg,[surf]; call [reg+slot]` the manual
// vtbl-struct dispatch did. External (never constructed here) so no vtable is emitted.
// Placeholder slots land Release(2)/GetSurfaceDesc(12)/Restore(19)/Configure(21) at
// their retail vtable indices.
struct CCachedSurface {
    virtual void __stdcall Method00();                // slot 0  (+0x00)
    virtual void __stdcall s04();                     // slot 1  (+0x04)
    virtual void __stdcall Release();                 // slot 2  (+0x08)
    virtual void __stdcall s0c();                     // slot 3
    virtual void __stdcall s10();                     // slot 4
    virtual void __stdcall s14();                     // slot 5
    virtual void __stdcall s18();                     // slot 6
    virtual void __stdcall s1c();                     // slot 7
    virtual void __stdcall s20();                     // slot 8
    virtual void __stdcall s24();                     // slot 9
    virtual void __stdcall s28();                     // slot 10
    virtual void __stdcall s2c();                     // slot 11
    virtual i32 __stdcall GetSurfaceDesc(void* desc); // slot 12 (+0x30)
    virtual void __stdcall s34();                     // slot 13
    virtual void __stdcall s38();                     // slot 14
    virtual void __stdcall s3c();                     // slot 15
    virtual void __stdcall s40();                     // slot 16
    virtual void __stdcall s44();                     // slot 17
    virtual void __stdcall s48();                     // slot 18
    virtual void __stdcall Restore();                 // slot 19 (+0x4c)
    virtual void __stdcall s50();                     // slot 20 (+0x50)
    // slot 21 (+0x54): reconfigure the surface (5 forwarded args); returns an HRESULT.
    virtual i32 __stdcall Configure(i32, i32, i32, i32, i32);
};

// The CDDrawPtrCollections host class (0x948 B) is the single-source shape from
// <DDrawMgr/DDrawPtrCollections.h> (included at the top); the method bodies below
// implement it and the CPoolItem* / CCachedSurface helper family it dispatches.

// ---------------------------------------------------------------------------
// Constructor (0x141cc0).  /GX EH frame to unwind the three containers.
// ---------------------------------------------------------------------------
RVA(0x00141cc0, 0x84)
CDDrawPtrCollections::CDDrawPtrCollections() : m_poolA(0xa), m_poolB(0xa), m_array() {
    m_surf0 = 0;
    m_surf4 = 0;
    m_534 = 0;
    m_palBpp = 0;
    m_hasPalette = 0;
    m_940 = 0;
    m_944 = 0;
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
        m_surf0->Restore();
    }
    for (i32 i = 0; i < m_array.GetSize(); i++) {
        RezFree(m_array.GetData()[i]);
    }
    m_array.SetSize(0, -1);
    EmptyPoolA();
    EmptyPoolB();
    g_DirectDrawMgr = 0;
    if (m_surf0) {
        m_surf0->Release();
        m_surf0 = 0;
    }
    if (m_surf4) {
        m_surf4->Release();
        m_surf4 = 0;
    }
    m_534 = 0;
}

// ---------------------------------------------------------------------------
// AddItemA (0x142100).  pool.AddTail(item); item->pos = position.
// ---------------------------------------------------------------------------
RVA(0x00142100, 0x18)
void CDDrawPtrCollections::AddItemA(CDDSurface* item) {
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
        CDDSurface* item = (CDDSurface*)cur->data;
        delete item;
    }
    m_poolA.RemoveAll();
}

// ---------------------------------------------------------------------------
// RemoveItemA (0x142160).  pool.RemoveAt(item->pos); virtual-delete item.
// ---------------------------------------------------------------------------
RVA(0x00142160, 0x24)
void CDDrawPtrCollections::RemoveItemA(CDDSurface* item) {
    m_poolA.RemoveAt(item->m_pos);
    delete item;
}

// ---------------------------------------------------------------------------
// Create7f0_1 (0x1421a0).  new 0xc0 item; ctor (CByteArray @+0x94, vtbl 0x5ef7f0
// stamped FIRST, then zero fields); dispatch vtbl[0x08] with 1 arg; on success
// AddItemA, else virtual-delete. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall: real-polymorphic `new CDDSurface` now emits the /GX ctor-in-flight
// frame (the throwing CByteArray member ctor), but the global __ehfuncinfo state-index
// push differs from retail (not reproducible from one TU); body byte-exact. Deferred.
RVA(0x001421a0, 0xbe)
CDDSurface* CDDrawPtrCollections::Create7f0_1(i32 a) {
    CDDSurface* item = new CDDSurface;
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
CDDSurface* CDDrawPtrCollections::CreateA(i32 a, i32 b, i32 c, i32 d, i32 e) {
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
CDDSurface* CDDrawPtrCollections::CreateB(i32 a, i32 b, i32 c, i32 d, i32 e) {
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
CDDSurface* CDDrawPtrCollections::Createa58_1(i32 a) {
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
CDDSurface* CDDrawPtrCollections::Createa58_3(i32 a, i32 b, i32 c) {
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
CDDSurface* CDDrawPtrCollections::Createa88_3(i32 a, i32 b, i32 c) {
    CPoolItemA88* item = new CPoolItemA88;
    if (item->v24(this, a, b, c, 0, 0)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// The shared base teardown the derived dtors inline (defined INLINE in the class
// body above): re-stamp the base vptr (0x5ef7f0), run FreeSurfaces(), then destroy
// the owned CByteArray member (auto).  /GX (trylevel 0 -> -1 around the member dtor).
// cl folds the redundant derived vptr stamp (dead store), leaving the base 0x5ef7f0
// stamp - matching retail's per-class inlined dtors.  (Base ~CDDSurface itself is
// CFileImage::~CFileImage @0x141350 in a sibling TU; the inline definition emits no
// out-of-line body here, so it does not collide.)
// ---------------------------------------------------------------------------
// ~CPoolItemA88 (0x142820).  Derived a88 non-deleting dtor - trivial body; inlines the
// base teardown above (INLINE ~CDDSurface: implicit stamp-first, FreeSurfaces, member
// dtor - the a88 vptr stamp folds as a dead store, leaving the base 0x5ef7f0 stamp).
// __thiscall, ret 0x0.  Byte-identical to every other pool-item dtor (the vptr operand
// reloc-masks to 0x5ef7f0); the OWNING class is fixed by its ??_G (0x142800, a88 vtable
// slot 0), not the byte pattern.  Byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00142820, 0x53)
CPoolItemA88::~CPoolItemA88() {}

// ---------------------------------------------------------------------------
// Createa88_1 (0x142880).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142880, 0xbe)
CDDSurface* CDDrawPtrCollections::Createa88_1(i32 a) {
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
// with 3 args; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success.
// /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142940, 0xd4)
CDDSurface* CDDrawPtrCollections::Createab8_3(i32 a, i32 b, i32 c) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->v24(this, a, b, c, 0, 0)) {
        AddItemA(item);
        m_palBpp = item->m_bitDepth;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// ~CPoolItemAB8 (0x142a40).  Derived ab8 non-deleting dtor - trivial body; inlines the
// shared base teardown (INLINE ~CDDSurface: stamp 0x5ef7f0 stamp-first + FreeSurfaces +
// member dtor; the ab8 vptr stamp folds as a dead store).  __thiscall, ret 0x0.  Byte-
// identical to the other pool-item dtors; owner fixed by its ??_G (0x142a20, ab8 vtable
// slot 0).  Was formerly mislabeled ~CDDSurface in DDSurfaceDtor.cpp.  Byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00142a40, 0x53)
CPoolItemAB8::~CPoolItemAB8() {}

// ---------------------------------------------------------------------------
// Createab8_1 (0x142aa0).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x08]
// with 1 arg; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success.
// /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142aa0, 0xca)
CDDSurface* CDDrawPtrCollections::Createab8_1(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->Init1(this, a)) {
        AddItemA(item);
        m_palBpp = item->m_bitDepth;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_24_3 (0x142b70).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch
// vtbl[0x24] as a 3-arg init with the two literal tags (0x18, 0x21) + the incoming
// arg; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall + arity: this call site invokes ab8 slot 9 with only 3 game args, but
// slot 9 (0x148af0) is the same virtual Createab8_3 calls with 5, so a single C++
// signature can serve only one - the 3-arg site over-pushes two zeros here (accepted;
// the retail author called the same slot with two arities). /GX state-index residue.
RVA(0x00142b70, 0xce)
CDDSurface* CDDrawPtrCollections::Createab8_24_3(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->v24(this, 0x18, 0x21, a, 0, 0)) {
        AddItemA(item);
        m_palBpp = item->m_bitDepth;
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
CDDSurface* CDDrawPtrCollections::Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
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
// shared base teardown (INLINE ~CDDSurface: stamp 0x5ef7f0 stamp-first + FreeSurfaces +
// member dtor).  /GX, ret 0x0.  Byte-identical codegen to ~CPoolItemA (0x142820); a
// distinct subclass.  Byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00142d40, 0x53)
CPoolItemAE8::~CPoolItemAE8() {}

// ---------------------------------------------------------------------------
// Createae8_1 (0x142da0).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142da0, 0xbe)
CDDSurface* CDDrawPtrCollections::Createae8_1(i32 a) {
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
CDDSurface* CDDrawPtrCollections::MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    return CreateB(a, b, c, d | 0x840, e);
}

// ---------------------------------------------------------------------------
// AddItemB (0x142eb0).  pool.AddTail(item); item->pos = position.
// ---------------------------------------------------------------------------
RVA(0x00142eb0, 0x17)
void CDDrawPtrCollections::AddItemB(CDDPalette* item) {
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
        CDDPalette* item = (CDDPalette*)cur->data;
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
void CDDrawPtrCollections::RemoveItemB(CDDPalette* item) {
    m_poolB.RemoveAt(item->m_pos);
    if (item) {
        item->Teardown();
        RezFree(item);
    }
}

// ---------------------------------------------------------------------------
// MakeB2 (0x142f40).  Sibling of MakeB: RezAlloc a 0x38-byte CDDPalette, zero its
// fields, init it via the alternate Init2 (0x147410) with (m_surf0, a, b); on success
// add to pool B and return it, else tear down + RezFree and return 0.  NO EH frame
// (no destructible local), so this matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x00142f40, 0x7c)
CDDPalette* CDDrawPtrCollections::MakeB2(i32 a, i32 b) {
    CDDPalette* item = new CDDPalette;
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
// MakeB (0x142fc0).  RezAlloc a 0x38-byte CDDPalette, zero its fields, init it via
// the external Item498_Init (0x1474d0) with (vtbl-of-this, a, b); on success add to
// pool B and return it, else tear down + RezFree and return 0.
// ---------------------------------------------------------------------------
RVA(0x00142fc0, 0x7c)
CDDPalette* CDDrawPtrCollections::MakeB(void* rgb, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->Init(m_surf0, rgb, flags)) {
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
// MakeB3 (0x1430c0).  Third sibling of MakeB: RezAlloc a 0x38-byte CDDPalette,
// zero its fields, init it via the 3-param Init3 (0x147840) with (m_surf0, a, b,
// c); on success add to pool B and return it, else tear down + RezFree and return
// 0.  No EH frame (no destructible local) -> matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x001430c0, 0x81)
CDDPalette* CDDrawPtrCollections::MakeB3(i32 a, i32 b, i32 c) {
    CDDPalette* item = new CDDPalette;
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
CDDPalette* CDDrawPtrCollections::LoadPaletteMakeB(const char* path, i32 z) {
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
    return MakeB(buf, z);
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
CDDPalette* CDDrawPtrCollections::LoadPaletteMake950(const char* path, i32 z) {
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
    return Make950(buf, z);
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
    i32 hr = m_surf0->GetSurfaceDesc(&desc);
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
    i32 hr = m_surf0->Configure(a0, a1, a2, a3, a4);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x8a2, hr);
        if (m_944 == 0) {
            m_944 = 0x3ec;
        }
        return hr;
    }
    if (ComputeColorMasks() == 0) {
        hr = (i32)0x80004005;
        if (m_944 == 0) {
            m_944 = 0x3ed;
        }
    }
    return hr;
}

SIZE_UNKNOWN(CCachedSurface);
SIZE_UNKNOWN(CPtrListNode);
SIZE_UNKNOWN(SurfDesc);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
VTBL(CCachedSurface, 0x001ef7f0);
