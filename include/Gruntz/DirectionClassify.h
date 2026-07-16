// DirectionClassify.h - the 8-direction descriptor record (owner:
// DirectionClassify.cpp; the g_dirDescTable element).
#ifndef GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
#define GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H

#include <rva.h>

// One 0x10-byte direction descriptor (9 records at 0x6448c8; entry [8] is the
// null/no-heading result). Returned by pointer; content is the direction's
// anim/geo data (opaque here).
struct DirDesc {
    char b[0x10];
};
SIZE_UNKNOWN(DirDesc);

#endif // GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
