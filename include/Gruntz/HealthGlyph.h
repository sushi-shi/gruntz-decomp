#ifndef GRUNTZ_GRUNTZ_HEALTHGLYPH_H
#define GRUNTZ_GRUNTZ_HEALTHGLYPH_H

#include <Ints.h>

inline i32 HealthGlyphIndex(i32 hp) {
    if (hp >= 0x50) {
        return 0x24;
    }
    if (hp >= 0x28) {
        return 0x25;
    }
    return (hp <= 0 ? 1 : 0) + 0x26;
}

#endif // GRUNTZ_GRUNTZ_HEALTHGLYPH_H
