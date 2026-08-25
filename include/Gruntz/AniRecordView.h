#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wwd/WwdAnimStepMode.h>

class SoundCueRegistry;
class CDDrawSurfaceMgr;
struct SoundCue;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

struct CAniRecordView : public CObject {
    // 0x1657a0 (RVA_COMPGEN pin at the keeper, DDrawSurfacePair.cpp - an RVA()
    // here would annotate BOTH cl dtor variants and collide with
    // ??_GCAniRecordView@0x165780).
    virtual ~CAniRecordView() OVERRIDE {
        CAniRecordView* r = this;
        if (r->m_cues != NULL) {
            delete[] r->m_cues;
        }
        r->m_loopMode = WWDLOOP_INVALID;
        r->m_cueCount = 0;
        r->m_cues = NULL;
    }

    i32 Parse(SoundCueRegistry* ctx, const i16* src);
    i32 GetSize();
    void ResolveIndices(SoundCueRegistry* owner, const char* str);

    // A __thiscall whose body never touches `this` (it steps the two globals at
    // 0x6c2798/0x6c278c).  The receiver is proved by the call sites in
    // CAniAdvanceCursor::Advance: `mov ecx,edi` (edi = m_element) immediately
    // before each `call 0x15cbe0`, dead for a __cdecl callee.
    i32 Rng2Next();

    // The wwd module's own revision of Monolith's GetRandomNumber. In-class so
    // the static's mangled name differs from the game header's - three distinct
    // names are the only mechanism for retail's three guard/seed pairs (same-name
    // COMMONs fold, `static __inline` layout disproven):
    // docs/patterns/header-inline-local-static-three-copies.md. Natural here: the
    // class already owns the bespoke Rng2Next family, whose body inlines this.
    i32 GetRandomNumber() {
        static long holdrand = timeGetTime();
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
    }

    inline CAniRecordView() {
        m_cueCount = 0;
        m_cues = NULL;
        m_loopMode = WWDLOOP_INVALID;
    }

    u16 m_flags;
    u16 m_pad06;
    WwdAnimStepMode m_stepMode;
    WwdAnimLoopMode m_loopMode;
    WwdAnimPositionMode m_positionMode;
    i32 m_param;
    i32 m_frameTime;
    i32 m_drawValue;
    i32 m_positionDeltaX;
    i32 m_positionDeltaY;
    u16 m_reserved28; // parsed from ANI frame record; never read
    u16 m_pad2a;
    i32 m_cueCount;
    SoundCue** m_cues;
};

#endif // GRUNTZ_CANIRECORDVIEW_H
