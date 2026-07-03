// CGameMenuMgrBuilders.h - the GAMETAB game-menu builder (BuildGameMenu TU,
// StatusBarGameMenu.cpp) shapes: the builder-facet base CSbMenuItem + its concrete
// SBI leaves + the registry factory view + the CGameMenuMgr class. Moved here from
// the per-TU inline defs so each carries a single shared definition (matching-neutral;
// only offsets + code bytes are load-bearing, engine callees are reloc-masked).
//
// BUILDER-FACET VIEW: CSbMenuItem is this TU's builder VIEW of the ONE retail
// CSBI_Image level (Configure lands at vtable slot +0x2c). Its sibling builder views
// (CSbConfigItem in CStatusBarMgr.cpp, CSbDialogItem in SBI_TabzDialogEh.cpp) model
// the SAME level; one MSVC5 spelling emits only one shape (out-of-line vs inline base
// ctor), so they stay renamed apart, NOT merged
// (docs/patterns/gx-frame-outofline-ctor.md).
#ifndef GRUNTZ_CGAMEMENUMGR_BUILDERS_H
#define GRUNTZ_CGAMEMENUMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>           // CPtrList (embedded Game-tab widget list in CGameMenuMgr)
#include <Gruntz/SbRect.h> // the by-value geometry rect each Configure takes (slot 0x2c)

class CGameMenuMgr;

// The GAMETAB widget base. Polymorphic so MSVC emits native __thiscall vtable
// dispatch (call [edx+0x2c] / [edx+0x30]); the concrete CSBI_ImageSet / CSBI_MenuItem
// leaves below auto-stamp the retail vtables. Eleven leading placeholder virtuals line
// Configure up to slot 0x2c. Slot 0 is the scalar-deleting dtor (the fail `delete it`).
// The inline base ctor zeroes the base fields the retail base ctor cleared.
class CSbMenuItem {
public:
    // OUT-OF-LINE ctor (declaration only): retail `new CSBI_X` CALLS the base ctor out
    // of line (call 0x101fa0), so the opaque may-throw call makes cl register the
    // `new`-expression operator-delete-on-ctor-throw cleanup and raise the /GX frame.
    // Folding it inline let cl prove no-throw -> no frame -> 0% (every byte shifted).
    // Reloc-masked call target, so one shared base ctor pairs with retail's per-class
    // ctors. See docs/patterns/gx-frame-outofline-ctor.md.
    CSbMenuItem();
    virtual ~CSbMenuItem(); // +0x00
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual i32 Configure(
        CGameMenuMgr* mgr,
        i32 code,
        i32 type,
        i32 idx,
        SbRect rect,
        const char* key,
        i32 flag,
        i32 e
    );                            // +0x2c
    virtual void Activate(i32 a); // +0x30

    i32 m_enabled; // +0x04  enabled flag (zeroed to disable the widget)
    i32 m_tag;     // +0x08  type tag (2 / 4)
    char pad0c[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    char pad2c[0x30 - 0x2c];
    i32 m_30; // +0x30
    i32 m_34; // +0x34
    i32 m_38; // +0x38
};
SIZE_UNKNOWN(CSbMenuItem);

// The concrete widget leaves. `new CSBI_ImageSet` / `new CSBI_MenuItem` makes MSVC
// auto-stamp the retail ??_7CSBI_ImageSet@@6B@ (0x5eac4c) / ??_7CSBI_MenuItem@@6B@
// (0x5eab4c) vtables (catalogued in config/vtable_names.csv) - no manual stamp. The
// inline ctor sets the per-tag fields the retail mk* helper wrote after the ctor
// (m_tag = tag, m_34 = m_30 = m_38 = 0). Both are 0x3c bytes (the base CSbMenuItem size).
class CSBI_ImageSet : public CSbMenuItem { // vtable 0x5eac4c, tag 4
public:
    CSBI_ImageSet() {
        m_tag = 4;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
};
SIZE(CSBI_ImageSet, 0x3c);
class CSBI_MenuItem : public CSbMenuItem { // vtable 0x5eab4c, tag 2
public:
    CSBI_MenuItem() {
        m_tag = 2;
        m_34 = 0;
        m_30 = 0;
        m_38 = 0;
    }
};
SIZE(CSBI_MenuItem, 0x3c);

// The game registry singleton (?g_gameReg, DATA 0x64556c). Only the fields the
// builder touches are modeled.
struct CGmFactory {
    char m_pad[0x288];
    i32 m_variant; // +0x288  MISSIONSTATUS variant selector
};
SIZE_UNKNOWN(CGmFactory);

// CGameMenuMgr layout (placeholder fields; only offsets are load-bearing).
class CGameMenuMgr {
public:
    void BuildGameMenu(); // 0x101580

    char m_pad00[0xc];
    i32 m_code;  // +0x0c  configure `code` arg
    i32 m_baseX; // +0x10  base x
    i32 m_baseY; // +0x14  base y
    char m_pad18[0xb8 - 0x18];
    CPtrList m_items; // +0xb8  Game-tab widget list (AddTail)
    char m_padd4[0x110 - (0xb8 + sizeof(CPtrList))];
    i32 m_briefingGate; // +0x110  briefing/MISSIONSTATUS gate (==0x1fb)
    char m_pad114[0x1dc - 0x114];
    CSbMenuItem* m_slotResume;   // +0x1dc  RESUME/PAUSE slot
    CSbMenuItem* m_slotLoad;     // +0x1e0  LOAD slot
    CSbMenuItem* m_slotSave;     // +0x1e4  SAVE slot
    CSbMenuItem* m_slotSettings; // +0x1e8  SETTINGS slot
    CSbMenuItem* m_slotHelp;     // +0x1ec  HELP slot
    CSbMenuItem* m_slotQuit;     // +0x1f0  QUIT slot
    char m_pad1f4[0x354 - 0x1f4];
    i32 m_showResume; // +0x354  show-RESUME gate
    char m_pad358[0x558 - 0x358];
    i32 m_558;           // +0x558
    i32 m_destructState; // +0x55c  DESTRUCT-button state
    char m_pad560[0x570 - 0x560];
    CSbMenuItem* m_slotDestruct; // +0x570  DESTRUCT/MISSIONSTATUS slot
};
SIZE_UNKNOWN(CGameMenuMgr);

#endif // GRUNTZ_CGAMEMENUMGR_BUILDERS_H
