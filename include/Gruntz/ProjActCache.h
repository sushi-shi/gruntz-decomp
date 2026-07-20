#ifndef GRUNTZ_PROJACTCACHE_H
#define GRUNTZ_PROJACTCACHE_H

#include <Ints.h>
#include <Wap32/zBitVec.h> // canonical zErrHandling + zBitVec + CVariantSlot
#include <rva.h>

#include <string.h> // strlen/strcmp/memcpy the trie insert lowers to inlines

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

extern "C" i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

SIZE_UNKNOWN(CTrieNode);
struct CTrieNode {
    CTrieNode* m_0; // +0x0  child[0]
    CTrieNode* m_4; // +0x4  child[1]
    i32 m_8;        // +0x8  crit-bit index
    char* m_c;      // +0xc  owned key copy
    void* m_10;     // +0x10 stored value
};

SIZE_UNKNOWN(CProjActMap);
class CProjActMap {
public:
    void* Insert(const char* key, void* value); // 0x1933b0

    char _vft0[4];     // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    CVariantSlot* m_4; // +0x04  error sink
    char m_pad8[0x14 - 0x8];
    i32 m_14;        // +0x14  node count
    CTrieNode* m_18; // +0x18  root
    CTrieNode* m_1c; // +0x1c  descent cursor
    CTrieNode* m_20; // +0x20  candidate child
    i32 m_24;        // +0x24  key bit-length (strlen*8)
    i32 m_28;        // +0x28  cleared on entry
};

#endif // GRUNTZ_PROJACTCACHE_H
