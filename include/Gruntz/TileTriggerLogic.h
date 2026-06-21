// TileTriggerLogic.h - Gruntz tile-trigger logic class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the constructor. Offsets are the
// load-bearing fact the match proves.
#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

// ---------------------------------------------------------------------------
// CTileTriggerLogic
//   vftable. size 0x9c. ctor:
//     mov edx,this; vptr@0; rep stosl zeroes 24 dwords (96 bytes) starting at
//     +0x3c (m_block); then m_1c (+0x1c) = 0 (reusing the zero in eax, emitted
//     AFTER the rep stosl -> the m_block array is initialised before m_1c).
// ---------------------------------------------------------------------------
class CTileTriggerLogic {
public:
    CTileTriggerLogic();
    virtual ~CTileTriggerLogic();
    virtual int TileLogicVfunc0();

    int m_pad4[(0x1c - 0x04) / 4]; // +0x04..0x1b
    int m_1c;                      // +0x1c  zeroed AFTER m_block
    char m_pad20[0x3c - 0x20];     // +0x20..0x3b
    int m_block[24];               // +0x3c..0x9b  (rep stosl, 24 dwords)
};

#endif // TILETRIGGERLOGIC_H
