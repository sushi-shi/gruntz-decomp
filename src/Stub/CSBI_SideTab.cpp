#include <rva.h>
// CSBI_SideTab.cpp - engine-label stubs for CSBI_SideTab.
//
// NOTE: 0x105070 was mislabeled `~CSBI_SideTab` by the rtti-vptr heuristic; the
// real scalar destructor is reconstructed at 0x105200 (src/Gruntz/SBI_SideTabEh.cpp).
// 0x105070 is a factory: it `new`s a 0x5c-byte object, stamps the 0x5eae3c
// (CSBI_SideTab) vptr and initializes the fields - a builder, not the destructor.
// Relabeled `Build` to free the ??1 name for the real dtor. The Build factory
// (0x105070) is reconstructed in src/Gruntz/SBI_SideTabBuild.cpp. Nothing left here.
