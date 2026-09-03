#ifndef SRC_IMAGE_IMAGE_H
#define SRC_IMAGE_IMAGE_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Image/RezDecodeKind.h>
#include <Io/FileStream.h>

class CDDrawDeviceManager;

struct CDibPal;

struct DIB_BMI256 {
    BITMAPINFOHEADER hdr;
    RGBQUAD colors[256];
};

GZ_ENUM_FLAGS_BEGIN(DibInitFlags, u32)
    DIB_INIT_FLAGS_NONE = 0,
    DIB_INIT_KEEP_TRANSPARENCY = 0x1
GZ_ENUM_FLAGS_END(DibInitFlags, u32)
GZ_ENUM_FLAGS_OPS(DibInitFlags)

class CDib {
public:
    CDib();
    ~CDib() {
        Term();
    }

    i32 Init(HDC dc, i32 width, i32 height, ColorDepth depth = BPP_PALETTED_8, u32 flags = 0);
    i32 Init(
        u8* bytes,
        HDC dc,
        i32 width,
        i32 height,
        ColorDepth depth = BPP_PALETTED_8,
        u32 flags = 0
    );
    i32 Init(u8* bytes, RezDecodeKind type, HDC dc, u32 flags = 0);
    i32 Init(const char* file, HDC dc, u32 flags = 0);
    i32 Init(HDC dc, CDib* dib, CDibPal* palette);
    void Term();

    b32 IsValid() {
        return m_hBmp != NULL && m_pBytes != NULL && m_pLines != NULL;
    }

    i32 InitBmp(u8* bytes, HDC dc, u32 flags = 0);
    i32 InitPcx(u8* bytes, HDC dc, u32 flags = 0);
    i32 InitRid(u8* bytes, HDC dc, u32 flags = 0);
    i32 InitPid(u8* bytes, HDC dc, u32 flags = 0);
    i32 InitBmp(const char* file, HDC dc, u32 flags = 0);
    i32 InitPcx(const char* file, HDC dc, u32 flags = 0);
    i32 InitRid(const char* file, HDC dc, u32 flags = 0);
    i32 InitPid(const char* file, HDC dc, u32 flags = 0);
    i32 InitRes(const char* file, HDC dc, u32 flags = 0);

    u8* GetBytes() {
        return m_pBytes;
    }
    u16* GetBuf16() {
        // The surviving API's byte-evidenced DIB storage has a 16-bit view.
        return reinterpret_cast<u16*>(m_pBytes);
    }
    HBITMAP GetBitmap() {
        return m_hBmp;
    }
    i32 GetWidth() {
        return m_nWidth;
    }
    i32 GetHeight() {
        return m_nHeight;
    }
    ColorDepth GetDepth() {
        return m_nDepth;
    }
    i32 GetPitch() {
        return m_nPitch;
    }
    i32 GetStride() {
        return m_nStride;
    }
    u8 GetPixel(i32 x, i32 y) {
        return m_pBytes[m_pLines[y] + x];
    }
    u32 GetBufferSize() {
        return m_nPitch * m_nHeight;
    }
    u32 GetFlags() {
        return m_dwFlags;
    }
    u32 GetIndex(i32 y) {
        return m_pLines[y];
    }
    u32 GetIndex(i32 x, i32 y) {
        return m_pLines[y] + x;
    }
    u8* GetAddress(i32 y) {
        return &m_pBytes[m_pLines[y]];
    }
    u8* GetAddress(i32 x, i32 y) {
        return &m_pBytes[m_pLines[y]] + x;
    }
    POSITION GetPos() {
        return m_pos;
    }
    CDibPal* GetPalette() {
        return m_pPal;
    }
    void SetPixel(i32 x, i32 y, u8 pixel) {
        m_pBytes[m_pLines[y] + x] = pixel;
    }
    void SetPos(POSITION pos) {
        m_pos = pos;
    }
    void SetTransparent(b32 transparent) {
        m_bTransparent = transparent;
    }
    void SetPalette(CDibPal* palette, b32 owner = false);

    i32 Resize(HDC dc, i32 width, i32 height, ColorDepth depth = BPP_PALETTED_8, u32 flags = 0);
    i32 Scale(i32 newWidth, i32 newHeight, i32 newDepth, u32 flags = 0);

    void Invert();
    void Mirror();
    void Mirvert() {
        Invert();
        Mirror();
    }

    i32 Blt(HDC dc) {
        return Blt(dc, 0, 0);
    }
    i32 Blt(HDC dc, i32 x, i32 y);
    i32 Blt(HDC dc, i32 x, i32 y, i32 width, i32 height);
    i32 Blt(CDib* dib, i32 x, i32 y);
    i32 StretchBlt(
        HDC dc,
        i32 dstX,
        i32 dstY,
        i32 dstWidth,
        i32 dstHeight,
        i32 srcX,
        i32 srcY,
        i32 srcWidth,
        i32 srcHeight,
        u32 rop = SRCCOPY
    );

    void Fill(u8 pixel);
    void Clear() {
        Fill(0);
    }
    void FillRect(RECT* rect, u32 color);
    void FillRect(i32 x, i32 y, RECT* sourceRect, u32 color);

    u8* Lock() {
        return GetBytes();
    }
    void Unlock() {}

    b32 IsStrideless() {
        return m_nStride == 0;
    }
    b32 IsTransparent() {
        return m_bTransparent;
    }
    b32 IsPaletteOwner() {
        return m_bPalOwner;
    }

    i32 Save(const char* filename, CDibPal* palette = NULL);

private:
    i32 Save8(const char* filename, CDibPal* palette = NULL);

    DIB_BMI256 m_bmi;
    HBITMAP m_hBmp;

    u8* m_pBytes;
    u32* m_pLines;
    u32 m_dwFlags;
    i32 m_nWidth;
    i32 m_nHeight;
    ColorDepth m_nDepth;
    i32 m_nPitch;
    i32 m_nStride;
    POSITION m_pos;
    b32 m_bTransparent;
    b32 m_bPalOwner;
    CDibPal* m_pPal;
};

inline CDib::CDib() {
    m_hBmp = NULL;
    m_pBytes = NULL;
    m_pLines = NULL;
    m_dwFlags = 0;
    m_nWidth = 0;
    m_nHeight = 0;
    m_nPitch = 0;
    m_nStride = 0;
    m_pos = NULL;
    m_bPalOwner = false;
    m_pPal = NULL;
}

inline i32 CDib::Blt(HDC dc, i32 x, i32 y) {
    ASSERT(m_nDepth == BPP_PALETTED_8 || m_nDepth == BPP_RGB_16 || m_nDepth == BPP_RGB_24);
    // DIB_BMI256 intentionally shares BITMAPINFO's API prefix.
    BITMAPINFO* bmi = reinterpret_cast<BITMAPINFO*>(&m_bmi); // API-forced
    if (m_nDepth == BPP_PALETTED_8) {
        return StretchDIBits(
            dc,
            x,
            y,
            m_nWidth,
            m_nHeight,
            0,
            0,
            m_nWidth,
            m_nHeight,
            m_pBytes,
            bmi,
            DIB_PAL_COLORS,
            SRCCOPY
        );
    }
    return StretchDIBits(
        dc,
        x,
        y,
        m_nWidth,
        m_nHeight,
        0,
        0,
        m_nWidth,
        m_nHeight,
        m_pBytes,
        bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

inline i32 CDib::Blt(HDC dc, i32 x, i32 y, i32 width, i32 height) {
    ASSERT(m_nDepth == BPP_PALETTED_8 || m_nDepth == BPP_RGB_16 || m_nDepth == BPP_RGB_24);
    // DIB_BMI256 intentionally shares BITMAPINFO's API prefix.
    BITMAPINFO* bmi = reinterpret_cast<BITMAPINFO*>(&m_bmi); // API-forced
    if (m_nDepth == BPP_PALETTED_8) {
        return StretchDIBits(
            dc,
            x,
            y,
            width,
            height,
            0,
            0,
            m_nWidth,
            m_nHeight,
            m_pBytes,
            bmi,
            DIB_PAL_COLORS,
            SRCCOPY
        );
    }
    return StretchDIBits(
        dc,
        x,
        y,
        width,
        height,
        0,
        0,
        m_nWidth,
        m_nHeight,
        m_pBytes,
        bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

inline i32 CDib::StretchBlt(
    HDC dc,
    i32 dstX,
    i32 dstY,
    i32 dstWidth,
    i32 dstHeight,
    i32 srcX,
    i32 srcY,
    i32 srcWidth,
    i32 srcHeight,
    u32 rop
) {
    ASSERT(m_nDepth == BPP_PALETTED_8 || m_nDepth == BPP_RGB_16 || m_nDepth == BPP_RGB_24);
    // DIB_BMI256 intentionally shares BITMAPINFO's API prefix.
    BITMAPINFO* bmi = reinterpret_cast<BITMAPINFO*>(&m_bmi); // API-forced
    return StretchDIBits(
        dc,
        dstX,
        dstY,
        dstWidth,
        dstHeight,
        srcX,
        srcY,
        srcWidth,
        srcHeight,
        m_pBytes,
        bmi,
        m_nDepth == BPP_PALETTED_8 ? DIB_PAL_COLORS : DIB_RGB_COLORS,
        rop
    );
}

class CFileImageSurface : public CDDSurface {
public:
    virtual ~CFileImageSurface() OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;

    virtual i32 ResolveEx(
        CDDrawDeviceManager* manager,
        void* data,
        FileImageFormat format,
        u32 dataSize,
        i32 surfaceCaps,
        i32 colorKey
    );
    virtual i32 LoadByExt(CDDrawDeviceManager* manager, char* path, i32 surfaceCaps, i32 colorKey);
    virtual i32 LoadKeyed(
        CDDrawDeviceManager* manager,
        i32 width,
        i32 height,
        ColorDepth bitDepth,
        i32 caps,
        i32 colorKey
    );
};

class CFileImagePal {
public:
    char _00[0x0c];
    PALETTEENTRY* m_srcPalette;
};

#endif // SRC_IMAGE_IMAGE_H
