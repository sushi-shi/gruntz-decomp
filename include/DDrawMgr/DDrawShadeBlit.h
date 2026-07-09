// DDrawShadeBlit.h - the DDrawMgr software shaded-sprite blitter (tracer
// placeholder tomalla-86). A non-polymorphic blit descriptor: the
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
#include <rva.h>

class CString;    // real MFC CString (4-byte ptr); completed via <Mfc.h> in the .cpp
                  // (fwd-decl so includers needn't choose <Mfc.h> vs <Win32.h>)
class CDDSurface; // the held DirectDraw surface (Blit's src arg); <DDrawMgr/DDSurface.h>

// The clip rectangle the blit path hands in (3rd arg of Blit): left/top/right/
// bottom-ish (only +0x0/+0x4/+0x8/+0xc are read in the bounds check).
SIZE_UNKNOWN(ShadeRect);
struct ShadeRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};

// The render-source / destination surface (2nd arg of Blit) IS the DirectDraw
// surface wrapper CDDSurface (<DDrawMgr/DDSurface.h>): the blit loops Lock() it
// (CDDSurface::Lock @0x13e6d0 -> locked dest bits), read its +0x20 pitch, its +0xb0
// blend field, and Unlock via the held IDirectDrawSurface at +0x08. The former
// ShadeSrc placeholder was that same physical struct (same offsets/methods) - unified
// into CDDSurface so the sprite blitters pass CImage::m_surface with no facet cast.

// A palette/format descriptor: m_1c on the blitter and the global g_blendDescr
// (DAT_006bf218) both point at one; only its +0x8 (a LUT/palette base) is read
// here. Read both as u8* (case 2/3/4/6 8-bit tables) and u16* (case 7/10 16-bit
// palettes), so the field stays a raw base.
SIZE_UNKNOWN(ShadeDescr);
struct ShadeDescr {
    char m_00[0x8];
    u8* m_lut; // +0x08 LUT/palette base
};

// The CImageFrameDesc as seen by CDDrawShadeBlit::Build: a flag word at +0x04 (bits
// 0x40/0x80/0x100/0x200 steer the decode), two ints copied to the sprite's
// +0x04/+0x08 (width/height), a +0x18 byte, and the raw frame data starting at +0x20.
class CImageBuildDesc {
public:
    char _00[0x04];
    i32 m_flags;  // +0x04  decode flags
    i32 m_width;  // +0x08  -> sprite width
    i32 m_height; // +0x0c  -> sprite height
    char _10[0x18 - 0x10];
    u8 m_srcKey; // +0x18  -> sprite color key when 0x100 set
    char _19[0x20 - 0x19];
    u8 m_frameData[1]; // +0x20  raw frame data (palette + pixels)
};

// The 8-dword (0x20) by-value frame descriptor Rebuild builds on the stack and hands
// to DecodeFrame: [0]=0, [1]=flags, [2]=width, [3]=height, [4]/[5]=the two int args,
// [6]=the low byte of the color key (0 when key==-1), [7]=0. Only the layout is
// load-bearing (passed whole via rep movs).
struct CImageFrameRebuildDesc {
    i32 f0;
    i32 f1;
    i32 f2;
    i32 f3;
    i32 f4;
    i32 f5;
    i32 f6;
    i32 f7;
};

// CDDrawShadeBlit IS the +0x30 owned object of the RTTI CImage (built by
// CImage::BuildSlot13, `new CDDrawShadeBlit()` @0x148ce0, 0x3c bytes): the shaded
// sprite. The build/decode methods (BuildRle/LoadFromFile/Build/Rebuild/...) prime
// the decoded RLE pixel buffer (+0x0c) + palette (+0x20) and the blit-descriptor
// defaults (draw type 1 @+0x14, light 0x80 @+0x18, src/dst bpp 1 @+0x28/+0x29, key
// -1 @+0x24); the blit methods draw it. The former CImageOwned placeholder was that
// same physical struct - unified so CImage::m_owned carries no facet cast.
SIZE(CDDrawShadeBlit, 0x3c);
class CDDrawShadeBlit {
public:
    // --- build / decode side (former CImageOwned; src/Image/ImageOwned.cpp) -----
    CDDrawShadeBlit(); // 0x148ce0
    i32 BuildRle(
        void* pixels,
        i32 width,
        i32 height,
        i32 stride,
        i32 keyVal,
        void* palette
    );                                                  // 0x148d40  (/GX, CByteArray RLE encode)
    i32 LoadFromFile(CString name, i32 fmt);            // 0x148fc0  (/GX, open + slurp + Build)
    i32 Build(CImageBuildDesc* src, i32 size, i32 fmt); // 0x1490d0
    void* Remap(void* pixels);                          // 0x1495d0  (palette-remap, external)
    void Teardown();                                    // 0x148d10
    i32 DecodeFrame(CString name, CImageFrameRebuildDesc desc); // 0x149250 (external)
    i32 Rebuild(CString name, i32 a1, i32 a2);                  // 0x1493b0
    i32 Decompress(void* dest);                                // 0x1494b0 (RLE expand)

    // --- blit side (src/DDrawMgr/DDrawShadeBlit.cpp) -----------------------------
    i32 Blit(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 sel, i32 p4); // 0x1497f0
    void Notify(i32 a, i32 b);                                                   // 0x14dd90
    // The unselected (h-aligned) RLE blit; sel picks the h-flipped sibling. The
    // big inner loops decode the high-bit RLE sprite stream (m_rleData/m_rleLen) into
    // the Lock'd destination surface, clipping x to [clip->left, clip->right].
    void BlitMode_149950(ShadeRect* dst, CDDSurface* surf, ShadeRect* clip, i32 vflip); // 0x149950
    void BlitMode_149d00(ShadeRect* dst, CDDSurface* surf, ShadeRect* clip, i32 vflip); // 0x149d00
    void BlitLoop(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 p4);            // 0x14a200
    void BlitMode_14b770(ShadeRect* dst, CDDSurface* surf, ShadeRect* clip, i32 vflip); // 0x14b770
    // The per-row format converter the inner blit loops call: dispatches on
    // (m_drawType - 2) to one of nine palette/blend conversions over a row.
    void ConvertRow(u8* dst, u8* src, i32 count); // 0x14c9f0
    // The h-flipped (right-to-left) twin of ConvertRow: same nine (m_14-2) blend
    // cases, but dst is walked DOWN and the saved-dest scratch line is read back to
    // front. Used by the selected (mirrored) blit path BlitMode_14b770.
    void ConvertRowFlip(u8* dst, u8* src, i32 count); // 0x14cfc0
    // The forward (left-to-right) twin of ConvertRowDouble: same dual-write (dst and
    // dst+rowDelta), but dst and the saved-dest scratch line walk UP. Dense (m_14-2)
    // jump table over cases 2/3/7/8 (4/5/6 fall through). Case 3 is symmetric (both
    // rows get the m_light LUT of the saved dest; src is unused).
    void ConvertRowDoubleFwd(u8* dst, u8* src, i32 count, i32 rowDelta); // 0x14d5e0
    // The dual-write (vertical-double) row converter: each pixel is written to dst
    // and dst+rowDelta. Five (m_14-2) blend cases (2/3/7/8); 4/5/6 fall through.
    void ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta); // 0x14d950

    i32 m_00;       // +0x00
    i32 m_width;    // +0x04 sprite row width (Build: src->m_width)
    i32 m_height;   // +0x08 height (Build: src->m_height)
    u8* m_rleData;  // +0x0c RLE sprite-stream base (decoded pixel buffer; new / RezFree)
    i32 m_rleLen;   // +0x10 RLE sprite-stream length (byte bound; pixel byte count)
    i32 m_drawType; // +0x14 draw type / row-convert selector (switch tag; ctor default 1)
    i32 m_light; // +0x18 light level (ctor default 0x80): >>3 selects the LUT bank (Blit); low index into m_palDescr->m_lut (cases 3/4); alpha (case 6); fill byte (case 5)
    ShadeDescr* m_palDescr; // +0x1c palette / source-descriptor pointer (ctor default 0)
    u8* m_palette; // +0x20 256-entry (0x400 B) palette byte buffer (Build/BuildRle; new 0x400 / RezFree)
    i32 m_colorKey; // +0x24 color key (ctor default -1)
    u8 m_srcBpp; // +0x28 source pixel size in bytes (1 or 2); RLE run stride, ==1 gate; ctor default 1
    u8 m_dstBpp; // +0x29 dest pixel size in bytes (= blend mode, used as row stride); ctor default 1
    char _2a[0x2c - 0x2a];
    i32 m_blendVariant; // +0x2c pixel-format blend variant flag (RGB555/565 shift check)
    u8* m_lutBank0;     // +0x30 blend LUT bank 0 (from g_lutBank0_673ca0)
    u8* m_lutBank1;     // +0x34 blend LUT bank 1 (from g_lutBank1_653ca0)
    u8* m_lutBank2;     // +0x38 blend LUT bank 2 (from g_lutBank2_663ca0)
};

#endif // GRUNTZ_CDDRAWSHADEBLIT_H
