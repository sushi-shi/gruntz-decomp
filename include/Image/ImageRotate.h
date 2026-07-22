// ImageRotate.h - the ImageRotate.cpp TU's exported globals/functions.
#ifndef GRUNTZ_IMAGE_IMAGEROTATE_H
#define GRUNTZ_IMAGE_IMAGEROTATE_H

#include <Ints.h>

void ImageRotateBlit(
    i32 a1,
    i32 a2,
    i32* pivot,
    void* dst,
    void* in,
    float rot,   // arg6 (deg->rad rotation)
    float scale, // arg7
    i32 mode,    // arg8
    i32 colorkey // arg9
);

#endif // GRUNTZ_IMAGE_IMAGEROTATE_H
