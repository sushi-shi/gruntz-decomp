// FinalVtables.cpp - realizes the last six anonymous retail vtables that had no
// class/stamp/name anywhere in src/ (the residual vtbl-placeholders.h placeholders).
//
// Each is modeled as a REAL polymorphic tracking class (CVtEmit_<rva>) whose virtual
// slots are declared in retail-.rdata slot order (VA = RVA + 0x400000). cl auto-
// emits `??_7CVtbl_<rva>@@6B@`; the VTBL() macro binds that name at the retail RVA
// so the delinked datum is named. An out-of-line virtual DESTRUCTOR (slot 1 in the
// CObject-style layout, calling an out-of-line Anchor member) is the CONSTRUCTION
// ANCHOR that keeps cl from dead-store-eliminating the implicit vptr store, so the
// `??_7` COMDAT is actually emitted (docs/vtable-conversion-log.md iteration-2
// mechanism: an EMPTY dtor may be DSE'd, an out-of-line-member call survives).
//
// The slot virtuals are DECLARED-ONLY (no bodies): the emitted vtable references
// their per-class mangled names, which reloc-mask against the retail slot RVAs in
// the cosmetic `vtables` unit (VTBL naming is matching-NEUTRAL tracking, not a match
// lever). The shared low slots 0x1bef01 / 0x0028ec / 0x00106e / 0x004034 / 0x001c08
// are the CObject/MFC base ILT jmp-thunks -> declared-only, kept on the FUN_<VA>
// convention (unnamed thunks; no canonical name). Slots whose RVA maps to an
// ALREADY-MATCHED function in another TU carry that function's canonical LEAF name
// here (traceability, matching-neutral: still a declared-only per-class virtual, the
// reloc masks). Slots still on FUN_<VA> / Stub_<rva> are the un-reconstructed
// final-sweep worklist. A couple of slots point at already-matched functions in
// other TUs (e.g. CDDrawWorkerRegistry / CDDrawWorkerMapSmall / CDDrawSurfacePair)
// and are NOT redefined here (no dup-RVA) - they stay declared-only.
#include <Ints.h>
#include <Wap32/Object.h> // the shared WAP CObject grand-base (slots 0/2/3/4 base thunks)
#include <Wap32/WapObj.h> // CWapObj : CObject - adds IsLoaded(5)/IsReady(6) defaults
#include <rva.h>

// ---------------------------------------------------------------------------
// 0x5ef7d0 (RVA 0x1ef7d0) - 8 slots. A Rez-family node vtable (0x13cxxx range);
// NOT CObject-style (no 0x1bef01 thunk): slot 0 is a real method, slot 1 the
// scalar-deleting dtor (0x13cb60). Slots 2/3/6 = CRezFile Read/Write/Flush.
// ---------------------------------------------------------------------------
struct CVtEmit_1ef7d0 {
    virtual void Slot00_13cef0(); // [0] 0x13cef0
    virtual ~CVtEmit_1ef7d0();    // [1] 0x13cb60 scalar-deleting dtor (anchor)
    virtual void Read();          // [2] 0x13cc00 = CRezFile::Read
    virtual void Write();         // [3] 0x13cca0 = CRezFile::Write
    virtual void Slot04_13cd40(); // [4] 0x13cd40
    virtual void Slot05_13cd50(); // [5] 0x13cd50
    virtual void Flush();         // [6] 0x13cd60 = CRezFile::Flush
    virtual void Slot07_13cdb0(); // [7] 0x13cdb0
    i32 m_0;
    i32 Anchor();
};
i32 CVtEmit_1ef7d0::Anchor() {
    return m_0 != 0;
}
CVtEmit_1ef7d0::~CVtEmit_1ef7d0() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtEmit_1ef7d0);
VTBL(CVtEmit_1ef7d0, 0x001ef7d0);

// ---------------------------------------------------------------------------
// 0x5efc58 (RVA 0x1efc58) - 8 slots. CObject-derived (slots 0/2/3/4 are the shared
// CObject base thunks), slot 1 dtor 0x155890. Slots 5/6/7 = CDDrawSurfaceMgr
// IsReady/Init/Cleanup -> this is CDDrawSurfaceMgr's own vtable.
// ---------------------------------------------------------------------------
struct CVtEmit_1efc58 : CObject {
    virtual ~CVtEmit_1efc58()
        OVERRIDE;                  // [1] 0x155890 scalar-deleting dtor (anchor, overrides slot 1)
    virtual void IsReady();        // [5] 0x155f00 = CDDrawSurfaceMgr::IsReady
    virtual void Init();           // [6] 0x155900 = CDDrawSurfaceMgr::Init
    virtual void Cleanup_155e20(); // [7] 0x155e20 = CDDrawSurfaceMgr::Cleanup_155e20
    i32 m_0;
    i32 Anchor();
};
i32 CVtEmit_1efc58::Anchor() {
    return m_0 != 0;
}
CVtEmit_1efc58::~CVtEmit_1efc58() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtEmit_1efc58);
VTBL(CVtEmit_1efc58, 0x001efc58);

// NOTE: the other four residual vtables (0x1efd28/0x1efd88/0x1efdc0/0x1eff70) are now
// bound by their REAL classes and no longer need a CVtEmit_ tracking shim:
//   0x1efd28 -> CWorkerVtableView (<DDrawMgr/DDrawWorkerRegistry.h>, all 23 slots)
//   0x1efd88 -> CDDrawWorkerList  (DDrawWorkerList.cpp)
//   0x1efdc0 -> CDDrawChildGroup  (<DDrawMgr/DDrawChildGroup.h>)
//   0x1eff70 -> CDDrawSurfaceChildA (DDrawSubMgrPages.cpp, CWapObj-derived)
// Their slot->function detail lives on those classes; the shims were redundant duplicates.
