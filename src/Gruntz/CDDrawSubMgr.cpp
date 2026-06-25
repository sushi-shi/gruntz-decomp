#include <rva.h>
// <Mfc.h> brings the real MFC CPtrList / CMapPtrToPtr (afxcoll) used by the
// CWwdObjMgr collection class below.  Must precede any windows/DirectX header.
#include <Mfc.h>
#include <string.h> // strcpy (the inline CRT copy in Serialize_15c970)
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
    CDDrawSubMgrBase(i32 x) {
        m_fieldBaseUnknown = x;
    }
    virtual ~CDDrawSubMgrBase() {}
    i32 m_fieldBaseUnknown; // +0x04

    // Engine-label backlog stubs.
    void Constructor_156e10();
};

class CDDrawSubMgr : public CDDrawSubMgrBase {
public:
    CDDrawSubMgr(CDDrawSurfaceMgr* pHarryPotter, i32 unknown2, i32 unknown3);
    virtual ~CDDrawSubMgr() OVERRIDE;
    virtual void VirtualMethodUnknown14();
    virtual i32 VirtualMethodUnknown18();
    virtual void VirtualMethodUnknown1C(); // cleanup — defined in CDDrawSubMgrDraco.cpp
    virtual void VirtualMethodUnknown20();

    // Engine-label backlog stubs.
    void Constructor_157630();
    void Stub_155720();

    i32 fieldUnknown8;                // +0x08
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
    virtual i32 Vfunc14(); // slot 5  (@0x14): surface-ready predicate
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual i32 Vfunc34(i32 a); // slot 13 (@0x34): 1-arg op
    char m_pad04[0x1c - 0x04];  // +0x04 .. +0x1b
    char m_rect1c[0x2c - 0x1c]; // +0x1c  scratch RECT passed to BltFast
    CDDSurface* m_surface;      // +0x2c
};

// The worker held at manager+0x0c: a flag byte sits at +0x34.
class CDDrawWorkerNode {
public:
    char m_pad00[0x34]; // +0x00 .. +0x33
    i32 m_34flagAt;     // +0x34 flag word (bit1 tested)
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
    i32 Method_158b40(i32 arg1, i32 arg2);
    i32 Method_158c70(CDDrawSurfacePair* dst);
    i32 Method_158d20();
    i32 Method_158dc0();
    i32 Method_158e40();
    i32 Method_158e90();
    i32 Method_158ee0();
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
    i32 m_10;           // +0x10 count
    char m_pad14[0x20 - 0x14];
    float m_20; // +0x20
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
    virtual void Av2C();
    virtual void Write(const void* buf, i32 len); // +0x30
};

// The worker held at CDDrawBlitParam+0x0c: holds a sub-object at +0x2c whose
// 0x152d30 method returns a CString (the label written by Serialize).
class CDDrawBlitWorker {
public:
    char m_pad00[0x2c]; // +0x00..0x2b
    void* m_2c;         // +0x2c sub-object (Method_152d30 -> CString)
};
class CDDrawBlitLabelSource {
public:
    CString GetLabel_152d30(i32 a);
};

class CDDrawBlitParam {
public:
    void Init_15c290(CDDrawBlitParamSrc* src);
    void Reset_15c2c0();
    void Setup_15c2d0(CDDrawBlitParamSrc* src);
    i32 Serialize_15c970(CWwdArchive* ar);
    i32 Deserialize_15ca70(CWwdArchive* ar);
    i32 Dispatch_15c900(CWwdArchive* ar, i32 type, i32 a3, i32 a4);

    char m_pad00[0x0c];     // +0x00 .. +0x0b
    CDDrawBlitWorker* m_0c; // +0x0c
    i32 m_10;               // +0x10
    i32 m_14;               // +0x14
    i32 m_18;               // +0x18
    i32 m_1c;               // +0x1c
    i32 m_20;               // +0x20
    i32 m_24;               // +0x24
    i32 m_28;               // +0x28
    i32 m_2c;               // +0x2c
    i32 m_30;               // +0x30
    i32 m_34;               // +0x34
    float m_38;             // +0x38
};

// ---------------------------------------------------------------------------
// CDDrawSubMgr::CDDrawSubMgr
// Chains the Hogwarts(int) base ctor (inlined: this+0x04 = unknown2), stamps
// the Lucius vtable (compiler-generated), then seeds the remaining fields.
// ---------------------------------------------------------------------------
RVA(0x00156cb0, 0x20)
CDDrawSubMgr::CDDrawSubMgr(CDDrawSurfaceMgr* pHarryPotter, i32 unknown2, i32 unknown3)
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
i32 CDDrawSubMgr::VirtualMethodUnknown18() {
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
i32 CDDrawWorkerMgr::Method_158b40(i32 arg1, i32 arg2) {
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
i32 CDDrawWorkerMgr::Method_158c70(CDDrawSurfacePair* dst) {
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
    i32 hr = d->Blt(s);
    return hr == 0;
}

// 0x158d20: return m_18->Vfunc14() != 0.
RVA(0x00158d20, 0x16)
i32 CDDrawWorkerMgr::Method_158d20() {
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
i32 CDDrawWorkerMgr::Method_158dc0() {
    CDDrawSurfacePair* p10 = m_10;
    CDDrawSurfacePair* p14 = m_14;
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
    i32 hr2 = bs->Blt(a->m_surface);
    return hr2 == 0;
}

// 0x158e40: if m_18->Vfunc14(): blt m_18's surface <- m_10's surface, return (==0).
// @early-stop
// 50% — structure/offsets byte-exact; the only residual is the `pop esi`
// scheduling (retail interleaves it before the test/sete; our cl emits it in
// the epilogue), a scheduling coin-flip (docs/patterns/zero-register-pinning.md).
RVA(0x00158e40, 0x4c)
i32 CDDrawWorkerMgr::Method_158e40() {
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
        i32 hr = as->Blt(bs);
        return hr == 0;
    }
    return 0;
}

// 0x158e90: if m_14 and m_18->Vfunc14(): BltFast(0,0,m_14 surf, &m_14[+0x1c], 0x10).
RVA(0x00158e90, 0x47)
i32 CDDrawWorkerMgr::Method_158e90() {
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
i32 CDDrawWorkerMgr::Method_158ee0() {
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
    m_10 = (i32)src;
    m_28 = 1;
    m_14 = 0;
    m_38 = 1.0f;
    m_24 = 1;
    m_2c = *(i32*)((char*)src->m_0c + 0x34) & 0x40;
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
    i32 v;
    m_14 = (i32)src;
    if (!src) {
        return;
    }
    m_1c = 0;
    if (src->m_10 > 0) {
        e = *(char**)src->m_0c;
    } else {
        e = 0;
    }
    m_18 = (i32)e;
    m_20 = 0;
    m_28 = 0;
    v = *(i32*)(e + 0x1c);
    m_30 = v;
    m_34 = v;
    {
        float f = src->m_20;
        m_38 = f;
    }
}

// ---------------------------------------------------------------------------
// 0x15c970: serialize the blit-param.  Writes the eight dwords m_1c..m_38 to
// the archive (4 bytes each via slot +0x30), zeroes a 0x80-byte label buffer,
// and if m_14 is set, fetches the worker label (a returns-by-value CString from
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
    ar->Write(&m_1c, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_28, 4);
    ar->Write(&m_2c, 4);
    ar->Write(&m_30, 4);
    ar->Write(&m_34, 4);
    ar->Write(&m_38, 4);
    char buf[0x80];
    for (i32 i = 0; i < 0x20; ++i) {
        ((i32*)buf)[i] = 0;
    }
    if (m_14 != 0) {
        CString label = ((CDDrawBlitLabelSource*)m_0c->m_2c)->GetLabel_152d30(m_14);
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
i32 CDDrawBlitParam::Dispatch_15c900(CWwdArchive* ar, i32 type, i32 a3, i32 a4) {
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
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30(); // +0x30 archive write
    virtual void Slot34();
    virtual void Slot38();
    virtual i32 Slot3C(i32 a1, i32 a2, i32 a3, void* obj); // +0x3c
    char m_pad04[0x08 - 0x04];
    i32 m_08;                  // +0x08 flag word
    char m_pad0c[0x74 - 0x0c]; // +0x0c..0x73
    i32 m_74;                  // +0x74 sort key
    i32 m_78;                  // +0x78 CPtrList POSITION cache
    char m_pad7c[0x188 - 0x7c];
    void* m_188; // +0x188 map key
};

// A worker held in the +0x2c/+0x48 maps that exposes a __thiscall probe at
// 0x151c00 (called from Method_15acb0).
class CWwdProbeObject : public CWwdObject {
public:
    i32 Probe_151c00(i32 a1, i32 a2);
};

class CWwdGameObject;

class CWwdObjMgr {
public:
    CWwdGameObject* CreateObject_159600(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 flags);
    void RemoveByPosition_15ab70(i32 pos, CWwdObject* obj);
    void AddToMap48_15aba0(CWwdObject* obj);
    void PruneList_15aa90();
    i32 CountActive_15abc0();
    i32 ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3);
    i32 ForEachProbe_15acb0(i32 a1, i32 a2);
    i32 ForEachSerialize_15b020(CWwdArchive* ar, i32 a2);
    i32 PruneOrphans_15b1d0();
    void InsertSorted_159e40(CWwdObject* obj, i32 addToMaps);

    char m_pad00[0x0c]; // +0x00..0x0b
    i32 m_0c;           // +0x0c parent handle
    CPtrList m_10;      // +0x10 sorted object list
    CMapPtrToPtr m_2c;  // +0x2c key -> object (primary)
    CMapPtrToPtr m_48;  // +0x48 key -> object (active set)
};

// CPtrList CNode shape (pNext@0, pPrev@4, data@8); the list head node is at
// CWwdObjMgr+0x14 (= m_10.m_pNodeHead).
struct CWwdNode {
    CWwdNode* m_next;  // +0x00
    void* m_prev;      // +0x04
    CWwdObject* m_obj; // +0x08
};

// ---------------------------------------------------------------------------
// 0x15ab70: drop a list slot + its primary-map entry.
RVA(0x0015ab70, 0x27)
void CWwdObjMgr::RemoveByPosition_15ab70(i32 pos, CWwdObject* obj) {
    m_10.RemoveAt((POSITION)pos);
    m_2c.RemoveKey(obj->m_188);
}

// ---------------------------------------------------------------------------
// 0x15aba0: m_48[obj->key] = obj.
RVA(0x0015aba0, 0x1a)
void CWwdObjMgr::AddToMap48_15aba0(CWwdObject* obj) {
    m_48[obj->m_188] = obj;
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
        if (obj != 0 && !(obj->m_08 & 0x200)) {
            m_10.RemoveAt((POSITION)cur);
            m_2c.RemoveKey(obj->m_188);
            m_48.RemoveKey(obj->m_188);
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
            if (val != 0 && !(val->m_08 & 0x4000000)) {
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
            if (val != 0 && !(val->m_08 & 0x4000000)) {
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
            if (val != 0 && !(val->m_08 & 0x4000000)) {
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
            if (val != 0 && !(val->m_08 & 0x4000000)) {
                void* k = val->m_188;
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
                if (!m_2c.Lookup(val->m_188, found)) {
                    found = 0;
                }
                if (found == 0) {
                    m_48.RemoveKey(val->m_188);
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
    if (obj->m_08 & 0x800) {
        obj->m_78 = 0;
        return;
    }
    if (addToMaps != 0) {
        m_2c[obj->m_188] = obj;
        m_48[obj->m_188] = obj;
    }
    struct HLayout {
        char _pad[0x14];
        CWwdNode* m_head;
    };
    CWwdNode* node = ((HLayout*)this)->m_head;
    i32 key = obj->m_74;
    while (node != 0) {
        CWwdNode* cur = node;
        CWwdObject* data = cur->m_obj;
        node = node->m_next;
        if (data->m_74 > key && !(data->m_08 & 0x20000)) {
            obj->m_78 = (i32)m_10.InsertBefore((POSITION)cur, obj);
            return;
        }
    }
    obj->m_78 = (i32)m_10.AddTail(obj);
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
class CQueueProbeNode {
public:
    CQueueProbeNode* m_next; // +0x00
    char m_pad04[0x04];      // +0x04
    void* m_data;            // +0x08 -> probed object
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
    char m_pad00[0x68];    // +0x00 .. +0x67
    CQueueProbeNode* m_68; // +0x68 list head
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
    while (m_68 != 0) {
        CQueueProbeNode* head = m_68;
        m_68 = head->m_next;
        CQueueProbeData* data = (CQueueProbeData*)head->m_data;
        if (data->Probe20() == 5) {
            return data;
        }
    }
    return 0;
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
DATA(0x0021ab14)
extern i32 g_wwdObjIdCounter; // 0x61ab14

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
// CRemusNode 3-arg ctor at 0x15b2c0 (root, a2, a3).
class CWwdRemusBase {
public:
    void Ctor(i32 root, i32 a2, i32 a3); // 0x15b2c0
};
// The 0x17c-byte sprite-animation worker built at +0x7c (SiriusWorker, 0x15b300).
class CWwdWorker {
public:
    void Ctor(i32 a, i32 b, i32 c); // 0x15b300
    virtual void V00();
    virtual void V04();
    virtual void V08();
    virtual void V0C();
    virtual i32 Kick(void* owner); // +0x10
};
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
};

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
        i32 root = m_0c;
        ((CWwdRemusBase*)obj)->Ctor(root, a1, flags);
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
