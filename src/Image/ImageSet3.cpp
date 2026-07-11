// ImageSet3.cpp - the WWD "imageSet3" collection object (its own vtable is
// g_imageSet3Vtbl @0x5f0228; the grand-base dtor vtable is g_wapObjectDtorVtbl
// @0x5e8cb4, the same CLoadable base CDDrawSurfacePair / CGameLevel derive
// from).  It owns a spatial GRID at +0xb0 (a CWwdGrid: its Prune sweeps the four
// held object-managers, GetSize sums them, and its dtor runs FreeGrids @0x1682f0)
// plus two RezAlloc'd buffers at +0x20/+0x24.
//
// This TU carries the object's out-of-line dtor + its 0x166e00 pixel scan. The
// three non-EH leaves Cleanup_161bf0 (0x161bf0) / Prune_1628d0 (0x1628d0) /
// GetSize_1633e0 (0x1633e0) are birth-positioned INSIDE the plane/render TU and
// were re-homed to src/Gruntz/LevelPlane.cpp (interval dossier 0x15ccd0, wave1-C);
// their declarations below stay as documentation of the same object.
//
// The /GX out-of-line ~CImageSet3 (0x161500) is defined below under its C161500
// placeholder identity (collapsed from ImageSet3Eh.cpp; the split companion TU was
// our invention - retail's one TU was compiled /GX). ~CWwdGrid (0x163a40) stays in
// the boundary backlog (needs the CLoadable base modeled as a non-trivial
// subobject - the deferred archetype documented on CDDrawSurfacePair::~).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing.  The grid methods are reloc-masked __thiscall externs.
#include <rva.h>

#include <Mfc.h> // CObject grand-base of the collapsed ~CImageSet3 (folds ??_7CObject @0x5e8cb4)
#include <Ints.h>

// Engine heap free (0x1b9b82). C++ linkage (NOT extern "C") so cl treats it as
// potentially-throwing and keeps the /GX base-subobject unwind frame in the
// collapsed ~CImageSet3 below. The Cleanup free is the NAFXCW global operator delete
// (??3@YAXPAX@Z @0x1b9b82), reloc-masked.

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
    i32 GetSize_1633e0();  // 0x1633e0 (out-of-line: m_b0 ? m_b0->GetSize_168430() : 0)
    void Cleanup_161bf0(); // 0x161bf0
    // 0x166e00 (vtable slot 10): scan left from (x,y) along the row for the first pixel
    // that differs from the pixel at (x,y); report its column + value. m_14 is the pixel
    // buffer, m_c the row-stride shift (row width == 1<<m_c).
    i32 ScanRunLeft_166e00(i32 x, i32 y, i32* outX, i32* outVal);

    char m_pad00[0x0c];        // +0x00 .. +0x0b (vptr + CLoadable base)
    i32 m_c;                   // +0x0c  row-stride shift (log2 of the row width)
    char m_pad10[0x14 - 0x10]; // +0x10 .. +0x13
    u8* m_14;                  // +0x14  pixel buffer base
    char m_pad18[0x20 - 0x18]; // +0x18 .. +0x1f
    void* m_20;                // +0x20  RezAlloc'd buffer
    void* m_24;                // +0x24  RezAlloc'd buffer
    char m_pad28[0xb0 - 0x28]; // +0x28 .. +0xaf
    CWwdGrid* m_b0;            // +0xb0  spatial grid
};

// ---------------------------------------------------------------------------
// 0x161500 - the out-of-line ~CImageSet3: stamp derived (0x5f0228), free the +0x14
// pixel buffer and zero it, fold the CObject base dtor (0x5e8cb4). Kept a DISTINCT
// placeholder identity (C161500): folding onto the real CImageSet3 (below) needs
// its CLoadable base modeled as a non-trivial subobject to emit the EH frame (the
// deferred archetype on CDDrawSurfacePair::~; @identity-TODO). 0x5f0228 ==
// ??_7CImageSet3 (bound by VTBL in GameLevel.cpp). Grand-base fold @0x161548 is the
// REAL ??_7CObject (0x5e8cb4, disasm-verified). Collapsed from ImageSet3Eh.cpp.
struct C161500 : CObject {
    char _4[0x14 - 0x4];
    char* m_14; // +0x14  pixel buffer
    virtual ~C161500() OVERRIDE;
};
SIZE_UNKNOWN(C161500);
RELOC_VTBL(C161500, 0x001f0228); // aliases CImageSet3 (dtor-stamp verified)
RVA(0x00161500, 0x58)
C161500::~C161500() {
    if (m_14) {
        ::operator delete(m_14);
    }
    m_14 = 0;
}

// ---------------------------------------------------------------------------
// 0x166e00: from the pixel at (x,y), walk LEFT along the row while the pixel value
// stays equal to that start pixel. On the first differing pixel, record its column
// in *outX and its value in *outVal and return 1; if the row edge (x reaches 0) is
// hit first, return 0. __thiscall, 4 args (ret 0x10).
// 100%: the former SIB base/index-swap residual (a 99.778% @early-stop) flipped to
// retail's pick when the TU regained its retail /GX profile + the collapsed dtor
// (the *Eh.cpp merge) - the codegen residue was TU-composition-sensitive, not a wall.
RVA(0x00166e00, 0xa8)
i32 CImageSet3::ScanRunLeft_166e00(i32 x, i32 y, i32* outX, i32* outVal) {
    i32 off = (y << m_c) + x;
    i32 target = m_14[off];
    while (x > 0) {
        --x;
        --off;
        if (m_14[off] != target) {
            *outX = x;
            *outVal = m_14[off];
            return 1;
        }
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CImageSet3);
