// LutShadeRect.cpp - a DirectDraw 16bpp shade/fade helper migrated out of
// src/Stub/ApiCallers.cpp (RVA 0x13f460). __thiscall ShadeRect(pct, clip):
// validate + clip the target rectangle, scale the fade percentage into a LUT
// bank offset, then walk the source surface rectangle row-by-row (copy the row
// to a scratch line, split each RGB565/555 pixel and recombine the three
// channels through the three shade-LUT banks, write back in place), and finally
// notify the surface + free the scratch line. Placeholder m_<hexoffset> field
// names; only OFFSETS + code bytes are load-bearing. The pixel-format descriptor
// globals gate two channel-shift variants (565 vs 555).
#include <Win32.h>

#include <rva.h>
#include <string.h>
#include <Globals.h>

// operator new / operator delete (NAFXCW) - external, reloc-masked rel32.
void* operator new(u32 n);
void operator delete(void* p);

// The three shade-LUT banks (u8[] read as u16 windows), keyed by the fade
// offset + the channel value.

// The active pixel-format descriptor (channel shifts/sizes), selecting variant.

// The held DirectDraw surface (m_8): dispatched thiscall through vtbl slot 0x80
// (Unlock/notify). Same held surface CFileImage models as CFileImageHeldSurface in
// <Image/Image.h>; only the notify slot is used here.
struct HeldDDSurface {
    void (**m_vtbl)(void*, i32); // +0x00 dispatch table (slot 0x80/4 = Unlock/notify)
};

// CFileImage - the DIRSURF.CPP surface (full class in <Image/Image.h>). Modeled as a
// partial view here: only the shade path's fields + Lock (0x13e6d0) are pinned. m_8
// is the held DirectDraw surface, m_18/m_1c the surface height/width, m_20 the pitch.
struct CFileImage {
    char m_pad0[8];
    HeldDDSurface* m_8; // +0x08 held DirectDraw surface
    char m_padc[0x18 - 0xc];
    i32 m_height;                       // +0x18 surface height (clip bottom bound)
    i32 m_width;                        // +0x1c surface width  (clip right bound)
    i32 m_pitch;                        // +0x20 surface pitch (bytes; /2 = pixels per row)
    u16* Lock(i32);                     // 0x13e6d0 __thiscall -> locked source pixel buffer
    i32 ShadeRect(i32 pct, RECT* clip); // 0x13f460
};

// @early-stop
// regalloc wall (~67%, was a 0.9% `return 0` stub). Logic + offsets + structure
// are faithful (verified base-vs-target via llvm-objdump -dr: the clip/CopyRect,
// scale imul-by-100, geometry, operator-new, memcpy row copy, both config-gated
// variants and the notify/free tails all line up instruction-for-instruction).
// The residual is a whole-function register-coloring divergence: retail pins
// this->edi and pct/off->esi and uses direct `test`/`jl` on the clip fields,
// while cl pins this->esi and materializes a zero in edi (regalloc-zero-pin) for
// the same compares - so every [this+off] ModRM byte and the channel-split reg
// choices differ. A regalloc coin-flip, not a codegen miss.
RVA(0x0013f460, 0x2da)
i32 CFileImage::ShadeRect(i32 pct, RECT* clip) {
    if (pct > 100) {
        return 0;
    }
    RECT rc;
    if (clip) {
        if (clip->left < 0) {
            return 0;
        }
        if (clip->right > m_width) {
            return 0;
        }
        if (clip->top < 0) {
            return 0;
        }
        if (clip->bottom > m_height) {
            return 0;
        }
        CopyRect(&rc, clip);
    } else {
        rc.left = 0;
        rc.top = 0;
        rc.right = m_width;
        rc.bottom = m_height;
    }
    i32 scale = pct * 32 / 100;
    u16* src = Lock(0);
    i32 rowPix = m_pitch / 2;
    u16* srcPix = src + rc.top * rowPix + rc.left;
    i32 stride = rc.left - rc.right + rowPix;
    i32 width = rc.right - rc.left;
    i32 height = rc.bottom - rc.top;
    u16* scratch = (u16*)operator new(width * 4);
    i32 off = scale << 11;

    if (g_pfRedSize == 3) {
        if (g_pfGreenShift == 3 && g_pfBlueSize == 3 && g_pfRedShift == 0xa && g_pfGreenSize == 5) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 5;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = (u16)(*(u16*)((char*)g_lutBank2_663ca0 + off + (blue << 6))
                                      | *(u16*)((char*)g_lutBank1_653ca0 + off + (green << 6))
                                      | *(u16*)((char*)g_lutBank0_673ca0 + off + red * 2));
                }
                srcPix += stride;
            }
        } else if (g_pfGreenShift == 2 && g_pfBlueSize == 3 && g_pfRedShift == 0xb
                   && g_pfGreenSize == 5) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 6;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = (u16)(*(u16*)((char*)g_lutBank2_663ca0 + off + (blue << 6))
                                      | *(u16*)((char*)g_lutBank1_653ca0 + off + (green << 6))
                                      | *(u16*)((char*)g_lutBank0_673ca0 + off + red * 2));
                }
                srcPix += stride;
            }
        } else {
            operator delete(scratch);
            m_8->m_vtbl[0x80 / 4](m_8, 0);
            return 0;
        }
    } else {
        operator delete(scratch);
        m_8->m_vtbl[0x80 / 4](m_8, 0);
        return 0;
    }

    m_8->m_vtbl[0x80 / 4](m_8, 0);
    operator delete(scratch);
    return 1;
}

SIZE_UNKNOWN(CFileImage);
SIZE_UNKNOWN(HeldDDSurface);
