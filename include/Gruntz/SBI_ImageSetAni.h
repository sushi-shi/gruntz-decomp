// SBI_ImageSetAni.h - CSBI_ImageSetAni (frameless method view), the "Resource
// SHREDDER conveyor" SBI leaf. RTTI .?AVCSBI_ImageSetAni@@; chain
//   CSBI_ImageSetAni : CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5eae3c; slot 1 Serialize (0xe7cd0, thunk 0x2829, inherited by
// CSBI_StatzTabArrow) saves/loads six persistent ints (m_3c..m_50) then chains the
// CSBI_ImageSet base serialize (0xe74f0).
//
// This is the FRAMELESS method view (parallel to <Gruntz/SBI_WarlordHead.h>); the
// builder-facet view (CSbConfigItem base) lives in <Gruntz/StatusBarMgrBuilders.h> -
// the SBI two-view split (deliberately-incompatible, never co-included).
#ifndef GRUNTZ_SBI_IMAGESETANI_H
#define GRUNTZ_SBI_IMAGESETANI_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_ImageSet.h> // canonical CSBI_ImageSet (base Serialize) + CImageSetStream

class CSBI_ImageSetAni : public CSBI_ImageSet {
public:
    // Real vtable shape (sema class: vtbl@0x1ead6c, 15 slots; overrides 0/1/4/5,
    // news 13/14). The out-of-line ~ (0x1047f0) lives in SBI_ImageSetAni.cpp via
    // the CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_ImageSetAni() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE;     // slot 1 (the Serialize below)
    virtual void SbiSlot4() OVERRIDE;     // slot 4
    virtual void SbiSlot5() OVERRIDE;     // slot 5
    virtual void AniInit();               // slot 13 (new)
    virtual void AniSetRange();           // slot 14 (new; 0xe7c30 SetRange)
    // Member teardown = the INHERITED CSBI_ImageSet::ResetCounters (0xe7400); retail's
    // ~CSBI_ImageSetAni calls it at its own level and again at the folded ImageSet level
    // (two `call 0xe7400`). The old declared-only DtorImageSetAni alias was a fake view
    // of that same function (unbound - would not link).

    // vtable slot 1 (0xe7cd0): serialize the six persistent ints (m_3c..m_50) through
    // the stream's Read/WriteBytes, then chain the CSBI_ImageSet base serialize.
    i32 Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4);

    // slot-13 body (vtbl 0x1ead6c slot [13], thunk 0x3b48 -> 0xe7980): seed the item
    // from a config host + rect + record key; 14 args (the caller passes the host as
    // BOTH arg0 and arg1). Ex CAniPlayer::Init (dossier #16 identity fold).
    i32 Init(
        i32 cfg,
        CSbiConfigHost* host,
        i32 a2,
        i32 a3,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3,
        i32 key,
        i32 b0,
        i32 b1,
        i32 b2,
        i32 b3,
        i32 b4
    ); // 0xe7980
    // slot-5 body (vtbl 0x1ead6c slot [5], thunk 0x2dfb): the timeGetTime-driven cel
    // advance within [m_4c, m_50]. Ex CAniPlayer::Tick (dossier #16 identity fold).
    i32 Tick(); // 0xe7b00
    // slot-14 body (vtbl 0x1ead6c slot [14], thunk 0x3bde): re-arm with a new frame
    // window without re-resolving the record. Ex CAniPlayer::SetRange (dossier #16).
    void SetRange_0e7c30(i32 start, i32 end, i32 step, i32 loop, i32 interval); // 0xe7c30

    i32 m_3c; // +0x3c  persistent serialized ints (Serialize save/load block)
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
};
SIZE_UNKNOWN(CSBI_ImageSetAni);
VTBL(CSBI_ImageSetAni, 0x001ead6c); // vtable_names -> code (RTTI game class; was in SbiDtorChain.h)

// CHAIN-DTOR device: the inline ~CSBI_ImageSetAni body a merged /GX leaf TU folds when
// CSBI_ImageSetAni is an INLINE base (a deeper leaf, e.g. CSBI_StatzTabArrow). The one TU
// that owns the out-of-line ??1 (SBI_ImageSetAni.cpp, the 0x1047f0 leaf) #defines
// SBI_OWN_IMAGESETANI_DTOR to suppress this inline. Mirrors the SBI_Image.h/SBI_ImageSet.h
// device (was <Gruntz/SbiDtorChain.h>, now retired). See StatusBarItem.h.
#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGESETANI_DTOR)
inline CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    ResetCounters();
}
#endif

// CSBI_StatzTabArrow - the deepest SBI leaf (RTTI .?AVCSBI_StatzTabArrow@@): the ANI-conveyor
// arrow tab. Full chain CSBI_StatzTabArrow : CSBI_ImageSetAni : CSBI_ImageSet : CSBI_Image :
// CSBI_RectOnly : CStatusBarItem. Its out-of-line /GX ~ (0x1048f0, folds all five base levels)
// lives in SBI_StatzTabArrowEh.cpp via the CHAIN-DTOR device. This is the FRAMELESS/chain view;
// the builder-facet view (CSbConfigItem base) is in <Gruntz/StatusBarMgrBuilders.h> (the SBI
// two-view split, never co-included).
struct CSBI_StatzTabArrow : CSBI_ImageSetAni {
    // Its /GX dtor's member teardown is the INHERITED CSBI_ImageSet::ResetCounters
    // (0xe7400), called three times: own level + the folded ImageSetAni + ImageSet
    // levels. (DtorStatzTabArrow was a declared-only fake view of that function.)
    virtual ~CSBI_StatzTabArrow() OVERRIDE;
};
SIZE_UNKNOWN(CSBI_StatzTabArrow);

#endif // GRUNTZ_SBI_IMAGESETANI_H
