// TileTriggerLogic.h - Gruntz tile-trigger logic class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the constructor. Offsets are the
// load-bearing fact the match proves.
#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CTileTriggerLogic
//   vftable (0x5eaea4, ONE slot). size 0x9c. ctor:
//     mov edx,this; vptr@0; rep stosl zeroes 24 dwords (96 bytes) starting at
//     +0x3c (m_block); then m_1c (+0x1c) = 0 (reusing the zero in eax, emitted
//     AFTER the rep stosl -> the m_block array is initialised before m_1c).
//
// The retail vtable at 0x5eaea4 holds exactly ONE slot (0x402072 -> the regular
// virtual at 0x110c10); the derived logic classes (CGiantRockLogic, ...) share
// that same slot value -> it is an INHERITED regular virtual, not a per-class
// destructor.  So CTileTriggerLogic is modeled with a single non-dtor virtual and
// NO virtual destructor (the earlier ??_G@0x116610 label was a misattribution: that
// function ends `ret 0x84`, a 33-arg engine fn, not a scalar-deleting dtor).
// ---------------------------------------------------------------------------
SIZE(CTileTriggerLogic, 0x9c);
class CTileTriggerLogic {
public:
    CTileTriggerLogic();
    virtual i32 TileLogicVfunc0(); // slot 0 (0x110c10 via ILT thunk 0x402072)

    i32 m_pad4[(0x1c - 0x04) / 4]; // +0x04..0x1b
    i32 m_1c;                      // +0x1c  zeroed AFTER m_block
    char m_pad20[0x3c - 0x20];     // +0x20..0x3b
    i32 m_block[24];               // +0x3c..0x9b  (rep stosl, 24 dwords)
};

#endif // TILETRIGGERLOGIC_H
