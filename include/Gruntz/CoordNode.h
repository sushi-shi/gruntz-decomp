#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    Coord* Set(i32 x, i32 y);
};
SIZE(0x8);

struct CoordNode {
    CoordNode* m_next;
    char m_pad04[0x04];
    Coord* m_coord;
};
SIZE(0xc);

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
