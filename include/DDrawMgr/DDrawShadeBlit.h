#ifndef GRUNTZ_CDDRAWSHADEBLIT_H
#define GRUNTZ_CDDRAWSHADEBLIT_H

#include <rva.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Enums.h>
#include <Ints.h>

class CString;
class CDDSurface;

typedef struct tagRECT ShadeRect;

struct PidHeader;

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

class CDDrawShadeBlit {
public:
    CDDrawShadeBlit();
    i32
    BuildRle(void* pixels, i32 width, i32 height, i32 stride, i32 keyVal, PALETTEENTRY* palette);
    i32 LoadFromFile(CString name, ColorDepth fmt);

    i32 BuildFromSurface(CDDSurface* surf, i32 keyVal, PALETTEENTRY* palette);
    i32 Build(PidHeader* src, i32 size, GZ_ENUM_PARAM(ColorDepth, u8) fmt);

    u8* EncodeRle16(const u8* src);
    void Teardown();
    i32 DecodeFrame(CString name, CImageFrameRebuildDesc desc);

    i32 Rebuild(CString name, i32 offsetX, i32 offsetY);
    i32 Decompress(u8* dest);

    i32 BlitAt(CDDSurface* dstSurf, i32 x, i32 y, i32 sel, i32 vflip);
    i32 Blit(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 sel, i32 vflip);

    void Select(ShadeMode mode, CShadeTable* descr);

    void BlitCopyForward(ShadeRect* dst, CDDSurface* surf, ShadeRect* clip, i32 vflip);
    void BlitCopyMirrored(ShadeRect* dst, CDDSurface* surf, ShadeRect* clip, i32 vflip);
    void BlitShadedForward(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 vflip);
    void BlitShadedMirrored(ShadeRect* dst, CDDSurface* surf, ShadeRect* clip, i32 vflip);

    void ConvertRow(u8* dst, u8* src, i32 count);

    void ConvertRowFlip(u8* dst, u8* src, i32 count);

    void ConvertRowDoubleFwd(u8* dst, u8* src, i32 count, i32 rowDelta);

    void ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta);

    i32 m_doubleScanlines;
    i32 m_width;
    i32 m_height;
    u8* m_rleData;
    u32 m_rleLen;
    ShadeMode m_drawType;
    i32 m_light;
    CShadeTable* m_palDescr;

    PALETTEENTRY* m_palette;
    i32 m_colorKey;
    u8 m_srcBpp;
    u8 m_dstBpp;
    char _2a[0x2c - 0x2a];
    i32 m_blendVariant;

    u16* m_lutBank0;
    u16* m_lutBank1;
    u16* m_lutBank2;
};

extern u8 g_scratch[];
extern CShadeTable* g_shadeDescr208;
extern CShadeTable* g_shadeDescr20c;
extern CShadeTable* g_shadeDescr210;
extern CShadeTable* g_shadeDescr214;
extern CShadeTable* g_blendDescr;
extern CShadeTable* g_shadeDescr21c;
extern CShadeTable* g_shadeDescr220;

#endif // GRUNTZ_CDDRAWSHADEBLIT_H
