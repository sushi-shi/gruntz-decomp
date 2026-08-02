#include <Image/FileImage.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/PixelShift.h>
#include <Image/FileImageRecords.h>
#include <Image/Image.h>
#include <Image/ImagePool.h>
#include <Pix16.h>

#include <ddraw.h>
#include <string.h>

enum {
    PCX_HEADER_SIZE = 0x80
};

DATA(0x00283ee0)
HINSTANCE g_resModule;

DATA(0x00283ef0)
PALETTEENTRY g_paletteRampBuf[0x100];
static PALETTEENTRY s_palBmp[0x100];
DATA(0x002846f0)
static PALETTEENTRY s_palPcx[0x100];
DATA(0x00284af0)
u8 g_grayRamp[0x401];
static u8 s_palPidData[0x400];
static PALETTEENTRY s_palPcxData[0x100];

// @early-stop
RVA(0x00143cf0, 0x16b)
i32 CDDSurface::DecodeRun(CDDrawPtrCollections* info, void* srcv, i32, i32 b) {

    BmpFileImage* img = static_cast<BmpFileImage*>(srcv);
    i32 srcFmt = img->info.bmiHeader.biBitCount;
    if (srcFmt != 8 && srcFmt != 0x18) {
        return 0;
    }

    i32 convert = 0;
    i32 curFmt = info->m_palBpp;
    if (curFmt != srcFmt) {
        convert = 1;
    }
    if (convert && curFmt == 8 && info->m_hasPalette == 0) {
        return 0;
    }

    void* pal = 0;
    if (convert) {
        if (srcFmt == 8) {
            RGBQUAD* p = img->info.bmiColors;
            i32 i = 0;
            do {

                g_paletteRampBuf[i].peRed = p->rgbRed;
                g_paletteRampBuf[i].peGreen = p->rgbGreen;
                g_paletteRampBuf[i].peBlue = p->rgbBlue;
                g_paletteRampBuf[i].peFlags = 0;
                p++;
                i++;
            } while (i < 0x100);
            pal = g_paletteRampBuf;
        } else if (curFmt == 8) {
            if (info->m_hasPalette != 0) {
                pal = info->m_palette;
            } else {
                pal = 0;
            }
        } else {
            pal = 0;
        }
    }

    if (CDDSurface::BlitSurf(info, img->info.bmiHeader.biWidth, img->info.bmiHeader.biHeight, 0, b)
        == 0) {
        return 0;
    }

    RecordBytes<BmpFileImage> base;
    base.m_rec = img;
    void* run = base.m_bytes + img->fh.bfOffBits;
    if (convert) {
        if (Blit(run, srcFmt, pal, 2) == 0) {
            return 0;
        }
    } else {
        if (BlitDirect(run, 2) == 0) {
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
    void* buf = operator new(len);
    if (buf == 0) {
        return 0;
    }
    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }
    i32 result = DecodeRun(info, buf, len, mode);
    operator delete(buf);
    return result;
}

RVA(0x00143fc0, 0x142)
i32 CDDSurface::DecodeBmp(CDDrawPtrCollections* pal, void* buf, u32 size) {

    BmpFileImage* bmp = static_cast<BmpFileImage*>(buf);
    BITMAPINFOHEADER* ih = &bmp->info.bmiHeader;
    i32 width = ih->biWidth;
    i32 bitcount = ih->biBitCount;
    i32 height = ih->biHeight;
    if (width == m_width && m_height == height && (bitcount == 8 || bitcount == 0x18)) {
        i32 remap = 0;
        i32 palBpp = pal->m_palBpp;
        if (palBpp != bitcount) {
            remap = 1;
        }
        if (!remap || palBpp != 8 || pal->m_hasPalette != 0) {
            void* palette = 0;
            if (remap && bitcount == 8) {
                u8* src =
                    static_cast<u8*>(buf) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
                i32 i = 0;
                do {
                    s_palBmp[i].peRed = src[2];
                    s_palBmp[i].peGreen = src[1];
                    s_palBmp[i].peBlue = src[0];
                    s_palBmp[i].peFlags = 0;
                    src += 4;
                    i++;
                } while (i < 0x100);
                palette = s_palBmp;
            } else if (remap && palBpp == 8) {
                if (pal->m_hasPalette != 0) {
                    palette = pal->m_palette;
                } else {
                    palette = 0;
                }
            }

            void* pixels =
                static_cast<char*>(buf) + (static_cast<BITMAPFILEHEADER*>(buf))->bfOffBits;
            if (remap) {
                if (!Blit(pixels, bitcount, palette, 2)) {
                    return 0;
                }
                return 1;
            }
            if (BlitDirect(pixels, 2)) {
                return 1;
            }
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

    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    i32 result = DecodeBmp(pal, buf, len);
    operator delete(buf);
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
    i32 height = bih->biHeight;
    if (bih->biBitCount != 8) {
        return 0;
    }
    memset(m_descWords, 0, sizeof(DDSURFACEDESC));
    m_descSize = sizeof(DDSURFACEDESC);
    m_surfaceCaps = c | 0x40;
    m_width = bih->biWidth;
    m_descFlags = 7;
    m_height = height;
    if (!CDDSurface::CreateFromDesc(a, 0)) {
        return 0;
    }

    RecordBytes<BITMAPINFOHEADER> ib;
    ib.m_rec = bih;
    BlitDirect(ib.m_bytes + bih->biSize + 256 * sizeof(RGBQUAD), 2);
    return 1;
}

RVA(0x00144350, 0x5f)
i32 CDDSurface::SaveDispatch(char* path, void* pal, i32 flag) {
    switch (m_bitDepth) {
        case 0x18:
            return SaveTga(path, pal, flag);
        case 0x10:
            return SaveRle16(path, pal, flag);
        case 8:
            return SaveBmp(path, pal, flag);
        default:
            return 0;
    }
}

// @early-stop
RVA(0x001443b0, 0x284)
i32 CDDSurface::SaveBmp(const char* path, void* pal, i32 mode) {
    if (this->IsValid() == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (m_bitDepth != 8) {
        return 0;
    }
    CFileImagePal* src = static_cast<CFileImagePal*>(pal);
    if (src == 0) {
        return 0;
    }
    if (src->m_srcPalette == 0) {
        return 0;
    }

    Bmp256Info info;
    memset(&info.bmiHeader, 0, sizeof(info.bmiHeader));
    i32 height = m_height;
    info.bmiHeader.biSize = 0x28;
    info.bmiHeader.biWidth = m_width;
    info.bmiHeader.biHeight = height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 8;
    info.bmiHeader.biCompression = 0;
    info.bmiHeader.biSizeImage = 0;

    PALETTEENTRY* spal = src->m_srcPalette;
    if (spal == 0) {
        return 0;
    }

    {
        i32 i = 0;
        i32 n = 0x100;
        do {
            info.bmiColors[i].rgbRed = spal->peRed;
            info.bmiColors[i].rgbGreen = spal->peGreen;
            info.bmiColors[i].rgbBlue = spal->peBlue;
            spal++;
            i++;
            --n;
        } while (n != 0);
    }

    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    fh.m_hdr.bfSize = info.bmiHeader.biSize * m_width + 0x436;
    fh.m_hdr.bfOffBits = 0x436;

    u8* buf = static_cast<u8*>(Lock(0));
    if (buf == 0) {
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

    file.Write(&fh.m_hdr, 0xe);
    file.Write(&info, 0x428);

    i32 row = m_height;
    while (--row >= 0) {
        file.Write(buf + row * m_pitch, m_width);
    }

    m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
RVA(0x00144640, 0x2be)
i32 CDDSurface::SaveRle16(void* path, void* pal, i32 flag) {
    if (this->IsValid() == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    if (*static_cast<char*>(path) == 0) {
        return 0;
    }
    if (this->m_bitDepth != 0x10) {
        return 0;
    }

    BmpFileHeaderStamp bfh;

    BmpInfoHeaderStamp bih;
    bih.m_biSize = 0;
    bih.m_biWidth = 0;
    bih.m_biHeight = 0;
    bih.m_planesAndBitCount = 0;
    bih.m_ih.biSizeImage = 0;
    bih.m_ih.biXPelsPerMeter = 0;
    bih.m_ih.biYPelsPerMeter = 0;
    bih.m_ih.biClrUsed = 0;
    bih.m_ih.biClrImportant = 0;

    strcpy(bfh.m_bytes, "BM");
    bfh.m_hdr.bfReserved1 = 0;
    bfh.m_hdr.bfReserved2 = 0;

    i32 height = this->m_height;
    i32 width = this->m_width;
    bih.m_ih.biHeight = height;
    bih.m_ih.biWidth = width;
    bfh.m_hdr.bfSize = 3 * width * height + 0x3a;
    bih.m_ih.biSize = 0x28;
    bih.m_ih.biPlanes = 1;
    bih.m_ih.biBitCount = 0x18;
    bfh.m_hdr.bfOffBits = 0x3a;

    u8* line = static_cast<u8*>(operator new(3 * width * height + 0x3a));
    if (line == 0) {
        return 0;
    }

    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == 0) {
        operator delete(line);
        return 0;
    }

    CFile file;
    i32 ok;
    if (flag != 0) {
        ok = file.Open(static_cast<char*>(pal), 0x2001, 0);
    } else {
        ok = file.Open(static_cast<char*>(pal), 0x1001, 0);
    }
    if (ok == 0) {
        this->m_ddSurface->Unlock(0);
        operator delete(line);
        return 0;
    }

    file.Seek(0, 2);
    file.Write(&bfh.m_hdr, 0xe);
    file.Write(&bih.m_ih, 0x2c);

    for (i32 row = height - 1; row >= 0; row--) {
        u8* src = locked + row * this->m_pitch;
        u8* dst = line;
        for (i32 x = 0; x < width; x++) {
            Pix16Ptr sp;
            sp.m_bytes = src;
            u16 px = *sp.m_words;
            src += 2;
            dst[0] = static_cast<u8>((static_cast<u8>(px) << g_bDown));
            dst[1] = static_cast<u8>((static_cast<u8>((px >> g_gUp)) << g_gDown));
            dst[2] = static_cast<u8>((static_cast<u8>((px >> g_rUp)) << g_rDown));
            dst += 3;
        }
        file.Write(line, 3 * width);
    }

    this->m_ddSurface->Unlock(0);
    operator delete(line);
    return 1;
}

// @early-stop
RVA(0x00144900, 0x227)
i32 CDDSurface::SaveTga(const char* path, void* pal, i32 mode) {
    static_cast<void>(pal);
    if (this->IsValid() == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (m_bitDepth != 0x18) {
        return 0;
    }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    i32 height = m_height;
    BmpFileHeaderStamp fh;
    memset(&fh, 0, sizeof(fh));
    i32 width = m_width;
    strcpy(fh.m_bytes, g_bmpHeaderTemplate);
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biSize = 0x28;
    bi.bmiHeader.biWidth = width;
    fh.m_hdr.bfSize = height * width * 3 + 0x3a;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 0x18;
    bi.bmiHeader.biCompression = 0;
    bi.bmiHeader.biSizeImage = 0;
    fh.m_hdr.bfOffBits = 0x3a;

    u8* buf = static_cast<u8*>(Lock(0));
    if (buf == 0) {
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

    file.Write(&fh.m_hdr, 0xe);
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
    if (src == 0) {
        return 0;
    }

    i32 width = src->m_xMax - src->m_xMin + 1;
    i32 height = src->m_yMax - src->m_yMin + 1;

    i32 srcFmt;
    if (src->m_planes == 1) {
        srcFmt = 8;
    } else if (src->m_planes == 3) {
        srcFmt = 0x18;
    } else {
        return 0;
    }

    i32 convert = 0;
    i32 curFmt = info->m_palBpp;
    if (srcFmt != curFmt) {
        convert = 1;
    }
    if (convert && curFmt == 8 && info->m_hasPalette == 0) {
        return 0;
    }

    void* palette = 0;
    if (convert) {
        if (srcFmt == 8) {

            RecordBytes<PcxHeader> sb;
            sb.m_rec = src;
            u8* p = sb.m_bytes + len - 0x300;
            i32 i = 0;
            do {
                g_grayRamp[i] = *p++;
                g_grayRamp[i + 1] = *p++;
                g_grayRamp[i + 2] = *p++;
                g_grayRamp[i + 3] = 0;
                i += 4;
            } while (i < 0x400);
            palette = g_grayRamp;
        } else if (curFmt == 8) {
            if (info->m_hasPalette != 0) {
                palette = info->m_palette;
            } else {
                palette = 0;
            }
        } else {
            palette = 0;
        }
    }

    if (this->BlitSurf(info, width, height, 0, mode) == 0) {
        return 0;
    }

    void* run = src->m_pixels;
    void* buf = 0;
    i32 result;
    if (convert == 0) {
        if (srcFmt == 8) {
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
        if (srcFmt == 8) {
            buf = operator new(height * width);
            if (buf == 0) {
                return 0;
            }
            result = RunDecode1(buf, run, width, height);
        } else {
            buf = operator new(height * width * 3);
            if (buf == 0) {
                return 0;
            }
            result = RunDecode3(buf, run, width, height);
        }
        if (result == 0) {
            operator delete(buf);
            return 0;
        }
    }

    if (convert) {
        if (Blit(buf, srcFmt, palette, 1) == 0) {
            operator delete(buf);
            return 0;
        }
    }
    if (buf != 0) {
        operator delete(buf);
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
    void* buf = operator new(len);
    if (buf == 0) {
        return 0;
    }
    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }
    i32 result = Decode(info, static_cast<PcxHeader*>(buf), len, mode);
    operator delete(buf);
    return result;
}

RVA(0x00144ee0, 0x225)
i32 CDDSurface::DecodePcx(CDDrawPtrCollections* pal, PcxHeader* hdr, u32 size) {
    if (hdr != 0) {
        i32 width = hdr->m_xMax - hdr->m_xMin + 1;
        i32 height = hdr->m_yMax - hdr->m_yMin + 1;
        u8 planes = hdr->m_planes;

        i32 bitcount = 0;
        if (planes == 1) {
            bitcount = 8;
        } else if (planes == 3) {
            bitcount = 0x18;
        }
        if (bitcount != 0 && m_width == width && m_height == height) {
            i32 remap = 0;
            i32 palBpp = pal->m_palBpp;
            if (palBpp != bitcount) {
                remap = 1;
            }
            if (!remap || palBpp != 8 || pal->m_hasPalette != 0) {
                void* palette = 0;
                if (remap && bitcount == 8) {
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
                } else if (remap && palBpp == 8) {
                    if (pal->m_hasPalette != 0) {
                        palette = pal->m_palette;
                    } else {
                        palette = 0;
                    }
                }

                u8* pixels = hdr->m_pixels;
                i32 ok;
                void* decoded = 0;
                if (!remap) {
                    if (bitcount == 8) {
                        if (!DecodeRun8(pixels)) {
                            return 0;
                        }
                    } else {
                        if (!DecodeRun24(pixels)) {
                            return 0;
                        }
                    }
                } else {
                    if (bitcount == 8) {
                        decoded = operator new(width * height);
                        if (decoded == 0) {
                            return 0;
                        }
                        ok = RunDecode1(decoded, pixels, width, height);
                    } else {
                        decoded = operator new(width * height * 3);
                        if (decoded == 0) {
                            return 0;
                        }
                        ok = RunDecode3(decoded, pixels, width, height);
                    }
                    if (!ok) {
                        operator delete(decoded);
                        return 0;
                    }
                }

                if (remap) {
                    if (!Blit(decoded, bitcount, palette, 1)) {
                        operator delete(decoded);
                        return 0;
                    }
                }
                if (decoded) {
                    operator delete(decoded);
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

    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    i32 result = DecodePcx(pal, static_cast<PcxHeader*>(buf), len);
    operator delete(buf);
    return result;
}

#pragma optimize("", off)

RVA(0x00145270, 0x17a)
i32 CDDSurface::RunDecode1(void* dstBuf, void* src, i32 width, i32 height) {
    u8* sp;
    i32 y;
    u8 tok;
    i32 hold;
    i32 k;
    u8* dstp;
    i32 len;
    i32 cols;
    if (dstBuf == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    hold = 0;
    sp = static_cast<u8*>(src);
    dstp = 0;
    for (y = 0; y < height; y++) {
        dstp = static_cast<u8*>(dstBuf) + width * y;
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
            if ((tok & 0xc0) == 0xc0) {
                len = tok & 0x3f;
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
i32 CDDSurface::RunDecode3(void* dstBuf, void* src, i32 width, i32 height) {
    u8* sp;
    i32 y;
    u8 tok;
    i32 hold;
    i32 k;
    u8* dstp;
    i32 len;
    i32 cols;
    i32 base;
    if (dstBuf == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    hold = 0;
    sp = static_cast<u8*>(src);
    dstp = 0;
    for (y = 0; y < height; y++) {
        base = y * width * 3;
        dstp = static_cast<u8*>(dstBuf) + base;
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
            if ((tok & 0xc0) == 0xc0) {
                len = tok & 0x3f;
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
        dstp = static_cast<u8*>(dstBuf) + base + 1;
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
            if ((tok & 0xc0) == 0xc0) {
                len = tok & 0x3f;
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
        dstp = static_cast<u8*>(dstBuf) + base + 2;
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
            if ((tok & 0xc0) == 0xc0) {
                len = tok & 0x3f;
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
    CDDrawPtrCollections* dst,
    PidHeader* hdr,
    i32 size,
    i32 caps,
    u32 key
) {
    i32 flags = static_cast<i32>(hdr->flags);
    i32 w = hdr->width;
    i32 h = hdr->height;
    u8* data = hdr->pixels;

    if (w & 3) {
        return 0;
    }
    if (flags & PID_SYSTEM_MEMORY) {
        caps = (caps & ~0x4000) | 0x800;
    } else if (flags & PID_VIDEO_MEMORY) {
        caps = caps & ~0x800;
    }

    void* palette = 0;
    if (dst->m_hasPalette) {
        palette = dst->m_palette;
    }
    i32 remap = 0;
    i32 palBpp = dst->m_palBpp;
    if (palBpp != 8) {
        remap = 1;
    }

    if (flags & PID_EMBEDDED_PALETTE) {
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
            if (palette == 0) {
                return 0;
            }
            if (palBpp == 8 && dst->m_hasPalette == 0) {
                return 0;
            }
        }
    }

    if (!CDDSurface::BlitSurf(dst, w, h, 0, caps)) {
        return 0;
    }

    void* decoded = 0;
    if (!remap) {
        if (!DecodeRun8(data)) {
            return 0;
        }
    } else {
        decoded = operator new(h * w);
        if (!decoded) {
            return 0;
        }
        if (!RunDecode1(decoded, data, w, h)) {
            operator delete(decoded);
            return 0;
        }
    }

    if (remap) {
        if (!Blit(decoded, 8, palette, 1)) {
            operator delete(decoded);
            return 0;
        }
    }
    if (decoded) {
        operator delete(decoded);
    }
    if (flags & PID_TRANSPARENCY) {
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
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    i32 result = DecodePcxData(pal, static_cast<PidHeader*>(buf), len, caps, key);
    operator delete(buf);
    return result;
}

RVA(0x00145b10, 0x1b5)
i32 CDDSurface::DecodePid(CDDrawPtrCollections* pal, PidHeader* hdr, u32 size, u32 colorKey) {
    i32 flags = static_cast<i32>(hdr->flags);
    i32 width = hdr->width;
    i32 height = hdr->height;
    u8* p = hdr->pixels;

    if (!(width & 3) && m_width == width && m_height == height) {
        void* palette = 0;
        i32 remap = 0;
        i32 hasPal = pal->m_hasPalette;
        if (hasPal != 0) {
            palette = pal->m_palette;
        }
        i32 palBpp = pal->m_palBpp;
        if (palBpp != 8) {
            remap = 1;
        }

        if (flags & PID_EMBEDDED_PALETTE) {
            if (size <= 0x300) {
                return 0;
            }

            RecordBytes<PidHeader> hb;
            hb.m_rec = hdr;
            u8* src = hb.m_bytes + size - 0x300;
            i32 i = 0;
            do {
                s_palPidData[i] = *src++;
                s_palPidData[i + 1] = *src++;
                s_palPidData[i + 2] = *src++;
                s_palPidData[i + 3] = 0;
                i += 4;
            } while (i < 0x400);
            palette = s_palPidData;
        } else if (remap) {
            if (palette == 0) {
                return 0;
            }
            if (remap && palBpp == 8 && hasPal == 0) {
                return 0;
            }
        }

        void* decoded = 0;
        if (!remap) {
            if (!DecodeRun8(p)) {
                return 0;
            }
        } else {
            decoded = operator new(height * width);
            if (!decoded) {
                return 0;
            }
            if (!RunDecode1(decoded, p, width, height)) {
                operator delete(decoded);
                return 0;
            }
        }

        if (remap) {
            if (!Blit(decoded, 8, palette, 1)) {
                operator delete(decoded);
                return 0;
            }
        }
        if (decoded) {
            operator delete(decoded);
        }
        if (flags & PID_TRANSPARENCY) {
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
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    i32 result = DecodePid(pal, static_cast<PidHeader*>(buf), len, colorKey);
    operator delete(buf);
    return result;
}
