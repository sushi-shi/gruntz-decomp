// ButeStore.h - CButeStore, the .bute keyed store.
//
// CButeStore IS zPTree. The binary says so: the store's destructor (all three copies of
// it - 0x174d70, 0x21310, 0x21570) stamps the vtable pair 0x1e94ac / 0x1e949c, and RTTI
// names 0x1e94ac `zPTree`; the zPTree constructor (0x16dff0) stamps the same pair. Same
// vtables, same 0x2c layout, same two bases (zErrHandling @0x00 + CButeNodeEntry @0x08)
// - one class, two reconstruction names. It is now ONE class: `CButeStore` is an alias,
// so the ~dozen call sites across the engine that spell it CButeStore keep compiling and
// mangle to the single real symbol (?ClearRecursive@zPTree@@ @0x16e070).
//
// What this dissolves: the CButeStorePrimary / CButeStoreSecond / CButeStoreBase2
// stand-ins (fabricated shells for the two REAL bases - they are why the store
// destructors called a ??1CButeStorePrimary / ?RestoreVptr@CButeStoreBase2@@ that is
// defined nowhere), and the store's fabricated "own" fields at +0x0c/+0x10/+0x14, which
// are CButeNodeEntry's m_teardown / m_kind / m_nodeCount.
#ifndef SRC_BUTE_BUTESTORE_H
#define SRC_BUTE_BUTESTORE_H

#include <rva.h>
#include <Ints.h>
#include <Bute/PTreeNode.h> // zPTree - the real class (bases, fields, ClearRecursive, Reset)

// The keyed-store node IS the crit-bit trie node: both are the same 0x14 bytes with the
// same fields (two child links, the order/crit-bit key, an owned name string, the stored
// value). The separate `CButeStoreNode` shape was a duplicate of <Bute/ButeTree.h>'s
// CButeTreeNode; it is an alias now, so there is one node type.
typedef CButeTreeNode CButeStoreNode;

typedef zPTree CButeStore;

#endif // SRC_BUTE_BUTESTORE_H
