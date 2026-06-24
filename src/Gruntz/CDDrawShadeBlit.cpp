// CDDrawShadeBlit.cpp - DDrawMgr software shaded-sprite blitter (tracer
// placeholder ClassUnknown_86). Methods in ascending retail-RVA order.
//
// 0x1497f0 Blit       - validate the destination rect, select the per-draw-type
//                       translucency LUTs, dispatch to one of four mode loops.
// 0x14a200 BlitLoop   - the big RLE-decode + 64KB-blend-table inner blit loop.
//
// Field names are placeholders; offsets + code bytes are load-bearing. The three
// other mode-loop callees (0x149950/0x149d00/0x14b770) and the surface Lock are
// external/reloc-masked. See <Gruntz/CDDrawShadeBlit.h> for the layout.
#include <Gruntz/CDDrawShadeBlit.h>

#include <rva.h>

// The live screen RGB-format shift table at 0x683ea0..0x683eb4 - already named by
// CLightFxRender.cpp; the mode-2 gate compares these against the magic 5/10/3/3/3
// state. Reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4

// The three 2048-byte-strided translucency LUT banks selected by the light level.
DATA(0x00253ca0)
extern u8 g_lutBank1_653ca0[];
DATA(0x00263ca0)
extern u8 g_lutBank2_663ca0[];
DATA(0x00273ca0)
extern u8 g_lutBank0_673ca0[];

RVA(0x001497f0, 0x154)
i32 CDDrawShadeBlit::Blit(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 sel, i32 p4) {
    if (clip->m_00 < 0 || clip->m_08 > m_04 - 1 || clip->m_04 < 0 || clip->m_0c > m_08 - 1) {
        return 0;
    }

    i32 mode = src->m_b0;
    m_29 = (u8)mode;
    if ((u8)mode == 2) {
        if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
            m_2c = 1;
        } else {
            m_2c = 0;
        }
    }

    i32 drawType = m_14;
    if (drawType == 1) {
        if (sel) {
            BlitMode_149d00(p0, src, clip, p4);
        } else {
            BlitMode_149950(p0, src, clip, p4);
        }
        return 1;
    }

    if (drawType == 7) {
        if (m_28 != 1 || (u8)mode != 2) {
            return 0;
        }
    }
    if (drawType == 0xa || drawType == 0xb) {
        if (m_28 != 1 || (u8)mode != 2) {
            return 0;
        }
    }
    if (drawType == 8 || drawType == 0xb) {
        i32 bank = (m_18 >> 3) * 0x800;
        m_30 = g_lutBank0_673ca0 + bank;
        m_34 = g_lutBank1_653ca0 + bank;
        m_38 = g_lutBank2_663ca0 + bank;
    }

    if (sel) {
        BlitMode_14b770(p0, src, clip, p4);
    } else {
        BlitLoop(p0, src, clip, p4);
    }
    return 1;
}

// @early-stop
// 5363 B software alpha-compositor: deferred to the final sweep (too large to
// converge in budget; a partial body diverges regalloc and under-counts). It
// Lock()s the destination DirectDraw surface (src->m_20 via CDirSurf::Lock
// 0x13e6d0, the only call carrying the "C:\Proj\DDrawMgr\DIRSURF.CPP" string),
// then runs the per-blend-mode RLE blit: it scans the high-bit RLE sprite stream
// (`test cl,0x80` -> run vs literal), decodes each row into the global scratch
// line DAT_006bed08, and writes pixels through the 64KB 2D blend tables
// (LUT[(srcByte<<8)|dstByte], via this->m_30/m_34/m_38 and the [this->m_1c]+8
// channel table) into the locked surface. FOUR dense jump tables dispatch on
// (this->m_14 - 2): 0x54b6f4 / 0x54b710 (8/10 cases) for the 8bpp paths and
// 0x54b738 / 0x54b754 for the 16bpp paths (`mov si,[esi+ebp*2]; or; mov [eax],cx`).
// DAT_006bf218 is a secondary surface/format descriptor. Identity, switch tags,
// and the LUT plumbing are confirmed; the per-case blend bodies remain.
RVA(0x0014a200, 0x14f3)
void CDDrawShadeBlit::BlitLoop(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 p4) {}
