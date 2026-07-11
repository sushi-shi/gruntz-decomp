// SBI_WarlordHead.h - Gruntz CSBI_WarlordHead (C:\Proj\Gruntz).
// RTTI .?AVCSBI_WarlordHead@@; the most-derived leaf of the SBI image chain
//   CSBI_WarlordHead : CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5ead24 (RTTI meta 0x5f4f40). The 5-level /GX-framed scalar destructor
// (0x104a00) lives in SBI_WarlordHeadEh.cpp.
//
// This leaf adds, over CSBI_ImageSet:
//   slot 5  (0xeb880)  the per-frame Render override (countdown-driven two-frame draw)
//   slot 11 (0xeb6b0)  the SetupImage override (forwards to the ImageSet base + latches state)
// plus two non-virtual helpers (0xeb740 ShowFrames, 0xeb830 SetState).
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact (the
// mangled ?<method>@CSBI_WarlordHead@@... names are layout-independent). The class
// is modeled with the SBI family's manual-vtable-stamp device (no real `virtual`),
// so each frameless method matches without forcing a divergent compiler vtable;
// sibling/engine callees are ILT/vtable-reloc-masked.
#ifndef SBI_WARLORDHEAD_H
#define SBI_WARLORDHEAD_H

#include <Ints.h>
#include <rva.h>

#include <Image/CImage.h>        // the canonical frame-record class (CImage::RenderFrame @0x153790)
#include <Gruntz/SBI_ImageSet.h> // canonical CSBI_ImageSet (base Serialize external) + CImageSetStream

struct ShadeDescr; // CImage::m_owned->m_palDescr type, latched by ShowFrames (CDDrawShadeBlit.h)

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call through them is reloc-masked).

// The frame record (an element of the config record's m_14 frame table) is the
// RTTI-confirmed CImage: a draw-offset pair at m_18/m_1c, an owned sprite/anim
// object at m_30 (a CImageOwned whose m_1c ShowFrames latches), drawn by
// CImage::RenderFrame (0x153790, __thiscall). Modeled by <Image/CImage.h>.

// The resolved config record (CSBI_Image::m_34 / the ImageSet lookup result): a
// frame-index range gate at m_64/m_68 and a frame table at m_14 (an array of
// CImage*). Same shape as CSbiConfigRecord (<Gruntz/SbiConfig.h>).
struct CWhConfig {
    char m_pad0[0x14];
    CImage** m_14; // +0x14  frame table (array of frame-record pointers)
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame-index range hi gate (idx > m_64 => reject)
    i32 m_68; // +0x68  frame-index range lo gate (idx < m_68 => reject)
};
SIZE_UNKNOWN(CWhConfig);

// The active surface context Render passes into RenderFrame is reached through the
// canonical resource manager: g_gameReg->m_world (CSpriteFactoryHolder) ->
// m_drawTarget (CDrawTarget, +0x04) -> m_drawContext (+0x14). Modeled by the shared
// <Gruntz/GameRegistry.h> + <Gruntz/ResMgr.h> types (see SBI_WarlordHead.cpp); no
// per-TU game-manager facet is kept.

// The base ImageSet SetupImage (CSBI_ImageSet vtable slot 11, 0xe72f0): same
// 11-arg shape (id, host, a3, a4, then the four rect ints, key, a10, a11). The
// rect block is forwarded as a by-value 4-int aggregate so MSVC stages it in a
// temp on the caller stack exactly as retail does.
struct CWhRect {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
};
SIZE_UNKNOWN(CWhRect);

// The serialization stream (Serialize arg1) + the immediate base CSBI_ImageSet
// (0xe74f0 vtable slot 1) come from the canonical <Gruntz/SBI_ImageSet.h> above.
// The base-serialize call emits the real ?Serialize@CSBI_ImageSet@@... external,
// whose reloc pairs against SBI_ImageSet.cpp's definition at 0xe74f0.

// ---------------------------------------------------------------------------
// CSBI_WarlordHead - the warlord-head status-bar item. Real RTTI base is
// CSBI_ImageSet (see top comment); kept FLAT (frameless method-view) because the
// render/serialize methods read base-region storage (m_14/m_18/m_28/m_30/m_34/m_38)
// under head-specific names spanning all four base levels.
class CSBI_WarlordHead : public CSBI_ImageSet {
public:
    // Real vtable shape (sema class: vtbl@0x1ead24, 13 slots; overrides 0/1/5/11).
    // The out-of-line ~ (0x104a00, calls DtorReset) lives in SBI_WarlordHead.cpp via
    // the CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_WarlordHead() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE;     // slot 1 (the Serialize below)
    virtual void SbiSlot5() OVERRIDE;     // slot 5 (the Render below)
    // slot 11 override of CSBI_Image::SetupImage; the out-of-line body is the
    // non-virtual SetupImage overload below (0xeb6b0). The vtable slot reloc-masks, so
    // this declared-only override just pins the slot in the model (was in the retired
    // SbiDtorChain.h). Distinct overload (CSbiConfigHost* vs i32) from the impl decl.
    virtual i32 SetupImage(
        i32,
        CSbiConfigHost*,
        i32,
        i32,
        i32,
        i32,
        i32,
        i32,
        i32,
        i32,
        i32
    ) OVERRIDE; // slot 11
    // Member teardown run by the dtor: 0xe7400 (the ImageSet-level counter reset the
    // chain view called DtorReset; reloc-masked extern).
    void DtorReset(); // 0xe7400

    // vtable slot 1 (0xeb970): serialize the head's single direction (m_3c), then
    // chain to the CSBI_ImageSet base serialize (0xe74f0). (The six-int slot-1
    // serialize formerly claimed here at 0xe7cd0 was CSBI_ImageSetAni's - re-homed
    // to SBI_ImageSetAni.cpp; warlord's own slot 1 is 0xeb970, thunk 0x3cd8.)
    i32 Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4);
    // vtable slot 11 (0xeb6b0): forward all 11 args to the ImageSet base setup; on
    // success latch the initial state (SetState(0)) and report 1.
    i32 SetupImage(
        i32 a1,
        i32 host,
        i32 a3,
        i32 a4,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3,
        i32 key,
        i32 a10,
        i32 a11
    );
    // The base ImageSet SetupImage (0xe72f0, vtable slot 11): the rect block is one
    // by-value aggregate so the temp-struct stage matches.
    i32 BaseSetupImage(i32 a1, i32 host, i32 a3, i32 a4, CWhRect rect, i32 key, i32 a10, i32 a11);

    // 0xeb740: drive the show/hide of the two anchor frames (frame table slots 1/2).
    i32 ShowFrames(i32 show, ShadeDescr* palDescr);
    // 0xeb830: latch the raw direction (m_3c) + the derived state (m_38 = 1 or 2).
    i32 SetState(i32 dir);
    // vtable slot 5 (0xeb880): the per-frame render of the head's two frames.
    i32 Render(i32 z);

    // The show/hide notifier for a frame's sprite handle (0x14dd90, __stdcall,
    // ret 8). Modeled as a free function so `push 0; push arg; call` falls out with
    // no this.

    // ----- own fields (after CSBI_ImageSet @0x3c); base region reuses inherited
    // m_rect14.m_0/m_4 (draw origin), m_28 (countdown), m_30 (frame, base-typed i32),
    // m_34 (config, base-typed CSprite* -> cast to CWhConfig*), m_38 (state index).
    i32 m_3c; // +0x3c  direction (SetState writes the raw dir; Serialize + Render read it)
};
SIZE_UNKNOWN(CSBI_WarlordHead);
VTBL(CSBI_WarlordHead, 0x001ead24); // vtable_names -> code (RTTI game class)

// The frame sprite show/hide notifier (0x14dd90, __stdcall, ret 8).
void WhShowItem(i32 handle, i32 flag);

#endif // SBI_WARLORDHEAD_H
