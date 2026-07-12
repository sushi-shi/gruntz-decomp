// ButeStoreLeafDtors.h - the two out-of-line /GX destructors homed in ButeMgr.cpp at
// 0x21310 / 0x21570. INCLUDED ONLY BY src/Bute/ButeMgr.cpp (it pulls <Wap32/zBitVec.h>
// for the REAL CContainerErr, which must not spread into the widely-shared ButeStore.h).
//
// RTTI-proven identities (matcher R54, vtable-slot + xref):
//   0x21310 = zPTree::~zPTree           (??_7zPTree@@6B@ 0x1e94ac slot0 -> ??_G 0x212e0 -> here)
//   0x21570 = CBSecStream::~CBSecStream (??_7CBSecStream 0x1f0510 slot0 -> ??_G 0x174d30 -> here)
// Both are byte-identical to CButeStore::~CButeStore (0x174d70, butenode): they stamp the two
// base vtables (0x5e94ac @+0 / 0x5e949c @+8), run the recursive keyed-node free (0x16e070),
// then auto-fold the second base (0x16dfc0, MI this-adjust null guard) and the first base
// (0x16da60). The two non-trivial bases are what earn the /GX frame.
//
// The real derived classes zPTree / CBSecStream are modeled in <Bute/PTreeNode.h> /
// <Bute/ButeSection.h>, but those pull PTreeNode.h's real `zPTree`, which ODR-clashes with
// ButeMgr.h's regalloc-frozen `zPTree` stand-in (the documented dual-model split). So the two
// leaves are homed here over the retail base sub-objects that ARE reachable in this TU.
//
// VTABLES: the two vptr stamps reloc-mask the ONE proper vtbl each already carries -
// VTBL(zPTree, 0x1e94ac) and VTBL(CButeStore, 0x1e949c) (both in ButeMgr.h). The leaves add
// no per-class vtbl annotation (a second VTBL/@data-symbol on those rvas would collide -
// one-name-per-rva - and evict zPTree/CButeStore/butenode; RELOC_VTBL alias shells removed).
#ifndef SRC_BUTE_BUTESTORELEAFDTORS_H
#define SRC_BUTE_BUTESTORELEAFDTORS_H

#include <rva.h>
#include <Wap32/zBitVec.h>  // REAL CContainerErr - the +0 container-error base (dtor 0x16da60)
#include <Bute/PTreeNode.h> // REAL CButeNodeEntry - the +8 second base (dtor 0x16dfc0)
#include <Bute/ButeStore.h> // CButeStoreNode - the recursive-clear node argument

// (The `CButeNodeSecondBase` phantom is GONE. Its real identity is CButeNodeEntry
//  (<Bute/PTreeNode.h>) - the +0x08 sub-object the whole store/config-node family
//  multiply-inherits - proven by its destructor at 0x16dfc0, whose 7 bytes are exactly
//  `mov [ecx],offset ??_7CButeNodeEntry@@6B@ (0x5f04d8); ret`. That dtor is now DEFINED
//  (src/Bute/TypeKeyColl.cpp), so the two leaf destructors' +8 base-dtor calls bind.
//  The old "PTreeNode.h is walled out of a ButeMgr.h TU" note was stale - ButeMgr.h
//  includes PTreeNode.h directly now, so the real class is simply used.)

// 0x21310 - a CContainerErr(+0), CButeNodeEntry(+8) store leaf.
// @identity-TODO: this and its twin below are, by the bytes, the SAME retail class as
// butenode's CButeStore (all three stamp the one vtable pair 0x1e94ac / 0x1e949c, which
// RTTI names ??_7zPTree). Retail carries three copies of that one destructor; C++ cannot
// give one symbol three RVAs, so the three keep distinct names here and, per rva, only
// one vptr-stamp reloc can name-bind. Likewise ClearRecursive: it is
// ?ClearRecursive@CButeStore@@ @0x16e070 (7 external callers pin that name), so under
// this leaf's own name the call stays reloc-masked/positional. Folding the family onto
// `zPTree` is the real fix and is reported as cross-lane deferred work.
struct CButeStoreZPTree : public CContainerErr, public CButeNodeEntry {
    void ClearRecursive(CButeStoreNode*); // 0x16e070
    ~CButeStoreZPTree();                  // 0x21310 (overrides CContainerErr's virtual ~)
};
SIZE_UNKNOWN(CButeStoreZPTree);

// 0x21570 - same shape as the twin above (the third copy of the one store destructor).
struct CButeStoreSection : public CContainerErr, public CButeNodeEntry {
    void ClearRecursive(CButeStoreNode*); // 0x16e070
    ~CButeStoreSection();                 // 0x21570
};
SIZE_UNKNOWN(CButeStoreSection);

#endif // SRC_BUTE_BUTESTORELEAFDTORS_H
