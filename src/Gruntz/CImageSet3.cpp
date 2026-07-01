// CImageSet3.cpp - the WWD "imageSet3" collection object (its own vtable is
// g_imageSet3Vtbl @0x5f0228; the grand-base dtor vtable is g_severusWorkerDtorVtbl
// @0x5e8cb4, the same SeverusWorker base CDDrawSurfacePair / CGameLevel derive
// from).  It owns a spatial GRID at +0xb0 (a CWwdGrid: its Prune sweeps the four
// held object-managers, GetSize sums them, and its dtor runs FreeGrids @0x1682f0)
// plus two RezAlloc'd buffers at +0x20/+0x24.
//
// This TU carries the three NON-EH members in retail-RVA order:
//   Prune_1628d0   (0x1628d0) - if(m_b0) tail-call CWwdGrid::Prune
//   GetSize_1633e0 (0x1633e0) - if(m_b0) tail-call CWwdGrid::GetSize
//   Cleanup_161bf0 (0x161bf0) - prune+destroy+free the grid, free m_20/m_24
//
// The /GX manual-vtable destructors (0x161500 ~CImageSet3, 0x163a40 ~CWwdGrid)
// stay in the boundary backlog: they need the SeverusWorker base modeled as a
// non-trivial subobject to emit the retail EH frame (the deferred archetype
// already documented on CDDrawSurfacePair::~).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing.  The grid methods are reloc-masked __thiscall externs.
#include <rva.h>

#include <Ints.h>

// Engine heap free (RezFree).  Reloc-masked __cdecl extern.  0x1b9b82.
extern "C" void RezFree(void* p);

// The +0xb0 spatial grid: Prune (0x1688b0) sweeps its four sub-managers, GetSize
// (0x168430) sums their element counts, and the dtor (0x163a40) frees the grids.
// All reloc-masked __thiscall callees (no body).
class CWwdGrid {
public:
    i32 Prune_1688b0();   // 0x1688b0
    i32 GetSize_168430(); // 0x168430
    void Dtor_163a40();   // 0x163a40
};

class CImageSet3 {
public:
    i32 Prune_1628d0();    // 0x1628d0
    i32 GetSize_1633e0();  // 0x1633e0
    void Cleanup_161bf0(); // 0x161bf0

    char m_pad00[0x20];        // +0x00 .. +0x1f (vptr + SeverusWorker base + ...)
    void* m_20;                // +0x20  RezAlloc'd buffer
    void* m_24;                // +0x24  RezAlloc'd buffer
    char m_pad28[0xb0 - 0x28]; // +0x28 .. +0xaf
    CWwdGrid* m_b0;            // +0xb0  spatial grid
};

// ---------------------------------------------------------------------------
// 0x1628d0: forward the grid's Prune when present (else 0).  __thiscall tail call.
RVA(0x001628d0, 0x12)
i32 CImageSet3::Prune_1628d0() {
    if (m_b0 == 0) {
        return 0;
    }
    return m_b0->Prune_1688b0();
}

// ---------------------------------------------------------------------------
// 0x1633e0: forward the grid's GetSize when present (else 0).  __thiscall tail call.
RVA(0x001633e0, 0x12)
i32 CImageSet3::GetSize_1633e0() {
    if (m_b0 == 0) {
        return 0;
    }
    return m_b0->GetSize_168430();
}

// ---------------------------------------------------------------------------
// 0x161bf0: tear down the owned resources.  Prune the grid, then destroy + free
// it (no null-out), then free the two RezAlloc'd buffers at +0x20/+0x24 (nulled).
RVA(0x00161bf0, 0x5e)
void CImageSet3::Cleanup_161bf0() {
    if (m_b0 != 0) {
        m_b0->Prune_1688b0();
    }
    CWwdGrid* g = m_b0;
    if (g != 0) {
        g->Dtor_163a40();
        RezFree(g);
    }
    if (m_20 != 0) {
        RezFree(m_20);
        m_20 = 0;
    }
    if (m_24 != 0) {
        RezFree(m_24);
        m_24 = 0;
    }
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CImageSet3);
