#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <Ints.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }
};

#define UNSET_COORD(dst)                                                                               {                                                                                                      Coord none;                                                                                        (dst) = *none.Set(-1, -1);                                                                     }

#endif
