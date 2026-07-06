#include <rva.h>

#include <Wap32/WapObj.h> // CWapObj : Wap::CObject - real base for the "A" spawned child
#include <DDrawMgr/DDrawSurfacePair.h> // single-source CDDrawSurfacePair (the "B" spawned child)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr (m_0c parent, m_lastError @+0x38)
#include <DDrawMgr/DDrawSubMgrPages.h> // THE single-source CDDrawSubMgrPages shape (23-slot vtable)
// CDDrawSubMgrPages.cpp - the page/child factory + teardown half of CDDrawSubMgrPages
// (a CDirectDrawMgr surface/page sub-manager in the "DDraw surface manager" family;
// see docs/ddraw-family-names.md). The 0x158b40..0x159ef0 surface-op cluster (former
// CDDrawWorkerMgr view) lives in src/DDrawMgr/DDrawSubMgr.cpp; both share the ONE
// class shape from <DDrawMgr/DDrawSubMgrPages.h>.
//
// CDDrawSubMgrPages is a real polymorphic CWapObj: the ctor stamps ??_7CDDrawSubMgrPages
// vptr-first and ~CDDrawSubMgrPages folds the grand-base re-stamp (0x5e8cb4) last, both
// cl-emitted - NO manual `*(void**)this = &g_*Vtbl` store. It owns three surface
// elements at +0x10/+0x14/+0x18 (front/back/overlay), each a polymorphic CDDrawSurfacePair
// (slot-1 scalar-deleting dtor), so DestroyChildren `delete`s them.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// The engine block allocator (retail 0x1b9b46), used raw by CreateChildren, plus the
// standard placement-new (the vptr-last child-spawn pattern needs raw alloc + explicit
// placement-construct + a manual derived-vtable re-stamp, not a plain `new` expression).
void* operator new(u32 n);
inline void* operator new(u32, void* p) {
    return p;
}

// The two spawned-child vtables, stamped manually into the heap block AFTER the base
// ctor runs (transitional workaround: the children's virtuals live in unmatched TUs,
// so cl can't emit these vtables - referenced as reloc-masked DATA externs).

// The "A" child built by CreateChildren (0x30 bytes, ctor 0x158f30, vtable 0x5eff70).
// Real CWapObj-derived: slots 0..4 (CObject thunks + scalar dtor) + slot 6 (IsReady
// 0x001c08) inherited, slot 5 the IsLoaded override (0x159150); own slots 7..9 named
// from their retail slot RVAs. The "B" child at +0x14/+0x18 is the retail
// CDDrawSurfacePair (own vtable 0x5eff30, from <DDrawMgr/DDrawSurfacePair.h>).
class CDDrawSurfaceChildA : public CWapObj {
public:
    i32 IsLoaded() OVERRIDE;                                      // slot 5 (@0x14) 0x159150
    virtual void Slot07_1591d0();                                 // slot 7 (@0x1c) 0x1591d0
    virtual void Slot08_159180();                                 // slot 8 (@0x20) 0x159180
    virtual i32 CreateModeSurface_1644a0(i32 a1, i32 a2, i32 a3); // slot 9 (@0x24) 0x1644a0
    CDDrawSurfaceChildA(i32 handle, i32 a2, i32 a3);              // 0x158f30
    char m_pad04[0x2c - 0x04];
    i32 m_2c; // +0x2c
}; // 0x30
SIZE(CDDrawSurfaceChildA, 0x30);

// ---------------------------------------------------------------------------
// slot 5 (IsLoaded, 0x157480): ready when all three owned child pointers are populated.
RVA(0x00157480, 0x1e)
i32 CDDrawSubMgrPages::IsLoaded() {
    if (m_backPair == 0) {
        goto fail;
    }
    if (m_overlayPair == 0) {
        goto fail;
    }
    if (m_frontPair != 0) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// slot 7 (DestroyChildren, 0x158ac0): for each owned child at +0x10/+0x14/+0x18, if
// non-null run its scalar-deleting destructor (vtbl slot 1, delete-flag 1) via delete
// and null the slot.
RVA(0x00158ac0, 0x44)
void CDDrawSubMgrPages::DestroyChildren() {
    if (m_frontPair != 0) {
        delete m_frontPair;
        m_frontPair = 0;
    }
    if (m_backPair != 0) {
        delete m_backPair;
        m_backPair = 0;
    }
    if (m_overlayPair != 0) {
        delete m_overlayPair;
        m_overlayPair = 0;
    }
}

// ---------------------------------------------------------------------------
// slot 8 (GetStateId, 0x1574a0): constant state id.
RVA(0x001574a0, 0x6)
StateId CDDrawSubMgrPages::GetStateId() {
    return STATE_SUBMGRPAGES; // 0xf
}

// ---------------------------------------------------------------------------
// Member-teardown destructor (0x1574d0; retail ~CDDrawSubMgrDraco). cl stamps the
// own vftable ??_7CDDrawSubMgrPages (masks 0x5efe08) at entry, devirtualizes
// DestroyChildren (slot 7) to a direct call, resets the three header words, then the
// empty grand-base subobject dtor folds the g_wapObjectDtorVtbl (0x5e8cb4) re-stamp
// last. The destructible CWapObj grand-base gives the /GX EH frame. The scalar-
// deleting dtor ??_G (0x1574b0) is cl-auto-generated from this virtual dtor.
RVA(0x001574d0, 0x5b)
CDDrawSubMgrPages::~CDDrawSubMgrPages() {
    DestroyChildren();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    // implicit ~CWapObj -> ~Wap::CObject folds the grand-base re-stamp (0x5e8cb4) last.
}

// ---------------------------------------------------------------------------
// slot 9 (CreateChildren, 0x1588f0): build the three owned children then run their
// per-stage init. Alloc child A (0x30, ctor 0x158f30(handle,0,0), vtable 0x5eff70) ->
// m_frontPair; children B and C (0x34, ctor 0x156cb0 = CDDrawSurfacePair(handle,N,0),
// vtable 0x5eff30, m_width=0/m_surface=0/m_ownsSurface=1) -> m_backPair/m_overlayPair.
// Then A->CreateModeSurface, B->Create, and (unless arg4&1) C->Create, each with
// (a1,a2,a3[,0]); on any failure stamp the root's m_lastError (0x7d1/0x7d2/0x7d3 if not
// already set) and return 0. All-ok -> 1. /GX EH frame tracks the partially-built
// children during construction.
// @early-stop
// vptr-position / worker-ctor-shape wall: retail stamps each child's derived vtable
// (0x5eff70 / 0x5eff30) AFTER the base ctor + field seeds (vptr-last); the placement
// `new` model stamps vptr-first, and the two child ctors/vtables are foreign engine
// data (reloc-masked). Logic/CFG/offsets/error-codes reproduced.
RVA(0x001588f0, 0x1c5)
i32 CDDrawSubMgrPages::CreateChildren(i32 a1, i32 a2, i32 a3, i32 a4) {
    CDDrawSurfaceChildA* a = (CDDrawSurfaceChildA*)operator new(0x30);
    if (a != 0) {
        new (a) CDDrawSurfaceChildA((i32)m_0c, 0, 0);
        *(
             i32**
        ) // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
         a->m_2c = 0;
    }
    m_frontPair = (CDDrawSurfacePair*)a;

    CDDrawSurfacePair* b = (CDDrawSurfacePair*)operator new(0x34);
    if (b != 0) {
        new (b) CDDrawSurfacePair((i32)m_0c, 1, 0);
        b->m_width = 0;
        *(
             i32**
        ) // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
         b->m_surface = 0;
        b->m_ownsSurface = 1;
    }
    m_backPair = b;

    CDDrawSurfacePair* c = (CDDrawSurfacePair*)operator new(0x34);
    if (c != 0) {
        new (c) CDDrawSurfacePair((i32)m_0c, 2, 0);
        c->m_width = 0;
        *(
             i32**
        ) // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
         c->m_surface = 0;
        c->m_ownsSurface = 1;
    }
    m_overlayPair = c;

    if (a->CreateModeSurface_1644a0(a1, a2, a3) == 0) {
        if (m_0c->m_lastError == 0) {
            m_0c->m_lastError = 0x7d1;
        }
        return 0;
    }
    if (b->Create(a1, a2, a3, 0) == 0) {
        if (m_0c->m_lastError == 0) {
            m_0c->m_lastError = 0x7d2;
        }
        return 0;
    }
    if (!(a4 & 1)) {
        if (c->Create(a1, a2, a3, 0) == 0) {
            if (m_0c->m_lastError == 0) {
                m_0c->m_lastError = 0x7d3;
            }
            return 0;
        }
    }
    return 1;
}

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
