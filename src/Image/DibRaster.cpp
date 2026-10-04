#include <StdAfx.h>
#include <Image/Image.h>
#include <Image/ImagePaletteNode.h>
#include <DDrawMgr/PixelFormatMacros.h>
#include <Image/RasterData.h>
#include <ComOutRef.h>
#include <DDrawMgr/PaletteSize.h>
#include <SafeDelete.h>
#include <string.h>

i32 CDib::Init(HDC dc, i32 width, i32 height, ColorDepth bitcount, u32 ctrl) {
    if (width <= 0 || !height || height == (-2147483647 - 1)
        || (bitcount != BPP_PALETTED_8 && bitcount != BPP_RGB_16 && bitcount != BPP_RGB_24 && bitcount != BPP_RGB_32)) return 0;
    m_dwFlags = 0;
    m_nWidth = width;
    m_nHeight = (height < 0) ? -height : height;
    m_nDepth = bitcount;
    const u32 bytesPerPixel = IDX(bitcount) / 8;
    if (static_cast<u32>(width) > (0x7fffffffU - 3) / bytesPerPixel) return 0;
    m_nPitch = (width * bytesPerPixel + 3) & ~3U;
    if (static_cast<u32>(m_nHeight) > 0x7fffffffU / static_cast<u32>(m_nPitch)) return 0;
    m_nStride = m_nPitch - width * bytesPerPixel;
    m_bPalOwner = 0;
    m_pPal = NULL;
    m_bTransparent = true;
    memset(&m_bmi.m_hdr, 0, sizeof(BITMAPINFOHEADER));
    m_bmi.m_hdr.biWidth = m_nWidth;
    m_bmi.m_hdr.biBitCount = static_cast<WORD>(IDX(m_nDepth));
    m_bmi.m_hdr.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.m_hdr.biHeight = height;
    m_bmi.m_hdr.biPlanes = 1;
    m_bmi.m_hdr.biCompression = BI_RGB;
    m_bmi.m_hdr.biSizeImage = 0;
    m_bmi.m_hdr.biClrUsed = 0;
    m_bmi.m_hdr.biClrImportant = 0;

    u16* pal = static_cast<u16*>(static_cast<void*>(m_bmi.m_colors));
    if (m_nDepth == BPP_PALETTED_8) {
        for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
            *pal++ = static_cast<u16>(i);
        }
        m_hBmp = CreateDIBSection(
            dc,
            static_cast<BITMAPINFO*>(static_cast<void*>(&m_bmi)),
            DIB_PAL_COLORS,
            PtrOut(&m_pBytes),
            NULL,
            0
        );
    } else {
        m_hBmp = CreateDIBSection(
            dc,
            static_cast<BITMAPINFO*>(static_cast<void*>(&m_bmi)),
            DIB_RGB_COLORS,
            PtrOut(&m_pBytes),
            NULL,
            0
        );
    }
    if (!m_hBmp) {
        return 0;
    }
    m_pLines = new u32[m_nHeight];
    for (i32 i = 0; i < m_nHeight; i++) {
        m_pLines[i] = (height < 0 ? i : m_nHeight - i - 1) * m_nPitch;
    }
    return 1;
}

void CDib::Term() {
    if (m_hBmp) {
        DeleteObject(m_hBmp);
        m_hBmp = NULL;
    }
    SAFE_DELETE_ARRAY(m_pLines);
    m_pBytes = NULL;
    m_pPal = NULL;
}

i32 CDib::InitRaster(raster::Image& image, HDC dc, u32 flags) {
    if (!Init(dc, image.width, image.height, image.channels == 1 ? BPP_PALETTED_8 : BPP_RGB_24, flags)) return 0;
    return raster::copyRows(image, m_pBytes, GetBufferSize(), m_nPitch, true, true);
}

i32 CDib::InitPcx(u8* buf, u32 dataSize, HDC dc, u32 ctrl) {
    raster::Image image;
    return raster::decodePcx(buf, dataSize, image) == raster::Decoded && InitRaster(image, dc, ctrl);
}

i32 CDib::InitPid(u8* buf, u32 dataSize, HDC dc, u32 ctrl) {
    raster::Image image;
    if (raster::decodePid(buf, dataSize, image) != raster::Decoded || !InitRaster(image, dc, ctrl)) return 0;
    m_bTransparent = (ctrl & 1) != 0 || (image.flags & IDX(PID_GRAMMAR_SKIPRUN)) != 0;
    return 1;
}

i32 CDib::Init(u8* src, HDC dc, i32 width, i32 height, ColorDepth bitcount, u32 ctrl, RasterRowOrder rowOrder) {
    if (!src || !Init(dc, width, height, bitcount, ctrl)) {
        return 0;
    }
    const size_t rowBytes = static_cast<size_t>(width) * (IDX(bitcount) / 8);
    for (i32 row = 0; row < GetHeight(); ++row) {
        const i32 targetRow = rowOrder == RASTER_ROWS_TOP_DOWN ? row : GetHeight() - row - 1;
        memcpy(GetAddress(targetRow), src + row * rowBytes, rowBytes);
    }
    return 1;
}

i32 CDib::Init(HDC dc, CDib* src, CDibPal* pal) {
    u8* srcBuf;
    u16* destBuf;
    i32 x;
    i32 y;
    PALETTEENTRY* entries;
    PALETTEENTRY entry;

    if (pal == NULL) {
        return 0;
    }
    entries = pal->GetPes();
    if (entries == NULL) {
        return 0;
    }
    if (!Init(dc, src->GetWidth(), src->GetHeight(), BPP_RGB_16, 0)) {
        return 0;
    }

    for (y = 0; y < GetHeight(); y++) {
        srcBuf = src->GetAddress(y);
        destBuf = reinterpret_cast<u16*>(GetAddress(y));

        for (x = 0; x < GetWidth(); x++) {
            entry = entries[*srcBuf];
            *destBuf = RGB_TO_16(entry);
            srcBuf++;
            destBuf++;
        }
    }
    return 1;
}

void CDibPal::Term() {
    if (m_hPal) {
        DeleteObject(m_hPal);
        m_hPal = NULL;
    }
    m_dwFlags = 0;
}
