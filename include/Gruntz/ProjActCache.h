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

// _ReturnAddress()-style helper (0x16e0f0: mov eax,[ebp+4]; ret) - records where
// the failing allocation was requested. Reloc-masked (no body).
void* GetCallerRetAddr(); // 0x16e0f0

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

#endif // GRUNTZ_PROJACTCACHE_H
