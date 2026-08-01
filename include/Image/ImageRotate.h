#ifndef GRUNTZ_IMAGE_IMAGEROTATE_H
#define GRUNTZ_IMAGE_IMAGEROTATE_H

#include <Ints.h>

class CDDSurface;

void ImageRotateBlit(
    i32 destX,
    i32 destY,
    i32* pivot,
    CDDSurface* dst,
    CDDSurface* src,
    float rot,
    float scale,
    i32 mode,
    i32 colorkey
);

#endif // GRUNTZ_IMAGE_IMAGEROTATE_H
