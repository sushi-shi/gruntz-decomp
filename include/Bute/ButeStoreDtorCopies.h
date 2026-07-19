// ButeStoreDtorCopies.h - the LAST RVA-binding anchor for a per-obj-file copy of a
// CButeStore (== zPTree) INLINE member.
//
// zPTree's ~ and Reset are INLINE members (<Bute/ButeStore.h> -> <Bute/PTreeNode.h>). MSVC5
// without /Gy emits an inline member per object file (it folds only the vftable COMDAT, not
// the function), so retail carries extra copies at their own RVAs.
//
// THE ANCHOR PATTERN IS RETIRED for copies our own cl also emits: when the base obj already
// carries the compiler-emitted symbol (??1zPTree in butemgr), a `// @rva-symbol:` pin in the
// owning .cpp names the retail copy directly and NO anchor class (and no RELOC_VTBL alias)
// is needed - see ButeMgr.cpp 0x212e0/0x21310/0x21600 (the ~zPTree sdd/dtor/adjustor trio)
// and 0x21570 (~CBSecStream). The two ex "dtor copy" siblings were real subclasses' dtors
// all along (0x174d70 = ~CButeNode, 0x21570 = ~CBSecStream; vtable-owner sdd proof).
//
// ONE anchor remains: butestoreclear's Reset copy. There the TU's only body IS the copy -
// our cl INLINES Reset() into whatever calls it and emits no out-of-line ?Reset@zPTree@@
// symbol for a pin to name, so the anchor method below reproduces the expansion verbatim
// (ClearRecursive(0), then zero the root / the +0x28 field / node count) and owns the RVA.
// It is a plain method on a ctor/dtor-free subclass: no vtable emission, no RELOC_VTBL.
#ifndef SRC_BUTE_BUTESTOREDTORCOPIES_H
#define SRC_BUTE_BUTESTOREDTORCOPIES_H

#include <Bute/ButeStore.h> // the canonical CButeStore (== zPTree; real bases + inline dtor/Reset)
#include <rva.h>

// 0x212a0 - the butestoreclear copy of CButeStore::Reset (body in ButeStoreClear.cpp).
struct CButeStoreResetCopyClear : public CButeStore {
    void ResetCopy(); // 0x212a0
};
SIZE(CButeStoreResetCopyClear, 0x2c);

#endif // SRC_BUTE_BUTESTOREDTORCOPIES_H
