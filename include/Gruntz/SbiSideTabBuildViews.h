// SbiSideTabBuildViews.h - the STATZTAB "Build" factory TU (SBI_SideTabBuild.cpp)
// shapes: the CTOR/builder view of CSBI_SideTab (the 0x5c child the builder `new`s)
// + the container CStatzTabBuilder + the settings singleton view. Moved here from the
// per-TU inline defs so each carries a single shared definition (matching-neutral;
// the OFFSETS + code bytes are the load-bearing facts).
//
// TWO-VIEW SPLIT: this CSBI_SideTab is the standalone polymorphic CTOR/builder view
// (real virtual dtor at slot 0 -> `new`/`delete` auto-stamp ??_7CSBI_SideTab@@6B@ =
// 0x5eae3c, size 0x5c). The FRAMELESS method view of the SAME retail class lives in
// <Gruntz/SBI_SideTab.h>; one MSVC5 spelling emits only one shape, so the two are
// NEVER co-included (see the two-view-split note atop <Gruntz/StatusBarItem.h>).
#ifndef GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H
#define GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // CObList (embedded child list in CStatzTabBuilder)

// The built status-bar item: a REAL polymorphic CSBI_SideTab child (0x5c bytes).
// `new CSBI_SideTab` (operator new == RezAlloc) makes MSVC auto-stamp the retail
// ??_7CSBI_SideTab@@6B@ vtable (0x5eae3c, catalogued in config/vtable_names.csv) - no
// manual stamp; the virtual dtor at slot 0 is the scalar-deleting dtor the fail-path
// `delete` dispatches. BuildStatzTabStatusBar (0xe9600, reached via ILT 0x33c3)
// populates it. The inline ctor writes the same field init retail's inline ctor did.
class CSBI_SideTab {
public:
    CSBI_SideTab() {
        m_4 = 0;
        m_8 = 0;
        m_24 = 0;
        m_28 = 0;
        m_30 = 0;
        m_34 = 0;
        m_38 = -1;
        m_44 = -1;
    }
    virtual ~CSBI_SideTab(); // slot 0 (scalar-deleting dtor)

    // 0xe9600 (CSBI_SideTab::BuildStatzTabStatusBar), __thiscall ret 13 args.
    i32 BuildStatzTabStatusBar(
        CSBI_SideTab* parent,
        void* statusbar,
        i32 p3,
        i32 p4,
        i32 p5,
        i32 p6,
        i32 p7,
        i32 p8,
        const char* key,
        i32 p10,
        i32 p11,
        i32 p12,
        i32 onLeft
    );

    i32 m_4, m_8; // +0x04, +0x08
    char m_pad0c[0x24 - 0x0c];
    i32 m_24, m_28; // +0x24, +0x28
    char m_pad2c[0x30 - 0x2c];
    i32 m_30, m_34; // +0x30, +0x34
    i32 m_38;       // +0x38
    char m_pad3c[0x44 - 0x3c];
    i32 m_44; // +0x44
    char m_pad48[0x5c - 0x48];
};
SIZE(CSBI_SideTab, 0x5c);

// The settings/registry singleton (0x64556c); its +0x30 is the level's status-bar
// owner passed as the StatzTab arg2.
struct CSbBuildSettings {
    char m_pad00[0x30];
    void* m_world; // +0x30
};
SIZE_UNKNOWN(CSbBuildSettings);

// CStatzTabBuilder - the STATZTAB CONTAINER Build runs on (0x105070 was MISLABELED
// ~CSBI_SideTab by the rtti-vptr heuristic; the CSBI_SideTab is the CHILD it builds,
// not this container - see the file header). A gate at +0x00, two geometry-base
// pointers at +0x10/+0x18, the +0x2c child CObList, and the parallel +0x114 key /
// +0x150 child-slot arrays (15 entries, 0x3c apart).
class CStatzTabBuilder {
public:
    i32 Build(); // 0x105070

    i32 m_0; // +0x00  gate (0 => geometry from m_10, else from m_18)
    char m_pad04[0x10 - 0x04];
    i32 m_10; // +0x10
    char m_pad14[0x18 - 0x14];
    i32 m_18; // +0x18
    char m_pad1c[0x2c - 0x1c];
    CObList m_2c; // +0x2c  child list (sizeof CObList == 0x1c -> ends +0x48)
    char m_pad48[0x114 - 0x48];
    i32 m_114[15];           // +0x114  per-slot key inputs
    CSBI_SideTab* m_150[15]; // +0x150  built child slots
};
SIZE_UNKNOWN(CStatzTabBuilder);

#endif // GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H
