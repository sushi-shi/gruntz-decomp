// CScanlineSurface.cpp - the software scanline surface in the DDrawMgr image
// family (mis-attributed to CImage by the static RVA bracketer; the real CImage
// tops out at +0x30, these methods drive the +0x428..+0x448 software-surface
// block). See include/Image/CScanlineSurface.h for the layout.
//
// Reconstructed in ascending retail-RVA order. Field names are placeholders; only
// the OFFSETS + the emitted bytes are load-bearing. The four "op" callees and the
// Create/Free members are external no-body fns -> their `call rel32` reloc-mask.
// ---------------------------------------------------------------------------
#include <rva.h>
#include <Image/CScanlineSurface.h>
#include <string.h> // engine inline memset

// The four blit/op callees the 0x175a00 dispatcher forwards to (a0, a2, a3);
// __stdcall (callee-cleanup, 3 args) -> their rel32 calls reloc-mask.
i32 __stdcall SoftSurfOp176000(void* a0, i32 a2, i32 a3); // 0x176000
i32 __stdcall SoftSurfOp175e00(void* a0, i32 a2, i32 a3); // 0x175e00
i32 __stdcall SoftSurfOp1762c0(void* a0, i32 a2, i32 a3); // 0x1762c0
i32 __stdcall SoftSurfOp176440(void* a0, i32 a2, i32 a3); // 0x176440

// ---------------------------------------------------------------------------
// 0x175a00: dispatch one of four CImage format decoders on `a0` keyed by `kind`
// (2..5; the callees alias CImage::Decode{Pcx,Res,Rid,Pid}Data), forwarding
// (a0, a2, a3); unknown kind -> 0. __stdcall (callee cleanup).
// @early-stop
// jump-table-data-overlap scoring artifact (docs/patterns/jumptable-data-overlap.md):
// retail's inline jump table is scored against our $L-symbol table; plus a 2-vs-3
// register arg-forward schedule per case (regalloc). Dispatch + cases logically exact.
RVA(0x00175a00, 0x74)
i32 __stdcall SoftSurfaceBlit(void* a0, i32 kind, i32 a2, i32 a3) {
    switch (kind) {
        case 2:
            return SoftSurfOp176000(a0, a2, a3);
        case 3:
            return SoftSurfOp175e00(a0, a2, a3);
        case 4:
            return SoftSurfOp1762c0(a0, a2, a3);
        case 5:
            return SoftSurfOp176440(a0, a2, a3);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x175b80: build a fresh 16bpp RGB555 copy of the 8bpp `src` surface through the
// `pal` 256-entry RGB table (8 bytes in). Returns TRUE on success.
// @early-stop
// regalloc wall: retail pins `palette` in ebp across the whole function while our
// recompile spills it to the stack and reloads it in the inner loop; that cascades
// into different register encodings throughout. Conversion logic is byte-faithful.
RVA(0x00175b80, 0x105)
i32 CScanlineSurface::Convert8To16(void* a0, CScanlineSurface* src, void* pal) {
    if (pal == 0) {
        return 0;
    }
    u32* palette = (u32*)((char*)pal + 8);
    if (palette == 0) {
        return 0;
    }
    if (!Create(a0, src->m_width, src->m_height, 0x10, 0)) {
        return 0;
    }
    for (i32 y = 0; y < m_height; y++) {
        u8* sp = src->m_pixels + y * src->m_stride;
        u16* dp = (u16*)(m_pixels + y * m_stride * 2);
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
// 0x175ce0: keep the surface at `w`x`h`. If the buffer is live and already that
// size, reuse it (return TRUE); otherwise Free + Create.
RVA(0x00175ce0, 0x6b)
i32 CScanlineSurface::EnsureSize(void* a0, i32 w, i32 h, i32 a3, i32 a4) {
    if (m_428 && m_pixels && m_scanlineOffsets && m_width == w && m_height == h) {
        return 1;
    }
    Free();
    return Create(a0, w, h, a3, a4);
}

// ---------------------------------------------------------------------------
// 0x175d50: fill every pixel with the low byte of `value`. Contiguous buffers
// (m_scanlineMode == 0) get one flat fill; scanline-table buffers fill row by row.
// @early-stop
// memset-inline LICM wall: the contiguous fast path is byte-exact, but retail hoists
// the per-scanline fill's value-mask out of the loop (`value &= 0xff` once + reload)
// while our recompile re-reads the byte each iteration; an explicit `value &= 0xff`
// is folded away as redundant before memset. ~91.4%.
RVA(0x00175d50, 0xad)
void CScanlineSurface::Fill(i32 value) {
    if (m_scanlineMode == 0) {
        memset(m_pixels, value, m_stride * m_height);
    } else {
        for (i32 y = 0; y < m_height; y++) {
            memset(m_pixels + m_scanlineOffsets[y], value, m_width);
        }
    }
}
