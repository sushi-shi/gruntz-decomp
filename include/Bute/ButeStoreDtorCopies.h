// ButeStoreDtorCopies.h - the RVA-binding anchors for the byte-identical COMDAT copies of
// CButeStore's (== zPTree's) INLINE Reset + destructor.
//
// zPTree::Reset are INLINE members (<Bute/ButeStore.h> -> <Bute/PTreeNode.h>). MSVC5 without
// /Gy emits an inline member as a per-OBJECT-FILE STATIC (it folds only the vftable COMDAT,
// not the function), so retail carries N byte-identical copies at N distinct RVAs - one per
// TU that destroys/resets a store: ~ at 0x174d70 (butenode), 0x21310 + 0x21570 (butemgr);
// Reset at 0x212a0 (butestoreclear). All three ~ copies stamp the SAME vtable pair
// (0x1e94ac / 0x1e949c), the signature of this MSVC5 behaviour.
//
// The delink pipeline is strictly one-symbol-per-RVA (build/labels.py) and verify_unique_names
// is a FATAL gate, so N distinct RVAs REQUIRE N distinct mangled symbols. A single out-of-line
// ~zPTree (the "(B)-form flip") is therefore WRONG twice over: (1) it would make every derived
// dtor + every scope-exit CALL ~zPTree instead of INLINING it, contradicting retail's inline
// emission and breaking the whole Bute dtor family's matches; and (2) it binds ONE RVA and
// orphans the other N-1 copies. So each copy is anchored on its own thin subclass that adds
// nothing; the inline ~zPTree / Reset expands into each, reproducing the retail body verbatim
// (stamp both vptrs, ClearRecursive(0) == the real ?ClearRecursive@CButeStore@@ 0x16e070, then
// fold the +0x08 base ~CButeNodeEntry 0x16dfc0 and the +0x00 base ~CContainerErr 0x16da60).
//
// Each anchor name reflects its owning TU (Node/MgrA/MgrB/Clear), not an unrecovered identity -
// the class IS CButeStore. Bodies live in the owning TUs (RVA-bound); this header is decl-only.
// (Ex CButeStoreCopy<rva>: the hex was RVA-documentation that needlessly tripped the placeholder
// regex; the copies are faithful MSVC5-required anchors, so the metric hit was a false positive.)
#ifndef SRC_BUTE_BUTESTOREDTORCOPIES_H
#define SRC_BUTE_BUTESTOREDTORCOPIES_H

#include <Bute/ButeStore.h> // the canonical CButeStore (== zPTree; real bases + inline dtor/Reset)
#include <rva.h>

// 0x174d70 - the butenode copy of ~CButeStore (body + free-adapter in ButeNode.cpp).
struct CButeStoreDtorCopyNode : public CButeStore {
    ~CButeStoreDtorCopyNode();
};
SIZE(CButeStoreDtorCopyNode, 0x2c); // adds nothing to CButeStore

// 0x21310 / 0x21570 - the two butemgr copies of ~CButeStore (bodies in ButeMgr.cpp).
struct CButeStoreDtorCopyMgrA : public CButeStore {
    ~CButeStoreDtorCopyMgrA();
};
SIZE(CButeStoreDtorCopyMgrA, 0x2c);

struct CButeStoreDtorCopyMgrB : public CButeStore {
    ~CButeStoreDtorCopyMgrB();
};
SIZE(CButeStoreDtorCopyMgrB, 0x2c);

// 0x212a0 - the butestoreclear copy of CButeStore::Reset (body in ButeStoreClear.cpp).
struct CButeStoreResetCopyClear : public CButeStore {
    void ResetCopy(); // 0x212a0
};
SIZE(CButeStoreResetCopyClear, 0x2c);

#endif // SRC_BUTE_BUTESTOREDTORCOPIES_H
