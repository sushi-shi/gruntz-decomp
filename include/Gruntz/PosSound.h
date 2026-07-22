// PosSound.h - the positional-sound spawn-path types the WorldSoundSet TU's free
// functions (SpawnPosSound / StopPosSound / CommitSpriteAction @0xca00/0xc9d0/0xc840,
// re-homed from ApiCallers.cpp) drive. These are the ambient-sound GAME OBJECT the
// gameobjectfactory unit builds: xref proves the callers are `_CreateSpotAmbientSound`
// (-> SpawnPosSound) and `_CreateAmbientSound` (-> CommitSpriteAction). Field names
// are placeholders; only offsets + emitted code bytes are load-bearing.
//
// Each carries a PROVEN identity whose canonical fold is DEFERRED (hot/shared header
// or an unresolved offset), documented per-type below - NOT a bare @identity-TODO.
#ifndef GRUNTZ_GRUNTZ_POSSOUND_H
#define GRUNTZ_GRUNTZ_POSSOUND_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // RECT (m_area/m_placed/m_28)

class CAmbientPosSound; // the live voice (aux->m_voice) - the REAL 0x48-byte channel
struct PosSoundAux;

struct PosSoundAux {
    char m_pad00[0x10];
    void (*m_handler)(); // +0x10  the object's init/action handler fn
                         //        (vs the DefaultActionHandler_2d15 label)
    char m_pad14[0x1c - 0x14];
    i32 m_requestState; // +0x1c  request state (0 spawn / 0x1e stop / 5 spawned)
    char m_pad20[0x2c - 0x20];
    i32 m_srcL; // +0x2c  emit src clip: left
    i32 m_srcR; // +0x30                   right
    i32 m_srcT; // +0x34                   top
    i32 m_srcB; // +0x38                   bottom
    char m_pad3c[0x168 - 0x3c];
    CAmbientPosSound* m_voice; // +0x168  the live voice (was the PosSoundVoice view)
};
SIZE_UNKNOWN();

struct PosSoundObj {
    char m_pad00[0x08];
    i32 m_flags08; // +0x08  flags
    char m_pad0c[0x40 - 0xc];
    i32 m_flags40; // +0x40  flags
    char m_pad44[0x5c - 0x44];
    i32 m_x; // +0x5c  x
    i32 m_y; // +0x60  y
    char m_pad64[0x7c - 0x64];
    PosSoundAux* m_aux; // +0x7c  the aux
    char m_pad80[0x120 - 0x80];
    i32 m_120; // +0x120
    char m_pad124[0x134 - 0x124];
    RECT m_extent; // +0x134  per-side emit extents (the same REAL RECT as CUserLogic's)
    RECT m_area;   // +0x144  emit source area (CopyRect base)
    RECT m_placed; // +0x154  placed rect written back on emit
    char m_pad164[0x19c - 0x164];
    struct LeafCue* m_layer; // +0x19c  the resolved leaf cue (its m_10 clone-inst
                             //        feeds the spawn factory)
};
SIZE_UNKNOWN();

struct PosSoundPlaced {
    char m_pad0[0x28];
    RECT m_28; // +0x28  == CAmbientSound::m_box2 written as the placement rect
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_POSSOUND_H
