#ifndef GRUNTZ_GRUNTZ_SBGEOM_H
#define GRUNTZ_GRUNTZ_SBGEOM_H

#include <Mfc.h>

#include <Ints.h>

inline RECT SbGeom(i32 l, i32 t, i32 r, i32 b) {
    RECT x;
    x.left = l;
    x.top = t;
    x.right = r;
    x.bottom = b;
    return x;
}

#endif // GRUNTZ_GRUNTZ_SBGEOM_H
