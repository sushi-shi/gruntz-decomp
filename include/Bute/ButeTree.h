#ifndef SRC_BUTE_BUTETREE_H
#define SRC_BUTE_BUTETREE_H

#include <rva.h>
#include <AddrWord.h>
#include <Bute/PTreeNode.h>

typedef void(__cdecl* VariantCallback)(i32 key, i32 value);

struct CVariantSlot {
    CVariantSlot(char* label);
    void Set(void* obj, void* item, i32 b);
    i32 Find(i32 key);
    void* Add(void* key, void* callback);
    void(__cdecl* m_callback)(char* buf, i32 v);
    i32 m_04;
    u16 m_valueWord;
    u16 m_0a;
    i32 m_typeTag;
    i32 m_10;
    char* m_label;
};
SIZE(0x18);

struct TypeKeyRec {
    i32 m_key;
    VariantCallback m_callback;
    short m_8;
    short m_a;
};
SIZE_UNKNOWN();

struct CButeTreeNode {
    CButeTreeNode* m_child[2];
    i32 m_bit;
    char* m_key;
    char* m_value;
};
SIZE(0x14);

class CButeTree : public zPTree {
public:
    CButeTree(void(__cdecl* teardown)(void*), i32 n);
};
SIZE_UNKNOWN();
VTBL(CButeTree, 0x001f04e0);

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
