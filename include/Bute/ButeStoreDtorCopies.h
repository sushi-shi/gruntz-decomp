// ButeStoreDtorCopies.h - the LAST RVA-binding anchor for a per-obj-file copy of a
// CButeStore (== zPTree) INLINE member.
//
// zPTree's ~ and Reset are INLINE members (<Bute/ButeStore.h> -> <Bute/PTreeNode.h>). MSVC5
// without /Gy emits an inline member per object file (it folds only the vftable COMDAT, not
// the function), so retail carries extra copies at their own RVAs.
//
// THE ANCHOR PATTERN IS RETIRED for copies our own cl also emits: when the base obj already
// carries the compiler-emitted symbol (??1zPTree in butemgr), an RVA_COMPGEN pin in the
// owning .cpp names the retail copy directly and NO anchor class is needed - see
// ButeMgr.cpp 0x212e0/0x21310/0x21600 (the ~zPTree sdd/dtor/adjustor trio)
// and 0x21570 (~CBSecStream). The two ex "dtor copy" siblings were real subclasses' dtors
// all along (0x174d70 = ~CButeNode, 0x21570 = ~CBSecStream; vtable-owner sdd proof).
//
// ONE anchor remains: the 0x212a0 Reset copy - AND IT IS A FAKE VIEW with a known
// death date. The retail function map places 0x212a0 at the tail of CHATBOXOWNER's
// band (HitTest 0x21140 precedes it; butemgr's COMDAT run 0x212e0+ follows), so the
// copy is almost certainly chatboxowner's own COMDAT tail: retail's known caller is
// CChatBoxOwner::ProcessCheatInput (`call 0x212a0` - MSVC5 declined to inline the
// inline-defined Reset inside that /GX body, exactly the mixed inline/call behavior
// measured on the fader/SBI ctors), and the 1-fn "butestoreclear" unit is a partition
// artifact of this anchor. DISSOLUTION PATH: when ProcessCheatInput (0.2%, final-sweep
// redo) is reconstructed, its non-inlined Reset() call makes our chatboxowner.obj emit
// the ?Reset@zPTree@@ COMDAT; then pin `RVA_COMPGEN(0x000212a0, 0, ?Reset@zPTree@@QAEXXZ)`
// there, delete this anchor + ButeStoreClear.cpp + the butestoreclear unit. Until that
// emitter exists, the anchor method below reproduces the expansion verbatim
// (ClearRecursive(0), then zero the root / the +0x28 field / node count) and owns the
// RVA. It is a plain method on a ctor/dtor-free subclass: no vtable emission.
#ifndef SRC_BUTE_BUTESTOREDTORCOPIES_H
#define SRC_BUTE_BUTESTOREDTORCOPIES_H

#include <Bute/ButeStore.h> // the canonical CButeStore (== zPTree; real bases + inline dtor/Reset)
#include <rva.h>

struct CButeStoreResetCopyClear : public CButeStore {
    void ResetCopy(); // 0x212a0
};
SIZE(CButeStoreResetCopyClear, 0x2c);

#endif // SRC_BUTE_BUTESTOREDTORCOPIES_H
