// CAmbientSound.h - the engine's positional ambient-sound game object
// (C:\Proj\Gruntz). RTTI: CAmbientSound : CUserBase, sizeof 0x40, vftable
// 0x5e710c (CUserBase's is 0x5e70b4). It owns a DirectSoundMgr voice handle
// (+0x04), a level/scale (+0x08/+0x0c/+0x10), an "is playing" flag (+0x14), TWO
// audible boxes (+0x18 / +0x28, sentinel left==0x80000000 == "no box / always
// in range"), and a sound id (+0x38). It is the non-random sibling of
// CRandomAmbientSound (see include/Gruntz/CRandomAmbientSound.h); the field
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

// The mgr's frame-reseed helper at 0x136300 (FUN_00536300, __thiscall, 4 args) -
// the (re)start path runs it to arm the sound id before the first level push. Not
// a DirectSoundMgr method in its header, so reached through this one-method shell
// (same device as CRandomAmbientSound::DsndReseed); `mov ecx,m_04; call` falls out
// reloc-masked.
SIZE_UNKNOWN(DsndReseed);
struct DsndReseed {
    void Reseed(i32 a1, i32 a2, i32 a3, i32 a4); // 0x136300
};

// ---------------------------------------------------------------------------
// The big game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
// Update gates the (re)start on the active-level handle (+0x10) and the active
// world view's object count (+0x54->m_24). Only the two touched offsets are
// modeled; same singleton CRandomAmbientSound taps.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(WwdActiveLevel);
struct WwdActiveLevel {
    char m_pad0[0x24];
    i32 m_24; // +0x24  object count (non-zero == playable)
};
struct WwdGameReg {
    char m_pad0[0x10];
    i32 m_10; // +0x10  active-level handle (Update gates on it)
    char m_pad14[0x54 - 0x14];
    WwdActiveLevel* m_54; // +0x54  the active world view
};
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

// CUserBase's retail vftable (0x5e70b4) - restamped at the tail of the dtor as
// the base sub-object is torn down. Transitional reloc-masked DIR32 store while
// the family's vtables (0x5e710c..) aren't fully reproduced (the ctors that
// stamp them stay stubbed), so the class stays non-polymorphic here.
DATA(0x001e70b4)
extern void* const g_CUserBaseVtbl[];

// ---------------------------------------------------------------------------
// CAmbientSound : CUserBase (sizeof 0x40, vftable 0x5e710c). Modeled
// NON-polymorphic with a leading vptr field + a manual vtable stamp (the family
// vtables aren't reproduced yet, the ctors that build them stay stubbed): the
// dtor restamps the CUserBase base vptr inline and must NOT chain a `call
// ~CUserBase`, which a real `virtual` base would force. The StreamFeeder device.
// ---------------------------------------------------------------------------
class CAmbientSound {
public:
    ~CAmbientSound();

    // The non-virtual level setter (0xc200): scale `value` through m_0c/m_10,
    // clamp to 0..100, then drive the voice (mode 0 -> SetVolumeByIndex, else
    // CloneAndPlay with the extra arg).
    i32 SetLevel(i32 value, i32 mode, i32 extra); // 0xc200

    // The per-frame update virtual (CAmbientSound vtable slot 3 = 0xc090): keeps
    // the voice running/paused based on whether (x,y) sits inside either audible
    // box, (re)starting or fading as the listener crosses in/out.
    void Update(i32 x, i32 y, i32 force); // 0xc090

    // The fade helper (CAmbientSound::SetLevel sibling 0xc2a0; ret 0xc) reached
    // through the same ILT thunk Update uses. External here (not a target):
    // declared so Update's `call` reloc-masks.
    i32 Fade(i32 a, i32 b, i32 c); // 0xc2a0

    // 0xbfb0: Restart - re-arm the voice at its CURRENT level (m_08). No-op unless
    // the voice exists, it is not already playing (m_14==0), and the active level /
    // world is live. Inlines SetLevel(m_08, 0, 0)'s scale+clamp then SetVolumeByIndex.
    void Restart(); // 0xbfb0

    void* m_vptr;         // +0x00  vptr (manual stamp; CAmbientSound vtable 0x5e710c)
    DirectSoundMgr* m_04; // +0x04  the sound-mgr voice handle
    i32 m_08;             // +0x08  current level (0..100)
    i32 m_0c;             // +0x0c  level scale A (compared to 5; -0xf above)
    i32 m_10;             // +0x10  level scale B (>0 multiplier, percent)
    i32 m_14;             // +0x14  "is playing" flag
    AmbientBox m_box1;    // +0x18  primary audible box
    AmbientBox m_box2;    // +0x28  secondary audible box
    i32 m_38;             // +0x38  sound id
    i32 m_3c;             // +0x3c  zero-init in dtor; role unproven
};
SIZE(CAmbientSound, 0x40);

#endif // GRUNTZ_CAMBIENTSOUND_H
