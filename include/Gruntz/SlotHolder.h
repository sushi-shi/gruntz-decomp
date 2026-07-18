// SlotHolder.h - CSlotHolder, a small game object holding a token at +0x34 plus the
// (group m_08, index m_0c) plane coordinates.
//
// DoSwap (0x1128b0) swaps the token parked in the registry's plane table with its
// own, notifies the registry's tile sub-manager, and adopts the previously-parked
// token. Defined in src/Gruntz/MgrSlotSwap.cpp.
//
// Names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_SLOTHOLDER_H
#define GRUNTZ_SLOTHOLDER_H

#include <rva.h>
#include <Ints.h>

// The owning object: group/index coordinates + the parked token.
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
