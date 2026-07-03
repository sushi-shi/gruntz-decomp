// FinalVtables.cpp - realizes the last six anonymous retail vtables that had no
// class/stamp/name anywhere in src/ (the residual vtbl-placeholders.h placeholders).
//
// Each is modeled as a REAL polymorphic tracking class (CVtbl_<rva>) whose virtual
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
// are the CObject/MFC base ILT jmp-thunks -> declared-only. The un-reconstructed
// engine slot bodies (0x15xxxx/0x16xxxx) remain the final-sweep worklist; a couple
// of slots point at already-matched functions in other TUs (e.g. slot fns owned by
// CDDrawWorkerRegistry / CDDrawWorkerMapSmall / CDDrawSurfacePair) and are NOT
// redefined here (no dup-RVA) - they stay declared-only.
#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// 0x5ef7d0 (RVA 0x1ef7d0) - 8 slots. A Rez-family node vtable (0x13cxxx range);
// NOT CObject-style (no 0x1bef01 thunk): slot 0 is a real method, slot 1 the
// scalar-deleting dtor (0x13cb60).
// ---------------------------------------------------------------------------
struct CVtbl_1ef7d0 {
    virtual void FUN_0053cef0(); // [0] 0x13cef0
    virtual ~CVtbl_1ef7d0();     // [1] 0x13cb60 scalar-deleting dtor (anchor)
    virtual void FUN_0053cc00(); // [2] 0x13cc00
    virtual void FUN_0053cca0(); // [3] 0x13cca0
    virtual void FUN_0053cd40(); // [4] 0x13cd40
    virtual void FUN_0053cd50(); // [5] 0x13cd50
    virtual void FUN_0053cd60(); // [6] 0x13cd60
    virtual void FUN_0053cdb0(); // [7] 0x13cdb0
    i32 m_0;
    i32 Anchor();
};
i32 CVtbl_1ef7d0::Anchor() {
    return m_0 != 0;
}
CVtbl_1ef7d0::~CVtbl_1ef7d0() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtbl_1ef7d0);
VTBL(CVtbl_1ef7d0, 0x001ef7d0);

// ---------------------------------------------------------------------------
// 0x5efc58 (RVA 0x1efc58) - 8 slots. CObject-style (slots 0/2/3/4 base thunks),
// slot 1 dtor 0x155890.
// ---------------------------------------------------------------------------
struct CVtbl_1efc58 {
    virtual void FUN_005bef01(); // [0] 0x1bef01 (base thunk)
    virtual ~CVtbl_1efc58();     // [1] 0x155890 scalar-deleting dtor (anchor)
    virtual void FUN_004028ec(); // [2] 0x0028ec (base thunk)
    virtual void FUN_0040106e(); // [3] 0x00106e (base thunk)
    virtual void FUN_00404034(); // [4] 0x004034 (base thunk)
    virtual void FUN_00555f00(); // [5] 0x155f00
    virtual void FUN_00555900(); // [6] 0x155900
    virtual void FUN_00555e20(); // [7] 0x155e20
    i32 m_0;
    i32 Anchor();
};
i32 CVtbl_1efc58::Anchor() {
    return m_0 != 0;
}
CVtbl_1efc58::~CVtbl_1efc58() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtbl_1efc58);
VTBL(CVtbl_1efc58, 0x001efc58);

// ---------------------------------------------------------------------------
// 0x5efd28 (RVA 0x1efd28) - 23 slots. CDDrawWorkerRegistry's OWN vtable
// (slot 1 = 0x156df0 = CDDrawWorkerRegistry::Stub_156df0, already matched -> NOT
// redefined here). CObject-style base thunks at 0/2/3/4.
// ---------------------------------------------------------------------------
struct CVtbl_1efd28 {
    virtual void FUN_005bef01(); // [0] 0x1bef01 (base thunk)
    virtual ~CVtbl_1efd28();     // [1] 0x156df0 scalar-deleting dtor (anchor)
    virtual void FUN_004028ec(); // [2] 0x0028ec (base thunk)
    virtual void FUN_0040106e(); // [3] 0x00106e (base thunk)
    virtual void FUN_00404034(); // [4] 0x004034 (base thunk)
    virtual void FUN_00556dc0(); // [5] 0x156dc0
    virtual void FUN_00554aa0(); // [6] 0x154aa0
    virtual void FUN_00554ac0(); // [7] 0x154ac0
    virtual void FUN_00556de0(); // [8] 0x156de0
    virtual void FUN_00554df0(); // [9] 0x154df0
    virtual void FUN_00554f60(); // [10] 0x154f60
    virtual void FUN_00554f40(); // [11] 0x154f40
    virtual void FUN_00554ce0(); // [12] 0x154ce0
    virtual void FUN_00554f20(); // [13] 0x154f20
    virtual void FUN_00554ae0(); // [14] 0x154ae0
    virtual void FUN_00554f00(); // [15] 0x154f00
    virtual void FUN_00554be0(); // [16] 0x154be0
    virtual void FUN_00556e80(); // [17] 0x156e80
    virtual void FUN_00554f80(); // [18] 0x154f80
    virtual void FUN_00555160(); // [19] 0x155160
    virtual void FUN_00555280(); // [20] 0x155280
    virtual void FUN_00556ec0(); // [21] 0x156ec0
    virtual void FUN_005552b0(); // [22] 0x1552b0
    i32 m_0;
    i32 Anchor();
};
i32 CVtbl_1efd28::Anchor() {
    return m_0 != 0;
}
CVtbl_1efd28::~CVtbl_1efd28() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtbl_1efd28);
VTBL(CVtbl_1efd28, 0x001efd28);

// ---------------------------------------------------------------------------
// 0x5efd88 (RVA 0x1efd88) - 14 slots. CObject-style, slot 1 dtor 0x156f30.
// Slot 7 (0x163bc0) points at a CDDrawWorkerList method (declared-only).
// ---------------------------------------------------------------------------
struct CVtbl_1efd88 {
    virtual void FUN_005bef01(); // [0] 0x1bef01 (base thunk)
    virtual ~CVtbl_1efd88();     // [1] 0x156f30 scalar-deleting dtor (anchor)
    virtual void FUN_004028ec(); // [2] 0x0028ec (base thunk)
    virtual void FUN_0040106e(); // [3] 0x00106e (base thunk)
    virtual void FUN_00404034(); // [4] 0x004034 (base thunk)
    virtual void FUN_00556f00(); // [5] 0x156f00
    virtual void FUN_00556fc0(); // [6] 0x156fc0
    virtual void FUN_00563bc0(); // [7] 0x163bc0
    virtual void FUN_00556f20(); // [8] 0x156f20
    virtual void FUN_00556fd0(); // [9] 0x156fd0
    virtual void FUN_005573e0(); // [10] 0x1573e0
    virtual void FUN_00557330(); // [11] 0x157330
    virtual void FUN_00557150(); // [12] 0x157150
    virtual void FUN_00563bf0(); // [13] 0x163bf0
    i32 m_0;
    i32 Anchor();
};
i32 CVtbl_1efd88::Anchor() {
    return m_0 != 0;
}
CVtbl_1efd88::~CVtbl_1efd88() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtbl_1efd88);
VTBL(CVtbl_1efd88, 0x001efd88);

// ---------------------------------------------------------------------------
// 0x5efdc0 (RVA 0x1efdc0) - 17 slots. A SECOND vtable whose slot-1 dtor
// (0x157610 = CDDrawWorkerMapSmall::Stub_157610, already matched -> NOT redefined)
// is owned by CDDrawWorkerMapSmall (its primary is ??_7CDDrawWorkerMapSmall
// @0x1efcc8). Modeled here as a standalone tracking class (a per-class ??_7CVtbl_
// primary names the datum; realizing it AS the MI-secondary of CDDrawWorkerMapSmall
// would need the +offset construction-vtable machinery). CObject-style thunks.
// ---------------------------------------------------------------------------
struct CVtbl_1efdc0 {
    virtual void FUN_005bef01(); // [0] 0x1bef01 (base thunk)
    virtual ~CVtbl_1efdc0();     // [1] 0x157610 scalar-deleting dtor (anchor)
    virtual void FUN_004028ec(); // [2] 0x0028ec (base thunk)
    virtual void FUN_0040106e(); // [3] 0x00106e (base thunk)
    virtual void FUN_00404034(); // [4] 0x004034 (base thunk)
    virtual void FUN_005575e0(); // [5] 0x1575e0
    virtual void FUN_005576c0(); // [6] 0x1576c0
    virtual void FUN_005591e0(); // [7] 0x1591e0
    virtual void FUN_00557600(); // [8] 0x157600
    virtual void FUN_00559a70(); // [9] 0x159a70
    virtual void FUN_00559c90(); // [10] 0x159c90
    virtual void FUN_00559cc0(); // [11] 0x159cc0
    virtual void FUN_00559cf0(); // [12] 0x159cf0
    virtual void FUN_00559d40(); // [13] 0x159d40
    virtual void FUN_00559d90(); // [14] 0x159d90
    virtual void FUN_005591f0(); // [15] 0x1591f0
    virtual void FUN_00559f00(); // [16] 0x159f00
    i32 m_0;
    i32 Anchor();
};
i32 CVtbl_1efdc0::Anchor() {
    return m_0 != 0;
}
CVtbl_1efdc0::~CVtbl_1efdc0() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtbl_1efdc0);
VTBL(CVtbl_1efdc0, 0x001efdc0);

// ---------------------------------------------------------------------------
// 0x5eff70 (RVA 0x1eff70) - 11 slots. CObject-style, slot 1 dtor 0x159190.
// Slot 9 (0x1644a0) points at a CDDrawSurfacePair method (declared-only), so this
// is a surface-pair-adjacent worker vtable.
// ---------------------------------------------------------------------------
struct CVtbl_1eff70 {
    virtual void FUN_005bef01(); // [0] 0x1bef01 (base thunk)
    virtual ~CVtbl_1eff70();     // [1] 0x159190 scalar-deleting dtor (anchor)
    virtual void FUN_004028ec(); // [2] 0x0028ec (base thunk)
    virtual void FUN_0040106e(); // [3] 0x00106e (base thunk)
    virtual void FUN_00404034(); // [4] 0x004034 (base thunk)
    virtual void FUN_00559150(); // [5] 0x159150
    virtual void FUN_00401c08(); // [6] 0x001c08 (base thunk)
    virtual void FUN_005591d0(); // [7] 0x1591d0
    virtual void FUN_00559180(); // [8] 0x159180
    virtual void FUN_005644a0(); // [9] 0x1644a0
    virtual void FUN_005646b0(); // [10] 0x1646b0
    i32 m_0;
    i32 Anchor();
};
i32 CVtbl_1eff70::Anchor() {
    return m_0 != 0;
}
CVtbl_1eff70::~CVtbl_1eff70() {
    if (Anchor()) {
        m_0 = 0;
    }
}
SIZE_UNKNOWN(CVtbl_1eff70);
VTBL(CVtbl_1eff70, 0x001eff70);
