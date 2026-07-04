// ScanlineSurface.cpp - the software (system-memory) DIB-surface ops of CRezImage
// (the REZ->image DIB-surface class defined in <Image/Image.h>). Formerly a separate
// `CScanlineSurface` view of the +0x428..+0x458 block; folded onto the ONE canonical
// class so the format-decode DispatchDecode forwards to the real Decode*Data decoders
// and Convert8To16/EnsureSize reallocate through the real DecodeBmpHeader ("Create")
// - the shared RVAs (0x175c90 Free / 0x1757c0 DecodeBmpHeader / 0x175b80 Convert8To16 /
// 0x175a00 DispatchDecode) are one class across Image.cpp / ImagePool.cpp / here.
//
// Reconstructed in ascending retail-RVA order. Field names are placeholders; only the
// OFFSETS + emitted bytes are load-bearing. The decoders (0x176000/0x175e00/0x1762c0/
// 0x176440) + DecodeBmpHeader are defined in Image.cpp, external here -> reloc-masked.
// ---------------------------------------------------------------------------
#include <rva.h>
#include <Image/Image.h>
#include <string.h> // engine inline memset

// ---------------------------------------------------------------------------
// CRezImage::DispatchDecode - select one of four CRezImage format decoders keyed by
// `kind` (2..5 -> DecodePcxData/DecodeResData/DecodeRidData/DecodePidData), forwarding
// `this` (kept in ecx) plus (buf, a2, a3); unknown kind -> 0. __thiscall, ret 0x10.
// The 2nd stack arg is the selector (`this` is the ecx-passed surface node); the four
// pass-through args match the AddSurfaceOp call site (buf, kind, hdc, ctrl).
// @early-stop
// jump-table-data-overlap scoring artifact (docs/patterns/jumptable-data-overlap.md):
// retail's inline jump table is scored against our $L-symbol table; plus a per-case
// arg-forward schedule (regalloc). Dispatch + cases logically exact; relocs now align
// to the real CRezImage decoders.
RVA(0x00175a00, 0x74)
i32 CRezImage::DispatchDecode(void* buf, i32 kind, void* dc, void* ctrl) {
    switch (kind) {
        case 2:
            return DecodePcxData(buf, dc, ctrl);
        case 3:
            return DecodeResData(buf, dc, ctrl);
        case 4:
            return DecodeRidData(buf, dc, ctrl);
        case 5:
            return DecodePidData(buf, dc, ctrl);
    }
    return 0;
}

// The palette object handed to Convert8To16: an 8-byte header then the 256-entry
// RGB table (one u32 per palette index) the 8bpp pixels look up.
struct ScanlinePalette {
    char m_pad0[8];    // +0x00  header
    u32 m_colors[256]; // +0x08  RGB table (indexed by the 8bpp pixel)
};
SIZE_UNKNOWN(ScanlinePalette); // partial view of the foreign palette object (pointer-passed)

// ---------------------------------------------------------------------------
// Build a fresh 16bpp RGB555 copy of the 8bpp `src` surface through the
// `pal` 256-entry RGB table (8 bytes in). Returns TRUE on success.
// @early-stop
// regalloc wall: retail pins `palette` in ebp across the whole function while our
// recompile spills it to the stack and reloads it in the inner loop; that cascades
// into different register encodings throughout. Conversion logic is byte-faithful.
RVA(0x00175b80, 0x105)
i32 CRezImage::Convert8To16(void* dc, CRezImage* src, void* pal) {
    if (pal == 0) {
        return 0;
    }
    u32* palette = ((ScanlinePalette*)pal)->m_colors;
    if (palette == 0) {
        return 0;
    }
    if (!DecodeBmpHeader(dc, src->m_width, src->m_height, 0x10, 0)) {
        return 0;
    }
    for (i32 y = 0; y < m_height; y++) {
        u8* sp = (u8*)src->m_pixels + y * src->m_stride;
        u16* dp = (u16*)((u8*)m_pixels + y * m_stride * 2);
        for (i32 x = 0; x < m_width; x++) {
            u32 c = palette[*sp];
            u32 r = c & 0xff;
            u32 g = (c >> 8) & 0xff;
            u32 b = (c >> 16) & 0xff;
            *dp = (u16)(((((r & 0xf8) << 5) | (g & 0xf8)) << 2) | (b >> 3));
            dp++;
            sp++;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Keep the surface at `w`x`h`. If the DIB object is live and already that
// size, reuse it (return TRUE); otherwise Free + reallocate via DecodeBmpHeader.
RVA(0x00175ce0, 0x6b)
i32 CRezImage::EnsureSize(void* dc, i32 w, i32 h, i32 bitCount, void* flag) {
    if (m_dibSection && m_pixels && m_rowOffsets && m_width == w && m_height == h) {
        return 1;
    }
    Free();
    return DecodeBmpHeader(dc, w, h, bitCount, flag);
}

// ---------------------------------------------------------------------------
// Fill every pixel with the low byte of `value`. Contiguous buffers
// (m_rowPad == 0) get one flat fill; padded buffers fill row by row.
// @early-stop
// memset-inline LICM wall: the contiguous fast path is byte-exact, but retail hoists
// the per-scanline fill's value-mask out of the loop (`value &= 0xff` once + reload)
// while our recompile re-reads the byte each iteration; an explicit `value &= 0xff`
// is folded away as redundant before memset. ~91.4%.
RVA(0x00175d50, 0xad)
void CRezImage::Fill(i32 value) {
    if (m_rowPad == 0) {
        memset(m_pixels, value, m_stride * m_height);
    } else {
        for (i32 y = 0; y < m_height; y++) {
            memset((u8*)m_pixels + m_rowOffsets[y], value, m_width);
        }
    }
}
