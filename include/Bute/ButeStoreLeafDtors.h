// ButeStoreLeafDtors.h - the OTHER TWO COPIES of the keyed store's inline destructor,
// the ones the linker placed in butemgr's block (0x21310 and 0x21570).
// INCLUDED ONLY BY src/Bute/ButeMgr.cpp.
//
// All three copies (these two plus 0x174d70 in butenode) are BYTE-IDENTICAL and stamp
// the SAME vtable pair (0x1e94ac / 0x1e949c). That is the signature of MSVC5 without
// /Gy: an inline member function is emitted as a per-object-file STATIC (so every obj
// that needs it keeps its own copy), while the vftable stays a COMDAT (so the linker
// folds it to one). Hence ONE vtable but THREE destructors - they are one retail class,
// <Bute/ButeStore.h>'s CButeStore, whose destructor is declared inline there.
//
// C++ cannot give one symbol three RVAs, so each copy is anchored on a thin subclass
// that adds nothing; the inline ~CButeStore expands into each one and reproduces the
// retail body exactly: stamp both most-derived vptrs, ClearRecursive(0), then fold the
// +0x08 base (~CButeNodeEntry 0x16dfc0, MI `this ? this+8 : 0` adjust) and the +0x00
// base (~CContainerErr 0x16da60).
//
// This replaces the former CButeStoreZPTree / CButeStoreSection / CButeNodeSecondBase
// stand-ins, which each declared their OWN ClearRecursive - a method defined nowhere in
// the tree, i.e. a guaranteed unresolved external at link. Inheriting the real one binds
// all three copies to the single ?ClearRecursive@CButeStore@@ at 0x16e070.
#ifndef SRC_BUTE_BUTESTORELEAFDTORS_H
#define SRC_BUTE_BUTESTORELEAFDTORS_H

#include <rva.h>
#include <Bute/ButeStore.h> // the canonical CButeStore (real bases + the inline dtor)

// 0x21310 - the second copy of ~CButeStore.
struct CButeStoreCopy21310 : public CButeStore {
    ~CButeStoreCopy21310();
};
SIZE(CButeStoreCopy21310, 0x2c); // adds nothing to CButeStore

// 0x21570 - the third copy of ~CButeStore.
struct CButeStoreCopy21570 : public CButeStore {
    ~CButeStoreCopy21570();
};
SIZE(CButeStoreCopy21570, 0x2c); // adds nothing to CButeStore

#endif // SRC_BUTE_BUTESTORELEAFDTORS_H
