#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    i32 operator==(const Coord& other) const {
        return m_x == other.m_x && m_y == other.m_y;
    }

    i32 operator!=(const Coord& other) const {
        return !(*this == other);
    }

    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }
};

#define UNSET_COORD(dst)                                                                           \
    {                                                                                              \
        Coord none;                                                                                \
        (dst) = *none.Set(-1, -1);                                                                 \
    }

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
