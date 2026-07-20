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
    // tag 8 + the ani window seeded (m_3c = 0x64 interval); the `new CSBI_ImageSetAni`
    // ctor leg the Resource-tab conveyor builders fold at each new-site.
    CSBI_ImageSetAni() {
        m_30 = 0;
        m_kind = 8;
        m_34 = 0;
        m_44 = 0;
        m_3c = 0x64;
    }
    // Real vtable shape (sema class: vtbl@0x1ead6c, 15 slots; overrides 0/1/4/5,
    // news 13/14). The out-of-line ~ (0x1047f0) lives in SBI_ImageSetAni.cpp via
    // the CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_ImageSetAni() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1ead6c thunk 0x2829 -> 0xe7cd0): serialize the six persistent ints
    // (m_3c..m_50) through the stream, then chain CSBI_ImageSet::SerializeFields.
    // CImageSetStream is a typedef of CSerialArchive == the real CFileMemBase, so this is
    // the same parameter type as the rest of the chain (mangles PAVCFileMemBase@@).
    virtual i32 SerializeFields(CImageSetStream* s, i32 mode, i32 a3, i32 a4) OVERRIDE; // 0xe7cd0
    virtual void SbiSlot4() OVERRIDE;                                                   // slot 4
    virtual void SbiSlot5() OVERRIDE;     // slot 5
    // Slots 13/14 ARE Init and SetRange - the ILT thunks prove it (0x3b48 -> jmp 0xe7980
    // = Init; 0x3bde -> jmp 0xe7c30 = SetRange). They used to be declared here TWICE: as
    // two body-less placeholder virtuals `AniInit`/`AniSetRange` (invented only to pad the
    // vtable out to 15 slots, and left as unresolved externals the vtable pointed at) and
    // again as the real non-virtual bodies below. One function, one slot: the real bodies
    // ARE the virtuals.
    virtual i32 Init( // slot 13 (new)  0xe7980
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 a3,
        i32 a4,
        SbRect rc,
        const char* key,
        i32 b0,
        i32 b1,
        i32 b2,
        i32 b3,
        i32 b4
    );
    // slot 14 (new) 0xe7c30: re-arm with a new frame window without re-resolving the
    // record. The Statz-arrow direction setters below drive exactly this slot.
    virtual void SetRange_0e7c30(i32 start, i32 end, i32 step, i32 loop, i32 interval);
    // Member teardown = the INHERITED CSBI_ImageSet::ResetCounters (0xe7400); retail's
    // ~CSBI_ImageSetAni calls it at its own level and again at the folded ImageSet level
    // (two `call 0xe7400`). The old declared-only DtorImageSetAni alias was a fake view
    // of that same function (unbound - would not link).

    // (0xe7cd0 was declared here as a non-virtual `Serialize` - it IS the slot-1
    //  SerializeFields override declared above.)

    // slot-5 body (vtbl 0x1ead6c slot [5], thunk 0x2dfb): the timeGetTime-driven cel
    // advance within [m_4c, m_50]. Ex CAniPlayer::Tick (dossier #16 identity fold).
    i32 Tick(); // 0xe7b00

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
class CSBI_StatzTabArrow : public CSBI_ImageSetAni {
public:
    // tag 5 (the Statz per-grunt arrow); same ani window seed as the base.
    CSBI_StatzTabArrow() {
        m_kind = 5;
    }
    // Its /GX dtor's member teardown is the INHERITED CSBI_ImageSet::ResetCounters
    // (0xe7400), called three times: own level + the folded ImageSetAni + ImageSet
    // levels. (DtorStatzTabArrow was a declared-only fake view of that function.)
    virtual ~CSBI_StatzTabArrow() OVERRIDE;

    // The two direction setters (0xea0f0 / 0xea170): pick one of four frame-window
    // tuples from the two selectors and forward to the slot-14 SetRange virtual. Owner
    // pinned by the call site - CStatusBarMgr::LoadTabSprites calls them on a freshly
    // `new CSBI_StatzTabArrow` (Statz per-grunt row); slot 14 exists from CSBI_ImageSetAni
    // down, so the arrow is the deepest class both callers agree on. Bodies live in
    // StatusBarTabBuilders.cpp. (They were methods of the fabricated CSbConfigItem.)
    void SetDirection(i32 a, i32 b);      // 0x0ea0f0
    void SetDirectionAlt(i32 a1, i32 a2); // 0x0ea170
    // The m_114-gated 2-arg arrow mode sink (reloc `M`).
    void SetArrowMode(i32 a, i32 b);
};
SIZE(CSBI_StatzTabArrow, 0x54);
VTBL(CSBI_StatzTabArrow, 0x001eac94); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_SBI_IMAGESETANI_H
