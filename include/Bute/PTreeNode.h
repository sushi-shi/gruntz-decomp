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

// zErrHandling - the container-library exception base (vptr@0, err-sink@4). RTTI names
// zPTree's primary base `zErrHandling`; that class IS <Wap32/zBitVec.h>'s CContainerErr
// (same ctor 0x16d9c0, same dtor 0x16da60, same one-slot vtable 0x1f04cc). The separate
// `class zErrHandling` that used to be declared here was a THIRD model of it, and it is
// what made every store/node destructor call a ??1zErrHandling that is defined nowhere.
// Folded onto the canonical class; the RTTI name is kept as an alias so the library-true
// spelling still reads in the sources that use it.
#include <Wap32/zBitVec.h> // the canonical CContainerErr (ctor 0x16d9c0 / dtor 0x16da60)
typedef CContainerErr zErrHandling;

// The node subobject at zPTree+0x8 (a small keyed-store entry): zPTree's SECOND
// polymorphic base. Ctor 0x16df70 auto-stamps ??_7CButeNodeEntry (retail 0x5f04d8)
// @+0x00, then desc@+4, (WORD)n@+8, 0@+0xc.
class CButeNodeEntry {
public:
    CButeNodeEntry(i32 n, void(__cdecl* teardown)(void*));
    virtual ~CButeNodeEntry(); // +0x00 vptr; real dtor 0x16dfc0 (defined in typekeycoll)

    // +0x04 the per-value teardown callback. This was `void* m_desc`, a "node descriptor"
    // whose dual role (opaque tag ptr / store callback) was never resolved. It is ONE
    // thing: a function pointer. CButeStore::ClearRecursive CALLS it on every node value
    // (`m_teardown(n->m_val)`), and the only value ever stored here is the address of
    // ButeValueTeardown (0x174df0) - the phantom `g_nodeDescriptor` / `g_node174df0Tag`
    // "globals" were that function's address all along.
    void(__cdecl* m_teardown)(void*); // +0x04 (abs +0x0c)
    i16 m_kind;                       // +0x08 (abs +0x10)  (WORD)n; bit 1 = "has teardown"
    char m_pada[2];
    i32 m_nodeCount; // +0x0c (abs +0x14)  zero-init; live-node count
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
    zPTree(void(__cdecl* teardown)(void*), i32 n);

    // The recursive keyed-node free (0x16e070, defined in src/Bute/TypeKeyColl.cpp).
    // node==0 starts from the root. This is the method the whole engine calls as
    // `CButeStore::ClearRecursive` - CButeStore IS zPTree (see the typedef in
    // <Bute/ButeStore.h>), so both spellings mangle to the one real symbol.
    void ClearRecursive(CButeTreeNode* node);

    // Reset-to-empty (inlined into CButeMgr::Parse; a standalone copy sits at 0x212a0).
    void Reset() {
        ClearRecursive(0);
        m_root = 0;
        m_lookupPending = 0;
        m_nodeCount = 0;
    }

    // INLINE, and that is load-bearing: retail carries THREE byte-identical copies of
    // this destructor (0x174d70 butenode, 0x21310 / 0x21570 butemgr) while sharing ONE
    // vtable pair - the signature of MSVC5 without /Gy, which emits an inline member as a
    // per-object-file static but keeps the vftable a folded COMDAT. Each copy is anchored
    // on a thin subclass; the body expands into all of them (stamp both vptrs, clear, then
    // fold the +0x08 base ~CButeNodeEntry and the +0x00 base ~CContainerErr).
    virtual ~zPTree() OVERRIDE {
        ClearRecursive(0);
    }

    CButeTreeNode* m_root;          // +0x18  trie/store root (ctor zeroes it)
    CButeTreeNode* m_descentCursor; // +0x1c  crit-bit descent cursor (CButeTree)
    CButeTreeNode* m_candidateLeaf; // +0x20  candidate leaf (CButeTree)
    i32 m_keyBitLength;             // +0x24  strlen*8 + 7 (CButeTree)
    i32 m_lookupPending;            // +0x28  lookup-pending / store reset field
};
SIZE(zPTree, 0x2c); // measured: new(0x2c) -> ctor 0x16dff0
// zPTree's two most-derived vtables. RTTI names 0x1e94ac `zPTree`; 0x1e949c is its
// CButeNodeEntry-base secondary. cl spells them through the ultimate polymorphic base, so
// the pins carry the through-base names (they live in src/Bute/ButeNode.cpp - labels.py
// reads @data-symbol from the .cpp only).

// ---------------------------------------------------------------------------
// CButeNode - the concrete .bute config-tree node: a per-tag keyed store. Both the
// out-of-line ctor at 0x174d00 (butenode) and ParseTagLine's inlined `new
// CButeNode(&ButeValueTeardown, 2)` (butemgr) build it, and BOTH stamp the same pair
// of most-derived vtables - primary 0x1f051c @+0x00, second-base-in-derived 0x1f0518
// @+0x08. That shared vtable pair is the proof that butenode's former
// `CButeCfgNode174d` and butemgr's `CButeNode` were ONE class, split across two TUs;
// they are folded here into the single shared definition, so both TUs emit the same
// ??_7CButeNode symbols and the two retail vtable cells bind.
//
// The `desc` every node is constructed with is NOT data: it is the address of the
// store's __cdecl per-value teardown callback (ButeValueTeardown @0x174df0), which
// CButeStore::ClearRecursive fires on each node's value. The former `g_nodeDescriptor`
// / `g_node174df0Tag` int/byte externs were phantoms for that function's address
// (0x174df0 lies in .text - a CODE address that had been modeled as data).
class CButeNode : public zPTree {
public:
    virtual ~CButeNode() OVERRIDE; // slot 0 (zPTree dtor); external no-body

    // 0x174d00 (butenode): the 1-arg form - passes the fixed teardown callback.
    CButeNode(i32 kind);
    // ParseTagLine inlines this 2-arg form (`new CButeNode(&ButeValueTeardown, 2)`).
    CButeNode(void(__cdecl* teardown)(void*), i32 n) : zPTree(teardown, n) {}
};
SIZE(CButeNode, 0x2c);       // new CButeNode(0x2c); zPTree provides the full layout
VTBL(CButeNode, 0x001f051c); // node primary (most-derived) vtable @+0x00
// (The two through-base ??_7CButeNode @data-symbol pins live in src/Bute/ButeNode.cpp -
//  labels.py reads @data-symbol out of the TU's .cpp only, never a header.)

#endif // SRC_BUTE_PTREENODE_H
