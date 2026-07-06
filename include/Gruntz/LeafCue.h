// LeafCue.h - LeafCue, a sound-cue leaf (throttled ConfigureItem forward). Promoted
// from DDrawSubMgrLeafScan.cpp so the reduced per-TU sound-entry views fold onto it.
#ifndef GRUNTZ_GRUNTZ_LEAFCUE_H
#define GRUNTZ_GRUNTZ_LEAFCUE_H

#include <Ints.h>
#include <rva.h>

class CSoundCueMgr;

SIZE_UNKNOWN(LeafCue);
struct LeafCue {
    i32 PlayIfElapsed_01f940(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1f940 (ret 0x10)

    char m_pad0[0x10];
    CSoundCueMgr* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;           // +0x14  last draw-clock
    i32 m_18;           // +0x18  interval
};

#endif // GRUNTZ_GRUNTZ_LEAFCUE_H
