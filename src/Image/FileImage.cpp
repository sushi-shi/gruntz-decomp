#include <Image/FileImage.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawPtrCollections.h>
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
// Calls, CFG, extent, relocations and the palette-copy loop match retail. The
// residue is only the order of the independent info/img reloads at the loop
// join. Thirty-two TU states, 256 syntax-aware shapes and 28 reviewed boundary,
// view-placement, pointer and call spellings did not reverse that pair.
RVA(0x00143cf0, 0x16b)
i32 CDDSurface::DecodeRun(CDDrawPtrCollections* info, BmpFileImage* img, i32, i32 b) {
    ColorDepth srcFmt = static_cast<ColorDepth>(img->info.bmiHeader.biBitCount);
    i32 width = img->info.bmiHeader.biWidth;
    i32 height = img->info.bmiHeader.biHeight;
    if (srcFmt != BPP_PALETTED_8 && srcFmt != BPP_RGB_24) {
        return 0;
    }

    i32 convert = 0;
    ColorDepth curFmt = info->m_palBpp;
    if (curFmt != srcFmt) {
        convert = 1;
    }
    if (convert && curFmt == BPP_PALETTED_8 && info->m_hasPalette == 0) {
        return 0;
    }

    PALETTEENTRY* pal = NULL;
    if (convert && srcFmt == BPP_PALETTED_8) {
        RGBQUAD* p = img->info.bmiColors;
        for (i32 i = 0; i < 0x100; i++) {
            g_paletteRampBuf[i].peRed = p[i].rgbRed;
            g_paletteRampBuf[i].peGreen = p[i].rgbGreen;
            g_paletteRampBuf[i].peBlue = p[i].rgbBlue;
            g_paletteRampBuf[i].peFlags = 0;
        }
        pal = g_paletteRampBuf;
    } else if (convert && curFmt == BPP_PALETTED_8) {
        if (info->m_hasPalette != 0) {
            pal = info->m_palette;
        } else {
            pal = NULL;
        }
    }

    if (CDDSurface::BlitSurf(info, width, height, BPP_UNSET, b) == 0) {
        return 0;
    }

    RecordBytes<BmpFileImage> base;
    base.m_rec = img;
    u8* run = base.m_bytes + img->fh.bfOffBits;
    if (convert) {
        if (Blit(run, srcFmt, pal, RASTER_ROWS_BOTTOM_UP) == 0) {
            return 0;
        }
    } else {
        if (BlitDirect(run, RASTER_ROWS_BOTTOM_UP) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00143e60, 0x15b)
i32 CDDSurface::LoadFile2(CDDrawPtrCollections* info, const char* path, i32 mode) {
    CFile file;
    if (!file.Open(path, 0, 0)) {
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
    i32 result = DecodeRun(info, data.m_rec, len, mode);
    delete[] buf;
    return result;
}

static inline i32 HasPalette(CDDrawPtrCollections* pal) {
    return pal->m_hasPalette;
}

RVA(0x00143fc0, 0x142)
i32 CDDSurface::DecodeBmp(CDDrawPtrCollections* pal, BmpFileImage* bmp, u32 size) {
    BITMAPINFOHEADER* ih = &bmp->info.bmiHeader;
    i32 width = ih->biWidth;
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    i32 height = ih->biHeight;
    if (m_width == width && m_height == height
        && (bitcount == BPP_PALETTED_8 || bitcount == BPP_RGB_24)) {
        i32 remap = 0;
        ColorDepth palBpp = pal->m_palBpp;
        if (palBpp != bitcount) {
            remap = 1;
        }
        if (!remap || palBpp != BPP_PALETTED_8 || HasPalette(pal) != 0) {
            PALETTEENTRY* palette = 0;
            if (remap && bitcount == BPP_PALETTED_8) {
                RGBQUAD* src = bmp->info.bmiColors;
                for (i32 i = 0; i < 0x100; i++) {
                    s_palBmp[i].peRed = src[i].rgbRed;
                    s_palBmp[i].peGreen = src[i].rgbGreen;
                    s_palBmp[i].peBlue = src[i].rgbBlue;
                    s_palBmp[i].peFlags = 0;
                }
                palette = s_palBmp;
            } else if (remap && palBpp == BPP_PALETTED_8) {
                if (pal->m_hasPalette != 0) {
                    palette = pal->m_palette;
                } else {
                    palette = NULL;
                }
            }

            RecordBytes<BmpFileImage> data;
            data.m_rec = bmp;
            u8* pixels = data.m_bytes + bmp->fh.bfOffBits;
            if (remap) {
                if (Blit(pixels, bitcount, palette, RASTER_ROWS_BOTTOM_UP) == 0) {
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
i32 CDDSurface::LoadBmp(CDDrawPtrCollections* pal, char* path) {
    CFile file;

    if (!file.Open(path, 0, 0)) {
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
    i32 result = DecodeBmp(pal, data.m_rec, len);
    delete[] buf;
    return result;
}

RVA(0x00144270, 0xd2)
i32 CDDSurface::Load(CDDrawPtrCollections* a, char* name, i32 c) {
    HRSRC hr = FindResourceA(g_resModule, name, RT_BITMAP);
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
    memset(m_descWords, 0, sizeof(DDSURFACEDESC));
    m_descSize = sizeof(DDSURFACEDESC);
    m_surfaceCaps = c | 0x40;
    m_descFlags = 7;
    m_width = width;
    m_height = height;
    if (!CDDSurface::CreateFromDesc(a, 0)) {
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

    u8* buf = static_cast<u8*>(Lock(0));
    if (buf == NULL) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
        file.Seek(0, 2);
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
    }

    file.Write(&fh.m_hdr, sizeof(fh.m_hdr));
    file.Write(&info, sizeof(info));

    i32 row = m_height;
    while (--row >= 0) {
        file.Write(buf + row * m_pitch, m_width);
    }

    m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
// residue: which dead parameter home slot each spill lands in.  Retail parks the row
// counter in `flag`'s and the packed-byte temp in `path`'s; cl picks the other way
// round, and the register names rotate with it.
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
    i32 height = this->m_height;
    BmpFileHeaderStamp bfh;
    memset(&bfh, 0, sizeof(bfh));
    bi.bmiHeader.biCompression = 0;
    bi.bmiHeader.biSizeImage = 0;
    i32 width = this->m_width;
    strcpy(bfh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    bfh.m_hdr.bfSize = height * width * 3 + 0x3a;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = IDX(BPP_RGB_24);
    bfh.m_hdr.bfOffBits = 0x3a;

    u8* line = new u8[3 * width];
    if (line == NULL) {
        return 0;
    }

    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        delete[] line;
        return 0;
    }

    CFile file;
    if (flag != 0) {
        // The name is `path`, NOT `pal`: retail reloads the FIRST argument in both
        // arms (`mov eax,[esp+0x78]` at 0x14471d and `mov edx,[esp+0x78]` at
        // 0x1448b6, each 4 bytes above the `push 0` it just made, i.e. the slot the
        // NULL guard above already read).  The one caller that reaches this arm is
        // SaveScreenshot, which passes pal = 0, so opening `pal` could never write a
        // save-game preview and CSaveGame::Save failed after a COMPLETE snapshot.
        if (file.Open(path, 0x2001, 0) == 0) {
            this->m_ddSurface->Unlock(0);
            delete[] line;
            return 0;
        }
        // modeNoTruncate: only the append open repositions to the end.
        file.Seek(0, 2);
    } else {
        if (file.Open(path, 0x1001, 0) == 0) {
            this->m_ddSurface->Unlock(0);
            delete[] line;
            return 0;
        }
    }
    file.Write(&bfh.m_hdr, sizeof(bfh.m_hdr));
    file.Write(&bi, sizeof(bi));

    for (i32 row = this->m_height - 1; row >= 0; row--) {
        u8* src = locked + row * this->m_pitch;
        u8* dst = line;
        for (i32 x = 0; x < this->m_width; x++) {
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
        }
        file.Write(line, 3 * this->m_width);
    }

    this->m_ddSurface->Unlock(0);
    delete[] line;
    return 1;
}

// @early-stop
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
    i32 width = m_width;
    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    i32 height = m_height;
    bi.bmiHeader.biCompression = 0;
    bi.bmiHeader.biSizeImage = 0;
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    fh.m_hdr.bfSize = height * width * 3 + 0x3a;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = IDX(BPP_RGB_24);
    fh.m_hdr.bfOffBits = 0x3a;

    u8* buf = static_cast<u8*>(Lock(0));
    if (buf == NULL) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
        file.Seek(0, 2);
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            m_ddSurface->Unlock(0);
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

    m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
RVA(0x00144b30, 0x250)
i32 CDDSurface::Decode(CDDrawPtrCollections* info, PcxHeader* src, i32 len, i32 mode) {
    if (src == NULL) {
        return 0;
    }

    i32 width = src->m_xMax - src->m_xMin + 1;
    i32 height = src->m_yMax - src->m_yMin + 1;

    ColorDepth srcFmt;
    if (src->m_planes == PCX_PLANES_PALETTED) {
        srcFmt = BPP_PALETTED_8;
    } else if (src->m_planes == PCX_PLANES_RGB) {
        srcFmt = BPP_RGB_24;
    } else {
        return 0;
    }

    i32 convert = 0;
    ColorDepth curFmt = info->m_palBpp;
    if (srcFmt != curFmt) {
        convert = 1;
    }
    if (convert && curFmt == BPP_PALETTED_8 && info->m_hasPalette == 0) {
        return 0;
    }

    PALETTEENTRY* palette = 0;
    if (convert) {
        if (srcFmt == BPP_PALETTED_8) {

            RecordBytes<PcxHeader> sb;
            sb.m_rec = src;
            u8* p = sb.m_bytes + len - 0x300;
            PALETTEENTRY* d = g_grayRamp;
            do {
                d->peRed = *p++;
                d->peGreen = *p++;
                d->peBlue = *p++;
                d->peFlags = 0;
                d++;
            } while (d < &g_grayRamp[0x100]);
            palette = g_grayRamp;
        } else if (curFmt == BPP_PALETTED_8) {
            if (info->m_hasPalette != 0) {
                palette = info->m_palette;
            } else {
                palette = NULL;
            }
        } else {
            palette = NULL;
        }
    }

    if (this->BlitSurf(info, width, height, BPP_UNSET, mode) == 0) {
        return 0;
    }

    u8* run = src->m_pixels;
    u8* buf = NULL;
    i32 result;
    if (convert == 0) {
        if (srcFmt == BPP_PALETTED_8) {
            result = DecodeRun8(run);
        } else {
            result = DecodeRun24(run);
        }
        if (result == 0) {
            return 0;
        }
    } else {
        if (width % 2 != 0) {
            return 0;
        }
        if (srcFmt == BPP_PALETTED_8) {
            buf = new u8[height * width];
            if (buf == NULL) {
                return 0;
            }
            result = RunDecode1(buf, run, width, height);
        } else {
            buf = new u8[height * width * 3];
            if (buf == NULL) {
                return 0;
            }
            result = RunDecode3(buf, run, width, height);
        }
        if (result == 0) {
            delete[] buf;
            return 0;
        }
    }

    if (convert) {
        if (Blit(buf, srcFmt, palette, RASTER_ROWS_TOP_DOWN) == 0) {
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
i32 CDDSurface::LoadFile(CDDrawPtrCollections* info, const char* path, i32 mode) {
    CFile file;
    if (!file.Open(path, 0, 0)) {
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
    i32 result = Decode(info, fileData.m_rec, len, mode);
    delete[] fileData.m_bytes;
    return result;
}

RVA(0x00144ee0, 0x225)
i32 CDDSurface::DecodePcx(CDDrawPtrCollections* pal, PcxHeader* hdr, u32 size) {
    if (hdr != NULL) {
        i32 width = hdr->m_xMax - hdr->m_xMin + 1;
        i32 height = hdr->m_yMax - hdr->m_yMin + 1;
        GZ_ENUM_STORAGE(PcxPlaneCount, i8) planes = hdr->m_planes;

        ColorDepth bitcount = BPP_UNSET;
        if (planes == PCX_PLANES_PALETTED) {
            bitcount = BPP_PALETTED_8;
        } else if (planes == PCX_PLANES_RGB) {
            bitcount = BPP_RGB_24;
        }
        if (bitcount != BPP_UNSET && m_width == width && m_height == height) {
            i32 remap = 0;
            ColorDepth palBpp = pal->m_palBpp;
            if (palBpp != bitcount) {
                remap = 1;
            }
            if (!remap || palBpp != BPP_PALETTED_8 || pal->m_hasPalette != 0) {
                PALETTEENTRY* palette = NULL;
                if (remap && bitcount == BPP_PALETTED_8) {
                    u8* src = hdr->m_pixels + size - 0x380;
                    i32 i = 0;
                    do {
                        s_palPcx[i].peRed = *src++;
                        s_palPcx[i].peGreen = *src++;
                        s_palPcx[i].peBlue = *src++;
                        s_palPcx[i].peFlags = 0;
                        i++;
                    } while (i < 0x100);
                    palette = s_palPcx;
                } else if (remap && palBpp == BPP_PALETTED_8) {
                    if (pal->m_hasPalette != 0) {
                        palette = pal->m_palette;
                    } else {
                        palette = NULL;
                    }
                }

                u8* pixels = hdr->m_pixels;
                i32 ok;
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
                        decoded = new u8[width * height];
                        if (decoded == NULL) {
                            return 0;
                        }
                        ok = RunDecode1(decoded, pixels, width, height);
                    } else {
                        decoded = new u8[width * height * 3];
                        if (decoded == NULL) {
                            return 0;
                        }
                        ok = RunDecode3(decoded, pixels, width, height);
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
i32 CDDSurface::LoadPcx(CDDrawPtrCollections* pal, char* path) {
    CFile file;

    if (!file.Open(path, 0, 0)) {
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

    i32 result = DecodePcx(pal, fileData.m_rec, len);
    delete[] fileData.m_bytes;
    return result;
}

#pragma optimize("", off)

RVA(0x00145270, 0x17a)
i32 CDDSurface::RunDecode1(u8* dstBuf, u8* src, i32 width, i32 height) {
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
i32 CDDSurface::RunDecode3(u8* dstBuf, u8* src, i32 width, i32 height) {
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

// @early-stop
// Retail reads the PidHeader fields through a walking cursor (lea +4 / add 4 /
// add 0x14) rather than fixed displacements off the parameter.
RVA(0x001457a0, 0x22c)
i32 CDDSurface::DecodePcxData(
    CDDrawPtrCollections* dst,
    PidHeader* hdr,
    i32 size,
    i32 caps,
    u32 key
) {
    PidFlags flags = hdr->flags;
    i32 w = hdr->width;
    i32 h = hdr->height;
    u8* data = hdr->pixels;

    if (w & 3) {
        return 0;
    }
    if (HAS(flags, PID_SYSTEM_MEMORY)) {
        caps = (caps & ~0x4000) | 0x800;
    } else if (HAS(flags, PID_VIDEO_MEMORY)) {
        caps = caps & ~0x800;
    }

    i32 remap = 0;
    PALETTEENTRY* palette;
    if (dst->m_hasPalette) {
        palette = dst->m_palette;
    } else {
        palette = NULL;
    }
    ColorDepth palBpp = dst->m_palBpp;
    if (palBpp != BPP_PALETTED_8) {
        remap = 1;
    }

    if (HAS(flags, PID_EMBEDDED_PALETTE)) {
        if (static_cast<u32>(size) <= 0x300) {
            return 0;
        }

        RecordBytes<PidHeader> hb;
        hb.m_rec = hdr;
        u8* src = hb.m_bytes + size - 0x300;
        i32 i = 0;
        do {
            s_palPcxData[i].peRed = *src++;
            s_palPcxData[i].peGreen = *src++;
            s_palPcxData[i].peBlue = *src++;
            s_palPcxData[i].peFlags = 0;
            i++;
        } while (i < 0x100);
        palette = s_palPcxData;
    } else {
        if (remap) {
            if (palette == NULL) {
                return 0;
            }
            if (palBpp == BPP_PALETTED_8 && dst->m_hasPalette == 0) {
                return 0;
            }
        }
    }

    if (!CDDSurface::BlitSurf(dst, w, h, BPP_UNSET, caps)) {
        return 0;
    }

    u8* decoded = 0;
    if (!remap) {
        if (!DecodeRun8(data)) {
            return 0;
        }
    } else {
        decoded = new u8[h * w];
        if (!decoded) {
            return 0;
        }
        if (!RunDecode1(decoded, data, w, h)) {
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
        FillPalette(key);
    }
    return 1;
}

RVA(0x001459d0, 0x135)
i32 CDDSurface::DecodePcxEx(CDDrawPtrCollections* pal, char* path, i32 caps, u32 key) {
    CFile file;

    if (!file.Open(path, 0, 0)) {
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

    i32 result = DecodePcxData(pal, fileData.m_rec, len, caps, key);
    delete[] fileData.m_bytes;
    return result;
}

// @early-stop
// Calls, branches, returns, and referents match; only the hdr/hasPal register
// rotation remains after cursor, guard-shape, and declaration-order controls.
RVA(0x00145b10, 0x1b5)
i32 CDDSurface::DecodePid(CDDrawPtrCollections* pal, PidHeader* hdr, u32 size, u32 colorKey) {
    RecordBytes<PidHeader> p;
    p.m_rec = hdr;
    p.m_dwords++;

    PidFlags flags = static_cast<PidFlags>(*p.m_dwords++);
    i32 width = *p.m_dwords++;
    i32 height = *p.m_dwords++;
    p.m_dwords += 4;

    if (!(width & 3) && m_width == width && m_height == height) {
        PALETTEENTRY* palette = 0;
        i32 remap = 0;
        i32 hasPal = pal->m_hasPalette;
        if (hasPal != 0) {
            palette = pal->m_palette;
        }
        ColorDepth palBpp = pal->m_palBpp;
        if (palBpp != BPP_PALETTED_8) {
            remap = 1;
        }

        if (HAS(flags, PID_EMBEDDED_PALETTE)) {
            if (size <= 0x300) {
                return 0;
            }

            RecordBytes<PidHeader> hb;
            hb.m_rec = hdr;
            u8* src = hb.m_bytes + size - 0x300;
            i32 i = 0;
            do {
                s_palPidData[i].peRed = *src++;
                s_palPidData[i].peGreen = *src++;
                s_palPidData[i].peBlue = *src++;
                s_palPidData[i].peFlags = 0;
                i++;
            } while (i < 0x100);
            palette = s_palPidData;
        } else if ((remap && palette == NULL)
                   || (remap && palBpp == BPP_PALETTED_8 && hasPal == 0)) {
            return 0;
        }

        u8* decoded = 0;
        if (!remap) {
            if (!DecodeRun8(p.m_bytes)) {
                return 0;
            }
        } else {
            decoded = new u8[height * width];
            if (!decoded) {
                return 0;
            }
            if (!RunDecode1(decoded, p.m_bytes, width, height)) {
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
    return 0;
}

RVA(0x00145cd0, 0x130)
i32 CDDSurface::LoadPid(CDDrawPtrCollections* pal, char* path, u32 colorKey) {
    CFile file;

    if (!file.Open(path, 0, 0)) {
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

    i32 result = DecodePid(pal, fileData.m_rec, len, colorKey);
    delete[] fileData.m_bytes;
    return result;
}
