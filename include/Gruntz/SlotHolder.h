#ifndef GRUNTZ_SLOTHOLDER_H
#define GRUNTZ_SLOTHOLDER_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Ints.h>

struct CSlotHolder {
    i32 DoSwap();
    char m_pad0[0x08];
    Coord m_tile;
    char m_pad10[0x34 - 0x10];
    i32 m_tileToken;
};

#endif // GRUNTZ_SLOTHOLDER_H
