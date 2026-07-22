#ifndef GRUNTZ_SBI_IMAGESETANI_H
#define GRUNTZ_SBI_IMAGESETANI_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_ImageSet.h> // canonical CSBI_ImageSet (base Serialize) + CImageSetStream

class CSBI_ImageSetAni : public CSBI_ImageSet {
public:
    // tag 8 + the ani window seeded (m_interval = 0x64); the `new CSBI_ImageSetAni`
    // ctor leg the Resource-tab conveyor builders fold at each new-site.
    CSBI_ImageSetAni() {
        m_frame = 0;
        m_kind = 8;
        m_34 = 0;
        m_step = 0;
        m_interval = 0x64;
    }
    // Real vtable shape (sema class: vtbl@0x1ead6c, 15 slots; overrides 0/1/4/5,
    // news 13/14). The out-of-line ~ (0x1047f0) lives in SBI_ImageSetAni.cpp via
    // the CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_ImageSetAni() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1ead6c thunk 0x2829 -> 0xe7cd0): serialize the six persistent ints
    // (m_interval..m_frameEnd) through the stream, then chain CSBI_ImageSet::SerializeFields.
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
    virtual void SetRange(i32 start, i32 end, i32 step, i32 loop, i32 interval);
    // Member teardown = the INHERITED CSBI_ImageSet::ResetCounters (0xe7400); retail's
    // ~CSBI_ImageSetAni calls it at its own level and again at the folded ImageSet level
    // (two `call 0xe7400`). The old declared-only DtorImageSetAni alias was a fake view
    // of that same function (unbound - would not link).

    // (0xe7cd0 was declared here as a non-virtual `Serialize` - it IS the slot-1
    //  SerializeFields override declared above.)

    // slot-5 body (vtbl 0x1ead6c slot [5], thunk 0x2dfb): the timeGetTime-driven cel
    // advance within [m_frameStart, m_frameEnd]. Ex CAniPlayer::Tick (dossier #16 identity fold).
    i32 Tick(); // 0xe7b00

    i32 m_interval; // +0x3c  persistent serialized ints (Serialize save/load block)
    i32 m_lastTime; // +0x40
    i32 m_step; // +0x44
    i32 m_loop; // +0x48
    i32 m_frameStart; // +0x4c
    i32 m_frameEnd; // +0x50
};
SIZE_UNKNOWN();
VTBL(CSBI_ImageSetAni, 0x001ead6c); // vtable_names -> code (RTTI game class; was in SbiDtorChain.h)

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGESETANI_DTOR)
inline CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    ResetCounters();
}
#endif

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
SIZE(0x54);
VTBL(CSBI_StatzTabArrow, 0x001eac94); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_SBI_IMAGESETANI_H
