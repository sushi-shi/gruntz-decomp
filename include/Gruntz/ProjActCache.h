// ProjActCache.h - zBitVec, a zErrHandling-derived bit-vector (the 0x16d7xx
// region; RTTI-less vtable @0x5f04c8). The ctor (0x16d790) builds the
// zErrHandling error-tracking base (which owns the shared "Out of memory"/
// "Overflow"/... diagnostic string table), stamps the derived vtable, sizes the
// small-buffer-optimized DWORD bit-set to cover the requested bit index, then sets
// that bit; on a SetSize failure it records the caller return address (the OOM
// diagnostic) and fires the container's CVariantSlot error sink. The shared
// instance is the projectile-action cache (g_projActCache).
//
// Names recovered from the delinked callees: zErrHandling::zErrHandling(const
// char*), zBitVec::SetSize(int), CVariantSlot::Set(void*,int,int). Layout from the
// ctor: vptr@+0x00, the +0x04 error sink, the +0x08 capacity, the +0x0c bit-buffer
// (an inline DWORD when capacity <= 0x20, else a heap DWORD*). Field names are
// placeholders; only offsets + emitted bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_PROJACTCACHE_H
#define GRUNTZ_PROJACTCACHE_H

#include <Ints.h>
#include <Wap32/zBitVec.h> // canonical zErrHandling + zBitVec + CVariantSlot
#include <rva.h>

#include <string.h> // strlen/strcmp/memcpy the trie insert lowers to inlines

// The container-error diagnostic globals (g_containerName / g_defaultProjActSize /
// g_projActCache / g_retAddrBreadcrumb / g_projActName) and GetCallerRetAddr (0x16e0f0)
// are the shared container infrastructure from <Wap32/zBitVec.h>.

// The engine heap allocator (NAFXCW operator new replacement). _RezAlloc (named).
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// The first-differing-bit helper (__cdecl): the crit-bit index where two keys
// diverge. Reloc-masked (the delinker's name for 0x16e480 is unrelated).
extern "C" i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

// One crit-bit trie node (20 bytes): two child links, the crit-bit index, the
// owned key copy, and the stored value.
SIZE_UNKNOWN(CTrieNode);
struct CTrieNode {
    CTrieNode* m_0; // +0x0  child[0]
    CTrieNode* m_4; // +0x4  child[1]
    i32 m_8;        // +0x8  crit-bit index
    char* m_c;      // +0xc  owned key copy
    void* m_10;     // +0x10 stored value
};

// zErrHandling (+0x04 CVariantSlot error sink) and zBitVec : zErrHandling (the
// small-buffer bit-vector; SetSize 0x16e100, EnsureSize 0x1936e0) are the canonical
// classes from <Wap32/zBitVec.h>.

// The string-keyed crit-bit trie (the projectile-action name -> value map). Shares
// the zErrHandling error-record idiom (the +0x04 CVariantSlot error sink). The
// working state spans +0x14..+0x28. Field names are placeholders.
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
