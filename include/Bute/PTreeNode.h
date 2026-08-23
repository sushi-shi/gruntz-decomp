#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/zBitVec.h>

#include <stddef.h>

struct CVariantSlot;
struct CButeTreeNode;
extern CVariantSlot g_symTabErrorSlot;

// Index of the first differing bit between two NUL-terminated keys; the PATRICIA
// critbit primitive shared by zPTree::Insert and zPTree::FindOrInsert.
i32 FirstDiffBit(const char* a, const char* b);

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

    void ClearRecursive(CButeTreeNode* node);

    void Reset() {
        ClearRecursive(NULL);
        m_root = NULL;
        m_lookupPending = 0;
        m_nodeCount = 0;
    }

    // Same body, out of line (0x212a0): CChatBoxOwner::ProcessCheatInput expands
    // Reset() for m_tree and CALLS this one for m_tree48/m_tree74. cl 5 emits one
    // shape per spelling, so retail's two shapes need two entities
    // (docs/patterns/two-shapes-need-two-entities.md).
    void ResetCopy();

    virtual ~zPTree() OVERRIDE {
        ClearRecursive(NULL);
    }

    void* Find(const char* key);

    // Insert(): only legal directly after a Find() on THIS node - it consumes the
    // cached descent (m_descentCursor/m_candidateLeaf/m_lookupPending) and errors
    // with "No prior lookup" otherwise.
    void* Insert(const char* key, void* value);

    // FindOrInsert(): self-contained - runs its own descent, returns the value
    // already stored under `key` if present, otherwise links a fresh node. This is
    // what the callers use on a node they just created (no prior Find to consume).
    void* FindOrInsert(const char* key, void* value);

    void Walk(void(__cdecl* fn)(char* key, void* value, void* ctx), void* ctx, CButeTreeNode* node);

    CButeTreeNode* m_root;
    CButeTreeNode* m_descentCursor;
    CButeTreeNode* m_candidateLeaf;
    i32 m_keyBitLength;
    i32 m_lookupPending;
};

class CButeNode : public zPTree {
public:
    virtual ~CButeNode() OVERRIDE;

    CButeNode(i32 kind);

    CButeNode(void(__cdecl* teardown)(void*), i32 n) : zPTree(teardown, n) {}
};

#endif // SRC_BUTE_PTREENODE_H
