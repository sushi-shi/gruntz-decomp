#include <rva.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
// <Mfc.h> brings the real MFC CPtrList / CMapPtrToPtr (afxcoll) used by the
// CWwdObjMgr collection class below.  Must precede any windows/DirectX header.
#include <Mfc.h>
#include <Gruntz/CWwdObjMgr.h>  // the shared object-collection manager class
#include <Gruntz/CWwdWorker.h>  // the shared per-object worker class
#include <Gruntz/LogicRecord.h> // CLogicRecord (the per-object kill-cue at m_7c)
#include <string.h>             // strcpy (the inline CRT copy in Serialize_15c970)
// The discovered cluster's surface helpers (Blt/Flip/BltFast on CDDSurface) come
// from the DDrawMgr group; reuse its real types instead of placeholder casts.
#include <Gruntz/CDirectDrawMgr.h>
#include <Gruntz/CDDrawBlitParam.h> // single-source CDDrawBlitParam (also used by WwdGameObject.cpp)
#include <Gruntz/CDDrawWorkerMgr.h> // single-source CDDrawWorkerMgr (0x158xxx surface ops)
#include <Globals.h>
// CDDrawSubMgr.cpp - tomalla-named DDraw surface/page-manager shared base
// (CDDrawSubMgr). The ctor 0x156cb0 (??0CDDrawSubMgr) constructs the CLoadable base
// vtable (0x5efc30) - it is the shared base arg-ctor called by CDDrawSurfaceMgr::Init,
// CreateChildren, CreateObject_1598d0 and ReadPlaneObjects to build a CLoadable-vtable
// subobject. The matching-mis-model that put a DERIVED dtor here (0x1574d0, which
// stamps 0x5efe08 = ??_7CDDrawSubMgrDraco and calls DestroyChildren) has been SPLIT
// out to CDDrawSubMgrPages.cpp as ~CDDrawSubMgrPages (that IS the CDDrawSubMgrDraco
// class - vtable dump: 0x5efe08 slot 1 = 0x1574b0 = CDDrawSubMgrPages::ScalarDtor).
//
// Field names are tomalla placeholders; only the OFFSETS + the emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// Forward-declare the family manager (root) stored at CGruntzMgr+0x30.
// Full definition lives in CDDrawSurfaceMgr.cpp (CDDrawSurfaceMgr unit); role
// names are documented in docs/ddraw-family-names.md.
class CDDrawSurfaceMgr;

// The CDDrawSubMgr and CObject vtables are used in the dtor vtable chain, emitted
// automatically by the compiler.
class CDDrawSubMgrBase {
public:
    CDDrawSubMgrBase() {}
    CDDrawSubMgrBase(i32 x) {
        m_base04 = x;
    }
    virtual ~CDDrawSubMgrBase() {}
    i32 m_base04; // +0x04
};

class CDDrawSubMgr : public CDDrawSubMgrBase {
public:
    CDDrawSubMgr(CDDrawSurfaceMgr* pSurfaceMgr, i32 a2, i32 a3);
    // NOTE: the member-teardown dtor at 0x1574d0 was NOT this class's - it is the
    // DERIVED ~CDDrawSubMgrPages (retail ~CDDrawSubMgrDraco, vtable 0x5efe08); it now
    // lives in CDDrawSubMgrPages.cpp. This ctor's class carries the CLoadable base
    // vtable (0x5efc30). The inline empty dtor only keeps the ctor polymorphic so the
    // (spurious, reloc-masked) ??_7CDDrawSubMgr vptr stamp still falls out.
    virtual ~CDDrawSubMgr() {}
    virtual void IsReady();
    virtual i32 Init();
    virtual i32 OnDestroy();      // 0x1576c0 (state predicate, returns 1)
    virtual StateId GetStateId(); // 0x157790 (state predicate, returns 1)

    // Engine-label backlog stub (scalar-deleting dtor of a far sibling class).
    void* Stub_155720(i32 flag);

    i32 m_field08;                   // +0x08
    CDDrawSurfaceMgr* m_pSurfaceMgr; // +0x0c
};

// operator delete (used indirectly via OnDestroy; may throw -> /GX).
void operator delete(void*);

// ---- Mis-homed family member-teardown destructors (from the vtable scan) --------
#include <Gruntz/CMapStringToOb.h>

// The CObject-like family grand-base: 5-slot vtable (masks 0x5e8cb4), header fields
// +0x04..+0x0c, non-virtual ~ = the field resets + the implicit base ??_7 re-stamp.
// Slot 1 is the (declared-only) ??_G scalar-deleting dtor. Same shape as
// CDDrawSubMgrGrandBase / CDDrawWorkerCacheBase.
struct FamilyMapBase {
    virtual void s0();                  // [0]
    virtual void* ScalarDtor(i32 flag); // [1]
    virtual void s2();                  // [2]
    virtual void s3();                  // [3]
    virtual void s4();                  // [4]
    ~FamilyMapBase();
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    FamilyMapBase() {}
};
inline FamilyMapBase::~FamilyMapBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

// 3-map sibling (vtable 0x5efdc0): member-teardown ~ at 0x157630; its ??_G scalar-dtor
// (0x157610) lives in CDDrawWorkerMapSmall.cpp and calls this ~.
struct CDDrawChildGroupDtorHost : public FamilyMapBase {
    void Cleanup_1591e0();       // teardown helper (0x1591e0), declared-only
    ~CDDrawChildGroupDtorHost(); // 0x157630
    CMapStringToOb m_10;         // +0x10
    CMapStringToOb m_2c;         // +0x2c
    CMapStringToOb m_48;         // +0x48
    i32 m_64;                    // +0x64
};

// 1-map sibling (vtable 0x5efd28): member-teardown ~ at 0x156e10; its ??_G scalar-dtor
// (0x156df0) lives in CDDrawWorkerRegistry.cpp and calls this ~.
struct CDDrawRegistryDtorHost : public FamilyMapBase {
    void Cleanup_154ac0();     // teardown helper (0x154ac0), declared-only
    ~CDDrawRegistryDtorHost(); // 0x156e10
    CMapStringToOb m_10;       // +0x10
};

// The far sibling class (real member-teardown ~ at 0xd5d70) whose ??_G scalar-deleting
// destructor (0x155720) landed in this TU; referenced so the ??_G call reloc names it.
class CDDrawSubMgrFar {
public:
    virtual ~CDDrawSubMgrFar();
};

// ---------------------------------------------------------------------------
// The 0x158xxx-0x15c970 discovered cluster is a DISTINCT, LARGER class than the
// small CDDrawSubMgr above (verified: that class's vtable is g_loadableVtbl
// with fields at 0x04/0x08/0x0c; the cluster reads 0x0c/0x10/0x14/0x18/0x2c/0x48/0x54
// and is constructed by Constructor_157630, whose vtable is g_wapObjectDtorVtbl).
// The cluster carries TWO objects:
//   - the worker MANAGER (this == 0x158xxx methods): m_worker worker, m_frontPair/m_backPair/m_overlayPair
//     polymorphic surface-pairs, m_2c/m_48 CMaps, m_54 bool.
//   - a small per-frame BLIT-PARAM/element struct (this == 0x15c2xx methods):
//     fields 0x10..0x38, vtable slot 6 = the 12-byte zero-init at 0x15c2c0.
// Names are placeholders; only offsets + emitted bytes are load-bearing.
//
// The cluster keeps the discovery-time "CDDrawSubMgr" class name purely for the
// stub bookkeeping; the byte-match depends only on the this/ecx field offsets and
// the (reloc-masked) external call targets, NOT on the C++ class identity.
// ---------------------------------------------------------------------------

// The polymorphic surface-pair held at manager+0x10/+0x14/+0x18: vtable at +0x00
// (slot 0x14 = "ready?"; slot 0x34 = a 1-arg op), a DDBLTFX-ish scratch RECT at
// +0x1c, and the held CDDSurface wrapper at +0x2c.
class CDDrawSurfacePair {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual i32 Vfunc14(); // slot 5  (@0x14): surface-ready predicate
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual i32 Vfunc28(i32 w, i32 h, i32 bpp); // slot 10 (@0x28): geometry setter
    virtual void v2c();
    virtual i32 Vfunc30(i32 w, i32 h, i32 bpp, i32 a4); // slot 12 (@0x30)
    virtual i32 Vfunc34(i32 a);                         // slot 13 (@0x34): 1-arg op

    // Non-virtual surface-state predicates (reloc-masked __thiscall callees).
    i32 Probe_164660(); // 0x164660
    i32 Probe_163f00(); // 0x163f00

    char m_pad04[0x10 - 0x04];  // +0x04 .. +0x0f
    i32 m_geom10;               // +0x10  width  (compared in 0x158bf0)
    i32 m_geom14;               // +0x14  height
    i32 m_geom18;               // +0x18  bpp
    char m_rect1c[0x2c - 0x1c]; // +0x1c  scratch RECT passed to BltFast
    CDDSurface* m_surface;      // +0x2c
};

// A geometry source read by the node's +0x2c dispatch (w @0x14, h @0x18).
class CDDrawWorkerGeom {
public:
    char m_pad00[0x14]; // +0x00 .. +0x13
    i32 m_width;        // +0x14  (1st dispatch arg)
    i32 m_height;       // +0x18  (2nd dispatch arg)
};

// A polymorphic dispatcher held at the node's +0x08: vtable slot 0x2c is a
// 2-arg op called by Method_158b90.
class CDDrawWorkerDisp {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void Vfunc2C(i32 a, i32 b); // slot 11 (@0x2c)
};

// The worker held at manager+0x0c: a geometry source @0x04, a dispatcher @0x08,
// and a flag word @0x34 (bit1 tested).
class CDDrawWorkerNode {
public:
    char m_pad00[0x04];       // +0x00 .. +0x03
    CDDrawWorkerGeom* m_geom; // +0x04
    CDDrawWorkerDisp* m_disp; // +0x08
    char m_pad0c[0x34 - 0x0c];
    i32 m_flags; // +0x34 flag word (bit1 tested)
};

// The worker manager (this for the 0x158xxx methods) is now the single-source
// CDDrawWorkerMgr from <Gruntz/CDDrawWorkerMgr.h> (included above); its owned
// CDDrawWorkerNode / CDDrawSurfacePair members are the local defs above.

// The small per-frame blit-param/element struct (this for the 0x15c2xx methods).
// Field +0x0c on the arg points at a worker-node-like object; +0x10 is a count,
// +0x0c -> [0] -> [+0x1c]; +0x20 is a float.
class CDDrawBlitParamSrc {
public:
    char m_pad00[0x0c]; // +0x00 .. +0x0b
    void* m_elements;   // +0x0c -> worker node (read as [+0x34] flag, or [0]->[+0x1c])
    i32 m_count;        // +0x10 count
    char m_pad14[0x20 - 0x14];
    float m_scale; // +0x20
};

// __thiscall archive/file sink (vtable slot +0x30 = Write(buf, len)).  Shared by
// CWwdObjMgr::ForEachSerialize and CDDrawBlitParam::Serialize.
class CWwdArchive {
public:
    virtual void Av00();
    virtual void Av04();
    virtual void Av08();
    virtual void Av0C();
    virtual void Av10();
    virtual void Av14();
    virtual void Av18();
    virtual void Av1C();
    virtual void Av20();
    virtual void Av24();
    virtual void Av28();
    virtual void Read(void* buf, i32 len);        // +0x2c (the read/load direction)
    virtual void Write(const void* buf, i32 len); // +0x30 (the write/store direction)
};

// The worker held at CDDrawBlitParam+0x0c: holds a sub-object at +0x2c whose
// 0x152d30 method returns a CString (the label written by Serialize).
class CDDrawBlitLabelSource;
class CDDrawBlitWorker {
public:
    char m_pad00[0x2c];                   // +0x00..0x2b
    CDDrawBlitLabelSource* m_labelSource; // +0x2c sub-object (Method_152d30 -> CString)
};
// The label map embedded at +0x10 in the worker sub-object: Lookup(key, &out)
// resolves a worker-label string to its worker pointer.  0x1b8438, reloc-masked.
class CBlitLabelMap {
public:
    i32 Lookup(const char* key, void** out); // 0x1b8438
};
class CDDrawBlitLabelSource {
public:
    CString GetLabel_152d30(CDDrawBlitParamSrc* a);
    char m_pad00[0x10];       // +0x00..+0x0f
    CBlitLabelMap m_labelMap; // +0x10 label -> worker map
};

// CDDrawBlitParam is the single-source shared class in <Gruntz/CDDrawBlitParam.h>
// (included at the top of this TU); its deps CDDrawBlitParamSrc / CWwdArchive /
// CDDrawBlitWorker are fully defined above.

// ---------------------------------------------------------------------------
// CDDrawSubMgr::CDDrawSubMgr
// Chains the Hogwarts(int) base ctor (inlined: this+0x04 = a2), stamps
// the CDDrawSubMgr vtable (compiler-generated), then seeds the remaining fields.
// ---------------------------------------------------------------------------
RVA(0x00156cb0, 0x20)
CDDrawSubMgr::CDDrawSubMgr(CDDrawSurfaceMgr* pSurfaceMgr, i32 a2, i32 a3) : CDDrawSubMgrBase(a2) {
    m_field08 = a3;
    m_pSurfaceMgr = pSurfaceMgr;
}

// NOTE: the retail member-teardown dtor at 0x1574d0 is ~CDDrawSubMgrPages (it stamps
// the DERIVED vtable 0x5efe08 = ??_7CDDrawSubMgrDraco and calls DestroyChildren 0x158ac0,
// NOT this ctor-class's OnDestroy) - see CDDrawSubMgrPages.cpp. The former single
// "CDDrawSubMgr" conflated the CLoadable base arg-ctor (0x156cb0, above) with that
// derived dtor; the split re-homes each to its real class.

// Out-of-line stubs for unmatched virtuals (anchors the vtable in this TU).
void CDDrawSubMgr::IsReady() {}
i32 CDDrawSubMgr::Init() {
    return 0;
}

// ---------------------------------------------------------------------------
// Constant state predicate returning 1.
RVA(0x00157790, 0x6)
StateId CDDrawSubMgr::GetStateId() {
    return STATE_SUBMGR; // 1
}

// ---------------------------------------------------------------------------
// Constant state predicate returning 1 (0x1576c0).
RVA(0x001576c0, 0x6)
i32 CDDrawSubMgr::OnDestroy() {
    return 1;
}

// ---------------------------------------------------------------------------
// 0x155720: scalar-deleting destructor of a far sibling class (real member-teardown
// ~ at 0xd5d70) that landed in this TU. Runs the real ~ then operator delete.
// @early-stop
// reloc-masked cross-module dtor name: the ~ target (0xd5d70) is a distinct
// engine class in another module; its label won't match the ??1CDDrawSubMgrFar
// reference. The scalar-dtor code bytes (call ~ / test flag / operator delete) match.
RVA(0x00155720, 0x1e)
void* CDDrawSubMgr::Stub_155720(i32 flag) {
    ((CDDrawSubMgrFar*)this)->CDDrawSubMgrFar::~CDDrawSubMgrFar();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x157630: member-teardown ~ of the 3-map sibling CDDrawChildGroupDtorHost (vtable 0x5efdc0).
// Runs the cleanup helper (0x1591e0), then the three CMapStringToOb members and the
// FamilyMapBase grand-base auto-destruct (reverse decl order + field resets + the
// grand-base ??_7 re-stamp masking 0x5e8cb4). /GX member-teardown frame.
// @early-stop
// vptr-position wall (~95%, family twin): every instruction matches retail except the
// grand-base vptr re-stamp position (cl stamps at base-dtor entry, retail sinks it
// after the field resets) + the reloc-masked EH-state/teardown/map-dtor names.
RVA(0x00157630, 0x82)
CDDrawChildGroupDtorHost::~CDDrawChildGroupDtorHost() {
    Cleanup_1591e0();
    // implicit: ~m_48, ~m_2c, ~m_10, ~FamilyMapBase (resets + base restamp).
}

// ---------------------------------------------------------------------------
// 0x156e10: member-teardown ~ of the 1-map sibling CDDrawRegistryDtorHost (vtable
// 0x5efd28). Runs the cleanup helper (0x154ac0), then the CMapStringToOb member and
// the FamilyMapBase grand-base auto-destruct. /GX member-teardown frame.
// @early-stop
// vptr-position wall (~95%, family twin): grand-base vptr re-stamp position + the
// reloc-masked EH-state/teardown/map-dtor names are the residual.
RVA(0x00156e10, 0x68)
CDDrawRegistryDtorHost::~CDDrawRegistryDtorHost() {
    Cleanup_154ac0();
    // implicit: ~m_10, ~FamilyMapBase (resets + base restamp).
}

// ===========================================================================
// Discovered cluster (CDDrawWorkerMgr / CDDrawBlitParam) — banked methods.
// Defined in ascending retail-RVA order; Blt/Flip/BltFast are reloc-masked
// __thiscall engine wrappers on the +0x2c surface proxy.
// ===========================================================================

// 0x158b40: pick m_overlayPair (arg2==2) or m_backPair, null-check, dispatch slot 0x34 with arg1.
RVA(0x00158b40, 0x2c)
i32 CDDrawWorkerMgr::Method_158b40(i32 arg1, i32 arg2) {
    CDDrawSurfacePair* p;
    if (arg2 == 2) {
        p = m_overlayPair;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backPair;
        if (!p) {
            return 0;
        }
    }
    return p->Vfunc34(arg1);
}

// 0x158b90: flip m_frontPair's surface, then dispatch the node's +0x08 op with the
// node's +0x04 geometry (w,h).
RVA(0x00158b90, 0x28)
void CDDrawWorkerMgr::Method_158b90() {
    m_frontPair->m_surface->Flip(0);
    CDDrawWorkerNode* n = m_worker;
    CDDrawWorkerDisp* c = n->m_disp;
    CDDrawWorkerGeom* s = n->m_geom;
    c->Vfunc2C(s->m_width, s->m_height);
}

// 0x158bc0: ready predicate over m_frontPair (Probe_164660) and m_overlayPair (Probe_163f00).
RVA(0x00158bc0, 0x2e)
i32 CDDrawWorkerMgr::Method_158bc0() {
    if (m_frontPair && !m_frontPair->Probe_164660()) {
        return 0;
    }
    if (m_overlayPair && !m_overlayPair->Probe_163f00()) {
        return 0;
    }
    return 1;
}

// 0x158bf0: if m_frontPair's cached geometry already == (a1,a2,a3) return 1; else set
// geometry on m_frontPair, m_backPair, and (if ready) m_overlayPair, returning 0 on any failure.
RVA(0x00158bf0, 0x7f)
i32 CDDrawWorkerMgr::Method_158bf0(i32 a1, i32 a2, i32 a3) {
    CDDrawSurfacePair* p = m_frontPair;
    if (p->m_geom10 != a1 || p->m_geom14 != a2 || p->m_geom18 != a3) {
        if (!m_frontPair->Vfunc28(a1, a2, a3)) {
            return 0;
        }
        if (!m_backPair->Vfunc28(a1, a2, a3)) {
            return 0;
        }
        if (m_overlayPair && m_overlayPair->Vfunc14()) {
            if (!m_overlayPair->Vfunc28(a1, a2, a3)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x158cb0: if m_overlayPair is ready, bail; else copy m_backPair's geometry into m_overlayPair via slot
// 0x30 and (if a2) BltFast m_backPair's surface into m_overlayPair's.
RVA(0x00158cb0, 0x6a)
i32 CDDrawWorkerMgr::Method_158cb0(i32 a1, i32 a2) {
    if (m_overlayPair->Vfunc14()) {
        return 0;
    }
    CDDrawSurfacePair* s14 = m_backPair;
    if (!m_overlayPair->Vfunc30(s14->m_geom10, s14->m_geom14, s14->m_geom18, a2)) {
        return 0;
    }
    if (a1) {
        m_overlayPair->m_surface->BltFast(0, 0, m_backPair->m_surface, m_backPair->m_rect1c, 0x10);
    }
    return 1;
}

// 0x158d50: fill m_backPair's surface and flip m_frontPair's, twice unconditionally, then once
// more if the node's +0x34 flag bit1 is set.
RVA(0x00158d50, 0x61)
void CDDrawWorkerMgr::Method_158d50(i32 a1) {
    m_backPair->m_surface->Fill(a1);
    m_frontPair->m_surface->Flip(0);
    m_backPair->m_surface->Fill(a1);
    m_frontPair->m_surface->Flip(0);
    if (m_worker->m_flags & 2) {
        m_backPair->m_surface->Fill(a1);
        m_frontPair->m_surface->Flip(0);
    }
}

// 0x158c70: blt dst's surface <- m_frontPair's surface; return (hr == 0).
RVA(0x00158c70, 0x36)
i32 CDDrawWorkerMgr::Method_158c70(CDDrawSurfacePair* dst) {
    if (!m_frontPair) {
        return 0;
    }
    CDDSurface* s = m_frontPair->m_surface;
    if (!s) {
        return 0;
    }
    CDDSurface* d = dst->m_surface;
    if (!d) {
        return 0;
    }
    i32 hr = d->Blt(s);
    return hr == 0;
}

// 0x158d20: return m_overlayPair->Vfunc14() != 0.
RVA(0x00158d20, 0x16)
i32 CDDrawWorkerMgr::Method_158d20() {
    if (!m_overlayPair) {
        return 0;
    }
    return m_overlayPair->Vfunc14() != 0;
}

// 0x158dc0: blt m_backPair's surface <- m_frontPair's surface; if the m_worker flag bit1 is set,
// flip m_frontPair and re-blt.  The first-block boolean (ok) is the carried return value.
// @early-stop
// 71% — logic + offsets exact; residual is branch-layout/scheduling of the
// first-block (Blt-fall-through vs our jmp) and the esi-pop placement, a
// regalloc/scheduling wall (see docs/patterns/zero-register-pinning.md).
RVA(0x00158dc0, 0x7d)
i32 CDDrawWorkerMgr::Method_158dc0() {
    CDDrawSurfacePair* p10 = m_frontPair;
    CDDrawSurfacePair* p14 = m_backPair;
    i32 ok;
    if (p10 && p10->m_surface) {
        CDDSurface* s10 = p10->m_surface;
        CDDSurface* s14 = p14->m_surface;
        if (s14) {
            i32 hr = s14->Blt(s10);
            ok = (hr == 0);
        } else {
            ok = 0;
        }
    } else {
        ok = 0;
    }
    if (!ok) {
        return ok;
    }
    if (!(m_worker->m_flags & 2)) {
        return ok;
    }
    m_frontPair->m_surface->Flip(0);
    CDDrawSurfacePair* a = m_backPair;
    CDDrawSurfacePair* b = m_frontPair;
    if (!b) {
        return 0;
    }
    CDDSurface* bs = b->m_surface;
    if (!bs) {
        return 0;
    }
    if (!a->m_surface) {
        return 0;
    }
    i32 hr2 = bs->Blt(a->m_surface);
    return hr2 == 0;
}

// 0x158e40: if m_overlayPair->Vfunc14(): blt m_overlayPair's surface <- m_frontPair's surface, return (==0).
// @early-stop
// 50% — structure/offsets byte-exact; the only residual is the `pop esi`
// scheduling (retail interleaves it before the test/sete; our cl emits it in
// the epilogue), a scheduling coin-flip (docs/patterns/zero-register-pinning.md).
RVA(0x00158e40, 0x4c)
i32 CDDrawWorkerMgr::Method_158e40() {
    if (m_overlayPair && m_overlayPair->Vfunc14()) {
        CDDrawSurfacePair* a = m_overlayPair;
        CDDrawSurfacePair* b = m_frontPair;
        if (!b) {
            return 0;
        }
        CDDSurface* bs = b->m_surface;
        if (!bs) {
            return 0;
        }
        CDDSurface* as = a->m_surface;
        if (!as) {
            return 0;
        }
        i32 hr = as->Blt(bs);
        return hr == 0;
    }
    return 0;
}

// 0x158e90: if m_backPair and m_overlayPair->Vfunc14(): BltFast(0,0,m_backPair surf, &m_backPair[+0x1c], 0x10).
RVA(0x00158e90, 0x47)
i32 CDDrawWorkerMgr::Method_158e90() {
    if (!m_backPair) {
        return 0;
    }
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->Vfunc14()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_backPair;
    CDDrawSurfacePair* b = m_overlayPair;
    b->m_surface->BltFast(0, 0, a->m_surface, a->m_rect1c, 0x10);
    return 1;
}

// 0x158ee0: if m_backPair, m_overlayPair and m_overlayPair->Vfunc14(): BltFast(0,0,m_overlayPair surf,&m_overlayPair[+0x1c],0x10).
RVA(0x00158ee0, 0x47)
i32 CDDrawWorkerMgr::Method_158ee0() {
    if (!m_backPair) {
        return 0;
    }
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->Vfunc14()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_overlayPair;
    CDDrawSurfacePair* b = m_backPair;
    b->m_surface->BltFast(0, 0, a->m_surface, a->m_rect1c, 0x10);
    return 1;
}

// 0x159ef0: tail-call this vtable slot 0x3c.
RVA(0x00159ef0, 0x5)
void CDDrawWorkerMgr::Method_159ef0() {
    this->Vfunc3c();
}

// 0x15c290: blit-param init.
// @early-stop
// 94.75% — structure/offsets/stores byte-exact; retail pins `src` in edx and the
// constant `1` in eax, our cl swaps them (eax<->edx phase shift), a regalloc
// coin-flip with no source lever (docs/patterns/zero-register-pinning.md).
RVA(0x0015c290, 0x2f)
void CDDrawBlitParam::Construct(void* srcv) {
    CDDrawBlitParamSrc* src = (CDDrawBlitParamSrc*)srcv;
    m_10 = (i32)src;
    m_28 = 1;
    m_srcRef = 0;
    m_scale = 1.0f;
    m_24 = 1;
    m_2c = *(i32*)((char*)src->m_elements + 0x34) & 0x40;
}

// 0x15c2c0: blit-param zero-reset (vtable slot 6 of the element class).
RVA(0x0015c2c0, 0xc)
void CDDrawBlitParam::Reset_15c2c0() {
    m_10 = 0;
    m_srcRef = 0;
    m_element = 0;
}

// 0x15c2d0: blit-param setup from a worker source.
RVA(0x0015c2d0, 0x45)
void CDDrawBlitParam::Setup_15c2d0(CDDrawBlitParamSrc* src) {
    char* e;
    i32 v;
    m_srcRef = src;
    if (!src) {
        return;
    }
    m_index = 0;
    if (src->m_count > 0) {
        e = *(char**)src->m_elements;
    } else {
        e = 0;
    }
    m_element = e;
    m_20 = 0;
    m_28 = 0;
    v = *(i32*)(e + 0x1c);
    m_30 = v;
    m_34 = v;
    {
        float f = src->m_scale;
        m_scale = f;
    }
}

extern i32 g_aniCueItem; // 0x61ab24 (DATA-annotated forward decl below)

// 0x157a80: pick the active cue object from m_worker->+0x20.  When `force` is null,
// require the cue present and its +0x78 set; cache it at m_2c (m_30 = present?
// 0 : 1), tag the global cue, and report success.  __thiscall, 1 arg (ret 0x4).
RVA(0x00157a80, 0x51)
i32 CDDrawBlitParam::SelectCue_157a80(void* force) {
    char* mgr = (char*)m_worker;
    if (mgr == 0) {
        return 0;
    }
    char* cue = *(char**)(mgr + 0x20);
    if (force == 0) {
        if (cue == 0) {
            return 0;
        }
        if (*(i32*)(cue + 0x78) == 0) {
            return 0;
        }
    }
    if (cue == 0) {
        m_30 = 1;
    } else {
        m_30 = 0;
    }
    m_2c = (i32)cue;
    g_aniCueItem = 0x64;
    return 1;
}

// 0x15c320: recompute the blit-param from the already-stored m_srcRef source (the
// Setup twin that keeps m_srcRef, fixes the scale to 1.0f, and only clears m_20 when
// `a1` is set).  Reads e = src->m_count > 0 ? *src->m_elements : 0, then v = *(e+0x1c).
RVA(0x0015c320, 0x40)
void CDDrawBlitParam::Recompute_15c320(i32 a1) {
    CDDrawBlitParamSrc* src = m_srcRef;
    if (src == 0) {
        return;
    }
    m_index = 0;
    char* e;
    if (src->m_count > 0) {
        e = *(char**)src->m_elements;
    } else {
        e = 0;
    }
    m_element = e;
    m_28 = 0;
    i32 v = *(i32*)(e + 0x1c);
    m_scale = 1.0f;
    m_30 = v;
    m_34 = v;
    if (a1 != 0) {
        m_20 = 0;
    }
}

// ---------------------------------------------------------------------------
// 0x15c970: serialize the blit-param.  Writes the eight dwords m_index..m_scale to
// the archive (4 bytes each via slot +0x30), zeroes a 0x80-byte label buffer,
// and if m_srcRef is set, fetches the worker label (a returns-by-value CString from
// the +0x2c sub-object's 0x152d30) and strcpy's it into the buffer; then writes
// the whole 0x80-byte buffer.  Returns 1.
// @early-stop
// 99.4% — the eight Writes + the buffer zero + the GetLabel call + the inline
// strcpy all byte-exact; the only residual is the NRVO-temp addressing of the
// returned CString: retail derefs the return pointer (`mov edi,[eax]`, 2 B), our
// cl re-reads the temp slot (`mov edi,[esp+0xc]`, 4 B).  Entropy tail / NRVO
// addressing choice; no source lever.
RVA(0x0015c970, 0xfe)
i32 CDDrawBlitParam::Serialize_15c970(CWwdArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_index, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_28, 4);
    ar->Write(&m_2c, 4);
    ar->Write(&m_30, 4);
    ar->Write(&m_34, 4);
    ar->Write(&m_scale, 4);
    char buf[0x80];
    for (i32 i = 0; i < 0x20; ++i) {
        ((i32*)buf)[i] = 0;
    }
    if (m_srcRef != 0) {
        CString label = m_worker->m_labelSource->GetLabel_152d30(m_srcRef);
        strcpy(buf, label);
    }
    ar->Write(buf, 0x80);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15c900: dispatch on `type` — type 4 serializes, type 7 deserializes (both
// via the named sibling), every other type is a no-op that returns 1.  A
// serialize/deserialize that returns 0 propagates the 0.  The (type-3) switch
// over [0,5] builds the 6-entry jump table.
// @early-stop
// 80% — logic exact (both sibling calls + the propagate-0 + return-1 paths
// match).  Residual is the switch lowering: retail emits a `.rdata` jump table
// (the 4 default-folding cases keep the range dense), but MSVC5 folds our empty
// cases into a cmp/je-subtract chain.  Not source-steerable — the lowering
// follows case-value density.  docs/patterns/switch-cmpje-tree-vs-jumptable.md.
RVA(0x0015c900, 0x42)
i32 CDDrawBlitParam::Find(CWwdArchive* ar, i32 type, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    switch (type) {
        case 3:
            break;
        case 4:
            if (Serialize_15c970(ar) == 0) {
                return 0;
            }
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            if (Deserialize_15ca70(ar) == 0) {
                return 0;
            }
            break;
        case 8:
            break;
        default:
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15ca70: deserialize the blit-param (the Serialize_15c970 twin).  Reads the
// eight dwords m_index..m_scale from the archive (4 bytes each via slot +0x2c), reads
// the 0x80-byte label buffer, and — when the label is non-empty — looks it up in
// the worker sub-object's +0x10 map to recover the worker into m_srcRef.  Then the
// Setup_15c2d0-style tail: pick element [m_index] (falling back to element 0 with
// m_index=0) into m_element and, when valid, reset m_20/m_28, snapshot m_30 into m_34,
// and recompute m_30 = elem->+0x1c.  __thiscall, ret 0x4.
// @early-stop
// 89.9% — every instruction/CFG/offset present and the logic is byte-faithful;
// residual is a register-allocation cascade seeded at the first field read:
// retail keeps &m_30 in callee-saved ebp across the function (frame 0x90, no
// spill slot), our cl spills &m_30 to [esp+0x20] (frame 0x94) and rotates
// eax/ebp through the eight reads + the index tail.  Pinning &m_30 in a named
// local regressed to 88.4%; not source-steerable.  docs/patterns/pin-local-for-
// callee-saved-reg.md / zero-register-pinning.md.
RVA(0x0015ca70, 0x15b)
i32 CDDrawBlitParam::Deserialize_15ca70(CWwdArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_index, 4);
    ar->Read(&m_20, 4);
    ar->Read(&m_24, 4);
    ar->Read(&m_28, 4);
    ar->Read(&m_2c, 4);
    ar->Read(&m_30, 4);
    ar->Read(&m_34, 4);
    ar->Read(&m_scale, 4);
    char buf[0x80];
    ar->Read(buf, 0x80);
    if (strlen(buf) == 0) {
        m_srcRef = 0;
    } else {
        void* out = 0;
        m_worker->m_labelSource->m_labelMap.Lookup(buf, &out);
        m_srcRef = (CDDrawBlitParamSrc*)out;
    }
    CDDrawBlitParamSrc* w = m_srcRef;
    if (w != 0) {
        char* e;
        if (m_index >= 0 && m_index < w->m_count) {
            e = ((char**)w->m_elements)[m_index];
        } else {
            e = 0;
        }
        m_element = e;
        if (e == 0) {
            m_index = 0;
            if (w->m_count > 0) {
                e = *(char**)w->m_elements;
            } else {
                e = 0;
            }
            m_element = e;
        }
        if (m_element != 0) {
            m_20 = 0;
            m_28 = 0;
            m_34 = m_30;
            m_30 = *(i32*)(m_element + 0x1c);
        }
    }
    return 1;
}

// ===========================================================================
// CWwdObjMgr — the 0x159xxx-0x15bxxx collection class.  A DISTINCT class from
// CDDrawWorkerMgr above: its `this` layout is a CPtrList at +0x10, two
// CMapPtrToPtr at +0x2c and +0x48, and field +0x0c (a parent handle copied
// into spawned objects).  +0x54 is the SECOND map's m_nCount (CMapPtrToPtr's
// count sits 0xc into the +0x48 map), so the `(*(p+0x54) != 0)?-1:0` start-
// position trick the iterators use is just `m_48.GetCount() != 0`.
//
// The managed objects (CWwdObject, the 0x1dc-byte g_wwdObjVtbl factory output
// of Method_159600) carry: a flag word at +0x08, a sort key at +0x74, a
// CPtrList POSITION cache at +0x78, and the map key at +0x188.  Modeled
// polymorphically only so the vtable dispatches lower to the exact
// `mov eax,[obj]; call [eax+slot]`; virtuals are never defined here.
// Field names are placeholders; only offsets + emitted bytes are load-bearing.
// ===========================================================================

class CWwdObject {
public:
    virtual void Slot00();
    virtual i32 ScalarDtor(i32 flag); // +0x04 scalar-deleting destructor
    virtual void Slot08();
    virtual void Slot0C();
    virtual i32 Slot10(void* a); // +0x10 (1-arg op, called from 159600)
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual i32 Slot20(); // status probe (5 == matched)
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30(); // +0x30 archive write
    virtual void Slot34();
    virtual void Slot38();
    virtual i32 Slot3C(i32 a1, i32 a2, i32 a3, void* obj); // +0x3c
    char m_pad04[0x08 - 0x04];
    i32 m_flags;               // +0x08 flag word
    char m_pad0c[0x74 - 0x0c]; // +0x0c..0x73
    i32 m_sortKey;             // +0x74 sort key
    i32 m_posCache;            // +0x78 CPtrList POSITION cache
    CLogicRecord* m_killCue;   // +0x7c kill-cue record (Consume/callback/refcount)
    char m_pad80[0x188 - 0x80];
    void* m_key; // +0x188 map key
};

// A worker held in the +0x2c/+0x48 maps that exposes a __thiscall probe at
// 0x151c00 (called from Method_15acb0).
class CWwdProbeObject : public CWwdObject {
public:
    i32 Probe_151c00(i32 a1, i32 a2);
};

class CWwdGameObject;

// CWwdObjMgr is the shared <Gruntz/CWwdObjMgr.h> class; this TU owns the bodies for
// the list/map ops (RemoveAll/InsertSorted/ForEach*/Prune*/FindBy*) and CreateObject_159600.

// CPtrList CNode shape (pNext@0, pPrev@4, data@8); the list head node is at
// CWwdObjMgr+0x14 (= m_10.m_pNodeHead).
struct CWwdNode {
    CWwdNode* m_next;  // +0x00
    CWwdNode* m_prev;  // +0x04  (CPtrList node pPrev; not walked here)
    CWwdObject* m_obj; // +0x08
};

// ---------------------------------------------------------------------------
// 0x15ab30: drop a list slot + BOTH map entries (the +0x2c primary AND the
// +0x48 active set).  The "remove everywhere" twin of RemoveByPosition_15ab70.
RVA(0x0015ab30, 0x38)
void CWwdObjMgr::RemoveAll_15ab30(i32 pos, CWwdObject* obj) {
    m_10.RemoveAt((POSITION)pos);
    m_2c.RemoveKey(obj->m_key);
    m_48.RemoveKey(obj->m_key);
}

// ---------------------------------------------------------------------------
// 0x15ab70: drop a list slot + its primary-map entry.
RVA(0x0015ab70, 0x27)
void CWwdObjMgr::RemoveByPosition_15ab70(i32 pos, CWwdObject* obj) {
    m_10.RemoveAt((POSITION)pos);
    m_2c.RemoveKey(obj->m_key);
}

// ---------------------------------------------------------------------------
// 0x15aba0: m_48[obj->key] = obj.
RVA(0x0015aba0, 0x1a)
void CWwdObjMgr::AddToMap48_15aba0(CWwdObject* obj) {
    m_48[obj->m_key] = obj;
}

// ---------------------------------------------------------------------------
// 0x15aa90: walk the list; for every object lacking flag 0x200, drop it from
// the list + both maps and destroy it.
// @early-stop
// 95.6% — list-walk twin-copy regalloc wall: retail materializes the cur node
// in two registers (eax+ecx) to free edi for the advance + push ecx for
// RemoveAt; our cl keeps cur in one reg.  Logic/CFG/offsets exact.
// docs/patterns/linked-list-walk-node-eax-rotation.md.
RVA(0x0015aa90, 0x5d)
void CWwdObjMgr::PruneList_15aa90() {
    struct HLayout {
        char _pad[0x14];
        CWwdNode* m_head;
    };
    CWwdNode* node = ((HLayout*)this)->m_head;
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj != 0 && !(obj->m_flags & 0x200)) {
            m_10.RemoveAt((POSITION)cur);
            m_2c.RemoveKey(obj->m_key);
            m_48.RemoveKey(obj->m_key);
            obj->ScalarDtor(1);
        }
    }
}

// ---------------------------------------------------------------------------
// 0x15abc0: count m_48 entries whose object lacks flag 0x4000000.
RVA(0x0015abc0, 0x5e)
i32 CWwdObjMgr::CountActive_15abc0() {
    i32 n = 0;
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                ++n;
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// 0x15ac20: for each active m_48 object, dispatch its +0x3c virtual with the
// three args + the object; always returns 1.  Returns 0 immediately if a1==0.
RVA(0x0015ac20, 0x81)
i32 CWwdObjMgr::ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                val->Slot3C(a1, a2, a3, val);
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15acb0: for each active m_48 object, run its __thiscall probe at 0x151c00
// with (a1, a2); always returns 1.  Returns 0 immediately if a1==0.
RVA(0x0015acb0, 0x76)
i32 CWwdObjMgr::ForEachProbe_15acb0(i32 a1, i32 a2) {
    if (a1 == 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                ((CWwdProbeObject*)val)->Probe_151c00(a1, a2);
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b020: for each active m_48 object, write its key to the archive then run
// its +0x3c virtual; bail to 0 on a 0 result, else 1.  Returns 0 if ar==0.
// @early-stop
// 90.1% — the per-element block (Write + the +0x3c dispatch) is byte-exact; the
// residual is the loop-tail structure: retail emits a bottom-tested loop with
// two distinct `return 1` epilogues (empty-map vs loop-done), our cl hoists the
// body and merges the epilogue.  An optimizer CFG-shape choice; logic exact.
RVA(0x0015b020, 0xc0)
i32 CWwdObjMgr::ForEachSerialize_15b020(CWwdArchive* ar, i32 a2) {
    if (ar == 0) {
        return 0;
    }
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0 && !(val->m_flags & 0x4000000)) {
                void* k = val->m_key;
                ar->Write(&k, 4);
                if (val->Slot3C((i32)ar, 4, a2, val) == 0) {
                    return 0;
                }
            }
        } while (pos != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b0e0: the read/load counterpart of ForEachSerialize - pull `count` object
// keys back through the archive's +0x2c read slot, resolve each in m_48, require
// it present + alive (+0x7c != 0), then run its +0x3c dispatch with (ar, 7, flag,
// obj). Any missing key / object / failed dispatch aborts to 0; all `count`
// succeed -> 1 (an empty count -> 1).
// @early-stop
// retail carries a conditional CString-cleanup latch (a name temp tracked by a
// stack "alive" flag, destroyed under `flag&1`) whose CONSTRUCTION the optimizer
// elided in this instantiation, leaving a never-taken `~CString` cleanup branch
// (docs/patterns/zero-register-pinning + scoring-artifact). The latch isn't
// reproducible from C without re-introducing the (here dead) name build; logic /
// CFG / offsets are exact, the dead cleanup branch is the residual.
RVA(0x0015b0e0, 0xec)
i32 CWwdObjMgr::Deserialize_15b0e0(CWwdArchive* ar, u32 count, i32 flag) {
    if (ar == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        void* key = 0;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        CWwdObject* obj = 0;
        if (!m_48.Lookup(key, (void*&)obj)) {
            obj = 0;
        }
        if (obj == 0) {
            return 0;
        }
        if (*(i32*)((char*)obj + 0x7c) == 0) {
            return 0;
        }
        if (obj->Slot3C((i32)ar, 7, flag, obj) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x15b1d0: for each m_48 object, look its key up in m_2c; if absent, remove it
// from m_48 and destroy it.  Returns the number removed.
// @early-stop
// 94.2% — logic/CFG/offsets exact; residual is the Lookup `found`-slot handling:
// retail reads `found` into a register only on Lookup-success and compares the
// register, our cl re-zeroes the slot and compares memory.  A found-slot regalloc
// coin-flip (docs/patterns/zero-register-pinning.md); no source lever flips it.
RVA(0x0015b1d0, 0x9b)
i32 CWwdObjMgr::PruneOrphans_15b1d0() {
    i32 n = 0;
    POSITION pos = (POSITION)(m_48.GetCount() != 0 ? -1 : 0);
    if (pos != 0) {
        do {
            void* key = 0;
            CWwdObject* val = 0;
            m_48.GetNextAssoc(pos, key, (void*&)val);
            if (val != 0) {
                void* found = 0;
                if (!m_2c.Lookup(val->m_key, found)) {
                    found = 0;
                }
                if (found == 0) {
                    m_48.RemoveKey(val->m_key);
                    if (val != 0) {
                        val->ScalarDtor(1);
                    }
                    ++n;
                }
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// The shared kill-cue clock (advanced once per tick) + its per-frame delta, and
// the cached timeGetTime import.  g_6bf3c0/g_6bf3bc are BSS mirrors used across
// the draw/tick paths; g_pTimeGetTime is the resolved WINMM entry (bound in
// PaletteLerp.cpp).
extern u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650
extern "C" u32 g_killCueClock;        // 0x6bf3c0 kill-cue clock (prev now)
extern "C" u32 g_6bf3bc;              // 0x6bf3bc per-frame delta

// The per-object cue callback fired when a cue expires (obj+0x7c +0x10; __cdecl,
// one arg = the owning object).
typedef void(__cdecl* KillCueFn)(void*);

// ---------------------------------------------------------------------------
// 0x159a70 (vtable slot 9): per-frame kill-cue tick.  Advances the shared kill
// clock (when `advance`), walks the sorted list running each object's cue
// (m_7c->Consume(delta)); an expired cue (Consume==0) either decrements its
// refcount (+0x24) or fires its callback (+0x10).  Objects flagged 0x10000 /
// 0x20000 are queued into two function-local static arrays; a post-pass then
// (0x10000) unlinks+destroys them (unless flag 0x800 => destroy only) and
// (0x20000) clears the flag and re-sorts them back into the list.
// @early-stop
// 93% (reconstructed from a bare stub; logic/CFG/offsets/calls all reproduced).
// Residual is (1) unmatchable reloc NAMES: the function-local static CObArrays +
// their guard are compiler-mangled locals (`_?killQueue@?1??...`) that the
// delinker only knows as DAT_006bf3a8/DAT_006bf390/DAT_006bf388, and the NAFXCW
// helpers (CObArray ctor/SetSize/SetAtGrow/atexit) are unannotated so Ghidra's
// undecorated names differ from cl's mangled ones — all reloc-masked (bytes
// match); (2) a regalloc/encoding coin-flip in the two array-append count loads:
// retail pins the count in EAX (compact `a1` moffs32 form), our cl uses ecx/edx
// (`8b 0d`), a 1-byte-per-load size slip that cascades the tail offsets.
RVA(0x00159a70, 0x200)
void CWwdObjMgr::TickKillCues_159a70(i32 advance) {
    static CObArray killQueue; // 0x6bf3a8  the 0x10000 (destroy) queue
    static CObArray sortQueue; // 0x6bf390  the 0x20000 (re-sort) queue
    killQueue.SetSize(0, -1);
    sortQueue.SetSize(0, -1);

    if (advance != 0) {
        u32 now = g_pTimeGetTime();
        u32 delta = now - g_killCueClock;
        g_killCueClock = now;
        g_6bf3bc = delta;
    }

    struct HLayout {
        char _pad[0x14];
        CWwdNode* m_head;
    };
    CWwdNode* node = ((HLayout*)this)->m_head;
    while (node != 0) {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        CLogicRecord* rec = obj->m_killCue;
        if (rec->Consume((i32)g_6bf3bc) == 0) {
            i32* refc = (i32*)((char*)rec + 0x24);
            if (*refc != 0) {
                --*refc;
            } else {
                ((KillCueFn)rec->m_10)(obj);
            }
        }
        i32 flags = obj->m_flags;
        if (flags & 0x10000) {
            killQueue.Add((CObject*)obj);
        } else if (flags & 0x20000) {
            sortQueue.Add((CObject*)obj);
        }
    }

    i32 i;
    for (i = 0; i < killQueue.GetSize(); i++) {
        CWwdObject* obj = (CWwdObject*)killQueue.GetData()[i];
        if (obj->m_flags & 0x80000) {
            CLogicRecord* rec = obj->m_killCue;
            rec->m_1c = 0x1d;
            ((KillCueFn)rec->m_10)(obj);
        }
        if (obj->m_flags & 0x800) {
            if (obj != 0) {
                obj->ScalarDtor(1);
            }
        } else {
            m_10.RemoveAt((POSITION)obj->m_posCache);
            m_48.RemoveKey(obj->m_key);
            m_2c.RemoveKey(obj->m_key);
            if (obj != 0) {
                obj->ScalarDtor(1);
            }
        }
    }

    for (i = 0; i < sortQueue.GetSize(); i++) {
        CWwdObject* obj = (CWwdObject*)sortQueue.GetData()[i];
        obj->m_flags &= ~0x20000;
        m_10.RemoveAt((POSITION)obj->m_posCache);
        InsertSorted_159e40(obj, 0);
    }
}

// ---------------------------------------------------------------------------
// 0x159e40: register `obj` — if its flag 0x800 is set, clear its POSITION cache
// and bail; otherwise (when addToMaps) record it in both maps, then insert it
// into the sorted list before the first node whose object has a larger sort key
// and lacks flag 0x20000.  The returned POSITION is cached in obj->+0x78.
// @early-stop
// 97.75% — code bytes match retail; residual is the reloc-typing scoring
// artifact on the two CMapPtrToPtr::operator[] calls (REL32 vs cl's DIR32 vs a
// differently-named symbol).  docs/patterns + objdiff-reloc-scoring.
RVA(0x00159e40, 0xaa)
void CWwdObjMgr::InsertSorted_159e40(CWwdObject* obj, i32 addToMaps) {
    if (obj->m_flags & 0x800) {
        obj->m_posCache = 0;
        return;
    }
    if (addToMaps != 0) {
        m_2c[obj->m_key] = obj;
        m_48[obj->m_key] = obj;
    }
    struct HLayout {
        char _pad[0x14];
        CWwdNode* m_head;
    };
    CWwdNode* node = ((HLayout*)this)->m_head;
    i32 key = obj->m_sortKey;
    while (node != 0) {
        CWwdNode* cur = node;
        CWwdObject* data = cur->m_obj;
        node = node->m_next;
        if (data->m_sortKey > key && !(data->m_flags & 0x20000)) {
            obj->m_posCache = (i32)m_10.InsertBefore((POSITION)cur, obj);
            return;
        }
    }
    obj->m_posCache = (i32)m_10.AddTail(obj);
}

// ---------------------------------------------------------------------------
// 0x15a860: scan the sorted list for the first object whose status probe (slot
// +0x20) is 5, whose +0x04 key matches `type`, and whose worker's +0x10 geometry
// matches the requested key's +0x10.  Returns 0 if none.
// @early-stop
// 86% — logic/offsets/CFG byte-exact; the residual is the loop-tail epilogue:
// retail bottom-tests `jne looptop` and falls through to a SEPARATE `xor eax,eax`
// return-0 (distinct from the empty-list return-0), our cl shares one return-0 and
// loops via `je exit; jmp looptop`.  The documented loop-epilogue-merge wall
// (same as CWwdObjMgr::ForEachSerialize_15b020); no source lever splits it.
RVA(0x0015a860, 0x57)
CWwdObject* CWwdObjMgr::FindByWorker_15a860(i32 type, void* key) {
    struct HLayout {
        char _pad[0x14];
        CWwdNode* m_head;
    };
    CWwdNode* node = ((HLayout*)this)->m_head;
    if (node == 0) {
        return 0;
    }
    do {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->Slot20() == 5 && *(i32*)((char*)obj + 0x4) == type) {
            void* worker = *(void**)((char*)obj + 0x7c);
            if (*(i32*)((char*)worker + 0x10) == *(i32*)((char*)key + 0x10)) {
                return obj;
            }
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x15a940: the +0xe8-field twin of FindByWorker_15a860 — match the object's
// +0xe8 field directly against `key` instead of the worker geometry.
// @early-stop
// 85% — same loop-epilogue-merge wall as FindByWorker_15a860 (logic/offsets exact).
RVA(0x0015a940, 0x52)
CWwdObject* CWwdObjMgr::FindByField_15a940(i32 type, void* key) {
    struct HLayout {
        char _pad[0x14];
        CWwdNode* m_head;
    };
    CWwdNode* node = ((HLayout*)this)->m_head;
    if (node == 0) {
        return 0;
    }
    do {
        CWwdNode* cur = node;
        node = node->m_next;
        CWwdObject* obj = cur->m_obj;
        if (obj->Slot20() == 5 && *(i32*)((char*)obj + 0x4) == type
            && *(void**)((char*)obj + 0xe8) == key) {
            return obj;
        }
    } while (node != 0);
    return 0;
}

// ===========================================================================
// 0x031250 — a NON-MEMBER of the cluster (distinct class): a queue-drain probe.
// Walks the singly-linked list at this+0x68, popping each head node; for each
// node it dispatches the node-data object's vtable slot +0x20 and returns that
// data object the first time the probe yields 5.  Empty/exhausted -> 0.
// __thiscall, no args, no EH frame, fully self-contained (rel32 calls only).
// Kept here under a neutral local class purely for stub bookkeeping; the byte
// match depends only on the this/ecx offsets + the (reloc-masked) vtable call.
// ===========================================================================

// A queued node: m_next at +0x00, data object at +0x08.  The data object exposes
// a vtable whose slot +0x20 is a status probe returning an int (5 == "ready").
class CQueueProbeData;
class CQueueProbeNode {
public:
    CQueueProbeNode* m_next; // +0x00
    char m_pad04[0x04];      // +0x04
    CQueueProbeData* m_data; // +0x08 -> probed object
};

class CQueueProbeData {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual i32 Probe20(); // +0x20 status probe
};

class CQueueDrainHost {
public:
    void* Drain_031250();
    char m_pad00[0x68];      // +0x00 .. +0x67
    CQueueProbeNode* m_head; // +0x68 list head
};

// @early-stop
// loop-top member re-read wall — retail re-loads `mov eax,[edi+0x68]` at the loop
// top (a redundant load the call clobbers) and merges the empty/exhausted zero
// epilogues into one xor-first tail; our cl carries the head pointer across the
// back-edge (CSE) and splits the epilogues.  Logic/CFG/offsets exact; the
// back-edge redundant-load survival is an optimizer coin-flip with no source
// lever (docs/patterns/reread-member-view-pointer.md).
RVA(0x00031250, 0x33)
void* CQueueDrainHost::Drain_031250() {
    while (m_head != 0) {
        CQueueProbeNode* head = m_head;
        m_head = head->m_next;
        CQueueProbeData* data = head->m_data;
        if (data->Probe20() == 5) {
            return data;
        }
    }
    return 0;
}

// ===========================================================================
// 0x15c360 — the animation-playback "Advance" cursor (a.k.a. CDDrawSubMgr_15c360
// in the discovery backlog).  A DISTINCT class from everything above: its `this`
// (esi) is a per-instance animation cursor that, given the number of game ticks
// elapsed (the single __thiscall arg), advances the held sprite-render context's
// frame sequence by one step of the current animation descriptor, updates the
// position deltas, fires any per-frame draw/sound trigger, reloads the per-frame
// timer (optionally scaled by a float speed), and selects the NEXT descriptor in
// the playlist via a 10-way loop-mode switch.  Returns the per-frame draw value
// (m_curDraw/m_pendingDraw), consuming it when the cursor owns the buffer (m_ownsBuffer != 0).
//
// Cursor `this` layout (offsets/sizes load-bearing; names are placeholders):
//   +0x10  m_ctx  sprite-render context (the +0x190/+0x194/+0x198 frame cursor,
//                +0x5c/+0x60 screen pos, +0x08 flags, +0x38 anchor, +0x40 byte)
//   +0x14  m_playlist  CObArray* of animation descriptors (the playlist)
//   +0x18  m_desc  current descriptor (a CAniElement-ish: +0x04 byte flags, +0x08
//                step-mode, +0x0c loop-mode word, +0x10 pos-mode, +0x14 param,
//                +0x18 frame-time reload, +0x1c draw-value, +0x20/+0x24 pos
//                deltas, +0x2c random modulus, +0x30 random-trigger table)
//   +0x1c  m_index  playlist index
//   +0x20  m_timer  per-frame timer remaining (ticks)
//   +0x24  m_decEachTick  "decrement-each-tick" flag
//   +0x28  m_paused  paused/done flag
//   +0x2c  m_ownsBuffer  owns-buffer flag (consume the draw value on read)
//   +0x30  m_pendingDraw  pending draw value
//   +0x34  m_curDraw  current draw value
//   +0x38  m_speed  float speed multiplier (raw bits; compared to 1.0f)
// ===========================================================================

// The frame sequence held at the render context +0x194: a frame-pointer array at
// +0x14 indexed by the cursor at +0x190, the valid frame range [+0x64..+0x68], and
// the resolved frame pointer cache at the context's +0x198.  GetFrame (0x15cc30)
// is the bounds-checked fetch; ClampFirst/ClampLast (0x15cc50/0x15cc90) reset the
// context cursor to the range ends.  All reloc-masked __thiscall externals.
class CAniFrameSeq {
public:
    i32 GetFrame_15cc30(i32 n); // 0x15cc30  __thiscall on the +0x194 sequence
    char m_pad00[0x14];         // +0x00..0x13
    void** m_frames;            // +0x14  frame-pointer array
    char m_pad18[0x64 - 0x18];  // +0x18..0x63
    i32 m_lowFrame;             // +0x64  low frame index
    i32 m_highFrame;            // +0x68  high frame index
};

// The sprite-render context the cursor drives (held at cursor+0x10).  ClampFirst/
// ClampLast (0x15cc50/0x15cc90) clamp its +0x190 cursor to the sequence ends and
// re-resolve +0x198; both reloc-masked __thiscall on the context.
class CAniRenderCtx {
public:
    void ClampFirst_15cc50(); // 0x15cc50  __thiscall on the context
    void ClampLast_15cc90();  // 0x15cc90  __thiscall on the context
    char m_pad00[0x08];       // +0x00..0x07
    i32 m_flags;              // +0x08  flags (bit 0x2000000 tested)
    char m_pad0c[0x10 - 0x0c];
    i32 m_posModeX; // +0x10  pos-mode X
    i32 m_posModeY; // +0x14  pos-mode Y
    char m_pad18[0x38 - 0x18];
    i32 m_anchor;              // +0x38  pos anchor (compared to -1)
    char m_pad3c[0x40 - 0x3c]; // +0x3c
    char m_byteFlags;          // +0x40  byte flags (bit 0x2 tested)
    char m_pad41[0x5c - 0x41];
    i32 m_screenX;              // +0x5c  screen X
    i32 m_screenY;              // +0x60  screen Y
    char m_pad64[0x190 - 0x64]; // +0x64..0x18f
    i32 m_frameCursor;          // +0x190  sequence frame cursor
    CAniFrameSeq* m_frameSeq;   // +0x194  active frame sequence
    i32 m_curFrame;             // +0x198  resolved current frame pointer
};

// The animation descriptor (cursor+0x18, a playlist entry).  +0x08 step-mode keys
// the 7-way frame-step switch; +0x0c loop-mode word keys the 10-way next-descriptor
// switch (and is range-checked against the 9 sentinel); +0x2c/+0x30 drive a random
// per-frame trigger.  Rand_15cbe0 is the engine LCG (reloc-masked __thiscall view).
class CAniDesc {
public:
    i32 Rand_15cbe0(); // 0x15cbe0  engine random (reloc-masked)
    char m_pad00[0x04];
    unsigned char
        m_flags; // +0x04  byte flags (bit1 = no-decrement, bit2 = pos-sub, bit3 = trigger-blit, bit8 = anchor)
    char m_pad05[0x08 - 0x05]; // +0x05..0x07
    i32 m_stepMode;            // +0x08  step-mode
    i32 m_loopMode;            // +0x0c  loop-mode word
    i32 m_posMode;             // +0x10  pos-mode
    i32 m_param;               // +0x14  step param
    i32 m_frameTime;           // +0x18  frame-time reload
    i32 m_drawValue;           // +0x1c  draw value
    i32 m_posDX;               // +0x20  pos delta X
    i32 m_posDY;               // +0x24  pos delta Y
    char m_pad28[0x2c - 0x28]; // +0x28
    i32 m_randMod;             // +0x2c  random modulus
    i32* m_randTable;          // +0x30  random-trigger table
};

// The descriptor playlist (cursor+0x14): a CObArray (data at +0x0c, count at +0x10).
// AtChecked_06b270 is the bounds-checked __thiscall fetch (shared with CAniElement);
// some advance cases inline the same bounds-checked index.
class CAniDescArray {
public:
    CAniDesc* AtChecked_06b270(i32 i); // 0x06b270  __thiscall bounds-checked fetch
    char m_pad00[0x0c];                // +0x00..0x0b
    CAniDesc** m_data;                 // +0x0c  data
    i32 m_count;                       // +0x10  count
};

// The sound-cue enable flag + a float pan/volume scale constant.
DATA(0x0021ab20)
extern i32 g_sndEnabled; // 0x61ab20
DATA(0x001eff2c)
extern float g_sndPanScale; // 0x5eff2c

// The +0x10 sound player: a __thiscall play entry that consumes 4 args.
class CAniSoundPlay {
public:
    i32 Play_1360d0(i32 vol, i32 pan, i32 a3, i32 a4); // 0x1360d0
};

// The per-frame draw trigger (the context's +0x5c screen-X is the blit cue arg).
// Overlaid on the cursor: +0x0c context, +0x10 sound player.
class CAniBlitTrigger {
public:
    i32 TriggerBlit_1587f0(
        i32 pos,
        i32 center,
        i32 range1,
        i32 range2
    );                            // 0x1587f0  __thiscall on the cursor
    char m_pad00[0x0c];           // +0x00..0x0b
    void* m_ctx;                  // +0x0c geometry context
    CAniSoundPlay* m_soundPlayer; // +0x10 sound player
};

// The random-trigger cue table entry (a LeafCue: the gated sound-play entry).
DATA(0x0021ab24)
extern i32 g_aniCueItem; // 0x61ab24  the cue-item id played through PlayIfElapsed
class CAniCue {
public:
    i32 PlayIfElapsed_01f940(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1f940  (ret 0x10)
};

class CAniAdvanceCursor {
public:
    i32 Advance_15c360(u32 elapsed); // 0x15c360

    char m_pad00[0x10];        // +0x00..0x0f
    CAniRenderCtx* m_ctx;      // +0x10
    CAniDescArray* m_playlist; // +0x14
    CAniDesc* m_desc;          // +0x18
    i32 m_index;               // +0x1c
    u32 m_timer;               // +0x20
    i32 m_decEachTick;         // +0x24
    i32 m_paused;              // +0x28
    i32 m_ownsBuffer;          // +0x2c
    i32 m_pendingDraw;         // +0x30
    i32 m_curDraw;             // +0x34
    i32 m_speed;               // +0x38  float speed, raw bits
};

// __ftol the (int)double scale-cast lowers to (0x11f570).
extern "C" i32 __ftol();

// ---------------------------------------------------------------------------
// 0x1587f0: per-frame sound-cue trigger.  Defaults the (center,range1,range2)
// triple from the geometry context when non-positive, clamps the signed offset
// (pos-center) to +/-min(range1,range2), scales it to a [-100,100] pan, derives
// the volume (100, or 100*cue*scale when the cue tag != 100), and hands both to
// the +0x10 sound player.  __thiscall, 4 args (ret 0x10).  No-op (0) when sound
// is disabled.
// @early-stop
// 72% — logic/CFG/offsets/stack-arg flow are instruction-for-instruction identical
// to retail; the entire residual is a register-allocation rotation: retail pins
// `this` in a 4th callee-saved register (ebp) and keeps the (center,range1,range2,d)
// quad in ebx/edi/esi/ecx, our cl reuses ebx for `this` and rotates the quad into
// edi/esi/ecx/eax — flipping the ModRM byte of nearly every access.  No source
// lever picks ebp for `this` (docs/patterns/zero-register-pinning.md).
RVA(0x001587f0, 0xf1)
i32 CAniBlitTrigger::TriggerBlit_1587f0(i32 pos, i32 center, i32 range1, i32 range2) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (center <= 0) {
        center = *(i32*)(*(char**)(*(char**)((char*)m_ctx + 0x24) + 0x5c) + 0x84);
    }
    if (range1 <= 0) {
        char* m4 = *(char**)((char*)m_ctx + 0x4);
        range1 = *(i32*)(*(char**)(m4 + 0x10) + 0x10) << 2;
    }
    if (range2 <= 0) {
        char* m4 = *(char**)((char*)m_ctx + 0x4);
        range2 = *(i32*)(*(char**)(m4 + 0x10) + 0x10) / 3;
    }
    i32 d = pos - center;
    i32 pan;
    if (d >= 0) {
        if (d < range1 && d < range2) {
            pan = d;
        } else {
            pan = range1 >= range2 ? range2 : range1;
        }
    } else {
        i32 ad = -d;
        if (ad < range1 && ad < range2) {
            pan = d;
        } else {
            pan = range1 < range2 ? range1 : range2;
            pan = -pan;
        }
    }
    i32 vol = (pan * 100) / range2;
    i32 cue = g_aniCueItem;
    i32 amp = 100;
    i32 vscale;
    if (cue == 100) {
        vscale = amp;
    } else {
        vscale = (i32)(amp * (cue * g_sndPanScale));
    }
    return m_soundPlayer->Play_1360d0(vscale, vol, 0, 0);
}

// ---------------------------------------------------------------------------
// 0x15c360: advance the animation cursor by `elapsed` ticks.  __thiscall, 1 arg
// (ret 4).
// @early-stop
// Zero-register-pinning plateau (1365 B, two jump-table switches): the body is a
// complete, logic-correct reconstruction.  Byte-exact: the entry + timer-decrement
// block, both jump-table switches (the 7-way frame step on m_desc->m_stepMode and the
// 10-way loop-mode on m_desc->m_loopMode — both emit the retail .rdata table AND match its
// physical case-body order, the lever that moved this 54%->72%), the pos-mode
// update, the random blit/sound trigger (incl. the LCG % mod table index), the
// float speed scale (fild/fmul/__ftol), the descriptor-advance variants, and the
// buffer-consuming return tail.  The residual is purely the documented register-
// pinning wall: (1) switch1's increment/step cases (+1 / +param / -param) pin the
// new frame index in a callee-saved ebx as a TWIN copy (`mov ebx,idx; op ebx;
// mov eax,ebx; mov [..],ebx`) where our cl keeps it single-register in eax; (2) the
// back half re-materializes the zero in ecx (`xor ecx,ecx`) vs our reuse of the
// ebp=0 pin, and the pos-mode switch swaps eax<->ecx for the switch value.  Same
// values, same stores, same CFG; no source lever flips the homing.  Deferred to the
// final sweep per the big-function stop rule.  docs/patterns/zero-register-pinning.md
// + linked-list-walk-node-eax-rotation.md (the twin-copy idiom).
RVA(0x0015c360, 0x555)
i32 CAniAdvanceCursor::Advance_15c360(u32 elapsed) {
    if (m_playlist == 0) {
        return -1;
    }

    // --- per-frame timer decrement --------------------------------------------
    if (m_timer > 0) {
        if (m_decEachTick != 0) {
            if (elapsed >= m_timer) {
                m_timer = 0;
                m_curDraw = m_pendingDraw;
            } else {
                m_timer -= elapsed;
                return m_curDraw;
            }
        } else {
            m_timer -= 1;
            return m_curDraw;
        }
    } else {
        m_curDraw = m_pendingDraw;
    }

    if (m_paused == 0) {
        CAniRenderCtx* ctx = m_ctx;
        CAniDesc* d = m_desc;

        // --- step the active frame sequence one step (7-way on d->m_stepMode) --------
        switch (d->m_stepMode - 1) {
            case 0: { // advance + wrap-to-first on overrun
                CAniRenderCtx* c = m_ctx;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor + 1;
                c->m_frameCursor = idx;
                c->m_curFrame = seq->GetFrame_15cc30(idx);
                if (c->m_curFrame == 0) {
                    i32 first = c->m_frameSeq->m_lowFrame;
                    c->m_frameCursor = first;
                    c->m_curFrame = c->m_frameSeq->GetFrame_15cc30(first);
                }
                break;
            }
            case 1: { // wrap-to-last when at first, else step back
                CAniRenderCtx* c = m_ctx;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor;
                if (idx == seq->m_lowFrame) {
                    c->m_frameCursor = seq->m_highFrame;
                } else {
                    c->m_frameCursor = idx - 1;
                }
                c->m_curFrame = seq->GetFrame_15cc30(c->m_frameCursor);
                break;
            }
            case 2: { // jump to an explicit frame (d->m_param)
                CAniRenderCtx* c = m_ctx;
                i32 frame = d->m_param;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                c->m_curFrame = seq->GetFrame_15cc30(frame);
                c->m_frameCursor = frame;
                break;
            }
            case 3: { // reset to first
                CAniRenderCtx* c = m_ctx;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 first = seq->m_lowFrame;
                c->m_frameCursor = first;
                c->m_curFrame = seq->GetFrame_15cc30(first);
                break;
            }
            case 4: { // reset to last
                CAniRenderCtx* c = m_ctx;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 last = seq->m_highFrame;
                c->m_frameCursor = last;
                c->m_curFrame = seq->GetFrame_15cc30(last);
                break;
            }
            case 5: { // advance by d->m_param, clamp-last on overrun
                CAniRenderCtx* c = m_ctx;
                i32 step = d->m_param;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor + step;
                c->m_frameCursor = idx;
                c->m_curFrame = seq->GetFrame_15cc30(idx);
                if (c->m_curFrame == 0) {
                    c->ClampLast_15cc90();
                }
                break;
            }
            case 6: { // retreat by d->m_param, clamp-first on underrun
                CAniRenderCtx* c = m_ctx;
                i32 step = d->m_param;
                CAniFrameSeq* seq = c->m_frameSeq;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_frameCursor - step;
                c->m_frameCursor = idx;
                c->m_curFrame = seq->GetFrame_15cc30(idx);
                if (c->m_curFrame == 0) {
                    c->ClampFirst_15cc50();
                }
                break;
            }
            default:
                break;
        }

        // --- apply the per-frame position delta (3-way on d->m_posMode) ------------
        ctx = m_ctx;
        ctx->m_posModeX = 0;
        ctx->m_posModeY = 0;
        d = m_desc;
        switch (d->m_posMode) {
            case 1:
                m_ctx->m_posModeX = d->m_posDX;
                m_ctx->m_posModeY = d->m_posDY;
                break;
            case 2: {
                CAniRenderCtx* c = m_ctx;
                i32 x = c->m_screenX;
                if (c->m_byteFlags & 0x2) {
                    i32 dy = d->m_posDY;
                    i32 dx = d->m_posDX;
                    c->m_screenX = x - dx;
                    c->m_screenY = c->m_screenY + dy;
                } else {
                    i32 dy = d->m_posDY;
                    i32 dx = d->m_posDX;
                    c->m_screenX = x + dx;
                    c->m_screenY = c->m_screenY + dy;
                }
                break;
            }
            case 3:
                m_ctx->m_screenX = d->m_posDX;
                m_ctx->m_screenY = d->m_posDY;
                break;
            default:
                break;
        }

        // --- per-frame draw/sound trigger -------------------------------------
        CAniRenderCtx* c = m_ctx;
        i32 fire = 1;
        if (!(c->m_flags & 0x2000000) && !(m_desc->m_flags & 0x8)) {
            if (c->m_anchor == -1) {
                fire = 0;
            }
        }
        if (fire) {
            CAniDesc* dd = m_desc;
            if (dd->m_flags & 0x4) {
                i32 cue = c->m_screenX;
                i32* tbl;
                i32 entry;
                if (dd->m_randMod == 0) {
                    entry = 0;
                } else {
                    tbl = dd->m_randTable;
                    entry = tbl[dd->Rand_15cbe0() % dd->m_randMod];
                }
                if (entry != 0) {
                    ((CAniBlitTrigger*)this)->TriggerBlit_1587f0(cue, 0, 0, 0);
                }
            } else {
                i32* tbl;
                i32 entry;
                if (dd->m_randMod == 0) {
                    entry = 0;
                } else {
                    tbl = dd->m_randTable;
                    entry = tbl[dd->Rand_15cbe0() % dd->m_randMod];
                }
                if (entry != 0) {
                    ((CAniCue*)entry)->PlayIfElapsed_01f940(g_aniCueItem, 0, 0, 0);
                }
            }
        }

        // --- reload the per-frame timer (optionally float-scaled) -------------
        CAniDesc* rd = m_desc;
        i32 reload = rd->m_frameTime;
        m_timer = reload;
        m_decEachTick = (~rd->m_flags) & 1;
        if (m_speed != 0x3f800000) {
            m_timer = (i32)((double)(u32)reload * (*(float*)&m_speed));
        }

        // --- select the NEXT descriptor (10-way loop-mode on rd->m_loopMode) --------
        // Cases are ordered to reproduce retail's physical case-body layout
        // (9, 8, 7, 1, 2, 3, 4, 0, 5).  Cases 2/3/4 test a sequence-position
        // predicate and, on a hit, fall into case 0's shared inline loop-restart
        // body (a goto into the single emitted block); the block's locals are
        // declared before the label so no init is bypassed.
        i32 modeWord = rd->m_loopMode;
        CAniDescArray* arr;
        i32 i;
        CAniDesc* nd;
        switch (modeWord & 0xffff) {
            case 9: // pause
                m_paused = 1;
                break;
            case 8: { // reset to the first descriptor and unscaled timing
                if (m_playlist != 0) {
                    m_index = 0;
                    m_desc = m_playlist->AtChecked_06b270(0);
                    m_paused = 0;
                    m_speed = 0x3f800000;
                    m_pendingDraw = m_desc->m_drawValue;
                    m_curDraw = m_desc->m_drawValue;
                }
                break;
            }
            case 7: { // hold on the first two descriptors (m_index = 1 then 0)
                m_index = 1;
                m_desc = m_playlist->AtChecked_06b270(1);
                if (m_desc == 0) {
                    m_index = 0;
                    m_desc = m_playlist->AtChecked_06b270(0);
                }
                if (m_desc != 0) {
                    m_paused = 0;
                    m_timer = 0;
                    m_curDraw = m_pendingDraw;
                    m_pendingDraw = m_desc->m_drawValue;
                }
                break;
            }
            case 1: { // advance only when the cursor's frame reached the descriptor param
                CAniRenderCtx* c2 = m_ctx;
                if (c2->m_frameCursor == m_desc->m_param) {
                    if (modeWord != 9) {
                        CAniDescArray* a = m_playlist;
                        i32 j = m_index + 1;
                        m_index = j;
                        m_desc = a->AtChecked_06b270(j);
                        if (m_desc == 0) {
                            m_index = 0;
                            m_desc = a->AtChecked_06b270(0);
                        }
                        if (m_desc != 0) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_desc->m_drawValue;
                        }
                    }
                }
                break;
            }
            case 2: { // advance only when the cursor reached the seq low frame
                CAniRenderCtx* c2 = m_ctx;
                CAniFrameSeq* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_lowFrame) {
                    goto loop_restart;
                }
                break;
            }
            case 3: { // advance only when the cursor reached the seq high frame
                CAniRenderCtx* c2 = m_ctx;
                CAniFrameSeq* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_highFrame) {
                    goto loop_restart;
                }
                break;
            }
            case 4: { // advance one past the seq low frame
                CAniRenderCtx* c2 = m_ctx;
                CAniFrameSeq* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_lowFrame + 1) {
                    goto loop_restart;
                }
                break;
            }
            case 0: // loop the playlist forward, inline bounds-checked fetch
            loop_restart:
                if (modeWord != 9) {
                    arr = m_playlist;
                    i = m_index + 1;
                    m_index = i;
                    if (i >= 0 && i < arr->m_count) {
                        nd = arr->m_data[i];
                    } else {
                        nd = 0;
                    }
                    m_desc = nd;
                    if (nd == 0) {
                        m_index = 0;
                        m_desc = arr->AtChecked_06b270(0);
                    }
                    if (m_desc != 0) {
                        m_curDraw = m_pendingDraw;
                        m_pendingDraw = m_desc->m_drawValue;
                    }
                }
                break;
            case 5: { // advance only when the cursor reached one before the high frame
                CAniRenderCtx* c2 = m_ctx;
                CAniFrameSeq* seq = c2->m_frameSeq;
                if (c2->m_frameCursor == seq->m_highFrame - 1) {
                    if (modeWord != 9) {
                        CAniDescArray* a = m_playlist;
                        i32 j = m_index + 1;
                        m_index = j;
                        CAniDesc* p;
                        if (j >= 0 && j < a->m_count) {
                            p = a->m_data[j];
                        } else {
                            p = 0;
                        }
                        m_desc = p;
                        if (p == 0) {
                            m_index = 0;
                            i32 cnt = a->m_count;
                            CAniDesc* first;
                            if (cnt > 0) {
                                first = a->m_data[0];
                            } else {
                                first = 0;
                            }
                            m_desc = first;
                        }
                        if (m_desc != 0) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_desc->m_drawValue;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // --- return the per-frame draw value, consuming it when buffer-owned ------
    if (m_ownsBuffer != 0) {
        if (m_timer != 0) {
            i32 r = m_curDraw;
            m_curDraw = 0;
            return r;
        }
        i32 r = m_pendingDraw;
        m_pendingDraw = 0;
        return r;
    }
    if (m_timer != 0) {
        return m_curDraw;
    }
    return m_pendingDraw;
}

// CAniRenderCtx::ClampFirst (0x15cc50) / ClampLast (0x15cc90): clamp the +0x190
// frame cursor to the active sequence's low/high frame and re-resolve +0x198
// through the same bounds-checked fetch GetFrame (0x15cc30) inlines.
// @early-stop
// shrink-wrapped-callee-save-push wall (~62%, docs/patterns/shrink-wrapped-callee-
// save-push.md): retail defers `push esi` past the null guard (no-save fast-path
// return) and emits the esi-pop epilogue inline per exit; cl hoists push esi to the
// prologue and tail-merges the exits. Body byte-exact; not source-steerable.
RVA(0x0015cc50, 0x38)
void CAniRenderCtx::ClampFirst_15cc50() {
    CAniFrameSeq* seq = m_frameSeq;
    if (seq == 0) {
        return;
    }
    i32 n = seq->m_lowFrame;
    m_frameCursor = n;
    if (n >= seq->m_lowFrame && n <= seq->m_highFrame) {
        m_curFrame = (i32)seq->m_frames[n];
    } else {
        m_curFrame = 0;
    }
}

// @early-stop
// shrink-wrapped-callee-save-push wall (~62%); twin of ClampFirst above.
RVA(0x0015cc90, 0x38)
void CAniRenderCtx::ClampLast_15cc90() {
    CAniFrameSeq* seq = m_frameSeq;
    if (seq == 0) {
        return;
    }
    i32 n = seq->m_highFrame;
    m_frameCursor = n;
    if (n >= seq->m_lowFrame && n <= seq->m_highFrame) {
        m_curFrame = (i32)seq->m_frames[n];
    } else {
        m_curFrame = 0;
    }
}

// ===========================================================================
// 0x159600 — CWwdObjMgr::CreateObject (a.k.a. CSpriteFactory::CreateSpriteImpl):
// allocate + construct a 0x1dc-byte CWwdGameObject, register it in the manager
// (InsertSorted_159e40), and (when arg `flags & 0x200000`) kick its worker's
// slot +0x10.  __thiscall, 6 stack args, ret 0x18.  A /GX EH frame guards the
// destructible sub-objects built inside the allocated block.
// ===========================================================================

// Engine heap allocator (operator new / RezAlloc).  Reloc-masked __cdecl extern.
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
// Engine heap free (RezFree); the worker dtor path.  0x1b9b82.
extern "C" void RezFree(void* p); // 0x1b9b82

// The global object-id counter the factory stamps into +0x188 and post-increments.

// The constructed wide object's first (CWwdGameObject) vtable, then its final
// (g_wwdObjVtbl) vtable.  Reloc-masked DATA externs (RVA = VA - 0x400000).
DATA(0x001f0020)
extern void* g_wwdGameObjectVtbl; // 0x5f0020
DATA(0x001f00a8)
extern void* g_wwdObjFinalVtbl; // 0x5f00a8

// Sub-object ctors hung off the wide object (all __thiscall, reloc-masked).
class CWwdSubCtorA { // 0x15b2b0
public:
    void Ctor();
};
class CWwdSubCtorB { // 0x15b270
public:
    void Ctor();
};
// CResolveNode 3-arg ctor at 0x15b2c0 (root, a2, a3).
class CWwdResolveBase {
public:
    void Ctor(i32 root, i32 a2, i32 a3); // 0x15b2c0
};
// The 0x17c-byte sprite-animation worker built at +0x7c (AnimWorker, 0x15b300).
// CWwdWorker is the shared <Gruntz/CWwdWorker.h> class (the per-object worker at +0x7c).
// The CString ctor (0x1b9b93) for the +0xdc label.
class CWwdLabel {
public:
    void Ctor(); // 0x1b9b93
};
// The command-dispatch sub-object at +0x1a0 (CmdMap ctor 0x15b730, 3 args).
class CWwdCmdMap {
public:
    void Ctor(i32 a, i32 b, i32 c); // 0x15b730
};

// The wide CWwdGameObject the factory builds.  Documented raw-offset access for
// the 0x1dc-byte object (campaign doctrine: the offsets are load-bearing, the
// field names are placeholders); the polymorphic slot +0x28 / +0x04 dispatch is
// modeled via a typed vtable interface so the call lowers exactly.
class CWwdFactoryObject {
public:
    virtual void Vs00();
    virtual i32 ScalarDtor(i32 flag); // +0x04 scalar-deleting destructor
    virtual void Vs08();
    virtual void Vs0C();
    virtual void Vs10();
    virtual void Vs14();
    virtual void Vs18();
    virtual void Vs1C();
    virtual void Vs20();
    virtual void Vs24();
    virtual i32 Build(i32 a, i32 b, i32 c, i32 d); // +0x28 deserialize/build

    // Reset/clear the wide object: release the four +0x7c..0x90 sub-objects and
    // reset the geometry/status fields.  Documented raw-offset access.
    void Reset_15b980();  // 0x15b980
    void Reset_15bf00();  // 0x15bf00
    void Reload_166810(); // 0x166810 (external base reset, no body)

    // Release the four +0x7c..0x90 sub-objects + re-seed status (the dtor's
    // shared "drop members" helper; two identical instantiations + a +0x18c
    // variant).  Raw-offset access (documented).
    void ReleaseSubs_15b5d0();
    void ReleaseSubs_15bc50();
    void ReleaseSubsClearKey_15c200();
    // 0x15b650: tick/notify — under flag 0x8 decrement the +0x128 budget (latch
    // the +0x7c sub-object's error on underflow); else hand `p` to the +0x80
    // notifier's +0x10 cdecl callback.
    void Notify_15b650(void* p);
};

// The +0x80 notifier: a cdecl callback pointer at +0x10 invoked with the owner.
class CWwdNotifier {
public:
    char m_pad00[0x10];        // +0x00..0x0f
    void (*m_callback)(void*); // +0x10
};

// ---------------------------------------------------------------------------
// CWwdGameObject reset (0x15b980): drop the four sub-objects at +0x7c/+0x80/
// +0x88/+0x90 via their scalar-dtor virtual (slot 1, delete flag), null the
// geometry cache (+0x18c..+0x198), then re-seed the status fields (+0xd8/+0x38 =
// -1, +0xc0/+0x5c/+0x20 = 0x80000000).  __thiscall, ret 0.  Raw-offset access.
RVA(0x0015b980, 0x96)
void CWwdFactoryObject::Reset_15b980() {
    char* o = (char*)this;
    *(i32*)(o + 0x18c) = -1;
    *(i32*)(o + 0x190) = -1;
    *(i32*)(o + 0x198) = 0;
    *(i32*)(o + 0x194) = 0;
    CWwdFactoryObject* s;
    if ((s = *(CWwdFactoryObject**)(o + 0x7c)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x7c) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x80)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x80) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x88)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x88) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x90)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x90) = 0;
    }
    *(i32*)(o + 0xd8) = -1;
    *(i32*)(o + 0xc0) = (i32)0x80000000;
    *(i32*)(o + 0x5c) = (i32)0x80000000;
    *(i32*)(o + 0x20) = (i32)0x80000000;
    *(i32*)(o + 0x38) = -1;
}

// ---------------------------------------------------------------------------
// CWwdGameObject reset (0x15bf00): the deeper-reset twin - first run the base
// reset (Reload_166810), clear +0x1f8, then the identical sub-object teardown +
// status re-seed as Reset_15b980.  __thiscall, ret 0.  Raw-offset access.
RVA(0x0015bf00, 0xa1)
void CWwdFactoryObject::Reset_15bf00() {
    char* o = (char*)this;
    Reload_166810();
    *(i32*)(o + 0x1f8) = 0;
    *(i32*)(o + 0x18c) = -1;
    *(i32*)(o + 0x190) = -1;
    *(i32*)(o + 0x198) = 0;
    *(i32*)(o + 0x194) = 0;
    CWwdFactoryObject* s;
    if ((s = *(CWwdFactoryObject**)(o + 0x7c)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x7c) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x80)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x80) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x88)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x88) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x90)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x90) = 0;
    }
    *(i32*)(o + 0xd8) = -1;
    *(i32*)(o + 0xc0) = (i32)0x80000000;
    *(i32*)(o + 0x5c) = (i32)0x80000000;
    *(i32*)(o + 0x20) = (i32)0x80000000;
    *(i32*)(o + 0x38) = -1;
}

// ---------------------------------------------------------------------------
// 0x15b5d0: drop the four +0x7c/+0x80/+0x88/+0x90 sub-objects (each via its
// scalar-dtor virtual, delete flag) and re-seed the status fields (+0xc0/+0x5c/
// +0x20 = 0x80000000, +0xd8/+0x38 = -1).  No geometry reset.  __thiscall, ret 0.
RVA(0x0015b5d0, 0x7c)
void CWwdFactoryObject::ReleaseSubs_15b5d0() {
    char* o = (char*)this;
    CWwdFactoryObject* s;
    if ((s = *(CWwdFactoryObject**)(o + 0x7c)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x7c) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x80)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x80) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x88)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x88) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x90)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x90) = 0;
    }
    *(i32*)(o + 0xc0) = (i32)0x80000000;
    *(i32*)(o + 0xd8) = -1;
    *(i32*)(o + 0x5c) = (i32)0x80000000;
    *(i32*)(o + 0x20) = (i32)0x80000000;
    *(i32*)(o + 0x38) = -1;
}

// ---------------------------------------------------------------------------
// 0x15bc50: identical instantiation of ReleaseSubs_15b5d0 in a sibling subclass.
RVA(0x0015bc50, 0x7c)
void CWwdFactoryObject::ReleaseSubs_15bc50() {
    char* o = (char*)this;
    CWwdFactoryObject* s;
    if ((s = *(CWwdFactoryObject**)(o + 0x7c)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x7c) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x80)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x80) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x88)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x88) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x90)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x90) = 0;
    }
    *(i32*)(o + 0xc0) = (i32)0x80000000;
    *(i32*)(o + 0xd8) = -1;
    *(i32*)(o + 0x5c) = (i32)0x80000000;
    *(i32*)(o + 0x20) = (i32)0x80000000;
    *(i32*)(o + 0x38) = -1;
}

// ---------------------------------------------------------------------------
// 0x15c200: the +0x18c-clearing twin — clear the byte at +0x18c first, then the
// identical sub-object release + status re-seed.  __thiscall, ret 0.
RVA(0x0015c200, 0x82)
void CWwdFactoryObject::ReleaseSubsClearKey_15c200() {
    char* o = (char*)this;
    *(char*)(o + 0x18c) = 0;
    CWwdFactoryObject* s;
    if ((s = *(CWwdFactoryObject**)(o + 0x7c)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x7c) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x80)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x80) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x88)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x88) = 0;
    }
    if ((s = *(CWwdFactoryObject**)(o + 0x90)) != 0) {
        s->ScalarDtor(1);
        *(i32*)(o + 0x90) = 0;
    }
    *(i32*)(o + 0xc0) = (i32)0x80000000;
    *(i32*)(o + 0xd8) = -1;
    *(i32*)(o + 0x5c) = (i32)0x80000000;
    *(i32*)(o + 0x20) = (i32)0x80000000;
    *(i32*)(o + 0x38) = -1;
}

// ---------------------------------------------------------------------------
// 0x15b650: per-tick notify.  When flag bit 0x8 is set, subtract `p`'s +0x120
// budget from this+0x128 and, if non-positive, latch error 0x1c on the +0x7c
// worker.  Otherwise hand `p` to the +0x80 notifier's +0x10 cdecl callback (with
// the owner), after recording `p` at +0x84.  __thiscall, 1 arg (ret 0x4).
// @early-stop
// 84% — structure/CFG/offsets/stores byte-exact; the residual is two instruction-
// selection coin-flips MSVC5 won't flip from source: the flag test (`movb;testb`
// vs retail `testb mem`) and the budget subtract (mem-operand `sub eax,[p+0x120]`
// vs retail's `mov edx,[p+0x120]` reg-load first).  Entropy-tail / zero-register-
// pinning wall (docs/patterns/zero-register-pinning.md).
RVA(0x0015b650, 0x4d)
void CWwdFactoryObject::Notify_15b650(void* p) {
    char* o = (char*)this;
    if (*(unsigned char*)(o + 0x8) & 0x8) {
        i32 d = *(i32*)(o + 0x128) - *(i32*)((char*)p + 0x120);
        *(i32*)(o + 0x128) = d;
        if (d <= 0) {
            *(i32*)(*(char**)(o + 0x7c) + 0x1c) = 0x1c;
        }
    } else {
        CWwdNotifier* h = *(CWwdNotifier**)(o + 0x80);
        if (h != 0) {
            *(void**)(o + 0x84) = p;
            h->m_callback(this);
        }
    }
}

// ===========================================================================
// Small leaf ctors + a rect-overlap predicate from the same cluster.
// ===========================================================================

// The +0x9c sub-object built by the 0x159250 factory: two zeroed dword fields.
class CWwdSlot9c {
public:
    char m_pad00[0x08]; // +0x00..0x07
    i32 m_08;           // +0x08
    i32 m_0c;           // +0x0c
    CWwdSlot9c();
};

// 0x15b2a0: zero +0x0c then +0x08; returns `this` (ctor).
RVA(0x0015b2a0, 0xb)
CWwdSlot9c::CWwdSlot9c() {
    m_0c = 0;
    m_08 = 0;
}

// The DDraw worker base vtable (stamped last in the wide-object dtors).
// Reloc-masked DATA extern (RVA = VA - 0x400000). The leaf-worker vtable (0x5effa0)
// is now the cl-emitted ??_7CDrawSubWorker (real-polymorphic; VTBL below).
DATA(0x001e8cb4)
extern void* g_wapObjectDtorVtbl; // 0x5e8cb4

// 0x158f30: 3-arg leaf-worker ctor — store the three args at +0x4/+0x8/+0xc,
// stamp the leaf vtable (cl-implicit vptr-first), zero +0x10.  __thiscall, ret 0xc.
// Real-polymorphic now: the single virtual forces cl to emit ??_7CDrawSubWorker +
// auto-stamp the vptr in the ctor prologue (was a manual g_drawSubWorkerVtbl store,
// vptr-middle -> vptr-first regression accepted per the all-vtables mandate).
struct CDrawSubWorker {
    virtual void v0(); // implicit vptr @ +0x00 (own vtable reloc-masks 0x5effa0)
    i32 m_04;          // +0x04
    i32 m_08;          // +0x08
    i32 m_0c;          // +0x0c
    i32 m_10;          // +0x10
    CDrawSubWorker(i32 a1, i32 a2, i32 a3);
};
RVA(0x00158f30, 0x27)
CDrawSubWorker::CDrawSubWorker(i32 a1, i32 a2, i32 a3) {
    m_04 = a2;
    m_08 = a3;
    m_0c = a1;
    m_10 = 0;
}
VTBL(CDrawSubWorker, 0x001effa0); // ??_7CDrawSubWorker (was g_drawSubWorkerVtbl)

// 0x158fb0: DDraw worker base re-init — +0x4 = -1, +0x8/+0xc/+0x10 = 0, stamp
// the base vtable.  A void method (keeps `this` in ecx; not a ctor).  ret 0.
class CDrawSubWorkerBase {
public:
    void* m_vtbl; // +0x00
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c
    i32 m_10;     // +0x10
    void Init_158fb0();
};
RVA(0x00158fb0, 0x19)
void CDrawSubWorkerBase::Init_158fb0() {
    m_04 = -1;
    m_10 = 0;
    m_08 = 0;
    m_0c = 0;
    m_vtbl = &g_wapObjectDtorVtbl;
}

// 0x15bfb0: rect-overlap predicate (RECT a, RECT b): true iff a.left <= b.right,
// a.right >= b.left, a.top <= b.bottom, a.bottom >= b.top.  __stdcall, 2 args.
struct CDDrawRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};
RVA(0x0015bfb0, 0x4a)
i32 __stdcall RectsOverlap_15bfb0(CDDrawRect* a, CDDrawRect* b) {
    if (a->left > b->right) {
        return 0;
    }
    if (a->right < b->left) {
        return 0;
    }
    if (a->top > b->bottom) {
        return 0;
    }
    return a->bottom >= b->top;
}

// 0x15a130: bounding-box overlap test between two wide objects.  Each box is its
// screen pos (+0x5c/+0x60) plus a local AABB (a1: +0x144..+0x150; a2: +0x154..
// +0x160).  Either box invalid (its first AABB field == INT_MIN) -> no overlap.
// __stdcall, 2 args (ret 0x8).
// @early-stop
// 76% — logic/CFG/offsets/compares byte-exact (both INT_MIN early-outs + the four
// edge tests match); the residual is a spill-frame strategy difference: retail
// allocates a fresh `sub esp,0x20` local frame for the 3 spilled box edges, our cl
// reuses the incoming arg stack slots (`push ecx`, no frame) — shifting every spill
// offset + rotating esi/edi.  A non-steerable codegen heuristic (zero-register-
// pinning family); docs/patterns/zero-register-pinning.md.
struct CWwdBox {
    char m_pad00[0x5c];
    i32 m_screenX; // screen X
    i32 m_screenY; // screen Y
    char m_pad64[0x144 - 0x64];
    i32 m_aabb0Left;   // a1 AABB: left
    i32 m_aabb0Top;    // a1 AABB: top
    i32 m_aabb0Right;  // a1 AABB: right
    i32 m_aabb0Bottom; // a1 AABB: bottom
    i32 m_aabb1Left;   // a2 AABB: left
    i32 m_aabb1Top;    // a2 AABB: top
    i32 m_aabb1Right;  // a2 AABB: right
    i32 m_aabb1Bottom; // a2 AABB: bottom
};
RVA(0x0015a130, 0xdc)
i32 __stdcall BoxesOverlap_15a130(CWwdBox* a1, CWwdBox* a2) {
    if (a2->m_aabb1Left == (i32)0x80000000) {
        return 0;
    }
    if (a1->m_aabb0Left == (i32)0x80000000) {
        return 0;
    }
    i32 a1L = a1->m_aabb0Left + a1->m_screenX;
    i32 a1R = a1->m_aabb0Right + a1->m_screenX;
    i32 a1T = a1->m_aabb0Top + a1->m_screenY;
    i32 a1B = a1->m_aabb0Bottom + a1->m_screenY;
    i32 a2L = a2->m_aabb1Left + a2->m_screenX;
    i32 a2T = a2->m_aabb1Top + a2->m_screenY;
    i32 a2B = a2->m_aabb1Bottom + a2->m_screenY;
    i32 a2R = a2->m_aabb1Right + a2->m_screenX;
    if (a1L > a2R) {
        return 0;
    }
    if (a1R < a2L) {
        return 0;
    }
    if (a1T > a2B) {
        return 0;
    }
    return a1B >= a2T;
}

// @early-stop
// RezAlloc + placement-construct EH-frame wall (docs/patterns/rezalloc-placement-
// new-no-eh-frame.md): the body is byte-exact, but MSVC5 predates placement
// operator delete so the in-place sub-object construction emits NO ctor-in-flight
// /GX EH state — retail's full `push -1 / fs:0` frame + shared jmp epilogue is
// absent, shifting every byte offset (objdiff alignment collapses to ~0% fuzzy).
// Deferred to the final sweep when the wide-object ctor + a class-`operator new`
// real-allocator path can emit the retail frame.  Logic/fields/offsets complete.
RVA(0x00159600, 0x1ab)
CWwdGameObject* CWwdObjMgr::CreateObject_159600(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 flags) {
    char* obj = (char*)RezAlloc(0x1dc);
    CWwdGameObject* result;
    if (obj != 0) {
        i32 root = (i32)m_0c;
        ((CWwdResolveBase*)obj)->Ctor(root, a1, flags);
        ((CWwdSubCtorA*)(obj + 0x9c))->Ctor();
        ((CWwdSubCtorB*)(obj + 0xb8))->Ctor();
        ((CWwdLabel*)(obj + 0xdc))->Ctor();
        *(void**)obj = &g_wwdGameObjectVtbl;
        *(i32*)(obj + 0x5c) = (i32)0x80000000;
        *(i32*)(obj + 0x78) = 0;
        char* worker = (char*)RezAlloc(0x17c);
        if (worker != 0) {
            ((CWwdWorker*)worker)->Ctor(root, a1, flags);
        } else {
            worker = 0;
        }
        *(void**)(obj + 0x7c) = worker;
        *(i32*)(obj + 0x98) = 0;
        *(i32*)(obj + 0x80) = 0;
        *(i32*)(obj + 0x88) = 0;
        *(i32*)(obj + 0x90) = 0;
        *(i32*)(obj + 0x188) = g_wwdObjIdCounter;
        g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
        ((CWwdCmdMap*)(obj + 0x1a0))->Ctor(root, a1, flags);
        *(void**)obj = &g_wwdObjFinalVtbl;
        *(i32*)(obj + 0x18c) = -1;
        *(i32*)(obj + 0x190) = -1;
        *(i32*)(obj + 0x198) = 0;
        *(i32*)(obj + 0x194) = 0;
        *(i32*)(obj + 0x19c) = 0;
        result = (CWwdGameObject*)obj;
    } else {
        result = 0;
    }
    if (((CWwdFactoryObject*)result)->Build(a2, a3, a4, a5) == 0) {
        if (result != 0) {
            ((CWwdFactoryObject*)result)->ScalarDtor(1);
        }
        return 0;
    }
    InsertSorted_159e40((CWwdObject*)result, 1);
    if (flags & 0x200000) {
        ((CWwdWorker*)*(void**)((char*)result + 0x7c))->Kick(result);
    }
    return result;
}

SIZE_UNKNOWN(CAniAdvanceCursor);
SIZE_UNKNOWN(CAniBlitTrigger);
SIZE_UNKNOWN(CAniCue);
SIZE_UNKNOWN(CAniDesc);
SIZE_UNKNOWN(CAniDescArray);
SIZE_UNKNOWN(CAniFrameSeq);
SIZE_UNKNOWN(CAniRenderCtx);
SIZE_UNKNOWN(CAniSoundPlay);
SIZE_UNKNOWN(CBlitLabelMap);
SIZE_UNKNOWN(CDDrawBlitLabelSource);
SIZE_UNKNOWN(CDDrawBlitParam);
SIZE_UNKNOWN(CDDrawBlitParamSrc);
SIZE_UNKNOWN(CDDrawBlitWorker);
SIZE_UNKNOWN(CDDrawRect);
SIZE_UNKNOWN(CDDrawSubMgr);
SIZE_UNKNOWN(CDDrawSubMgrBase);
SIZE_UNKNOWN(CDDrawSubMgrFar);
SIZE_UNKNOWN(CDDrawChildGroupDtorHost);
SIZE_UNKNOWN(CDDrawRegistryDtorHost);
SIZE_UNKNOWN(FamilyMapBase);
SIZE_UNKNOWN(CDDrawWorkerDisp);
SIZE_UNKNOWN(CDDrawWorkerGeom);
SIZE_UNKNOWN(CDDrawWorkerNode);
SIZE_UNKNOWN(CQueueDrainHost);
SIZE_UNKNOWN(CQueueProbeData);
SIZE_UNKNOWN(CQueueProbeNode);
SIZE_UNKNOWN(CDrawSubWorker);
SIZE_UNKNOWN(CDrawSubWorkerBase);
SIZE_UNKNOWN(CWwdArchive);
SIZE_UNKNOWN(CWwdBox);
SIZE_UNKNOWN(CWwdCmdMap);
SIZE_UNKNOWN(CWwdFactoryObject);
SIZE_UNKNOWN(CWwdLabel);
SIZE_UNKNOWN(CWwdNode);
SIZE_UNKNOWN(CWwdNotifier);
SIZE_UNKNOWN(CWwdObjMgr);
SIZE_UNKNOWN(CWwdObject);
SIZE_UNKNOWN(CWwdProbeObject);
SIZE_UNKNOWN(CWwdResolveBase);
SIZE_UNKNOWN(CWwdSlot9c);
SIZE_UNKNOWN(CWwdSubCtorA);
SIZE_UNKNOWN(CWwdSubCtorB);
SIZE_UNKNOWN(CWwdWorker);
