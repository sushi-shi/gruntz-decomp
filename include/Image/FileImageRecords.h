// FileImageRecords.h - the internal data-record layouts the CFileImage save/decode
// path (src/Image/FileImage.cpp) reads/builds: the run-length decode source header and
// the two on-stack export file headers. Kept in this dedicated header (included only by
// CFileImage.cpp) rather than in <Image/Image.h> so the other Image TUs that include
// Image.h are not perturbed (adding these type defs to Image.h reschedules Image.cpp's
// decoders - an MSVC cross-function codegen leak). Offsets + code bytes are load-bearing.
#ifndef SRC_IMAGE_CFILEIMAGERECORDS_H
#define SRC_IMAGE_CFILEIMAGERECORDS_H

#include <Ints.h>

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

// The on-stack 14-byte BITMAPFILEHEADER SaveBmp builds (the 2-byte "BM" magic is
// strcpy'd from g_bmpHeaderTemplate, then bfSize / bfOffBits).
struct BmpFileHeader {
    char magic[2]; // +0x00  "BM"
    char _02[0x06 - 0x02];
    u32 bfSize;    // +0x06  (width*0x28 + 0x436)
    u32 _0a;       // +0x0a
    u32 bfOffBits; // +0x0e -> stored at struct +0x0e... (0x436)
};

// The on-stack 0x2c-byte header the 24-bit export (SaveTga) writes (a TGA-ish record):
// the "BM"-style magic strcpy'd from g_bmpHeaderTemplate at +0, the width/height + a +0x06 size
// (width*height*3 + 0x3a), and the +0x10/+0x12 plane/bitcount words.
struct TgaHeader {
    char magic[2]; // +0x00
    char _02[0x06 - 0x02];
    u32 size; // +0x06  (width*height*3 + 0x3a)
    char _0a[0x10 - 0x0a];
    i16 planes;   // +0x10
    i16 bitCount; // +0x12
};

// The locked RT_BITMAP resource header CDDSurface::Load (0x144270) validates:
// payload size at +0 (data follows at +m_0+0x400), a width word at +4, the
// slot-2-init word at +8, and the +0xe format tag (must be 8).
SIZE_UNKNOWN(RtBitmapResHeader); // resource-header view (only the first 0x10 bytes pinned)
struct RtBitmapResHeader {
    i32 m_0; // +0x00 (payload size; data follows at +m_0+0x400)
    i32 m_4; // +0x04 (stored into the surface desc width)
    i32 m_8; // +0x08 (forwarded to the slot-2 init)
    char m_padc[0xe - 0xc];
    i16 m_e; // +0x0e (must be 8)
};

#endif // SRC_IMAGE_CFILEIMAGERECORDS_H
