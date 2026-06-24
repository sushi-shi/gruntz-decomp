#include <rva.h>
// The discovered cluster's surface helpers (Blt/Flip/BltFast on CDDSurface) come
// from the DDrawMgr group; reuse its real types instead of placeholder casts.
#include <Gruntz/CDirectDrawMgr.h>
// UnknownLucius.cpp - tomalla-named DDraw surface/page-manager shared base
// (CDDrawSubMgr).  This is the polymorphic base for the 10 sub-
// managers (Draco, Hermiona, Hagrid, etc.).  Two functions:
//   ctor  - seeds the three fields + stamps vtable.
//   dtor  - SEH-framed: calls VirtualMethodUnknown1C
//           cleanup, resets fields, chains base dtor.
//
// Field names are tomalla placeholders; only the OFFSETS + the emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// Forward-declare the family manager (root) stored at CGruntzMgr+0x30.
// Full definition lives in HarryPotter.cpp (HarryPotter unit) and in
// src/Stub/types/ddrawmgr_surface_family.h.
class CDDrawSurfaceMgr;

// The Lucius and CObject vtables are used in the dtor vtable chain, emitted
// automatically by the compiler.
class CDDrawSubMgrBase {
public:
    CDDrawSubMgrBase() {}
    CDDrawSubMgrBase(int x) {
        m_fieldBaseUnknown = x;
    }
    virtual ~CDDrawSubMgrBase() {}
    int m_fieldBaseUnknown; // +0x04

    // Engine-label backlog stubs.
    void Constructor_156e10();
};

class CDDrawSubMgr : public CDDrawSubMgrBase {
public:
    CDDrawSubMgr(CDDrawSurfaceMgr* pHarryPotter, int unknown2, int unknown3);
    virtual ~CDDrawSubMgr() OVERRIDE;
    virtual void VirtualMethodUnknown14();
    virtual int VirtualMethodUnknown18();
    virtual void VirtualMethodUnknown1C(); // cleanup — defined in CDDrawSubMgrDraco.cpp
    virtual void VirtualMethodUnknown20();

    // Engine-label backlog stubs.
    void Constructor_157630();
    void Stub_155720();

    int fieldUnknown8;                // +0x08
    CDDrawSurfaceMgr* m_pHarryPotter; // +0x0c
};

// operator delete (used indirectly via VirtualMethodUnknown1C; may throw -> /GX).
void operator delete(void*);

// ---------------------------------------------------------------------------
// The 0x158xxx-0x15c970 discovered cluster is a DISTINCT, LARGER class than the
// small CDDrawSubMgr above (verified: that class's vtable is g_severusWorkerBaseVtbl
// with fields at 0x04/0x08/0x0c; the cluster reads 0x0c/0x10/0x14/0x18/0x2c/0x48/0x54
// and is constructed by Constructor_157630, whose vtable is g_remusBaseDtorVtbl).
// The cluster carries TWO objects:
//   - the worker MANAGER (this == 0x158xxx methods): m_0c worker, m_10/m_14/m_18
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
    virtual int Vfunc14(); // slot 5  (@0x14): surface-ready predicate
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual int Vfunc34(int a); // slot 13 (@0x34): 1-arg op
    char m_pad04[0x1c - 0x04];  // +0x04 .. +0x1b
    char m_rect1c[0x2c - 0x1c]; // +0x1c  scratch RECT passed to BltFast
    CDDSurface* m_surface;      // +0x2c
};

// The worker held at manager+0x0c: a flag byte sits at +0x34.
class CDDrawWorkerNode {
public:
    char m_pad00[0x34]; // +0x00 .. +0x33
    int m_34flagAt;     // +0x34 flag word (bit1 tested)
};

// The worker manager (this for the 0x158xxx methods).  Polymorphic: its own
// vtable holds a slot at +0x3c that Method_159ef0 tail-calls.
class CDDrawWorkerMgr {
public:
    virtual void Vfunc00();
    virtual void Vfunc04();
    virtual void Vfunc08();
    virtual void Vfunc0c();
    virtual void Vfunc10();
    virtual void Vfunc14();
    virtual void Vfunc18();
    virtual void Vfunc1c();
    virtual void Vfunc20();
    virtual void Vfunc24();
    virtual void Vfunc28();
    virtual void Vfunc2c();
    virtual void Vfunc30();
    virtual void Vfunc34();
    virtual void Vfunc38();
    virtual void Vfunc3c(); // slot 15 (@0x3c): Method_159ef0 forwards here

    // Surface ops.
    int Method_158b40(int arg1, int arg2);
    int Method_158c70(CDDrawSurfacePair* dst);
    int Method_158d20();
    int Method_158dc0();
    int Method_158e40();
    int Method_158e90();
    int Method_158ee0();
    void Method_159ef0();

    char m_pad04[0x0c - 0x04]; // +0x04 .. +0x0b
    CDDrawWorkerNode* m_0c;    // +0x0c worker (flag at [+0x34])
    CDDrawSurfacePair* m_10;   // +0x10
    CDDrawSurfacePair* m_14;   // +0x14
    CDDrawSurfacePair* m_18;   // +0x18
};

// The small per-frame blit-param/element struct (this for the 0x15c2xx methods).
// Field +0x0c on the arg points at a worker-node-like object; +0x10 is a count,
// +0x0c -> [0] -> [+0x1c]; +0x20 is a float.
class CDDrawBlitParamSrc {
public:
    char m_pad00[0x0c]; // +0x00 .. +0x0b
    void* m_0c;         // +0x0c -> worker node (read as [+0x34] flag, or [0]->[+0x1c])
    int m_10;           // +0x10 count
    char m_pad14[0x20 - 0x14];
    float m_20; // +0x20
};

class CDDrawBlitParam {
public:
    void Init_15c290(CDDrawBlitParamSrc* src);
    void Reset_15c2c0();
    void Setup_15c2d0(CDDrawBlitParamSrc* src);

    char m_pad00[0x10]; // +0x00 .. +0x0f
    int m_10;           // +0x10
    int m_14;           // +0x14
    int m_18;           // +0x18
    int m_1c;           // +0x1c
    int m_20;           // +0x20
    int m_24;           // +0x24
    int m_28;           // +0x28
    int m_2c;           // +0x2c
    int m_30;           // +0x30
    int m_34;           // +0x34
    float m_38;         // +0x38
};

// ---------------------------------------------------------------------------
// CDDrawSubMgr::CDDrawSubMgr
// Chains the Hogwarts(int) base ctor (inlined: this+0x04 = unknown2), stamps
// the Lucius vtable (compiler-generated), then seeds the remaining fields.
// ---------------------------------------------------------------------------
RVA(0x00156cb0, 0x20)
CDDrawSubMgr::CDDrawSubMgr(CDDrawSurfaceMgr* pHarryPotter, int unknown2, int unknown3)
    : CDDrawSubMgrBase(unknown2) {
    fieldUnknown8 = unknown3;
    m_pHarryPotter = pHarryPotter;
}

// ---------------------------------------------------------------------------
// CDDrawSubMgr::~CDDrawSubMgr
// Scalar-deleting destructor.  Under /GX the compiler emits a C++ EH frame
// (push -1 / handler info / fs:0) around the body because VirtualMethod-
// Unknown1C may throw (it calls operator delete).  After the body runs, the
// compiler changes the vtable to the base (CObject) and chains
// through the base destructors.
// ---------------------------------------------------------------------------
RVA(0x001574d0, 0x5b)
CDDrawSubMgr::~CDDrawSubMgr() {
    VirtualMethodUnknown1C();
    m_fieldBaseUnknown = -1;
    fieldUnknown8 = 0;
    m_pHarryPotter = 0;
}

// Out-of-line stubs for unmatched virtuals (anchors the vtable in this TU).
void CDDrawSubMgr::VirtualMethodUnknown14() {}
int CDDrawSubMgr::VirtualMethodUnknown18() {
    return 0;
}

// Engine-label backlog stubs (moved from src/Stub/CDDrawSubMgr.cpp).
// VirtualMethodUnknown20 is the vtable anchor above; carry its backlog RVA here.
// @confidence: med
// @source: tomalla
// @stub
RVA(0x00157790, 0x6)
void CDDrawSubMgr::VirtualMethodUnknown20() {}

// 0x155720 was labeled ~CDDrawSubMgr in the backlog, but the real (virtual) dtor
// is 0x1574d0 above (??1...@UAE, matched); 0x155720 is a distinct retail function
// of unknown identity - keep it in the worklist under a neutral name.
// @confidence: low
// @source: rtti-vptr
// @stub
RVA(0x00155720, 0x1e)
void CDDrawSubMgr::Stub_155720() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x00157630, 0x82)
void CDDrawSubMgr::Constructor_157630() {}

// @confidence: med
// @source: tomalla
// @stub
RVA(0x001576c0, 0x6)
void CDDrawSubMgr::VirtualMethodUnknown1C() {}

// Engine-label backlog stubs (moved from src/Stub/CDDrawSubMgrBase.cpp).

// @confidence: med
// @source: call-xref
// @stub
RVA(0x00156e10, 0x68)
void CDDrawSubMgrBase::Constructor_156e10() {}

// ===========================================================================
// Discovered cluster (CDDrawWorkerMgr / CDDrawBlitParam) — banked methods.
// Defined in ascending retail-RVA order; Blt/Flip/BltFast are reloc-masked
// __thiscall engine wrappers on the +0x2c surface proxy.
// ===========================================================================

// 0x158b40: pick m_18 (arg2==2) or m_14, null-check, dispatch slot 0x34 with arg1.
RVA(0x00158b40, 0x2c)
int CDDrawWorkerMgr::Method_158b40(int arg1, int arg2) {
    CDDrawSurfacePair* p;
    if (arg2 == 2) {
        p = m_18;
        if (!p) {
            return 0;
        }
    } else {
        p = m_14;
        if (!p) {
            return 0;
        }
    }
    return p->Vfunc34(arg1);
}

// 0x158c70: blt dst's surface <- m_10's surface; return (hr == 0).
RVA(0x00158c70, 0x36)
int CDDrawWorkerMgr::Method_158c70(CDDrawSurfacePair* dst) {
    if (!m_10) {
        return 0;
    }
    CDDSurface* s = m_10->m_surface;
    if (!s) {
        return 0;
    }
    CDDSurface* d = dst->m_surface;
    if (!d) {
        return 0;
    }
    int hr = d->Blt(s);
    return hr == 0;
}

// 0x158d20: return m_18->Vfunc14() != 0.
RVA(0x00158d20, 0x16)
int CDDrawWorkerMgr::Method_158d20() {
    if (!m_18) {
        return 0;
    }
    return m_18->Vfunc14() != 0;
}

// 0x158dc0: blt m_14's surface <- m_10's surface; if the m_0c flag bit1 is set,
// flip m_10 and re-blt.  The first-block boolean (ok) is the carried return value.
// @early-stop
// 71% — logic + offsets exact; residual is branch-layout/scheduling of the
// first-block (Blt-fall-through vs our jmp) and the esi-pop placement, a
// regalloc/scheduling wall (see docs/patterns/zero-register-pinning.md).
RVA(0x00158dc0, 0x7d)
int CDDrawWorkerMgr::Method_158dc0() {
    CDDrawSurfacePair* p10 = m_10;
    CDDrawSurfacePair* p14 = m_14;
    int ok;
    if (p10 && p10->m_surface) {
        CDDSurface* s10 = p10->m_surface;
        CDDSurface* s14 = p14->m_surface;
        if (s14) {
            int hr = s14->Blt(s10);
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
    if (!(m_0c->m_34flagAt & 2)) {
        return ok;
    }
    m_10->m_surface->Flip(0);
    CDDrawSurfacePair* a = m_14;
    CDDrawSurfacePair* b = m_10;
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
    int hr2 = bs->Blt(a->m_surface);
    return hr2 == 0;
}

// 0x158e40: if m_18->Vfunc14(): blt m_18's surface <- m_10's surface, return (==0).
// @early-stop
// 50% — structure/offsets byte-exact; the only residual is the `pop esi`
// scheduling (retail interleaves it before the test/sete; our cl emits it in
// the epilogue), a scheduling coin-flip (docs/patterns/zero-register-pinning.md).
RVA(0x00158e40, 0x4c)
int CDDrawWorkerMgr::Method_158e40() {
    if (m_18 && m_18->Vfunc14()) {
        CDDrawSurfacePair* a = m_18;
        CDDrawSurfacePair* b = m_10;
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
        int hr = as->Blt(bs);
        return hr == 0;
    }
    return 0;
}

// 0x158e90: if m_14 and m_18->Vfunc14(): BltFast(0,0,m_14 surf, &m_14[+0x1c], 0x10).
RVA(0x00158e90, 0x47)
int CDDrawWorkerMgr::Method_158e90() {
    if (!m_14) {
        return 0;
    }
    if (!m_18) {
        return 0;
    }
    if (!m_18->Vfunc14()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_14;
    CDDrawSurfacePair* b = m_18;
    b->m_surface->BltFast(0, 0, a->m_surface, a->m_rect1c, 0x10);
    return 1;
}

// 0x158ee0: if m_14, m_18 and m_18->Vfunc14(): BltFast(0,0,m_18 surf,&m_18[+0x1c],0x10).
RVA(0x00158ee0, 0x47)
int CDDrawWorkerMgr::Method_158ee0() {
    if (!m_14) {
        return 0;
    }
    if (!m_18) {
        return 0;
    }
    if (!m_18->Vfunc14()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_18;
    CDDrawSurfacePair* b = m_14;
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
void CDDrawBlitParam::Init_15c290(CDDrawBlitParamSrc* src) {
    m_10 = (int)src;
    m_28 = 1;
    m_14 = 0;
    m_38 = 1.0f;
    m_24 = 1;
    m_2c = *(int*)((char*)src->m_0c + 0x34) & 0x40;
}

// 0x15c2c0: blit-param zero-reset (vtable slot 6 of the element class).
RVA(0x0015c2c0, 0xc)
void CDDrawBlitParam::Reset_15c2c0() {
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
}

// 0x15c2d0: blit-param setup from a worker source.
RVA(0x0015c2d0, 0x45)
void CDDrawBlitParam::Setup_15c2d0(CDDrawBlitParamSrc* src) {
    char* e;
    int v;
    m_14 = (int)src;
    if (!src) {
        return;
    }
    m_1c = 0;
    if (src->m_10 > 0) {
        e = *(char**)src->m_0c;
    } else {
        e = 0;
    }
    m_18 = (int)e;
    m_20 = 0;
    m_28 = 0;
    v = *(int*)(e + 0x1c);
    m_30 = v;
    m_34 = v;
    {
        float f = src->m_20;
        m_38 = f;
    }
}
