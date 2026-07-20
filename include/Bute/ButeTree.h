// ButeTree.h - CButeTree, the engine's string-keyed crit-bit (PATRICIA) trie.
//
// The single shared keyed store the whole .att/.bute config layer AND the
// game-object type registry (g_buteTree @0x6bf620) funnel lookups through. A leaf
// node is 20 bytes: two child links, the crit-bit index, an owned key copy, and the
// stored value. Recovered from Find/Insert (src/Bute/ButeTree.cpp) and the
// ctor/dtor of g_buteTree (src/Gruntz/TypeKeyColl.cpp). Field names are
// placeholders; only the OFFSETS + emitted bytes are load-bearing.
//
// STRUCTURE-RECOVERY (RTTI-proven): CButeTree derives the RTTI-real `zPTree`
// (<Bute/PTreeNode.h>: zErrHandling @0, CButeNodeEntry/zPtrColl @8). The former fake
// single 0x2c-byte config-tree base every node view shares (CButeStore m_tree, CButeNode,
// CBSecStream). The trie fields (m_root/m_descentCursor/... at +0x18.., m_nodeCount@+0x14,
// m_errorSink@+0x04) now live on zPTree/zErrHandling/CButeNodeEntry; CButeTree adds none.
#ifndef SRC_BUTE_BUTETREE_H
#define SRC_BUTE_BUTETREE_H

#include <rva.h>
#include <Bute/PTreeNode.h> // the RTTI-real zErrHandling/CButeNodeEntry/zPTree base hierarchy

// The +0x04 error sink the trie/registry/container reports a fatal failure through.
// __thiscall(this; obj, a, b); the delinker names 0x16d850
// ?Set@CVariantSlot@@QAEXPAXHH@Z. The registry seeds its +0x00 callback slot and
// stores a probe index / word / type-tag / label (used by the hot Set switch). This
// is ALSO zErrHandling::m_errSink (<Wap32/zBitVec.h>): the container OOM paths call
// Set here, and ~zErrHandling unregisters via Remove (0x16e360) - the old
// zErrRegistry/Reg23 view of the same +0x04 object is folded in.
SIZE_UNKNOWN(CVariantSlot);
struct CVariantSlot {
    void Set(void* obj, i32 a, i32 b);           // 0x16d850
    i32 Remove(void* obj, i32 flag);             // 0x16e360 (~zErrHandling unregister)
    void(__cdecl* m_callback)(char* buf, i32 v); // +0x00 (call [this])
    i32 m_04;                                    // +0x04 probe index slot
    u16 m_08;                                    // +0x08 word storage
    u16 m_0a;                                    // +0x0a
    i32 m_0c;                                    // +0x0c type tag (1/2/4)
    i32 m_10;                                    // +0x10
    char* m_14;                                  // +0x14 label / format text
};

// The sorted key table's 12-byte record {key, value/fn, flag-word} @0x6bf498. Find
// reads it as a flat i32[] (stride 3); CVariantSlot::Set dispatches through the value/fn
// member at +4; the count @0x6bf618 doubles as Set's probe-enable gate. Owned by
// TypeKeyColl.cpp (g_recs23[32] + g_recCount23).
SIZE_UNKNOWN(TypeKeyRec);
struct TypeKeyRec {
    i32 m_key; // +0x00  the key (CKeyFinder::Find subtracts the probe key from it)
    void* m_4; // +0x04  value, or the __cdecl set-fn Set dispatches (variant slot)
    short m_8; // flag / word slot
    short m_a;
};

// CKeyFinder - the binary-search cursor over the sorted global key table (the 12-byte-
// stride record array @0x6bf498 with its count @0x6bf618). CVariantSlot::Set drives it
// as a cursor over its own +0x04 slot (the probe index), and ~zErrHandling reaches Add
// through the error-sink; both are CVariantSlot-family objects reinterpreted through this
// cursor facet (the +0x04 index slot is shared). Bodies in TypeKeyColl.cpp.
SIZE_UNKNOWN(CKeyFinder);
struct CKeyFinder {
    char _vft0[4];                   // +0x00 base object vptr (reduced view; not owned/dispatched)
    i32 m_index;                     // +0x04  found index / insertion point (the ex-Reg23 m_4)
    u16 m_08;                        // +0x08
    u16 m_0a;                        // +0x0a  (padding)
    i32 m_0c;                        // +0x0c  = 2
    i32 m_10;                        // +0x10  = 2
    void* m_owner;                   // +0x14
    CKeyFinder(void* owner);         // 0x16e1a0
    i32 Find(i32 key);               // 0x16e1d0
    void* Add(void* key, void* val); // 0x16e360
};

// One crit-bit trie node (20 bytes).
SIZE(CButeTreeNode, 0x14);
struct CButeTreeNode {
    CButeTreeNode* m_child[2]; // +0x00 / +0x04
    i32 m_bit;                 // +0x08  crit-bit index
    char* m_key;               // +0x0c  owned key copy
    void* m_value;             // +0x10  stored value
};

// CButeTree - the string-keyed crit-bit trie, a concrete zPTree-derived view. Its
// runtime primary/second vtables (0x5f04e0 @+0, 0x5f04dc @+8) are the CButeTree-most-
// derived stamps the ctor writes over zPTree's (0x5e94ac/0x5e949c); ~CButeTree's
// ??_G (0x16e9c0) restores zPTree's (the construction/destruction vtable transition of
// CButeTree : zPTree). Adds no fields - the trie state lives on the zPTree base.
class CButeTree : public zPTree {
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

// The engine's shared game-object type registry / keyed config trie singleton
// (?g_buteTree@@3VCButeTree@@A @0x6bf620); dynamic-init'd by retail's initializer
// @0x16e690, constructed/destructed in src/Gruntz/TypeKeyColl.cpp. Declared here (its
// type's owner header) so consumers reach it via <Bute/ButeTree.h> (or <Bute/ButeMgr.h>,
// which includes this) instead of a per-TU extern.
extern CButeTree g_buteTree;

// --- vtable catalog ---

#endif // SRC_BUTE_BUTETREE_H
