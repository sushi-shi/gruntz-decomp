// ButeStoreDtorCopies.h - the RVA-binding anchors for the per-obj-file COMDAT copies of
// CButeStore's (== zPTree's) INLINE Reset + destructor that have NO class of their own.
//
// zPTree's ~ and Reset are INLINE members (<Bute/ButeStore.h> -> <Bute/PTreeNode.h>). MSVC5
// without /Gy emits an inline member as a per-OBJECT-FILE STATIC (it folds only the vftable
// COMDAT, not the function), so retail carries extra copies at their own RVAs: ~ at 0x21310
// (butemgr, dispatched by zPTree's OWN vtable 0x1e94ac slot-0 sdd 0x212e0 via ILT 0x4372);
// Reset at 0x212a0 (butestoreclear). The delink pipeline is strictly one-symbol-per-RVA
// (build/labels.py) and verify_unique_names is a FATAL gate, so each extra copy needs its own
// mangled symbol: a thin anchor subclass that adds nothing, into which the inline body expands
// verbatim (stamp both vptrs, ClearRecursive(0) == the real ?ClearRecursive@CButeStore@@
// 0x16e070, then fold the +0x08 base ~CButeNodeEntry 0x16dfc0 and the +0x00 base
// ~CContainerErr 0x16da60). A single out-of-line ~zPTree instead would be WRONG twice over:
// derived dtors would CALL it instead of INLINING it, and it could bind only one RVA.
//
// TWO EX-ANCHORS DISSOLVED (2026-07-19, vtable-owner sdd proof): the other two "~ copies"
// were the real compiler-generated destructors of real subclasses - 0x174d70 is
// ~CButeNode (its vtable 0x1f051c slot-0 sdd 0x174d50 dispatches it; body in ButeNode.cpp)
// and 0x21570 is ~CBSecStream (vtable 0x1f0510 slot-0 sdd 0x174d30 via ILT 0x3567; body in
// ButeMgr.cpp). Their bodies are byte-identical to the true copy only because neither
// subclass adds destructible state. The anchors' RELOC_VTBL(0x1e94ac) bindings were the
// audit's 2 MISBOUND: the vtables that actually dispatch those dtors are the classes' own.
#ifndef SRC_BUTE_BUTESTOREDTORCOPIES_H
#define SRC_BUTE_BUTESTOREDTORCOPIES_H

#include <Bute/ButeStore.h> // the canonical CButeStore (== zPTree; real bases + inline dtor/Reset)
#include <rva.h>

// 0x21310 - the butemgr copy of ~CButeStore (body in ButeMgr.cpp).
struct CButeStoreDtorCopyMgrA : public CButeStore {
    ~CButeStoreDtorCopyMgrA();
};
SIZE(CButeStoreDtorCopyMgrA, 0x2c); // adds nothing to CButeStore

// 0x212a0 - the butestoreclear copy of CButeStore::Reset (body in ButeStoreClear.cpp).
struct CButeStoreResetCopyClear : public CButeStore {
    void ResetCopy(); // 0x212a0
};
SIZE(CButeStoreResetCopyClear, 0x2c);

// The /GX dtor-copy twin stamps zPTree's RTTI-named pair (0x1e94ac primary, header doc
// above); its cl-emitted ??_7 reloc-masks that bound table.
RELOC_VTBL(CButeStoreDtorCopyMgrA, 0x001e94ac);

#endif // SRC_BUTE_BUTESTOREDTORCOPIES_H
