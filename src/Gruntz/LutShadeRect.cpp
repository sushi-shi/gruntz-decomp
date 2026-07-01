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

// operator new / operator delete (NAFXCW) - external, reloc-masked rel32.
void* operator new(u32 n);
void operator delete(void* p);

// The three shade-LUT banks (u8[] read as u16 windows), keyed by the fade
// offset + the channel value.
extern u8 g_lutBank0_673ca0[];
extern u8 g_lutBank1_653ca0[];
extern u8 g_lutBank2_663ca0[];

// The active pixel-format descriptor (channel shifts/sizes), selecting variant.
DATA(0x00683ea0)
extern i32 g_pfRedShift; // 0x683ea0
DATA(0x00683ea4)
extern i32 g_pfGreenSize; // 0x683ea4
DATA(0x00683eac)
extern i32 g_pfRedSize; // 0x683eac
DATA(0x00683eb0)
extern i32 g_pfGreenShift; // 0x683eb0
DATA(0x00683eb4)
extern i32 g_pfBlueSize; // 0x683eb4

struct LutSurf {
    void (**m_vtbl)(void*, i32); // +0x00 dispatch table (slot 0x80/4 = notify)
};
struct LutBlit {
    char m_pad0[8];
    LutSurf* m_8; // +0x08
    char m_padc[0x18 - 0xc];
    i32 m_18; // +0x18 clip bottom bound
    i32 m_1c; // +0x1c clip right bound
    i32 m_20; // +0x20 surface pitch (bytes; /2 = pixels per row)
    u16* GetSrc(i32); // 0x13e6d0 __thiscall -> source pixel buffer
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
i32 LutBlit::ShadeRect(i32 pct, RECT* clip) {
    if (pct > 100) {
        return 0;
    }
    RECT rc;
    if (clip) {
        if (clip->left < 0) {
            return 0;
        }
        if (clip->right > m_1c) {
            return 0;
        }
        if (clip->top < 0) {
            return 0;
        }
        if (clip->bottom > m_18) {
            return 0;
        }
        CopyRect(&rc, clip);
    } else {
        rc.left = 0;
        rc.top = 0;
        rc.right = m_1c;
        rc.bottom = m_18;
    }
    i32 scale = pct * 32 / 100;
    u16* src = GetSrc(0);
    i32 rowPix = m_20 / 2;
    u16* srcPix = src + rc.top * rowPix + rc.left;
    i32 stride = rc.left - rc.right + rowPix;
    i32 width = rc.right - rc.left;
    i32 height = rc.bottom - rc.top;
    u16* scratch = (u16*)operator new(width * 4);
    i32 off = scale << 11;

    if (g_pfRedSize == 3) {
        if (g_pfGreenShift == 3 && g_pfBlueSize == 3 && g_pfRedShift == 0xa &&
            g_pfGreenSize == 5) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 5;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = (u16)(*(u16*)((char*)g_lutBank2_663ca0 + off + (blue << 6)) |
                                      *(u16*)((char*)g_lutBank1_653ca0 + off + (green << 6)) |
                                      *(u16*)((char*)g_lutBank0_673ca0 + off + red * 2));
                }
                srcPix += stride;
            }
        } else if (g_pfGreenShift == 2 && g_pfBlueSize == 3 && g_pfRedShift == 0xb &&
                   g_pfGreenSize == 5) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 6;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = (u16)(*(u16*)((char*)g_lutBank2_663ca0 + off + (blue << 6)) |
                                      *(u16*)((char*)g_lutBank1_653ca0 + off + (green << 6)) |
                                      *(u16*)((char*)g_lutBank0_673ca0 + off + red * 2));
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

SIZE_UNKNOWN(LutBlit);
SIZE_UNKNOWN(LutSurf);
