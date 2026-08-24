#ifndef IMAGE_REZDECODEKIND_H
#define IMAGE_REZDECODEKIND_H

#include <Enums.h>

// Which decoder CRezImage::DispatchDecode runs over an in-memory buffer.
//
// Taken from the jump table in retail, NOT from the arm order in our source -
// the two disagreed, and the table is what shipped:
//
//   case 2 -> 0x175e00  DecodeBmpData   (reads a BITMAPINFOHEADER)
//   case 3 -> 0x176000  DecodePcxData
//   case 4 -> 0x1762c0  DecodeRidData
//   case 5 -> 0x176440  DecodePidData
//
// The save-game preview corroborates DECODE_BMP independently. It hands
// LoadSurfaceFromData `&readBuf[0xe]`, i.e. the buffer past a 14-byte
// BITMAPFILEHEADER, which is exactly the DIB pointer DecodeBmpData expects; and
// the block it reads is 0x3843a bytes, which is 320 * 240 * 3 pixels plus the
// 40-byte header, its 4-byte tail and that same 14-byte file header.
//
// The four kinds are the four REZ image formats in tag order, one higher than
// FileImageFormat numbers the same four (FMT_BMP = 1 .. FMT_PID = 4).
GZ_ENUM_BEGIN(RezDecodeKind)
    DECODE_BMP = 2,
    DECODE_PCX = 3,
    DECODE_RID = 4,
    DECODE_PID = 5
GZ_ENUM_END(RezDecodeKind)

#endif // IMAGE_REZDECODEKIND_H
