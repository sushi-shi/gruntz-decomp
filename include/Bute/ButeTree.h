#ifndef SRC_BUTE_BUTETREE_H
#define SRC_BUTE_BUTETREE_H

#include <rva.h>

#include <AddrWord.h>
#include <Bute/PTreeNode.h>
#include <Enums.h>

typedef void(__cdecl* VariantCallback)(char* message, i32 value);

GZ_ENUM_BEGIN(VariantSlotKind)
    VARIANT_SLOT_RECORD_VALUE = 1,
    VARIANT_SLOT_CALLBACK = 2,
    VARIANT_SLOT_DIRECT_VALUE = 4
GZ_ENUM_END(VariantSlotKind)

struct CVariantSlot {
    CVariantSlot(char* label);
    void Set(zErrHandling* obj, char* item, i32 b);
    CVariantSlot* EnsureTmErrorCallback();
    i32 Find(i32 key);
    void* Add(void* key, void* value);
    VariantCallback m_callback;
    i32 m_searchIndex;
    u16 m_valueWord;
    VariantSlotKind m_typeTag;
    i32 m_reserved10;
    char* m_label;
};

inline void zErrHandling::handle(const char* message, i32 code) const {
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(const_cast<zErrHandling*>(this), const_cast<char*>(message), code);
}

struct TypeKeyRec {
    TypeKeyRec() {}
    i32 m_key;
    VariantCallback m_callback;
    short m_value;
    char m_pad0a[2];
};

class CButeTree : public zPTree {
public:
    CButeTree(dtorf_t destructor, cleanup_behaviour cleanup) : zPTree(destructor, cleanup) {}

    void* lookup(const char* key) {
        return zPTree::lookup(key);
    }

    void* add(const char* key, void* value) {
        return zPTree::add(key, value);
    }
};

void ButeTreeNopFree(void*);

extern CButeTree g_buteTree;

static inline i32 ActFindId(const char* key) {
    AddrWord<char> v;
    v.m_addr = static_cast<char*>(g_buteTree.lookup(key));
    return v.m_word;
}
static inline void ActInsertId(const char* key, i32 id) {
    AddrWord<char> v;
    v.m_word = id;
    g_buteTree.add(key, v.m_addr);
}

#endif // SRC_BUTE_BUTETREE_H
