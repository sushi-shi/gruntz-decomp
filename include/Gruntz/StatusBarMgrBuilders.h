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
#include <Mfc.h>           // CPtrList (embedded tab lists in CStatusBarMgr)
#include <Gruntz/SbRect.h> // the by-value geometry rect the Configure virtuals take

class CStatusBarMgr;

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
    void SetState(i32 s);            // thunk 0x11e5  (Multiplayer HEAD-loop state set)
    void ShowFrames(i32 a, i32 b);   // thunk 0x23dd  (Multiplayer HEAD-loop frame set)
    void SetArrowMode(i32 a, i32 b); // Statz arrow: the m_114-gated 2-arg sink (reloc `M`)

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

class CSBI_ImageSet : public CSbConfigItem { // vtable 0x5eac4c, size 0x3c
public:
    CSBI_ImageSet() {
        m_30 = 0;
        m_8 = 4;
        m_34 = 0;
    }
    i32 m_34; // +0x34
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

// The two shredder/machine widgets stamp their retail vtable by ADDRESS (a manual
// vptr store), because their retail ctors inline the whole base (no out-of-line base
// call) - the reloc-masked DIR32 to ??_7... pairs the same as an auto-emitted vtable.
extern void* g_vtblCSBI_GruntMachine;     // 0x5eadbc ??_7CSBI_GruntMachine@@6B@
extern void* g_vtblCSBI_StatzTabGruntBar; // ??_7CSBI_StatzTabGruntBar@@6B@
extern void* g_vtblCSBI_Image;            // 0x5eac0c ??_7CSBI_Image@@6B@

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
        *(void**)this = &g_vtblCSBI_Image;
        m_8 = 3;
        m_30 = 0;
    }
    void* m_vptr; // +0x00
    i32 m_4;      // +0x04
    i32 m_8;      // +0x08 tag 3
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
        *(void**)this = &g_vtblCSBI_GruntMachine;
        m_8 = 9;
        m_34 = 0;
        m_3c = 0;
        m_44 = 0;
        m_30 = 0;
    }
    void* m_vptr; // +0x00
    i32 m_4;      // +0x04
    i32 m_8;      // +0x08 tag 9
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
        *(void**)this = &g_vtblCSBI_StatzTabGruntBar;
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
    void* m_vptr; // +0x00
    i32 m_4;      // +0x04
    i32 m_8;      // +0x08 tag 6
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
class CSbIconSet {
public:
    i32 Probe(i32 a);        // thunk 0x1582 -> FUN_00479b30
    void SetA(i32 v);        // thunk 0x11e5 -> FUN_004eb830
    void SetB(i32 a, i32 b); // thunk 0x23dd -> FUN_004eb740
    void AddRef(i32 v);      // thunk 0x3b98 -> FUN_004ea170
};
SIZE_UNKNOWN(CSbIconSet);

// CStatusBarMgr layout (placeholder fields; only offsets are load-bearing).
class CStatusBarMgr {
public:
    i32 LoadTabSprites();
    void BuildGameMenu(); // 0x101580 (the GAMETAB menu builder; called at the Game-tab tail)

    char m_pad00[0xc];
    i32 m_code; // +0x0c  the Configure `code` arg (== BuildGameMenu m_code)
    i32 m_10;   // +0x10  base x
    i32 m_14;   // +0x14  base y
    char m_pad18[0x48 - 0x18];
    CPtrList m_48; // +0x48  Statz tab list
    CPtrList m_64; // +0x64  Gruntz tab list
    CPtrList m_80; // +0x80  Resource tab list
    CPtrList m_9c; // +0x9c  Multiplayer tab list
    CPtrList m_b8; // +0xb8  Game tab list
    char m_padd4[0x10c - 0xd4];
    i32 m_10c; // +0x10c  current tab selector (1..5)
    // ----- created-widget cache slots (only offsets are load-bearing) -----
    char m_pad110[0x114 - 0x110];
    i32 m_114; // +0x114  Statz smallicon cache base
    char m_pad118[0x204 - 0x118];
    i32 m_204; // +0x204  Gruntz GRUNTOVEN item cache (5, stride 4)
    char m_pad208[0x218 - 0x208];
    i32 m_218; // +0x218  Gruntz OVENZTEXT slot
    i32 m_21c; // +0x21c  Gruntz WELLGOO slot
    char m_pad220[0x224 - 0x220];
    i32 m_224; // +0x224  Gruntz GRUNTOVEN format cache (5, stride 0x18)
    char m_pad228[0x298 - 0x228];
    i32 m_298; // +0x298  Gruntz WELLGOO config-d source
    char m_pad29c[0x2c4 - 0x29c];
    i32 m_2c4; // +0x2c4  Resource BELT config-d source (3, stride 0x18)
    char m_pad2c8[0x2dc - 0x2c8];
    i32 m_2dc; // +0x2dc  (BELT[1])
    char m_pad2e0[0x2f4 - 0x2e0];
    i32 m_2f4; // +0x2f4  (BELT[2])
    char m_pad2f8[0x308 - 0x2f8];
    i32 m_308; // +0x308  Resource BELT item cache (3, stride 4)
    i32 m_30c;
    i32 m_310;
    char m_pad314[0x31c - 0x314];
    i32 m_31c; // +0x31c  Resource MACHINE cross-call idxB source
    char m_pad320[0x334 - 0x320];
    i32 m_334; // +0x334  Resource MACHINE cross-call idxA source
    char m_pad338[0x348 - 0x338];
    i32 m_348; // +0x348  Resource MACHINE item slot
    char m_pad34c[0x364 - 0x34c];
    i32 m_364; // +0x364  Resource MAINBG slot
    i32 m_368; // +0x368  (multiplayer slot base)
    i32 m_36c; // +0x36c  Resource UPPERBG slot
    i32 m_370; // +0x370  Resource WINDOWBG slot
    char m_pad374[0x3dc - 0x374];
    i32 m_3dc; // +0x3dc  Resource SHREDDER config-d array
    char m_pad3e0[0x4a8 - 0x3e0];
    i32 m_4a8; // +0x4a8  Resource SHREDDER item cache
    char m_pad4ac[0x4cc - 0x4ac];
    i32 m_4cc; // +0x4cc  Resource MACHINE config-d source
    char m_pad4d0[0x4e0 - 0x4d0];
    i32 m_4e0; // +0x4e0  Resource MACHINE-shredder item slot
    char m_pad4e4[0x4ec - 0x4e4];
    i32 m_4ec; // +0x4ec  Multiplayer config-d source
    char m_pad4f0[0x500 - 0x4f0];
    i32 m_500; // +0x500  Multiplayer shredder item slot
    i32 m_504; // +0x504  rect-cached x
    i32 m_508; // +0x508  rect-cached y
    i32 m_50c; // +0x50c  rect-cached
    i32 m_510; // +0x510  rect-cached
    i32 m_514; // +0x514  Resource MACHINE rect-cached x
    i32 m_518; // +0x518  rect-cached y
    i32 m_51c; // +0x51c  rect-cached
    i32 m_520; // +0x520  rect-cached
    char m_pad524[0x61c - 0x524];
    i32 m_61c; // +0x61c  Multiplayer player-slot cache (4, stride 4)
    i32 m_620;
    i32 m_624;
    i32 m_628;
    i32 m_62c; // +0x62c  Multiplayer head slot
};
SIZE_UNKNOWN(CStatusBarMgr);

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H
