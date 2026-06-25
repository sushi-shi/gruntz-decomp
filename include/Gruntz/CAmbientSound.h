// CAmbientSound.h - the engine's positional ambient-sound game object
// (C:\Proj\Gruntz). RTTI: CAmbientSound : CUserBase, sizeof 0x40, vftable
// 0x5e710c (CUserBase's is 0x5e70b4). It owns a sound handle (+0x04), a
// volume/scale (+0x08/+0x0c/+0x10), a play flag (+0x14), TWO audible rectangles
// (+0x18.. / +0x28..) seeded with the 0x80000000 "no bound" sentinel, and a
// sound id (+0x38).
//
// Field names are placeholders; the OFFSETS and code bytes are the load-bearing
// facts the matches prove. Only the parts the reconstructed methods touch are
// modeled; every cross-class callee is declared NO-body so its `call`/`mov ds:`
// reloc-masks.
#ifndef GRUNTZ_CAMBIENTSOUND_H
#define GRUNTZ_CAMBIENTSOUND_H

#include <rva.h>

// ---------------------------------------------------------------------------
// The engine sound channel the ambient source drives (this->m_04). Its three
// operations live in the sound-effect TU; modeled NO-body so the __thiscall
// `call` reloc-masks:
//   SetVolume(pct)           = 0x1355c0  (35B; set channel level)
//   SetVolumePan(pct, b, c)  = 0x135660  (224B; set level + pan/extra)
// ---------------------------------------------------------------------------
struct CSoundChannel {
    i32 SetVolume(i32 pct);                  // 0x1355c0
    i32 SetVolumePan(i32 pct, i32 b, i32 c); // 0x135660
};

// ---------------------------------------------------------------------------
// The game-manager singleton (g_gameReg) - the ambient source polls whether the
// audio subsystem (+0x10) and its active level/view object (+0x54->m_24) are
// live before (re)starting playback. Same WwdGameReg singleton the rest of the
// family taps; only the two touched offsets are modeled here.
// ---------------------------------------------------------------------------
struct CGameRegView {
    char m_pad00[0x24];
    i32 m_24; // +0x24  active view object
};

struct CAmbientGameReg {
    char m_pad00[0x10];
    i32 m_10; // +0x10  audio subsystem (live?)
    char m_pad14[0x54 - 0x14];
    CGameRegView* m_54; // +0x54  level/view holder
};
DATA(0x0024556c)
extern CAmbientGameReg* g_ambGameReg;

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
    // clamp to 0..100, then drive the channel (mode 0 -> SetVolume, else
    // SetVolumePan with the extra arg).
    i32 SetLevel(i32 value, i32 mode, i32 extra); // 0xc200

    // The per-frame update virtual (CAmbientSound vtable slot 3 = 0xc090): keeps
    // the channel running/paused based on whether (x,y) sits inside either
    // audible rectangle, (re)starting or fading as the listener crosses in/out.
    void Update(i32 x, i32 y, i32 force); // 0xc090

    // The fade helper (CAmbientSound::SetLevel sibling 0xc2a0; ret 0xc) reached
    // through the same ILT thunk Update uses. External here (not a target):
    // declared so Update's `call` reloc-masks.
    i32 Fade(i32 a, i32 b, i32 c); // 0xc2a0

    // Channel setup (0x136300, __thiscall 4 args) the (re)start path runs to
    // arm the sound id before the first level push.
    i32 SetupChannel(i32 a, i32 id, i32 c, i32 d); // 0x136300

    void* m_vptr;        // +0x00  vptr (manual stamp; CAmbientSound vtable 0x5e710c)
    CSoundChannel* m_04; // +0x04  sound channel handle
    i32 m_08;            // +0x08  current level (0..100)
    i32 m_0c;            // +0x0c  level scale mode
    i32 m_10;            // +0x10  secondary level multiplier (percent)
    i32 m_14;            // +0x14  playing flag
    i32 m_18;            // +0x18  rect0 left  (0x80000000 = unbounded)
    i32 m_1c;            // +0x1c  rect0 top
    i32 m_20;            // +0x20  rect0 right
    i32 m_24;            // +0x24  rect0 bottom
    i32 m_28;            // +0x28  rect1 left  (0x80000000 = unbounded)
    i32 m_2c;            // +0x2c  rect1 top
    i32 m_30;            // +0x30  rect1 right
    i32 m_34;            // +0x34  rect1 bottom
    i32 m_38;            // +0x38  sound id
    i32 m_3c;            // +0x3c
};
SIZE(CAmbientSound, 0x40);

#endif // GRUNTZ_CAMBIENTSOUND_H
