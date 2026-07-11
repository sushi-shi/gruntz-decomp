// SBI_Image.h - Gruntz CSBI_Image (C:\Proj\Gruntz), the FRAMELESS method view.
// RTTI .?AVCSBI_Image@@; the real single-inheritance chain (RTTI-confirmed):
//   CSBI_Image : CSBI_RectOnly : CStatusBarItem   (vtable 0x5eac0c).
//
// This is the ONE canonical frameless spelling of CSBI_Image + its RectOnly
// intermediate, shared by the concrete method TUs (SBI_Image.cpp defines
// SetupImage; SBI_ImageSet.h derives CSBI_ImageSet from it). It is the FRAMELESS
// counterpart of two deliberate, NEVER-co-included views of the SAME retail class:
//   * <Gruntz/SbiDtorChain.h> - the CHAIN view (polymorphic /GX dtor chain, size
//     0x60 grand-base) the *Eh.cpp dtor TUs fold; and
//   * the builder-facet views (CSbConfigItem/CSbDialogItem/CSbMenuItem in the
//     CStatusBarMgr.cpp / SBI_TabzDialogEh.cpp / StatusBarGameMenu.cpp builders) -
//     the same CSBI_Image level modeled as a native-dispatch polymorphic base so
//     `new CSBI_X` auto-stamps the retail vtable and Configure lowers to
//     `call [edx+0x2c]`. One MSVC5 spelling emits only one of these shapes, so the
//     views stay split (see the two-view-split note atop StatusBarItem.h).
//
// Fields are placeholders; the offsets + total 0x34 size are the load-bearing fact.
#ifndef GRUNTZ_SBI_IMAGE_H
#define GRUNTZ_SBI_IMAGE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base

// The +0x24 config host is the shared canonical CSbiConfigHost (SbiConfig.h, pulled
// in the .cpp); only a pointer is needed here, so forward-declare it.
struct CSbiConfigHost;

// CSBI_RectOnly - the empty intermediate between CStatusBarItem and CSBI_Image
// (RTTI: CSBI_Image : CSBI_RectOnly : CStatusBarItem). In retail it overrides only
// the virtual destructor and adds no fields; the frameless method view constructs
// nothing here, so modeling it as a field-less intermediate is layout-identical to
// deriving CStatusBarItem directly (matching-neutral) while recovering the real level.
class CSBI_RectOnly : public CStatusBarItem {
public:
    virtual ~CSBI_RectOnly() OVERRIDE; // slot 0
    virtual i32
    Setup(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9, i32 a10)
        OVERRIDE; // slot 2 (0xe86e0, declared-only here; body in SBI_RectOnly.cpp)
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    i32 InsertPtr(i32 a, i32 b);      // 0x108410 (i32 ret, matches SBI_RectOnly.h + retail)
    // Tab-group helpers defined in SBI_RectOnly.cpp on the 0x570 HOST that wears the
    // same CSBI_RectOnly name (a known conflation - see CSbiRectSub in SBI_RectOnly.h).
    // Declared here so callers (SBI_MenuItem.cpp SetState) mangle the identical
    // ?...@CSBI_RectOnly@@ symbols without a per-TU view; decl-only = byte-neutral.
    void ClearTabGroup(); // 0x100b00
    i32 Deactivate();     // 0x100cb0
    // Member teardown run by the CHAIN-DTOR device (see StatusBarItem.h).
    void DtorRect(); // 0xe8760
};
SIZE_UNKNOWN(CSBI_RectOnly);
VTBL(CSBI_RectOnly, 0x001eab8c); // vtable_names -> code (RTTI game class)

// CHAIN-DTOR device (see StatusBarItem.h): inline base-dtor body for the merged
// /GX leaf TUs; SBI_OWN_RECTONLY_DTOR marks the TU that owns the out-of-line ??1.
#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_RECTONLY_DTOR)
inline CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}
#endif

// CSBI_Image - the image status-bar item. Inherits the CStatusBarItem base-region
// fields (via CSBI_RectOnly), INCLUDING the id slot at +0x2c (base CStatusBarItem::m_2c,
// which SetupImage latches the id into - the base owns +0x2c, proven by its slot-2
// CStatusBarItem::Setup @0x100660 storing arg1 there); adds only the latched-value m_30.
class CSBI_Image : public CSBI_RectOnly {
public:
    virtual ~CSBI_Image() OVERRIDE;   // slot 0
    virtual i32 SbiVfunc0() OVERRIDE; // slot 1
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    // vtable slot 11 (0xe6c80): the 11-arg image setup; arg1 = id, arg2 = config
    // host, args 3..8 = the rect block, arg9 = the lookup key (args 10/11 unused).
    virtual i32 SetupImage( // slot 11 (new)

        i32 a1,
        CSbiConfigHost* host,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 a8,
        i32 key,
        i32 a10,
        i32 a11
    );

    // slot-3 body AND the dtor's member teardown (ONE retail body, 0xe6d90 - the
    // chain dtors call it; the vtable slot-3 thunk 0x1b59 jmps to it). Re-attributed
    // from CSBI_MenuItem (dossier #16: vtbl 0x1eac0c slot [3]); body in SBI_Image.cpp.
    void ClearFrame(); // 0xe6d90
    // slot-5 body (vtbl 0x1eac0c slot [5], thunk 0x16e5): one play step rendering the
    // CURRENT resolved frame m_30 (no table re-lookup). Ex CAniPlayer view (dossier #16).
    i32 TickRenderCurrent_0e6dd0(); // 0xe6dd0
    // slot-1 body (vtbl 0x1eac0c slot [1], thunk 0x2077): the CSBI_Image serialize leg;
    // tail-chains the base SerializeFields. Re-attributed from CSBI_MenuItem (dossier #16).
    i32 SerializeChain(void* ar, i32 kind, i32 a, i32 b); // 0xe6e40

    // +0x2c is the inherited base CStatusBarItem::m_2c (the id slot SetupImage latches).
    i32 m_30; // +0x30  latched config value
};
SIZE(CSBI_Image, 0x34);
VTBL(CSBI_Image, 0x001eac0c); // vtable_names -> code (RTTI game class)

// CHAIN-DTOR device (see StatusBarItem.h): inline base-dtor body for the merged
// /GX leaf TUs; SBI_OWN_IMAGE_DTOR marks the TU that owns the out-of-line ??1
// (SBI_Image.cpp, RVA 0x100870).
#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGE_DTOR)
inline CSBI_Image::~CSBI_Image() {
    ClearFrame();
}
#endif

#endif // GRUNTZ_SBI_IMAGE_H
