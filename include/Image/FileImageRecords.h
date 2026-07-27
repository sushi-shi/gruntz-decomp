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

#pragma pack(push, 1)
// DecodeSrc was a pad-view of the standard BMP file image (BITMAPFILEHEADER +
// BITMAPINFO); its m_0a/m_12/m_16/m_1c were bfOffBits/biWidth/biHeight/biBitCount.
// DecodeRun now reads the real Win32 types - do not reintroduce it.
#pragma pack(pop)

#pragma pack(push, 1)
// The ZSoft PCX file header (0x80 B) - the on-disk layout the PCX decoders read.
struct PcxHeader {
    u8 m_magic;         // +0x00  0x0a
    u8 m_version;       // +0x01
    u8 m_encoding;      // +0x02  1 = RLE
    u8 m_bitsPerPixel;  // +0x03  bits per pixel per plane (the decoders require 8)
    i16 m_xMin;         // +0x04  window left
    i16 m_yMin;         // +0x06  window top
    i16 m_xMax;         // +0x08  window right  (width  = m_xMax - m_xMin + 1)
    i16 m_yMax;         // +0x0a  window bottom (height = m_yMax - m_yMin + 1)
    char m_pad0c[0x41 - 0x0c];
    u8 m_planes; // +0x41  colour planes (1 -> 8bpp, 3 -> 24bpp)
    char m_pad42[0x80 - 0x42];
    // +0x80: the RLE pixel stream (m_pixels); a trailing 0x300 palette when 8bpp.
    u8 m_pixels[1];
};
SIZE(0x80); // header proper; m_pixels is the trailing stream
#pragma pack(pop)

struct RtBitmapResHeader {
    i32 m_0; // +0x00 (payload size; data follows at +m_0+0x400)
    i32 m_4; // +0x04 (stored into the surface desc width)
    class CDDrawPtrCollections* m_8; // +0x08  the pool forwarded to the slot-2 init
    char m_padc[0xe - 0xc];
    i16 m_e; // +0x0e (must be 8)
};
SIZE_UNKNOWN(); // resource-header view (only the first 0x10 bytes pinned)

#endif // SRC_IMAGE_CFILEIMAGERECORDS_H
