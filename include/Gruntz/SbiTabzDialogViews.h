// SbiTabzDialogViews.h - the TABZ-dialog builder (BuildTabzDialog TU,
// SBI_TabzDialogEh.cpp) shapes: the builder-facet base CSbDialogItem + its concrete
// SBI leaves + the singleton/active-level views + the CTabzBuilder host. Moved here
// from the per-TU inline defs so each carries a single shared definition
// (matching-neutral; only offsets + code bytes are load-bearing, engine callees are
// reloc-masked).
//
// BUILDER-FACET VIEW: CSbDialogItem is this TU's builder VIEW of the ONE retail
// CSBI_Image level (Setup lands at vtable slot +0x2c). Its sibling builder views
// (CSbConfigItem in CStatusBarMgr.cpp, CSbMenuItem in StatusBarGameMenu.cpp) model
// the SAME level; this view uses an OUT-OF-LINE base ctor so `new` emits the throwing
// base-ctor call + /GX ctor-in-flight EH frame - cannot merge with CSbConfigItem's
// inline-ctor view (measured-regression wall, docs/patterns/gx-frame-outofline-ctor.md).
//
// FOLD VERDICT (P1): CSBI_Image/CSBI_MenuItem/CSBI_ImageSet are ONE retail class each
// (single RTTI ??_7 + single vtable) - view, not sibling. This OUT-OF-LINE facet also
// cannot merge with the other OUT-OF-LINE one (StatusBarGameMenu's CSbMenuItem): the
// slot-0x2c virtual's arg2 differs at the call sites - BuildTabzDialog passes a
// `TabzSub*` pointer (m_c), BuildGameMenu an `i32 code`, so no single param type is
// cast-free for both. The multi-def is an irreducible codegen wall, not lazy dup.
#ifndef GRUNTZ_SBI_TABZDIALOG_VIEWS_H
#define GRUNTZ_SBI_TABZDIALOG_VIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                 // CPtrList (item list in CTabzBuilder) + RECT
#include <Gruntz/SbRect.h>       // the by-value geometry rect the slot-11 SetupImage takes
#include <Gruntz/SBI_ImageSet.h> // the CANONICAL CSBI_Image / CSBI_ImageSet chain
                                 // (: CSBI_RectOnly : CStatusBarItem) + SetupImage (slot 11)

// The concrete status-bar leaves the tabz-dialog builder `new`s are the CANONICAL
// retail CSBI_Image / CSBI_ImageSet classes (SBI_Image.h / SBI_ImageSet.h). The
// former per-TU views here derived CStatusBarItem DIRECTLY (skipping the real
// CSBI_RectOnly intermediate) and declared only ~dtor + a void*-typed `Setup`, so
// their emitted ??_7CSBI_Image / ??_7CSBI_ImageSet carried CStatusBarItem's inherited
// slot bodies at 1..5 (and an unbound slot 11) - a DIVERGENT COMDAT vs every other
// SBI TU, and for CSBI_ImageSet also the WRONG length (12 slots / 48 B vs the real
// 13 / 52). Dissolved onto the canonicals (2026-07-14): `new CSBI_Image` still emits
// the retail base-ctor call (SBI_ITEM_OWN_CTOR makes CStatusBarItem's ctor out-of-
// line -> `call ??0CStatusBarItem` @0x1005d0 via thunk 0x22c0, RectOnly ctor inlined),
// and `new CSBI_ImageSet` calls ??0CSBI_RectOnly @0x101fa0 (thunk 0x1e88), exactly as
// retail's BuildTabzDialog - the CSBI_RectOnly intermediate is required to reproduce
// that second ctor shape. slot 11 SetupImage takes the geometry rect BY VALUE, and the
// call sites pass an INLINE TEMPORARY (SbRect(...)) so cl builds the struct in place.

// DISSOLVED (2026-07-17): the CSBI_MenuItemDlg view IS the canonical CSBI_MenuItem
// (<Gruntz/SBI_MenuItem.h>) - the follow-on fold this header flagged. Retail settles it:
// every `new` site here stamps ??_7CSBI_MenuItem@@6B@ (0x5eab4c) and inlines exactly the
// view's ctor body, and the shapes were already identical (: CSBI_Image + m_34 + m_38,
// size 0x3c). The view's sole unique knowledge - that inline ctor - has been migrated
// onto the canonical, which was missing it.

// (TabzGmFactory is GONE - the +0x68 active-level object IS the canonical
// CTriggerMgr (m_cmdGrid): its "+0x288 mission-complete selector" is m_phase (the
// PROVEN round phase) and "+0x3ec reason code" is m_3ec. The SPLIT verdict vs the
// m_30 resource manager stands - it was a different class, and that class is
// CTriggerMgr, not a new one.)
// (TabzPlayer is FOLDED: the +0x174-based per-player walk IS g_gameReg->m_options[]
// (GruntzPlayer m_clearedRound/m_joined/m_doneFlag); SBI_TabzDialogEh reads it typed.)


// DISSOLVED (2026-07-17): the CTabzBuilder / TabzSub / TabzRectHolder views are gone -
// all three were fake views of classes we already model, and the builder's own casts
// named every one of them:
//
//   CTabzBuilder    IS CStatusBarMgr     - BuildStatusBarTabs (0xffde0) keeps its `this`
//                                          in edi and calls BuildTabzDialog with it
//                                          unchanged (`mov ecx,edi; call 0x41a1`), so the
//                                          receiver is a CStatusBarMgr. That is why the
//                                          view cast its own `this` to CStatusBarMgr* 16x.
//   TabzSub         IS CDDrawSurfaceMgr  - the canonical CStatusBarMgr already types +0x0c
//                                          as CDDrawSurfaceMgr*, which is exactly what the
//                                          view cast m_c to at every SetupImage arg2.
//   TabzRectHolder  IS CGameLevel        - CDDrawSurfaceMgr::m_level (+0x24), whose
//                                          m_planeCtx (LevelCoordRect, +0x10) is the rect.
//
// Retail proves the whole chain at 0x10a36e: `mov eax,[ebx+0xc]` (m_c) -> `mov eax,[eax+0x24]`
// (m_level) -> `add eax,0x10` (m_planeCtx) -> four dword loads (the 4-int rect copy).
// BuildTabzDialog now lives on CStatusBarMgr in <Gruntz/StatusBarMgr.h>.

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_SBI_TABZDIALOG_VIEWS_H
