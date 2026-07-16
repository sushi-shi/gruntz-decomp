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

// tag 2 menu item: m_8=2, clear m_34/m_30/m_38.  vtable 0x5eab4c.
// == retail CSBI_MenuItem; kept under a distinct name (Dlg suffix) so its emitted
// ??_7 does not collide with the CSBI_MenuItem canonical (SBI_MenuItem.h) until that
// leaf's slot-0/1/3/4/5 overrides are wired here (a follow-on fold, not a DIVERGENT:
// this name is emitted only in this TU, reloc-masked against retail's 0x5eab4c stamp).
class CSBI_MenuItemDlg : public CSBI_Image {
public:
    CSBI_MenuItemDlg() {
        m_8 = 2;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
    i32 m_34; // +0x34
    i32 m_38; // +0x38
}; // size 0x3c
SIZE(CSBI_MenuItemDlg, 0x3c);

// The g_gameReg singleton chain (DATA 0x64556c, RVA 0x24556c). Only the fields
// this builder reads are modeled: the mission-complete selector (m_68->m_288), the
// reason code (m_68->m_3ec), the test-mode gate (m_134) and the 4-player active
// table at +0x174 (stride 0x238).
//
// SPLIT VERDICT (conflation check vs the g_gameReg->m_world resource manager, CDDrawSurfaceMgr):
// TabzGmFactory is reached through TabzGameReg::m_68 (+0x68), NOT +0x30, and reads
// FAR-out fields (+0x288 mission-complete selector, +0x3ec reason code) well beyond
// CDDrawSurfaceMgr's ~0x30 span. It is the g_gameReg->m_68 ACTIVE-LEVEL / mission object (the
// same +0x68 object SBI_RectOnly models as CSbiActiveObj, m_288 == its MISSIONSTATUS
// selector), a genuinely DIFFERENT class from the m_30 resource manager. It is
// therefore kept DISTINCT (NOT folded into CDDrawSurfaceMgr) - the name "TabzGmFactory" is a
// placeholder for that +0x68 active-level object, not a resource/game-manager factory.
struct TabzGmFactory {
    char _00[0x288];
    i32 m_288; // +0x288  mission-complete selector
    char _28c[0x3ec - 0x28c];
    i32 m_3ec; // +0x3ec  reason code
};
SIZE_UNKNOWN(TabzGmFactory);
struct TabzPlayer {
    i32 m_174; // rel +0x00 (abs +0x174)
    i32 m_178; // rel +0x04
    i32 m_17c; // rel +0x08
    char _pad[0x238 - 0xc];
};
SIZE_UNKNOWN(TabzPlayer);

// The host sub-object at +0xc: a two-hop RECT holder (m_c->m_24 + 0x10 = RECT).
struct TabzRectHolder {
    char _00[0x10];
    RECT m_10; // +0x10
};
SIZE_UNKNOWN(TabzRectHolder);
struct TabzSub {
    char _00[0x24];
    TabzRectHolder* m_24; // +0x24
};
SIZE_UNKNOWN(TabzSub);

// The builder host (a CSBI_RectOnly-family status bar). Placeholder fields; only
// the offsets are load-bearing.
class CTabzBuilder {
public:
    i32 BuildTabzDialog(); // 0x10a340

    char _00[0x0c];
    TabzSub* m_c; // +0x0c
    char _10[0xd4 - 0x10];
    CPtrList m_d4; // +0xd4  item list (AddTail)
    char _padd4[0x1f4 - (0xd4 + sizeof(CPtrList))];
    CSBI_Image* m_1f4; // +0x1f4
    CSBI_Image* m_1f8; // +0x1f8
    CSBI_Image* m_1fc; // +0x1fc
    CSBI_Image* m_200; // +0x200
    char _204[0x550 - 0x204];
    i32 m_550; // +0x550  active gate
    i32 m_554; // +0x554  confirm-dialog selector
    char _558[0x578 - 0x558];
    i32 m_578; // +0x578  observe/statz-only flag
};
SIZE_UNKNOWN(CTabzBuilder);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_SBI_TABZDIALOG_VIEWS_H
