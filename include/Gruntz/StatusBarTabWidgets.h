// StatusBarTabWidgets.h - the widget views the status-bar HOST's own builder TU
// (SBI_RectOnly.cpp: BuildStatusBarTabs / the tab builders) instantiates.
//
// SCOPE: included by that ONE TU. These are per-TU reconstruction views of the real
// SBI chain leaves, kept here (rather than in <Gruntz/StatusBarMgr.h>) precisely so
// the host class can be included by TUs that carry the REAL chain classes
// (<Gruntz/SBI_Image.h> / <Gruntz/SBI_MenuItem.h>) without a redefinition clash.
//
// @identity-TODO (pre-existing debt, NOT introduced by the split): CSbiRectSub IS the
// real CSBI_RectOnly and the CSBI_MenuItem below IS the real CSBI_MenuItem. The
// CSBI_RectOnly NAME is now free (the 2026-07-12 host split released it), so the only
// thing still blocking the fold onto the real chain is that this TU also carries
// <Gruntz/SbiTabzDialogViews.h>, which defines its OWN `class CSBI_Image` - so pulling
// in the real <Gruntz/SBI_Image.h> here would be a redefinition. Folding THAT view is
// the next step; it is a separate, measured change (SbiTabzDialogViews.h documents the
// base-ctor inline/out-of-line cost), not something to smuggle into the split.
#ifndef GRUNTZ_STATUSBARTABWIDGETS_H
#define GRUNTZ_STATUSBARTABWIDGETS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/StatusBarItem.h>

// (the CSbiTab base is GONE - it was never a type. It was a fabricated stand-in for the
// canonical CStatusBarItem (already included above), and it declared THIRTEEN virtuals
// against a real base of ELEVEN, so cl emitted a 13-slot vtable for both leaves below
// where retail has 11 and 12. That is not a cosmetic defect - every dispatch past the
// divergence point would have gone to the wrong slot - and its 13 declared-only virtuals
// were 13 guaranteed unresolved externals on top.
//
// The remap, read off the real vtables (sema class) rather than guessed:
//   retail CStatusBarItem  vtbl@0x1eabcc  11 slots (the base; all bodies real)
//   retail CSBI_RectOnly   vtbl@0x1eab8c  11 slots  : CStatusBarItem   <- CSbiRectSub
//   retail CSBI_Image      vtbl@0x1eac0c  12 slots  : CSBI_RectOnly  (adds slot 11)
//   retail CSBI_MenuItem   vtbl@0x1eab4c  12 slots  : CSBI_Image      <- CSBI_MenuItem
// So the leaves derive DIRECTLY from CStatusBarItem here (the CSBI_RectOnly/CSBI_Image
// intermediates add no fields, so this is layout-identical), and the ONE extra slot the
// menu item really has - slot 11, CSBI_Image::SetupImage, which this class overrides -
// is declared on the menu item itself with the canonical signature. The old view's
// "Configure" WAS that slot-11 call, and its "Activate" @ slot 12 was pure fabrication:
// retail's deepest vtable here has 12 slots (0..11), and nothing ever called it.
//
// The CSBI_RectOnly/CSBI_Image intermediates cannot be NAMED here: <Gruntz/SBI_Image.h>
// defines a different class under the CSBI_RectOnly name than this header's 0x570 HOST
// does, so the two cannot be co-included. Freeing that name is the documented cross-lane
// host rename (see the note above); it is not needed to get the slot shape right.

// tag-1 rect-only sub-widget. Its TRUE retail class is CSBI_RectOnly (vtable 0x5eab8c),
// but that name is bound in this TU to the big status-bar HOST, so the sub-widget carries
// this placeholder name; MSVC still auto-stamps a real vtable (reloc-masked).
//
// SIZE IS 0x3c, NOT 0x30 - binary-proven at the allocation site rather than inferred from
// the (empty) field list. Retail's CStatusBarMgr::LoadTabSprites does, at 0x10237d:
//     push 0x3c            ; sizeof
//     call 0x1b9b46        ; operator new
//     mov  ecx,eax
//     call 0x1e88          ; -> 0x101fa0 ??0CSBI_RectOnly (stamps ??_7CSBI_RectOnly)
// so CSBI_RectOnly = CStatusBarItem (0x30) + three words. The ctor leaves them
// uninitialised (Setup fills them), which is why the old 0x30 guess looked self-
// consistent: an incomplete ctor does NOT bound an object.
// SIZE IS 0x30, NOT 0x3c - and the ALLOCATION SITE IN THIS TU says so. The three rect
// sub-widgets BuildStatusBarTabs creates are allocated `push 0x30; call ??2@YAPAXI@Z`
// (@0xffe46 / 0xffed5 / 0xfff4b), while the five menu items in the SAME function are
// `push 0x3c` (@0xfffd9 / 0x100071 / 0x100105 / 0x100199 / 0x100275). So the +0x30/+0x34/
// +0x38 trio belongs to the MENU ITEM ONLY - exactly as the old base's own comment said
// ("fields end at +0x30 (the rect-widget size); the menu-item's m_30/m_34/m_38 live in
// CSBI_MenuItem") before it handed them to the rect widget anyway, on the strength of a
// DIFFERENT allocation site (0x10237d, in CStatusBarMgr::LoadTabSprites - which allocates
// a different, larger object). We were over-allocating these by 12 bytes.
class CSbiRectSub : public CStatusBarItem { // TRUE class CSBI_RectOnly, vtable 0x5eab8c
public:
    CSbiRectSub() {
        m_8 = 1;
    }
};
SIZE(CSbiRectSub, 0x30);

// tag-2 menu item (0x3c). vtable 0x5eab4c -> auto-named ??_7CSBI_MenuItem@@6B@.
// The dtor is declared OUT-OF-LINE (no body): an implicit one makes cl5 emit a
// divergent 5-byte COMDAT `??1CSBI_MenuItem@@UAE@XZ = jmp ??1CSbiTab@@UAE@XZ`,
// duplicating the real dtor (SBI_MenuItem.cpp 0x1007d0) and calling a base dtor no
// obj defines (CSbiTab is a view) -> unresolved external at link. Declared-only =>
// the reference binds to the one real body at its retail rva.
class CSBI_MenuItem : public CStatusBarItem {
public:
    CSBI_MenuItem() {
        m_8 = 2;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
    // Retail vtable shape (sema class: vtbl@0x1eab4c, TWELVE slots; overrides 0/1/3/4/5
    // and slot 11) - mirrored EXACTLY from the canonical CSBI_MenuItem in
    // <Gruntz/SBI_MenuItem.h>, so every entry this TU emits binds to the same symbol the
    // canonical does. Declared-only => the references bind to the one real body per rva.
    virtual ~CSBI_MenuItem();         // slot 0  0x1007d0 (SBI_MenuItem.cpp)
    virtual i32 SbiVfunc0() OVERRIDE; // slot 1
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    // slot 11 - the 11-arg image setup CSBI_Image introduces and this class overrides.
    // It is what the old view called "Configure": the tab-configure call site pushes
    // exactly these 11 dwords. Declared with the canonical signature so it mangles to the
    // same ?SetupImage@CSBI_MenuItem@@ symbol (out-of-line body: InitItem @0xe80e0).
    // slot 11 - args 5..8 are ONE by-value SbRect, the same 11 dwords the sibling
    // builder view (SbiTabzDialogViews.h) already spelled correctly.
    virtual i32
    SetupImage(i32 a1, CSbiConfigHost* host, i32 a3, i32 a4, SbiRect rc, i32 key, i32 a10, i32 a11);
    // The tab-widget drivers CSBI_RectOnly reaches through m_tabSprite* (non-virtual
    // reloc-masked call rel32; bodies + rvas bound in SBI_MenuItem.cpp). These fold
    // the former fake CSbiSprite view onto the real class: Release->Blit, Show->
    // SetState, Hide->ProbeState, Configure->ResolveFrame (proven by shared rva).
    i32 ResolveFrame(i32 key, i32 a); // 0xe81e0  (was CSbiSprite::Configure)
    i32 SetState(i32 state, i32 a);   // 0xe8310  (was CSbiSprite::Show)
    i32 ProbeState(i32 state);        // 0xe8480  (was CSbiSprite::Hide)
    i32 Blit();                       // 0xe84f0  (was CSbiSprite::Release)
    i32 m_30;                         // +0x30
    i32 m_34;                         // +0x34
    i32 m_38;                         // +0x38
};
SIZE(CSBI_MenuItem, 0x3c);

#endif // GRUNTZ_STATUSBARTABWIDGETS_H
