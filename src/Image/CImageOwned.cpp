#include <rva.h>
// CImageOwned.cpp - the +0x30 owned buffer-holder of the RTTI CImage (built by
// CImage::BuildSlot13). Two methods in retail-RVA order: the ctor (0x148ce0) that
// primes the defaults, and Build (0x1490d0) - decodes one frame out of a
// CImageFrameDesc into the owned decoded-pixel buffer (+0x0c) and a 256-entry
// hardware palette (+0x20), copying the desc's dimension/format metadata. Teardown
// (0x148d10) and the palette-remap helper (0x1495d0) are external engine callees
// (reloc-masked); see include/Image/CImage.h for the layout.
//
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
// No destructible stack local -> no /GX frame (flags="base").
// ---------------------------------------------------------------------------

#include <Image/CImage.h>

#include <string.h> // memcpy (inlined to rep movs)

// The engine allocator/deallocator (reloc-masked rel32). Global operator new
// @0x1b9b46 (NAFXCW), _RezFree @0x1b9b82.
void* operator new(u32 n);
extern "C" void RezFree(void* p);

// ---------------------------------------------------------------------------
// 0x148ce0: the constructor. Zero the buffers/counters; prime m_14=1, m_18=0x80,
// m_24=-1, and both format-flag bytes m_28/m_29=1. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x00148ce0, 0x2f)
CImageOwned::CImageOwned() {
    m_0c = 0;
    m_10 = 0;
    m_1c = 0;
    m_14 = 1;
    m_18 = 0x80;
    m_00 = 0;
    m_20 = 0;
    m_28 = 1;
    m_29 = 1;
    m_24 = -1;
}

// ---------------------------------------------------------------------------
// 0x1490d0: decode a frame from the descriptor. The desc flag word (+0x04) and the
// format code steer two flag bytes (m_28/m_29) and the palette/pixel layout; on a
// 16-bit palette frame the 768-byte RGB palette is unpacked into a padded 0x400
// hardware buffer, then the pixels are copied into a fresh m_0c. When m_28 came out
// as 2 the pixels are run through the palette-remap helper. __thiscall, ret 0xc.
// @early-stop
// 79.7% - body byte-faithful through the palette-loop entry (prologue, flag-byte
// branches, m_24/m_10 setup, the operator-new + 0xfffffd00 stride, the do-while
// counter structure with the mid-body `i += 3` and `cmp 0x300/jl` all exact). The
// residual is the zero/const-register-pinning wall (docs/patterns/
// zero-register-pinning.md): retail pins the constant 2 in `bl` across the whole
// body (used for the m_28/m_29 byte stores AND the trailing `cmp [0x28],bl`) and
// keeps the m_20 palette pointer in `edi` inside the loop; our cl puts m_20 in
// `ebx` (clobbering bl -> a reload `mov bl,2` before the compare) and folds the
// induction var `i` into the address base (`lea (i,src)` + `m_10` as index) where
// retail forms `src+m_10` as the base + `i` as the scaled index. The downstream
// memcpy-remainder + Remap-tail register naming all cascade from that one loop
// allocation. No source lever flips it under /O2. Logic complete; deferred to the
// final sweep.
RVA(0x001490d0, 0x173)
i32 CImageOwned::Build(CImageBuildDesc* src, i32 size, i32 fmt) {
    i32 flags = src->m_04;
    if ((flags & 0x40) || (flags & 0x200)) {
        if ((u8)fmt == 0x10) {
            m_28 = 1;
            m_29 = 2;
        } else {
            m_28 = 1;
            m_29 = 1;
        }
    } else if ((u8)fmt == 0x10) {
        m_28 = 2;
        m_29 = 2;
    } else {
        m_28 = 1;
        m_29 = 1;
    }

    if (src->m_04 & 0x100) {
        m_24 = src->m_18;
    } else {
        m_24 = -1;
    }

    i32 stride = size - 0x20;
    m_10 = stride;
    if ((u8)fmt != 0x8 && (u8)fmt != 0x10) {
        return 0;
    }

    if (src->m_04 & 0x80) {
        stride -= 0x300;
        m_10 = stride;
        if ((u8)fmt == 0x10) {
            if (m_20 != 0) {
                RezFree(m_20);
            }
            m_20 = operator new(0x400);
            i32 i = 0;
            i32 d = 0;
            do {
                d += 4;
                ((u8*)m_20)[d - 4] = ((u8*)src + m_10)[i + 0x20];
                i += 3;
                ((u8*)m_20)[d - 3] = ((u8*)src + m_10)[i + 0x1e];
                ((u8*)m_20)[d - 2] = ((u8*)src + m_10)[i + 0x1f];
            } while (i < 0x300);
        }
    }

    m_04 = src->m_08;
    m_08 = src->m_0c;
    if (m_0c != 0) {
        RezFree(m_0c);
    }
    m_0c = operator new(m_10);
    memcpy(m_0c, src->m_20, m_10);

    if (m_28 == 2) {
        void* remapped = Remap(m_0c);
        RezFree(m_0c);
        m_0c = remapped;
        RezFree(m_20);
        m_20 = 0;
    }
    return 1;
}
