#ifndef SRC_IMAGE_IMAGE_H
#define SRC_IMAGE_IMAGE_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Image/RezDecodeKind.h>
#include <Io/FileStream.h>

class CDDrawPtrCollections;

struct CImagePaletteNode;

struct CRezFillRect {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};
SIZE(0x10);

struct ScanlinePalette {
    char m_pad0[8];
    PALETTEENTRY m_colors[256];
};
SIZE_UNKNOWN();

class CRezImage {
public:
    CRezImage() {
        m_dibSection = NULL;
        m_pixels = NULL;
        m_rowOffsets = NULL;
        m_reserved434 = 0;
        m_width = 0;
        m_height = 0;
        m_stride = 0;
        m_rowPad = 0;
        m_listPosition = NULL;
        m_paletteScalar = 0;
        m_paletteNode = NULL;
    }

    i32 LoadFromRez(char* name, HDC dc, i32 ctrl);

    i32 LoadBmp(char* name, HDC dc, i32 ctrl);
    i32 LoadPcx(char* name, HDC dc, i32 ctrl);
    i32 LoadRid(char* name, HDC dc, i32 ctrl);
    i32 LoadPid(char* name, HDC dc, i32 ctrl);
    i32 LoadDefault(char* name, HDC dc, i32 ctrl);

    i32 DecodeBmpHeader(HDC dc, i32 width, i32 height, ColorDepth bitcount, i32 ctrl);
    i32 DecodePcxData(void* buf, HDC dc, i32 ctrl);
    i32 DecodeRidData(void* buf, HDC dc, i32 ctrl);
    i32 DecodePidData(void* buf, HDC dc, i32 ctrl);
    i32 DecodeBmpData(void* buf, HDC dc, i32 ctrl);

    i32 DecodeBlit(void* src, HDC dc, i32 width, i32 height, ColorDepth bitcount, i32 ctrl);

    i32 DispatchDecode(void* buf, RezDecodeKind kind, HDC dc, i32 ctrl);
    i32 Convert8To16(HDC dc, CRezImage* src, void* pal);
    i32 EnsureSize(HDC dc, i32 w, i32 h, ColorDepth bitCount, i32 flag);
    void Fill(i32 value);
    void Free();
    void SetPalette(void* paletteNode, i32 scalar);
    i32 Save(const char* filename, void* paletteObj);
    i32 SaveBmp(const char* filename, void* paletteObj);
    void FillRect(CRezFillRect* r, i32 color);
    void FillRectAt(i32 dx, i32 dy, CRezFillRect* src, i32 color);
    void FlipVertical();
    i32 PasteFrom(CRezImage* src, i32 x, i32 y);

    union {
        BITMAPINFO m_bmi;
        struct {
            BITMAPINFOHEADER m_bih;
            u16 m_pal[256];
        };
    };
    char m_pad228[0x428 - 0x228];
    HBITMAP m_dibSection;

    u8* m_pixels;
    i32* m_rowOffsets;
    i32 m_reserved434;
    i32 m_width;
    i32 m_height;
    ColorDepth m_bitCount;
    i32 m_stride;
    i32 m_rowPad;
    POSITION m_listPosition;
    i32 m_transparent;
    i32 m_paletteScalar;
    CImagePaletteNode* m_paletteNode;
};
SIZE_UNKNOWN();

class CFileImageSurface : public CDDSurface {
public:
    virtual ~CFileImageSurface() OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;

    virtual i32
    ResolveEx(void* surf, void* buf, FileImageFormat type, u32 size, i32 ctrl, i32 trans);
    virtual i32 LoadByExt(CDDrawPtrCollections* info, char* path, i32 flags, i32 key);
    virtual i32
    LoadKeyed(void* surf, i32 width, i32 height, ColorDepth bitDepth, i32 caps, i32 key);
};
SIZE(0xc0);

class CFileImagePal {
public:
    char _00[0x0c];
    PALETTEENTRY* m_srcPalette;
};
SIZE_UNKNOWN();

#endif // SRC_IMAGE_IMAGE_H
