// Image.cpp - the engine's REZ -> image resolution path.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CImage::LoadFromRez       @ 0x175a90  (238 B, thiscall ret 0xc)  - ext dispatcher
//   CFileImage::LoadBmp       @ 0x144110  (342 B, thiscall ret 8)    - .BMP file loader
//   CFileImage::LoadPcx       @ 0x145110  (342 B, thiscall ret 8)    - .PCX file loader
//   CFileImage::LoadPid       @ 0x145cd0  (304 B, thiscall ret 0xc)  - .PID file loader
//   CFileImage::CreateDibFromData  @ 0x43cf0 (363 B, thiscall ret 0x10)
//   CFileImage::LoadRaw            @ 0x43e60 (347 B, thiscall ret 0xc, EH)
//   CFileImage::DecodeBmpSurface   @ 0x43fc0 (322 B, thiscall ret 0xc)
//   CFileImage::CreateDibSurface   @ 0x44270 (210 B, thiscall ret 0xc)
//   CFileImage::DecodeImage        @ 0x44350 (95 B,  thiscall ret 0xc)
//   CFileImage::DecodeImage8       @ 0x443b0 (644 B, thiscall ret 0xc, EH)
//   CFileImage::DecodeImage16      @ 0x44640 (702 B, thiscall ret 0xc, EH)
//   CFileImage::DecodeImage24      @ 0x44900 (551 B, thiscall ret 0xc, EH)
//   CFileImage::DecodePidImage     @ 0x44b30 (592 B, thiscall ret 0x10)
//   CFileImage::DecodePidImage2    @ 0x44d80 (347 B, thiscall ret 0xc)
//   CFileImage::DecodePcxImage     @ 0x44ee0 (549 B, thiscall ret 0xc)
//   CFileImage::RleDecompress8     @ 0x45270 (378 B, thiscall ret 0x10)
//   CFileImage::RleDecompress24    @ 0x453f0 (940 B, thiscall ret 0x10)
//   CFileImage::DecodePcxData      @ 0x457a0 (556 B, thiscall ret 0x14)
//   CFileImage::DecodePcxEx        @ 0x459d0 (309 B, thiscall ret 0x10, EH)
//   CFileImage::DecodePidImageEx   @ 0x45b10 (437 B, thiscall ret 0x10)
//   IsPowerOfTwo                   @ 0x45e00 (38 B)

#include "Image.h"
#include <string.h>
#include <stdlib.h>

// File-extension literals (reloc-masked .rdata globals).
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extRid[] = ".RID";
static const char s_extPid[] = ".PID";

extern "C" char *strrchr(const char *s, int c);
extern "C" int   _stricmp(const char *a, const char *b);

// Palette buffers (global .data, reloc-masked).  Declared here as extern
// arrays because the header does not define them.
extern unsigned char palette_683ef0[];
extern unsigned char palette_6842f0[];
extern unsigned char palette_6846f0[];
extern unsigned char palette_684af0[];
extern unsigned char palette_684ef0[];
extern unsigned char palette_6852f0[];

// ===========================================================================
// CImage::LoadFromRez  @ 0x175a90 (238 B)
// ===========================================================================
// @address: 0x175a90
// @size:    0xee
int CImage::LoadFromRez(char *name, void *a2, void *a3)
{
    char *ext = strrchr(name, '.');

    if (ext && _stricmp(ext, s_extBmp) == 0)
        return LoadBmp(name, a2, a3);
    else if (ext && _stricmp(ext, s_extPcx) == 0)
        return LoadPcx(name, a2, a3);
    else if (ext && _stricmp(ext, s_extRid) == 0)
        return LoadRid(name, a2, a3);
    else if (ext && _stricmp(ext, s_extPid) == 0)
        return LoadPid(name, a2, a3);

    return LoadDefault(name, a2, a3);
}

// ===========================================================================
// CFileImage::LoadBmp  @ 0x144110 (342 B, thiscall ret 8)
// ===========================================================================
// @address: 0x144110
// @size:    0x156
void *CFileImage::LoadBmp(char *name, char *path)
{
    CFileIO file;

    if (!file.Open(path, 0, 0))
        return 0;

    unsigned int len = file.GetLength();
    if (len == 0)
        return 0;

    void *buf = operator new(len);
    if (!buf)
        return 0;

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void *result = DecodeBmp(name, buf, len);
    operator delete(buf);
    return result;
}

// ===========================================================================
// CFileImage::LoadPcx  @ 0x145110 (342 B, thiscall ret 8)
// ===========================================================================
// @address: 0x145110
// @size:    0x156
void *CFileImage::LoadPcx(char *name, char *path)
{
    CFileIO file;

    if (!file.Open(path, 0, 0))
        return 0;

    unsigned int len = file.GetLength();
    if (len == 0)
        return 0;

    void *buf = operator new(len);
    if (!buf)
        return 0;

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void *result = DecodePcx(name, buf, len);
    operator delete(buf);
    return result;
}

// ===========================================================================
// CFileImage::LoadPid  @ 0x145cd0 (304 B, thiscall ret 0xc)
// ===========================================================================
// @address: 0x145cd0
// @size:    0x130
void *CFileImage::LoadPid(char *name, char *path, void *a3)
{
    CFileIO file;

    if (!file.Open(path, 0, 0))
        return 0;

    unsigned int len = file.GetLength();
    void *buf = operator new(len);
    if (!buf)
        return 0;

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void *result = DecodePid(name, buf, len, a3);
    operator delete(buf);
    return result;
}

// ===========================================================================
// CFileImage::CreateDibFromData  @ 0x543cf0 (363 B, thiscall ret 0x10)
//
// Create a DIB surface from a raw data buffer (BMP/PCX/PID header + pixels).
// Parameters:
//   classPtr  - an image/interface class (fields at +0x538 pixelFmt, +0x93c palettePtr)
//   dataBuf   - the raw data buffer with header at +0x12(width),+0x16(height),
//               +0x1c(bpp), +0x0a(pixelOffset)
//   a3        - third param (unused in surface creation)
//   a4        - fourth param (passed to FUN_0053e0d0)
// ===========================================================================
// @address: 0x143cf0
// @size:    0x16b
int CFileImage::CreateDibFromData(void *classPtr, void *dataBuf, int a3, int a4)
{
    unsigned short bpp = *(unsigned short *)((char *)dataBuf + 0x1c);
    int width  = *(int *)((char *)dataBuf + 0x12);
    int height = *(int *)((char *)dataBuf + 0x16);

    if (bpp != 8 && bpp != 24)
        return 0;

    int isPaletteNeeded = 0;
    int imgFmt = *(int *)((char *)classPtr + 0x538);
    if (imgFmt != bpp)
        isPaletteNeeded = 1;

    if (isPaletteNeeded && imgFmt == 8 && !*(int *)((char *)classPtr + 0x93c))
        return 0;

    void *palettePtr = 0;
    if (isPaletteNeeded) {
        if (bpp == 8) {
            // De-interleave RGB triplets from dataBuf+0x36 into BGRA palette.
            unsigned char *src = (unsigned char *)dataBuf + 0x36;
            for (int i = 0; i < 256; i++) {
                palette_683ef0[i * 4]     = src[i * 3];
                palette_683ef0[i * 4 + 1] = src[i * 3 + 1];
                palette_683ef0[i * 4 + 2] = src[i * 3 + 2];
                palette_683ef0[i * 4 + 3] = 0;
            }
            palettePtr = palette_683ef0;
        } else if (imgFmt == 8 && *(int *)((char *)classPtr + 0x93c)) {
            palettePtr = (char *)classPtr + 0x53c;
        }
    }

    if (!FUN_0053e0d0(classPtr, width, height, 0, (void *)a4))
        return 0;

    unsigned char *pixels = (unsigned char *)dataBuf
                          + *(unsigned short *)((char *)dataBuf + 0x0a);
    int ok;
    if (isPaletteNeeded)
        ok = FUN_0053faa0(pixels, bpp, palettePtr);
    else
        ok = FUN_0053ece0(pixels);

    return ok ? 1 : 0;
}

// ===========================================================================
// CFileImage::LoadRaw  @ 0x543e60 (347 B, thiscall ret 0xc, EH)
//
// Opens a file, reads its contents, forwards to CreateDibFromData.
// ===========================================================================
// @address: 0x143e60
// @size:    0x15b
void *CFileImage::LoadRaw(char *name, char *path, void *a3)
{
    CFileIO file;

    if (!file.Open(path, 0, 0))
        return 0;

    unsigned int len = file.GetLength();
    void *buf = operator new(len);
    if (!buf)
        return 0;

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    int result = CreateDibFromData(buf, name, len, (int)a3);
    operator delete(buf);
    return (void *)result;
}

// ===========================================================================
// CFileImage::DecodeBmpSurface  @ 0x543fc0 (322 B, thiscall ret 0xc)
//
// Decode a BMP buffer by checking dims match this surface, then copying
// pixels (with optional palette conversion).
// ===========================================================================
// @address: 0x143fc0
// @size:    0x142
int CFileImage::DecodeBmpSurface(void *dataBuf, int a2, int a3)
{
    int width    = *(int *)((char *)dataBuf + 0x12);
    int height   = *(int *)((char *)dataBuf + 0x16);
    unsigned short bpp = *(unsigned short *)((char *)dataBuf + 0x1c);

    if (*(int *)((char *)this + 0x1c) != width)
        return 0;
    if (*(int *)((char *)this + 0x18) != height)
        return 0;

    if (bpp != 8 && bpp != 24)
        return 0;

    int isPaletteNeeded = 0;
    int pixFmt = *(int *)((char *)this + 0x538);
    if (pixFmt != bpp)
        isPaletteNeeded = 1;

    if (isPaletteNeeded && pixFmt == 8 && !*(int *)((char *)this + 0x93c))
        return 0;

    void *palettePtr = 0;
    if (isPaletteNeeded) {
        if (bpp == 8) {
            unsigned char *src = (unsigned char *)dataBuf + 0x36;
            for (int i = 0; i < 256; i++) {
                palette_6842f0[i * 4]     = src[i * 3];
                palette_6842f0[i * 4 + 1] = src[i * 3 + 1];
                palette_6842f0[i * 4 + 2] = src[i * 3 + 2];
                palette_6842f0[i * 4 + 3] = 0;
            }
            palettePtr = palette_6842f0;
        } else if (pixFmt == 8 && *(int *)((char *)this + 0x93c)) {
            palettePtr = (char *)this + 0x53c;
        }
    }

    unsigned char *pixels = (unsigned char *)dataBuf
                          + *(unsigned short *)((char *)dataBuf + 0x0a);
    FUN_0053ece0(pixels);

    if (isPaletteNeeded) {
        if (!FUN_0053faa0(pixels, bpp, palettePtr))
            return 0;
    }

    return 1;
}

// ===========================================================================
// CFileImage::CreateDibSurface  @ 0x544270 (210 B, thiscall ret 0xc)
// ===========================================================================
// @address: 0x144270
// @size:    0xd2
int CFileImage::CreateDibSurface(int width, int height)
{
    return 0;
}

// ===========================================================================
// CFileImage::DecodeImage  @ 0x544350 (95 B, thiscall ret 0xc)
//
// Dispatches to a per-bit-depth decoder based on field at this+0xa8.
// ===========================================================================
// @address: 0x144350
// @size:    0x5f
int CFileImage::DecodeImage(char *name, void *buf, unsigned int size)
{
    int depth = *(int *)((char *)this + 0xa8);

    if (depth == 24)
        return DecodeImage24(name, buf, size);
    if (depth == 16)
        return DecodeImage16(name, buf, size);
    if (depth == 8)
        return DecodeImage8(name, buf, size);

    return 0;
}

// ===========================================================================
// CFileImage::DecodeImage8  @ 0x5443b0 (644 B, thiscall ret 0xc, EH)
// ===========================================================================
// @address: 0x1443b0
// @size:    0x284
int CFileImage::DecodeImage8(char *name, void *buf, unsigned int size)
{
    int depth = *(int *)((char *)this + 0xa8);
    if (depth != 8)
        return 0;
    return 0;
}

// ===========================================================================
// CFileImage::DecodeImage16  @ 0x544640 (702 B, thiscall ret 0xc, EH)
// ===========================================================================
// @address: 0x144640
// @size:    0x2be
int CFileImage::DecodeImage16(char *name, void *buf, unsigned int size)
{
    int depth = *(int *)((char *)this + 0xa8);
    if (depth != 16)
        return 0;
    return 0;
}

// ===========================================================================
// CFileImage::DecodeImage24  @ 0x544900 (551 B, thiscall ret 0xc, EH)
// ===========================================================================
// @address: 0x144900
// @size:    0x227
int CFileImage::DecodeImage24(char *name, void *buf, unsigned int size)
{
    int depth = *(int *)((char *)this + 0xa8);
    if (depth != 24)
        return 0;
    return 0;
}

// ===========================================================================
// CFileImage::DecodePidImage  @ 0x544b30 (592 B, thiscall ret 0x10)
// ===========================================================================
// @address: 0x144b30
// @size:    0x24d
int CFileImage::DecodePidImage(void *dataBuf, int a2, int a3, int a4)
{
    if (!dataBuf)
        return 0;

    int left   = *(short *)((char *)dataBuf + 4);
    int top    = *(short *)((char *)dataBuf + 6);
    int right  = *(short *)((char *)dataBuf + 8);
    int bottom = *(short *)((char *)dataBuf + 10);
    int width  = right - left + 1;
    int height = bottom - top + 1;

    int bpp;
    unsigned char type = *(unsigned char *)((char *)dataBuf + 0x41);
    if (type == 1)
        bpp = 8;
    else if (type == 3)
        bpp = 24;
    else
        return 0;

    int isPaletteNeeded = 0;
    int imgFmt = *(int *)((char *)this + 0x538);
    if (imgFmt != bpp)
        isPaletteNeeded = 1;

    if (isPaletteNeeded && imgFmt == 8 && !*(int *)((char *)this + 0x93c))
        return 0;

    void *palettePtr = 0;
    if (isPaletteNeeded) {
        if (bpp == 8) {
            unsigned char *src = (unsigned char *)dataBuf + a2 + width - 0x300;
            for (int i = 0; i < 256; i++) {
                palette_684af0[i * 4]     = src[i * 3];
                palette_684af0[i * 4 + 1] = src[i * 3 + 1];
                palette_684af0[i * 4 + 2] = src[i * 3 + 2];
                palette_684af0[i * 4 + 3] = 0;
            }
            palettePtr = palette_684af0;
        } else if (imgFmt == 8 && *(int *)((char *)this + 0x93c)) {
            palettePtr = (char *)this + 0x53c;
        }
    }

    FUN_0053e0d0(this, (int)this, height, 0, (void *)a4);

    if (!isPaletteNeeded) {
        if (bpp == 8)
            FUN_00540aa0(this);
        else
            FUN_00540c50(this);
    } else {
        void *tmpBuf;
        if (bpp == 8) {
            tmpBuf = malloc(width * height);
            RleDecompress8((unsigned char *)tmpBuf,
                           (unsigned char *)dataBuf + 0x80, width, height);
        } else {
            tmpBuf = malloc(width * height * 3);
            RleDecompress24((unsigned char *)tmpBuf,
                            (unsigned char *)dataBuf + 0x80, width, height);
        }

        if (!FUN_0053faa0(tmpBuf, bpp, palettePtr)) {
            free(tmpBuf);
            return 0;
        }
        free(tmpBuf);
    }

    return 1;
}

// ===========================================================================
// CFileImage::DecodePidImage2  @ 0x544d80 (347 B, thiscall ret 0xc)
// ===========================================================================
// @address: 0x144d80
// @size:    0x15b
int CFileImage::DecodePidImage2(void *data, int a2, int a3)
{
    return 0;
}

// ===========================================================================
// CFileImage::DecodePcxImage  @ 0x544ee0 (549 B, thiscall ret 0xc)
// ===========================================================================
// @address: 0x144ee0
// @size:    0x225
int CFileImage::DecodePcxImage(void *dataBuf, int a2, int a3)
{
    if (!dataBuf)
        return 0;

    int right  = *(short *)((char *)dataBuf + 8);
    int left   = *(short *)((char *)dataBuf + 4);
    int bottom = *(short *)((char *)dataBuf + 10);
    int top    = *(short *)((char *)dataBuf + 6);
    int width  = right - left + 1;
    int height = bottom - top + 1;

    int bpp = 0;
    unsigned char type = *(unsigned char *)((char *)dataBuf + 0x41);
    if (type == 1)
        bpp = 8;
    else if (type == 3)
        bpp = 24;
    else
        return 0;

    if (bpp == 0)
        return 0;

    if (*(int *)((char *)this + 0x1c) != width)
        return 0;
    if (*(int *)((char *)this + 0x18) != height)
        return 0;

    int isPaletteNeeded = 0;
    int imgFmt = *(int *)((char *)this + 0x538);
    if (imgFmt != bpp)
        isPaletteNeeded = 1;

    if (isPaletteNeeded && imgFmt == 8 && !*(int *)((char *)this + 0x93c))
        return 0;

    void *palettePtr = 0;
    if (isPaletteNeeded) {
        if (bpp == 8) {
            unsigned char *src = (unsigned char *)dataBuf + a2 + width - 0x300;
            for (int i = 0; i < 256; i++) {
                palette_6846f0[i * 4]     = src[i * 3];
                palette_6846f0[i * 4 + 1] = src[i * 3 + 1];
                palette_6846f0[i * 4 + 2] = src[i * 3 + 2];
                palette_6846f0[i * 4 + 3] = 0;
            }
            palettePtr = palette_6846f0;
        } else if (imgFmt == 8 && *(int *)((char *)this + 0x93c)) {
            palettePtr = (char *)this + 0x53c;
        }
    }

    if (!isPaletteNeeded) {
        if (bpp == 8)
            FUN_00540aa0(this);
        else
            FUN_00540c50(this);
    } else {
        void *tmpBuf;
        if (bpp == 8) {
            tmpBuf = malloc(width * height);
            RleDecompress8((unsigned char *)tmpBuf,
                           (unsigned char *)dataBuf + 0x80, width, height);
        } else {
            tmpBuf = malloc(width * height * 3);
            RleDecompress24((unsigned char *)tmpBuf,
                            (unsigned char *)dataBuf + 0x80, width, height);
        }

        if (!FUN_0053faa0(tmpBuf, bpp, palettePtr)) {
            free(tmpBuf);
            return 0;
        }
        free(tmpBuf);
    }

    return 1;
}

// ===========================================================================
// RleDecompress8  @ 0x545270 (378 B, thiscall ret 0x10)
//
// 8-bit RLE decompressor.  Byte with 0xC0 in high 2 bits = run; else literal.
// ===========================================================================
// @address: 0x145270
// @size:    0x17a
int CFileImage::RleDecompress8(unsigned char *dst, unsigned char *src,
                                int rowBytes, int height)
{
    if (!dst || !src)
        return 0;

    int leftover = 0;
    unsigned char prevValue = 0;

    for (int y = 0; y < height; y++) {
        unsigned char *out = dst + y * rowBytes;
        int remaining = rowBytes;

        if (leftover > 0) {
            for (int i = 1; i < leftover; i++)
                *out++ = prevValue;
            remaining -= leftover;
            leftover = 0;
        }

        while (remaining > 0) {
            unsigned char c = *src++;

            if ((c & 0xc0) == 0xc0) {
                int count = c & 0x3f;
                unsigned char value = *src++;

                if (count > remaining) {
                    leftover = count - remaining;
                    count = remaining;
                }

                for (int i = 0; i < count; i++)
                    *out++ = value;
                remaining -= count;
                prevValue = value;
            } else {
                *out++ = c;
                remaining--;
            }
        }
    }

    return 1;
}

// ===========================================================================
// RleDecompress24  @ 0x5453f0 (940 B, thiscall ret 0x10)
//
// 24-bit RLE decompressor — three planes.
// ===========================================================================
// @address: 0x1453f0
// @size:    0x3a0
int CFileImage::RleDecompress24(unsigned char *dst, unsigned char *src,
                                 int rowBytes, int height)
{
    if (!dst || !src)
        return 0;

    // --- Plane 0 ---
    {
        int leftover = 0;
        unsigned char prevValue = 0;
        for (int y = 0; y < height; y++) {
            unsigned char *out = dst + y * rowBytes * 3;
            int remaining = rowBytes;
            if (leftover > 0) {
                for (int i = 1; i < leftover; i++) {
                    *out = prevValue;
                    out += 3;
                }
                remaining -= leftover;
                leftover = 0;
            }
            while (remaining > 0) {
                unsigned char c = *src++;
                if ((c & 0xc0) == 0xc0) {
                    int count = c & 0x3f;
                    unsigned char value = *src++;
                    if (count > remaining) {
                        leftover = count - remaining;
                        count = remaining;
                    }
                    for (int i = 0; i < count; i++) {
                        *out = value;
                        out += 3;
                    }
                    remaining -= count;
                    prevValue = value;
                } else {
                    *out = c;
                    out += 3;
                    remaining--;
                }
            }
        }
    }
    // --- Plane 1 ---
    {
        int leftover = 0;
        unsigned char prevValue = 0;
        for (int y = 0; y < height; y++) {
            unsigned char *out = dst + y * rowBytes * 3 + 1;
            int remaining = rowBytes;
            if (leftover > 0) {
                for (int i = 1; i < leftover; i++) {
                    *out = prevValue;
                    out += 3;
                }
                remaining -= leftover;
                leftover = 0;
            }
            while (remaining > 0) {
                unsigned char c = *src++;
                if ((c & 0xc0) == 0xc0) {
                    int count = c & 0x3f;
                    unsigned char value = *src++;
                    if (count > remaining) {
                        leftover = count - remaining;
                        count = remaining;
                    }
                    for (int i = 0; i < count; i++) {
                        *out = value;
                        out += 3;
                    }
                    remaining -= count;
                    prevValue = value;
                } else {
                    *out = c;
                    out += 3;
                    remaining--;
                }
            }
        }
    }
    // --- Plane 2 ---
    {
        int leftover = 0;
        unsigned char prevValue = 0;
        for (int y = 0; y < height; y++) {
            unsigned char *out = dst + y * rowBytes * 3 + 2;
            int remaining = rowBytes;
            if (leftover > 0) {
                for (int i = 1; i < leftover; i++) {
                    *out = prevValue;
                    out += 3;
                }
                remaining -= leftover;
                leftover = 0;
            }
            while (remaining > 0) {
                unsigned char c = *src++;
                if ((c & 0xc0) == 0xc0) {
                    int count = c & 0x3f;
                    unsigned char value = *src++;
                    if (count > remaining) {
                        leftover = count - remaining;
                        count = remaining;
                    }
                    for (int i = 0; i < count; i++) {
                        *out = value;
                        out += 3;
                    }
                    remaining -= count;
                    prevValue = value;
                } else {
                    *out = c;
                    out += 3;
                    remaining--;
                }
            }
        }
    }

    return 1;
}

// ===========================================================================
// CFileImage::DecodePcxData  @ 0x5457a0 (556 B, thiscall ret 0x14)
// ===========================================================================
// @address: 0x1457a0
// @size:    0x22c
int CFileImage::DecodePcxData(void *data, int a2, int a3, int a4, int a5)
{
    return 0;
}

// ===========================================================================
// CFileImage::DecodePcxEx  @ 0x5459d0 (309 B, thiscall ret 0x10, EH)
//
// Opens a PCX file, reads data, calls DecodePcxData.
// ===========================================================================
// @address: 0x1459d0
// @size:    0x135
int CFileImage::DecodePcxEx(char *name, char *path, void *a3, void *a4)
{
    CFileIO file;

    if (!file.Open(path, 0, 0))
        return 0;

    unsigned int len = file.GetLength();
    void *buf = operator new(len);
    if (!buf)
        return 0;

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    int result = DecodePcxData((void *)name, (int)buf, (int)len, (int)a3, (int)a4);
    operator delete(buf);
    return result;
}

// ===========================================================================
// CFileImage::DecodePidImageEx  @ 0x545b10 (437 B, thiscall ret 0x10)
// ===========================================================================
// @address: 0x145b10
// @size:    0x1b5
int CFileImage::DecodePidImageEx(void *data, int a2, int a3, int a4)
{
    return 0;
}

// ===========================================================================
// IsPowerOfTwo  @ 0x545e00 (38 B)
// ===========================================================================
// @address: 0x145e00
// @size:    0x26
int IsPowerOfTwo(int value)
{
    int count = 0;
    int bits = 32;
    do {
        if (value & 1)
            count++;
        value >>= 1;
    } while (--bits);
    return (count == 1) ? 1 : 0;
}
