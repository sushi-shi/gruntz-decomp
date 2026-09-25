#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

struct Coord {
    Coord() {}

    Coord(i32 x, i32 y) : m_x(x), m_y(y) {}

    i32 m_x;
    i32 m_y;

    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }
};

#define SCREEN_TILE_INPLACE(pos)                                                                   \
    (pos)->m_x >>= TILE_SHIFT_PX;                                                                  \
    (pos)->m_y >>= TILE_SHIFT_PX

#define COORD_EQUALS_COMPONENTS(coord, x, y) ((coord).m_x == (x) && (coord).m_y == (y))
#define SET_VECTOR2_COMPONENTS(coord, x, y)                                                        \
    (coord).m_x = (x);                                                                             \
    (coord).m_y = (y)

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
