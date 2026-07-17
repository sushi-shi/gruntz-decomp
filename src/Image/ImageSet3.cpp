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
#include <Gruntz/ImageSets.h> // the canonical 18-slot CImageSet3 (RTTI vtbl 0x1f0228)

// Engine heap free (0x1b9b82). C++ linkage (NOT extern "C") so cl treats it as
// potentially-throwing and keeps the /GX base-subobject unwind frame in the
// collapsed ~CImageSet3 below. The Cleanup free is the NAFXCW global operator delete
// (??3@YAXPAX@Z @0x1b9b82), reloc-masked.

// CImageSet3 is the CANONICAL 18-slot class (<Gruntz/ImageSets.h>, RTTI vtbl 0x1f0228,
// CObject base). This TU used to define its OWN `class CImageSet3 : public CObject`
// declaring only the dtor as a virtual - 5 slots - so cl emitted ??_7CImageSet3@@6B@ at
// 20 BYTES here against the 72 B (18-slot) vtable gamelevel emits from the canonical
// header. One mangled name, two lengths; MSVC5 keeps one COMDAT and discards the rest,
// so 13 of this class's 18 virtual slots could vanish under the truncated table.
//
// The grid-owner facet the old view carried (Prune/GetSize/Cleanup, m_b0@+0xb0, and the
// CWwdGrid view) is documented in LevelPlane.cpp and was UNUSED here; its `0xb0`-sized
// object cannot be this record anyway (a flagged conflation) - dropped rather than
// re-stated. GetSize_1633e0 lives on its real owner CDDrawWorkerHost (LevelPlane.cpp).

// ---------------------------------------------------------------------------
// 0x161500 - the out-of-line /GX ~CImageSet3: stamp the derived vtable (0x5f0228 ==
// ??_7CImageSet3@@6B@, VTBL-bound in GameLevel.cpp), free the +0x14 pixel buffer and
// zero it, then fold the CObject grand-base dtor (0x5e8cb4 == ??_7CObject,
// disasm-verified). Collapsed from ImageSet3Eh.cpp; folded off the old C161500
// placeholder view so the vptr stamp binds the REAL vtable rva (reloc-faithful).
RVA(0x00161500, 0x58)
CImageSet3::~CImageSet3() {
    if (m_pixels) {
        ::operator delete(m_pixels);
    }
    m_pixels = 0;
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
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (x > 0) {
        --x;
        --off;
        if ((m_pixels)[off] != target) {
            *outX = x;
            *outVal = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}
