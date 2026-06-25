#ifndef SRC_IMAGE_CSCANLINESURFACE_H
#define SRC_IMAGE_CSCANLINESURFACE_H

// CScanlineSurface.h - a software (system-memory) scanline surface in the DDrawMgr
// image family. NOT the RTTI CImage (which tops out at +0x30); this class carries
// a deep +0x428..+0x448 block: an owner latch @+0x428, the pixel buffer @+0x42c, a
// per-scanline offset table @+0x430, dims @+0x438/+0x43c, and the byte stride @+0x444,
// plus a "scanline mode" latch @+0x448 (0 = the buffer is contiguous, !=0 = walk the
// +0x430 offset table). Mis-attributed to CImage by the static RVA bracketer.
//
// Reconstructed from the four touched methods (8bpp->RGB555 conversion, the
// resize/realloc gate, and the fill). Field names are placeholders; only the
// OFFSETS + emitted bytes are load-bearing (campaign carcass doctrine). The
// allocator/free and the four "op" callees are external no-body fns so their
// `call rel32`/jump-table relocs reloc-mask.

#include <rva.h>

class CScanlineSurface {
public:
    // 0x175b80: convert the 8bpp `src` surface into a fresh 16bpp RGB555 surface
    // through `palette` (a 256-entry RGB table 8 bytes past `pal`); returns TRUE.
    i32 Convert8To16(void* a0, CScanlineSurface* src, void* pal);
    // 0x175ce0: keep the surface at `w`x`h`; reallocate (Free + Create) if the
    // current buffer is unset or a different size; returns TRUE.
    i32 EnsureSize(void* a0, i32 w, i32 h, i32 a3, i32 a4);
    // 0x175d50: fill the whole surface with the low byte of `value`.
    void Fill(i32 value);

    // External (out-of-line) members this TU calls but does not define -> the
    // `call rel32` reloc-masks.
    i32 Create(void* a0, i32 w, i32 h, i32 bpp, i32 flag); // 0x1757c0
    void Free();                                           // 0x175c90

    // --- layout (placeholder names; offsets are the load-bearing truth) ---
    char m_pad000_428[0x428];
    void* m_428; // +0x428  owner / valid latch
    u8* m_42c;   // +0x42c  pixel buffer
    u32* m_430;  // +0x430  per-scanline byte-offset table
    i32 m_434;   // +0x434
    i32 m_438;   // +0x438  width
    i32 m_43c;   // +0x43c  height
    i32 m_440;   // +0x440
    i32 m_444;   // +0x444  stride (bytes per scanline)
    i32 m_448;   // +0x448  scanline-mode latch (0 = contiguous, !=0 = offset table)
};

#endif // SRC_IMAGE_CSCANLINESURFACE_H
