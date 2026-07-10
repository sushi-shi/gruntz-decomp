// StatusBarItem.cpp - labeling stand-in for the standalone complete-object ctor.
//
// CStatusBarItem is ONE class, defined canonically in <Gruntz/StatusBarItem.h>
// with an INLINE ctor (so the derived CSBI_RectOnly folds it - see SBI_RectOnly.cpp).
// Retail emits that same inline ctor out-of-line as a COMDAT (the complete-object
// ctor at 0x1005d0) wherever a bare CStatusBarItem is constructed. MSVC 5.0,
// however, inlines this tiny ctor at every instantiation we can synthesize, so it
// will not emit a labelable standalone ??0 from the canonical inline form.
//
// To keep the 0x1005d0 byte-match, this TU takes the canonical class from
// <Gruntz/StatusBarItem.h> but #defines SBI_ITEM_OWN_CTOR first: that flips the
// header's inline ctor to an out-of-line DECLARATION, so MSVC emits a standalone ??0
// this TU labels with the RVA-keyed body below. Same one class, out-of-line-ctor
// facet; a tooling workaround for the inline-ctor COMDAT, NOT a second class.
#include <rva.h>

#define SBI_ITEM_OWN_CTOR
#include <Gruntz/StatusBarItem.h>

// ---------------------------------------------------------------------------
// CStatusBarItem::CStatusBarItem()
// Out-of-line complete-object ctor: zeroes m_4/m_8/m_24/m_28 after the vftable
// is installed. Byte-identical to the COMDAT copy of the header's inline ctor.
RVA(0x001005d0, 0x17)
CStatusBarItem::CStatusBarItem() {
    m_4 = 0;
    m_8 = 0;
    m_24 = 0;
    m_28 = 0;
}

// Out-of-line stubs anchor the CStatusBarItem vftable in this TU (not matched).
CStatusBarItem::~CStatusBarItem() {}
// Scalar-deleting dtor (??_G, slot 0): compiler-generated thunk wrapping the real
// ~CStatusBarItem cleanup (calls one base dtor; not reconstructed, so this only NAMES
// the retail function). MSVC synthesizes ??_G from the virtual dtor above.
// @rva-symbol: ??_GCStatusBarItem@@UAEPAXI@Z 0x00100620 0x24
i32 CStatusBarItem::SbiVfunc0() {
    return 0;
}

// CStatusBarItem vtable RVA binding (moved from the deleted
// src/Stub/BoundaryLowerThunks.cpp vtable catalog).
VTBL(CStatusBarItem, 0x001eabcc);
