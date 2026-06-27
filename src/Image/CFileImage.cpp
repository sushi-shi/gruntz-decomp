// CFileImage.cpp - the file-backed surface image cluster (0x13ebb0..0x144d80).
// Six methods in retail-RVA order: a vertical row-flip of the locked surface, a
// decode thunk, two export-to-file paths (BMP + TGA headers + bottom-up rows), the
// run-length decode dispatcher, and the file load wrapper. The held surface, its
// Lock (0x13e6d0) / Unlock (vtable +0x80), the decode runners (0x140aa0/0x140c50/
// 0x145270/0x1453f0), the Blit (0x13faa0) and the inner thunk (0x1471d0) are all
// external engine callees (reloc-masked); the export/load methods slurp through the
// real MFC CFile. See include/Image/CFileImage.h for the layout.
//
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
// ---------------------------------------------------------------------------
#include <Image/CFileImage.h>

#include <Io/FileStream.h> // CFileIO (engine KERNEL32 file wrapper; LoadFile2)

#include <string.h> // memcpy / strlen (inlined to rep movs / repne scas)

// Engine allocator/freer (reloc-masked rel32). operator new @0x1b9b46 (NAFXCW),
// _RezFree @0x1b9b82.
void* operator new(u32 n);
extern "C" void RezFree(void* p);

// The DecodeRun source descriptor (a run-length image header distinct from
// CFileImageSrc): the dims @+0x12/+0x16 fed to BlitSurf, the run-data offset @+0x0a,
// the format word @+0x1c (8/0x18), and the trailing palette @+0x36. Packed: the dword
// fields sit at unaligned offsets (the engine reads them with plain x86 movs).
#pragma pack(push, 1)
struct DecodeSrc {
    char _00[0x0a];
    i32 m_0a; // +0x0a  run-data byte offset
    char _0e[0x12 - 0x0e];
    i32 m_12; // +0x12  dim a
    i32 m_16; // +0x16  dim b
    char _1a[0x1c - 0x1a];
    u16 m_1c; // +0x1c  format word
    char _1e[0x36 - 0x1e];
    // +0x36  source palette the grayscale-ramp build reads
};
#pragma pack(pop)

// The DecodeRun grayscale-ramp scratch (reloc-masked global at 0x683ef0; 0x400 bytes).
extern u8 g_683ef0[];

// The inner blit/decode worker 0x141280 forwards into (reloc-masked __thiscall). It
// takes a 16-byte rect/clip record by value (built on the thunk's stack from the last
// 4 args) plus the six leading scalar args.
struct ClipRect16 {
    i32 a, b, c, d;
};
struct ImageWorkerThis {
    i32 Run(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, ClipRect16 clip); // 0x1471d0
};

// The 256*3 grayscale-ramp scratch buffer the 24-bit decode-convert path fills
// (reloc-masked global at 0x684af0; +0x401 = the running write cursor, +0x801 = end).
extern u8 g_grayRamp[];

// The format string the save/decode path scans with strlen (reloc-masked).
extern char g_imageTag[];

// ---------------------------------------------------------------------------
// 0x13ebb0: FlipVertical - swap the locked surface's rows top-to-bottom through a
// one-row temp buffer. No-op for a <= 1-row image. Locks the surface (Lock), and on
// success allocates the temp row; a failed temp alloc unlocks and returns. __thiscall.
//
// @early-stop
// regalloc wall (~62%): logic + offsets + CFG + the 3 inner row-copy loops are exact.
// Residue is the callee-saved-register assignment cascade - retail pins `this` in ebx
// (every member read is [ebx+N]) and the loop var i in esi, while our cl assigns this
// to ebp and spills i to [esp+0x10]; the prologue push order (push esi before the
// first member read) and the running bottom-row pointer (add ecx,ebx) all cascade from
// that one choice. docs/patterns/zero-register-pinning.md + reread-member-view-pointer.md.
// Not source-steerable on a leaf this small; deferred to the final sweep.
RVA(0x0013ebb0, 0x126)
void CFileImage::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* buf = (u8*)Lock(0);
    if (buf == 0) {
        return;
    }
    u8* tmp = (u8*)operator new(m_width);
    if (tmp == 0) {
        m_surface->Unlock(0);
        return;
    }

    i32 width = m_width;
    i32 i = 0;
    i32 half = m_height / 2;
    if (half > 0) {
        do {
            // top row -> tmp
            u8* top = buf + i * m_pitch;
            i32 j = 0;
            if (width > 0) {
                do {
                    tmp[j] = *top;
                    ++top;
                    ++j;
                } while (j < width);
            }
            // bottom row -> top row
            i32 botRow = m_height - i - 1;
            u8* topDst = buf + i * m_pitch;
            u8* botSrc = buf + botRow * m_pitch;
            if (width > 0) {
                i32 k = width;
                do {
                    *topDst = *botSrc;
                    ++topDst;
                    ++botSrc;
                    --k;
                } while (k != 0);
            }
            // tmp -> bottom row
            u8* botDst = buf + botRow * m_pitch;
            i32 m = 0;
            if (width > 0) {
                do {
                    ++botDst;
                    botDst[-1] = tmp[m];
                    ++m;
                } while (m < width);
            }
            ++i;
        } while (i < half);
    }

    m_surface->Unlock(0);
    RezFree(tmp);
}

// ---------------------------------------------------------------------------
// 0x141280: DecodeThunk - a glue forwarder that rebuilds a 16-byte rect/clip record
// from its trailing args on the stack and tail-calls the image worker (0x1471d0) with
// the six leading scalar args + that record passed by value, then cleans 0x2c of stack
// (ret 0x28). The worker `this` arrives in ecx (re-pushed, not reloaded).
//
// @early-stop
// stack-forward wall (~48%): the 16-byte record build on the stack, the scalar
// re-pushes and the `ret 0x28` are faithful; residue is the exact scratch-register
// choice for the record copy + that retail re-pushes `this` (ecx) as the trailing arg
// (the worker gets `this` both in ecx and pushed) which has no clean /O2 source
// spelling. Deferred to the final sweep.
RVA(0x00141280, 0x4a)
void CFileImage::DecodeThunk(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 r0, i32 r1,
                            i32 r2, i32 r3) {
    ClipRect16 clip;
    clip.a = r0;
    clip.b = r1;
    clip.c = r2;
    clip.d = r3;
    ((ImageWorkerThis*)this)->Run(a1, a2, a3, a4, a5, a6, clip);
}

// ---------------------------------------------------------------------------
// 0x143cf0: DecodeRun - decode a run-length image (`src`, the DecodeSrc shape) into
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
i32 CFileImage::DecodeRun(CFileImageInfo* info, void* srcv, i32, i32 b) {
    DecodeSrc* src = (DecodeSrc*)srcv;
    i32 srcFmt = src->m_1c;
    if (srcFmt != 8 && srcFmt != 0x18) {
        return 0;
    }

    i32 convert = 0;
    i32 curFmt = info->m_538;
    if (curFmt != srcFmt) {
        convert = 1;
    }
    if (convert && curFmt == 8 && info->m_93c == 0) {
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
            if (info->m_93c != 0) {
                pal = info->m_53c;
            } else {
                pal = 0;
            }
        } else {
            pal = 0;
        }
    }

    if (BlitSurf(info, src->m_12, src->m_16, 0, b) == 0) {
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
// 0x143e60: LoadFile2 - open `path` via the engine CFileIO, slurp it whole into a heap
// buffer and hand it to DecodeRun. Each failure unwinds the CFileIO + returns 0; the
// buffer is RezFree'd after DecodeRun (and on a short read). /GX EH frame. ret 0xc.
//
// @early-stop
// ~98%: open/length/read/decode/free shapes + offsets are byte-faithful; residue is the
// /GX funcinfo state index push (eh-state-numbering-base.md) plus the DecodeRun callee's
// own divergence (it is the branch-scheduling wall above). Deferred to the final sweep.
RVA(0x00143e60, 0x15b)
i32 CFileImage::LoadFile2(CFileImageInfo* info, const char* path, i32 mode) {
    CFileIO file;
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
        RezFree(buf);
        return 0;
    }
    i32 result = DecodeRun(info, buf, len, mode);
    RezFree(buf);
    return result;
}

// A BMP RGBQUAD palette entry + the on-stack 14-byte BITMAPFILEHEADER the export
// builds (the 2-byte "BM" magic is strcpy'd from g_imageTag, then bfSize / bfOffBits).
struct BmpFileHeader {
    char magic[2];   // +0x00  "BM"
    char _02[0x06 - 0x02];
    u32 bfSize;    // +0x06  (width*0x28 + 0x436)
    u32 _0a;       // +0x0a
    u32 bfOffBits; // +0x0e -> stored at struct +0x0e... (0x436)
};

// ---------------------------------------------------------------------------
// 0x1443b0: SaveBmp - write this 8-bit image to a BMP file. Validates the surface
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
i32 CFileImage::SaveBmp(const char* path, void* pal, i32 mode) {
    if (IsValid() == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (m_bpp != 8) {
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
            m_surface->Unlock(0);
            return 0;
        }
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            m_surface->Unlock(0);
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

    m_surface->Unlock(0);
    return 1;
}

// The on-stack 0x2c-byte header the 24-bit export writes (a TGA-ish record): the
// "BM"-style magic strcpy'd from g_imageTag at +0, the width/height + a +0x10 size
// (width*height*3 + 0x3a), and the +0x10/+0x12 plane/bitcount words.
struct TgaHeader {
    char magic[2]; // +0x00
    char _02[0x06 - 0x02];
    u32 size;      // +0x06  (width*height*3 + 0x3a)
    char _0a[0x10 - 0x0a];
    i16 planes;    // +0x10
    i16 bitCount;  // +0x12
};

// ---------------------------------------------------------------------------
// 0x144900: SaveTga - write this 24-bit image to a TGA file. Mirrors SaveBmp but for
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
i32 CFileImage::SaveTga(const char* path, void* pal, i32 mode) {
    (void)pal;
    if (IsValid() == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    if (*path == 0) {
        return 0;
    }
    if (m_bpp != 0x18) {
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
            m_surface->Unlock(0);
            return 0;
        }
    } else {
        if (!file.Open(path, 0x1001, 0)) {
            m_surface->Unlock(0);
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

    m_surface->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x144b30: Decode - decode a run-length image (src) into this image. Validates the
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
i32 CFileImage::Decode(CFileImageInfo* info, CFileImageSrc* src, i32 len, i32 mode) {
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
    i32 curFmt = info->m_538;
    if (curFmt != srcFmt) {
        convert = 1;
    }
    if (convert && curFmt == 8 && info->m_93c == 0) {
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
            if (info->m_93c != 0) {
                palette = info->m_53c;
            } else {
                palette = 0;
            }
        } else {
            palette = 0;
        }
    }

    if (BeginDecode(info, width, height, 0, mode) == 0) {
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
            RezFree(buf);
            return 0;
        }
    }

    if (convert) {
        if (Blit(buf, srcFmt, palette, 1) == 0) {
            RezFree(buf);
            return 0;
        }
    }
    if (buf != 0) {
        RezFree(buf);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x144d80: LoadFile - open `path`, slurp it whole into a heap buffer, and hand the
// buffer (plus the caller's `src` descriptor + `mode`) to Decode. The local CFile
// destructs on every exit -> /GX EH frame. Each failure (open / empty / alloc /
// short read) returns 0; otherwise returns Decode's result. The read buffer is
// RezFree'd after Decode (and on a short read). ret 0xc.
// ---------------------------------------------------------------------------
RVA(0x00144d80, 0x15b)
i32 CFileImage::LoadFile(CFileImageInfo* info, const char* path, i32 mode) {
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
        RezFree(buf);
        return 0;
    }
    i32 result = Decode(info, (CFileImageSrc*)buf, len, mode);
    RezFree(buf);
    return result;
}
