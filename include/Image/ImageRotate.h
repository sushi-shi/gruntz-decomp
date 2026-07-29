// ImageRotate.h - the ImageRotate.cpp TU's exported globals/functions.
#ifndef GRUNTZ_IMAGE_IMAGEROTATE_H
#define GRUNTZ_IMAGE_IMAGEROTATE_H

#include <Ints.h>

class CDDSurface; // <DDrawMgr/DDSurface.h> - both the dest and the source surface

void ImageRotateBlit(
    i32 destX,
    i32 destY,
    i32* pivot,
    CDDSurface* dst,
    CDDSurface* src,
    float rot,   // arg6 (deg->rad rotation)
    float scale, // arg7
    i32 mode,    // arg8
    i32 colorkey // arg9
);

#endif // GRUNTZ_IMAGE_IMAGEROTATE_H
