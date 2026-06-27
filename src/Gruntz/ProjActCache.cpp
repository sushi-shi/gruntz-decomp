// ProjActCache.cpp - zBitVec::zBitVec (0x16d790): construct the CContainerErr
// error-tracking base, stamp the derived vtable, size the bit-vector to cover
// `idx`, then set bit `idx`; on a sizing failure record the caller return address
// and fire the error sink. The destructible base forces the /GX frame.
#include <Gruntz/ProjActCache.h>

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
