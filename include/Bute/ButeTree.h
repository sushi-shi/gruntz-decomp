#ifndef SRC_BUTE_BUTETREE_H
#define SRC_BUTE_BUTETREE_H

#include <rva.h>

#include <AddrWord.h>
#include <Bute/PTreeNode.h>
#include <Enums.h>

typedef void(__cdecl* VariantCallback)(i32 key, i32 value);

GZ_ENUM_BEGIN(VariantSlotKind)
    VARIANT_SLOT_RECORD_VALUE = 1,
    VARIANT_SLOT_CALLBACK = 2,
    VARIANT_SLOT_DIRECT_VALUE = 4
GZ_ENUM_END(VariantSlotKind)

struct CVariantSlot {
    CVariantSlot(char* label);
    void Set(void* obj, void* item, i32 b);
    i32 Find(i32 key);
    void* Add(void* key, void* callback);
    void(__cdecl* m_callback)(char* buf, i32 v);
    i32 m_searchIndex;
    u16 m_valueWord;
    VariantSlotKind m_typeTag;
    i32 m_reserved10;
    char* m_label;
};

struct TypeKeyRec {
    TypeKeyRec() {}
    i32 m_key;
    VariantCallback m_callback;
    short m_value;
    char m_pad0a[2];
};

struct CButeTreeNode {
    CButeTreeNode* m_child[2];
    i32 m_bit;
    char* m_key;
    // Generic payload: the tag tables store CButeNode*, group nodes store
    // CButeValue*, g_buteTree stores an id packed into the pointer.
    char* m_value;
};

class CButeTree : public zPTree {
public:
    CButeTree(void(__cdecl* teardown)(void*), i32 n) : zPTree(teardown, n) {}
};

// g_buteTree's teardown callback; the tree owns no node payloads.
void ButeTreeNopFree(void*);

extern CButeTree g_buteTree;

static inline i32 ActFindId(const char* key) {
    AddrWord<char> v;
    v.m_addr = static_cast<char*>(g_buteTree.Find(key));
    return v.m_word;
}
static inline void ActInsertId(const char* key, i32 id) {
    AddrWord<char> v;
    v.m_word = id;
    g_buteTree.Insert(key, v.m_addr);
}

#endif // SRC_BUTE_BUTETREE_H
