#ifndef GRUNTZ_CRANDOMAMBIENTSOUND_H
#define GRUNTZ_CRANDOMAMBIENTSOUND_H

#include <Gruntz/AmbientSound.h>
#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)

extern "C" i32 g_frameDelta;

extern "C" i32 winapi_00cd00_timeGetTime();

extern "C" i32 __ftol(double v);

struct AmbientPoint {
    i32 x; // +0x00
    i32 y; // +0x04
};
SIZE_UNKNOWN();

struct AmbSoundMapHolder {
    char m_pad00[0x10];
    CMapStringToPtr m_map; // +0x10  the key -> AmbSoundRecord* table
};
SIZE_UNKNOWN();
struct AmbSoundRecord {
    char m_pad00[0x10];
    DirectSoundMgr* m_mgr; // +0x10
};
SIZE_UNKNOWN();

class CRandomAmbientSound : public CAmbientSound {
public:
    // Inline: the Create* factories inline the vptr stamp (??_7CRandomAmbientSound)
    // directly (no OOL ctor call), so this must be inline to collapse into them.
    CRandomAmbientSound() {}
    // Setup(world, a2, a3, box, a5): seed the mgr handle + play params, copy/clear
    // the primary box, clear the secondary box. Returns 1 (0 on a null world).
    i32 Setup(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientBox* box, i32 a5); // 0x00be50
    // 0xbdd0: resolve a mgr record for `key` from a1's embedded CMapStringToOb (+0x10),
    // then Setup(record->m_10, a3, a4, box, a6). Null return on a miss. (This is the
    // real home of the old CObj_bdd0::Dispatch placeholder - `this` is a CRandomAmbientSound,
    // proven by the tail-call into Setup @0xbe50.)
    void*
    Dispatch(AmbSoundMapHolder* a1, const char* key, i32 a3, i32 a4, AmbientBox* box, i32 a6);
    // Update(playFlag, pos, kind): start or stop the sound this frame. 0x00c2a0.
    // 0x00c2a0 is NOT the slot-3 virtual (no retail vtable holds it) - a plain
    // random-cue driver, self-called qualified from the Update override.
    void PlayRandom(i32 playFlag, i32 pos, i32 kind); // 0x00c2a0
    virtual void Update(i32 x, i32 y, i32 force) OVERRIDE; // slot 3 - 0xcb30 (ex Step)
    // The positional variant's entry points (same CAmbientSound base layout, but
    // m_40/m_44 hold an anchor position instead of the interval roller):
    void
    SetupFromMap(
        AmbSoundMapHolder* holder,
        const char* key,
        i32 a3,
        i32 a4,
        AmbientPoint* pos,
        i32 a5
    );
    i32 SetupPos(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5);
    void StopPos(i32 obj);                  // 0x00c9d0  request stop via slot 2
    i32 TickObj(i32 obj);                   // 0x00ca00  per-object placement tick

    // Step(x, y, force): the per-frame tick (vtable slot 3). 0x00cb30.
    // Inline leaf dtor: inlines the (inline) base ~CAmbientSound, collapsing to the
    // same bytes as ~CAmbientSound/~CAmbientPosSound (stamp ??_7CUserBase, clear
    // m_voice/m_listNode). The OOL COMDAT is at 0xbb40 (Ghidra-mislabeled as the ??0
    // ctor; it is really ??1, called by the scalar-deleting-dtor 0xbb10 = vtable slot
    // 0) and is pinned by RVA_COMPGEN in WorldSoundSet.cpp.
    virtual ~CRandomAmbientSound() OVERRIDE {}
    // Init2(lo, hi, lo2, hi2): the interval-roller seed run by the box factory
    // (CWorldSoundSet::CreateRandomBox; unreconstructed, reloc-masked).
    void Init2(i32 a0, i32 a1, i32 a2, i32 a3);

    // --- layout (sizeof 0x58) -------------------------------------------------
    // +0x00..+0x3f come from the CAmbientSound base (vptr, m_voice, m_level,
    // m_scaleA/m_scaleB, m_isPlaying, m_box1/m_box2, m_panIndex, m_listNode).
    // (An earlier revision re-declared the base fields here, shadowing them at
    // +0x40.. - a proven layout bug: retail Setup/Update write +0x04..+0x38.)
    //
    // +0x40/+0x44 are a union: the random path (Setup/Step) uses them as the
    // phase-A interval bounds [lo,hi]; the positional path (SetupPos/UpdateAt) uses
    // them as the (x,y) anchor. Left as raw offsets since the role forks by instance.
    i32 m_40;          // +0x40  interval-roller phase-A lo  / anchor x
    i32 m_44;          // +0x44  interval-roller phase-A hi  / anchor y
    i32 m_intervalLoB; // +0x48  interval-roller phase-B lo (roller-only)
    i32 m_intervalHiB; // +0x4c  interval-roller phase-B hi (roller-only)
    i32 m_countdownMs; // +0x50  rolled countdown (ms, drained by g_frameDelta)
    i32 m_phase;       // +0x54  roller phase flag (toggled each reroll)
};
SIZE(0x58);

#endif // GRUNTZ_CRANDOMAMBIENTSOUND_H
