// ButeTree.h - CButeTree, the engine's string-keyed crit-bit (PATRICIA) trie.
//
// The single shared keyed store the whole .att/.bute config layer AND the
// game-object type registry (g_buteTree @0x6bf620) funnel lookups through. A leaf
// node is 20 bytes: two child links, the crit-bit index, an owned key copy, and the
// stored value. Recovered from Find/Insert (src/Bute/ButeTree.cpp) and the
// ctor/dtor of g_buteTree (src/Gruntz/TypeKeyColl.cpp). Field names are
// placeholders; only the OFFSETS + emitted bytes are load-bearing.
//
// NOTE: CButeMgr's owned sub-trees (m_tree/m_tree48/m_tree74) are instances of
// THIS class, but ButeMgr.h models them from the /GX destructor's teardown view as
// `struct CButeStore` (also real-polymorphic MI now, two bases at +0/+8). CButeStore
// and CButeTree are the SAME 0x2c-byte class; fully folding CButeStore into CButeTree
// is a deep re-match of CButeMgr::~CButeMgr (0x213c0) and is deferred. See the
// dedup report / docs.
#ifndef SRC_BUTE_BUTETREE_H
#define SRC_BUTE_BUTETREE_H

#include <rva.h>

// The +0x04 error sink the trie/registry/container reports a fatal failure through.
// __thiscall(this; obj, a, b); the delinker names 0x16d850
// ?Set@CVariantSlot@@QAEXPAXHH@Z. The registry seeds its +0x00 callback slot and
// stores a probe index / word / type-tag / label (used by the hot Set switch). This
// is ALSO CContainerErr::m_errSink (<Wap32/zBitVec.h>): the container OOM paths call
// Set here, and ~CContainerErr unregisters via Remove (0x16e360) - the old
// zErrRegistry/Reg23 view of the same +0x04 object is folded in.
SIZE_UNKNOWN(CVariantSlot);
struct CVariantSlot {
    void Set(void* obj, i32 a, i32 b);           // 0x16d850
    i32 Remove(void* obj, i32 flag);             // 0x16e360 (~CContainerErr unregister)
    void(__cdecl* m_callback)(char* buf, i32 v); // +0x00 (call [this])
    i32 m_04;                                    // +0x04 probe index slot
    u16 m_08;                                    // +0x08 word storage
    u16 m_0a;                                    // +0x0a
    i32 m_0c;                                    // +0x0c type tag (1/2/4)
    i32 m_10;                                    // +0x10
    char* m_14;                                  // +0x14 label / format text
};

// One crit-bit trie node (20 bytes).
SIZE(CButeTreeNode, 0x14);
struct CButeTreeNode {
    CButeTreeNode* m_child[2]; // +0x00 / +0x04
    i32 m_bit;                 // +0x08  crit-bit index
    char* m_key;               // +0x0c  owned key copy
    void* m_value;             // +0x10  stored value
};

// The +0x08 second-base sub-object dtor (0x16dfc0, __thiscall, `this ? this+8 : 0`
// adjust). Modeled as a tiny receiver so the masked `this+8` lands in ecx.
struct CButeTreeBase2 {
    void Dtor(); // 0x16dfc0
};
SIZE(CButeTreeBase2, 0x24); // receiver view of the +0x08 second base (spans +0x08..+0x2c)

// CButeTree is multiply-derived (two vptrs), the same 0x2c-byte shape as CButeStore.
// REAL POLYMORPHIC (ALL-VTABLES): a primary base at +0x00 and a second base at
// +0x08. The two runtime/dtor-phase vptr stamps (TypeKeyColl.cpp) write these via raw
// casts; making the class polymorphic keeps the fields flat but drops the manual vptr
// field names.
struct CButeTreePrimary {
    virtual ~CButeTreePrimary(); // +0x00 slot 0 (primary vptr dtor)
    CVariantSlot* m_errorSink;   // +0x04
};
SIZE(CButeTreePrimary, 0x8); // { vptr, error-sink }
struct CButeTreeSecond {
    virtual void S0();              // +0x00 (this+0x08)  second-base vptr
    char m_pad0c[0x14 - 0x0c];      // +0x04 (+0x0c)
    i32 m_nodeCount;                // +0x08 (+0x14)
    CButeTreeNode* m_root;          // +0x0c (+0x18)
    CButeTreeNode* m_descentCursor; // +0x10 (+0x1c)
    CButeTreeNode* m_candidateLeaf; // +0x14 (+0x20)
    i32 m_keyBitLength;             // +0x18 (+0x24)  strlen*8 + 7
    i32 m_lookupPending;            // +0x1c (+0x28)
};
SIZE(CButeTreeSecond, 0x20); // second base (spans +0x08..+0x28 of CButeTree)

class CButeTree : public CButeTreePrimary, public CButeTreeSecond {
public:
    virtual ~CButeTree() OVERRIDE;              // slot 0 (scalar-dtor 0x16e9c0)
    void* Find(const char* key);                // 0x16d190
    void* Insert(const char* key, void* value); // 0x16db90
    // Walk (0x193340) - invoke fn(key, value, ctx) for each node of the crit-bit
    // trie, recursing left (child[0]) and iterating right (child[1]) while a child's
    // crit-bit index still exceeds the node's; `node`==0 starts from m_root.
    void Walk(void(__cdecl* fn)(char* key, void* value, void* ctx), void* ctx, CButeTreeNode* node);

    // g_buteTree ctor/dtor (TypeKeyColl.cpp). Construct runs the deeper base ctor;
    // ClearRecursive frees the keyed nodes; BaseDtor is the primary-base teardown;
    // scalar-dtor is the `scalar deleting destructor'. All reloc-masked __thiscall.
    void Construct(void* arg, i32 b); // 0x16dff0
    void ClearRecursive(i32 recurse); // 0x16e070
    void BaseDtor();                  // 0x16da60
};
SIZE_UNKNOWN(CButeTree);
VTBL(CButeTree, 0x001f04e0); // ??_7CButeTree@@6B@ (1-slot scalar-deleting-dtor vtable)

// --- vtable catalog ---

#endif // SRC_BUTE_BUTETREE_H
