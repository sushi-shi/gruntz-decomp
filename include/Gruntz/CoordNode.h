#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

SIZE(Coord, 0x8);
struct Coord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
    // 0x75a10 - fill both coords, return this (body in TriggerMgrHitTest.cpp; the
    // ex-CPairXY/CTrigPoint views' Set - three names, one {x,y} pair, folded here).
    Coord* Set(i32 x, i32 y);
};

SIZE(CoordNode, 0xc);
struct CoordNode {
    CoordNode* m_next;  // +0x00  intrusive list/free link
    char m_pad04[0x04]; // +0x04  unused
    Coord* m_coord;     // +0x08  {x,y} payload
};

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
