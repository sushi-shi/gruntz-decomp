#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }
};

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
