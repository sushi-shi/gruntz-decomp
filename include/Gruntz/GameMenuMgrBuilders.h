// GameMenuMgrBuilders.h - the GAMETAB game-menu builder (BuildGameMenu TU,
// StatusBarGameMenu.cpp) shapes: the builder-facet base CSBI_Image + its concrete
// SBI leaves + the registry factory view + the CGameMenuMgr class. Moved here from
// the per-TU inline defs so each carries a single shared definition (matching-neutral;
// only offsets + code bytes are load-bearing, engine callees are reloc-masked).
//
// BUILDER-FACET VIEW: CSBI_Image is this TU's builder VIEW of the ONE retail
// CSBI_Image level (Configure lands at vtable slot +0x2c). Its sibling builder views
// (CSbConfigItem in CStatusBarMgr.cpp, CSbDialogItem in SBI_TabzDialogEh.cpp) model
// the SAME level; one MSVC5 spelling emits only one shape (out-of-line vs inline base
// ctor), so they stay renamed apart, NOT merged
// (docs/patterns/gx-frame-outofline-ctor.md).
//
// FOLD VERDICT (P1): CSBI_MenuItem/CSBI_ImageSet are ONE retail class each (single
// RTTI ??_7 + single vtable, config/vtable_names.csv 0x5eab4c / 0x5eac4c) - view, not
// sibling. Even the two OUT-OF-LINE-ctor facets (this CSBI_Image + Tabz's
// CSbDialogItem, both need the throwing base-ctor `call` for the /GX frame) can NOT
// merge to one base: their slot-0x2c virtual's arg2 differs at the call sites -
// BuildGameMenu passes an `i32 code`, BuildTabzDialog a `TabzSub*` pointer, so no
// single param type is cast-free for both (verified by the call sites). The multi-def
// is an irreducible codegen wall, not lazy duplication.
#ifndef GRUNTZ_CGAMEMENUMGR_BUILDERS_H
#define GRUNTZ_CGAMEMENUMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>           // CPtrList (embedded Game-tab widget list in CGameMenuMgr)
#include <Gruntz/SbRect.h> // the by-value geometry rect each Configure takes (slot 0x2c)

// The FABRICATED HIERARCHY THAT USED TO LIVE HERE (deleted):
//   struct CStatusBarItem {};                 // empty stub
//   struct CSBI_RectOnly : CStatusBarItem {}; // empty stub
//   class  CSBI_Image : CSBI_RectOnly { 13 virtuals };  // 11 of them placeholders
//   class  CSBI_ImageSet : CSBI_Image {};  class CSBI_MenuItem : CSBI_Image {};
//
// Eleven body-less placeholder virtuals padded `Configure` out to slot +0x2c and
// `Activate` to +0x30, so every leaf got a 13-slot (52 B) vtable. RTTI says CSBI_MenuItem
// has TWELVE (48 B): ??_7CSBI_MenuItem@@6B@ was emitted at 52 B here and 48 B by
// sbi_menuitem - one mangled name, two lengths, and MSVC5 keeps only one COMDAT.
// The whole facet was unnecessary: `Configure` IS CSBI_Image::SetupImage (slot 11) and
// `Activate` IS CSBI_ImageSet::SbiSlot12 (slot 12). The builder now uses the canonical
// classes from <Gruntz/SBI_MenuItem.h> / <Gruntz/SBI_ImageSet.h>.
//
// CGameMenuMgr is likewise gone: it IS CStatusBarMgr. Proof, not a guess:
//   1. CStatusBarMgr::LoadTabSprites (0x102250) calls `BuildGameMenu()` UNQUALIFIED - on
//      its own `this` - and CStatusBarMgr::BuildGameMenu was declared at the very RVA
//      (0x101580) that CGameMenuMgr::BuildGameMenu defined. Two mangled names, one
//      function: the caller's reference resolved to NO definition at link (a phantom).
//   2. The view's m_items sat at +0xb8 == CStatusBarMgr::m_tabLists[5] (0x2c + 5*0x1c) -
//      exactly the Game-tab list, which is the list BuildGameMenu appends to.
//   3. Every other field lined up with the canonical, offset for offset: m_code/m_baseX/
//      m_baseY == m_c/m_10/m_rect14.m_0, and +0x110 / +0x1dc..+0x1f0 / +0x354 / +0x558 /
//      +0x55c / +0x570 all already existed on CStatusBarMgr (two independent
//      reconstructions of one object).

// The game registry singleton (?g_gameReg, DATA 0x64556c). Only the fields the
// builder touches are modeled.
struct CGmFactory {
    char m_pad[0x288];
    i32 m_variant; // +0x288  MISSIONSTATUS variant selector
};
SIZE_UNKNOWN(CGmFactory);

// CGameMenuMgr's fields are the canonical CStatusBarMgr's, offset for offset (see the
// identity proof at the top of this file). The view's SEMANTIC names, preserved here
// because the canonical carries placeholders for them:
//   +0x0c m_code=m_c | +0x10 m_baseX=m_10 | +0x14 m_baseY=m_rect14.m_0
//   +0xb8  m_items         = m_tabLists[5]        (the Game-tab list)
//   +0x110 m_briefingGate  = m_itemKind           (== 0x1fb selects the briefing variant)
//   +0x1dc m_slotResume    = m_tabSprite5         +0x1e0 m_slotLoad     = m_tabSprite6
//   +0x1e4 m_slotSave      = m_tabSprite7         +0x1e8 m_slotSettings = m_tabSprite8
//   +0x1ec m_slotHelp      = m_tabSprite9         +0x1f0 m_slotQuit     = m_tabSprite10
//   +0x354 m_showResume    = m_hitTestDisabled    (the two readings DISAGREE - unresolved)
//   +0x558 m_558           = m_destructWarnActive +0x55c m_destructState = m_modeState
//   +0x570 m_slotDestruct  = m_modeNotify         (typed CSbiSlotPtr* - itself a fake view
//          of the SBI item: StatusBarMgr.cpp already casts CSBI_Image* -> CSbiSlotPtr*)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_CGAMEMENUMGR_BUILDERS_H
