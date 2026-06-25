#include <rva.h>
// CBrickz.cpp - remaining engine-label stub for the CBrickz game-object.
//
// The 1-arg ctor (0x10e800) is reconstructed in src/Gruntz/CBrickz.cpp, and the
// grid allocator Ghidra mislabeled "winapi_09ea60_IntersectRect" (0x09ea60) is
// actually a method of the SEPARATE pathfinding container (the placeholder
// "CBrickz" in <Gruntz/Brickz.h>), reconstructed there as CBrickz::AllocGrid.
//
// LoadAttributes (0x0810f0) is the level-load attribute parser - a 2228-byte
// multi-switch bute reader (Brickz/Brown/Black colors, the freelist node recycle,
// two jump tables). DEFERRED to the final sweep (a leaf-first redo); too big to
// converge from a single pass without diverging its regalloc.
#include <Stub/CBrickz.h>

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000810f0, 0x8b4)
void CBrickz::LoadAttributes(i32, i32) {}
