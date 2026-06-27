// ProjActCache.h - zBitVec, a CContainerErr-derived bit-vector (the 0x16d7xx
// region; RTTI-less vtable @0x5f04c8). The ctor (0x16d790) builds the
// CContainerErr error-tracking base (which owns the shared "Out of memory"/
// "Overflow"/... diagnostic string table), stamps the derived vtable, sizes the
// small-buffer-optimized DWORD bit-set to cover the requested bit index, then sets
// that bit; on a SetSize failure it records the caller return address (the OOM
// diagnostic) and fires the container's CVariantSlot error sink. The shared
// instance is the projectile-action cache (g_projActCache).
//
// Names recovered from the delinked callees: CContainerErr::CContainerErr(const
// char*), zBitVec::SetSize(int), CVariantSlot::Set(void*,int,int). Layout from the
// ctor: vptr@+0x00, the +0x04 error sink, the +0x08 capacity, the +0x0c bit-buffer
// (an inline DWORD when capacity <= 0x20, else a heap DWORD*). Field names are
// placeholders; only offsets + emitted bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_PROJACTCACHE_H
#define GRUNTZ_PROJACTCACHE_H

#include <Ints.h>
#include <rva.h>

// The derived class's vtable (0x5f04c8) - stamped by hand (its 6 slots point into
// other TUs, so the compiler cannot emit a matching one). Reloc-masked DATA.
struct CProjActVtbl;
DATA(0x001f04c8)
extern CProjActVtbl g_projActVtbl; // 0x5f04c8

// Globals the OOM path touches (.data). g_containerName is the const-char* anchor
// the base ctor records; g_defaultSize is the fallback capacity; g_projActCache /
// g_projActAllocResult are the diagnostic record cells. Reloc-masked.
DATA(0x002bf408)
extern char g_containerName[]; // 0x6bf408 (base ctor const char* arg)
DATA(0x0021ad28)
extern i32 g_defaultProjActSize; // 0x61ad28
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464 (?g_projActCache@@3PAXA)
DATA(0x002bf428)
extern void* g_projActAllocResult; // 0x6bf428 (?g_projActAllocResult@@3PAXA)
DATA(0x002bf454)
extern void* g_projActName; // 0x6bf454 (the bad-arg diagnostic record cell)

// _ReturnAddress()-style helper (0x16e0f0: mov eax,[ebp+4]; ret) - records where
// the failing allocation was requested. Reloc-masked (no body).
void* GetCallerRetAddr(); // 0x16e0f0

// The engine heap allocator (NAFXCW operator new replacement). _RezAlloc (named).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46

// The first-differing-bit helper (__cdecl): the crit-bit index where two keys
// diverge. Reloc-masked (the delinker's name for 0x16e480 is unrelated).
extern "C" i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

// Inline CRT string intrinsics the trie insert lowers to (repnz scas / sbb idiom
// / rep movs). Declared so MSVC5 /O2 inlines them.
extern "C" u32 strlen(const char* s);
extern "C" i32 strcmp(const char* a, const char* b);
extern "C" void* memcpy(void* d, const void* s, u32 n);

// One crit-bit trie node (20 bytes): two child links, the crit-bit index, the
// owned key copy, and the stored value.
struct CTrieNode {
    CTrieNode* m_0; // +0x0  child[0]
    CTrieNode* m_4; // +0x4  child[1]
    i32 m_8;        // +0x8  crit-bit index
    char* m_c;      // +0xc  owned key copy
    void* m_10;     // +0x10 stored value
};

// The +0x04 error sink: on a sizing failure the container reports through it.
class CVariantSlot {
public:
    void Set(void* obj, i32 a, i32 b); // 0x16d850
};

// The CContainerErr error-tracking container base.
class CContainerErr {
public:
    CContainerErr(const char* name); // 0x16d9c0 (??0CContainerErr@@QAE@PBD@Z)
    ~CContainerErr();                // makes the derived ctor's /GX unwind destruct the base

    void* m_vptr;       // +0x00
    CVariantSlot* m_4;  // +0x04 (error sink)
    u32 m_8;            // +0x08 (capacity; unsigned -> jbe)
    void* m_c;          // +0x0c (inline DWORD when m_8 <= 0x20, else heap DWORD*)
};

class zBitVec : public CContainerErr {
public:
    zBitVec(i32 idx, i32 sizehint); // 0x16d790
    i32 SetSize(i32 n);             // 0x16e100 (?SetSize@zBitVec@@QAEHH@Z)
    i32 EnsureSize(i32 nbits);      // 0x1936e0 (grow + preserve, reports OOM)
};

// The string-keyed crit-bit trie (the projectile-action name -> value map). Shares
// the CContainerErr error-record idiom (the +0x04 CVariantSlot error sink). The
// working state spans +0x14..+0x28. Field names are placeholders.
class CProjActMap {
public:
    void* Insert(const char* key, void* value); // 0x1933b0

    void* m_vptr;      // +0x00
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
