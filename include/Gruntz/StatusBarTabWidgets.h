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
#include <Gruntz/SBI_MenuItem.h> // the CANONICAL CSBI_MenuItem : CSBI_Image : CSBI_RectOnly
                                 // : CStatusBarItem chain (pulls StatusBarItem.h)

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
// (CSbiRectSub is GONE - it was CSBI_RectOnly (the real 0x30 game class, vtable
// 0x5eab8c): the two-class split existed only to reproduce retail's INLINED leaf-
// widget ctor at BuildStatusBarTabs vs the OUT-OF-LINE base-subobject ctor 0x101fa0
// in the CSBI_MenuItem chain. One retail class -> one C++ class; the leaf sites now
// call the real ctor (a compiler-inlining artifact we cannot steer, not a 2nd class).
#include <Gruntz/SBI_Image.h> // the canonical CSBI_RectOnly the tab builders `new`

// The tag-2 menu item is the CANONICAL CSBI_MenuItem (<Gruntz/SBI_MenuItem.h>,
// included above): : CSBI_Image : CSBI_RectOnly : CStatusBarItem, vtable 0x5eab4c,
// 12 slots (overrides 0/1/3/4/5/11). The former per-TU view here derived
// CStatusBarItem DIRECTLY (skipping CSBI_RectOnly + CSBI_Image), so its emitted
// ??_7CSBI_MenuItem carried CStatusBarItem's slot-2 Setup (0x443f) where retail has
// CSBI_RectOnly's (0x2a2c) and mangled slot 11 with a void*/i32 SetupImage signature -
// a DIVERGENT COMDAT vs sbi_menuitem/statusbargamemenu. Dissolved onto the canonical
// (2026-07-14); the builders' `new CSBI_MenuItem` sites now call the canonical slot-11
// SetupImage (arg1 CStatusBarMgr*, arg5 SbRect by value, arg6 const char* key).

#endif // GRUNTZ_STATUSBARTABWIDGETS_H
