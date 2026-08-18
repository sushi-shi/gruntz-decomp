#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    // TmDeflectStep is the only retail caller of the out-of-line COMDAT.
    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }
};

struct CoordNode {
    CoordNode* m_next;
    CoordNode* m_prev;
    Coord* m_coord;
};

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
