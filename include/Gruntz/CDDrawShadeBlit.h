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

// The unlock interface (ShadeSrc::m_08): a COM-like object whose vtable slot 0x20
// releases the surface lock. Typed vtable so the dispatch (mov eax,[iface];
// call [eax+0x80]) falls out with no cast.
struct ShadeUnlockIface {
    struct Vtbl {
        void* s0[0x20];
        void(__stdcall* Unlock)(ShadeUnlockIface*, i32); // slot 0x20 -> [vtbl+0x80]
    }* vtbl;
};

// The render-source / destination surface (2nd arg of Blit). +0xb0 is the blend
// mode (==2 enables the special global-state check; read in Blit). The per-mode
// blit loops Lock() it (0x13e6d0 -> locked dest bits), read +0x20 (row pitch),
// and Unlock via +0x08.
struct ShadeSrc {
    char m_00[0x08];
    ShadeUnlockIface* m_08; // +0x08 unlock interface
    char m_0c[0x20 - 0x0c];
    i32 m_20; // +0x20 surface pitch (row stride, bytes)
    char m_24[0xb0 - 0x24];
    i32 m_b0;             // +0xb0 blend mode
    u8* Lock(void* rect); // 0x13e6d0 (CDDSurface::Lock; external, reloc-masked)
};

// A palette/format descriptor: m_1c on the blitter and the global g_blendDescr
// (DAT_006bf218) both point at one; only its +0x8 (a LUT/palette base) is read
// here. Read both as u8* (case 2/3/4/6 8-bit tables) and u16* (case 7/10 16-bit
// palettes), so the field stays a raw base.
struct ShadeDescr {
    char m_00[0x8];
    u8* m_08; // +0x08 LUT/palette base
};

class CDDrawShadeBlit {
public:
    i32 Blit(ShadeRect* dst, ShadeSrc* src, ShadeRect* clip, i32 sel, i32 p4); // 0x1497f0
    // The unselected (h-aligned) RLE blit; sel picks the h-flipped sibling. The
    // big inner loops decode the high-bit RLE sprite stream (m_0c/m_10) into the
    // Lock'd destination surface, clipping x to [clip->m_00, clip->m_08].
    void BlitMode_149950(ShadeRect* dst, ShadeSrc* surf, ShadeRect* clip, i32 vflip); // 0x149950
    void BlitMode_149d00(ShadeRect* dst, ShadeSrc* surf, ShadeRect* clip, i32 vflip); // 0x149d00
    void BlitLoop(ShadeRect* dst, ShadeSrc* src, ShadeRect* clip, i32 p4);            // 0x14a200
    void BlitMode_14b770(ShadeRect* dst, ShadeSrc* surf, ShadeRect* clip, i32 vflip); // 0x14b770
    // The per-row format converter the inner blit loops call: dispatches on
    // (m_14 - 2) to one of nine palette/blend conversions over a row.
    void ConvertRow(u8* dst, u8* src, i32 count); // 0x14c9f0
    // The h-flipped (right-to-left) twin of ConvertRow: same nine (m_14-2) blend
    // cases, but dst is walked DOWN and the saved-dest scratch line is read back to
    // front. Used by the selected (mirrored) blit path BlitMode_14b770.
    void ConvertRowFlip(u8* dst, u8* src, i32 count); // 0x14cfc0
    // The dual-write (vertical-double) row converter: each pixel is written to dst
    // and dst+rowDelta. Five (m_14-2) blend cases (2/3/7/8); 4/5/6 fall through.
    void ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta); // 0x14d950

    char m_00[0x4];
    i32 m_04; // +0x04 sprite row width
    i32 m_08; // +0x08 height
    u8* m_0c; // +0x0c RLE sprite-stream base
    i32 m_10; // +0x10 RLE sprite-stream length (byte bound)
    i32 m_14;         // +0x14 draw type / row-convert selector
    i32 m_18;         // +0x18 light level (>>3 indexes the LUT bank) / alpha / fill byte
    ShadeDescr* m_1c; // +0x1c palette / source-descriptor pointer
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
