#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/zBitVec.h>

struct CVariantSlot;
struct CButeTreeNode;
extern CVariantSlot g_symTabErrorSlot;

class zPtrColl {
public:
    zPtrColl(i32 n, void(__cdecl* teardown)(void*));
    virtual ~zPtrColl();

    void(__cdecl* m_teardown)(void*);
    i16 m_kind;
    char m_pada[2];
    i32 m_nodeCount;
};
SIZE(0x10);

class zPTree : public zErrHandling, public zPtrColl {
public:
    zPTree(void(__cdecl* teardown)(void*), i32 n);

    void ClearRecursive(CButeTreeNode* node);

    void Reset() {
        ClearRecursive(0);
        m_root = 0;
        m_lookupPending = 0;
        m_nodeCount = 0;
    }

    virtual ~zPTree() OVERRIDE {
        ClearRecursive(0);
    }

    void* Find(const char* key);
    void* Insert(const char* key, void* value);

    void Walk(void(__cdecl* fn)(char* key, void* value, void* ctx), void* ctx, CButeTreeNode* node);

    CButeTreeNode* m_root;
    CButeTreeNode* m_descentCursor;
    CButeTreeNode* m_candidateLeaf;
    i32 m_keyBitLength;
    i32 m_lookupPending;
};
SIZE(0x2c);

class CButeNode : public zPTree {
public:
    virtual ~CButeNode() OVERRIDE;

    CButeNode(i32 kind);

    CButeNode(void(__cdecl* teardown)(void*), i32 n) : zPTree(teardown, n) {}
};
SIZE(0x2c);

#endif // SRC_BUTE_PTREENODE_H
