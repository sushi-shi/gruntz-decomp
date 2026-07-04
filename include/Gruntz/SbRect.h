// SbRect.h - the status-bar geometry rectangle passed by value into the item
// "configure"/"setup" virtuals. Built as base + per-item offsets
// (left/top/right/bottom = origin.x + dx / origin.y + dy). The 4-arg ctor is the
// inline-temp construction the tabz/menu builders use (sub esp,0x10 + four
// stores per call); the empty default ctor keeps `SbRect r; r.left = ...;` sites
// zero-code. One shape shared across the status-bar builders.
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
