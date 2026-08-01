#ifndef GRUNTZ_PROJACTCACHE_H
#define GRUNTZ_PROJACTCACHE_H

#include <Ints.h>
#include <Wap32/zBitVec.h>
#include <rva.h>

#include <string.h>

#include <Rez/RezAlloc.h>

extern "C" i32 FirstDiffBit(const char* a, const char* b);

class CButeNode;

struct CTrieNode {
    CTrieNode* m_child[2];

    i32 m_8;
    char* m_c;
    CButeNode* m_10;
};
SIZE_UNKNOWN();

class CProjActMap {
public:
    CButeNode* Insert(const char* key, CButeNode* value);

    char _vft0[4];
    CVariantSlot* m_4;
    char m_pad8[0x14 - 0x8];
    i32 m_14;
    CTrieNode* m_18;
    CTrieNode* m_1c;
    CTrieNode* m_20;
    i32 m_24;
    i32 m_28;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_PROJACTCACHE_H
