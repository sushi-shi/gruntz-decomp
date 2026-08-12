#ifndef SRC_IMAGE_CFILEIMAGERECORDS_H
#define SRC_IMAGE_CFILEIMAGERECORDS_H

#include <rva.h>

#include <Win32.h>

#include <Image/PcxFormat.h>
#include <Ints.h>

struct Bmp256Info {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
};

union BmpInfoHeaderStamp {
    BITMAPINFOHEADER m_ih;
    struct {
        DWORD m_biSize;
        LONG m_biWidth;
        LONG m_biHeight;
        DWORD m_planesAndBitCount;
    };
};

union BmpFileHeaderStamp {
    BITMAPFILEHEADER m_hdr;
    char m_bytes[0xe];
};

#pragma pack(push, 1)

struct BmpFileImage {
    BITMAPFILEHEADER fh;
    Bmp256Info info;
};
#pragma pack(pop)

#pragma pack(push, 1)

struct PcxHeader {
    u8 m_magic;
    u8 m_version;
    u8 m_encoding;
    GZ_ENUM_STORAGE(PcxBitsPerPlane, u8) m_bitsPerPixel;
    i16 m_xMin;
    i16 m_yMin;
    i16 m_xMax;
    i16 m_yMax;
    char m_pad0c[0x41 - 0x0c];
    GZ_ENUM_STORAGE(PcxPlaneCount, i8) m_planes;
    char m_pad42[0x80 - 0x42];

    u8 m_pixels[1];
};
#pragma pack(pop)

#endif // SRC_IMAGE_CFILEIMAGERECORDS_H
