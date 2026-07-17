// ScrollState.h - the map auto-scroll state block at 0x64cfb0 (owner TU
// src/Gruntz/MgrAutoScroll.cpp). Declared here so the scroll consumers
// (CmdScrollApply, MapLogic's scroll-state serializer) reference these from
// this owner header instead of per-TU externs. DATA homes live in MgrAutoScroll.cpp.
#ifndef GRUNTZ_GRUNTZ_SCROLLSTATE_H
#define GRUNTZ_GRUNTZ_SCROLLSTATE_H

#include <Ints.h>

// The block, in address order. Every slot is a REAL, separately-referenced global -
// MapSerializeCurve (0x0ec230) streams the whole run field by field, and each of its
// operands is an absolute .data reference to one of these, not an offset from the head.
// (It used to spell them `(char*)&g_scrollAccum + 0xNN`, which both reached these
// globals behind pointer arithmetic and hid the references from reloc-fidelity.)
extern i64 g_scrollAccum; // 0x64cfb0 (64-bit scroll accumulator; serialized block head)
extern i64 g_scrollLimit; // 0x64cfb8 (+0x08)  UpdateMgrScroll's threshold
extern u32 g_scrollClock; // 0x64cfc0 (+0x10)
extern u32 g_scrollTimer; // 0x64cfc4 (+0x14)  Cmd_ResetScroll / UpdateMgrScroll
// +0x18 / +0x1c (0x64cfc8 / 0x64cfcc) are DELIBERATELY NOT declared here. They are real
// globals - each has 2 absolute .text references - but BOTH references of each are
// MapSerializeCurve's own read and write paths (sites 0xec2b5/0xec311 and
// 0xec2c3/0xec31f). Nothing else in the binary touches them, so there is no usage to
// name them from: they round-trip through the save file and no runtime code reads them.
// The full chase is done (xref by absolute reference, attributed to the owning function
// via symbol_names) and it dead-ends; naming them would be fabrication, so the
// serializer keeps two documented offset-casts rather than assert a meaning.
// @identity-TODO: 0x64cfc8 / 0x64cfcc - serializer-only scroll-state dwords.
extern i32 g_lastScrollX; // 0x64cfd0 (+0x20)  last-frame scroll position
extern i32 g_lastScrollY; // 0x64cfd4 (+0x24)

#endif // GRUNTZ_GRUNTZ_SCROLLSTATE_H
