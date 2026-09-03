#ifndef GRUNTZ_GRUNTZ_SBGEOM_H
#define GRUNTZ_GRUNTZ_SBGEOM_H

#include <Mfc.h>

#include <Ints.h>
#include <MakeRect.h>

inline RECT SbGeom(i32 l, i32 t, i32 r, i32 b) {
    return MakeRect(l, t, r, b);
}

#endif // GRUNTZ_GRUNTZ_SBGEOM_H
