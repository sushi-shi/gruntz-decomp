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
#include <Mfc.h>           // CObList (item list in CTabzBuilder) + RECT
#include <Gruntz/SbRect.h>       // the by-value geometry rect the slot-11 Setup takes
#include <Gruntz/StatusBarItem.h> // the REAL base (was the CSbDialogItem fabrication)

// The status-bar item family: a REAL polymorphic base (CSbDialogItem) whose
// concrete leaves are the retail CSBI_* classes. `new CSBI_Image` makes MSVC
// auto-stamp the retail ??_7CSBI_Image@@6B@ vtable (0x5eac0c, catalogued in
// config/vtable_names.csv) - no manual stamp. The base ctor is OUT-OF-LINE
// (declared-only) so `new` emits the retail throwing base-ctor `call` + the /GX
// ctor-in-flight EH frame; the derived ctors are inline (the tag + field clears
// that land at each new-site). The virtual dtor (slot 0) is the scalar-deleting dtor
// the fail-path `delete` dispatches; Setup is slot 0x2c (`call [edx+0x2c]`).
// (the CSbDialogItem base is GONE - it was never a type. Like the CSbiTab fabrication next
// door, it was a stand-in for the canonical CStatusBarItem (<Gruntz/StatusBarItem.h>), and
// its 13 declared-only members were 13 guaranteed unresolved externals, all referenced from
// this one unit. Its slots mapped 1:1 onto the real base -
//     ~dtor / Serialize==SbiVfunc0 / SetupRect==Setup(slot 2) / ClearFrame==SbiSlot3 /
//     Poll==SbiSlot4 / Tick==SbiSlot5 / HitHandlerA..D==SbiSlot6..9 / Refresh==SetSubtype
// - and unlike CSbiTab its slot COUNT was already right (12 = retail's CSBI_Image), so
// there was no wrong-dispatch bug here, only the unresolved symbols. The leaves below now
// derive DIRECTLY from CStatusBarItem (the CSBI_RectOnly intermediate adds no fields, so
// this is layout-identical) and CSBI_Image declares the ONE extra slot it really has:
// slot 11, the image setup.
//
// NOTE: this header cannot name the real CSBI_RectOnly/CSBI_Image chain from
// <Gruntz/SBI_Image.h>, because the TU that includes this (the merged SBI_RectOnly.cpp)
// also defines a DIFFERENT class under the CSBI_RectOnly name - the 0x630 non-polymorphic
// host. That is the documented split, not a fold I can do from here.
//
// THE FABRICATION WAS BUYING SOMETHING, AND THIS IS WHAT IT COST TO STOP. CSbDialogItem
// declared its base ctor OUT-OF-LINE, so `new CSBI_Image` emitted a base-ctor CALL - which
// is what retail's BuildTabzDialog does (`mov ecx,eax; call ??0CStatusBarItem@@QAE@XZ`
// @0x1005d0 via thunk 0x22c0, THEN the ??_7CSBI_Image stamp). CStatusBarItem's ctor is
// INLINE, so folding onto it inlines that ctor and costs BuildTabzDialog 83.7% -> 75.4%.
// StatusBarItem.h has an SBI_ITEM_OWN_CTOR knob that makes the base ctor out-of-line, and
// it DOES restore BuildTabzDialog to 83.7% - but the knob is per-TU, and the same merged TU
// also builds the rect widgets, where retail INLINES the base ctor. Turning it on craters
// ??0CSBI_RectOnly (0x101fa0) from 100% to 25.5% and BuildStatusBarTabs to 67%. Measured
// both ways; kept the one that preserves a 100% function and the higher exact count.
// The real fix is to UN-MERGE this TU (BuildTabzDialog was its own obj, SBI_TabzDialogEh.cpp,
// before the wave1-E one-TU merge) - then each new-site gets its own retail ctor spelling.
// Until then the 8.3% is the honest price of not shipping 13 unresolved externals.)

// tag 3 image item: m_8=3, clear m_30.  vtable 0x5eac0c (12 slots).
// The dtor is declared OUT-OF-LINE (no body) so it binds to the one real body at its rva
// (SBI_Image.cpp 0x100870) instead of cl5 synthesising a divergent COMDAT copy.
class CSBI_Image : public CStatusBarItem {
public:
    CSBI_Image() {
        m_8 = 3;
        m_30 = 0;
    }
    virtual ~CSBI_Image(); // slot 0  0x00100870 (SBI_Image.cpp)
    // slot 11 - the image setup (`call [edx+0x2c]`). CStatusBarItem ends at slot 10, so
    // this new virtual lands at 11, exactly where retail's CSBI_Image vtable has it. It
    // takes the geometry rect BY VALUE, and the call sites pass an INLINE TEMPORARY - the
    // spelling that makes cl build the struct directly in the outgoing arg frame
    // (`sub esp,0x10; mov ecx,esp; ...`), which is what retail emits. This header had that
    // right all along; it is what unblocked slot 2 in StatusBarItem.h.
    virtual i32 Setup(
        void* mgr,
        void* sub,
        i32 type,
        i32 idx,
        SbRect rc,
        const char* key,
        i32 flag,
        i32 e
    );

    i32 m_30; // +0x30
}; // size 0x34
SIZE(CSBI_Image, 0x34);

// tag 2 menu item: m_8=2, clear m_34/m_30/m_38.  vtable 0x5eab4c.
// == retail CSBI_MenuItem; renamed apart (Dlg suffix) because the merged
// SBI_RectOnly TU (which hosts BuildTabzDialog since the wave1-E one-TU merge)
// already defines the CSbiTab-based CSBI_MenuItem view (SBI_RectOnly.h) - a
// redefinition-merge TODO; until unified the emitted ??_7CSBI_MenuItemDlg vtable
// name mismatches the target's ??_7CSBI_MenuItem (reloc-masked).
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

// tag 4 image-set item: clear m_30, m_8=4, clear m_34.  vtable 0x5eac4c
class CSBI_ImageSet : public CSBI_Image {
public:
    CSBI_ImageSet() {
        m_30 = 0;
        m_8 = 4;
        m_34 = 0;
    }
    virtual ~CSBI_ImageSet(); // 0x00102000 (SBI_ImageSet.cpp) - see CSBI_Image above
    i32 m_34;                 // +0x34
    i32 m_38;                 // +0x38
}; // size 0x3c
SIZE(CSBI_ImageSet, 0x3c);

// The g_gameReg singleton chain (DATA 0x64556c, RVA 0x24556c). Only the fields
// this builder reads are modeled: the mission-complete selector (m_68->m_288), the
// reason code (m_68->m_3ec), the test-mode gate (m_134) and the 4-player active
// table at +0x174 (stride 0x238).
//
// SPLIT VERDICT (conflation check vs the g_gameReg->m_world resource manager, CResMgr):
// TabzGmFactory is reached through TabzGameReg::m_68 (+0x68), NOT +0x30, and reads
// FAR-out fields (+0x288 mission-complete selector, +0x3ec reason code) well beyond
// CResMgr's ~0x30 span. It is the g_gameReg->m_68 ACTIVE-LEVEL / mission object (the
// same +0x68 object SBI_RectOnly models as CSbiActiveObj, m_288 == its MISSIONSTATUS
// selector), a genuinely DIFFERENT class from the m_30 resource manager. It is
// therefore kept DISTINCT (NOT folded into CResMgr) - the name "TabzGmFactory" is a
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
    CObList m_d4; // +0xd4  item list (AddTail)
    char _padd4[0x1f4 - (0xd4 + sizeof(CObList))];
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
