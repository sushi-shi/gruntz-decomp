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
#include <Gruntz/SbRect.h>        // SetupImage args 5..8 - ONE by-value geometry rect
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base

// The +0x24 config host is the shared canonical CSbiConfigHost (SbiConfig.h, pulled
// in the .cpp); only a pointer is needed here, so forward-declare it.
struct CSbiConfigHost;
// SetupImage arg1 is the owning status-bar manager (latched into the base m_2c slot);
// only a pointer is needed here. `class` (not `struct`) - the mangling depends on it.
class CStatusBarMgr;

// CSBI_RectOnly - the empty intermediate between CStatusBarItem and CSBI_Image
// (RTTI: CSBI_Image : CSBI_RectOnly : CStatusBarItem). In retail it overrides only
// the virtual destructor and adds no fields; the frameless method view constructs
// nothing here, so modeling it as a field-less intermediate is layout-identical to
// deriving CStatusBarItem directly (matching-neutral) while recovering the real level.
class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();                   // 0x00101fa0 (SBI_RectOnlyBase.cpp) - m_8 = 1
    virtual ~CSBI_RectOnly() OVERRIDE; // slot 0
    // (NO slot-1 override: sema class says vtbl 0x1eab8c slot [1] is INHERITED
    // (CStatusBarItem::SerializeFields, thunk 0x1848). The `SbiVfunc0` the old merged TU
    // defined under this class name belonged to the host's fabricated vtable, not here.)
    // slot 2 (0xe86e0). Args 5..8 are ONE by-value SbRect - see StatusBarItem.h.
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10) OVERRIDE;
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    // Member teardown run by the CHAIN-DTOR device (see StatusBarItem.h).
    void DtorRect(); // 0xe8760
    // (InsertPtr 0x108410 / ClearTabGroup 0x100b00 / Deactivate 0x100cb0 were declared
    // here on the strength of the old CSBI_RectOnly/CStatusBarMgr name conflation. They
    // are methods of the 0x630 status-bar HOST (CStatusBarMgr) and moved there with the
    // split - this thin sub-widget never had them.)
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
    // The type tag (m_8 = 3) is CSBI_Image's own ctor leg: every `new CSBI_Image` in the
    // tab builders (CStatusBarMgr::LoadTabSprites, CGameMenuMgr::BuildGameMenu) produces
    // tag 3 with m_30 cleared, on top of the out-of-line CSBI_RectOnly base ctor (0x101fa0)
    // retail calls first. Inline, so the builders fold it at the new-site like retail.
    CSBI_Image() {
        m_8 = 3;
        m_30 = 0;
    }
    virtual ~CSBI_Image() OVERRIDE;   // slot 0
    virtual i32 SbiVfunc0() OVERRIDE; // slot 1
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    // vtable slot 11 (0xe6c80): the image setup, 11 dwords of args. The RETAIL BODY pins
    // the arg types (disasm 0xe6c80): entry `mov eax,[esp+8]` reads arg2 and later
    // `mov ecx,[eax+0x10]` DEREFERENCES it => arg2 is the CSbiConfigHost*; arg1 is only
    // null-tested and stored (m_2c = owner), and every call site passes the building
    // manager's `this`. Args 5..8 are ONE by-value rect (the builders fill an SbRect and
    // pass it), arg9 the asset key string. This is the ONE signature: the tab builders
    // used to call it through a fabricated 15-slot `CSbConfigItem::Configure` view, which
    // made cl emit a 60 B ??_7CSBI_Image@@6B@ in statusbarmgr against retail's 48 B.
    virtual i32 SetupImage( // slot 11 (new)
        CStatusBarMgr* owner,
        CSbiConfigHost* host,
        i32 a3,
        i32 a4,
        SbRect rc,
        const char* key,
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
