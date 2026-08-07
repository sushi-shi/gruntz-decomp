#ifndef IMAGE_PCXFORMAT_H
#define IMAGE_PCXFORMAT_H

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_BEGIN_SPLIT(PcxPlaneCount, i8)
    PCX_PLANES_PALETTED = 1,
    PCX_PLANES_RGB = 3
GZ_ENUM_END_SPLIT(PcxPlaneCount, i8)

GZ_ENUM_BEGIN_SPLIT(PcxBitsPerPlane, u8)
    PCX_BITS_PER_PLANE_8 = 8
GZ_ENUM_END_SPLIT(PcxBitsPerPlane, u8)

GZ_ENUM_CONST_BEGIN(PcxFormatConstants)
    PCX_HEADER_SIZE = 0x80
GZ_ENUM_CONST_END(PcxFormatConstants)

#endif // IMAGE_PCXFORMAT_H
