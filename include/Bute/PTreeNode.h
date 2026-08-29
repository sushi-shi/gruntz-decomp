#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Wap32/zBitVec.h>

#include <stddef.h>

struct CVariantSlot;
struct CButeValue;
extern CVariantSlot g_rezArchiveErrorSlot;

GZ_ENUM_CONST_BEGIN(PTreeBitLayout)
    PTREE_BITS_PER_BYTE = 8,
    PTREE_BYTE_BIT_SHIFT = 3,
    PTREE_BYTE_BIT_MASK = 7
GZ_ENUM_CONST_END(PTreeBitLayout)

class zPTreeNode {
    friend class zPTree;

    zPTreeNode*& ptr(i32 d) {
        return d ? right : left;
    }

    zPTreeNode* left;
    zPTreeNode* right;
    i32 index;
    char* symbol;
    void* body;
};

class zPtrColl {
public:
    zPtrColl(i32 n, void(__cdecl* teardown)(void*));
    virtual ~zPtrColl();

    void(__cdecl* m_teardown)(void*);
    i16 m_kind;
    char m_pada[2];
    i32 m_nodeCount;
};

class zPTree : public zErrHandling, public zPtrColl {
public:
    zPTree(void(__cdecl* teardown)(void*), i32 n);

    void ClearRecursive(zPTreeNode* node);

    RVA(0x000212a0, 0x21)
    void Reset() {
        ClearRecursive(NULL);
        root = NULL;
        preview = false;
        m_nodeCount = 0;
    }

    virtual ~zPTree() OVERRIDE {
        ClearRecursive(NULL);
    }

    void* Find(const char* key);

    void* Insert(const char* key, void* value);

    void* FindOrInsert(const char* key, void* value);

    void Walk(void(__cdecl* fn)(char* key, void* value, void* ctx), void* ctx, zPTreeNode* node);

    static i32 bit(const char* s, i32 n) {
        return s[n >> PTREE_BYTE_BIT_SHIFT] & (1 << (n & PTREE_BYTE_BIT_MASK));
    }

    static i32 diffpos(const char* a, const char* b);

    zPTreeNode* root;
    zPTreeNode* p;
    zPTreeNode* q;
    i32 sbits;
    i32 preview;
};

template<class T> class zSymTab : public zPTree {
public:
    zSymTab(i32 n = 2)
        : zPTree(
              reinterpret_cast<void(__cdecl*)(void*)>(
                  dtf
              ), // PROVEN: original zSymTab erases its typed teardown callback at this ABI seam.
              n
          ) {}

    T* FindOrInsert(const char* key, T* value) {
        return static_cast<T*>(zPTree::FindOrInsert(key, value));
    }

    T* Find(const char* key) {
        return static_cast<T*>(zPTree::Find(key));
    }

    T* Insert(const char* key, T* value) {
        return static_cast<T*>(zPTree::Insert(key, value));
    }

    void Walk(void(__cdecl* fn)(char* key, void* value, void* ctx), void* ctx, zPTreeNode* node) {
        zPTree::Walk(fn, ctx, node);
    }

private:
    static void dtf(T* p) {
        p->T::~T();
    }
};

#endif // SRC_BUTE_PTREENODE_H
