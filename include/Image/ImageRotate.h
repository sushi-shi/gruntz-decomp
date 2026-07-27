// ImageRotate.h - the ImageRotate.cpp TU's exported globals/functions.
#ifndef GRUNTZ_IMAGE_IMAGEROTATE_H
#define GRUNTZ_IMAGE_IMAGEROTATE_H

#include <Ints.h>

class CDDSurface; // the destination surface (all three retail callers pass `this`)

void ImageRotateBlit(
    i32 a1,
    i32 a2,
    i32* pivot,
    CDDSurface* dst,
    CDDSurface* in,
    float rot,   // arg6 (deg->rad rotation)
    float scale, // arg7
    i32 mode,    // arg8
    i32 colorkey // arg9
);

#endif // GRUNTZ_IMAGE_IMAGEROTATE_H
