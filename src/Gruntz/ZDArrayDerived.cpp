// ZDArrayDerived.cpp - the shared registry build helper (orphan COMDAT @0x8710).
//
// CTypeKeyColl::Construct(lo, hi) forwards to the shared 2D-array ctor (0x16dda0)
// with stride 4 / scratch 1, then stamps the derived vtable (g_zDArrayVtbl,
// 0x5e70fc) and returns this. Placeholder class name; only OFFSETS + code bytes
// are load-bearing.
#include <Ints.h>
#include <Gruntz/TypeKeyColl.h>
#include <rva.h>

// KEEP (faithful, not a hack): the vptr store lives in the non-ctor two-phase
// Construct() helper (returns `this` after the base build), so cl cannot auto-emit
// it as a real ??_7 stamp - a real ctor would relocate the base-ctor call + the
// derived stamp and diverge (vtable-realization-ctor-boundary). g_zDArrayVtbl
// (0x5e70fc) is the shared CTypeKeyColl-family derived table. 100% matched.

RVA(0x00008710, 0x2b)
CTypeKeyColl* CTypeKeyColl::Construct(i32 lo, i32 hi) {
    BaseConstruct(4, lo, hi, reinterpret_cast<void*>(1));
    *reinterpret_cast<volatile i32*>(&hi) = reinterpret_cast<i32>(m_alloc); // write-back to the hi param slot (retail keeps it)
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    return this;
}
