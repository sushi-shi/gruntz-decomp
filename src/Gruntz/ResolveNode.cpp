#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : Wap::CObject (9-slot base)
#include <rva.h>
// CResolveNode.cpp - a leaf node in the CDirectDrawMgr surface/page-manager
// CDDrawSubMgr family (the Wap::CObject base dtor vtable lineage; the base subobject
// vftable g_wapObjectDtorVtbl @0x5e8cb4 is restamped at ~CResolveNode exit). The
// node's own primary vftable is at RVA 0x1efbc0 (g_resolveNodeVtbl). The node's
// virtuals are not yet matched, so the vftable is referenced as a reloc-masked
// DIR32 datum and stamped manually rather than modeled polymorphically (an
// incomplete polymorphic class would emit a divergent ??_7 vtable). Only the
// OFFSETS + emitted bytes are load-bearing; field names are placeholders.
//
// Methods (all plain /O2 /MT leaves, NO EH frame, __thiscall):
//   0x1549d0  default ctor (seeds sentinels, m_0c = 0)
//   0x154a30  ??_G scalar-deleting destructor (in Discovered.cpp slot 1 of vtbl)
//   0x154a50  ~CResolveNode (resets fields, restamps base vptr)
//   0x15b2c0  parameterized ctor (root + two scalars)
//   0x1647e0  Init(a1..a6): seeds fields, dispatches virtual slot 9, returns 1

// ALL-VTABLES mandate: the node is now a REAL polymorphic two-level class. The
// CObject grand-base (Wap::CObject, 5-slot interface @0x5e8cb4) and the
// node's own primary vtable (10-slot @0x5efbc0, mapped ??_7CResolveNode in
// config/vtable_names.csv) are cl-auto-emitted: the ctors stamp ??_7CResolveNode
// (vptr-first) and ~CResolveNode folds the ??_7Wap@@CObject grand-base re-stamp
// (masks 0x5e8cb4) - no manual `*(void**)this = &g_*Vtbl` stores.

// The 0x68-byte node. Layout recovered from the ctor/dtor/Init stores; the gaps
// are unread scratch (the family's resolution-ladder block).
class CResolveNode : public CLoadable {
public:
    // Re-based onto the canonical 9-slot CLoadable (fulfils the former TODO): the
    // m_04/m_08/m_0c header + slots 5..8 (IsLoaded/IsReady/Unload/GetClassId) come
    // from CLoadable. This node OVERRIDES slot 5 (IsLoaded @0x154a10) and slot 7
    // (Unload/reset @0x154a80), and INHERITS slot 6 (IsReady @0x001c08) and slot 8
    // (GetClassId @0x154a00 = CLASSID_NONE) unchanged - GetClassId is no longer
    // re-declared. Resolve (slot 9) is the node's own new virtual.
    i32 IsLoaded() OVERRIDE;       // [5] 0x154a10  (checks m_04!=-1 && m_0c)
    i32 Unload() OVERRIDE;         // [7] 0x154a80  reset/reload hook
    virtual i32 Resolve(i32, i32); // [9] 0x164790  CResolveNode's own new virtual

    CResolveNode();
    CResolveNode(i32 owner, i32 field04, i32 field08);
    virtual ~CResolveNode() OVERRIDE;
    i32 Init(i32 owner, i32 field04, i32 resolveX, i32 resolveY, i32 field40, i32 field08);

    // vptr @+0x00 + m_04/m_08/m_0c inherited from CLoadable; own scratch from +0x10.
    char _pad10[0x20 - 0x10]; // +0x10..+0x1f
    i32 m_20;                 // +0x20  = 0x80000000
    char _pad24[0x38 - 0x24]; // +0x24..+0x37
    i32 m_38;                 // +0x38  = -1
    i32 m_3c;                 // +0x3c  = 0
    i32 m_40;                 // +0x40
    char _pad44[0x4c - 0x44]; // +0x44..+0x4b
    i32 m_4c;                 // +0x4c
    i32 m_50;                 // +0x50
    char _pad54[0x58 - 0x54]; // +0x54..+0x57
    i32 m_58;                 // +0x58
    i32 m_5c;                 // +0x5c  = 0x80000000
    char _pad60[0x64 - 0x60]; // +0x60..+0x63
    i32 m_64;                 // +0x64  = 0x80000000
};

// Default ctor: seeds the resolution/scaling sentinels and zeroes the base
// fields, then stamps the node vftable. No arg-store to float, unlike the
// parameterized ctor below.
RVA(0x001549d0, 0x29)
CResolveNode::CResolveNode() {
    m_0c = 0;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_5c = (i32)0x80000000;
    m_64 = (i32)0x80000000;
    m_3c = 0;
    m_40 = 0;
}

// ~CResolveNode: resets the sentinels/base fields and restamps the base-subobject
// (CObject-like) dtor vftable. Trivial base -> no member teardown, no EH frame.
// @early-stop
// sentinel-seed store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// the body is byte-identical EXCEPT the single immediate vptr restamp store
// (`mov [ecx],&g_wapObjectDtorVtbl`). Retail defers it to just after the eax
// sentinel chain (`xor eax,eax` then the store, before m_08/m_0c); MSVC5 here
// eagerly hoists the data-independent immediate store to position 2 (right after
// `mov eax,0x80000000`). No source order steers it (tried vptr-first/mid/last; all
// ~78.8%) - the immediate store carries no dependency. Logic complete.
RVA(0x00154a50, 0x23)
CResolveNode::~CResolveNode() {
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    // ~CLoadable folds here: m_04=-1, m_08=0, m_0c=0, then the grand-base vptr
    // re-stamp (masks 0x5e8cb4) via ~CWapObj -> ~Wap::CObject.
}

// @early-stop
// sentinel-seed ctor store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// identical instruction multiset, but cl floats the m_08 (edx=arg3) store and the
// m_38 (-1) store to different positions than retail; 3 field-order spellings all
// ~60%. Source steers which arg lands in edx, not the store schedule. Logic complete.
RVA(0x0015b2c0, 0x3d)
CResolveNode::CResolveNode(i32 owner, i32 field04, i32 field08) {
    m_04 = field04;
    m_08 = field08;
    m_0c = owner;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_5c = (i32)0x80000000;
    m_64 = (i32)0x80000000;
    m_3c = 0;
    m_40 = 0;
}

// Init: seeds m_0c/m_04/m_08 (owner/field04/field08), clears m_4c/m_58, sets
// m_50 = 1, dispatches the node's virtual slot 9 (g_resolveNodeVtbl[9] = 0x164790)
// with (resolveX, resolveY), finally stores field40 into m_40 and returns TRUE.
RVA(0x001647e0, 0x48)
i32 CResolveNode::Init(
    i32 owner,
    i32 field04,
    i32 resolveX,
    i32 resolveY,
    i32 field40,
    i32 field08
) {
    m_0c = owner;
    m_04 = field04;
    m_08 = field08;
    m_4c = 0;
    m_58 = 0;
    m_50 = 1;
    Resolve(resolveX, resolveY); // virtual slot 9 (0x164790)
    m_40 = field40;
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CResolveNode);
SIZE_UNKNOWN(Wap::CObject);
