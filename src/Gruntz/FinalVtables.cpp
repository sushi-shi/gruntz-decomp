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
// Wap::CObject base thunks), slot 1 dtor 0x155890. Slots 5/6/7 = CDDrawSurfaceMgr
// IsReady/Init/Cleanup -> this is CDDrawSurfaceMgr's own vtable.
// ---------------------------------------------------------------------------
struct CVtEmit_1efc58 : Wap::CObject {
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

// ---------------------------------------------------------------------------
// 0x5efd28 (RVA 0x1efd28) - 23 slots. CDDrawWorkerRegistry's OWN vtable
// (slot 1 = 0x156df0 = CDDrawWorkerRegistry::RegScalarDtor, already matched -> NOT
// redefined here). CObject-style base thunks at 0/2/3/4. Slots carry the matched
// CDDrawWorkerRegistry leaf names (Stub_<rva> slots stay on the worklist).
// ---------------------------------------------------------------------------
struct CVtEmit_1efd28 : Wap::CObject {
    ~CVtEmit_1efd28();         // [1] 0x156df0 scalar-deleting dtor (anchor, overrides slot 1)
    void Slot05_156dc0();      // [5] 0x156dc0
    void ResetScratch();       // [6] 0x154aa0 = CDDrawWorkerRegistry::ResetScratch
    void Shutdown();           // [7] 0x154ac0 = CDDrawWorkerRegistry::Shutdown
    void GetStateId();         // [8] 0x156de0 = CDDrawWorkerRegistry::GetStateId
    void DispatchKeyed2C();    // [9] 0x154df0
    void Forward2C();          // [10] 0x154f60
    void Forward30();          // [11] 0x154f40
    void DispatchKeyed30();    // [12] 0x154ce0
    void Forward38();          // [13] 0x154f20
    void DispatchKeyed38();    // [14] 0x154ae0
    void Forward34();          // [15] 0x154f00
    void DispatchKeyed34();    // [16] 0x154be0
    void ProbeWorkerKey();     // [17] 0x156e80 = ProbeWorkerKey (worklist)
    void InsertWorkerKey();    // [18] 0x154f80 = InsertWorkerKey (worklist)
    void LookupWorkerKey();    // [19] 0x155160 = LookupWorkerKey (worklist)
    void RemoveWorker();       // [20] 0x155280 = CDDrawWorkerRegistry::RemoveWorker
    void RemoveByKey();        // [21] 0x156ec0 = CDDrawWorkerRegistry::RemoveByKey
    void MapTeardown_1552b0(); // [22] 0x1552b0
    i32 m_0;
    i32 Anchor();
};
i32 CVtEmit_1efd28::Anchor() {
    return m_0 != 0;
}
CVtEmit_1efd28::~CVtEmit_1efd28() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtEmit_1efd28);

// ---------------------------------------------------------------------------
// 0x5efd88 (RVA 0x1efd88) - 14 slots. CObject-style, slot 1 dtor 0x156f30.
// Slots carry the matched CDDrawWorkerList leaf names.
// ---------------------------------------------------------------------------
struct CVtEmit_1efd88 : Wap::CObject {
    ~CVtEmit_1efd88();       // [1] 0x156f30 scalar-deleting dtor (anchor, overrides slot 1)
    void IsReady();          // [5] 0x156f00 = CDDrawWorkerList::IsReady
    void IsReadyPredicate(); // [6] 0x156fc0 = IsReadyPredicate (worklist)
    void Dtor_163bc0();      // [7] 0x163bc0 = CDDrawWorkerList::~CDDrawWorkerList
    void GetStateId();       // [8] 0x156f20 = CDDrawWorkerList::GetStateId
    void CreateWorkerA();    // [9] 0x156fd0 = CDDrawWorkerList::CreateWorkerA
    void CreateWorkerB28();  // [10] 0x1573e0
    void CreateWorkerB2C();  // [11] 0x157330
    void CreateWorkerB30();  // [12] 0x157150
    void PruneWorkers();     // [13] 0x163bf0 = CDDrawWorkerList::PruneWorkers
    i32 m_0;
    i32 Anchor();
};
i32 CVtEmit_1efd88::Anchor() {
    return m_0 != 0;
}
CVtEmit_1efd88::~CVtEmit_1efd88() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtEmit_1efd88);

// ---------------------------------------------------------------------------
// 0x5efdc0 (RVA 0x1efdc0) - 17 slots. A SECOND vtable whose slot-1 dtor
// (0x157610 = CDDrawWorkerMapSmall::MapSmallScalarDtor, already matched -> NOT redefined)
// is owned by CDDrawWorkerMapSmall (its primary is ??_7CDDrawWorkerMapSmall
// @0x1efcc8). Modeled here as a standalone tracking class (a per-class ??_7CVtbl_
// primary names the datum; realizing it AS the MI-secondary of CDDrawWorkerMapSmall
// would need the +offset construction-vtable machinery). CObject-style thunks.
// Slots carry the matched CDDrawChildGroup/CDDrawSubMgr/CWwdObjMgr leaf names.
// ---------------------------------------------------------------------------
struct CVtEmit_1efdc0 : Wap::CObject {
    ~CVtEmit_1efdc0();          // [1] 0x157610 scalar-deleting dtor (anchor, overrides slot 1)
    void IsReady();             // [5] 0x1575e0 = CDDrawChildGroup::IsReady
    void OnDestroy();           // [6] 0x1576c0 = CDDrawSubMgr::OnDestroy
    void ForwardTo3C();         // [7] 0x1591e0 = CDDrawChildGroup::ForwardTo3C
    void GetStateId();          // [8] 0x157600 = CDDrawWorkerMapSmall::GetStateId
    void TickKillCues_159a70(); // [9] 0x159a70 = CWwdObjMgr::TickKillCues_159a70
    void WalkDispatch2C();      // [10] 0x159c90 = CDDrawChildGroup::WalkDispatch2C
    void WalkDispatch30();      // [11] 0x159cc0
    void WalkDispatch34();      // [12] 0x159cf0
    void WalkDispatch38();      // [13] 0x159d40
    void ResetChildD8();        // [14] 0x159d90 = CDDrawChildGroup::ResetChildD8
    void DestroyChildren();     // [15] 0x1591f0 = DestroyChildren (worklist)
    void Slot16_159f00();       // [16] 0x159f00
    i32 m_0;
    i32 Anchor();
};
i32 CVtEmit_1efdc0::Anchor() {
    return m_0 != 0;
}
CVtEmit_1efdc0::~CVtEmit_1efdc0() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtEmit_1efdc0);

// ---------------------------------------------------------------------------
// 0x5eff70 (RVA 0x1eff70) - 11 slots. A CWapObj-derived worker: slots 0-4 are the
// CObject grand-base thunks, slot 5 overrides IsLoaded (0x159150), slot 6 is the
// inherited IsReady default (0x001c08 - the CWapObj binary fingerprint), and slots
// 7-10 are its own virtuals. Slot 9 (0x1644a0) points at a CDDrawSurfacePair method
// (declared-only), so this is a surface-pair-adjacent worker vtable.
// ---------------------------------------------------------------------------
struct CVtEmit_1eff70 : CWapObj {
    ~CVtEmit_1eff70();    // [1] 0x159190 scalar-deleting dtor (anchor, overrides slot 1)
    i32 IsLoaded();       // [5] 0x159150
    void Slot07_1591d0(); // [7] 0x1591d0
    void Slot08_159180(); // [8] 0x159180
    void
    directx_wrapper_caller_1644a0_DirectDrawCreate_DirectDrawEnumerateA(); // [9] 0x1644a0 = CDDrawSurfacePair
    void Slot10_1646b0();                                                  // [10] 0x1646b0
    i32 m_0;
    i32 Anchor();
};
i32 CVtEmit_1eff70::Anchor() {
    return m_0 != 0;
}
CVtEmit_1eff70::~CVtEmit_1eff70() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtEmit_1eff70);
