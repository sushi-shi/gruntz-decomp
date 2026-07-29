#ifndef SRC_IMAGE_CFILEIMAGERECORDS_H
#define SRC_IMAGE_CFILEIMAGERECORDS_H

#include <Ints.h>
#include <Win32.h> // BITMAPINFOHEADER / RGBQUAD (inert when <Mfc.h> already pulled windows.h)
#include <rva.h>

// The 0x428-byte BMP info block CRezImage::SaveBmp builds on the stack and Writes
// straight after the file header: a BITMAPINFOHEADER followed by the FULL 256-entry
// colour table. wingdi's BITMAPINFO declares that table `[1]`, so the 8bpp form has
// to be named separately (retail zeroes exactly 0x10a dwords = 0x428 B over it).
struct Bmp256Info {
    BITMAPINFOHEADER bmiHeader; // +0x000
    RGBQUAD bmiColors[256];     // +0x028
};
SIZE(0x428);

// BITMAPINFOHEADER as the BMP writers ZERO it: retail clears the header with a
// `mov ecx,0xb; rep stosd` and the field-wise spelling reproduces it, which means
// the biPlanes|biBitCount pair goes down as ONE dword. (An explicit memset measured
// 14 points WORSE - 52.57 -> 38.40 - do not retry it.) Naming that dword here keeps
// the store shape without a pun; the fields' own names stay on the SDK arm.
union BmpInfoHeaderStamp {
    BITMAPINFOHEADER m_ih; // the field view
    struct {
        DWORD m_biSize;
        LONG m_biWidth;
        LONG m_biHeight;
        DWORD m_planesAndBitCount; // biPlanes | biBitCount<<16 - cleared as one dword
    };
};
SIZE(0x28);

// BITMAPFILEHEADER as every BMP writer here STAMPS it: retail inlines a strcpy off
// the g_bmpHeaderTemplate / "BM" literal (`mov edi,<lit>; repnz scas al` then
// `rep movs`) over the header's leading bytes - a bfType word store emits neither.
// Both readings of the same 14 bytes are real, so the byte arm is named here rather
// than punned at each writer. (No pragma pack: BITMAPFILEHEADER carries its own
// 2-byte packing, and matching the union's alignment to it keeps the stack slot.)
union BmpFileHeaderStamp {
    BITMAPFILEHEADER m_hdr; // the field view
    char m_bytes[0xe];      // the "BM" magic-stamp view
};
SIZE(0xe);

#pragma pack(push, 1)
// A whole 8bpp BMP file image as it sits in the read buffer: the file header, then
// the info header and its full 256-entry colour table, then the bits at bfOffBits.
// (DecodeSrc was a pad-view of exactly this - its m_0a/m_12/m_16/m_1c were
// bfOffBits/biWidth/biHeight/biBitCount. Do not reintroduce it.)
struct BmpFileImage {
    BITMAPFILEHEADER fh; // +0x000  packed to 14 B
    Bmp256Info info;     // +0x00e  BITMAPINFOHEADER + bmiColors[256]
};
SIZE(0x436); // 0x0e file header + 0x428 info block; the bits follow at bfOffBits
#pragma pack(pop)

#pragma pack(push, 1)
// The ZSoft PCX file header (0x80 B) - the on-disk layout the PCX decoders read.
struct PcxHeader {
    u8 m_magic;        // +0x00  0x0a
    u8 m_version;      // +0x01
    u8 m_encoding;     // +0x02  1 = RLE
    u8 m_bitsPerPixel; // +0x03  bits per pixel per plane (the decoders require 8)
    i16 m_xMin;        // +0x04  window left
    i16 m_yMin;        // +0x06  window top
    i16 m_xMax;        // +0x08  window right  (width  = m_xMax - m_xMin + 1)
    i16 m_yMax;        // +0x0a  window bottom (height = m_yMax - m_yMin + 1)
    char m_pad0c[0x41 - 0x0c];
    u8 m_planes; // +0x41  colour planes (1 -> 8bpp, 3 -> 24bpp)
    char m_pad42[0x80 - 0x42];
    // +0x80: the RLE pixel stream (m_pixels); a trailing 0x300 palette when 8bpp.
    u8 m_pixels[1];
};
SIZE(0x80); // header proper; m_pixels is the trailing stream
#pragma pack(pop)

// RtBitmapResHeader was a pad-view of BITMAPINFOHEADER (an RT_BITMAP resource IS
// one): m_0/m_4/m_8/m_e were biSize/biWidth/biHeight/biBitCount. Its +0x08 was typed
// as a CDDrawPtrCollections* only so a wrong Init1 call would compile. Do not revive.

#endif // SRC_IMAGE_CFILEIMAGERECORDS_H
