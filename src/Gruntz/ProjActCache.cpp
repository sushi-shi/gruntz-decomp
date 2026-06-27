// ProjActCache.cpp - zBitVec::zBitVec (0x16d790): construct the CContainerErr
// error-tracking base, stamp the derived vtable, size the bit-vector to cover
// `idx`, then set bit `idx`; on a sizing failure record the caller return address
// and fire the error sink. The destructible base forces the /GX frame.
#include <Gruntz/ProjActCache.h>

// Heap externs the grow path reaches (reloc-masked rel32 callees). memset stays
// the rep-stos intrinsic; memcpy is forced out-of-line (call) - see below.
extern "C" void* realloc(void* p, u32 n);               // 0x125180
extern "C" void* malloc(u32 n);                         // 0x120b60
extern "C" void* memcpy(void* d, const void* s, u32 n); // 0x121960
extern "C" void* memset(void* d, i32 c, u32 n);         // (inlined to rep stos)
#pragma function(memcpy)

// ===========================================================================
// zBitVec::zBitVec  (0x16d790)
// ===========================================================================
// @early-stop
// /GX EH-epilogue + RMW-fusion wall (topic:eh topic:regalloc; see docs/patterns/
// identical-return-epilogue-tailmerge.md). Logic, recovered types (CContainerErr/
// zBitVec/CVariantSlot), the const-char* base ctor, the unsigned size compares,
// the SBO bit-buffer select and the inverted branch layout (failure inline /
// success out-of-line) are all byte-faithful, but two MSVC5 /O2 choices diverge
// with no source lever:
//   (a) retail SHARES one /GX teardown epilogue (the failure path `jmp`s to it,
//       success falls through); our cl pops edi early in the success bitset (idx
//       dies after the address calc) so the two exit epilogues are NOT identical
//       and cl duplicates them instead of merging.
//   (b) the bit set is `or [eax],edx` (RMW) in retail; cl emits load/or/store
//       (`base[i] |= mask`, `*slot |= mask` and a precomputed mask all do the
//       same), +3 bytes that shift the success tail.
// ~77.8%, logic complete; deferred to the final sweep.
RVA(0x0016d790, 0xb1)
zBitVec::zBitVec(i32 idx, i32 sizehint) : CContainerErr(g_containerName) {
    *(void**)this = &g_projActVtbl;
    u32 n = (u32)sizehint;
    if (n == 0) {
        n = (u32)g_defaultProjActSize;
    }
    if ((u32)idx >= n) {
        n = (u32)idx + 1;
    }
    if (!SetSize((i32)n)) {
        void* cache = g_projActCache;
        g_projActAllocResult = GetCallerRetAddr();
        m_4->Set(this, (i32)cache, 0xc);
    } else {
        u32* base = (m_8 > 0x20) ? (u32*)m_c : (u32*)&m_c;
        u32* slot = base + ((u32)idx >> 5);
        *slot |= 1u << (idx & 0x1f);
    }
}

// ===========================================================================
// zBitVec::EnsureSize  (0x1936e0)
// ===========================================================================
// Grow the word band to cover `nbits` bits, preserving existing words. When the
// band is already on the heap (capacity > 32) realloc and zero only the grown
// tail; from the inline 4-byte SBO buffer malloc fresh, zero it, and copy the
// inline word in. On allocation failure record the caller return address and
// fire the container error sink, returning 0.
RVA(0x001936e0, 0xd3)
i32 zBitVec::EnsureSize(i32 nbits) {
    u32 ndwords = ((nbits & 0x1f) != 0 ? 1 : 0) + ((u32)nbits >> 5);
    void* nbuf;
    if (m_8 > 0x20) {
        nbuf = realloc(m_c, ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        u32 oldn = m_8 >> 5;
        memset((u32*)nbuf + oldn, 0, (ndwords - oldn) * 4);
    } else {
        nbuf = malloc(ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        memset(nbuf, 0, ndwords * 4);
        memcpy(nbuf, &m_c, 4);
    }
    m_c = nbuf;
    m_8 = ndwords * 32;
    return 1;
fail:
    void* cache = g_projActCache;
    g_projActAllocResult = GetCallerRetAddr();
    m_4->Set(this, (i32)cache, 0xc);
    return 0;
}
