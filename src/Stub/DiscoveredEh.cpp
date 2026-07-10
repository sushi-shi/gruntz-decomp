// DiscoveredEh.cpp - trace-discovered /GX leaf destructors re-homed from
// src/Stub/Discovered.cpp (engine_discovered is base-profile; these dtors carry a
// C++ exception-handling frame, so they need the eh profile). Only OFFSETS + code
// bytes are load-bearing.
//
// Item-7 owner audit (sema xref):
//  - ~CU55 (0x16460): its only caller is its own deleting-dtor
//    (0x16430, vtable slot); RTTI name unrecovered (Ghidra ClassUnknown_55_016460).
//    It is a byte-identical twin of ~CImgHolder (0x16500, Dialogs.cpp) - a dialog
//    image-holder freeing an embedded CImageList - so it belongs to the dialog
//    cluster, but no real class name is recoverable; kept here with this note.
//  - ~CButeStore (0x174d70): the real class CButeStore has a TU (src/Bute/
//    ButeStoreClear.cpp, CButeStore::ClearRecursive). It is NOT folded there because
//    the two CButeStore models diverge: ButeStoreClear.cpp uses <Bute/ButeMgr.h>'s
//    FLAT CButeStore, while this dtor is multiple-inheritance-modeled (CContainerErr
//    @+0 + CObj50 @+8, two vptr re-stamps + two base dtors). Folding requires
//    reconciling the two models (extend ButeMgr.h's CButeStore to MI) - deferred to
//    a depth pass so it can't regress ButeStoreClear.cpp; kept here meanwhile.
#include <Ints.h>
#include <rva.h>

// (tomalla-55 @0x016460 re-homed to src/Gruntz/Dialogs.cpp as CImgHolder2 - a SECOND
// byte-identical dialog image-holder, dissolved onto that TU's existing CImgHolderBase
// grand-base + shared CImageList shim (twin of the ~CImgHolder @0x16500 already there).)

// ---------------------------------------------------------------------------
// CButeStore @0x174d70 - a multiple-inheritance /GX dtor. The most-derived class
// re-stamps its primary vptr (+0, 0x5e94ac) and its secondary base vptr (+8,
// 0x5e949c) at entry, runs ClearRecursive(0) (0x16e070), then destructs its bases
// in reverse declaration order: the secondary base @+8 (~CObj50 0x16dfc0, with the
// `this ? this+8 : 0` adjust) then the primary base @0 (~CContainerErr 0x16da60).
// Real-polymorphic multiple-inheritance model so cl emits both implicit vptr stamps
// + both base-dtor CALLs (all reloc-mask). The non-trivial bases earn the /GX frame.
struct CContainerErr {
    virtual ~CContainerErr(); // 0x16da60 (external)
    char m_pad[8 - 4];        // vptr@+0; CObj50 begins at +8
};
struct CObj50 {
    virtual ~CObj50(); // 0x16dfc0 (external; stamps 0x5f04d8 internally)
};
struct CButeStore : CContainerErr, CObj50 {
    virtual ~CButeStore() OVERRIDE; // 0x174d70
    void ClearRecursive(struct CButeStoreNode*); // 0x16e070 (real takes CButeStoreNode*; 0 = null)
};
RVA(0x00174d70, 0x70)
CButeStore::~CButeStore() {
    ClearRecursive(0);
}

SIZE_UNKNOWN(CObj50);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
