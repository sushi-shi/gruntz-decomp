#include <Image/FileImage.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DirPal.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/RasterRowOrder.h>
#include <Enums.h>
#include <Image/ByteRunEncoding.h>
#include <Image/FileImageRecords.h>
#include <Image/Image.h>
#include <Image/ImagePool.h>
#include <Pix16.h>

#include <ddraw.h>
#include <string.h>

DATA(0x00283ef0)
PALETTEENTRY g_paletteRampBuf[0x100];
DATA(0x002842f0)
static PALETTEENTRY s_palBmp[0x100];
DATA(0x002846f0)
static PALETTEENTRY s_palPcx[0x100];
DATA(0x00284af0)
PALETTEENTRY g_grayRamp[0x100];
DATA(0x00284ef0)
static PALETTEENTRY s_palPidData[0x100];
DATA(0x002852f0)
static PALETTEENTRY s_palPcxData[0x100];

// @early-stop
RVA(0x00143cf0, 0x16b)
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
    CSize imageSize(pBmiHdr->biWidth, pBmiHdr->biHeight);
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
        RGBQUAD* sourcePalette = image->info.bmiColors;
        COPY_BGRX_PALETTE(g_paletteRampBuf, sourcePalette, i, PALETTE_ENTRY_COUNT)
        pal = g_paletteRampBuf;
    } else if (convert && displayBitDepth == BPP_PALETTED_8) {
        if (manager->m_hasPalette != false) {
            pal = manager->m_palette;
        } else {
            pal = NULL;
        }
    }

    if (CDDSurface::BlitSurf(manager, imageSize.cx, imageSize.cy, BPP_UNSET, surfaceCaps)
        == BPP_UNSET) {
        return 0;
    }

    pData = pStart + image->fh.bfOffBits;
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

RVA(0x00143e60, 0x15b)
i32 CDDSurface::CreateFromBmpFile(CDDrawDeviceManager* manager, const char* path, i32 surfaceCaps) {
    CFile file;
    if (!file.Open(path, 0, NULL)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (buf == NULL) {
        return 0;
    }
    if (file.Read(buf, len) != len) {
        delete[] buf;
        return 0;
    }
    RecordBytes<BmpFileImage> data;
    data.m_bytes = buf;
    i32 result = CreateFromBmpData(manager, data.m_rec, len, surfaceCaps);
    delete[] buf;
    return result;
}

static inline i32 HasPalette(CDDrawDeviceManager* manager) {
    return manager->m_hasPalette;
}

RVA(0x00143fc0, 0x142)
i32 CDDSurface::DecodeBmp(CDDrawDeviceManager* manager, BmpFileImage* image, u32 dataSize) {
    BITMAPINFOHEADER* ih = &image->info.bmiHeader;
    CSize imageSize(ih->biWidth, ih->biHeight);
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    if (m_width == imageSize.cx && m_height == imageSize.cy
        && (bitcount == BPP_PALETTED_8 || bitcount == BPP_RGB_24)) {
        i32 remap = 0;
        ColorDepth palBpp = manager->m_displayColorDepth;
        if (palBpp != bitcount) {
            remap = 1;
        }
        if (!remap || palBpp != BPP_PALETTED_8 || HasPalette(manager) != 0) {
            PALETTEENTRY* palette = NULL;
            if (remap && bitcount == BPP_PALETTED_8) {
                RGBQUAD* src = image->info.bmiColors;
                for (i32 i = 0; i < 0x100; i++) {
                    s_palBmp[i].peRed = src[i].rgbRed;
                    s_palBmp[i].peGreen = src[i].rgbGreen;
                    s_palBmp[i].peBlue = src[i].rgbBlue;
                    s_palBmp[i].peFlags = 0;
                }
                palette = s_palBmp;
            } else if (remap && palBpp == BPP_PALETTED_8) {
                if (manager->m_hasPalette != false) {
                    palette = manager->m_palette;
                } else {
                    palette = NULL;
                }
            }

            RecordBytes<BmpFileImage> data;
            data.m_rec = image;
            u8* pixels = data.m_bytes + image->fh.bfOffBits;
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

RVA(0x00144110, 0x156)
i32 CDDSurface::LoadBmp(CDDrawDeviceManager* manager, char* path) {
    CFile file;

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }

    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }

    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        delete[] buf;
        return 0;
    }

    RecordBytes<BmpFileImage> data;
    data.m_bytes = buf;
    i32 result = DecodeBmp(manager, data.m_rec, len);
    delete[] buf;
    return result;
}

RVA(0x00144270, 0xd2)
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
    CSize imageSize(bih->biWidth, bih->biHeight);
    if (static_cast<ColorDepth>(bih->biBitCount) != BPP_PALETTED_8) {
        return 0;
    }
    memset(m_descWords, 0, sizeof(DDSURFACEDESC));
    m_descSize = sizeof(DDSURFACEDESC);
    m_surfaceCaps = surfaceCaps | 0x40;
    m_descFlags = 7;
    m_width = imageSize.cx;
    m_height = imageSize.cy;
    if (!CDDSurface::CreateFromDesc(manager, NULL)) {
        return 0;
    }

    RecordBytes<BITMAPINFOHEADER> ib;
    ib.m_rec = bih;
    BlitDirect(ib.m_bytes + bih->biSize + 256 * sizeof(RGBQUAD), RASTER_ROWS_BOTTOM_UP);
    return 1;
}

RVA(0x00144350, 0x5f)
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

RVA(0x001443b0, 0x284)
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
    memset(&info.bmiHeader, 0, sizeof(info.bmiHeader));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = m_width;
    i32 height = m_height;
    info.bmiHeader.biHeight = height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 8;
    info.bmiHeader.biCompression = 0;
    info.bmiHeader.biSizeImage = 0;

    PALETTEENTRY* spal = src->m_srcPalette;
    if (spal == NULL) {
        return 0;
    }

    for (i32 i = 0; i < 0x100; i++) {
        info.bmiColors[i].rgbRed = spal[i].peRed;
        info.bmiColors[i].rgbGreen = spal[i].peGreen;
        info.bmiColors[i].rgbBlue = spal[i].peBlue;
    }

    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    fh.m_hdr.bfSize = height * m_width + 0x436;
    fh.m_hdr.bfOffBits = 0x436;

    u8* buf = static_cast<u8*>(Lock(NULL));
    if (buf == NULL) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, NULL)) {
            m_ddSurface->Unlock(NULL);
            return 0;
        }
        file.Seek(0, 2);
    } else {
        if (!file.Open(path, 0x1001, NULL)) {
            m_ddSurface->Unlock(NULL);
            return 0;
        }
    }

    file.Write(&fh.m_hdr, sizeof(fh.m_hdr));
    file.Write(&info, sizeof(info));

    i32 row = m_height;
    while (--row >= 0) {
        file.Write(buf + row * m_pitch, m_width);
    }

    m_ddSurface->Unlock(NULL);
    return 1;
}

RVA(0x00144640, 0x2be)
i32 CDDSurface::SaveRle16(char* path, CFileImagePal* pal, i32 flag) {
    if (this->IsValid() == 0) {
        return 0;
    }
    if (path == NULL) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (this->m_bitDepth != BPP_RGB_16) {
        return 0;
    }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    CSize imageSize(this->m_width, this->m_height);
    BmpFileHeaderStamp bfh;
    memset(&bfh, 0, sizeof(bfh));
    bi.bmiHeader.biCompression = 0;
    bi.bmiHeader.biSizeImage = 0;
    strcpy(bfh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = imageSize.cy;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = imageSize.cx;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = IDX(BPP_RGB_24);
    bfh.m_hdr.bfSize = imageSize.cy * imageSize.cx * 3 + 0x3a;
    bfh.m_hdr.bfOffBits = 0x3a;

    u8* line = new u8[3 * imageSize.cx];
    if (line == NULL) {
        return 0;
    }

    u8* locked = static_cast<u8*>(Lock(NULL));
    if (locked == NULL) {
        delete[] line;
        return 0;
    }

    CFile file;
    if (flag != 0) {
        if (file.Open(path, 0x2001, NULL) == false) {
            this->m_ddSurface->Unlock(NULL);
            delete[] line;
            return 0;
        }
        file.Seek(0, 2);
    } else {
        if (file.Open(path, 0x1001, NULL) == false) {
            this->m_ddSurface->Unlock(NULL);
            delete[] line;
            return 0;
        }
    }
    file.Write(&bfh.m_hdr, sizeof(bfh.m_hdr));
    file.Write(&bi, sizeof(bi));

    i32 row = this->m_height;
    while (--row >= 0) {
        u8* src = locked + row * this->m_pitch;
        i32 x = 0;
        u8* dst = line;
        while (x < this->m_width) {
            Pix16Ptr sp;
            sp.m_bytes = src;
            u16 px = *sp.m_words;
            src += 2;
            u8 r = static_cast<u8>((static_cast<u8>((px >> g_rUp)) << g_rDown));
            u8 g = static_cast<u8>((static_cast<u8>((px >> g_gUp)) << g_gDown));
            u8 b = static_cast<u8>((static_cast<u8>(px) << g_bDown));
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
            x++;
        }
        file.Write(line, 3 * this->m_width);
    }

    this->m_ddSurface->Unlock(NULL);
    delete[] line;
    return 1;
}

RVA(0x00144900, 0x227)
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
    CSize imageSize(m_width, m_height);
    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    bi.bmiHeader.biCompression = 0;
    bi.bmiHeader.biSizeImage = 0;
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = imageSize.cy;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = imageSize.cx;
    fh.m_hdr.bfSize = imageSize.cy * imageSize.cx * 3 + 0x3a;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = IDX(BPP_RGB_24);
    fh.m_hdr.bfOffBits = 0x3a;

    u8* buf = static_cast<u8*>(Lock(NULL));
    if (buf == NULL) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, NULL)) {
            m_ddSurface->Unlock(NULL);
            return 0;
        }
        file.Seek(0, 2);
    } else {
        if (!file.Open(path, 0x1001, NULL)) {
            m_ddSurface->Unlock(NULL);
            return 0;
        }
    }

    file.Write(&fh.m_hdr, sizeof(fh.m_hdr));
    file.Write(&bi, sizeof(bi));

    for (i32 row = m_height - 1; row >= 0; row--) {
        i32 col = 0;
        if (m_width > 0) {
            do {
                file.Write(buf + row * m_pitch, m_width * 3);
                ++col;
            } while (col < m_width);
        }
    }

    m_ddSurface->Unlock(NULL);
    return 1;
}

RVA(0x00144b30, 0x250)
i32 CDDSurface::CreateFromPcxData(
    CDDrawDeviceManager* manager,
    PcxHeader* image,
    i32 dataSize,
    i32 surfaceCaps
) {
    if (image == NULL) {
        return 0;
    }

    BYTE* pStart = static_cast<BYTE*>(static_cast<void*>(image));
    PcxHeader* pPcxHdr = static_cast<PcxHeader*>(static_cast<void*>(pStart));

    CSize imageSize(pPcxHdr->m_xMax - pPcxHdr->m_xMin + 1, pPcxHdr->m_yMax - pPcxHdr->m_yMin + 1);

    ColorDepth sourceBitDepth;
    if (pPcxHdr->m_planes == PCX_PLANES_PALETTED) {
        sourceBitDepth = BPP_PALETTED_8;
    } else if (pPcxHdr->m_planes == PCX_PLANES_RGB) {
        sourceBitDepth = BPP_RGB_24;
    } else {
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

    PALETTEENTRY* palette = NULL;
    if (convert && sourceBitDepth == BPP_PALETTED_8) {

        u8* p = pStart + dataSize - 0x300;
        COPY_RGB_PALETTE(g_grayRamp, p, i, 0x100)
        palette = g_grayRamp;
    } else if (convert && displayBitDepth == BPP_PALETTED_8) {
        if (manager->m_hasPalette != false) {
            palette = manager->m_palette;
        } else {
            palette = NULL;
        }
    }

    if (this->BlitSurf(manager, imageSize.cx, imageSize.cy, BPP_UNSET, surfaceCaps) == BPP_UNSET) {
        return 0;
    }

    DWORD offset = sizeof(PcxHeader);
    BYTE* pPacked = &pStart[offset];
    u8* buf = NULL;
    i32 result;
    if (convert == 0) {
        if (sourceBitDepth == BPP_PALETTED_8) {
            if (DecodeRun8(pPacked) == 0) {
                return 0;
            }
        } else {
            if (DecodeRun24(pPacked) == 0) {
                return 0;
            }
        }
    } else {
        if (sourceBitDepth == BPP_PALETTED_8) {
            if (imageSize.cx % 2 != 0) {
                return 0;
            }
            buf = new u8[imageSize.cy * imageSize.cx];
            if (buf == NULL) {
                return 0;
            }
            result = DecodeByteRun1Plane(buf, pPacked, imageSize.cx, imageSize.cy);
        } else {
            if (imageSize.cx % 2 != 0) {
                return 0;
            }
            buf = new u8[imageSize.cy * imageSize.cx * 3];
            if (buf == NULL) {
                return 0;
            }
            result = DecodeByteRun3Planes(buf, pPacked, imageSize.cx, imageSize.cy);
        }
        if (result == 0) {
            delete[] buf;
            return 0;
        }
    }

    if (convert) {
        if (Blit(buf, sourceBitDepth, palette, RASTER_ROWS_TOP_DOWN) == BPP_UNSET) {
            delete[] buf;
            return 0;
        }
    }
    if (buf != NULL) {
        delete[] buf;
    }
    return 1;
}

RVA(0x00144d80, 0x15b)
i32 CDDSurface::CreateFromPcxFile(CDDrawDeviceManager* manager, const char* path, i32 surfaceCaps) {
    CFile file;
    if (!file.Open(path, 0, NULL)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    RecordBytes<PcxHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (fileData.m_bytes == NULL) {
        return 0;
    }
    if (file.Read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }
    i32 result = CreateFromPcxData(manager, fileData.m_rec, len, surfaceCaps);
    delete[] fileData.m_bytes;
    return result;
}

RVA(0x00144ee0, 0x225)
i32 CDDSurface::DecodePcx(CDDrawDeviceManager* manager, PcxHeader* image, u32 dataSize) {
    if (image != NULL) {
        CSize imageSize(image->m_xMax - image->m_xMin + 1, image->m_yMax - image->m_yMin + 1);
        GZ_ENUM_STORAGE(PcxPlaneCount, i8) planes = image->m_planes;

        ColorDepth bitcount = BPP_UNSET;
        if (planes == PCX_PLANES_PALETTED) {
            bitcount = BPP_PALETTED_8;
        } else if (planes == PCX_PLANES_RGB) {
            bitcount = BPP_RGB_24;
        }
        if (bitcount != BPP_UNSET && m_width == imageSize.cx && m_height == imageSize.cy) {
            b32 remap = false;
            ColorDepth palBpp = manager->m_displayColorDepth;
            if (palBpp != bitcount) {
                remap = true;
            }
            if (!remap || palBpp != BPP_PALETTED_8 || manager->m_hasPalette != false) {
                PALETTEENTRY* palette = NULL;
                if (remap && bitcount == BPP_PALETTED_8) {
                    u8* src = static_cast<u8*>(static_cast<void*>(image)) + dataSize - 0x300;
                    COPY_RGB_PALETTE_DO(s_palPcx, src, i, 0x100)
                    palette = s_palPcx;
                } else if (remap && palBpp == BPP_PALETTED_8) {
                    if (manager->m_hasPalette != false) {
                        palette = manager->m_palette;
                    } else {
                        palette = NULL;
                    }
                }

                u8* pixels = static_cast<u8*>(static_cast<void*>(image)) + sizeof(PcxHeader);
                b32 ok;
                u8* decoded = NULL;
                if (!remap) {
                    if (bitcount == BPP_PALETTED_8) {
                        if (!DecodeRun8(pixels)) {
                            return 0;
                        }
                    } else {
                        if (!DecodeRun24(pixels)) {
                            return 0;
                        }
                    }
                } else {
                    if (bitcount == BPP_PALETTED_8) {
                        decoded = new u8[imageSize.cx * imageSize.cy];
                        if (decoded == NULL) {
                            return 0;
                        }
                        ok = DecodeByteRun1Plane(decoded, pixels, imageSize.cx, imageSize.cy);
                    } else {
                        decoded = new u8[imageSize.cx * imageSize.cy * 3];
                        if (decoded == NULL) {
                            return 0;
                        }
                        ok = DecodeByteRun3Planes(decoded, pixels, imageSize.cx, imageSize.cy);
                    }
                    if (!ok) {
                        delete[] decoded;
                        return 0;
                    }
                }

                if (remap) {
                    if (!Blit(decoded, bitcount, palette, RASTER_ROWS_TOP_DOWN)) {
                        delete[] decoded;
                        return 0;
                    }
                }
                if (decoded) {
                    delete[] decoded;
                }
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x00145110, 0x156)
i32 CDDSurface::LoadPcx(CDDrawDeviceManager* manager, char* path) {
    CFile file;

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }

    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }

    RecordBytes<PcxHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (!fileData.m_bytes) {
        return 0;
    }

    if (file.Read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }

    i32 result = DecodePcx(manager, fileData.m_rec, len);
    delete[] fileData.m_bytes;
    return result;
}

#pragma optimize("", off)

RVA(0x00145270, 0x17a)
i32 CDDSurface::DecodeByteRun1Plane(u8* dstBuf, u8* src, i32 width, i32 height) {
    u8* sp;
    i32 y;
    u8 tok;
    i32 hold;
    i32 k;
    u8* dstp;
    i32 len;
    i32 cols;
    if (dstBuf == NULL) {
        return 0;
    }
    if (src == NULL) {
        return 0;
    }
    hold = 0;
    sp = src;
    dstp = NULL;
    for (y = 0; y < height; y++) {
        dstp = dstBuf + width * y;
        cols = width;
        if (hold > 0) {
            for (k = 0; k < hold; k++) {
                *dstp = tok;
                dstp++;
            }
            cols -= hold;
            hold = 0;
        }
        while (cols > 0) {
            tok = *sp;
            sp++;
            if ((tok & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                len = tok & BYTE_RUN_LENGTH_MASK;
                tok = *sp;
                sp++;
                if (len > cols) {
                    hold = len - cols;
                    len = cols;
                }
                for (k = 0; k < len; k++) {
                    *dstp = tok;
                    dstp++;
                }
                cols -= len;
            } else {
                *dstp = tok;
                dstp++;
                cols--;
            }
        }
    }
    return 1;
}

RVA(0x001453f0, 0x3ac)
i32 CDDSurface::DecodeByteRun3Planes(u8* dstBuf, u8* src, i32 width, i32 height) {
    u8* sp;
    i32 y;
    u8 tok;
    i32 hold;
    i32 k;
    u8* dstp;
    i32 len;
    i32 cols;
    i32 base;
    if (dstBuf == NULL) {
        return 0;
    }
    if (src == NULL) {
        return 0;
    }
    hold = 0;
    sp = src;
    dstp = NULL;
    for (y = 0; y < height; y++) {
        base = y * width * 3;
        dstp = dstBuf + base;
        cols = width;
        if (hold > 0) {
            for (k = 0; k < hold; k++) {
                *dstp = tok;
                dstp += 3;
            }
            cols -= hold;
            hold = 0;
        }
        while (cols > 0) {
            tok = *sp;
            sp++;
            if ((tok & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                len = tok & BYTE_RUN_LENGTH_MASK;
                tok = *sp;
                sp++;
                if (len > cols) {
                    hold = len - cols;
                    len = cols;
                }
                for (k = 0; k < len; k++) {
                    *dstp = tok;
                    dstp += 3;
                }
                cols -= len;
            } else {
                *dstp = tok;
                dstp += 3;
                cols--;
            }
        }
        dstp = dstBuf + base + 1;
        cols = width;
        if (hold > 0) {
            for (k = 0; k < hold; k++) {
                *dstp = tok;
                dstp += 3;
            }
            cols -= hold;
            hold = 0;
        }
        while (cols > 0) {
            tok = *sp;
            sp++;
            if ((tok & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                len = tok & BYTE_RUN_LENGTH_MASK;
                tok = *sp;
                sp++;
                if (len > cols) {
                    hold = len - cols;
                    len = cols;
                }
                for (k = 0; k < len; k++) {
                    *dstp = tok;
                    dstp += 3;
                }
                cols -= len;
            } else {
                *dstp = tok;
                dstp += 3;
                cols--;
            }
        }
        dstp = dstBuf + base + 2;
        cols = width;
        if (hold > 0) {
            for (k = 0; k < hold; k++) {
                *dstp = tok;
                dstp += 3;
            }
            cols -= hold;
            hold = 0;
        }
        while (cols > 0) {
            tok = *sp;
            sp++;
            if ((tok & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                len = tok & BYTE_RUN_LENGTH_MASK;
                tok = *sp;
                sp++;
                if (len > cols) {
                    hold = len - cols;
                    len = cols;
                }
                for (k = 0; k < len; k++) {
                    *dstp = tok;
                    dstp += 3;
                }
                cols -= len;
            } else {
                *dstp = tok;
                dstp += 3;
                cols--;
            }
        }
    }
    return 1;
}

#pragma optimize("", on)

RVA(0x001457a0, 0x22c)
i32 CDDSurface::DecodePcxData(
    CDDrawDeviceManager* manager,
    PidHeader* image,
    i32 dataSize,
    i32 surfaceCaps,
    u32 colorKey
) {
    RecordBytes<PidHeader> record;
    record.m_rec = image;
    record.m_dwords++;

    PidFlags flags = static_cast<PidFlags>(*record.m_dwords++);
    CSize imageSize;
    imageSize.cx = *record.m_dwords++;
    imageSize.cy = *record.m_dwords++;
    record.m_dwords += 4;

    if (imageSize.cx & 3) {
        return 0;
    }
    if (HAS(flags, PID_SYSTEM_MEMORY)) {
        surfaceCaps = (surfaceCaps & ~DDSCAPS_VIDEOMEMORY) | DDSCAPS_SYSTEMMEMORY;
    } else if (HAS(flags, PID_VIDEO_MEMORY)) {
        surfaceCaps = surfaceCaps & ~DDSCAPS_SYSTEMMEMORY;
    }

    i32 remap = 0;
    PALETTEENTRY* palette;
    if (manager->m_hasPalette) {
        palette = manager->m_palette;
    } else {
        palette = NULL;
    }
    ColorDepth displayBitDepth = manager->m_displayColorDepth;
    if (displayBitDepth != BPP_PALETTED_8) {
        remap = 1;
    }

    if (HAS(flags, PID_EMBEDDED_PALETTE)) {
        if (static_cast<u32>(dataSize) <= 0x300) {
            return 0;
        }

        RecordBytes<PidHeader> headerBytes;
        headerBytes.m_rec = image;
        u8* src = headerBytes.m_bytes + dataSize - 0x300;
        COPY_RGB_PALETTE_DO(s_palPcxData, src, i, 0x100)
        palette = s_palPcxData;
    } else {
        if (remap && palette == NULL) {
            return 0;
        }
        if (remap && displayBitDepth == BPP_PALETTED_8 && manager->m_hasPalette == false) {
            return 0;
        }
    }

    if (!CDDSurface::BlitSurf(manager, imageSize.cx, imageSize.cy, BPP_UNSET, surfaceCaps)) {
        return 0;
    }

    u8* decoded = NULL;
    if (!remap) {
        if (!DecodeRun8(record.m_bytes)) {
            return 0;
        }
    } else {
        decoded = new u8[imageSize.cy * imageSize.cx];
        if (!decoded) {
            return 0;
        }
        if (!DecodeByteRun1Plane(decoded, record.m_bytes, imageSize.cx, imageSize.cy)) {
            delete[] decoded;
            return 0;
        }
    }

    if (remap) {
        if (!Blit(decoded, BPP_PALETTED_8, palette, RASTER_ROWS_TOP_DOWN)) {
            delete[] decoded;
            return 0;
        }
    }
    if (decoded) {
        delete[] decoded;
    }
    if (HAS(flags, PID_TRANSPARENCY)) {
        FillPalette(colorKey);
    }
    return 1;
}

RVA(0x001459d0, 0x135)
i32 CDDSurface::DecodePcxEx(
    CDDrawDeviceManager* manager,
    char* path,
    i32 surfaceCaps,
    u32 colorKey
) {
    CFile file;

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }

    u32 len = file.GetLength();
    RecordBytes<PidHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (!fileData.m_bytes) {
        return 0;
    }

    if (file.Read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }

    i32 result = DecodePcxData(manager, fileData.m_rec, len, surfaceCaps, colorKey);
    delete[] fileData.m_bytes;
    return result;
}

// @early-stop
RVA(0x00145b10, 0x1b5)
i32 CDDSurface::DecodePid(
    CDDrawDeviceManager* manager,
    PidHeader* image,
    u32 dataSize,
    u32 colorKey
) {
    DWORD* pDWord = static_cast<DWORD*>(static_cast<void*>(image));
    DWORD id = *pDWord++;
    PidFlags flags2 = static_cast<PidFlags>(*pDWord++);
    CSize imageSize;
    imageSize.cx = *pDWord++;
    imageSize.cy = *pDWord++;
    CPoint offset;
    offset.x = *pDWord++;
    offset.y = *pDWord++;
    DWORD user1 = *pDWord++;
    DWORD user2 = *pDWord++;
    u8* pPacked = static_cast<u8*>(static_cast<void*>(pDWord));

    if (!(imageSize.cx & 3) && m_width == imageSize.cx && m_height == imageSize.cy) {
        PALETTEENTRY* palette = NULL;
        i32 remap = 0;
        b32 hasPalette = manager->HasPalette();
        if (hasPalette != false) {
            palette = manager->GetPaletteEntries();
        }
        ColorDepth displayBitDepth = manager->GetDisplayColorDepth();
        if (displayBitDepth != BPP_PALETTED_8) {
            remap = 1;
        }

        if (HAS(flags2, PID_EMBEDDED_PALETTE)) {
            if (dataSize <= 0x300) {
                return 0;
            }

            u8* src = static_cast<u8*>(static_cast<void*>(image)) + dataSize - 0x300;
            COPY_RGB_PALETTE_DO(s_palPidData, src, i, 0x100)
            palette = s_palPidData;
        } else if ((remap && palette == NULL)
                   || (remap && displayBitDepth == BPP_PALETTED_8 && hasPalette == false)) {
            return 0;
        }

        u8* decoded = NULL;
        if (!remap) {
            if (!DecodeRun8(pPacked)) {
                return 0;
            }
        } else {
            decoded = new u8[imageSize.cy * imageSize.cx];
            if (!decoded) {
                return 0;
            }
            if (!DecodeByteRun1Plane(decoded, pPacked, imageSize.cx, imageSize.cy)) {
                delete[] decoded;
                return 0;
            }
        }

        if (remap) {
            if (!Blit(decoded, BPP_PALETTED_8, palette, RASTER_ROWS_TOP_DOWN)) {
                delete[] decoded;
                return 0;
            }
        }
        if (decoded) {
            delete[] decoded;
        }
        if (HAS(flags2, PID_TRANSPARENCY)) {
            FillPalette(colorKey);
        }
        return 1;
    }
    return 0;
}

RVA(0x00145cd0, 0x130)
i32 CDDSurface::LoadPid(CDDrawDeviceManager* manager, char* path, u32 colorKey) {
    CFile file;

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }

    u32 len = file.GetLength();
    RecordBytes<PidHeader> fileData;
    fileData.m_bytes = new u8[len];
    if (!fileData.m_bytes) {
        return 0;
    }

    if (file.Read(fileData.m_bytes, len) != len) {
        delete[] fileData.m_bytes;
        return 0;
    }

    i32 result = DecodePid(manager, fileData.m_rec, len, colorKey);
    delete[] fileData.m_bytes;
    return result;
}
