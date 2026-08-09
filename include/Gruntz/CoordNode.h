#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    // Inline: retail's only out-of-line emission is the COMDAT at 0x00075a10,
    // which cl left behind in TriggerMgrHitTest.cpp after its inline budget ran
    // out inside TmDeflectStep. That function is also its only caller.
    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return this;
    }
};
SIZE(0x8);

struct CoordNode {
    CoordNode* m_next;
    char m_pad04[0x04];
    Coord* m_coord;
};
SIZE(0xc);

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
