#ifndef GRUNTZ_SLOTHOLDER_H
#define GRUNTZ_SLOTHOLDER_H

#include <rva.h>
#include <Ints.h>

struct CSlotHolder {
    i32 DoSwap(); // 0x1128b0
    char m_pad0[0x08];
    i32 m_08; // +0x08  group
    i32 m_0c; // +0x0c  index
    char m_pad10[0x34 - 0x10];
    i32 m_34; // +0x34  token
};
SIZE_UNKNOWN(CSlotHolder);

#endif // GRUNTZ_SLOTHOLDER_H
