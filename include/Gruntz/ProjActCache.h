#ifndef GRUNTZ_PROJACTCACHE_H
#define GRUNTZ_PROJACTCACHE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/zBitVec.h>

#include <string.h>

extern "C" i32 FirstDiffBit(const char* a, const char* b);

class CButeNode;

struct CTrieNode {
    CTrieNode* m_child[2];

    i32 m_bit;
    char* m_key;
    CButeNode* m_value;
};
SIZE_UNKNOWN();

class CProjActMap {
public:
    CButeNode* Insert(const char* key, CButeNode* value);

    char _vft0[4];
    CVariantSlot* m_errSink;
    char m_pad8[0x14 - 0x8];
    i32 m_nodeCount;
    CTrieNode* m_root;
    CTrieNode* m_descentCursor;
    CTrieNode* m_candidateLeaf;
    i32 m_keyBitLength;
    i32 m_lookupPending;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_PROJACTCACHE_H
