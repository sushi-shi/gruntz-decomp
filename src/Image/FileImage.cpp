// FileImage.cpp - the CDDSurface file-format CODEC file, an ORIGINAL DDrawMgr TU
// (wave4-K; interval dossier #14I): one obj spanning retail 0x143cf0-0x145dff -
// the surface Load/Save/Decode paths for BMP/PCX/PID/TGA/RLE16 + the plain-buffer
// RLE run-decoders. It CANNOT be DIRSURF.CPP (the whole DDRAWMGR obj sits between
// the two blocks; obj contributions are contiguous at first link); it carries no
// __FILE__ assert so the original name is unknown (a DIRFILE.CPP-like sibling).
// The former image-unit codec fns and the fileimagerundecode RunDecode1/3 pair
// (an in-source `#pragma optimize("",off)` island - see below) are folded in, in
// retail-RVA order, plus CDDSurface::Load (0x144270, the RT_BITMAP loader - the
// former ResLoaders::ResLoad_144270 view, RVA-proven to be this surface class).
// ---------------------------------------------------------------------------
#include <Mfc.h> // CFile (the export path slurps through the real MFC CFile) - afx-first

#include <Image/Image.h>            // the single-source CDDSurface (the DIRSURF surface)
#include <Image/FileImageRecords.h> // DecodeSrc / BmpFileHeader / TgaHeader / ResHdr_144270
#include <DDrawMgr/DDrawPtrCollections.h> // the palette-context (m_palBpp/m_palette/m_hasPalette) `info` points at

#include <ddraw.h> // real IDirectDrawSurface dispatch (this->m_8->Unlock in the exporters)

#include <string.h> // memcpy / strlen (inlined to rep movs / repne scas)
#include <Globals.h>

// The file loaders open through the real MFC CFile (RTTI 0x1ed15c); the heap
// buffers are ::operator new @0x1b9b46 / ::operator delete @0x1b9b82 (NAFXCW),
// both declared by <Mfc.h> - all reloc-masked library calls (was CFileIO/RezFree).

// The app HINSTANCE used as the RT_BITMAP resource module (DAT_00683ee0).
DATA(0x00283ee0)
extern HINSTANCE g_resModule;

// Per-decoder 256-entry RGBQUAD palette buffers (file-scope BSS, reloc-masked).
// Each decoder builds its own when the source carries an inline/trailing palette;
// the buffer addresses are differently-named symbols in the base obj (the absolute
// pushes/compares reloc-mask against the retail DAT_* names). Declared in their
// retail BSS order so the layout follows the binary.
static u8 s_palBmp[0x400];     // 0x6842f0
static u8 s_palPcx[0x400];     // 0x6846f0
static u8 s_palPidData[0x400]; // 0x684ef0 (CDDSurface::DecodePid)
static u8 s_palPcxData[0x400]; // 0x6852f0 (CDDSurface::DecodePcxData)

// ---------------------------------------------------------------------------
// DecodeRun - decode a run-length image (`src`, the DecodeSrc shape) into
// `info`. The source format is its +0x1c word (8/0x18); a convert pass runs when it
// differs from info's current format (+0x538): an 8-bit source builds an RGB-reversed
// ramp from its +0x36 palette into g_683ef0; a 24-bit-into-8-bit uses info's palette
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
    DecodeSrc* src = (DecodeSrc*)srcv;
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
            u8* w = g_683ef0;
            u8* p = (u8*)src + 0x36;
            do {
                w[0] = p[2];
                w[1] = p[1];
                w[2] = p[0];
                w[3] = 0;
                p += 4;
                w += 4;
            } while (w < g_683ef0 + 0x400);
            pal = g_683ef0;
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

    void* run = (u8*)src + src->m_0a;
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

// ---------------------------------------------------------------------------
// CDDSurface::DecodeBmp
// `buf` is a whole .BMP file (packed BITMAPFILEHEADER + BITMAPINFOHEADER). Pull
// biWidth/biHeight/biBitCount, validate them against the destination surface's
// geometry (m_width/m_height) and require 8 or 24 bpp. `surf` is the palette context
// (the CDDrawPtrCollections display manager: m_palBpp source bpp / m_palette palette /
// m_hasPalette have-palette). When the surface bpp
// differs from the file's, build a palette (for 8bpp: byte-reverse the BMP's
// in-file RGBQUADs into s_palBmp; for 24bpp reuse the surface palette) and blit
// through the remapping Blit; otherwise straight-copy via BlitDirect. The pixel
// data starts at buf + bfOffBits. Returns 1 on a successful blit, else 0.
RVA(0x00143fc0, 0x142)
void* CDDSurface::DecodeBmp(void* surf, void* buf, u32 size) {
    CDDrawPtrCollections* pal = (CDDrawPtrCollections*)surf;
    BITMAPINFOHEADER* ih = (BITMAPINFOHEADER*)((char*)buf + 0xe);
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
            u8* src = (u8*)buf + 0x36;
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

    void* pixels = (char*)buf + ((BITMAPFILEHEADER*)buf)->bfOffBits;
    if (remap) {
        return Blit(pixels, bitcount, palette, 2) ? (void*)1 : (void*)0;
    }
    return BlitDirect(pixels, 2) ? (void*)1 : (void*)0;
}

// ---------------------------------------------------------------------------
// CDDSurface::LoadBmp
// Open the file named by `path`; on failure return 0. GetLength(); if the length
// is zero return 0. `operator new` a buffer of that size; if it fails return 0.
// Read the file; if the read count != length, free + return 0. Else decode and
// return the decoder's result. The CFile dtor + buffer free run on every exit.
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

// ---------------------------------------------------------------------------
// CDDSurface::Load (0x144270) - the RT_BITMAP resource loader LoadByExt's default
// branch dispatches (the former ResLoaders::ResLoad_144270 view, folded in wave4-K:
// the view's Init@0x13e0a0 / Parse@0x13ece0 ARE CDDSurface::Init1/BlitDirect, its
// +0x10..+0x78 fields ARE this surface's DDSURFACEDESC scratch). Find/load/lock
// the named RT_BITMAP, validate its header (+0xe == 8), zero + seed the desc
// (dwSize/dwFlags/width/height + the +0x78 control word), run the slot-2 init
// with the resource's +0x8 word, then straight-copy the payload that follows the
// 0x400-byte header. __thiscall(a, name, c); `a` unused.
RVA(0x00144270, 0xd2)
i32 CDDSurface::Load(i32 a, char* name, i32 c) {
    HRSRC hr = FindResourceA(g_resModule, name, (LPCSTR)2);
    if (!hr) {
        return 0;
    }
    HGLOBAL hg = LoadResource(g_resModule, hr);
    if (!hg) {
        return 0;
    }
    ResHdr_144270* p = (ResHdr_144270*)LockResource(hg);
    if (!p) {
        return 0;
    }
    i32 saved = p->m_8;
    if (p->m_e != 8) {
        return 0;
    }
    memset(m_desc, 0, 0x6c);
    m_descSize = 0x6c;
    *(i32*)(m_desc + 0x68) = c | 0x40; // +0x78 control word (the BlitSurf a5 slot)
    *(i32*)(m_desc + 4) = 7;           // dwFlags
    m_width = p->m_4;
    m_height = c;
    if (!Init1((CDDrawPtrCollections*)saved, 0)) {
        return 0;
    }
    BlitDirect((char*)p + p->m_0 + 0x400, 2);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::SaveDispatch (ret 0xc) - pick the per-bit-depth file writer by the
// surface's raw bit depth m_bitDepth (8 -> Save8, 16 -> SaveRle16, 24 -> Save24),
// forwarding all three pass-through args; any other depth returns 0. Case bodies in
// retail .text order (24, 16, 8); the near case labels lower to MSVC's compare
// ladder.
RVA(0x00144350, 0x5f)
i32 CDDSurface::SaveDispatch(char* a1, void* a2, void* a3) {
    switch (m_bitDepth) {
        case 0x18:
            return SaveTga(a1, a2, (i32)a3); // 24bpp -> 0x144900
        case 0x10:
            return SaveRle16(a1, a2, a3);
        case 8:
            return SaveBmp(a1, a2, (i32)a3); // 8bpp -> 0x1443b0
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// SaveBmp - write this 8-bit image to a BMP file. Validates the surface
// (IsValid), the filename (non-null, non-empty), the 8-bit depth, and the palette
// source (arg2->m_0c). Builds a BITMAPINFOHEADER + a 256-entry RGBQUAD palette (the
// source palette's bytes 0/1/2 swizzled across the quad lanes), a 14-byte file header
// (the "BM" magic strcpy'd from g_imageTag + the size/offset fields), locks the
// surface, opens the file (mode 0x2001 / 0x1001 by `mode`), seeks past the magic,
// writes the headers + the rows bottom-up, then unlocks. /GX EH frame (the local
// CFile). ret 0xc.
//
// @early-stop
// large /GX export wall: the inline strlen+strcpy of g_imageTag, the pointer-biased
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
    CFileImagePal* src = (CFileImagePal*)pal;
    if (src == 0) {
        return 0;
    }
    u8* spal = src->m_0c;
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
    strcpy(fh.magic, g_imageTag);
    fh.bfSize = bi.biSize * m_width + 0x436;
    fh.bfOffBits = 0x436;

    u8* buf = (u8*)Lock(0);
    if (buf == 0) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, 0)) {
            ((CFileImageHeldSurface*)m_8)->Unlock(0);
            return 0;
        }
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            ((CFileImageHeldSurface*)m_8)->Unlock(0);
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

    ((CFileImageHeldSurface*)m_8)->Unlock(0);
    return 1;
}

// The live screen RGB-format unpack-shift table (file RVA 0x283ea0..0x283eb4 =
// VA 0x683ea0..). SaveRle16 uses them to expand a screen-native 16bpp pixel into
// an 8-bit-per-channel BGR triple (the inverse of the CLightFxRender Pack). Same
// differently-named symbols as elsewhere; reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // red   up-shift   (channel position in the 16bpp word)
DATA(0x00283ea4)
extern i32 g_gUp; // green up-shift
DATA(0x00283eac)
extern i32 g_rDown; // red   down-shift (scale 5/6-bit -> 8-bit)
DATA(0x00283eb0)
extern i32 g_gDown; // green down-shift
DATA(0x00283eb4)
extern i32 g_bDown; // blue  down-shift

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
    if (*(char*)a1 == 0) {
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
    *(i32*)&bih.biPlanes = 0;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    strcpy((char*)&bfh, "BM");
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

    u8* line = (u8*)operator new(3 * width * height + 0x3a);
    if (line == 0) {
        return 0;
    }

    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        operator delete(line);
        return 0;
    }

    CFile file;
    i32 ok;
    if (a3 != 0) {
        ok = file.Open((char*)a2, 0x2001, 0);
    } else {
        ok = file.Open((char*)a2, 0x1001, 0);
    }
    if (ok == 0) {
        this->m_8->Unlock(0);
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
            u16 px = *(u16*)src;
            src += 2;
            dst[0] = (u8)((u8)px << g_bDown);
            dst[1] = (u8)((u8)(px >> g_gUp) << g_gDown);
            dst[2] = (u8)((u8)(px >> g_rUp) << g_rDown);
            dst += 3;
        }
        file.Write(line, 3 * width);
    }

    this->m_8->Unlock(0);
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
    (void)pal;
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
    strcpy(hdr.magic, g_imageTag);
    i32 width = m_width;
    hdr.size = width * height * 3 + 0x3a;
    hdr.planes = 1;
    hdr.bitCount = 0x18;

    u8* buf = (u8*)Lock(0);
    if (buf == 0) {
        return 0;
    }

    CFile file;
    if (mode != 0) {
        if (!file.Open(path, 0x2001, 0)) {
            ((CFileImageHeldSurface*)m_8)->Unlock(0);
            return 0;
        }
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            ((CFileImageHeldSurface*)m_8)->Unlock(0);
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

    ((CFileImageHeldSurface*)m_8)->Unlock(0);
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
    i32 height = src->m_08 - src->m_04 + 1;
    i32 width = src->m_0a - src->m_06 + 1;

    i32 srcFmt;
    if (src->m_41 == 1) {
        srcFmt = 8;
    } else if (src->m_41 == 3) {
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
            u8* p = (u8*)src + len - 0x300;
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

    void* run = (u8*)src + 0x80;
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

// ---------------------------------------------------------------------------
// LoadFile - open `path`, slurp it whole into a heap buffer, and hand the
// buffer (plus the caller's `src` descriptor + `mode`) to Decode. The local CFile
// destructs on every exit -> /GX EH frame. Each failure (open / empty / alloc /
// short read) returns 0; otherwise returns Decode's result. The read buffer is
// freed after Decode (and on a short read). ret 0xc.
// ---------------------------------------------------------------------------
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
    i32 result = Decode(info, (CFileImageSrc*)buf, len, mode);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CDDSurface::DecodePcx
// `buf` is a whole .PCX file (128-byte ZSoft header, pixels at +0x80). Geometry
// comes from the window (width = Xmax-Xmin+1 @ +8/+4, height = Ymax-Ymin+1 @
// +0xa/+6); NPlanes @ +0x41 picks 8bpp (1 plane) or 24bpp (3 planes). Validate vs
// the destination (m_width/m_height). When no remap is needed the planes are RLE-expanded
// straight into the surface (DecodeRun8/DecodeRun24); when the surface bpp differs
// the run is decoded into a scratch buffer (RunDecode1/RunDecode3) and then blit
// through the palette (built from the PCX trailing 768-byte VGA palette for 8bpp,
// or the surface palette for 24bpp). Returns 1 on success, 0 on failure.
RVA(0x00144ee0, 0x225)
void* CDDSurface::DecodePcx(void* surf, void* buf, u32 size) {
    if (!buf) {
        return 0;
    }
    CDDrawPtrCollections* pal = (CDDrawPtrCollections*)surf;
    u8* hdr = (u8*)buf;
    i32 width = *(i16*)(hdr + 8) - *(i16*)(hdr + 4) + 1;
    i32 height = *(i16*)(hdr + 0xa) - *(i16*)(hdr + 6) + 1;
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
            u8* src = (u8*)buf + size - 0x300;
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

    u8* pixels = (u8*)buf + 0x80;
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
    return (void*)1;
}

// ---------------------------------------------------------------------------
// CDDSurface::LoadPcx
// Byte-identical to LoadBmp except for the per-format decode helper (DecodePcx).
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

// ---------------------------------------------------------------------------
// The two in-surface RLE row-decoders retail compiled UNOPTIMIZED: a full ebp
// frame with every local spilled to the stack, no register allocation or strength
// reduction - inside this otherwise-/O2 obj. One obj = one flag set, so the old
// per-unit /Od profile (fileimagerundecode) was structurally impossible; the
// only period mechanism is a `#pragma optimize("", off)` island in the source
// (VC5 supports it), modeled exactly so here. Locals are declared in retail's
// /Od stack-slot order so the [ebp-N] displacements match.
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
    sp = (u8*)src;
    dst = 0;
    for (row = 0; row < height; row++) {
        dst = (u8*)dstBuf + width * row;
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
    sp = (u8*)src;
    dst = 0;
    for (row = 0; row < height; row++) {
        base = row * width * 3;
        dst = (u8*)dstBuf + base;
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
        dst = (u8*)dstBuf + base + 1;
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
        dst = (u8*)dstBuf + base + 2;
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

// ---------------------------------------------------------------------------
// CDDSurface::DecodePcxData
// The low-level PID-ish run decoder shared by DecodePcxEx. `surf` is the source
// header (flags @ +4, width @ +8, height @ +0xc, run data @ +0x20); a4 is a
// control word whose high bits are toggled from flags&4 / flags&2; a5 the
// transparency colour. Width must be 4-aligned. The destination plane is set up
// via BlitSurf, then the run is expanded in place (DecodeRun8) or, when the
// surface bpp differs, into a scratch buffer (RunDecode1) and blit through the
// palette (built from the trailing 768-byte VGA palette when flags&0x80 is set,
// else the surface palette). flags&1 (TRANSPARENCY) installs a5 via FillPalette.
RVA(0x001457a0, 0x22c)
i32 CDDSurface::DecodePcxData(void* surf, void* buf, i32 size, i32 a4, i32 a5) {
    u8* hdr = (u8*)buf; // the source PID/PCX header
    CDDrawPtrCollections* dst = (CDDrawPtrCollections*)surf;
    i32 flags = *(i32*)(hdr + 4);
    i32 w = *(i32*)(hdr + 8);
    i32 h = *(i32*)(hdr + 0xc);
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
        if ((u32)size <= 0x300) {
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

// ===========================================================================
// CDDSurface::DecodePcxEx
//
// Opens a PCX file, reads data, calls DecodePcxData.
// ===========================================================================
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

    i32 result = DecodePcxData(surf, buf, len, (i32)a3, (i32)a4);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CDDSurface::DecodePid
// `buf` is a .PID payload (libwap32 layout: flags @ +4, width @ +8, height @
// +0xc, run data @ +0x20). Width must be 4-aligned and the geometry must match
// the destination surface (this->m_width / m_height). `surf` is the palette context
// (the CDDrawPtrCollections display manager: m_palBpp bpp / m_palette palette /
// m_hasPalette have-palette). When flags&0x80
// (EMBEDDED_PALETTE) the trailing 768-byte VGA palette is expanded into
// s_palPidData; otherwise the surface palette is used. The 8bpp run is expanded
// in place (DecodeRun8) or, when the surface bpp differs, into a scratch buffer
// (RunDecode1) and blit through the palette. flags&1 (TRANSPARENCY) installs the
// transparent colour (surf2) via FillPalette. Returns 1 on success, 0 on failure.
RVA(0x00145b10, 0x1b5)
void* CDDSurface::DecodePid(void* surf, void* buf, u32 size, void* surf2) {
    CDDrawPtrCollections* pal = (CDDrawPtrCollections*)surf;
    u8* hdr = (u8*)buf;
    i32 flags = *(i32*)(hdr + 4);
    i32 width = *(i32*)(hdr + 8);
    i32 height = *(i32*)(hdr + 0xc);
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
        u8* src = (u8*)buf + size - 0x300;
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
        FillPalette((u32)surf2);
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// CDDSurface::LoadPid
// Like LoadBmp/LoadPcx, but: (1) it does NOT guard length==0 - it allocates the
// buffer for whatever GetLength() returns and only null-checks the allocation;
// (2) the decoder takes a fourth pass-through arg (a3).
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

// ===========================================================================
// Class-metadata annotations (EOF-hosted). CFileImageHeldSurface (the thiscall
// held-surface slot-dispatch view) emits no vtable -> VTBL skip. CFileImageInfo was
// folded into CDDSurface (it is a 2nd CDDSurface instance passed as `info`).
// ===========================================================================
// --- shared CDDSurface (Image.h) header classes ---
SIZE_UNKNOWN(CFileImageHeldSurface);
SIZE_UNKNOWN(CFileImageSrc);
SIZE_UNKNOWN(CFileImagePal);
SIZE_UNKNOWN(CDDSurface);
// --- header data-record structs (Image.h / CFileImageRecords.h; annotated here) ---
SIZE_UNKNOWN(DecodeSrc);
SIZE(ClipRect16, 0x10); // 16-byte by-value rect/clip record
SIZE_UNKNOWN(BmpFileHeader);
SIZE_UNKNOWN(TgaHeader);
