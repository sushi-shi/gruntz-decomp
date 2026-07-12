// PTreeNode.h - the shared zPTree config-tree node family (Bute .bute parser).
// zPTree is multiply-derived (zErrHandling primary @+0x00, CButeNodeEntry second
// base @+0x08); REAL POLYMORPHIC per the ALL-VTABLES mandate (cl auto-stamps both
// most-derived vptrs in the derived ctor). Concrete nodes derive zPTree and get
// their own most-derived primary (+0x00) + second-base-in-derived (+0x08) vtables.
// Ctor bodies live in src/Bute/ButeNode.cpp; only the shapes are here so sibling
// TUs (e.g. ButeSectionCtor.cpp's CBSecStream) can derive zPTree too.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.
#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H
#include <rva.h>
#include <Ints.h>

extern void* g_buteNodeErrMsg; // DAT_006bf480 - the node's error-message global

struct CVariantSlot;  // <Bute/ButeTree.h> - the +0x04 error sink (Set 0x16d850)
struct CButeTreeNode; // <Bute/ButeTree.h> - the 0x14-byte crit-bit trie node

// zErrHandling - the container-library exception base (vptr@0, err-sink@4), RTTI-real
// (zPTree's base-class-array names its primary base `zErrHandling`). Ctor 0x16d9c0
// (real body in GameText), __thiscall(this,msg); external no-body here so the call
// reloc-masks. REAL POLYMORPHIC: vtbl@0 is the implicit vptr (virtual dtor).
class zErrHandling {
public:
    zErrHandling(void* msg);
    virtual ~zErrHandling(); // +0x00 vptr; external no-body dtor (real body in GameText)

    // +0x04 the error object the ctor seeds (&g_buteNodeErrMsg); the trie/store report
    // fatal lookups through it (CButeTree::Find calls m_errorSink->Set). void* in the
    // generic-base view; CVariantSlot* here (the .bute node/trie error sink).
    CVariantSlot* m_errorSink; // +0x04
};

// The node subobject at zPTree+0x8 (a small keyed-store entry): zPTree's SECOND
// polymorphic base. Ctor 0x16df70 auto-stamps ??_7CButeNodeEntry (retail 0x5f04d8)
// @+0x00, then desc@+4, (WORD)n@+8, 0@+0xc.
class CButeNodeEntry {
public:
    CButeNodeEntry(i32 n, void* desc);
    virtual ~CButeNodeEntry(); // +0x00 vptr; external no-body scalar-deleting dtor

    // +0x04 the node/keyed-store descriptor. Dual role of this one 0x10-byte second
    // base (RTTI `zPtrColl`): config-node kind descriptor (opaque tag ptr), OR the
    // keyed store's per-value callback fn-ptr (CButeStore::ClearRecursive fires it).
    void* m_desc;     // +0x04 (abs +0x0c)
    i16 m_kind;       // +0x08 (abs +0x10)  (WORD)n / store flag byte (bit 1 = has cb)
    char m_pada[2];
    i32 m_nodeCount;  // +0x0c (abs +0x14)  zero-init; trie live-node count (CButeTree)
};
SIZE(CButeNodeEntry, 0x10);       // { vptr, desc, kind, count }
VTBL(CButeNodeEntry, 0x001f04d8); // the entry member's own (base) vtable

// zPTree layout (two vptrs): +0x00 zErrHandling base, +0x08 CButeNodeEntry base
// (spans +0x08..+0x18), then the crit-bit trie / keyed-store fields (+0x18..+0x2c).
// Ctor 0x16dff0. The concrete derived views (CButeTree the trie, CButeStore the store,
// CButeNode/CBSecStream config nodes) share this one layout; config-node ctors just
// zero m_root/m_lookupPending, the trie/store methods use the whole field set.
class zPTree : public zErrHandling, public CButeNodeEntry {
public:
    virtual ~zPTree() OVERRIDE; // slot 0 (scalar-dtor 0x004372; overrides zErrHandling)
    zPTree(void* desc, i32 n);

    CButeTreeNode* m_root;          // +0x18  trie/store root (ctor zeroes it)
    CButeTreeNode* m_descentCursor; // +0x1c  crit-bit descent cursor (CButeTree)
    CButeTreeNode* m_candidateLeaf; // +0x20  candidate leaf (CButeTree)
    i32 m_keyBitLength;             // +0x24  strlen*8 + 7 (CButeTree)
    i32 m_lookupPending;            // +0x28  lookup-pending flag (ctor zeroes it)
};
SIZE(zPTree, 0x2c);              // measured: new(0x2c) -> ctor 0x16dff0
VTBL(zPTree, 0x001e94ac);        // ??_7zPTree@@6B@ (primary base vtable, RTTI-real)

#endif // SRC_BUTE_PTREENODE_H
