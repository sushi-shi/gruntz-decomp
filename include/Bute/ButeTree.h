#ifndef SRC_BUTE_BUTETREE_H
#define SRC_BUTE_BUTETREE_H

#include <rva.h>
#include <Bute/PTreeNode.h> // the RTTI-real zErrHandling/CButeNodeEntry/zPTree base hierarchy

// The keyed error-handling slot: it is BOTH the variant slot (m_callback/word/tag/label,
// via Set) and the key-table cursor (Find/Add over the global g_recs23 table). The ex
// CKeyFinder "reduced view" was this same 0x18-byte object (byte-identical layout, and its
// Add IS this Add @0x16e360); that cast-based view is dissolved here
// (m_index->m_04, m_owner->m_label).
struct CVariantSlot {
    CVariantSlot(char* label);         // 0x16e1a0 (cursor ctor: typeTag=2, m_10=2)
    void Set(void* obj, void* item, i32 b); // 0x16d850  (item: the reported record/name)
    i32 Find(i32 key);                 // 0x16e1d0 (binary-search the g_recs23 key table)
    void* Add(void* key, void* val);   // 0x16e360 (keyed insert/update/remove; val==0 removes)
    void(__cdecl* m_callback)(char* buf, i32 v); // +0x00 (call [this]; the error callback)
    i32 m_04;        // +0x04 probe/found index (Find writes it; ex m_index)
    u16 m_valueWord; // +0x08 word storage
    u16 m_0a;        // +0x0a
    i32 m_typeTag;   // +0x0c type tag (1/2/4; the cursor ctor sets 2)
    i32 m_10;        // +0x10 (the cursor ctor sets 2)
    char* m_label;   // +0x14 label / format text / cursor owner (ex m_owner)
};
SIZE(0x18);

struct TypeKeyRec {
    i32 m_key; // +0x00  the key (CVariantSlot::Find subtracts the probe key from it)
    void* m_4; // +0x04  value, or the __cdecl set-fn Set dispatches (variant slot)
    short m_8; // flag / word slot
    short m_a;
};
SIZE_UNKNOWN();

struct CButeTreeNode {
    CButeTreeNode* m_child[2]; // +0x00 / +0x04
    i32 m_bit;                 // +0x08  crit-bit index
    char* m_key;               // +0x0c  owned key copy
    void* m_value;             // +0x10  stored value
};
SIZE(0x14);

class CButeTree : public zPTree {
public:
    CButeTree(void(__cdecl* teardown)(void*), i32 n);
    void* Find(const char* key);                // 0x16d190
    void* Insert(const char* key, void* value); // 0x16db90
    // Walk (0x193340) - invoke fn(key, value, ctx) for each node of the crit-bit
    // trie, recursing left (child[0]) and iterating right (child[1]) while a child's
    // crit-bit index still exceeds the node's; `node`==0 starts from m_root.
    void Walk(void(__cdecl* fn)(char* key, void* value, void* ctx), void* ctx, CButeTreeNode* node);

};
SIZE_UNKNOWN();
VTBL(CButeTree, 0x001f04e0); // ??_7CButeTree@@6B@ (1-slot scalar-deleting-dtor vtable)

extern CButeTree g_buteTree;

// The act-registry id convention: the tree VALUES for act keys are small integer
// ids stored in the void* slot. These wrappers keep that one reinterpret at the
// boundary instead of at every call site.
static inline i32 ActFindId(const char* key) {
    return reinterpret_cast<i32>(g_buteTree.Find(key));
}
static inline void ActInsertId(const char* key, i32 id) {
    g_buteTree.Insert(key, reinterpret_cast<void*>(id));
}

#endif // SRC_BUTE_BUTETREE_H
