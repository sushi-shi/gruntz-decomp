#ifndef GRUNTZ_GRUNTZ_FADERBUFFERINLINE_H
#define GRUNTZ_GRUNTZ_FADERBUFFERINLINE_H

#include <Ints.h>

#include <string.h>

static inline void CopyBytes(u8* dst, const u8* src, i32 count) {
    while (count-- > 0) {
        *dst++ = *src++;
    }
}

static inline void ClearSample(u8* row, i32 sample, i32 bpp) {
    if (bpp > 0) {
        memset(row + sample * bpp, 0, bpp);
    }
}

#endif // GRUNTZ_GRUNTZ_FADERBUFFERINLINE_H
