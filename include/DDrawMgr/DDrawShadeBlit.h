#ifndef GRUNTZ_CDDRAWSHADEBLIT_H
#define GRUNTZ_CDDRAWSHADEBLIT_H

#include <Ints.h>
#include <rva.h>

class CString;    // real MFC CString (4-byte ptr); completed via <Mfc.h> in the .cpp
class CDDSurface; // the held DirectDraw surface (Blit's src arg); <DDrawMgr/DDSurface.h>

SIZE_UNKNOWN(ShadeRect);
typedef struct tagRECT ShadeRect; // (incomplete-ok: unifies with windows.h's tagRECT)

SIZE_UNKNOWN(ShadeDescr);
struct ShadeDescr {
    char m_00[0x8];
    u8* m_lut; // +0x08 LUT/palette base
};

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
    );                                       // 0x148d40  (/GX, CByteArray RLE encode)
    i32 LoadFromFile(CString name, i32 fmt); // 0x148fc0  (/GX, open + slurp + Build)
    // Lock a source DirectDraw surface, RLE-encode its locked bits (via BuildRle) with
    // the surface's own width/height/pitch, then unlock it. Returns BuildRle's result.
    i32 BuildFromSurface(CDDSurface* surf, i32 keyVal, void* palette); // 0x148f50
    i32 Build(CImageBuildDesc* src, i32 size, i32 fmt);                // 0x1490d0
    // 0x1495d0 (body: src/Image/ImageRle16Encode.cpp; ex the fake CImageRle16 view).
    // Two-pass RLE re-encode: expands the 8bpp token stream through a palette->16bpp
    // table into a fresh 16bpp RLE buffer. Build uses it as the srcBpp==2 remap.
    void* EncodeRle16(const u8* src);
    void Teardown();           // 0x148d10
    i32 DecodeFrame(CString name, CImageFrameRebuildDesc desc); // 0x149250 (body: ImageSaveBmp.cpp)
    i32 Rebuild(CString name, i32 a1, i32 a2);                  // 0x1493b0
    i32 Decompress(void* dest);                                 // 0x1494b0 (RLE expand)

    // --- blit side (src/DDrawMgr/DDrawShadeBlit.cpp) -----------------------------
    // Position the sprite at (x,y) on `dstSurf`: build a {x,y,x+w-1,y+h-1} destination
    // rect + a {0,0,w-1,h-1} clip rect from m_width/m_height, then forward to Blit.
    i32 BlitAt(CDDSurface* dstSurf, i32 x, i32 y, i32 sel, i32 p4);              // 0x149780
    i32 Blit(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 sel, i32 p4); // 0x1497f0
    // 0x14dd90 - latch the draw type and select the shade/palette descriptor: writes
    // m_drawType (+0x14) = mode, then m_palDescr (+0x1c) = descr, or, when descr is null,
    // the mode's global default from g_shadeDescr208..220. This is the REAL owner of that
    // rva (the disasm writes exactly [ecx+0x14] and [ecx+0x1c] - this class's own two
    // fields). It used to wear FOUR names: a declared-only CDDrawShadeBlit::Notify, a
    // whole fake `ShadeSelector` class that the body was BOUND to, a `CImageFormat::SetType`
    // on the deleted ImageFrame.h view, and a __stdcall `ImageNotify` free function - which
    // was not even the right calling convention (retail leaves ecx = m_owned from the null
    // test at 0x152ffb and calls straight through, i.e. __thiscall on the owned sprite).
    void Select(i32 mode, ShadeDescr* descr); // 0x14dd90
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
    // The three blend LUT banks are word tables (every read site indexes them as
    // u16); the seeds keep byte-granular g_clut arithmetic (odd interior offsets).
    u16* m_lutBank0; // +0x30 blend LUT bank 0 (g_clut interior plane R, +0x20002)
    u16* m_lutBank1; // +0x34 blend LUT bank 1 (g_clut interior plane G, +0x2)
    u16* m_lutBank2; // +0x38 blend LUT bank 2 (g_clut interior plane B, +0x10002)
};

#endif // GRUNTZ_CDDRAWSHADEBLIT_H
