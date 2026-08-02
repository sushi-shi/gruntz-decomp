#ifndef GRUNTZ_SLOTHOLDER_H
#define GRUNTZ_SLOTHOLDER_H

#include <rva.h>
#include <Ints.h>

struct CSlotHolder {
    i32 DoSwap();
    char m_pad0[0x08];
    i32 m_tileX;
    i32 m_tileY;
    char m_pad10[0x34 - 0x10];
    i32 m_tileToken;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_SLOTHOLDER_H
