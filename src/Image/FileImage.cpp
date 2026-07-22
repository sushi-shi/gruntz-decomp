#include <Mfc.h> // CFile (the export path slurps through the real MFC CFile) - afx-first
#include <DDrawMgr/PixelShift.h> // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown

#include <Image/Image.h>            // the single-source CDDSurface (the DIRSURF surface)
#include <Image/FileImageRecords.h> // DecodeSrc / BmpFileHeader / TgaHeader / RtBitmapResHeader
#include <DDrawMgr/DDrawPtrCollections.h> // the palette-context (m_palBpp/m_palette/m_hasPalette) `info` points at

#include <ddraw.h> // real IDirectDrawSurface dispatch (this->m_8->Unlock in the exporters)

#include <string.h> // memcpy / strlen (inlined to rep movs / repne scas)
#include <Globals.h>

enum {
    PCX_HEADER_SIZE = 0x80
}; // the fixed PCX file header (pixel runs follow)

DATA(0x00283ee0)
HINSTANCE g_resModule;

DATA(0x00283ef0)
u8 g_paletteRampBuf[0x400]; // 0x683ef0
static u8 s_palBmp[0x400];  // 0x6842f0
static u8 s_palPcx[0x400];  // 0x6846f0
DATA(0x00284af0)
u8 g_grayRamp[0x401];          // 0x684af0  (indices [1..0x400] written)
static u8 s_palPidData[0x400]; // 0x684ef0 (CDDSurface::DecodePid)
static u8 s_palPcxData[0x400]; // 0x6852f0 (CDDSurface::DecodePcxData)

// ---------------------------------------------------------------------------
// DecodeRun - decode a run-length image (`src`, the DecodeSrc shape) into
// `info`. The source format is its +0x1c word (8/0x18); a convert pass runs when it
// differs from info's current format (+0x538): an 8-bit source builds an RGB-reversed
// ramp from its +0x36 palette into g_paletteRampBuf; a 24-bit-into-8-bit uses info's palette
// (+0x53c) when present. BlitSurf preps the target; the convert path runs Blit, the
// straight path BlitDirect. ret 0x10. No /GX frame.
//
// @early-stop
// branch-scheduling wall (sibling of Decode @0x144b30, same archetype): the format/
// convert dispatch, the ramp build and the BlitSurf/Blit/BlitDirect merge are logic/
// offset/CFG-faithful, but MSVC's spilled-reg + ramp-cursor scheduling diverges from the
// one allocation retail emitted. Deferred to the final sweep.
RVA(0x00143cf0, 0x16b)
i32 CDDSurface::DecodeRun(CDDrawPtrCollections* info, void* srcv, i32, i32 b) {
    DecodeSrc* src = static_cast<DecodeSrc*>(srcv);
    i32 srcFmt = src->m_1c;
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
            u8* w = g_paletteRampBuf;
            u8* p = reinterpret_cast<u8*>(src) + sizeof(BITMAPFILEHEADER)
                    + sizeof(BITMAPINFOHEADER); // the BMP palette
            do {
                w[0] = p[2];
                w[1] = p[1];
                w[2] = p[0];
                w[3] = 0;
                p += 4;
                w += 4;
            } while (w < g_paletteRampBuf + 0x400);
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

    if (CDDSurface::BlitSurf(info, src->m_12, src->m_16, 0, b)
        == 0) { // direct (qualified) slot-3 call
        return 0;
    }

    void* run = reinterpret_cast<u8*>(src) + src->m_0a;
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

// ---------------------------------------------------------------------------
// LoadFile2 - open `path` via the real MFC CFile, slurp it whole into a heap
// buffer and hand it to DecodeRun. Each failure unwinds the CFile + returns 0; the
// buffer is freed after DecodeRun (and on a short read). /GX EH frame. ret 0xc.
//
// @early-stop
// ~98%: open/length/read/decode/free shapes + offsets are byte-faithful; residue is the
// /GX funcinfo state index push (eh-state-numbering-base.md) plus the DecodeRun callee's
// own divergence (it is the branch-scheduling wall above). Deferred to the final sweep.
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
void* CDDSurface::DecodeBmp(void* surf, void* buf, u32 size) {
    CDDrawPtrCollections* pal = static_cast<CDDrawPtrCollections*>(surf);
    BITMAPINFOHEADER* ih = reinterpret_cast<BITMAPINFOHEADER*>(
        reinterpret_cast<char*>(buf) + sizeof(BITMAPFILEHEADER)
    );
    i32 width = ih->biWidth;
    i32 bitcount = ih->biBitCount;
    i32 height = ih->biHeight;
    if (m_width != width) {
        return 0;
    }
    if (m_height != height) {
        return 0;
    }
    if (bitcount != 8 && bitcount != 0x18) {
        return 0;
    }

    i32 remap = 0;
    i32 palBpp = pal->m_palBpp;
    if (palBpp != bitcount) {
        remap = 1;
    }
    if (remap && palBpp == 8 && pal->m_hasPalette == 0) {
        return 0;
    }

    void* palette = 0;
    if (remap) {
        if (bitcount == 8) {
            u8* src = reinterpret_cast<u8*>(buf) + sizeof(BITMAPFILEHEADER)
                      + sizeof(BITMAPINFOHEADER); // the BMP palette
            u8* d = s_palBmp;
            do {
                d[0] = src[2];
                d[1] = src[1];
                d[2] = src[0];
                d[3] = 0;
                src += 4;
                d += 4;
            } while (d < s_palBmp + 0x400);
            palette = s_palBmp;
        }
    } else if (palBpp == 8 && pal->m_hasPalette != 0) {
        palette = pal->m_palette;
    }

    void* pixels = reinterpret_cast<char*>(buf) + (static_cast<BITMAPFILEHEADER*>(buf))->bfOffBits;
    if (remap) {
        return Blit(pixels, bitcount, palette, 2) ? reinterpret_cast<void*>(1)
                                                  : static_cast<void*>(0);
    }
    return BlitDirect(pixels, 2) ? reinterpret_cast<void*>(1) : static_cast<void*>(0);
}

RVA(0x00144110, 0x156)
void* CDDSurface::LoadBmp(char* name, char* path) {
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

    void* result = DecodeBmp(name, buf, len);
    operator delete(buf);
    return result;
}

RVA(0x00144270, 0xd2)
i32 CDDSurface::Load(i32 a, char* name, i32 c) {
    HRSRC hr = FindResourceA(g_resModule, name, reinterpret_cast<LPCSTR>(2));
    if (!hr) {
        return 0;
    }
    HGLOBAL hg = LoadResource(g_resModule, hr);
    if (!hg) {
        return 0;
    }
    RtBitmapResHeader* p = static_cast<RtBitmapResHeader*>(LockResource(hg));
    if (!p) {
        return 0;
    }
    i32 saved = p->m_8;
    if (p->m_e != 8) {
        return 0;
    }
    memset(m_desc, 0, 0x6c);
    m_descSize = 0x6c;
    *reinterpret_cast<i32*>((m_desc + 0x68)) =
        c | 0x40;                              // +0x78 control word (the BlitSurf a5 slot)
    *reinterpret_cast<i32*>((m_desc + 4)) = 7; // dwFlags
    m_width = p->m_4;
    m_height = c;
    if (!Init1(reinterpret_cast<CDDrawPtrCollections*>(saved), 0)) {
        return 0;
    }
    BlitDirect(reinterpret_cast<char*>(p) + p->m_0 + 0x400, 2);
    return 1;
}

RVA(0x00144350, 0x5f)
i32 CDDSurface::SaveDispatch(char* a1, void* a2, void* a3) {
    switch (m_bitDepth) {
        case 0x18:
            return SaveTga(a1, a2, reinterpret_cast<i32>(a3)); // 24bpp -> 0x144900
        case 0x10:
            return SaveRle16(a1, a2, a3);
        case 8:
            return SaveBmp(a1, a2, reinterpret_cast<i32>(a3)); // 8bpp -> 0x1443b0
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// SaveBmp - write this 8-bit image to a BMP file. Validates the surface
// (IsValid), the filename (non-null, non-empty), the 8-bit depth, and the palette
// source (arg2->m_0c). Builds a BITMAPINFOHEADER + a 256-entry RGBQUAD palette (the
// source palette's bytes 0/1/2 swizzled across the quad lanes), a 14-byte file header
// (the "BM" magic strcpy'd from g_bmpHeaderTemplate + the size/offset fields), locks the
// surface, opens the file (mode 0x2001 / 0x1001 by `mode`), seeks past the magic,
// writes the headers + the rows bottom-up, then unlocks. /GX EH frame (the local
// CFile). ret 0xc.
//
// @early-stop
// large /GX export wall: the inline strlen+strcpy of g_bmpHeaderTemplate, the pointer-biased
// 256-entry RGBQUAD palette swizzle, the BITMAPINFOHEADER field stores and the
// bottom-up row-write loop are logic/offset/CFG-faithful, but the 0x44c-byte frame's
// spill scheduling + the EH-state numbering diverge from retail; not source-steerable.
// Deferred to the final sweep.
RVA(0x001443b0, 0x284)
i32 CDDSurface::SaveBmp(const char* path, void* pal, i32 mode) {
    if (this->IsValid() == 0) { // slot-5 virtual dispatch (+0x14)
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
    u8* spal = src->m_srcPalette;
    if (spal == 0) {
        return 0;
    }

    BITMAPINFOHEADER bi;
    memset(&bi, 0, sizeof(bi));
    i32 height = m_height;
    bi.biSize = 0x28;
    bi.biWidth = m_width;
    bi.biHeight = m_height;
    bi.biPlanes = 1;
    bi.biBitCount = 8;
    bi.biCompression = 0;
    bi.biSizeImage = 0;

    // 256 RGBQUADs: copy the source palette's bytes 0/1/2 into the quad's B/G/R lanes.
    u8 quads[0x400];
    {
        u8* d = quads;
        i32 n = 0x100;
        do {
            d[0] = spal[0];
            d[1] = spal[1];
            d[2] = spal[2];
            spal += 4;
            d += 4;
            --n;
        } while (n != 0);
    }

    BmpFileHeader fh;
    memset(&fh, 0, sizeof(fh));
    strcpy(fh.magic, g_bmpHeaderTemplate);
    fh.bfSize = bi.biSize * m_width + 0x436;
    fh.bfOffBits = 0x436;

    u8* buf = reinterpret_cast<u8*>(Lock(0));
    if (buf == 0) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
    }

    file.Seek(0, 2);
    file.Write(&fh, 0xe);
    file.Write(&bi, 0x428);

    i32 row = height - 1;
    while (row >= 0) {
        file.Write(buf + row * m_pitch, m_width);
        --row;
    }

    m_ddSurface->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::SaveRle16 (0x144640, ret 0xc) - the 16bpp surface -> 24bpp BMP file
// writer (DIRSURF.CPP). Bail unless the surface is valid (slot-5 IsValid), the
// name buffer `a1` is non-null and non-empty (*a1 != 0) and the surface is 16bpp
// (m_bitDepth == 0x10). Build a packed BITMAPFILEHEADER ("BM", bfSize = 3*w*h + 0x3a,
// bfOffBits = 0x3a) + a zeroed BITMAPINFOHEADER (biSize 0x28, biWidth/biHeight =
// surface w/h, biPlanes 1, biBitCount 0x18), operator-new a one-scanline 24bpp
// buffer, Lock the surface, open the CFile (mode 0x2001 / 0x1001 by a3), write
// the two headers, then walk the rows bottom-up expanding each 16bpp pixel into a
// BGR triple and writing the scanline. On any failure Unlock + free + close +
// return 0; on success return 1. The CFile stack object -> a /GX EH frame.
// @early-stop
// Two stacked walls (~52%): (1) the /GX shared-cleanup ladder - retail's per-reject
// unwind funclets converge on one Unlock/delete/~CFile tail that idiomatic C++
// scope-exit can't reproduce (docs/patterns/gx-state-machine-scalar-delete-cleanup.md);
// (2) register-allocation entropy in the 16->24bpp conversion inner loop. Logic is
// complete + correct; both are documented non-steerable plateaus.
RVA(0x00144640, 0x2be)
i32 CDDSurface::SaveRle16(void* a1, void* a2, void* a3) {
    if (this->IsValid() == 0) { // slot-5 virtual dispatch (+0x14)
        return 0;
    }
    if (a1 == 0) {
        return 0;
    }
    if (*static_cast<char*>(a1) == 0) {
        return 0;
    }
    if (this->m_bitDepth != 0x10) {
        return 0;
    }

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    bih.biSize = 0;
    bih.biWidth = 0;
    bih.biHeight = 0;
    *reinterpret_cast<i32*>(&bih.biPlanes) = 0;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    strcpy(reinterpret_cast<char*>(&bfh), "BM");
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;

    i32 height = this->m_height; // dwHeight
    i32 width = this->m_width;   // dwWidth
    bih.biHeight = height;
    bih.biWidth = width;
    bfh.bfSize = 3 * width * height + 0x3a;
    bih.biSize = 0x28;
    bih.biPlanes = 1;
    bih.biBitCount = 0x18;
    bfh.bfOffBits = 0x3a;

    u8* line = static_cast<u8*>(operator new(3 * width * height + 0x3a));
    if (line == 0) {
        return 0;
    }

    u8* locked = reinterpret_cast<u8*>(Lock(0));
    if (locked == 0) {
        operator delete(line);
        return 0;
    }

    CFile file;
    i32 ok;
    if (a3 != 0) {
        ok = file.Open(static_cast<char*>(a2), 0x2001, 0);
    } else {
        ok = file.Open(static_cast<char*>(a2), 0x1001, 0);
    }
    if (ok == 0) {
        this->m_ddSurface->Unlock(0);
        operator delete(line);
        return 0;
    }

    file.Seek(0, 2);
    file.Write(&bfh, 0xe);
    file.Write(&bih, 0x2c);

    for (i32 row = height - 1; row >= 0; row--) {
        u8* src = locked + row * this->m_pitch;
        u8* dst = line;
        for (i32 x = 0; x < width; x++) {
            u16 px = *reinterpret_cast<u16*>(src);
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

// ---------------------------------------------------------------------------
// SaveTga - write this 24-bit image to a TGA file. Mirrors SaveBmp but for
// the 0x18-bpp case: validates the surface / filename / 24-bit depth, builds an
// 0x2c-byte header (the "BM" magic + width/height + the width*height*3+0x3a size and
// the plane/bitcount words), locks, opens (mode 0x2001 / 0x1001), seeks past the
// magic, writes the header then the rows bottom-up (each row re-written width times,
// matching retail), and unlocks. /GX EH frame. ret 0xc.
//
// @early-stop
// large /GX export wall: same shape as SaveBmp - the inline strlen+strcpy, header
// field stores and the nested bottom-up row-write loop are logic/offset/CFG-faithful,
// but the frame spill scheduling + EH-state numbering diverge; deferred to the sweep.
RVA(0x00144900, 0x227)
i32 CDDSurface::SaveTga(const char* path, void* pal, i32 mode) {
    static_cast<void>(pal);
    if (this->IsValid() == 0) { // slot-5 virtual dispatch (+0x14)
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

    TgaHeader hdr;
    memset(&hdr, 0, 0x2c);
    i32 height = m_height;
    strcpy(hdr.magic, g_bmpHeaderTemplate);
    i32 width = m_width;
    hdr.size = width * height * 3 + 0x3a;
    hdr.planes = 1;
    hdr.bitCount = 0x18;

    u8* buf = reinterpret_cast<u8*>(Lock(0));
    if (buf == 0) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            m_ddSurface->Unlock(0);
            return 0;
        }
    }

    file.Seek(0, 2);
    file.Write(&hdr, 0xe);
    file.Write(&hdr, 0x2c);

    i32 row = height - 1;
    while (row >= 0) {
        i32 col = 0;
        if (m_width > 0) {
            do {
                file.Write(buf + row * m_pitch, m_width * 3);
                ++col;
            } while (col < m_width);
        }
        --row;
    }

    m_ddSurface->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// Decode - decode a run-length image (src) into this image. Validates the
// src, derives width/height from its int16 box (+0x4..+0xa) and the source format
// from +0x41 (1=8-bit, 3=24-bit). When the source format differs from the info's
// current format (+0x538) a convert pass runs: an 8-bit source builds a grayscale
// ramp from its trailing 0x300 palette, a 24-bit source into an 8-bit target uses the
// info palette (+0x53c) when present. BeginDecode (slot +0x0c) locks/preps the target;
// the straight path runs DecodeRun8/24 in place, the convert path allocates a scratch
// buffer (width even), runs RunDecode1/3 then Blit. ret 0x10. No /GX frame.
//
// @early-stop
// branch-scheduling wall (~87%): the format/convert dispatch, the width%2 parity guard,
// the grayscale-ramp copy loop and the alloc/decode/blit/free merge schedule the saved
// regs + the spilled width/height/fmt/palette in an order MSVC reproduces only for one
// allocation; logic + offsets + CFG + the run-decoder dispatch are exact (the base
// disasm is structurally byte-faithful). Deferred to the final sweep.
RVA(0x00144b30, 0x250)
i32 CDDSurface::Decode(CDDrawPtrCollections* info, CFileImageSrc* src, i32 len, i32 mode) {
    if (src == 0) {
        return 0;
    }
    i32 height = src->m_boxBottom - src->m_boxTop + 1;
    i32 width = src->m_boxRight - src->m_boxLeft + 1;

    i32 srcFmt;
    if (src->m_format == 1) {
        srcFmt = 8;
    } else if (src->m_format == 3) {
        srcFmt = 0x18;
    } else {
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

    void* palette = 0;
    if (convert) {
        if (srcFmt == 8) {
            // build the grayscale ramp from the source's trailing 0x300 palette
            u8* p = reinterpret_cast<u8*>(src) + len - 0x300;
            u8* w = g_grayRamp + 1;
            do {
                w[-1] = *p;
                ++p;
                w[2] = *p;
                ++p;
                w[1] = *p;
                ++p;
                w[0] = 0;
                w += 4;
            } while (w < g_grayRamp + 0x401);
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

    if (this->BlitSurf(info, width, height, 0, mode) == 0) { // slot-3 virtual dispatch (+0x0c)
        return 0;
    }

    void* run = reinterpret_cast<u8*>(src) + PCX_HEADER_SIZE;
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
    i32 result = Decode(info, static_cast<CFileImageSrc*>(buf), len, mode);
    operator delete(buf);
    return result;
}

RVA(0x00144ee0, 0x225)
void* CDDSurface::DecodePcx(void* surf, void* buf, u32 size) {
    if (!buf) {
        return 0;
    }
    CDDrawPtrCollections* pal = static_cast<CDDrawPtrCollections*>(surf);
    u8* hdr = static_cast<u8*>(buf);
    i32 width = *reinterpret_cast<i16*>(hdr + 8) - *reinterpret_cast<i16*>(hdr + 4) + 1;
    i32 height = *reinterpret_cast<i16*>(hdr + 0xa) - *reinterpret_cast<i16*>(hdr + 6) + 1;
    u8 planes = hdr[0x41];

    i32 bitcount = 0;
    if (planes == 1) {
        bitcount = 8;
    } else if (planes == 3) {
        bitcount = 0x18;
    }
    if (bitcount == 0) {
        return 0;
    }
    if (m_width != width) {
        return 0;
    }
    if (m_height != height) {
        return 0;
    }

    i32 remap = 0;
    i32 palBpp = pal->m_palBpp;
    if (palBpp != bitcount) {
        remap = 1;
    }
    if (remap && palBpp == 8 && pal->m_hasPalette == 0) {
        return 0;
    }

    void* palette = 0;
    if (remap) {
        if (bitcount == 8) {
            u8* src = reinterpret_cast<u8*>(buf) + size - 0x300;
            u8* d = s_palPcx;
            do {
                d[0] = *src++;
                d[1] = *src++;
                d[2] = *src++;
                d[3] = 0;
                d += 4;
            } while (d < s_palPcx + 0x400);
            palette = s_palPcx;
        } else if (palBpp == 8 && pal->m_hasPalette != 0) {
            palette = pal->m_palette;
        }
    }

    u8* pixels = reinterpret_cast<u8*>(buf) + PCX_HEADER_SIZE;
    i32 ok;
    void* decoded = 0;
    if (!remap) {
        if (bitcount == 8) {
            ok = DecodeRun8(pixels);
        } else {
            ok = DecodeRun24(pixels);
        }
        if (!ok) {
            return 0;
        }
    } else {
        if (bitcount == 8) {
            decoded = operator new(width * height);
            if (!decoded) {
                return 0;
            }
            ok = RunDecode1(decoded, pixels, width, height);
        } else {
            decoded = operator new(width * height * 3);
            if (!decoded) {
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
    return reinterpret_cast<void*>(1);
}

RVA(0x00145110, 0x156)
void* CDDSurface::LoadPcx(char* name, char* path) {
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

    void* result = DecodePcx(name, buf, len);
    operator delete(buf);
    return result;
}

#pragma optimize("", off)

// ---------------------------------------------------------------------------
// CDDSurface::RunDecode1 (ret 0x10) - the plain-buffer 8bpp variant of
// DecodeRun8: RLE-decode `src` into `dst` (no surface Lock; dimensions explicit).
// Each row starts at dst + width*row; same token grammar as DecodeRun8.
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): byte-
// identical instruction stream, only the [ebp-N] local displacements differ.
RVA(0x00145270, 0x17a)
i32 CDDSurface::RunDecode1(void* dstBuf, void* src, i32 width, i32 height) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 cols;
    if (dstBuf == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    carry = 0;
    sp = static_cast<u8*>(src);
    dst = 0;
    for (row = 0; row < height; row++) {
        dst = reinterpret_cast<u8*>(dstBuf) + width * row;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst++;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst++;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst++;
                cols--;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::RunDecode3 (ret 0x10) - the plain-buffer 24bpp variant of
// DecodeRun24: three stride-3 channel passes per row into `dst` (channels at +0,
// +1, +2), row base = dst + row*width*3 (cached once per row). No surface Lock.
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): byte-
// identical instruction stream, only the [ebp-N] local displacements differ.
RVA(0x001453f0, 0x3ac)
i32 CDDSurface::RunDecode3(void* dstBuf, void* src, i32 width, i32 height) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 cols;
    i32 base;
    if (dstBuf == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    carry = 0;
    sp = static_cast<u8*>(src);
    dst = 0;
    for (row = 0; row < height; row++) {
        base = row * width * 3;
        dst = reinterpret_cast<u8*>(dstBuf) + base;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = reinterpret_cast<u8*>(dstBuf) + base + 1;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = reinterpret_cast<u8*>(dstBuf) + base + 2;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
    }
    return 1;
}

#pragma optimize("", on)

RVA(0x001457a0, 0x22c)
i32 CDDSurface::DecodePcxData(void* surf, void* buf, i32 size, i32 a4, i32 a5) {
    u8* hdr = static_cast<u8*>(buf); // the source PID/PCX header
    CDDrawPtrCollections* dst = static_cast<CDDrawPtrCollections*>(surf);
    i32 flags = *reinterpret_cast<i32*>((hdr + 4));
    i32 w = *reinterpret_cast<i32*>((hdr + 8));
    i32 h = *reinterpret_cast<i32*>((hdr + 0xc));
    u8* data = hdr + 0x20;

    if (w & 3) {
        return 0;
    }
    if (flags & PID_SYSTEM_MEMORY) {
        a4 = (a4 & ~0x4000) | 0x800;
    } else if (flags & PID_VIDEO_MEMORY) {
        a4 = a4 & ~0x800;
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
        u8* src = hdr + size - 0x300;
        u8* d = s_palPcxData;
        do {
            d[0] = *src++;
            d[1] = *src++;
            d[2] = *src++;
            d[3] = 0;
            d += 4;
        } while (d < s_palPcxData + 0x400);
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

    if (!CDDSurface::BlitSurf(dst, w, h, 0, a4)) { // direct (qualified) slot-3 call
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
        FillPalette(a5);
    }
    return 1;
}

RVA(0x001459d0, 0x135)
i32 CDDSurface::DecodePcxEx(void* surf, char* path, void* a3, void* a4) {
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

    i32 result =
        DecodePcxData(surf, buf, len, reinterpret_cast<i32>(a3), reinterpret_cast<i32>(a4));
    operator delete(buf);
    return result;
}

RVA(0x00145b10, 0x1b5)
void* CDDSurface::DecodePid(void* surf, void* buf, u32 size, void* surf2) {
    CDDrawPtrCollections* pal = static_cast<CDDrawPtrCollections*>(surf);
    u8* hdr = static_cast<u8*>(buf);
    i32 flags = *reinterpret_cast<i32*>((hdr + 4));
    i32 width = *reinterpret_cast<i32*>((hdr + 8));
    i32 height = *reinterpret_cast<i32*>((hdr + 0xc));
    u8* data = hdr + 0x20;

    if (width & 3) {
        return 0;
    }
    if (m_width != width) {
        return 0;
    }
    if (m_height != height) {
        return 0;
    }

    void* palette = 0;
    if (pal->m_hasPalette != 0) {
        palette = pal->m_palette;
    }
    i32 remap = 0;
    i32 palBpp = pal->m_palBpp;
    if (palBpp != 8) {
        remap = 1;
    }

    if (flags & PID_EMBEDDED_PALETTE) {
        if (size <= 0x300) {
            return 0;
        }
        u8* src = reinterpret_cast<u8*>(buf) + size - 0x300;
        u8* d = s_palPidData;
        do {
            d[0] = *src++;
            d[1] = *src++;
            d[2] = *src++;
            d[3] = 0;
            d += 4;
        } while (d < s_palPidData + 0x400);
        palette = s_palPidData;
    } else if (remap) {
        if (palette == 0) {
            return 0;
        }
        if (palBpp == 8 && pal->m_hasPalette == 0) {
            return 0;
        }
    }

    void* decoded = 0;
    if (!remap) {
        if (!DecodeRun8(data)) {
            return 0;
        }
    } else {
        decoded = operator new(height * width);
        if (!decoded) {
            return 0;
        }
        if (!RunDecode1(decoded, data, width, height)) {
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
        FillPalette(reinterpret_cast<u32>(surf2));
    }
    return reinterpret_cast<void*>(1);
}

RVA(0x00145cd0, 0x130)
void* CDDSurface::LoadPid(char* name, char* path, void* a3) {
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

    void* result = DecodePid(name, buf, len, a3);
    operator delete(buf);
    return result;
}

