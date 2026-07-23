#ifndef GRUNTZ_CAMBIENTSOUND_H
#define GRUNTZ_CAMBIENTSOUND_H

#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/UserLogic.h> // CUserBase base (CAmbientSound : CUserBase)
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)

typedef enum AmbientBoxSentinel {
    AMBIENT_BOX_UNBOUNDED = 0x80000000, // INT_MIN sentinel (same immediate)
} AmbientBoxSentinel;

struct AmbientPoint {
    i32 x; // +0x00
    i32 y; // +0x04
};
SIZE_UNKNOWN();

struct CRandomAmbientWorld; // <Gruntz/WorldSoundSet.h> (m_map @+0x10, the Init6 cue lookup)

class CAmbientSound : public CUserBase {
public:
    // Inline ctor: the CWorldSoundSet::Create* factories use `new CAmbientSound`
    // (operator new == RezAlloc @0x1b9b46), which inlines the vptr stamp + these
    // four seed stores directly. Being inline is what lets that fold happen.
    CAmbientSound() {
        m_voice = 0;
        m_level = 0x64;
        m_isPlaying = 0;
        m_listNode = 0;
    }

    // Inline leaf dtor: clears m_voice/m_listNode then folds the inline ~CUserBase
    // (final ??_7CUserBase vptr store). Being inline lets ~CAmbientPosSound inline it
    // (collapsing to byte-identical bytes) instead of tail-jmp'ing. The out-of-line
    // COMDAT copy (0xb790) is pinned by RVA_COMPGEN in WorldSoundSet.cpp.
    virtual ~CAmbientSound() OVERRIDE {
        m_voice = 0;
        m_listNode = 0;
    }

    // The non-virtual level setter (0xc200): scale `value` through m_0c/m_10,
    // clamp to 0..100, then drive the voice (mode 0 -> SetVolumeByIndex, else
    // CloneAndPlay with the extra arg).
    i32 SetLevel(i32 value, i32 mode, i32 extra); // 0xc200

    // The per-frame update virtual (CAmbientSound vtable slot 3 = 0xc090): keeps
    // the voice running/paused based on whether (x,y) sits inside either audible
    // box, (re)starting or fading as the listener crosses in/out.
    virtual void Update(i32 x, i32 y, i32 force); // 0xc090  (slot 3)

    // The non-virtual play/stop helper shared by the base and random update
    // paths. It owns only CAmbientSound state.
    void Fade(i32 playFlag, i32 level, i32 mode); // 0xc2a0

    // 0xbfb0: Restart - re-arm the voice at its CURRENT level (m_08). No-op unless
    // the voice exists, it is not already playing (m_14==0), and the active level /
    // world is live. Inlines SetLevel(m_08, 0, 0)'s scale+clamp then SetVolumeByIndex.
    void Restart(); // 0xbfb0

    // 0xbf10: per-channel volume recompute (CWorldSoundSet::Restart pushes the new
    // master level to every live channel). Caches the master in m_scaleA, then runs
    // the SetLevel scale/clamp and drives SetVolumeByIndex. (Was the dissolved
    // CSoundChannel view's "Recompute(frame)".)
    void Recompute(i32 master); // 0xbf10

    // One-time inits run by the CWorldSoundSet::Create* factories right after
    // construction. Init6 (0xbdd0, ex "Dispatch") resolves the mgr record for
    // `key` from the world's cue map, then seeds via Init5 (0xbe50, ex "Setup"):
    // mgr handle + play params, copy/clear the primary box, clear the secondary.
    void* Init6(
        CRandomAmbientWorld* world,
        const char* key,
        i32 a3,
        i32 a4,
        RECT* box,
        i32 a6
    );                                                                 // 0xbdd0
    i32 Init5(DirectSoundMgr* mgr, i32 a2, i32 a3, RECT* box, i32 a5); // 0xbe50

    // +0x00  vptr provided by CUserBase base
    DirectSoundMgr* m_voice; // +0x04  the sound-mgr voice handle it drives
    i32 m_level;             // +0x08  current level (0..100), scaling base
    i32 m_scaleA;            // +0x0c  level scale A (compared to 5; -0xf above)
    i32 m_scaleB;            // +0x10  level scale B (>0 multiplier, percent)
    i32 m_isPlaying;         // +0x14  "is playing" flag
    RECT m_box1;             // +0x18  primary audible box
    RECT m_box2;             // +0x28  secondary audible box
    i32 m_panIndex;          // +0x38  pan index (ApplyAndPlay pan arg; matches CRandomAmbientSound)
    POSITION m_listNode;     // +0x3c  the CWorldSoundSet::m_list POSITION this channel was
                             //        appended at (Create* factories store AddTail's return;
                             //        Teardown feeds it to m_list.RemoveAt); zeroed in the dtor
};
SIZE(0x40);

class CAmbientPosSound : public CAmbientSound {
public:
    // Inline: the CWorldSoundSet::CreatePos* factories inline the vptr stamp
    // (??_7CAmbientPosSound) directly (no OOL ctor call), so this must be inline.
    CAmbientPosSound() {}

    // Inline leaf dtor: inlines the (now inline) base ~CAmbientSound, collapsing to
    // the same bytes (stamp ??_7CUserBase, clear m_voice/m_listNode). Its own OOL
    // COMDAT (0xb940) is pinned by RVA_COMPGEN in WorldSoundSet.cpp. m_40/m_44 are
    // plain ints (no cleanup).
    virtual ~CAmbientPosSound() OVERRIDE {} // slot 0
    virtual void Update(i32 x, i32 y, i32 force)
        OVERRIDE; // slot 3 - 0xc5b0 (ex CRandomAmbientSound::UpdateAt; position-driven volume/pan)

    // The positional one-time inits (name-hide the base AmbientBox pair): Init6
    // (0xc4b0, ex "SetupFromMap") resolves the mgr record for `key` then seeds via
    // Init5 (0xc530, ex "SetupPos"): play params + the (x,y) anchor into m_40/m_44.
    // Init6's miss path returns Lookup's 0 (the factory tests it - byte-proven).
    i32 Init6(
        CRandomAmbientWorld* world,
        const char* key,
        i32 a3,
        i32 a4,
        AmbientPoint* pos,
        i32 a5
    );                                                                         // 0xc4b0
    i32 Init5(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5); // 0xc530

    i32 m_40; // +0x40  anchor position x (seeded by Init5)
    i32 m_44; // +0x44  anchor position y
};
SIZE(0x48);

#endif // GRUNTZ_CAMBIENTSOUND_H
