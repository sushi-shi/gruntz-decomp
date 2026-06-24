// CDDrawShadeBlit.h - the DDrawMgr software shaded-sprite blitter (tracer
// placeholder ClassUnknown_86). A non-polymorphic blit descriptor: the
// CopyRect/RenderFrame path (src/Gruntz/*CopyRect*) builds one off [obj+0x30] and
// calls Blit() (0x1497f0), which validates the destination rect, picks the
// per-draw-type translucency LUTs, and dispatches to one of four per-mode RLE
// blit loops. The big inner loop (0x14a200) decodes a high-bit RLE sprite stream
// into a global scratch line (DAT_006bed08) and writes it through the 64KB blend
// tables to a locked DirectDraw surface.
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. Engine callees (Lock / the sibling blit loops) are reloc-masked.
#ifndef GRUNTZ_CDDRAWSHADEBLIT_H
#define GRUNTZ_CDDRAWSHADEBLIT_H

#include <Ints.h>

// The clip rectangle the blit path hands in (3rd arg of Blit): left/top/right/
// bottom-ish (only +0x0/+0x4/+0x8/+0xc are read in the bounds check).
struct ShadeRect {
    i32 m_00; // +0x00
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
};

// The render-source object (2nd arg of Blit). Its +0xb0 is the blend mode
// (==2 enables the special global-state check); the inner loop reads +0x0/+0x4/
// +0xc geometry off the same family of source objects.
struct ShadeSrc {
    char m_pad[0xb0];
    i32 m_b0; // +0xb0 blend mode
};

class CDDrawShadeBlit {
public:
    i32 Blit(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 sel, i32 p4); // 0x1497f0
    void BlitLoop(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 p4);     // 0x14a200
    // The other three per-blend-mode loop variants (sibling functions, no body):
    // thiscall on the same object so the call falls out with no stack cleanup.
    void BlitMode_149950(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 p4);
    void BlitMode_149d00(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 p4);
    void BlitMode_14b770(i32 p0, ShadeSrc* src, ShadeRect* clip, i32 p4);

    char m_00[0x4];
    i32 m_04; // +0x04 width
    i32 m_08; // +0x08 height
    char m_0c[0x14 - 0x0c];
    i32 m_14;   // +0x14 draw type
    i32 m_18;   // +0x18 light level (>>3 indexes the LUT bank)
    void* m_1c; // +0x1c palette / source-descriptor pointer
    char m_20[0x28 - 0x20];
    u8 m_28; // +0x28 byte flag (require-mode gate)
    u8 m_29; // +0x29 blend mode mirror (= src->m_b0)
    char m_2a[0x2c - 0x2a];
    i32 m_2c; // +0x2c global-state-equal flag
    u8* m_30; // +0x30 LUT bank 0 (blue/channel 2)
    u8* m_34; // +0x34 LUT bank 1 (green/channel 1)
    u8* m_38; // +0x38 LUT bank 2 (red/channel 0)
};

#endif // GRUNTZ_CDDRAWSHADEBLIT_H
