#ifndef GRUNTZ_MAKERECT_H
#define GRUNTZ_MAKERECT_H

#include <Win32.h>

#include <Ints.h>

// A RECT built from its four edges and returned by value. Retail expands this at
// every use; it was transcribed per site in DDSurface.cpp and Grunt.cpp.
static __inline RECT MakeRect(i32 l, i32 t, i32 r, i32 b) {
    RECT rc;
    rc.left = l;
    rc.top = t;
    rc.right = r;
    rc.bottom = b;
    return rc;
}

#endif // GRUNTZ_MAKERECT_H
