#ifndef GRUNTZ_RECTINTERPOLATION_H
#define GRUNTZ_RECTINTERPOLATION_H

#include <Mfc.h>
#include <MfcWin.h>

#include <Ints.h>

inline i32 InterpolateRectCoord(i32 first, i32 last, float amount) {
    return first + static_cast<i32>(static_cast<float>(last - first) * amount);
}

inline CRect InterpolateRect(const RECT& first, const RECT& last, float amount) {
    return CRect(
        InterpolateRectCoord(first.left, last.left, amount),
        InterpolateRectCoord(first.top, last.top, amount),
        InterpolateRectCoord(first.right, last.right, amount),
        InterpolateRectCoord(first.bottom, last.bottom, amount)
    );
}

#endif // GRUNTZ_RECTINTERPOLATION_H
