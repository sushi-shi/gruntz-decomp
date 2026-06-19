// MapMgr.cpp - the engine's CMapMgr (the level/map manager) ctor/dtor + the
// slot-0 Reset cleanup, plus the two embedded growable-array sub-objects
// (CMapArrayA @+0x30 / CMapArrayB @+0x3c) it owns. Self-located via the RTTI
// vftable. Names are placeholders; only offsets +
// code bytes are load-bearing.
//
// Functions matched in this TU:  7/9 BYTE-EXACT.
//   CMapMgr::CMapMgr()            BYTE-EXACT - ctor
//   CMapMgr::~CMapMgr()           BYTE-EXACT - dtor
//   CMapMgr::Reset()              BYTE-EXACT - slot-0 cleanup
//   CMapArrayA::CMapArrayA()      BYTE-EXACT - sub-obj A ctor
//   CMapArrayA::~CMapArrayA()     BYTE-EXACT - sub-obj A dtor
//   CMapArrayB::CMapArrayB()      BYTE-EXACT - sub-obj B ctor
//   CMapArrayB::~CMapArrayB()     BYTE-EXACT - sub-obj B dtor
// The two free-list builders are NOT byte-exact (NOT claimed in symbol_names.csv;
// bodies kept here to document the element strides, which ARE the load-bearing
// layout fact):
//   CMapArrayA::Allocate(count)   ~70% codegen plateau
//   CMapArrayB::Allocate(count)   ~64% codegen plateau
//
// The CMapMgr ctor constructs the two array members (out-of-line ctors), zeroes
// the scalar bookkeeping members, stores the vftable, then seeds m_50=-1 / m_5c=1.
// The dtor restores the vftable, calls the slot-0 Reset (which frees m_4/m_8 and
// resets the two arrays), then the two member-array destructors run on unwind.
// CMapMgr carries a C++ EH frame (the member sub-objects with destructors) -> /GX.
//
// Allocate{A,B} PLATEAU (~70%/64%): both build a doubly-linked free list over
// `count` fixed-stride (0x24 / 0x0c) elements with prev/next links (A: next@+0x14
// prev@+0x18; B: data@+0x00 prev@+0x04 next@+0x08), first-element prev=0 and
// last-element next=0 - all offsets/strides/values/control-flow correct. The
// residue is a pervasive MSVC5 register-allocation + strength-reduction divergence:
// the target strength-reduces the loop induction onto eax = &elem.prev (indexing
// elem base as eax-0x18) and avoids a held-zero register (using `test`/immediate
// zero stores), whereas our codegen keeps the element pointer in ecx and caches 0
// in ebx. Four source forms (raw-offset cursor on the prev field, element-struct
// pointer walk, cached m_block local, ptr-cast index) all normalized to the same
// (valid) ecx-relative codegen - no source lever flips the induction register.
// Entropy-class; left per the campaign doctrine (the strides + link layout, the
// deliverable here, are fully recovered).
#include <Gruntz/MapMgr.h>
#include <rva.h>

// ===========================================================================
// CMapArrayA (embedded at CMapMgr+0x30; element stride 0x24).
// ===========================================================================

// CMapArrayA::CMapArrayA(): zero m_0(+4), m_block(+0), m_count(+8).
RVA(0x09e700, 0xd)
CMapArrayA::CMapArrayA()
{
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

// CMapArrayA::~CMapArrayA(): free m_0(+4) if set, then zero all.
RVA(0x09e7e0, 0x29)
CMapArrayA::~CMapArrayA()
{
    if (m_0)
        MapFree(m_0);
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

// CMapArrayA::Allocate(count): allocate count*0x24
// bytes, then carve the block into a doubly-linked free list (next @elem+0x14,
// prev @elem+0x18). Returns 0 on alloc failure, else 1.
// A 0x24-byte element of CMapArrayA's block: next link @+0x14, prev link @+0x18.
struct MapElemA {
    char  m_pad0[0x14];
    MapElemA *m_next;   // +0x14
    MapElemA *m_prev;   // +0x18
    char  m_pad1c[0x24 - 0x1c];
};

int CMapArrayA::Allocate(unsigned int count)
{
    MapElemA *block = (MapElemA *)MapAlloc(count * sizeof(MapElemA));
    m_0 = block;
    if (!block)
        return (int)block;

    m_block = block;
    m_count = count;
    block->m_prev = 0;

    MapElemA *e = block;
    for (unsigned int i = 0; i < m_count; ++i) {
        if (e == (MapElemA *)m_block)
            e->m_prev = 0;
        else
            e->m_prev = e - 1;
        e->m_next = e + 1;
        ++e;
    }
    ((MapElemA *)m_block)[m_count - 1].m_next = 0;
    return 1;
}

// ===========================================================================
// CMapArrayB (embedded at CMapMgr+0x3c; element stride 0x0c).
// ===========================================================================

// CMapArrayB::CMapArrayB(): zero m_0(+0), m_block(+4), m_count(+8).
RVA(0x09e820, 0xd)
CMapArrayB::CMapArrayB()
{
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

// CMapArrayB::~CMapArrayB(): free m_0(+0) if set, then zero all.
RVA(0x09e900, 0x28)
CMapArrayB::~CMapArrayB()
{
    if (m_0)
        MapFree(m_0);
    m_0 = 0;
    m_block = 0;
    m_count = 0;
}

// CMapArrayB::Allocate(count): allocate count*0x0c
// bytes, then carve the block into a doubly-linked free list (next @elem+0x08,
// prev @elem+0x04). Returns 0 on alloc failure, else 1.
// A 0x0c-byte element of CMapArrayB's block: data @+0x00, prev @+0x04, next @+0x08.
struct MapElemB {
    void     *m_0;      // +0x00
    MapElemB *m_prev;   // +0x04
    MapElemB *m_next;   // +0x08
};

int CMapArrayB::Allocate(unsigned int count)
{
    MapElemB *block = (MapElemB *)MapAlloc(count * sizeof(MapElemB));
    m_0 = block;
    if (!block)
        return 0;

    m_block = block;
    m_count = count;
    block->m_prev = 0;

    MapElemB *e = block;
    for (unsigned int i = 0; i < m_count; ++i) {
        if (e == (MapElemB *)m_block)
            e->m_prev = 0;
        else
            e->m_prev = e - 1;
        e->m_0 = 0;
        e->m_next = e + 1;
        ++e;
    }
    ((MapElemB *)m_block)[m_count - 1].m_next = 0;
    return 1;
}

// ===========================================================================
// CMapMgr.
// ===========================================================================

// CMapMgr::CMapMgr(). The two array members are constructed
// first (out-of-line ctors), then the body zeroes the scalar members, stores the
// vftable and seeds m_50=-1 / m_5c=1.
RVA(0x09e940, 0x73)
CMapMgr::CMapMgr()
{
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_10 = 0;
    m_18 = 0;
    m_1c = 0;
    m_4c = 0;
    m_58 = 0;
    m_50 = -1;
    m_5c = 1;
}

// CMapMgr::~CMapMgr(). Calls the slot-0 Reset (frees m_4/m_8,
// resets the two arrays), then the two member-array destructors run automatically.
RVA(0x09e9e0, 0x5d)
CMapMgr::~CMapMgr()
{
    Reset();
}

// CMapMgr::Reset() (slot 0). Frees m_4 and m_8 if set, resets
// the two embedded arrays (calls their destructors in place), then zeroes the
// scalar bookkeeping members.
RVA(0x09ec30, 0x4b)
void CMapMgr::Reset()
{
    if (m_4)
        MapFree(m_4);
    if (m_8)
        MapFree(m_8);

    m_colA.~CMapArrayA();
    m_colB.~CMapArrayB();

    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_10 = 0;
    m_18 = 0;
    m_1c = 0;
}

// Out-of-line stubs so the vftable is emitted in this TU.
// Not matched / not in symbol_names.csv; present only to anchor the vftable
// relocation that the ctor stores (the CGameWnd vftable-in-TU idiom).
void CMapMgr::Vfunc1() {}
void CMapMgr::Vfunc2() {}
void CMapMgr::Vfunc3() {}
void CMapMgr::Vfunc4() {}
void CMapMgr::Vfunc5() {}
