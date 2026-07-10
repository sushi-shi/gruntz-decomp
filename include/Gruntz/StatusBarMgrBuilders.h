// StatusBarMgrBuilders.h - the CStatusBarMgr per-tab builder (LoadTabSprites TU,
// CStatusBarMgr.cpp) shapes: the builder-facet base CSbConfigItem + its concrete
// SBI leaves + the icon/factory helpers + the CStatusBarMgr class. Moved here from
// the per-TU inline defs so each carries a single shared definition (matching-neutral;
// only offsets + code bytes are load-bearing, engine callees are reloc-masked).
//
// BUILDER-FACET VIEW: CSbConfigItem is the per-TU builder VIEW of the ONE retail
// CSBI_Image level (Configure lands at vtable slot +0x2c = CSBI_Image's own virtual).
// Its two sibling builder views model the SAME level with the SAME slot -
// CSbDialogItem (SBI_TabzDialogEh.cpp) and CSbMenuItem (StatusBarGameMenu.cpp) - but
// with an OUT-OF-LINE base ctor; one MSVC5 spelling emits only one shape so they stay
// renamed apart, NOT merged (measured-regression wall,
// docs/patterns/gx-frame-outofline-ctor.md).
#ifndef GRUNTZ_CSTATUSBARMGR_BUILDERS_H
#define GRUNTZ_CSTATUSBARMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                 // CPtrList (embedded tab lists in CStatusBarMgr)
#include <Gruntz/SbRect.h>       // the by-value geometry rect the Configure virtuals take
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr (moved out of this header)

// CSbConfigItem is the shared base "view": polymorphic so MSVC emits native
// __thiscall vtable dispatch (call [edx+0x2c] etc.). The eleven leading placeholder
// virtuals line Configure up to slot +0x2c and ConfigureEx to +0x34; the deleting
// dtor is slot 0. Each concrete tab widget is a REAL derived class (CSBI_Image /
// CSBI_ImageSet / CSBI_WellGoo below), so `new CSBI_Image` makes cl auto-stamp the
// retail ??_7CSBI_Image@@6B@ vtable - no manual vtable stamp. The inline base ctor
// zeroes the base fields the retail ??0CStatusBarItem (0x1005d0) cleared.
class CSbConfigItem {
public:
    // OUT-OF-LINE ctor (declaration only): retail `new CSBI_X` CALLS the base ctor out
    // of line (call 0x22c0/0x1e88 = CStatusBarItem/CSBI_RectOnly), so the opaque may-throw
    // call makes cl register the `new`-expression operator-delete-on-ctor-throw cleanup
    // and raise the /GX frame with retail's Order-A (this->esi) prologue - once the WHOLE
    // 7629 B LoadTabSprites body is reconstructed (matching-neutral call target, reloc-
    // masked, so one shared base ctor pairs with retail's per-class ctors). Proven on the
    // COMPLETE sibling CGameMenuMgr::BuildGameMenu (0x101580, 37%->63%). See
    // docs/patterns/gx-frame-outofline-ctor.md.
#ifdef CSBCONFIGITEM_OUTOFLINE_CTOR
    CSbConfigItem();
#else
    CSbConfigItem() {
        m_4 = 0;
        m_24 = 0;
        m_28 = 0;
    }
#endif
    virtual ~CSbConfigItem();   // +0x00 (scalar deleting dtor)
    virtual void Serialize();   // +0x04 (slot 1)
    virtual void Setup();       // +0x08 (slot 2)
    virtual void ClearFrame();  // +0x0c (slot 3)
    virtual void Poll();        // +0x10 (slot 4)
    virtual void Tick();        // +0x14 (slot 5)
    virtual void HitHandlerA(); // +0x18 (slot 6)
    virtual void HitHandlerB(); // +0x1c (slot 7)
    virtual void HitHandlerC(); // +0x20 (slot 8)
    virtual void HitHandlerD(); // +0x24 (slot 9)
    virtual void Refresh();     // +0x28 (slot 10)
    virtual i32 Configure(
        CStatusBarMgr* mgr,
        i32 a,
        i32 b,
        i32 c,
        SbRect rect,
        const char* key,
        i32 d,
        i32 e
    );                      // +0x2c
    virtual void s30_pad(); // +0x30 (filler between Configure and ConfigureEx)
    virtual i32 ConfigureEx(
        CStatusBarMgr* mgr,
        i32 a,
        i32 b,
        i32 c,
        SbRect rect,
        const char* key,
        i32 d,
        i32 e,
        i32 f,
        i32 g,
        i32 h
    );                                                             // +0x34
    virtual void ApplyDir(i32 p0, i32 p1, i32 p2, i32 p3, i32 p4); // +0x38 (SetDirection sink)

    // SetDirection (0xea0f0): pick one of four direction tuples from the two
    // boolean selectors and forward to the +0x38 virtual.
    void SetDirection(i32 a, i32 b); // 0x0ea0f0
    // SetDirectionAlt (0xea170): the mirror sibling of SetDirection - the same four
    // ApplyDir tuples, re-keyed on (a1,a2). @orphan `this==ebp` sub-object of
    // LoadTabSprites; RTTI-confirmed CSbConfigItem (slots 0-13 + ApplyDir @+0x38).
    void SetDirectionAlt(i32 a1, i32 a2); // 0x0ea170
    void SetState(i32 s);                 // thunk 0x11e5  (Multiplayer HEAD-loop state set)
    void ShowFrames(i32 a, i32 b);        // thunk 0x23dd  (Multiplayer HEAD-loop frame set)
    void SetArrowMode(i32 a, i32 b);      // Statz arrow: the m_114-gated 2-arg sink (reloc `M`)

    // Cross-tab builders invoked on freshly-created items (reloc-masked rel32 calls;
    // the real bodies live in StatusBarTabBuilders.cpp). `statusbar` is the mgr's
    // saved owner pointer (`code`); `g` is the by-value geometry rect.
    i32 BuildResourceTabStatusBar(
        CStatusBarMgr* parent,
        i32 statusbar,
        i32 p3,
        i32 p4,
        SbRect g,
        const char* key,
        i32 idxA,
        i32 idxB
    ); // 0xe8a70
    i32 BuildMultiplayerTabStatusBar(
        CStatusBarMgr* parent,
        i32 statusbar,
        i32 p3,
        i32 p4,
        SbRect g,
        const char* key,
        i32 p10,
        i32 p11,
        i32 selMode
    ); // 0xea1f0

    i32 m_4; // +0x04
    i32 m_8; // +0x08 type tag (3/4/5/6/7/8/9/0xb)
    char pad[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c  Setup id (filled by Configure)
    i32 m_30; // +0x30
};
SIZE_UNKNOWN(CSbConfigItem);

// The concrete tab-widget subclasses. `new CSBI_Image` makes MSVC auto-stamp the
// retail ??_7CSBI_Image@@6B@ vtable (0x5eac0c) - the vtable-name catalog in
// config/vtable_names.csv names it on the target side (reloc-masked). The inline
// ctor sets the per-tag fields the retail Construct wrote after the base ctor
// (m_8 = tag, m_30 = 0). Trailing padding pins each operator-new size to retail.
class CSBI_Image : public CSbConfigItem { // vtable 0x5eac0c, size 0x34
public:
    CSBI_Image() {
        m_8 = 3;
        m_30 = 0;
    }
};
SIZE(CSBI_Image, 0x34);

class CImageSet; // the icon-set the GRUNTOVEN item drives (SetAllTypes/SetAllFormats)
class CSBI_ImageSet : public CSbConfigItem { // vtable 0x5eac4c, size 0x3c
public:
    CSBI_ImageSet() {
        m_30 = 0;
        m_8 = 4;
        m_34 = 0;
    }
    CImageSet* m_34; // +0x34  owned/resolved icon-set
    char _pad38[0x3c - 0x38];
};
SIZE(CSBI_ImageSet, 0x3c);

class CSBI_WellGoo : public CSbConfigItem { // vtable 0x5eadfc, size 0x6c
public:
    CSBI_WellGoo() {
        m_8 = 7;
        m_30 = 0;
    }
    char _pad34[0x6c - 0x34];
};
SIZE(CSBI_WellGoo, 0x6c);

class CSBI_WarlordHead
    : public CSbConfigItem { // vtable 0x5ead24, tag 0xb, size 0x40 (multiplayer HEAD)
public:
    CSBI_WarlordHead() {
        m_30 = 0;
        m_8 = 0xb;
        m_34 = 0;
    }
    i32 m_34; // +0x34
    char _pad38[0x40 - 0x38];
};
SIZE(CSBI_WarlordHead, 0x40);

// CSBI_ImageSetAni (Resource SHREDDER conveyor): out-of-line base ctor + own vtable;
// tag 8, m_3c seeded 0x64. Configured via ConfigureEx (slot +0x34).
class CSBI_ImageSetAni : public CSbConfigItem { // vtable 0x5eae3c, size 0x54
public:
    CSBI_ImageSetAni() {
        m_30 = 0;
        m_8 = 8;
        m_34 = 0;
        m_44 = 0;
        m_3c = 0x64;
    }
    i32 m_34; // +0x34
    char _pad38[0x3c - 0x38];
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    char _pad48[0x54 - 0x48];
};
SIZE(CSBI_ImageSetAni, 0x54);

// CSBI_StatzTabArrow (Statz tab arrows): out-of-line base ctor + own vtable; tag 5,
// m_3c seeded 0x64. Configured via ConfigureEx (slot +0x34) then SetDirection/M.
class CSBI_StatzTabArrow : public CSbConfigItem { // vtable 0x5eaebc, size 0x54
public:
    CSBI_StatzTabArrow() {
        m_30 = 0;
        m_34 = 0;
        m_44 = 0;
        m_3c = 0x64;
        m_8 = 5;
    }
    i32 m_34; // +0x34
    char _pad38[0x3c - 0x38];
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    char _pad48[0x54 - 0x48];
};
SIZE(CSBI_StatzTabArrow, 0x54);
VTBL(CSBI_StatzTabArrow, 0x001eac94); // vtable_names -> code (RTTI game class)

// The two shredder/machine widgets stamp their retail vtable by ADDRESS (a manual
// vptr store), because their retail ctors inline the whole base (no out-of-line base
// call) - the reloc-masked DIR32 to ??_7... pairs the same as an auto-emitted vtable.

// A CSBI_Image whose retail ctor is INLINED at the call site (manual vptr, no base
// call) - one Resource-tab machine-background item is built this way (a per-site MSVC
// inlining choice that the out-of-line-base CSBI_Image cannot reproduce). Same layout
// / vtable as CSBI_Image; the manual stamp matches the inline codegen.
class CSBI_ImageInline {
public:
    CSBI_ImageInline() {
        m_4 = 0;
        m_24 = 0;
        m_28 = 0;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
        m_8 = 3;
        m_30 = 0;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08 tag 3
    char _pad0c[0x24 - 0x0c];
    i32 m_24, m_28; // +0x24,+0x28
    char _pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
};
SIZE(CSBI_ImageInline, 0x34);

// CSBI_GruntMachine (Resource MACHINE): retail ctor inlines the base (manual vptr),
// tag 9. Built through BuildResourceTabStatusBar. size 0x48.
class CSBI_GruntMachine {
public:
    CSBI_GruntMachine() {
        m_4 = 0;
        m_24 = 0;
        m_28 = 0;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
        m_8 = 9;
        m_34 = 0;
        m_3c = 0;
        m_44 = 0;
        m_30 = 0;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08 tag 9
    char _pad0c[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    char _pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    char _pad38[0x3c - 0x38];
    i32 m_3c; // +0x3c
    char _pad40[0x44 - 0x40];
    i32 m_44; // +0x44
};
SIZE(CSBI_GruntMachine, 0x48);

// CSBI_StatzTabGruntBar (Multiplayer SMALLICONZ + Statz per-grunt bar): retail ctor
// inlines the base (manual vptr), tag 6; four fields (m_38/m_44/m_50/m_70) seed -1.
// Built through BuildMultiplayerTabStatusBar. size 0x88.
class CSBI_StatzTabGruntBar {
public:
    CSBI_StatzTabGruntBar() {
        m_4 = 0;
        m_24 = 0;
        m_28 = 0;
        m_78 = 0;
        m_80 = 0;
        m_7c = 0;
        m_84 = 0;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
        m_8 = 6;
        m_34 = 0;
        m_40 = 0;
        m_4c = 0;
        m_58 = 0;
        m_74 = 0;
        m_30 = 0;
        m_3c = 0;
        m_48 = 0;
        m_54 = 0;
        m_50 = -1;
        m_44 = -1;
        m_38 = -1;
        m_5c = 0;
        m_68 = 0;
        m_70 = -1;
        m_6c = 0;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08 tag 6
    char _pad0c[0x24 - 0x0c];
    i32 m_24, m_28; // +0x24,+0x28
    char _pad2c[0x30 - 0x2c];
    i32 m_30, m_34, m_38, m_3c, m_40, m_44, m_48, m_4c, m_50, m_54, m_58, m_5c; // +0x30..+0x5c
    char _pad60[0x68 - 0x60];
    i32 m_68, m_6c, m_70, m_74, m_78, m_7c, m_80, m_84; // +0x68..+0x84
};
SIZE(CSBI_StatzTabGruntBar, 0x88);

// The shared item helpers driven on a freshly created icon-set item.
class CSbItemHelp {
public:
};
SIZE_UNKNOWN(CSbItemHelp);

// The icon/sprite factory the resource/game tabs pull chip + warpstone sprites
// from (g_gameReg.m_74 / m_68); __thiscall on the factory.
class CSbFactory {
public:
    void* GetByIndex(i32 idx, i32 z); // thunk 0x4165 -> FUN_004e23c0
};
SIZE_UNKNOWN(CSbFactory);
// The m_cmdGrid probed here is a CTriggerMgr; Probe @0x79b30 is CTriggerMgr::ByteTableHas.
// (SetA/SetB/AddRef were unused facet decls -> deleted.) Cast at each Probe call.
class CSbIconSet {};
SIZE_UNKNOWN(CSbIconSet);

// CStatusBarMgr moved to its natural home <Gruntz/StatusBarMgr.h> (pulled at the
// top of this header) so non-builder TUs (SBI_MenuItem.cpp LoadTabSprites) can see
// the manager without this header's builder-facet SBI leaf views (which clash with
// the canonical chain classes in <Gruntz/SBI_Image.h>).

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H
