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
#include <Mfc.h> // RECT (m_area/m_placed/m_28)

class CAmbientPosSound; // the live voice (aux->m_voice) - the REAL 0x48-byte channel
                        // the spawn factory (0xb960 = CWorldSoundSet::CreatePos5_b960)
                        // returns; PosSoundVoice was its view, now folded.
struct PosSoundAux;

// The aux sub-object at PosSoundObj+0x7c: the init/action handler, the request state,
// the emit src-clip and the live voice slot.
// PROVEN IDENTITY: this IS the canonical +0x7c worker AnimWorkerObj (0x17c bytes; the
// same slot Demo.cpp's anim handlers read as owner->m_7c). DEFERRED-FOLD: folding onto
// <Gruntz/AnimWorker.h>'s AnimWorkerObj needs its m_10/m_1c/m_2c..m_38/m_168 named as
// m_handler/m_requestState/m_srcL..m_srcB/m_voice (a shared-header rename), deferred.
struct PosSoundAux {
    char m_pad00[0x10];
    void* m_handler; // +0x10  the object's init/action handler (vs the default at 0x402d15)
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

// The CGameObject the request rides on (only the touched offsets).
// PROVEN IDENTITY: this IS the canonical CGameObject (<Gruntz/UserLogic.h>) - the object
// _CreateAmbientSound / _CreateSpotAmbientSound (gameobjectfactory) build and pass here;
// m_aux@+0x7c == CGameObject::m_7c, m_120@+0x120, m_layer, m_x/m_y all line up.
// DEFERRED-FOLD (UserLogic.h is a hot/owned header - read-only this wave) + one offset
// to reconcile: this view puts m_layer@+0x19c while CGameObject::m_layer is @+0x198
// (CGameObject::m_19c is a separate "WwdFile stamp" word) - the layer read here is
// `*reinterpret_cast<void**>(m_layer+0x10)`, so either this +0x19c is off-by-4 or CGameObject's +0x198
// vs +0x19c pair needs the owner's attention. Flagged for the CGameObject owner.
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
    i32 m_extentL; // +0x134  per-side emit extents (L/T/R/B)
    i32 m_extentT; // +0x138
    i32 m_extentR; // +0x13c
    i32 m_extentB; // +0x140
    RECT m_area;   // +0x144  emit source area (CopyRect base)
    RECT m_placed; // +0x154  placed rect written back on emit
    char m_pad164[0x19c - 0x164];
    void* m_layer; // +0x19c  layer/desc (its +0x10 feeds the factory) - see the offset note
};

// The create-helper return record: the factory (WorldSoundCreateFull == CreateRandom
// @0xbb60 / WorldSoundCreateSimple == CreateAmbient5 @0xb7b0) returns a CAmbientSound-
// family channel; CommitSpriteAction writes the placed RECT into its +0x28 box.
// PROVEN IDENTITY: this IS the returned channel (CRandomAmbientSound / CAmbientSound);
// its +0x28 record IS CAmbientSound::m_box2 (an AmbientBox == a 4-int RECT). DEFERRED-
// FOLD: folding needs the +0x28 box viewed as a RECT (an AmbientBox/RECT union on the
// canonical CAmbientSound, a cross-header edit), deferred.
struct PosSoundPlaced {
    char m_pad0[0x28];
    RECT m_28; // +0x28  == CAmbientSound::m_box2 written as the placement rect
};

#endif // GRUNTZ_GRUNTZ_POSSOUND_H
