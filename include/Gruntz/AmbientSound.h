// AmbientSound.h - the engine's positional ambient-sound game object
// (C:\Proj\Gruntz). RTTI: CAmbientSound : CUserBase, sizeof 0x40, vftable
// 0x5e710c (CUserBase's is 0x5e70b4). It owns a DirectSoundMgr voice handle
// (+0x04), a level/scale (+0x08/+0x0c/+0x10), an "is playing" flag (+0x14), TWO
// audible boxes (+0x18 / +0x28, sentinel left==0x80000000 == "no box / always
// in range"), and a sound id (+0x38). It is the non-random sibling of
// CRandomAmbientSound (see include/Gruntz/RandomAmbientSound.h); the field
// layout, the DirectSoundMgr voice, the box sentinel, and the signed-/100 level
// scale are shared between the two.
//
// Some field roles past +0x14 stay placeholders (m_<hexoffset>); OFFSETS + the
// emitted code bytes are the load-bearing facts the matches prove. Only the parts
// the reconstructed methods touch are modeled; every cross-class callee is
// declared NO-body so its `call`/`mov ds:` reloc-masks.
#ifndef GRUNTZ_CAMBIENTSOUND_H
#define GRUNTZ_CAMBIENTSOUND_H

#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/UserLogic.h> // CUserBase base (CAmbientSound : CUserBase)
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>

// left == AMBIENT_BOX_UNBOUNDED in either box means "no box / always in range":
// the listener test for that box is skipped and the source is treated as audible
// everywhere (the ctors seed the boxes with it).
#define AMBIENT_BOX_UNBOUNDED ((i32)0x80000000)

// An axis-aligned region the listener (x,y) must be inside for the sound to play.
// left == AMBIENT_BOX_UNBOUNDED is the "no box / always pass" sentinel. Same shape
// as CRandomAmbientSound::AmbientBox.
SIZE_UNKNOWN(AmbientBox);
struct AmbientBox {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};

// The (re)start path arms the voice through DirectSoundMgr::ApplyAndPlay (0x136300,
// declared in <Dsndmgr/DirectSoundMgr.h> as m_voice->ApplyAndPlay(vol,pan,freq,dur)).

// ---------------------------------------------------------------------------
// The big game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
// Update gates the (re)start on the active-level handle (+0x10) and the +0x54
// active-level object's armed/playable gate (m_inputState->m_armed; CInput54 in
// <Gruntz/InputState.h> - the same object CRandomAmbientSound taps).
// ---------------------------------------------------------------------------
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CAmbientSound : CUserBase (sizeof 0x40, vftable 0x5e710c). Real polymorphic:
// derives from CUserBase (its +0x00 vptr) and gives cl the implicit vptr stamp /
// base teardown. The retail ~CAmbientSound (0xb790, 15 B) stores ONLY the base
// vptr 0x5e70b4 - the derived-vptr store is DCE'd (the body has no virtual
// dispatch); cl reproduces the same shape from the real virtual dtor. Its own
// vftable (0x5e710c, 4 slots) is emitted by cl (VTBL below); the ctors that build
// it stay stubbed. The StreamFeeder device.
// ---------------------------------------------------------------------------
VTBL(CAmbientSound, 0x001e710c);
class CAmbientSound : public CUserBase {
public:
    virtual ~CAmbientSound() OVERRIDE;
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1

    // The non-virtual level setter (0xc200): scale `value` through m_0c/m_10,
    // clamp to 0..100, then drive the voice (mode 0 -> SetVolumeByIndex, else
    // CloneAndPlay with the extra arg).
    i32 SetLevel(i32 value, i32 mode, i32 extra); // 0xc200

    // The per-frame update virtual (CAmbientSound vtable slot 3 = 0xc090): keeps
    // the voice running/paused based on whether (x,y) sits inside either audible
    // box, (re)starting or fading as the listener crosses in/out.
    virtual void Update(i32 x, i32 y, i32 force); // 0xc090  (slot 3)

    // The fade helper (CAmbientSound::SetLevel sibling 0xc2a0; ret 0xc) reached
    // through the same ILT thunk Update uses. External here (not a target):
    // declared so Update's `call` reloc-masks.
    i32 Fade(i32 a, i32 b, i32 c); // 0xc2a0

    // 0xbfb0: Restart - re-arm the voice at its CURRENT level (m_08). No-op unless
    // the voice exists, it is not already playing (m_14==0), and the active level /
    // world is live. Inlines SetLevel(m_08, 0, 0)'s scale+clamp then SetVolumeByIndex.
    void Restart(); // 0xbfb0

    // +0x00  vptr provided by CUserBase base
    DirectSoundMgr* m_voice; // +0x04  the sound-mgr voice handle it drives
    i32 m_level;             // +0x08  current level (0..100), scaling base
    i32 m_scaleA;            // +0x0c  level scale A (compared to 5; -0xf above)
    i32 m_scaleB;            // +0x10  level scale B (>0 multiplier, percent)
    i32 m_isPlaying;         // +0x14  "is playing" flag
    AmbientBox m_box1;       // +0x18  primary audible box
    AmbientBox m_box2;       // +0x28  secondary audible box
    i32 m_panIndex;          // +0x38  pan index (ApplyAndPlay pan arg; matches CRandomAmbientSound)
    i32 m_3c;                // +0x3c  zero-init in dtor; role unproven
};
SIZE(CAmbientSound, 0x40);

#endif // GRUNTZ_CAMBIENTSOUND_H
