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

// (CStatzTabBuilder is GONE - the "container" was CStatusBarMgr itself: the +0x00
// gate is m_position (the status-bar SIDE selector - it picks the geometry base!),
// +0x10/+0x18 are m_10/m_rect14.m_4, +0x2c is m_tabLists[0], +0x114 m_statFlags,
// +0x150 m_hitRects. Build @0x105070 is CStatusBarMgr::BuildSideTabs.)

#endif // GRUNTZ_SBI_SIDETAB_BUILD_VIEWS_H
