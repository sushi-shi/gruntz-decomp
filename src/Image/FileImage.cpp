#include <StdAfx.h>
#include <Io/File.h>

#include <Image/FileImage.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DirPal.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/RasterRowOrder.h>
#include <Enums.h>
#include <Image/RasterData.h>
#include <Image/FileImageRecords.h>
#include <Image/Image.h>
#include <Image/ImagePool.h>
#include <Pix16.h>

#include <ddraw.h>
#include <string.h>

PALETTEENTRY g_paletteRampBuf[0x100];

static PALETTEENTRY s_palBmp[0x100];

i32 CDDSurface::CreateFromBmpData(
    CDDrawDeviceManager* manager,
    BmpFileImage* image,
    i32 dataSize,
    i32 surfaceCaps
) {
    u8* pData = static_cast<u8*>(static_cast<void*>(image));
    u8* pStart = pData;
    BITMAPINFOHEADER* pBmiHdr =
        static_cast<BITMAPINFOHEADER*>(static_cast<void*>(pData + sizeof(BITMAPFILEHEADER)));

    ColorDepth sourceBitDepth = static_cast<ColorDepth>(pBmiHdr->biBitCount);
    i32 width = pBmiHdr->biWidth;
    i32 height = pBmiHdr->biHeight;
    if (sourceBitDepth != BPP_PALETTED_8 && sourceBitDepth != BPP_RGB_24) {
        return 0;
    }

    i32 convert = 0;
    ColorDepth displayBitDepth = manager->m_displayColorDepth;
    if (displayBitDepth != sourceBitDepth) {
        convert = 1;
    }
    if (convert && displayBitDepth == BPP_PALETTED_8 && manager->m_hasPalette == false) {
        return 0;
    }

    PALETTEENTRY* pal = NULL;
    if (convert && sourceBitDepth == BPP_PALETTED_8) {
        RGBQUAD* sourcePalette = image->m_info.m_bmiColors;
        COPY_BGRX_PALETTE(g_paletteRampBuf, sourcePalette, i, PALETTE_ENTRY_COUNT)
        pal = g_paletteRampBuf;
    } else if (convert && displayBitDepth == BPP_PALETTED_8) {
        pal = manager->GetActivePalette();
    }

    if (CDDSurface::BlitSurf(manager, width, height, BPP_UNSET, surfaceCaps) == BPP_UNSET) {
        return 0;
    }

    pData = pStart + image->m_fh.bfOffBits;
    if (convert) {
        if (Blit(pData, sourceBitDepth, pal, RASTER_ROWS_BOTTOM_UP) == BPP_UNSET) {
            return 0;
        }
    } else {
        if (BlitDirect(pData, RASTER_ROWS_BOTTOM_UP) == 0) {
            return 0;
        }
    }
    return 1;
}

i32 CDDSurface::CreateFromBmpFile(CDDrawDeviceManager* manager, const char* path, i32 surfaceCaps) {
    io::File file;
    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }
    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (buf == NULL) {
        return 0;
    }
    if (file.read(buf, len) != len) {
        delete[] buf;
        return 0;
    }
    RecordBytes<BmpFileImage> data;
    data.m_bytes = buf;
    i32 result = CreateFromBmpData(manager, data.m_rec, len, surfaceCaps);
    delete[] buf;
    return result;
}

i32 CDDSurface::DecodeBmp(CDDrawDeviceManager* manager, BmpFileImage* image, u32 dataSize) {
    BITMAPINFOHEADER* ih = &image->m_info.m_bmiHeader;
    i32 width = ih->biWidth;
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    i32 height = ih->biHeight;
    if (m_apiDesc.dwWidth == width && m_apiDesc.dwHeight == height
        && (bitcount == BPP_PALETTED_8 || bitcount == BPP_RGB_24)) {
        i32 remap = 0;
        ColorDepth palBpp = manager->m_displayColorDepth;
        if (palBpp != bitcount) {
            remap = 1;
        }
        if (!remap || palBpp != BPP_PALETTED_8 || manager->HasPalette() != 0) {
            PALETTEENTRY* palette = NULL;
            if (remap && bitcount == BPP_PALETTED_8) {
                RGBQUAD* src = image->m_info.m_bmiColors;
                COPY_BGRX_PALETTE(s_palBmp, src, i, 0x100)
                palette = s_palBmp;
            } else if (remap && palBpp == BPP_PALETTED_8) {
                palette = manager->GetActivePalette();
            }

            RecordBytes<BmpFileImage> data;
            data.m_rec = image;
            u8* pixels = data.m_bytes + image->m_fh.bfOffBits;
            if (remap) {
                if (Blit(pixels, bitcount, palette, RASTER_ROWS_BOTTOM_UP) == BPP_UNSET) {
                    return 0;
                }
            } else if (BlitDirect(pixels, RASTER_ROWS_BOTTOM_UP) == 0) {
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

i32 CDDSurface::LoadBmp(CDDrawDeviceManager* manager, char* path) {
    io::File file;

    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }

    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }

    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }

    if (file.read(buf, len) != len) {
        delete[] buf;
        return 0;
    }

    RecordBytes<BmpFileImage> data;
    data.m_bytes = buf;
    i32 result = DecodeBmp(manager, data.m_rec, len);
    delete[] buf;
    return result;
}

i32 CDDSurface::Load(CDDrawDeviceManager* manager, char* resourceName, i32 surfaceCaps) {
    HRSRC hr = FindResourceA(g_resModule, resourceName, RT_BITMAP);
    if (!hr) {
        return 0;
    }
    HGLOBAL hg = LoadResource(g_resModule, hr);
    if (!hg) {
        return 0;
    }

    BITMAPINFOHEADER* bih = static_cast<BITMAPINFOHEADER*>(LockResource(hg));
    if (!bih) {
        return 0;
    }
    i32 width = bih->biWidth;
    i32 height = bih->biHeight;
    if (static_cast<ColorDepth>(bih->biBitCount) != BPP_PALETTED_8) {
        return 0;
    }
    memset(&m_apiDesc, 0, sizeof(m_apiDesc));
    m_apiDesc.dwSize = sizeof(DDSURFACEDESC);
    m_apiDesc.ddsCaps.dwCaps = surfaceCaps | DDSCAPS_OFFSCREENPLAIN;
    m_apiDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    m_apiDesc.dwWidth = width;
    m_apiDesc.dwHeight = height;
    if (!CDDSurface::CreateFromDesc(manager, NULL)) {
        return 0;
    }

    RecordBytes<BITMAPINFOHEADER> ib;
    ib.m_rec = bih;
    BlitDirect(ib.m_bytes + bih->biSize + 256 * sizeof(RGBQUAD), RASTER_ROWS_BOTTOM_UP);
    return 1;
}

i32 CDDSurface::SaveDispatch(char* path, CFileImagePal* pal, i32 flag) {
    switch (m_bitDepth) {
        case BPP_RGB_24:
            return SaveTga(path, pal, flag);
        case BPP_RGB_16:
            return SaveRle16(path, pal, flag);
        case BPP_PALETTED_8:
            return SaveBmp(path, pal, flag);
        default:
            return 0;
    }
}

i32 CDDSurface::SaveBmp(const char* path, CFileImagePal* pal, i32 mode) {
    if (this->IsValid() == 0) {
        return 0;
    }
    if (path == NULL) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (m_bitDepth != BPP_PALETTED_8) {
        return 0;
    }
    CFileImagePal* src = pal;
    if (src == NULL) {
        return 0;
    }
    if (src->m_srcPalette == NULL) {
        return 0;
    }

    Bmp256Info info;
    memset(&info.m_bmiHeader, 0, sizeof(info.m_bmiHeader));
    info.m_bmiHeader.biSize = sizeof(info.m_bmiHeader);
    info.m_bmiHeader.biWidth = m_apiDesc.dwWidth;
    i32 height = m_apiDesc.dwHeight;
    info.m_bmiHeader.biHeight = height;
    info.m_bmiHeader.biPlanes = 1;
    info.m_bmiHeader.biBitCount = 8;
    info.m_bmiHeader.biCompression = BI_RGB;
    info.m_bmiHeader.biSizeImage = 0;

    PALETTEENTRY* spal = src->m_srcPalette;
    if (spal == NULL) {
        return 0;
    }

    for (i32 i = 0; i < 0x100; i++) {
        info.m_bmiColors[i].rgbRed = spal[i].peRed;
        info.m_bmiColors[i].rgbGreen = spal[i].peGreen;
        info.m_bmiColors[i].rgbBlue = spal[i].peBlue;
    }

    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    fh.m_hdr.bfSize = height * m_apiDesc.dwWidth + 0x436;
    fh.m_hdr.bfOffBits = 0x436;

    u8* buf = static_cast<u8*>(Lock(NULL));
    if (buf == NULL) {
        return 0;
    }

    io::File file;
    if (mode != 0) {
        if (!file.open(path, io::Update)) {
            Unlock();
            return 0;
        }

        file.seek(0, io::End);
    } else {
        if (!file.open(path, io::Replace)) {
            Unlock();
            return 0;
        }
    }

    file.write(&fh.m_hdr, sizeof(fh.m_hdr));
    file.write(&info, sizeof(info));

    i32 row = m_apiDesc.dwHeight;
    while (--row >= 0) {
        file.write(buf + row * m_apiDesc.lPitch, m_apiDesc.dwWidth);
    }

    Unlock();
    return file.finish();
}

i32 CDDSurface::SaveRle16(char* path, CFileImagePal* pal, i32 flag) {
    static_cast<void>(pal);
    if (!path || !*path || !IsValid() || m_bitDepth != BPP_RGB_16) return 0;
    io::File file;
    if (!file.open(path, flag ? io::Update : io::Replace)) return 0;
    if (flag && !file.seek(0, io::End)) return 0;
    return SaveRle16(file) && file.finish();
}

i32 CDDSurface::SaveRle16(io::Output& target) {
    if (!IsValid() || m_bitDepth != BPP_RGB_16 || !target.good()) return 0;
    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    i32 width = this->m_apiDesc.dwWidth;
    BmpFileHeaderStamp bfh;
    memset(&bfh, 0, sizeof(bfh));
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    i32 height = this->m_apiDesc.dwHeight;
    strcpy(bfh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = IDX(BPP_RGB_24);
    bfh.m_hdr.bfSize = height * width * 3 + 0x3a;
    bfh.m_hdr.bfOffBits = 0x3a;

    u8* line = new u8[3 * width];
    if (line == NULL) {
        return 0;
    }

    u8* locked = static_cast<u8*>(Lock(NULL));
    if (locked == NULL) {
        delete[] line;
        return 0;
    }

    bool written = target.write(&bfh.m_hdr, sizeof(bfh.m_hdr))
        && target.write(&bi, sizeof(bi));

    i32 row = this->m_apiDesc.dwHeight;
    while (written && --row >= 0) {
        u8* src = locked + row * this->m_apiDesc.lPitch;
        i32 x = 0;
        u8* dst = line;
        while (x < static_cast<i32>(this->m_apiDesc.dwWidth)) {
            u16 px = Load16(src);
            src += 2;
            u8 r;
            u8 g;
            u8 b;
            UnpackPixel16(px, r, g, b);
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
            x++;
        }
        written = target.write(line, 3 * this->m_apiDesc.dwWidth);
    }

    Unlock();
    delete[] line;
    return written && target.good();
}

i32 CDDSurface::SaveTga(const char* path, CFileImagePal* pal, i32 mode) {
    static_cast<void>(pal);
    if (this->IsValid() == 0) {
        return 0;
    }
    if (path == NULL) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (m_bitDepth != BPP_RGB_24) {
        return 0;
    }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    i32 width = m_apiDesc.dwWidth;
    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    i32 height = m_apiDesc.dwHeight;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = 0;
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    fh.m_hdr.bfSize = height * width * 3 + 0x3a;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = IDX(BPP_RGB_24);
    fh.m_hdr.bfOffBits = 0x3a;

    u8* buf = static_cast<u8*>(Lock(NULL));
    if (buf == NULL) {
        return 0;
    }

    io::File file;
    if (mode != 0) {
        if (!file.open(path, io::Update)) {
            Unlock();
            return 0;
        }

        file.seek(0, io::End);
    } else {
        if (!file.open(path, io::Replace)) {
            Unlock();
            return 0;
        }
    }

    file.write(&fh.m_hdr, sizeof(fh.m_hdr));
    file.write(&bi, sizeof(bi));

    for (i32 row = m_apiDesc.dwHeight - 1; row >= 0; row--) {
        i32 col = 0;
        if (static_cast<i32>(m_apiDesc.dwWidth) > 0) {
            do {
                file.write(buf + row * m_apiDesc.lPitch, m_apiDesc.dwWidth * 3);
                ++col;
            } while (col < static_cast<i32>(m_apiDesc.dwWidth));
        }
    }

    Unlock();
    return file.finish();
}

i32 CDDSurface::UploadRaster(CDDrawDeviceManager* manager, raster::Image& image) {
    if (!manager || image.width != m_apiDesc.dwWidth || image.height != m_apiDesc.dwHeight) return 0;
    const ColorDepth sourceDepth = image.channels == 1 ? BPP_PALETTED_8 : BPP_RGB_24;
    if (sourceDepth == m_bitDepth) {
        u8* target = static_cast<u8*>(Lock(NULL));
        if (!target) return 0;
        const i32 pitch = m_apiDesc.lPitch;
        const bool copied = pitch > 0 && image.height <= 0x7fffffffU / static_cast<u32>(pitch)
            && raster::copyRows(image, target, static_cast<size_t>(pitch) * image.height, pitch, true, false);
        Unlock();
        return copied;
    }
    PALETTEENTRY colors[256];
    PALETTEENTRY* palette = manager->GetActivePalette();
    if (image.channels == 1 && image.palette.size() == 768) {
        for (size_t i = 0; i < 256; ++i) {
            colors[i].peRed = image.palette[i * 3]; colors[i].peGreen = image.palette[i * 3 + 1];
            colors[i].peBlue = image.palette[i * 3 + 2]; colors[i].peFlags = 0;
        }
        palette = colors;
    }
    if ((sourceDepth == BPP_PALETTED_8 || m_bitDepth == BPP_PALETTED_8) && !palette) return 0;
    if (sourceDepth == BPP_RGB_24 && m_bitDepth == BPP_PALETTED_8) {
        // The existing quantizer consumes BGR; portable decoded storage is RGB.
        std::vector<u8> bgr(image.pixels.size());
        if (!raster::copyRows(image, &bgr[0], bgr.size(), image.width * 3, true, false)) return 0;
        return Blit(&bgr[0], sourceDepth, palette, RASTER_ROWS_TOP_DOWN);
    }
    return Blit(&image.pixels[0], sourceDepth, palette, RASTER_ROWS_TOP_DOWN);
}

i32 CDDSurface::CreateFromPcxData(CDDrawDeviceManager* manager, PcxHeader* image,
    i32 dataSize, i32 surfaceCaps) {
    raster::Image decoded;
    if (!manager || dataSize < 0 || raster::decodePcx(image, static_cast<size_t>(dataSize), decoded) != raster::Decoded) return 0;
    if (!BlitSurf(manager, decoded.width, decoded.height, BPP_UNSET, surfaceCaps)) return 0;
    return UploadRaster(manager, decoded);
}

i32 CDDSurface::CreateFromPcxFile(CDDrawDeviceManager* manager, const char* path, i32 surfaceCaps) {
    io::File file;
    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }
    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }
    RecordBytes<PcxHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (fileData.m_bytes == NULL) {
        return 0;
    }
    if (file.read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }
    i32 result = CreateFromPcxData(manager, fileData.m_rec, len, surfaceCaps);
    delete[] fileData.m_bytes;
    return result;
}

i32 CDDSurface::DecodePcx(CDDrawDeviceManager* manager, PcxHeader* image, u32 dataSize) {
    raster::Image decoded;
    if (raster::decodePcx(image, dataSize, decoded) != raster::Decoded) return 0;
    return UploadRaster(manager, decoded);
}

i32 CDDSurface::LoadPcx(CDDrawDeviceManager* manager, char* path) {
    io::File file;

    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }

    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }

    RecordBytes<PcxHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (!fileData.m_bytes) {
        return 0;
    }

    if (file.read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }

    i32 result = DecodePcx(manager, fileData.m_rec, len);
    delete[] fileData.m_bytes;
    return result;
}

i32 CDDSurface::DecodePcxData(CDDrawDeviceManager* manager, PidHeader* image,
    i32 dataSize, i32 surfaceCaps, u32 colorKey) {
    raster::Image decoded;
    if (!manager || dataSize < 0 || raster::decodePid(image, static_cast<size_t>(dataSize), decoded) != raster::Decoded) return 0;
    if (decoded.flags & IDX(PID_SYSTEM_MEMORY)) surfaceCaps = (surfaceCaps & ~DDSCAPS_VIDEOMEMORY) | DDSCAPS_SYSTEMMEMORY;
    else if (decoded.flags & IDX(PID_VIDEO_MEMORY)) surfaceCaps &= ~DDSCAPS_SYSTEMMEMORY;
    if (!BlitSurf(manager, decoded.width, decoded.height, BPP_UNSET, surfaceCaps)
        || !UploadRaster(manager, decoded)) return 0;
    if (decoded.flags & IDX(PID_TRANSPARENCY)) FillPalette(colorKey);
    return 1;
}

i32 CDDSurface::DecodePcxEx(
    CDDrawDeviceManager* manager,
    char* path,
    i32 surfaceCaps,
    u32 colorKey
) {
    io::File file;

    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }

    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    RecordBytes<PidHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (!fileData.m_bytes) {
        return 0;
    }

    if (file.read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }

    i32 result = DecodePcxData(manager, fileData.m_rec, len, surfaceCaps, colorKey);
    delete[] fileData.m_bytes;
    return result;
}

i32 CDDSurface::DecodePid(CDDrawDeviceManager* manager, PidHeader* image, u32 dataSize, u32 colorKey) {
    raster::Image decoded;
    if (raster::decodePid(image, dataSize, decoded) != raster::Decoded || !UploadRaster(manager, decoded)) return 0;
    if (decoded.flags & IDX(PID_TRANSPARENCY)) FillPalette(colorKey);
    return 1;
}

i32 CDDSurface::LoadPid(CDDrawDeviceManager* manager, char* path, u32 colorKey) {
    io::File file;

    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }

    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    RecordBytes<PidHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (!fileData.m_bytes) {
        return 0;
    }

    if (file.read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }

    i32 result = DecodePid(manager, fileData.m_rec, len, colorKey);
    delete[] fileData.m_bytes;
    return result;
}
