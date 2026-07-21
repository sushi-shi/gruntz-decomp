#ifndef SBI_WARLORDHEAD_H
#define SBI_WARLORDHEAD_H

#include <Ints.h>
#include <rva.h>

#include <Image/CImage.h>        // the canonical frame-record class (CImage::RenderFrame @0x153790)
#include <Gruntz/SBI_ImageSet.h> // canonical CSBI_ImageSet (base Serialize external) + CImageSetStream

struct ShadeDescr; // CImage::m_owned->m_palDescr type, latched by ShowFrames (CDDrawShadeBlit.h)

struct CWhRect {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
};
SIZE_UNKNOWN(CWhRect);

class CSBI_WarlordHead : public CSBI_ImageSet {
public:
    // tag 0xb (the multiplayer WARLORDHEAD slot).
    CSBI_WarlordHead() {
        m_30 = 0;
        m_kind = 0xb;
        m_34 = 0;
    }
    // Real vtable shape (sema class: vtbl@0x1ead24, 13 slots; overrides 0/1/5/11).
    // The out-of-line ~ (0x104a00, calls DtorReset) lives in SBI_WarlordHead.cpp via
    // the CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_WarlordHead() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1ead24 thunk 0x3cd8 -> 0xeb970): serialize the head's direction
    // (m_direction), then chain CSBI_ImageSet::SerializeFields (0xe74f0).
    virtual i32 SerializeFields(CImageSetStream* s, i32 mode, i32 a3, i32 a4) OVERRIDE; // 0xeb970
    virtual void SbiSlot5() OVERRIDE; // slot 5 (the Render below)
    // slot 11 (0xeb6b0), the CSBI_Image::SetupImage override. This USED to be split in
    // two: a body-less `virtual` declared purely to pin the slot, plus the real body as a
    // separate NON-virtual overload distinguished only by `i32 host` vs `CDDrawSurfaceMgr*`.
    // One function, one slot - the real body IS the override.
    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 a3,
        i32 a4,
        SbRect rc,
        const char* key,
        i32 a10,
        i32 a11
    ) OVERRIDE; // slot 11
    // Member teardown = the INHERITED CSBI_ImageSet::ResetCounters (0xe7400); retail's
    // ~CSBI_WarlordHead calls it at its own level and again at the folded ImageSet level
    // (two `call 0xe7400`). The old declared-only DtorReset alias was a fake view of that
    // same function (unbound - would not link).

    // (0xeb970 was declared here as a non-virtual `Serialize` - it IS the slot-1
    //  SerializeFields override declared above. The six-int slot-1 serialize formerly
    //  claimed here at 0xe7cd0 was CSBI_ImageSetAni's - re-homed to SBI_ImageSetAni.cpp.)
    // (The `BaseSetupImage` declared here was a fake alias of CSBI_ImageSet::SetupImage
    // (0xe72f0) - the real base slot. With the real chain modeled, ~SetupImage just calls
    // the base qualified, so the alias is gone.)

    // 0xeb740: drive the show/hide of the two anchor frames (frame table slots 1/2).
    i32 ShowFrames(i32 show, ShadeDescr* palDescr);
    // 0xeb830: latch the raw direction (m_direction) + the derived state (m_38 = 1 or 2).
    i32 SetState(i32 dir);
    // vtable slot 5 (0xeb880): the per-frame render of the head's two frames.
    i32 Render(); // 0-arg: body ends `retl` (cleans 0); the ex-`i32 z` was fabricated + unused

    // The show/hide notifier for a frame's sprite handle (0x14dd90, __stdcall,
    // ret 8). Modeled as a free function so `push 0; push arg; call` falls out with
    // no this.

    // ----- own fields (after CSBI_ImageSet @0x3c); base region reuses inherited
    // m_rect14.m_0/m_4 (draw origin), m_28 (countdown), m_30 (frame, base-typed i32),
    // m_34 (config, the base's CImageSet*), m_38 (state index).
    i32 m_direction; // +0x3c  direction (SetState writes the raw dir; Serialize + Render read it)
};
SIZE_UNKNOWN(CSBI_WarlordHead);
VTBL(CSBI_WarlordHead, 0x001ead24); // vtable_names -> code (RTTI game class)

void WhShowItem(i32 handle, i32 flag);

#endif // SBI_WARLORDHEAD_H
