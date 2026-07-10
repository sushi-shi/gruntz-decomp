// SbConfigItem.h - the shared builder-facet base CSbConfigItem (extracted from
// StatusBarMgrBuilders.h so TUs that only need the base - e.g. the merged
// StatusBarTabBuilders TU defining SetDirection/SetDirectionAlt - can include it
// without pulling the per-tab leaf views, whose CSBI_GruntMachine builder view
// still duplicates the canonical <Gruntz/SBI_GruntMachine.h> class).
#ifndef GRUNTZ_SBCONFIGITEM_H
#define GRUNTZ_SBCONFIGITEM_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SbRect.h>       // the by-value geometry rect the Configure virtuals take
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr (Configure/ConfigureEx sigs)

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

#endif // GRUNTZ_SBCONFIGITEM_H
