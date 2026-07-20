#ifndef GRUNTZ_GRUNTZ_SBRECT_H
#define GRUNTZ_GRUNTZ_SBRECT_H

#include <Ints.h>
#include <rva.h>

struct SbRect {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
    SbRect() {}
    SbRect(i32 l, i32 t, i32 r, i32 b) : left(l), top(t), right(r), bottom(b) {}
};
SIZE_UNKNOWN(SbRect);

#endif // GRUNTZ_GRUNTZ_SBRECT_H
