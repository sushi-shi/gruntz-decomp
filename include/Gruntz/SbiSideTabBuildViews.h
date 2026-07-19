// SbiSideTabBuildViews.h - the STATZTAB "Build" factory TU (SBI_SideTabBuild.cpp) shapes:
// the container CStatzTabBuilder + the settings singleton view. The CHILD it builds is the
// canonical CSBI_SideTab (<Gruntz/SBI_SideTab.h>), pulled below.
//
// This header USED to define its own `class CSBI_SideTab` with exactly ONE virtual (the
// dtor) - so cl emitted ??_7CSBI_SideTab@@6B@ at 4 BYTES here, against the 44 B (11-slot)
// vtable RTTI proves and the rest of the tree emits. One mangled name, two lengths; MSVC5
// keeps one COMDAT and discards the other. Had the linker picked the 4-byte one, EVERY
// side-tab virtual call past slot 0 would have dispatched off the end of the table. The
// "two-view split" that justified it ("one MSVC5 spelling emits only one shape") was wrong:
// the canonical class emits the right 11-slot vtable AND takes the builder's inline ctor.
#ifndef GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H
#define GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                // CPtrList (embedded child list in CStatzTabBuilder)
#include <Gruntz/SBI_SideTab.h> // the canonical CSBI_SideTab child (11 slots, vtbl 0x5eae3c)

// The settings/registry singleton (0x64556c); its +0x30 is the level's status-bar
// owner passed as the StatzTab arg2.

// CStatzTabBuilder - the STATZTAB CONTAINER Build runs on (0x105070 was MISLABELED
// ~CSBI_SideTab by the rtti-vptr heuristic; the CSBI_SideTab is the CHILD it builds,
// not this container - see the file header). A gate at +0x00, two geometry-base
// pointers at +0x10/+0x18, the +0x2c child CPtrList, and the parallel +0x114 key /
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
    CPtrList m_2c; // +0x2c  child list (sizeof CPtrList == 0x1c -> ends +0x48)
    char m_pad48[0x114 - 0x48];
    i32 m_114[15];           // +0x114  per-slot key inputs
    CSBI_SideTab* m_150[15]; // +0x150  built child slots
};
SIZE_UNKNOWN(CStatzTabBuilder);

#endif // GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H
