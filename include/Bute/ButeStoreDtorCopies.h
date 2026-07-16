// ButeStoreDtorCopies.h - the RVA-binding anchors for the byte-identical COMDAT copies of
// CButeStore's (== zPTree's) INLINE Reset + destructor.
//
// WHY THESE EXIST (and why they CANNOT be dissolved into one out-of-line def): ~zPTree and
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
// The hex in each name is the copy's RVA (documentation), not an unrecovered identity - the
// class IS CButeStore. Bodies live in the owning TUs (RVA-bound); this header is decl-only.
#ifndef SRC_BUTE_BUTESTOREDTORCOPIES_H
#define SRC_BUTE_BUTESTOREDTORCOPIES_H

#include <Bute/ButeStore.h> // the canonical CButeStore (== zPTree; real bases + inline dtor/Reset)
#include <rva.h>

// 0x174d70 - the butenode copy of ~CButeStore (body + free-adapter in ButeNode.cpp).
struct CButeStoreCopy174d : public CButeStore {
    ~CButeStoreCopy174d();
};
SIZE(CButeStoreCopy174d, 0x2c); // adds nothing to CButeStore

// 0x21310 / 0x21570 - the two butemgr copies of ~CButeStore (bodies in ButeMgr.cpp).
struct CButeStoreCopy21310 : public CButeStore {
    ~CButeStoreCopy21310();
};
SIZE(CButeStoreCopy21310, 0x2c);

struct CButeStoreCopy21570 : public CButeStore {
    ~CButeStoreCopy21570();
};
SIZE(CButeStoreCopy21570, 0x2c);

// 0x212a0 - the butestoreclear copy of CButeStore::Reset (body in ButeStoreClear.cpp).
struct CButeStoreResetCopy212a0 : public CButeStore {
    void ResetCopy(); // 0x212a0
};
SIZE(CButeStoreResetCopy212a0, 0x2c);

#endif // SRC_BUTE_BUTESTOREDTORCOPIES_H
